#include "http/c_http_server.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <mstcpip.h>

#pragma comment(lib, "ws2_32.lib")

c_http_server::~c_http_server() {
    stop();
}

std::expected<void, std::string> c_http_server::start(
    const std::string& host, uint16_t port, c_http_router* router
) {
    if (m_running.load()) {
        return std::unexpected("Server is already running");
    }

    m_router = router;
    m_port = port;

    // Initialize Winsock
    WSADATA wsa_data{};
    auto wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_result != 0) {
        return std::unexpected("WSAStartup failed with error: " + std::to_string(wsa_result));
    }

    // Create listening socket
    m_listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listen_socket == INVALID_SOCKET) {
        auto err = WSAGetLastError();
        WSACleanup();
        return std::unexpected("socket() failed with error: " + std::to_string(err));
    }

    // Allow address reuse
    int opt_val = 1;
    setsockopt(m_listen_socket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt_val), sizeof(opt_val));

    // Bind to localhost only
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (bind(m_listen_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        auto err = WSAGetLastError();
        closesocket(m_listen_socket);
        m_listen_socket = INVALID_SOCKET;
        WSACleanup();
        return std::unexpected("bind() failed with error: " + std::to_string(err));
    }

    if (listen(m_listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        auto err = WSAGetLastError();
        closesocket(m_listen_socket);
        m_listen_socket = INVALID_SOCKET;
        WSACleanup();
        return std::unexpected("listen() failed with error: " + std::to_string(err));
    }

    m_running.store(true);
    m_listener_thread = std::thread(&c_http_server::listener_loop, this);

    return {};
}

void c_http_server::stop() {
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);

    // Close the listening socket to unblock accept()
    if (m_listen_socket != INVALID_SOCKET) {
        closesocket(m_listen_socket);
        m_listen_socket = INVALID_SOCKET;
    }

    // Wait for the listener thread to finish
    if (m_listener_thread.joinable()) {
        m_listener_thread.join();
    }

    WSACleanup();
}

void c_http_server::listener_loop() {
    // Use select() with timeout so we can check m_running periodically
    // instead of blocking forever in accept()
    while (m_running.load()) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(m_listen_socket, &read_set);

        timeval tv{};
        tv.tv_sec = 1;  // 1 second poll interval
        tv.tv_usec = 0;

        auto sel_result = select(0, &read_set, nullptr, nullptr, &tv);
        if (sel_result == SOCKET_ERROR) {
            if (!m_running.load()) break;
            continue;
        }
        if (sel_result == 0) {
            // Timeout - just loop and check m_running
            continue;
        }

        sockaddr_in client_addr{};
        int client_addr_len = sizeof(client_addr);

        SOCKET client_socket = accept(
            m_listen_socket,
            reinterpret_cast<sockaddr*>(&client_addr),
            &client_addr_len
        );

        if (client_socket == INVALID_SOCKET) {
            if (!m_running.load()) break;
            continue;
        }

        // Handle each connection on a detached thread
        std::thread(&c_http_server::handle_connection, this, client_socket).detach();
    }
}

void c_http_server::handle_connection(SOCKET client_socket) {
    // Set receive timeout
    DWORD timeout = RECV_TIMEOUT_MS;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    // Set send timeout
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    // Disable Nagle's algorithm for low latency
    int nodelay = 1;
    setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

    // Enable TCP keepalive to detect dead connections
    int keepalive = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE,
               reinterpret_cast<const char*>(&keepalive), sizeof(keepalive));

    // Read the full request
    std::string raw_data;
    raw_data.reserve(4096);

    char buffer[4096];
    size_t content_length = 0;
    bool headers_complete = false;
    size_t header_end_pos = std::string::npos;

    while (true) {
        auto bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) break;

        raw_data.append(buffer, static_cast<size_t>(bytes_read));

        // Check if we've received all headers
        if (!headers_complete) {
            header_end_pos = raw_data.find("\r\n\r\n");
            if (header_end_pos != std::string::npos) {
                headers_complete = true;

                // Extract Content-Length
                auto cl_pos = raw_data.find("Content-Length:");
                if (cl_pos == std::string::npos) {
                    cl_pos = raw_data.find("content-length:");
                }
                if (cl_pos != std::string::npos) {
                    auto val_start = cl_pos + 15; // length of "Content-Length:"
                    auto val_end = raw_data.find("\r\n", val_start);
                    auto val_str = raw_data.substr(val_start, val_end - val_start);
                    content_length = std::stoull(val_str);
                }
            }
        }

        // Check if we have the complete body
        if (headers_complete) {
            auto body_start = header_end_pos + 4;
            auto body_received = raw_data.size() - body_start;
            if (body_received >= content_length) break;
        }

        // Safety check: don't read more than MAX_REQUEST_SIZE
        if (raw_data.size() > MAX_REQUEST_SIZE) break;
    }

    // Parse and dispatch
    auto parse_result = parse_request(raw_data);
    s_http_response response;

    if (parse_result.has_value()) {
        response = m_router->dispatch(parse_result.value());
    } else {
        response = s_http_response::bad_request(parse_result.error());
    }

    // Send response (handle partial sends)
    auto response_str = response.serialize();
    auto total = static_cast<int>(response_str.size());
    int sent = 0;

    while (sent < total) {
        auto result = send(client_socket, response_str.c_str() + sent, total - sent, 0);
        if (result == SOCKET_ERROR) break;
        sent += result;
    }

    // Graceful shutdown
    shutdown(client_socket, SD_SEND);
    closesocket(client_socket);
}

