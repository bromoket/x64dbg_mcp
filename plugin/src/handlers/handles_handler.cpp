#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"
#include "bridgelist.h"

namespace handlers {

void register_handles_routes(c_http_router& router) {
    // GET /api/handles/list - List handles in debugged process
    router.get("/api/handles/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        BridgeList<HANDLEINFO> handles;
        auto success = DbgFunctions()->EnumHandles(&handles);

        if (!success) {
            return s_http_response::ok({
                {"handles", nlohmann::json::array()},
                {"count", 0}
            });
        }

        auto result = nlohmann::json::array();
        for (int i = 0; i < handles.Count(); ++i) {
            char name[256] = {};
            char type_name[256] = {};
            DbgFunctions()->GetHandleName(handles[i].Handle, name, sizeof(name), type_name, sizeof(type_name));

            result.push_back({
                {"handle",         format_utils::format_address(handles[i].Handle)},
                {"type_number",    handles[i].TypeNumber},
                {"granted_access", format_utils::format_address(handles[i].GrantedAccess)},
                {"name",           std::string(name)},
                {"type_name",      std::string(type_name)}
            });
        }

        return s_http_response::ok({
            {"handles", result},
            {"count", result.size()}
        });
    });

    // GET /api/handles/get?handle= - Get handle name and type
    router.get("/api/handles/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto handle_str = req.get_query("handle");
        if (handle_str.empty()) {
            return s_http_response::bad_request("Missing 'handle' query parameter");
        }

        auto handle = bridge.eval_expression(handle_str);

        char name[256] = {};
        char type_name[256] = {};
        auto success = DbgFunctions()->GetHandleName(handle, name, sizeof(name), type_name, sizeof(type_name));

        return s_http_response::ok({
            {"handle",    format_utils::format_address(handle)},
            {"name",      std::string(name)},
            {"type_name", std::string(type_name)},
            {"found",     success}
        });
    });

    // GET /api/handles/tcp - List TCP connections
    router.get("/api/handles/tcp", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        BridgeList<TCPCONNECTIONINFO> connections;
        auto success = DbgFunctions()->EnumTcpConnections(&connections);

        if (!success) {
            return s_http_response::ok({
                {"connections", nlohmann::json::array()},
                {"count", 0}
            });
        }

        auto result = nlohmann::json::array();
        for (int i = 0; i < connections.Count(); ++i) {
            result.push_back({
                {"remote_address", std::string(connections[i].RemoteAddress)},
                {"remote_port",    connections[i].RemotePort},
                {"local_address",  std::string(connections[i].LocalAddress)},
                {"local_port",     connections[i].LocalPort},
                {"state_text",     std::string(connections[i].StateText)},
                {"state",          connections[i].State}
            });
        }

        return s_http_response::ok({
            {"connections", result},
            {"count", result.size()}
        });
    });

    // GET /api/handles/windows - List windows
    router.get("/api/handles/windows", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        BridgeList<WINDOW_INFO> windows;
        auto success = DbgFunctions()->EnumWindows(&windows);

        if (!success) {
            return s_http_response::ok({
                {"windows", nlohmann::json::array()},
                {"count", 0}
            });
        }

        auto result = nlohmann::json::array();
        for (int i = 0; i < windows.Count(); ++i) {
            result.push_back({
                {"handle",       format_utils::format_address(windows[i].handle)},
                {"parent",       format_utils::format_address(windows[i].parent)},
                {"thread_id",    windows[i].threadId},
                {"style",        format_utils::format_address(windows[i].style)},
                {"style_ex",     format_utils::format_address(windows[i].styleEx)},
                {"wnd_proc",     format_utils::format_address(windows[i].wndProc)},
                {"enabled",      windows[i].enabled},
                {"title",        std::string(windows[i].windowTitle)},
                {"class_name",   std::string(windows[i].windowClass)}
            });
        }

        return s_http_response::ok({
            {"windows", result},
            {"count", result.size()}
        });
    });

    // GET /api/handles/heaps - List heaps
    router.get("/api/handles/heaps", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        BridgeList<HEAPINFO> heaps;
        auto success = DbgFunctions()->EnumHeaps(&heaps);

        if (!success) {
            return s_http_response::ok({
                {"heaps", nlohmann::json::array()},
                {"count", 0}
            });
        }

        auto result = nlohmann::json::array();
        for (int i = 0; i < heaps.Count(); ++i) {
            result.push_back({
                {"address", format_utils::format_address(heaps[i].addr)},
                {"size",    heaps[i].size},
                {"flags",   format_utils::format_address(heaps[i].flags)}
            });
        }

        return s_http_response::ok({
            {"heaps", result},
            {"count", result.size()}
        });
    });

    // POST /api/handles/close - Close a handle
    router.post("/api/handles/close", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("handle")) {
            return s_http_response::bad_request("Missing 'handle' field");
        }

        auto handle = body["handle"].get<std::string>();
        auto cmd = "HandleClose " + handle;
        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"handle", handle}
        });
    });
}

} // namespace handlers
