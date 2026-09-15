#ifndef SND_INJECTION_STATUS_H
#define SND_INJECTION_STATUS_H

#include <sindri/common/macros.h>
#include <sindri/status.h>

SND_BEGIN_EXTERN_C

/*
 * The injection domain uses a single shared status header across all injection subdomains.
 * Currently empty as the injection domain does not require domain-specific status codes at present.
 */

SND_END_EXTERN_C

#endif // SND_INJECTION_STATUS_H
