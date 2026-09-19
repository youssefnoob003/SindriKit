#include <sindri.h>
#include <unified/commands.h>
#include <unified/common.h>
#include <unified/main.h>
#include <unified/print.h>

#if !defined(SND_CRTLESS)
int main(int argc, char *argv[]) {
    return unified_main(argc, argv);
}
#endif

static void print_global_usage(const char *prog) {
    poc_fprintf("SindriKit Unified PoC Tool\n"
                "Usage: %s <command> <subcommand> [options]\n\n"
                "Commands:\n"
                "  load pe        Load and execute a PE file (DLL or EXE)\n"
                "  load coff      Load and execute a COFF object file\n"
                "  inject classic Classic remote thread injection into an existing process\n"
                "  inject apc     Early-bird APC injection (spawns target process)\n"
                "  inject hijack  Thread-hijack injection (spawns suspended target)\n"
#if defined(_WIN32) && !defined(_WIN64)
                "  hg             Heaven's Gate (WOW64 32-bit only)\n"
#endif
                "\n"
                "Use '%s <command> <subcommand> --help' for command-specific options.\n",
                prog, prog);
}

int unified_main(int argc, char *argv[]) {
    if (argc < 2) {
        print_global_usage(argv[0]);
        return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
    }

    const char *cmd  = argv[1];
    const char *prog = argv[0];

    if (poc_strcmp(cmd, "-h") == 0 || poc_strcmp(cmd, "--help") == 0) {
        print_global_usage(prog);
        return SND_SUCCESS;
    }

    if (poc_strcmp(cmd, "load") == 0 || poc_strcmp(cmd, "inject") == 0) {
        if (argc < 3) {
            poc_fprintf("[-] Missing subcommand.\n");
            print_global_usage(prog);
            return SND_STATUS_MISSING_COMMAND_LINE_ARGS;
        }

        const char *sub = argv[2];
        argc -= 3;
        argv += 3;

        if (poc_strcmp(cmd, "load") == 0) {
            if (poc_strcmp(sub, "pe") == 0)
                return cmd_load_pe(argc, argv, prog);
            if (poc_strcmp(sub, "coff") == 0)
                return cmd_load_coff(argc, argv, prog);
            poc_fprintf("[-] Unknown load subcommand: %s\n", sub);
            return SND_STATUS_INVALID_COMMAND_LINE_ARG;
        } else {
            if (poc_strcmp(sub, "classic") == 0)
                return cmd_inject_classic(argc, argv, prog);
            if (poc_strcmp(sub, "apc") == 0)
                return cmd_inject_apc(argc, argv, prog);
            if (poc_strcmp(sub, "hijack") == 0)
                return cmd_inject_hijack(argc, argv, prog);
            poc_fprintf("[-] Unknown inject subcommand: %s\n", sub);
            return SND_STATUS_INVALID_COMMAND_LINE_ARG;
        }
    }
#if defined(_WIN32) && !defined(_WIN64)
    else if (poc_strcmp(cmd, "hg") == 0) {
        argc -= 2;
        argv += 2;
        return cmd_hg(argc, argv, prog);
    }
#endif
    else {
        poc_fprintf("[-] Unknown command: %s\n", cmd);
        print_global_usage(prog);
        return SND_STATUS_INVALID_COMMAND_LINE_ARG;
    }
}
