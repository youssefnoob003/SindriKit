#include <sindri/primitives/memory.h>
#include <windows.h>

static snd_status_t WINAPI win_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                     LPVOID *out_address) {
    if (!out_address)
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    *out_address = VirtualAlloc(address, size, allocation_type, protect);
    return *out_address ? SND_OK : SND_ERR_W32(SND_STATUS_ALLOC_FAILED);
}

static snd_status_t WINAPI win_free(LPVOID address, SIZE_T size, DWORD free_type) {
    return VirtualFree(address, size, free_type) ? SND_OK : SND_ERR_W32(SND_ERROR_GENERIC);
}

static snd_status_t WINAPI win_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    return VirtualProtect(address, size, new_protect, old_protect) ? SND_OK
                                                                   : SND_ERR_W32(SND_STATUS_PROTECTION_UPDATE_FAILED);
}

const snd_memory_api_t snd_mem_win = {.alloc = win_alloc, .free = win_free, .protect = win_protect};
