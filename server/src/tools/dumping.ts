import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDumpingTools(server: McpServer) {
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
    'get_pe_header',
    'Parse and return PE header fields from a module loaded in memory. Shows MZ/PE signatures, machine type, sections count, entry point, image base.',
    {
      address: z.string().describe('Base address of the PE image (module base or expression)'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/dump/pe_header', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_sections',
    'Get PE section headers (name, virtual address, sizes, characteristics) for a loaded module',
    {
      module: z.string().describe('Module name (e.g. "ntdll", "target.exe")'),
    },
    async ({ module }) => {
      const data = await httpClient.get('/api/dump/sections', { module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_imports',
    'Display the import table (IAT) for a loaded module in x64dbg references view',
    {
      module: z.string().describe('Module name'),
    },
    async ({ module }) => {
      const data = await httpClient.get('/api/dump/imports', { module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_exports',
    'Display the export table for a loaded module in x64dbg references view',
    {
      module: z.string().describe('Module name'),
    },
    async ({ module }) => {
      const data = await httpClient.get('/api/dump/exports', { module });
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

  server.tool(
    'get_relocations',
    'Get relocation entries for the module containing the given address',
    {
      address: z.string().describe('Address within the module to get relocations for'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/dump/relocations', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'export_patched_file',
    'Export all current patches to a patched copy of the executable',
    {
      filename: z.string().describe('Output file path for the patched binary'),
    },
    async ({ filename }) => {
      const data = await httpClient.post('/api/patches/export_file', { filename });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_entry_point',
    'Get the entry point (AddressOfEntryPoint) of a loaded module',
    {
      module: z.string().describe('Module name'),
    },
    async ({ module }) => {
      const data = await httpClient.get('/api/dump/entry_point', { module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
