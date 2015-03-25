/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "pie_file.h"

#include "plus/pie_mem.h"
#include "plus/pie_assert.h"
#include "plus/pie_except.h"

#include <stdio.h>
#include <string.h>

const struct PieExcept PieFile_failed = {"File operation failed"};

#define FILE_ENROLL(P, F) \
        if (PIE_OK != PieExcept_enroll((P), (void (*)(void *))(F))) { \
                PIE_FREE((P)); \
                PIE_RAISE_INFO(PieFile_failed, \
                        ("Failed to register file against except stack")); \
        }

struct PieFileStd {
        PieBool close;
        FILE *stream;
};

static void PieFile_streamclose(FILE *stream) 
{
        if (0 != fclose((FILE *)(stream)))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to close file"));
}

static void PieFile_stdclose(struct PieFile *it)
{
        struct PieFileStd *t;
        PIE_ASSERT(it && it->close == PieFile_stdclose);

        t = (struct PieFileStd *)(it->inst);
        PieExcept_drop(t->stream);
        if (t->close)
                PieFile_streamclose(t->stream);
        PIE_FREE(t);
}

static char *PieFile_stdreadline(struct PieFile *it, char *buffer, long max)
{
        char *retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->readLine == PieFile_stdreadline);
        PIE_ASSERT(buffer && max > 0);
        t = (struct PieFileStd *)(it->inst);
        PIE_ASSERT_EXTREME(t->stream);

        buffer[max - 1] = '\0';
        retval = fgets(buffer, max, t->stream);
        if (PIE_NULL == retval && ferror(t->stream))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to read file"));
        if ('\0' != buffer[max - 1])
                PIE_RAISE_INFO(PieFile_failed, ("Buffer overflow"));

        return retval;
}

static long 
PieFile_stdread(struct PieFile *it, char *buffer, long unit, long num)
{
        long retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->read == PieFile_stdread);
        PIE_ASSERT(buffer && unit > 0 && num > 0);
        t = (struct PieFileStd *)(it->inst);
        PIE_ASSERT_EXTREME(t->stream);

        retval = fread(buffer, unit, num, t->stream);
        if (0 == retval && ferror(t->stream))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to read file"));
        return retval;
}

static const char *PieFile_stdwrites(struct PieFile *it, const char *buffer)
{
        long retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->writes == PieFile_stdwrites);
        PIE_ASSERT(buffer);
        t = (struct PieFileStd *)(it->inst);
        fflush(stdout);
        PIE_ASSERT_EXTREME(t->stream);

        retval = fputs(buffer, t->stream);
        if (retval < 0 && ferror(t->stream))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to write file"));
        return buffer;
}

static long 
PieFile_stdwrite(struct PieFile *it, const char *buffer, long unit, long num)
{
        long retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->write == PieFile_stdwrite);
        PIE_ASSERT(buffer && unit >= 0 && num >= 0);
        t = (struct PieFileStd *)(it->inst);
        PIE_ASSERT_EXTREME(t->stream);

        retval = fwrite(buffer, unit, num, t->stream);
        if (num != retval && ferror(t->stream))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to write file"));
        return retval;
}

static int PieFile_stdfmtput(int code, struct PieFile *it)
{
        int retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->fmtput == PieFile_stdfmtput);
        t = (struct PieFileStd *)(it->inst);
        PIE_ASSERT_EXTREME(t->stream);

        retval = fputc(code, t->stream);
        if (code != retval && ferror(t->stream))
                PIE_RAISE_INFO(PieFile_failed, ("Failed to write file"));
        return retval;
}

static long PieFile_stdseek(struct PieFile *it, long offset, int origin)
{
        long retval;
        struct PieFileStd *t;

        PIE_ASSERT(it && it->seek == PieFile_stdseek);
        t = (struct PieFileStd *)(it->inst);
        PIE_ASSERT_EXTREME(t->stream);

        retval = fseek(t->stream, offset, origin);
        if (0 != retval)
                PIE_RAISE_INFO(PieFile_failed, ("Failed to seek file"));
        retval = ftell(t->stream);
        if (-1L == retval)
                PIE_RAISE_INFO(PieFile_failed, ("Failed to tell file"));
        return retval;
}

struct PieFile *PieFile_open(const char *name, const char *mode)
{
        FILE *stream;
        PIE_ASSERT(name && mode);
        stream = fopen(name, mode);
        if (PIE_NULL == stream) {
                PIE_RAISE_INFO(PieFile_failed, 
                        ("Failed to open file %s", name));
        }
        return PieFile_stream(stream, PIE_TRUE);
}

struct PieFile *PieFile_stream(FILE *stream, PieBool close)
{
        struct PieFile *it;
        struct PieFileStd *t;
        PIE_ASSERT(stream);
        it = PIE_NEW(struct PieFile);
        it->close = PieFile_stdclose;
        it->readLine = PieFile_stdreadline;
        it->read = PieFile_stdread;
        it->writes = PieFile_stdwrites;
        it->write = PieFile_stdwrite;
        it->fmtput = PieFile_stdfmtput;
        it->seek = PieFile_stdseek;
        t = PIE_NEW(struct PieFileStd);
        t->close = close;
        t->stream = stream;
        it->inst = t;
        FILE_ENROLL(t->stream, PieFile_streamclose);
        return it;
}

struct PieFileMem {
        char *ptr;
        char *base, *end;
        PieBool ro;
};

static void PieFile_memclose(struct PieFile *it) {
        struct PieFileMem *t;
        PIE_ASSERT(it && it->close == PieFile_memclose);
        t = (struct PieFileMem *)(it->inst);
        PIE_FREE(t);
}

