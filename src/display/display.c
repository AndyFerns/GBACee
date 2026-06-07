#include "display.h"
#include <stdio.h>
#include <string.h>

Display display;

static bool quit_requested = false;

/**
 * @brief display_init - Initializes the SDL Window with default configuration for emulator state
 * 
 * Isolated from PPU business logic
 * 
 * @param scale 
 */
void display_init(int scale) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[DISPLAY] SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    display.scale      = scale;
    display.fullscreen = false;
    display.show_fps   = false;
    display.fps        = 0.0f;
    display.frame_count = 0;
    display.last_frame_ticks = SDL_GetTicks();

    display.window = SDL_CreateWindow(
        "GBCee",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH  * scale,
        SCREEN_HEIGHT * scale,
        SDL_WINDOW_RESIZABLE
    );

    display.renderer = SDL_CreateRenderer(
        display.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    // nearest-neighbor for crisp pixels
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    display.texture = SDL_CreateTexture(
        display.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
}

/**
 * @brief display_shutdown - called to close the SDL instance
 * 
 * Call at the end of Emulator lifecycle
 * 
 */
void display_shutdown(void) {
    if (display.texture)  SDL_DestroyTexture(display.texture);
    if (display.renderer) SDL_DestroyRenderer(display.renderer);
    if (display.window)   SDL_DestroyWindow(display.window);
    SDL_Quit();
}

/**
 * @brief display_present - To be called once per completed frame by PPU
 * 
 * Handles FPS changes for games and texture updation + render clearing
 * 
 * @param framebuffer 
 */
void display_present(const uint32_t *framebuffer) {
    // FPS calculation
    display.frame_count++;
    uint32_t now     = SDL_GetTicks();
    uint32_t elapsed = now - display.last_frame_ticks;
    if (elapsed >= 1000) {
        display.fps = display.frame_count * 1000.0f / (float)elapsed;
        display.frame_count      = 0;
        display.last_frame_ticks = now;

        if (display.show_fps) {
            char title[64];
            snprintf(title, sizeof(title), "GBCee — %.1f FPS", display.fps);
            SDL_SetWindowTitle(display.window, title);
        }
    }

    SDL_UpdateTexture(display.texture, NULL, framebuffer,
                      SCREEN_WIDTH * sizeof(uint32_t));

    SDL_RenderClear(display.renderer);

    // integer scaling: center the game rect
    int win_w, win_h;
    SDL_GetWindowSize(display.window, &win_w, &win_h);
    int scale  = display.fullscreen
                 ? (int)(win_h / SCREEN_HEIGHT)  // fit height in fullscreen
                 : display.scale;
    SDL_Rect dst = {
        .w = SCREEN_WIDTH  * scale,
        .h = SCREEN_HEIGHT * scale,
        .x = (win_w - SCREEN_WIDTH  * scale) / 2,
        .y = (win_h - SCREEN_HEIGHT * scale) / 2,
    };
    SDL_RenderCopy(display.renderer, display.texture, NULL, &dst);
    SDL_RenderPresent(display.renderer);
}

/**
 * @brief display_poll_events - Call once every main loop
 * 
 * Dependent on game-specific events
 * 
 */
void display_poll_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit_requested = true;
                break;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: quit_requested = true;      break;
                    case SDLK_F11:    display_toggle_fullscreen(); break;
                    case SDLK_1: display_set_scale(1); break;
                    case SDLK_2: display_set_scale(2); break;
                    case SDLK_3: display_set_scale(3); break;
                    case SDLK_4: display_set_scale(4); break;
                    default: break;
                }
                break;

            default: break;
        }
    }
}

/**
 * @brief Util function to check whether the user has requiested an SDL instance shutdown or not
 * 
 * @return true - if the user requested application quit
 * @return false - if the user DIDNT request application quit
 */
bool display_should_quit(void) {
    return quit_requested;
}

void display_set_scale(int scale) {
    if (scale < 1 || scale > 8) return;
    display.scale = scale;
    if (!display.fullscreen) {
        SDL_SetWindowSize(display.window,
                          SCREEN_WIDTH  * scale,
                          SCREEN_HEIGHT * scale);
    }
}

/**
 * @brief Helper function to toggle SDL window fullscreen operation
 */
void display_toggle_fullscreen(void) {
    display.fullscreen = !display.fullscreen;
    SDL_SetWindowFullscreen(
        display.window,
        display.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0
    );
}