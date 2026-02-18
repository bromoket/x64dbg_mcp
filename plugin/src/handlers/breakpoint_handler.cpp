#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_breakpoint_routes(c_http_router& router) {
    // GET /api/breakpoints/list - List all breakpoints
    router.get("/api/breakpoints/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();

        // Gather all breakpoint types
        nlohmann::json all_bps = nlohmann::json::array();

        auto normal = bridge.get_breakpoint_list(bp_normal);
        if (normal.has_value()) {
            for (auto& bp : normal.value()) {
                bp["type_name"] = "software";
                all_bps.push_back(bp);
            }
        }

        auto hardware = bridge.get_breakpoint_list(bp_hardware);
        if (hardware.has_value()) {
            for (auto& bp : hardware.value()) {
                bp["type_name"] = "hardware";
                all_bps.push_back(bp);
            }
        }

        auto memory = bridge.get_breakpoint_list(bp_memory);
        if (memory.has_value()) {
            for (auto& bp : memory.value()) {
                bp["type_name"] = "memory";
                all_bps.push_back(bp);
            }
        }

        return s_http_response::ok({
            {"breakpoints", all_bps},
            {"count",       all_bps.size()}
        });
    });

    // GET /api/breakpoints/get?address=0x... - Get breakpoint at address
    router.get("/api/breakpoints/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto addr_hex = format_utils::format_address(address);

        // Search all breakpoint types
        for (auto type : {bp_normal, bp_hardware, bp_memory}) {
            auto bps = bridge.get_breakpoint_list(type);
            if (!bps.has_value()) continue;
            for (const auto& bp : bps.value()) {
                if (bp["address"] == addr_hex) {
                    return s_http_response::ok(bp);
                }
            }
        }

        return s_http_response::not_found("No breakpoint at " + address_str);
    });

    // POST /api/breakpoints/set - Set software breakpoint
    router.post("/api/breakpoints/set", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto singleshot = body.value("singleshot", false);

        auto cmd = singleshot
            ? "bp " + address_str + ", ss"
            : "bp " + address_str;

        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address",    address_str},
            {"type",       "software"},
            {"singleshot", singleshot}
        });
    });

    // POST /api/breakpoints/set_hardware - Set hardware breakpoint
    router.post("/api/breakpoints/set_hardware", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto type = body.value("type", "x"); // r/w/x
        auto size = body.value("size", "1"); // 1/2/4/8

        auto cmd = "bphws " + address_str + ", " + type + ", " + size;
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address", address_str},
            {"type",    "hardware"},
            {"hw_type", type},
            {"hw_size", size}
        });
    });

    // POST /api/breakpoints/set_memory - Set memory breakpoint
    router.post("/api/breakpoints/set_memory", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto type = body.value("type", "a"); // a=access, r=read, w=write, x=execute

        auto cmd = "bpm " + address_str + ", " + type;
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address",  address_str},
            {"type",     "memory"},
            {"mem_type", type}
        });
    });

    // POST /api/breakpoints/delete - Delete breakpoint
    router.post("/api/breakpoints/delete", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto type = body.value("type", "software");

        std::string cmd;
        if (type == "hardware") {
            cmd = "bphwc " + address_str;
        } else if (type == "memory") {
            cmd = "bpmc " + address_str;
        } else {
            cmd = "bc " + address_str;
        }

        bridge.exec_command(cmd);
        return s_http_response::ok({
            {"address", address_str},
            {"deleted", true}
        });
    });

    // POST /api/breakpoints/enable - Enable breakpoint
    router.post("/api/breakpoints/enable", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        bridge.exec_command("bpe " + address_str);

        return s_http_response::ok({{"address", address_str}, {"enabled", true}});
    });

    // POST /api/breakpoints/disable - Disable breakpoint
    router.post("/api/breakpoints/disable", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        bridge.exec_command("bpd " + address_str);

        return s_http_response::ok({{"address", address_str}, {"enabled", false}});
    });

    // POST /api/breakpoints/toggle - Toggle breakpoint
    router.post("/api/breakpoints/toggle", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        bridge.exec_command("bptoggle " + address_str);

        return s_http_response::ok({{"address", address_str}, {"toggled", true}});
    });

    // POST /api/breakpoints/set_condition - Set breakpoint condition
    router.post("/api/breakpoints/set_condition", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("condition")) {
            return s_http_response::bad_request("Missing 'address' and/or 'condition' fields");
        }

        auto address_str = body["address"].get<std::string>();
        auto condition = body["condition"].get<std::string>();

        auto cmd = "SetBreakpointCondition " + address_str + ", \"" + condition + "\"";
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address",   address_str},
            {"condition", condition}
        });
    });

    // POST /api/breakpoints/set_log - Set breakpoint log message
    router.post("/api/breakpoints/set_log", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("text")) {
            return s_http_response::bad_request("Missing 'address' and/or 'text' fields");
        }

        auto address_str = body["address"].get<std::string>();
        auto text = body["text"].get<std::string>();

        auto cmd = "SetBreakpointLog " + address_str + ", \"" + text + "\"";
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address", address_str},
            {"log",     text}
        });
    });
}

} // namespace handlers
