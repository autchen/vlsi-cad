/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_ini.h"

#include "plus/pie_file.h"
#include "plus/pie_text.h"
#include "plus/pie_atom.h"
#include "plus/pie_list.h"
#include "plus/pie_pool.h"
#include "plus/pie_mem.h"
#include "plus/pie_assert.h"
#include "plus/pie_except.h"

#include <string.h>
#include <stdio.h>

const struct PieExcept PieIni_failed = {"Failed to parse ini file"};

#define DIV_LEN 78
#define LINE_BUF_LEN 1024
#define FORMAT_ERROR(L) \
        PIE_RAISE_INFO(PieIni_failed, ("Format error at line %d", (L)));

struct PieIniItem {
        struct PieIniItem *link;
        PieAtom key;
        PieText value;
};

struct PieIni {
        PieBool dirty;
        struct PieList *section;
        struct PieTextBuffer *valBuf;
        struct PiePool *itemPool;
        char tmp[LINE_BUF_LEN];
};

struct PieIni *PieIni_new(const char *path)
{
        struct PieIni *self;
        struct PieFile *file;
        file = PieFile_open(path, "r");
        self = PieIni_new2(file);
        PieFile_close(&file);
        return self;
}

struct PieIni *PieIni_new2(struct PieFile *file)
{
        static const PieText _s1 = {2, "[]"};
        static const PieText _s2 = {1, "\\"};

        struct PieIni *self;
        struct PieList *section;
        struct PiePool *pool;
        struct PieTextBuffer *text;
        struct PieText t, v;
        struct PieIniItem *item, *sec = PIE_NULL;

        int line = 0, pos;
        char buf[LINE_BUF_LEN];

        /* Allocate ini instance */
        PIE_ASSERT(file);
        self = PIE_NEW(struct PieIni);
        self->dirty = PIE_FALSE;
        self->valBuf = PieText_newBuffer(2 * LINE_BUF_LEN);
        self->itemPool = PiePool_new(sizeof(struct PieIniItem), 64);
        self->section = PieList_new(0);

        /* Init file parsing */
        text = self->valBuf;
        pool = self->itemPool;
        section = self->section;
        v = PieText_null;

        while (file->readLine(file, buf, LINE_BUF_LEN)) {

                item = PIE_NULL;
                line++;
                t = PieText_trim(PieText_box(buf, strlen(buf)), 0, -1);

                /* Empty line */
                if (0 == t.len) {
                        if (0 == v.len)
                                continue;
                        else
                                buf[t.str - buf] = ' ';
                        /* To terminate a continuous line */
                }

                /* New section */
                if ('[' == t.str[0]) {
                        if (v.len || t.len <= 2 || ']' != t.str[t.len - 1]
                            || -1 != PieText_lset(t, 1, -2, _s1))
                                FORMAT_ERROR(line);
                        sec = (struct PieIniItem *)PIE_POOL_ALLOC(pool);
                        sec->link = PIE_NULL;
                        sec->key = PieAtom_new(t.str + 1, t.len - 2);
                        sec->value = PieText_null;
                        PieList_push(section, sec);

                /* Comment line */
                } else if (';' == t.str[0]) {
                        if (v.len)
                                FORMAT_ERROR(line);
                        item = (struct PieIniItem *)PIE_POOL_ALLOC(pool);
                        item->key = PIE_NULL;
                        item->value = PieText_new(t, text);

                /* Line not yet finished */
                } else if ('\\' == t.str[t.len - 1]) {
                        pos = PieText_rstep(t, 0, -1, _s2);
                        t = -1 == pos ? PieText_null : PieText_sub(t, 0, pos);
                        if (v.len)
                                v = PieText_cat(v, t, text);
                        else if (t.len)
                                v = PieText_new(t, text);

                /* Key-value pair */
                } else {
                        v = v.len > 0   ? PieText_cat(v, t, text) 
                                        : PieText_new(t, text);
                        pos = PieText_lchr(v, 0, -1, '=');
                        if (-1 == pos)
                                FORMAT_ERROR(line);
                        item = (struct PieIniItem *)PIE_POOL_ALLOC(pool);
                        item->value = PieText_trim(v, pos + 1, -1);
                        v = PieText_trim(v, 0, pos);
                        if (0 == v.len)
                                FORMAT_ERROR(line);
                        item->key = PieAtom_new(v.str, v.len);
                        v = PieText_null;
                }

                /* Insert profile to section */
                if (item) {
                        /* Default section */
                        if (PIE_NULL == sec) {
                                sec = (struct PieIniItem *)PIE_POOL_ALLOC(pool);
                                sec->key = PIE_NULL;
                                sec->value = PieText_null;
                                sec->link = PIE_NULL;
                                PieList_push(section, sec);
                        }
                        item->link = PIE_NULL;
                        sec->link = item;
                        sec = item;
                }
        }

