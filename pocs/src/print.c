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

#endif
