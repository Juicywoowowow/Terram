#include "database.hpp"
#include "../vendor/json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <signal.h>
#include <filesystem>
#include <random>

namespace luaweb {

static const char* BRIDGE_HOST = "127.0.0.1";

// ============================================================
// HTTP Client (minimal implementation for bridge communication)
// ============================================================

static std::string http_post(int port, const std::string& path, const std::string& body) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return "";
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, BRIDGE_HOST, &server_addr.sin_addr);
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return "";
    }
    
    // Build HTTP request
    std::ostringstream request;
    request << "POST " << path << " HTTP/1.1\r\n";
    request << "Host: " << BRIDGE_HOST << ":" << port << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body.size() << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    request << body;
    
    std::string req_str = request.str();
    send(sock, req_str.c_str(), req_str.size(), 0);
    
    // Read response
    std::string response;
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        response += buffer;
    }
    
    close(sock);
    
    // Extract body (after \r\n\r\n)
    size_t body_start = response.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        return response.substr(body_start + 4);
    }
    
    return response;
}

static std::string http_get(int port, const std::string& path) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return "";
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, BRIDGE_HOST, &server_addr.sin_addr);
    
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // Don't spam stderr on connect failure (expected during startup)
        close(sock);
        return "";
    }
    
    std::ostringstream request;
    request << "GET " << path << " HTTP/1.1\r\n";
    request << "Host: " << BRIDGE_HOST << "\r\n";
    request << "Connection: close\r\n";
    request << "\r\n";
    
    std::string req_str = request.str();
    ssize_t sent = send(sock, req_str.c_str(), req_str.size(), 0);
    if (sent < 0) {
        std::cerr << "Send failed: " << strerror(errno) << std::endl;
        close(sock);
        return "";
    }
    
    std::string response;
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        response += buffer;
    }
    
    close(sock);
    
    size_t body_start = response.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        return response.substr(body_start + 4);
    }
    
    return response;
}

// Generate random hex string
static std::string generate_random_id(size_t length = 8) {
    static const char hex_chars[] = "0123456789abcdef";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += hex_chars[dis(gen)];
    }
    return result;
}

// ============================================================
// DatabaseBridge
// ============================================================

DatabaseBridge::DatabaseBridge(const std::string& server_id)
    : server_id_(server_id.empty() ? generate_random_id() : server_id) {
}

DatabaseBridge::~DatabaseBridge() {
    stop();
}

int DatabaseBridge::find_free_port() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 9876;  // Fallback
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0;  // Let OS choose
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return 9876;
    }
    
    socklen_t len = sizeof(addr);
    if (getsockname(sock, (struct sockaddr*)&addr, &len) < 0) {
        close(sock);
        return 9876;
    }
    
    int port = ntohs(addr.sin_port);
    close(sock);
    return port;
}

bool DatabaseBridge::check_health() {
    if (port_ == 0) return false;
    
    std::string response = http_get(port_, "/health");
    if (response.empty()) {
        return false;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        bool ok = json.value("ok", false);
        std::string sid = json.value("serverId", "");
        
        if (!ok || sid != server_id_) {
            std::cerr << "Health check mismatch: ok=" << ok << " sid=" << sid << " expected=" << server_id_ << std::endl;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Health check parse error: " << e.what() << " Resp: " << response << std::endl;
        return false;
    }
}

bool DatabaseBridge::start() {
    if (running_ && check_health()) {
        return true;
    }
    
    // Find the database server script
    std::vector<std::string> search_paths = {
        "./database/server.js",
        "../database/server.js",
        "database/server.js",
    };
    
    std::string script_path;
    for (const auto& p : search_paths) {
        if (std::filesystem::exists(p)) {
            script_path = std::filesystem::absolute(p).string();
            break;
        }
    }
    
    if (script_path.empty()) {
        std::cerr << "[LuaWeb:" << server_id_ << "] Database bridge script not found" << std::endl;
        return false;
    }
    
    // Find a free port
    port_ = find_free_port();
    
    // Fork and exec node
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "[LuaWeb:" << server_id_ << "] Failed to fork for database bridge" << std::endl;
        return false;
    }
    
    if (pid == 0) {
        // Child process
        // Redirect stdout/stderr to /dev/null in production, or keep for debug
        // freopen("/dev/null", "w", stdout);
        // freopen("/dev/null", "w", stderr);
        
        // NO chdir - keep running in current directory so __DBWEB__ is created here
        
        // Start with port and server ID
        std::string port_str = std::to_string(port_);
        execlp("node", "node", script_path.c_str(), port_str.c_str(), server_id_.c_str(), nullptr);
        
        std::cerr << "[LuaWeb:" << server_id_ << "] Failed to execlp node: " << strerror(errno) << std::endl;
        _exit(1);
    }
    
    // Parent process
    bridge_pid_ = pid;
    
    // Wait for bridge to start
    for (int i = 0; i < 50; ++i) {  // Up to 5 seconds
        usleep(100000);  // 100ms
        if (check_health()) {
            running_ = true;
            std::cout << "[LuaWeb:" << server_id_ << "] Database bridge started on port " << port_ << std::endl;
            return true;
        }
    }
    
    // Failed to start - check if process is still alive
    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);
    if (result == pid) {
        std::cerr << "[LuaWeb:" << server_id_ << "] Database bridge process exited early with status " << status << std::endl;
    } else {
        std::cerr << "[LuaWeb:" << server_id_ << "] Database bridge timed out (health check failed)" << std::endl;
        kill(pid, SIGTERM);
    }
    
    return false;
}

