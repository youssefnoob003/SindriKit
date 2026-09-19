cmake_minimum_required(VERSION 3.16)
project(SindriKit VERSION 2.2.1 LANGUAGES C ASM_MASM)

message(STATUS "SindriKit ${PROJECT_VERSION}")

if(NOT WIN32)
    message(FATAL_ERROR "SindriKit supports Windows targets only.")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64|ARM)$" OR CMAKE_GENERATOR_PLATFORM MATCHES "ARM")
    message(FATAL_ERROR "SindriKit supports x86 and x64 Windows targets only "
                        "(processor='${CMAKE_SYSTEM_PROCESSOR}', platform='${CMAKE_GENERATOR_PLATFORM}').")
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(SND_ENABLE_DEBUG   "Enable debug prints and tracking"             OFF)
option(SND_USE_PRINTF     "Use printf instead of OutputDebugString"      OFF)
option(SND_BUILD_PAYLOADS   "Build the unified PoC (unified.exe)"          OFF)
option(SND_BUILD_TESTS      "Build test binaries (DLLs and EXEs)"          OFF)
option(SND_BUILD_UNIT_TESTS "Build the host-side unit test binary"         OFF)
option(SND_ENABLE_ASAN      "Instrument the engine (and consumers) with AddressSanitizer" OFF)
option(SND_CRTLESS        "Build the framework for pure CRT-less operation" OFF)
option(SND_RANDOMIZE_SEED "Use randomized seed for compile-time hashes"  OFF)
option(SND_USE_DEFAULTS   "Use sensible defaults for configuration globals" OFF)
option(SND_MORPH        "Run polymorphic mutation engine before build" OFF)

if(SND_ENABLE_ASAN AND MSVC)
    # MSVC AddressSanitizer requires the dynamic CRT.
    message(STATUS "SindriKit: SND_ENABLE_ASAN -> MSVC runtime switched to dynamic (/MD).")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# --- Configuration Guards ---
