#include "server.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <array>

namespace luaweb {

Server::Server(int port) 
    : port_(port), server_fd_(-1), running_(false), web_lua_enabled_(false) {
    // Ignore SIGPIPE to prevent crashes on broken connections
    signal(SIGPIPE, SIG_IGN);
}

Server::~Server() {
    stop();
}

void Server::route(const std::string& method, const std::string& path, RouteHandler handler) {
    Route r;
    r.method = method;
    r.pattern = path;
    
    auto [regex, param_names] = compile_pattern(path);
    r.regex = regex;
    r.param_names = param_names;
    r.handler = handler;
    
    routes_.push_back(std::move(r));
}

void Server::get(const std::string& path, RouteHandler handler) {
    route("GET", path, handler);
}

void Server::post(const std::string& path, RouteHandler handler) {
    route("POST", path, handler);
}

void Server::put(const std::string& path, RouteHandler handler) {
    route("PUT", path, handler);
}

void Server::del(const std::string& path, RouteHandler handler) {
    route("DELETE", path, handler);
}

void Server::use(MiddlewareHandler handler) {
    middlewares_.push_back(handler);
}

void Server::serve_static(const std::string& url_prefix, const std::string& directory) {
    StaticMount mount;
    mount.url_prefix = url_prefix;
    // Ensure directory doesn't have trailing slash
    mount.directory = directory;
    if (!mount.directory.empty() && mount.directory.back() == '/') {
        mount.directory.pop_back();
    }
    static_mounts_.push_back(mount);
}

void Server::enable_web_lua(bool enabled) {
    web_lua_enabled_ = enabled;
    
    if (enabled) {
        // Register the web lua endpoint
        post("/lua/run", [this](Request& req, Response& res) {
            handle_web_lua(req, res);
        });
    }
}

std::pair<std::regex, std::vector<std::string>> Server::compile_pattern(const std::string& pattern) {
    std::vector<std::string> param_names;
    std::string regex_str = "^";
    
    size_t i = 0;
    while (i < pattern.size()) {
        if (pattern[i] == ':') {
            // Extract parameter name
            size_t start = i + 1;
            while (i + 1 < pattern.size() && pattern[i + 1] != '/') {
                ++i;
            }
            std::string param_name = pattern.substr(start, i - start + 1);
            param_names.push_back(param_name);
            regex_str += "([^/]+)";
            ++i;
        } else if (pattern[i] == '*') {
            // Wildcard
            regex_str += ".*";
            ++i;
        } else {
            // Escape regex special characters
            if (std::string(".[{}()*+?|^$\\").find(pattern[i]) != std::string::npos) {
                regex_str += '\\';
            }
            regex_str += pattern[i];
            ++i;
        }
    }
    
    regex_str += "$";
    
    return {std::regex(regex_str), param_names};
}

bool Server::match_route(const Route& route, const std::string& path, 
                         std::unordered_map<std::string, std::string>& params) {
    std::smatch match;
    if (std::regex_match(path, match, route.regex)) {
        // Extract parameters
        for (size_t i = 0; i < route.param_names.size() && i + 1 < match.size(); ++i) {
            params[route.param_names[i]] = match[i + 1].str();
        }
        return true;
    }
    return false;
}

bool Server::bind_socket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[LuaWeb] Failed to create socket" << std::endl;
        return false;
    }
    
    // Allow address reuse
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[LuaWeb] Failed to bind to port " << port_ << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    if (listen(server_fd_, 128) < 0) {
        std::cerr << "[LuaWeb] Failed to listen" << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    return true;
}

bool Server::run() {
    if (!bind_socket()) {
        return false;
    }
    
    running_ = true;
    std::cout << "[LuaWeb] Server running on http://localhost:" << port_ << std::endl;
    
    while (running_) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd_, &read_fds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(server_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (activity < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        if (activity == 0) {
            continue;  // Timeout, check running_ flag
        }
        
        if (FD_ISSET(server_fd_, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd >= 0) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                handle_client(client_fd, client_ip);
            }
        }
    }
    
    std::cout << "[LuaWeb] Server stopped" << std::endl;
    return true;
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
}

void Server::handle_client(int client_fd, const std::string& client_ip) {
    // Read request with timeout
    std::string raw_request;
    char buffer[4096];
    
    // Set socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    fd_set read_fds;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(client_fd, &read_fds);
    
    if (select(client_fd + 1, &read_fds, nullptr, nullptr, &timeout) > 0) {
        ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            raw_request = buffer;
        }
    }
    
    if (raw_request.empty()) {
        close(client_fd);
        return;
    }
    
    // Parse request
    auto req = Request::parse(raw_request);
    req->client_ip = client_ip;
    
    Response res;
    
    // Dispatch to handler
    dispatch_request(*req, res);
    
    // Send response
    std::string response_str = res.build();
    write(client_fd, response_str.c_str(), response_str.size());
    
    close(client_fd);
}

