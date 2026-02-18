# x64dbg MCP Server

A production-grade [Model Context Protocol](https://modelcontextprotocol.io/) server for [x64dbg](https://x64dbg.com/), enabling AI-powered debugging through Claude Code, Claude Desktop, or any MCP-compatible client.

## Architecture

```
+------------------+         +-------------------+          +-------------------+
|                  |  stdio   |                   |  HTTP    |                   |
|  Claude Code /   |<-------->|  TypeScript MCP   |<-------->|  C++ Plugin DLL   |
|  Claude Desktop  |          |  Server (Node.js) | localhost|  (x64dbg_mcp.dp64)|
|                  |          |                   |          |                   |
+------------------+         +-------------------+          +---+---------------+
                                                                 |
                                                                 | Bridge API
                                                                 v
                                                            +-------------------+
                                                            |  x64dbg Debugger  |
                                                            +-------------------+
```

**Two-component design:**
- **C++ Plugin** (`x64dbg_mcp.dp64`) - Lightweight REST API server inside x64dbg
- **TypeScript MCP Server** - Official SDK, stdio transport, 65 typed tools

**Why stdio?** No SSE reconnection issues, no port conflicts, no dropped connections. Just works.

## Features

- **65 MCP tools** across 12 categories
- **Fully typed** - every parameter has a Zod schema with descriptions
- **Stdio transport** - rock-solid connection (no SSE drops)
- **Official MCP SDK** - full protocol compliance
- **Thread-safe** bridge executor for x64dbg API calls
- **Localhost-only** binding - no security exposure

### Tool Categories

| Category | Tools | Description |
|----------|-------|-------------|
| Debug Control | 9 | run, pause, step_into/over/out, stop, restart |
| Registers | 4 | get all, get one, set, decode flags |
| Memory | 7 | read, write, validate, page info, alloc, free, protect |
| Disassembly | 4 | disassemble, function view, instruction info, assemble |
| Breakpoints | 11 | set/delete/enable/disable software/hardware/memory BPs |
| Symbols | 9 | resolve, search, labels, comments, bookmarks |
| Stack | 4 | call stack, read stack, pointers, SEH chain |
| Threads | 7 | list, switch, suspend, resume |
| Modules | 3 | list, info, base address |
| Memory Map | 2 | full map, region query |
| Search | 2 | byte pattern (AOB), string search |
| Command | 3 | raw command, expression eval, batch script |

## Prerequisites

- [x64dbg](https://github.com/x64dbg/x64dbg/releases) (latest snapshot)
- [Node.js](https://nodejs.org/) >= 18
- [Visual Studio 2022](https://visualstudio.microsoft.com/) with C++ workload (for building plugin)
- [CMake](https://cmake.org/) >= 3.20
- [Ninja](https://ninja-build.org/)
- [vcpkg](https://vcpkg.io/) (for nlohmann-json dependency)
- Clang-cl (ships with VS 2022)

## Building

### C++ Plugin

```powershell
cd plugin

# Configure (ensure VCPKG_ROOT is set)
cmake --preset x64-release

# Build
cmake --build build/x64-release

# Output: build/x64-release/bin/x64dbg_mcp.dp64
```

### TypeScript Server

```powershell
cd server
npm install
npm run build
```

## Installation

1. Copy `x64dbg_mcp.dp64` to your x64dbg `plugins/` directory
2. Start x64dbg - the plugin auto-starts the REST API on `127.0.0.1:27042`
3. You should see `[MCP] x64dbg MCP Server started on 127.0.0.1:27042` in the log

## Configuration

### Claude Code

Add to your MCP settings (`.claude/settings.json` or project-level):

```json
{
  "mcpServers": {
    "x64dbg": {
      "command": "node",
      "args": ["E:/GitHub/x64dbg_mcp/server/dist/index.js"],
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
      "args": ["E:/GitHub/x64dbg_mcp/server/dist/index.js"],
      "env": {
        "X64DBG_MCP_HOST": "127.0.0.1",
        "X64DBG_MCP_PORT": "27042"
      }
    }
  }
}
```

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
| `X64DBG_MCP_TIMEOUT` | `10000` | Request timeout (ms) |
| `X64DBG_MCP_RETRIES` | `2` | Request retry count |

## Usage Examples

Once configured, you can ask Claude to debug programs naturally:

- "Load notepad.exe and set a breakpoint on CreateFileW"
- "Step through the next 5 instructions and show me the register state"
- "Search for the string 'password' in the main module"
- "Disassemble the current function and add comments explaining each block"
- "Read 64 bytes of memory at the address pointed to by RSI"
- "Set a hardware breakpoint on write at the address in RAX"

## License

MIT
