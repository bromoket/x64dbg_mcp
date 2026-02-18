#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_module_routes(c_http_router& router) {
    // GET /api/modules/list - List loaded modules
    router.get("/api/modules/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        // Get memory map and extract unique modules
        auto memmap = bridge.get_memory_map();
        if (!memmap.has_value()) {
            return s_http_response::internal_error(memmap.error());
        }

        std::unordered_map<std::string, nlohmann::json> modules;
        for (const auto& page : memmap.value()) {
            auto info = page.value("info", "");
            if (info.empty()) continue;

            // Check if this looks like a module section
            auto base_str = page["base"].get<std::string>();
            auto base = format_utils::parse_address(base_str);
            auto mod_name = bridge.get_module_at(base);

            if (mod_name.empty()) continue;

            if (modules.find(mod_name) == modules.end()) {
                auto mod_base = bridge.get_module_base(mod_name);
                auto mod_size = bridge.eval_expression("mod.size(" + mod_name + ")");
                auto mod_entry = bridge.eval_expression("mod.entry(" + mod_name + ")");

                modules[mod_name] = {
                    {"name",  mod_name},
                    {"base",  format_utils::format_address(mod_base)},
                    {"size",  mod_size},
                    {"entry", format_utils::format_address(mod_entry)}
                };
            }
        }

        auto result = nlohmann::json::array();
        for (const auto& [name, info] : modules) {
            result.push_back(info);
        }

        return s_http_response::ok({
            {"modules", result},
            {"count",   result.size()}
        });
    });

    // GET /api/modules/get?name=... - Module info
    router.get("/api/modules/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto name = req.get_query("name");
        if (name.empty()) {
            return s_http_response::bad_request("Missing 'name' query parameter");
        }

        auto base = bridge.get_module_base(name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + name);
        }

        auto size = bridge.eval_expression("mod.size(" + name + ")");
        auto entry = bridge.eval_expression("mod.entry(" + name + ")");
        auto party = bridge.eval_expression("mod.party(" + name + ")");

        return s_http_response::ok({
            {"name",  name},
            {"base",  format_utils::format_address(base)},
            {"size",  size},
            {"entry", format_utils::format_address(entry)},
            {"party", static_cast<int>(party)} // 0=user, 1=system
        });
    });

    // GET /api/modules/base?name=... - Module base address
    router.get("/api/modules/base", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto name = req.get_query("name");
        if (name.empty()) {
            return s_http_response::bad_request("Missing 'name' query parameter");
        }

        auto base = bridge.get_module_base(name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + name);
        }

        return s_http_response::ok({
            {"name", name},
            {"base", format_utils::format_address(base)}
        });
    });
}

} // namespace handlers
