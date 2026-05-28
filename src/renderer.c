#include "renderer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── Tonal palette ───────────────────────────────────────────────
#define C_BG_R 0x08  // deepest background — chart area
#define C_BG_G 0x08
#define C_BG_B 0x18
#define C_UI_BG_R 0x1e  // toolbar — mid-tone
#define C_UI_BG_G 0x1e
#define C_UI_BG_B 0x38
#define C_STAT_R 0x14  // status bar — between bg and toolbar
#define C_STAT_G 0x14
#define C_STAT_B 0x28
#define C_BDR_R 0x2c  // dividers
#define C_BDR_G 0x2c
#define C_BDR_B 0x44
#define C_GRID_R 0x14  // grid lines
#define C_GRID_G 0x14
#define C_GRID_B 0x28

// Control surfaces
#define C_VAL_BG_R 0x18
#define C_VAL_BG_G 0x18
#define C_VAL_BG_B 0x30
#define C_VAL_BDR_R 0x30
#define C_VAL_BDR_G 0x30
#define C_VAL_BDR_B 0x4c
#define C_BTN_R 0x24
#define C_BTN_G 0x24
#define C_BTN_B 0x3c
#define C_BTNH_R 0x30
#define C_BTNH_G 0x30
#define C_BTNH_B 0x4e
#define C_BTNB_R 0x34
#define C_BTNB_G 0x34
#define C_BTNB_B 0x52

// Semantic
#define C_GREEN_R 0x2e
#define C_GREEN_G 0x7e
#define C_GREEN_B 0x4e
#define C_GREENH_R 0x3a
#define C_GREENH_G 0x92
#define C_GREENH_B 0x5a
#define C_RED_R 0xa4
#define C_RED_G 0x3e
#define C_RED_B 0x3e
#define C_REDH_R 0xb8
#define C_REDH_G 0x48
#define C_REDH_B 0x48
#define C_BLUE_R 0x2e
#define C_BLUE_G 0x64
#define C_BLUE_B 0x90
#define C_BLUEH_R 0x36
#define C_BLUEH_G 0x74
#define C_BLUEH_B 0xa4

// Text
#define C_TEXT_R 0xe0
#define C_TEXT_G 0xe0
#define C_TEXT_B 0xfc
#define C_DIM_R 0x80
#define C_DIM_G 0x84
#define C_DIM_B 0xa2
#define C_LBL_R 0x5e
#define C_LBL_G 0x9e
#define C_LBL_B 0xde
#define C_SORTED_ST_R 0x4a
#define C_SORTED_ST_G 0xd0
#define C_SORTED_ST_B 0x5a

// Bar gradient stops: blue → teal → green → yellow → peach
static const int BAR_STOPS[5][3] = {
    {137, 180, 250},
    {116, 199, 236},
    {166, 227, 161},
    {249, 226, 175},
    {250, 179, 135},
};

// ── Layout constants ────────────────────────────────────────────
#define UI_H 64
#define STATUS_H 28
#define CHART_PAD_L 16
#define CHART_PAD_R 16
#define CHART_PAD_T 14

static SDL_Renderer *g_renderer = NULL;
static TTF_Font *g_font = NULL;
static SDL_AudioDeviceID g_audio_dev = 0;
static int g_mouse_x = 0, g_mouse_y = 0;
static bool g_app_quitting = false;

// ── Helpers ──────────────────────────────────────────────────────

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

static void rtext_in_rect(SDL_Renderer *r, TTF_Font *f, const char *text,
                          SDL_Rect rect, SDL_Color col)
{
    int tw = 0;
    TTF_SizeText(f, text, &tw, NULL);
    int tx = rect.x + (rect.w - tw) / 2;
    int ty = rect.y + (rect.h - 16) / 2;
    if (tx < rect.x) tx = rect.x;
    render_text(r, f, text, tx, ty, col);
}

// ── Bevel helper ────────────────────────────────────────────────

