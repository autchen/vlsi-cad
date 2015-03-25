/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_FMT_H_INCLUDED
#define PIE_FMT_H_INCLUDED

/**
 * @package plus
 * @file pie_fmt.h
 * @brief Formatted string output functions.
 */

#include "pie_config.h"
#include <stdio.h>
#include <stdarg.h>

/* Flag state manipulate */

typedef PieUint32 PieFmtFlag[8];

/* F[C / 32] |= (1 << C % 32) */
#define PIE_FMT_SET(F, C) ((F)[(C) >> 5] |= (0x0001 << ((C) & 0x1f)))
#define PIE_FMT_ISSET(F, C) (((F)[(C) / 32] >> ((C) % 32)) & 0x0001)
#define PIE_FMT_ZEROS(F) \
        do { \
                (F)[0] = 0; (F)[1] = 0; (F)[2] = 0; (F)[3] = 0; \
                (F)[4] = 0; (F)[5] = 0; (F)[6] = 0; (F)[7] = 0; \
        } while (0)

/* Interface function */

/**
 * @brief Fmt callback for customized type outputs.
 *
 * @param code Character denotes the specific type;
 * @param app Pointer to variable list;
 * @param put Output interface of formatted string;
 * @param cl Closure parameter for put function;
 * @param flag Flag state, use macro PIE_FMT_ISSET and PIE_FMT_SET for querying 
 *        or setting up;
 * @param width Format width;
 * @param precision Format precision.
 */
typedef void (*PieFmt)(int code, va_list *app,
                       int (*put)(int c, void *cl), void *cl, 
                       PieFmtFlag flag, int width, int precision);

/* Public methods */

/**
 * @brief Formatted output function.
 *
 * @param put Output interface of formatted string;
 * @param cl Closure parameter for put function
 * @param fmt Format string;
 * @param ... Variable list.
 */
extern void 
PieFmt_out(int (*put)(int c, void *cl), void *cl, const char *fmt, ...);

/**
 * @brief Formatted output function.
 *
 * @param put Output interface of formatted string;
 * @param cl Closure parameter for put function
 * @param fmt Format string;
 * @param ap Variable list pointer.
 */
extern void 
PieFmt_vout(int (*put)(int c, void *cl), void *cl, const char *fmt, va_list ap);

/**
 * @brief Formatted output to stdout.
 *
 * @param fmt Format string;
 * @param ... Variable list.
 */
extern void PieFmt_print(const char *fmt, ...);

/**
 * @brief Formatted output to specified file stream.
 *
 * @param file Pointer to output stream;
 * @param fmt Format string;
 * @param ... Variable list.
 */
extern void PieFmt_fprint(FILE *file, const char *fmt, ...);

/**
 * @brief Formatted output to indicated buffer.
 *
 * @param buf Output buffer;
 * @param n Maximum length of the buffer; if the result exceeds this limit,
 *        the output will be clampped;
 * @param fmt Format string;
 * @param ... Variable list.
 *
 * @return Length of complete output, including overflow bytes.
 */
extern int PieFmt_str(char *buf, int n, const char *fmt, ...);

/**
 * @brief Formatted output to indicated buffer.
 *
 * @param buf Output buffer;
 * @param n Maximum length of the buffer; if the result exceeds this limit,
 *        the output will be clampped;
 * @param fmt Format string;
 * @param ap Variable list pointer.
 *
 * @return Length of complete output, including overflow bytes.
 */
extern int PieFmt_vstr(char *buf, int n, const char *fmt, va_list ap);

/* Utility functions */

extern void 
PieFmt_puts(const char *str, int len, int (*put)(int c, void *cl), void *cl,
            PieFmtFlag flag, int width, int precision);

extern void 
PieFmt_putd(const char *str, int len, int (*put)(int c, void *cl), void *cl,
            PieFmtFlag flag, int width, int precision);

/**
 * @brief Register a Fmt function in the global character map.
 *
 * This method manipulate static variable, and should not be invoked from 
 * multiple threads.
 *
 * @param code Character for % mapping;
 * @param fp Pointer to Fmt function
 *
 * @return The last Fmt entry in the table.
 */
extern PieFmt PieFmt_register(int code, PieFmt fp);

#endif

