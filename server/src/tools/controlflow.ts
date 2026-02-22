import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerControlFlowTools(server: McpServer) {
  server.tool(
    'x64dbg_control_flow',
    'Get CFG/branch/loop info or manage function definitions',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("cfg"), address: z.string().optional().default("cip") }),
        z.object({ action: z.literal("branch_dest"), address: z.string().optional().default("cip") }),
        z.object({ action: z.literal("is_jump_taken"), address: z.string().optional().default("cip") }),
        z.object({ action: z.literal("loops"), address: z.string().optional().default("cip") }),
        z.object({ action: z.literal("func_type"), address: z.string().optional().default("cip") }),
        z.object({ action: z.literal("add_function"), start: z.string(), end: z.string() }),
        z.object({ action: z.literal("delete_function"), address: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'cfg': data = await httpClient.get('/api/cfg/function', { address: action.address }); break;
        case 'branch_dest': data = await httpClient.get('/api/cfg/branch_dest', { address: action.address }); break;
        case 'is_jump_taken': data = await httpClient.get('/api/cfg/is_jump_taken', { address: action.address }); break;
        case 'loops': data = await httpClient.get('/api/cfg/loops', { address: action.address }); break;
        case 'func_type': data = await httpClient.get('/api/cfg/func_type', { address: action.address }); break;
        case 'add_function': data = await httpClient.post('/api/cfg/add_function', { start: action.start, end: action.end }); break;
        case 'delete_function': data = await httpClient.post('/api/cfg/delete_function', { address: action.address }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
