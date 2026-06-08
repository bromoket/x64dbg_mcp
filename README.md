# x64dbg MCP Server

[![npm version](https://img.shields.io/npm/v/x64dbg-mcp-server)](https://www.npmjs.com/package/x64dbg-mcp-server)
[![MCP Registry](https://img.shields.io/badge/MCP-registry-blue)](https://registry.modelcontextprotocol.io)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Drive [x64dbg](https://x64dbg.com/) with your AI.** Talk to Claude, Cursor, Windsurf, Cline,
or any MCP client in plain English and it sets breakpoints, reads memory, disassembles, traces,
dumps PEs, and bypasses anti-debug — live, inside the debugger.

**23 mega-tools** over **153 REST endpoints**, fully typed with Zod. A C++ plugin runs inside
x64dbg; a tiny TypeScript server bridges it to your client over stdio. Everything stays on
`127.0.0.1` — nothing leaves your machine.

> **Latest — v2.3.0**
> - **Hardened & crash-proof.** A malformed HTTP request can no longer crash x64dbg; the plugin
>   server drains connections cleanly on stop and ships an **optional auth token** (CORS is
>   locked down).
> - **Real data from more tools.** `imports`/`exports`, `symbols` search/list, `patches` list,
>   and `strings` now return actual parsed results instead of pointing you at a GUI view.
> - **Live trace status.** New `/api/trace/status` (+ `tracing status`) reports whether a trace
>   is running, and the exception/trace tools now honor every parameter they accept.
> - Plus the v2.2.x fixes: x32dbg loads on current snapshots, and requests no longer time out
>   on long operations.
>
> [Download the v2.3.0 plugins →](https://github.com/bromoket/x64dbg_mcp/releases/latest)

---

## What it looks like

```
"Set a breakpoint on CreateFileW and run the program"
"Disassemble the current function and explain what it does"
"Search for 48 8B ?? 48 85 C0 in the main module and disassemble the hits"
"Hide the debugger and bypass the anti-debug checks"
"Trace into the VM dispatcher and log every instruction to a file"
"Dump the main module to disk and fix the import table"
```

Real use: tracing VMProtect'd code, finding anti-cheat scanner threads, decoding XOR'd class
names, mapping detection logic — all by asking, no manual scripting.

## Install

### 1 · Plugin (inside x64dbg)

**Download** `x64dbg_mcp.dp64` / `.dp32` from the
[latest release](https://github.com/bromoket/x64dbg_mcp/releases/latest) and drop them in:

```
x64dbg/x64/plugins/x64dbg_mcp.dp64    ← 64-bit targets
x64dbg/x32/plugins/x64dbg_mcp.dp32    ← 32-bit targets
```

…or **build + install** it yourself (auto-detects your x64dbg — no path editing):

```powershell
.\build.ps1 -Install
```

Start x64dbg; the log shows `[MCP] x64dbg MCP Server started on 127.0.0.1:27042`.

### 2 · Server (your AI client)

No install — just point your client at npx. Claude Code:

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

Claude Desktop / Cursor / Windsurf / Cline use the same block without the `cmd /c` wrapper:
`{ "command": "npx", "args": ["-y", "x64dbg-mcp-server"] }`.
Full per-client paths are in the [reference](docs/REFERENCE.md#configuration).

### 3 · Go

Open a target in x64dbg and start talking to your assistant.

## Tools at a glance

23 action-based tools spanning the whole debugger:

- **Control** — run/step/pause, raw commands, scripts, expression eval
- **CPU & memory** — registers (incl. AVX-512), read/write/alloc/protect, memory map
- **Stack** — call stack, SEH chain, return addresses
- **Code analysis** — disassemble, assemble, xrefs, basic blocks, CFG, loops
- **Breakpoints & tracing** — software/hardware/memory/conditional/logging, batch, trace logs
- **Symbols & search** — labels, comments, bookmarks, AOB pattern + string scan
- **Process & system** — threads/TEB, handles, TCP, PEB, **anti-debug hide**
- **Patching & dumping** — byte patches, PE dump, IAT fix, patch export

Every tool, action, and endpoint is documented in **[docs/REFERENCE.md](docs/REFERENCE.md)**.

## Links

- **[Full reference](docs/REFERENCE.md)** — tools, architecture, build, config, troubleshooting
- **[npm: x64dbg-mcp-server](https://www.npmjs.com/package/x64dbg-mcp-server)**
- **[Releases](https://github.com/bromoket/x64dbg_mcp/releases)** — prebuilt plugin DLLs
- **[x64dbg](https://x64dbg.com/)** — the debugger

## Security

The plugin binds to `127.0.0.1` only; the server talks pure stdio. All traffic stays on
localhost — no remote access, no telemetry, no data leaves your machine. For defense against
other local processes, set a token in the plugin's **Settings** and pass it via
`X64DBG_MCP_TOKEN` — every request must then carry it.

## Author

**bromo** — [GitHub](https://github.com/bromoket). Built with
[Claude Code](https://claude.ai/claude-code). [MIT](LICENSE).
