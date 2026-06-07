#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH  160       // 160p width 
#define SCREEN_HEIGHT 144       // 144p height
#define SCREEN_SCALE_DEFAULT 3  // default scale : 3

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;

    int     scale;
    bool    fullscreen;
    bool    show_fps;
    float   fps;

    // frame timing
    uint32_t last_frame_ticks;
    uint32_t frame_count;
} Display;

extern Display display;

// lifecycle
void display_init(int scale);
void display_shutdown(void);

// called once per completed frame by PPU
void display_present(const uint32_t *framebuffer);

// called every main loop iteration
void display_poll_events(void);

// returns true if the user has requested quit
bool display_should_quit(void);

// scale control
void display_set_scale(int scale);
void display_toggle_fullscreen(void);

#endif