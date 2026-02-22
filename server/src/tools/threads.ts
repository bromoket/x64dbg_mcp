import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerThreadTools(server: McpServer) {
  server.tool(
    'x64dbg_threads',
    'Thread operations: list, get current/specific/teb/name, or switch/suspend/resume',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("list") }),
        z.object({ action: z.literal("current") }),
        z.object({ action: z.literal("count") }),
        z.object({ action: z.literal("info"), tid: z.string().describe("Thread ID (decimal)") }),
        z.object({ action: z.literal("teb"), tid: z.string() }),
        z.object({ action: z.literal("name"), tid: z.string() }),
        z.object({ action: z.literal("switch"), id: z.number().describe("Thread ID (decimal)") }),
        z.object({ action: z.literal("suspend"), id: z.number() }),
        z.object({ action: z.literal("resume"), id: z.number() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'list':
          data = await httpClient.get('/api/threads/list');
          break;
        case 'current':
          data = await httpClient.get('/api/threads/current');
          break;
        case 'count':
          data = await httpClient.get('/api/threads/count');
          break;
        case 'info':
          data = await httpClient.get('/api/threads/get', { id: action.tid });
          break;
        case 'teb':
        case 'name':
          data = await httpClient.get(`/api/threads/${action.action}`, { tid: action.tid });
          break;
        case 'switch':
        case 'suspend':
        case 'resume':
          data = await httpClient.post(`/api/threads/${action.action}`, { id: action.id });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
