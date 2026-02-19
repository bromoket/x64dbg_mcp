#pragma once

#include <string>
#include <expected>
#include <cstdint>
#include <mutex>
#include <vector>

#include <nlohmann/json.hpp>
#include "_plugin_types.h"

// Thread-safe wrapper around x64dbg Bridge API calls.
// Most Bridge functions are already internally synchronized,
// but we add a mutex for compound operations (e.g., step + wait).
class c_bridge_executor {
public:
    // Debugger state check
    [[nodiscard]] bool is_debugging() const;
    [[nodiscard]] bool is_running() const;

    // Get the current debugger state as a string
    [[nodiscard]] std::string get_state_string() const;

    // Execute a command synchronously (return value often intentionally ignored)
    bool exec_command(const std::string& cmd);

    // Execute a command asynchronously (non-blocking, for traces/animations)
    bool exec_command_async(const std::string& cmd);

    // Execute a command and wait for the debugger to pause (with timeout)
    [[nodiscard]] bool exec_command_and_wait(const std::string& cmd, int timeout_ms = 5000);

    // Evaluate an expression (e.g., "rax", "module.entry", "0x401000+10")
    [[nodiscard]] duint eval_expression(const std::string& expression);

    // Check if an expression is valid
    [[nodiscard]] bool is_valid_expression(const std::string& expression);

    // Memory operations
    [[nodiscard]] std::expected<std::vector<uint8_t>, std::string> read_memory(duint address, size_t size);
    [[nodiscard]] std::expected<void, std::string> write_memory(duint address, const std::vector<uint8_t>& data);
    [[nodiscard]] bool is_valid_read_ptr(duint address);

    // Register dump
    [[nodiscard]] std::expected<REGDUMP, std::string> get_register_dump();

    // Memory map
    [[nodiscard]] std::expected<nlohmann::json, std::string> get_memory_map();

    // Breakpoint list
    [[nodiscard]] std::expected<nlohmann::json, std::string> get_breakpoint_list(BPXTYPE type);

    // Thread list
    [[nodiscard]] std::expected<nlohmann::json, std::string> get_thread_list();

    // Labels and comments
    [[nodiscard]] std::string get_label_at(duint address);
    [[nodiscard]] bool set_label_at(duint address, const std::string& text);
    [[nodiscard]] std::string get_comment_at(duint address);
    [[nodiscard]] bool set_comment_at(duint address, const std::string& text);
    [[nodiscard]] bool set_bookmark_at(duint address, bool set);

    // Disassembly
    [[nodiscard]] std::expected<nlohmann::json, std::string> disassemble_at(duint address, int count);
    [[nodiscard]] std::expected<nlohmann::json, std::string> get_basic_info(duint address);

    // Function analysis
    [[nodiscard]] std::expected<nlohmann::json, std::string> get_function_bounds(duint address);

    // Module info
    [[nodiscard]] duint get_module_base(const std::string& name);
    [[nodiscard]] std::string get_module_at(duint address);

    // Require paused state, return error response if not paused
    [[nodiscard]] bool require_paused() const;

    // Require debugging state, return error response if not debugging
    [[nodiscard]] bool require_debugging() const;

private:
    mutable std::mutex m_mutex;  // For compound operations only

    // Wait for debugger to reach paused state
    [[nodiscard]] bool wait_for_pause(int timeout_ms);
};

// Global singleton
c_bridge_executor& get_bridge();
