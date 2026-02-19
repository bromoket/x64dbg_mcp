import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerRegisterTools(server: McpServer) {
  server.tool(
    'get_all_registers',
    'Get all CPU register values (general purpose, segment, debug, flags)',
    {},
    async () => {
      const data = await httpClient.get('/api/registers/all');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_register',
    'Get the value of a single CPU register',
    { name: z.string().describe('Register name (e.g. "rax", "eip", "dr0")') },
    async ({ name }) => {
      const data = await httpClient.get('/api/registers/get', { name });
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

  server.tool(
    'get_flags',
    'Get decoded EFLAGS register (CF, ZF, SF, OF, etc.)',
    {},
    async () => {
      const data = await httpClient.get('/api/registers/flags');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_avx512_registers',
    'Get AVX-512 extended register dump (requires CPU and OS support)',
    {},
    async () => {
      const data = await httpClient.get('/api/registers/avx512');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
