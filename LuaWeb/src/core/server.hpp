#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <regex>
#include <atomic>

#include "request.hpp"
#include "response.hpp"

namespace luaweb {

// Handler function type
using RouteHandler = std::function<void(Request&, Response&)>;

/**
 * Route - stores path pattern and handler
 */
struct Route {
    std::string method;
    std::string pattern;           // Original pattern like "/users/:id"
    std::regex regex;              // Compiled regex
    std::vector<std::string> param_names;  // Parameter names in order
    RouteHandler handler;
};

/**
 * HTTP Server - main server class
 */
class Server {
public:
    explicit Server(int port = 8080);
    ~Server();

    // Route registration
    void route(const std::string& method, const std::string& path, RouteHandler handler);
    
    // Convenience methods
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);

    // Server control
    bool run();
    void stop();
    bool is_running() const { return running_.load(); }

    // Web Lua execution
    void enable_web_lua(bool enabled = true);
    bool web_lua_enabled() const { return web_lua_enabled_; }

private:
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
    std::vector<Route> routes_;
    bool web_lua_enabled_;

    // Internal methods
    bool bind_socket();
    void handle_client(int client_fd, const std::string& client_ip);
    void dispatch_request(Request& req, Response& res);
    bool match_route(const Route& route, const std::string& path, 
                     std::unordered_map<std::string, std::string>& params);
    
    // Path pattern to regex conversion
    std::pair<std::regex, std::vector<std::string>> compile_pattern(const std::string& pattern);
    
    // Web Lua handler
    void handle_web_lua(Request& req, Response& res);
};

} // namespace luaweb
