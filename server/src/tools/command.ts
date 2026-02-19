import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerCommandTools(server: McpServer) {
  server.tool(
    'execute_command',
    'Execute a raw x64dbg command (e.g. "bp 0x401000", "run", "mov eax, 0"). Use this for any command not covered by dedicated tools.',
    { command: z.string().describe('x64dbg command string to execute') },
    async ({ command }) => {
      const data = await httpClient.post('/api/command/exec', { command });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'evaluate_expression',
    'Evaluate an x64dbg expression and return its numeric value. Supports registers, symbols, arithmetic, and debugger functions.',
    { expression: z.string().describe('Expression to evaluate (e.g. "rax+0x10", "kernel32.CreateFileW", "mod.main()", "$pid")') },
    async ({ expression }) => {
      const data = await httpClient.post('/api/command/eval', { expression });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'execute_script',
    'Execute a batch of x64dbg commands sequentially',
    { commands: z.array(z.string()).describe('Array of x64dbg command strings to execute in order') },
    async ({ commands }) => {
      const data = await httpClient.post('/api/command/script', { commands });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'format_string',
    'Format a string using x64dbg expression engine (e.g. "{rax:x}" → "00007FF...", "{s:rsp}" → stack string)',
    {
      format: z.string().describe('Format string with x64dbg expressions (e.g. "{rax:x}", "{s:rsp}", "{a:cip}")'),
    },
    async ({ format }) => {
      const data = await httpClient.post('/api/command/format', { format });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_debug_events',
    'Get the total number of debug events that have occurred',
    {},
    async () => {
      const data = await httpClient.get('/api/command/events');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_init_script',
    'Set the debuggee initialization script (runs when debug session starts)',
    {
      file: z.string().describe('Path to the script file'),
    },
    async ({ file }) => {
      const data = await httpClient.post('/api/command/init_script', { file });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_init_script',
    'Get the current debuggee initialization script path',
    {},
    async () => {
      const data = await httpClient.get('/api/command/init_script');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_database_hash',
    'Get the hash of the current x64dbg database',
    {},
    async () => {
      const data = await httpClient.get('/api/command/hash');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
