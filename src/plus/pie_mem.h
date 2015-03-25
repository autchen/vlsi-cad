/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_MEM_H_INCLUDED
#define PIE_MEM_H_INCLUDED

/**
 * @package plus
 * @file pie_mem.h
 * @brief Memory allocation interfaces
 */

#include "pie_config.h"

/* Exported exception */

extern const struct PieExcept PieMem_failed;

/* Macro wrapped interfaces */

/**
 * malloc function, S long 
 * @see PieMem_malloc
 */
#define PIE_MALLOC(S) PieMem_malloc((S), __FILE__, __LINE__)

/**
 * calloc function, N long, S long
 * @see PieMem_calloc
 */
#define PIE_CALLOC(N, S) PieMem_calloc((N), (S), __FILE__, __LINE__)

/**
 * malloc according to data size, T type
 * @see PieMem_malloc
 */
#define PIE_NEW(T) ((T *)PIE_MALLOC((long)sizeof(T)))

/**
 * new and reset, T type
 * @see PieMem_calloc
 */
#define PIE_NEW0(T) ((T *)PIE_CALLOC(1, (long)sizeof(T)))

/**
 * realloc function, P variable, S long
 * @see PieMem_resize
 */
#define PIE_RESIZE(P, S) PieMem_resize((P), (S), __FILE__, __LINE__)

/**
 * Free and reset the pointer, P variable
 * @see PieMem_free
 */
#define PIE_FREE(P) ((void)(PieMem_free((P)), (P) = PIE_NULL))

/* Public methods */

/**
 * @brief Memory allocation function.
 *
 * @param size Required memory chunk size;
 * @param file Function invoking spot;
 * @param line Function invoking spot;
 *
 * @return Pointer to allocated space.
 * @except PieMem_failed
 */
extern void *PieMem_malloc(long size, const char *file, int line);

/**
 * @brief Allocate (num * size) bytes and reset the content to '\0'
 *
 * @param num Number of required chunks;
 * @param size Size per chunk;
 * @param file Function invoking spot;
 * @param line Function invoking spot;
 *
 * @return Pointer to allocated space.
 * @except PieMem_failed
 */
extern void *PieMem_calloc(long num, long size, const char *file, int line);

/**
 * @brief Free space pointed by "ptr" and re-allocate "size" bytes
 *
 * @param ptr Original pointer;
 * @param size The new requested memory size;
 * @param file Function invoking spot;
 * @param line Function invoking spot;
 *
 * @return Pointer to allocated space.
 * @except PieMem_failed
 */
extern void *PieMem_resize(void *ptr, long size, const char *file, int line);

/**
 * @brief Free space pointed by ptr.
 *
 * @param ptr
 */
extern void PieMem_free(void *ptr);

/* Debug */

/**
 * @brief Logging memroy allocation history.
 *
 * This function is only supportted when debug level >= 2; An error message
 * will be wriiten on trying to access in production implementation.
 */
extern void PieMem_log(void);

#endif /* PIE_MEM_H_INCLUDED */


