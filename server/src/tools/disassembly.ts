import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDisassemblyTools(server: McpServer) {
  server.tool(
    'x64dbg_disassembly',
    'Disassemble or assemble instructions (at address, function, info, assemble)',
    {
      action: z.discriminatedUnion("action", [
        z.object({
          action: z.literal("at_address"),
          address: z.string().optional().default("cip"),
          count: z.string().optional().default("10")
        }),
        z.object({
          action: z.literal("function"),
          address: z.string().optional().default("cip"),
          max_instructions: z.number().optional().default(50)
        }),
        z.object({
          action: z.literal("info"),
          address: z.string().optional().default("cip")
        }),
        z.object({
          action: z.literal("assemble"),
          address: z.string().describe("Address to assemble at"),
          instruction: z.string().describe("Assembly instruction (e.g. 'nop')")
        })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'at_address':
          data = await httpClient.get('/api/disasm/at', { address: action.address, count: action.count });
          break;
        case 'function':
          data = await httpClient.get('/api/disasm/function', {
            address: action.address,
            max_instructions: String(action.max_instructions),
          });
          break;
        case 'info':
          data = await httpClient.get('/api/disasm/basic', { address: action.address });
          break;
        case 'assemble':
          data = await httpClient.post('/api/disasm/assemble', { address: (action as any).address, instruction: (action as any).instruction });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
