#include <sindri/common/hash.h>
#include <sindri/common/memory.h>
#include <sindri/common/string.h>
#include <sindri/internal/nt/peb.h>
#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/env/status.h>
#include <sindri/status/core.h>
#include <stddef.h>

static snd_status_t peb_get_module_base(const wchar_t *module_name, DWORD module_hash, PVOID *out_base) {
    *out_base    = NULL;
    PSND_PEB peb = snd_peb_get_local();
    if (!peb) {
        return SND_ERR(SND_STATUS_PEB_GET_FAILED);
    }

    if (!peb->Ldr) {
        return SND_ERR(SND_STATUS_PEB_LDR_NOT_INITIALIZED);
    }

    SND_LIST_ENTRY *head           = &peb->Ldr->InMemoryOrderModuleList;
    SND_LIST_ENTRY *curr           = head->Flink;
    size_t          target_len     = module_name ? snd_wcsnlen(module_name, SND_MAX_PATH) : 0;
    SIZE_T          max_iterations = MAX_ITERATIONS;

    while (curr != head && curr != NULL) {
        if (max_iterations-- == 0) {
            return SND_ERR(SND_STATUS_MODULE_LIST_CORRUPTED);
        }

        PSND_LDR_DATA_TABLE_ENTRY data_table =
            (PSND_LDR_DATA_TABLE_ENTRY)SND_PTR_SUB(curr, offsetof(SND_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));

        if (data_table->BaseDllName.Buffer != NULL) {
            size_t buffer_char_count = data_table->BaseDllName.Length / sizeof(wchar_t);

            if (module_name != NULL) {
                // String-based comparison
                if (buffer_char_count == target_len) {
                    if (snd_wcsnicmp(data_table->BaseDllName.Buffer, module_name, target_len) == 0) {
                        *out_base = data_table->DllBase;
                        return SND_OK;
                    }
                }
            } else if (module_hash != 0) {
                // Hash-based comparison
                if (buffer_char_count < SND_MAX_PATH) {
                    wchar_t wide_name[SND_MAX_PATH];
                    snd_wcsncpy(wide_name, SND_MAX_PATH, data_table->BaseDllName.Buffer, buffer_char_count);

                    if (snd_hash_wide_lower(wide_name) == module_hash) {
                        *out_base = data_table->DllBase;
                        return SND_OK;
                    }
                }
            }
        }
        curr = curr->Flink;
    }

    return SND_ERR(SND_STATUS_MODULE_NOT_FOUND);
}

snd_status_t WINAPI snd_peb_get_module_base(const wchar_t *module_name, PVOID *out_base) {
    SND_CHECK_NULL(module_name, out_base);
    return peb_get_module_base(module_name, 0, out_base);
}

snd_status_t WINAPI snd_peb_get_module_base_hash(DWORD module_hash, PVOID *out_base) {
    SND_CHECK_NULL(module_hash, out_base);
    return peb_get_module_base(NULL, module_hash, out_base);
}

snd_status_t WINAPI snd_env_get_process_params(const PSND_PEB peb, PSND_RTL_USER_PROCESS_PARAMETERS *out_params) {
    SND_CHECK_NULL(out_params);

    *out_params = NULL;

    const SND_PEB *target_peb = peb ? peb : snd_peb_get_local();
    if (!target_peb) {
        return SND_ERR(SND_STATUS_PEB_GET_FAILED);
    }

    if (!target_peb->ProcessParameters) {
        return SND_ERR(SND_STATUS_PROCESS_PARAMS_NOT_FOUND);
    }

    *out_params = target_peb->ProcessParameters;
    return SND_OK;
}

snd_status_t WINAPI snd_env_get_command_line(const PSND_PEB peb, SND_UNICODE_STRING **out_cmd_line) {
    SND_CHECK_NULL(out_cmd_line);

    *out_cmd_line = NULL;

    PSND_RTL_USER_PROCESS_PARAMETERS params = NULL;
    SND_TRY(snd_env_get_process_params(peb, &params));

    *out_cmd_line = &params->CommandLine;
    return SND_OK;
}
