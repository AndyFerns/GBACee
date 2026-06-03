#include "args.h"

#include <string.h>
#include <stdio.h>

/**
 * @brief Parses command-line arguments into a diagnostics config.
 *
 * Walks argv looking for --flags and sets the corresponding
 * fields in the diag_config_t structure.
 *
 * When --trace-file is supplied, --trace-all is automatically enabled.
 *
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 * @param cfg  Pointer to the diagnostics config to populate.
 *
 * @return Index of the ROM file argument in argv,
 *         or -1 if --help was requested or no ROM was found.
 */
int diag_parse_args(int argc, char *argv[], diag_config_t *cfg) {
    int rom_index = -1;

    // zero out the config
    memset(cfg, 0, sizeof(diag_config_t));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            diag_print_help(argv[0]);
            return -1;
        }
        else if (strcmp(argv[i], "--trace") == 0) {
            cfg->trace = true;
        }
        else if (strcmp(argv[i], "--trace-cpu") == 0) {
            cfg->trace_cpu = true;
        }
        else if (strcmp(argv[i], "--trace-registers") == 0) {
            cfg->trace_registers = true;
        }
        else if (strcmp(argv[i], "--trace-stack") == 0) {
            cfg->trace_stack = true;
        }
        else if (strcmp(argv[i], "--trace-all") == 0) {
            cfg->trace = true;
            cfg->trace_cpu = true;
            cfg->trace_registers = true;
            cfg->trace_stack = true;
        }
        else if (strcmp(argv[i], "--trace-file") == 0) {
            if (i + 1 < argc) {
                cfg->output_file = argv[++i];
                // --trace-file automatically enables --trace-all
                cfg->trace = true;
                cfg->trace_cpu = true;
                cfg->trace_registers = true;
                cfg->trace_stack = true;
            } else {
                fprintf(stderr, "Error: --trace-file requires a filename argument.\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "--blargg") == 0) {
            cfg->blargg = true;
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'. Use --help for usage.\n", argv[i]);
            return -1;
        }
        else {
            // First non-flag argument is the ROM path
            if (rom_index == -1) {
                rom_index = i;
            }
        }
    }

    return rom_index;
}
