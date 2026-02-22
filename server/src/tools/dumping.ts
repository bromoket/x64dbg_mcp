import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDumpingTools(server: McpServer) {
  server.tool(
    'get_module_dump_info',
    'Get PE header, sections, imports, exports, Entry Point, or relocations for a loaded module',
    {
      action: z.enum(['pe_header', 'sections', 'imports', 'exports', 'entry_point', 'relocations']).describe('Information to get'),
      module: z.string().optional().describe('Module name (required for most actions)'),
      address: z.string().optional().describe('Address (required for pe_header and relocations)')
    },
    async ({ action, module, address }) => {
      let data: any;
      switch (action) {
        case 'pe_header':
          if (!address) throw new Error("address is required for pe_header");
          data = await httpClient.get('/api/dump/pe_header', { address });
          break;
        case 'sections':
          if (!module) throw new Error("module is required for sections");
          data = await httpClient.get('/api/dump/sections', { module });
          break;
        case 'imports':
          if (!module) throw new Error("module is required for imports");
          data = await httpClient.get('/api/dump/imports', { module });
          break;
        case 'exports':
          if (!module) throw new Error("module is required for exports");
          data = await httpClient.get('/api/dump/exports', { module });
          break;
        case 'entry_point':
          if (!module) throw new Error("module is required for entry_point");
          data = await httpClient.get('/api/dump/entry_point', { module });
          break;
        case 'relocations':
          if (!address) throw new Error("address is required for relocations");
          data = await httpClient.get('/api/dump/relocations', { address });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'dump_module',
    'Dump a loaded module from memory to a file. Critical for unpacking VMProtect/Themida protected binaries.',
    {
      module: z.string().describe('Module name to dump (e.g. "target.exe", "packed.dll")'),
      file: z.string().optional().default('').describe('Output file path (empty = x64dbg will prompt)'),
    },
    async ({ module, file }) => {
      const data = await httpClient.post('/api/dump/module', { module, file });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'fix_iat',
    'Attempt IAT (Import Address Table) reconstruction using Scylla. Used after dumping a packed binary to fix import references.',
    {
      oep: z.string().describe('Original Entry Point address for IAT reconstruction'),
    },
    async ({ oep }) => {
      const data = await httpClient.post('/api/dump/fix_iat', { oep });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
