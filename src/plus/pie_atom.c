/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_atom.h"
#include "plus/pie_assert.h"
#include "plus/pie_mem.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

/* Singleton atom table */
/* TODO multi-thread support */

static struct PieAtomEntry {
        struct PieAtomEntry *link;
        int len;
        char *str;
} *PieAtom_htab[2048];

PieAtom PieAtom_new(const char *str, int len)
{
        struct PieAtomEntry *p;
        PieUint32 idx = 0;

        PIE_ASSERT(str);
        PIE_ASSERT(len >= 0);

        /* Hash map index */
        do {
                PieUint32 seed = 131;
                const char *hstr = str;
                const char *end = hstr + len;
                while (hstr < end)
                        idx = idx * seed + (*(hstr++));
                idx &= 0x07ff; /* idx = idx % 2048 */
        } while (0);

        /* Search the bucket */
        for (p = PieAtom_htab[idx]; p; p = p->link) {
                if (len == p->len) {
                        int i;
                        for(i = 0; i < len && p->str[i] == str[i]; i++);
                        if (i == len) 
                                return (PieAtom)p->str;
                }
        }

        /* Allocate new entry if not found */
        p = (struct PieAtomEntry *)PIE_MALLOC(sizeof *p + len + 1);
        p->len = len;
        p->str = (char *)(p + 1);
        if (0 < len)
                memcpy(p->str, str, len);
        p->str[len] = '\0';
        p->link = PieAtom_htab[idx];
        PieAtom_htab[idx] = p;

        return (PieAtom)p->str;
}

PieAtom PieAtom_str(const char *str)
{
        PIE_ASSERT(str);
        return PieAtom_new(str, strlen(str));
}

PieAtom PieAtom_int(long num)
{
        char buf[43];
        char *str = buf + sizeof buf;
        unsigned long m;

        if (LONG_MIN == num)
                m = LONG_MAX + 1ul;
        else if (0 > num)
                m = -num;
        else
                m = num;
        do {
                *(--str) = m % 10 + '0';
        } while (0 < (m /= 10));
        if (0 > num)
                *(--str) = '-';
        return PieAtom_new(str, ((buf + sizeof buf) - str));
}

