import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';

import { registerDebugTools } from './debug.js';
import { registerRegisterTools } from './registers.js';
import { registerMemoryTools } from './memory.js';
import { registerDisassemblyTools } from './disassembly.js';
import { registerBreakpointTools } from './breakpoints.js';
import { registerSymbolTools } from './symbols.js';
import { registerStackTools } from './stack.js';
import { registerThreadTools } from './threads.js';
import { registerModuleTools } from './modules.js';
import { registerMemmapTools } from './memmap.js';
import { registerSearchTools } from './search.js';
import { registerCommandTools } from './command.js';
import { registerAnalysisTools } from './analysis.js';
import { registerTracingTools } from './tracing.js';
import { registerDumpingTools } from './dumping.js';
import { registerAntiDebugTools } from './antidebug.js';
import { registerExceptionTools } from './exceptions.js';
import { registerProcessTools } from './process.js';
import { registerHandleTools } from './handles.js';
import { registerControlFlowTools } from './controlflow.js';
import { registerPatchTools } from './patches.js';

export function registerAllTools(server: McpServer) {
  registerDebugTools(server);        // 11 tools (10 + force_pause)
  registerRegisterTools(server);     // 5 tools
  registerMemoryTools(server);       // 9 tools
  registerDisassemblyTools(server);  // 4 tools
  registerBreakpointTools(server);   // 14 tools (11 + configure_breakpoint + configure_breakpoints + reset_breakpoint_hit_count)
  registerSymbolTools(server);       // 9 tools
  registerStackTools(server);        // 7 tools
  registerThreadTools(server);       // 9 tools
  registerModuleTools(server);       // 5 tools
  registerMemmapTools(server);       // 2 tools
  registerSearchTools(server);       // 5 tools
  registerCommandTools(server);      // 8 tools
  registerAnalysisTools(server);     // 13 tools
  registerTracingTools(server);      // 10 tools
  registerDumpingTools(server);      // 9 tools
  registerAntiDebugTools(server);    // 4 tools
  registerExceptionTools(server);    // 5 tools
  registerProcessTools(server);      // 6 tools
  registerHandleTools(server);       // 6 tools
  registerControlFlowTools(server);  // 7 tools
  registerPatchTools(server);        // 4 tools
  // Total: 152 tools
}
