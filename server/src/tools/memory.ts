import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerMemoryTools(server: McpServer) {
  server.tool(
    'x64dbg_memory',
    'Core memory operations: read, write, info, allocate, free, protect, map',
    {
      action: z.discriminatedUnion("action", [
        z.object({
          action: z.literal("read"),
          address: z.string().describe("Hex string or expression"),
          size: z.string().optional().default("256").describe("Size in bytes (decimal)")
        }),
        z.object({
          action: z.literal("write"),
          address: z.string().describe("Hex string or expression"),
          bytes: z.string().describe("Hex bytes (e.g. '90 90' or 'CC')"),
          verify: z.boolean().optional().default(false).describe("Verify write success")
        }),
        z.object({ action: z.literal("info"), address: z.string().describe("Address to query page info") }),
        z.object({ action: z.literal("is_valid"), address: z.string() }),
        z.object({ action: z.literal("is_code"), address: z.string() }),
        z.object({
          action: z.literal("allocate"),
          size: z.string().optional().default("0x1000").describe("Size in hex")
        }),
        z.object({
          action: z.literal("free"),
          address: z.string()
        }),
        z.object({
          action: z.literal("protect"),
          address: z.string(),
          size: z.string().optional().default("0x1000"),
          protection: z.string().describe("E.g. 'PAGE_EXECUTE_READWRITE'")
        }),
        z.object({
          action: z.literal("map"),
          address: z.string().optional().describe("Return region info for address, or full map if omitted")
        }),
        z.object({ action: z.literal("update_map") })
      ])
    },
    async ({ action }) => {
      let endpoint = '';
      let payload: any = undefined;

      switch (action.action) {
        case 'read':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memory/read', { address: action.address, size: action.size }), null, 2) }] };
        case 'write':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.post('/api/memory/write', { address: action.address, bytes: action.bytes, verify: action.verify }), null, 2) }] };
        case 'info':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memory/page_info', { address: action.address }), null, 2) }] };
        case 'is_valid':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memory/is_valid', { address: action.address }), null, 2) }] };
        case 'is_code':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memory/is_code', { address: action.address }), null, 2) }] };
        case 'allocate':
          endpoint = '/api/memory/allocate';
          payload = { size: action.size };
          break;
        case 'free':
          endpoint = '/api/memory/free';
          payload = { address: action.address };
          break;
        case 'protect':
          endpoint = '/api/memory/protect';
          payload = { address: action.address, size: action.size, protection: action.protection };
          break;
        case 'map':
          if ((action as any).address) {
            return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memmap/at', { address: (action as any).address }), null, 2) }] };
          } else {
            return { content: [{ type: 'text', text: JSON.stringify(await httpClient.get('/api/memmap/list'), null, 2) }] };
          }
        case 'update_map':
          return { content: [{ type: 'text', text: JSON.stringify(await httpClient.post('/api/memory/update_map'), null, 2) }] };
      }

      if (endpoint) {
        const data = await httpClient.post(endpoint, payload);
        return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
      }
      return { content: [{ type: 'text', text: "{}" }] };
    }
  );
}