if(SND_CRTLESS)
    if(SND_BUILD_TESTS OR SND_BUILD_UNIT_TESTS)
        message(WARNING "SindriKit: test builds require the CRT. Forcing SND_CRTLESS=OFF.")
        set(SND_CRTLESS OFF CACHE BOOL "Build the framework for pure CRT-less operation" FORCE)
    else()
        if(SND_ENABLE_DEBUG OR SND_USE_PRINTF)
            message(WARNING "SindriKit: CRT-less build requested. Forcing SND_ENABLE_DEBUG=OFF and SND_USE_PRINTF=OFF.")
            set(SND_ENABLE_DEBUG OFF CACHE BOOL "Enable debug prints and tracking" FORCE)
            set(SND_USE_PRINTF OFF CACHE BOOL "Use printf instead of OutputDebugString" FORCE)
        endif()
    endif()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(SND_MORPH)
    set(MORPH_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mutator/morph.py")
    set(MORPH_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/morphed")
    message(STATUS "SindriKit: SND_MORPH enabled. Running mutation engine...")
    message(STATUS "  Python: ${Python3_EXECUTABLE}")
    message(STATUS "  Script: ${MORPH_SCRIPT}")
    message(STATUS "  In Dir: ${CMAKE_CURRENT_SOURCE_DIR}")
    message(STATUS "  Out Dir: ${MORPH_OUT_DIR}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -u "${MORPH_SCRIPT}" --in-dir "${CMAKE_CURRENT_SOURCE_DIR}" --out-dir "${MORPH_OUT_DIR}"
        RESULT_VARIABLE MORPH_RES
        OUTPUT_VARIABLE MORPH_OUT
        ERROR_VARIABLE MORPH_ERR
    )
    if(NOT MORPH_RES EQUAL 0)
        message(FATAL_ERROR "SindriKit: Mutation engine failed.\nResult: ${MORPH_RES}\nOutput:\n${MORPH_OUT}\nError:\n${MORPH_ERR}")
    endif()

    set(SND_SRC_DIR "${MORPH_OUT_DIR}/src")
    set(SND_INC_DIR "${MORPH_OUT_DIR}/include")
    set(SND_ASM_DIR "${MORPH_OUT_DIR}/asm")
    set(SND_POCS_DIR "${MORPH_OUT_DIR}/pocs")
    set(SND_TESTS_DIR "${MORPH_OUT_DIR}/tests")
    set(SND_CONFIG_DIR "${MORPH_OUT_DIR}/config")
    set(SND_SCRIPTS_DIR "${MORPH_OUT_DIR}/scripts")
else()
    set(SND_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
    set(SND_INC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
    set(SND_ASM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/asm")
    set(SND_POCS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pocs")
    set(SND_TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    set(SND_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/config")
    set(SND_SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
endif()

# --- Architecture-specific assembly ---
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SINDRI_EXEC_ASM
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/direct_x64.asm
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/indirect_x64.asm
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/spoofed_x64.asm
        ${SND_SRC_DIR}/primitives/execution/ffi/asm/ffi_x64.asm
    )
    set(HEAVENSGATE_ASM "")
else()
    set(SINDRI_EXEC_ASM
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/direct_x86.asm
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/indirect_x86.asm
        ${SND_SRC_DIR}/primitives/execution/syscalls/invokers/spoofed_x86.asm
        ${SND_SRC_DIR}/primitives/execution/ffi/asm/ffi_x86.asm
    )
    set(HEAVENSGATE_ASM ${SND_SRC_DIR}/primitives/execution/heavens_gate/asm/heavens_gate_x86.asm)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(SND_HASH_ALGO "DJB2" CACHE STRING "Hashing algorithm (DJB2, FNV1A)")
string(TOUPPER "${SND_HASH_ALGO}" SND_HASH_ALGO)

set(HASH_MANIFEST    "${SND_CONFIG_DIR}/hashes.ini")
set(HASH_SCRIPT      "${SND_SCRIPTS_DIR}/generate_hashes.py")
set(GENERATED_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/sindri_hashes.h")

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${HASH_SCRIPT} ${HASH_MANIFEST} ${GENERATED_HEADER} ${SND_HASH_ALGO} ${SND_RANDOMIZE_SEED}
    RESULT_VARIABLE HASH_RES
)
if(NOT HASH_RES EQUAL 0)
    message(FATAL_ERROR "SindriKit: Failed to generate API hashes.")
endif()

set(STATUS_SCRIPT "${SND_SCRIPTS_DIR}/generate_status_codes.py")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${STATUS_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE STATUS_RES
)
if(NOT STATUS_RES EQUAL 0)
    message(FATAL_ERROR "SindriKit: Failed to generate status codes markdown.")
endif()

# Force CMake to reconfigure automatically if the manifest or script changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${HASH_MANIFEST} ${HASH_SCRIPT})

add_library(sindri_engine STATIC
    # Status engine
    ${SND_SRC_DIR}/status/status.c
    ${SND_SRC_DIR}/status/generic.c
    ${SND_SRC_DIR}/status/cli.c
    ${SND_SRC_DIR}/status/file.c

    # Common
    ${SND_SRC_DIR}/common/debug.c
    ${SND_SRC_DIR}/common/buffer.c
    ${SND_SRC_DIR}/common/hash.c

    # PE parser
    ${SND_SRC_DIR}/parsers/pe/parser.c
    ${SND_SRC_DIR}/parsers/pe/exports.c
    ${SND_SRC_DIR}/parsers/pe/imports.c
    ${SND_SRC_DIR}/parsers/pe/relocations.c
    ${SND_SRC_DIR}/parsers/pe/utils.c
    ${SND_SRC_DIR}/parsers/pe/section.c
    ${SND_SRC_DIR}/parsers/pe/status.c

    # COFF parser
    ${SND_SRC_DIR}/parsers/coff/parser.c
    ${SND_SRC_DIR}/parsers/coff/utils.c
    ${SND_SRC_DIR}/parsers/coff/symbols.c
    ${SND_SRC_DIR}/parsers/coff/relocations.c
    ${SND_SRC_DIR}/parsers/coff/status.c

    # Environment parsers
    ${SND_SRC_DIR}/parsers/env/peb.c
    ${SND_SRC_DIR}/parsers/env/ntdll.c
    ${SND_SRC_DIR}/parsers/env/status.c

    # Reflective PE loader
    ${SND_SRC_DIR}/loaders/pe/engine.c
    ${SND_SRC_DIR}/loaders/pe/chain.c
    ${SND_SRC_DIR}/loaders/pe/status.c

    # COFF loader
    ${SND_SRC_DIR}/loaders/coff/engine.c
    ${SND_SRC_DIR}/loaders/coff/chain.c
    ${SND_SRC_DIR}/loaders/coff/status.c

    # Injection domain
    ${SND_SRC_DIR}/injection/context.c
    ${SND_SRC_DIR}/injection/classic/engine.c
    ${SND_SRC_DIR}/injection/classic/chain.c
    ${SND_SRC_DIR}/injection/apc/engine.c
    ${SND_SRC_DIR}/injection/apc/chain.c
    ${SND_SRC_DIR}/injection/hijack/engine.c
    ${SND_SRC_DIR}/injection/hijack/chain.c

    # Primitives domain
    ${SND_SRC_DIR}/primitives/status.c
    ${SND_SRC_DIR}/primitives/execution/syscalls/status.c

    # Memory primitives
    ${SND_SRC_DIR}/primitives/memory/sys.c
    ${SND_SRC_DIR}/primitives/memory/nt.c

    # Mapping primitives
    ${SND_SRC_DIR}/primitives/mapping/sys.c
    ${SND_SRC_DIR}/primitives/mapping/nt.c

    # Module primitives
    ${SND_SRC_DIR}/primitives/modules/nt.c

    # Process primitives
    ${SND_SRC_DIR}/primitives/process/sys.c
    ${SND_SRC_DIR}/primitives/process/nt.c

    # Thread primitives
    ${SND_SRC_DIR}/primitives/thread/sys.c
    ${SND_SRC_DIR}/primitives/thread/nt.c

    # Object manager
    ${SND_SRC_DIR}/primitives/object_manager/knowndlls.c

    # Execution
    ${SND_SRC_DIR}/primitives/execution/ffi/ffi_invoke.c
    ${SND_SRC_DIR}/primitives/execution/heavens_gate/heavens_gate.c

    # File primitives
    ${SND_SRC_DIR}/primitives/files/sys.c
    ${SND_SRC_DIR}/primitives/files/nt.c

    # Syscall pipeline & components
    ${SND_SRC_DIR}/primitives/execution/syscalls/pipeline.c
    ${SND_SRC_DIR}/primitives/execution/syscalls/resolvers/scan.c
    ${SND_SRC_DIR}/primitives/execution/syscalls/resolvers/sort.c
    ${SND_SRC_DIR}/primitives/execution/syscalls/finders/gadget.c
    ${SND_SRC_DIR}/primitives/execution/syscalls/finders/spoof.c

    # Assembly & generated
    ${SINDRI_EXEC_ASM}
    ${HEAVENSGATE_ASM}
    ${GENERATED_HEADER}
)

if(SND_CRTLESS)
    target_sources(sindri_engine PRIVATE ${SND_SRC_DIR}/common/crt_manifest.c)
else()
    target_sources(sindri_engine PRIVATE
        ${SND_SRC_DIR}/primitives/files/win.c
        ${SND_SRC_DIR}/primitives/memory/win.c
        ${SND_SRC_DIR}/primitives/mapping/win.c
        ${SND_SRC_DIR}/primitives/modules/win.c
        ${SND_SRC_DIR}/primitives/process/win.c
        ${SND_SRC_DIR}/primitives/thread/win.c
    )
endif()

add_library(sindri::engine ALIAS sindri_engine)

# AddressSanitizer is applied to the engine only (PUBLIC propagates to
# consumers such as unified/unit tests). Test payloads are standalone DLLs/EXEs
# that do not link the engine, so they stay clean — important because they are
# reflectively loaded rather than started by the OS loader.
if(SND_ENABLE_ASAN)
    if(MSVC)
        # Scope to C only: MASM rejects /fsanitize (warning A4018). /Zi silences
        # C5072 ("ASAN enabled without debug information"), which /WX otherwise
        # promotes to an error.
        target_compile_options(sindri_engine PUBLIC
            $<$<COMPILE_LANGUAGE:C>:/fsanitize=address>
            $<$<COMPILE_LANGUAGE:C>:/Zi>
            $<$<COMPILE_LANGUAGE:C>:/wd5072>
        )
        target_link_options(sindri_engine PUBLIC /fsanitize=address)
    else()
        target_compile_options(sindri_engine PUBLIC -fsanitize=address -g)
        target_link_options(sindri_engine PUBLIC -fsanitize=address)
    endif()
endif()

if(NOT SND_CRTLESS)
    set(SND_WINDOWS_SDK_SOURCES
        ${SND_SRC_DIR}/primitives/files/win.c
        ${SND_SRC_DIR}/primitives/memory/win.c
        ${SND_SRC_DIR}/primitives/mapping/win.c
        ${SND_SRC_DIR}/primitives/modules/win.c
        ${SND_SRC_DIR}/primitives/process/win.c
        ${SND_SRC_DIR}/primitives/thread/win.c
    )
    set_source_files_properties(${SND_WINDOWS_SDK_SOURCES}
        PROPERTIES COMPILE_DEFINITIONS SND_USE_WINDOWS_SDK=1
    )
endif()

target_include_directories(sindri_engine
    PUBLIC
        ${SND_INC_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    PRIVATE
        ${SND_SRC_DIR}
)

target_compile_features(sindri_engine PUBLIC c_std_99)

if(SND_USE_PRINTF)
    set(DEF_USE_PRINTF 1)
    set(PRINT_MODE "CONSOLE (printf)")
else()
    set(DEF_USE_PRINTF 0)
    set(PRINT_MODE "DEBUGGER (OutputDebugString)")
endif()

if(SND_ENABLE_DEBUG)
    set(DEF_DEBUG 1)
    message(STATUS "SindriKit ${PROJECT_VERSION}: DEBUG | Output: ${PRINT_MODE} | Hash: ${SND_HASH_ALGO}")
else()
    set(DEF_DEBUG 0)
    message(STATUS "SindriKit ${PROJECT_VERSION}: SILENT | Hash: ${SND_HASH_ALGO}")
endif()

target_compile_definitions(sindri_engine PUBLIC
    SND_DEBUG=${DEF_DEBUG}
    SND_USE_PRINTF=${DEF_USE_PRINTF}
    SND_HASH_${SND_HASH_ALGO}=1
)

if(SND_CRTLESS)
    target_compile_definitions(sindri_engine PUBLIC SND_CRTLESS=1)
endif()

if(SND_USE_DEFAULTS)
    target_compile_definitions(sindri_engine PUBLIC SND_USE_DEFAULTS=1)
endif()

if(MSVC)
    # Conforming preprocessor (PUBLIC so consumers that expand the same macros
    # inherit it). The traditional MSVC preprocessor mis-expands the variadic
    # argument-count selection in SND_CHECK_NULL(...), which silently drops the
    # guard for a 3-argument call such as snd_om_knowndll_map.
    target_compile_options(sindri_engine PUBLIC
        $<$<COMPILE_LANGUAGE:C>:/Zc:preprocessor>
    )
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
    if(SND_CRTLESS)
        # Do not emit MSVC's CRT-provided __chkstk helper. CRT-less code uses
        # bounded local frames and the OS reserves/commits the process stack.
        target_compile_options(sindri_engine PRIVATE
            $<$<COMPILE_LANGUAGE:C>:/Gs2147483647>
        )
    endif()
else()
    target_compile_options(sindri_engine PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-Wall>
        $<$<COMPILE_LANGUAGE:C>:-Wextra>
        $<$<COMPILE_LANGUAGE:C>:-Werror>
    )
endif()

if(SND_BUILD_PAYLOADS)
    add_subdirectory(${SND_POCS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/pocs)
endif()

if(SND_BUILD_TESTS)
    add_subdirectory(${SND_TESTS_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tests)
endif()

if(SND_BUILD_UNIT_TESTS)
    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests/unit ${CMAKE_CURRENT_BINARY_DIR}/tests/unit)
endif()
