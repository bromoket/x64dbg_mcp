#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

// Apply all configurable breakpoint fields present in the JSON body.
// Uses SetBreakpointXxx commands. Only sets fields that are explicitly provided.
// x64dbg expression syntax: use [addr] for dereference, NOT poi(addr).
static void apply_bp_config(c_bridge_executor& bridge, const std::string& addr_str,
                             const nlohmann::json& body) {
    if (body.contains("break_condition")) {
        auto cond = body["break_condition"].get<std::string>();
        bridge.exec_command("SetBreakpointCondition " + addr_str + ", \"" + cond + "\"");
    }
    if (body.contains("command_condition")) {
        auto cond = body["command_condition"].get<std::string>();
        bridge.exec_command("SetBreakpointCommandCondition " + addr_str + ", \"" + cond + "\"");
    }
    if (body.contains("command_text")) {
        auto cmd = body["command_text"].get<std::string>();
        bridge.exec_command("SetBreakpointCommand " + addr_str + ", \"" + cmd + "\"");
    }
    if (body.contains("log_text")) {
        auto log = body["log_text"].get<std::string>();
        bridge.exec_command("SetBreakpointLog " + addr_str + ", \"" + log + "\"");
    }
    if (body.contains("log_condition")) {
        auto cond = body["log_condition"].get<std::string>();
        bridge.exec_command("SetBreakpointLogCondition " + addr_str + ", \"" + cond + "\"");
    }
    if (body.contains("silent")) {
        auto silent = body["silent"].get<bool>();
        bridge.exec_command("SetBreakpointSilent " + addr_str + ", " + (silent ? "1" : "0"));
    }
    if (body.contains("fast_resume")) {
        auto fr = body["fast_resume"].get<bool>();
        bridge.exec_command("SetBreakpointFastResume " + addr_str + ", " + (fr ? "1" : "0"));
    }
    if (body.contains("name")) {
        auto name = body["name"].get<std::string>();
        bridge.exec_command("SetBreakpointName " + addr_str + ", \"" + name + "\"");
    }
}

