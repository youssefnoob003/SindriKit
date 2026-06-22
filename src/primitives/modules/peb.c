#include "sindri/common/status.h"

#include <intrin.h>
#include <sindri/common/hash.h>
#include <sindri/internal/nt_defs.h>
#include <sindri/primitives/peb.h>
#include <stddef.h>
#include <windows.h>

static size_t custom_wcslen(const wchar_t *s) {
    size_t len = 0;
    while (s && s[len] != L'\0') {
        len++;
    }
    return len;
}

static int custom_wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t n) {
    if (n == 0)
        return 0;
    do {
        wchar_t c1 = *s1++;
        wchar_t c2 = *s2++;
        if (c1 >= L'A' && c1 <= L'Z')
            c1 += (L'a' - L'A');
        if (c2 >= L'A' && c2 <= L'Z')
            c2 += (L'a' - L'A');

        if (c1 != c2)
            return (int)(c1 - c2);
        if (c1 == L'\0')
            break;
    } while (--n);
    return 0;
}

static PVOID snd_peb_get_module_base_internal(const wchar_t *module_name, DWORD module_hash) {
#if defined(_WIN64)
    SND_PEB *peb = (SND_PEB *)__readgsqword(0x60);
#elif defined(_WIN32)
    SND_PEB *peb = (SND_PEB *)__readfsdword(0x30);
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif

    if (!peb || !peb->Ldr) {
        return NULL;
    }

    LIST_ENTRY *head       = &peb->Ldr->InMemoryOrderModuleList;
    LIST_ENTRY *curr       = head->Flink;
    size_t      target_len = module_name ? custom_wcslen(module_name) : 0;

    while (curr != head && curr != NULL) {
        PSND_LDR_DATA_TABLE_ENTRY data_table =
            (PSND_LDR_DATA_TABLE_ENTRY)((BYTE *)curr - offsetof(SND_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        if (data_table->BaseDllName.Buffer != NULL) {
            size_t buffer_char_count = data_table->BaseDllName.Length / sizeof(wchar_t);

            if (module_name != NULL) {
                // String-based comparison
                if (buffer_char_count == target_len) {
                    if (custom_wcsnicmp(data_table->BaseDllName.Buffer, module_name, target_len) == 0) {
                        return data_table->DllBase;
                    }
                }
            } else if (module_hash != 0) {
                // Hash-based comparison
                if (buffer_char_count < MAX_PATH) {
                    wchar_t wide_name[MAX_PATH];
                    for (size_t i = 0; i < buffer_char_count; i++) {
                        wide_name[i] = data_table->BaseDllName.Buffer[i];
                    }
                    wide_name[buffer_char_count] = L'\0';

                    if (snd_hash_wide_lower(wide_name) == module_hash) {
                        return data_table->DllBase;
                    }
                }
            }
        }
        curr = curr->Flink;
    }
    return NULL;
}

snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base) {
    if (!out_base) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    *out_base = NULL;

    if (module_name == NULL) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    *out_base = snd_peb_get_module_base_internal(module_name, 0);
    return *out_base ? SND_OK : SND_ERR_CTX(SND_STATUS_PEB_MODULE_NOT_FOUND, "Module name: %ls", module_name);
}

snd_status_t WINAPI snd_peb_get_module_base_by_hash(DWORD module_hash, PVOID *out_base) {
    if (!out_base) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }
    *out_base = NULL;

    if (module_hash == 0) {
        return SND_ERR(SND_STATUS_INVALID_PARAMETER);
    }

    *out_base = snd_peb_get_module_base_internal(NULL, module_hash);
    return *out_base ? SND_OK : SND_ERR_CTX(SND_STATUS_PEB_MODULE_NOT_FOUND, "Module hash: 0x%x", module_hash);
}
