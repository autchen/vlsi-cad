/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_random.h"

#include "plus/pie_assert.h"
#include "plus/pie_mem.h"

/**
 * Algorithm WELL1024a
 * @see http://www.iro.umontreal.ca/~lecuyer/myftp/papers/lfsr04.pdf 
 */

#define WELL_W 32
#define WELL_R 32
#define WELL_M1 3
#define WELL_M2 24
#define WELL_M3 10

struct PieRandom {
        unsigned int idx;
        unsigned int state[WELL_R];
};

struct PieRandom *PieRandom_new(unsigned int seed)
{
        int i;
        struct PieRandom *self;
        self = PIE_NEW(struct PieRandom);
        self->idx = 0;
        self->state[0] = seed;
        for (i = 1; i < WELL_R; i++) {
                self->state[i] = 0xffffffffU & (1812433253 * (self->state[i - 1] 
                                ^ (self->state[i - 1] >> 30)) + i);
        }
        return self;
}

void PieRandom_del(struct PieRandom **self)
{
        PIE_ASSERT(self);
        if (*self)
                PIE_FREE(*self);
}

#define MATOPOS(T, V) ((V) ^ ((V) >> (T)))
#define MATONEG(T, V) ((V) ^ ((V) << (-(T))))
#define IDENTITY(V) (V)

#define WELL_V0    (state[idx])
#define WELL_VM1   (state[(idx + WELL_M1) & 0x0000001fU])
#define WELL_VM2   (state[(idx + WELL_M2) & 0x0000001fU])
#define WELL_VM3   (state[(idx + WELL_M3) & 0x0000001fU])
#define WELL_VRM1  (state[(idx + 31) & 0x0000001fU])
#define WELL_NEWV0 (state[(idx + 31) & 0x0000001fU])
#define WELL_NEWV1 (state[idx])

#define WELL_FACT (2.32830643653869628906e-10)

double PieRandom_gen(struct PieRandom *self)
{
        unsigned int z0, z1, z2;
        unsigned int idx, *state;
        PIE_ASSERT(self);

        idx = self->idx;
        state = self->state;

        z0 = WELL_VRM1;
        z1 = IDENTITY(WELL_V0) ^ MATOPOS(8, WELL_VM1);
        z2 = MATONEG(-19, WELL_VM2) ^ MATONEG(-14, WELL_VM3);
        WELL_NEWV1 = z1 ^ z2;
        WELL_NEWV0 = MATONEG(-11, z0) ^ MATONEG(-7, z1) ^ MATONEG(-13, z2);
        idx = (idx + 31) & 0x0000001fU;
        self->idx = idx;

        return ((double)state[idx] * WELL_FACT);
}

double PieRandom_range(struct PieRandom *self, double low, double high)
{
        PIE_ASSERT(high > low);
        return low + (high - low) * PieRandom_gen(self);
}

int PieRandom_bin(struct PieRandom *self, double p1)
{
        PIE_ASSERT(p1 >= 0.0 && p1 <= 1.0);
        return PieRandom_gen(self) <= p1 ? 1 : 0;
}

