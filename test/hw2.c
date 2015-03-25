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
 * @brief Homework 2 
 */

#include <stdio.h>
#include <stdlib.h>

#include "cse788_gordian.h"
#include "cse788_display.h"

int main(int argc, char *argv[])
{
        struct cse788_gordian_t *t;
        struct cse788_display_t *d;
        struct cse788_gordian_report_i *r;
        int w, h;

        sscanf(argv[2], "%d", &w);
        sscanf(argv[3], "%d", &h);
        d = cse788_display_new(w, h);
        if (d == NULL)
                return 3;
        r = cse788_display_gordian_interface(d);
        t = cse788_gordian_new(argv[1], r);
        if (t == NULL)
                return 3;
        cse788_display_hold(d);
        cse788_display_del(d);
        cse788_gordian_del(t);

        return 0;
}