        return self;
}

void PieIni_del(struct PieIni **self)
{
        PIE_ASSERT(self);
        if (*self) {
                PiePool_del(&(*self)->itemPool);
                PieList_del(&(*self)->section);
                PieText_delBuffer(&(*self)->valBuf);
                PIE_FREE(*self);
        }
}

PieBool PieIni_save(struct PieIni *self, const char *path)
{
        struct PieFile *file;
        PIE_ASSERT(self && path);
        /* if (!(self->dirty)) */
                /* return PIE_FALSE; */
        file = PieFile_open(path, "w");
        PieIni_save2(self, file);
        PieFile_close(&file);
        return PIE_TRUE;
}

PieBool PieIni_save2(struct PieIni *self, struct PieFile *file)
{
        int i;
        const struct PieListRep *section;
        struct PieIniItem *item;

        PIE_ASSERT(self && file);
        /* if (!(self->dirty)) */
                /* return PIE_FALSE; */

        section = PieList_rep(self->section);
        for (i = 0; i < section->len; i++) {
                item = (struct PieIniItem *)(section->ary[i]);
                if (item->key) {
                        file->fmtput('[', file);
                        file->writes(file, (const char *)(item->key));
                        file->fmtput(']', file);
                        file->fmtput('\n', file);
                }
                item = item->link;
                while (item) {
                        if (!(item->key)) {
                                file->write(file, item->value.str, sizeof(char),
                                                item->value.len);
                                file->fmtput('\n', file);
                                item = item->link;
                                continue;
                        }
                        file->writes(file, (const char *)(item->key));
                        file->fmtput('=', file);
                        if (item->value.len + strlen((const char *)(item->key)) 
                                        <= DIV_LEN) {
                                file->write(file, item->value.str, sizeof(char), 
                                                item->value.len);
                        } else {
                                const char *str = item->value.str;
                                const char *end = str + item->value.len;
                                file->fmtput('\\', file);
                                file->fmtput('\n', file);
                                while (str + DIV_LEN < end) {
                                        file->write(file, str, sizeof(char),
                                                        DIV_LEN);
                                        file->fmtput('\\', file);
                                        file->fmtput('\n', file);
                                }
                                file->write(file, str, sizeof(char), end - str);
                        }
                        file->fmtput('\n', file);
                        item = item->link;
                }
                file->fmtput('\n', file);
        }

        return PIE_TRUE;
}

static struct PieIniItem *
PieIni_get(struct PieIni *self, PieAtom section, PieAtom key, PieBool create)
{
        int i;
        const struct PieListRep *rep;
        struct PieIniItem *item, *head;

        PIE_ASSERT(self);
        PIE_ASSERT(key);

        head = PIE_NULL;
        rep = PieList_rep(self->section);
        for (i = 0; i < rep->len; i++) {
                head = (struct PieIniItem *)(rep->ary[i]);
                if (head->key == section) {
                        item = head->link;
                        while (item) {
                                if (item->key == key)
                                        return item;
                                item = item->link;
                        }
                        break;
                }
        }

        if (create) {
                if (i >= rep->len || !head) {
                        head = (struct PieIniItem *)PIE_POOL_ALLOC(
                                        self->itemPool);
                        head->key = section;
                        head->link = PIE_NULL;
                        head->value = PieText_null;
                        PieList_push(self->section, head);
                }
                item = (struct PieIniItem *)PIE_POOL_ALLOC(self->itemPool);
                item->key = key;
                item->value = PieText_null;
                item->link = head->link;
                head->link = item;
                return item;
        }

        return PIE_NULL;
}

struct PieText 
PieIni_getText(struct PieIni *self, PieAtom section, PieAtom key)
{
        struct PieIniItem *item = PieIni_get(self, section, key, PIE_FALSE);
        if (PIE_NULL == item || 0 == item->value.len)
                return PieText_null;
        return item->value;
}

char *
PieIni_getStr(struct PieIni *self, PieAtom section, PieAtom key, 
              char *buffer, long bufLen)
{
        struct PieIniItem *item;
        PIE_ASSERT(buffer && bufLen > 0);
        item = PieIni_get(self, section , key, PIE_FALSE);
        if (PIE_NULL == item || 0 == item->value.len)
                return PIE_NULL;
        if (bufLen < item->value.len + 1)
                PIE_RAISE_INFO(PieIni_failed, ("Buffer overflow"));
        PieText_toStr(item->value, buffer, bufLen);
        return buffer;
}

