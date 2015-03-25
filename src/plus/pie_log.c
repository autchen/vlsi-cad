/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_log.h"

#include "plus/pie_fmt.h"
#include "plus/pie_assert.h"

#include <stdarg.h>
#include <stdio.h>

static int (*_logput)(int c, void *cl) = PIE_NULL;
static void *_logcl = PIE_NULL;

#define PUT_CHECK \
        do { \
                if (!_logput) { \
                        _logput = (int (*)(int, void *))fputc; \
                        _logcl = (void *)stderr; \
                } \
        } while (0)

void PieLog_print(const char *fmt, ...)
{
        va_list ap;
        PUT_CHECK;
        va_start(ap, fmt);
        PieFmt_vout(_logput, _logcl, fmt, ap);
        va_end(ap);
}

void PieLog_dump(void *data, long size)
{
        PieUint32 i;
        const char *ptr = (const char *)data;
        PIE_ASSERT(data && size > 0);
        PUT_CHECK;
        PieFmt_out(_logput, _logcl, "PieLog_dump: %#p\n", data);
        for (i = 0; i < size; i++) {
                PieFmt_out(_logput, _logcl, "%02x ", (0xff & ptr[i]));
                if (0 == ((i + 1) & 0x0f))
                        _logput('\n', _logcl);
        }
        _logput('\n', _logcl);
}

void PieLog_trace(const char *file, int line)
{
        PUT_CHECK;
        PieFmt_out(_logput, _logcl, "trace at %s:%d\n", file, line);
}

void PieLog_setOutput(int (*put)(int c, void *cl), void *cl)
{
        PIE_ASSERT(put);
        PIE_ASSERT_IX;
        _logput = put;
        _logcl = cl;
}
