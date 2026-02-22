import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerPatchTools(server: McpServer) {
  server.tool(
    'manage_patch',
    'List, apply, or restore byte patches in memory',
    {
      action: z.enum(['list', 'apply', 'restore']).describe('Action to perform'),
      address: z.string().optional().describe('Target address (required for apply and restore)'),
      bytes: z.string().optional().describe('Hex bytes to write (required for apply)'),
    },
    async ({ action, address, bytes }) => {
      let data: any;
      if (action === 'apply') {
        if (!address || !bytes) throw new Error("address and bytes required for apply");
        data = await httpClient.post('/api/patches/apply', { address, bytes });
      } else if (action === 'restore') {
        if (!address) throw new Error("address is required for restore");
        data = await httpClient.post('/api/patches/restore', { address });
      } else {
        data = await httpClient.get('/api/patches/list');
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'export_patched_module',
    'Export a patched module to a file on disk. Saves the module with all current patches applied.',
    {
      module: z.string().optional().describe('Module name to export (optional, exports full memory if omitted)'),
      path: z.string().describe('Output file path for the patched module'),
    },
    async ({ module, path }) => {
      const body: Record<string, string> = { path };
      if (module) {
        body.module = module;
      }
      const data = await httpClient.post('/api/patches/export', body);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