static char *PieFile_memreadline(struct PieFile *it, char *buffer, long max)
{
        struct PieFileMem *t;
        char *to, *ptr, *end;

        PIE_ASSERT(buffer && max > 0);
        PIE_ASSERT(it && it->readLine == PieFile_memreadline);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        ptr = t->ptr;
        end = t->end;
        if (ptr >= end)
                return PIE_NULL;
        to = ptr;
        while (to < end) {
                if (*to++ == '\n')
                        break;
        }
        if (to - ptr >= max)
                PIE_RAISE_INFO(PieFile_failed, ("Buffer overflow"));
        memcpy(buffer, ptr, to - ptr);
        buffer[to - ptr] = '\0';
        t->ptr = to;

        return buffer;
}

static long 
PieFile_memread(struct PieFile *it, char *buffer, long unit, long num)
{
        struct PieFileMem *t;
        char *to, *ptr, *end;

        PIE_ASSERT(it && it->read == PieFile_memread);
        PIE_ASSERT(buffer && unit > 0 && num > 0);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        ptr = t->ptr;
        end = t->end;
        if (ptr >= end)
                return 0;
        to = ptr + unit * num;
        if (to > end)
                to = end;
        memcpy(buffer, ptr, to - ptr);
        t->ptr = to;

        return num;
}

static const char *PieFile_memwrites(struct PieFile *it, const char *buffer)
{
        struct PieFileMem *t;
        char *to;

        PIE_ASSERT(buffer);
        PIE_ASSERT(it && it->writes == PieFile_memwrites);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        if (t->ro)
                PIE_RAISE_INFO(PieFile_failed, ("Write to const block"));
        to = t->ptr + strlen(buffer);
        if (to > t->end)
                PIE_RAISE_INFO(PieFile_failed, ("Target block overflow"));
        memcpy(t->ptr, buffer, to - t->ptr);
        t->ptr = to;

        return buffer;
}

static long 
PieFile_memwrite(struct PieFile *it, const char *buffer, long unit, long num)
{
        struct PieFileMem *t;
        char *to, *ptr, *end;

        PIE_ASSERT(it && it->read == PieFile_memread);
        PIE_ASSERT(buffer && unit >= 0 && num >= 0);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        if (t->ro)
                PIE_RAISE_INFO(PieFile_failed, ("Write to const block"));
        ptr = t->ptr;
        end = t->end;
        to = ptr + unit * num;
        if (to > end)
                PIE_RAISE_INFO(PieFile_failed, ("Target block overflow"));
        memcpy(ptr, buffer, to - ptr);
        t->ptr = to;

        return num;
}

static int PieFile_memfmtput(int code, struct PieFile *it)
{
        struct PieFileMem *t;

        PIE_ASSERT(it && it->fmtput == PieFile_memfmtput);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        if (t->ro)
                PIE_RAISE_INFO(PieFile_failed, ("Write to const block"));
        if (t->ptr >= t->end)
                PIE_RAISE_INFO(PieFile_failed, ("Target block overflow"));
        *((t->ptr)++) = (char)code;
        return code;
}

static long PieFile_memseek(struct PieFile *it, long offset, int origin)
{
        struct PieFileMem *t;
        char *pos = PIE_NULL;

        PIE_ASSERT(it && it->fmtput == PieFile_memfmtput);
        t = (struct PieFileMem *)(it->inst);
        PIE_ASSERT_EXTREME(t->ptr && t->base && t->end);

        switch(origin) {
        case PIE_FILE_SET:
                pos = t->base + offset;
                break;
        case PIE_FILE_CUR:
                pos = t->ptr + offset;
                break;
        case PIE_FILE_END:
                pos = t->end + offset;
                break;
        default:
                PIE_RAISE_INFO(PieFile_failed, ("Unknown origin value"));
        }

        if (pos > t->end)
                pos = t->end;
        else if (pos < t->base)
                pos = t->base;
        t->ptr = pos;
        return (long)(pos - t->base);
}

struct PieFile *PieFile_mem(void *ptr, long size)
{
        struct PieFileMem *t;
        struct PieFile *it;

        PIE_ASSERT(ptr && size > 0);
        t = PIE_NEW(struct PieFileMem);
        t->ro = PIE_FALSE;
        t->base = (char *)ptr;
        t->ptr = t->base;
        t->end = t->base + size;

        it = PIE_NEW(struct PieFile);
        it->close = PieFile_memclose;
        it->readLine = PieFile_memreadline;
        it->read = PieFile_memread;
        it->writes = PieFile_memwrites;
        it->write = PieFile_memwrite;
        it->fmtput = PieFile_memfmtput;
        it->seek = PieFile_memseek;
        it->inst = t;

        return it;
}

struct PieFile *PieFile_constMem(const void *ptr, long size)
{
        struct PieFileMem *t;
        struct PieFile *i = PieFile_mem((void *)ptr, size);
        t = (struct PieFileMem *)(i->inst);
        t->ro = PIE_TRUE;
        return i;
}

void PieFile_close(struct PieFile **interface)
{
        PIE_ASSERT(interface);
        if (*interface) {
                (*interface)->close(*interface);
                PIE_FREE(*interface);
        }
}

struct PieFile *PieFile_custom(void *inst, void (*close)(struct PieFile *it))
{
        struct PieFile *it;
        PIE_ASSERT(close);

        it = PIE_NEW(struct PieFile);
        it->close = close;
        it->inst = inst;

        return it;
}

