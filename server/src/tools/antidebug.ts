import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerAntiDebugTools(server: McpServer) {
  server.tool(
    'get_antidebug_info',
    'Read PEB/TEB fields or check DEP status for anti-debugging analysis',
    {
      action: z.enum(['peb', 'teb', 'dep']).describe('Information to gather'),
      id: z.string().optional().describe('Process ID (for peb) or Thread ID (for teb)')
    },
    async ({ action, id }) => {
      let data: any;
      switch (action) {
        case 'peb': data = await httpClient.get('/api/antidebug/peb', { pid: id || '' }); break;
        case 'teb': data = await httpClient.get('/api/antidebug/teb', { tid: id || '' }); break;
        case 'dep': data = await httpClient.get('/api/antidebug/dep_status'); break;
      }
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
}
