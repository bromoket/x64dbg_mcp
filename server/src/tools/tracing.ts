import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerTracingTools(server: McpServer) {
  server.tool(
    'manage_trace',
    'Control execution tracing (start, stop, conditional run, animate)',
    {
      action: z.enum(['into', 'over', 'run', 'stop', 'animate', 'conditional_run', 'log_setup']).describe('Trace action'),
      condition: z.string().optional().describe('Break or log condition expression'),
      max_steps: z.string().optional().describe('Max steps for into/over'),
      log_text: z.string().optional().describe('Log text format'),
      party: z.string().optional().describe('0 for user, 1 for system (used with run action)'),
      command: z.string().optional().describe('Animation command (used with animate action)'),
      file: z.string().optional().describe('Output trace log file (used with log_setup)'),
      command_text: z.string().optional().describe('Command to execute (used with conditional_run)')
    },
    async ({ action, condition, max_steps, log_text, party, command, file, command_text }) => {
      let data: any;
      switch (action) {
        case 'into': data = await httpClient.post('/api/trace/into', { condition: condition || '', max_steps: max_steps || '', log_text: log_text || '' }); break;
        case 'over': data = await httpClient.post('/api/trace/over', { condition: condition || '', max_steps: max_steps || '', log_text: log_text || '' }); break;
        case 'run': data = await httpClient.post('/api/trace/run', { party: party || '0' }); break;
        case 'stop': data = await httpClient.post('/api/trace/stop'); break;
        case 'animate':
          if (!command) throw new Error("command required for animate");
          data = await httpClient.post('/api/trace/animate', { command });
          break;
        case 'conditional_run': data = await httpClient.post('/api/trace/conditional_run', {
            break_condition: condition || '', log_text: log_text || '', log_condition: '', command_text: command_text || '', command_condition: '', type: 'into'
          }); break;
        case 'log_setup':
          if (!file) throw new Error("file required for log_setup");
          data = await httpClient.post('/api/trace/log', { file, text: log_text || '', condition: condition || '' });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_trace_info',
    'Get trace record hit count or byte type at an address',
    {
      action: z.enum(['hitcount', 'type']).describe('Info to retrieve'),
      address: z.string().describe('Address to query')
    },
    async ({ action, address }) => {
      let data: any;
      if (action === 'hitcount') {
        data = await httpClient.get('/api/trace/record/hitcount', { address });
      } else {
        data = await httpClient.get('/api/trace/record/type', { address });
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_trace_record_type',
    'Set the trace record type for a memory page (None, BitExec, ByteWithExecTypeAndCounter, WordWithExecTypeAndCounter)',
    {
      address: z.string().describe('Page address'),
      type: z.number().optional().default(0).describe('Trace record type: 0=None, 1=BitExec, 2=ByteWithCounter, 3=WordWithCounter'),
    },
    async ({ address, type }) => {
      const data = await httpClient.post('/api/trace/record/set_type', { address, type });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
