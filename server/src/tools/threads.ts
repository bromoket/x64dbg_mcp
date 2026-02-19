import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerThreadTools(server: McpServer) {
  server.tool(
    'list_threads',
    'List all threads in the debugged process with IDs, names, and start addresses',
    {},
    async () => {
      const data = await httpClient.get('/api/threads/list');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_current_thread',
    'Get info about the currently active thread',
    {},
    async () => {
      const data = await httpClient.get('/api/threads/current');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_thread',
    'Get info about a specific thread by its thread ID',
    { id: z.string().describe('Thread ID (decimal)') },
    async ({ id }) => {
      const data = await httpClient.get('/api/threads/get', { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'switch_thread',
    'Switch the debugger focus to a different thread',
    { id: z.number().describe('Thread ID to switch to') },
    async ({ id }) => {
      const data = await httpClient.post('/api/threads/switch', { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'suspend_thread',
    'Suspend a thread (increment suspend count)',
    { id: z.number().describe('Thread ID to suspend') },
    async ({ id }) => {
      const data = await httpClient.post('/api/threads/suspend', { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'resume_thread',
    'Resume a suspended thread (decrement suspend count)',
    { id: z.number().describe('Thread ID to resume') },
    async ({ id }) => {
      const data = await httpClient.post('/api/threads/resume', { id });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_thread_count',
    'Get the total number of threads in the debugged process',
    {},
    async () => {
      const data = await httpClient.get('/api/threads/count');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_thread_teb',
    'Get the TEB (Thread Environment Block) address for a thread',
    {
      tid: z.string().describe('Thread ID (decimal)'),
    },
    async ({ tid }) => {
      const data = await httpClient.get('/api/threads/teb', { tid });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_thread_name',
    'Get the name of a thread by its thread ID',
    {
      tid: z.string().describe('Thread ID (decimal)'),
    },
    async ({ tid }) => {
      const data = await httpClient.get('/api/threads/name', { tid });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
