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
}
