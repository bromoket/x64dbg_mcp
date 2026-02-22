import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerTracingTools(server: McpServer) {
  server.tool(
    'x64dbg_tracing',
    'Execution tracing operations: step into/over, run, animate, get trace info',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("into"), condition: z.string().optional(), max_steps: z.string().optional(), log_text: z.string().optional() }),
        z.object({ action: z.literal("over"), condition: z.string().optional(), max_steps: z.string().optional(), log_text: z.string().optional() }),
        z.object({ action: z.literal("run"), party: z.string().optional() }),
        z.object({ action: z.literal("stop") }),
        z.object({ action: z.literal("animate"), command: z.string() }),
        z.object({ action: z.literal("conditional_run"), condition: z.string().optional(), log_text: z.string().optional(), command_text: z.string().optional() }),
        z.object({ action: z.literal("log_setup"), file: z.string(), log_text: z.string().optional(), condition: z.string().optional() }),
        z.object({ action: z.literal("hitcount"), address: z.string() }),
        z.object({ action: z.literal("type"), address: z.string() }),
        z.object({ action: z.literal("set_type"), address: z.string(), type: z.number().optional().default(0) })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'into': data = await httpClient.post('/api/trace/into', { condition: action.condition || '', max_steps: action.max_steps || '', log_text: action.log_text || '' }); break;
        case 'over': data = await httpClient.post('/api/trace/over', { condition: action.condition || '', max_steps: action.max_steps || '', log_text: action.log_text || '' }); break;
        case 'run': data = await httpClient.post('/api/trace/run', { party: action.party || '0' }); break;
        case 'stop': data = await httpClient.post('/api/trace/stop'); break;
        case 'animate': data = await httpClient.post('/api/trace/animate', { command: action.command }); break;
        case 'conditional_run': data = await httpClient.post('/api/trace/conditional_run', { break_condition: action.condition || '', log_text: action.log_text || '', log_condition: '', command_text: action.command_text || '', command_condition: '', type: 'into' }); break;
        case 'log_setup': data = await httpClient.post('/api/trace/log', { file: action.file, text: action.log_text || '', condition: action.condition || '' }); break;
        case 'hitcount': data = await httpClient.get('/api/trace/record/hitcount', { address: action.address }); break;
        case 'type': data = await httpClient.get('/api/trace/record/type', { address: action.address }); break;
        case 'set_type': data = await httpClient.post('/api/trace/record/set_type', { address: action.address, type: action.type }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
