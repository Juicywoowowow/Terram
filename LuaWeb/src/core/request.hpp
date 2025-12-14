#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>
#include "../vendor/json.hpp"

namespace luaweb {

/**
 * HTTP Request - parsed from incoming connection
 */
class Request {
public:
    std::string method;
    std::string path;
    std::string body;
    std::string raw_query;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;       // URL params like :id
    std::unordered_map<std::string, std::string> query_params; // ?foo=bar
    std::unordered_map<std::string, std::string> cookies;      // Cookie header parsed
    std::string client_ip;
    
    // Parsed JSON body (lazily parsed on first access)
    std::optional<nlohmann::json> json_body;
    bool json_parsed = false;

    // Parse raw HTTP request
    static std::unique_ptr<Request> parse(const std::string& raw_request);
    
    // Parse JSON body (call after parsing request)
    bool parse_json_body();
    
    // Check if request has JSON content type
    bool has_json_content_type() const;

private:
    void parse_query_string(const std::string& query);
    void parse_cookies(const std::string& cookie_header);
};

} // namespace luaweb

