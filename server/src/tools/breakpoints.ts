import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerBreakpointTools(server: McpServer) {
  server.tool(
    'x64dbg_breakpoints',
    'Unified tool for all breakpoint operations (set, get, list, configure, toggle, remove)',
    {
      action: z.discriminatedUnion("action", [
        z.object({
          action: z.literal("set_software"),
          address: z.string().describe("Address of the breakpoint"),
          singleshot: z.boolean().optional()
        }),
        z.object({
          action: z.literal("set_hardware"),
          address: z.string(),
          type: z.enum(['r', 'w', 'x']).optional().describe("Hardware BP type (read/write/execute)"),
          size: z.enum(['1', '2', '4', '8']).optional().describe("Hardware size")
        }),
        z.object({
          action: z.literal("set_memory"),
          address: z.string(),
          type: z.enum(['a', 'r', 'w', 'x']).optional().describe("Memory BP type")
        }),
        z.object({
          action: z.literal("delete"),
          address: z.string(),
          type: z.enum(['software', 'hardware', 'memory']).optional()
        }),
        z.object({ action: z.literal("enable"), address: z.string() }),
        z.object({ action: z.literal("disable"), address: z.string() }),
        z.object({ action: z.literal("toggle"), address: z.string() }),
        z.object({
          action: z.literal("set_condition"),
          address: z.string(),
          condition: z.string().describe("Expression, e.g. eax==0")
        }),
        z.object({
          action: z.literal("set_log"),
          address: z.string(),
          text: z.string().describe("Format string for logging")
        }),
        z.object({ action: z.literal("reset_hit_count"), address: z.string() }),
        z.object({ action: z.literal("get"), address: z.string() }),
        z.object({ action: z.literal("list") }),
        z.object({
          action: z.literal("configure"),
          address: z.string(),
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
          name: z.string().optional()
        }),
        z.object({
          action: z.literal("configure_batch"),
          breakpoints: z.array(z.object({
            address: z.string(),
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
            name: z.string().optional()
          }))
        })
      ])
    },
    async ({ action }) => {
      let endpoint = '';
      let payload: any = action.action !== 'list' && action.action !== 'configure_batch'
        ? { address: (action as any).address }
        : {};

      switch(action.action) {
        case 'set_software':
          endpoint = '/api/breakpoints/set';
          payload.singleshot = action.singleshot || false;
          break;
        case 'set_hardware':
          endpoint = '/api/breakpoints/set_hardware';
          payload.type = action.type || 'x';
          payload.size = action.size || '1';
          break;
        case 'set_memory':
          endpoint = '/api/breakpoints/set_memory';
          payload.type = action.type || 'a';
          break;
        case 'delete':
          endpoint = '/api/breakpoints/delete';
          payload.type = action.type;
          break;
        case 'enable': endpoint = '/api/breakpoints/enable'; break;
        case 'disable': endpoint = '/api/breakpoints/disable'; break;
        case 'toggle': endpoint = '/api/breakpoints/toggle'; break;
        case 'set_condition':
          endpoint = '/api/breakpoints/set_condition';
          payload.condition = action.condition;
          break;
        case 'set_log':
          endpoint = '/api/breakpoints/set_log';
          payload.text = action.text;
          break;
        case 'reset_hit_count': endpoint = '/api/breakpoints/reset_hit_count'; break;
        case 'get':
          const getData = await httpClient.get('/api/breakpoints/get', { address: action.address });
          return { content: [{ type: 'text', text: JSON.stringify(getData, null, 2) }] };
        case 'list':
          const listData = await httpClient.get('/api/breakpoints/list');
          return { content: [{ type: 'text', text: JSON.stringify(listData, null, 2) }] };
        case 'configure':
          endpoint = '/api/breakpoints/configure';
          payload = { ...action };
          break;
        case 'configure_batch':
          endpoint = '/api/breakpoints/configure_batch';
          payload = { breakpoints: action.breakpoints };
          break;
      }

      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
