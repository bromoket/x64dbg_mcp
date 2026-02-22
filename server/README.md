# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)](https://nodejs.org/)

An [MCP server](https://modelcontextprotocol.io/) that gives AI assistants full control over the [x64dbg](https://x64dbg.com/) debugger. **20 powerful mega-tools** using Zod discriminated unions - stepping, breakpoints, memory, disassembly, tracing, anti-debug bypasses, control flow analysis, and more.

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
      "type": "stdio",
      "command": "cmd",
      "args": [
        "/c",
        "npx",
        "-y",
        "x64dbg-mcp-server"
      ]
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

- **TypeScript MCP Server** (`x64dbg-mcp-server` on npm) implements the MCP protocol over stdio. The 20 mega-tools use Zod discriminated unions to validate parameters before routing requests to one of the 152 specific REST endpoints on the plugin via localhost HTTP.

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

## Tool Reference (20 Mega-Tools)

The server exposes exactly **20 powerful mega-tools** using Zod discriminated unions. When calling a tool, provide the `action` argument to determine the exact operation.

| Tool | Actions (Operations) | Description |
|------|-----------|-------------|
| `x64dbg_debug` | `run`, `pause`, `force_pause`, `step_into`, `step_over`, `step_out`, `stop_debug`, `restart_debug`, `run_to_address`, `state` | Control debugger execution and query current state |
| `x64dbg_registers` | `all`, `specific`, `flags`, `avx512`, `set_register` | Read or write CPU registers |
| `x64dbg_memory` | `read`, `write`, `info`, `is_valid`, `is_code`, `allocate`, `free`, `protect`, `map`, `update_map` | Read, write, allocate, and analyze memory regions |
| `x64dbg_disassembly` | `disassemble`, `disassemble_function`, `instruction_info`, `assemble` | Assemble/disassemble code and analyze instructions |
| `x64dbg_breakpoints`| `set_software`, `set_hardware`, `set_memory`, `delete`, `enable`, `disable`, `toggle`, `set_condition`, `set_log`, `reset_hit_count`, `list`, `configure`, `configure_batch` | Comprehensive breakpoint management |
| `x64dbg_symbols` | `resolve_name`, `resolve_address`, `search_pattern`, `list_module`, `get_label`, `set_label`, `get_comment`, `set_comment`, `set_bookmark`, `clear_bookmark` | Resolve symbols, names, labels, and bookmarks |
| `x64dbg_stack` | `get_call_stack`, `read`, `pointers`, `seh_chain`, `return_address`, `comment` | Call stack and stack memory analysis |
| `x64dbg_threads` | `list`, `current`, `count`, `info`, `teb`, `name`, `switch`, `suspend`, `resume` | Thread enumeration and control |
| `x64dbg_modules` | `list`, `get_info`, `get_base`, `get_section`, `get_party` | Inspect loaded modules and sections |
| `x64dbg_search` | `pattern`, `string`, `string_at`, `symbol_auto_complete`, `encode_type` | Search memory for byte patterns or strings |
| `x64dbg_command` | `execute`, `evaluate`, `format`, `execute_script`, `init_script_get`, `init_script_set`, `db_hash`, `events` | Execute raw x64dbg commands, scripts, or evaluate expressions |
| `x64dbg_analysis` | `xrefs_to`, `xrefs_from`, `function_info`, `basic_blocks`, `strings`, `source`, `mnemonic`, `constants`, `errors`, `structs`, `va_to_file`, `file_to_va` | Deep binary analysis, cross-references, and database querying |
| `x64dbg_tracing` | `into`, `over`, `run`, `stop`, `log_setup`, `animate`, `conditional_run`, `hit_count`, `branch_dest`, `will_jump`, `set_record_type` | Advanced execution tracing and hit-counters |
| `x64dbg_dumping` | `dump_module`, `pe_header`, `sections`, `imports`, `exports`, `relocations`, `entry_point`, `fix_iat` | Dump modules from memory and manipulate PE headers |
| `x64dbg_antidebug` | `hide_debugger`, `peb`, `teb`, `dep` | Patch PEB/TEB to bypass anti-debugging protections |
| `x64dbg_exceptions`| `set`, `delete`, `list`, `list_codes`, `skip` | Manage exception handling and breakpoints |
| `x64dbg_process` | `basic`, `detailed`, `cmdline`, `elevated`, `dbversion`, `set_cmdline` | Query process info and manipulate command arguments |
| `x64dbg_handles` | `list_handles`, `list_tcp`, `list_windows`, `list_heaps`, `get_name`, `close` | Inspect handles, networking, and windows associated with the process |
| `x64dbg_control_flow`| `cfg`, `branch_dest`, `is_jump_taken`, `loops`, `func_type`, `add_function`, `delete_function` | Perform control-flow graph (CFG) analysis |
| `x64dbg_patches` | `list`, `apply`, `restore`, `export` | Apply and export inline byte patches |

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