PieBool 
PieIni_getBool(struct PieIni *self, PieAtom section, PieAtom key, 
               PieBool defaultVal)
{
        struct PieIniItem *item = PieIni_get(self, section, key, PIE_FALSE);
        if (PIE_NULL == item || 0 == item->value.len)
                return defaultVal;
        return (1 == item->value.len && '0' == item->value.str[0]) ? 
                        PIE_FALSE : PIE_TRUE;
}

long 
PieIni_getInt(struct PieIni *self, PieAtom section, PieAtom key, 
              long defaultVal)
{
        char *t;
        long retval;
        int i;
        PIE_ASSERT(self);
        t = self->tmp;
        t = PieIni_getStr(self, section, key, t, LINE_BUF_LEN);
        if (PIE_NULL == t)
                return defaultVal;
        i = sscanf(t, "%ld", &retval);
        if (1 != i) {
                PIE_RAISE_INFO(PieIni_failed, 
                        ("failed to convert to int: %s", t));
        }
        return retval;
}

long 
PieIni_getIntVec(struct PieIni *self, PieAtom section, PieAtom key, 
                 struct PieList *vec)
{
        PieText t, v;
        char *tmp;
        long retval;
        int i, k, k1;
        PIE_ASSERT(self);
        PIE_ASSERT(vec && PieList_size(vec) >= sizeof(long));

        tmp = self->tmp;
        t = PieIni_getText(self, section, key);
        if (0 == t.len)
                return 0;
        k = -1;
        do {
                k1 = k + 1;
                k = PieText_lchr(t, k1, -1, ',');
                v = PieText_sub(t, k1, k);
                if (0 == v.len)
                        continue;
                tmp = PieText_toStr(v, tmp, LINE_BUF_LEN);
                i = sscanf(tmp, "%ld", &retval);
                if (1 != i) {
                        PIE_RAISE_INFO(PieIni_failed, 
                                ("failed to convert to int: %s", tmp));
                }
                PieList_push(vec, &retval);
        } while (-1 != k);

        return PieList_length(vec);
}

double 
PieIni_getFloat(struct PieIni *self, PieAtom section, PieAtom key, 
                double defaultVal)
{
        
        char *t;
        double retval;
        int i;
        PIE_ASSERT(self);
        t = self->tmp;
        t = PieIni_getStr(self, section, key, t, LINE_BUF_LEN);
        if (PIE_NULL == t)
                return defaultVal;
        i = sscanf(t, "%lf", &retval);
        if (1 != i) {
                PIE_RAISE_INFO(PieIni_failed, 
                        ("failed to convert to float: %s", t));
        }
        return retval;
}

long 
PieIni_getFloatVec(struct PieIni *self, PieAtom section, PieAtom key, 
                   struct PieList *vec)
{
        PieText t, v;
        char *tmp;
        double retval;
        int i, k, k1;
        PIE_ASSERT(self);
        PIE_ASSERT(vec && PieList_size(vec) >= sizeof(double));

        tmp = self->tmp;
        t = PieIni_getText(self, section, key);
        if (0 == t.len)
                return 0;
        k = -1;
        do {
                k1 = k + 1;
                k = PieText_lchr(t, k1, -1, ',');
                v = PieText_sub(t, k1, k);
                if (0 == v.len)
                        continue;
                tmp = PieText_toStr(v, tmp, LINE_BUF_LEN);
                i = sscanf(tmp, "%lf", &retval);
                if (1 != i) {
                        PIE_RAISE_INFO(PieIni_failed, 
                                ("failed to convert to float: %s", tmp));
                }
                PieList_push(vec, &retval);
        } while (-1 != k);

        return PieList_length(vec);
}

char *
PieIni_setStr(struct PieIni *self, PieAtom section, PieAtom key, 
              char *buffer, long bufLen)
{
        struct PieIniItem *item;
        PIE_ASSERT(buffer && bufLen >= 0);
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        if (bufLen == 0)
                item->value = PieText_str(buffer, self->valBuf);
        else {
                struct PieText t;
                t = PieText_box(buffer, bufLen);
                item->value = PieText_new(t, self->valBuf);
        }
        return buffer;
}

