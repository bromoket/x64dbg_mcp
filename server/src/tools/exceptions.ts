import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerExceptionTools(server: McpServer) {
  server.tool(
    'x64dbg_exceptions',
    'Manage exception breakpoints, list Windows exception codes, or skip exceptions',
    {
      action: z.discriminatedUnion("action", [
        z.object({
          action: z.literal("set"),
          code: z.string().describe("Exception code (hex, e.g. C0000005)"),
          chance: z.enum(['first', 'second', 'all']).optional().default("first"),
          bp_action: z.enum(['break', 'log', 'command']).optional().default("break")
        }),
        z.object({ action: z.literal("delete"), code: z.string() }),
        z.object({ action: z.literal("list") }),
        z.object({ action: z.literal("list_codes") }),
        z.object({ action: z.literal("skip") })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'set': data = await httpClient.post('/api/exceptions/set_bp', { code: action.code, chance: action.chance, action: action.bp_action }); break;
        case 'delete': data = await httpClient.post('/api/exceptions/delete_bp', { code: action.code }); break;
        case 'list': data = await httpClient.get('/api/exceptions/list_bps'); break;
        case 'list_codes': data = await httpClient.get('/api/exceptions/list_codes'); break;
        case 'skip': data = await httpClient.post('/api/exceptions/skip'); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
