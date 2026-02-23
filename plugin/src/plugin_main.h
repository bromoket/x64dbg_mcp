#pragma once

#include "_plugins.h"
#include "http/c_http_server.h"
#include "http/c_http_router.h"

// Plugin info
constexpr auto PLUGIN_NAME = "x64dbg MCP Server";
constexpr auto PLUGIN_AUTHOR = "bromo";
constexpr auto PLUGIN_VERSION = 1;
constexpr auto PLUGIN_VERSION_STR = "2.2.1";
constexpr auto PLUGIN_REPO_URL = "https://github.com/bromoket/x64dbg_mcp";
constexpr uint16_t DEFAULT_PORT = 27042;
constexpr auto DEFAULT_HOST = "127.0.0.1";

// Persistent settings keys (BridgeSettingGet/Set)
constexpr auto SETTINGS_SECTION = "MCP";
constexpr auto SETTINGS_KEY_HOST = "Host";
constexpr auto SETTINGS_KEY_PORT = "Port";
constexpr auto SETTINGS_KEY_AUTOSTART = "AutoStart";

/// @brief Plugin settings persisted via BridgeSetting
struct s_plugin_settings {
    char host[64] = "127.0.0.1";
    uint16_t port = 27042;
    bool auto_start = true;
};

// Menu entry IDs
enum e_menu_entry : int {
    menu_start_server = 0,
    menu_stop_server  = 1,
    menu_settings     = 2,
    menu_about        = 3
};

// Plugin export macro (functions are also listed in plugin.def)
#ifndef PLUG_EXPORT
#define PLUG_EXPORT extern "C" __declspec(dllexport)
#endif

// Register all API routes on the router
void register_all_routes(c_http_router& router);
