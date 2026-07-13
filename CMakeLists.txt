cmake_minimum_required(VERSION 3.16)
project(SindriKit VERSION 1.0.1 LANGUAGES C ASM_MASM)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

option(SND_ENABLE_DEBUG   "Enable debug prints and tracking"             OFF)
option(SND_USE_PRINTF     "Use printf instead of OutputDebugString"      OFF)
option(SND_BUILD_PAYLOADS "Build bundled PoC loaders"                    OFF)
option(SND_BUILD_TESTS    "Build test binaries (DLLs and EXEs)"          OFF)
option(SND_CRTLESS "Build the framework for pure CRT-less operation"     OFF)
option(SND_RANDOMIZE_SEED "Use randomized seed for compile-time hashes"  OFF)
option(SND_USE_DEFAULTS "Use sensible defaults for configuration globals" OFF)

# --- Configuration Guards ---
if(SND_CRTLESS)
    if(SND_BUILD_TESTS)
        message(WARNING "SindriKit: SND_BUILD_TESTS requires the CRT. Forcing SND_CRTLESS=OFF. (If you chose the correct parameters, do a clean build and try again.)")
        set(SND_CRTLESS OFF CACHE BOOL "Build the framework for pure CRT-less operation" FORCE)
    else()
        if(SND_ENABLE_DEBUG OR SND_USE_PRINTF)
            message(WARNING "SindriKit: CRT-less build requested. Forcing SND_ENABLE_DEBUG=OFF and SND_USE_PRINTF=OFF. (If you chose the correct parameters, do a clean build and try again.)")
            set(SND_ENABLE_DEBUG OFF CACHE BOOL "Enable debug prints and tracking" FORCE)
            set(SND_USE_PRINTF OFF CACHE BOOL "Use printf instead of OutputDebugString" FORCE)
        endif()
    endif()
endif()

if(NOT WIN32)
    message(FATAL_ERROR "SindriKit supports Windows targets only.")
endif()

# --- Architecture-specific assembly ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SINDRI_EXEC_ASM
    src/primitives/execution/syscalls/asm/invoke_direct_x64.asm
    src/primitives/execution/syscalls/asm/invoke_indirect_x64.asm
    src/primitives/execution/syscalls/asm/invoke_spoofed_x64.asm
    src/primitives/execution/ffi/asm/ffi_x64.asm
)
    set(HEAVENSGATE_ASM "")
else()
    set(SINDRI_EXEC_ASM
    src/primitives/execution/syscalls/asm/invoke_direct_x86.asm
    src/primitives/execution/syscalls/asm/invoke_indirect_x86.asm
    src/primitives/execution/syscalls/asm/invoke_spoofed_x86.asm
    src/primitives/execution/ffi/asm/ffi_x86.asm
)
    set(HEAVENSGATE_ASM src/primitives/execution/heavens_gate/asm/heavens_gate_x86.asm)
endif()

# --- Compile-time hash generation ---
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(SND_HASH_ALGO "DJB2" CACHE STRING "Hashing algorithm (DJB2, FNV1A)")
string(TOUPPER "${SND_HASH_ALGO}" SND_HASH_ALGO)

set(HASH_MANIFEST    "${CMAKE_CURRENT_SOURCE_DIR}/config/hashes.ini")
set(HASH_SCRIPT      "${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_hashes.py")
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/sindri_hashes.h")

# Generate hashes at configure time.
# We use execute_process so it only runs during CMake configure.
# The Python script natively skips writing if the content hasn't changed, preserving timestamps.
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${HASH_SCRIPT} ${HASH_MANIFEST} ${GENERATED_HEADER} ${SND_HASH_ALGO} ${SND_RANDOMIZE_SEED}
    RESULT_VARIABLE HASH_RES
)
if(NOT HASH_RES EQUAL 0)
    message(FATAL_ERROR "SindriKit: Failed to generate API hashes.")
endif()

# Force CMake to reconfigure automatically if the manifest or script changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HASH_MANIFEST} ${HASH_SCRIPT})

