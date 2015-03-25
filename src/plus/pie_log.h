/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_LOG_H_INCLUDED
#define PIE_LOG_H_INCLUDED

/**
 * @package plus
 * @file pie_log.h
 * @brief Unified interface for logging.
 */

#include "pie_config.h"

/* Public methods */

extern void PieLog_print(const char *fmt, ...);

extern void PieLog_dump(void *data, long size);

extern void PieLog_trace(const char *file, int line);

/* Trace wrapping */

#if PIE_ENABLE_TRACE == 1
#define PIE_TRACE() PieLog_trace(__FILE__, __LINE__)
#else
#define PIE_TRACE() ((void)0)
#endif /* PIE_ENABLE_TRACE */

extern void PieLog_setOutput(int (*put)(int c, void *cl), void *cl);

#endif /* PIE_LOG_H_INCLUDED */


