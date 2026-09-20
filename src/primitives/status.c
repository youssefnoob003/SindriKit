#include <sindri/primitives/status.h>

const char *snd_prim_status_to_string(int code) {
#if SND_DEBUG
    switch ((snd_primitives_status_code_t)code) {
    // Mapping
    case SND_STATUS_MAPPING_OPEN_FAILED:
        return "Mapping open failed";
    case SND_STATUS_MAPPING_VIEW_FAILED:
        return "Mapping view failed";
    case SND_STATUS_MAPPING_QUERY_FAILED:
        return "Mapping query failed";
    case SND_STATUS_MAPPING_HANDLE_CLOSE_FAILED:
        return "Mapping handle close failed";

    // Allocation
    case SND_STATUS_ALLOC_FAILED:
        return "Allocation failed";
    case SND_STATUS_FREE_FAILED:
        return "Free allocation failed";
    case SND_STATUS_PROTECT_FAILED:
        return "Memory protection change failed";

    // Modules
    case SND_STATUS_MODULE_LOAD_FAILED:
        return "Module load failed";
    case SND_STATUS_PROC_RESOLVE_FAILED:
        return "Procedure resolve failed";
    case SND_STATUS_MODULE_BASE_GET_FAILED:
        return "Module base address query failed";

    // Object Management
    case SND_STATUS_OM_NOT_INITIALIZED:
        return "Object manager not initialized";

    // Handle
    case SND_STATUS_HANDLE_CLOSE_FAILED:
        return "Handle close failed";

    // PEB
    case SND_STATUS_PEB_LOCAL_NOT_FOUND:
        return "Local PEB not found";

    // Process
    case SND_STATUS_PROCESS_CREATE_PARAMS_FAILED:
        return "Process creation parameters setup failed";
    case SND_STATUS_PROCESS_FREE_PARAMS_FAILED:
        return "Process creation parameters cleanup failed";
    case SND_STATUS_PROCESS_CREATE_FAILED:
        return "Process creation failed";
    case SND_STATUS_PROCESS_OPEN_FAILED:
        return "Process open failed";
    case SND_STATUS_PROCESS_REMOTE_ALLOC_FAILED:
        return "Remote memory allocation failed";
    case SND_STATUS_PROCESS_REMOTE_FREE_FAILED:
        return "Remote memory release failed";
    case SND_STATUS_PROCESS_REMOTE_WRITE_FAILED:
        return "Remote memory write failed";
    case SND_STATUS_PROCESS_REMOTE_PROTECT_FAILED:
        return "Remote memory protection change failed";

    // Thread
    case SND_STATUS_THREAD_REMOTE_CREATE_FAILED:
        return "Remote thread creation failed";
    case SND_STATUS_THREAD_QUEUE_FAILED:
        return "Thread APC queue failed";
    case SND_STATUS_THREAD_RESUME_FAILED:
        return "Thread resume failed";
    case SND_STATUS_THREAD_SUSPEND_FAILED:
        return "Thread suspend failed";
    case SND_STATUS_THREAD_GET_CONTEXT_FAILED:
        return "Thread context get failed";
    case SND_STATUS_THREAD_SET_CONTEXT_FAILED:
        return "Thread context set failed";
    case SND_STATUS_THREAD_EXIT_FAILED:
        return "Thread exit failed";

    // Syscall
    case SND_STATUS_SYSCALL_INVOKER_NOT_INITIALIZED:
        return "Syscall invoker not initialized";

    default:
        return "Unknown primitives status code";
    }
#else
    (void)code;
    return "";
#endif
}
