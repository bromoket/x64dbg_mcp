import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDisassemblyTools(server: McpServer) {
  server.tool(
    'disassemble',
    'Disassemble instructions at an address or within a function',
    {
      action: z.enum(['at_address', 'function', 'info']).describe('Type of disassembly to perform'),
      address: z.string().optional().default('cip').describe('Start address (hex string or expression, default: cip)'),
      count: z.string().optional().default('10').describe('Number of instructions (for at_address)'),
      max_instructions: z.number().optional().default(50).describe('Max instructions if no func boundary (for function)'),
    },
    async ({ action, address, count, max_instructions }) => {
      let data: any;
      switch (action) {
        case 'at_address':
          data = await httpClient.get('/api/disasm/at', { address, count });
          break;
        case 'function':
          data = await httpClient.get('/api/disasm/function', {
            address,
            max_instructions: String(max_instructions),
          });
          break;
        case 'info':
          data = await httpClient.get('/api/disasm/basic', { address });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'assemble',
    'Assemble an instruction at a specific address (modifies memory)',
    {
      address: z.string().describe('Address where the instruction will be assembled'),
      instruction: z.string().describe('Assembly instruction text (e.g. "nop", "mov eax, 1", "jmp 0x401000")'),
    },
    async ({ address, instruction }) => {
      const data = await httpClient.post('/api/disasm/assemble', { address, instruction });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
