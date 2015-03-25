/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_table.h"
#include "plus/pie_assert.h"
#include "plus/pie_mem.h"
#include "plus/pie_pool.h"
#include "plus/pie_log.h"

#include <limits.h>

/* Data structs */

struct PieTableEntry {
        struct PieTableEntry *link;
        const void *key;
        void *val;
};

struct PieTable {
        int stamp;
        int size;
        struct PiePool *pool;
        int (*cmp)(const void *p1, const void *p2);
        unsigned (*hash)(const void *key);
        struct PieTableEntry **htab;
};

/* Implementations */

static const int primes[] = {
        509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, 131071, 262147, 
        524287, INT_MAX
};


struct PieTable *
PieTable_new(int hint, int (*cmp)(const void *p1, const void *p2),
             unsigned (*hash)(const void *key))
{
        int i;
        struct PieTable *self;

        PIE_ASSERT(hint >= 0);
        for (i = 1; primes[i] < hint; i++);
        self = PIE_NEW(struct PieTable);
        self->stamp = 0;
        self->size = primes[i - 1];
        self->cmp = cmp ? cmp : PieTable_atomcmp;
        self->hash = hash ? hash : PieTable_atomhash;
        self->pool = PiePool_new(sizeof **(self->htab), 256);
        self->htab = (struct PieTableEntry **)PIE_MALLOC(primes[i - 1]
                        * sizeof *(self->htab));
        for (i = 0; i < self->size; i++)
                self->htab[i] = PIE_NULL;
        return self;
}

void PieTable_del(struct PieTable **self)
{
        PIE_ASSERT(self);
        if (*self) {
                PiePool_del(&((*self)->pool));
                PIE_FREE(*self);
        }
}

void *PieTable_put(PieTable *self, const void *key, void *val)
{
        unsigned idx;
        struct PieTableEntry *e;
        void *retval;

        PIE_ASSERT(self);
        PIE_ASSERT(key);
        idx = self->hash(key) % self->size;
        for (e = self->htab[idx]; e; e = e->link) {
                if (0 == self->cmp(e->key, key))
                        break;
        }

        if (PIE_NULL == e) {
                e = (struct PieTableEntry *)PIE_POOL_ALLOC(self->pool);
                e->key = key;
                e->val = val;
                e->link = self->htab[idx];
                self->htab[idx] = e;
                retval = PIE_NULL;
        } else
                retval = val;

        self->stamp++;
        return retval;
}

void *PieTable_sub(struct PieTable *self, const void *key, void *val)
{
        unsigned idx;
        struct PieTableEntry *e;
        void *retval;

        PIE_ASSERT(self);
        PIE_ASSERT(key);
        idx = self->hash(key) % self->size;
        for (e = self->htab[idx]; e; e = e->link) {
                if (0 == self->cmp(e->key, key))
                        break;
        }

        if (PIE_NULL == e) {
                e = (struct PieTableEntry *)PIE_POOL_ALLOC(self->pool);
                e->key = key;
                e->link = self->htab[idx];
                self->htab[idx] = e;
                retval = PIE_NULL;
        } else
                retval = e->val;

        e->val = val;
        self->stamp++;
        return retval;
}

void *PieTable_get(struct PieTable *self, const void *key)
{
        struct PieTableEntry *e;

        PIE_ASSERT(self);
        PIE_ASSERT(key);
        for (e = self->htab[self->hash(key) % self->size]; e; e = e->link) {
                if (0 == self->cmp(e->key, key))
                        return e->val;
        }
        return PIE_NULL;
}

void *PieTable_remove(struct PieTable *self, const void *key)
{
        struct PieTableEntry *e, *prev = PIE_NULL;
        void *retval = PIE_NULL;
        unsigned idx;

        PIE_ASSERT(self);
        PIE_ASSERT(key);
        idx = self->hash(key) % self->size;
        for (e = self->htab[idx]; e; e = e->link) {
                if (0 == self->cmp(e->key, key)) {
                        if (PIE_NULL == prev)
                                self->htab[idx] = e->link;
                        else
                                prev->link = e->link;
                        retval = e->val;
                        PiePool_free(self->pool, e);
                        break;
                }
                prev = e;
        }
        self->stamp++;
        return retval;
}

