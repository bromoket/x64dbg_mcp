import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSearchTools(server: McpServer) {
  server.tool(
    'search_pattern',
    'Search for a byte pattern (AOB scan) in process memory. Returns ALL matches. Supports wildcard bytes (??).\n' +
    'Pattern formats: "C4 CB 75 5B" or "C4CB755B" (spaces optional), wildcards as "??" or "??".\n' +
    'With address+size: scans that specific range. Without: scans all committed readable memory.\n' +
    'Returns matches[] array with all hit addresses.',
    {
      pattern: z.string().describe(
        'Byte pattern in hex (e.g. "48 89 5C 24 ??", "C4CB755B", "E8 ?? ?? ?? ??"). ' +
        'Use ?? for wildcard bytes. Spaces are optional.'
      ),
      address: z.string().optional().describe('Start address to limit search range (optional, omit to scan all memory)'),
      size: z.string().optional().describe('Size of range to search in bytes (required when address is specified)'),
      max_results: z.number().optional().default(1000).describe('Maximum number of matches to return (default: 1000, max: 10000)'),
    },
    async ({ pattern, address, size, max_results }) => {
      const body: Record<string, unknown> = { pattern, max_results };
      if (address) body.address = address;
      if (size) body.size = size;
      const data = await httpClient.post('/api/search/pattern', body);
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
    'Get the string value at a memory address. Always returns raw_hex for transparency.\n' +
    'encoding="auto" uses x64dbg\'s built-in detection plus raw ASCII comparison.\n' +
    'encoding="ascii" reads a null-terminated ASCII string directly.\n' +
    'encoding="unicode" reads UTF-16LE (common in Windows API strings).',
    {
      address: z.string().describe('Address to read string from'),
      encoding: z.enum(['auto', 'ascii', 'unicode']).optional().default('auto').describe(
        'String encoding: auto (use x64dbg detection), ascii (null-terminated ASCII), unicode (UTF-16LE)'
      ),
      max_length: z.number().optional().default(256).describe('Max bytes to read (default 256, max 4096)'),
    },
    async ({ address, encoding, max_length }) => {
      const data = await httpClient.get('/api/search/string_at', {
        address,
        encoding,
        max_length: String(max_length),
      });
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
