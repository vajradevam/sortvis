#include "renderer.h"
#include <stdlib.h>
#include <math.h>

#define BG_R 0x1a
#define BG_G 0x1a
#define BG_B 0x2e

#define UI_BG_R 0x16
#define UI_BG_G 0x1e
#define UI_BG_B 0x3e

#define BAR_NORMAL_R 52
#define BAR_NORMAL_G 152
#define BAR_NORMAL_B 219

#define BAR_SORTED_R 46
#define BAR_SORTED_G 204
#define BAR_SORTED_B 113

#define BAR_CMP1_R 241
#define BAR_CMP1_G 196
#define BAR_CMP1_B 15

#define BAR_CMP2_R 230
#define BAR_CMP2_G 126
#define BAR_CMP2_B 34

static SDL_Renderer *g_renderer = NULL;
static TTF_Font *g_font = NULL;
static SDL_AudioDeviceID g_audio_dev = 0;

void renderer_init(SDL_Renderer *r, TTF_Font *f, SDL_AudioDeviceID a) {
    g_renderer = r;
    g_font = f;
    g_audio_dev = a;
}

// ----------------------------------------------------------------

static void get_bar_color(SortState *s, int idx, Uint8 *r, Uint8 *g, Uint8 *b) {
    if (s->sorted && idx < s->sorted_until) {
        *r = BAR_SORTED_R; *g = BAR_SORTED_G; *b = BAR_SORTED_B;
        return;
    }
    if (s->sorting) {
        if (idx == s->current_idx) {
            *r = BAR_CMP1_R; *g = BAR_CMP1_G; *b = BAR_CMP1_B;
            return;
        }
        if (idx == s->current_idx2) {
            *r = BAR_CMP2_R; *g = BAR_CMP2_G; *b = BAR_CMP2_B;
            return;
        }
    }
    float t = (float)s->arr[idx] / s->max_val;
    *r = (Uint8)(t * 255);
    *g = (Uint8)((1.0f - fabsf(2.0f * t - 1.0f)) * 128);
    *b = (Uint8)((1.0f - t) * 255);
}

void render_bars(SDL_Renderer *r, SortState *s, int x, int y, int w, int h) {
    if (s->n == 0) return;
    int bar_w = w / s->n;
    if (bar_w < 1) bar_w = 1;
    int pad = bar_w > 2 ? 1 : 0;

    for (int i = 0; i < s->n; i++) {
        int bar_h = (s->arr[i] * (h - 4)) / s->max_val;
        if (bar_h < 1) bar_h = 1;
        int bx = x + i * bar_w;
        int by = y + h - bar_h - 2;

        Uint8 cr, cg, cb;
        get_bar_color(s, i, &cr, &cg, &cb);
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
        SDL_Rect rect = { bx, by, bar_w - 2 * pad, bar_h };
        SDL_RenderFillRect(r, &rect);
    }
}

// ----------------------------------------------------------------

