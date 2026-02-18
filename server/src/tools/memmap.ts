import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerMemmapTools(server: McpServer) {
  server.tool(
    'get_memory_map',
    'Get the full virtual memory map of the debugged process with regions, protections, and types',
    {},
    async () => {
      const data = await httpClient.get('/api/memmap/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_memory_region',
    'Get the memory region info containing a specific address',
    { address: z.string().describe('Address to query (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/memmap/at', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
