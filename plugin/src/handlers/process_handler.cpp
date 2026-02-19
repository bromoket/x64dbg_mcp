#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_process_routes(c_http_router& router) {
    // GET /api/process/info - Extended process info
    // Note: basic /api/process/info exists in plugin_main.cpp,
    // this route is registered with a different path
    router.get("/api/process/details", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pid = bridge.eval_expression("$pid");
        auto peb = DbgGetPebAddress(static_cast<DWORD>(pid));
        auto process_handle = DbgGetProcessHandle();
        auto entry = bridge.eval_expression("mod.entry(0)");

        return s_http_response::ok({
            {"pid",              pid},
            {"peb_address",      format_utils::format_address(peb)},
            {"process_handle",   format_utils::format_address(reinterpret_cast<duint>(process_handle))},
            {"entry_point",      format_utils::format_address(entry)},
            {"debugger_state",   bridge.get_state_string()},
            {"is_elevated",      DbgFunctions()->IsProcessElevated()},
            {"dep_enabled",      DbgFunctions()->IsDepEnabled()}
        });
    });

    // GET /api/process/cmdline - Get command line
    router.get("/api/process/cmdline", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        // First call to get required size
        size_t size = 0;
        DbgFunctions()->GetCmdline(nullptr, &size);

        if (size == 0) {
            return s_http_response::ok({{"cmdline", ""}});
        }

        std::vector<char> buffer(size + 1, 0);
        DbgFunctions()->GetCmdline(buffer.data(), &size);

        return s_http_response::ok({
            {"cmdline", std::string(buffer.data())}
        });
    });

    // POST /api/process/set_cmdline - Set command line
    router.post("/api/process/set_cmdline", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("cmdline")) {
            return s_http_response::bad_request("Missing 'cmdline' field");
        }

        auto cmdline = body["cmdline"].get<std::string>();
        auto success = DbgFunctions()->SetCmdline(cmdline.c_str());

        return s_http_response::ok({
            {"success", success},
            {"cmdline", cmdline}
        });
    });

    // GET /api/process/elevated - Is process elevated
    router.get("/api/process/elevated", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        return s_http_response::ok({
            {"elevated", DbgFunctions()->IsProcessElevated()}
        });
    });

    // GET /api/process/dbversion - Debugger version
    router.get("/api/process/dbversion", [](const s_http_request&) -> s_http_response {
        auto version = BridgeGetDbgVersion();

        return s_http_response::ok({
            {"version", version}
        });
    });
}

} // namespace handlers
