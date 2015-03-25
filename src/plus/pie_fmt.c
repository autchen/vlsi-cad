/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_fmt.h"
#include "plus/pie_assert.h"

#include <stdio.h>
#include <float.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>

/* Context */

#define FMT_PAD(L, C) \
        do { \
                int NN = (L); \
                while (NN-- > 0) \
                        put((C), cl); \
        } while (0)

static void 
convMap_c(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('c' == code);
        if (!PIE_FMT_ISSET(flag, '-'))
                FMT_PAD(width - 1, ' ');
        put((unsigned char)(va_arg(*app, int)), cl);
        if (PIE_FMT_ISSET(flag, '-'))
                FMT_PAD(width - 1, ' ');
}

static void 
convMap_s(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char *str;
        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('s' == code);
        str = va_arg(*app, char *);
        PIE_ASSERT(str);
        PieFmt_puts(str, strlen(str), put, cl, flag, width, precision);
}

#define D_TO_STR(T, MAX1, MIN) \
        do { \
                T val = va_arg(*app, T); \
                unsigned T m; \
                if ((MIN) == val) \
                        m = (MAX1); \
                else if (val < 0) \
                        m = -val; \
                else \
                        m = val; \
                do { \
                        *(--str) = m % 10 + '0'; \
                } while ((m /= 10) > 0); \
                if (val < 0) \
                        *(--str) = '-'; \
                else \
                        *(--str) = '+'; \
        } while (0)

static void 
convMap_d(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[44], *str = buf + sizeof buf;

        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('d' == code);

        if (PIE_FMT_ISSET(flag, 'l'))
                D_TO_STR(long, LONG_MAX + 1UL, LONG_MIN);
        else
                D_TO_STR(int, INT_MAX + 1U, INT_MIN);

        PieFmt_putd(str, buf + sizeof buf - str, put, cl, flag, width, 
                        precision);
}

#define U_TO_STR(T) \
        do { \
                T val = va_arg(*app, T); \
                do { \
                        *(--str) = val % 10 + '0'; \
                } while ((val /= 10) != 0); \
        } while (0)

static void 
convMap_u(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[44], *str = buf + sizeof buf;

        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('u' == code);

        if (PIE_FMT_ISSET(flag, 'l'))
                U_TO_STR(unsigned long);
        else
                U_TO_STR(unsigned int);

        PieFmt_putd(str, buf + sizeof buf - str, put, cl, flag, width, 
                        precision);
}

#define O_TO_STR(T, B) \
        do { \
                T val = va_arg(*app, T); \
                do { \
                        *(--str) = (val & (B)) + '0'; \
                } while ((val >>= 3) != 0); \
                if (PIE_FMT_ISSET(flag, '#')) \
                        *(--str) = 'O'; \
        } while (0)

static void 
convMap_o(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[44], *str = buf + sizeof buf;

        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('o' == code);

        if (PIE_FMT_ISSET(flag, 'l'))
                O_TO_STR(unsigned long, 0x7L);
        else
                O_TO_STR(unsigned int, 0x7);

        PieFmt_putd(str, buf + sizeof buf - str, put, cl, flag, width, 
                        precision);
}

static const char *_hexBase = "0123456789abcdef";

#define X_TO_STR(T, B) \
        do { \
                T val = va_arg(*app, T); \
                do { \
                        *(--str) = _hexBase[val & B]; \
                } while ((val >>= 4) != 0); \
                if (PIE_FMT_ISSET(flag, '#')) \
                        *(--str) = 'X'; \
        } while (0)

static void 
convMap_x(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[44], *str = buf + sizeof buf;

        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('x' == code);

        if (PIE_FMT_ISSET(flag, 'l'))
                X_TO_STR(unsigned long, 0xfL);
        else
                X_TO_STR(unsigned int, 0xf);

        PieFmt_putd(str, buf + sizeof buf - str, put, cl, flag, width, 
                        precision);
        
}

static void 
convMap_p(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[44], *str = buf + sizeof buf;

        PIE_ASSERT(app);
        PIE_ASSERT_EXTREME('p' == code);
        X_TO_STR(unsigned long, 0xfL);

        PieFmt_putd(str, buf + sizeof buf - str, put, cl, flag, width, 
                        2 * sizeof(void *));
}

