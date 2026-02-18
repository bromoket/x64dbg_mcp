import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSymbolTools(server: McpServer) {
  server.tool(
    'resolve_symbol',
    'Resolve a symbol or function name to its address',
    { name: z.string().describe('Symbol name to resolve (e.g. "kernel32.CreateFileW", "main", "ntdll.RtlInitUnicodeString")') },
    async ({ name }) => {
      const data = await httpClient.get('/api/symbols/resolve', { name });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_symbol_at',
    'Get the symbol/label name at a given address',
    { address: z.string().describe('Address to look up (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/symbols/at', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'search_symbols',
    'Search for symbols matching a pattern',
    {
      pattern: z.string().describe('Search pattern (e.g. "Create*", "*File*")'),
      module: z.string().optional().default('').describe('Optional module name to restrict search'),
    },
    async ({ pattern, module }) => {
      const data = await httpClient.get('/api/symbols/search', { pattern, module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_module_symbols',
    'Load and list symbols for a specific module',
    { module: z.string().describe('Module name (e.g. "kernel32", "ntdll", "user32")') },
    async ({ module }) => {
      const data = await httpClient.get('/api/symbols/list', { module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_label',
    'Get the user-defined label at an address',
    { address: z.string().describe('Address to get label for (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/labels/get', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_label',
    'Set a user-defined label at an address (visible in disassembly)',
    {
      address: z.string().describe('Address to label'),
      text: z.string().describe('Label text (e.g. "decrypt_function", "main_loop")'),
    },
    async ({ address, text }) => {
      const data = await httpClient.post('/api/labels/set', { address, text });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_comment',
    'Get the comment at an address',
    { address: z.string().describe('Address to get comment for') },
    async ({ address }) => {
      const data = await httpClient.get('/api/comments/get', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_comment',
    'Set a comment at an address (visible in disassembly)',
    {
      address: z.string().describe('Address to comment'),
      text: z.string().describe('Comment text'),
    },
    async ({ address, text }) => {
      const data = await httpClient.post('/api/comments/set', { address, text });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_bookmark',
    'Set or clear a bookmark at an address',
    {
      address: z.string().describe('Address to bookmark'),
      set: z.boolean().optional().default(true).describe('true to set, false to clear (default: true)'),
    },
    async ({ address, set }) => {
      const data = await httpClient.post('/api/bookmarks/set', { address, set });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
