/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_SYS_TIME_H_
#define WINDOWS_SYS_TIME_H_

#include "libfuse_windows_internal.h"

int gettimeofday(struct timeval *tv, void *tz);

#endif /* WINDOWS_SYS_TIME_H_ */
