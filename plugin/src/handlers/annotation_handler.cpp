#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_annotation_routes(c_http_router& router) {
    // GET /api/labels/get?address=0x... - Get label
    router.get("/api/labels/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto label = bridge.get_label_at(address);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"label",   label}
        });
    });

    // POST /api/labels/set - Set label
    router.post("/api/labels/set", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("text")) {
            return s_http_response::bad_request("Missing 'address' and/or 'text' fields");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto text = body["text"].get<std::string>();

        if (!bridge.set_label_at(address, text)) {
            return s_http_response::internal_error("Failed to set label");
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"label",   text}
        });
    });

    // GET /api/comments/get?address=0x... - Get comment
    router.get("/api/comments/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto comment = bridge.get_comment_at(address);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"comment", comment}
        });
    });

    // POST /api/comments/set - Set comment
    router.post("/api/comments/set", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("text")) {
            return s_http_response::bad_request("Missing 'address' and/or 'text' fields");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto text = body["text"].get<std::string>();

        if (!bridge.set_comment_at(address, text)) {
            return s_http_response::internal_error("Failed to set comment");
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"comment", text}
        });
    });

    // POST /api/bookmarks/set - Set/clear bookmark
    router.post("/api/bookmarks/set", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto set = body.value("set", true);

        if (!bridge.set_bookmark_at(address, set)) {
            return s_http_response::internal_error("Failed to set bookmark");
        }

        return s_http_response::ok({
            {"address",    format_utils::format_address(address)},
            {"bookmarked", set}
        });
    });
}

} // namespace handlers
