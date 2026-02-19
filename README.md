# x64dbg MCP Server

A production-grade [Model Context Protocol](https://modelcontextprotocol.io/) server for [x64dbg](https://x64dbg.com/), enabling AI-powered debugging through Claude Code, Claude Desktop, or any MCP-compatible client.

**152 tools** across **21 categories** give an LLM complete control over x64dbg - from basic stepping to advanced tracing, memory dumping, anti-debug bypasses, and control flow analysis.

## Architecture

```
+------------------+         +-------------------+          +-------------------+
|                  |  stdio   |                   |  HTTP    |                   |
|  Claude Code /   |<-------->|  TypeScript MCP   |<-------->|  C++ Plugin DLL   |
|  Claude Desktop  |          |  Server (Node.js) | localhost|  (x64dbg_mcp.dp64)|
|                  |          |                   |  :27042  |                   |
+------------------+         +-------------------+          +---+---------------+
                                                                 |
                                                                 | Bridge API
                                                                 v
                                                            +-------------------+
                                                            |  x64dbg Debugger  |
                                                            +-------------------+
```

**Two-component design:**
- **C++ Plugin** (`x64dbg_mcp.dp64` / `.dp32`) - Lightweight REST API server inside x64dbg with 152 endpoints
- **TypeScript MCP Server** - Official SDK, stdio transport, 152 typed tools with Zod schemas

**Why stdio?** No SSE reconnection issues, no port conflicts, no dropped connections. Just works.

## Quick Start (Prebuilt Release)

1. Download the latest release from [Releases](https://github.com/bromoket/x64dbg_mcp/releases)
2. Copy `x64dbg_mcp.dp64` to your x64dbg `x64/plugins/` directory
3. Copy `x64dbg_mcp.dp32` to your x64dbg `x32/plugins/` directory
4. Start x64dbg - you should see `[MCP] x64dbg MCP Server started on 127.0.0.1:27042` in the log
5. Configure your MCP client (see [Configuration](#configuration) below)

## Features

- **152 MCP tools** across 21 categories - full debugger control
- **Fully typed** - every parameter has a Zod schema with descriptions
- **Stdio transport** - rock-solid connection (no SSE drops)
- **Official MCP SDK** - full protocol compliance
- **Thread-safe** bridge executor for x64dbg API calls
- **Localhost-only** binding - no network exposure
- **Both x32 and x64** plugin builds included

### Tool Categories

| Category | Tools | Description |
|----------|-------|-------------|
| Debug Control | 11 | run, pause, force_pause, step into/over/out, stop, restart, hide debugger, health check |
| Registers | 5 | get all, get one, set, decode flags, AVX-512 dump |
| Memory | 9 | read, write (with verify), validate, page info, alloc, free, protect, string read, memory map refresh |
| Disassembly | 4 | disassemble, function view (with max_instructions), instruction info, assemble |
| Breakpoints | 14 | set/delete/enable/disable/toggle software/hardware/memory BPs, unified configure, batch configure, conditions, logging, reset hit count |
| Symbols | 9 | resolve, search, autocomplete, labels, comments, bookmarks, module symbols |
| Stack | 7 | call stack, read stack, pointers, SEH chain, return address, stack comments, per-thread callstack |
| Threads | 9 | list, switch, suspend, resume, get info, TEB, thread count, thread name, current thread |
| Modules | 5 | list, info, base address, entry point, party (user/system) |
| Memory Map | 2 | full map, region query |
| Search | 5 | byte pattern (AOB), string search, find strings in module, symbol search, constants |
| Command | 8 | raw command, expression eval, format string, batch script, debug events, init script (get/set), database hash |
| Analysis | 13 | xrefs to/from, function info, basic blocks, CFG, loops, imports, exports, PE header, sections, relocations, source location, mnemonic brief |
| Tracing | 10 | trace into/over, conditional trace run, trace stop, log setup, animation, hit counts, trace record type, branch destination, jump prediction |
| Dumping | 9 | dump module, IAT fix, export patched file, file offset/VA conversion, section query, get cmdline, set cmdline, debugger version |
| Anti-Debug | 4 | hide debugger, read PEB info, read TEB info, DEP status |
| Exceptions | 5 | list exception BPs, set exception BP, delete exception BP, skip exception, list exception codes |
| Process | 6 | process info, basic process info, PID/elevation check, list windows, list TCP connections, list heaps |
| Handles | 6 | list handles, get handle name, close handle, list error codes, list structs, get encode type |
| Control Flow | 7 | get CFG, basic blocks, is code page, is jump taken, get function type, add/delete function definitions |
| Patches | 4 | list patches, apply patch, restore patch, export patched module |

## Prerequisites

### For using prebuilt releases
- [x64dbg](https://github.com/x64dbg/x64dbg/releases) (latest snapshot)
- [Node.js](https://nodejs.org/) >= 18

### For building from source (additional)
- [CMake](https://cmake.org/) >= 3.20
- [Ninja](https://ninja-build.org/)
- [vcpkg](https://vcpkg.io/) (for nlohmann-json dependency)
- Clang-cl (ships with [Visual Studio 2022](https://visualstudio.microsoft.com/) C++ workload)

## Building from Source

### C++ Plugin

```powershell
cd plugin

# Configure (set VCPKG_ROOT to your vcpkg installation)
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
cmake --preset x64-release    # For 64-bit
cmake --preset x32-release    # For 32-bit

# Build
cmake --build build/x64-release
cmake --build build/x32-release

# Output:
#   build/x64-release/bin/x64dbg_mcp.dp64
#   build/x32-release/bin/x64dbg_mcp.dp32
```

### TypeScript Server

```powershell
cd server
npm install
npm run build
```

## Installation

### Plugin

Copy the appropriate plugin DLL to your x64dbg plugins directory:

```
x64dbg/
  x64/
    plugins/
      x64dbg_mcp.dp64    <-- 64-bit plugin
  x32/
    plugins/
      x64dbg_mcp.dp32    <-- 32-bit plugin
```

Start x64dbg. The plugin auto-starts the REST API on `127.0.0.1:27042`. You should see:
```
[MCP] x64dbg MCP Server started on 127.0.0.1:27042
```

### Server

The TypeScript server runs via Node.js. No global install needed - just point your MCP client at the built `index.js`.

## Configuration

### Claude Code

Add to your project-level `.claude/settings.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "node",
      "args": ["path/to/x64dbg_mcp/server/dist/index.js"],
      "env": {
        "X64DBG_MCP_HOST": "127.0.0.1",
        "X64DBG_MCP_PORT": "27042"
      }
    }
  }
}
```

Or use npx if the server is published:
```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["x64dbg-mcp-server"],
      "env": {
        "X64DBG_MCP_HOST": "127.0.0.1",
        "X64DBG_MCP_PORT": "27042"
      }
    }
  }
}
```

### Claude Desktop

Add to `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "node",
      "args": ["path/to/x64dbg_mcp/server/dist/index.js"],
      "env": {
        "X64DBG_MCP_HOST": "127.0.0.1",
        "X64DBG_MCP_PORT": "27042"
      }
    }
  }
}
```

### Other MCP Clients

Any MCP-compatible client can use this server. The server uses **stdio transport** - just spawn the Node.js process and communicate over stdin/stdout.

## Plugin Commands

Control the REST API from the x64dbg command bar:

```
mcpserver start    - Start the HTTP server
mcpserver stop     - Stop the HTTP server
mcpserver status   - Show server status
```

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `3` | Request retry count |

## Tool Reference

### Debug Control (11 tools)
| Tool | Description |
|------|-------------|
| `get_health` | Check if the plugin is running and responsive |
| `get_debug_state` | Get debugger state (stopped/running/paused), CIP, module |
| `run` | Resume execution |
| `pause` | Pause execution |
| `force_pause` | Force pause even when high-frequency fast-resume BPs are active |
| `step_into` | Single step into calls |
| `step_over` | Single step over calls |
| `step_out` | Run until current function returns |
| `stop_debug` | Stop the debug session |
| `restart_debug` | Restart from the beginning |
| `hide_debugger` | Zero PEB.BeingDebugged and NtGlobalFlag (bypass anti-debug) |

### Registers (5 tools)
| Tool | Description |
|------|-------------|
| `get_all_registers` | Get all CPU registers (GP, segment, debug, flags) |
| `get_register` | Get a single register value |
| `set_register` | Set a register to a new value |
| `get_flags` | Get decoded EFLAGS (CF, ZF, SF, OF, etc.) |
| `get_avx512_registers` | Get AVX-512 extended register dump |

### Memory (9 tools)
| Tool | Description |
|------|-------------|
| `read_memory` | Read bytes from address (hex dump + ASCII) |
| `write_memory` | Write bytes to address (optional `verify` readback check) |
| `is_valid_address` | Check if address is a valid readable pointer |
| `get_memory_info` | Get memory page protection info |
| `allocate_memory` | Allocate memory via VirtualAllocEx |
| `free_memory` | Free allocated memory |
| `set_memory_protection` | Change page protection (VirtualProtectEx) |
| `get_string_at` | Read string at address (auto/ascii/unicode encoding param) |
| `update_memory_map` | Force refresh the memory map |

### Disassembly (4 tools)
| Tool | Description |
|------|-------------|
| `disassemble` | Disassemble instructions at address |
| `disassemble_function` | Disassemble an entire function (configurable `max_instructions`) |
| `get_instruction_info` | Quick info about a single instruction |
| `assemble` | Assemble an instruction at address |

### Breakpoints (14 tools)
| Tool | Description |
|------|-------------|
| `set_breakpoint` | Set a software breakpoint (INT3) |
| `set_hardware_breakpoint` | Set a hardware breakpoint (debug registers) |
| `set_memory_breakpoint` | Set a memory breakpoint (page guard) |
| `delete_breakpoint` | Delete a breakpoint |
| `enable_breakpoint` | Enable a disabled breakpoint |
| `disable_breakpoint` | Disable without deleting |
| `toggle_breakpoint` | Toggle between enabled/disabled |
| `list_breakpoints` | List all active breakpoints (with resolved symbol labels) |
| `get_breakpoint` | Get detailed info about a specific breakpoint |
| `set_breakpoint_condition` | Set break_condition on a breakpoint |
| `set_breakpoint_log` | Set a log message on a breakpoint |
| `configure_breakpoint` | **Unified**: set + all conditions + command + silent + fast_resume in one call |
| `configure_breakpoints` | **Batch**: configure multiple breakpoints in one call (e.g. 8 BPs → 1 call) |
| `reset_breakpoint_hit_count` | Reset hit counter to zero |

### Symbols (9 tools)
| Tool | Description |
|------|-------------|
| `resolve_symbol` | Resolve a symbol/function name to address |
| `get_symbol_at` | Get the symbol name at an address |
| `search_symbols` | Search for symbols matching a pattern |
| `symbol_auto_complete` | Auto-complete a partial symbol name |
| `set_label` | Set a user-defined label at address |
| `get_label` | Get the label at address |
| `set_comment` | Set a comment at address |
| `get_comment` | Get the comment at address |
| `set_bookmark` | Set or clear a bookmark at address |

### Stack (7 tools)
| Tool | Description |
|------|-------------|
| `get_call_stack` | Get the call stack (stack trace) |
| `read_stack` | Read raw stack memory with symbol resolution |
| `get_stack_pointers` | Get current RSP/ESP and RBP/EBP |
| `get_seh_chain` | Get the SEH handler chain |
| `get_return_address` | Get return address from top of stack |
| `get_stack_comment` | Get stack comment and color at address |
| `get_callstack_by_thread` | Get call stack for a specific thread |

### Threads (9 tools)
| Tool | Description |
|------|-------------|
| `list_threads` | List all threads with IDs and start addresses |
| `switch_thread` | Switch debugger focus to a different thread |
| `suspend_thread` | Suspend a thread |
| `resume_thread` | Resume a suspended thread |
| `get_thread` | Get info about a specific thread |
| `get_current_thread` | Get info about the active thread |
| `get_thread_count` | Get total thread count |
| `get_thread_name` | Get the name of a thread |
| `get_thread_teb` | Get the TEB address for a thread |

### Modules (5 tools)
| Tool | Description |
|------|-------------|
| `list_modules` | List all loaded modules with base addresses and sizes |
| `get_module_info` | Get detailed info about a specific module |
| `get_module_base` | Get the base address of a module |
| `get_entry_point` | Get the entry point of a module |
| `get_module_party` | Check if module is user code or system code |

### Memory Map (2 tools)
| Tool | Description |
|------|-------------|
| `get_memory_map` | Get the full memory map |
| `get_memory_region` | Get region info for a specific address |

### Search (5 tools)
| Tool | Description |
|------|-------------|
| `search_pattern` | Byte pattern (AOB) scan with wildcard support - returns all matches |
| `search_strings` | Search for string references in a module |
| `find_strings_in_module` | Find and display all strings in a module |
| `list_module_symbols` | Load and list symbols for a module |
| `list_constants` | List all known constants (Windows API constants) |

### Command (8 tools)
| Tool | Description |
|------|-------------|
| `execute_command` | Execute a raw x64dbg command |
| `evaluate_expression` | Evaluate an expression to numeric value |
| `format_string` | Format string using x64dbg expression engine |
| `execute_script` | Execute a batch of commands sequentially |
| `get_debug_events` | Get the total debug event count |
| `get_init_script` | Get the debuggee initialization script |
| `set_init_script` | Set the debuggee initialization script |
| `get_database_hash` | Get the current database hash |

### Analysis (13 tools)
| Tool | Description |
|------|-------------|
| `get_xrefs_to` | Cross-references TO an address (who calls this) |
| `get_xrefs_from` | Cross-references FROM an address (what does this reference) |
| `get_function_info` | Function boundaries, label, and module |
| `get_basic_blocks` | Basic blocks (CFG) for a function |
| `get_cfg` | Full control flow graph with branch targets and instructions |
| `get_loops` | Loop info at address with nesting depth |
| `get_imports` | Display IAT for a module |
| `get_exports` | Display export table for a module |
| `get_pe_header` | Get PE section headers |
| `get_sections` | Get section info for a module |
| `get_relocations` | Get relocation entries |
| `get_source_location` | Source file and line number (requires symbols) |
| `get_mnemonic_brief` | Brief description of an instruction mnemonic |

### Tracing (10 tools)
| Tool | Description |
|------|-------------|
| `trace_into` | Conditional trace into (single-step through calls) |
| `trace_over` | Conditional trace over (step over calls) |
| `trace_run` | Conditional trace run with full control |
| `trace_stop` | Stop an active trace |
| `trace_log_setup` | Set up trace logging to a file |
| `animate_command` | Single-step with visual update |
| `get_trace_hit_count` | Get how many times an instruction was traced |
| `conditional_run` | Run with break/log conditions |
| `get_branch_destination` | Get destination of a branch/jump/call |
| `is_jump_going_to_execute` | Check if conditional jump will be taken |

### Dumping (9 tools)
| Tool | Description |
|------|-------------|
| `dump_module` | Dump a module from memory to file |
| `fix_iat` | IAT reconstruction using Scylla |
| `export_patched_file` | Export all patches to a patched executable |
| `file_offset_to_va` | Convert file offset to virtual address |
| `va_to_file_offset` | Convert virtual address to file offset |
| `get_section_at` | Get section info for an address |
| `get_cmdline` | Get command line of debugged process |
| `set_cmdline` | Set command line (takes effect on restart) |
| `get_debugger_version` | Get x64dbg bridge version number |

### Anti-Debug (4 tools)
| Tool | Description |
|------|-------------|
| `hide_debugger` | Zero PEB.BeingDebugged and NtGlobalFlag |
| `get_peb_info` | Read PEB fields (BeingDebugged, NtGlobalFlag, ProcessHeap) |
| `get_teb_info` | Read TEB fields (SEH chain, stack base/limit, PEB pointer) |
| `get_dep_status` | Check if DEP is enabled |

### Exceptions (5 tools)
| Tool | Description |
|------|-------------|
| `list_exception_breakpoints` | List all exception breakpoints |
| `set_exception_breakpoint` | Set breakpoint on specific exception code |
| `delete_exception_breakpoint` | Delete an exception breakpoint |
| `skip_exception` | Skip/pass current exception and continue |
| `list_exception_codes` | List all known Windows exception codes |

### Process (6 tools)
| Tool | Description |
|------|-------------|
| `get_process_info` | Detailed process info (PID, PEB, handles, elevation, DEP) |
| `get_basic_process_info` | Quick process info (PID, PEB, entry point, state) |
| `is_process_elevated` | Check if running as administrator |
| `list_windows` | List all windows with titles, classes, and styles |
| `list_tcp_connections` | List TCP connections with addresses and ports |
| `list_heaps` | List all heaps with addresses, sizes, and flags |

### Handles (6 tools)
| Tool | Description |
|------|-------------|
| `list_handles` | List all handles in the process |
| `get_handle_name` | Get the name/path of a handle |
| `close_handle` | Force-close a handle (use with caution) |
| `list_error_codes` | List Windows error codes (GetLastError values) |
| `list_structs` | List known structure definitions |
| `is_watchdog_triggered` | Check if a watch expression watchdog triggered |

### Control Flow (7 tools)
| Tool | Description |
|------|-------------|
| `get_cfg` | Control flow graph with nodes, edges, and instructions |
| `get_basic_blocks` | Basic blocks with start/end addresses |
| `is_code_page` | Check if address is in executable memory |
| `is_jump_going_to_execute` | Check if conditional jump will be taken |
| `get_function_info` | Function boundaries at an address |
| `add_function` | Define a function boundary in the database |
| `delete_function` | Delete a function definition |

### Patches (4 tools)
| Tool | Description |
|------|-------------|
| `list_patches` | List all current byte patches |
| `apply_patch` | Apply a byte patch at an address |
| `restore_patch` | Restore original bytes at a patched address |
| `export_patched_module` | Export a module with all patches applied |

## Usage Examples

Once configured, you can ask Claude to debug programs naturally:

```
"Attach to the process and set a breakpoint on CreateFileW"
"Step through the next 5 instructions and show me the register state"
"Search for the string 'password' in the main module"
"Disassemble the current function and explain what it does"
"Read 64 bytes of memory at the address pointed to by RSI"
"Set a hardware breakpoint on write at the address in RAX"
"Hide the debugger and bypass the anti-debug checks"
"Dump the main module to disk and fix the IAT"
"Trace into the VM dispatcher and log all instructions to a file"
"Get the control flow graph of this function and identify the switch cases"
"List all threads and show me which one is the anti-cheat scanner"
"Set a conditional breakpoint that only triggers when EAX == 0"
```

## Project Structure

```
x64dbg_mcp/
  plugin/                     # C++ plugin (REST API server)
    CMakeLists.txt
    CMakePresets.json
    sdk/                      # x64dbg plugin SDK
    src/
      plugin_main.cpp         # Plugin entry point, HTTP server lifecycle
      bridge/
        c_bridge_executor.*   # Thread-safe bridge to x64dbg API
      handlers/
        debug_handler.cpp     # Debug control endpoints (11)
        register_handler.cpp  # Register endpoints (5)
        memory_handler.cpp    # Memory endpoints (9)
        disasm_handler.cpp    # Disassembly endpoints (4)
        breakpoint_handler.cpp # Breakpoint endpoints (14)
        annotation_handler.cpp # Symbols/labels/comments endpoints (5)
        symbol_handler.cpp    # Symbol search endpoints (4)
        stack_handler.cpp     # Stack endpoints (7)
        thread_handler.cpp    # Thread endpoints (9)
        module_handler.cpp    # Module endpoints (5)
        memmap_handler.cpp    # Memory map endpoints (2)
        search_handler.cpp    # Search endpoints (5)
        command_handler.cpp   # Command/eval endpoints (8)
        analysis_handler.cpp  # Analysis endpoints (13)
        tracing_handler.cpp   # Tracing endpoints (10)
        dumping_handler.cpp   # Dumping endpoints (9)
        antidebug_handler.cpp # Anti-debug endpoints (4)
        exceptions_handler.cpp # Exception endpoints (5)
        process_handler.cpp   # Process endpoints (6)
        handles_handler.cpp   # Handle endpoints (6)
        controlflow_handler.cpp # Control flow endpoints (7)
        patch_handler.cpp     # Patch endpoints (4)
      http/
        c_http_server.*       # Lightweight HTTP server (Winsock2)
        c_http_router.*       # Request routing
      util/
        format_utils.*        # Address formatting utilities
  server/                     # TypeScript MCP server
    src/
      index.ts                # Server entry point (stdio transport)
      http_client.ts          # HTTP client for plugin communication
      tools/
        index.ts              # Tool registration (21 categories)
        debug.ts              # 11 debug tools
        registers.ts          # 5 register tools
        memory.ts             # 9 memory tools
        ...                   # (21 tool files total)
    package.json
    tsconfig.json
```

## Security

- The C++ plugin binds to `127.0.0.1` only - no remote access
- The MCP server communicates exclusively via stdio (stdin/stdout)
- No authentication is needed because all communication is local
- The plugin only accepts connections from localhost

## Author

**bromo** - [GitHub](https://github.com/bromoket)

This project was built with the help of [Claude Code](https://claude.ai/claude-code) (Opus 4.6) by Anthropic.

## License

MIT
