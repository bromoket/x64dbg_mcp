#pragma once

#include <string>
#include <sstream>
#include <nlohmann/json.hpp>

struct s_http_response {
    int status_code = 200;
    std::string content_type = "application/json";
    std::string body;

    // Build a success response with data payload
    static s_http_response ok(const nlohmann::json& data) {
        nlohmann::json envelope = {
            {"success", true},
            {"data", data}
        };
        return {200, "application/json", envelope.dump()};
    }

    // Build an error response
    static s_http_response error(int code, const std::string& message) {
        nlohmann::json envelope = {
            {"success", false},
            {"error", {
                {"code", code},
                {"message", message}
            }}
        };
        return {code, "application/json", envelope.dump()};
    }

    // 400 Bad Request
    static s_http_response bad_request(const std::string& message) {
        return error(400, message);
    }

    // 404 Not Found
    static s_http_response not_found(const std::string& message = "Not found") {
        return error(404, message);
    }

    // 409 Conflict (wrong debugger state)
    static s_http_response conflict(const std::string& message) {
        return error(409, message);
    }

    // 500 Internal Server Error
    static s_http_response internal_error(const std::string& message) {
        return error(500, message);
    }

    // Serialize to HTTP response string
    [[nodiscard]] std::string serialize() const {
        std::ostringstream oss;
        oss << "HTTP/1.1 " << status_code << " " << status_text() << "\r\n";
        oss << "Content-Type: " << content_type << "\r\n";
        oss << "Content-Length: " << body.size() << "\r\n";
        oss << "Connection: close\r\n";
        oss << "Access-Control-Allow-Origin: *\r\n";
        oss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        oss << "Access-Control-Allow-Headers: Content-Type\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }

private:
    [[nodiscard]] std::string status_text() const {
        switch (status_code) {
            case 200: return "OK";
            case 400: return "Bad Request";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            case 409: return "Conflict";
            case 500: return "Internal Server Error";
            default:  return "Unknown";
        }
    }
};
