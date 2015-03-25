
#ifndef CSE788_GORDIAN_H_INCLUDED
#define CSE788_GORDIAN_H_INCLUDED

/**
 * @package 
 * @file cse788_gordian.h
 * @brief Gordian placement
 */

/* Entity typedef */

typedef struct cse788_gordian_t cse788_gordian_t;

typedef struct cse788_gordian_report_i  {
        void *inst;
        int (*inclevel)(void *inst);
        int (*postcell)(void *inst, int ID, int movable, double cw, double ch,
                        double dimx, double dimy, double posx, double posy);
        int (*postline)(void *inst, double cw, double ch, 
                        double srcx, double srcy, double dstx, double dsty);
        int (*postpart)(void *inst, double cw, double ch, 
                        double x, double y, double w, double h);
        int (*present)(void *inst);
} cse788_gordian_report_i;

/* Public methods */

extern struct cse788_gordian_t *
cse788_gordian_new(const char *file, struct cse788_gordian_report_i *r);

extern void cse788_gordian_del(struct cse788_gordian_t *t);

#endif


