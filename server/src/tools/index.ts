import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';

import { registerAnalysisTools } from './analysis.js';
import { registerAntiDebugTools } from './antidebug.js';
import { registerBreakpointTools } from './breakpoints.js';
import { registerCommandTools } from './command.js';
import { registerControlFlowTools } from './controlflow.js';
import { registerDebugTools } from './debug.js';
import { registerDisassemblyTools } from './disassembly.js';
import { registerDumpingTools } from './dumping.js';
import { registerExceptionTools } from './exceptions.js';
import { registerHandleTools } from './handles.js';
import { registerMemoryTools } from './memory.js';
import { registerModuleTools } from './modules.js';
import { registerPatchTools } from './patches.js';
import { registerProcessTools } from './process.js';
import { registerRegisterTools } from './registers.js';
import { registerSearchTools } from './search.js';
import { registerStackTools } from './stack.js';
import { registerSymbolTools } from './symbols.js';
import { registerThreadTools } from './threads.js';
import { registerTracingTools } from './tracing.js';

export function registerAllTools(server: McpServer) {
  registerDebugTools(server);
  registerRegisterTools(server);
  registerMemoryTools(server);
  registerDisassemblyTools(server);
  registerBreakpointTools(server);
  registerSymbolTools(server);
  registerStackTools(server);
  registerThreadTools(server);
  registerModuleTools(server);
  registerSearchTools(server);
  registerCommandTools(server);
  registerAnalysisTools(server);
  registerTracingTools(server);
  registerDumpingTools(server);
  registerAntiDebugTools(server);
  registerExceptionTools(server);
  registerProcessTools(server);
  registerHandleTools(server);
  registerControlFlowTools(server);
  registerPatchTools(server);
}
