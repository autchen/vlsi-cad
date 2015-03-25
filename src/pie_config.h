#ifndef PIE_CONFIG_H_INCLUDED
#define PIE_CONFIG_H_INCLUDED

/**
 * @package 
 * @file pie_config.h
 * @brief Global configurations
 */

/**
 * Platform information
 */

#define PIE_VERSION 0.1
#define PIE_PLATFORM_WIN32 1
#define PIE_MULTI_THREADED 0
#define PIE_PLATFORM_ARGON 0

/**
 * Debug information
 * 0 - unprotected
 * 1 - release
 * 2 - debug 
 * 3 - stringent
 */
#define PIE_DEBUG_LEVEL  2
#define PIE_ENABLE_TRACE 0

/**
 * Primitives
 */

typedef enum {
        PIE_FALSE = 0,
        PIE_TRUE
} PieBool;

typedef char               PieSint8;
typedef unsigned char      PieUint8;
typedef short              PieSint16;
typedef unsigned short     PieUint16;
typedef int                PieSint32;
typedef unsigned int       PieUint32;
typedef long long          PieSint64;
typedef unsigned long long PieUint64;

typedef unsigned long      PieSize;
typedef unsigned long      PieUptr;

#define PIE_NULL ((void *)0)

/* @see plus/pie_atom.h */
typedef const struct PieOpaque0 *PieAtom;

/* @see plus/pie_except.h */
typedef const struct PieExcept *PieErr;
#define PIE_OK PIE_NULL

/**
 * Helpers
 */

#define PIE_ARRAY_SIZE(A) ((sizeof((A))) / sizeof((A)[0]))

union PieAlign {
        int i; long l; long long ll;
        float f; double d; long double ld;
        void *p; void (*fp)(void);
};

#define PIE_ALIGN (sizeof(union PieAlign))
#define PIE_ALIGNED(S) ((((S) + PIE_ALIGN - 1) / PIE_ALIGN) * PIE_ALIGN)

#endif


