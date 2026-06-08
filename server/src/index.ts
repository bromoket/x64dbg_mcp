#!/usr/bin/env node
import { readFileSync } from 'node:fs';
import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { config } from './config.js';
import { httpClient } from './http_client.js';
import { registerAllTools } from './tools/index.js';

// Single source of truth for the version: package.json (next to dist/ in the
// published package), so the reported version never drifts from the release.
function getVersion(): string {
  try {
    const pkgUrl = new URL('../package.json', import.meta.url);
    const pkg = JSON.parse(readFileSync(pkgUrl, 'utf8')) as { version?: string };
    return pkg.version ?? '0.0.0';
  } catch {
    return '0.0.0';
  }
}

async function main() {
  const server = new McpServer({
    name: 'x64dbg',
    version: getVersion(),
  });

  // Register all tools
  registerAllTools(server);

  // Connect via stdio (rock-solid transport, no connection drops)
  const transport = new StdioServerTransport();
  await server.connect(transport);

  // Log to stderr (stdout is used by MCP protocol)
  console.error(`[x64dbg-mcp] Server started (23 tools), plugin expected at ${config.host}:${config.port}`);
  const timeout_desc = config.timeout > 0 ? `${config.timeout}ms` : 'none (waits indefinitely)';
  console.error(`[x64dbg-mcp] Timeout: ${timeout_desc}, Retries: ${config.retries}`);

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
