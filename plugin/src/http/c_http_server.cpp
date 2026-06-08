#include "http/c_http_server.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
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
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        auto err = WSAGetLastError();
        WSACleanup();
        return std::unexpected("socket() failed with error: " + std::to_string(err));
    }

    // Allow address reuse
    int opt_val = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt_val), sizeof(opt_val));

    // Bind to localhost only
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        auto err = WSAGetLastError();
        closesocket(sock);
        WSACleanup();
        return std::unexpected("bind() failed with error: " + std::to_string(err));
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        auto err = WSAGetLastError();
        closesocket(sock);
        WSACleanup();
        return std::unexpected("listen() failed with error: " + std::to_string(err));
    }

    m_listen_socket.store(sock);
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
    SOCKET ls = m_listen_socket.exchange(INVALID_SOCKET);
    if (ls != INVALID_SOCKET) {
        closesocket(ls);
    }

    // Wait for the listener thread to finish
    if (m_listener_thread.joinable()) {
        m_listener_thread.join();
    }

    // Drain in-flight detached connection threads before WSACleanup, so they
    // don't operate on a torn-down Winsock or the soon-to-be-freed router.
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(DRAIN_TIMEOUT_MS);
    while (m_active_connections.load() > 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    WSACleanup();
}

void c_http_server::listener_loop() {
    // Use select() with timeout so we can check m_running periodically
    // instead of blocking forever in accept()
    while (m_running.load()) {
        SOCKET ls = m_listen_socket.load();
        if (ls == INVALID_SOCKET) {
            break;
        }

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(ls, &read_set);

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
            ls,
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

bool c_http_server::is_authorized(const s_http_request& request) const {
    if (m_auth_token.empty()) {
        return true; // auth disabled
    }

    // Accept either "Authorization: Bearer <token>" or "X-Auth-Token: <token>".
    // Header keys are stored lowercased by parse_request.
    auto it = request.headers.find("authorization");
    if (it != request.headers.end()) {
        static const std::string prefix = "Bearer ";
        const std::string& v = it->second;
        if (v.size() > prefix.size() &&
            v.compare(0, prefix.size(), prefix) == 0 &&
            v.substr(prefix.size()) == m_auth_token) {
            return true;
        }
    }

    auto it2 = request.headers.find("x-auth-token");
    if (it2 != request.headers.end() && it2->second == m_auth_token) {
        return true;
    }

    return false;
}

void c_http_server::handle_connection(SOCKET client_socket) {
    // Track this connection so stop() can drain in-flight requests before WSACleanup.
    m_active_connections.fetch_add(1);

    s_http_response response;

    // Everything below runs on a detached thread. A single uncaught exception
    // here would call std::terminate() and crash all of x64dbg, so the entire
    // body is wrapped: any throw becomes a 500 instead of a process kill.
    try {
        // Set receive/send timeouts
        DWORD timeout = RECV_TIMEOUT_MS;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));
        setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout), sizeof(timeout));

        int nodelay = 1;
        setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

        int keepalive = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE,
                   reinterpret_cast<const char*>(&keepalive), sizeof(keepalive));

        // Read the full request
        std::string raw_data;
        raw_data.reserve(4096);

        char buffer[4096];
        size_t content_length = 0;
        bool headers_complete = false;
        bool too_large = false;
        size_t header_end_pos = std::string::npos;

        while (true) {
            auto bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_read <= 0) break;

            raw_data.append(buffer, static_cast<size_t>(bytes_read));

            if (!headers_complete) {
                header_end_pos = raw_data.find("\r\n\r\n");
                if (header_end_pos != std::string::npos) {
                    headers_complete = true;
                    content_length = parse_content_length(raw_data, header_end_pos, too_large);
                    if (too_large) break;
                }
            }

            if (headers_complete) {
                auto body_start = header_end_pos + 4;
                auto body_received = raw_data.size() - body_start;
                if (body_received >= content_length) break;
            }

            // Safety: never buffer more than MAX_REQUEST_SIZE
            if (raw_data.size() > MAX_REQUEST_SIZE) {
                too_large = true;
                break;
            }
        }

        if (too_large) {
            response = s_http_response::bad_request("Request exceeds maximum size");
        } else {
            auto parse_result = parse_request(raw_data);
            if (!parse_result.has_value()) {
                response = s_http_response::bad_request(parse_result.error());
            } else if (!is_authorized(parse_result.value())) {
                response = s_http_response::unauthorized(
                    "Missing or invalid auth token (Authorization: Bearer <token>)");
            } else {
                response = m_router->dispatch(parse_result.value());
            }
        }
    } catch (const std::exception& e) {
        response = s_http_response::internal_error(std::string("Server exception: ") + e.what());
    } catch (...) {
        response = s_http_response::internal_error("Unknown server exception");
    }

    // Send response (best effort, handle partial sends)
    auto response_str = response.serialize();
    auto total = static_cast<int>(response_str.size());
    int sent = 0;
    while (sent < total) {
        auto result = send(client_socket, response_str.c_str() + sent, total - sent, 0);
        if (result == SOCKET_ERROR) break;
        sent += result;
    }

    shutdown(client_socket, SD_SEND);
    closesocket(client_socket);

    m_active_connections.fetch_sub(1);
}

size_t c_http_server::parse_content_length(
    const std::string& raw_data, size_t header_end_pos, bool& too_large
) {
    too_large = false;

    auto cl_pos = raw_data.find("Content-Length:");
    if (cl_pos == std::string::npos) {
        cl_pos = raw_data.find("content-length:");
    }
    if (cl_pos == std::string::npos || cl_pos > header_end_pos) {
        return 0; // no body declared
    }

    auto val_start = cl_pos + 15; // length of "Content-Length:"
    auto val_end = raw_data.find("\r\n", val_start);
    if (val_end == std::string::npos || val_end > header_end_pos + 2) {
        val_end = header_end_pos;
    }

    // Trim surrounding whitespace, then parse without throwing.
    auto begin = raw_data.find_first_not_of(" \t", val_start);
    if (begin == std::string::npos || begin >= val_end) {
        return 0;
    }
    auto last = raw_data.find_last_not_of(" \t\r", val_end - 1);
    if (last == std::string::npos || last < begin) {
        return 0;
    }

    unsigned long long value = 0;
    auto res = std::from_chars(raw_data.data() + begin, raw_data.data() + last + 1, value);
    if (res.ec != std::errc{}) {
        return 0; // malformed Content-Length -> treat as no body, never throw
    }

    if (value > MAX_REQUEST_SIZE) {
        too_large = true;
        return 0;
    }

    return static_cast<size_t>(value);
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
    auto hex_value = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };

    std::string result;
    result.reserve(encoded.size());

    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int hi = hex_value(encoded[i + 1]);
            int lo = hex_value(encoded[i + 2]);
            if (hi >= 0 && lo >= 0) {
                result += static_cast<char>((hi << 4) | lo);
                i += 2;
            } else {
                // Malformed escape: keep the '%' literally instead of throwing.
                result += encoded[i];
            }
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }

    return result;
}
