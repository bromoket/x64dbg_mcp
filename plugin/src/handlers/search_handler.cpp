#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_search_routes(c_http_router& router) {
    // POST /api/search/pattern - AOB/byte pattern scan
    router.post("/api/search/pattern", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("pattern")) {
            return s_http_response::bad_request("Missing 'pattern' field");
        }

        auto pattern = body["pattern"].get<std::string>();
        auto address_str = body.value("address", "0");
        auto size_str = body.value("size", "0");

        // Use x64dbg find command
        std::string cmd;
        if (address_str == "0" && size_str == "0") {
            // Search entire memory
            cmd = "findall 0, " + pattern;
        } else {
            cmd = "find " + address_str + ", " + pattern;
            if (size_str != "0") {
                cmd += ", " + size_str;
            }
        }

        bridge.exec_command(cmd);

        // Get the first result via $result
        auto result_addr = bridge.eval_expression("$result");

        nlohmann::json data = {
            {"pattern",      pattern},
            {"first_match",  result_addr != 0 ? format_utils::format_address(result_addr) : ""},
            {"found",        result_addr != 0}
        };

        if (result_addr != 0) {
            data["message"] = "Pattern found. Use 'findall' in x64dbg to see all matches in the references view.";
        }

        return s_http_response::ok(data);
    });

    // POST /api/search/string - String search
    router.post("/api/search/string", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("text")) {
            return s_http_response::bad_request("Missing 'text' field");
        }

        auto text = body["text"].get<std::string>();
        auto module_name = body.value("module", "");
        auto encoding = body.value("encoding", "utf8"); // utf8, ascii, unicode

        // Convert string to byte pattern
        std::string byte_pattern;
        if (encoding == "unicode" || encoding == "utf16") {
            // UTF-16LE encoding
            for (char c : text) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02X 00 ", static_cast<unsigned char>(c));
                byte_pattern += buf;
            }
        } else {
            // ASCII / UTF-8
            for (char c : text) {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X ", static_cast<unsigned char>(c));
                byte_pattern += buf;
            }
        }

        // Trim trailing space
        if (!byte_pattern.empty() && byte_pattern.back() == ' ') {
            byte_pattern.pop_back();
        }

        std::string cmd;
        if (!module_name.empty()) {
            auto base = bridge.get_module_base(module_name);
            auto size = bridge.eval_expression("mod.size(" + module_name + ")");
            cmd = "find " + format_utils::format_address(base) + ", " + byte_pattern + ", " + format_utils::format_hex(size);
        } else {
            cmd = "findall 0, " + byte_pattern;
        }

        bridge.exec_command(cmd);
        auto result_addr = bridge.eval_expression("$result");

        return s_http_response::ok({
            {"text",        text},
            {"encoding",    encoding},
            {"pattern",     byte_pattern},
            {"first_match", result_addr != 0 ? format_utils::format_address(result_addr) : ""},
            {"found",       result_addr != 0}
        });
    });
}

} // namespace handlers
