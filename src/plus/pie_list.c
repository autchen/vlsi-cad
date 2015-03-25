/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_list.h"

#include "plus/pie_mem.h"
#include "plus/pie_assert.h"
#include "plus/pie_arena.h"

#include <limits.h>
#include <string.h>

#define LIST_INISIZE 16
#define LIST_CAPMAX (INT_MAX / 2)

/* Local exception */

const struct PieExcept PieList_expandFailed = 
                {"List capacity expansion failed"};

/* Data struct */

struct PieList {
        int len, size;
        void **ary;
        int cap;
        struct PieArena *arena;
};

static void PieList_expand(struct PieList *self)
{
        int i;
        PIE_ASSERT_EXTREME(self);
        if (LIST_CAPMAX <= self->cap) {
                PIE_RAISE_INFO(PieList_expandFailed, (
                               "Array capacity (%d * 2) exceeds max support",
                               self->cap ));
        }
        int newcap = self->cap * 2;
        if (PIE_NULL != self->arena) {
                PieArena_setExpand(self->arena, (self->cap - 1) 
                                * PIE_ALIGNED(self->size));
        }
        self->ary = (void **)PIE_RESIZE(self->ary, 
                        newcap * sizeof *(self->ary));
        for (i = self->cap; i < newcap; i++)
                self->ary[i] = PIE_NULL;
        /* memset(self->ary + self->len, '\0', self->cap * sizeof *(self->ary)); */
        self->cap = newcap;
}

static int PieList_ptrcmp(const void *p1, const void *p2, int num)
{
        return p1 - p2;
}

static int PieList_memcmp(const void *p1, const void *p2, int num)
{
        if (PIE_NULL != p2)
                return memcmp(p1, p2, num);
        else {
                char *pp = (char *)p1;
                int i;
                for (i = 0; i < num; i++) {
                        if (pp[i] != '\0')
                                return -1;
                }
                return 0;
        }
}

/* Implementations */

struct PieList *PieList_new(int size)
{
        struct PieList *self;
        PIE_ASSERT(size >= 0);
        self = PIE_NEW(struct PieList);

        self->ary = (void **)PIE_CALLOC(LIST_INISIZE, sizeof *(self->ary));
        self->len = 0;
        self->cap = LIST_INISIZE;
        self->size = size;
        if (0 < size) {
                self->arena = PieArena_new(0, (self->cap - 1) 
                                * PIE_ALIGNED(size));
        } else
                self->arena = PIE_NULL;

        return self;
}

void PieList_del(struct PieList **self)
{
        PIE_ASSERT(self);
        if (*self) {
                PieArena_del(&((*self)->arena));
                PIE_FREE((*self)->ary);
                PIE_FREE(*self);
        }
}

int PieList_length(struct PieList *self)
{
        return self->len;
}

int PieList_size(struct PieList *self)
{
        return self->size;
}

void *PieList_set(struct PieList *self, int idx, void *val)
{
        PIE_ASSERT(self);
        PIE_ASSERT(idx >= 0 && idx < self->len);
        if (0 == self->size) {
                self->ary[idx] = val;
                return val;
        }
        if (PIE_NULL == self->ary[idx])
                self->ary[idx] = PIE_ARENA_MALLOC(self->arena, self->size);
        if (PIE_NULL != val)
                return memcpy(self->ary[idx], val, self->size);
        else
                return memset(self->ary[idx], '\0', self->size);
}

void *PieList_get(struct PieList *self, int idx)
{
        PIE_ASSERT(self);
        PIE_ASSERT(idx >= 0 && idx < self->len);
        return self->ary[idx];
}

void *PieList_push(struct PieList *self, void *val)
{
        PIE_ASSERT(self);
        if (self->cap <= self->len)
                PieList_expand(self);
        return PieList_set(self, (self->len)++, val);
}

void *PieList_pop(struct PieList *self, void *out)
{
        PIE_ASSERT(self);
        if (0 == self->size)
                return self->ary[--(self->len)];
        if (PIE_NULL != out)
                memcpy(out, self->ary[self->len - 1], self->size);
        self->len--;
        return out;
}

void *PieList_insert(struct PieList *self, int idx, void *val)
{
        int i;
        void **ary = self->ary;

        PIE_ASSERT(self);
        PIE_ASSERT(idx >= 0 && idx <= self->len);
        if (self->cap <= self->len)
                PieList_expand(self);
        for (i = (self->len)++; i > idx; i--)
                ary[i] = ary[i - 1];
        return PieList_set(self, idx, val);
}

void *PieList_remove(struct PieList *self, int idx, void *out)
{
        void *prev;
        void **ary = self->ary;

        PIE_ASSERT(self);
        PIE_ASSERT(idx >= 0 && idx < self->len);

        prev = ary[idx];
        for (--(self->len); idx < self->len; idx++)
                ary[idx] = ary[idx + 1];
        if (0 == self->size)
                return prev;
        if (PIE_NULL != out)
                memcpy(out, prev, self->size);
        ary[self->len] = prev;
        return out;
}

int PieList_removeVal(struct PieList *self, void *val)
{
        int i, cnt;
        void **ary = self->ary, *tmp;
        int (*cmp)(const void *, const void *, int num) =
                        0 == self->size ? PieList_ptrcmp : PieList_memcmp;

        PIE_ASSERT(self);
        for (i = 0, cnt = 0; i < self->len; i++) {
                if (0 == cmp(ary[i], val, self->size)) {
                        cnt++;
                        break;
                }
        }
        while (i + cnt < self->len) {
                if (0 == cmp(ary[i + cnt], val, self->size)) {
                        cnt++;
                        continue;
                }
                tmp = ary[i];
                ary[i] = ary[i + cnt];
                ary[i + cnt] = tmp;
                i++;
        }
        self->len -= cnt;

        return cnt;
}

int PieList_find(struct PieList *self, void *val, int num)
{
        int i;
        int (*cmp)(const void *, const void *, int num) =
                        0 == self->size ? PieList_ptrcmp : PieList_memcmp;

        for (i = 0; i < self->len; i++) {
                void *item = self->ary[i];
                if (0 == cmp(item, val, self->size)) {
                        num--;
                        if (num == 0)
                                return i;
                }
        }

        return -1;
}

void PieList_clean(struct PieList *self)
{
        PIE_ASSERT(self);
        self->len = 0;
}

void PieList_truncate(struct PieList *self, int len)
{
        PIE_ASSERT(self);
        PIE_ASSERT(len >= 0 && len <= self->len);
        self->len = len;
}

void PieList_trimToSize(struct PieList *self)
{
        void **ary = (void **)PIE_MALLOC(self->len * sizeof *ary);
        if (0 == self->size)
                memcpy(ary, self->ary, self->len);
        else {
                int i;
                struct PieArena *arena = PieArena_new(0, (self->len - 1) 
                                * PIE_ALIGNED(self->size));
                for (i = 0; i < self->len; i++) {
                        void *val = PIE_ARENA_MALLOC(arena, self->size);
                        memcpy(val, self->ary[i], self->size);
                        ary[i] = val;
                }
                PieArena_del(&(self->arena));
                self->arena = arena;
        }
        PIE_FREE(self->ary);
        self->ary = ary;
        self->cap = self->len;
}

const struct PieListRep *PieList_rep(struct PieList *self)
{
        return (PieListRep *)self;
}

