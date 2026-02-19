export const config = {
  host: process.env.X64DBG_MCP_HOST ?? '127.0.0.1',
  port: parseInt(process.env.X64DBG_MCP_PORT ?? '27042', 10),
  timeout: parseInt(process.env.X64DBG_MCP_TIMEOUT ?? '30000', 10),
  retries: parseInt(process.env.X64DBG_MCP_RETRIES ?? '3', 10),
};

export function getBaseUrl(): string {
  return `http://${config.host}:${config.port}`;
}
