/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef LIBFUSE_WINDOWS_INTERNAL_H_
#define LIBFUSE_WINDOWS_INTERNAL_H_

#include <windows.h>
#include <stdint.h>

#if !defined(FUSE_TYPES_H_)

typedef intptr_t ssize_t;

typedef int64_t fuse_off_t;

#if defined(_WIN64)
struct fuse_timespec
{
    int64_t tv_sec;
    int64_t tv_nsec;
};
#elif defined(_WIN32)
struct fuse_timespec
{
    int32_t tv_sec;
    int32_t tv_nsec;
};
#else
#error unknown windows platform
#endif

struct fuse_iovec
{
    void *iov_base;
    size_t iov_len;
};

#endif

DWORD PthreadCancelableWaitForSingleObject(HANDLE Handle, DWORD Timeout);
VOID PthreadCancelableIo(BOOL Pending);
#define PthreadCancelableIoEnter()      PthreadCancelableIo(TRUE)
#define PthreadCancelableIoLeave()      PthreadCancelableIo(FALSE)

#define FuseFdIsHandle(H)               (1 == ((intptr_t)(H) & 3))
#define FuseHandleToFd(H)               ((int)(((intptr_t)(H) << 2) | 0x00010001))
#define FuseFdToHandle(H)               ((HANDLE)(((intptr_t)(H) & 0xfffc) >> 2))
BOOL FuseReadFile(HANDLE Handle, void *Buffer, DWORD Length, PDWORD PBytesTransferred);
BOOL FuseWriteFile(HANDLE Handle, const void *Buffer, DWORD Length, PDWORD PBytesTransferred);

#endif /* LIBFUSE_WINDOWS_INTERNAL_H_ */
