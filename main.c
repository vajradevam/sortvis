#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "sorter.h"
#include "renderer.h"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 700

static TTF_Font *find_font(void) {
    const char *paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/System/Library/Fonts/Helvetica.ttf",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        TTF_Font *f = TTF_OpenFont(paths[i], 15);
        if (f) return f;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Sorting Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return 1;
    }

    TTF_Font *font = find_font();
    if (!font) {
        fprintf(stderr, "No font found. Install DejaVu fonts or similar.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_AudioDeviceID audio_dev = audio_init();
    if (!audio_dev) {
        fprintf(stderr, "Audio init failed — continuing without sound.\n");
    }

    renderer_init(renderer, font, audio_dev);

    srand((unsigned int)time(NULL));
    SortState state;
    init_state(&state, 50);

    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    if (state.sorting)
                        state.quit = true;
                    else
                        running = false;
                }
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT
                && !state.sorting)
            {
                if (handle_ui_click(&state, e.button.x, e.button.y))
                    run_sort(&state);
                // If the window was closed during sorting, exit main loop
                if (app_should_quit())
                    running = false;
                clear_quit_flag();
            }
            if (e.type == SDL_MOUSEMOTION) {
                track_mouse(e.motion.x, e.motion.y);
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x08, 0x08, 0x18, 255);
        SDL_RenderClear(renderer);

        int bars_y = ui_bar_height();
        int bars_h = WINDOW_HEIGHT - bars_y;
        render_bars(renderer, &state, 0, bars_y, WINDOW_WIDTH, bars_h);
        render_ui(renderer, font, &state);

        SDL_RenderPresent(renderer);

        if (!state.sorting && !app_should_quit())
            SDL_Delay(16);
    }

    free_state(&state);
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
