# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

An [MCP server](https://modelcontextprotocol.io/) that gives AI assistants full control over the [x64dbg](https://x64dbg.com/) debugger. **23 mega-tools** covering 151 REST endpoints via Zod discriminated unions - stepping, breakpoints, memory, disassembly, tracing, anti-debug bypasses, control flow analysis, PE dumping, and more.

Works with Claude Code, Claude Desktop, Cursor, Windsurf, Cline, and any MCP-compatible client.

## Table of Contents

- [Quick Start](#quick-start)
- [How It Works](#how-it-works)
- [Tool Reference](#tool-reference-23-mega-tools)
- [Usage Examples](#usage-examples)
- [Configuration](#configuration)
- [Architecture](#architecture)
- [Building from Source](#building-from-source)
- [Troubleshooting](#troubleshooting)
- [Security](#security)

## Quick Start

### Prerequisites

1. **x64dbg** - [Download latest snapshot](https://github.com/x64dbg/x64dbg/releases)
2. **Node.js** >= 18 - [Download](https://nodejs.org/)
3. **MCP plugin** - [Download from releases](https://github.com/bromoket/x64dbg_mcp/releases) (`x64dbg_mcp.dp64` and/or `x64dbg_mcp.dp32`)

### Step 1: Install the Plugin

Copy the plugin DLLs into your x64dbg plugins directories:

```
x64dbg/
  x64/plugins/x64dbg_mcp.dp64    <-- for 64-bit debugging
  x32/plugins/x64dbg_mcp.dp32    <-- for 32-bit debugging
```

Start x64dbg. You should see in the log:

```
[MCP] x64dbg MCP Server started on 127.0.0.1:27042
```

### Step 2: Add to Your AI Client

Pick your client and copy the config:

<details open>
<summary><b>Claude Code</b></summary>

Add to `.claude/settings.json` (project-level) or `~/.claude/settings.json` (global):

```json
{
  "mcpServers": {
    "x64dbg": {
      "type": "stdio",
      "command": "cmd",
      "args": ["/c", "npx", "-y", "x64dbg-mcp-server"]
    }
  }
}
```

</details>

<details>
<summary><b>Claude Desktop</b></summary>

Add to `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}
```

</details>

<details>
<summary><b>Cursor</b></summary>

Add to `.cursor/mcp.json` (project-level) or `~/.cursor/mcp.json` (global):

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}
```

</details>

<details>
<summary><b>Windsurf</b></summary>

Add to `~/.codeium/windsurf/mcp_config.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}
```

</details>

<details>
<summary><b>Cline (VSCode extension)</b></summary>

Open Cline settings > MCP Servers > Configure, then add to `cline_mcp_settings.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}
```

</details>

<details>
<summary><b>Any other MCP client</b></summary>

The server uses **stdio transport**. Spawn it as a child process:

```bash
npx -y x64dbg-mcp-server
```

Communicate over stdin/stdout using the [MCP protocol](https://modelcontextprotocol.io/).

</details>

### Step 3: Start Debugging

Open any executable in x64dbg, then talk to your AI assistant:

```
"Set a breakpoint on CreateFileW and run the program"
"Disassemble the current function and explain what it does"
"Search for the byte pattern 48 8B ?? 48 85 C0 in the main module"
"Hide the debugger and bypass the anti-debug checks"
"List all threads and show me which one is the anti-cheat scanner"
"Trace into the VM dispatcher and log all instructions to a file"
```

## How It Works

The system has two components:

```
                         stdio                        HTTP (localhost)
 MCP Client  <───────────────────>  TypeScript MCP  <──────────────────>  C++ Plugin
 (Claude,                           Server            127.0.0.1:27042     (inside x64dbg)
  Cursor,                           23 mega-tools                         151 REST endpoints
  etc.)                             Zod validation                        Bridge/Plugin SDK
```

- **C++ Plugin** (`x64dbg_mcp.dp64` / `.dp32`) runs inside x64dbg as a lightweight REST API server on `127.0.0.1:27042`. It wraps the x64dbg Bridge/Plugin SDK with 151 JSON endpoints across 22 handler files.

- **TypeScript MCP Server** (`x64dbg-mcp-server` on npm) implements the MCP protocol over stdio. The 23 mega-tools use Zod discriminated unions to validate parameters, then route requests to the correct REST endpoint on the plugin via localhost HTTP.

The MCP server waits up to 2 minutes for the plugin to become available, performs health checks every 15 seconds, and automatically reconnects if x64dbg restarts. Requests retry up to 3 times with exponential backoff.

**Why stdio?** No SSE reconnection issues, no port conflicts, no dropped connections. The MCP client spawns the server as a child process - it just works.

## Tool Reference (23 Mega-Tools)

Each tool accepts an `action` parameter that selects the specific operation. Parameters are validated with Zod schemas at runtime.

### Debugger Control

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_debug` | `run`, `pause`, `force_pause`, `step_into`, `step_over`, `step_out`, `stop_debug`, `restart_debug`, `run_to_address`, `state` | Control execution flow and query debugger state |
| `x64dbg_command` | `execute`, `script`, `evaluate`, `format`, `set_init_script`, `get_init_script`, `get_hash`, `get_events` | Execute raw x64dbg commands, batch scripts, and expression evaluation |

### CPU & Memory

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_registers` | `get_all`, `get_specific`, `get_flags`, `get_avx512`, `set` | Read/write CPU registers including GPR, flags, and AVX-512 |
| `x64dbg_memory` | `read`, `write`, `info`, `is_valid`, `is_code`, `allocate`, `free`, `protect`, `map`, `update_map` | Full memory operations: read, write, allocate, protect, and memory map |
| `x64dbg_stack` | `get_call_stack`, `read`, `pointers`, `seh_chain`, `return_address`, `comment` | Call stack unwinding, raw stack reads, SEH chain, return address |

### Code Analysis

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_disassembly` | `at_address`, `function`, `info`, `assemble` | Disassemble instructions, whole functions, or assemble new code |
| `x64dbg_analysis` | `function`, `xrefs_to`, `xrefs_from`, `basic_blocks`, `source`, `mnemonic_brief` | Cross-references, function boundaries, basic blocks, source mapping |
| `x64dbg_control_flow` | `cfg`, `branch_dest`, `is_jump_taken`, `loops`, `func_type`, `add_function`, `delete_function` | Control flow graph, branch analysis, loop detection, function management |
| `x64dbg_database` | `constants`, `error_codes`, `structs`, `strings` | Query x64dbg's analysis database for constants, errors, structs, strings |
| `x64dbg_address_convert` | `va_to_file`, `file_to_va` | Convert between virtual addresses and file offsets |
| `x64dbg_watchdog` | *(id parameter)* | Check if a watch expression watchdog has been triggered |

### Breakpoints & Tracing

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_breakpoints` | `set_software`, `set_hardware`, `set_memory`, `delete`, `enable`, `disable`, `toggle`, `set_condition`, `set_log`, `reset_hit_count`, `get`, `list`, `configure`, `configure_batch` | Full breakpoint management: software, hardware, memory, conditional, logging, batch |
| `x64dbg_tracing` | `into`, `over`, `run`, `stop`, `animate`, `conditional_run`, `log_setup`, `hitcount`, `type`, `set_type` | Execution tracing, trace logging, hit counters, conditional tracing |
| `x64dbg_exceptions` | `set`, `delete`, `list`, `list_codes`, `skip` | Exception breakpoints, known exception codes, skip/pass exceptions |

### Symbols & Annotations

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_symbols` | `resolve`, `address`, `search`, `list_module`, `get_label`, `set_label`, `get_comment`, `set_comment`, `bookmark` | Symbol resolution, labels, comments, bookmarks |
| `x64dbg_search` | `pattern`, `string`, `string_at`, `symbol_auto_complete`, `encode_type` | AOB/byte pattern scan, string search, symbol autocomplete |
| `x64dbg_modules` | `list`, `get_info`, `get_base`, `get_section`, `get_party` | Loaded modules, base addresses, sections, user/system classification |

### Process & System

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_process` | `basic`, `detailed`, `cmdline`, `elevated`, `dbversion`, `set_cmdline` | Process info, PID, PEB, elevation status, debugger version |
| `x64dbg_threads` | `list`, `current`, `count`, `info`, `teb`, `name`, `switch`, `suspend`, `resume` | Thread enumeration, TEB access, thread control |
| `x64dbg_handles` | `list_handles`, `list_tcp`, `list_windows`, `list_heaps`, `get_name`, `close` | Handles, TCP connections, windows, heaps |
| `x64dbg_antidebug` | `peb`, `teb`, `dep`, `hide_debugger` | PEB/TEB inspection, DEP status, hide debugger from anti-debug |

### Patching & Dumping

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_patches` | `list`, `apply`, `restore`, `export` | Apply byte patches, restore originals, export patched module |
| `x64dbg_dumping` | `pe_header`, `sections`, `imports`, `exports`, `entry_point`, `relocations`, `dump_module`, `fix_iat`, `export_patch_file` | PE analysis, module dumping, IAT reconstruction, patch file export |

## Usage Examples

**Basic debugging:**
```
"Set a breakpoint on kernel32.CreateFileW and run"
"Step over 10 instructions and show me the registers"
"What's the current call stack?"
```

**Memory analysis:**
```
"Read 128 bytes at the address pointed to by RDI"
"Search for the byte pattern FF 15 ?? ?? ?? ?? in the .text section"
"Write 90 90 90 (NOPs) at 0x401000 and verify the write"
"Allocate a page of memory and write my shellcode there"
```

**Reverse engineering:**
```
"Disassemble the current function and explain the algorithm"
"Get the control flow graph and identify the switch cases"
"Show me cross-references to this function - who calls it?"
"List all imports from kernel32 and advapi32"
"What loops are in this function? Show me the loop boundaries"
```

**Anti-debug bypass:**
```
"Hide the debugger from anti-debug checks"
"Show me the PEB fields - is BeingDebugged set?"
"Set an exception breakpoint on STATUS_ACCESS_VIOLATION"
"Check DEP status for this process"
```

**Tracing and logging:**
```
"Configure a logging breakpoint on GetProcAddress that logs the function name"
"Set up 8 breakpoints in one call using configure_batch"
"Trace into this function and log every instruction to trace.log"
"Run to user code (skip system DLLs)"
```

**Process inspection:**
```
"List all threads and tell me which one is the main thread"
"Show me all open file handles in this process"
"List TCP connections - is it phoning home?"
"Dump the main module to disk and fix the import table"
```

## Configuration

Environment variables for the MCP server:

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout in milliseconds |
| `X64DBG_MCP_RETRIES` | `3` | Retry count on transient failures |

Set these in your MCP client config if needed:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"],
      "env": {
        "X64DBG_MCP_PORT": "27043"
      }
    }
  }
}
```

### Plugin Commands

Control the REST API from the x64dbg command bar:

```
mcpserver start     Start the HTTP server
mcpserver stop      Stop the HTTP server
mcpserver status    Show server status and port
```

The plugin also provides GUI dialogs accessible from `Plugins > x64dbg MCP Server`:

- **Settings...** — configure host, port, and auto-start (persisted via BridgeSetting)
- **About...** — version, live server status (green/red), GitHub link, Discord contact

## Architecture

### System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│  MCP Client (Claude Code, Cursor, etc.)                         │
│  Spawns server as child process, communicates via stdin/stdout   │
└──────────────────────────┬──────────────────────────────────────┘
                           │ stdio (MCP JSON-RPC)
┌──────────────────────────▼──────────────────────────────────────┐
│  TypeScript MCP Server (x64dbg-mcp-server)                      │
│                                                                 │
│  23 tools registered via @modelcontextprotocol/sdk              │
│  Zod discriminated unions validate action + parameters          │
│  HttpClient: auto-reconnect, health checks, retry logic         │
└──────────────────────────┬──────────────────────────────────────┘
                           │ HTTP GET/POST (127.0.0.1:27042)
┌──────────────────────────▼──────────────────────────────────────┐
│  C++ Plugin DLL (x64dbg_mcp.dp64 / .dp32)                      │
│                                                                 │
│  Winsock2 HTTP server, JSON via nlohmann/json                   │
│  22 handler files, 151 REST endpoints                           │
│  c_bridge_executor: thread-safe calls to x64dbg SDK             │
└──────────────────────────┬──────────────────────────────────────┘
                           │ x64dbg Bridge/Plugin SDK
┌──────────────────────────▼──────────────────────────────────────┐
│  x64dbg Debugger Engine                                         │
│  DbgFunctions(), Script API, Bridge API                         │
└─────────────────────────────────────────────────────────────────┘
```

### Project Structure

```
x64dbg_mcp/
├── plugin/                         # C++ x64dbg plugin (REST API server)
│   ├── CMakeLists.txt              # Build config (C++23, clang-cl)
│   ├── CMakePresets.json           # x64-release, x32-release, x64-debug presets
│   ├── plugin.def                  # DLL export definitions
│   ├── sdk/                        # x64dbg Plugin SDK headers + libs
│   │   ├── _plugins.h              # Plugin API
│   │   ├── _dbgfunctions.h         # DbgFunctions() interface
│   │   ├── bridgemain.h            # Bridge API
│   │   ├── jansson/                # JSON library (SDK dependency)
│   │   └── *.lib                   # x64bridge, x32bridge, x64dbg, x32dbg
│   └── src/
│       ├── plugin_main.cpp/.h      # Plugin entry, /api/health, /api/process/info
│       ├── bridge/
│       │   └── c_bridge_executor.* # Thread-safe wrapper for x64dbg API calls
│       ├── handlers/               # 22 REST endpoint handler files
│       │   ├── debug_handler.cpp       # /api/debug/* (11 endpoints)
│       │   ├── register_handler.cpp    # /api/registers/* (5 endpoints)
│       │   ├── memory_handler.cpp      # /api/memory/* (9 endpoints)
│       │   ├── breakpoint_handler.cpp  # /api/breakpoints/* (15 endpoints)
│       │   ├── disasm_handler.cpp      # /api/disasm/* (4 endpoints)
│       │   ├── module_handler.cpp      # /api/modules/* (5 endpoints)
│       │   ├── thread_handler.cpp      # /api/threads/* (9 endpoints)
│       │   ├── stack_handler.cpp       # /api/stack/* (7 endpoints)
│       │   ├── symbol_handler.cpp      # /api/symbols/* (4 endpoints)
│       │   ├── annotation_handler.cpp  # /api/labels/*, /api/comments/*, /api/bookmarks/* (5 endpoints)
│       │   ├── search_handler.cpp      # /api/search/* (5 endpoints)
│       │   ├── command_handler.cpp     # /api/command/* (8 endpoints)
│       │   ├── analysis_handler.cpp    # /api/analysis/* (13 endpoints)
│       │   ├── tracing_handler.cpp     # /api/trace/* (10 endpoints)
│       │   ├── dumping_handler.cpp     # /api/dump/*, /api/patches/export_file (10 endpoints)
│       │   ├── patch_handler.cpp       # /api/patches/* (4 endpoints)
│       │   ├── memmap_handler.cpp      # /api/memmap/* (2 endpoints)
│       │   ├── antidebug_handler.cpp   # /api/antidebug/* (4 endpoints)
│       │   ├── exceptions_handler.cpp  # /api/exceptions/* (5 endpoints)
│       │   ├── process_handler.cpp     # /api/process/* (5 endpoints)
│       │   ├── handles_handler.cpp     # /api/handles/* (6 endpoints)
│       │   └── controlflow_handler.cpp # /api/cfg/* (7 endpoints)
│       ├── http/
│       │   ├── c_http_server.*     # Winsock2 HTTP server (localhost only)
│       │   ├── c_http_router.*     # Method + path routing
│       │   ├── s_http_request.h    # Request struct (method, path, body, query)
│       │   └── s_http_response.h   # Response helpers (ok, bad_request, conflict, etc.)
│       ├── ui/
│       │   ├── settings_dialog.*   # Settings dialog (host, port, auto-start)
│       │   └── about_dialog.*      # About dialog (version, status, links)
│       └── util/
│           └── format_utils.*      # Address formatting, hex parsing
│
├── server/                         # TypeScript MCP server (npm package)
│   ├── package.json                # x64dbg-mcp-server v2.2.1
│   ├── tsconfig.json               # ES2022, Node16, strict mode
│   ├── server.json                 # MCP registry manifest
│   └── src/
│       ├── index.ts                # McpServer entry, stdio transport, graceful shutdown
│       ├── config.ts               # Environment variable config
│       ├── http_client.ts          # HTTP client with auto-reconnect and health monitoring
│       └── tools/                  # 19 tool files, 23 MCP tools
│           ├── index.ts            # Registers all tools on the McpServer
│           ├── debug.ts            # x64dbg_debug
│           ├── registers.ts        # x64dbg_registers
│           ├── memory.ts           # x64dbg_memory (includes memmap)
│           ├── disassembly.ts      # x64dbg_disassembly
│           ├── breakpoints.ts      # x64dbg_breakpoints
│           ├── symbols.ts          # x64dbg_symbols (includes labels, comments, bookmarks)
│           ├── stack.ts            # x64dbg_stack
│           ├── threads.ts          # x64dbg_threads
│           ├── modules.ts          # x64dbg_modules
│           ├── search.ts           # x64dbg_search
│           ├── command.ts          # x64dbg_command
│           ├── analysis.ts         # x64dbg_analysis, x64dbg_database, x64dbg_address_convert, x64dbg_watchdog
│           ├── tracing.ts          # x64dbg_tracing
│           ├── dumping.ts          # x64dbg_dumping
│           ├── antidebug.ts        # x64dbg_antidebug
│           ├── exceptions.ts       # x64dbg_exceptions
│           ├── process.ts          # x64dbg_process
│           ├── handles.ts          # x64dbg_handles
│           ├── controlflow.ts      # x64dbg_control_flow
│           └── patches.ts          # x64dbg_patches
│
├── install.ps1                     # PowerShell deploy script (local use, gitignored paths)
├── LICENSE                         # MIT
└── README.md
```

### Tech Stack

**MCP Server (TypeScript)**
- **Runtime**: Node.js >= 18
- **Language**: TypeScript (ES2022, strict mode)
- **MCP SDK**: `@modelcontextprotocol/sdk` ^1.12.1
- **Validation**: `zod` ^3.25.1
- **Transport**: stdio (stdin/stdout JSON-RPC)

**Plugin (C++)**
- **Standard**: C++23 (`/std:c++latest`)
- **Compiler**: Clang-cl (ships with Visual Studio 2022)
- **Build System**: CMake 3.20+ with Ninja
- **Dependencies**: nlohmann/json (via vcpkg), x64dbg Plugin SDK, Winsock2
- **Package Manager**: vcpkg

## Building from Source

### C++ Plugin

Requires CMake >= 3.20, Ninja, vcpkg, and Clang-cl (ships with Visual Studio 2022 C++ workload).

```powershell
cd plugin

# Set VCPKG_ROOT to your vcpkg installation
$env:VCPKG_ROOT = "C:\path\to\vcpkg"

# Configure
cmake --preset x64-release    # For 64-bit plugin
cmake --preset x32-release    # For 32-bit plugin

# Build
cmake --build build/x64-release
cmake --build build/x32-release

# Output:
#   build/x64-release/bin/x64dbg_mcp.dp64
#   build/x32-release/bin/x64dbg_mcp.dp32
```

Copy the `.dp64` / `.dp32` files to your x64dbg `plugins/` directory and restart x64dbg.

### TypeScript Server

```bash
cd server
npm install
npm run build
```

To run the server from source instead of npm:

```bash
node server/dist/index.js
```

Or point your MCP client config to the local build:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "node",
      "args": ["C:/path/to/x64dbg_mcp/server/dist/index.js"]
    }
  }
}
```

## Troubleshooting

### "Connection refused" or server can't reach plugin

1. Make sure x64dbg is running with a target loaded
2. Verify the plugin is in the correct `plugins/` directory
3. Check the x64dbg log for `[MCP] x64dbg MCP Server started on 127.0.0.1:27042`
4. Test manually: `curl http://127.0.0.1:27042/api/health`

### "Waiting for x64dbg plugin..." hangs

The MCP server waits up to 2 minutes for the plugin to come online. Either:
- Start x64dbg **before** launching your MCP client
- Or restart the MCP client after x64dbg is running

### Tools return errors about debugger state

| Error | Meaning | Solution |
|-------|---------|----------|
| "Debugger must be paused" | Inspection tools need paused state | Pause the target or hit a breakpoint first |
| "No active debug session" | No executable loaded | Load a target in x64dbg (`File > Open`) |
| "Debugger must be running" | `pause`/`force_pause` need running target | Run the target first |

### 32-bit vs 64-bit

Use the correct plugin for your target architecture:

| Target | Debugger | Plugin File |
|--------|----------|-------------|
| 64-bit | x64dbg | `x64dbg_mcp.dp64` |
| 32-bit | x32dbg | `x64dbg_mcp.dp32` |

Both use the same MCP server - just `npx -y x64dbg-mcp-server`.

### Plugin not loading

If the plugin doesn't appear in the x64dbg log on startup:

1. Check that the DLL is in the right directory (e.g., `x64/plugins/` for 64-bit)
2. Make sure you're using a recent x64dbg snapshot (2024+)
3. Check if another plugin is conflicting on port 27042
4. Try manually: type `mcpserver start` in the x64dbg command bar

### Request timeouts

For operations on large binaries (full memory scan, module dump), increase the timeout:

```bash
X64DBG_MCP_TIMEOUT=120000 npx -y x64dbg-mcp-server
```

## Security

- The C++ plugin binds to `127.0.0.1` only - no remote access, no network exposure
- The MCP server communicates exclusively via stdio (stdin/stdout)
- All HTTP traffic stays on localhost - no data leaves your machine
- No authentication is needed because the REST API is localhost-only

## Author

**bromo** - [GitHub](https://github.com/bromoket)

Built with [Claude Code](https://claude.ai/claude-code) by Anthropic.

## License

[MIT](LICENSE)