void DatabaseBridge::stop() {
    if (!running_) return;
    
    // Send shutdown command
    if (port_ > 0) {
        http_post(port_, "/shutdown", "{}");
        usleep(200000);  // Wait for graceful shutdown
    }
    
    // Make sure it's dead
    if (bridge_pid_ > 0) {
        kill(bridge_pid_, SIGTERM);
        bridge_pid_ = 0;
    }
    
    running_ = false;
    port_ = 0;
    
    std::cout << "[LuaWeb:" << server_id_ << "] Database bridge stopped" << std::endl;
}

std::string DatabaseBridge::request(const std::string& endpoint, const std::string& json_body) {
    if (!running_ && !start()) {
        return "";
    }
    return http_post(port_, endpoint, json_body);
}

// ============================================================
// Database
// ============================================================

// Helper to convert DbValue to JSON
static nlohmann::json value_to_json(const DbValue& val) {
    return std::visit([](auto&& arg) -> nlohmann::json {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) {
            return nullptr;
        } else {
            return arg;
        }
    }, val);
}

Database::Database(DatabaseBridge& bridge, const std::string& path) 
    : bridge_(&bridge), path_(path) {
    
    if (!bridge_->start()) {
        last_error_ = "Database bridge not available";
        return;
    }
    
    // Open database
    nlohmann::json body;
    body["path"] = path.empty() ? "default.db" : path;
    
    std::string response = bridge_->request("/open", body.dump());
    if (response.empty()) {
        last_error_ = "Failed to connect to database bridge";
        return;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        if (json.value("ok", false)) {
            db_id_ = json.value("id", "");
            path_ = json.value("path", path);
        } else {
            last_error_ = json.value("error", "Unknown error");
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("JSON parse error: ") + e.what();
    }
}

Database::~Database() {
    close();
}

Database::Database(Database&& other) noexcept
    : bridge_(other.bridge_)
    , db_id_(std::move(other.db_id_))
    , path_(std::move(other.path_))
    , last_error_(std::move(other.last_error_)) {
    other.bridge_ = nullptr;
    other.db_id_.clear();
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        close();
        bridge_ = other.bridge_;
        db_id_ = std::move(other.db_id_);
        path_ = std::move(other.path_);
        last_error_ = std::move(other.last_error_);
        other.bridge_ = nullptr;
        other.db_id_.clear();
    }
    return *this;
}

ExecResult Database::exec(const std::string& sql, 
                          const std::vector<DbValue>& params) {
    ExecResult result;
    last_error_.clear();
    
    if (!is_open() || !bridge_) {
        last_error_ = "Database not open";
        return result;
    }
    
    nlohmann::json body;
    body["id"] = db_id_;
    body["sql"] = sql;
    body["params"] = nlohmann::json::array();
    for (const auto& p : params) {
        body["params"].push_back(value_to_json(p));
    }
    
    std::string response = bridge_->request("/exec", body.dump());
    if (response.empty()) {
        last_error_ = "Failed to execute query";
        return result;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        if (json.value("ok", false)) {
            result.changes = json.value("changes", 0);
            result.last_insert_id = json.value("lastInsertRowid", 0);
        } else {
            last_error_ = json.value("error", "Unknown error");
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("JSON parse error: ") + e.what();
    }
    
    return result;
}

DbResult Database::query(const std::string& sql,
                         const std::vector<DbValue>& params) {
    DbResult result;
    last_error_.clear();
    
    if (!is_open() || !bridge_) {
        last_error_ = "Database not open";
        return result;
    }
    
    nlohmann::json body;
    body["id"] = db_id_;
    body["sql"] = sql;
    body["params"] = nlohmann::json::array();
    for (const auto& p : params) {
        body["params"].push_back(value_to_json(p));
    }
    
    std::string response = bridge_->request("/query", body.dump());
    if (response.empty()) {
        last_error_ = "Failed to execute query";
        return result;
    }
    
    try {
        auto json = nlohmann::json::parse(response);
        if (json.value("ok", false)) {
            for (const auto& row : json["rows"]) {
                DbRow db_row;
                for (auto& [key, val] : row.items()) {
                    if (val.is_null()) {
                        db_row[key] = nullptr;
                    } else if (val.is_number_integer()) {
                        db_row[key] = val.get<int64_t>();
                    } else if (val.is_number()) {
                        db_row[key] = val.get<double>();
                    } else if (val.is_string()) {
                        db_row[key] = val.get<std::string>();
                    }
                }
                result.push_back(std::move(db_row));
            }
        } else {
            last_error_ = json.value("error", "Unknown error");
        }
    } catch (const std::exception& e) {
        last_error_ = std::string("JSON parse error: ") + e.what();
    }
    
    return result;
}

void Database::close() {
    if (db_id_.empty() || !bridge_) {
        return;
    }
    
    nlohmann::json body;
    body["id"] = db_id_;
    bridge_->request("/close", body.dump());
    db_id_.clear();
}

} // namespace luaweb
