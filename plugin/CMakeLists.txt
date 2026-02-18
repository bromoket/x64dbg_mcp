cmake_minimum_required(VERSION 3.20)
project(x64dbg_mcp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X64DBG_ARCH "x64")
    set(PLUGIN_EXT ".dp64")
    set(BRIDGE_LIB "x64bridge")
    set(DBG_LIB "x64dbg")
else()
    set(X64DBG_ARCH "x32")
    set(PLUGIN_EXT ".dp32")
    set(BRIDGE_LIB "x32bridge")
    set(DBG_LIB "x32dbg")
endif()

# vcpkg dependencies
find_package(nlohmann_json CONFIG REQUIRED)

# Source files
set(SOURCES
    src/plugin_main.cpp
    src/http/c_http_server.cpp
    src/http/c_http_router.cpp
    src/bridge/c_bridge_executor.cpp
    src/util/format_utils.cpp
    src/handlers/debug_handler.cpp
    src/handlers/register_handler.cpp
    src/handlers/memory_handler.cpp
    src/handlers/breakpoint_handler.cpp
    src/handlers/disasm_handler.cpp
    src/handlers/module_handler.cpp
    src/handlers/thread_handler.cpp
    src/handlers/stack_handler.cpp
    src/handlers/symbol_handler.cpp
    src/handlers/annotation_handler.cpp
    src/handlers/search_handler.cpp
    src/handlers/patch_handler.cpp
    src/handlers/memmap_handler.cpp
    src/handlers/command_handler.cpp
    src/handlers/analysis_handler.cpp
)

# Create shared library (plugin DLL)
add_library(${PROJECT_NAME} SHARED ${SOURCES} plugin.def)

# Include directories (SDK as SYSTEM to suppress its warnings)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk
)

# Determine jansson lib based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(JANSSON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/sdk/jansson/jansson_x64.lib")
else()
    set(JANSSON_LIB "${CMAKE_CURRENT_SOURCE_DIR}/sdk/jansson/jansson_x86.lib")
endif()

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/${BRIDGE_LIB}.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/sdk/${DBG_LIB}.lib
    ${JANSSON_LIB}
    nlohmann_json::nlohmann_json
    ws2_32
    dbghelp
)

# Set output name to match x64dbg plugin convention
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "x64dbg_mcp"
    SUFFIX ${PLUGIN_EXT}
    PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Compiler flags for clang-cl / MSVC
target_compile_options(${PROJECT_NAME} PRIVATE
    /W4
    /utf-8
    /EHsc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    BUILD_PLUGIN
)
