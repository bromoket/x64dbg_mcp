#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "_dbgfunctions.h"

namespace handlers {

void register_command_routes(c_http_router& router) {
    // POST /api/command/exec - Execute x64dbg command
    router.post("/api/command/exec", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("command")) {
            return s_http_response::bad_request("Missing 'command' field");
        }

        auto command = body["command"].get<std::string>();
        auto success = bridge.exec_command(command);

        return s_http_response::ok({
            {"command", command},
            {"success", success}
        });
    });

    // POST /api/command/eval - Evaluate expression
    router.post("/api/command/eval", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("expression")) {
            return s_http_response::bad_request("Missing 'expression' field");
        }

        auto expression = body["expression"].get<std::string>();

        if (!bridge.is_valid_expression(expression)) {
            return s_http_response::bad_request("Invalid expression: " + expression);
        }

        auto result = bridge.eval_expression(expression);

        return s_http_response::ok({
            {"expression", expression},
            {"value",      format_utils::format_address(result)},
            {"decimal",    result}
        });
    });

    // POST /api/command/format - Format string using x64dbg expression engine
    router.post("/api/command/format", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("format")) {
            return s_http_response::bad_request("Missing 'format' field");
        }

        auto fmt = body["format"].get<std::string>();
        char result[1024] = {};
        auto success = DbgFunctions()->StringFormatInline(fmt.c_str(), sizeof(result), result);

        return s_http_response::ok({
            {"success", success},
            {"format",  fmt},
            {"result",  std::string(result)}
        });
    });

    // GET /api/command/events - Get debug event count
    router.get("/api/command/events", [](const s_http_request&) -> s_http_response {
        auto events = DbgFunctions()->GetDbgEvents();

        return s_http_response::ok({
            {"event_count", events}
        });
    });

    // POST /api/command/init_script - Set debuggee init script
    router.post("/api/command/init_script", [](const s_http_request& req) -> s_http_response {
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("file")) {
            return s_http_response::bad_request("Missing 'file' field");
        }

        auto file = body["file"].get<std::string>();
        DbgFunctions()->DbgSetDebuggeeInitScript(file.c_str());

        return s_http_response::ok({
            {"file", file},
            {"message", "Init script set"}
        });
    });

    // GET /api/command/init_script - Get debuggee init script
    router.get("/api/command/init_script", [](const s_http_request&) -> s_http_response {
        auto* script = DbgFunctions()->DbgGetDebuggeeInitScript();

        return s_http_response::ok({
            {"file", script ? std::string(script) : ""}
        });
    });

    // GET /api/command/hash - Get database hash
    router.get("/api/command/hash", [](const s_http_request&) -> s_http_response {
        auto hash = DbgFunctions()->DbGetHash();

        return s_http_response::ok({
            {"hash", format_utils::format_address(hash)}
        });
    });

    // POST /api/command/script - Execute batch of commands
    router.post("/api/command/script", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("commands")) {
            return s_http_response::bad_request("Missing 'commands' field (array of strings)");
        }

        auto commands = body["commands"];
        if (!commands.is_array()) {
            return s_http_response::bad_request("'commands' must be an array of strings");
        }

        auto results = nlohmann::json::array();
        int succeeded = 0;
        int failed = 0;

        for (const auto& cmd : commands) {
            auto cmd_str = cmd.get<std::string>();
            auto success = bridge.exec_command(cmd_str);

            results.push_back({
                {"command", cmd_str},
                {"success", success}
            });

            if (success) ++succeeded;
            else ++failed;
        }

        return s_http_response::ok({
            {"results",   results},
            {"total",     commands.size()},
            {"succeeded", succeeded},
            {"failed",    failed}
        });
    });
}

} // namespace handlers
