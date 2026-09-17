#ifndef UNIFIED_PRINT_H
#define UNIFIED_PRINT_H

void poc_fprintf(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_ok(const char *fmt, ...);
void log_err(const char *fmt, ...);

void usage_header(const char *prog, const char *cmd, const char *sub, const char *synopsis);
void usage_mode(const char *name, const char *desc);
void usage_opt(const char *shorthand, const char *longhand, const char *desc);
void usage_note(const char *note);
void usage_backend_flags(void);
void usage_syscall_flags(void);

#endif