void Server::dispatch_request(Request& req, Response& res) {
    // Run middleware chain, then handle request
    run_middleware_chain(req, res, 0, [this, &req, &res]() {
        // Check if response was already sent by middleware
        if (res.is_sent()) {
            return;
        }
        
        // Try static file serving first (for GET requests)
        if (req.method == "GET" && try_serve_static(req, res)) {
            return;
        }
        
        // Try matching routes
        for (const auto& route : routes_) {
            if (route.method != req.method) {
                continue;
            }
            
            if (match_route(route, req.path, req.params)) {
                try {
                    route.handler(req, res);
                    return;
                } catch (const std::exception& e) {
                    res.status(500).text(std::string("Internal Server Error: ") + e.what());
                    return;
                }
            }
        }
        
        // No route matched
        res.status(404).text("Not Found");
    });
}

void Server::run_middleware_chain(Request& req, Response& res, size_t index, std::function<void()> final_handler) {
    if (index >= middlewares_.size()) {
        // All middleware executed, run final handler
        final_handler();
        return;
    }
    
    // Check if response already sent
    if (res.is_sent()) {
        return;
    }
    
    // Run current middleware with next() callback
    middlewares_[index](req, res, [this, &req, &res, index, final_handler]() {
        run_middleware_chain(req, res, index + 1, final_handler);
    });
}

bool Server::try_serve_static(Request& req, Response& res) {
    for (const auto& mount : static_mounts_) {
        // Check if path starts with the URL prefix
        if (req.path.find(mount.url_prefix) == 0) {
            // Get relative path after the prefix
            std::string relative_path = req.path.substr(mount.url_prefix.length());
            if (relative_path.empty() || relative_path == "/") {
                relative_path = "/index.html";  // Default to index.html
            }
            
            // Security: prevent directory traversal
            if (relative_path.find("..") != std::string::npos) {
                res.status(403).text("Forbidden");
                return true;
            }
            
            // Build full filesystem path
            std::filesystem::path file_path = mount.directory + relative_path;
            
            // Check if file exists and is a regular file
            if (std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path)) {
                // Read file content
                std::ifstream file(file_path, std::ios::binary);
                if (file) {
                    std::stringstream buffer;
                    buffer << file.rdbuf();
                    
                    // Set content type based on extension
                    std::string mime_type = get_mime_type(file_path.string());
                    res.header("Content-Type", mime_type);
                    res.body(buffer.str());
                    res.mark_sent();
                    return true;
                }
            }
        }
    }
    return false;
}

std::string Server::get_mime_type(const std::string& path) {
    // Get file extension
    size_t dot_pos = path.rfind('.');
    if (dot_pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dot_pos);
    
    // Convert to lowercase
    for (char& c : ext) {
        c = std::tolower(c);
    }
    
    // Common MIME types
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css; charset=utf-8"},
        {".js", "application/javascript; charset=utf-8"},
        {".json", "application/json; charset=utf-8"},
        {".xml", "application/xml; charset=utf-8"},
        {".txt", "text/plain; charset=utf-8"},
        {".md", "text/markdown; charset=utf-8"},
        
        // Images
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".webp", "image/webp"},
        
        // Fonts
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".ttf", "font/ttf"},
        {".otf", "font/otf"},
        {".eot", "application/vnd.ms-fontobject"},
        
        // Other
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".wasm", "application/wasm"},
    };
    
    auto it = mime_types.find(ext);
    if (it != mime_types.end()) {
        return it->second;
    }
    
    return "application/octet-stream";
}

void Server::handle_web_lua(Request& req, Response& res) {
    if (!web_lua_enabled_) {
        res.status(403).text("Web Lua execution is disabled");
        return;
    }
    
    std::string lua_code = req.body;
    if (lua_code.empty()) {
        res.status(400).text("No Lua code provided");
        return;
    }
    
    // Create __cacheweb__ directory in current working directory
    std::filesystem::path cache_dir = std::filesystem::current_path() / "__cacheweb__";
    std::filesystem::create_directories(cache_dir);
    
    // Generate unique filename
    std::string file_id = std::to_string(std::hash<std::string>{}(lua_code + std::to_string(time(nullptr))));
    std::filesystem::path temp_file = cache_dir / ("exec_" + file_id + ".lua");
    
    // Get path to sandbox.lua and run_lua.py
    std::filesystem::path exe_path = std::filesystem::current_path();
    std::filesystem::path sandbox_path = exe_path / "lua" / "sandbox.lua";
    std::filesystem::path runner_path = exe_path / "scripts" / "run_lua.py";
    
    // Write Lua code to temp file
    {
        std::ofstream file(temp_file);
        file << lua_code;
    }
    
    // Run via Python sandbox runner - pass the FILE PATH not the code
    std::string command = "python3 \"" + runner_path.string() + "\" \"" + 
                          sandbox_path.string() + "\" \"" + temp_file.string() + "\" 2>&1";
    
    std::array<char, 4096> buffer;
    std::string output;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }
        pclose(pipe);
    }
    
    // Clean up temp file
    std::filesystem::remove(temp_file);
    
    res.text(output);
}

} // namespace luaweb
