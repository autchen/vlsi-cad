/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_FILE_H_INCLUDED
#define PIE_FILE_H_INCLUDED

/**
 * @package plus
 * @file pie_file.h
 * @brief Abstract file interface
 */

#include "pie_config.h"
#include <stdio.h>

/* Entity typedef */

typedef struct PieFile {
        void *inst;
        char *(*readLine)(struct PieFile *it, char *buffer, long max);
        long (*read)(struct PieFile *it, char *buffer, long unit, long num);
        const char *(*writes)(struct PieFile *it, const char *buffer);
        long (*write)(struct PieFile *it, const char *buffer, long unit, 
                      long num);
        int (*fmtput)(int code, struct PieFile *it);
        long (*seek)(struct PieFile *it, long offset, int origin);

        /* do not call directly */
        void (*close)(struct PieFile *it);
} PieFile;

#define PIE_FILE_SET SEEK_SET
#define PIE_FILE_CUR SEEK_CUR
#define PIE_FILE_END SEEK_END 

/* Exported exceptions */

extern const struct PieExcept PieMem_failed;
extern const struct PieExcept PieFile_failed;

/* Public methods */

extern struct PieFile *PieFile_open(const char *name, const char *mode);

extern struct PieFile *PieFile_stream(FILE *stream, PieBool close);

extern struct PieFile *PieFile_mem(void *ptr, long size);

extern struct PieFile *PieFile_constMem(const void *ptr, long size);

extern void PieFile_close(struct PieFile **it);

extern struct PieFile *
PieFile_custom(void *inst, void (*close)(struct PieFile *it));

#endif


