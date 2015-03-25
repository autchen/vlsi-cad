/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_POOL_H_INCLUDED
#define PIE_POOL_H_INCLUDED

/**
 * @package plus
 * @file pie_pool.h
 * @brief Pool for fixed sized objects.
 */

#include "pie_config.h"

/**
 * Entity definition
 */

typedef struct PiePool PiePool;

/**
 * Exproted exception
 */

extern const struct PieExcept PieMem_failed;

/**
 * Public methods
 */

#define PIE_POOL_ALLOC(P) PiePool_alloc((P), __FILE__, __LINE__);

#define PIE_POOL_FREE(P, PT) ((void)(PiePool_free((P), (PT)), (PT) = PIE_NULL))

extern struct PiePool *PiePool_new(long unit, long expand);

extern void PiePool_del(struct PiePool **self);

extern void *PiePool_alloc(struct PiePool *self, const char *file, int line);

extern void PiePool_free(struct PiePool *self, void *ptr);

extern long PiePool_setExpand(struct PiePool *self, long expand);

extern long PiePool_avail(struct PiePool *self);

extern long PiePool_total(struct PiePool *self);

#endif /* PIE_POOL_H_INCLUDED */


