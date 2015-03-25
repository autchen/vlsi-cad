/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_assert.h"
#include "plus/pie_except.h"

/* Exception entity */

const struct PieExcept PieAssert_failed = {"Assertion Failed"};

/* Public methods */

PieBool PieAssert_exclude(long ixGuard)
{
        /* TODO */
        return PIE_TRUE;
}
