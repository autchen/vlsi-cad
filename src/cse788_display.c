
#include "cse788_display.h"

#include "cse788_gordian.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>

#define CHECK_ERROR(E) \
        if (E) { \
                ret = __LINE__; \
                break; \
        }

#define MAX_WIN 100
#define BORDER_X 50
#define BORDER_Y 50

struct cse788_display_t {
        int w, h;
        int level;
        SDL_Window *wins[MAX_WIN];
        SDL_Renderer *rends[MAX_WIN];
};

struct cse788_display_t *cse788_display_new(int w, int h)
{
        int ret = 0;
        struct cse788_display_t *t;

        do {
                ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
                CHECK_ERROR(ret != 0);
                t = (struct cse788_display_t *)malloc(sizeof *t);
                CHECK_ERROR(t == NULL);
                t->w = w;
                t->h = h;
                t->level = -1;
        } while (0);
        if (ret != 0) {
                printf("display error: %d\n", ret);
                return NULL;
        }

        return t;
}

void cse788_display_del(struct cse788_display_t *t)
{
        int i;
        for (i = 0; i < t->level; i++) {
                SDL_DestroyRenderer(t->rends[i]);
                SDL_DestroyWindow(t->wins[i]);
        }
        free(t);
        SDL_Quit();
}

static int cse788_display_inclevel(void *inst)
{
        int ret = 0;
        struct cse788_display_t *t = (struct cse788_display_t *)inst;
        char a[10];

        do {
                t->level++;
                sprintf(a, "%d", t->level);
                t->wins[t->level] = SDL_CreateWindow(a, 50, 50, 
                                2 * (t->w + 2 * BORDER_X), 
                                t->h + 2 * BORDER_Y, 0);
                CHECK_ERROR(t->wins[t->level] == NULL);
                t->rends[t->level] = SDL_CreateRenderer(t->wins[t->level], -1, 
                                SDL_RENDERER_ACCELERATED);
                CHECK_ERROR(t->rends[t->level] == NULL);
                SDL_RenderClear(t->rends[t->level]);
                SDL_SetRenderDrawColor(t->rends[t->level], 255, 255, 255, 255);
                SDL_RenderFillRect(t->rends[t->level], NULL);
        } while (0);

        return ret;
}

#define TRANS(cl, dl, x) \
        (int)(((x) / (cl)) * (double)(dl))

static int 
cse788_display_postcell(void *inst, int ID, int movable, double cw, double ch,
                        double dimx, double dimy, double posx, double posy)
{
        int ret = 0;
        struct cse788_display_t *t = (struct cse788_display_t *)inst;

        do {
                SDL_Renderer *rend = t->rends[t->level];
                SDL_Rect rect;
                
                if (movable)
                        SDL_SetRenderDrawColor(rend, 0, 0, 0, 255);
                else
                        SDL_SetRenderDrawColor(rend, 0, 0, 255, 255);
                rect.x = TRANS(cw, t->w, (posx - dimx / 2.0)) + BORDER_X;
                rect.y = t->h - TRANS(ch, t->h, (posy + dimy / 2.0)) + BORDER_Y;
                rect.w = TRANS(cw, t->w, dimx);
                rect.h = TRANS(ch, t->h, dimy);
                ret = SDL_RenderDrawRect(rend, &rect);
                CHECK_ERROR(ret != 0);
                rect.x += t->w + 2 * BORDER_X;
                ret = SDL_RenderDrawRect(rend, &rect);
                CHECK_ERROR(ret != 0);
        } while (0);

        return ret;
}

static int 
cse788_display_postline(void *inst, double cw, double ch, 
                        double srcx, double srcy, double dstx, double dsty)
{
        int ret = 0;
        struct cse788_display_t *t = (struct cse788_display_t *)inst;

        do {
                SDL_Renderer *rend = t->rends[t->level];
                int x1, y1, x2, y2;
                
                SDL_SetRenderDrawColor(rend, 128, 128, 128, 255);
                x1 = TRANS(cw, t->w, srcx) + BORDER_X;
                y1 = t->h - TRANS(ch, t->h, srcy) + BORDER_Y;
                x2 = TRANS(cw, t->w, dstx) + BORDER_X;
                y2 = t->h - TRANS(ch, t->h, dsty) + BORDER_Y;
                x1 += t->w + 2 * BORDER_X;
                x2 += t->w + 2 * BORDER_X;
                ret = SDL_RenderDrawLine(rend, x1, y1, x2, y2);
                CHECK_ERROR(ret != 0);
        } while (0);

        return ret;
}