static void draw_bevel(SDL_Renderer *r, int x, int y, int w, int h,
                       Uint8 lr, Uint8 lg, Uint8 lb,
                       Uint8 dr, Uint8 dg, Uint8 db)
{
    SDL_SetRenderDrawColor(r, lr, lg, lb, 255);
    SDL_RenderDrawLine(r, x, y, x + w - 2, y);
    SDL_RenderDrawLine(r, x, y, x, y + h - 2);
    SDL_SetRenderDrawColor(r, dr, dg, db, 255);
    SDL_RenderDrawLine(r, x + w - 1, y + 1, x + w - 1, y + h - 1);
    SDL_RenderDrawLine(r, x + 1, y + h - 1, x + w - 1, y + h - 1);
}

// ── Chevron arrows ──────────────────────────────────────────────

static void draw_chevron(SDL_Renderer *r, int cx, int cy, int sz, bool left) {
    SDL_Point pts[3];
    if (left) {
        pts[0] = (SDL_Point){ cx + sz, cy - sz };
        pts[1] = (SDL_Point){ cx - sz, cy };
        pts[2] = (SDL_Point){ cx + sz, cy + sz };
    } else {
        pts[0] = (SDL_Point){ cx - sz, cy - sz };
        pts[1] = (SDL_Point){ cx + sz, cy };
        pts[2] = (SDL_Point){ cx - sz, cy + sz };
    }
    SDL_SetRenderDrawColor(r, C_DIM_R, C_DIM_G, C_DIM_B, 255);
    // Draw twice for thicker line
    for (int pass = 0; pass < 2; pass++) {
        SDL_RenderDrawLine(r, pts[0].x, pts[0].y + pass,
                              pts[1].x, pts[1].y + pass);
        SDL_RenderDrawLine(r, pts[1].x, pts[1].y + pass,
                              pts[2].x, pts[2].y + pass);
    }
}

// ── Bar coloring ────────────────────────────────────────────────

static void bar_normal_color(SortState *s, int idx, Uint8 *r, Uint8 *g, Uint8 *b) {
    float t = (float)s->arr[idx] / s->max_val;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    float pos = t * 4;
    int seg = (int)pos;
    if (seg > 3) seg = 3;
    float frac = pos - seg;
    *r = (Uint8)((1 - frac) * BAR_STOPS[seg][0] + frac * BAR_STOPS[seg+1][0]);
    *g = (Uint8)((1 - frac) * BAR_STOPS[seg][1] + frac * BAR_STOPS[seg+1][1]);
    *b = (Uint8)((1 - frac) * BAR_STOPS[seg][2] + frac * BAR_STOPS[seg+1][2]);
}

static void get_bar_color(SortState *s, int idx, Uint8 *r, Uint8 *g, Uint8 *b) {
    // Completion flash overrides everything
    if (s->sorted && s->sorted_timer > 0 && !s->sorting) {
        float t = s->sorted_timer / 60.0f;
        Uint8 base_r = C_SORTED_ST_R, base_g = C_SORTED_ST_G, base_b = C_SORTED_ST_B;
        *r = (Uint8)(base_r + (255 - base_r) * t);
        *g = (Uint8)(base_g + (255 - base_g) * t);
        *b = (Uint8)(base_b + (255 - base_b) * t);
        return;
    }
    if (s->sorted && idx < s->sorted_until) {
        *r = C_SORTED_ST_R; *g = C_SORTED_ST_G; *b = C_SORTED_ST_B;
        return;
    }
    if (s->sorting) {
        if (idx == s->current_idx)  { *r = 0xff; *g = 0x77; *b = 0x44; return; }
        if (idx == s->current_idx2) { *r = 0xff; *g = 0x55; *b = 0x77; return; }
    }
    bar_normal_color(s, idx, r, g, b);
}

// ── Grid / scale ────────────────────────────────────────────────

static void render_chart_grid(SDL_Renderer *r, int cx, int cy, int cw, int ch)
{
    SDL_SetRenderDrawColor(r, C_GRID_R, C_GRID_G, C_GRID_B, 255);
    int lines[] = { 0, 25, 50, 75, 100 };
    for (int li = 0; li < 5; li++) {
        int yy = cy + ch - (ch * lines[li] / 100) - 1;
        if (yy < cy || yy > cy + ch - 1) continue;
        SDL_RenderDrawLine(r, cx, yy, cx + cw - 1, yy);
    }
}

