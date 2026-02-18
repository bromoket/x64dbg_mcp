#include "plugin_main.h"

#include <string>

#include "_plugins.h"
#include "http/c_http_server.h"
#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

// Forward declarations for handler registration functions
namespace handlers {
    void register_debug_routes(c_http_router& router);
    void register_register_routes(c_http_router& router);
    void register_memory_routes(c_http_router& router);
    void register_breakpoint_routes(c_http_router& router);
    void register_disasm_routes(c_http_router& router);
    void register_module_routes(c_http_router& router);
    void register_thread_routes(c_http_router& router);
    void register_stack_routes(c_http_router& router);
    void register_symbol_routes(c_http_router& router);
    void register_annotation_routes(c_http_router& router);
    void register_search_routes(c_http_router& router);
    void register_patch_routes(c_http_router& router);
    void register_memmap_routes(c_http_router& router);
    void register_command_routes(c_http_router& router);
    void register_analysis_routes(c_http_router& router);
} // namespace handlers

// Globals
static int g_plugin_handle = -1;
static c_http_server g_server;
static c_http_router g_router;

// ============================================================================
// Route registration
// ============================================================================

void register_all_routes(c_http_router& router) {
    // Health check endpoint
    router.get("/api/health", [](const s_http_request&) -> s_http_response {
        return s_http_response::ok({
            {"version", "1.0.0"},
            {"plugin",  PLUGIN_NAME},
            {"status",  "ok"}
        });
    });

    // Process info endpoint
    router.get("/api/process/info", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pid = bridge.eval_expression("$pid");
        auto peb = bridge.eval_expression("peb()");
        auto entry = bridge.eval_expression("mod.entry(0)");

        return s_http_response::ok({
            {"pid",           pid},
            {"peb",           format_utils::format_address(peb)},
            {"entry_point",   format_utils::format_address(entry)},
            {"debugger_state", bridge.get_state_string()}
        });
    });

    // Register all handler categories
    handlers::register_debug_routes(router);
    handlers::register_register_routes(router);
    handlers::register_memory_routes(router);
    handlers::register_breakpoint_routes(router);
    handlers::register_disasm_routes(router);
    handlers::register_module_routes(router);
    handlers::register_thread_routes(router);
    handlers::register_stack_routes(router);
    handlers::register_symbol_routes(router);
    handlers::register_annotation_routes(router);
    handlers::register_search_routes(router);
    handlers::register_patch_routes(router);
    handlers::register_memmap_routes(router);
    handlers::register_command_routes(router);
    handlers::register_analysis_routes(router);
}

// ============================================================================
// MCP Server command handler
// ============================================================================

static bool mcp_server_command(int argc, char* argv[]) {
    if (argc < 2) {
        _plugin_logputs("[MCP] Usage: mcpserver <start|stop|status>");
        return false;
    }

    std::string subcommand = argv[1];

    if (subcommand == "start") {
        if (g_server.is_running()) {
            _plugin_logputs("[MCP] Server is already running");
            return true;
        }

        auto result = g_server.start(DEFAULT_HOST, DEFAULT_PORT, &g_router);
        if (result.has_value()) {
            _plugin_logprintf("[MCP] Server started on %s:%d\n", DEFAULT_HOST, DEFAULT_PORT);
        } else {
            _plugin_logprintf("[MCP] Failed to start server: %s\n", result.error().c_str());
        }
        return result.has_value();
    }

    if (subcommand == "stop") {
        if (!g_server.is_running()) {
            _plugin_logputs("[MCP] Server is not running");
            return true;
        }

        g_server.stop();
        _plugin_logputs("[MCP] Server stopped");
        return true;
    }

    if (subcommand == "status") {
        if (g_server.is_running()) {
            _plugin_logprintf("[MCP] Server is running on %s:%d\n", DEFAULT_HOST, g_server.get_port());
        } else {
            _plugin_logputs("[MCP] Server is not running");
        }
        return true;
    }

    _plugin_logputs("[MCP] Unknown subcommand. Usage: mcpserver <start|stop|status>");
    return false;
}

// ============================================================================
// Plugin exports
// ============================================================================

PLUG_EXPORT bool pluginit(PLUG_INITSTRUCT* init_struct) {
    init_struct->sdkVersion = PLUG_SDKVERSION;
    init_struct->pluginVersion = PLUGIN_VERSION;
    strncpy_s(init_struct->pluginName, PLUGIN_NAME, _TRUNCATE);

    g_plugin_handle = init_struct->pluginHandle;

    // Register the mcpserver command
    _plugin_registercommand(g_plugin_handle, "mcpserver", mcp_server_command, false);

    return true;
}

PLUG_EXPORT bool plugstop() {
    // Unregister command
    _plugin_unregistercommand(g_plugin_handle, "mcpserver");

    // Stop the HTTP server
    g_server.stop();

    _plugin_logputs("[MCP] Plugin stopped");
    return true;
}

PLUG_EXPORT void plugsetup(PLUG_SETUPSTRUCT* /*setup_struct*/) {
    // Register all API routes
    register_all_routes(g_router);

    // Auto-start the server
    auto result = g_server.start(DEFAULT_HOST, DEFAULT_PORT, &g_router);
    if (result.has_value()) {
        _plugin_logprintf("[MCP] x64dbg MCP Server started on %s:%d\n", DEFAULT_HOST, DEFAULT_PORT);
    } else {
        _plugin_logprintf("[MCP] Failed to auto-start server: %s\n", result.error().c_str());
        _plugin_logputs("[MCP] Use 'mcpserver start' to retry");
    }
}
