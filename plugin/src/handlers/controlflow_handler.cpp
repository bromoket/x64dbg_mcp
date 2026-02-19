#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"
#include "bridgegraph.h"

namespace handlers {

void register_controlflow_routes(c_http_router& router) {
    // GET /api/cfg/function?address= - Get control flow graph
    router.get("/api/cfg/function", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        BridgeCFGraphList graph_list{};
        if (!DbgAnalyzeFunction(address, &graph_list)) {
            return s_http_response::not_found("Failed to analyze function at " + address_str);
        }

        BridgeCFGraph graph(&graph_list, true);

        auto nodes = nlohmann::json::array();
        for (const auto& [start, node] : graph.nodes) {
            auto exits = nlohmann::json::array();
            for (auto exit_addr : node.exits) {
                exits.push_back(format_utils::format_address(exit_addr));
            }

            auto instrs = nlohmann::json::array();
            for (const auto& instr : node.instrs) {
                instrs.push_back({
                    {"address", format_utils::format_address(instr.addr)},
                    {"data",    format_utils::format_bytes_hex(instr.data, sizeof(instr.data))}
                });
            }

            nodes.push_back({
                {"start",          format_utils::format_address(node.start)},
                {"end",            format_utils::format_address(node.end)},
                {"brtrue",         format_utils::format_address(node.brtrue)},
                {"brfalse",        format_utils::format_address(node.brfalse)},
                {"terminal",       node.terminal},
                {"split",          node.split},
                {"indirectcall",   node.indirectcall},
                {"exits",          exits},
                {"instructions",   instrs}
            });
        }

        return s_http_response::ok({
            {"entry_point", format_utils::format_address(graph.entryPoint)},
            {"nodes",       nodes},
            {"node_count",  nodes.size()}
        });
    });

    // GET /api/cfg/branch_dest?address= - Get branch destination
    router.get("/api/cfg/branch_dest", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);
        auto dest = DbgGetBranchDestination(address);

        auto label = bridge.get_label_at(dest);
        auto module_name = bridge.get_module_at(dest);

        return s_http_response::ok({
            {"address",     format_utils::format_address(address)},
            {"destination", format_utils::format_address(dest)},
            {"label",       label},
            {"module",      module_name},
            {"has_dest",    dest != 0}
        });
    });

    // GET /api/cfg/is_jump_taken?address= - Will jump execute?
    router.get("/api/cfg/is_jump_taken", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);
        auto will_execute = DbgIsJumpGoingToExecute(address);

        return s_http_response::ok({
            {"address",      format_utils::format_address(address)},
            {"will_execute", will_execute}
        });
    });

    // GET /api/cfg/loops?address= - Get loop info
    router.get("/api/cfg/loops", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);

        auto loops = nlohmann::json::array();
        for (int depth = 0; depth < 10; ++depth) {
            duint loop_start = 0, loop_end = 0;
            if (!DbgLoopGet(depth, address, &loop_start, &loop_end)) {
                break;
            }
            loops.push_back({
                {"depth", depth},
                {"start", format_utils::format_address(loop_start)},
                {"end",   format_utils::format_address(loop_end)},
                {"size",  loop_end - loop_start}
            });
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"loops",   loops},
            {"count",   loops.size()}
        });
    });

    // POST /api/cfg/add_function - Define a function
    router.post("/api/cfg/add_function", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("start") || !body.contains("end")) {
            return s_http_response::bad_request("Missing 'start' and/or 'end' fields");
        }

        auto start = bridge.eval_expression(body["start"].get<std::string>());
        auto end = bridge.eval_expression(body["end"].get<std::string>());

        auto success = DbgFunctionAdd(start, end);

        return s_http_response::ok({
            {"success", success},
            {"start",   format_utils::format_address(start)},
            {"end",     format_utils::format_address(end)}
        });
    });

    // POST /api/cfg/delete_function - Delete a function definition
    router.post("/api/cfg/delete_function", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto success = DbgFunctionDel(address);

        return s_http_response::ok({
            {"success", success},
            {"address", format_utils::format_address(address)}
        });
    });

    // GET /api/cfg/func_type?address= - Get function type at address
    router.get("/api/cfg/func_type", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto address_str = req.get_query("address", "cip");
        auto address = bridge.eval_expression(address_str);
        auto func_type = DbgGetFunctionTypeAt(address);

        std::string type_str;
        switch (func_type) {
            case FUNC_NONE:   type_str = "none"; break;
            case FUNC_BEGIN:  type_str = "begin"; break;
            case FUNC_MIDDLE: type_str = "middle"; break;
            case FUNC_END:    type_str = "end"; break;
            case FUNC_SINGLE: type_str = "single"; break;
            default:          type_str = "unknown"; break;
        }

        return s_http_response::ok({
            {"address",   format_utils::format_address(address)},
            {"func_type", type_str},
            {"type_id",   static_cast<int>(func_type)}
        });
    });
}

} // namespace handlers
