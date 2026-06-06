#include <stdio.h>
#include <stdbool.h> 
#include "cpu.h"
#include "mmu.h"
#include "timer.h"
#include "interrupts.h"
#include "diagnostics.h"
#include "ppu.h"
#include <SDL2/SDL.h>
#include "args.h"


/**
 * @brief main:
 * Entry point of the emulator.
 * Loads the ROM and starts the emulation loop.
 *  
 * @param argc: Number of command-line arguments.
 * @param argv: Array of argument strings.
 *
 *
 * @returns 
 * 0 on success, non-zero on failure.
 */
int main(int argc, char *argv[]) {
    // Parse command-line arguments
    int rom_index = diag_parse_args(argc, argv, &diag);

    if (rom_index == -1) {
        // --help was printed, or an error occurred
        if (argc < 2) {
            fprintf(stderr, "Usage: %s <ROM file> [options]\n", argv[0]);
            fprintf(stderr, "Use --help for full usage information.\n");
        }
        return (argc < 2) ? 1 : 0;
    }

    // Initialize the diagnostics subsystem
    diag_init(&diag);

    // should follow emulator lifecycle:
    // initialize hardware -> load the game -> run main loop -> clean up resources 

    // 1. Initialize hardware
    mmu_init();
    cpu_reset();
    ppu_init();      // Initialize the Picture Processing Unit 
    // timer_init(); // placeholder for initializing the timer

    // 2. Load the game rom
    // only call mmu_load_rom and not load_rom
    if (mmu_load_rom(argv[rom_index]) != 0) {
        fprintf(stderr, "Error: Failed to load ROM '%s'.\n", argv[rom_index]);
        diag_shutdown();
        return 1;
    }

    // Main emulation loop
    TRACE(" --- Starting Emulation --- \n");
    while (true) { 
        // Poll SDL events (needed for window close, future joypad)
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) goto cleanup;
        }

        /** Execute one instruction per cycle 
         * cpu step handles the halted state internally
         * doesnt fetch an opcode for halting
        */
        int cycles_this_step = cpu_step();

        static int insn_count = 0;
        if (++insn_count % 1000000 == 0) {
            printf("Executed %d million instructions. PC=0x%04X, LY=%d, LCDC=0x%02X\n", insn_count / 1000000, cpu.PC, ppu.ly, mmu_read(0xFF40));
            fflush(stdout);
        }

        if (cpu.error) {
            LOG_ERROR("CPU encountered a fatal error. Stopping.\n");
            break;
        }
        // halted CPUs still tick at 4 cycles waiting for interrupts — don't break on them

        // update other hardware components with the elapsed cycles
        timer_step(cycles_this_step);
        ppu_step(cycles_this_step);

        // Check for interrupts after all hardware has been updated
        handle_interrupts();
    }

cleanup:
    // 4. cleanup  
    TRACE(" --- Emulation Halted --- \n");
    ppu_shutdown();
    diag_shutdown();
    mmu_free(); // prevent memory leaks from loaded roms
    return 0;
}