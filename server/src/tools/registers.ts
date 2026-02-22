import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerRegisterTools(server: McpServer) {
  server.tool(
    'get_registers',
    'Get CPU register values (all, specific, flags, avx512)',
    {
      type: z.enum(['all', 'specific', 'flags', 'avx512']).describe('Type of register info to get'),
      name: z.string().optional().describe('Register name (e.g. "rax", "eip", "dr0") required for "specific"')
    },
    async ({ type, name }) => {
      let data: any;
      switch (type) {
        case 'all': data = await httpClient.get('/api/registers/all'); break;
        case 'flags': data = await httpClient.get('/api/registers/flags'); break;
        case 'avx512': data = await httpClient.get('/api/registers/avx512'); break;
        case 'specific':
          if (!name) throw new Error("name is required for specific register");
          data = await httpClient.get('/api/registers/get', { name });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_register',
    'Set a CPU register to a new value',
    {
      name: z.string().describe('Register name (e.g. "rax", "rcx")'),
      value: z.string().describe('New value (hex string, e.g. "0x1234" or expression)'),
    },
    async ({ name, value }) => {
      const data = await httpClient.post('/api/registers/set', { name, value });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
