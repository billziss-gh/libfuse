/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef FUSE_TYPES_H_
#define FUSE_TYPES_H_

#if !defined(_WIN32)

#include <fcntl.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/uio.h>

typedef uint64_t fuse_ino_t;

#define fuse_uid_t                      uid_t
#define fuse_gid_t                      gid_t
#define fuse_pid_t                      pid_t

#define fuse_dev_t                      dev_t
#define fuse_mode_t                     mode_t
#define fuse_nlink_t                    nlink_t
#define fuse_off_t                      off_t

#define fuse_fsblkcnt_t                 fsblkcnt_t
#define fuse_fsfilcnt_t                 fsfilcnt_t
#define fuse_blksize_t                  blksize_t
#define fuse_blkcnt_t                   blkcnt_t

#define fuse_timespec                   timespec

#define fuse_stat                       stat

#define fuse_statvfs                    statvfs

#define fuse_flock                      flock

#define fuse_iovec                      iovec

#else

#include <fcntl.h>
#include <stdint.h>

#define O_RDONLY                        _O_RDONLY
#define O_WRONLY                        _O_WRONLY
#define O_RDWR                          _O_RDWR
#define O_APPEND                        _O_APPEND
#define O_CREAT                         _O_CREAT
#define O_EXCL                          _O_EXCL
#define O_TRUNC                         _O_TRUNC

#define S_IFMT                          0170000
#define S_IFDIR                         0040000
#define S_IFCHR                         0020000
#define S_IFBLK                         0060000
#define S_IFREG                         0100000
#define S_IFLNK                         0120000
#define S_IFSOCK                        0140000
#define S_IFIFO                         0010000
#define S_ISUID                         0004000
#define S_ISGID                         0002000
#define S_ISVTX                         0001000

#define	S_ISDIR(m)                      (((m) & S_IFMT) == S_IFDIR)
#define	S_ISCHR(m)                      (((m) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(m)                      (((m) & S_IFMT) == S_IFBLK)
#define	S_ISREG(m)                      (((m) & S_IFMT) == S_IFREG)
#define	S_ISLNK(m)                      (((m) & S_IFMT) == S_IFLNK)
#define	S_ISSOCK(m)                     (((m) & S_IFMT) == S_IFSOCK)
#define	S_ISFIFO(m)                     (((m) & S_IFMT) == S_IFIFO)

#define F_GETLK                         7
#define F_SETLK                         8
#define F_SETLKW                        9
#define F_RDLCK                         1
#define F_WRLCK                         2
#define F_UNLCK                         3

#define st_atime                        st_atim.tv_sec
#define st_mtime                        st_mtim.tv_sec
#define st_ctime                        st_ctim.tv_sec
#define st_birthtime                    st_birthtim.tv_sec

typedef intptr_t ssize_t;

typedef uint64_t fuse_ino_t;

typedef uint32_t fuse_uid_t;
typedef uint32_t fuse_gid_t;
typedef int32_t fuse_pid_t;

typedef uint32_t fuse_dev_t;
typedef uint32_t fuse_mode_t;
typedef uint16_t fuse_nlink_t;
typedef int64_t fuse_off_t;

#if defined(_WIN64)
typedef uint64_t fuse_fsblkcnt_t;
typedef uint64_t fuse_fsfilcnt_t;
#else
typedef uint32_t fuse_fsblkcnt_t;
typedef uint32_t fuse_fsfilcnt_t;
#endif
typedef int32_t fuse_blksize_t;
typedef int64_t fuse_blkcnt_t;

#if defined(_WIN64)
struct fuse_timespec
{
    int64_t tv_sec;
    int64_t tv_nsec;
};
#else
struct fuse_timespec
{
    int32_t tv_sec;
    int32_t tv_nsec;
};
#endif

struct fuse_stat
{
    fuse_dev_t st_dev;
    fuse_ino_t st_ino;
    fuse_mode_t st_mode;
    fuse_nlink_t st_nlink;
    fuse_uid_t st_uid;
    fuse_gid_t st_gid;
    fuse_dev_t st_rdev;
    fuse_off_t st_size;
    struct fuse_timespec st_atim;
    struct fuse_timespec st_mtim;
    struct fuse_timespec st_ctim;
    fuse_blksize_t st_blksize;
    fuse_blkcnt_t st_blocks;
    struct fuse_timespec st_birthtim;
};

#if defined(_WIN64)
struct fuse_statvfs
{
    uint64_t f_bsize;
    uint64_t f_frsize;
    fuse_fsblkcnt_t f_blocks;
    fuse_fsblkcnt_t f_bfree;
    fuse_fsblkcnt_t f_bavail;
    fuse_fsfilcnt_t f_files;
    fuse_fsfilcnt_t f_ffree;
    fuse_fsfilcnt_t f_favail;
    uint64_t f_fsid;
    uint64_t f_flag;
    uint64_t f_namemax;
};
#else
struct fuse_statvfs
{
    uint32_t f_bsize;
    uint32_t f_frsize;
    fuse_fsblkcnt_t f_blocks;
    fuse_fsblkcnt_t f_bfree;
    fuse_fsblkcnt_t f_bavail;
    fuse_fsfilcnt_t f_files;
    fuse_fsfilcnt_t f_ffree;
    fuse_fsfilcnt_t f_favail;
    uint32_t f_fsid;
    uint32_t f_flag;
    uint32_t f_namemax;
};
#endif

struct fuse_flock
{
    int16_t l_type;
    int16_t l_whence;
    fuse_off_t l_start;
    fuse_off_t l_len;
    fuse_pid_t l_pid;
};

struct fuse_iovec
{
    void *iov_base;
    size_t iov_len;
};

#endif

#endif /* FUSE_TYPES_H_ */
