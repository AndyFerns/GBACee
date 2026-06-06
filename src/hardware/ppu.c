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
    SDL_SetMainReady();
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


/**
 * @brief Renders a single scanline into the framebuffer.
 *
 * Draws background, window, and sprite layers for the current scanline (ppu.ly).
 */
void ppu_render_scanline(void) {
    uint8_t lcdc = mmu_read(0xFF40);
    uint8_t scx  = mmu_read(0xFF43);
    uint8_t scy  = mmu_read(0xFF42);
    uint8_t wx   = mmu_read(0xFF4B);
    uint8_t wy   = mmu_read(0xFF4A);
    uint8_t bgp  = mmu_read(0xFF47);

    // bg_color_idx[x]: tracks BG color index at each pixel for sprite priority
    uint8_t bg_color_idx[SCREEN_WIDTH] = {0};

    // --- Background ---
    if (lcdc & LCDC_BG_ENABLE) {
        // Which tile map: 0x9800 or 0x9C00
        uint16_t bg_map = (lcdc & LCDC_BG_MAP) ? 0x9C00 : 0x9800;

        // Which tile data: 0x8000 (unsigned) or 0x8800 (signed)
        bool     signed_tile = !(lcdc & LCDC_BG_TILE_DATA);

        uint8_t y_in_map = (ppu.ly + scy) & 0xFF;  // wraps at 256
        uint8_t tile_row = y_in_map / 8;
        uint8_t fine_y   = y_in_map % 8;

        for (int px = 0; px < SCREEN_WIDTH; px++) {
            uint8_t x_in_map  = (px + scx) & 0xFF;
            uint8_t tile_col  = x_in_map / 8;
            uint8_t fine_x    = 7 - (x_in_map % 8);  // bit 7 = leftmost pixel

            uint16_t map_addr = bg_map + tile_row * 32 + tile_col;
            uint8_t  tile_id  = mmu_read(map_addr);

            uint16_t tile_addr;
            if (signed_tile) {
                // Signed addressing: tile 0 is at 0x9000, range -128 to 127
                tile_addr = 0x9000 + (int8_t)tile_id * 16;
            } else {
                tile_addr = 0x8000 + tile_id * 16;
            }

            // Each tile row = 2 bytes (lo and hi bit planes)
            uint8_t lo = mmu_read(tile_addr + fine_y * 2);
            uint8_t hi = mmu_read(tile_addr + fine_y * 2 + 1);

            static bool has_logged_vram = false;
            if ((lo != 0 || hi != 0) && !has_logged_vram) {
                printf("VRAM has non-zero pixels! tile_id=0x%02X\n", tile_id);
                has_logged_vram = true;
            }

            uint8_t bit_1 = (lo >> fine_x) & 1;
            uint8_t bit_2 = (hi >> fine_x) & 1;
            uint8_t color_idx = (bit_2 << 1) | bit_1;
            bg_color_idx[px]  = color_idx;

            uint8_t shade = decode_palette(bgp, color_idx);
            ppu.framebuffer[ppu.ly * SCREEN_WIDTH + px] = dmg_colors[shade];
        }
    }

    // --- Window ---
    if ((lcdc & LCDC_WINDOW_ENABLE) && ppu.ly >= wy) {
        uint16_t win_map    = (lcdc & LCDC_WINDOW_MAP) ? 0x9C00 : 0x9800;
        bool     signed_tile = !(lcdc & LCDC_BG_TILE_DATA);

        uint8_t fine_y   = ppu.window_line % 8;
        uint8_t tile_row = ppu.window_line / 8;

        int win_x_start = (int)wx - 7;  // WX is offset by 7

        for (int px = (win_x_start < 0 ? 0 : win_x_start); px < SCREEN_WIDTH; px++) {
            uint8_t x_in_win = px - win_x_start;
            uint8_t tile_col = x_in_win / 8;
            uint8_t fine_x   = 7 - (x_in_win % 8);

            uint16_t map_addr = win_map + tile_row * 32 + tile_col;
            uint8_t  tile_id  = mmu_read(map_addr);

            uint16_t tile_addr;
            if (signed_tile) {
                tile_addr = 0x9000 + (int8_t)tile_id * 16;
            } else {
                tile_addr = 0x8000 + tile_id * 16;
            }

            uint8_t lo = mmu_read(tile_addr + fine_y * 2);
            uint8_t hi = mmu_read(tile_addr + fine_y * 2 + 1);

            uint8_t color_idx = ((hi >> fine_x) & 1) << 1 | ((lo >> fine_x) & 1);
            bg_color_idx[px]  = color_idx;

            uint8_t shade = decode_palette(bgp, color_idx);
            ppu.framebuffer[ppu.ly * SCREEN_WIDTH + px] = dmg_colors[shade];
        }
        ppu.window_line++;
    }

    // --- Sprites ---
    if (lcdc & LCDC_OBJ_ENABLE) {
        int sprite_h = (lcdc & LCDC_OBJ_SIZE) ? 16 : 8;

        // Draw sprites in reverse order so lower-index sprites win on overlap
        for (int i = ppu.sprite_count - 1; i >= 0; i--) {
            OAMEntry *sp = &ppu.sprite_buffer[i];

            int sprite_x = (int)sp->x - 8;  // X is offset by 8
            int sprite_y = (int)sp->y - 16; // Y is offset by 16
            int row      = ppu.ly - sprite_y;

            // Y flip
            if (sp->flags & SPRITE_FLIP_Y) row = (sprite_h - 1) - row;

            // For 8x16, mask bit 0 of tile index
            uint8_t tile_id = sp->tile;
            if (sprite_h == 16) tile_id &= 0xFE;

            uint16_t tile_addr = 0x8000 + tile_id * 16 + row * 2;
            uint8_t  lo        = mmu_read(tile_addr);
            uint8_t  hi        = mmu_read(tile_addr + 1);

            uint8_t pal_reg = (sp->flags & SPRITE_PALETTE)
                              ? mmu_read(0xFF49)   // OBP1
                              : mmu_read(0xFF48);  // OBP0

            for (int bit = 7; bit >= 0; bit--) {
                int px = sprite_x + (7 - bit);
                if (px < 0 || px >= SCREEN_WIDTH) continue;

                // X flip
                int real_bit = (sp->flags & SPRITE_FLIP_X) ? (7 - bit) : bit;

                uint8_t color_idx = ((hi >> real_bit) & 1) << 1 | ((lo >> real_bit) & 1);
                if (color_idx == 0) continue;  // color 0 = transparent for sprites

                // BG priority: if flag set and BG color is not 0, sprite is hidden
                if ((sp->flags & SPRITE_PRIORITY) && bg_color_idx[px] != 0) continue;

                uint8_t shade = decode_palette(pal_reg, color_idx);
                ppu.framebuffer[ppu.ly * SCREEN_WIDTH + px] = dmg_colors[shade];
            }
        }
    }
}

