#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <expected>
#include <cstdint>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "http/c_http_router.h"

class c_http_server {
public:
    c_http_server() = default;
    ~c_http_server();

    // Non-copyable, non-movable
    c_http_server(const c_http_server&) = delete;
    c_http_server& operator=(const c_http_server&) = delete;
    c_http_server(c_http_server&&) = delete;
    c_http_server& operator=(c_http_server&&) = delete;

    // Start the HTTP server on the given host:port
    [[nodiscard]] std::expected<void, std::string> start(
        const std::string& host, uint16_t port, c_http_router* router
    );

    // Stop the server and join the listener thread
    void stop();

    // Check if the server is currently running
    [[nodiscard]] bool is_running() const { return m_running.load(); }

    // Get the bound port
    [[nodiscard]] uint16_t get_port() const { return m_port; }

private:
    static constexpr size_t MAX_REQUEST_SIZE = 1024 * 1024; // 1MB max request body
    static constexpr int RECV_TIMEOUT_MS = 5000;

    SOCKET m_listen_socket = INVALID_SOCKET;
    std::atomic<bool> m_running{false};
    std::thread m_listener_thread;
    c_http_router* m_router = nullptr;
    uint16_t m_port = 0;

    // Main listener loop (runs on m_listener_thread)
    void listener_loop();

    // Handle a single client connection
    void handle_connection(SOCKET client_socket);

    // Parse raw HTTP request data into s_http_request
    [[nodiscard]] static std::expected<s_http_request, std::string> parse_request(
        const std::string& raw_data
    );

    // Parse query string into key-value pairs
    static void parse_query_string(
        const std::string& query_string,
        std::unordered_map<std::string, std::string>& out
    );

    // URL-decode a string
    [[nodiscard]] static std::string url_decode(const std::string& encoded);
};
