#include <sindri/primitives/memory.h>
#include <sindri/primitives/status.h>
#include <windows.h>

void snd_buffer_free_win(snd_buffer_t *buffer) {
    if (buffer == NULL || buffer->data == NULL) {
        return;
    }

    HeapFree(GetProcessHeap(), 0, buffer->data);
}

static snd_status_t WINAPI win_alloc(LPVOID address, SIZE_T size, DWORD allocation_type, DWORD protect,
                                     LPVOID *out_address) {
    SND_CHECK_NULL(out_address);

    *out_address = VirtualAlloc(address, size, allocation_type, protect);
    return *out_address ? SND_OK : SND_ERR_W32(SND_STATUS_ALLOC_FAILED);
}

static snd_status_t WINAPI win_free(LPVOID address, SIZE_T size, DWORD free_type) {
    if (!address) {
        return SND_OK;
    }

    return VirtualFree(address, size, free_type) ? SND_OK : SND_ERR_W32(SND_STATUS_FREE_FAILED);
}

static snd_status_t WINAPI win_protect(LPVOID address, SIZE_T size, DWORD new_protect, DWORD *old_protect) {
    DWORD  dummy_old_protect  = 0;
    PDWORD target_old_protect = old_protect ? old_protect : &dummy_old_protect;

    return VirtualProtect(address, size, new_protect, target_old_protect) ? SND_OK
                                                                          : SND_ERR_W32(SND_STATUS_PROTECT_FAILED);
}

const snd_memory_api_t snd_mem_win = {.alloc = win_alloc, .free = win_free, .protect = win_protect};