int PieTable_total(struct PieTable *self)
{
        int i, total = 0;
        struct PieTableEntry *e;

        PIE_ASSERT(self);
        for (i = 0; i < self->size; i++) {
                for (e = self->htab[i]; e; e = e->link)
                        total++;
        }
        return total;
}

void 
PieTable_apply(struct PieTable *self,
               void (*func)(const void *key, void *val, void *data),
               void *data)
{
        int stamp;
        int i;
        struct PieTableEntry *e;

        PIE_ASSERT(self);
        PIE_ASSERT(func);
        stamp = self->stamp;
        for (i = 0; i < self->size; i++) {
                for (e = self->htab[i]; e; e = e->link) {
                        func(e->key, e->val, data);
                        PIE_ASSERT_RELEASE(stamp == self->stamp);
                }
        }
}

void PieTable_adjust(struct PieTable *self)
{
        int i, idx, newSize, total;
        struct PieTableEntry **newHtab, *e1, *e2;

        PIE_ASSERT(self);
        total = PieTable_total(self);
        if (total <= self->size)
                return;
        for (i = 1; primes[i] < total; i++);
        newSize = primes[i];
        newHtab = (struct PieTableEntry **)PIE_MALLOC(newSize
                        * sizeof *newHtab);
        for (i = 0; i < newSize; i++)
                newHtab[i] = PIE_NULL;

        for (i = 0; i < self->size; i++) {
                e1 = self->htab[i];
                while (e1) {
                        idx = self->hash(e1->key) % newSize;
                        for (e2 = newHtab[idx]; e2; e2 = e2->link) {
                                if (0 == self->cmp(e1->key, e2->key))
                                        PIE_ASSERT_RELEASE(0);
                        }
                        e2 = e1;
                        e1 = e1->link;
                        e2->link = newHtab[idx];
                        newHtab[idx] = e2;
                }
        }
        PIE_FREE(self->htab);
        self->htab = newHtab;
        self->size = newSize;
        self->stamp++;
}

void PieTable_statLog(struct PieTable *self)
{
        int i;
        int itemTotal = 0, bucketPopulated = 0;
        int bucketMax = 0, bucketMin = -1;
        long long idxTotal = 0, idx2Total = 0;
        int chainMax = 0, chainTotal = 0;
        struct PieTableEntry *e;

        PIE_ASSERT(self);
        for (i = 0; i < self->size; i++) {
                e = self->htab[i];
                int tmpchain = 0;
                if (PIE_NULL != e) {
                        if (-1 == bucketMin)
                                bucketMin = i;
                        bucketMax = i;
                        bucketPopulated++;
                }
                while (e) {
                        itemTotal++;
                        idxTotal += i;
                        idx2Total += i * i;
                        tmpchain++;
                        e = e->link;
                }
                chainTotal += tmpchain;
                if (tmpchain > chainMax)
                        chainMax = tmpchain;
        }
        PieLog_print("Table Log: %p\n", self);
        PieLog_print("Items Total: %d\n", itemTotal);
        PieLog_print("Buckets Populated: %d/%d\n", bucketPopulated, self->size);
        PieLog_print("Buckets Utilization: %f\n", 
                        (1.0 * bucketPopulated) / self->size);
        PieLog_print("Buckets Index Range: %d-%d\n", bucketMin, bucketMax);
        PieLog_print("Index Variance: %f\n", 
                        (1.0 * idx2Total) / itemTotal 
                        - ((1.0 * idxTotal) / itemTotal) 
                        * ((1.0 * idxTotal) / itemTotal));
        PieLog_print("Longest Chain: %d\n", chainMax);
        PieLog_print("Average Chain: %f\n", (1.0 * chainTotal) / bucketPopulated);
}

int PieTable_longcmp(const void *p1, const void *p2)
{
        long a1, a2;
        PIE_ASSERT(p1 && p2);
        a1 = *((long *)p1);
        a2 = *((long *)p2);
        return (int)(a1 - a2);
}

unsigned PieTable_longhash(const void *key)
{
        long a;
        PIE_ASSERT(key);
        a = *((long *)key);
        return a * 2654435761U;
}


int PieTable_atomcmp(const void *p1, const void *p2)
{
        return p1 != p2;
}

unsigned PieTable_atomhash(const void *key)
{
        return (PieUptr)key >> 2;
}

