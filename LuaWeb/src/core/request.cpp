#include "request.hpp"
#include "../vendor/json.hpp"
#include <sstream>
#include <algorithm>

namespace luaweb {

std::unique_ptr<Request> Request::parse(const std::string& raw_request) {
    auto req = std::make_unique<Request>();
    
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parse request line: GET /path?query HTTP/1.1
    if (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        std::istringstream request_line(line);
        std::string version;
        request_line >> req->method >> req->path >> version;
        
        // Parse query string from path
        size_t query_pos = req->path.find('?');
        if (query_pos != std::string::npos) {
            req->raw_query = req->path.substr(query_pos + 1);
            req->path = req->path.substr(0, query_pos);
            req->parse_query_string(req->raw_query);
        }
    }
    
    // Parse headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            break;  // End of headers
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim leading whitespace from value
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos) {
                value = value.substr(start);
            }
            
            req->headers[key] = value;
            
            // Parse cookies from Cookie header
            if (key == "Cookie") {
                req->parse_cookies(value);
            }
        }
    }
    
    // Parse body (rest of the request)
    std::stringstream body_stream;
    body_stream << stream.rdbuf();
    req->body = body_stream.str();
    
    // Auto-parse JSON if content-type is JSON
    if (req->has_json_content_type() && !req->body.empty()) {
        req->parse_json_body();
    }
    
    return req;
}

void Request::parse_query_string(const std::string& query) {
    std::istringstream stream(query);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            
            // Basic URL decoding (replace + with space, decode %XX)
            std::string decoded_value;
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '+') {
                    decoded_value += ' ';
                } else if (value[i] == '%' && i + 2 < value.size()) {
                    int hex_val;
                    std::istringstream hex_stream(value.substr(i + 1, 2));
                    if (hex_stream >> std::hex >> hex_val) {
                        decoded_value += static_cast<char>(hex_val);
                        i += 2;
                    } else {
                        decoded_value += value[i];
                    }
                } else {
                    decoded_value += value[i];
                }
            }
            
            query_params[key] = decoded_value;
        }
    }
}

void Request::parse_cookies(const std::string& cookie_header) {
    // Cookie header format: name1=value1; name2=value2; ...
    std::istringstream stream(cookie_header);
    std::string pair;
    
    while (std::getline(stream, pair, ';')) {
        // Trim leading whitespace
        size_t start = pair.find_first_not_of(" \t");
        if (start != std::string::npos) {
            pair = pair.substr(start);
        }
        
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string name = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            cookies[name] = value;
        }
    }
}

bool Request::has_json_content_type() const {
    auto it = headers.find("Content-Type");
    if (it == headers.end()) {
        return false;
    }
    
    const std::string& content_type = it->second;
    return content_type.find("application/json") != std::string::npos;
}

bool Request::parse_json_body() {
    if (json_parsed) {
        return json_body.has_value();
    }
    
    json_parsed = true;
    
    if (body.empty()) {
        return false;
    }
    
    try {
        json_body = nlohmann::json::parse(body);
        return true;
    } catch (const nlohmann::json::parse_error&) {
        return false;
    }
}

} // namespace luaweb

