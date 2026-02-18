#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_symbol_routes(c_http_router& router) {
    // GET /api/symbols/resolve?name=... - Name to address
    router.get("/api/symbols/resolve", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto name = req.get_query("name");
        if (name.empty()) {
            return s_http_response::bad_request("Missing 'name' query parameter");
        }

        if (!bridge.is_valid_expression(name)) {
            return s_http_response::not_found("Cannot resolve: " + name);
        }

        auto address = bridge.eval_expression(name);
        if (address == 0) {
            return s_http_response::not_found("Symbol not found: " + name);
        }

        return s_http_response::ok({
            {"name",    name},
            {"address", format_utils::format_address(address)}
        });
    });

    // GET /api/symbols/at?address=0x... - Address to name
    router.get("/api/symbols/at", [](const s_http_request& req) -> s_http_response {
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
        auto module_name = bridge.get_module_at(address);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"label",   label},
            {"module",  module_name}
        });
    });

    // GET /api/symbols/search?pattern=... - Search symbols by pattern
    router.get("/api/symbols/search", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pattern = req.get_query("pattern");
        auto module = req.get_query("module");
        if (pattern.empty()) {
            return s_http_response::bad_request("Missing 'pattern' query parameter");
        }

        // Use x64dbg's symfind command via expression evaluation
        // Build a filter expression
        auto search_expr = module.empty() ? pattern : module + "." + pattern;

        // Use the command to search and retrieve through eval
        auto cmd = "symfind " + search_expr;
        bridge.exec_command(cmd);

        // Since symfind outputs to log, we return what we can resolve
        return s_http_response::ok({
            {"pattern",  pattern},
            {"module",   module},
            {"message",  "Symbol search initiated. Check x64dbg symbol view for results."}
        });
    });

    // GET /api/symbols/list?module=... - List module symbols
    router.get("/api/symbols/list", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module = req.get_query("module");
        if (module.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module);
        }

        // Trigger symbol loading for the module
        bridge.exec_command("symload " + module);

        return s_http_response::ok({
            {"module",  module},
            {"base",    format_utils::format_address(base)},
            {"message", "Symbols loaded. Use symbol search to find specific symbols."}
        });
    });
}

} // namespace handlers
