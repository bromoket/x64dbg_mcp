#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"

namespace handlers {

void register_memmap_routes(c_http_router& router) {
    // GET /api/memmap/list - Full memory map
    router.get("/api/memmap/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto result = bridge.get_memory_map();
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok({
            {"regions", result.value()},
            {"count",   result->size()}
        });
    });

    // GET /api/memmap/at?address=0x... - Region containing address
    router.get("/api/memmap/at", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);

        // Find the region via DbgMemFindBaseAddr
        duint region_size = 0;
        auto base = DbgMemFindBaseAddr(address, &region_size);

        if (base == 0) {
            return s_http_response::not_found("No memory region at " + address_str);
        }

        auto module_name = bridge.get_module_at(base);

        return s_http_response::ok({
            {"address",     format_utils::format_address(address)},
            {"base",        format_utils::format_address(base)},
            {"region_size", region_size},
            {"module",      module_name}
        });
    });
}

} // namespace handlers
