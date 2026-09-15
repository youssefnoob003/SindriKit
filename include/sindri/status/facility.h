#ifndef SND_STATUS_FACILITY_H
#define SND_STATUS_FACILITY_H

#include <sindri/common/macros.h>

SND_BEGIN_EXTERN_C

// Domain Facility Identifiers (16-bit)
typedef enum _SND_FACILITY_ID {
    SND_FACILITY_GENERIC,
    SND_FACILITY_CLI,
    SND_FACILITY_FILE,
    SND_FACILITY_CONTEXT_MACHINES,
    SND_FACILITY_PARSER_PE,
    SND_FACILITY_PARSER_COFF,
    SND_FACILITY_PARSER_ENV,
    SND_FACILITY_LOADER_PE,
    SND_FACILITY_LOADER_COFF,
    SND_FACILITY_SYSCALL,
    SND_FACILITY_PRIMITIVES,
} snd_facility_id_t;

// Facility Bitmask Helpers
#define SND_MAKE_STATUS(fac, code)         (((int)(fac) << 16) | ((int)(code) & 0xFFFF))
#define SND_STATUS_FACILITY(status_code)   (((unsigned int)(status_code) >> 16) & 0xFFFF)
#define SND_STATUS_LOCAL_CODE(status_code) ((unsigned int)(status_code) & 0xFFFF)

SND_END_EXTERN_C

#endif // SND_STATUS_FACILITY_H
