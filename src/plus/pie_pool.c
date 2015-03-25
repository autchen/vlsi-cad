/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_pool.h"

#include "plus/pie_assert.h"
#include "plus/pie_mem.h"

#define CHUNK_SIZE (((sizeof(struct PiePoolChunk) + PIE_ALIGN - 1) \
                        / PIE_ALIGN) * PIE_ALIGN)

struct PiePoolChunk {
        struct PiePoolChunk *next;
        long expand;
        char *chips;
};

union PiePoolChip {
        union PiePoolChip *next;
        char data[1];
};

struct PiePool {
        long unit, expand;
        struct PiePoolChunk *chunks;
        union PiePoolChip *freelist;
};

struct PiePool *PiePool_new(long unit, long expand)
{
        struct PiePool *pool;

        PIE_ASSERT(unit > 0 && expand > 0);
        pool = PIE_NEW(struct PiePool);
        pool->unit = ((unit + PIE_ALIGN - 1) / PIE_ALIGN) * PIE_ALIGN;
        pool->expand = expand;
        pool->chunks = PIE_NULL;
        pool->freelist = PIE_NULL;

        return pool;
}

void PiePool_del(struct PiePool **self)
{

        PIE_ASSERT(self);
        if (*self) {
                struct PiePoolChunk *chunk, *tmp;
                chunk = (*self)->chunks;
                while (PIE_NULL != chunk) {
                        tmp = chunk->next;
                        PIE_FREE(chunk);
                        chunk = tmp;
                }
                PIE_FREE(*self);
        }
}

void *PiePool_alloc(struct PiePool *self, const char *file, int line)
{
        char *data;

        PIE_ASSERT(self);
        if (PIE_NULL == self->freelist) {
                int i;
                union PiePoolChip *chip;
                struct PiePoolChunk *newchunk;
                newchunk = (struct PiePoolChunk *)PIE_MALLOC(CHUNK_SIZE
                                + self->unit * self->expand);
                newchunk->expand = self->expand;
                newchunk->chips = (char *)newchunk + CHUNK_SIZE;
                newchunk->next = self->chunks;
                self->chunks = newchunk;
                chip = (union PiePoolChip *)(newchunk->chips);
                for (i = 0; i < self->expand - 1; i++) {
                        chip->next = (union PiePoolChip *)((char *)chip 
                                        + self->unit);
                        chip = chip->next;
                }
                chip->next = PIE_NULL;
                self->freelist = (union PiePoolChip *)(newchunk->chips);
        }

        data = self->freelist->data;
        self->freelist = self->freelist->next;
        return data;
}

void PiePool_free(struct PiePool *self, void *ptr)
{
        union PiePoolChip *chip = (union PiePoolChip *)ptr;
        PIE_ASSERT(self && ptr);

#if PIE_DEBUG_LEVEL > 1
        do {
                char *pp = (char *)ptr;
                struct PiePoolChunk *chunk = self->chunks;
                long span = chunk->expand * self->unit;
                while (PIE_NULL != chunk) {
                        if (pp >= chunk->chips && pp < chunk->chips + span
                                && 0 == (pp - chunk->chips) % self->unit)
                                break;
                        chunk = chunk->next;
                }
                PIE_ASSERT(PIE_NULL != chunk);
        } while (0);
#endif /* PIE_DEBUG_LEVEL */

        chip->next = self->freelist;
        self->freelist = chip;
}

long PiePool_setExpand(struct PiePool *self, long expand)
{
        long oldExpand;
        PIE_ASSERT(self && expand > 0);
        oldExpand = self->expand;
        self->expand = expand;
        return oldExpand;
}

long PiePool_avail(struct PiePool *self)
{
        union PiePoolChip *chip = self->freelist;
        long cnt = 0;
        while (PIE_NULL != chip) {
                cnt++;
                chip = chip->next;
        }
        return cnt;
}

long PiePool_total(struct PiePool *self)
{
        struct PiePoolChunk *chunk = self->chunks;
        long cnt = 0;
        while (PIE_NULL != chunk) {
                cnt += chunk->expand;
                chunk = chunk->next;
        }
        return cnt;
}
