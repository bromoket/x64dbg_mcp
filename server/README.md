# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Node.js](https://img.shields.io/badge/node-%3E%3D18-brightgreen)](https://nodejs.org/)

An [MCP server](https://modelcontextprotocol.io/) that gives AI assistants full control over the [x64dbg](https://x64dbg.com/) debugger. **152 tools** across **21 categories** - stepping, breakpoints, memory, disassembly, tracing, anti-debug bypasses, control flow analysis, and more.

## Setup

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

This npm package is the MCP server component. It communicates over stdio with your AI client, validates parameters with Zod schemas, and forwards requests to the C++ plugin running inside x64dbg over localhost HTTP.

The server waits up to 2 minutes for the plugin to become available, performs health checks every 15 seconds, and automatically reconnects if x64dbg restarts. Requests retry up to 3 times with exponential backoff.

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `3` | Retry count on transient failures |

Pass them in your MCP client config:

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

## Tool Reference (152 tools)

### Debug Control (11 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_health` | - | Check if the plugin is running and responsive |
| `get_debug_state` | - | Get debugger state (stopped/running/paused), CIP, module |
| `run` | - | Resume execution |
| `pause` | - | Pause execution |
| `force_pause` | - | Force pause even when high-frequency fast-resume BPs are active |
| `step_into` | - | Single step into calls |
| `step_over` | - | Single step over calls |
| `step_out` | - | Run until current function returns |
| `stop_debug` | - | Stop the debug session |
| `restart_debug` | - | Restart the debugged process |
| `run_to_address` | `address` | Run until a specific address is reached |

### Registers (5 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_all_registers` | - | Get all CPU registers (GP, segment, debug, flags) |
| `get_register` | `name` | Get a single register value (e.g., "rax", "eip") |
| `set_register` | `name`, `value` | Set a register to a new value |
| `get_flags` | - | Get decoded EFLAGS (CF, ZF, SF, OF, etc.) |
| `get_avx512_registers` | - | Get AVX-512 extended register dump |

### Memory (9 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `read_memory` | `address`, `size?` | Read bytes from address (hex dump + ASCII) |
| `write_memory` | `address`, `bytes`, `verify?` | Write bytes with optional readback verification |
| `is_valid_address` | `address` | Check if address is a valid readable pointer |
| `get_memory_info` | `address` | Get memory page protection info |
| `allocate_memory` | `size?` | Allocate memory via VirtualAllocEx |
| `free_memory` | `address` | Free allocated memory |
| `set_memory_protection` | `address`, `size?`, `protection` | Change page protection |
| `is_code_page` | `address` | Check if address is in executable memory |
| `update_memory_map` | - | Force refresh the memory map |

### Disassembly (4 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `disassemble` | `address?`, `count?` | Disassemble N instructions at address |
| `disassemble_function` | `address?`, `max_instructions?` | Disassemble an entire function |
| `get_instruction_info` | `address?` | Get instruction details (size, is_call, is_branch) |
| `assemble` | `address`, `instruction` | Assemble an instruction at address |

### Breakpoints (14 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `set_breakpoint` | `address`, `singleshot?` | Set a software breakpoint (INT3) |
| `set_hardware_breakpoint` | `address`, `type?`, `size?` | Set a hardware breakpoint (r/w/x) |
| `set_memory_breakpoint` | `address`, `type?` | Set a memory breakpoint (page guard) |
| `delete_breakpoint` | `address`, `type?` | Delete a breakpoint |
| `enable_breakpoint` | `address` | Enable a disabled breakpoint |
| `disable_breakpoint` | `address` | Disable without deleting |
| `toggle_breakpoint` | `address` | Toggle enabled/disabled |
| `list_breakpoints` | - | List all breakpoints with resolved symbol labels |
| `get_breakpoint` | `address` | Get detailed info about a specific breakpoint |
| `set_breakpoint_condition` | `address`, `condition` | Set a break condition (e.g., "eax==0") |
| `set_breakpoint_log` | `address`, `text` | Set a log format string |
| `configure_breakpoint` | `address`, `bp_type?`, `break_condition?`, `log_text?`, `fast_resume?`, ... | **Unified**: set BP + all options in one call |
| `configure_breakpoints` | `breakpoints[]` | **Batch**: configure multiple breakpoints in one call |
| `reset_breakpoint_hit_count` | `address` | Reset hit counter to zero |

### Symbols (9 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `resolve_symbol` | `name` | Resolve symbol name to address |
| `get_symbol_at` | `address` | Get the symbol/label name at an address |
| `search_symbols` | `pattern`, `module?` | Search symbols by wildcard pattern |
| `list_module_symbols` | `module` | Load and list all symbols for a module |
| `get_label` | `address` | Get user-defined label at address |
| `set_label` | `address`, `text` | Set a user-defined label |
| `get_comment` | `address` | Get comment at address |
| `set_comment` | `address`, `text` | Set a comment at address |
| `set_bookmark` | `address`, `set?` | Set or clear a bookmark |

### Stack (7 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_call_stack` | `max_depth?` | Get the call stack (stack trace) |
| `read_stack` | `address?`, `size?` | Read raw stack memory with symbol resolution |
| `get_stack_pointers` | - | Get current RSP/ESP and RBP/EBP |
| `get_seh_chain` | - | Get the SEH handler chain |
| `get_return_address` | - | Get return address from top of stack |
| `get_stack_comment` | `address` | Get stack comment at address |
| `get_callstack_by_thread` | `handle` | Get call stack for a specific thread |

### Threads (9 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_threads` | - | List all threads with IDs, names, and start addresses |
| `get_current_thread` | - | Get info about the currently active thread |
| `get_thread` | `id` | Get detailed info about a specific thread |
| `switch_thread` | `id` | Switch debugger focus to a different thread |
| `suspend_thread` | `id` | Suspend a thread |
| `resume_thread` | `id` | Resume a suspended thread |
| `get_thread_count` | - | Get total thread count |
| `get_thread_teb` | `tid` | Get the TEB address for a thread |
| `get_thread_name` | `tid` | Get the name of a thread |

### Modules (5 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_modules` | - | List all loaded modules with base addresses and sizes |
| `get_module_info` | `name` | Get detailed info about a specific module |
| `get_module_base` | `name` | Get the base address of a module |
| `get_section_at` | `address` | Get PE section name at address |
| `get_module_party` | `base` | Check if module is user code or system code |

### Memory Map (2 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_memory_map` | - | Get the full virtual memory map |
| `get_memory_region` | `address` | Get memory region info for a specific address |

### Search (5 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `search_pattern` | `pattern`, `address?`, `size?`, `max_results?` | Byte pattern (AOB) scan with `??` wildcards - returns all matches |
| `search_strings` | `text`, `module?`, `encoding?` | Search for string references in memory |
| `get_string_at` | `address`, `encoding?`, `max_length?` | Read string at address (auto/ascii/unicode) |
| `symbol_auto_complete` | `search`, `max_results?` | Auto-complete a partial symbol name |
| `get_encode_type` | `address`, `size?` | Get data encoding type at address |

### Command (8 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `execute_command` | `command` | Execute any raw x64dbg command |
| `evaluate_expression` | `expression` | Evaluate an expression to numeric value |
| `format_string` | `format` | Format string using x64dbg expression engine |
| `execute_script` | `commands[]` | Execute a batch of commands sequentially |
| `get_debug_events` | - | Get the total debug event count |
| `get_init_script` | - | Get the initialization script path |
| `set_init_script` | `file` | Set the initialization script |
| `get_database_hash` | - | Get the current x64dbg database hash |

### Analysis (13 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_xrefs_to` | `address` | Cross-references TO an address (who calls this) |
| `get_xrefs_from` | `address` | Cross-references FROM an address |
| `get_function_info` | `address?` | Function boundaries, label, and module |
| `get_basic_blocks` | `address?` | Basic blocks (CFG nodes) for a function |
| `find_strings_in_module` | `module` | Find all string references in a module |
| `list_constants` | - | List all known constants |
| `list_error_codes` | - | List all known Windows error codes |
| `is_watchdog_triggered` | `id?` | Check if a watch expression has triggered |
| `list_structs` | - | List all defined structure types |
| `get_source_location` | `address?` | Get source file and line number (requires PDB) |
| `va_to_file_offset` | `address` | Convert virtual address to file offset |
| `file_offset_to_va` | `module`, `offset` | Convert file offset to virtual address |
| `get_mnemonic_brief` | `mnemonic` | Brief description of an instruction mnemonic |

### Tracing (10 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `trace_into` | `condition?`, `max_steps?`, `log_text?` | Conditional trace into (follows calls) |
| `trace_over` | `condition?`, `max_steps?`, `log_text?` | Conditional trace over (skips calls) |
| `trace_run` | `party?` | Run to user code (0) or system code (1) |
| `trace_stop` | - | Stop an active trace |
| `trace_log_setup` | `file`, `text?`, `condition?` | Set up trace logging to a file |
| `animate_command` | `command` | Single-step with visual update |
| `get_trace_hit_count` | `address` | Get how many times an instruction was traced |
| `conditional_run` | `break_condition?`, `log_text?`, `log_condition?`, ... | Full conditional trace with break/log/command control |
| `get_branch_destination` | `address?` | Get destination of a branch/jump/call |
| `is_jump_going_to_execute` | `address?` | Check if conditional jump will be taken |

### Dumping (9 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `dump_module` | `module`, `file?` | Dump a module from memory to disk |
| `get_pe_header` | `address` | Parse PE header from module in memory |
| `get_sections` | `module` | Get PE section headers |
| `get_imports` | `module` | Display import address table (IAT) |
| `get_exports` | `module` | Display export table |
| `fix_iat` | `oep` | IAT reconstruction using Scylla |
| `get_relocations` | `address` | Get relocation entries |
| `export_patched_file` | `filename` | Export all patches to a patched executable |
| `get_entry_point` | `module` | Get module entry point address |

### Anti-Debug (4 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `hide_debugger` | - | Zero PEB.BeingDebugged and NtGlobalFlag |
| `get_peb_info` | `pid?` | Read PEB fields (BeingDebugged, NtGlobalFlag, ProcessHeap) |
| `get_teb_info` | `tid?` | Read TEB fields (SEH chain, stack base/limit, PEB pointer) |
| `get_dep_status` | - | Check if DEP is enabled |

### Exceptions (5 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `set_exception_breakpoint` | `code`, `chance?`, `action?` | Set breakpoint on specific exception code |
| `delete_exception_breakpoint` | `code` | Delete an exception breakpoint |
| `list_exception_breakpoints` | - | List all active exception breakpoints |
| `list_exception_codes` | - | List all known Windows exception codes |
| `skip_exception` | - | Skip/pass current exception and continue |

### Process (6 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_process_info` | - | Detailed process info (PID, PEB, handles, elevation, DEP) |
| `get_basic_process_info` | - | Quick process info (PID, PEB, entry point, state) |
| `is_process_elevated` | - | Check if running as administrator |
| `get_cmdline` | - | Get command line of debugged process |
| `set_cmdline` | `cmdline` | Set command line (takes effect on restart) |
| `get_debugger_version` | - | Get x64dbg bridge version number |

### Handles (6 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_handles` | - | List all open handles (files, registry keys, mutexes) |
| `get_handle_name` | `handle` | Get the name/path of a handle |
| `close_handle` | `handle` | Force-close a handle |
| `list_tcp_connections` | - | List TCP connections with addresses and ports |
| `list_windows` | - | List all windows with titles, classes, and WndProc |
| `list_heaps` | - | List all heaps with addresses, sizes, and flags |

### Control Flow (7 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `get_cfg` | `address?` | Full control flow graph with nodes and branches |
| `get_loops` | `address?` | Loop detection with nesting depth |
| `get_branch_destination` | `address?` | Get destination of branch/jump/call |
| `is_jump_going_to_execute` | `address?` | Check if conditional jump will be taken |
| `get_function_type` | `address?` | Get function type (none/begin/middle/end/single) |
| `add_function` | `start`, `end` | Define a function boundary in the database |
| `delete_function` | `address` | Delete a function definition |

### Patches (4 tools)

| Tool | Parameters | Description |
|------|-----------|-------------|
| `list_patches` | - | List all current byte patches |
| `apply_patch` | `address`, `bytes` | Apply a byte patch at an address |
| `restore_patch` | `address` | Restore original bytes at a patched address |
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

## Tech Stack

- **Runtime**: Node.js >= 18
- **Language**: TypeScript (ES2022, strict mode)
- **MCP SDK**: `@modelcontextprotocol/sdk` - official MCP SDK
- **Validation**: `zod` - runtime type checking for all 152 tool schemas
- **Transport**: stdio (stdin/stdout)

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
