#include <sindri/common/debug.h>
#include <sindri/common/status.h>

const char *snd_status_to_string(snd_status_t status) {
#if SND_DEBUG
    switch (status.code) {
    case SND_SUCCESS:
        return "Operation completed successfully";
    case SND_ERROR_GENERIC:
        return "An unspecified error occurred";
    case SND_STATUS_NULL_POINTER:
        return "Null pointer provided";
    case SND_STATUS_INTEGER_OVERFLOW:
        return "Integer overflow";
    case SND_STATUS_UNSUPPORTED:
        return "Unsupported operation";
    case SND_STATUS_NOT_INITIALIZED:
        return "A paramater is not initialized";
    case SND_STATUS_BUFFER_TOO_SMALL:
        return "Buffer too small";

    // Core Engine Execution Errors
    case SND_STATUS_TOO_MANY_ARGUMENTS:
        return "Too many arguments provided";
    case SND_STATUS_NTDLL_NOT_INITIALIZED:
        return "NTDLL base is not initialized";
    case SND_STATUS_RESOLVER_NOT_INITIALIZED:
        return "Syscall resolver pipeline is not configured";
    case SND_STATUS_OM_NOT_INITIALIZED:
        return "Object Manager (KnownDlls) not initialized";
    case SND_STATUS_PIPELINE_EXHAUSTED:
        return "Resolver pipeline is full";

    // Command-line argument errors
    case SND_STATUS_MISSING_COMMAND_LINE_ARGS:
        return "Required command-line arguments are missing";
    case SND_STATUS_INVALID_COMMAND_LINE_ARG:
        return "Unknown or invalid command-line argument";

    // File / input errors
    case SND_STATUS_INVALID_PATH:
        return "Payload path is invalid";
    case SND_STATUS_FILE_CREATE_FAILED:
        return "Failed to open specified file";
    case SND_STATUS_FILE_SIZE_QUERY_FAILED:
        return "Failed to query file size";
    case SND_STATUS_FILE_TOO_LARGE:
        return "File too large";
    case SND_STATUS_FILE_TOO_SMALL:
        return "File too small";
    case SND_STATUS_FILE_READ_FAILED:
        return "Failed to read file";
    case SND_STATUS_ALLOC_FAILED:
        return "Failed to allocate memory";

    // PE parsing / validation errors
    case SND_STATUS_DOS_HEADER_TRUNCATED:
        return "PE is truncated before DOS header";
    case SND_STATUS_INVALID_DOS_SIGNATURE:
        return "Invalid DOS signature (MZ)";
    case SND_STATUS_NT_HEADERS_TRUNCATED:
        return "PE is truncated before NT headers";
    case SND_STATUS_INVALID_NT_SIGNATURE:
        return "Invalid NT signature (PE)";
    case SND_STATUS_INVALID_NT_HEADER_OFFSET:
        return "Invalid NT header offset";
    case SND_STATUS_UNSUPPORTED_OPTIONAL_HEADER_MAGIC:
        return "Unsupported PE optional header format";
    case SND_STATUS_INVALID_SECTION_COUNT:
        return "PE has an invalid section count";
    case SND_STATUS_SECTION_TABLE_OVERFLOW:
        return "PE section table size overflow";
    case SND_STATUS_INVALID_SECTION_TABLE:
        return "PE section table is invalid or truncated";
    case SND_STATUS_ARCH_MISMATCH:
        return "Payload architecture does not match loader architecture";
    case SND_STATUS_ENTRY_POINT_NOT_FOUND:
        return "Entry point not found";
    case SND_STATUS_INVALID_IMPORT_TABLE_SIZE:
        return "Invalid import table size";
    case SND_STATUS_INVALID_FILE_OFFSET:
        return "Invalid file offset";
    case SND_STATUS_DIRECTORY_NOT_FOUND:
        return "PE directory not found";
    case SND_STATUS_CORRUPTED_STATE:
        return "Corrupted internal state";
    case SND_STATUS_IMAGE_NOT_MAPPED:
        return "Image is not mapped in memory";
    case SND_STATUS_COFF_SECTION_NOT_FOUND:
        return "COFF section not found";
    case SND_STATUS_COFF_SYMBOL_NOT_FOUND:
        return "COFF symbol not found";
    case SND_STATUS_COFF_ENTRY_NOT_RWX:
        return "Entry point is not in a valid executable section";
    case SND_STATUS_COFF_ENTRY_UNMAPPED:
        return "Entry point was not mapped";
    case SND_STATUS_COFF_NAKED_SYMB_REJECTED:
        return "BOF standard requires [MODULE]$[Function] syntax";

    // Reflective loading errors
    case SND_STATUS_SECTION_COPY_FAILED:
        return "Failed to copy PE sections into mapped image";
    case SND_STATUS_RELOCATION_DIRECTORY_INVALID:
        return "PE relocation directory is invalid";
    case SND_STATUS_RELOCATION_PATCH_OUT_OF_RANGE:
        return "PE relocation patch target is out of range";
    case SND_STATUS_RELOCATION_TYPE_UNSUPPORTED:
        return "PE uses unsupported relocation type";
    case SND_STATUS_IMPORT_DIRECTORY_INVALID:
        return "PE import directory is invalid";
    case SND_STATUS_IMPORT_DLL_LOAD_FAILED:
        return "Failed to load imported DLL";
    case SND_STATUS_IMPORT_THUNK_INVALID:
        return "PE import thunk table is invalid";
    case SND_STATUS_IMPORT_SYMBOL_RESOLVE_FAILED:
        return "Failed to resolve imported symbol";
    case SND_STATUS_PROTECTION_UPDATE_FAILED:
        return "Failed to set section memory protections";
    case SND_STATUS_DLL_INITIALIZATION_FAILED:
        return "DLL entry point returned failure";
    case SND_STATUS_EXPORT_DIRECTORY_INVALID:
        return "PE export directory is invalid";
    case SND_STATUS_EXPORT_NOT_FOUND:
        return "Requested export was not found";
    case SND_STATUS_EXPORT_FORWARDER_UNSUPPORTED:
        return "Forwarded exports are not supported";
    case SND_STATUS_MISSING_DLL_EXPORT_NAME:
        return "DLL payload requires an export name";
    case SND_STATUS_INVALID_STAGE_SEQUENCE:
        return "Invalid loader stage sequence";
    case SND_FAILED_TO_EXECUTE:
        return "Failed to execute payload";
    case SND_STATUS_LOCAL_EXECUTION_BLOCKED:
        return "Local execution is blocked by configuration";
    case SND_STATUS_COFF_RELOCATION_UNSUPPORTED:
        return "Unsupported COFF relocation type";
    case SND_STATUS_COFF_SYMBOL_RESOLUTION_FAILED:
        return "COFF external symbol resolution failed";

    // SSN related errors
    case SND_STATUS_SSN_NOT_FOUND:
        return "SSN not found";
    case SND_STATUS_GADGET_NOT_FOUND:
        return "Gadget not found";

    // PEB related errors
    case SND_STATUS_PEB_MODULE_NOT_FOUND:
        return "PEB module not found";
    case SND_STATUS_PEB_PROCESS_PARAMETERS_NOT_FOUND:
        return "PEB process parameters not found";

    // OS errors
    case SND_STATUS_MODULE_NOT_FOUND:
        return "Module not found";
    case SND_STATUS_SECTION_OPEN_FAILED:
        return "Section open failed";
    case SND_STATUS_SECTION_MAP_FAILED:
        return "Section map failed";
    case SND_STATUS_HANDLE_CLOSE_FAILED:
        return "Handle close failed";
    case SND_STATUS_PROCESS_OPEN_FAILED:
        return "Process open failed";
    case SND_STATUS_VIRTUAL_ALLOC_FAILED:
        return "Virtual alloc failed";
    case SND_STATUS_VIRTUAL_WRITE_FAILED:
        return "Virtual write failed";
    case SND_STATUS_VIRTUAL_PROTECT_FAILED:
        return "Virtual protect failed";
    case SND_STATUS_THREAD_CREATE_FAILED:
        return "Thread create failed";
    case SND_STATUS_VIRTUAL_FREE_FAILED:
        return "Virtual free failed";

    // Remote process errors
    case SND_STATUS_ACCESS_DENIED:
        return "Access denied";
    case SND_STATUS_INVALID_PAYLOAD:
        return "Payload buffer is empty or structurally invalid";
    case SND_STATUS_INVALID_INJECTION_TARGET:
        return "Injection target PID or process handle is invalid";
    default:
        return "Unknown error code";
    }
#else
    (void)status;
    return "";
#endif
}

void snd_status_print(snd_status_t status) {
#if SND_DEBUG
    const char *description = snd_status_to_string(status);

    if (SND_SUCCEEDED(status)) {
        SND_DEBUG_PRINT("[+] Success: %s\n", description);
    } else {
        SND_DEBUG_PRINT("[-] Error: %s\n", description);

        if (status.context[0] != '\0') {
            SND_DEBUG_PRINT("    Context: %s\n", status.context);
        }

        if (status.os_error != 0) {
            SND_DEBUG_PRINT("    OS Error: %lu (0x%08lX)\n", (unsigned long)status.os_error,
                            (unsigned long)status.os_error);
        }

        SND_DEBUG_PRINT("    Location: %s:%d\n", status.file ? status.file : "unknown", status.line);
    }
#else
    (void)status;
#endif
}
