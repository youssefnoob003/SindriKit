#include <sindri/common/hash.h>
#include <sindri/common/status.h>
#include <sindri/common/string.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/parsers/env/peb.h>
#include <stddef.h>
#include <windows.h>

static PVOID peb_get_module_base(const wchar_t *module_name, DWORD module_hash) {
    PSND_PEB peb = snd_peb_get_local();

    if (!peb || !peb->Ldr) {
        return NULL;
    }

    LIST_ENTRY *head       = &peb->Ldr->InMemoryOrderModuleList;
    LIST_ENTRY *curr       = head->Flink;
    size_t      target_len = module_name ? snd_wcsnlen(module_name, MAX_PATH) : 0;

    while (curr != head && curr != NULL) {
        PSND_LDR_DATA_TABLE_ENTRY data_table =
            (PSND_LDR_DATA_TABLE_ENTRY)((BYTE *)curr - offsetof(SND_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        if (data_table->BaseDllName.Buffer != NULL) {
            size_t buffer_char_count = data_table->BaseDllName.Length / sizeof(wchar_t);

            if (module_name != NULL) {
                // String-based comparison
                if (buffer_char_count == target_len) {
                    if (snd_wcsnicmp(data_table->BaseDllName.Buffer, module_name, target_len) == 0) {
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
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }
    *out_base = NULL;

    if (module_name == NULL) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    *out_base = peb_get_module_base(module_name, 0);
    return *out_base ? SND_OK : SND_ERR_CTX(SND_STATUS_PEB_MODULE_NOT_FOUND, "Module name: %ls", module_name);
}

snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base) {
    if (!out_base) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }
    *out_base = NULL;

    if (module_hash == 0) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    *out_base = peb_get_module_base(NULL, module_hash);
    return *out_base ? SND_OK : SND_ERR_CTX(SND_STATUS_PEB_MODULE_NOT_FOUND, "Module hash: 0x%x", module_hash);
}

snd_status_t snd_env_get_process_params(const PSND_PEB peb, PSND_RTL_USER_PROCESS_PARAMETERS *out_params) {
    if (!out_params) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    const SND_PEB *target_peb = (peb != NULL) ? peb : snd_peb_get_local();

    if (!target_peb->ProcessParameters) {
        return SND_ERR(SND_STATUS_PEB_PROCESS_PARAMETERS_NOT_FOUND);
    }

    *out_params = target_peb->ProcessParameters;
    return SND_OK;
}

snd_status_t snd_env_get_command_line(const PSND_PEB peb, SND_UNICODE_STRING *out_cmdline) {
    if (!out_cmdline) {
        return SND_ERR(SND_STATUS_NULL_POINTER);
    }

    SND_RTL_USER_PROCESS_PARAMETERS *params = NULL;
    snd_status_t                     status = snd_env_get_process_params(peb, &params);

    if (SND_FAILED(status)) {
        return status;
    }

    *out_cmdline = params->CommandLine;
    return SND_OK;
}
