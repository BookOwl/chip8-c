#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH   64
#define SCREEN_HEIGHT  32
#define PIXEL_WIDTH    10
#define PIXEL_HEIGHT   10
#define WINDOW_WIDTH   (SCREEN_WIDTH * PIXEL_WIDTH)
#define WINDOW_HEIGHT  (SCREEN_HEIGHT * PIXEL_HEIGHT)

#define BLACK 0, 0, 0
#define WHITE 255, 255, 255

typedef struct {
    double fps_millis;
    double last_tick;
} fps_clock;

fps_clock new_fps_clock(uint32_t fps) {
    fps_clock c;
    c.fps_millis = 1000.0 / (double) fps;
    c.last_tick = SDL_GetTicks();
    return c;
}

void fps_clock_tick(fps_clock* clock) {
    double elapsed = SDL_GetTicks() - clock->last_tick;
    int32_t diff = lround(clock->fps_millis - elapsed);
    if (diff > 0) {
        SDL_Delay((uint32_t) diff);
    }
    clock->last_tick = SDL_GetTicks();
}

typedef struct {
    SDL_Window* window;
    SDL_Surface* surf;
    uint8_t screen[64][32];
    uint32_t bg;
    uint32_t fg;
} sdl_handle;

void clear_screen(sdl_handle*);

sdl_handle graphics_init() {
    sdl_handle h;
    SDL_Window* window = NULL;
    SDL_Surface* surf = NULL;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    window = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, \
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if(window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }
    h.window = window;
    h.surf = SDL_GetWindowSurface(window);
    h.bg = SDL_MapRGB(h.surf->format, BLACK);
    h.fg = SDL_MapRGB(h.surf->format, WHITE);
    clear_screen(&h);
    return h;
}

void clear_screen(sdl_handle* gfx) {
    SDL_FillRect(gfx->surf, NULL, gfx->bg);
    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 32; y++) {
            gfx->screen[x][y] = 0;
        }
    }
}

bool draw_sprite(sdl_handle* gfx, uint8_t start_x, uint8_t start_y, uint8_t sprite[], uint8_t sprite_len) {
    start_x = start_x % SCREEN_WIDTH;
    start_y = start_y % SCREEN_HEIGHT;
    uint8_t old;
    uint16_t new;
    uint8_t *pix;
    bool cleared_pixel = false;
    for (int y = 0; y < sprite_len; y++) {
        for (int x = 0; x < 8; x++) {
            if ((start_x + x >= SCREEN_WIDTH) || (start_y + y) >= SCREEN_HEIGHT) break;
            pix = &gfx->screen[start_x + x][start_y + y];
            old = *pix;
            new = (sprite[y] >> (7 - x)) & 0x1;
            printf("%s", new ? "#" : "_");
            // XOR the sprite data to the screen
            if ((old == 1) && (new == 1)) {
                *pix = 0;
                cleared_pixel = true;
            } else if ((old == 0) && (new == 1)) {
                *pix = 1;
            }
        }
        printf("\n");
    }
    printf("---\n\n");
    return cleared_pixel;
}

void display_screen(sdl_handle* gfx) {
    uint32_t colors[2] = {gfx->bg, gfx->fg};
    SDL_Rect pixel;
    pixel.w = PIXEL_WIDTH;
    pixel.h = PIXEL_HEIGHT;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            pixel.x = x * PIXEL_WIDTH;
            pixel.y = y * PIXEL_HEIGHT;
            SDL_FillRect(gfx->surf, &pixel, colors[gfx->screen[x][y]]);
        }
    }
    SDL_UpdateWindowSurface(gfx->window);
}