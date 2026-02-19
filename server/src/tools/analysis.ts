import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerAnalysisTools(server: McpServer) {
  // Existing analysis routes (were previously not in MCP)
  server.tool(
    'get_function_info',
    'Get function boundaries, label, and module at an address',
    {
      address: z.string().optional().default('cip').describe('Address within the function (default: current instruction)'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/function', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_xrefs_to',
    'Get cross-references TO an address (who calls/jumps to this address)',
    {
      address: z.string().describe('Target address to find references to'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/xrefs_to', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_xrefs_from',
    'Get cross-references FROM an address (what does this instruction reference)',
    {
      address: z.string().describe('Source address to find references from'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/xrefs_from', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_basic_blocks',
    'Get basic blocks (CFG) for a function. Returns block start/end addresses and sizes.',
    {
      address: z.string().optional().default('cip').describe('Address within the function'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/basic_blocks', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'find_strings_in_module',
    'Find string references in a module (displayed in x64dbg references view)',
    {
      module: z.string().describe('Module name to search for strings'),
    },
    async ({ module }) => {
      const data = await httpClient.get('/api/analysis/strings', { module });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  // New Tier 2 tools: Constants, error codes, watches, structs
  server.tool(
    'list_constants',
    'List all known constants in the x64dbg database (Windows API constants, etc.)',
    {},
    async () => {
      const data = await httpClient.get('/api/analysis/constants');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_error_codes',
    'List all known Windows error codes (GetLastError values) with their names',
    {},
    async () => {
      const data = await httpClient.get('/api/analysis/error_codes');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'is_watchdog_triggered',
    'Check if a watch expression watchdog has been triggered',
    {
      id: z.string().optional().default('0').describe('Watch ID (decimal)'),
    },
    async ({ id }) => {
      const data = await httpClient.get('/api/analysis/watch', { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'list_structs',
    'List all defined struct types in the x64dbg database',
    {},
    async () => {
      const data = await httpClient.get('/api/analysis/structs');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  // Tier 3 analysis tools
  server.tool(
    'get_source_location',
    'Get the source file and line number for an address (requires debug symbols)',
    {
      address: z.string().optional().default('cip').describe('Address to look up source for'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/source', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'va_to_file_offset',
    'Convert a virtual address (VA) to the corresponding file offset on disk',
    {
      address: z.string().describe('Virtual address to convert'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/analysis/va_to_file', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'file_offset_to_va',
    'Convert a file offset to the corresponding virtual address (VA) in the loaded module',
    {
      module: z.string().describe('Module name'),
      offset: z.string().describe('File offset (hex)'),
    },
    async ({ module, offset }) => {
      const data = await httpClient.get('/api/analysis/file_to_va', { module, offset });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_mnemonic_brief',
    'Get a brief description of an x86/x64 instruction mnemonic (e.g. "mov", "jmp", "call")',
    {
      mnemonic: z.string().describe('Instruction mnemonic (e.g. "mov", "push", "xor")'),
    },
    async ({ mnemonic }) => {
      const data = await httpClient.get('/api/analysis/mnemonic_brief', { mnemonic });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
