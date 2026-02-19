#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"
#include "bridgelist.h"

namespace handlers {

void register_exception_routes(c_http_router& router) {
    // POST /api/exceptions/set_bp - Set exception breakpoint
    router.post("/api/exceptions/set_bp", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("code")) {
            return s_http_response::bad_request("Missing 'code' field");
        }

        auto code = body["code"].get<std::string>();
        auto chance = body.value("chance", "first"); // first, second, all
        auto action = body.value("action", "break"); // break, log, command

        auto cmd = "SetExceptionBPX " + code;
        if (chance == "second") {
            cmd += ", 1"; // second chance
        }

        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"code", code},
            {"chance", chance}
        });
    });

    // POST /api/exceptions/delete_bp - Delete exception breakpoint
    router.post("/api/exceptions/delete_bp", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("code")) {
            return s_http_response::bad_request("Missing 'code' field");
        }

        auto code = body["code"].get<std::string>();
        auto cmd = "DeleteExceptionBPX " + code;
        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"code", code}
        });
    });

    // GET /api/exceptions/list_bps - List exception breakpoints
    router.get("/api/exceptions/list_bps", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto result = bridge.get_breakpoint_list(bp_exception);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok({
            {"breakpoints", result.value()},
            {"count", result.value().size()}
        });
    });

    // GET /api/exceptions/list_codes - List known exception codes
    router.get("/api/exceptions/list_codes", [](const s_http_request&) -> s_http_response {
        BridgeList<CONSTANTINFO> constants;
        DbgFunctions()->EnumExceptions(&constants);

        auto result = nlohmann::json::array();
        for (int i = 0; i < constants.Count(); ++i) {
            result.push_back({
                {"name",  constants[i].name},
                {"value", format_utils::format_address(constants[i].value)}
            });
        }

        return s_http_response::ok({
            {"exceptions", result},
            {"count", result.size()}
        });
    });

    // POST /api/exceptions/skip - Skip/pass current exception
    router.post("/api/exceptions/skip", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto success = bridge.exec_command("skip");

        return s_http_response::ok({
            {"success", success},
            {"message", "Exception skipped"}
        });
    });
}

} // namespace handlers
