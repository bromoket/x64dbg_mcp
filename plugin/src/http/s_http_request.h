#pragma once

#include <string>
#include <unordered_map>

struct s_http_request {
    std::string method;                                     // "GET", "POST", etc.
    std::string path;                                       // "/api/debug/state"
    std::string query_string;                               // "address=0x401000&size=64"
    std::unordered_map<std::string, std::string> query;     // Parsed query parameters
    std::unordered_map<std::string, std::string> headers;   // Request headers (lowercased keys)
    std::string body;                                       // Raw request body

    // Get a query parameter with a default value
    [[nodiscard]] std::string get_query(const std::string& key, const std::string& default_value = "") const {
        auto it = query.find(key);
        return (it != query.end()) ? it->second : default_value;
    }

    // Get a header value (key must be lowercase)
    [[nodiscard]] std::string get_header(const std::string& key, const std::string& default_value = "") const {
        auto it = headers.find(key);
        return (it != headers.end()) ? it->second : default_value;
    }
};
