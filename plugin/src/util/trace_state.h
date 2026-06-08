#pragma once

#include <string>
#include <nlohmann/json.hpp>

// Trace state shared between the x64dbg trace callbacks (CB_STARTTRACE /
// CB_STOPTRACE, fired on the debugger thread) and the HTTP /api/trace/status
// endpoint (served on a connection thread). Implemented in plugin_main.cpp.
namespace mcp {

// Update the current trace state. Called from the trace callbacks.
void trace_set_active(bool active, const std::string& file);

// Snapshot the current trace state for the status endpoint.
[[nodiscard]] nlohmann::json trace_status();

} // namespace mcp
