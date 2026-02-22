import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerExceptionTools(server: McpServer) {
  server.tool(
    'manage_exception_breakpoint',
    'Set, delete, or list exception breakpoints',
    {
      action: z.enum(['set', 'delete', 'list']).describe('Action to perform'),
      code: z.string().optional().describe('Exception code (hex, e.g. "C0000005") (required for set/delete)'),
      chance: z.enum(['first', 'second', 'all']).optional().default('first').describe('Exception chance (used with set)'),
      bp_action: z.enum(['break', 'log', 'command']).optional().default('break').describe('Action to take (used with set)')
    },
    async ({ action, code, chance, bp_action }) => {
      let data: any;
      if (action === 'set') {
        if (!code) throw new Error("code is required for set");
        data = await httpClient.post('/api/exceptions/set_bp', { code, chance, action: bp_action });
      } else if (action === 'delete') {
        if (!code) throw new Error("code is required for delete");
        data = await httpClient.post('/api/exceptions/delete_bp', { code });
      } else {
        data = await httpClient.get('/api/exceptions/list_bps');
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_exception_codes',
    'List all known Windows exception codes with their names (ACCESS_VIOLATION, BREAKPOINT, SINGLE_STEP, etc.)',
    {},
    async () => {
      const data = await httpClient.get('/api/exceptions/list_codes');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'skip_exception',
    'Skip/pass the current exception and continue execution. Useful when a VM protector triggers intentional exceptions.',
    {},
    async () => {
      const data = await httpClient.post('/api/exceptions/skip');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
