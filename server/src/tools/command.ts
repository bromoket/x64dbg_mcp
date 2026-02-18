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
}
