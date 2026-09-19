#include <windows.h>

void snd_target_entry(void) {
    for (;;) {
        /* APC tests require an alertable wait. Early-bird delivery may happen
         * during startup, but Win32/sys APC queues otherwise remain pending if
         * the target only calls Sleep(). */
        SleepEx(1000, TRUE);
    }
}
