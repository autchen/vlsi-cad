
#ifndef CSE788_DISPLAY_H_INCLUDED
#define CSE788_DISPLAY_H_INCLUDED

/**
 * @package 
 * @file cse788_display.h
 * @brief SDL display for placement
 */

/* Entity typedef */

typedef struct cse788_display_t cse788_display_t;

/* Public methods */

extern struct cse788_display_t *cse788_display_new(int w, int h);

extern void cse788_display_del(struct cse788_display_t *t);

struct cse788_gordian_report_i;

extern struct cse788_gordian_report_i *
cse788_display_gordian_interface(struct cse788_display_t *t);

extern void cse788_display_hold(struct cse788_display_t *t);

/*  */

typedef struct cse788_display2_t cse788_display2_t;

struct cse788_floorplan_report_i;

extern struct cse788_display2_t *cse788_display2_new(int w, int h);

extern void cse788_display2_del(struct cse788_display2_t **t);

extern struct cse788_floorplan_report_i *
cse788_display2_floorplan_interface(struct cse788_display2_t *t);

#endif


