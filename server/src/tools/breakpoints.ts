import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerBreakpointTools(server: McpServer) {
  server.tool(
    'list_breakpoints',
    'List all breakpoints (software, hardware, and memory)',
    {},
    async () => {
      const data = await httpClient.get('/api/breakpoints/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_breakpoint',
    'Get detailed info about a breakpoint at a specific address',
    { address: z.string().describe('Breakpoint address (hex string or expression)') },
    async ({ address }) => {
      const data = await httpClient.get('/api/breakpoints/get', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_breakpoint',
    'Set a software breakpoint (INT3) at an address',
    {
      address: z.string().describe('Address for the breakpoint (hex string, symbol name, or expression)'),
      singleshot: z.boolean().optional().default(false).describe('If true, breakpoint is deleted after first hit'),
    },
    async ({ address, singleshot }) => {
      const data = await httpClient.post('/api/breakpoints/set', { address, singleshot });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_hardware_breakpoint',
    'Set a hardware breakpoint using debug registers (limited to 4)',
    {
      address: z.string().describe('Address for the hardware breakpoint'),
      type: z.enum(['r', 'w', 'x']).optional().default('x').describe('Type: r=read, w=write, x=execute (default: x)'),
      size: z.enum(['1', '2', '4', '8']).optional().default('1').describe('Size in bytes: 1, 2, 4, or 8 (default: 1)'),
    },
    async ({ address, type, size }) => {
      const data = await httpClient.post('/api/breakpoints/set_hardware', { address, type, size });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_memory_breakpoint',
    'Set a memory breakpoint (page guard) on a memory region',
    {
      address: z.string().describe('Address for the memory breakpoint'),
      type: z.enum(['a', 'r', 'w', 'x']).optional().default('a').describe('Type: a=access, r=read, w=write, x=execute (default: a)'),
    },
    async ({ address, type }) => {
      const data = await httpClient.post('/api/breakpoints/set_memory', { address, type });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'delete_breakpoint',
    'Delete a breakpoint at an address',
    {
      address: z.string().describe('Address of the breakpoint to delete'),
      type: z.enum(['software', 'hardware', 'memory']).optional().default('software').describe('Breakpoint type (default: software)'),
    },
    async ({ address, type }) => {
      const data = await httpClient.post('/api/breakpoints/delete', { address, type });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'enable_breakpoint',
    'Enable a disabled breakpoint',
    { address: z.string().describe('Address of the breakpoint to enable') },
    async ({ address }) => {
      const data = await httpClient.post('/api/breakpoints/enable', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'disable_breakpoint',
    'Disable a breakpoint without deleting it',
    { address: z.string().describe('Address of the breakpoint to disable') },
    async ({ address }) => {
      const data = await httpClient.post('/api/breakpoints/disable', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'toggle_breakpoint',
    'Toggle a breakpoint between enabled and disabled',
    { address: z.string().describe('Address of the breakpoint to toggle') },
    async ({ address }) => {
      const data = await httpClient.post('/api/breakpoints/toggle', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_breakpoint_condition',
    'Set a conditional expression on a breakpoint (breaks only when condition is true)',
    {
      address: z.string().describe('Address of the breakpoint'),
      condition: z.string().describe('Condition expression (e.g. "eax==0", "rcx>0x100")'),
    },
    async ({ address, condition }) => {
      const data = await httpClient.post('/api/breakpoints/set_condition', { address, condition });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_breakpoint_log',
    'Set a log message on a breakpoint (logs without breaking)',
    {
      address: z.string().describe('Address of the breakpoint'),
      text: z.string().describe('Log format string (e.g. "eax={eax}, ecx={ecx}")'),
    },
    async ({ address, text }) => {
      const data = await httpClient.post('/api/breakpoints/set_log', { address, text });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'configure_breakpoint',
    'Unified breakpoint configuration in a single call. Creates the BP if needed, then sets all provided fields.\n' +
    'Replaces the 6-call workflow (set + condition + command_condition + command_text + silent + fast_resume).\n\n' +
    'IMPORTANT x64dbg expression syntax:\n' +
    '  - Memory dereference: use [addr] NOT poi(addr). poi() silently fails in breakpoint commands!\n' +
    '  - Format strings: use {format:expr} NOT {expr:format}\n' +
    '  - Example break_condition: "0" (never pause), "[esp+8]==11"\n' +
    '  - Example command_text: "eax=0;eip=[esp];esp=esp+C;run"\n' +
    '  - Example command_condition: "[esp+8]>=0x675C0000&&[esp+8]<0x67DC3000"',
    {
      address: z.string().describe('Breakpoint address (hex string, symbol name, or expression)'),
      bp_type: z.enum(['software', 'hardware', 'memory']).optional().default('software').describe('Breakpoint type'),
      singleshot: z.boolean().optional().describe('(software only) Delete after first hit'),
      hw_type: z.enum(['r', 'w', 'x']).optional().describe('(hardware only) r=read, w=write, x=execute'),
      hw_size: z.enum(['1', '2', '4', '8']).optional().describe('(hardware only) Size in bytes'),
      mem_type: z.enum(['a', 'r', 'w', 'x']).optional().describe('(memory only) a=access, r=read, w=write, x=execute'),
      break_condition: z.string().optional().describe('Condition to PAUSE execution (empty=always, "0"=never, "[eax]==5")'),
      command_condition: z.string().optional().describe('Condition to run command_text (empty=always, "[esp+8]==11")'),
      command_text: z.string().optional().describe('x64dbg commands to run on hit (e.g. "eax=0;eip=[esp];esp=esp+C;run")'),
      log_text: z.string().optional().describe('Log format string on hit (e.g. "{p:[esp]} called")'),
      log_condition: z.string().optional().describe('Condition to log (empty=always)'),
      silent: z.boolean().optional().describe('Suppress log output'),
      fast_resume: z.boolean().optional().describe('Resume immediately without pausing (for non-intrusive BP monitoring)'),
      name: z.string().optional().describe('Human-readable name for this breakpoint'),
    },
    async (params) => {
      const data = await httpClient.post('/api/breakpoints/configure', params);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'configure_breakpoints',
    'Batch configure multiple breakpoints in a single call. ' +
    'Reduces 48 MCP calls (8 BPs × 6 calls each) to a single call. ' +
    'Each entry in the breakpoints array has the same fields as configure_breakpoint.',
    {
      breakpoints: z.array(z.object({
        address: z.string().describe('Breakpoint address'),
        bp_type: z.enum(['software', 'hardware', 'memory']).optional(),
        singleshot: z.boolean().optional(),
        hw_type: z.string().optional(),
        hw_size: z.string().optional(),
        mem_type: z.string().optional(),
        break_condition: z.string().optional(),
        command_condition: z.string().optional(),
        command_text: z.string().optional(),
        log_text: z.string().optional(),
        log_condition: z.string().optional(),
        silent: z.boolean().optional(),
        fast_resume: z.boolean().optional(),
        name: z.string().optional(),
      })).describe('Array of breakpoint configurations'),
    },
    async ({ breakpoints }) => {
      const data = await httpClient.post('/api/breakpoints/configure_batch', { breakpoints });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'reset_breakpoint_hit_count',
    'Reset the hit counter for a breakpoint to zero. ' +
    'Useful when testing bypass configurations to get a clean delta from a known baseline.',
    {
      address: z.string().describe('Address of the breakpoint whose hit count to reset'),
    },
    async ({ address }) => {
      const data = await httpClient.post('/api/breakpoints/reset_hit_count', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
