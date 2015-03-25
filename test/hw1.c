/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

/**
 * @testbench
 * @brief Homework 1 layout generation
 */

#include <stdio.h>
#include <stdlib.h>

#include "cse788_netlist.h"

int main(int argc, char *argv[])
{

        if (argc == 2) {
                int retval;
                struct cse788_netlist_t *t;
                char seq[200];

                t = cse788_netlist_new(argv[1]);
                if (t == NULL)
                        return 3;
                retval = cse788_netlist_gate_seq(t, seq, 200);
                if (0 != retval) {
                        printf("err: netlist %d\n", retval);
                        return 3;
                }
                printf("gate_seq: %s\n", seq);
                retval = cse788_netlist_list(t, stdout);
                if (0 != retval) {
                        printf("err: netlist %d\n", retval);
                        return 3;
                }
                cse788_netlist_del(&t);
        }

        return 0;
}