/**
 * @brief Presents the completed framebuffer to the SDL window.
 */
static void ppu_present_frame(void) {
    SDL_UpdateTexture(ppu.texture, NULL, ppu.framebuffer,
                      SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(ppu.renderer);
    SDL_RenderCopy(ppu.renderer, ppu.texture, NULL, NULL);
    SDL_RenderPresent(ppu.renderer);

    static int frame_count = 0;
    frame_count++;
    if (frame_count == 60) {
        FILE *f = fopen("frame_60.ppm", "wb");
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
                uint32_t p = ppu.framebuffer[i];
                uint8_t r = (p >> 16) & 0xFF;
                uint8_t g = (p >> 8) & 0xFF;
                uint8_t b = p & 0xFF;
                fwrite(&r, 1, 1, f);
                fwrite(&g, 1, 1, f);
                fwrite(&b, 1, 1, f);
            }
            fclose(f);
            printf("Dumped frame_60.ppm\n");
        }
    }

    // rudimentary frame pacing (~60 FPS)
    SDL_Delay(16);

    // Frame rate limiting (59.73 Hz ~ 16.74 ms per frame)
    static uint32_t last_frame_time = 0;
    uint32_t current_time = SDL_GetTicks();
    uint32_t elapsed = current_time - last_frame_time;
    if (elapsed < 16) {
        SDL_Delay(16 - elapsed);
    }
    last_frame_time = SDL_GetTicks();
}

/**
 * @brief Shuts down the PPU and cleans up SDL resources.
 */
void ppu_shutdown(void) {
    if (ppu.texture)  SDL_DestroyTexture(ppu.texture);
    if (ppu.renderer) SDL_DestroyRenderer(ppu.renderer);
    if (ppu.window)   SDL_DestroyWindow(ppu.window);
}