# --- Static library ---
add_library(sindri_engine STATIC
    # Common
    src/common/status.c
    src/common/debug.c
    src/common/buffer.c
    src/common/hash.c
    src/common/disk.c
    # PE parser
    src/parsers/pe/parser.c
    src/parsers/pe/exports.c
    src/parsers/pe/imports.c
    src/parsers/pe/relocations.c
    src/parsers/pe/utils.c
    src/parsers/pe/section_utils.c
    # COFF parser
    src/parsers/coff/parser.c
    src/parsers/coff/utils.c
    src/parsers/coff/symbols.c
    src/parsers/coff/relocations.c
    # Environment parsers
    src/parsers/env/peb.c
    # Reflective loader
    src/loaders/pe/engine.c
    src/loaders/pe/chain.c
    # COFF loader
    src/loaders/coff/engine.c
    src/loaders/coff/chain.c
    # Injection domain
    src/injection/context.c
    src/injection/classic/engine.c
    src/injection/classic/chain.c
    # Memory primitives
    src/primitives/memory/win.c
    src/primitives/memory/sys.c
    src/primitives/memory/nt.c
    # Mapping primitives
    src/primitives/mapping/win.c
    src/primitives/mapping/sys.c
    src/primitives/mapping/nt.c
    # Module primitives
    src/primitives/modules/win.c
    src/primitives/modules/nt.c
    # Process primitives
    src/primitives/process/win.c
    src/primitives/process/sys.c
    src/primitives/process/nt.c
    # Object manager
    src/primitives/object_manager/knowndlls.c
    # Execution
    src/primitives/execution/ffi/ffi_invoke.c
    src/primitives/execution/heavens_gate/heavens_gate.c
    # Syscall resolvers
    src/primitives/execution/syscalls/syscalls.c
    src/primitives/execution/syscalls/syscalls_scan.c
    src/primitives/execution/syscalls/syscalls_sort.c
    src/primitives/execution/syscalls/gadget_scan.c
    src/primitives/execution/syscalls/spoof_scan.c

    # Assembly & generated
    ${SINDRI_EXEC_ASM}
    ${HEAVENSGATE_ASM}
    ${GENERATED_HEADER}
)

if(SND_CRTLESS)
    target_sources(sindri_engine PRIVATE src/common/crt_manifest.c)
endif()

add_library(sindri::engine ALIAS sindri_engine)

target_include_directories(sindri_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(sindri_engine PUBLIC c_std_99)

# --- Debug & Print Configuration ---
if(SND_USE_PRINTF)
    set(DEF_USE_PRINTF 1)
    set(PRINT_MODE "CONSOLE (printf)")
else()
    set(DEF_USE_PRINTF 0)
    set(PRINT_MODE "DEBUGGER (OutputDebugString)")
endif()

if(SND_ENABLE_DEBUG)
    set(DEF_DEBUG 1)
    message(STATUS "SindriKit: DEBUG | Output: ${PRINT_MODE} | Hash: ${SND_HASH_ALGO}")
else()
    set(DEF_DEBUG 0)
    message(STATUS "SindriKit: SILENT | Hash: ${SND_HASH_ALGO}")
endif()

target_compile_definitions(sindri_engine PUBLIC
    SND_DEBUG=${DEF_DEBUG}
    SND_USE_PRINTF=${DEF_USE_PRINTF}
    SND_HASH_${SND_HASH_ALGO}=1
)

if(SND_USE_DEFAULTS)
    target_compile_definitions(sindri_engine PUBLIC SND_USE_DEFAULTS=1)
endif()

# --- Compiler warnings ---
if(MSVC)
    target_compile_options(sindri_engine PRIVATE
        $<$<COMPILE_LANGUAGE:C>:/W4>
        $<$<COMPILE_LANGUAGE:C>:/WX>
    )
    if(NOT SND_ENABLE_DEBUG)
        target_compile_options(sindri_engine PRIVATE
            $<$<COMPILE_LANGUAGE:C>:/GS->
            $<$<COMPILE_LANGUAGE:C>:/sdl->
        )
    endif()
else()
    target_compile_options(sindri_engine PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wall>
        $<$<COMPILE_LANGUAGE:C>:-Wextra>
        $<$<COMPILE_LANGUAGE:C>:-Werror>
    )
endif()

# --- Optional subdirectories ---
if(SND_BUILD_PAYLOADS)
    add_subdirectory(pocs)
endif()

if(SND_BUILD_TESTS)
    add_subdirectory(tests)
endif()
