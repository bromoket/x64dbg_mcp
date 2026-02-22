import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSymbolTools(server: McpServer) {
  server.tool(
    'x64dbg_symbols',
    'Symbol, label, comment, and bookmark operations',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("resolve"), name: z.string() }),
        z.object({ action: z.literal("address"), address: z.string() }),
        z.object({ action: z.literal("search"), pattern: z.string(), module: z.string().optional() }),
        z.object({ action: z.literal("list_module"), module: z.string() }),
        z.object({ action: z.literal("get_label"), address: z.string() }),
        z.object({ action: z.literal("set_label"), address: z.string(), text: z.string() }),
        z.object({ action: z.literal("get_comment"), address: z.string() }),
        z.object({ action: z.literal("set_comment"), address: z.string(), text: z.string() }),
        z.object({ action: z.literal("bookmark"), address: z.string(), set: z.boolean().optional().default(true) })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'resolve':
          data = await httpClient.get('/api/symbols/resolve', { name: action.name });
          break;
        case 'address':
          data = await httpClient.get('/api/symbols/at', { address: action.address });
          break;
        case 'search':
          let searchParams: any = { pattern: action.pattern };
          if (action.module) searchParams.module = action.module;
          data = await httpClient.get('/api/symbols/search', searchParams);
          break;
        case 'list_module':
          data = await httpClient.get('/api/symbols/list', { module: action.module });
          break;
        case 'get_label':
          data = await httpClient.get('/api/labels/get', { address: action.address });
          break;
        case 'set_label':
          data = await httpClient.post('/api/labels/set', { address: action.address, text: action.text });
          break;
        case 'get_comment':
          data = await httpClient.get('/api/comments/get', { address: action.address });
          break;
        case 'set_comment':
          data = await httpClient.post('/api/comments/set', { address: action.address, text: action.text });
          break;
        case 'bookmark':
          data = await httpClient.post('/api/bookmarks/set', { address: action.address, set: action.set });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