// ── Bar rendering ───────────────────────────────────────────────

void render_bars(SDL_Renderer *r, SortState *s, int x, int y, int w, int h) {
    if (s->n == 0) return;
    int cx = x + CHART_PAD_L;
    int cw = w - CHART_PAD_L - CHART_PAD_R;
    int cy = y + CHART_PAD_T;
    int ch = h - CHART_PAD_T;
    if (cw < 1 || ch < 1) return;

    render_chart_grid(r, cx, cy, cw, ch);

    int bar_w = cw / s->n;
    if (bar_w < 1) bar_w = 1;
    int gap = bar_w > 14 ? 3 : (bar_w > 8 ? 2 : (bar_w > 4 ? 1 : 0));
    int bw = bar_w - gap;
    if (bw < 1) bw = 1;

    for (int i = 0; i < s->n; i++) {
        int bar_h = (s->arr[i] * (ch - 2)) / s->max_val;
        if (bar_h < 1) bar_h = 1;
        int bx = cx + i * bar_w;
        int by = cy + ch - bar_h - 1;

        Uint8 cr, cg, cb;
        get_bar_color(s, i, &cr, &cg, &cb);
        SDL_SetRenderDrawColor(r, cr, cg, cb, 255);

        if (bar_w > 2) {
            SDL_Rect rect = { bx, by, bw, bar_h };
            SDL_RenderFillRect(r, &rect);
            if (bar_w > 6) {
                SDL_SetRenderDrawColor(r, cr + 24 > 255 ? 255 : cr + 24,
                                       cg + 24 > 255 ? 255 : cg + 24,
                                       cb + 24 > 255 ? 255 : cb + 24, 255);
                SDL_RenderDrawLine(r, bx + 1, by, bx + bw - 2, by);
            }
        } else {
            SDL_RenderDrawLine(r, bx, by + bar_h - 1, bx + bw - 1, by + bar_h - 1);
        }
    }

    // Decrement completion timer
    if (s->sorted && s->sorted_timer > 0 && !s->sorting)
        s->sorted_timer--;
}

// ── UI layout ───────────────────────────────────────────────────

typedef struct {
    SDL_Rect algo_label, algo_prev, algo_name, algo_next;
    SDL_Rect size_label, size_prev, size_val, size_next;
    SDL_Rect shuffle, start;
    SDL_Rect speed_label, speed_prev, speed_val, speed_next;
} UILayout;

static void get_layout(UILayout *l) {
    int y = (UI_H - 30) / 2;
    int h = 30;
    int x = 16;
    int G = 4;
    int GG = 28;

    l->algo_label = (SDL_Rect){ x, y, 40, h }; x += 40 + G;
    l->algo_prev  = (SDL_Rect){ x, y, 28, h }; x += 28;
    l->algo_name  = (SDL_Rect){ x, y, 130, h }; x += 130;
    l->algo_next  = (SDL_Rect){ x, y, 28, h }; x += 28 + GG;

    l->size_label = (SDL_Rect){ x, y, 44, h }; x += 44 + G;
    l->size_prev  = (SDL_Rect){ x, y, 28, h }; x += 28;
    l->size_val   = (SDL_Rect){ x, y, 40, h }; x += 40;
    l->size_next  = (SDL_Rect){ x, y, 28, h }; x += 28 + GG;

    l->shuffle = (SDL_Rect){ x, y, 80, h }; x += 88;
    l->start   = (SDL_Rect){ x, y, 74, h }; x += 74 + GG;

    l->speed_label = (SDL_Rect){ x, y, 44, h }; x += 44 + G;
    l->speed_prev  = (SDL_Rect){ x, y, 28, h }; x += 28;
    l->speed_val   = (SDL_Rect){ x, y, 54, h }; x += 54;
    l->speed_next  = (SDL_Rect){ x, y, 28, h };
}

static bool pt_in_rect(int px, int py, SDL_Rect r) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

// ── UI rendering ────────────────────────────────────────────────

