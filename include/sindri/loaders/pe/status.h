#ifndef SND_LOADERS_PE_STATUS_H
#define SND_LOADERS_PE_STATUS_H

#include <sindri/common/macros.h>
#include <sindri/status/core.h>
#include <sindri/status/facility.h>

SND_BEGIN_EXTERN_C

typedef enum _SND_PE_LOADER_STATUS_CODE {
    SND_STATUS_HEADERS_SIZE_INVALID = SND_MAKE_STATUS(SND_FACILITY_LOADER_PE, 1),
    SND_STATUS_SECTION_TABLE_MISSING,
    SND_STATUS_SECTION_SIZE_INVALID,
    SND_STATUS_RELOCATION_DIRECTORY_MISSING,
    SND_STATUS_RELOCATION_TYPE_INVALID,
    SND_STATUS_RELOCATION_DIRECTORY_STRIPPED,
    SND_STATUS_IMAGE_SIZE_NULL,
    SND_STATUS_DLL_INITIALIZATION_FAILED,
    SND_STATUS_LOCAL_EXECUTION_BLOCKED,
    SND_STATUS_IMAGE_ENTRY_POINT_MISSING,
} snd_pe_loader_status_code_t;

/**
 * @brief Converts a PE loader status code into a human-readable description string.
 * @param code Error code value.
 * @retval Pointer to a string description.
 * @retval NULL If @p code is not recognized by the PE loader facility
 * (`SND_FACILITY_LOADER_PE`).
 */
const char *snd_ldr_pe_status_to_string(int code);

SND_END_EXTERN_C

#endif // SND_LOADERS_PE_STATUS_H
