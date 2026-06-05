#ifndef PPU_H
#define PPU_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 144
#define SCREEN_SCALE  3

// PPU Modes
typedef enum {
    PPU_MODE_HBLANK  = 0,
    PPU_MODE_VBLANK  = 1,
    PPU_MODE_OAM     = 2,
    PPU_MODE_DRAWING = 3,
} PPUMode;

// LCDC bit masks
#define LCDC_LCD_ENABLE        0x80
#define LCDC_WINDOW_MAP        0x40  // 0=9800, 1=9C00
#define LCDC_WINDOW_ENABLE     0x20
#define LCDC_BG_TILE_DATA      0x10  // 0=8800, 1=8000
#define LCDC_BG_MAP            0x08  // 0=9800, 1=9C00
#define LCDC_OBJ_SIZE          0x04  // 0=8x8, 1=8x16
#define LCDC_OBJ_ENABLE        0x02
#define LCDC_BG_ENABLE         0x01

// STAT bit masks
#define STAT_LYC_INT           0x40
#define STAT_OAM_INT           0x20
#define STAT_VBLANK_INT        0x10
#define STAT_HBLANK_INT        0x08
#define STAT_LYC_FLAG          0x04
#define STAT_MODE_MASK         0x03


// OAM entry
typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t tile;
    uint8_t flags;
} OAMEntry;

// OAM sprite flags
#define SPRITE_PRIORITY   0x80  // 0=above BG, 1=behind BG colors 1-3
#define SPRITE_FLIP_Y     0x40
#define SPRITE_FLIP_X     0x20
#define SPRITE_PALETTE    0x10  // 0=OBP0, 1=OBP1

// PPU struct containing 
typedef struct {
    // SDL
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    uint32_t      framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

    // Timing
    PPUMode  mode;
    int      dots;          // T-cycles elapsed in current scanline
    uint8_t  ly;            // current scanline (mirrors 0xFF44)
    int      window_line;   // internal window line counter

    // Scanline sprite buffer (max 10 sprites per line on DMG)
    OAMEntry sprite_buffer[10];
    int      sprite_count;
} PPU;

extern PPU ppu;


/**
 * init_ppu - Initializes the PPU (Pixel Processing Unit).
 *
 * Resets internal registers and prepares the PPU for rendering.
 */
void init_ppu();

/**
 * ppu_step - Advances the PPU by one step (typically per CPU cycle).
 *
 * Simulates rendering phases (OAM Search, Drawing, HBlank, VBlank).
 * Should be called once per CPU cycle or as part of frame scheduling.
 */
void ppu_step();

/**
 * ppu_render_frame - Renders the full frame to the SDL window.
 *
 * Draws the current screen contents using SDL.
 * Should be called after each complete frame (VBlank).
 */
void ppu_render_frame();

#endif
