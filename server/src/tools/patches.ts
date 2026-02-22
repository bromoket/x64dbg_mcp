import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerPatchTools(server: McpServer) {
  server.tool(
    'x64dbg_patches',
    'List, apply, restore patches or export patched module',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("list") }),
        z.object({ action: z.literal("apply"), address: z.string(), bytes: z.string() }),
        z.object({ action: z.literal("restore"), address: z.string() }),
        z.object({ action: z.literal("export"), module: z.string().optional(), path: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'list': data = await httpClient.get('/api/patches/list'); break;
        case 'apply': data = await httpClient.post('/api/patches/apply', { address: action.address, bytes: action.bytes }); break;
        case 'restore': data = await httpClient.post('/api/patches/restore', { address: action.address }); break;
        case 'export':
          const body: Record<string, string> = { path: action.path };
          if (action.module) body.module = action.module;
          data = await httpClient.post('/api/patches/export', body);
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
