#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <sstream>
#include <iomanip>

#include "_plugin_types.h"

namespace format_utils {

    // Format a duint address as hex string with 0x prefix
    [[nodiscard]] inline std::string format_address(duint addr) {
#ifdef _WIN64
        char buf[32];
        snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(addr));
#else
        char buf[16];
        snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned int>(addr));
#endif
        return buf;
    }

    // Format a duint value as hex string without prefix
    [[nodiscard]] inline std::string format_hex(duint value) {
#ifdef _WIN64
        char buf[32];
        snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(value));
#else
        char buf[16];
        snprintf(buf, sizeof(buf), "%X", static_cast<unsigned int>(value));
#endif
        return buf;
    }

    // Format bytes as hex string (space-separated)
    [[nodiscard]] inline std::string format_bytes_hex(const uint8_t* data, size_t size) {
        std::ostringstream oss;
        for (size_t i = 0; i < size; ++i) {
            if (i > 0) oss << ' ';
            oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    // Format bytes as contiguous hex string (no spaces)
    [[nodiscard]] inline std::string format_bytes_compact(const uint8_t* data, size_t size) {
        std::ostringstream oss;
        for (size_t i = 0; i < size; ++i) {
            oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    // Parse hex string to bytes
    [[nodiscard]] inline std::vector<uint8_t> parse_hex_bytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        std::string clean;

        // Strip spaces, 0x prefix, commas
        for (size_t i = 0; i < hex.size(); ++i) {
            char c = hex[i];
            if (c == ' ' || c == ',' || c == '\t' || c == '\n') continue;
            if (c == '0' && i + 1 < hex.size() && (hex[i + 1] == 'x' || hex[i + 1] == 'X')) {
                ++i;
                continue;
            }
            clean += c;
        }

        for (size_t i = 0; i + 1 < clean.size(); i += 2) {
            auto byte_str = clean.substr(i, 2);
            bytes.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
        }

        return bytes;
    }

    // Parse address string (supports 0x prefix and plain hex)
    [[nodiscard]] inline duint parse_address(const std::string& addr_str) {
        if (addr_str.empty()) return 0;

        std::string clean = addr_str;
        if (clean.size() > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
            clean = clean.substr(2);
        }

#ifdef _WIN64
        return static_cast<duint>(std::stoull(clean, nullptr, 16));
#else
        return static_cast<duint>(std::stoul(clean, nullptr, 16));
#endif
    }

    // Format DWORD protection flags as readable string
    [[nodiscard]] inline std::string format_protection(DWORD protect) {
        std::string result;
        switch (protect & 0xFF) {
            case PAGE_NOACCESS:          result = "PAGE_NOACCESS"; break;
            case PAGE_READONLY:          result = "PAGE_READONLY"; break;
            case PAGE_READWRITE:         result = "PAGE_READWRITE"; break;
            case PAGE_WRITECOPY:         result = "PAGE_WRITECOPY"; break;
            case PAGE_EXECUTE:           result = "PAGE_EXECUTE"; break;
            case PAGE_EXECUTE_READ:      result = "PAGE_EXECUTE_READ"; break;
            case PAGE_EXECUTE_READWRITE: result = "PAGE_EXECUTE_READWRITE"; break;
            case PAGE_EXECUTE_WRITECOPY: result = "PAGE_EXECUTE_WRITECOPY"; break;
            default:                     result = "UNKNOWN"; break;
        }
        if (protect & PAGE_GUARD)   result += " | PAGE_GUARD";
        if (protect & PAGE_NOCACHE) result += " | PAGE_NOCACHE";
        return result;
    }

    // Format memory state
    [[nodiscard]] inline std::string format_mem_state(DWORD state) {
        switch (state) {
            case MEM_COMMIT:  return "MEM_COMMIT";
            case MEM_RESERVE: return "MEM_RESERVE";
            case MEM_FREE:    return "MEM_FREE";
            default:          return "UNKNOWN";
        }
    }

    // Format memory type
    [[nodiscard]] inline std::string format_mem_type(DWORD type) {
        switch (type) {
            case MEM_IMAGE:   return "MEM_IMAGE";
            case MEM_MAPPED:  return "MEM_MAPPED";
            case MEM_PRIVATE: return "MEM_PRIVATE";
            default:          return "UNKNOWN";
        }
    }

} // namespace format_utils
