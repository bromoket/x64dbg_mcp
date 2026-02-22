import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerThreadTools(server: McpServer) {
  server.tool(
    'list_threads',
    'List all threads in the debugged process with IDs, names, and start addresses',
    {},
    async () => {
      const data = await httpClient.get('/api/threads/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_thread_info',
    'Get information about threads (current, specific by ID, total count, TEB address, or thread name)',
    {
      action: z.enum(['current', 'specific', 'count', 'teb', 'name']).describe('The kind of info to fetch'),
      tid: z.string().optional().describe('Thread ID (decimal), required for specific/teb/name')
    },
    async ({ action, tid }) => {
      let data: any;
      switch (action) {
        case 'current':
          data = await httpClient.get('/api/threads/current');
          break;
        case 'count':
          data = await httpClient.get('/api/threads/count');
          break;
        case 'specific':
        case 'teb':
        case 'name':
          if (!tid) throw new Error("tid is required");
          const ep = action === 'specific' ? 'get' : action;
          data = await httpClient.get(`/api/threads/${ep}`, action === 'specific' ? { id: tid } : { tid });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_thread',
    'Perform actions on a thread (switch focus, suspend, or resume)',
    {
      action: z.enum(['switch', 'suspend', 'resume']).describe('Action to perform'),
      id: z.number().describe('Thread ID (decimal)')
    },
    async ({ action, id }) => {
      const data = await httpClient.post(`/api/threads/${action}`, { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
