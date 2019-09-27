/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_UNISTD_H_
#define WINDOWS_UNISTD_H_

#include "libfuse_windows_internal.h"

#define FUSE_PAGE_SIZE                  4096

static inline
int getpagesize(void)
{
    return FUSE_PAGE_SIZE;
}

static inline
unsigned sleep(unsigned seconds)
{
    PthreadCancelableWaitForSingleObject(GetCurrentThread(), seconds * 1000);
    return 0;
}

int close(int fd);
ssize_t pread(int fd, void *buf, size_t nbyte, fuse_off_t offset);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t pwrite(int fd, const void *buf, size_t nbyte, fuse_off_t offset);
ssize_t write(int fd, const void *buf, size_t nbyte);
ssize_t writev(int fd, const struct fuse_iovec *iov, int iovcnt);

#endif /* WINDOWS_UNISTD_H_ */
