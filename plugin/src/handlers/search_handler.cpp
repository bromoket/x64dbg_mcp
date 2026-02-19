#include "http/c_http_router.h"
#include "bridge/c_bridge_executor.h"
#include "util/format_utils.h"

#include <nlohmann/json.hpp>
#include "bridgemain.h"
#include "_dbgfunctions.h"

namespace handlers {

// Parse a hex byte pattern string (e.g., "C4 CB 75 5B" or "C4CB755B" or "C4 ?? 75 5B")
// Returns pairs of (byte_value, is_wildcard).
// Returns empty vector if the pattern is malformed.
struct pattern_byte {
    uint8_t value = 0;
    bool    is_wildcard = false;
};

static std::vector<pattern_byte> parse_byte_pattern(const std::string& pattern_str) {
    // Strip all spaces to normalize
    std::string cleaned;
    cleaned.reserve(pattern_str.size());
    for (char c : pattern_str) {
        if (c != ' ') cleaned += c;
    }

    if (cleaned.empty() || (cleaned.size() % 2) != 0) {
        return {};
    }

    std::vector<pattern_byte> result;
    result.reserve(cleaned.size() / 2);

    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        char hi = cleaned[i];
        char lo = cleaned[i + 1];

        bool hi_wild = (hi == '?' || hi == '*');
        bool lo_wild = (lo == '?' || lo == '*');

        if (hi_wild || lo_wild) {
            result.push_back({0, true});
        } else {
            // Validate hex chars
            auto is_hex = [](char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
            };
            if (!is_hex(hi) || !is_hex(lo)) {
                return {};  // Invalid pattern
            }
            char hex[3] = {hi, lo, '\0'};
            result.push_back({static_cast<uint8_t>(std::stoul(hex, nullptr, 16)), false});
        }
    }

    return result;
}

// Scan a memory buffer for a byte pattern starting at any offset.
// Returns all offsets (relative to buffer start) where the pattern matches.
static std::vector<size_t> scan_buffer(
    const uint8_t* buf, size_t buf_size,
    const std::vector<pattern_byte>& pattern
) {
    std::vector<size_t> hits;
    if (pattern.empty() || buf_size < pattern.size()) return hits;

    const size_t pat_len = pattern.size();
    const size_t search_end = buf_size - pat_len + 1;

    for (size_t i = 0; i < search_end; ++i) {
        bool match = true;
        for (size_t j = 0; j < pat_len; ++j) {
            if (!pattern[j].is_wildcard && buf[i + j] != pattern[j].value) {
                match = false;
                break;
            }
        }
        if (match) {
            hits.push_back(i);
        }
    }

    return hits;
}

