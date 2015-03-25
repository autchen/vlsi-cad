/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#ifndef PIE_ATOM_H_INCLUDED
#define PIE_ATOM_H_INCLUDED

/**
 * @package plus
 * @file pie_atom.h
 * @brief Globally unique byte sequences
 */

#include "pie_config.h"

/* pie_config.h */
/* typedef const struct PieAtom *PieAtom; */

/* Exported exception */

extern const struct PieExcept PieMem_failed;

/* Public methods */

extern PieAtom PieAtom_new(const char *str, int len);

extern PieAtom PieAtom_str(const char *str);

extern PieAtom PieAtom_int(long num);

#endif /* PIE_ATOM_H_INCLUDED */
