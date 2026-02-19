import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerProcessTools(server: McpServer) {
  server.tool(
    'get_basic_process_info',
    'Get basic process info: PID, PEB address, entry point, and current debugger state',
    {},
    async () => {
      const data = await httpClient.get('/api/process/info');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_process_info',
    'Get detailed process information including PID, PEB address, handle, elevation status, and DEP state',
    {},
    async () => {
      const data = await httpClient.get('/api/process/details');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_cmdline',
    'Get the command line string of the debugged process',
    {},
    async () => {
      const data = await httpClient.get('/api/process/cmdline');
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

  server.tool(
    'is_process_elevated',
    'Check if the debugged process is running with elevated (administrator) privileges',
    {},
    async () => {
      const data = await httpClient.get('/api/process/elevated');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_debugger_version',
    'Get the x64dbg debugger bridge version number',
    {},
    async () => {
      const data = await httpClient.get('/api/process/dbversion');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
