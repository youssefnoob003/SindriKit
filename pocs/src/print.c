#include <stdarg.h>
#include <unified/print.h>

#if defined(SND_CRTLESS)

void poc_fprintf(const char *fmt, ...) {
    (void)fmt;
}

void log_info(const char *fmt, ...) {
    (void)fmt;
}
void log_ok(const char *fmt, ...) {
    (void)fmt;
}
void log_err(const char *fmt, ...) {
    (void)fmt;
}

void usage_header(const char *prog, const char *cmd, const char *sub, const char *synopsis) {
    (void)prog;
    (void)cmd;
    (void)sub;
    (void)synopsis;
}
void usage_mode(const char *name, const char *desc) {
    (void)name;
    (void)desc;
}
void usage_opt(const char *shorthand, const char *longhand, const char *desc) {
    (void)shorthand;
    (void)longhand;
    (void)desc;
}
void usage_note(const char *note) {
    (void)note;
}
void usage_backend_flags(void) {}
void usage_syscall_flags(void) {}

#else

#include <stdio.h>

void poc_fprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static void vout(FILE *stream, const char *prefix, const char *fmt, va_list ap) {
    fprintf(stream, "%s ", prefix);
    vfprintf(stream, fmt, ap);
    fprintf(stream, "\n");
}

void log_info(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vout(stdout, "[*]", fmt, ap);
    va_end(ap);
}
void log_ok(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vout(stdout, "[+]", fmt, ap);
    va_end(ap);
}
void log_err(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vout(stderr, "[-]", fmt, ap);
    va_end(ap);
}

void usage_header(const char *prog, const char *cmd, const char *sub, const char *synopsis) {
    fprintf(stderr, "Usage: %s %s %s %s\n\n", prog, cmd, sub, synopsis);
}

void usage_mode(const char *name, const char *desc) {
    fprintf(stderr, "  %-11s %s\n", name, desc);
}

void usage_opt(const char *shorthand, const char *longhand, const char *desc) {
    if (shorthand && *shorthand) {
        fprintf(stderr, "  %s %-10s  %s\n", shorthand, longhand, desc);
    } else {
        fprintf(stderr, "  %-14s %s\n", longhand, desc);
    }
}

void usage_note(const char *note) {
    fprintf(stderr, "\nNotes:\n  %s\n", note);
}

void usage_backend_flags(void) {
    usage_opt("", "--win", "Win32 API backend.");
    usage_opt("", "--nt", "Native API (ntdll exports) backend.");
    usage_opt("", "--sys", "Direct syscalls (KnownDlls clean ntdll + SSN).");
}

void usage_syscall_flags(void) {
    usage_opt("", "--invoke-direct", "Syscall invoker: direct assembly.");
    usage_opt("", "--invoke-indirect", "Syscall invoker: indirect assembly.");
    usage_opt("", "--invoke-spoofed", "Syscall invoker: spoofed / stack-duplicated assembly.");
    usage_opt("", "--resolve-scan", "SSN resolver: in-memory scan.");
    usage_opt("", "--resolve-sort", "SSN resolver: export-table sort.");
    usage_opt("", "--sys-cache", "Memoize resolved syscall entries (bypassed when spoofed).");
}

#endif
