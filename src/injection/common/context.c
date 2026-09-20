#include <sindri/common/debug.h>
#include <sindri/injection/common/context.h>

const char *snd_inj_stage_to_string(snd_inj_stage_t stage) {
#if SND_DEBUG
    switch (stage) {
    case SND_INJ_STAGE_UNINITIALIZED:
        return "UNINITIALIZED";
    case SND_INJ_STAGE_TARGET_ACQUIRED:
        return "TARGET_ACQUIRED";
    case SND_INJ_STAGE_MEMORY_ALLOCATED:
        return "MEMORY_ALLOCATED";
    case SND_INJ_STAGE_PAYLOAD_WRITTEN:
        return "PAYLOAD_WRITTEN";
    case SND_INJ_STAGE_PROTECTIONS_SET:
        return "PROTECTIONS_SET";
    case SND_INJ_STAGE_CONTEXT_APPLIED:
        return "CONTEXT_APPLIED";
    case SND_INJ_STAGE_EXECUTED:
        return "EXECUTED";
    default:
        return "UNKNOWN";
    }
#else
    (void)stage;
    return "";
#endif
}
