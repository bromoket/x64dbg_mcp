import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerBreakpointTools(server: McpServer) {
  server.tool(
    'manage_breakpoint',
    'Set, get, toggle, enable, disable, delete, or reset breakpoints.',
    {
      action: z.enum([
        'set_software', 'set_hardware', 'set_memory',
        'delete', 'enable', 'disable', 'toggle',
        'set_condition', 'set_log', 'reset_hit_count', 'get'
      ]).describe('The breakpoint action to perform'),
      address: z.string().describe('Address of the breakpoint (hex string, symbol name, or expression)'),
      type: z.enum(['software', 'hardware', 'memory', 'r', 'w', 'x', 'a']).optional().describe('Hardware/Memory type or general BP type used for hardware/memory/delete/etc.'),
      size: z.enum(['1', '2', '4', '8']).optional().describe('Hardware size (1, 2, 4, 8)'),
      condition: z.string().optional().describe('Condition expression (e.g. "eax==0") for set_condition'),
      text: z.string().optional().describe('Log format string for set_log'),
      singleshot: z.boolean().optional().describe('If true, breakpoint is deleted after first hit for software BP')
    },
    async ({ action, address, type, size, condition, text, singleshot }) => {
      let endpoint = '';
      let payload: any = { address };

      switch(action) {
        case 'set_software':
          endpoint = '/api/breakpoints/set';
          payload.singleshot = singleshot || false;
          break;
        case 'set_hardware':
          endpoint = '/api/breakpoints/set_hardware';
          payload.type = type || 'x';
          payload.size = size || '1';
          break;
        case 'set_memory':
          endpoint = '/api/breakpoints/set_memory';
          payload.type = type || 'a';
          break;
        case 'delete':
          endpoint = '/api/breakpoints/delete';
          payload.type = ['software', 'hardware', 'memory'].includes(type as string) ? type : 'software';
          break;
        case 'enable': endpoint = '/api/breakpoints/enable'; break;
        case 'disable': endpoint = '/api/breakpoints/disable'; break;
        case 'toggle': endpoint = '/api/breakpoints/toggle'; break;
        case 'set_condition':
          endpoint = '/api/breakpoints/set_condition';
          if (!condition) throw new Error("condition is required");
          payload.condition = condition;
          break;
        case 'set_log':
          endpoint = '/api/breakpoints/set_log';
          if (!text) throw new Error("text is required");
          payload.text = text;
          break;
        case 'reset_hit_count': endpoint = '/api/breakpoints/reset_hit_count'; break;
        case 'get':
          const getData = await httpClient.get('/api/breakpoints/get', { address });
          return { content: [{ type: 'text', text: JSON.stringify(getData, null, 2) }] };
      }

      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

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
    'configure_breakpoint',
    'Unified breakpoint configuration in a single call. Creates the BP if needed, then sets all provided fields.\n' +
    'Replaces the 6-call workflow (set + condition + command_condition + command_text + silent + fast_resume).\n\n' +
    'IMPORTANT x64dbg expression syntax:\n' +
    '  - Memory dereference: use [addr] NOT poi(addr). poi() silently fails in breakpoint commands!\n' +
    '  - Format strings: use {format:expr} NOT {expr:format}\n' +
    '  - Example break_condition: "0" (never pause), "[esp+8]==11"\n' +
    '  - Example command_text: "eax=0;eip=[esp];esp=esp+C;run"',
    {
      address: z.string().describe('Breakpoint address (hex string, symbol name, or expression)'),
      bp_type: z.enum(['software', 'hardware', 'memory']).optional().default('software'),
      singleshot: z.boolean().optional(),
      hw_type: z.enum(['r', 'w', 'x']).optional(),
      hw_size: z.enum(['1', '2', '4', '8']).optional(),
      mem_type: z.enum(['a', 'r', 'w', 'x']).optional(),
      break_condition: z.string().optional(),
      command_condition: z.string().optional(),
      command_text: z.string().optional(),
      log_text: z.string().optional(),
      log_condition: z.string().optional(),
      silent: z.boolean().optional(),
      fast_resume: z.boolean().optional(),
      name: z.string().optional(),
    },
    async (params) => {
      const data = await httpClient.post('/api/breakpoints/configure', params);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'configure_breakpoints',
    'Batch configure multiple breakpoints in a single call.',
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
}
