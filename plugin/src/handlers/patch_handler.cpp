#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_patch_routes(c_http_router& router) {
    // GET /api/patches/list - List current patches
    router.get("/api/patches/list", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        // Use patchlist command to show patches
        bridge.exec_command("patchlist");

        return s_http_response::ok({
            {"message", "Patch list displayed in x64dbg log. Use the patches dialog for full patch management."}
        });
    });

    // POST /api/patches/apply - Apply byte patch
    router.post("/api/patches/apply", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("bytes")) {
            return s_http_response::bad_request("Missing 'address' and/or 'bytes' fields");
        }

        auto address = bridge.eval_expression(body["address"].get<std::string>());
        auto hex_str = body["bytes"].get<std::string>();
        auto bytes = format_utils::parse_hex_bytes(hex_str);

        if (bytes.empty()) {
            return s_http_response::bad_request("No valid bytes to patch");
        }

        // Read original bytes first
        auto original = bridge.read_memory(address, bytes.size());

        // Write the patch
        auto result = bridge.write_memory(address, bytes);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        nlohmann::json data = {
            {"address",       format_utils::format_address(address)},
            {"bytes_patched", bytes.size()},
            {"new_bytes",     format_utils::format_bytes_hex(bytes.data(), bytes.size())}
        };

        if (original.has_value()) {
            data["original_bytes"] = format_utils::format_bytes_hex(original->data(), original->size());
        }

        return s_http_response::ok(data);
    });

    // POST /api/patches/restore - Restore original bytes
    router.post("/api/patches/restore", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();

        // Use x64dbg's patch restore command
        auto cmd = "patchrestore " + address_str;
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address", address_str},
            {"message", "Patch restore requested"}
        });
    });

    // POST /api/patches/export - Export patched module
    router.post("/api/patches/export", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto module_name = body.value("module", "");
        auto output_path = body.value("path", "");

        if (output_path.empty()) {
            return s_http_response::bad_request("Missing 'path' field for output file");
        }

        auto cmd = "savedata \"" + output_path + "\"";
        if (!module_name.empty()) {
            auto base = bridge.get_module_base(module_name);
            auto size = bridge.eval_expression("mod.size(" + module_name + ")");
            cmd = "savedata \"" + output_path + "\", " + format_utils::format_address(base)
                + ", " + format_utils::format_hex(size);
        }

        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"module", module_name},
            {"path",   output_path},
            {"message", "Module export initiated"}
        });
    });
}

} // namespace handlers
