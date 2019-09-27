/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_DLFCN_H_
#define WINDOWS_DLFCN_H_

#include "libfuse_windows_internal.h"

#define RTLD_NOW                        2

void *dlopen(const char *path, int flag);
int dlclose(void *handle);
void *dlsym(void *handle, const char *name);
char *dlerror(void);

#endif /* WINDOWS_DLFCN_H_ */
