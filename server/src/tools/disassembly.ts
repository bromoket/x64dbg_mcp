import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDisassemblyTools(server: McpServer) {
  server.tool(
    'disassemble',
    'Disassemble instructions at a given address. Shows address, instruction text, size, labels, and comments.',
    {
      address: z.string().optional().default('cip').describe('Start address (hex string or expression, default: current instruction pointer)'),
      count: z.string().optional().default('10').describe('Number of instructions to disassemble (default 10, max 1000)'),
    },
    async ({ address, count }) => {
      const data = await httpClient.get('/api/disasm/at', { address, count });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'disassemble_function',
    'Disassemble an entire function containing the given address. ' +
    'When no function boundary is found (common with VMP/Themida/packed modules), ' +
    'falls back to showing max_instructions instructions from the address. ' +
    'Run analyze first if you get the fallback note.',
    {
      address: z.string().optional().default('cip').describe('Any address within the function (default: CIP)'),
      max_instructions: z.number().optional().default(50).describe(
        'Max instructions to show when no function boundary is found (default 50, max 5000). ' +
        'Increase this for large VMP/obfuscated functions.'
      ),
    },
    async ({ address, max_instructions }) => {
      const data = await httpClient.get('/api/disasm/function', {
        address,
        max_instructions: String(max_instructions),
      });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_instruction_info',
    'Get quick info about a single instruction (size, is_call, is_branch)',
    {
      address: z.string().optional().default('cip').describe('Instruction address (default: CIP)'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/disasm/basic', { address });
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
