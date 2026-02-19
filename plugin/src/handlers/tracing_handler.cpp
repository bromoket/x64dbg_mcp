#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_tracing_routes(c_http_router& router) {
    // POST /api/trace/into - Trace into (conditional)
    router.post("/api/trace/into", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto condition = body.value("condition", "");
        auto max_steps = body.value("max_steps", "");
        auto log_text = body.value("log_text", "");

        std::string cmd = "TraceIntoConditional";
        if (!condition.empty()) {
            cmd += " " + condition;
        }
        if (!max_steps.empty()) {
            cmd += ", " + max_steps;
        }
        if (!log_text.empty()) {
            cmd += ", " + log_text;
        }

        auto success = bridge.exec_command_async(cmd);

        return s_http_response::ok({
            {"success", success},
            {"command", cmd},
            {"message", "Trace into started (async)"}
        });
    });

    // POST /api/trace/over - Trace over (conditional)
    router.post("/api/trace/over", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto condition = body.value("condition", "");
        auto max_steps = body.value("max_steps", "");
        auto log_text = body.value("log_text", "");

        std::string cmd = "TraceOverConditional";
        if (!condition.empty()) {
            cmd += " " + condition;
        }
        if (!max_steps.empty()) {
            cmd += ", " + max_steps;
        }
        if (!log_text.empty()) {
            cmd += ", " + log_text;
        }

        auto success = bridge.exec_command_async(cmd);

        return s_http_response::ok({
            {"success", success},
            {"command", cmd},
            {"message", "Trace over started (async)"}
        });
    });

    // POST /api/trace/run - Run to user code (RunToParty 0)
    router.post("/api/trace/run", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto party = body.value("party", "0"); // 0=user, 1=system

        auto cmd = "RunToParty " + party;
        auto success = bridge.exec_command_async(cmd);

        return s_http_response::ok({
            {"success", success},
            {"command", cmd},
            {"message", "Run to party started (async)"}
        });
    });

    // POST /api/trace/stop - Stop trace recording
    router.post("/api/trace/stop", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto success = bridge.exec_command("StopRunTrace");

        return s_http_response::ok({
            {"success", success},
            {"message", "Trace stopped"}
        });
    });

    // GET /api/trace/record/hitcount?address= - Get trace record hit count
    router.get("/api/trace/record/hitcount", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto hit_count = DbgFunctions()->GetTraceRecordHitCount(address);

        return s_http_response::ok({
            {"address",   format_utils::format_address(address)},
            {"hit_count", hit_count}
        });
    });

    // GET /api/trace/record/type?address= - Get trace record byte type
    router.get("/api/trace/record/type", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto byte_type = DbgFunctions()->GetTraceRecordByteType(address);

        std::string type_str;
        switch (byte_type) {
            case InstructionBody:      type_str = "InstructionBody"; break;
            case InstructionHeading:   type_str = "InstructionHeading"; break;
            case InstructionTailing:   type_str = "InstructionTailing"; break;
            case InstructionOverlapped: type_str = "InstructionOverlapped"; break;
            case DataByte:             type_str = "DataByte"; break;
            case DataWord:             type_str = "DataWord"; break;
            case DataDWord:            type_str = "DataDWord"; break;
            case DataQWord:            type_str = "DataQWord"; break;
            case DataFloat:            type_str = "DataFloat"; break;
            case DataDouble:           type_str = "DataDouble"; break;
            case DataLongDouble:       type_str = "DataLongDouble"; break;
            case DataXMM:              type_str = "DataXMM"; break;
            case DataYMM:              type_str = "DataYMM"; break;
            case DataMMX:              type_str = "DataMMX"; break;
            case DataMixed:            type_str = "DataMixed"; break;
            case InstructionDataMixed: type_str = "InstructionDataMixed"; break;
            default:                   type_str = "Unknown"; break;
        }

        return s_http_response::ok({
            {"address",   format_utils::format_address(address)},
            {"type",      type_str},
            {"type_id",   static_cast<int>(byte_type)}
        });
    });

    // POST /api/trace/record/set_type - Set trace record type for a page
    router.post("/api/trace/record/set_type", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto type_id = body.value("type", 0);

        auto success = DbgFunctions()->SetTraceRecordType(address, static_cast<TRACERECORDTYPE>(type_id));

        return s_http_response::ok({
            {"success", success},
            {"address", format_utils::format_address(address)},
            {"type",    type_id}
        });
    });

    // POST /api/trace/animate - Animate command
    router.post("/api/trace/animate", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("command")) {
            return s_http_response::bad_request("Missing 'command' field");
        }

        auto command = body["command"].get<std::string>();
        auto success = DbgFunctions()->AnimateCommand(command.c_str());

        return s_http_response::ok({
            {"success", success},
            {"command", command}
        });
    });

    // POST /api/trace/conditional_run - Start a conditional trace run
    router.post("/api/trace/conditional_run", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto break_condition = body.value("break_condition", "");
        auto log_text = body.value("log_text", "");
        auto log_condition = body.value("log_condition", "");
        auto cmd_text = body.value("command_text", "");
        auto cmd_condition = body.value("command_condition", "");
        auto trace_type = body.value("type", "into"); // into or over

        std::string cmd;
        if (trace_type == "over") {
            cmd = "TraceOverConditional";
        } else {
            cmd = "TraceIntoConditional";
        }

        if (!break_condition.empty()) {
            cmd += " " + break_condition;
        }

        auto success = bridge.exec_command_async(cmd);

        return s_http_response::ok({
            {"success", success},
            {"command", cmd},
            {"type", trace_type},
            {"message", "Conditional trace started (async)"}
        });
    });

    // POST /api/trace/log - Setup trace logging
    router.post("/api/trace/log", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto file = body.value("file", "");
        auto text = body.value("text", "");
        auto condition = body.value("condition", "");

        std::string cmd;
        if (!file.empty()) {
            cmd = "StartRunTrace " + file;
            if (!text.empty()) {
                cmd += ", " + text;
            }
        } else {
            return s_http_response::bad_request("Missing 'file' field for trace log output");
        }

        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"command", cmd},
            {"file", file}
        });
    });
}

} // namespace handlers
