#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"

namespace handlers {

void register_analysis_routes(c_http_router& router) {
    // GET /api/analysis/function?address=0x... - Function boundaries
    router.get("/api/analysis/function", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        auto bounds = bridge.get_function_bounds(address);
        if (!bounds.has_value()) {
            return s_http_response::not_found("No function at " + address_str);
        }

        auto start_addr = format_utils::parse_address(bounds.value()["start"].get<std::string>());
        auto label = bridge.get_label_at(start_addr);
        auto module_name = bridge.get_module_at(start_addr);

        auto data = bounds.value();
        data["label"] = label;
        data["module"] = module_name;

        return s_http_response::ok(data);
    });

    // GET /api/analysis/xrefs_to?address=0x... - Cross-references to address
    router.get("/api/analysis/xrefs_to", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto xref_count = DbgGetXrefCountAt(address);

        auto xrefs = nlohmann::json::array();

        if (xref_count > 0) {
            XREF_INFO xref_info{};
            if (DbgXrefGet(address, &xref_info)) {
                for (duint i = 0; i < xref_info.refcount; ++i) {
                    const auto& ref = xref_info.references[i];
                    auto label = bridge.get_label_at(ref.addr);
                    auto module_name = bridge.get_module_at(ref.addr);

                    std::string type_str;
                    switch (ref.type) {
                        case XREF_CALL: type_str = "call"; break;
                        case XREF_JMP:  type_str = "jmp"; break;
                        case XREF_DATA: type_str = "data"; break;
                        default:        type_str = "unknown"; break;
                    }

                    xrefs.push_back({
                        {"address", format_utils::format_address(ref.addr)},
                        {"type",    type_str},
                        {"label",   label},
                        {"module",  module_name}
                    });
                }

                if (xref_info.references) {
                    BridgeFree(xref_info.references);
                }
            }
        }

        return s_http_response::ok({
            {"target", format_utils::format_address(address)},
            {"xrefs",  xrefs},
            {"count",  xrefs.size()}
        });
    });

    // GET /api/analysis/xrefs_from?address=0x... - Cross-references from address
    router.get("/api/analysis/xrefs_from", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);

        // Disassemble the instruction to find references
        auto basic = bridge.get_basic_info(address);
        if (!basic.has_value()) {
            return s_http_response::internal_error(basic.error());
        }

        auto refs = nlohmann::json::array();
        if (basic.value()["is_call"].get<bool>() || basic.value()["is_branch"].get<bool>()) {
            // Try to evaluate the target
            // x64dbg uses dis.branchexec(addr) and dis.branchtarget(addr) expressions
            auto target = bridge.eval_expression("dis.branchtarget(" + address_str + ")");
            if (target != 0) {
                auto label = bridge.get_label_at(target);
                auto module_name = bridge.get_module_at(target);

                refs.push_back({
                    {"address", format_utils::format_address(target)},
                    {"type",    basic.value()["is_call"].get<bool>() ? "call" : "branch"},
                    {"label",   label},
                    {"module",  module_name}
                });
            }
        }

        return s_http_response::ok({
            {"source", format_utils::format_address(address)},
            {"refs",   refs},
            {"count",  refs.size()}
        });
    });

    // GET /api/analysis/basic_blocks?address=0x... - CFG basic blocks
    router.get("/api/analysis/basic_blocks", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        // Get function boundaries first
        auto bounds = bridge.get_function_bounds(address);
        if (!bounds.has_value()) {
            return s_http_response::not_found("No function at " + address_str);
        }

        auto func_start = format_utils::parse_address(bounds.value()["start"].get<std::string>());
        auto func_end = format_utils::parse_address(bounds.value()["end"].get<std::string>());

        // Walk the function to identify basic blocks
        auto blocks = nlohmann::json::array();
        auto current_block_start = func_start;
        auto current_addr = func_start;

        while (current_addr <= func_end) {
            BASIC_INSTRUCTION_INFO info{};
            DbgDisasmFastAt(current_addr, &info); // Returns void
            if (info.size == 0) break;

            bool is_block_end = info.branch || info.call;

            // Check if next instruction starts a new block (e.g., is a branch target)
            if (is_block_end || current_addr + info.size > func_end) {
                blocks.push_back({
                    {"start", format_utils::format_address(current_block_start)},
                    {"end",   format_utils::format_address(current_addr)},
                    {"size",  current_addr + info.size - current_block_start}
                });
                current_block_start = current_addr + info.size;
            }

            current_addr += info.size;
        }

        return s_http_response::ok({
            {"function_start", bounds.value()["start"]},
            {"function_end",   bounds.value()["end"]},
            {"blocks",         blocks},
            {"count",          blocks.size()}
        });
    });

    // GET /api/analysis/strings?module=... - Find strings in module
    router.get("/api/analysis/strings", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module_name = req.get_query("module");
        if (module_name.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        // Use x64dbg's strref command
        auto cmd = "strref " + format_utils::format_address(base);
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"module",  module_name},
            {"base",    format_utils::format_address(base)},
            {"message", "String references displayed in x64dbg references view"}
        });
    });
}

} // namespace handlers
