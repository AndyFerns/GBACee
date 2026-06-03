#include "diagnostics.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "version.h"

// =================================================
// Global State
// =================================================

/// Global diagnostics configuration singleton
diag_config_t diag = {0};

/// Internal output stream (stdout or log file)
static FILE *diag_stream = NULL;


// =================================================
// Diagnostics API Implementation
// =================================================


/**
 * @brief Initializes the diagnostics subsystem.
 *
 * Opens the log file if cfg->output_file is set.
 * Falls back to stdout if the file cannot be opened.
 *
 * @param cfg Pointer to the diagnostics configuration.
 */
void diag_init(diag_config_t *cfg) {
    if (cfg->output_file) {
        diag_stream = fopen(cfg->output_file, "w");
        if (!diag_stream) {
            fprintf(stderr, "[DIAG] Warning: Could not open trace file '%s', falling back to stdout.\n",
                cfg->output_file);
            diag_stream = stdout;
        }
    } else {
        diag_stream = stdout;
    }
}


/**
 * @brief Writes formatted output to the diagnostics stream.
 *
 * @param fmt printf-style format string.
 * @param ... format arguments.
 */
void diag_write(const char *fmt, ...) {
    if (!diag_stream) {
        diag_stream = stdout;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(diag_stream, fmt, args);
    va_end(args);

    fflush(diag_stream);
}


/**
 * @brief Shuts down the diagnostics subsystem.
 *
 * Flushes and closes the log file if one was opened.
 */
void diag_shutdown(void) {
    if (diag_stream && diag_stream != stdout && diag_stream != stderr) {
        fflush(diag_stream);
        fclose(diag_stream);
    }
    diag_stream = NULL;
}


/**
 * @brief Prints help text for command-line usage.
 *
 * @param prog The program name (argv[0]).
 */
void diag_print_help(const char *prog) {
    printf("GBCee - Game Boy Emulator v%s\n", GBCEE_FULL_VERSION);
    printf("\n");
    printf("Usage:\n");
    printf("  %s <rom> [options]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  --help              Show this help message and exit\n");
    printf("  --trace             Enable general lifecycle/internal tracing\n");
    printf("  --trace-cpu         Enable per-instruction CPU state tracing\n");
    printf("  --trace-registers   Enable register state tracing\n");
    printf("  --trace-stack       Enable stack operation tracing\n");
    printf("  --trace-all         Enable all trace categories\n");
    printf("  --trace-file <file> Write all trace output to a file\n");
    printf("                      (automatically enables --trace-all)\n");
    printf("  --blargg            Enable BGB/SameBoy-compatible trace formatting\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s tetris.gb\n", prog);
    printf("  %s tetris.gb --trace\n", prog);
    printf("  %s tetris.gb --trace-cpu\n", prog);
    printf("  %s tetris.gb --trace-file trace.log\n", prog);
    printf("  %s tetris.gb --trace-all --trace-file trace.log\n", prog);
    printf("  %s cpu_instrs.gb --blargg\n", prog);
}
