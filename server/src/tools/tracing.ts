import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerTracingTools(server: McpServer) {
  server.tool(
    'trace_into',
    'Start conditional trace into (single-step through calls). Runs asynchronously for millions of instructions. Essential for VM dispatcher analysis.',
    {
      condition: z.string().optional().default('').describe('Break condition expression (e.g. "cip >= 0x401000 && cip <= 0x402000")'),
      max_steps: z.string().optional().default('').describe('Maximum number of steps before stopping'),
      log_text: z.string().optional().default('').describe('Log text format string'),
    },
    async ({ condition, max_steps, log_text }) => {
      const data = await httpClient.post('/api/trace/into', { condition, max_steps, log_text });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'trace_over',
    'Start conditional trace over (step over calls). Runs asynchronously. Useful for tracing VM handler dispatch loops.',
    {
      condition: z.string().optional().default('').describe('Break condition expression'),
      max_steps: z.string().optional().default('').describe('Maximum number of steps'),
      log_text: z.string().optional().default('').describe('Log text format string'),
    },
    async ({ condition, max_steps, log_text }) => {
      const data = await httpClient.post('/api/trace/over', { condition, max_steps, log_text });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'trace_run',
    'Run to user/system code (RunToParty). Party 0 = user code, 1 = system code. Useful to skip past system DLL transitions.',
    {
      party: z.string().optional().default('0').describe('Party type: "0" for user code, "1" for system code'),
    },
    async ({ party }) => {
      const data = await httpClient.post('/api/trace/run', { party });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'trace_stop',
    'Stop an active trace recording or conditional trace',
    {},
    async () => {
      const data = await httpClient.post('/api/trace/stop');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_trace_hit_count',
    'Get the trace record hit count at an address. Shows how many times an instruction was executed during tracing.',
    {
      address: z.string().describe('Address to query (hex or expression)'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/trace/record/hitcount', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_trace_record_type',
    'Get the trace record byte type at an address (instruction heading/body/tailing, data types, etc.)',
    {
      address: z.string().describe('Address to query'),
    },
    async ({ address }) => {
      const data = await httpClient.get('/api/trace/record/type', { address });
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

  server.tool(
    'animate_command',
    'Execute an animation command (single-step with visual update). Slower than trace but shows each step in the UI.',
    {
      command: z.string().describe('Animation command (e.g. "StepInto", "StepOver")'),
    },
    async ({ command }) => {
      const data = await httpClient.post('/api/trace/animate', { command });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'conditional_run',
    'Start a conditional trace run with full control over break/log/command conditions',
    {
      break_condition: z.string().optional().default('').describe('Expression that when true, stops the trace'),
      log_text: z.string().optional().default('').describe('Text to log each step'),
      log_condition: z.string().optional().default('').describe('Condition for logging'),
      command_text: z.string().optional().default('').describe('Command to execute each step'),
      command_condition: z.string().optional().default('').describe('Condition for command execution'),
      type: z.enum(['into', 'over']).optional().default('into').describe('Trace type: into (follow calls) or over (skip calls)'),
    },
    async ({ break_condition, log_text, log_condition, command_text, command_condition, type }) => {
      const data = await httpClient.post('/api/trace/conditional_run', {
        break_condition, log_text, log_condition, command_text, command_condition, type,
      });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'trace_log_setup',
    'Set up trace logging to a file. Must be called before starting a trace to capture output.',
    {
      file: z.string().describe('Output file path for trace log'),
      text: z.string().optional().default('').describe('Log text format string'),
      condition: z.string().optional().default('').describe('Log condition expression'),
    },
    async ({ file, text, condition }) => {
      const data = await httpClient.post('/api/trace/log', { file, text, condition });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
