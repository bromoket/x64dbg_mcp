# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)](https://nodejs.org/)

An [MCP server](https://modelcontextprotocol.io/) that gives AI assistants full control over the [x64dbg](https://x64dbg.com/) debugger. **~78 consolidated tools** across **21 categories** - stepping, breakpoints, memory, disassembly, tracing, anti-debug bypasses, control flow analysis, and more.

## Quick Start

### What You Need

1. **x64dbg** - [Download latest snapshot](https://github.com/x64dbg/x64dbg/releases)
2. **Node.js** >= 18 - [Download](https://nodejs.org/)
3. **MCP plugin** - [Download from releases](https://github.com/bromoket/x64dbg_mcp/releases) (`x64dbg_mcp.dp64` and/or `x64dbg_mcp.dp32`)

### Step 1: Install the Plugin

Download the plugin DLLs from [GitHub Releases](https://github.com/bromoket/x64dbg_mcp/releases) and copy them to your x64dbg plugins directories:

- `x64dbg_mcp.dp64` goes in `x64/plugins/`
- `x64dbg_mcp.dp32` goes in `x32/plugins/`

Start x64dbg. You should see: `[MCP] x64dbg MCP Server started on 127.0.0.1:27042`

### Step 2: Add to Your AI Client

Pick your client and copy the config:

<details open>
<summary><b>Claude Code</b></summary>

Add to `.claude/settings.json` (project-level) or `~/.claude/settings.json` (global):

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

Or install globally instead of using npx:

```bash
npm install -g x64dbg-mcp-server
```

### Step 3: Start Debugging

Open any executable in x64dbg, then talk to your AI assistant:

```
"Set a breakpoint on CreateFileW and run the program"
"Disassemble the current function and explain what it does"
"Search for the byte pattern 48 8B ?? 48 85 C0 in the main module"
"Read 64 bytes at the address pointed to by RDI"
"Hide the debugger and bypass the anti-debug checks"
"Trace into the VM dispatcher and log all instructions to a file"
```

## How It Works

The system has two components:

- **C++ Plugin** (`x64dbg_mcp.dp64` / `.dp32`) runs inside x64dbg as a lightweight REST API server on `127.0.0.1:27042`. It wraps the x64dbg Bridge/Plugin SDK with 152 JSON endpoints.

- **TypeScript MCP Server** (`x64dbg-mcp-server` on npm) implements the MCP protocol over stdio. Each of the consolidated tools (~78 total) validates parameters with Zod, then routes the request to one of the 152 specific REST endpoints on the plugin via localhost HTTP.

The MCP server waits up to 2 minutes for the plugin to become available, performs health checks every 15 seconds, and automatically reconnects if x64dbg restarts. Requests retry up to 3 times with exponential backoff.

**Why stdio?** No SSE reconnection issues, no port conflicts, no dropped connections. The MCP client spawns the server as a child process - it just works.

## Tech Stack

- **Runtime**: Node.js >= 18
- **Language**: TypeScript (ES2022, strict mode)
- **MCP SDK**: `@modelcontextprotocol/sdk` - official MCP SDK
- **Validation**: `zod` - runtime type checking for all 152 tool schemas
- **Transport**: stdio (stdin/stdout)

## Configuration

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `3` | Retry count on transient failures |

## Plugin Commands

Control the REST API from the x64dbg command bar:

```
mcpserver start     Start the HTTP server
mcpserver stop      Stop the HTTP server
mcpserver status    Show server status and port
```

## Tool Reference (~78 tools)

### Debug Control (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_debug_state` | - | Get debugger state (stopped/running/paused), CIP, module, and health check |
| `execute_debug_action` | `action`, `address?` | Actions: run, pause, force_pause, step_into, step_over, step_out, stop_debug, restart_debug, run_to_address |

### Registers (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_registers` | `action`, `register_name?` | Actions: all, specific, flags, avx512 |
| `set_register` | `name`, `value` | Set a register to a new value |

### Memory (4 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `read_memory` | `address`, `size?` | Read bytes from address (hex dump + ASCII) |
| `write_memory` | `address`, `bytes`, `verify?` | Write bytes with optional readback verification |
| `get_memory_info` | `action`, `address` | Actions: info, is_valid, is_code |
| `manage_memory` | `action`, `address?`, `size?`, `protection?` | Actions: allocate, free, protect |
| `update_memory_map` | - | Force refresh the memory map |

### Disassembly (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `disassemble` | `action`, `address?`, `count?`, `max_instructions?` | Actions: at_address, function, info |
| `assemble` | `address`, `instruction` | Assemble an instruction at address |

### Breakpoints (4 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `manage_breakpoint` | `action`, `address`, `bp_type?`, `size?`, `condition?`, `text?` | Actions: set_software, set_hardware, set_memory, delete, enable, disable, toggle, set_condition, set_log, reset_hit_count |
| `list_breakpoints` | - | List all breakpoints with resolved symbol labels |
| `configure_breakpoint` | `address`, `bp_type?`, `break_condition?`, `log_text?`, `fast_resume?`, ... | **Unified**: set BP + all options in one call |
| `configure_breakpoints` | `breakpoints[]` | **Batch**: configure multiple breakpoints in one call |

### Symbols (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `resolve_symbol` | `action`, `name?`, `address?`, `pattern?`, `module?` | Actions: name, at_address, search_pattern, list_module |
| `manage_annotation` | `action`, `address`, `text?` | Actions: get_label, set_label, get_comment, set_comment |
| `manage_bookmark` | `action`, `address` | Actions: set, clear |

### Stack (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_call_stack` | `action`, `max_depth?`, `thread_id?` | Actions: current, specific_thread |
| `read_stack` | `address?`, `size?` | Read raw stack memory with symbol resolution |
| `get_stack_info` | `action`, `address?` | Actions: pointers, seh_chain, return_address, comment |

### Threads (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_threads` | - | List all threads with IDs, names, and start addresses |
| `get_thread_info` | `action`, `thread_id?` | Actions: current, specific, teb, name, count |
| `manage_thread` | `action`, `thread_id` | Actions: switch, suspend, resume |

### Modules (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_modules` | - | List all loaded modules with base addresses and sizes |
| `get_module_info` | `action`, `name?`, `address?` | Actions: get_info, get_base, get_section, get_party |

### Memory Map (1 tool)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_memory_map` | `address?` | Get the full memory map or info for a specific region |

### Search (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `search_memory` | `action`, `pattern?`, `text?`, `address?`, `size?`, `max_results?`, `module?`, `encoding?` | Actions: pattern, string |
| `get_search_info` | `action`, `address?`, `search?`, `encoding?`, `max_length?`, `max_results?`, `size?` | Actions: string_at, autocomplete, encode_type |

### Command (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `execute_command` | `action`, `command?`, `expression?`, `format?` | Actions: execute, evaluate, format |
| `execute_script` | `commands[]` | Execute a batch of commands sequentially |
| `manage_debug_session` | `action`, `file?` | Actions: init_script_get, init_script_set, db_hash, events |

### Analysis (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_analysis_info` | `action`, `address?`, `module?`, `id?`, `mnemonic?` | Actions: xrefs_to, xrefs_from, function, basic_blocks, strings, source, mnemonic |
| `list_database_info` | `action` | Actions: constants, errors, structs |
| `convert_address` | `action`, `address?`, `module?`, `offset?` | Actions: va_to_file, file_to_va |

### Tracing (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `manage_trace` | `action`, `condition?`, `max_steps?`, `log_text?`, `party?`, `file?`, `command?`, ... | Actions: into, over, run, stop, log_setup, animate, conditional_run |
| `get_trace_info` | `action`, `address?` | Actions: hit_count, branch_dest, will_jump |
| `set_trace_record_type` | `type` | Set the trace recording type (bit, byte, word, etc.) |

### Dumping (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `dump_module` | `module`, `file?` | Dump a module from memory to disk |
| `get_module_dump_info` | `action`, `address?`, `module?` | Actions: pe_header, sections, imports, exports, relocations, entry_point |
| `fix_iat` | `oep` | IAT reconstruction using Scylla |

### Anti-Debug (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `hide_debugger` | - | Zero PEB.BeingDebugged and NtGlobalFlag |
| `get_antidebug_info` | `action`, `id?` | Actions: peb, teb, dep |

### Exceptions (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `manage_exception_breakpoint` | `action`, `code?`, `chance?`, `bp_action?` | Actions: set, delete, list |
| `list_exception_codes` | - | List all known Windows exception codes |
| `skip_exception` | - | Skip/pass current exception and continue |

### Process (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_process_info` | `action` | Actions: detailed, basic, is_elevated, cmdline, version |
| `set_cmdline` | `cmdline` | Set command line (takes effect on restart) |

### Handles (3 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_handle_info` | `action` | Actions: list, tcp, windows, heaps |
| `get_handle_name` | `handle` | Get the name/path of a handle |
| `close_handle` | `handle` | Force-close a handle |

### Control Flow (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_control_flow_info` | `action`, `address?` | Actions: cfg, loops, branch_dest, will_jump, func_type |
| `manage_function_definitions` | `action`, `address?`, `start?`, `end?` | Actions: add, delete |

### Patches (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `manage_patch` | `action`, `address?`, `bytes?` | Actions: list, apply, restore |
| `export_patched_module` | `path`, `module?` | Export a module with all patches applied |

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
```

**Reverse engineering:**
```
"Disassemble the current function and explain the algorithm"
"Get the control flow graph and identify the switch cases"
"Show me cross-references to this function - who calls it?"
"List all imports from kernel32 and advapi32"
```

**Anti-debug bypass:**
```
"Hide the debugger from anti-debug checks"
"Show me the PEB fields - is BeingDebugged set?"
"Set an exception breakpoint on STATUS_ACCESS_VIOLATION"
```

**Tracing and logging:**
```
"Configure a logging breakpoint on GetProcAddress that logs the function name"
"Set up 8 breakpoints in one call using configure_breakpoints"
"Trace into this function and log every instruction to trace.log"
```

**Process inspection:**
```
"List all threads and tell me which one is the main thread"
"Show me all open file handles in this process"
"List TCP connections - is it phoning home?"
"Dump the main module to disk and fix the import table"
```

## Troubleshooting

### "Connection refused" or server can't reach plugin

Make sure:
1. x64dbg is running with a target loaded
2. The MCP plugin is installed in the correct `plugins/` directory
3. You see `[MCP] x64dbg MCP Server started on 127.0.0.1:27042` in the x64dbg log

Test connectivity: `curl http://127.0.0.1:27042/api/debug/state`

### "Waiting for x64dbg plugin..." hangs

The server waits up to 2 minutes for the plugin. Start x64dbg **before** your MCP client, or restart the client after x64dbg is running.

### Tools return errors about debugger state

- **Paused required**: Most inspection tools (registers, memory, disassembly) need the target to be paused
- **Running required**: `pause` and `force_pause` need the target to be running
- **Loaded required**: A target executable must be loaded in x64dbg

### 32-bit vs 64-bit

Use the correct plugin for your target:
- 64-bit process: x64dbg with `x64dbg_mcp.dp64`
- 32-bit process: x32dbg with `x64dbg_mcp.dp32`

Both use the same MCP server - just `npx -y x64dbg-mcp-server`.

### Plugin commands in x64dbg

You can control the REST API from the x64dbg command bar:

```
mcpserver start     Start the HTTP server
mcpserver stop      Stop the HTTP server
mcpserver status    Show server status and port
```

## Security

- The C++ plugin binds to `127.0.0.1` only - no remote access
- The MCP server communicates exclusively via stdio
- All HTTP traffic stays on localhost - no data leaves your machine

## Links

- [GitHub Repository](https://github.com/bromoket/x64dbg_mcp) - source code, C++ plugin build instructions
- [Plugin Releases](https://github.com/bromoket/x64dbg_mcp/releases) - prebuilt plugin DLLs
- [x64dbg](https://x64dbg.com/) - the debugger

## Author

**bromo** - [GitHub](https://github.com/bromoket)

Built with [Claude Code](https://claude.ai/claude-code) by Anthropic.

## License

MIT
