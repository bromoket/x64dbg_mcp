#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_antidebug_routes(c_http_router& router) {
    // GET /api/antidebug/peb?pid= - Read PEB info
    router.get("/api/antidebug/peb", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pid_str = req.get_query("pid", "");
        DWORD pid = 0;
        if (pid_str.empty()) {
            pid = static_cast<DWORD>(bridge.eval_expression("$pid"));
        } else {
            pid = static_cast<DWORD>(std::stoul(pid_str));
        }

        auto peb_addr = DbgGetPebAddress(pid);
        if (peb_addr == 0) {
            return s_http_response::not_found("Failed to get PEB address");
        }

        nlohmann::json data = {
            {"peb_address", format_utils::format_address(peb_addr)},
            {"pid", pid}
        };

        // Read BeingDebugged (offset 0x2 in PEB, 1 byte)
        auto being_debugged = bridge.read_memory(peb_addr + 0x2, 1);
        if (being_debugged.has_value()) {
            data["being_debugged"] = being_debugged.value()[0];
        }

        // Read NtGlobalFlag (offset 0x68 on x86, 0xBC on x64)
#ifdef _WIN64
        constexpr duint ntglobalflag_offset = 0xBC;
#else
        constexpr duint ntglobalflag_offset = 0x68;
#endif
        auto ntglobal = bridge.read_memory(peb_addr + ntglobalflag_offset, 4);
        if (ntglobal.has_value()) {
            DWORD flags = 0;
            memcpy(&flags, ntglobal.value().data(), 4);
            data["nt_global_flag"] = format_utils::format_address(flags);
            data["nt_global_flag_decimal"] = flags;
        }

        // Read ProcessHeap (offset 0x18 on x86, 0x30 on x64)
#ifdef _WIN64
        constexpr duint heap_offset = 0x30;
#else
        constexpr duint heap_offset = 0x18;
#endif
        auto heap_data = bridge.read_memory(peb_addr + heap_offset, sizeof(duint));
        if (heap_data.has_value()) {
            duint heap_addr = 0;
            memcpy(&heap_addr, heap_data.value().data(), sizeof(duint));
            data["process_heap"] = format_utils::format_address(heap_addr);
        }

        return s_http_response::ok(data);
    });

    // GET /api/antidebug/teb?tid= - Read TEB info
    router.get("/api/antidebug/teb", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto tid_str = req.get_query("tid", "");
        DWORD tid = 0;
        if (tid_str.empty()) {
            tid = static_cast<DWORD>(bridge.eval_expression("$tid"));
        } else {
            tid = static_cast<DWORD>(std::stoul(tid_str));
        }

        auto teb_addr = DbgGetTebAddress(tid);
        if (teb_addr == 0) {
            return s_http_response::not_found("Failed to get TEB address");
        }

        nlohmann::json data = {
            {"teb_address", format_utils::format_address(teb_addr)},
            {"tid", tid}
        };

        // Read SEH chain pointer (offset 0x0 in TEB)
        auto seh = bridge.read_memory(teb_addr, sizeof(duint));
        if (seh.has_value()) {
            duint seh_addr = 0;
            memcpy(&seh_addr, seh.value().data(), sizeof(duint));
            data["seh_frame"] = format_utils::format_address(seh_addr);
        }

        // Read stack base (offset 0x4/0x8) and limit (offset 0x8/0x10)
#ifdef _WIN64
        constexpr duint stack_base_offset = 0x8;
        constexpr duint stack_limit_offset = 0x10;
        constexpr duint peb_offset = 0x60;
#else
        constexpr duint stack_base_offset = 0x4;
        constexpr duint stack_limit_offset = 0x8;
        constexpr duint peb_offset = 0x30;
#endif

        auto stack_base = bridge.read_memory(teb_addr + stack_base_offset, sizeof(duint));
        if (stack_base.has_value()) {
            duint val = 0;
            memcpy(&val, stack_base.value().data(), sizeof(duint));
            data["stack_base"] = format_utils::format_address(val);
        }

        auto stack_limit = bridge.read_memory(teb_addr + stack_limit_offset, sizeof(duint));
        if (stack_limit.has_value()) {
            duint val = 0;
            memcpy(&val, stack_limit.value().data(), sizeof(duint));
            data["stack_limit"] = format_utils::format_address(val);
        }

        auto peb = bridge.read_memory(teb_addr + peb_offset, sizeof(duint));
        if (peb.has_value()) {
            duint val = 0;
            memcpy(&val, peb.value().data(), sizeof(duint));
            data["peb_address"] = format_utils::format_address(val);
        }

        return s_http_response::ok(data);
    });

    // POST /api/antidebug/hide_debugger - Hide debugger from PEB
    router.post("/api/antidebug/hide_debugger", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pid = static_cast<DWORD>(bridge.eval_expression("$pid"));
        auto peb_addr = DbgGetPebAddress(pid);
        if (peb_addr == 0) {
            return s_http_response::internal_error("Failed to get PEB address");
        }

        nlohmann::json changes = nlohmann::json::array();

        // Zero out BeingDebugged (PEB + 0x2)
        std::vector<uint8_t> zero_byte = {0x00};
        auto result = bridge.write_memory(peb_addr + 0x2, zero_byte);
        if (result.has_value()) {
            changes.push_back({{"field", "BeingDebugged"}, {"offset", "0x2"}, {"value", 0}});
        }

        // Zero out NtGlobalFlag
#ifdef _WIN64
        constexpr duint ntglobalflag_offset = 0xBC;
#else
        constexpr duint ntglobalflag_offset = 0x68;
#endif
        std::vector<uint8_t> zero_dword = {0x00, 0x00, 0x00, 0x00};
        result = bridge.write_memory(peb_addr + ntglobalflag_offset, zero_dword);
        if (result.has_value()) {
            changes.push_back({{"field", "NtGlobalFlag"}, {"offset", format_utils::format_hex(ntglobalflag_offset)}, {"value", 0}});
        }

        return s_http_response::ok({
            {"peb_address", format_utils::format_address(peb_addr)},
            {"changes", changes},
            {"message", "Debugger hidden from PEB checks"}
        });
    });

    // GET /api/antidebug/dep_status - DEP enabled status
    router.get("/api/antidebug/dep_status", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto dep_enabled = DbgFunctions()->IsDepEnabled();

        return s_http_response::ok({
            {"dep_enabled", dep_enabled}
        });
    });
}

} // namespace handlers
