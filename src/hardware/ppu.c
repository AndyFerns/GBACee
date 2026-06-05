#include "ppu.h"
#include "mmu.h"
#include <SDL2/SDL.h>
#include <string.h>

// Global PPU state
PPU ppu;

// DMG grayscale palette — maps 2-bit shade to ARGB
static const uint32_t dmg_colors[4] = {
    0xFFFFFFFF,  // 0 — White
    0xFFAAAAAA,  // 1 — Light gray
    0xFF555555,  // 2 — Dark gray
    0xFF000000,  // 3 — Black
};

// Palette decode: maps 2-bit color index through a palette register
static uint8_t decode_palette(uint8_t palette_reg, uint8_t color_idx) {
    return (palette_reg >> (color_idx * 2)) & 0x03;
}

// Forward declarations for internal helpers
static void ppu_set_mode(PPUMode mode);
static void ppu_check_lyc(void);
static void ppu_oam_scan(void);
static void ppu_present_frame(void);

/**
 * @brief Initializes the PPU, creates the SDL window and rendering context.
 */
void ppu_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    ppu.window = SDL_CreateWindow(
        "GBCee - Game Boy Emulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH * SCREEN_SCALE,
        SCREEN_HEIGHT * SCREEN_SCALE,
        0
    );
    ppu.renderer = SDL_CreateRenderer(ppu.window, -1, SDL_RENDERER_ACCELERATED);
    ppu.texture = SDL_CreateTexture(
        ppu.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    memset(ppu.framebuffer, 0xFF, sizeof(ppu.framebuffer)); // White screen

    // Initialize PPU state
    ppu.mode = PPU_MODE_OAM;
    ppu.dots = 0;
    ppu.ly = 0;
    ppu.window_line = 0;
    ppu.sprite_count = 0;
}
