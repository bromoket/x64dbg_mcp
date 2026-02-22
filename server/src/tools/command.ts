import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerCommandTools(server: McpServer) {
  server.tool(
    'x64dbg_command',
    'Execute x64dbg commands, scripts, eval expressions, or manage debug session',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("execute"), command: z.string().describe("x64dbg command string") }),
        z.object({ action: z.literal("script"), commands: z.array(z.string()).describe("Array of commands") }),
        z.object({ action: z.literal("evaluate"), expression: z.string() }),
        z.object({ action: z.literal("format"), format: z.string() }),
        z.object({ action: z.literal("set_init_script"), file: z.string().describe("Path to script file") }),
        z.object({ action: z.literal("get_init_script") }),
        z.object({ action: z.literal("get_hash") }),
        z.object({ action: z.literal("get_events") })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'execute':
          data = await httpClient.post('/api/command/exec', { command: action.command });
          break;
        case 'script':
          data = await httpClient.post('/api/command/script', { commands: action.commands });
          break;
        case 'evaluate':
          data = await httpClient.post('/api/command/eval', { expression: action.expression });
          break;
        case 'format':
          data = await httpClient.post('/api/command/format', { format: action.format });
          break;
        case 'set_init_script':
          data = await httpClient.post('/api/command/init_script', { file: action.file });
          break;
        case 'get_init_script':
          data = await httpClient.get('/api/command/init_script');
          break;
        case 'get_hash':
          data = await httpClient.get('/api/command/hash');
          break;
        case 'get_events':
          data = await httpClient.get('/api/command/events');
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