void register_breakpoint_routes(c_http_router& router) {
    // GET /api/breakpoints/list - List all breakpoints (with symbol labels resolved)
    router.get("/api/breakpoints/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();

        nlohmann::json all_bps = nlohmann::json::array();

        auto add_bps_with_type = [&](BPXTYPE type, const char* type_name) {
            auto bps = bridge.get_breakpoint_list(type);
            if (!bps.has_value()) return;
            for (auto& bp : bps.value()) {
                // Resolve symbol label if name is empty
                if (bp["name"].get<std::string>().empty()) {
                    auto addr = bridge.eval_expression(bp["address"].get<std::string>());
                    auto label = bridge.get_label_at(addr);
                    bp["label"] = label;  // Symbol name (e.g. "FindWindowA")
                } else {
                    bp["label"] = bp["name"].get<std::string>();
                }
                bp["type_name"] = type_name;
                all_bps.push_back(bp);
            }
        };

        add_bps_with_type(bp_normal,   "software");
        add_bps_with_type(bp_hardware, "hardware");
        add_bps_with_type(bp_memory,   "memory");

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

        for (auto type : {bp_normal, bp_hardware, bp_memory}) {
            auto bps = bridge.get_breakpoint_list(type);
            if (!bps.has_value()) continue;
            for (auto& bp : bps.value()) {
                if (bp["address"] == addr_hex) {
                    // Resolve label
                    if (bp["name"].get<std::string>().empty()) {
                        bp["label"] = bridge.get_label_at(address);
                    } else {
                        bp["label"] = bp["name"].get<std::string>();
                    }
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

    // POST /api/breakpoints/set_condition - Set breakpoint break_condition
    // Note: this sets break_condition (controls whether to PAUSE), not command_condition.
    // Use /api/breakpoints/configure for all conditions in one call.
    router.post("/api/breakpoints/set_condition", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("condition")) {
            return s_http_response::bad_request("Missing 'address' and/or 'condition' fields");
        }

        auto address_str = body["address"].get<std::string>();
        auto condition = body["condition"].get<std::string>();

        // Note: x64dbg uses [addr] bracket syntax for memory dereference, NOT poi(addr)
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

    // POST /api/breakpoints/configure - Unified breakpoint configuration
    // Creates the BP if it does not exist, then applies all provided fields.
    // This replaces 6 separate calls (set + condition + command_condition + command_text + silent + fast_resume).
    //
    // Expression syntax note: x64dbg uses [addr] for memory dereference, NOT poi(addr).
    // Examples:
    //   break_condition: "0"                      (never pause - fast-resume style)
    //   command_condition: "[esp+8]==11"           (condition to run command)
    //   command_text: "eax=0;eip=[esp];esp=esp+C;run"  (command on BP hit)
    router.post("/api/breakpoints/configure", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto bp_type = body.value("bp_type", "software");  // software, hardware, memory

        // Set the breakpoint if it doesn't exist yet
        if (bp_type == "hardware") {
            auto hw_type = body.value("hw_type", "x");
            auto hw_size = body.value("hw_size", "1");
            bridge.exec_command("bphws " + address_str + ", " + hw_type + ", " + hw_size);
        } else if (bp_type == "memory") {
            auto mem_type = body.value("mem_type", "a");
            bridge.exec_command("bpm " + address_str + ", " + mem_type);
        } else {
            // Software BP
            auto singleshot = body.value("singleshot", false);
            if (singleshot) {
                bridge.exec_command("bp " + address_str + ", ss");
            } else {
                bridge.exec_command("bp " + address_str);
            }
        }

        // Apply all config fields
        apply_bp_config(bridge, address_str, body);

        return s_http_response::ok({
            {"address", address_str},
            {"bp_type", bp_type},
            {"configured", true}
        });
    });

    // POST /api/breakpoints/configure_batch - Batch configure multiple breakpoints
    // Accepts an array of configure objects. Reduces N*6 calls to 1 call.
    // Each entry has the same fields as /api/breakpoints/configure.
    router.post("/api/breakpoints/configure_batch", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("breakpoints")) {
            return s_http_response::bad_request("Missing 'breakpoints' array field");
        }

        auto& bps_json = body["breakpoints"];
        if (!bps_json.is_array()) {
            return s_http_response::bad_request("'breakpoints' must be an array");
        }

        auto results = nlohmann::json::array();
        int succeeded = 0;
        int failed = 0;

        for (const auto& entry : bps_json) {
            if (!entry.contains("address")) {
                results.push_back({{"error", "missing address"}, {"success", false}});
                ++failed;
                continue;
            }

            auto address_str = entry["address"].get<std::string>();
            auto bp_type = entry.value("bp_type", "software");

            // Set the BP
            if (bp_type == "hardware") {
                auto hw_type = entry.value("hw_type", "x");
                auto hw_size = entry.value("hw_size", "1");
                bridge.exec_command("bphws " + address_str + ", " + hw_type + ", " + hw_size);
            } else if (bp_type == "memory") {
                auto mem_type = entry.value("mem_type", "a");
                bridge.exec_command("bpm " + address_str + ", " + mem_type);
            } else {
                auto singleshot = entry.value("singleshot", false);
                if (singleshot) {
                    bridge.exec_command("bp " + address_str + ", ss");
                } else {
                    bridge.exec_command("bp " + address_str);
                }
            }

            // Apply config
            apply_bp_config(bridge, address_str, entry);

            results.push_back({{"address", address_str}, {"success", true}});
            ++succeeded;
        }

        return s_http_response::ok({
            {"results",   results},
            {"total",     bps_json.size()},
            {"succeeded", succeeded},
            {"failed",    failed}
        });
    });

    // POST /api/breakpoints/reset_hit_count - Reset hit counter for a breakpoint
    router.post("/api/breakpoints/reset_hit_count", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        bridge.exec_command("ResetBreakpointHitCount " + address_str);

        return s_http_response::ok({
            {"address",   address_str},
            {"hit_count", 0}
        });
    });
}

} // namespace handlers
