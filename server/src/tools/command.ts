import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerCommandTools(server: McpServer) {
  server.tool(
    'execute_command',
    'Execute a raw x64dbg command. Also supports evaluating expressions or formatting strings.',
    {
      action: z.enum(['execute', 'evaluate', 'format']).describe('Type of command action'),
      command: z.string().describe('Command, expression, or format string depending on action')
    },
    async ({ action, command }) => {
      let endpoint = '';
      let payload: any = {};

      switch (action) {
        case 'execute':
          endpoint = '/api/command/exec';
          payload.command = command;
          break;
        case 'evaluate':
          endpoint = '/api/command/eval';
          payload.expression = command;
          break;
        case 'format':
          endpoint = '/api/command/format';
          payload.format = command;
          break;
      }
      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'execute_script',
    'Execute a batch of x64dbg commands sequentially',
    { commands: z.array(z.string()).describe('Array of x64dbg command strings to execute in order') },
    async ({ commands }) => {
      const data = await httpClient.post('/api/command/script', { commands });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_debug_session',
    'Get/Set initialization scripts, check database hash, or get debug events count',
    {
      action: z.enum(['set_init_script', 'get_init_script', 'get_hash', 'get_events']).describe('Action to perform'),
      file: z.string().optional().describe('Path to the script file (required for set_init_script)')
    },
    async ({ action, file }) => {
      let data: any;
      switch (action) {
        case 'set_init_script':
          if (!file) throw new Error("file is required for set_init_script");
          data = await httpClient.post('/api/command/init_script', { file });
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
