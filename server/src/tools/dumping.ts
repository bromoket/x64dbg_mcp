import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { httpClient } from '../http_client.js';

export function registerDumpingTools(server: McpServer) {
  server.tool(
    'x64dbg_dumping',
    'Dump modules, fix IAT, or get PE header/sections/imports/exports',
    {
      action: z.discriminatedUnion("action", [
        z.object({ action: z.literal("pe_header"), address: z.string() }),
        z.object({ action: z.literal("sections"), module: z.string() }),
        z.object({ action: z.literal("imports"), module: z.string() }),
        z.object({ action: z.literal("exports"), module: z.string() }),
        z.object({ action: z.literal("entry_point"), module: z.string() }),
        z.object({ action: z.literal("relocations"), address: z.string() }),
        z.object({ action: z.literal("dump_module"), module: z.string(), file: z.string().optional().default("") }),
        z.object({ action: z.literal("fix_iat"), oep: z.string() })
      ])
    },
    async ({ action }) => {
      let data: any;
      switch (action.action) {
        case 'pe_header': data = await httpClient.get('/api/dump/pe_header', { address: action.address }); break;
        case 'sections': data = await httpClient.get('/api/dump/sections', { module: action.module }); break;
        case 'imports': data = await httpClient.get('/api/dump/imports', { module: action.module }); break;
        case 'exports': data = await httpClient.get('/api/dump/exports', { module: action.module }); break;
        case 'entry_point': data = await httpClient.get('/api/dump/entry_point', { module: action.module }); break;
        case 'relocations': data = await httpClient.get('/api/dump/relocations', { address: action.address }); break;
        case 'dump_module': data = await httpClient.post('/api/dump/module', { module: action.module, file: action.file }); break;
        case 'fix_iat': data = await httpClient.post('/api/dump/fix_iat', { oep: action.oep }); break;
      }
      return { content: [{ type: 'text', text: JSON.stringify(data, null, 2) }] };
    }
  );
}
