import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSearchTools(server: McpServer) {
  server.tool(
    'search_pattern',
    'Search for a byte pattern (AOB scan) in the process memory. Supports wildcards.',
    {
      pattern: z.string().describe('Byte pattern in hex (e.g. "48 89 5C 24 ??", "CC CC CC", "E8 ?? ?? ?? ??"). Use ?? for wildcards.'),
      address: z.string().optional().default('0').describe('Start address for search (0 = search all memory)'),
      size: z.string().optional().default('0').describe('Region size to search (0 = auto)'),
    },
    async ({ pattern, address, size }) => {
      const data = await httpClient.post('/api/search/pattern', { pattern, address, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'search_strings',
    'Search for a text string in process memory',
    {
      text: z.string().describe('String to search for'),
      module: z.string().optional().default('').describe('Module name to restrict search (empty = all memory)'),
      encoding: z.enum(['utf8', 'ascii', 'unicode']).optional().default('utf8').describe('String encoding (default: utf8, unicode = UTF-16LE)'),
    },
    async ({ text, module, encoding }) => {
      const data = await httpClient.post('/api/search/string', { text, module, encoding });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_string_at',
    'Get the string value at a memory address (auto-detects ASCII/Unicode)',
    {
      address: z.string().describe('Address to read string from'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/search/string_at', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'symbol_auto_complete',
    'Auto-complete a symbol name. Returns matching symbols for a partial search string.',
    {
      search: z.string().describe('Partial symbol name to auto-complete'),
      max_results: z.number().optional().default(20).describe('Maximum number of results'),
    },
    async ({ search, max_results }) => {
      const data = await httpClient.post('/api/search/auto_complete', { search, max_results });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_encode_type',
    'Get the encode type at an address (byte, word, dword, code, ascii, unicode, etc.)',
    {
      address: z.string().describe('Address to query'),
      size: z.string().optional().default('1').describe('Size to check'),
    },
    async ({ address, size }) => {
      const data = await httpClient.get('/api/search/encode_type', { address, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
