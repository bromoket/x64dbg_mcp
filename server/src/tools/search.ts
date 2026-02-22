import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerSearchTools(server: McpServer) {
  server.tool(
    'x64dbg_search',
    'Pattern/string search, symbol autocomplete, or get string at address',
    {
      action: z.discriminatedUnion("action", [
        z.object({
          action: z.literal("pattern"),
          query: z.string().describe("Byte pattern (e.g. '48 89 5C ??')"),
          address: z.string().optional().describe("Start address"),
          size: z.string().optional().describe("Size of range"),
          max_results: z.number().optional().default(1000)
        }),
        z.object({
          action: z.literal("string"),
          query: z.string().describe("Text string to search for"),
          module: z.string().optional().default(""),
          encoding: z.enum(['utf8', 'ascii', 'unicode']).optional().default("utf8")
        }),
        z.object({
          action: z.literal("string_at"),
          query: z.string().describe("Address"),
          encoding: z.enum(['auto', 'ascii', 'unicode']).optional().default("auto"),
          max_length: z.number().optional().default(256)
        }),
        z.object({
          action: z.literal("symbol_auto_complete"),
          query: z.string().describe("Partial symbol name"),
          max_results: z.number().optional().default(256)
        }),
        z.object({
          action: z.literal("encode_type"),
          query: z.string().describe("Address"),
          size: z.string().optional().default("1")
        })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'pattern':
          const body: Record<string, unknown> = { pattern: action.query, max_results: action.max_results };
          if (action.address) body.address = action.address;
          if (action.size) body.size = action.size;
          data = await httpClient.post('/api/search/pattern', body);
          break;
        case 'string':
          data = await httpClient.post('/api/search/string', { text: action.query, module: action.module, encoding: action.encoding });
          break;
        case 'string_at':
          data = await httpClient.get('/api/search/string_at', { address: action.query, encoding: action.encoding, max_length: String(action.max_length) });
          break;
        case 'symbol_auto_complete':
          data = await httpClient.post('/api/search/auto_complete', { search: action.query, max_results: action.max_results });
          break;
        case 'encode_type':
          data = await httpClient.get('/api/search/encode_type', { address: action.query, size: action.size });
          break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
