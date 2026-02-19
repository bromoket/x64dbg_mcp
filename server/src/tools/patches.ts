import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerPatchTools(server: McpServer) {
  server.tool(
    'list_patches',
    'List all current byte patches applied to the debugged process',
    {},
    async () => {
      const data = await httpClient.get('/api/patches/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'apply_patch',
    'Apply a byte patch at a specific address. Writes new bytes and returns the original bytes for reverting.',
    {
      address: z.string().describe('Target address to patch (hex string or expression, e.g. "0x401000")'),
      bytes: z.string().describe('Hex bytes to write (e.g. "90 90 90" for NOPs)'),
    },
    async ({ address, bytes }) => {
      const data = await httpClient.post('/api/patches/apply', { address, bytes });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'restore_patch',
    'Restore original bytes at a previously patched address',
    {
      address: z.string().describe('Address of the patch to restore (hex string or expression)'),
    },
    async ({ address }) => {
      const data = await httpClient.post('/api/patches/restore', { address });
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
