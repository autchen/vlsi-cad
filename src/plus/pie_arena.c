/** * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_arena.h"

#include "plus/pie_assert.h"
#include "plus/pie_mem.h"
#include "plus/pie_log.h"

#include <string.h>

/* Implementations */

struct PieArena {
        struct PieArena *prev;
        char *avail;
        char *limit;
        long expand;
};

struct PieArenaSave {
        struct PieArena *inst, *prev;
        char *avail;
};

#define ALIGN PIE_ALIGN
#define ARENA_SIZE (((sizeof(struct PieArena) + ALIGN - 1) / ALIGN) * ALIGN)

struct PieArena *PieArena_new(long init, long expand)
{
        struct PieArena *arena;
        PIE_ASSERT(init >= 0);
        PIE_ASSERT(expand >= 0);
        arena = (struct PieArena *)PIE_MALLOC(ARENA_SIZE + init);
        arena->prev = PIE_NULL;
        arena->avail = (char *)arena + ARENA_SIZE;
        arena->limit = arena->avail + init;
        arena->expand = expand > 0 ? expand : 1024;
        return arena;
}

void PieArena_del(struct PieArena **self)
{
        PIE_ASSERT(self);
        if (*self) {
                struct PieArena *arena, *tmp;
                arena = *self;
                while (arena) {
                        tmp = arena;
                        arena = arena->prev;
                        PieMem_free(tmp);
                }
                *self = PIE_NULL;
        }
}

void *
PieArena_malloc(struct PieArena *self, long size, const char *file, int line)
{
        PIE_ASSERT(self);
        PIE_ASSERT(size > 0);
        size = ((size + ALIGN - 1) / ALIGN) * ALIGN;

        if (self->limit - self->avail < size) {
                struct PieArena *arena;
                long s = ARENA_SIZE + size + self->expand;
                arena = (struct PieArena *)PieMem_malloc(s, file, line);
                *arena = *self;
                self->prev = arena;
                self->avail = (char *)arena + ARENA_SIZE;
                self->limit = (char *)arena + s;
        }

        self->avail += size;
        return self->avail - size;
}

void *
PieArena_calloc(struct PieArena *self, long num, long size, const char *file, 
                int line)
{
        void *ptr;
        PIE_ASSERT(num > 0);
        ptr = PieArena_malloc(self, num * size, file, line);
        memset(ptr, '\0', num * size);
        return ptr;
}

long PieArena_setExpand(struct PieArena *self, long expand)
{
        long oldExpand;
        PIE_ASSERT(self);
        PIE_ASSERT(expand >= 0);
        oldExpand = self->expand;
        self->expand = expand > 0 ? expand : 1024;
        return oldExpand;
}

struct PieArenaSave *PieArena_save(struct PieArena *arena)
{
        struct PieArenaSave *save;
        struct PieArena *prev;
        char *avail;

        PIE_ASSERT(arena);
        prev = arena->prev;
        avail = arena->avail;
        save = (struct PieArenaSave *)PIE_ARENA_MALLOC(arena, sizeof *save);
        save->inst = arena;
        save->prev = prev;
        save->avail = avail;
        return save;
}

struct PieArena *PieArena_restore(struct PieArenaSave *save)
{
        struct PieArena *arena, *tmp;
        char *limit;

        PIE_ASSERT(save);
        arena = save->inst->prev;
        limit = save->inst->limit;
        while (arena != save->prev) {
                tmp = arena;
                limit = arena->limit;
                arena = arena->prev;
                PieMem_free(tmp);
        }
        arena = save->inst;
        arena->prev = save->prev;
        arena->avail = save->avail;
        arena->limit = limit;
        return arena;
}

void PieArena_log(struct PieArena *self)
{
        struct PieArena *arena;
        PIE_ASSERT(self);
        
        PieLog_print("PieArena_log: %#p\n", self);
        arena = self;
        while (arena) {
                int i;
                long total, avail = arena->limit - arena->avail;
                if (arena->prev) {
                        total = arena->limit - (char *)arena->prev 
                                - ARENA_SIZE;
                } else
                        total = arena->limit - (char *)self - ARENA_SIZE;

                if (0 != total) {
                        PieLog_print("%#p ", arena->prev);
                        for (i = 0; i < ((total - avail) * 50) / total ; i++)
                                PieLog_print("|");
                        for (; i < 50; i++)
                                PieLog_print("-");
                        PieLog_print(" %ld/%ld\n", avail, total);
                }
                arena = arena->prev;
        }
}
