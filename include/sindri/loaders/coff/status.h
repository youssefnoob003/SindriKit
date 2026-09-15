#ifndef SND_LOADERS_COFF_STATUS_H
#define SND_LOADERS_COFF_STATUS_H

#include <sindri/common/macros.h>
#include <sindri/status/core.h>
#include <sindri/status/facility.h>

SND_BEGIN_EXTERN_C

typedef enum _SND_COFF_LOADER_STATUS_CODE {
    SND_STATUS_COFF_LOADER_SYMBOL_MISSING = SND_MAKE_STATUS(SND_FACILITY_LOADER_COFF, 1),
    SND_STATUS_SYMBOL_ENTRY_OUT_OF_RANGE,
    SND_STATUS_SYMBOL_ADDRESS_NULL,
    SND_STATUS_COFF_LOADER_RELOC_OUT_OF_RANGE,
    SND_STATUS_RELOCATION_UNKNOWN_TYPE,
    SND_STATUS_COFF_LOADER_MAP_SIZE_OVERFLOW,
    SND_STATUS_COFF_LOADER_BSS_SIZE_OVERFLOW,
    SND_STATUS_COFF_LOADER_TRAMPOLINE_OVERFLOW,
    SND_STATUS_COFF_LOADER_VIRTUAL_SIZE_OVERFLOW,
} snd_coff_loader_status_code_t;

/**
 * @brief Converts a COFF loader status code into a human-readable description string.
 * @param code Error code value.
 * @retval Pointer to a string description.
 * @retval NULL If @p code is not recognized by the COFF loader facility
 * (`SND_FACILITY_LOADER_COFF`).
 */
const char *snd_ldr_coff_status_to_string(int code);

SND_END_EXTERN_C

#endif // SND_LOADERS_COFF_STATUS_H
