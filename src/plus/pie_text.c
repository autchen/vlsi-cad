/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_text.h"
#include "plus/pie_assert.h"
#include "plus/pie_mem.h"
#include "plus/pie_except.h"

#include <string.h>
#include <stdarg.h>

/* Constant strings */

static const char _cset[] = 
        "\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
        "\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
        "\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
        "\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
        "\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
        "\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
        "\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
        "\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
        "\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217"
        "\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237"
        "\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257"
        "\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277"
        "\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317"
        "\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337"
        "\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357"
        "\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";

const struct PieText PieText_cset =  {256, _cset};
const struct PieText PieText_ascii = {128, _cset};
const struct PieText PieText_ucase = {26,  _cset + 'A'};
const struct PieText PieText_lcase = {26,  _cset + 'a'};
const struct PieText PieText_digit = {10,  _cset + '0'};
const struct PieText PieText_null =  {0,   _cset};

/* Context */

struct PieTextChunk {
        struct PieTextChunk *link;
        char *avail;
        char *limit;
};

struct PieTextBuffer {
        struct PieTextChunk chunk;
        char *map;
        long expand;
};

static char *PieText_alloc(int size, struct PieTextBuffer *buf)
{
        struct PieTextChunk *chunk;

        PIE_ASSERT(size >= 0);
        PIE_ASSERT(buf);
        chunk = &(buf->chunk);

        if (chunk->avail + size > chunk->limit) {
                struct PieTextChunk *newchunk;
                int s = sizeof *chunk + 10 * 1024 + size;
                newchunk = (struct PieTextChunk *)PIE_MALLOC(s);
                *newchunk = *chunk;
                chunk->link = newchunk;
                chunk->avail = (char *)(newchunk + 1);
                chunk->limit = chunk->avail + s - sizeof *newchunk;
        }
        chunk->avail += size;
        return chunk->avail - size;
}

#define TEXT_ISEND(T, L, B) (((T).str + (T).len == (B)->chunk.avail) \
                && ((B)->chunk.avail + (L) <= (B)->chunk.limit))

/* Position manipulate */

#define TEXT_IDX(i, len) ((i) < 0 ? (i) + (len) + 1 : (i))

#define TEXT_CONVERT(i, j, len) \
        do { \
                (i) = TEXT_IDX((i), (len)); \
                (j) = TEXT_IDX((j), (len)); \
                if ((i) > (j)) { \
                        int tmp = (i); \
                        (i) = (j); \
                        (j) = tmp; \
                } \
                PIE_ASSERT((i) >= 0 && (j) <= (len)); \
        } while (0)

/* String creation/manipulation */

struct PieText PieText_new(struct PieText t, struct PieTextBuffer *buf)
{
        char *s;
        struct PieText nt;
        PIE_ASSERT(t.str && t.len > 0);
        nt.len = t.len;
        s = PieText_alloc(nt.len, buf);
        memcpy(s, t.str, nt.len);
        nt.str = s;
        return nt;
}

int PieText_idx(struct PieText t, int pos)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        pos = TEXT_IDX(pos, t.len);
        PIE_ASSERT(pos >= 0 && pos < t.len);
        return pos;
}

struct PieText PieText_box(const char *str, int len)
{
        struct PieText t;
        PIE_ASSERT(len >= 0 && str);
        t.len = len;
        t.str = str;
        return t;
}

struct PieText PieText_sub(struct PieText t, int i, int j)
{
        struct PieText t2;
        PIE_ASSERT(t.len >= 0 && t.str);
        TEXT_CONVERT(i, j, t.len);
        t2.len = j - i;
        t2.str = t.str + i;
        return t2;
}

struct PieText PieText_str(const char *str, struct PieTextBuffer *buf)
{
        char *s;
        struct PieText t;
        PIE_ASSERT(str);
        t.len = strlen(str);
        s = PieText_alloc(t.len, buf);
        memcpy(s, str, t.len);
        t.str = s;
        return t;
}

char *PieText_toStr(struct PieText t, char *str, int len)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        if (PIE_NULL == str) {
                int l = len > t.len ? len : t.len;
                str = (char *)PIE_MALLOC(l + 1);
        } else
                PIE_ASSERT_RELEASE(len >= t.len + 1);
        memcpy(str, t.str, t.len);
        str[t.len] = '\0';
        return str;
}

struct PieText 
PieText_dup(struct PieText t, int num, struct PieTextBuffer *buf)
{
        struct PieText t2;
        char *p;

        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(num >= 0);
        if (0 == num || 0 == t.len)
                return PieText_null;
        if (1 == num)
                return t;

        t2.len = num * t.len;
        if (TEXT_ISEND(t, t2.len - t.len, buf)) {
                p = PieText_alloc(t2.len - t.len, buf);
                t2.str = t.str;
                num--;
        } else {
                p = PieText_alloc(t2.len, buf);
                t2.str = p;
        }
        for (; num > 0; num--, p += t.len)
                memcpy(p, t.str, t.len);
        return t2;
}