void register_search_routes(c_http_router& router) {
    // POST /api/search/pattern - AOB/byte pattern scan
    // Returns ALL matches (up to max_results). Supports wildcard bytes (??)
    router.post("/api/search/pattern", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded() || !body.contains("pattern")) {
            return s_http_response::bad_request("Missing 'pattern' field");
        }

        auto pattern_str = body["pattern"].get<std::string>();
        auto pattern = parse_byte_pattern(pattern_str);

        if (pattern.empty()) {
            return s_http_response::bad_request(
                "Invalid pattern '" + pattern_str + "'. Use hex bytes (e.g. 'C4 CB 75 5B' or 'C4CB755B'), wildcards as '?" "?'");
        }

        auto max_results = body.value("max_results", 1000);
        if (max_results < 1) max_results = 1;
        if (max_results > 10000) max_results = 10000;

        // Optional: restrict to a specific memory range
        std::string address_str = body.value("address", "");
        std::string size_str    = body.value("size", "");

        auto matches = nlohmann::json::array();

        if (!address_str.empty() && !size_str.empty()) {
            // Scan a specific range
            auto base = bridge.eval_expression(address_str);
            auto range_size = static_cast<size_t>(bridge.eval_expression(size_str));

            if (range_size == 0 || range_size > 256 * 1024 * 1024) {
                return s_http_response::bad_request("Invalid size (must be 1 byte - 256MB)");
            }

            auto mem = bridge.read_memory(base, range_size);
            if (mem.has_value()) {
                auto hits = scan_buffer(mem.value().data(), mem.value().size(), pattern);
                for (auto offset : hits) {
                    if (static_cast<int>(matches.size()) >= max_results) break;
                    auto match_addr = base + static_cast<duint>(offset);
                    matches.push_back(format_utils::format_address(match_addr));
                }
            }
        } else {
            // Scan all mapped memory pages (read + execute pages)
            MEMMAP memmap{};
            if (!DbgMemMap(&memmap)) {
                return s_http_response::internal_error("Failed to get memory map");
            }

            // Read each page and scan it (with overlap to catch cross-page matches)
            const size_t overlap = pattern.size() - 1;
            std::vector<uint8_t> prev_tail;

            for (int i = 0; i < memmap.count && static_cast<int>(matches.size()) < max_results; ++i) {
                const auto& page = memmap.page[i];
                auto page_base = reinterpret_cast<duint>(page.mbi.BaseAddress);
                auto page_size = static_cast<size_t>(page.mbi.RegionSize);

                // Skip non-committed or non-readable pages
                if (page.mbi.State != MEM_COMMIT) {
                    prev_tail.clear();
                    continue;
                }
                if (page.mbi.Protect == PAGE_NOACCESS || page.mbi.Protect == 0) {
                    prev_tail.clear();
                    continue;
                }

                // Read the page (limit single reads to 64MB)
                const size_t read_size = (page_size > 64 * 1024 * 1024) ? 64 * 1024 * 1024 : page_size;
                auto mem = bridge.read_memory(page_base, read_size);
                if (!mem.has_value()) {
                    prev_tail.clear();
                    continue;
                }

                const auto& buf = mem.value();

                // Build combined buffer: overlap from previous page + current page
                // This catches patterns that straddle page boundaries
                std::vector<uint8_t> combined;
                if (!prev_tail.empty()) {
                    combined.reserve(prev_tail.size() + buf.size());
                    combined.insert(combined.end(), prev_tail.begin(), prev_tail.end());
                    combined.insert(combined.end(), buf.begin(), buf.end());
                    auto base_addr = page_base - static_cast<duint>(prev_tail.size());

                    auto hits = scan_buffer(combined.data(), combined.size(), pattern);
                    for (auto offset : hits) {
                        if (static_cast<int>(matches.size()) >= max_results) break;
                        matches.push_back(format_utils::format_address(base_addr + static_cast<duint>(offset)));
                    }
                } else {
                    auto hits = scan_buffer(buf.data(), buf.size(), pattern);
                    for (auto offset : hits) {
                        if (static_cast<int>(matches.size()) >= max_results) break;
                        matches.push_back(format_utils::format_address(page_base + static_cast<duint>(offset)));
                    }
                }

                // Save tail for next iteration (cross-page match detection)
                if (buf.size() >= overlap && overlap > 0) {
                    prev_tail.assign(buf.end() - static_cast<ptrdiff_t>(overlap), buf.end());
                } else {
                    prev_tail = buf;
                }
            }

            if (memmap.page) {
                BridgeFree(memmap.page);
            }
        }

        bool found = !matches.empty();
        nlohmann::json data = {
            {"pattern",      pattern_str},
            {"found",        found},
            {"count",        matches.size()},
            {"matches",      matches},
        };

        // Backwards-compat: first_match field
        if (found) {
            data["first_match"] = matches[0];
        } else {
            data["first_match"] = "";
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

        auto pattern = parse_byte_pattern(byte_pattern);

        // Determine search range
        duint base = 0;
        size_t range = 0;

        if (!module_name.empty()) {
            base = bridge.get_module_base(module_name);
            if (base == 0) {
                return s_http_response::not_found("Module not found: " + module_name);
            }
            range = static_cast<size_t>(bridge.eval_expression("mod.size(" + module_name + ")"));
        }

        auto matches = nlohmann::json::array();

        if (base != 0 && range != 0) {
            auto mem = bridge.read_memory(base, range);
            if (mem.has_value()) {
                auto hits = scan_buffer(mem.value().data(), mem.value().size(), pattern);
                for (auto offset : hits) {
                    if (static_cast<int>(matches.size()) >= 1000) break;
                    matches.push_back(format_utils::format_address(base + static_cast<duint>(offset)));
                }
            }
        } else {
            // Full memory scan via pattern endpoint logic (simplified: use findall command)
            std::string cmd = "findall 0, " + byte_pattern;
            bridge.exec_command(cmd);
            auto result_addr = bridge.eval_expression("$result");
            if (result_addr != 0) {
                matches.push_back(format_utils::format_address(result_addr));
            }
        }

        return s_http_response::ok({
            {"text",        text},
            {"encoding",    encoding},
            {"pattern",     byte_pattern},
            {"found",       !matches.empty()},
            {"count",       matches.size()},
            {"matches",     matches},
            {"first_match", !matches.empty() ? matches[0] : ""}
        });
    });

    // GET /api/search/string_at?address=&encoding=auto&max_length=256 - Get string at address
    // encoding: auto (default), ascii, unicode
    router.get("/api/search/string_at", [](const s_http_request& req) -> s_http_response {
        auto& bridge = get_bridge();
        if (!bridge.require_debugging()) {
            return s_http_response::conflict("No active debug session");
        }

        auto address_str = req.get_query("address");
        if (address_str.empty()) {
            return s_http_response::bad_request("Missing 'address' query parameter");
        }

        auto encoding = req.get_query("encoding", "auto");
        auto max_length_str = req.get_query("max_length", "256");
        auto max_length = static_cast<size_t>(std::stoull(max_length_str));
        if (max_length < 1) max_length = 1;
        if (max_length > 4096) max_length = 4096;

        auto address = bridge.eval_expression(address_str);

        nlohmann::json data = {
            {"address",  format_utils::format_address(address)},
            {"encoding", encoding}
        };

        // Always read raw bytes for transparency
        auto raw_mem = bridge.read_memory(address, max_length);
        if (raw_mem.has_value()) {
            const auto& raw = raw_mem.value();
            data["raw_hex"] = format_utils::format_bytes_hex(raw.data(), raw.size());

            if (encoding == "unicode" || encoding == "utf16") {
                // Read as UTF-16LE
                std::string utf16_str;
                for (size_t i = 0; i + 1 < raw.size(); i += 2) {
                    uint16_t wc = raw[i] | (static_cast<uint16_t>(raw[i + 1]) << 8);
                    if (wc == 0) break;
                    // Simple ASCII-range conversion
                    if (wc < 0x80) {
                        utf16_str += static_cast<char>(wc);
                    } else {
                        utf16_str += '?';  // Non-ASCII wide char
                    }
                }
                data["text"] = utf16_str;
                data["found"] = !utf16_str.empty();
            } else if (encoding == "ascii") {
                // Read as null-terminated ASCII
                std::string ascii_str;
                for (size_t i = 0; i < raw.size(); ++i) {
                    if (raw[i] == 0) break;
                    ascii_str += static_cast<char>(raw[i]);
                }
                data["text"] = ascii_str;
                data["found"] = !ascii_str.empty();
            } else {
                // Auto-detect: use DbgGetStringAt first, then cross-check
                char dbg_text[MAX_STRING_SIZE] = {};
                auto found = DbgGetStringAt(address, dbg_text);
                data["text"] = std::string(dbg_text);
                data["found"] = found;

                // Also attempt raw ASCII read for comparison
                std::string raw_ascii;
                for (size_t i = 0; i < raw.size(); ++i) {
                    if (raw[i] == 0) break;
                    uint8_t b = raw[i];
                    if (b >= 0x20 && b < 0x7F) {
                        raw_ascii += static_cast<char>(b);
                    } else {
                        break;  // Stop at non-printable
                    }
                }
                if (!raw_ascii.empty()) {
                    data["raw_ascii"] = raw_ascii;
                }
            }
        } else {
            // Fallback to DbgGetStringAt only
            char text[MAX_STRING_SIZE] = {};
            auto found = DbgGetStringAt(address, text);
            data["found"] = found;
            data["text"]  = std::string(text);
        }

        return s_http_response::ok(data);
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