static void render_text(SDL_Renderer *r, TTF_Font *f, const char *text,
                        int x, int y, SDL_Color color)
{
    SDL_Surface *surf = TTF_RenderText_Blended(f, text, color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(r, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

static void fill_rect(SDL_Renderer *r, int x, int y, int w, int h,
                      Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_Rect rect = { x, y, w, h };
    SDL_RenderFillRect(r, &rect);
}

static void draw_rect(SDL_Renderer *r, int x, int y, int w, int h,
                      Uint8 cr, Uint8 cg, Uint8 cb)
{
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_Rect rect = { x, y, w, h };
    SDL_RenderDrawRect(r, &rect);
}

static void draw_triangle(SDL_Renderer *r, int cx, int cy, int size,
                          Uint8 cr, Uint8 cg, Uint8 cb, bool left)
{
    SDL_Point pts[3];
    if (left) {
        pts[0] = (SDL_Point){ cx + size/2, cy - size };
        pts[1] = (SDL_Point){ cx - size/2, cy };
        pts[2] = (SDL_Point){ cx + size/2, cy + size };
    } else {
        pts[0] = (SDL_Point){ cx - size/2, cy - size };
        pts[1] = (SDL_Point){ cx + size/2, cy };
        pts[2] = (SDL_Point){ cx - size/2, cy + size };
    }
    SDL_SetRenderDrawColor(r, cr, cg, cb, 255);
    SDL_RenderDrawLines(r, pts, 3);
    SDL_RenderDrawLine(r, pts[2].x, pts[2].y, pts[0].x, pts[0].y);
}

#define UI_H 80
#define STATUS_H 28
#define BTN_H 34
#define BTN_Y ((UI_H - BTN_H) / 2)
#define EDGE 10

typedef struct {
    SDL_Rect algo_label;
    SDL_Rect algo_prev, algo_name_rect, algo_next;
    SDL_Rect size_label;
    SDL_Rect size_prev, size_val, size_next;
    SDL_Rect shuffle, start;
    SDL_Rect speed_label;
    SDL_Rect speed_prev, speed_val, speed_next;
} UILayout;

static void get_layout(UILayout *l, int win_w) {
    (void)win_w;
    int y = BTN_Y;
    int h = BTN_H;
    int x = EDGE;

    int LBL_W = 52;  // label text width
    int ARW_W = 24;  // arrow button width
    int GAP   = 10;  // gap between groups

    // -- Algorithm --
    l->algo_label  = (SDL_Rect){ x, y, LBL_W, h }; x += LBL_W;
    l->algo_prev   = (SDL_Rect){ x, y, ARW_W, h }; x += ARW_W;
    l->algo_name_rect = (SDL_Rect){ x, y, 130, h }; x += 130;
    l->algo_next   = (SDL_Rect){ x, y, ARW_W, h }; x += ARW_W + GAP;

    // -- Items --
    l->size_label  = (SDL_Rect){ x, y, LBL_W, h }; x += LBL_W;
    l->size_prev   = (SDL_Rect){ x, y, ARW_W, h }; x += ARW_W;
    l->size_val    = (SDL_Rect){ x, y, 44, h };    x += 44;
    l->size_next   = (SDL_Rect){ x, y, ARW_W, h }; x += ARW_W + GAP;

    // -- Shuffle + Start --
    l->shuffle = (SDL_Rect){ x, y, 76, h }; x += 82;
    l->start   = (SDL_Rect){ x, y, 76, h }; x += 82 + GAP;

    // -- Delay --
    l->speed_label = (SDL_Rect){ x, y, LBL_W, h }; x += LBL_W;
    l->speed_prev  = (SDL_Rect){ x, y, ARW_W, h }; x += ARW_W;
    l->speed_val   = (SDL_Rect){ x, y, 54, h };    x += 54;
    l->speed_next  = (SDL_Rect){ x, y, ARW_W, h };
}

static bool pt_in_rect(int px, int py, SDL_Rect r) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

static int g_mouse_x = 0, g_mouse_y = 0;
static bool g_app_quitting = false;

int ui_bar_height(void) { return UI_H + STATUS_H; }

void track_mouse(int mx, int my) {
    g_mouse_x = mx;
    g_mouse_y = my;
}

bool app_should_quit(void) { return g_app_quitting; }
void clear_quit_flag(void) { g_app_quitting = false; }

void render_ui(SDL_Renderer *r, TTF_Font *f, SortState *s) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(r, &win_w, &win_h);

    UILayout l;
    get_layout(&l, win_w);

    // UI bar background
    fill_rect(r, 0, 0, win_w, UI_H, UI_BG_R, UI_BG_G, UI_BG_B);
    // Separator line
    fill_rect(r, 0, UI_H - 1, win_w, 1, 0x2c, 0x3e, 0x50);
    // Status bar background
    fill_rect(r, 0, UI_H, win_w, STATUS_H, 0x12, 0x12, 0x20);
    fill_rect(r, 0, UI_H + STATUS_H - 1, win_w, 1, 0x2c, 0x3e, 0x50);

    SDL_Color label_col = { 140, 200, 240, 255 }; // light blue labels
    SDL_Color text_col  = { 220, 220, 220, 255 };
    SDL_Color bright    = { 255, 255, 255, 255 };
    SDL_Color dim       = { 140, 140, 160, 255 };
    SDL_Color btn_col   = { 0x2c, 0x3e, 0x50, 255 };
    SDL_Color btn_hov   = { 0x3c, 0x4e, 0x60, 255 };
    SDL_Color green     = { 0x27, 0xae, 0x60, 255 };
    SDL_Color green_h   = { 0x2e, 0xcc, 0x71, 255 };
    SDL_Color blue      = { 0x29, 0x80, 0xb9, 255 };
    SDL_Color blue_h    = { 0x34, 0x98, 0xdb, 255 };
    SDL_Color red       = { 0xc0, 0x39, 0x2b, 255 };
    SDL_Color red_h     = { 0xe7, 0x4c, 0x3c, 255 };

    int lfy = BTN_Y + (BTN_H - 16) / 2; // label text y

    // ---- Algorithm:  ◄ Bubble Sort ► ----
    {
        render_text(r, f, "Algo", l.algo_label.x + 2, lfy, label_col);

        SDL_Rect *pr = &l.algo_prev, *nr = &l.algo_next;
        int r0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.r : btn_col.r;
        int g0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.g : btn_col.g;
        int b0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.b : btn_col.b;
        fill_rect(r, pr->x, pr->y, pr->w, pr->h, r0, g0, b0);
        draw_rect(r, pr->x, pr->y, pr->w, pr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, pr->x + pr->w/2, pr->y + pr->h/2, 6, 200, 200, 200, true);

        int r1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.r : btn_col.r;
        int g1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.g : btn_col.g;
        int b1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.b : btn_col.b;
        fill_rect(r, nr->x, nr->y, nr->w, nr->h, r1, g1, b1);
        draw_rect(r, nr->x, nr->y, nr->w, nr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, nr->x + nr->w/2, nr->y + nr->h/2, 6, 200, 200, 200, false);

        const char *name = ALGO_NAMES[s->algo];
        int tx = l.algo_name_rect.x + 6;
        render_text(r, f, name, tx, l.algo_name_rect.y + (l.algo_name_rect.h - 16) / 2, text_col);
    }

    // ---- Items:  ◄ 50 ► ----
    {
        render_text(r, f, "Items", l.size_label.x + 2, lfy, label_col);

        SDL_Rect *pr = &l.size_prev, *nr = &l.size_next;
        int r0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.r : btn_col.r;
        int g0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.g : btn_col.g;
        int b0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.b : btn_col.b;
        fill_rect(r, pr->x, pr->y, pr->w, pr->h, r0, g0, b0);
        draw_rect(r, pr->x, pr->y, pr->w, pr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, pr->x + pr->w/2, pr->y + pr->h/2, 6, 200, 200, 200, true);

        int r1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.r : btn_col.r;
        int g1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.g : btn_col.g;
        int b1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.b : btn_col.b;
        fill_rect(r, nr->x, nr->y, nr->w, nr->h, r1, g1, b1);
        draw_rect(r, nr->x, nr->y, nr->w, nr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, nr->x + nr->w/2, nr->y + nr->h/2, 6, 200, 200, 200, false);

        char buf[16];
        snprintf(buf, sizeof(buf), "%d", s->n);
        int vx = l.size_val.x + (l.size_val.w - strlen(buf) * 8) / 2;
        render_text(r, f, buf, vx, l.size_val.y + (l.size_val.h - 16) / 2, text_col);
    }

    // ---- Shuffle + Start buttons ----
    {
        bool hover_sf = pt_in_rect(g_mouse_x, g_mouse_y, l.shuffle);
        if (!s->sorting) {
            fill_rect(r, l.shuffle.x, l.shuffle.y, l.shuffle.w, l.shuffle.h,
                      hover_sf ? blue_h.r : blue.r, hover_sf ? blue_h.g : blue.g, hover_sf ? blue_h.b : blue.b);
            draw_rect(r, l.shuffle.x, l.shuffle.y, l.shuffle.w, l.shuffle.h,
                      blue.r, blue.g, blue.b);
            render_text(r, f, "Shuffle", l.shuffle.x + 8, l.shuffle.y + 9, bright);
        } else {
            fill_rect(r, l.shuffle.x, l.shuffle.y, l.shuffle.w, l.shuffle.h, 0x20, 0x20, 0x30);
            draw_rect(r, l.shuffle.x, l.shuffle.y, l.shuffle.w, l.shuffle.h, 0x30, 0x30, 0x40);
            render_text(r, f, "Shuffle", l.shuffle.x + 8, l.shuffle.y + 9, dim);
        }

        bool hover_st = pt_in_rect(g_mouse_x, g_mouse_y, l.start);
        if (s->sorting) {
            fill_rect(r, l.start.x, l.start.y, l.start.w, l.start.h,
                      hover_st ? red_h.r : red.r, hover_st ? red_h.g : red.g, hover_st ? red_h.b : red.b);
            draw_rect(r, l.start.x, l.start.y, l.start.w, l.start.h, red.r, red.g, red.b);
            render_text(r, f, "Stop!", l.start.x + 16, l.start.y + 9, bright);
        } else {
            fill_rect(r, l.start.x, l.start.y, l.start.w, l.start.h,
                      hover_st ? green_h.r : green.r, hover_st ? green_h.g : green.g, hover_st ? green_h.b : green.b);
            draw_rect(r, l.start.x, l.start.y, l.start.w, l.start.h, green.r, green.g, green.b);
            render_text(r, f, "Sort!", l.start.x + 18, l.start.y + 9, bright);
        }
    }

    // ---- Delay:  ◄ 20ms ► ----
    {
        render_text(r, f, "Delay", l.speed_label.x + 2, lfy, label_col);

        SDL_Rect *pr = &l.speed_prev, *nr = &l.speed_next;
        int r0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.r : btn_col.r;
        int g0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.g : btn_col.g;
        int b0 = pt_in_rect(g_mouse_x, g_mouse_y, *pr) ? btn_hov.b : btn_col.b;
        fill_rect(r, pr->x, pr->y, pr->w, pr->h, r0, g0, b0);
        draw_rect(r, pr->x, pr->y, pr->w, pr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, pr->x + pr->w/2, pr->y + pr->h/2, 6, 200, 200, 200, true);

        int r1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.r : btn_col.r;
        int g1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.g : btn_col.g;
        int b1 = pt_in_rect(g_mouse_x, g_mouse_y, *nr) ? btn_hov.b : btn_col.b;
        fill_rect(r, nr->x, nr->y, nr->w, nr->h, r1, g1, b1);
        draw_rect(r, nr->x, nr->y, nr->w, nr->h, 0x3c, 0x4e, 0x60);
        draw_triangle(r, nr->x + nr->w/2, nr->y + nr->h/2, 6, 200, 200, 200, false);

        char buf[16];
        snprintf(buf, sizeof(buf), "%dms", s->delay_ms);
        int vx = l.speed_val.x + (l.speed_val.w - strlen(buf) * 8) / 2;
        render_text(r, f, buf, vx, l.speed_val.y + (l.speed_val.h - 16) / 2, text_col);
    }

    // ---- Status bar ----
    {
        int sty = UI_H + (STATUS_H - 16) / 2;

        char buf[128];
        snprintf(buf, sizeof(buf), "Comparisons: %d", s->comparisons);
        render_text(r, f, buf, EDGE, sty, dim);

        snprintf(buf, sizeof(buf), "Swaps: %d", s->swaps);
        render_text(r, f, buf, 170, sty, dim);

        const char *status;
        if (s->sorting)
            status = "Status: Sorting...";
        else if (s->sorted)
            status = "Status: Sorted";
        else
            status = "Status: Idle";
        render_text(r, f, status, 340, sty, dim);

        // Color legend
        SDL_Color unsorted_c = { 140, 140, 160, 255 };
        render_text(r, f, "■ unsorted", 560, sty, unsorted_c);
        // draw a small filled rect as swatch
        SDL_SetRenderDrawColor(r, BAR_CMP1_R, BAR_CMP1_G, BAR_CMP1_B, 255);
        SDL_Rect sw = { 560 + (int)strlen("■ unsorted") * 8 + 8, sty + 2, 12, 12 };
        SDL_RenderFillRect(r, &sw);
        render_text(r, f, "comparing", sw.x + sw.w + 4, sty, dim);

        SDL_SetRenderDrawColor(r, BAR_SORTED_R, BAR_SORTED_G, BAR_SORTED_B, 255);
        SDL_Rect sw2 = { sw.x + sw.w + 4 + (int)strlen("comparing") * 8 + 8, sty + 2, 12, 12 };
        SDL_RenderFillRect(r, &sw2);
        render_text(r, f, "sorted", sw2.x + sw2.w + 4, sty, dim);
    }
}

// ----------------------------------------------------------------
// Audio
// ----------------------------------------------------------------

SDL_AudioDeviceID audio_init(void) {
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 4096;
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev == 0) return 0;
    SDL_PauseAudioDevice(dev, 0);
    return dev;
}

void audio_play_tone(SDL_AudioDeviceID dev, int freq_hz, int duration_ms) {
    if (!dev || freq_hz <= 0) return;
    int nsamples = (int)(44100 * duration_ms / 1000);
    if (nsamples <= 0) return;
    Sint16 *buf = (Sint16 *)malloc(nsamples * sizeof(Sint16));
    if (!buf) return;
    for (int i = 0; i < nsamples; i++) {
        double t = (double)i / 44100.0;
        double env = exp(-4.0 * (double)i / nsamples);
        buf[i] = (Sint16)(8000 * sin(2.0 * M_PI * freq_hz * t) * env);
    }
    if (SDL_GetQueuedAudioSize(dev) < 44100) {
        SDL_QueueAudio(dev, buf, nsamples * sizeof(Sint16));
    }
    free(buf);
}

// ----------------------------------------------------------------
// on_sort_step  (called by sorting algorithms in sorter.c)
// ----------------------------------------------------------------

void on_sort_step(SortState *s) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            s->quit = true;
            g_app_quitting = true;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            s->quit = true;
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            // Directly check if click is on the Stop button
            UILayout layout;
            int ww;
            SDL_GetRendererOutputSize(g_renderer, &ww, NULL);
            get_layout(&layout, ww);
            if (pt_in_rect(e.button.x, e.button.y, layout.start))
                s->quit = true;
        }
        if (e.type == SDL_MOUSEMOTION) {
            g_mouse_x = e.motion.x;
            g_mouse_y = e.motion.y;
        }
    }

    // Render
    int w, h;
    SDL_GetRendererOutputSize(g_renderer, &w, &h);
    SDL_SetRenderDrawColor(g_renderer, BG_R, BG_G, BG_B, 255);
    SDL_RenderClear(g_renderer);

    int bars_y = UI_H + STATUS_H;
    int bars_h = h - bars_y;
    render_bars(g_renderer, s, 0, bars_y, w, bars_h);
    render_ui(g_renderer, g_font, s);

    SDL_RenderPresent(g_renderer);

    // Audio
    if (s->current_idx >= 0 && s->current_idx < s->n) {
        int freq = 200 + (s->arr[s->current_idx] * 800) / (s->max_val > 0 ? s->max_val : 1);
        audio_play_tone(g_audio_dev, freq, s->delay_ms);
    }

    SDL_Delay(s->delay_ms);
}

