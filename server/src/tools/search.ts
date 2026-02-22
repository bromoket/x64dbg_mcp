import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSearchTools(server: McpServer) {
  server.tool(
    'search_memory',
    'Search for byte patterns or strings in process memory',
    {
      type: z.enum(['pattern', 'string']).describe('Search type'),
      query: z.string().describe('Byte pattern (e.g. "48 89 5C ??") or text string'),
      address: z.string().optional().describe('Start address for pattern search'),
      size: z.string().optional().describe('Size of range for pattern search'),
      max_results: z.number().optional().default(1000).describe('Max matches for pattern search'),
      module: z.string().optional().default('').describe('Module name to restrict string search'),
      encoding: z.enum(['utf8', 'ascii', 'unicode']).optional().default('utf8').describe('Encoding for string search'),
    },
    async ({ type, query, address, size, max_results, module, encoding }) => {
      let data: any;
      if (type === 'pattern') {
        const body: Record<string, unknown> = { pattern: query, max_results };
        if (address) body.address = address;
        if (size) body.size = size;
        data = await httpClient.post('/api/search/pattern', body);
      } else {
        data = await httpClient.post('/api/search/string', { text: query, module, encoding });
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_search_info',
    'Get string value, symbol autocomplete, or encode type at an address',
    {
      action: z.enum(['string_at', 'symbol_auto_complete', 'encode_type']).describe('Action'),
      query: z.string().describe('Address (for string_at/encode_type) or partial symbol name (for autocomplete)'),
      encoding: z.enum(['auto', 'ascii', 'unicode']).optional().default('auto').describe('String encoding (for string_at)'),
      max_length: z.number().optional().default(256).describe('Max bytes to read (for string_at) or max results (for autocomplete)'),
      size: z.string().optional().default('1').describe('Size to check (for encode_type)'),
    },
    async ({ action, query, encoding, max_length, size }) => {
      let data: any;
      switch (action) {
        case 'string_at':
          data = await httpClient.get('/api/search/string_at', {
            address: query,
            encoding,
            max_length: String(max_length),
          });
          break;
        case 'symbol_auto_complete':
          data = await httpClient.post('/api/search/auto_complete', {
            search: query,
            max_results: max_length
          });
          break;
        case 'encode_type':
          data = await httpClient.get('/api/search/encode_type', {
            address: query,
            size
          });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
