
#ifndef CSE788_NETLIST_H_INCLUDED
#define CSE788_NETLIST_H_INCLUDED

/**
 * @package 
 * @file cse788_netlist.h
 * @brief Generate netlist and gate sequence from boolean function.
 */

#include <stdio.h>

/* Entity typedef */

typedef struct cse788_netlist_t cse788_netlist_t;

/* Public methods */

extern struct cse788_netlist_t *cse788_netlist_new(const char *func_str);

extern void cse788_netlist_del(struct cse788_netlist_t **t);

extern int 
cse788_netlist_gate_seq(struct cse788_netlist_t *t, char *out, int len);

extern int cse788_netlist_list(struct cse788_netlist_t *t, FILE *stream);

#include "pie_config.h"

typedef struct cse788_gate_conn {
        char gate;
        PieAtom npins[2];
        PieAtom ppins[2];
} cse788_gate_conn;

extern int 
cse788_netlist_list_s(struct cse788_netlist_t *t, 
                      struct cse788_gate_conn **conn);

#endif


