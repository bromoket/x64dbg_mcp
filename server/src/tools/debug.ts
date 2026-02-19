import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDebugTools(server: McpServer) {
  server.tool(
    'get_health',
    'Check if the x64dbg MCP plugin is running and responsive. Returns version, plugin name, and status.',
    {},
    async () => {
      const data = await httpClient.get('/api/health');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'get_debug_state',
    'Get the current debugger state (stopped/running/paused), CIP, and module info',
    {},
    async () => {
      const data = await httpClient.get('/api/debug/state');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'run',
    'Resume execution of the debugged process',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/run');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'pause',
    'Pause execution of the debugged process',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/pause');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'step_into',
    'Execute a single instruction, stepping into calls',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/step_into');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'step_over',
    'Execute a single instruction, stepping over calls',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/step_over');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'step_out',
    'Run until the current function returns',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/step_out');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'stop_debug',
    'Stop the current debug session',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/stop');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'restart_debug',
    'Restart the debugged process from the beginning',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/restart');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'run_to_address',
    'Run execution until a specific address is reached',
    { address: z.string().describe('Target address to run to (hex string, e.g. "0x401000")') },
    async ({ address }) => {
      const data = await httpClient.post('/api/debug/run_to', { address });
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );

  server.tool(
    'force_pause',
    'Force the debuggee to pause even when high-frequency fast-resume breakpoints are active. ' +
    'Normal pause often loses the race against fast-resume BPs firing at high frequency (e.g. 46/sec). ' +
    'This temporarily disables fast-resume on all breakpoints, sends pause, waits, then restores fast-resume.',
    {},
    async () => {
      const data = await httpClient.post('/api/debug/force_pause');
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
