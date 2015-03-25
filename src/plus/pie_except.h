/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_EXCEPT_H_INCLUDED
#define PIE_EXCEPT_H_INCLUDED

/**
 * @package plus
 * @file pie_except.h
 * @brief Exception handling module.
 */

#include "pie_config.h"
#include <setjmp.h>

/**
 * Type definitions
 */

typedef struct PieExcept {
        const char *name;
} PieExcept;

typedef struct PieExceptFrame {
        struct PieExceptFrame *prev;
        jmp_buf env;
        const struct PieExcept *except;
        const char *file;
        int line;
        const char *info;
        struct PieExceptFrame **head;
} PieExceptFrame;

/* Exproted exception */

extern const struct PieExcept PieExcept_enrollFailed;

/**
 * Macros
 */

#if PIE_DEBUG_LEVEL > 0

#define PIE_RAISE(E) \
        PieExcept_raise(&(E), __FILE__, __LINE__, PIE_NULL)

#define PIE_RAISE_INFO(E, V) \
        PieExcept_raise(&(E), __FILE__, __LINE__, PieExcept_setInfo V)

#define PIE_TRY \
        do { \
                struct PieExceptFrame _caught; \
                PieExcept_pushFrame(&_caught); \
                if (!setjmp(_caught.env)) { \
                
#define PIE_CATCH(E, N) \
                } else if (&(E) == _caught.except) { \
                        struct PieExceptFrame *N = &_caught;
                
#define PIE_CATCH_ELSE(N) \
                } else if (_caught.except) { \
                        struct PieExceptFrame *N = &_caught; \

#define PIE_TRIED \
                } else \
                        PIE_RERAISE(&_caught); \
                *(_caught.head) = _caught.prev; \
        } while (0)

#define PIE_RERAISE(F) \
        PieExcept_raise((F)->except, (F)->file, (F)->line, (F)->info)

#else
#define PIE_RAISE(E)         ((void)0)
#define PIE_RAISE_INFO(E, V) ((void)0)
#define PIE_TRY              ((void)0)
#define PIE_CATCH(E, N)      ((void)0)
#define PIE_CATCH_ELSE(N)    ((void)0)
#define PIE_TRIED            ((void)0)
#define PIE_RERAISE(F)       ((void)0)

#endif /* PIE_DEBUG_LEVEL */

/**
 * Public methods for exception handling
 */

extern void 
PieExcept_raise(const struct PieExcept *except, const char *file, int line, 
                const char *info);

extern void PieExcept_pushFrame(struct PieExceptFrame *frame);

/**
 * Error message managements
 */

extern const char *PieExcept_setInfo(const char *fmt, ...);

extern const char *PieExcept_getInfo(char *buf, int len);

/**
 * Methods for resource reclaim upon exception raised
 */

extern PieErr PieExcept_enroll(void *resource, void (*dtor)(void *));

extern void PieExcept_drop(void *resource);

extern void PieExcept_reclaim(struct PieExceptFrame *frame);

#endif /* PIE_EXCEPT_H_INCLUDED */


