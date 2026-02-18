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
    'Get detailed info about a specific loaded module',
    { name: z.string().describe('Module name (e.g. "kernel32", "ntdll", "target.exe")') },
    async ({ name }) => {
      const data = await httpClient.get('/api/modules/get', { name });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_module_base',
    'Get the base address of a loaded module',
    { name: z.string().describe('Module name') },
    async ({ name }) => {
      const data = await httpClient.get('/api/modules/base', { name });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
