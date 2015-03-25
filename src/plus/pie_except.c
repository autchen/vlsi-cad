/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_except.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if PIE_DEBUG_LEVEL <= 1
#define NDEBUG
#endif /* PIE_DEBUG_LEVEL */

#include <assert.h>

#define INFO_BUF_LEN 1024
#define RESTABLE_LEN 2048
#define MAP_RES(P) (((PieUptr)(P) >> 3) % RESTABLE_LEN)
#define EXPAND_SIZE 512

const struct PieExcept PieExcept_enrollFailed = {
        "Failed to Enroll Resource with Exception Waypoint"};

struct PieExceptRes{
        struct PieExceptRes*link;
        void *resource;
        void (*dtor)(void *);
        struct PieExceptFrame *tag;
};

struct PieExceptContext {
        struct PieExceptFrame *head;
        char infoBuf[INFO_BUF_LEN];
        struct PieExceptRes *res[RESTABLE_LEN];
        struct PieExceptRes *freelist;
        PieBool lock;
};

#if PIE_MULTI_THREADED == 0
static struct PieExceptContext _context;
#define GET_CONTEXT (&_context);

#else
/* TODO */

#endif /* PIE_MULTI_THREADED */

extern void 
PieExcept_raise(const struct PieExcept *except, const char *file, int line, 
                const char *info)
{
        struct PieExceptContext *ctx = GET_CONTEXT;
        struct PieExceptFrame *frame = ctx->head;

        assert(except);
        if (PIE_NULL == frame) {
                fprintf(stderr, "Uncaught Exception: ");
                if (except->name)
                        fprintf(stderr, "%s\n", except->name);
                else
                        fprintf(stderr, "at 0x%p\n", except);
                if (info)
                        fprintf(stderr, "\tinfo: %s\n", info);
                if (file && line > 0)
                        fprintf(stderr, "\traised at %s:%d\n", file, line);
                fprintf(stderr, "Program exiting ...\n");
                fflush(stderr);
                exit(3);
        }

        frame->except = except;
        frame->info = info;
        frame->file = file;
        frame->line = line;
        ctx->head = frame->prev;

        longjmp(frame->env, 1);
}

void PieExcept_pushFrame(struct PieExceptFrame *frame)
{
        struct PieExceptContext *ctx = GET_CONTEXT;
        assert(frame);
        frame->prev = ctx->head;
        ctx->head = frame;
        frame->except = PIE_NULL;
        frame->info = PIE_NULL;
        frame->file = PIE_NULL;
        frame->line = 0;
        frame->head = &(ctx->head);
}

PieErr PieExcept_enroll(void *resource, void (*dtor)(void *))
{
        int idx;
        struct PieExceptRes *res;
        struct PieExceptContext *ctx;

        assert(resource && dtor);
        ctx = GET_CONTEXT;
        idx = MAP_RES(resource);
        
        res = ctx->res[idx];
        while (res) {
                if (res->resource == resource) {
                        res->dtor = dtor;
                        return PIE_OK;
                }
                res = res->link;
        }

        if (PIE_NULL == ctx->freelist) {
                int i;
                res = (struct PieExceptRes *)calloc(EXPAND_SIZE, sizeof *res);
                if (PIE_NULL == res)
                        return &PieExcept_enrollFailed;
                ctx->freelist = res;
                for (i = 0; i < EXPAND_SIZE - 1; i++) {
                        res->link = res + 1;
                        res++; 
                }
                res->link = PIE_NULL;
        }
        res = ctx->freelist;
        ctx->freelist = res->link;
        
        res->resource = resource;
        res->dtor = dtor;
        res->tag = ctx->head;
        res->link = ctx->res[idx];
        ctx->res[idx] = res;

        return PIE_OK;
}

void PieExcept_drop(void *resource)
{
        int idx;
        struct PieExceptContext *ctx;
        struct PieExceptRes *res, *prev = PIE_NULL;

        assert(resource);
        ctx = GET_CONTEXT;
        if (PIE_TRUE == ctx->lock)
                return;
        idx = MAP_RES(resource);
        res = ctx->res[idx];
        while (res) {
                if (res->resource == resource) {
                        if (PIE_NULL == prev)
                                ctx->res[idx] = res->link;
                        else
                                prev->link = res->link;
                        res->link = ctx->freelist;
                        ctx->freelist = res;
                        return;
                }
                prev = res;
                res = res->link;
        }
}

void PieExcept_reclaim(struct PieExceptFrame *frame) {
        int i;
        struct PieExceptContext *ctx = GET_CONTEXT;
        assert(frame);
        ctx->lock = PIE_TRUE;
        for (i = 0; i < RESTABLE_LEN; i++) {
                struct PieExceptRes *res = ctx->res[i];
                struct PieExceptRes *prev = PIE_NULL;
                while (res) {
                        if (res->tag == frame) {
                                res->dtor(res->resource);
                                if (PIE_NULL == prev)
                                        ctx->res[i] = res->link;
                                else
                                        prev->link = res->link;
                                res->link = ctx->freelist;
                                ctx->freelist = res;
                                res = prev ? prev->link : ctx->res[i];
                        } else {
                                prev = res;
                                res = res->link;
                        }
                }
        }
        ctx->lock = PIE_FALSE;
}

#include "plus/pie_fmt.h"
#include "plus/pie_assert.h"

const char *PieExcept_setInfo(const char *fmt, ...)
{
        struct PieExceptContext *ctx = GET_CONTEXT;
        if (fmt) {
                va_list ap;
                va_start(ap, fmt);
                if (INFO_BUF_LEN 
                        == PieFmt_vstr(ctx->infoBuf, INFO_BUF_LEN, fmt, ap))
                        ctx->infoBuf[INFO_BUF_LEN - 1] = '\0';
                va_end(ap);
        } else
                ctx->infoBuf[0] = '\0'; /* Clear the buffer */
        return ctx->infoBuf;
}

const char *PieExcept_getInfo(char *buf, int len)
{
        struct PieExceptContext *ctx = GET_CONTEXT;
        if (PIE_NULL == buf || 0 == len)
                return ctx->infoBuf;
        strncpy(buf, ctx->infoBuf, len);
        return buf;
}
 
/**
 * Formatted output function; this will be automatically registered with
 * PieFmt module on program started.
 * @see plus/pie_fmt.c
 */
void 
PieExcept_fmt(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
              PieFmtFlag flag, int width, int precision)
{
        struct PieExceptFrame *frame;

        PIE_ASSERT_EXTREME('E' == code);
        PIE_ASSERT_EXTREME(app && put);
        frame = va_arg(*app, struct PieExceptFrame *);

        PIE_ASSERT(frame && frame->except);
        if (frame->except->name)
                PieFmt_out(put, cl, "Exception: %s\n", frame->except->name);
        else
                PieFmt_out(put, cl, "Exception: at %#p\n", frame->except);
        if (frame->info)
                PieFmt_out(put, cl, "\tinfo: %s\n", frame->info);
        if (frame->file && frame->line > 0) {
                PieFmt_out(put, cl, "\traised at %s:%d\n", frame->file, 
                           frame->line);
        }
}
