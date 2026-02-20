# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A [Model Context Protocol](https://modelcontextprotocol.io/) server for [x64dbg](https://x64dbg.com/), giving AI assistants full control over the debugger through **152 tools** across **21 categories**.

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

## Prerequisites

1. **x64dbg** with the MCP plugin installed ([download plugin from releases](https://github.com/bromoket/x64dbg_mcp/releases))
2. **Node.js** >= 18

## Quick Start

### 1. Install the x64dbg plugin

Download `x64dbg_mcp.dp64` / `x64dbg_mcp.dp32` from [GitHub Releases](https://github.com/bromoket/x64dbg_mcp/releases) and copy to your x64dbg `plugins/` directory.

### 2. Configure your MCP client

**Claude Code** - add to `.claude/settings.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["x64dbg-mcp-server"]
    }
  }
}
```

**Claude Desktop** - add to `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["x64dbg-mcp-server"]
    }
  }
}
```

### 3. Start debugging

Open a target in x64dbg, then ask your AI assistant:

```
"Set a breakpoint on CreateFileW and run"
"Disassemble the current function and explain what it does"
"Search for the byte pattern 48 8B ?? 48 85 C0 in the main module"
"Hide the debugger and bypass the anti-debug checks"
"Trace into the function and log all instructions to a file"
```

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `X64DBG_MCP_HOST` | `127.0.0.1` | Plugin REST API host |
| `X64DBG_MCP_PORT` | `27042` | Plugin REST API port |
| `X64DBG_MCP_TIMEOUT` | `30000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `3` | Request retry count |

## Tool Categories (152 tools)

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
| Search | 5 | byte pattern (AOB) scan returning all matches, string search, find strings in module, symbol search, constants |
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

For the full tool reference with individual descriptions, see the [GitHub README](https://github.com/bromoket/x64dbg_mcp#tool-reference).

## Security

- The C++ plugin binds to `127.0.0.1` only - no remote access
- The MCP server communicates exclusively via stdio (stdin/stdout)
- All communication is local - no authentication needed

## Links

- [GitHub Repository](https://github.com/bromoket/x64dbg_mcp) - source code, C++ plugin build instructions, full tool reference
- [Plugin Releases](https://github.com/bromoket/x64dbg_mcp/releases) - prebuilt plugin DLLs
- [x64dbg](https://x64dbg.com/) - the debugger

## License

MIT
