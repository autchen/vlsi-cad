/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_TABLE_H_INCLUDED
#define PIE_TABLE_H_INCLUDED

/**
 * @package plus
 * @file pie_table.h
 * @brief Hash table interface
 */

#include "pie_config.h"

/* Entity typedef */

typedef struct PieTable PieTable;

/* Exported exception */

extern const struct PieExcept PieMem_failed;

/* Public methods */

extern struct PieTable *
PieTable_new(int hint, int (*cmp)(const void *p1, const void *p2),
             unsigned (*hash)(const void *key));

extern void PieTable_del(struct PieTable **self);

extern void *PieTable_put(struct PieTable *self, const void *key, void *val);

extern void *PieTable_sub(struct PieTable *self, const void *key, void *val);

extern void *PieTable_get(struct PieTable *self, const void *key);

extern void *PieTable_remove(struct PieTable *self, const void *key);

extern int PieTable_total(struct PieTable *self);

extern void 
PieTable_apply(struct PieTable *self,
               void (*func)(const void *key, void *val, void *data),
               void *data);

extern void PieTable_adjust(struct PieTable *self);

extern void PieTable_statLog(struct PieTable *self);

/* Utilities */

extern int PieTable_longcmp(const void *p1, const void *p2);
extern unsigned PieTable_longhash(const void *key);

extern int PieTable_atomcmp(const void *p1, const void *p2);
extern unsigned PieTable_atomhash(const void *key);

#endif /* PIE_TABLE_H_INCLUDED */
