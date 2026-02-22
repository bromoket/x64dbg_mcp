import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerControlFlowTools(server: McpServer) {
  server.tool(
    'get_control_flow_info',
    'Get CFG, branch destinations, jump states, loops, or function block type',
    {
      action: z.enum(['cfg', 'branch_dest', 'is_jump_taken', 'loops', 'func_type']).describe('Information to get'),
      address: z.string().optional().default('cip').describe('Address to inspect')
    },
    async ({ action, address }) => {
      let data: any;
      switch (action) {
        case 'cfg': data = await httpClient.get('/api/cfg/function', { address }); break;
        case 'branch_dest': data = await httpClient.get('/api/cfg/branch_dest', { address }); break;
        case 'is_jump_taken': data = await httpClient.get('/api/cfg/is_jump_taken', { address }); break;
        case 'loops': data = await httpClient.get('/api/cfg/loops', { address }); break;
        case 'func_type': data = await httpClient.get('/api/cfg/func_type', { address }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_function_definitions',
    'Add or delete function boundaries in the x64dbg database',
    {
      action: z.enum(['add', 'delete']).describe('Action to perform'),
      start: z.string().optional().describe('Start address (required for add)'),
      end: z.string().optional().describe('End address (required for add)'),
      address: z.string().optional().describe('Address within function to delete (required for delete)')
    },
    async ({ action, start, end, address }) => {
      let data: any;
      if (action === 'add') {
        if (!start || !end) throw new Error("start and end are required for add");
        data = await httpClient.post('/api/cfg/add_function', { start, end });
      } else {
        if (!address) throw new Error("address is required for delete");
        data = await httpClient.post('/api/cfg/delete_function', { address });
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
