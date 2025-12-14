#pragma once

#include <string>
#include <unordered_map>
#include <sstream>

namespace luaweb {

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
    std::string body_;
    bool sent_;

    static std::string get_status_text(int code);
};

} // namespace luaweb