std::expected<s_http_request, std::string> c_http_server::parse_request(
    const std::string& raw_data
) {
    if (raw_data.empty()) {
        return std::unexpected("Empty request");
    }

    s_http_request req;

    // Find end of request line
    auto line_end = raw_data.find("\r\n");
    if (line_end == std::string::npos) {
        return std::unexpected("Malformed request line");
    }

    auto request_line = raw_data.substr(0, line_end);

    // Parse: METHOD PATH HTTP/1.x
    auto first_space = request_line.find(' ');
    if (first_space == std::string::npos) {
        return std::unexpected("Malformed request line: no method");
    }
    req.method = request_line.substr(0, first_space);

    auto second_space = request_line.find(' ', first_space + 1);
    if (second_space == std::string::npos) {
        return std::unexpected("Malformed request line: no path");
    }
    auto full_path = request_line.substr(first_space + 1, second_space - first_space - 1);

    // Split path and query string
    auto query_pos = full_path.find('?');
    if (query_pos != std::string::npos) {
        req.path = full_path.substr(0, query_pos);
        req.query_string = full_path.substr(query_pos + 1);
        parse_query_string(req.query_string, req.query);
    } else {
        req.path = full_path;
    }

    // Parse headers
    auto header_end = raw_data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return std::unexpected("Malformed headers");
    }

    auto headers_section = raw_data.substr(line_end + 2, header_end - line_end - 2);
    std::istringstream header_stream(headers_section);
    std::string header_line;

    while (std::getline(header_stream, header_line)) {
        // Remove trailing \r
        if (!header_line.empty() && header_line.back() == '\r') {
            header_line.pop_back();
        }
        if (header_line.empty()) continue;

        auto colon = header_line.find(':');
        if (colon == std::string::npos) continue;

        auto key = header_line.substr(0, colon);
        auto value = header_line.substr(colon + 1);

        // Lowercase the key
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        // Trim leading whitespace from value
        auto val_start = value.find_first_not_of(" \t");
        if (val_start != std::string::npos) {
            value = value.substr(val_start);
        }

        req.headers[key] = value;
    }

    // Extract body
    auto body_start = header_end + 4;
    if (body_start < raw_data.size()) {
        req.body = raw_data.substr(body_start);
    }

    return req;
}

void c_http_server::parse_query_string(
    const std::string& query_string,
    std::unordered_map<std::string, std::string>& out
) {
    std::istringstream stream(query_string);
    std::string pair;

    while (std::getline(stream, pair, '&')) {
        auto eq = pair.find('=');
        if (eq != std::string::npos) {
            auto key = url_decode(pair.substr(0, eq));
            auto value = url_decode(pair.substr(eq + 1));
            out[key] = value;
        } else {
            out[url_decode(pair)] = "";
        }
    }
}

std::string c_http_server::url_decode(const std::string& encoded) {
    std::string result;
    result.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            auto hex_str = encoded.substr(i + 1, 2);
            auto ch = static_cast<char>(std::stoi(hex_str, nullptr, 16));
            result += ch;
            i += 2;
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }

    return result;
}
