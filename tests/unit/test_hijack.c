#include "test_util.h"

#include <sindri/internal/windows/context.h>

#if defined(_WIN64)
static void test_frame_x64_register_args(void) {
    SND_THREAD_REGISTERS live = {0};
    live.sp                   = (ULONG_PTR)0x1000; /* 16-byte aligned */
    live.rflags               = 0x0206;            /* IF set + garbage status bits */

    snd_inj_entry_frame_t out = {0};
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, (PVOID)0x5000, 0x11, 0x22, &out)));
    CHECK(out.registers.ip == 0x4000);
    CHECK(out.registers.cx == 0x11);
    CHECK(out.registers.dx == 0x22);
    CHECK((out.registers.sp & 0xF) == 8);              /* RSP == 8 (mod 16) at entry */
    CHECK(out.registers.sp == 0x1000 - sizeof(PVOID)); /* one word below the line     */
    CHECK(out.stack_size == sizeof(PVOID));
    CHECK(out.stack[0] == 0x5000);
    CHECK((out.registers.rflags & SND_EFLAGS_IF) != 0);
    CHECK((out.registers.rflags & SND_EFLAGS_RESERVED1) != 0);
    /* Live flags are preserved, and the required ABI bits remain set. */
    CHECK((out.registers.rflags & live.rflags) == live.rflags);

    /* Non-aligned live stack still yields a 16-byte boundary slot. */
    live.sp = 0x1014;
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, NULL, 0, 0, &out)));
    CHECK((out.registers.sp & 0xF) == 8);
    CHECK(out.registers.sp < 0x1014);

    /* entry fallback helpers and NULL guards. */
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(NULL, (PVOID)0x4000, NULL, 0, 0, &out)));
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, NULL, NULL, 0, 0, &out)));
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, NULL, 0, 0, NULL)));
}
#else
static void test_frame_x86_stack_args(void) {
    SND_THREAD_REGISTERS  live = {0};
    snd_inj_entry_frame_t out  = {0};
    live.sp                    = (ULONG_PTR)0x1000;
    live.rflags                = 0x0206;
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, (PVOID)0x5000, 0x11, 0x22, &out)));
    CHECK(out.registers.ip == 0x4000);
    CHECK(out.registers.sp == 0x1000 - (3 * sizeof(PVOID)));
    CHECK(out.stack_size == 3 * sizeof(PVOID));
    CHECK(out.stack[0] == 0x5000);
    CHECK(out.stack[1] == 0x11);
    CHECK(out.stack[2] == 0x22);
    CHECK((out.registers.rflags & SND_EFLAGS_IF) != 0);
    CHECK((out.registers.rflags & SND_EFLAGS_RESERVED1) != 0);
    CHECK(SND_FAILED(snd_inj_hijack_prepare_frame(&live, NULL, NULL, 0, 0, &out)));
    live.sp = 0;
    CHECK(SND_SUCCEEDED(snd_inj_hijack_prepare_frame(&live, (PVOID)0x4000, NULL, 0, 0, &out)));
    CHECK(out.registers.sp == 0);
    CHECK(out.stack_size == 0);
}
#endif

void snd_test_register_hijack(void) {
#if defined(_WIN64)
    snd_test_register("hijack: prepare frame (x64 register args)", test_frame_x64_register_args);
#else
    snd_test_register("hijack: prepare frame (x86 stack args)", test_frame_x86_stack_args);
#endif
}
