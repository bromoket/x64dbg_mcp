import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerMemoryTools(server: McpServer) {
  server.tool(
    'read_memory',
    'Read bytes from a memory address. Returns hex dump and ASCII representation.',
    {
      address: z.string().describe('Memory address to read from (hex string or expression, e.g. "0x401000", "rax", "rsp+8")'),
      size: z.string().optional().default('256').describe('Number of bytes to read (decimal, default 256)'),
    },
    async ({ address, size }) => {
      const data = await httpClient.get('/api/memory/read', { address, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'write_memory',
    'Write bytes to a memory address in the debugged process. ' +
    'Set verify=true to read back after writing and confirm success - ' +
    'useful when patching write-protected or copy-on-write pages where writes silently fail.',
    {
      address: z.string().describe('Target memory address (hex string or expression)'),
      bytes: z.string().describe('Hex bytes to write (e.g. "90 90 90" or "CC" or "0F FF")'),
      verify: z.boolean().optional().default(false).describe(
        'If true, reads back memory after write and verifies it matches. ' +
        'Detects silent failures on write-protected pages.'
      ),
    },
    async ({ address, bytes, verify }) => {
      const data = await httpClient.post('/api/memory/write', { address, bytes, verify });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_memory_info',
    'Get information about a memory address (page info, validity, or if it is code)',
    {
      action: z.enum(['info', 'is_valid', 'is_code']).describe('Type of information to query'),
      address: z.string().describe('Address to check (hex string or expression)')
    },
    async ({ action, address }) => {
      let endpoint = '';
      switch (action) {
        case 'info': endpoint = '/api/memory/page_info'; break;
        case 'is_valid': endpoint = '/api/memory/is_valid'; break;
        case 'is_code': endpoint = '/api/memory/is_code'; break;
      }
      const data = await httpClient.get(endpoint, { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_memory',
    'Allocate, free, or change protection of memory in the debugged process',
    {
      action: z.enum(['allocate', 'free', 'protect']).describe('Action to perform'),
      address: z.string().optional().describe('Address (required for free and protect)'),
      size: z.string().optional().default('0x1000').describe('Size in hex (for allocate and protect)'),
      protection: z.string().optional().describe('New protection string (e.g. "PAGE_EXECUTE_READWRITE", required for protect)')
    },
    async ({ action, address, size, protection }) => {
      let endpoint = '';
      let payload: any = {};

      switch (action) {
        case 'allocate':
          endpoint = '/api/memory/allocate';
          payload.size = size;
          break;
        case 'free':
          endpoint = '/api/memory/free';
          if (!address) throw new Error("address is required for free");
          payload.address = address;
          break;
        case 'protect':
          endpoint = '/api/memory/protect';
          if (!address) throw new Error("address is required for protect");
          if (!protection) throw new Error("protection is required for protect");
          payload.address = address;
          payload.size = size;
          payload.protection = protection;
          break;
      }
      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'update_memory_map',
    'Force refresh the memory map. Call after memory allocations, protections changes, or module loads/unloads.',
    {},
    async () => {
      const data = await httpClient.post('/api/memory/update_map');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
