#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"
#include "bridgelist.h"

namespace handlers {

void register_dumping_routes(c_http_router& router) {
    // POST /api/dump/module - Dump module to file
    router.post("/api/dump/module", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("module")) {
            return s_http_response::bad_request("Missing 'module' field");
        }

        auto module_name = body["module"].get<std::string>();
        auto file_path = body.value("file", "");

        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        // Use x64dbg's savedata command
        auto size = bridge.eval_expression("mod.size(" + module_name + ")");
        std::string cmd;
        if (!file_path.empty()) {
            cmd = "savedata " + file_path + ", " + format_utils::format_address(base) + ", " + format_utils::format_hex(size);
        } else {
            cmd = "savedata :memdump:, " + format_utils::format_address(base) + ", " + format_utils::format_hex(size);
        }

        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"module", module_name},
            {"base", format_utils::format_address(base)},
            {"size", size},
            {"file", file_path.empty() ? "(prompted)" : file_path}
        });
    });

    // GET /api/dump/pe_header?address= - Parse PE header from memory
    router.get("/api/dump/pe_header", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address", "");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto base = bridge.eval_expression(address_str);

        // Read DOS header (first 64 bytes)
        auto dos_header = bridge.read_memory(base, 64);
        if (!dos_header.has_value()) {
            return s_http_response::internal_error("Failed to read DOS header");
        }

        const auto& dos = dos_header.value();
        if (dos.size() < 64 || dos[0] != 'M' || dos[1] != 'Z') {
            return s_http_response::bad_request("Not a valid PE file (no MZ signature)");
        }

        // e_lfanew at offset 0x3C
        DWORD e_lfanew = 0;
        memcpy(&e_lfanew, dos.data() + 0x3C, 4);

        // Read PE signature + headers (enough for COFF + optional header)
        auto pe_header = bridge.read_memory(base + e_lfanew, 264);
        if (!pe_header.has_value()) {
            return s_http_response::internal_error("Failed to read PE header");
        }

        const auto& pe = pe_header.value();
        if (pe.size() < 4 || pe[0] != 'P' || pe[1] != 'E') {
            return s_http_response::bad_request("Invalid PE signature");
        }

        // COFF header starts at offset 4 in PE signature block
        WORD machine = 0;
        memcpy(&machine, pe.data() + 4, 2);

        WORD num_sections = 0;
        memcpy(&num_sections, pe.data() + 6, 2);

        DWORD timestamp = 0;
        memcpy(&timestamp, pe.data() + 8, 4);

        WORD size_of_optional = 0;
        memcpy(&size_of_optional, pe.data() + 20, 2);

        WORD characteristics = 0;
        memcpy(&characteristics, pe.data() + 22, 2);

        nlohmann::json data = {
            {"base",              format_utils::format_address(base)},
            {"e_lfanew",          format_utils::format_address(e_lfanew)},
            {"machine",           format_utils::format_address(machine)},
            {"number_of_sections", num_sections},
            {"timestamp",         timestamp},
            {"characteristics",   format_utils::format_address(characteristics)},
            {"size_of_optional_header", size_of_optional}
        };

        // Optional header starts at offset 24
        if (pe.size() >= 28) {
            WORD magic = 0;
            memcpy(&magic, pe.data() + 24, 2);
            data["magic"] = format_utils::format_address(magic);
            data["is_pe32plus"] = (magic == 0x20B);

            if (magic == 0x10B && pe.size() >= 64) {
                // PE32
                DWORD entry_point = 0;
                memcpy(&entry_point, pe.data() + 40, 4);
                data["address_of_entry_point"] = format_utils::format_address(entry_point);

                DWORD image_base_32 = 0;
                memcpy(&image_base_32, pe.data() + 52, 4);
                data["image_base"] = format_utils::format_address(image_base_32);

                DWORD size_of_image = 0;
                memcpy(&size_of_image, pe.data() + 80, 4);
                data["size_of_image"] = size_of_image;
            } else if (magic == 0x20B && pe.size() >= 88) {
                // PE32+
                DWORD entry_point = 0;
                memcpy(&entry_point, pe.data() + 40, 4);
                data["address_of_entry_point"] = format_utils::format_address(entry_point);

                uint64_t image_base_64 = 0;
                memcpy(&image_base_64, pe.data() + 48, 8);
                data["image_base"] = format_utils::format_address(static_cast<duint>(image_base_64));

                DWORD size_of_image = 0;
                memcpy(&size_of_image, pe.data() + 80, 4);
                data["size_of_image"] = size_of_image;
            }
        }

        return s_http_response::ok(data);
    });

    // GET /api/dump/sections?module= - Get PE sections
    router.get("/api/dump/sections", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module_name = req.get_query("module", "");
        if (module_name.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        // Read DOS header to get e_lfanew
        auto dos = bridge.read_memory(base, 64);
        if (!dos.has_value() || dos.value().size() < 64) {
            return s_http_response::internal_error("Failed to read DOS header");
        }

        DWORD e_lfanew = 0;
        memcpy(&e_lfanew, dos.value().data() + 0x3C, 4);

        // Read PE header to get number of sections and optional header size
        auto pe = bridge.read_memory(base + e_lfanew, 24);
        if (!pe.has_value() || pe.value().size() < 24) {
            return s_http_response::internal_error("Failed to read PE header");
        }

        WORD num_sections = 0;
        memcpy(&num_sections, pe.value().data() + 6, 2);

        WORD optional_size = 0;
        memcpy(&optional_size, pe.value().data() + 20, 2);

        // Section headers start after optional header
        auto section_offset = e_lfanew + 24 + optional_size;
        auto section_data = bridge.read_memory(base + section_offset, num_sections * 40); // IMAGE_SECTION_HEADER is 40 bytes
        if (!section_data.has_value()) {
            return s_http_response::internal_error("Failed to read section headers");
        }

        auto sections = nlohmann::json::array();
        for (WORD i = 0; i < num_sections; ++i) {
            auto* sec = section_data.value().data() + (i * 40);

            char name[9] = {};
            memcpy(name, sec, 8);

            DWORD virtual_size = 0, virtual_addr = 0, raw_size = 0, raw_ptr = 0, chars = 0;
            memcpy(&virtual_size, sec + 8, 4);
            memcpy(&virtual_addr, sec + 12, 4);
            memcpy(&raw_size, sec + 16, 4);
            memcpy(&raw_ptr, sec + 20, 4);
            memcpy(&chars, sec + 36, 4);

            sections.push_back({
                {"name",           std::string(name)},
                {"virtual_address", format_utils::format_address(virtual_addr)},
                {"virtual_size",   virtual_size},
                {"raw_size",       raw_size},
                {"raw_offset",     format_utils::format_address(raw_ptr)},
                {"characteristics", format_utils::format_address(chars)}
            });
        }

        return s_http_response::ok({
            {"module", module_name},
            {"base", format_utils::format_address(base)},
            {"sections", sections},
            {"count", sections.size()}
        });
    });

    // GET /api/dump/imports?module= - Get import table
    router.get("/api/dump/imports", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module_name = req.get_query("module", "");
        if (module_name.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        // Use x64dbg's modimports command to display in reference view
        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        auto cmd = "modimports " + format_utils::format_address(base);
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"module", module_name},
            {"base", format_utils::format_address(base)},
            {"message", "Import table displayed in x64dbg references view"}
        });
    });

    // GET /api/dump/exports?module= - Get export table
    router.get("/api/dump/exports", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module_name = req.get_query("module", "");
        if (module_name.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        auto cmd = "modexports " + format_utils::format_address(base);
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"module", module_name},
            {"base", format_utils::format_address(base)},
            {"message", "Export table displayed in x64dbg references view"}
        });
    });

    // POST /api/dump/fix_iat - IAT reconstruction (uses x64dbg Scylla)
    router.post("/api/dump/fix_iat", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto oep = body.value("oep", "");

        if (oep.empty()) {
            return s_http_response::bad_request("Missing 'oep' (original entry point) field");
        }

        // Use Scylla plugin commands
        auto cmd = "scylla iatAutoFix " + oep;
        auto success = bridge.exec_command(cmd);

        return s_http_response::ok({
            {"success", success},
            {"oep", oep},
            {"message", "IAT fix attempted via Scylla"}
        });
    });

    // GET /api/dump/relocations?address= - Get relocations for module
    router.get("/api/dump/relocations", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address", "");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);

        BridgeList<DBGRELOCATIONINFO> relocs;
        auto success = DbgFunctions()->ModRelocationsFromAddr(address, &relocs);

        if (!success) {
            return s_http_response::ok({
                {"address", format_utils::format_address(address)},
                {"relocations", nlohmann::json::array()},
                {"count", 0},
                {"message", "No relocations found or relocation data unavailable"}
            });
        }

        auto result = nlohmann::json::array();
        for (int i = 0; i < relocs.Count(); ++i) {
            result.push_back({
                {"rva",  format_utils::format_address(relocs[i].rva)},
                {"type", relocs[i].type},
                {"size", relocs[i].size}
            });
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"relocations", result},
            {"count", result.size()}
        });
    });

    // POST /api/patches/export_file - Export patches to file
    router.post("/api/patches/export_file", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("filename")) {
            return s_http_response::bad_request("Missing 'filename' field");
        }

        auto filename = body["filename"].get<std::string>();

        // Get all patches
        size_t patch_count = 0;
        DbgFunctions()->PatchEnum(nullptr, &patch_count);

        if (patch_count == 0) {
            return s_http_response::ok({
                {"success", false},
                {"message", "No patches to export"}
            });
        }

        std::vector<DBGPATCHINFO> patches(patch_count);
        DbgFunctions()->PatchEnum(patches.data(), &patch_count);

        char error[MAX_ERROR_SIZE] = {};
        auto result = DbgFunctions()->PatchFile(patches.data(), static_cast<int>(patch_count), filename.c_str(), error);

        return s_http_response::ok({
            {"success",     result > 0},
            {"patch_count", patch_count},
            {"filename",    filename},
            {"error",       std::string(error)}
        });
    });

    // GET /api/dump/entry_point?module= - Get entry point of module
    router.get("/api/dump/entry_point", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module_name = req.get_query("module", "");
        if (module_name.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module_name);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module_name);
        }

        auto entry = bridge.eval_expression("mod.entry(" + module_name + ")");

        return s_http_response::ok({
            {"module", module_name},
            {"base", format_utils::format_address(base)},
            {"entry_point", format_utils::format_address(entry)},
            {"rva", format_utils::format_address(entry - base)}
        });
    });
}

} // namespace handlers
