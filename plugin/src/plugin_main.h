#pragma once

#include "_plugins.h"
#include "http/c_http_server.h"
#include "http/c_http_router.h"

// Plugin info
constexpr auto PLUGIN_NAME = "x64dbg MCP Server";
constexpr auto PLUGIN_VERSION = 1;
constexpr uint16_t DEFAULT_PORT = 27042;
constexpr auto DEFAULT_HOST = "127.0.0.1";

// Plugin export macro (functions are also listed in plugin.def)
#ifndef PLUG_EXPORT
#define PLUG_EXPORT extern "C" __declspec(dllexport)
#endif

// Register all API routes on the router
void register_all_routes(c_http_router& router);
