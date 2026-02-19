#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_thread_routes(c_http_router& router) {
    // GET /api/threads/list - List all threads
    router.get("/api/threads/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto result = bridge.get_thread_list();
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok(result.value());
    });

    // GET /api/threads/current - Current thread info
    router.get("/api/threads/current", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto result = bridge.get_thread_list();
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        auto current_idx = result.value()["current_thread"].get<int>();
        const auto& threads = result.value()["threads"];

        for (const auto& t : threads) {
            if (t["number"].get<int>() == current_idx) {
                return s_http_response::ok(t);
            }
        }

        // Fallback: return first thread
        if (!threads.empty()) {
            return s_http_response::ok(threads[0]);
        }

        return s_http_response::not_found("No current thread");
    });

    // GET /api/threads/get?id=N - Specific thread by ID
    router.get("/api/threads/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto id_str = req.get_query("id");
        if (id_str.empty()) {
            return s_http_response::bad_request("Missing 'id' query parameter");
        }

        auto tid = std::stoul(id_str);
        auto result = bridge.get_thread_list();
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        for (const auto& t : result.value()["threads"]) {
            if (t["id"].get<DWORD>() == tid) {
                return s_http_response::ok(t);
            }
        }

        return s_http_response::not_found("Thread not found: " + id_str);
    });

    // POST /api/threads/switch - Switch active thread
    router.post("/api/threads/switch", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("id")) {
            return s_http_response::bad_request("Missing 'id' field");
        }

        auto tid = body["id"].get<DWORD>();
        auto cmd = "switchthread " + std::to_string(tid);
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"switched_to", tid},
            {"message",     "Switched to thread " + std::to_string(tid)}
        });
    });

    // POST /api/threads/suspend - Suspend thread
    router.post("/api/threads/suspend", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("id")) {
            return s_http_response::bad_request("Missing 'id' field");
        }

        auto tid = body["id"].get<DWORD>();
        auto cmd = "suspendthread " + std::to_string(tid);
        bridge.exec_command(cmd);

        return s_http_response::ok({{"id", tid}, {"suspended", true}});
    });

    // POST /api/threads/resume - Resume thread
    router.post("/api/threads/resume", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("id")) {
            return s_http_response::bad_request("Missing 'id' field");
        }

        auto tid = body["id"].get<DWORD>();
        auto cmd = "resumethread " + std::to_string(tid);
        bridge.exec_command(cmd);

        return s_http_response::ok({{"id", tid}, {"resumed", true}});
    });

    // GET /api/threads/count - Thread count
    router.get("/api/threads/count", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto result = bridge.get_thread_list();
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok({{"count", result.value()["count"]}});
    });

    // GET /api/threads/teb?tid= - Get TEB address for thread
    router.get("/api/threads/teb", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto tid_str = req.get_query("tid");
        if (tid_str.empty()) {
            return s_http_response::bad_request("Missing 'tid' query parameter");
        }

        auto tid = static_cast<DWORD>(std::stoul(tid_str));
        auto teb = DbgGetTebAddress(tid);

        return s_http_response::ok({
            {"tid", tid},
            {"teb", format_utils::format_address(teb)},
            {"found", teb != 0}
        });
    });

    // GET /api/threads/name?tid= - Get thread name
    router.get("/api/threads/name", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto tid_str = req.get_query("tid");
        if (tid_str.empty()) {
            return s_http_response::bad_request("Missing 'tid' query parameter");
        }

        auto tid = static_cast<DWORD>(std::stoul(tid_str));
        char name[MAX_THREAD_NAME_SIZE] = {};
        auto found = DbgFunctions()->ThreadGetName(tid, name);

        return s_http_response::ok({
            {"tid", tid},
            {"name", std::string(name)},
            {"found", found}
        });
    });
}

} // namespace handlers
