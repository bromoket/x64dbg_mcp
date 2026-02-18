#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>

namespace handlers {

void register_memory_routes(c_http_router& router) {
    // GET /api/memory/read?address=0x...&size=N - Read memory bytes
    router.get("/api/memory/read", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        auto size_str = req.get_query("size", "256");

        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto size = static_cast<size_t>(std::stoull(size_str));

        auto result = bridge.read_memory(address, size);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        const auto& bytes = result.value();

        // Build ASCII representation
        std::string ascii;
        ascii.reserve(bytes.size());
        for (auto b : bytes) {
            ascii += (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"size",    bytes.size()},
            {"hex",     format_utils::format_bytes_hex(bytes.data(), bytes.size())},
            {"ascii",   ascii}
        });
    });

    // POST /api/memory/write - Write bytes to memory
    router.post("/api/memory/write", [](const s_http_request& req) -> s_http_response {
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
            return s_http_response::bad_request("No valid bytes to write");
        }

        auto result = bridge.write_memory(address, bytes);
        if (!result.has_value()) {
            return s_http_response::internal_error(result.error());
        }

        return s_http_response::ok({
            {"address",       format_utils::format_address(address)},
            {"bytes_written", bytes.size()}
        });
    });

    // GET /api/memory/is_valid?address=0x... - Check pointer validity
    router.get("/api/memory/is_valid", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto valid = bridge.is_valid_read_ptr(address);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"valid",   valid}
        });
    });

    // GET /api/memory/page_info?address=0x... - Memory page info
    router.get("/api/memory/page_info", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        duint region_size = 0;
        auto base = DbgMemFindBaseAddr(address, &region_size);

        if (base == 0) {
            return s_http_response::not_found("No memory region at " + address_str);
        }

        auto module_name = bridge.get_module_at(address);

        return s_http_response::ok({
            {"address",     format_utils::format_address(address)},
            {"base",        format_utils::format_address(base)},
            {"region_size", region_size},
            {"module",      module_name}
        });
    });

    // POST /api/memory/allocate - Allocate memory in target process
    router.post("/api/memory/allocate", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        auto size_str = body.value("size", "0x1000");

        auto cmd = "alloc " + size_str;
        bridge.exec_command(cmd);

        auto result = bridge.eval_expression("$result");
        if (result == 0) {
            return s_http_response::internal_error("Memory allocation failed");
        }

        return s_http_response::ok({
            {"address", format_utils::format_address(result)},
            {"size",    size_str}
        });
    });

    // POST /api/memory/free - Free memory in target process
    router.post("/api/memory/free", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address")) {
            return s_http_response::bad_request("Missing 'address' field");
        }

        auto address_str = body["address"].get<std::string>();
        auto cmd = "free " + address_str;
        bridge.exec_command(cmd);

        return s_http_response::ok({{"message", "Memory freed at " + address_str}});
    });

    // POST /api/memory/protect - Change page protection
    router.post("/api/memory/protect", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("address") || !body.contains("protection")) {
            return s_http_response::bad_request("Missing 'address' and/or 'protection' fields");
        }

        auto address_str = body["address"].get<std::string>();
        auto size_str = body.value("size", "0x1000");
        auto protection = body["protection"].get<std::string>();

        auto cmd = "VirtualProtect " + address_str + ", " + size_str + ", " + protection;
        bridge.exec_command(cmd);

        return s_http_response::ok({
            {"address",    address_str},
            {"size",       size_str},
            {"protection", protection}
        });
    });
}

} // namespace handlers
