#ifndef UNIFIED_COMMANDS_H
#define UNIFIED_COMMANDS_H

int cmd_load_pe(int argc, char *argv[], const char *prog);
int cmd_load_coff(int argc, char *argv[], const char *prog);
int cmd_inject_classic(int argc, char *argv[], const char *prog);
int cmd_inject_apc(int argc, char *argv[], const char *prog);
int cmd_inject_hijack(int argc, char *argv[], const char *prog);
int cmd_hg(int argc, char *argv[], const char *prog);

#endif
