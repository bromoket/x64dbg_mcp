import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerStackTools(server: McpServer) {
  server.tool(
    'get_call_stack',
    'Get the call stack for the current or a specific thread',
    {
      handle: z.string().optional().describe('Thread handle (hex). If omitted, gets current thread stack.'),
      max_depth: z.string().optional().default('50').describe('Maximum stack frames to retrieve (default 50)')
    },
    async ({ handle, max_depth }) => {
      let data: any;
      if (handle) {
         data = await httpClient.get('/api/stack/callstack_thread', { handle });
      } else {
         data = await httpClient.get('/api/stack/trace', { max_depth });
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'read_stack',
    'Read raw stack memory as pointer-sized entries with symbol resolution',
    {
      address: z.string().optional().default('csp').describe('Stack address (default: csp)'),
      size: z.string().optional().default('256').describe('Number of bytes to read (default 256)'),
    },
    async ({ address, size }) => {
      const data = await httpClient.get('/api/stack/read', { address, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_stack_info',
    'Query information about the stack (pointers, SEH chain, comments, or top return address)',
    {
      action: z.enum(['pointers', 'seh_chain', 'comment', 'return_address']).describe('Type of info to get'),
      address: z.string().optional().describe('Stack address (required only for "comment" action)')
    },
    async ({ action, address }) => {
      let endpoint = '';
      let params: any = {};

      switch (action) {
        case 'pointers': endpoint = '/api/stack/pointers'; break;
        case 'seh_chain': endpoint = '/api/stack/seh_chain'; break;
        case 'return_address': endpoint = '/api/stack/return_address'; break;
        case 'comment':
          endpoint = '/api/stack/comment';
          if (!address) throw new Error("address is required for comment action");
          params.address = address;
          break;
      }
      const data = await httpClient.get(endpoint, params);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
