/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_ASSERT_H_INCLUDED
#define PIE_ASSERT_H_INCLUDED

/**
 * @package plus
 * @file pie_assert.h
 * @brief Assertions with debug level.
 */

#include "pie_config.h"
#include "plus/pie_except.h"

/* Exported exception */

extern const struct PieExcept PieAssert_failed;

/**
 * On assertion failed, this module raises an uncaught exception.
 *
 * @see plus/pie_except.h 
 */
#define PIE_ASSERT_ENABLED(C)  ((void)((C) || ( \
        PieExcept_raise(&PieAssert_failed, __FILE__, __LINE__, #C), 0)))

#define PIE_ASSERT_DISABLED(C) ((void)0)

/**
 * Assertion for thread exclusive access, e.g. static variables.
 */
#if PIE_MULTI_THREADED == 1
extern PieBool PieAssert_exclude(long ixGuard);

#define PIE_ASSERT_IX_ENABLED \
        do { \
                static long ixGuard = 0; \
                PIE_ASSERT_ENABLED(PieAssert_exclude(ixGuard)); \
        } while (0)

#else
#define PIE_ASSERT_IX_ENABLED  ((void)0)

#endif /* PIE_MULTI_THREADED */

/* Extreme mode */
#if PIE_DEBUG_LEVEL >= 3
#define PIE_ASSERT              PIE_ASSERT_ENABLED
#define PIE_ASSERT_RELEASE      PIE_ASSERT_ENABLED
#define PIE_ASSERT_EXTREME      PIE_ASSERT_ENABLED
#define PIE_ASSERT_IX           PIE_ASSERT_IX_ENABLED

/* Debug mode */
#elif PIE_DEBUG_LEVEL == 2
#define PIE_ASSERT              PIE_ASSERT_ENABLED
#define PIE_ASSERT_RELEASE      PIE_ASSERT_ENABLED
#define PIE_ASSERT_EXTREME      PIE_ASSERT_DISABLED
#define PIE_ASSERT_IX           PIE_ASSERT_IX_ENABLED

/* Release mode */
#elif PIE_DEBUG_LEVEL == 1
#define PIE_ASSERT              PIE_ASSERT_DISABLED
#define PIE_ASSERT_RELEASE      PIE_ASSERT_ENABLED
#define PIE_ASSERT_EXTREME      PIE_ASSERT_DISABLED
#define PIE_ASSERT_IX           PIE_ASSERT_DISABLED(0)

/* Unprotected */
#else
#define PIE_ASSERT              PIE_ASSERT_DISABLED
#define PIE_ASSERT_RELEASE      PIE_ASSERT_DISABLED
#define PIE_ASSERT_EXTREME      PIE_ASSERT_DISABLED
#define PIE_ASSERT_IX           PIE_ASSERT_DISABLED(0)

#endif /* PIE_DEBUG_LEVEL */

#endif /* PIE_ASSERT_H_INCLUDED */