static int 
cse788_display_postpart(void *inst, double cw, double ch, 
                        double x, double y, double w, double h)
{
        int ret = 0;
        struct cse788_display_t *t = (struct cse788_display_t *)inst;

        do {
                SDL_Renderer *rend = t->rends[t->level];
                SDL_Rect rect;
                
                SDL_SetRenderDrawColor(rend, 0, 255, 0, 255);
                rect.x = TRANS(cw, t->w, x) + BORDER_X;
                rect.y = t->h - TRANS(ch, t->h, y + h) + BORDER_Y;
                rect.w = TRANS(cw, t->w, w);
                rect.h = TRANS(ch, t->h, h);
                ret = SDL_RenderDrawRect(rend, &rect);
                CHECK_ERROR(ret != 0);
                rect.x += t->w + 2 * BORDER_X;
                ret = SDL_RenderDrawRect(rend, &rect);
                CHECK_ERROR(ret != 0);
        } while (0);

        return ret;
}

static int cse788_display_present(void *inst)
{
        int ret = 0;
        struct cse788_display_t *t = (struct cse788_display_t *)inst;

        do {
                SDL_Renderer *rend = t->rends[t->level];
                SDL_RenderPresent(rend);
                CHECK_ERROR(ret != 0);
        } while (0);

        return ret;
}

struct cse788_gordian_report_i *
cse788_display_gordian_interface(struct cse788_display_t *t)
{
        struct cse788_gordian_report_i *r;

        r = (struct cse788_gordian_report_i *)malloc(sizeof *r);
        r->inst = t;
        r->inclevel = cse788_display_inclevel;
        r->postcell = cse788_display_postcell;
        r->postline = cse788_display_postline;
        r->postpart = cse788_display_postpart;
        r->present = cse788_display_present;

        return r;
}

void cse788_display_hold(struct cse788_display_t *t)
{
        while (1) {
                SDL_Event e;
                if (SDL_PollEvent(&e)) {
                        if (e.type == SDL_WINDOWEVENT) {
                                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                                        break;
                                }
                        }
                }
                SDL_Delay(20);
        }
}

#include "cse788_floorplan.h"
#include "plus/pie_assert.h"
#include "plus/pie_mem.h"

struct cse788_display2_t {
        struct cse788_floorplan_report_i it;
        int w, h;
        double cw, ch;
        SDL_Window *win;
        SDL_Renderer *rend;
        SDL_Texture *tex;
};

static void cse788_display2_plotLimit(void *inst, double cw, double ch)
{
        int ret;
        struct cse788_display2_t *t;
        SDL_Rect rect;

        t = (struct cse788_display2_t *)inst;
        PIE_ASSERT(t->it.plotLimit == cse788_display2_plotLimit);
        PIE_ASSERT(cw > 0.0 && ch > 0.0);

        if (t->win == PIE_NULL) {
                FILE *fp;
                SDL_RWops *rw;
                SDL_Surface *face;

                t->win = SDL_CreateWindow("floorplan", 50, 50, 
                                t->w + 2 * BORDER_X, 
                                t->h + 2 * BORDER_Y, 0);
                PIE_ASSERT_RELEASE(t->win);
                t->rend = SDL_CreateRenderer(t->win, -1, 
                                SDL_RENDERER_ACCELERATED);
                fp = fopen("number.bmp", "r");
                rw = SDL_RWFromFP(fp, 1);
                face = SDL_LoadBMP_RW(rw, 1);
                PIE_ASSERT_RELEASE(face);
                t->tex = SDL_CreateTextureFromSurface(t->rend, face);
                PIE_ASSERT_RELEASE(t->tex);
                SDL_FreeSurface(face);
        }
        
        SDL_RenderClear(t->rend);
        SDL_SetRenderDrawColor(t->rend, 255, 255, 255, 255);
        SDL_RenderFillRect(t->rend, NULL);

        t->cw = cw;
        t->ch = ch;
        SDL_SetRenderDrawColor(t->rend, 0, 255, 0, 255);
        rect.x = TRANS(cw, t->w, 0) + BORDER_X - 1;
        rect.y = t->h - TRANS(ch, t->h, ch) + BORDER_Y - 1;
        rect.w = TRANS(cw, t->w, cw) + 2;
        rect.h = TRANS(ch, t->h, ch) + 2;
        ret = SDL_RenderDrawRect(t->rend, &rect);
        PIE_ASSERT_RELEASE(ret == 0);
}

static void 
cse788_display2_plotSlice(void *inst, double x, double y, double w, double h)
{
        int ret;
        struct cse788_display2_t *t;
        SDL_Rect rect;

        t = (struct cse788_display2_t *)inst;
        PIE_ASSERT(t->it.plotLimit == cse788_display2_plotLimit);
        PIE_ASSERT(w > 0 && h > 0);

        SDL_SetRenderDrawColor(t->rend, 0, 0, 255, 255);
        rect.x = TRANS(t->cw, t->w, x) + BORDER_X;
        rect.y = t->h - TRANS(t->ch, t->h, y + h) + BORDER_Y;
        rect.w = TRANS(t->cw, t->w, w);
        rect.h = TRANS(t->ch, t->h, h);
        ret = SDL_RenderDrawRect(t->rend, &rect);
        PIE_ASSERT_RELEASE(ret == 0);

        SDL_SetRenderDrawColor(t->rend, 192, 192, 192, 255);
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
        ret = SDL_RenderFillRect(t->rend, &rect);
        PIE_ASSERT_RELEASE(ret == 0);
}

