/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_STDLIB_H_
#define WINDOWS_STDLIB_H_

//#include_next <stdlib.h>
#include <../ucrt/stdlib.h>

char *realpath_ex(const char *path, char *resolved, int exists);
static inline
char *realpath(const char *path, char *resolved)
{
    return realpath_ex(path, resolved, 1);
}

#endif /* WINDOWS_STDLIB_H_ */