static void 
convMap_f(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
           PieFmtFlag flag, int width, int precision)
{
        char buf[DBL_MAX_10_EXP + 1 + 1 + 99 + 1];
                /* Integer part + . + decimal + \0 */
        char fmt[] = "%.ddc";

        PIE_ASSERT(precision <= 99);
        PIE_ASSERT(app);
        if (-1 == precision)
                precision = 6;
        fmt[4] = code;
        fmt[3] = precision % 10 + '0';
        fmt[2] = precision / 10 + '0';
        sprintf(buf, fmt, va_arg(*app, double));
        PieFmt_putd(buf, strlen(buf), put, cl, flag, width, -1);
}

extern void 
PieExcept_fmt(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
              PieFmtFlag flag, int width, int precision);

extern void 
PieText_fmt(int code, va_list *app, int (*put)(int c, void *cl), void *cl,
            PieFmtFlag flag, int width, int precision);

static PieFmt _convMap[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        PieExcept_fmt, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, PieText_fmt, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, convMap_c, convMap_d, 
        convMap_f, convMap_f, convMap_f, 0, 0, 0, 0, 0, 0, 0, convMap_o, 
        convMap_p, 0, 0, convMap_s, 0, convMap_u, 0, 0, convMap_x, 0, 0, 0, 0, 
        0, 0, 0 };

/* Public methods */

void PieFmt_out(int (*put)(int c, void *cl), void *cl, const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        PieFmt_vout(put, cl, fmt, ap);
        va_end(ap);
}

#define FMT_READ_NUM(X) \
        do { \
                int n = 0; \
                if ('*' == *fmt) { \
                        n = va_arg(ap, int); \
                        fmt++; \
                } else if (isdigit(*fmt)) { \
                        do { \
                                int d = *(fmt++) - '0'; \
                                n = 10 * n + d; \
                                PIE_ASSERT(n <= INT_MAX); \
                        } while (isdigit(*fmt)); \
                } \
                (X) = n; \
        } while (0)

void 
PieFmt_vout(int (*put)(int c, void *cl), void *cl, const char *fmt, va_list ap)
{
        static const char *flagSet = "-+ #0";
        va_list ap1;

        PIE_ASSERT(fmt);
        PIE_ASSERT(put);

#if PIE_PLATFORM_ARGON == 1
        ap1[0] = *ap;
#else
        ap1 = ap;
#endif

        while (*fmt) {
                if ('%' != *fmt || '%' == *(++fmt))
                        put((unsigned char)(*(fmt++)), cl);
                else {
                        PieFmtFlag flag;
                        int width = -1, precision = -1;
                        unsigned char c = *fmt;
                        
                        /* Flag */
                        memset(flag, 0, sizeof flag);
                        while (c && strchr(flagSet, c)) {
                                PIE_FMT_SET(flag, c);
                                c = *(++fmt);
                        }

                        /* Width */
                        FMT_READ_NUM(width);

                        /* Precision */
                        if ('.' == *fmt) {
                                fmt++;
                                FMT_READ_NUM(precision);
                        }

                        /* Length */
                        if ('l' == *fmt) {
                                PIE_FMT_SET(flag, *fmt);
                                fmt++;
                        }

                        c = *(fmt++);
                        PIE_ASSERT(_convMap[c]);
                        _convMap[c](c, &ap1, put, cl, flag, width, precision);
                }
        }
}

void PieFmt_print(const char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        PieFmt_vout((int (*)(int, void *))fputc, stdout, fmt, ap);
        va_end(ap);
}

void PieFmt_fprint(FILE *file, const char *fmt, ...)
{
        va_list ap;
        PIE_ASSERT(file);
        va_start(ap, fmt);
        PieFmt_vout((int (*)(int, void *))fputc, file, fmt, ap);
        va_end(ap);
}

int PieFmt_str(char *buf, int n, const char *fmt, ...)
{
        int len;
        va_list ap;
        va_start(ap, fmt);
        len = PieFmt_vstr(buf, n, fmt, ap);
        va_end(ap);
        return len;
}

