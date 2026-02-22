#include "plugin_main.h"

#include <string>
#include <cstdio>
#include <cstring>

#include "_plugins.h"
#include "http/c_http_server.h"
#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"
#include "resources/plugin_icon.h"
#include "ui/settings_dialog.h"
#include "ui/about_dialog.h"

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
    void register_tracing_routes(c_http_router& router);
    void register_dumping_routes(c_http_router& router);
    void register_antidebug_routes(c_http_router& router);
    void register_exception_routes(c_http_router& router);
    void register_process_routes(c_http_router& router);
    void register_handles_routes(c_http_router& router);
    void register_controlflow_routes(c_http_router& router);
} // namespace handlers

// Globals
static int g_plugin_handle = -1;
static int g_menu_handle = -1;
static HWND g_hwnd_dlg = nullptr;
static c_http_server g_server;
static c_http_router g_router;
static s_plugin_settings g_settings;

// ============================================================================
// Menu helpers
// ============================================================================

static void update_menu_state() {
    const bool running = g_server.is_running();
    _plugin_menuentrysetchecked(g_plugin_handle, menu_start_server, running);
    _plugin_menuentrysetchecked(g_plugin_handle, menu_stop_server, !running);
}

// ============================================================================
// Settings persistence
// ============================================================================

static void load_settings() {
    char buf[256];

    if (BridgeSettingGet(SETTINGS_SECTION, SETTINGS_KEY_HOST, buf)) {
        strncpy_s(g_settings.host, buf, _TRUNCATE);
    }

    duint port_val = 0;
    if (BridgeSettingGetUint(SETTINGS_SECTION, SETTINGS_KEY_PORT, &port_val)) {
        if (port_val >= 1 && port_val <= 65535) {
            g_settings.port = static_cast<uint16_t>(port_val);
        }
    }

    duint autostart_val = 0;
    if (BridgeSettingGetUint(SETTINGS_SECTION, SETTINGS_KEY_AUTOSTART, &autostart_val)) {
        g_settings.auto_start = (autostart_val != 0);
    }
}

static void save_settings() {
    BridgeSettingSet(SETTINGS_SECTION, SETTINGS_KEY_HOST, g_settings.host);
    BridgeSettingSetUint(SETTINGS_SECTION, SETTINGS_KEY_PORT, g_settings.port);
    BridgeSettingSetUint(SETTINGS_SECTION, SETTINGS_KEY_AUTOSTART, g_settings.auto_start ? 1 : 0);
    BridgeSettingFlush();
}

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
    handlers::register_tracing_routes(router);
    handlers::register_dumping_routes(router);
    handlers::register_antidebug_routes(router);
    handlers::register_exception_routes(router);
    handlers::register_process_routes(router);
    handlers::register_handles_routes(router);
    handlers::register_controlflow_routes(router);
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

        auto result = g_server.start(g_settings.host, g_settings.port, &g_router);
        if (result.has_value()) {
            _plugin_logprintf("[MCP] Server started on %s:%u\n", g_settings.host, g_settings.port);
        } else {
            _plugin_logprintf("[MCP] Failed to start server: %s\n", result.error().c_str());
        }
        update_menu_state();
        return result.has_value();
    }

    if (subcommand == "stop") {
        if (!g_server.is_running()) {
            _plugin_logputs("[MCP] Server is not running");
            return true;
        }

        g_server.stop();
        _plugin_logputs("[MCP] Server stopped");
        update_menu_state();
        return true;
    }

    if (subcommand == "status") {
        if (g_server.is_running()) {
            _plugin_logprintf("[MCP] Server is running on %s:%u\n", g_settings.host, g_server.get_port());
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

PLUG_EXPORT void plugsetup(PLUG_SETUPSTRUCT* setup_struct) {
    // Store GUI handles
    g_hwnd_dlg = setup_struct->hwndDlg;
    g_menu_handle = setup_struct->hMenu;

    // Load persisted settings
    load_settings();

    // Set plugin menu icon
    ICONDATA icon_data;
    icon_data.data = plugin_icon::png_data;
    icon_data.size = plugin_icon::png_size;
    _plugin_menuseticon(g_menu_handle, &icon_data);

    // Build menu entries
    _plugin_menuaddentry(g_menu_handle, menu_start_server, "Start Server");
    _plugin_menuaddentry(g_menu_handle, menu_stop_server, "Stop Server");
    _plugin_menuaddseparator(g_menu_handle);
    _plugin_menuaddentry(g_menu_handle, menu_settings, "Settings...");
    _plugin_menuaddentry(g_menu_handle, menu_about, "About...");

    // Register all API routes
    register_all_routes(g_router);

    // Auto-start the server (if enabled in settings)
    if (g_settings.auto_start) {
        auto result = g_server.start(g_settings.host, g_settings.port, &g_router);
        if (result.has_value()) {
            _plugin_logprintf("[MCP] x64dbg MCP Server started on %s:%u\n",
                g_settings.host, g_settings.port);
        } else {
            _plugin_logprintf("[MCP] Failed to auto-start server: %s\n", result.error().c_str());
            _plugin_logputs("[MCP] Use 'mcpserver start' to retry");
        }
    } else {
        _plugin_logputs("[MCP] Auto-start disabled. Use 'mcpserver start' or menu to start.");
    }

    // Sync checkmarks with actual server state
    update_menu_state();
}

PLUG_EXPORT void CBMENUENTRY(CBTYPE, void* call_info) {
    auto* info = static_cast<PLUG_CB_MENUENTRY*>(call_info);

    switch (info->hEntry) {
    case menu_start_server:
        if (g_server.is_running()) {
            _plugin_logputs("[MCP] Server is already running");
        } else {
            auto result = g_server.start(g_settings.host, g_settings.port, &g_router);
            if (result.has_value()) {
                _plugin_logprintf("[MCP] Server started on %s:%u\n",
                    g_settings.host, g_settings.port);
            } else {
                _plugin_logprintf("[MCP] Failed to start server: %s\n", result.error().c_str());
            }
        }
        update_menu_state();
        break;

    case menu_stop_server:
        if (!g_server.is_running()) {
            _plugin_logputs("[MCP] Server is not running");
        } else {
            g_server.stop();
            _plugin_logputs("[MCP] Server stopped");
        }
        update_menu_state();
        break;

    case menu_settings: {
        // Snapshot current settings in case we need to detect host/port changes
        const auto old_host = std::string(g_settings.host);
        const auto old_port = g_settings.port;

        if (show_settings_dialog(g_hwnd_dlg, g_settings) == IDOK) {
            save_settings();
            _plugin_logputs("[MCP] Settings saved");

            // Restart server if host/port changed and server is running
            const bool host_changed = (old_host != g_settings.host);
            const bool port_changed = (old_port != g_settings.port);

            if (g_server.is_running() && (host_changed || port_changed)) {
                g_server.stop();
                auto result = g_server.start(g_settings.host, g_settings.port, &g_router);
                if (result.has_value()) {
                    _plugin_logprintf("[MCP] Server restarted on %s:%u\n",
                        g_settings.host, g_settings.port);
                } else {
                    _plugin_logprintf("[MCP] Failed to restart server: %s\n",
                        result.error().c_str());
                }
                update_menu_state();
            }
        }
        break;
    }

    case menu_about:
        show_about_dialog(g_hwnd_dlg, g_server.is_running(),
            g_settings.host, g_server.get_port());
        break;

    default:
        break;
    }
}
