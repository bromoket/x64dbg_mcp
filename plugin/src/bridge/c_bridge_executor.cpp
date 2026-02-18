#include "bridge/c_bridge_executor.h"

#include <thread>
#include <chrono>

#include "bridgemain.h"
#include "_plugins.h"
#include "_dbgfunctions.h"
#include "util/format_utils.h"

// Global singleton instance
static c_bridge_executor g_bridge;

c_bridge_executor& get_bridge() {
    return g_bridge;
}

bool c_bridge_executor::is_debugging() const {
    return DbgIsDebugging();
}

bool c_bridge_executor::is_running() const {
    return DbgIsRunning();
}

std::string c_bridge_executor::get_state_string() const {
    if (!DbgIsDebugging()) return "stopped";
    if (DbgIsRunning())    return "running";
    return "paused";
}

bool c_bridge_executor::exec_command(const std::string& cmd) {
    return DbgCmdExecDirect(cmd.c_str());
}

bool c_bridge_executor::exec_command_and_wait(const std::string& cmd, int timeout_ms) {
    std::lock_guard lock(m_mutex);

    if (!DbgCmdExecDirect(cmd.c_str())) {
        return false;
    }

    return wait_for_pause(timeout_ms);
}

bool c_bridge_executor::wait_for_pause(int timeout_ms) {
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);

    // First, wait a tiny bit for the command to take effect
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    while (DbgIsRunning()) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            return false; // Timed out waiting for pause
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;
}

duint c_bridge_executor::eval_expression(const std::string& expression) {
    return DbgValFromString(expression.c_str());
}

bool c_bridge_executor::is_valid_expression(const std::string& expression) {
    return DbgIsValidExpression(expression.c_str());
}

std::expected<std::vector<uint8_t>, std::string> c_bridge_executor::read_memory(duint address, size_t size) {
    if (size == 0 || size > 10 * 1024 * 1024) { // 10MB max
        return std::unexpected("Invalid read size (must be 1 to 10MB)");
    }

    std::vector<uint8_t> buffer(size);
    if (!DbgMemRead(address, buffer.data(), static_cast<duint>(size))) {
        return std::unexpected("Failed to read memory at " + format_utils::format_address(address));
    }

    return buffer;
}

std::expected<void, std::string> c_bridge_executor::write_memory(duint address, const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::unexpected("No data to write");
    }

    if (!DbgMemWrite(address, data.data(), static_cast<duint>(data.size()))) {
        return std::unexpected("Failed to write memory at " + format_utils::format_address(address));
    }

    return {};
}

bool c_bridge_executor::is_valid_read_ptr(duint address) {
    return DbgMemIsValidReadPtr(address);
}

std::expected<REGDUMP, std::string> c_bridge_executor::get_register_dump() {
    REGDUMP dump{};
    // DbgGetRegDumpEx uses size parameter to distinguish REGDUMP vs REGDUMP_AVX512
    if (!DbgGetRegDumpEx(reinterpret_cast<REGDUMP_AVX512*>(&dump), sizeof(REGDUMP))) {
        return std::unexpected("Failed to get register dump");
    }
    return dump;
}

std::expected<nlohmann::json, std::string> c_bridge_executor::get_memory_map() {
    MEMMAP memmap{};
    if (!DbgMemMap(&memmap)) {
        return std::unexpected("Failed to get memory map");
    }

    auto result = nlohmann::json::array();
    for (int i = 0; i < memmap.count; ++i) {
        const auto& page = memmap.page[i];
        result.push_back({
            {"base",             format_utils::format_address(reinterpret_cast<duint>(page.mbi.BaseAddress))},
            {"allocation_base",  format_utils::format_address(reinterpret_cast<duint>(page.mbi.AllocationBase))},
            {"size",             static_cast<duint>(page.mbi.RegionSize)},
            {"size_hex",         format_utils::format_hex(static_cast<duint>(page.mbi.RegionSize))},
            {"state",            format_utils::format_mem_state(page.mbi.State)},
            {"protect",          format_utils::format_protection(page.mbi.Protect)},
            {"type",             format_utils::format_mem_type(page.mbi.Type)},
            {"info",             page.info}
        });
    }

    if (memmap.page) {
        BridgeFree(memmap.page);
    }

    return result;
}

std::expected<nlohmann::json, std::string> c_bridge_executor::get_breakpoint_list(BPXTYPE type) {
    BPMAP bpmap{};
    DbgGetBpList(type, &bpmap); // Returns count (int), 0 is valid (no breakpoints)

    auto result = nlohmann::json::array();
    for (int i = 0; i < bpmap.count; ++i) {
        const auto& bp = bpmap.bp[i];
        result.push_back({
            {"address",          format_utils::format_address(bp.addr)},
            {"enabled",          bp.enabled},
            {"active",           bp.active},
            {"singleshoot",      bp.singleshoot},
            {"name",             bp.name},
            {"module",           bp.mod},
            {"hit_count",        bp.hitCount},
            {"fast_resume",      bp.fastResume},
            {"silent",           bp.silent},
            {"break_condition",  bp.breakCondition},
            {"log_text",         bp.logText},
            {"log_condition",    bp.logCondition},
            {"command_text",     bp.commandText},
            {"command_condition", bp.commandCondition},
            {"type",             static_cast<int>(bp.type)}
        });
    }

    if (bpmap.bp) {
        BridgeFree(bpmap.bp);
    }

    return result;
}

