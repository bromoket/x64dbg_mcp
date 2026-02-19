import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerHandleTools(server: McpServer) {
  server.tool(
    'list_handles',
    'List all open handles in the debugged process with their names and types (files, registry keys, mutexes, etc.)',
    {},
    async () => {
      const data = await httpClient.get('/api/handles/list');
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
    'list_tcp_connections',
    'List TCP network connections of the debugged process with remote/local addresses, ports, and connection states',
    {},
    async () => {
      const data = await httpClient.get('/api/handles/tcp');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_windows',
    'List all windows created by the debugged process with titles, class names, styles, and window procedures',
    {},
    async () => {
      const data = await httpClient.get('/api/handles/windows');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_heaps',
    'List all heaps in the debugged process with addresses, sizes, and flags',
    {},
    async () => {
      const data = await httpClient.get('/api/handles/heaps');
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
