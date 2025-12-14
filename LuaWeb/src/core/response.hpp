#pragma once

#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>

namespace luaweb {

/**
 * Cookie options for Set-Cookie header
 */
struct CookieOptions {
    int max_age = -1;              // Max age in seconds (-1 = session cookie)
    std::string path = "/";         // Cookie path
    std::string domain = "";        // Cookie domain
    bool http_only = false;         // HttpOnly flag
    bool secure = false;            // Secure flag (HTTPS only)
    std::string same_site = "";     // SameSite: Strict, Lax, None
};

/**
 * HTTP Response - built and sent to client
 */
class Response {
public:
    Response();

    // Chainable setters
    Response& status(int code);
    Response& header(const std::string& key, const std::string& value);
    Response& body(const std::string& content);
    Response& json(const std::string& json_str);
    
    // Cookie support
    Response& cookie(const std::string& name, const std::string& value, 
                     const CookieOptions& options = CookieOptions());
    Response& clear_cookie(const std::string& name, const std::string& path = "/");

    // Build the raw HTTP response
    std::string build() const;

    // Helper to send HTML
    Response& html(const std::string& content);

    // Helper to send plain text
    Response& text(const std::string& content);

    bool is_sent() const { return sent_; }
    void mark_sent() { sent_ = true; }

private:
    int status_code_;
    std::string status_text_;
    std::unordered_map<std::string, std::string> headers_;
    std::vector<std::string> cookies_;  // Set-Cookie header values
    std::string body_;
    bool sent_;

    static std::string get_status_text(int code);
};

} // namespace luaweb