std::expected<nlohmann::json, std::string> c_bridge_executor::get_thread_list() {
    THREADLIST thread_list{};
    DbgGetThreadList(&thread_list);

    auto result = nlohmann::json::object();
    auto threads = nlohmann::json::array();

    for (int i = 0; i < thread_list.count; ++i) {
        const auto& t = thread_list.list[i];
        threads.push_back({
            {"number",        t.BasicInfo.ThreadNumber},
            {"id",            t.BasicInfo.ThreadId},
            {"handle",        format_utils::format_address(reinterpret_cast<duint>(t.BasicInfo.Handle))},
            {"start_address", format_utils::format_address(t.BasicInfo.ThreadStartAddress)},
            {"local_base",    format_utils::format_address(t.BasicInfo.ThreadLocalBase)},
            {"name",          t.BasicInfo.threadName},
            {"cip",           format_utils::format_address(t.ThreadCip)},
            {"suspend_count", t.SuspendCount},
            {"priority",      static_cast<int>(t.Priority)},
            {"last_error",    t.LastError}
        });
    }

    result["threads"] = threads;
    result["count"] = thread_list.count;
    result["current_thread"] = thread_list.CurrentThread;

    if (thread_list.list) {
        BridgeFree(thread_list.list);
    }

    return result;
}

std::string c_bridge_executor::get_label_at(duint address) {
    char label[MAX_LABEL_SIZE] = {};
    if (DbgGetLabelAt(address, SEG_DEFAULT, label)) {
        return label;
    }
    return "";
}

bool c_bridge_executor::set_label_at(duint address, const std::string& text) {
    return DbgSetLabelAt(address, text.c_str());
}

std::string c_bridge_executor::get_comment_at(duint address) {
    char comment[MAX_COMMENT_SIZE] = {};
    if (DbgGetCommentAt(address, comment)) {
        return comment;
    }
    return "";
}

bool c_bridge_executor::set_comment_at(duint address, const std::string& text) {
    return DbgSetCommentAt(address, text.c_str());
}

bool c_bridge_executor::set_bookmark_at(duint address, bool set) {
    return DbgSetBookmarkAt(address, set);
}

std::expected<nlohmann::json, std::string> c_bridge_executor::disassemble_at(duint address, int count) {
    auto instructions = nlohmann::json::array();
    auto current_addr = address;

    for (int i = 0; i < count; ++i) {
        DISASM_INSTR instr{};
        DbgDisasmAt(current_addr, &instr); // Returns void
        if (instr.instr_size == 0) break;

        // Get basic info for branch/call flags
        BASIC_INSTRUCTION_INFO basic{};
        DbgDisasmFastAt(current_addr, &basic);

        // Get label if any
        char label[MAX_LABEL_SIZE] = {};
        DbgGetLabelAt(current_addr, SEG_DEFAULT, label);

        // Get comment if any
        char comment[MAX_COMMENT_SIZE] = {};
        DbgGetCommentAt(current_addr, comment);

        instructions.push_back({
            {"address",     format_utils::format_address(current_addr)},
            {"instruction", instr.instruction},
            {"size",        instr.instr_size},
            {"type",        static_cast<int>(instr.type)},
            {"is_branch",   basic.branch},
            {"is_call",     basic.call},
            {"label",       label},
            {"comment",     comment}
        });

        current_addr += instr.instr_size;
    }

    return instructions;
}

std::expected<nlohmann::json, std::string> c_bridge_executor::get_basic_info(duint address) {
    BASIC_INSTRUCTION_INFO info{};
    DbgDisasmFastAt(address, &info); // Returns void

    if (info.size == 0) {
        return std::unexpected("Failed to get instruction info at " + format_utils::format_address(address));
    }

    return nlohmann::json{
        {"address",     format_utils::format_address(address)},
        {"size",        info.size},
        {"is_branch",   info.branch},
        {"is_call",     info.call},
        {"instruction", info.instruction}
    };
}

std::expected<nlohmann::json, std::string> c_bridge_executor::get_function_bounds(duint address) {
    duint start = 0, end = 0;
    if (!DbgFunctionGet(address, &start, &end)) {
        return std::unexpected("No function found at " + format_utils::format_address(address));
    }

    return nlohmann::json{
        {"start", format_utils::format_address(start)},
        {"end",   format_utils::format_address(end)},
        {"size",  end - start}
    };
}

duint c_bridge_executor::get_module_base(const std::string& name) {
    return DbgModBaseFromName(name.c_str());
}

std::string c_bridge_executor::get_module_at(duint address) {
    char mod_name[MAX_MODULE_SIZE] = {};
    if (DbgGetModuleAt(address, mod_name)) {
        return mod_name;
    }
    return "";
}

bool c_bridge_executor::require_paused() const {
    return is_debugging() && !is_running();
}

bool c_bridge_executor::require_debugging() const {
    return is_debugging();
}
