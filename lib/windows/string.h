/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_STRING_H_
#define WINDOWS_STRING_H_

//#include_next <string.h>
#include <../ucrt/string.h>

#define strtok_r(s, d, p)               strtok_s(s, d, p)

#endif /* WINDOWS_STRING_H_ */
