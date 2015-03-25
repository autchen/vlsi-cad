/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_INI_H_INCLUDED
#define PIE_INI_H_INCLUDED

/**
 * @package plus
 * @file pie_ini.h
 * @brief Ini file parser
 */

#include "pie_config.h"

/* Entity typedef */

typedef struct PieIni PieIni;

struct PieList;
struct PieFile;
struct PieText;

/* Exported exceptions */

extern const struct PieExcept PieFile_failed;
extern const struct PieExcept PieMem_failed;
extern const struct PieExcept PieIni_failed;

/* Public methods */

extern struct PieIni *PieIni_new(const char *path);

extern struct PieIni *PieIni_new2(struct PieFile *file);

extern void PieIni_del(struct PieIni **self);

extern PieBool PieIni_save(struct PieIni *self, const char *path);

extern PieBool PieIni_save2(struct PieIni *self, struct PieFile *file);

extern struct PieText 
PieIni_getText(struct PieIni *self, PieAtom section, PieAtom key);

extern char *
PieIni_getStr(struct PieIni *self, PieAtom section, PieAtom key, 
              char *buffer, long bufLen);

extern PieBool 
PieIni_getBool(struct PieIni *self, PieAtom section, PieAtom key, 
               PieBool defaultVal);

extern long 
PieIni_getInt(struct PieIni *self, PieAtom section, PieAtom key, 
              long defaultVal);

extern long 
PieIni_getIntVec(struct PieIni *self, PieAtom section, PieAtom key, 
                 struct PieList *vec);

extern double 
PieIni_getFloat(struct PieIni *self, PieAtom section, PieAtom key, 
                double defaultVal);

extern long 
PieIni_getFloatVec(struct PieIni *self, PieAtom section, PieAtom key, 
                   struct PieList *vec);

extern char *
PieIni_setStr(struct PieIni *self, PieAtom section, PieAtom key, 
              char *buffer, long bufLen);

extern void 
PieIni_setBool(struct PieIni *self, PieAtom section, PieAtom key, 
               PieBool val);

extern void 
PieIni_setInt(struct PieIni *self, PieAtom section, PieAtom key, long val);

extern void 
PieIni_setIntVec(struct PieIni *self, PieAtom section, PieAtom key, 
                 struct PieList *vec);

extern void 
PieIni_setFloat(struct PieIni *self, PieAtom section, PieAtom key, 
                double val);

extern void 
PieIni_setFloatVec(struct PieIni *self, PieAtom section, PieAtom key, 
                   struct PieList *vec);

extern void PieIni_delSection(struct PieIni *self, PieAtom section);

extern void 
PieIni_delProfile(struct PieIni *self, PieAtom section, PieAtom key);

#endif


