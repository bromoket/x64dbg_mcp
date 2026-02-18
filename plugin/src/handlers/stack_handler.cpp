#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_stack_routes(c_http_router& router) {
    // GET /api/stack/trace?max_depth=50 - Call stack
    router.get("/api/stack/trace", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        DBGCALLSTACK callstack{};
        DbgFunctions()->GetCallStackEx(&callstack, false);

        auto frames = nlohmann::json::array();
        for (int i = 0; i < callstack.total; ++i) {
            const auto& entry = callstack.entries[i];
            auto label = bridge.get_label_at(entry.to);
            auto module_name = bridge.get_module_at(entry.to);

            frames.push_back({
                {"index",   i},
                {"address", format_utils::format_address(entry.addr)},
                {"from",    format_utils::format_address(entry.from)},
                {"to",      format_utils::format_address(entry.to)},
                {"label",   label},
                {"module",  module_name},
                {"comment", entry.comment}
            });
        }

        if (callstack.entries) {
            BridgeFree(callstack.entries);
        }

        return s_http_response::ok({
            {"frames", frames},
            {"count",  frames.size()}
        });
    });

    // GET /api/stack/read?address=0x...&size=N - Read stack memory
    router.get("/api/stack/read", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "csp");
        auto size_str = req.get_query("size", "256");

        auto address = bridge.eval_expression(address_str);
        auto size = static_cast<size_t>(std::stoull(size_str));

        auto result = bridge.read_memory(address, size);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        const auto& bytes = result.value();

        // Build pointer-sized entries
        auto entries = nlohmann::json::array();
        auto ptr_size = sizeof(duint);
        for (size_t offset = 0; offset + ptr_size <= bytes.size(); offset += ptr_size) {
            duint value = 0;
            memcpy(&value, bytes.data() + offset, ptr_size);

            auto entry_addr = address + offset;
            auto label = bridge.get_label_at(value);
            auto module_name = bridge.get_module_at(value);

            entries.push_back({
                {"address", format_utils::format_address(entry_addr)},
                {"value",   format_utils::format_address(value)},
                {"label",   label},
                {"module",  module_name}
            });
        }

        return s_http_response::ok({
            {"base",    format_utils::format_address(address)},
            {"size",    bytes.size()},
            {"entries", entries}
        });
    });

    // GET /api/stack/pointers - RSP/RBP values
    router.get("/api/stack/pointers", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto csp = bridge.eval_expression("csp");
        auto cbp = bridge.eval_expression("cbp");

        return s_http_response::ok({
#ifdef _WIN64
            {"rsp", format_utils::format_address(csp)},
            {"rbp", format_utils::format_address(cbp)},
#else
            {"esp", format_utils::format_address(csp)},
            {"ebp", format_utils::format_address(cbp)},
#endif
        });
    });

    // GET /api/stack/seh_chain - SEH handler chain
    router.get("/api/stack/seh_chain", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        DBGSEHCHAIN seh_chain{};
        DbgFunctions()->GetSEHChain(&seh_chain);

        auto chain = nlohmann::json::array();
        for (duint i = 0; i < seh_chain.total; ++i) {
            const auto& record = seh_chain.records[i];
            auto label = bridge.get_label_at(record.handler);
            auto module_name = bridge.get_module_at(record.handler);

            chain.push_back({
                {"address", format_utils::format_address(record.addr)},
                {"handler", format_utils::format_address(record.handler)},
                {"label",   label},
                {"module",  module_name}
            });
        }

        if (seh_chain.records) {
            BridgeFree(seh_chain.records);
        }

        return s_http_response::ok({
            {"chain", chain},
            {"count", chain.size()}
        });
    });
}

} // namespace handlers
