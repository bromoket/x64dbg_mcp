import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerAntiDebugTools(server: McpServer) {
  server.tool(
    'x64dbg_antidebug',
    'Anti-debugging analysis: Read PEB/TEB/DEP, or hide debugger',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("peb"), pid: z.string().optional() }),
        z.object({ action: z.literal("teb"), tid: z.string().optional() }),
        z.object({ action: z.literal("dep") }),
        z.object({ action: z.literal("hide_debugger") })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'peb': data = await httpClient.get('/api/antidebug/peb', { pid: action.pid || '' }); break;
        case 'teb': data = await httpClient.get('/api/antidebug/teb', { tid: action.tid || '' }); break;
        case 'dep': data = await httpClient.get('/api/antidebug/dep_status'); break;
        case 'hide_debugger': data = await httpClient.post('/api/antidebug/hide_debugger'); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
