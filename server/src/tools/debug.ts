import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';


export function registerDebugTools(server: McpServer) {
  server.tool(
    'x64dbg_debug',
    'Execute core debugger actions (run, pause, step, state, etc.)',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("run") }),
        z.object({ action: z.literal("pause") }),
        z.object({ action: z.literal("force_pause") }),
        z.object({ action: z.literal("step_into") }),
        z.object({ action: z.literal("step_over") }),
        z.object({ action: z.literal("step_out") }),
        z.object({ action: z.literal("stop_debug") }),
        z.object({ action: z.literal("restart_debug") }),
        z.object({
          action: z.literal("run_to_address"),
          address: z.string().describe("Target address to run to")
        }),
        z.object({
          action: z.literal("state"),
          include_health: z.boolean().optional().describe("Also check plugin health/version")
        })
      ])
    },
    async ({ action }) => {
      let endpoint = '';
      let payload: any = undefined;

      switch(action.action) {
        case 'run': endpoint = '/api/debug/run'; break;
        case 'pause': endpoint = '/api/debug/pause'; break;
        case 'force_pause': endpoint = '/api/debug/force_pause'; break;
        case 'step_into': endpoint = '/api/debug/step_into'; break;
        case 'step_over': endpoint = '/api/debug/step_over'; break;
        case 'step_out': endpoint = '/api/debug/step_out'; break;
        case 'stop_debug': endpoint = '/api/debug/stop'; break;
        case 'restart_debug': endpoint = '/api/debug/restart'; break;
        case 'run_to_address':
          endpoint = '/api/debug/run_to';
          payload = { address: action.address };
          break;
        case 'state':
          const stateData = await httpClient.get('/api/debug/state');
          let result: any = { state: stateData };
          if (action.include_health) {
            try {
              result.health = await httpClient.get('/api/health');
            } catch (e) {
              result.health = { error: "Health check failed" };
            }
          }
          return { content: [{ type: 'text', text: JSON.stringify(result, null, 2) }] };
      }

      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
