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
 * @brief hw3 main
 */

#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "cse788_floorplan.h"
#include "cse788_display.h"

int main(int argc, char *argv[])
{
        struct cse788_floorplan_t *fp;
        struct cse788_display2_t *dp1, *dp2;
        double temp0, temp_e, temp_r, accept_e;
        int m, ww, wh, show, n, i;
        double cost1, cost2;

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

        fp = cse788_floorplan_new(argv[1]);

        sscanf(argv[2], "%lf", &temp0);
        sscanf(argv[3], "%lf", &temp_e);
        sscanf(argv[4], "%lf", &temp_r);
        sscanf(argv[5], "%lf", &accept_e);
        sscanf(argv[6], "%d", &m);
        sscanf(argv[7], "%d", &ww);
        sscanf(argv[8], "%d", &wh);
        sscanf(argv[9], "%d", &show);
        sscanf(argv[10], "%d", &n);

        dp1 = cse788_display2_new(ww, wh);
        dp2 = PIE_NULL;

        cse788_floorplan_show_progress(fp, show == 0 ? PIE_FALSE : PIE_TRUE);
        /* cse788_floorplan_annealing_time(fp, n); */
        for (i = 0, cost1 = -1.0; i < n; i++) {
                cost2 = cse788_floorplan_annealing(fp, temp0, temp_e, temp_r, 
                                accept_e, m, 
                                cse788_display2_floorplan_interface(dp1));
                if (cost1 < 0.0 || cost2 < cost1) {
                        cost1 = cost2;
                        if (dp2)
                                cse788_display2_del(&dp2);
                        dp2 = dp1;
                        if (i < n - 1)
                                dp1 = cse788_display2_new(ww, wh);
                        else
                                dp1 = PIE_NULL;
                }
        }
        printf("annealing iter: %d, lowest cost: %.2lf\n", n, cost1);

        if (dp1)
                cse788_display2_del(&dp1);
        cse788_display_hold(PIE_NULL);
        cse788_floorplan_del(&fp);
        cse788_display2_del(&dp2);
        SDL_Quit();

        return 0;
}


