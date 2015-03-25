/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_mem.h"
#include "plus/pie_except.h"
#include "plus/pie_assert.h"
#include "plus/pie_log.h"

#include <stdlib.h>

/* Exception */

const struct PieExcept PieMem_failed = {"Memory Allocation Failed"};

#define MEM_RAISE(V) \
        do { \
                if (PIE_NULL == file) { \
                        file = __FILE__; \
                        line = __LINE__; \
                } \
                PieExcept_raise(&PieMem_failed, file, line, \
                                PieExcept_setInfo V); \
        } while (0)

#define ENROLL_EXCEPT(P, F) \
        if (PIE_OK != PieExcept_enroll((P), (void (*)(void *))(F))) { \
                PIE_FREE((P)); \
                MEM_RAISE(("failed to register resource")); \
        }

/**
 * Multi-thread solution
 */

#if PIE_MULTI_THREADED == 1

void *PieMem_chanCall(void *op, ...);
PieBool PieMem_check(void);

#define CHECK_CALL(V) \
        do { \
                if (!_PieMem_check()) \
                        return _PieMem_chanCall V ; \
        } while (0)

#define CHECK_FREE(P) \
        do { \
                if (!_PieMem_check()) { \
                        _PieMem_chanCall(PieMem_free, (P)); \
                        return; \
                } \
        } while (0)

#else
#define CHECK_CALL(V) ((void)0)
#define CHECK_FREE(P) ((void)0)

#endif /* PIE_MULTI_THREADED */

/* Production code */
#if PIE_DEBUG_LEVEL < 3

void *PieMem_malloc(long size, const char *file, int line)
{
        void *ptr;
        PIE_ASSERT(size > 0);
        CHECK_CALL((PieMem_malloc, size, file, line));

        ptr = malloc(size);
        if (PIE_NULL == ptr)
                MEM_RAISE(("malloc failed, size = %ld", size));
        ENROLL_EXCEPT(ptr, free);

        return ptr;
}

void *PieMem_calloc(long num, long size, const char *file, int line)
{
        void *ptr;

        PIE_ASSERT(num > 0);
        PIE_ASSERT(size > 0);
        CHECK_CALL((PieMem_calloc, num, size, file, line));

        ptr = calloc(num, size);
        if (PIE_NULL == ptr)
                MEM_RAISE(("calloc failed, num = %ld, size = %ld", num, size));
        ENROLL_EXCEPT(ptr, free);

        return ptr;
}

void *PieMem_resize(void *ptr, long size, const char *file, int line)
{
        PIE_ASSERT(ptr);
        PIE_ASSERT(size > 0);
        CHECK_CALL((PieMem_resize, ptr, size, file, line));
        
        PieExcept_drop(ptr);
        ptr = realloc(ptr, size);
        if (PIE_NULL == ptr)
                MEM_RAISE(("resize failed, ptr = %#p, size = %ld", ptr, size));
        ENROLL_EXCEPT(ptr, free);

        return ptr;
}

void PieMem_free(void *ptr)
{
        if (ptr) {
                CHECK_FREE(ptr);
                PieExcept_drop(ptr);
                free(ptr);
        }
}

void PieMem_log(void)
{
        PieLog_print("PieMem_log: Function not supported in release mode.");
}

/* Checking implementations */
#else

#include <string.h>
#include <limits.h>

#define MEM_TABSIZE 2048
#define MEM_NMEMREC 512
#define MEM_MAP(P) (((PieUptr)(P) >> 3) % MEM_TABSIZE)

/* Context */

static struct PieMemRecord {
        struct PieMemRecord *link;
        struct PieMemRecord *free;
        const char *file;
        int line;
        long seqnum;
        long size;
        void *ptr;
} *_htab[MEM_TABSIZE];

static struct PieMemRecord _freelist = {
        PIE_NULL, &_freelist, PIE_NULL, 0, 0, 0, PIE_NULL};

static long _seqnum = 0;

/* Alignment */

#define MEM_ALIGN PIE_ALIGN
#define MEM_NALLOC (((4096 + MEM_ALIGN - 1) / MEM_ALIGN) * MEM_ALIGN)

static struct PieMemRecord *PieMem_find(const void *ptr)
{
        struct PieMemRecord *rec;

        PIE_ASSERT_EXTREME(ptr);
        rec = _htab[MEM_MAP(ptr)];
        while (rec && rec->ptr != ptr)
                rec = rec->link;
        return rec;
}

static struct PieMemRecord *
PieMem_getRec(void *ptr, long size, const char *file, int line)
{
        static struct PieMemRecord *avail;
        static int navail = 0;

        if (navail <= 0) {
                avail = (struct PieMemRecord *)calloc(MEM_NMEMREC, 
                                sizeof *avail);
                if (PIE_NULL == avail)
                        return PIE_NULL;
                navail = MEM_NMEMREC;
        }
        avail->ptr = ptr;
        avail->size = size;
        avail->file = file;
        avail->line = line;

        if (LONG_MAX != _seqnum) {
                avail->seqnum = _seqnum;
                _seqnum++;
        } else
                avail->seqnum = -1;

        navail--;
        return avail++;
}

