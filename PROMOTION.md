# Promotion Checklist - x64dbg MCP Server

Everything below is ready to copy-paste. Work through the list.

---

## 1. MCP Official Registry (HIGH PRIORITY)

**URL**: https://registry.modelcontextprotocol.io

They have a CLI publisher tool. Follow the guide at the registry site under "Adding Servers to the MCP Registry" for server maintainers. Your package name is `x64dbg-mcp-server` on npm.

---

## 2. awesome-mcp-servers (HIGH PRIORITY)

### Option A: mcpservers.org (punkpeye's list)
**Submit at**: https://mcpservers.org/submit

### Option B: wong2/awesome-mcp-servers
**URL**: https://github.com/wong2/awesome-mcp-servers
Open a PR adding this line under a "Debugging / Reverse Engineering" category (or wherever it fits):

```markdown
- **[x64dbg MCP Server](https://github.com/bromoket/x64dbg_mcp)** - 152-tool MCP server for the x64dbg debugger. Full control over debugging, disassembly, breakpoints, tracing, memory analysis, and anti-debug bypass. C++ plugin + TypeScript server. `npx x64dbg-mcp-server`
```

### Option C: appcypher/awesome-mcp-servers
**URL**: https://github.com/appcypher/awesome-mcp-servers
Same - open a PR with the line above.

---

## 3. x64dbg Wiki Plugin List

**URL**: https://github.com/x64dbg/x64dbg/wiki/Plugins

The wiki is editable. Add an entry:

```markdown
### [x64dbg MCP Server](https://github.com/bromoket/x64dbg_mcp)
AI-powered debugging through the Model Context Protocol. 152 tools give LLMs (Claude, Cursor, etc.) full control over x64dbg - breakpoints, memory, disassembly, tracing, anti-debug, and more. C++ plugin + TypeScript MCP server on npm.
```

---

## 4. awesome-ida-x64-olly-plugin

**URL**: https://github.com/fr0gger/awesome-ida-x64-olly-plugin
Open a PR. Add under x64dbg section:

```markdown
- [x64dbg MCP Server](https://github.com/bromoket/x64dbg_mcp) - MCP server giving AI assistants (Claude, Cursor, Windsurf) full control over x64dbg. 152 tools for debugging, disassembly, tracing, memory analysis, anti-debug bypass.
```

---

## 5. Reddit Posts

### r/ReverseEngineering

**Title**: I built an MCP server that lets AI control x64dbg - 152 tools for AI-powered reverse engineering

**Body**:
```
I've been working on an MCP (Model Context Protocol) server for x64dbg that gives AI assistants like Claude, Cursor, and Windsurf full control over the debugger.

**What it does**: 152 tools across 21 categories - breakpoints, memory read/write, disassembly, tracing, anti-debug bypass, control flow graphs, IAT dumping, pattern scanning, and more. You talk to the AI in natural language and it operates x64dbg for you.

**How it works**: A C++ plugin runs inside x64dbg as a REST API on localhost. A TypeScript MCP server bridges that to any MCP-compatible AI client over stdio. All local, nothing leaves your machine.

**Example prompts**:
- "Set a breakpoint on CreateFileW, run, then show me the call stack"
- "Search for the byte pattern 48 8B ?? 48 85 C0 and disassemble whatever you find"
- "Hide the debugger from anti-debug checks and trace through the VM dispatcher"
- "Configure 8 logging breakpoints in one call that log function arguments"

**Install**: Copy the plugin DLL to x64dbg, add one JSON block to your AI client config pointing at `npx x64dbg-mcp-server`.

GitHub: https://github.com/bromoket/x64dbg_mcp
npm: https://www.npmjs.com/package/x64dbg-mcp-server

Open source, MIT licensed. Feedback welcome.
```

### r/ClaudeAI

**Title**: Built a 152-tool MCP server for x64dbg - AI-powered binary debugging and reverse engineering

**Body**:
```
Made an MCP server that connects Claude (Code or Desktop) to the x64dbg debugger. 152 fully-typed tools let Claude set breakpoints, read memory, disassemble functions, trace execution, bypass anti-debug, dump modules, and more.

Setup is just:
1. Copy plugin DLL to x64dbg
2. Add to your Claude config:
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}

Then you can say things like "disassemble the current function and explain what it does" or "set a conditional breakpoint that only triggers when EAX == 0" and Claude does it.

Works with Claude Code, Claude Desktop, Cursor, Windsurf, Cline.

GitHub: https://github.com/bromoket/x64dbg_mcp
npm: https://www.npmjs.com/package/x64dbg-mcp-server
```

### r/Malware

**Title**: MCP server for x64dbg - let AI assist your malware analysis (152 debugging tools)

