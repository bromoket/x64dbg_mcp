import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerHandleTools(server: McpServer) {
  server.tool(
    'x64dbg_handles',
    'List handles/tcp/windows/heaps, get handle name, or close a handle',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("list_handles") }),
        z.object({ action: z.literal("list_tcp") }),
        z.object({ action: z.literal("list_windows") }),
        z.object({ action: z.literal("list_heaps") }),
        z.object({ action: z.literal("get_name"), handle: z.string() }),
        z.object({ action: z.literal("close"), handle: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'list_handles': data = await httpClient.get('/api/handles/list'); break;
        case 'list_tcp': data = await httpClient.get('/api/handles/tcp'); break;
        case 'list_windows': data = await httpClient.get('/api/handles/windows'); break;
        case 'list_heaps': data = await httpClient.get('/api/handles/heaps'); break;
        case 'get_name': data = await httpClient.get('/api/handles/get', { handle: action.handle }); break;
        case 'close': data = await httpClient.post('/api/handles/close', { handle: action.handle }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