static void draw_arrow_btn(SDL_Renderer *r, SDL_Rect rect, bool left) {
    bool h = pt_in_rect(g_mouse_x, g_mouse_y, rect);
    fill_rect(r, rect.x, rect.y, rect.w, rect.h,
              h ? C_BTNH_R : C_BTN_R, h ? C_BTNH_G : C_BTN_G, h ? C_BTNH_B : C_BTN_B);
    draw_rect(r, rect.x, rect.y, rect.w, rect.h, C_BTNB_R, C_BTNB_G, C_BTNB_B);
    draw_bevel(r, rect.x, rect.y, rect.w, rect.h,
               h ? 0x44 : 0x34, h ? 0x44 : 0x34, h ? 0x5a : 0x46,
               h ? 0x18 : 0x14, h ? 0x18 : 0x14, h ? 0x24 : 0x1e);
    draw_chevron(r, rect.x + rect.w / 2, rect.y + rect.h / 2, 7, left);
}

static void draw_val_box(SDL_Renderer *r, TTF_Font *f, SDL_Rect rect,
                         const char *text, SDL_Color col)
{
    fill_rect(r, rect.x, rect.y, rect.w, rect.h, C_VAL_BG_R, C_VAL_BG_G, C_VAL_BG_B);
    draw_rect(r, rect.x, rect.y, rect.w, rect.h, C_VAL_BDR_R, C_VAL_BDR_G, C_VAL_BDR_B);
    draw_bevel(r, rect.x, rect.y, rect.w, rect.h,
               0x24, 0x24, 0x38,
               0x14, 0x14, 0x24);
    rtext_in_rect(r, f, text, rect, col);
}

static void draw_action_btn(SDL_Renderer *r, TTF_Font *f, SDL_Rect rect,
                            const char *text, bool hover, bool active,
                            Uint8 br, Uint8 bg, Uint8 bb,
                            Uint8 bhr, Uint8 bhg, Uint8 bhb)
{
    Uint8 fr = active ? br  : (hover ? bhr : br);
    Uint8 fg = active ? bg  : (hover ? bhg : bg);
    Uint8 fb = active ? bb  : (hover ? bhb : bb);
    fill_rect(r, rect.x, rect.y, rect.w, rect.h, fr, fg, fb);
    draw_bevel(r, rect.x, rect.y, rect.w, rect.h,
               hover ? 0x50 : 0x3c, hover ? 0x50 : 0x3c, hover ? 0x64 : 0x4e,
               hover ? 0x18 : 0x10, hover ? 0x18 : 0x10, hover ? 0x28 : 0x1c);
    SDL_Color white = { 255, 255, 255, 255 };
    rtext_in_rect(r, f, text, rect, white);
}

