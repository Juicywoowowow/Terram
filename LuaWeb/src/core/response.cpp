#include "response.hpp"

namespace luaweb {

Response::Response() 
    : status_code_(200), status_text_("OK"), sent_(false) {
    headers_["Content-Type"] = "text/html; charset=utf-8";
    headers_["Connection"] = "close";
}

Response& Response::status(int code) {
    status_code_ = code;
    status_text_ = get_status_text(code);
    return *this;
}

Response& Response::header(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

Response& Response::body(const std::string& content) {
    body_ = content;
    return *this;
}

Response& Response::json(const std::string& json_str) {
    headers_["Content-Type"] = "application/json; charset=utf-8";
    body_ = json_str;
    return *this;
}

Response& Response::html(const std::string& content) {
    headers_["Content-Type"] = "text/html; charset=utf-8";
    body_ = content;
    return *this;
}

Response& Response::text(const std::string& content) {
    headers_["Content-Type"] = "text/plain; charset=utf-8";
    body_ = content;
    return *this;
}

std::string Response::build() const {
    std::ostringstream response;
    
    // Status line
    response << "HTTP/1.1 " << status_code_ << " " << status_text_ << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers_) {
        response << key << ": " << value << "\r\n";
    }
    
    // Content-Length
    response << "Content-Length: " << body_.size() << "\r\n";
    
    // Empty line + body
    response << "\r\n" << body_;
    
    return response.str();
}

std::string Response::get_status_text(int code) {
    static const std::unordered_map<int, std::string> status_texts = {
        {200, "OK"},
        {201, "Created"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {304, "Not Modified"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {500, "Internal Server Error"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {429, "Too Many Requests"},
    };
    
    auto it = status_texts.find(code);
    if (it != status_texts.end()) {
        return it->second;
    }
    return "Unknown";
}

} // namespace luaweb
