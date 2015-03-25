
#ifndef CSE788_FLOORPLAN_H_INCLUDED
#define CSE788_FLOORPLAN_H_INCLUDED

/**
 * @package 
 * @file cse788_floorplan.h
 * @brief Floorplan with simulated annealing
 */

#include "pie_config.h"

/* Exported exceptions */

extern const struct PieExcept cse788_floorplan_failed;
extern const struct PieExcept PieMem_failed;
extern const struct PieExcept PieFile_failed;

/* Entity typedef */

typedef struct cse788_floorplan_t cse788_floorplan_t;

typedef struct cse788_floorplan_report_i {
        void *inst;
        void (*plotLimit)(void *inst, double p, double q);
        void (*plotSlice)(void *inst, double x, double y, double w, double h);
        void (*plotModule)(void *inst, double x, double y, double w, double h,
                           int index);
        void (*present)(void *inst);
} cse788_floorplan_report_i;

/* Public methods */

extern struct cse788_floorplan_t *cse788_floorplan_new(const char *path);

extern void cse788_floorplan_del(struct cse788_floorplan_t **t);

extern void 
cse788_floorplan_show_progress(struct cse788_floorplan_t *t, PieBool a);

extern double 
cse788_floorplan_annealing(struct cse788_floorplan_t *t, double temp0, 
                           double temp_e, double temp_r, double accept_e, 
                           int m, const struct cse788_floorplan_report_i *rp);

#endif