void render_ui(SDL_Renderer *r, TTF_Font *f, SortState *s) {
    int win_w, win_h;
    SDL_GetRendererOutputSize(r, &win_w, &win_h);
    UILayout l;
    get_layout(&l);

    // Toolbar
    fill_rect(r, 0, 0, win_w, UI_H, C_UI_BG_R, C_UI_BG_G, C_UI_BG_B);
    fill_rect(r, 0, UI_H - 1, win_w, 1, C_BDR_R, C_BDR_G, C_BDR_B);
    // Status bar
    fill_rect(r, 0, UI_H, win_w, STATUS_H, C_STAT_R, C_STAT_G, C_STAT_B);
    fill_rect(r, 0, UI_H + STATUS_H - 1, win_w, 1, C_BDR_R, C_BDR_G, C_BDR_B);

    SDL_Color text_col  = { C_TEXT_R, C_TEXT_G, C_TEXT_B, 255 };
    SDL_Color dim_col   = { C_DIM_R, C_DIM_G, C_DIM_B, 255 };
    SDL_Color label_col = { C_LBL_R, C_LBL_G, C_LBL_B, 255 };

    // ── Algo ──
    {
        int ly = l.algo_label.y + (l.algo_label.h - 16) / 2;
        render_text(r, f, "Algo", l.algo_label.x + 2, ly, label_col);
    }
    draw_arrow_btn(r, l.algo_prev, true);
    {
        fill_rect(r, l.algo_name.x, l.algo_name.y, l.algo_name.w, l.algo_name.h,
                  0x18, 0x18, 0x30);
        draw_rect(r, l.algo_name.x, l.algo_name.y, l.algo_name.w, l.algo_name.h,
                  0x30, 0x30, 0x4c);
        draw_bevel(r, l.algo_name.x, l.algo_name.y, l.algo_name.w, l.algo_name.h,
                   0x24, 0x24, 0x38,
                   0x14, 0x14, 0x24);
        rtext_in_rect(r, f, ALGO_NAMES[s->algo], l.algo_name, text_col);
    }
    draw_arrow_btn(r, l.algo_next, false);

    // ── Items ──
    {
        int ly = l.size_label.y + (l.size_label.h - 16) / 2;
        render_text(r, f, "Items", l.size_label.x + 2, ly, label_col);
    }
    draw_arrow_btn(r, l.size_prev, true);
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", s->n);
        draw_val_box(r, f, l.size_val, buf, text_col);
    }
    draw_arrow_btn(r, l.size_next, false);

    // ── Shuffle ──
    {
        bool h = pt_in_rect(g_mouse_x, g_mouse_y, l.shuffle);
        draw_action_btn(r, f, l.shuffle, "Shuffle",
                        h, s->sorting,
                        C_BLUE_R, C_BLUE_G, C_BLUE_B,
                        C_BLUEH_R, C_BLUEH_G, C_BLUEH_B);
    }

    // ── Start / Stop ──
    {
        bool h = pt_in_rect(g_mouse_x, g_mouse_y, l.start);
        if (s->sorting) {
            draw_action_btn(r, f, l.start, "Stop", h, false,
                            C_RED_R, C_RED_G, C_RED_B,
                            C_REDH_R, C_REDH_G, C_REDH_B);
        } else {
            draw_action_btn(r, f, l.start, "Sort!", h, false,
                            C_GREEN_R, C_GREEN_G, C_GREEN_B,
                            C_GREENH_R, C_GREENH_G, C_GREENH_B);
        }
    }

    // ── Delay ──
    {
        int ly = l.speed_label.y + (l.speed_label.h - 16) / 2;
        render_text(r, f, "Delay", l.speed_label.x + 2, ly, label_col);
    }
    draw_arrow_btn(r, l.speed_prev, true);
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%dms", s->delay_ms);
        draw_val_box(r, f, l.speed_val, buf, text_col);
    }
    draw_arrow_btn(r, l.speed_next, false);

    // ── Status bar ──
    {
        int sy = UI_H + (STATUS_H - 16) / 2;
        char buf[80];

        // Metrics — slightly brighter than before
        SDL_Color cmp_col = { C_DIM_R + 20, C_DIM_G + 20, C_DIM_B + 20, 255 };
        snprintf(buf, sizeof(buf), "Cmp: %d", s->comparisons);
        render_text(r, f, buf, 14, sy, cmp_col);
        snprintf(buf, sizeof(buf), "Swp: %d", s->swaps);
        render_text(r, f, buf, 130, sy, cmp_col);

        // Status — green when sorted
        SDL_Color st_col = dim_col;
        const char *st;
        if (s->sorting) {
            st = "Sorting...";
        } else if (s->sorted) {
            st = "Sorted!";
            SDL_Color green_st = { C_SORTED_ST_G, C_SORTED_ST_G, C_SORTED_ST_B, 255 };
            st_col = green_st;
        } else {
            st = "Ready";
        }
        snprintf(buf, sizeof(buf), "Status: %s", st);
        render_text(r, f, buf, 250, sy, st_col);

        // Color legend
        int lx = 470;
        SDL_Color leg_lab = { C_DIM_R + 10, C_DIM_G + 10, C_DIM_B + 10, 255 };
        SDL_Color leg_uns  = { BAR_STOPS[2][0], BAR_STOPS[2][1], BAR_STOPS[2][2], 255 };
        SDL_Color leg_cmp  = { 0xff, 0x77, 0x44, 255 };
        SDL_Color leg_srt  = { C_SORTED_ST_R, C_SORTED_ST_G, C_SORTED_ST_B, 255 };
        int sw = 12;
        int shy = sy + 2;

        SDL_SetRenderDrawColor(r, leg_uns.r, leg_uns.g, leg_uns.b, 255);
        SDL_Rect r1 = { lx, shy, sw, sw }; SDL_RenderFillRect(r, &r1);
        render_text(r, f, "unsorted", lx + sw + 6, sy, leg_lab);
        lx += sw + 6 + 84;

        SDL_SetRenderDrawColor(r, leg_cmp.r, leg_cmp.g, leg_cmp.b, 255);
        SDL_Rect r2 = { lx, shy, sw, sw }; SDL_RenderFillRect(r, &r2);
        render_text(r, f, "comparing", lx + sw + 6, sy, leg_lab);
        lx += sw + 6 + 96;

        SDL_SetRenderDrawColor(r, leg_srt.r, leg_srt.g, leg_srt.b, 255);
        SDL_Rect r3 = { lx, shy, sw, sw }; SDL_RenderFillRect(r, &r3);
        render_text(r, f, "sorted", lx + sw + 6, sy, leg_lab);
    }
}

