#include <sindri/internal/windows/types.h>
#include <sindri/parsers/env/ntdll.h>
#include <sindri/parsers/env/peb.h>
#include <sindri/parsers/env/status.h>
#include <sindri/parsers/pe/exports.h>
#include <sindri/parsers/pe/parser.h>
#include <sindri_hashes.h>

static snd_ntdll_entry_t g_peb_ntdll   = {0};
static snd_ntdll_entry_t g_clean_ntdll = {0};

static snd_status_t init_ntdll_entry(PVOID base, snd_ntdll_entry_t *entry) {
    SND_CHECK_NULL(base, entry);

    snd_buffer_t buf = {.data = base, .size = SND_SYS_DLL_SIZE_DEFAULT};

    snd_pe_parser_t parser = {0};
    SND_TRY(snd_pe_parse(&buf, TRUE, &parser));

    entry->base           = base;
    entry->parser         = parser;
    entry->is_initialized = TRUE;

    return SND_OK;
}

snd_status_t snd_ntdll_get_active_parser(const snd_pe_parser_t **out_parser) {
    SND_CHECK_NULL(out_parser);

    if (!g_peb_ntdll.is_initialized) {
        PVOID base = NULL;
        SND_TRY(snd_peb_get_module_base_hash(SND_HASH_NTDLL_DLL, &base));
        SND_TRY(init_ntdll_entry(base, &g_peb_ntdll));
    }

    *out_parser = &g_peb_ntdll.parser;
    return SND_OK;
}

snd_status_t snd_ntdll_get_active_base(PVOID *out_base) {
    SND_CHECK_NULL(out_base);

    const snd_pe_parser_t *parser = NULL;
    SND_TRY(snd_ntdll_get_active_parser(&parser));

    *out_base = g_peb_ntdll.base;
    return SND_OK;
}

snd_status_t snd_ntdll_get_active_export(DWORD func_hash, FARPROC *func_addr_out) {
    const snd_pe_parser_t *parser = NULL;
    SND_TRY(snd_ntdll_get_active_parser(&parser));

    return snd_pe_get_export_address_hash(parser, func_hash, func_addr_out, NULL);
}

snd_status_t snd_ntdll_set_clean(PVOID clean_base) {
    SND_CHECK_NULL(clean_base);
    return init_ntdll_entry(clean_base, &g_clean_ntdll);
}

snd_status_t snd_ntdll_get_clean_parser(const snd_pe_parser_t **out_parser) {
    SND_CHECK_NULL(out_parser);

    if (!g_clean_ntdll.is_initialized) {
        return SND_ERR_CTX(SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED, "Clean NTDLL target is NULL");
    }

    *out_parser = &g_clean_ntdll.parser;
    return SND_OK;
}

snd_status_t snd_ntdll_get_clean_base(PVOID *out_base) {
    SND_CHECK_NULL(out_base);

    if (!g_clean_ntdll.is_initialized) {
        return SND_ERR_CTX(SND_STATUS_NTDLL_CLEAN_NOT_INITIALIZED, "Clean NTDLL target is NULL");
    }

    *out_base = g_clean_ntdll.base;
    return SND_OK;
}

snd_status_t snd_ntdll_get_clean_export(DWORD func_hash, FARPROC *func_addr_out) {
    const snd_pe_parser_t *parser = NULL;
    SND_TRY(snd_ntdll_get_clean_parser(&parser));

    return snd_pe_get_export_address_hash(parser, func_hash, func_addr_out, NULL);
}
