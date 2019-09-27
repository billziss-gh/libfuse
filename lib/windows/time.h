/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_TIME_H_
#define WINDOWS_TIME_H_

//#include_next <time.h>
#include <../ucrt/time.h>

#define CLOCK_REALTIME                  ((clockid_t)1)
#define CLOCK_MONOTONIC                 ((clockid_t)4)

typedef uintptr_t clockid_t;

int clock_gettime(clockid_t clockid, struct fuse_timespec *ts);

#endif /* WINDOWS_TIME_H_ */
