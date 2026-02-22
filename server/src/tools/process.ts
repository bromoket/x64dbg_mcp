import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerProcessTools(server: McpServer) {
  server.tool(
    'x64dbg_process',
    'Get process info (basic, detailed, cmdline, elevated, dbversion) or set cmdline',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("basic") }),
        z.object({ action: z.literal("detailed") }),
        z.object({ action: z.literal("cmdline") }),
        z.object({ action: z.literal("elevated") }),
        z.object({ action: z.literal("dbversion") }),
        z.object({ action: z.literal("set_cmdline"), cmdline: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'basic': data = await httpClient.get('/api/process/info'); break;
        case 'detailed': data = await httpClient.get('/api/process/details'); break;
        case 'cmdline': data = await httpClient.get('/api/process/cmdline'); break;
        case 'elevated': data = await httpClient.get('/api/process/elevated'); break;
        case 'dbversion': data = await httpClient.get('/api/process/dbversion'); break;
        case 'set_cmdline': data = await httpClient.post('/api/process/set_cmdline', { cmdline: action.cmdline }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