struct PieText 
PieText_cat(struct PieText t1, struct PieText t2, struct PieTextBuffer *buf)
{
        struct PieText t;

        PIE_ASSERT(t1.len >= 0 && t1.str);
        PIE_ASSERT(t2.len >= 0 && t2.str);
        if (0 == t1.len)
                return t2;
        if (0 == t2.len)
                return t1;
        if (t1.str + t1.len == t2.str) {
                t1.len += t2.len;
                return t1;
        }

        t.len = t1.len + t2.len;
        if (TEXT_ISEND(t1, t2.len, buf)) {
                t.str = t1.str;
                memcpy(PieText_alloc(t2.len, buf), t2.str, t2.len);
        } else {
                char *p = PieText_alloc(t.len, buf);
                t.str = p;
                memcpy(p, t1.str, t1.len);
                memcpy(p + t1.len, t2.str, t2.len);
        }
        return t;
}

struct PieText 
PieText_reverse(struct PieText t, struct PieTextBuffer *buf)
{
        struct PieText t2;
        char *p;
        int i;
        
        PIE_ASSERT(t.len >= 0 && t.str);
        if (0 == t.len)
                return PieText_null;
        if (1 == t.len)
                return t;

        p = PieText_alloc(t.len, buf);
        t2.str = p;
        t2.len = t.len;
        for (i = t2.len - 1; i >= 0; i--)
                *p++ = t.str[i];
        return t2;
}

struct PieText 
PieText_map(struct PieText t, const struct PieText *from, 
            const struct PieText *to, struct PieTextBuffer *buf)
{
        char *p, *map;
        int i;
        struct PieText t2;

        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(buf);

        if (from && to) {
                if (PIE_NULL == buf->map)
                        buf->map = (char *)PIE_MALLOC(256 * sizeof(char));
                map = buf->map;
                for (i = 0; i < 256; i++)
                        map[i] = i;
                PIE_ASSERT(from->len <= to->len);
                for (i = 0; i < from->len; i++)
                        map[(unsigned char)from->str[i]] = to->str[i];
        } else {
                PIE_ASSERT(PIE_NULL == from && PIE_NULL == to);
                PIE_ASSERT(buf->map);
                map = buf->map;
        }

        if (0 == t.len)
                return t;
        p = PieText_alloc(t.len, buf);
        t2.str = p;
        t2.len = t.len;
        for (i = 0; i < t.len; i++)
                *p++ = map[(unsigned char)t.str[i]];
        return t2;
}

/* Text analysis */

int PieText_cmp(struct PieText t1, struct PieText t2)
{
        int c = 0;
        PIE_ASSERT(t1.str && t1.len >= 0);
        PIE_ASSERT(t2.str && t2.len >= 0);

        if (t1.str == t2.str) {
                c = t1.len - t2.len;
        } else if (t1.len < t2.len) {
                c = memcmp(t1.str, t2.str, t1.len);
                if (0 == c)
                        c = -1;
        } else if (t1.len > t2.len) {
                c = memcmp(t1.str, t2.str, t2.len);
                if (0 == c)
                        c = +1;
        } else
                c = memcmp(t1.str, t2.str, t1.len);
        return c;
}

int PieText_lchr(struct PieText t, int i, int j, char c)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        TEXT_CONVERT(i, j, t.len);
        for (; i < j; i++) {
                if (c == t.str[i])
                        return i;
        }
        return -1;
}

int PieText_rchr(struct PieText t, int i, int j, char c)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        TEXT_CONVERT(i, j, t.len);
        for (j--; j >= i; j--) {
                if (c == t.str[j])
                        return j;
        }
        return -1;
}

int PieText_lset(struct PieText t, int i, int j, struct PieText set)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(set.len >= 0 && set.str);
        TEXT_CONVERT(i, j, t.len);
        for (; i < j; i++) {
                if (memchr(set.str, t.str[i], set.len))
                        return i;
        }
        return -1;
}

int PieText_rset(struct PieText t, int i, int j, struct PieText set)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(set.len >= 0 && set.str);
        TEXT_CONVERT(i, j, t.len);
        for (j--; j >= i; j--) {
                if (memchr(set.str, t.str[j], set.len))
                        return j;
        }
        return -1;
}

int PieText_lstr(struct PieText t, int i, int j, struct PieText str)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(str.len >= 0 && str.str);
        TEXT_CONVERT(i, j, t.len);
        if (0 == str.len)
                return i;
        else if (1 == str.len) {
                for (; i < j; i++) {
                        if (*(str.str) == t.str[i])
                                return i;
                }
        } else {
                for (; i + str.len < j; i++) {
                        if (0 == memcmp(t.str + i, str.str, str.len))
                                return i;
                }
        }
        return -1;
}