void *PieMem_malloc(long size, const char *file, int line)
{
        void *ptr = PIE_NULL;
        struct PieMemRecord *rec;

        PIE_ASSERT(size > 0);
        CHECK_CALL((PieMem_malloc, size, file, line));
        size = ((size + MEM_ALIGN - 1) / MEM_ALIGN) * MEM_ALIGN;

        for (rec = _freelist.free; rec; rec = rec->free) {
                if (rec->size > size) {
                        rec->size -= size;
                        ptr = rec->ptr + rec->size;
                        rec = PieMem_getRec(ptr, size, file, line);
                        if (PIE_NULL == rec)
                                MEM_RAISE(("failed creating malloc record"));
                        else {
                                int idx = MEM_MAP(ptr);
                                rec->link = _htab[idx];
                                _htab[idx] = rec;
                                break;
                        }
                }
                if (&_freelist == rec) {
                        struct PieMemRecord *rec2;
                        ptr = malloc(MEM_NALLOC + size);
                        if (PIE_NULL == ptr)
                                MEM_RAISE(("malloc failed, size = %ld", size));
                        rec2 = PieMem_getRec(ptr, MEM_NALLOC + size, 
                                        __FILE__, __LINE__);
                        if (PIE_NULL == rec2) {
                                free(ptr);
                                MEM_RAISE(("failed creating malloc record"));
                        }
                        rec2->free = _freelist.free;
                        _freelist.free = rec2;
                }
        }

        PIE_ASSERT_EXTREME(PIE_NULL != ptr);
        ENROLL_EXCEPT(ptr, PieMem_free);
        return ptr;
}

void *PieMem_calloc(long num, long size, const char *file, int line)
{
        void *ptr;

        PIE_ASSERT(num > 0);
        PIE_ASSERT(size > 0);

        ptr = PieMem_malloc(num * size, file, line);
        memset(ptr, '\0', num * size);
        return ptr;
}

void *PieMem_resize(void *ptr, long size, const char *file, int line)
{
        void *ptr2;
        struct PieMemRecord *rec;

        PIE_ASSERT(ptr);
        PIE_ASSERT(size > 0);
        CHECK_CALL((PieMem_resize, ptr, size, file, line));

        rec = PieMem_find(ptr);
        PIE_ASSERT(PIE_NULL != rec);
        PIE_ASSERT(PIE_NULL == rec->free);
        PIE_ASSERT_EXTREME(0 == (unsigned long)(rec->size) % MEM_ALIGN);

        ptr2 = PieMem_malloc(size, file, line);
        memcpy(ptr2, ptr, (rec->size < size ? rec->size : size));
        PieMem_free(ptr);

        return ptr2;
}

void PieMem_free(void *ptr)
{
        if (ptr) {
                struct PieMemRecord *rec;

                CHECK_FREE(ptr);
                PieExcept_drop(ptr);
                rec = PieMem_find(ptr);
                PIE_ASSERT(PIE_NULL != rec);
                PIE_ASSERT(PIE_NULL == rec->free);
                PIE_ASSERT_EXTREME(0 == (unsigned long)(rec->size) % MEM_ALIGN);

                rec->free = _freelist.free;
                _freelist.free = rec;
        }
}

void PieMem_log(void)
{
        int i, max = _seqnum;
        struct PieMemRecord **seq = PIE_NULL;

        PIE_ASSERT_IX;
        PIE_TRY
                /* seqnum(allocated) + 2(seq + possible freelist alloc) */
                seq = (struct PieMemRecord **)PIE_CALLOC(max + 2, sizeof *seq);
        PIE_CATCH(PieMem_failed, ex)
                PieLog_print("PieMem_log: Log buffer alloc failed"
                                "seqnum = %d.\n%E\n", max, ex);
                return;
        PIE_TRIED;

        PieLog_print("PieMem_log:\n");
        for (i = 0; i < MEM_TABSIZE; i++) {
                struct PieMemRecord *rec = _htab[i];
                while (rec) {
                        seq[rec->seqnum] = rec;
                        rec = rec->link;
                }
        }
        PieLog_print("%-6s %-10s %10s %s\n", "Seq", "Addr", "Size", "Location");
        for (i = 0; i < max; i++) {
                struct PieMemRecord *rec = seq[i];
                if (PIE_NULL == rec)
                        continue;
                PieLog_print("#%-5ld %#p %10ld %s:%d\n", rec->seqnum, 
                                rec->ptr, rec->free ? -1 : rec->size,
                                rec->file, rec->line);
        }
        PIE_FREE(seq);
}

#endif /* PIE_DEBUG_LEVEL */

#if PIE_MULTI_THREADED == 1
/* TODO */
#endif /* PIE_MULTI_THREADED */
