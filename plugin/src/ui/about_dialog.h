#pragma once

#include <cstdint>
#include <windows.h>

/// @brief Show the about dialog (modal).
/// @param parent Parent window handle
/// @param is_server_running Whether the MCP server is currently running
/// @param host Current server host
/// @param port Current server port
void show_about_dialog(HWND parent, bool is_server_running, const char* host, uint16_t port);
