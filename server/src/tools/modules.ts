import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerModuleTools(server: McpServer) {
  server.tool(
    'x64dbg_modules',
    'List modules or get info (base, section, party) for a specific module',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("list") }),
        z.object({ action: z.literal("get_info"), module: z.string() }),
        z.object({ action: z.literal("get_base"), module: z.string() }),
        z.object({ action: z.literal("get_section"), address: z.string() }),
        z.object({ action: z.literal("get_party"), base: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'list':
          data = await httpClient.get('/api/modules/list');
          break;
        case 'get_info':
          data = await httpClient.get('/api/modules/get', { name: action.module });
          break;
        case 'get_base':
          data = await httpClient.get('/api/modules/base', { name: action.module });
          break;
        case 'get_section':
          data = await httpClient.get('/api/modules/section', { address: action.address });
          break;
        case 'get_party':
          data = await httpClient.get('/api/modules/party', { base: action.base });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
