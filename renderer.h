#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "sorter.h"

void renderer_init(SDL_Renderer *r, TTF_Font *f, SDL_AudioDeviceID a);
void render_bars(SDL_Renderer *r, SortState *s, int x, int y, int w, int h);
void render_ui(SDL_Renderer *r, TTF_Font *f, SortState *s);
SDL_AudioDeviceID audio_init(void);
void audio_play_tone(SDL_AudioDeviceID dev, int freq_hz, int duration_ms);

void handle_ui_click(SortState *s, int mx, int my);
void track_mouse(int mx, int my);
int  ui_bar_height(void);

#endif
