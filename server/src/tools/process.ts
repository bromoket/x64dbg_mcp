import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerProcessTools(server: McpServer) {
  server.tool(
    'get_process_info',
    'Get information about the process, basic/detailed info, command line, elevation, or debugger db version',
    {
      action: z.enum(['basic', 'detailed', 'cmdline', 'elevated', 'dbversion']).describe('Information to query')
    },
    async ({ action }) => {
      let data: any;
      switch (action) {
        case 'basic': data = await httpClient.get('/api/process/info'); break;
        case 'detailed': data = await httpClient.get('/api/process/details'); break;
        case 'cmdline': data = await httpClient.get('/api/process/cmdline'); break;
        case 'elevated': data = await httpClient.get('/api/process/elevated'); break;
        case 'dbversion': data = await httpClient.get('/api/process/dbversion'); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'set_cmdline',
    'Set/modify the command line of the debugged process (takes effect on next restart)',
    {
      cmdline: z.string().describe('New command line string'),
    },
    async ({ cmdline }) => {
      const data = await httpClient.post('/api/process/set_cmdline', { cmdline });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
