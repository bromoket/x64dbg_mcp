#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"

namespace handlers {

void register_register_routes(c_http_router& router) {
    // GET /api/registers/all - All register values
    router.get("/api/registers/all", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto dump_result = bridge.get_register_dump();
        if (!dump_result.has_value()) {
            return s_http_response::internal_error(dump_result.error());
        }

        const auto& ctx = dump_result->regcontext;
        nlohmann::json regs;

#ifdef _WIN64
        regs["rax"] = format_utils::format_address(ctx.cax);
        regs["rcx"] = format_utils::format_address(ctx.ccx);
        regs["rdx"] = format_utils::format_address(ctx.cdx);
        regs["rbx"] = format_utils::format_address(ctx.cbx);
        regs["rsp"] = format_utils::format_address(ctx.csp);
        regs["rbp"] = format_utils::format_address(ctx.cbp);
        regs["rsi"] = format_utils::format_address(ctx.csi);
        regs["rdi"] = format_utils::format_address(ctx.cdi);
        regs["r8"]  = format_utils::format_address(ctx.r8);
        regs["r9"]  = format_utils::format_address(ctx.r9);
        regs["r10"] = format_utils::format_address(ctx.r10);
        regs["r11"] = format_utils::format_address(ctx.r11);
        regs["r12"] = format_utils::format_address(ctx.r12);
        regs["r13"] = format_utils::format_address(ctx.r13);
        regs["r14"] = format_utils::format_address(ctx.r14);
        regs["r15"] = format_utils::format_address(ctx.r15);
        regs["rip"] = format_utils::format_address(ctx.cip);
#else
        regs["eax"] = format_utils::format_address(ctx.cax);
        regs["ecx"] = format_utils::format_address(ctx.ccx);
        regs["edx"] = format_utils::format_address(ctx.cdx);
        regs["ebx"] = format_utils::format_address(ctx.cbx);
        regs["esp"] = format_utils::format_address(ctx.csp);
        regs["ebp"] = format_utils::format_address(ctx.cbp);
        regs["esi"] = format_utils::format_address(ctx.csi);
        regs["edi"] = format_utils::format_address(ctx.cdi);
        regs["eip"] = format_utils::format_address(ctx.cip);
#endif

        regs["eflags"] = format_utils::format_address(ctx.eflags);

        // Segment registers
        regs["cs"] = ctx.cs;
        regs["ds"] = ctx.ds;
        regs["es"] = ctx.es;
        regs["fs"] = ctx.fs;
        regs["gs"] = ctx.gs;
        regs["ss"] = ctx.ss;

        // Debug registers
        regs["dr0"] = format_utils::format_address(ctx.dr0);
        regs["dr1"] = format_utils::format_address(ctx.dr1);
        regs["dr2"] = format_utils::format_address(ctx.dr2);
        regs["dr3"] = format_utils::format_address(ctx.dr3);
        regs["dr6"] = format_utils::format_address(ctx.dr6);
        regs["dr7"] = format_utils::format_address(ctx.dr7);

        return s_http_response::ok(regs);
    });

    // GET /api/registers/get?name=rax - Single register
    router.get("/api/registers/get", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto name = req.get_query("name");
        if (name.empty()) {
            return s_http_response::bad_request("Missing 'name' query parameter");
        }

        if (!bridge.is_valid_expression(name)) {
            return s_http_response::bad_request("Invalid register name: " + name);
        }

        auto value = bridge.eval_expression(name);
        return s_http_response::ok({
            {"name",  name},
            {"value", format_utils::format_address(value)}
        });
    });

    // POST /api/registers/set - Set register value
    router.post("/api/registers/set", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("name") || !body.contains("value")) {
            return s_http_response::bad_request("Missing 'name' and/or 'value' fields");
        }

        auto name = body["name"].get<std::string>();
        auto value = body["value"].get<std::string>();

        auto cmd = "mov " + name + ", " + value;
        if (!bridge.exec_command(cmd)) {
            return s_http_response::internal_error("Failed to set register " + name);
        }

        auto new_value = bridge.eval_expression(name);
        return s_http_response::ok({
            {"name",  name},
            {"value", format_utils::format_address(new_value)}
        });
    });

    // GET /api/registers/flags - EFLAGS decoded
    router.get("/api/registers/flags", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        auto dump_result = bridge.get_register_dump();
        if (!dump_result.has_value()) {
            return s_http_response::internal_error(dump_result.error());
        }

        const auto& flags = dump_result->flags;
        return s_http_response::ok({
            {"CF", flags.c},
            {"PF", flags.p},
            {"AF", flags.a},
            {"ZF", flags.z},
            {"SF", flags.s},
            {"TF", flags.t},
            {"IF", flags.i},
            {"DF", flags.d},
            {"OF", flags.o},
            {"eflags", format_utils::format_address(dump_result->regcontext.eflags)}
        });
    });

    // GET /api/registers/avx512 - Get AVX-512 register dump
    router.get("/api/registers/avx512", [](const s_http_request&) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_paused()) {
            return s_http_response::conflict("Debugger must be paused");
        }

        REGDUMP_AVX512 avx512{};
        if (!DbgGetRegDumpEx(&avx512, sizeof(REGDUMP_AVX512))) {
            return s_http_response::internal_error("Failed to get AVX-512 register dump (may not be supported)");
        }

        // Return the standard REGDUMP portion plus AVX-512 indicator
        const auto& ctx = avx512.regcontext;
        nlohmann::json data;

#ifdef _WIN64
        data["rax"] = format_utils::format_address(ctx.cax);
        data["rcx"] = format_utils::format_address(ctx.ccx);
        data["rdx"] = format_utils::format_address(ctx.cdx);
        data["rbx"] = format_utils::format_address(ctx.cbx);
        data["rsp"] = format_utils::format_address(ctx.csp);
        data["rbp"] = format_utils::format_address(ctx.cbp);
        data["rsi"] = format_utils::format_address(ctx.csi);
        data["rdi"] = format_utils::format_address(ctx.cdi);
        data["rip"] = format_utils::format_address(ctx.cip);
#else
        data["eax"] = format_utils::format_address(ctx.cax);
        data["ecx"] = format_utils::format_address(ctx.ccx);
        data["edx"] = format_utils::format_address(ctx.cdx);
        data["ebx"] = format_utils::format_address(ctx.cbx);
        data["esp"] = format_utils::format_address(ctx.csp);
        data["ebp"] = format_utils::format_address(ctx.cbp);
        data["esi"] = format_utils::format_address(ctx.csi);
        data["edi"] = format_utils::format_address(ctx.cdi);
        data["eip"] = format_utils::format_address(ctx.cip);
#endif

        data["avx512_supported"] = true;
        data["eflags"] = format_utils::format_address(ctx.eflags);

        return s_http_response::ok(data);
    });
}

} // namespace handlers