void 
PieIni_setBool(struct PieIni *self, PieAtom section, PieAtom key, 
               PieBool val)
{
        struct PieIniItem *item;
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        item->value = val ? PieText_str("1", self->valBuf) 
                        : PieText_str("0", self->valBuf);
}

void 
PieIni_setInt(struct PieIni *self, PieAtom section, PieAtom key, long val)
{
        char *buf;
        struct PieIniItem *item;
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        buf = self->tmp;
        if (0 > sprintf(buf, "%ld", val))
                PIE_RAISE_INFO(PieIni_failed, ("failed to convert %ld", val));
        item->value = PieText_str(buf, self->valBuf);
}

void 
PieIni_setIntVec(struct PieIni *self, PieAtom section, PieAtom key, 
                 struct PieList *vec)
{
        struct PieText t;
        struct PieIniItem *item;
        const struct PieListRep *rep;
        int i, cnt;
        char *tmp;

        PIE_ASSERT(vec);
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        rep = PieList_rep(vec);
        tmp = self->tmp;

        for (i = 0; i < rep->len; i++) {
                long *val = (long *)(rep->ary[i]);
                PIE_ASSERT(val);
                if (0 == i) {
                        cnt = sprintf(tmp, "%ld", *val);
                        if (cnt < 0)
                                break;
                        t = PieText_str(tmp, self->valBuf);
                } else {
                        cnt = sprintf(tmp, ", %ld", *val);
                        if (cnt < 0)
                                break;
                        t = PieText_cat(t, PieText_box(tmp, cnt), self->valBuf);
                }
        }
        if (i < rep->len)
                PIE_RAISE_INFO(PieIni_failed, ("failed to convert vector"));
        item->value = t;
}

void 
PieIni_setFloat(struct PieIni *self, PieAtom section, PieAtom key, 
                double val)
{
        char *buf;
        struct PieIniItem *item;
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        buf = self->tmp;
        if (0 > sprintf(buf, "%lf", val))
                PIE_RAISE_INFO(PieIni_failed, ("failed to convert %lf", val));
        item->value = PieText_str(buf, self->valBuf);
}

void 
PieIni_setFloatVec(struct PieIni *self, PieAtom section, PieAtom key, 
                   struct PieList *vec)
{
        struct PieText t;
        struct PieIniItem *item;
        const struct PieListRep *rep;
        int i, cnt;
        char *tmp;

        PIE_ASSERT(vec);
        item = PieIni_get(self, section, key, PIE_TRUE);
        PIE_ASSERT(item);
        rep = PieList_rep(vec);
        tmp = self->tmp;

        for (i = 0; i < rep->len; i++) {
                double *val = (double *)(rep->ary[i]);
                PIE_ASSERT(val);
                if (0 == i) {
                        cnt = sprintf(tmp, "%lf", *val);
                        if (cnt < 0)
                                break;
                        t = PieText_str(tmp, self->valBuf);
                } else {
                        cnt = sprintf(tmp, ", %lf", *val);
                        if (cnt < 0)
                                break;
                        t = PieText_cat(t, PieText_box(tmp, cnt), self->valBuf);
                }
        }
        if (i < rep->len)
                PIE_RAISE_INFO(PieIni_failed, ("failed to convert vector"));
        item->value = t;
}

void PieIni_delSection(struct PieIni *self, PieAtom section)
{
        struct PieIniItem *item;
        const struct PieListRep *rep;
        int i;

        PIE_ASSERT(self);
        rep = PieList_rep(self->section);
        for (i = 0; i < rep->len; i++) {
                item = (struct PieIniItem *)(rep->ary[i]);
                if (item->key == section)
                        break;
        }
        if (i >= rep->len)
                return;
        while (item) {
                struct PieIniItem *tmp = item;
                item = item->link;
                PiePool_free(self->itemPool, tmp);
        }
        PieList_remove(self->section, i, PIE_NULL);
}

void 
PieIni_delProfile(struct PieIni *self, PieAtom section, PieAtom key)
{
        struct PieIniItem *item;
        const struct PieListRep *rep;
        int i;

        PIE_ASSERT(self);
        PIE_ASSERT(key);
        rep = PieList_rep(self->section);
        for (i = 0; i < rep->len; i++) {
                item = (struct PieIniItem *)(rep->ary[i]);
                if (item->key == section) {
                        struct PieIniItem *tmp = item;
                        item = item->link;
                        while (item) {
                                if (item->key == key) {
                                        tmp->link = item->link;
                                        PiePool_free(self->itemPool, item);
                                        break;
                                }
                                tmp = item;
                                item = item->link;
                        }
                        break;
                }
        }
}

