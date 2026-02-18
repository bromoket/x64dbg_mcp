#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

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
