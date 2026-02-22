import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDebugTools(server: McpServer) {
  server.tool(
    'execute_debug_action',
    'Execute a fundamental debugger action (run, pause, step, stop, restart, run_to_address)',
    {
      action: z.enum([
        'run', 'pause', 'force_pause', 'step_into', 'step_over', 'step_out',
        'stop_debug', 'restart_debug', 'run_to_address'
      ]).describe('The debug action to perform'),
      address: z.string().optional().describe('Target address (required only for run_to_address)')
    },
    async ({ action, address }) => {
      let endpoint = '';
      let payload: any = undefined;

      switch(action) {
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
          if (!address) throw new Error("address is required for run_to_address");
          payload = { address };
          break;
      }

      const data = await httpClient.post(endpoint, payload);
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_debug_state',
    'Get the overall debugger state, health, and current instruction pointer (CIP)',
    {
      include_health: z.boolean().optional().default(false).describe('Also check plugin health/version')
    },
    async ({ include_health }) => {
      const stateData = await httpClient.get('/api/debug/state');
      let result: any = { state: stateData };

      if (include_health) {
        try {
          result.health = await httpClient.get('/api/health');
        } catch (e) {
          result.health = { error: "Health check failed" };
        }
      }
      return { content: [{ type: 'text', text: JSON.stringify(result, null, 2) }] };
    }
  );
}
