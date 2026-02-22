# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)](https://nodejs.org/)

An [MCP server](https://modelcontextprotocol.io/) that gives AI assistants full control over the [x64dbg](https://x64dbg.com/) debugger. **23 mega-tools** covering 151 REST endpoints via Zod discriminated unions - stepping, breakpoints, memory, disassembly, tracing, anti-debug bypasses, control flow analysis, PE dumping, and more.

## Quick Start

### Prerequisites

1. **x64dbg** - [Download latest snapshot](https://github.com/x64dbg/x64dbg/releases)
2. **Node.js** >= 18 - [Download](https://nodejs.org/)
3. **MCP plugin** - [Download from releases](https://github.com/bromoket/x64dbg_mcp/releases) (`x64dbg_mcp.dp64` and/or `x64dbg_mcp.dp32`)

### Step 1: Install the Plugin

Download the plugin DLLs from [GitHub Releases](https://github.com/bromoket/x64dbg_mcp/releases) and copy them to your x64dbg plugins directories:

- `x64dbg_mcp.dp64` goes in `x64/plugins/`
- `x64dbg_mcp.dp32` goes in `x32/plugins/`

Start x64dbg. You should see: `[MCP] x64dbg MCP Server started on 127.0.0.1:27042`

### Step 2: Add to Your AI Client

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

```
 MCP Client  ──stdio──>  TypeScript MCP Server  ──HTTP──>  C++ Plugin (inside x64dbg)
 (Claude,                 23 mega-tools                     151 REST endpoints
  Cursor)                 Zod validation                    127.0.0.1:27042
```

- **C++ Plugin** runs inside x64dbg as a REST API on `127.0.0.1:27042`, wrapping the x64dbg Bridge/Plugin SDK with 151 JSON endpoints.
- **TypeScript MCP Server** (this package) implements the MCP protocol over stdio. 23 mega-tools use Zod discriminated unions to validate parameters and route to the correct endpoint.

The server waits up to 2 minutes for the plugin, health-checks every 15 seconds, and auto-reconnects if x64dbg restarts. Requests retry up to 3 times.

## Tool Reference (23 Mega-Tools)

Each tool accepts an `action` parameter that selects the specific operation.

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
| `x64dbg_control_flow` | `cfg`, `branch_dest`, `is_jump_taken`, `loops`, `func_type`, `add_function`, `delete_function` | Control flow graph, branch analysis, loop detection |
| `x64dbg_database` | `constants`, `error_codes`, `structs`, `strings` | Query x64dbg's analysis database |
| `x64dbg_address_convert` | `va_to_file`, `file_to_va` | Convert between virtual addresses and file offsets |
| `x64dbg_watchdog` | *(id parameter)* | Check if a watch expression watchdog triggered |

### Breakpoints & Tracing

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_breakpoints` | `set_software`, `set_hardware`, `set_memory`, `delete`, `enable`, `disable`, `toggle`, `set_condition`, `set_log`, `reset_hit_count`, `get`, `list`, `configure`, `configure_batch` | Full breakpoint management: software, hardware, memory, conditional, logging, batch |
| `x64dbg_tracing` | `into`, `over`, `run`, `stop`, `animate`, `conditional_run`, `log_setup`, `hitcount`, `type`, `set_type` | Execution tracing, trace logging, hit counters |
| `x64dbg_exceptions` | `set`, `delete`, `list`, `list_codes`, `skip` | Exception breakpoints and exception handling |

### Symbols & Annotations

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_symbols` | `resolve`, `address`, `search`, `list_module`, `get_label`, `set_label`, `get_comment`, `set_comment`, `bookmark` | Symbol resolution, labels, comments, bookmarks |
| `x64dbg_search` | `pattern`, `string`, `string_at`, `symbol_auto_complete`, `encode_type` | AOB/byte pattern scan, string search, symbol autocomplete |
| `x64dbg_modules` | `list`, `get_info`, `get_base`, `get_section`, `get_party` | Loaded modules, base addresses, sections |

### Process & System

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_process` | `basic`, `detailed`, `cmdline`, `elevated`, `dbversion`, `set_cmdline` | Process info, PID, PEB, elevation status |
| `x64dbg_threads` | `list`, `current`, `count`, `info`, `teb`, `name`, `switch`, `suspend`, `resume` | Thread enumeration, TEB, thread control |
| `x64dbg_handles` | `list_handles`, `list_tcp`, `list_windows`, `list_heaps`, `get_name`, `close` | Handles, TCP connections, windows, heaps |
| `x64dbg_antidebug` | `peb`, `teb`, `dep`, `hide_debugger` | PEB/TEB inspection, DEP, hide debugger |

### Patching & Dumping

| Tool | Actions | Description |
|------|---------|-------------|
| `x64dbg_patches` | `list`, `apply`, `restore`, `export` | Apply byte patches, restore originals, export patched module |
| `x64dbg_dumping` | `pe_header`, `sections`, `imports`, `exports`, `entry_point`, `relocations`, `dump_module`, `fix_iat`, `export_patch_file` | PE analysis, module dumping, IAT reconstruction, patch file export |

## Configuration

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `3` | Retry count on transient failures |

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

## Troubleshooting

### "Connection refused" or server can't reach plugin

1. Make sure x64dbg is running with a target loaded
2. Verify the plugin is in the correct `plugins/` directory
3. Check the x64dbg log for `[MCP] x64dbg MCP Server started on 127.0.0.1:27042`
4. Test manually: `curl http://127.0.0.1:27042/api/health`

### "Waiting for x64dbg plugin..." hangs

The server waits up to 2 minutes for the plugin. Start x64dbg **before** your MCP client, or restart the client after x64dbg is running.

### Tools return errors about debugger state

- **"Debugger must be paused"**: Inspection tools need paused state - hit a breakpoint or use pause first
- **"No active debug session"**: Load a target in x64dbg (`File > Open`)
- **"Debugger must be running"**: `pause`/`force_pause` need the target running

### 32-bit vs 64-bit

| Target | Debugger | Plugin |
|--------|----------|--------|
| 64-bit | x64dbg | `x64dbg_mcp.dp64` |
| 32-bit | x32dbg | `x64dbg_mcp.dp32` |

Both use the same MCP server.

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
