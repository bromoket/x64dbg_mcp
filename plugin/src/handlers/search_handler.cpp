#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

void register_search_routes(c_http_router& router) {
    // POST /api/search/pattern - AOB/byte pattern scan
    router.post("/api/search/pattern", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("pattern")) {
            return s_http_response::bad_request("Missing 'pattern' field");
        }

        auto pattern = body["pattern"].get<std::string>();
        auto address_str = body.value("address", "0");
        auto size_str = body.value("size", "0");

        // Use x64dbg find command
        std::string cmd;
        if (address_str == "0" && size_str == "0") {
            // Search entire memory
            cmd = "findall 0, " + pattern;
        } else {
            cmd = "find " + address_str + ", " + pattern;
            if (size_str != "0") {
                cmd += ", " + size_str;
            }
        }

        bridge.exec_command(cmd);

        // Get the first result via $result
        auto result_addr = bridge.eval_expression("$result");

        nlohmann::json data = {
            {"pattern",      pattern},
            {"first_match",  result_addr != 0 ? format_utils::format_address(result_addr) : ""},
            {"found",        result_addr != 0}
        };

        if (result_addr != 0) {
            data["message"] = "Pattern found. Use 'findall' in x64dbg to see all matches in the references view.";
        }

        return s_http_response::ok(data);
    });

    // POST /api/search/string - String search
    router.post("/api/search/string", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("text")) {
            return s_http_response::bad_request("Missing 'text' field");
        }

        auto text = body["text"].get<std::string>();
        auto module_name = body.value("module", "");
        auto encoding = body.value("encoding", "utf8"); // utf8, ascii, unicode

        // Convert string to byte pattern
        std::string byte_pattern;
        if (encoding == "unicode" || encoding == "utf16") {
            // UTF-16LE encoding
            for (char c : text) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%02X 00 ", static_cast<unsigned char>(c));
                byte_pattern += buf;
            }
        } else {
            // ASCII / UTF-8
            for (char c : text) {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X ", static_cast<unsigned char>(c));
                byte_pattern += buf;
            }
        }

        // Trim trailing space
        if (!byte_pattern.empty() && byte_pattern.back() == ' ') {
            byte_pattern.pop_back();
        }

        std::string cmd;
        if (!module_name.empty()) {
            auto base = bridge.get_module_base(module_name);
            auto size = bridge.eval_expression("mod.size(" + module_name + ")");
            cmd = "find " + format_utils::format_address(base) + ", " + byte_pattern + ", " + format_utils::format_hex(size);
        } else {
            cmd = "findall 0, " + byte_pattern;
        }

        bridge.exec_command(cmd);
        auto result_addr = bridge.eval_expression("$result");

        return s_http_response::ok({
            {"text",        text},
            {"encoding",    encoding},
            {"pattern",     byte_pattern},
            {"first_match", result_addr != 0 ? format_utils::format_address(result_addr) : ""},
            {"found",       result_addr != 0}
        });
    });

    // GET /api/search/string_at?address= - Get string at address
    router.get("/api/search/string_at", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto address = bridge.eval_expression(address_str);
        char text[MAX_STRING_SIZE] = {};
        auto found = DbgGetStringAt(address, text);

        return s_http_response::ok({
            {"address", format_utils::format_address(address)},
            {"found",   found},
            {"text",    std::string(text)}
        });
    });

    // POST /api/search/auto_complete - Symbol auto-complete
    router.post("/api/search/auto_complete", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("search")) {
            return s_http_response::bad_request("Missing 'search' field");
        }

        auto search = body["search"].get<std::string>();
        auto max_results = body.value("max_results", 20);

        // Allocate buffer for results
        std::vector<char*> buffer(max_results, nullptr);
        auto count = DbgFunctions()->SymAutoComplete(search.c_str(), buffer.data(), max_results);

        auto results = nlohmann::json::array();
        for (int i = 0; i < count && i < max_results; ++i) {
            if (buffer[i]) {
                results.push_back(std::string(buffer[i]));
                BridgeFree(buffer[i]);
            }
        }

        return s_http_response::ok({
            {"search",  search},
            {"results", results},
            {"count",   results.size()}
        });
    });

    // GET /api/search/encode_type?address= - Get encode type at address
    router.get("/api/search/encode_type", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto size_str = req.get_query("size", "1");
        auto address = bridge.eval_expression(address_str);
        auto size = bridge.eval_expression(size_str);

        auto encode_type = DbgGetEncodeTypeAt(address, size);

        std::string type_str;
        switch (encode_type) {
            case enc_unknown: type_str = "unknown"; break;
            case enc_byte:    type_str = "byte"; break;
            case enc_word:    type_str = "word"; break;
            case enc_dword:   type_str = "dword"; break;
            case enc_fword:   type_str = "fword"; break;
            case enc_qword:   type_str = "qword"; break;
            case enc_tbyte:   type_str = "tbyte"; break;
            case enc_oword:   type_str = "oword"; break;
            case enc_mmword:  type_str = "mmword"; break;
            case enc_xmmword: type_str = "xmmword"; break;
            case enc_ymmword: type_str = "ymmword"; break;
            case enc_real4:   type_str = "real4"; break;
            case enc_real8:   type_str = "real8"; break;
            case enc_real10:  type_str = "real10"; break;
            case enc_ascii:   type_str = "ascii"; break;
            case enc_unicode: type_str = "unicode"; break;
            case enc_code:    type_str = "code"; break;
            case enc_junk:    type_str = "junk"; break;
            case enc_middle:  type_str = "middle"; break;
            default:          type_str = "unknown"; break;
        }

        return s_http_response::ok({
            {"address",     format_utils::format_address(address)},
            {"encode_type", type_str},
            {"type_id",     static_cast<int>(encode_type)}
        });
    });
}

} // namespace handlers
