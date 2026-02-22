import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerRegisterTools(server: McpServer) {
  server.tool(
    'x64dbg_registers',
    'Get or set CPU register values (all, specific, flags, avx512, set)',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("get_all") }),
        z.object({ action: z.literal("get_flags") }),
        z.object({ action: z.literal("get_avx512") }),
        z.object({
          action: z.literal("get_specific"),
          name: z.string().describe("Register name (e.g. 'rax', 'eip', 'dr0')")
        }),
        z.object({
          action: z.literal("set"),
          name: z.string().describe("Register name (e.g. 'rax', 'rcx')"),
          value: z.string().describe("New value (hex string, e.g. '0x1234' or expression)")
        })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'get_all': data = await httpClient.get('/api/registers/all'); break;
        case 'get_flags': data = await httpClient.get('/api/registers/flags'); break;
        case 'get_avx512': data = await httpClient.get('/api/registers/avx512'); break;
        case 'get_specific':
          data = await httpClient.get('/api/registers/get', { name: action.name });
          break;
        case 'set':
          data = await httpClient.post('/api/registers/set', { name: action.name, value: action.value });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
