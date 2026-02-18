#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_debug_routes(c_http_router& router) {
    // GET /api/debug/state - Current debugger state + CIP
    router.get("/api/debug/state", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        auto state = bridge.get_state_string();

        nlohmann::json data = {{"state", state}};

        if (bridge.is_debugging() && !bridge.is_running()) {
            auto cip = bridge.eval_expression("cip");
            data["cip"] = format_utils::format_address(cip);

            auto module_name = bridge.get_module_at(cip);
            if (!module_name.empty()) {
                data["module"] = module_name;
            }

            auto label = bridge.get_label_at(cip);
            if (!label.empty()) {
                data["label"] = label;
            }
        }

        return s_http_response::ok(data);
    });

    // POST /api/debug/run - Resume execution
    router.post("/api/debug/run", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        bridge.exec_command("run");
        return s_http_response::ok({{"message", "Execution resumed"}});
    });

    // POST /api/debug/pause - Pause execution
    router.post("/api/debug/pause", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.is_debugging()) {
            return s_http_response::conflict("No active debug session");
        }
        if (!bridge.is_running()) {
            return s_http_response::ok({{"message", "Already paused"}});
        }

        bridge.exec_command("pause");
        return s_http_response::ok({{"message", "Pause requested"}});
    });

    // POST /api/debug/step_into - Step into
    router.post("/api/debug/step_into", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        if (!bridge.exec_command_and_wait("StepInto")) {
            return s_http_response::internal_error("Step into timed out");
        }

        auto cip = bridge.eval_expression("cip");
        return s_http_response::ok({
            {"cip", format_utils::format_address(cip)},
            {"message", "Stepped into"}
        });
    });

    // POST /api/debug/step_over - Step over
    router.post("/api/debug/step_over", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        if (!bridge.exec_command_and_wait("StepOver")) {
            return s_http_response::internal_error("Step over timed out");
        }

        auto cip = bridge.eval_expression("cip");
        return s_http_response::ok({
            {"cip", format_utils::format_address(cip)},
            {"message", "Stepped over"}
        });
    });

    // POST /api/debug/step_out - Run to return
    router.post("/api/debug/step_out", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        if (!bridge.exec_command_and_wait("StepOut", 30000)) {
            return s_http_response::internal_error("Step out timed out");
        }

        auto cip = bridge.eval_expression("cip");
        return s_http_response::ok({
            {"cip", format_utils::format_address(cip)},
            {"message", "Stepped out"}
        });
    });

    // POST /api/debug/stop - Stop debugging
    router.post("/api/debug/stop", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.is_debugging()) {
            return s_http_response::ok({{"message", "Not debugging"}});
        }

        bridge.exec_command("stop");
        return s_http_response::ok({{"message", "Debug session stopped"}});
    });

    // POST /api/debug/restart - Restart debuggee
    router.post("/api/debug/restart", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.is_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        bridge.exec_command("restart");
        return s_http_response::ok({{"message", "Restart initiated"}});
    });

    // POST /api/debug/run_to - Run to specific address
    router.post("/api/debug/run_to", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto cmd = "bp " + address_str + ", ss";  // Single-shot breakpoint
        bridge.exec_command(cmd.c_str());
        bridge.exec_command("run");

        return s_http_response::ok({
            {"message", "Running to " + address_str},
            {"target", address_str}
        });
    });
}

} // namespace handlers
