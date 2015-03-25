/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_ARENA_H_INCLUDED
#define PIE_ARENA_H_INCLUDED

/**
 * @package plus
 * @file pie_arena.h
 * @brief Arena for simple life time managements.
 */

#include "pie_config.h"

/* Exported exceptions */

extern const struct PieExcept PieMem_failed;

/* Entity def */

typedef struct PieArena PieArena;
typedef struct PieArenaSave PieArenaSave;

/* Macros */

#define PIE_ARENA_MALLOC(A, S) \
        PieArena_malloc((A), (S), __FILE__, __LINE__)

#define PIE_ARENA_CALLOC(A, N, S) \
        PieArena_calloc((A), (N), (S), __FILE__, __LINE__)

/* Public methods */

extern struct PieArena *PieArena_new(long init, long expand);

extern void PieArena_del(struct PieArena **self);

extern void *
PieArena_malloc(struct PieArena *self, long size, const char *file, int line);

extern void *
PieArena_calloc(struct PieArena *self, long num, long size, const char *file, 
                int line);

extern long PieArena_setExpand(struct PieArena *self, long expand);

extern struct PieArenaSave *PieArena_save(struct PieArena *arena);

extern struct PieArena *PieArena_restore(struct PieArenaSave *save);

extern void PieArena_log(struct PieArena *self);

#endif