// ── Audio ───────────────────────────────────────────────────────

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
    if (SDL_GetQueuedAudioSize(dev) < 44100)
        SDL_QueueAudio(dev, buf, nsamples * sizeof(Sint16));
    free(buf);
}

// ── on_sort_step ───────────────────────────────────────────────

void on_sort_step(SortState *s) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { s->quit = true; g_app_quitting = true; }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            s->quit = true;
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            UILayout layout;
            get_layout(&layout);
            if (pt_in_rect(e.button.x, e.button.y, layout.start))
                s->quit = true;
        }
        if (e.type == SDL_MOUSEMOTION) {
            g_mouse_x = e.motion.x;
            g_mouse_y = e.motion.y;
        }
    }

    if (s->delay_ms <= 3 && s->sorting) {
        static int skip = 49;
        if (++skip % 50 != 0)
            return;
    }

    int w, h;
    SDL_GetRendererOutputSize(g_renderer, &w, &h);
    SDL_SetRenderDrawColor(g_renderer, C_BG_R, C_BG_G, C_BG_B, 255);
    SDL_RenderClear(g_renderer);

    int bars_y = UI_H + STATUS_H;
    int bars_h = h - bars_y;
    render_bars(g_renderer, s, 0, bars_y, w, bars_h);
    render_ui(g_renderer, g_font, s);
    SDL_RenderPresent(g_renderer);

    if (s->current_idx >= 0 && s->current_idx < s->n) {
        int freq = 200 + (s->arr[s->current_idx] * 800) / (s->max_val > 0 ? s->max_val : 1);
        audio_play_tone(g_audio_dev, freq, s->delay_ms);
    }

    if (s->delay_ms > 0)
        SDL_Delay(s->delay_ms);
}

// ── Public helpers ──────────────────────────────────────────────

void renderer_init(SDL_Renderer *r, TTF_Font *f, SDL_AudioDeviceID a) {
    g_renderer = r; g_font = f; g_audio_dev = a;
}

int ui_bar_height(void) { return UI_H + STATUS_H; }

void track_mouse(int mx, int my) { g_mouse_x = mx; g_mouse_y = my; }

bool app_should_quit(void) { return g_app_quitting; }
void clear_quit_flag(void) { g_app_quitting = false; }

bool handle_ui_click(SortState *s, int mx, int my) {
    if (s->sorting) return false;
    UILayout l;
    get_layout(&l);

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
        int step = s->delay_ms >= 200 ? 25 : s->delay_ms >= 100 ? 10 : s->delay_ms >= 20 ? 5 : s->delay_ms >= 5 ? 2 : 1;
        s->delay_ms = s->delay_ms - step;
        if (s->delay_ms < 1) s->delay_ms = 1;
        return false;
    }
    if (pt_in_rect(mx, my, l.speed_next)) {
        int step = s->delay_ms >= 200 ? 25 : s->delay_ms >= 100 ? 10 : s->delay_ms >= 20 ? 5 : s->delay_ms >= 5 ? 2 : 1;
        s->delay_ms = s->delay_ms + step;
        if (s->delay_ms > 500) s->delay_ms = 500;
        return false;
    }

    if (pt_in_rect(mx, my, l.shuffle)) {
        shuffle_array(s);
        return false;
    }

    return pt_in_rect(mx, my, l.start);
}
