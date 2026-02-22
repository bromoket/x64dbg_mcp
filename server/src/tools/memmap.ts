import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerMemmapTools(server: McpServer) {
  server.tool(
    'get_memory_map',
    'Get the full virtual memory map or a specific region for the debugged process',
    {
      address: z.string().optional().describe('Address to query a specific region (optional, returns full map if omitted)')
    },
    async ({ address }) => {
      let data: any;
      if (address) {
        data = await httpClient.get('/api/memmap/at', { address });
      } else {
        data = await httpClient.get('/api/memmap/list');
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
