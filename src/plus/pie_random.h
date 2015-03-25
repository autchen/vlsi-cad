/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_RANDOM_H_INCLUDED
#define PIE_RANDOM_H_INCLUDED

/**
 * @package plus
 * @file pie_random.h
 * @brief Random number generator.
 */

#include "pie_config.h"

/* Entity typedef */

typedef struct PieRandom PieRandom;

/* Exported exception */

extern const struct PieExcept PieMem_failed;

/* Public methods */

extern struct PieRandom *PieRandom_new(unsigned int seed);

extern void PieRandom_del(struct PieRandom **self);

extern double PieRandom_gen(struct PieRandom *self);

extern double PieRandom_range(struct PieRandom *self, double low, double high);

extern int PieRandom_bin(struct PieRandom *self, double p1);

#endif


