import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerControlFlowTools(server: McpServer) {
  server.tool(
    'get_cfg',
    'Get the control flow graph (CFG) for a function. Returns nodes with branch targets, instructions, and exit edges. Critical for understanding VM dispatcher structure.',
    {
      address: z.string().optional().default('cip').describe('Address within the function (default: current instruction pointer)'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/cfg/function', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_branch_destination',
    'Get the destination address of a branch/jump/call instruction. Resolves indirect jumps when possible.',
    {
      address: z.string().optional().default('cip').describe('Address of the branch instruction'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/cfg/branch_dest', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'is_jump_going_to_execute',
    'Check if a conditional jump at the given address will be taken based on current CPU flags',
    {
      address: z.string().optional().default('cip').describe('Address of the conditional jump'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/cfg/is_jump_taken', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_loops',
    'Get loop information at an address. Returns nested loops with start/end addresses and nesting depth.',
    {
      address: z.string().optional().default('cip').describe('Address to check for loops'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/cfg/loops', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'add_function',
    'Define a function boundary in the x64dbg database (start and end addresses)',
    {
      start: z.string().describe('Function start address'),
      end: z.string().describe('Function end address'),
    },
    async ({ start, end }) => {
      const data = await httpClient.post('/api/cfg/add_function', { start, end });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'delete_function',
    'Delete a function definition from the x64dbg database',
    {
      address: z.string().describe('Address within the function to delete'),
    },
    async ({ address }) => {
      const data = await httpClient.post('/api/cfg/delete_function', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_function_type',
    'Get the function type at an address (none, begin, middle, end, single). Indicates where the address falls within a defined function.',
    {
      address: z.string().optional().default('cip').describe('Address to query'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/cfg/func_type', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
