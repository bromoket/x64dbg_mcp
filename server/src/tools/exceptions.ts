import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerExceptionTools(server: McpServer) {
  server.tool(
    'set_exception_breakpoint',
    'Set an exception breakpoint on a specific exception code. VM protectors trigger exceptions for control flow obfuscation.',
    {
      code: z.string().describe('Exception code (hex, e.g. "C0000005" for ACCESS_VIOLATION, "80000003" for BREAKPOINT)'),
      chance: z.enum(['first', 'second', 'all']).optional().default('first').describe('Exception chance: first, second, or all'),
      action: z.enum(['break', 'log', 'command']).optional().default('break').describe('Action to take'),
    },
    async ({ code, chance, action }) => {
      const data = await httpClient.post('/api/exceptions/set_bp', { code, chance, action });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'delete_exception_breakpoint',
    'Delete an exception breakpoint',
    {
      code: z.string().describe('Exception code to remove the breakpoint for'),
    },
    async ({ code }) => {
      const data = await httpClient.post('/api/exceptions/delete_bp', { code });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_exception_breakpoints',
    'List all active exception breakpoints',
    {},
    async () => {
      const data = await httpClient.get('/api/exceptions/list_bps');
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
