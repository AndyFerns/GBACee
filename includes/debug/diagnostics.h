#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>
#include <stdio.h>

/*
========================================
        DIAGNOSTICS SUBSYSTEM
========================================

Runtime-configurable tracing and logging.

Replaces the compile-time debug.h approach.
All trace output is gated behind command-line
flags with zero source modification required.

Usage:
  #include "diagnostics.h"

  TRACE("MMU Initialized!\n");
  TRACE_CPU("[PC=0x%04X] ...\n", pc, ...);
*/

// =================================================
// Diagnostics Configuration
// =================================================

typedef struct {
    bool trace;           // --trace            general lifecycle/internal messages
    bool trace_cpu;       // --trace-cpu        per-instruction CPU state dumps
    bool trace_registers; // --trace-registers  register state tracing
    bool trace_stack;     // --trace-stack      stack operation tracing
    bool blargg;          // --blargg           BGB/SameBoy-compatible trace formatting
    const char *output_file; // --trace-file <path>  log file path (NULL = stdout)
} diag_config_t;

/// Global diagnostics configuration singleton
extern diag_config_t diag;


// =================================================
// Diagnostics API
// =================================================

/**
 * @brief Initializes the diagnostics subsystem.
 *
 * Opens the log file if cfg->output_file is set.
 * Must be called before any trace macros are used.
 *
 * @param cfg Pointer to the diagnostics configuration.
 */
void diag_init(diag_config_t *cfg);

/**
 * @brief Writes formatted output to the diagnostics stream.
 *
 * Routes output to stdout or the configured log file.
 * All trace macros call this internally.
 *
 * @param fmt printf-style format string.
 * @param ... format arguments.
 */
void diag_write(const char *fmt, ...);

/**
 * @brief Shuts down the diagnostics subsystem.
 *
 * Flushes and closes the log file if one was opened.
 * Safe to call multiple times.
 */
void diag_shutdown(void);

/**
 * @brief Prints help text for command-line usage.
 *
 * @param prog The program name (argv[0]).
 */
void diag_print_help(const char *prog);


// =================================================
// Trace Macros
// =================================================

/// General lifecycle/internal trace (behind --trace)
#define TRACE(...) \
    do { if (diag.trace) diag_write(__VA_ARGS__); } while(0)

/// CPU instruction trace (behind --trace-cpu)
#define TRACE_CPU(...) \
    do { if (diag.trace_cpu) diag_write(__VA_ARGS__); } while(0)

/// Register state trace (behind --trace-registers)
#define TRACE_REGISTERS(...) \
    do { if (diag.trace_registers) diag_write(__VA_ARGS__); } while(0)

/// Stack operation trace (behind --trace-stack)
#define TRACE_STACK(...) \
    do { if (diag.trace_stack) diag_write(__VA_ARGS__); } while(0)

/// Blargg-formatted trace output (behind --blargg)
#define TRACE_BLARGG(...) \
    do { if (diag.blargg) diag_write(__VA_ARGS__); } while(0)


// =================================================
// Error Logging (always-on, stderr)
// =================================================

/// Always-on error output to stderr
#define LOG_ERROR(...) \
    do { fprintf(stderr, __VA_ARGS__); } while(0)

#endif /* DIAGNOSTICS_H */
