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


/**
 * @brief Advances the PPU by the given number of T-cycles.
 *
 * Implements the PPU mode timing state machine:
 *   Each scanline = 456 T-cycles
 *     Mode 2 (OAM scan):  80 dots   — cycles 0-79
 *     Mode 3 (Drawing):   172 dots  — cycles 80-251
 *     Mode 0 (HBlank):    204 dots  — cycles 252-455
 *   Lines 0-143: active display (Mode 2 -> 3 -> 0)
 *   Lines 144-153: VBlank (Mode 1, 456 dots each = 4560 total)
 *   Full frame: 154 lines x 456 dots = 70224 T-cycles
 *
 * @param cycles Number of T-cycles elapsed since last call
 */
void ppu_step(int cycles) {
    uint8_t lcdc = mmu_read(0xFF40);
    if (!(lcdc & LCDC_LCD_ENABLE)) {
        // LCD off: reset state, draw blank white frame
        ppu.dots = 0;
        ppu.ly   = 0;
        ppu.mode = PPU_MODE_HBLANK;
        mmu_write(0xFF44, 0);
        return;
    }

    ppu.dots += cycles;

    switch (ppu.mode) {
        case PPU_MODE_OAM:
            if (ppu.dots >= 80) {
                ppu.dots -= 80;
                ppu_set_mode(PPU_MODE_DRAWING);
            }
            break;

        case PPU_MODE_DRAWING:
            if (ppu.dots >= 172) {
                ppu.dots -= 172;
                ppu_render_scanline();        // render this line
                ppu_set_mode(PPU_MODE_HBLANK);
            }
            break;

        case PPU_MODE_HBLANK:
            if (ppu.dots >= 204) {
                ppu.dots -= 204;
                ppu.ly++;
                mmu_write(0xFF44, ppu.ly);
                ppu_check_lyc();

                if (ppu.ly == 144) {
                    ppu_set_mode(PPU_MODE_VBLANK);
                    // trigger VBlank interrupt
                    uint8_t ifl = mmu_read(0xFF0F);
                    mmu_write(0xFF0F, ifl | 0x01);
                    // present the completed frame
                    ppu_present_frame();
                } else {
                    ppu_set_mode(PPU_MODE_OAM);
                }
            }
            break;

        case PPU_MODE_VBLANK:
            if (ppu.dots >= 456) {
                ppu.dots -= 456;
                ppu.ly++;
                mmu_write(0xFF44, ppu.ly);
                ppu_check_lyc();

                if (ppu.ly > 153) {
                    ppu.ly = 0;
                    ppu.window_line = 0;
                    mmu_write(0xFF44, 0);
                    ppu_set_mode(PPU_MODE_OAM);
                }
            }
            break;
    }
}

/**
 * @brief Sets the PPU mode and updates the STAT register, firing STAT interrupts as needed.
 *
 * @param mode The new PPU mode
 */
static void ppu_set_mode(PPUMode mode) {
    ppu.mode = mode;
    uint8_t stat = mmu_read(0xFF41);
    stat = (stat & ~STAT_MODE_MASK) | (uint8_t)mode;

    // check STAT interrupt sources
    bool stat_irq = false;
    if (mode == PPU_MODE_HBLANK  && (stat & STAT_HBLANK_INT)) stat_irq = true;
    if (mode == PPU_MODE_VBLANK  && (stat & STAT_VBLANK_INT)) stat_irq = true;
    if (mode == PPU_MODE_OAM     && (stat & STAT_OAM_INT))    stat_irq = true;

    mmu_write(0xFF41, stat);

    if (stat_irq) {
        uint8_t ifl = mmu_read(0xFF0F);
        mmu_write(0xFF0F, ifl | 0x02);  // LCD STAT interrupt bit 1
    }

    // Perform OAM scan when entering OAM mode
    if (mode == PPU_MODE_OAM) {
        ppu_oam_scan();
    }
}

/**
 * @brief Checks LY == LYC and updates STAT register, firing LYC interrupt if enabled.
 */
static void ppu_check_lyc(void) {
    uint8_t lyc  = mmu_read(0xFF45);
    uint8_t stat = mmu_read(0xFF41);

    if (ppu.ly == lyc) {
        stat |= STAT_LYC_FLAG;
        if (stat & STAT_LYC_INT) {
            uint8_t ifl = mmu_read(0xFF0F);
            mmu_write(0xFF0F, ifl | 0x02);
        }
    } else {
        stat &= ~STAT_LYC_FLAG;
    }
    mmu_write(0xFF41, stat);
}

/**
 * @brief Scans OAM for sprites visible on the current scanline (max 10 per line).
 */
static void ppu_oam_scan(void) {
    uint8_t lcdc     = mmu_read(0xFF40);
    int     sprite_h = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;
    ppu.sprite_count = 0;

    for (int i = 0; i < 40 && ppu.sprite_count < 10; i++) {
        uint16_t addr  = 0xFE00 + (i * 4);
        OAMEntry entry = {
            .y    = mmu_read(addr),
            .x    = mmu_read(addr + 1),
            .tile = mmu_read(addr + 2),
            .flags= mmu_read(addr + 3),
        };

        // sprite Y is offset by 16
        int sprite_top = (int)entry.y - 16;
        if (ppu.ly >= sprite_top && ppu.ly < sprite_top + sprite_h) {
            ppu.sprite_buffer[ppu.sprite_count++] = entry;
        }
    }
}
