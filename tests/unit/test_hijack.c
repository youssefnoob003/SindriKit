#include "test_util.h"

#include <sindri/internal/windows/context.h>

#if SND_ENTRY_REGISTER_ARGS
static void test_frame_x64_register_args(void) {
    SND_THREAD_REGISTERS live = {0};
    live.sp                   = (ULONG_PTR)0x1000; /* 16-byte aligned */
    live.rflags               = 0x0206;            /* IF set + garbage status bits */

    SND_THREAD_REGISTERS out = {0};
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, 0x11, 0x22, &out)));
    CHECK(out.ip == 0x4000);
    CHECK(out.cx == 0x11);
    CHECK(out.dx == 0x22);
    CHECK((out.sp & 0xF) == 8);              /* RSP == 8 (mod 16) at entry */
    CHECK(out.sp == 0x1000 - sizeof(PVOID)); /* one word below the line     */
    CHECK((out.rflags & SND_EFLAGS_IF) != 0);
    CHECK((out.rflags & SND_EFLAGS_RESERVED1) != 0);
    CHECK((out.rflags & ~(SND_EFLAGS_IF | SND_EFLAGS_RESERVED1)) == 0);

    /* Non-aligned live stack still yields a 16-byte boundary slot. */
    live.sp = 0x1014;
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, 0, 0, &out)));
    CHECK((out.sp & 0xF) == 8);
    CHECK(out.sp < 0x1014);

    /* entry fallback helpers and NULL guards. */
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(NULL, (PVOID)0x4000, 0, 0, &out)));
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, NULL, 0, 0, &out)));
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, 0, 0, NULL)));
}
#else
static void test_frame_x86_stack_args(void) {
    SND_THREAD_REGISTERS live = {0};
    SND_THREAD_REGISTERS out  = {0};
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, 0, 0, &out)));
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, NULL, 0, 0, &out)));
}
#endif

void snd_test_register_hijack(void) {
#if SND_ENTRY_REGISTER_ARGS
    snd_test_register("hijack: prepare frame (x64 register args)", test_frame_x64_register_args);
#else
    snd_test_register("hijack: prepare frame (x86 stack args -> mismatch)", test_frame_x86_stack_args);
#endif
}