int PieText_rstr(struct PieText t, int i, int j, struct PieText str)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(str.len >= 0 && str.str);
        TEXT_CONVERT(i, j, t.len);
        if (0 == str.len)
                return j;
        else if (1 == str.len) {
                for (j--; j >= i; j--) {
                        if (*(str.str) == t.str[j])
                                return j;
                }
        } else {
                for (j -= str.len; j >= i; j--) {
                        if (0 == memcmp(t.str + j, str.str, str.len))
                                return j;
                }
        }
        return -1;
}

int PieText_lstep(struct PieText t, int i, int j, struct PieText set)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(set.len >= 0 && set.str);
        TEXT_CONVERT(i, j, t.len);
        for (; i < j && memchr(set.str, t.str[i], set.len); i++);
        return i == j ? -1 : i;
}

int PieText_rstep(struct PieText t, int i, int j, struct PieText set)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(set.len >= 0 && set.str);
        TEXT_CONVERT(i, j, t.len);
        for (j--; j >= i && memchr(set.str, t.str[j], set.len); j--);
        return j < i ? -1 : j + 1;
}

int PieText_lmatch(struct PieText t, int i, int j, struct PieText str)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(str.len >= 0 && str.str);
        TEXT_CONVERT(i, j, t.len);
        if (0 == str.len)
                return i;
        else if (1 == str.len) {
                if (i < j && *(str.str) == t.str[i])
                        return i + 1;
        } else if (i + str.len <= j 
                   && 0 == memcmp(t.str + i, str.str, str.len))
                return i + str.len;
        return -1;
}

int PieText_rmatch(struct PieText t, int i, int j, struct PieText str)
{
        PIE_ASSERT(t.len >= 0 && t.str);
        PIE_ASSERT(str.len >= 0 && str.str);
        TEXT_CONVERT(i, j, t.len);
        if (0 == str.len)
                return j;
        else if (1 == str.len) {
                if (i < j && *(str.str) == t.str[j - 1])
                        return j - 1;
        } else if (j - str.len >= i 
                   && 0 == memcmp(t.str + j - str.len, str.str, str.len))
                return j - str.len;
        return -1;
}

/* Text utility */

struct PieText PieText_trim(struct PieText t, int i, int j)
{
        struct PieText t2;
        PIE_ASSERT(t.len >= 0 && t.str);
        TEXT_CONVERT(i, j, t.len);
        while (i < j 
               && (' ' == t.str[i] || '\t' == t.str[i] || '\n' == t.str[i]))
                i++;
        j--;
        while (j > i 
               && (' ' == t.str[j] || '\t' == t.str[j] || '\n' == t.str[j]))
                j--;
        t2.len = j + 1 - i;
        t2.str = t.str + i;
        return t2;
}

struct PieText 
PieText_basename(struct PieText t, int i, int j, struct PieText suffix)
{
        static PieText slash = {2, "\\/"};
        struct PieText t2;
        int tmp;

        TEXT_CONVERT(i, j, t.len);
        tmp = PieText_rset(t, i, j, slash);
        if (-1 != tmp)
                i = tmp + 1;
        tmp = PieText_rmatch(t, i, j, suffix);
        if (-1 != tmp)
                j = tmp;
        t2.len = j - i;
        t2.str = t.str + i;
        return t2;
}

/* Text memory management */

struct PieTextBuffer *PieText_newBuffer(long expand)
{
        struct PieTextBuffer *buf = PIE_NEW(struct PieTextBuffer);
        PIE_ASSERT(expand > 0);
        buf->map = PIE_NULL;
        buf->expand = expand;
        buf->chunk.link = PIE_NULL;
        buf->chunk.avail = PIE_NULL;
        buf->chunk.limit = PIE_NULL;
        return buf;
}

void PieText_delBuffer(struct PieTextBuffer **buf)
{
        PIE_ASSERT(buf);
        if (*buf) {
                struct PieTextChunk *chunk = &((*buf)->chunk);
                struct PieTextChunk *tmp;
                if ((*buf)->map)
                        PIE_FREE((*buf)->map);
                while (chunk) {
                        tmp = chunk;
                        chunk = chunk->link;
                        PIE_FREE(tmp);
                }
                *buf = PIE_NULL;
        }
}

/**
 * Fmt function
 */

#include "../plus/pie_fmt.h"

void 
PieText_fmt(int code, va_list *app, int (*put)(int c, void *cl), void *cl, 
            PieFmtFlag flag, int width, int precision)
{
        struct PieText *t;
        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('T' == code);
        t = va_arg(*app, PieText *);
        PIE_ASSERT(t);
        PieFmt_puts(t->str, t->len, put, cl, flag, width, precision);
}
