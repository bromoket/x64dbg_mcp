#!/usr/bin/env node
import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { config } from './config.js';
import { httpClient } from './http_client.js';
import { registerAllTools } from './tools/index.js';

async function main() {
  const server = new McpServer({
    name: 'x64dbg',
    version: '1.0.0',
  });

  // Register all tools (~78 consolidated down from 152)
  registerAllTools(server);

  // Connect via stdio (rock-solid transport, no connection drops)
  const transport = new StdioServerTransport();
  await server.connect(transport);

  // Log to stderr (stdout is used by MCP protocol)
  console.error(`[x64dbg-mcp] Server started (~78 tools), plugin expected at ${config.host}:${config.port}`);
  console.error(`[x64dbg-mcp] Timeout: ${config.timeout}ms, Retries: ${config.retries}`);

  // Graceful shutdown
  const cleanup = () => {
    httpClient.destroy();
    process.exit(0);
  };
  process.on('SIGINT', cleanup);
  process.on('SIGTERM', cleanup);
}

main().catch((err) => {
  console.error('[x64dbg-mcp] Fatal error:', err);
  process.exit(1);
});
