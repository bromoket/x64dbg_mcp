import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerModuleTools(server: McpServer) {
  server.tool(
    'list_modules',
    'List all loaded modules (DLLs and EXE) with base addresses, sizes, and entry points',
    {},
    async () => {
      const data = await httpClient.get('/api/modules/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_module_info',
    'Get detailed info about a specific module, its base address, section, or party (user/system)',
    {
      action: z.enum(['get_info', 'get_base', 'get_section', 'get_party']).describe('Information to get'),
      query: z.string().describe('Module name (for get_info, get_base), Address (for get_section), or Base (for get_party)')
    },
    async ({ action, query }) => {
      let data: any;
      switch (action) {
        case 'get_info':
          data = await httpClient.get('/api/modules/get', { name: query });
          break;
        case 'get_base':
          data = await httpClient.get('/api/modules/base', { name: query });
          break;
        case 'get_section':
          data = await httpClient.get('/api/modules/section', { address: query });
          break;
        case 'get_party':
          data = await httpClient.get('/api/modules/party', { base: query });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
