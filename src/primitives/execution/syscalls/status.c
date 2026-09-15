#include <sindri/primitives/syscalls.h>

const char *snd_syscall_status_to_string(int code) {
#if SND_DEBUG
    switch (code) {
    case SND_STATUS_SSN_NOT_FOUND:
        return "SSN not found in target image export table";
    case SND_STATUS_GADGET_NOT_FOUND:
        return "Required syscall/trampoline gadget not found in loaded modules";
    case SND_STATUS_SYSCALL_PIPELINE_EXHAUSTED:
        return "All registered syscall resolution strategies failed";
    case SND_STATUS_RESOLVER_NOT_INITIALIZED:
        return "Syscall resolver pipeline is not configured";
    case SND_STATUS_NTDLL_NOT_INITIALIZED:
        return "Clean NTDLL image is not initialized in the environment";
    case SND_STATUS_SPOOF_GADGET_NOT_FOUND:
        return "Syscall spoofing gadget not found in loaded modules";
    default:
        return "Unkown error code";
    }
#else
    (void)code;
    return "";
#endif
}