#define DH 70
#define DW 50

static void 
cse788_display2_plotModule(void *inst, double x, double y, double w, double h,
                           int index)
{
        int ret, i, dw, dh;
        struct cse788_display2_t *t;
        SDL_Rect rect;
        /* SDL_Surface *text_face; */
        /* SDL_Color text_color = {0, 0, 0}; */
        /* SDL_Texture *text_tex; */
        char text[20], *str;

        t = (struct cse788_display2_t *)inst;
        PIE_ASSERT(t->it.plotLimit == cse788_display2_plotLimit);
        PIE_ASSERT(w > 0 && h > 0);

        SDL_SetRenderDrawColor(t->rend, 255, 0, 0, 255);
        rect.x = TRANS(t->cw, t->w, x) + BORDER_X + 1;
        rect.y = t->h - TRANS(t->ch, t->h, y + h) + BORDER_Y + 1;
        rect.w = TRANS(t->cw, t->w, w) - 2;
        rect.h = TRANS(t->ch, t->h, h) - 2;
        ret = SDL_RenderDrawRect(t->rend, &rect);
        PIE_ASSERT_RELEASE(ret == 0);

        SDL_SetRenderDrawColor(t->rend, 255, 255, 255, 255);
        rect.x += 1;
        rect.y += 1;
        rect.w -= 2;
        rect.h -= 2;
        ret = SDL_RenderFillRect(t->rend, &rect);
        PIE_ASSERT_RELEASE(ret == 0);


        sprintf(text, "%d", index);
        str = text;
        i = strlen(str);
        if (rect.w >= rect.h * i * (1.0 * DW / DH)) {
                dh = rect.h;
                dw = dh * (1.0 * DW / DH);
        } else {
                dw = rect.w / i;
                dh = dw * (1.0 * DH / DW);
        }
        i = 0;
        while(*str) {
                SDL_Rect src, dst;
                int a = *str - '0';
                src.x = DW * a;
                src.y = 0;
                src.w = DW;
                src.h = DH;
                dst.x = rect.x + dw * i;
                dst.y = rect.y;
                dst.w = dw;
                dst.h = dh;
                SDL_RenderCopy(t->rend, t->tex, &src, &dst);
                i++;
                str++;
        }
        /* text_face = TTF_RenderText_Solid(t->font, text, text_color); */
        /* text_tex = SDL_CreateTextureFromSurface(t->rend, text_face); */
        /* SDL_FreeSurface(text_face); */
        /* rect.x += 5; */
        /* rect.y += 5; */
        /* rect.w = 8; */
        /* rect.h = 8; */
        /* SDL_RenderCopy(t->rend, text_tex, PIE_NULL, &rect); */
        /* SDL_DestroyTexture(text_tex); */
}

static void cse788_display2_present(void *inst)
{
        struct cse788_display2_t *t = (struct cse788_display2_t *)inst;
        SDL_RenderPresent(t->rend);
        SDL_Delay(300);
}

struct cse788_display2_t *cse788_display2_new(int w, int h)
{
        /* int ret = 0; */
        /* FILE *font_file; */
        /* SDL_RWops *font_abs; */
        struct cse788_display2_t *t;
        
                SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
        /* ret = TTF_Init(); */
        /* PIE_ASSERT_RELEASE(ret == 0); */

        t = PIE_NEW(struct cse788_display2_t);
        t->w = w;
        t->h = h;

        t->win = PIE_NULL;
        t->rend = PIE_NULL;
        /* font_file = fopen("resources/FreeSans.ttf", "r"); */
        /* PIE_ASSERT_RELEASE(font_file); */
        /* font_abs = SDL_RWFromFP(font_file, 1); */
        /* PIE_ASSERT_RELEASE(font_abs); */
        /* t->font = TTF_OpenFontRW(font_abs, 1, 12); */
        /* printf("%s\n", TTF_GetError()); */
        /* PIE_ASSERT_RELEASE(t->font); */

        /* PIE_ASSERT_RELEASE(t->rend); */
        t->it.inst = t;
        t->it.plotLimit = cse788_display2_plotLimit;
        t->it.plotSlice = cse788_display2_plotSlice;
        t->it.plotModule = cse788_display2_plotModule;
        t->it.present = cse788_display2_present;

        return t;
}

void cse788_display2_del(struct cse788_display2_t **t)
{
        if ((*t)->win) {
                SDL_DestroyTexture((*t)->tex);
                SDL_DestroyRenderer((*t)->rend);
                SDL_DestroyWindow((*t)->win);
        }
}

struct cse788_floorplan_report_i *
cse788_display2_floorplan_interface(struct cse788_display2_t *t)
{
        return &t->it;
}
