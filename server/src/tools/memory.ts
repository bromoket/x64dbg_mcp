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
    'is_valid_address',
    'Check if a memory address is a valid readable pointer',
    { address: z.string().describe('Address to check (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/memory/is_valid', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_memory_info',
    'Get memory page protection info for an address',
    { address: z.string().describe('Address to query (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/memory/page_info', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'allocate_memory',
    'Allocate memory in the debugged process via VirtualAllocEx',
    { size: z.string().optional().default('0x1000').describe('Size to allocate (hex, default 0x1000)') },
    async ({ size }) => {
      const data = await httpClient.post('/api/memory/allocate', { size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'free_memory',
    'Free previously allocated memory in the debugged process',
    { address: z.string().describe('Address of allocated memory to free') },
    async ({ address }) => {
      const data = await httpClient.post('/api/memory/free', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_memory_protection',
    'Change memory page protection (VirtualProtectEx)',
    {
      address: z.string().describe('Start address of the memory region'),
      size: z.string().optional().default('0x1000').describe('Size of the region (hex)'),
      protection: z.string().describe('New protection (e.g. "PAGE_EXECUTE_READWRITE", "40" for ERW)'),
    },
    async ({ address, size, protection }) => {
      const data = await httpClient.post('/api/memory/protect', { address, size, protection });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'is_code_page',
    'Check if an address belongs to an executable (code) memory page',
    {
      address: z.string().describe('Address to check'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/memory/is_code', { address });
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
