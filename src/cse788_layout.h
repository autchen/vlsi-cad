/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef CSE788_LAYOUT_H_INCLUDED
#define CSE788_LAYOUT_H_INCLUDED

/**
 * @package 
 * @file cse788_layout.h
 * @brief Layout
 */

#include "pie_config.h"

/* Entity typedef */

typedef struct cse788_layout_t cse788_layout_t;

/* Public methods */

#include "cse788_netlist.h"

extern struct cse788_layout_t *
cse788_layout_new(char *seq, struct cse788_gate_conn *conn);

extern void cse788_layout_magic(struct cse788_layout_t *t, FILE *stream);

#endif


