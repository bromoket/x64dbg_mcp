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

export function registerAllTools(server: McpServer) {
  registerDebugTools(server);       // 9 tools
  registerRegisterTools(server);    // 4 tools
  registerMemoryTools(server);      // 7 tools
  registerDisassemblyTools(server); // 4 tools
  registerBreakpointTools(server);  // 11 tools
  registerSymbolTools(server);      // 9 tools
  registerStackTools(server);       // 4 tools
  registerThreadTools(server);      // 7 tools
  registerModuleTools(server);      // 3 tools
  registerMemmapTools(server);      // 2 tools
  registerSearchTools(server);      // 2 tools
  registerCommandTools(server);     // 3 tools
  // Total: 65 tools
}