struct PieFmtBuf {
        char *buf;
        char *ptr;
        int size;
};

static int PieFmt_putc2b(int code, void *inst)
{
        struct PieFmtBuf *buf = (struct PieFmtBuf *)inst;
        PIE_ASSERT(inst);
        if (buf->ptr - buf->buf < buf->size)
                *(buf->ptr) = (unsigned char)code;
        else
                return 0;
        (buf->ptr)++;
        return 1;
}

int PieFmt_vstr(char *buf, int n, const char *fmt, va_list ap)
{
        struct PieFmtBuf data;

        PIE_ASSERT(buf);
        PIE_ASSERT(n > 0);

        data.buf = buf;
        data.ptr = buf;
        data.size = n;
        PieFmt_vout(PieFmt_putc2b, &data, fmt, ap);
        PieFmt_putc2b((int)'\0', &data);
        return data.ptr - data.buf;
}

/* Utility functions */

#define PUT_STR(STR, LEN) \
        do { \
                int i; \
                for (i = 0; i < (LEN); i++) \
                        put((int)(STR)[i], cl); \
        } while (0)

void 
PieFmt_puts(const char *str, int len, int (*put)(int c, void *cl), void *cl,
            PieFmtFlag flag, int width, int precision)
{
        PIE_ASSERT(str && len >= 0);
        PIE_ASSERT(put);
        PIE_ASSERT(width >= -1 && precision >= -1);
        PIE_ASSERT(flag);

        if (-1 == precision)
                precision = INT_MAX;
        if (len > precision)
                len = precision;
        if (!PIE_FMT_ISSET(flag, '-'))
                FMT_PAD(width - len, ' ');
        PUT_STR(str, len);
        if (PIE_FMT_ISSET(flag, '-'))
                FMT_PAD(width - len, ' ');
}

#define FMT_SIGN_OX \
        do { \
                if (sign) \
                        put((unsigned char)sign, cl); \
                if (ox > 0) { \
                        put('0', cl); \
                        if (2 == ox) \
                                put('x', cl); \
                } \
        } while (0)

void 
PieFmt_putd(const char *str, int len, int (*put)(int c, void *cl), void *cl,
            PieFmtFlag flag, int width, int precision)
{
        int sign = 0, ox = 0, n;

        PIE_ASSERT(str && len >= 0);
        PIE_ASSERT(put);
        PIE_ASSERT(flag && width >= -1 && precision >= -1);

        if ('-' == *str) {
                sign = *(str++);
                len--;
        } else if ('+' == *str) {
                if (PIE_FMT_ISSET(flag, '+'))
                        sign = '+';
                else if (PIE_FMT_ISSET(flag, ' '))
                        sign = ' ';
                str++;
                len--;
        }

        if ('O' == *str) {
                ox = 1;
                str++;
                len--;
        } else if ('X' == *str) {
                ox = 2;
                str++;
                len--;
        }

        if (-1 == precision)
                precision = 1;
        if (len < precision)
                n = precision;
        else if (0 == precision && 1 == len && '0' == *str)
                n = 0;
        else
                n = len;

        if (!PIE_FMT_ISSET(flag, '-')) {
                if (!PIE_FMT_ISSET(flag, '0')) {
                        FMT_PAD(width - (sign ? 1 : 0) - ox - n, ' ');
                        FMT_SIGN_OX;
                } else {
                        FMT_SIGN_OX;
                        FMT_PAD(width - (sign ? 1 : 0) - ox - n, '0');
                }
        } else
                FMT_SIGN_OX;
        FMT_PAD(precision - len, '0');
        PUT_STR(str, len);
        if (PIE_FMT_ISSET(flag, '-'))
                FMT_PAD(width - n, ' ');
}

PieFmt PieFmt_register(int code, PieFmt fp)
{
        PieFmt ptr;
        PIE_ASSERT_IX;
        PIE_ASSERT(0 < code && code < PIE_ARRAY_SIZE(_convMap));
        ptr = _convMap[code];
        _convMap[code] = fp;
        return ptr;
}