**Body**:
```
Built an MCP server that bridges AI assistants to x64dbg. Useful for malware analysis workflows where you want AI to help navigate through packed/obfuscated code.

152 tools including:
- Anti-debug: hide debugger, read/modify PEB, DEP status
- Tracing: conditional trace with logging, hit counting
- Memory: AOB pattern scan (returns all matches), read/write with verify
- Breakpoints: batch configure (set 8 logging BPs in one call), fast resume
- Analysis: xrefs, CFG, basic blocks, imports/exports, PE header parsing
- Dumping: module dump, IAT fix via Scylla, export patched files

Everything runs locally on 127.0.0.1. The C++ plugin sits inside x64dbg, the MCP server bridges it to Claude/Cursor/etc over stdio.

GitHub: https://github.com/bromoket/x64dbg_mcp
npm: https://www.npmjs.com/package/x64dbg-mcp-server

MIT licensed.
```

### r/cursor

**Title**: x64dbg MCP server - 152 debugging/RE tools for Cursor

**Body**:
```
If anyone here does reverse engineering or binary debugging, I built an MCP server that connects Cursor to the x64dbg debugger.

152 tools: breakpoints, memory, disassembly, tracing, anti-debug, pattern scanning, control flow graphs, etc.

Add to .cursor/mcp.json:
{
  "mcpServers": {
    "x64dbg": {
      "command": "npx",
      "args": ["-y", "x64dbg-mcp-server"]
    }
  }
}

You also need the x64dbg plugin DLL from the releases page.

GitHub: https://github.com/bromoket/x64dbg_mcp
```

---

## 6. Hacker News - Show HN

**Title**: Show HN: 152-tool MCP server that lets AI control the x64dbg debugger

**URL**: https://github.com/bromoket/x64dbg_mcp

**Comment** (post as first comment on your own submission):
```
I built this because I wanted Claude to help me reverse engineer binaries without constantly copy-pasting between the AI and x64dbg.

It's two components:
- A C++ plugin that runs inside x64dbg as a REST API on localhost
- A TypeScript MCP server on npm that bridges to any MCP client (Claude, Cursor, Windsurf, etc.)

152 tools covering: debug control, registers, memory, disassembly, breakpoints (including batch configure), symbols, stack, threads, modules, search (AOB pattern scan), command execution, analysis (xrefs, CFG), tracing, dumping (with Scylla IAT fix), anti-debug bypass, exceptions, process inspection, handles, control flow, and patching.

Everything is local - the plugin binds to 127.0.0.1 only, the MCP server uses stdio. No data leaves your machine.

Real use case: I used it to analyze a game's anti-cheat system. Claude traced through VMProtect'd code, identified FindWindowA-based scanner threads, decoded XOR-encrypted class names, and mapped out the detection logic - all through natural language commands.

Install: copy plugin DLL + add one JSON config block pointing at `npx x64dbg-mcp-server`.
```

---

## 7. Twitter/X

**Post 1 - Launch announcement**:
```
I built an MCP server that gives AI full control over the x64dbg debugger.

152 tools: breakpoints, memory, disassembly, tracing, anti-debug bypass, CFG, pattern scanning, IAT dumping...

Works with Claude, Cursor, Windsurf, Cline.

One config line: npx x64dbg-mcp-server

github.com/bromoket/x64dbg_mcp

#ReverseEngineering #MCP #AI #x64dbg
```

**Post 2 - Use case thread**:
```
How I reversed an anti-cheat using AI + x64dbg:

1/ Claude set logging breakpoints on FindWindowA with fast resume
2/ Traced through VMProtect'd code at 46 hits/sec
3/ Identified XOR-encrypted window class names (0xF4AACCBC key)
4/ Mapped 5 scanner call sites + 2 scan threads (4s and 300ms intervals)

All through natural language. No manual scripting.

This is what 152 MCP tools looks like in practice.
github.com/bromoket/x64dbg_mcp
```

**Tag**: @anthropic @x64dbg

---

## 8. YouTube / Video Content

Record a 2-3 minute demo showing:
1. Open a packed binary in x64dbg
2. Claude: "Hide the debugger and set a breakpoint on VirtualAlloc"
3. Claude: "Run and show me the call stack when it hits"
4. Claude: "Search for the string 'license' in the main module"
5. Claude: "Disassemble that function and explain it"

Title: "AI-Powered Reverse Engineering with Claude + x64dbg (152 MCP Tools)"

Upload to YouTube, embed in README.

---

## Done Already (automated)

- [x] GitHub topics: mcp, mcp-server, x64dbg, debugger, reverse-engineering, model-context-protocol, claude, ai-debugging, malware-analysis, binary-analysis, plugin, typescript, cpp, x64dbg-plugin
- [x] GitHub repo description updated
- [x] npm published with keywords
- [x] READMEs restructured with multi-client setup guides

## Priority Order

1. MCP Official Registry (registry.modelcontextprotocol.io)
2. awesome-mcp-servers (mcpservers.org/submit)
3. Hacker News Show HN
4. Reddit r/ReverseEngineering
5. Reddit r/ClaudeAI
6. Twitter/X launch post
7. x64dbg wiki
8. awesome-ida-x64-olly-plugin PR
9. Reddit r/Malware, r/cursor
10. YouTube demo video
