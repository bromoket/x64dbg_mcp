import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerStackTools(server: McpServer) {
  server.tool(
    'get_call_stack',
    'Get the call stack (stack trace) of the current thread with return addresses, labels, and modules',
    { max_depth: z.string().optional().default('50').describe('Maximum stack frames to retrieve (default 50)') },
    async ({ max_depth }) => {
      const data = await httpClient.get('/api/stack/trace', { max_depth });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'read_stack',
    'Read raw stack memory as pointer-sized entries with symbol resolution',
    {
      address: z.string().optional().default('csp').describe('Stack address to read from (default: current stack pointer)'),
      size: z.string().optional().default('256').describe('Number of bytes to read (default 256)'),
    },
    async ({ address, size }) => {
      const data = await httpClient.get('/api/stack/read', { address, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_stack_pointers',
    'Get the current stack pointer (RSP/ESP) and frame pointer (RBP/EBP)',
    {},
    async () => {
      const data = await httpClient.get('/api/stack/pointers');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_seh_chain',
    'Get the SEH (Structured Exception Handler) chain',
    {},
    async () => {
      const data = await httpClient.get('/api/stack/seh_chain');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
