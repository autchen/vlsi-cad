/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_LIST_H_INCLUDED
#define PIE_LIST_H_INCLUDED

/**
 * @package plus
 * @file pie_list.h
 * @brief Dynamic list holding object pointers
 */

#include "pie_config.h"

/* Entity typedef */

typedef struct PieList PieList;

typedef struct PieListRep {
        int len, size;
        void **ary;
} PieListRep;

/* Exported exception */

extern const struct PieExcept PieMem_failed;
extern const struct PieExcept PieList_expandFailed;

struct PieExcept;

/* Public methods */

extern struct PieList *PieList_new(int size);

extern void PieList_del(struct PieList **self);

extern int PieList_length(struct PieList *self);

extern int PieList_size(struct PieList *self);

extern void *PieList_set(struct PieList *self, int idx, void *val);

extern void *PieList_get(struct PieList *self, int idx);

extern void *PieList_push(struct PieList *self, void *val);

extern void *PieList_pop(struct PieList *self, void *out);

extern void *PieList_insert(struct PieList *self, int idx, void *val);

extern void *PieList_remove(struct PieList *self, int idx, void *out);

extern int PieList_removeVal(struct PieList *self, void *val);

extern int PieList_find(struct PieList *self, void *val, int num);

extern void PieList_clean(struct PieList *self);

extern void PieList_truncate(struct PieList *self, int len);

extern void PieList_trimToSize(struct PieList *self);

extern const struct PieListRep *PieList_rep(struct PieList *self);

#endif


