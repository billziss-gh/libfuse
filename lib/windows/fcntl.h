/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_FCNTL_H_
#define WINDOWS_FCNTL_H_

//#include_next <fcntl.h>
#include <../ucrt/fcntl.h>

int open(const char *path, int oflag, ...);

#endif /* WINDOWS_FCNTL_H_ */
