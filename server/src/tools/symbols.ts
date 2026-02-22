import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSymbolTools(server: McpServer) {
  server.tool(
    'resolve_symbol',
    'Resolve symbols, search for symbols, or list symbols from a module',
    {
      action: z.enum(['name', 'at_address', 'search_pattern', 'list_module']).describe('Action to perform'),
      query: z.string().describe('The name, address, pattern, or module to search/resolve depending on action'),
      module: z.string().optional().describe('Optional module name to restrict search_pattern')
    },
    async ({ action, query, module }) => {
      let endpoint = '';
      let params: any = {};

      switch (action) {
        case 'name':
          endpoint = '/api/symbols/resolve';
          params.name = query;
          break;
        case 'at_address':
          endpoint = '/api/symbols/at';
          params.address = query;
          break;
        case 'search_pattern':
          endpoint = '/api/symbols/search';
          params.pattern = query;
          if (module) params.module = module;
          break;
        case 'list_module':
          endpoint = '/api/symbols/list';
          params.module = query;
          break;
      }

      const data = await httpClient.get(endpoint, params);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_annotation',
    'Get or set user-defined labels and comments at an address',
    {
      action: z.enum(['get_label', 'set_label', 'get_comment', 'set_comment']).describe('Action to perform'),
      address: z.string().describe('Address to label/comment'),
      text: z.string().optional().describe('Label or comment text (required for set actions)')
    },
    async ({ action, address, text }) => {
      let data: any;
      if (action.startsWith('get_')) {
        const type = action === 'get_label' ? 'labels' : 'comments';
        data = await httpClient.get(`/api/${type}/get`, { address });
      } else {
        const type = action === 'set_label' ? 'labels' : 'comments';
        if (!text) throw new Error("text is required for set actions");
        data = await httpClient.post(`/api/${type}/set`, { address, text });
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'manage_bookmark',
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
