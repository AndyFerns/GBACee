#ifndef ARGS_H
#define ARGS_H

#include "diagnostics.h"

/*
========================================
        ARGUMENT PARSING
========================================

Parses command-line arguments for the
diagnostics subsystem. Only main.c needs
to include this header.
*/

/**
 * @brief Parses command-line arguments into a diagnostics config.
 *
 * Recognizes: --trace, --trace-cpu, --trace-registers,
 * --trace-stack, --trace-all, --trace-file <path>,
 * --blargg, --help.
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
int diag_parse_args(int argc, char *argv[], diag_config_t *cfg);

#endif /* ARGS_H */
