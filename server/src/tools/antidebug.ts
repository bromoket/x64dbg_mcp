import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerAntiDebugTools(server: McpServer) {
  server.tool(
    'get_peb_info',
    'Read PEB (Process Environment Block) fields including BeingDebugged, NtGlobalFlag, and ProcessHeap. Essential for understanding anti-debug checks in VMProtect/Themida.',
    {
      pid: z.string().optional().default('').describe('Process ID (default: current debuggee PID)'),
    },
    async ({ pid }) => {
      const data = await httpClient.get('/api/antidebug/peb', { pid });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_teb_info',
    'Read TEB (Thread Environment Block) fields including SEH chain, stack base/limit, and PEB pointer',
    {
      tid: z.string().optional().default('').describe('Thread ID (default: current thread)'),
    },
    async ({ tid }) => {
      const data = await httpClient.get('/api/antidebug/teb', { tid });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'hide_debugger',
    'Hide the debugger by zeroing PEB.BeingDebugged and PEB.NtGlobalFlag. Bypasses common anti-debug checks used by VMProtect, Themida, and other protectors.',
    {},
    async () => {
      const data = await httpClient.post('/api/antidebug/hide_debugger');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_dep_status',
    'Check if Data Execution Prevention (DEP) is enabled for the debugged process',
    {},
    async () => {
      const data = await httpClient.get('/api/antidebug/dep_status');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