// ----------------------------------------------------------------
// UI click handling
// ----------------------------------------------------------------

bool handle_ui_click(SortState *s, int mx, int my) {
    if (s->sorting) return false;

    UILayout l;
    int win_w;
    SDL_GetRendererOutputSize(g_renderer, &win_w, NULL);
    get_layout(&l, win_w);

    if (pt_in_rect(mx, my, l.algo_prev)) {
        s->algo = (s->algo - 1 + ALGO_COUNT) % ALGO_COUNT;
        return false;
    }
    if (pt_in_rect(mx, my, l.algo_next)) {
        s->algo = (s->algo + 1) % ALGO_COUNT;
        return false;
    }

    if (pt_in_rect(mx, my, l.size_prev)) {
        int new_n = s->n - 10;
        if (new_n < 10) new_n = 10;
        if (new_n != s->n) { free_state(s); init_state(s, new_n); }
        return false;
    }
    if (pt_in_rect(mx, my, l.size_next)) {
        int new_n = s->n + 10;
        if (new_n > 300) new_n = 300;
        if (new_n != s->n) { free_state(s); init_state(s, new_n); }
        return false;
    }

    if (pt_in_rect(mx, my, l.speed_prev)) {
        s->delay_ms = s->delay_ms > 5 ? s->delay_ms - 5 : 1;
        return false;
    }
    if (pt_in_rect(mx, my, l.speed_next)) {
        s->delay_ms = s->delay_ms < 500 ? s->delay_ms + 5 : 500;
        return false;
    }

    if (pt_in_rect(mx, my, l.shuffle)) {
        shuffle_array(s);
        return false;
    }

    return pt_in_rect(mx, my, l.start);
}
