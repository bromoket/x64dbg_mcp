import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerHandleTools(server: McpServer) {
  server.tool(
    'get_handle_info',
    'List all open handles, TCP connections, windows, or heaps in the debugged process',
    {
      action: z.enum(['handles', 'tcp', 'windows', 'heaps']).describe('Type of handles/objects to list')
    },
    async ({ action }) => {
      let data: any;
      switch (action) {
        case 'handles': data = await httpClient.get('/api/handles/list'); break;
        case 'tcp': data = await httpClient.get('/api/handles/tcp'); break;
        case 'windows': data = await httpClient.get('/api/handles/windows'); break;
        case 'heaps': data = await httpClient.get('/api/handles/heaps'); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_handle_name',
    'Get the name and type of a specific handle',
    {
      handle: z.string().describe('Handle value (hex)'),
    },
    async ({ handle }) => {
      const data = await httpClient.get('/api/handles/get', { handle });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'close_handle',
    'Force-close a handle in the debugged process. Use with caution as this may cause instability.',
    {
      handle: z.string().describe('Handle value to close (hex)'),
    },
    async ({ handle }) => {
      const data = await httpClient.post('/api/handles/close', { handle });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
