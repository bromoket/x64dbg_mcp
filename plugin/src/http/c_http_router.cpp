#include "http/c_http_router.h"

void c_http_router::add_route(const std::string& method, const std::string& path, route_handler_t handler) {
    m_routes.push_back({method, path, std::move(handler)});
}

void c_http_router::get(const std::string& path, route_handler_t handler) {
    add_route("GET", path, std::move(handler));
}

void c_http_router::post(const std::string& path, route_handler_t handler) {
    add_route("POST", path, std::move(handler));
}

s_http_response c_http_router::dispatch(const s_http_request& request) const {
    // Handle CORS preflight
    if (request.method == "OPTIONS") {
        s_http_response resp;
        resp.status_code = 200;
        resp.body = "";
        return resp;
    }

    // Linear scan through routes (78 routes, negligible cost)
    for (const auto& route : m_routes) {
        if (route.method == request.method && route.path == request.path) {
            try {
                return route.handler(request);
            } catch (const std::exception& e) {
                return s_http_response::internal_error(
                    std::string("Handler exception: ") + e.what()
                );
            } catch (...) {
                return s_http_response::internal_error("Unknown handler exception");
            }
        }
    }

    return s_http_response::not_found(
        "No route for " + request.method + " " + request.path
    );
}
