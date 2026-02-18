#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_disasm_routes(c_http_router& router) {
    // GET /api/disasm/at?address=0x...&count=10 - Disassemble N instructions
    router.get("/api/disasm/at", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto count_str = req.get_query("count", "10");

        auto address = bridge.eval_expression(address_str);
        auto count = std::stoi(count_str);

        if (count < 1) count = 1;
        if (count > 1000) count = 1000;

        auto result = bridge.disassemble_at(address, count);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok({
            {"address",      format_utils::format_address(address)},
            {"count",        result->size()},
            {"instructions", result.value()}
        });
    });

    // GET /api/disasm/function?address=0x... - Disassemble entire function
    router.get("/api/disasm/function", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        // Get function boundaries
        auto bounds = bridge.get_function_bounds(address);
        if (!bounds.has_value()) {
            // If no function found, just disassemble 50 instructions
            auto result = bridge.disassemble_at(address, 50);
            if (!result.has_value()) {
                return s_http_response::internal_error(result.error());
            }
            return s_http_response::ok({
                {"address",      format_utils::format_address(address)},
                {"note",         "No function boundary found, showing 50 instructions"},
                {"instructions", result.value()}
            });
        }

        auto start = format_utils::parse_address(bounds.value()["start"].get<std::string>());
        auto end_addr = format_utils::parse_address(bounds.value()["end"].get<std::string>());

        // Estimate instruction count (average ~4 bytes per instruction)
        auto estimated_count = static_cast<int>((end_addr - start) / 2) + 1;
        if (estimated_count > 5000) estimated_count = 5000;

        auto result = bridge.disassemble_at(start, estimated_count);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        // Filter to only instructions within the function
        auto filtered = nlohmann::json::array();
        for (const auto& instr : result.value()) {
            auto instr_addr = format_utils::parse_address(instr["address"].get<std::string>());
            if (instr_addr > end_addr) break;
            filtered.push_back(instr);
        }

        return s_http_response::ok({
            {"function_start", bounds.value()["start"]},
            {"function_end",   bounds.value()["end"]},
            {"function_size",  bounds.value()["size"]},
            {"count",          filtered.size()},
            {"instructions",   filtered}
        });
    });

    // GET /api/disasm/basic?address=0x... - Fast instruction info
    router.get("/api/disasm/basic", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        auto result = bridge.get_basic_info(address);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok(result.value());
    });

    // POST /api/disasm/assemble - Assemble instruction at address
    router.post("/api/disasm/assemble", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("instruction")) {
            return s_http_response::bad_request("Missing 'address' and/or 'instruction' fields");
        }

        auto address_str = body["address"].get<std::string>();
        auto instruction = body["instruction"].get<std::string>();

        auto cmd = "asm " + address_str + ", \"" + instruction + "\"";
        if (!bridge.exec_command(cmd)) {
            return s_http_response::internal_error("Failed to assemble instruction");
        }

        return s_http_response::ok({
            {"address",     address_str},
            {"instruction", instruction}
        });
    });
}

} // namespace handlers
