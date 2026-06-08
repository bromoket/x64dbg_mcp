#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <nlohmann/json.hpp>
#include "bridgemain.h"

namespace handlers {

namespace {

struct sym_collect {
    nlohmann::json* arr = nullptr;
    std::string filter; // lowercase substring; empty = no filter
    size_t limit = 0;
};

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Non-capturing callback so it converts to the C function pointer CBSYMBOLENUM.
bool sym_enum_cb(const SYMBOLPTR* symbol, void* user) {
    auto* ctx = static_cast<sym_collect*>(user);
    if (ctx->arr->size() >= ctx->limit) {
        return false; // stop enumeration
    }

    SYMBOLINFOCPP info; // RAII: frees decorated/undecorated on scope exit
    DbgGetSymbolInfo(symbol, &info);

    std::string decorated = info.decoratedSymbol ? info.decoratedSymbol : "";
    std::string undecorated = info.undecoratedSymbol ? info.undecoratedSymbol : "";

    if (!ctx->filter.empty()) {
        auto hay = to_lower(decorated + " " + undecorated);
        if (hay.find(ctx->filter) == std::string::npos) {
            return true; // skip, keep going
        }
    }

    ctx->arr->push_back({
        {"address",     format_utils::format_address(info.addr)},
        {"decorated",   decorated},
        {"undecorated", undecorated},
        {"type",        static_cast<int>(info.type)},
        {"ordinal",     info.ordinal}
    });
    return true;
}

} // namespace

void register_symbol_routes(c_http_router& router) {
    // GET /api/symbols/resolve?name=... - Name to address
    router.get("/api/symbols/resolve", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto name = req.get_query("name");
        if (name.empty()) {
            return s_http_response::bad_request("Missing 'name' query parameter");
        }

        if (!bridge.is_valid_expression(name)) {
            return s_http_response::not_found("Cannot resolve: " + name);
        }

        auto address = bridge.eval_expression(name);
        if (address == 0) {
            return s_http_response::not_found("Symbol not found: " + name);
        }

        return s_http_response::ok({
            {"name",    name},
            {"address", format_utils::format_address(address)}
        });
    });

    // GET /api/symbols/at?address=0x... - Address to name
    router.get("/api/symbols/at", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        auto label = bridge.get_label_at(address);
        auto module_name = bridge.get_module_at(address);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"label",   label},
            {"module",  module_name}
        });
    });

    // GET /api/symbols/search?pattern=... - Search symbols by pattern
    router.get("/api/symbols/search", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto pattern = req.get_query("pattern");
        auto module = req.get_query("module");
        if (pattern.empty()) {
            return s_http_response::bad_request("Missing 'pattern' query parameter");
        }

        // Resolve the module to search: explicit module, else the main module.
        auto base = module.empty() ? bridge.eval_expression("mod.main()")
                                   : bridge.get_module_base(module);
        if (base == 0) {
            return s_http_response::not_found(
                module.empty() ? "No main module" : ("Module not found: " + module));
        }

        constexpr size_t kLimit = 1000;
        auto symbols = nlohmann::json::array();
        sym_collect ctx{&symbols, to_lower(pattern), kLimit};
        DbgSymbolEnum(base, sym_enum_cb, &ctx);

        return s_http_response::ok({
            {"pattern",   pattern},
            {"module",    module},
            {"base",      format_utils::format_address(base)},
            {"symbols",   symbols},
            {"count",     symbols.size()},
            {"truncated", symbols.size() >= kLimit}
        });
    });

    // GET /api/symbols/list?module=... - List module symbols
    router.get("/api/symbols/list", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto module = req.get_query("module");
        if (module.empty()) {
            return s_http_response::bad_request("Missing 'module' query parameter");
        }

        auto base = bridge.get_module_base(module);
        if (base == 0) {
            return s_http_response::not_found("Module not found: " + module);
        }

        // Make sure symbols are loaded, then enumerate them.
        bridge.exec_command("symload " + module);

        constexpr size_t kLimit = 5000;
        auto symbols = nlohmann::json::array();
        sym_collect ctx{&symbols, "", kLimit};
        DbgSymbolEnum(base, sym_enum_cb, &ctx);

        return s_http_response::ok({
            {"module",    module},
            {"base",      format_utils::format_address(base)},
            {"symbols",   symbols},
            {"count",     symbols.size()},
            {"truncated", symbols.size() >= kLimit}
        });
    });
}

} // namespace handlers
