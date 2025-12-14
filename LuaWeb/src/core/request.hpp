#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

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
    std::unordered_map<std::string, std::string> params;      // URL params like :id
    std::unordered_map<std::string, std::string> query_params; // ?foo=bar
    std::string client_ip;

    // Parse raw HTTP request
    static std::unique_ptr<Request> parse(const std::string& raw_request);

private:
    void parse_query_string(const std::string& query);
};

} // namespace luaweb
