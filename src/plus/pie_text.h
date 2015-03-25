/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_TEXT_H_INCLUDED
#define PIE_TEXT_H_INCLUDED

/**
 * @package plus
 * @file pie_text.h
 * @brief Text processing routines
 */

#include "pie_config.h"

/* Exported entity */

typedef struct PieText {
        int len;
        const char *str;
} PieText;

typedef struct PieTextBuffer PieTextBuffer;

/* Exported exception */

extern const struct PieExcept PieMem_failed;

/* Exported char sets */

extern const struct PieText PieText_cset;
extern const struct PieText PieText_ascii;
extern const struct PieText PieText_ucase;
extern const struct PieText PieText_lcase;
extern const struct PieText PieText_digit;
extern const struct PieText PieText_null;

/* Public methods */

/* Text creation/manipulation */

extern struct PieText PieText_new(struct PieText t, struct PieTextBuffer *buf);

extern int PieText_idx(struct PieText t, int pos);

extern struct PieText PieText_box(const char *str, int len);

extern struct PieText PieText_sub(struct PieText t, int i, int j);

extern struct PieText PieText_str(const char *str, struct PieTextBuffer *buf);

extern char *PieText_toStr(struct PieText t, char *str, int len);

extern struct PieText 
PieText_dup(struct PieText t, int num, struct PieTextBuffer *buf);

extern struct PieText 
PieText_cat(struct PieText t1, struct PieText t2, struct PieTextBuffer *buf);

extern struct PieText 
PieText_reverse(struct PieText t, struct PieTextBuffer *buf);

extern struct PieText 
PieText_map(struct PieText t, const struct PieText *from, 
            const struct PieText *to, struct PieTextBuffer *buf);

/* Text analysis */

extern int PieText_cmp(struct PieText t1, struct PieText t2);

extern int PieText_lchr(struct PieText t, int i, int j, char c);
extern int PieText_rchr(struct PieText t, int i, int j, char c);

extern int PieText_lset(struct PieText t, int i, int j, struct PieText set);
extern int PieText_rset(struct PieText t, int i, int j, struct PieText set);

extern int PieText_lstr(struct PieText t, int i, int j, struct PieText str);
extern int PieText_rstr(struct PieText t, int i, int j, struct PieText str);

extern int PieText_lstep(struct PieText t, int i, int j, struct PieText set);
extern int PieText_rstep(struct PieText t, int i, int j, struct PieText set);

extern int PieText_lmatch(struct PieText t, int i, int j, struct PieText str);
extern int PieText_rmatch(struct PieText t, int i, int j, struct PieText str);

/* Text utility */

extern struct PieText PieText_trim(struct PieText t, int i, int j);

extern struct PieText 
PieText_basename(struct PieText t, int i, int j, struct PieText suffix);

/* Text memory management */

extern struct PieTextBuffer *PieText_newBuffer(long expand);

extern void PieText_delBuffer(struct PieTextBuffer **buf);

#endif


