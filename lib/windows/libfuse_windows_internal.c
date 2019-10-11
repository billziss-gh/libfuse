/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#include "libfuse_windows_internal.h"
#include "dlfcn.h"
#include "fcntl.h"
#include "pthread.h"
#include "stdlib.h"
#include "sys/time.h"
#include "time.h"
#include "unistd.h"

/*
 * errors
 */

static int maperror(int winerrno)
{
    switch (winerrno)
    {
    case ERROR_INVALID_FUNCTION:
        return EINVAL;
    case ERROR_FILE_NOT_FOUND:
        return ENOENT;
    case ERROR_PATH_NOT_FOUND:
        return ENOENT;
    case ERROR_TOO_MANY_OPEN_FILES:
        return EMFILE;
    case ERROR_ACCESS_DENIED:
        return EACCES;
    case ERROR_INVALID_HANDLE:
        return EBADF;
    case ERROR_ARENA_TRASHED:
        return ENOMEM;
    case ERROR_NOT_ENOUGH_MEMORY:
        return ENOMEM;
    case ERROR_INVALID_BLOCK:
        return ENOMEM;
    case ERROR_BAD_ENVIRONMENT:
        return E2BIG;
    case ERROR_BAD_FORMAT:
        return ENOEXEC;
    case ERROR_INVALID_ACCESS:
        return EINVAL;
    case ERROR_INVALID_DATA:
        return EINVAL;
    case ERROR_INVALID_DRIVE:
        return ENOENT;
    case ERROR_CURRENT_DIRECTORY:
        return EACCES;
    case ERROR_NOT_SAME_DEVICE:
        return EXDEV;
    case ERROR_NO_MORE_FILES:
        return ENOENT;
    case ERROR_LOCK_VIOLATION:
        return EACCES;
    case ERROR_BAD_NETPATH:
        return ENOENT;
    case ERROR_NETWORK_ACCESS_DENIED:
        return EACCES;
    case ERROR_BAD_NET_NAME:
        return ENOENT;
    case ERROR_FILE_EXISTS:
        return EEXIST;
    case ERROR_CANNOT_MAKE:
        return EACCES;
    case ERROR_FAIL_I24:
        return EACCES;
    case ERROR_INVALID_PARAMETER:
        return EINVAL;
    case ERROR_NO_PROC_SLOTS:
        return EAGAIN;
    case ERROR_DRIVE_LOCKED:
        return EACCES;
    case ERROR_BROKEN_PIPE:
        return EPIPE;
    case ERROR_DISK_FULL:
        return ENOSPC;
    case ERROR_INVALID_TARGET_HANDLE:
        return EBADF;
    case ERROR_WAIT_NO_CHILDREN:
        return ECHILD;
    case ERROR_CHILD_NOT_COMPLETE:
        return ECHILD;
    case ERROR_DIRECT_ACCESS_HANDLE:
        return EBADF;
    case ERROR_NEGATIVE_SEEK:
        return EINVAL;
    case ERROR_SEEK_ON_DEVICE:
        return EACCES;
    case ERROR_DIR_NOT_EMPTY:
        return ENOTEMPTY;
    case ERROR_NOT_LOCKED:
        return EACCES;
    case ERROR_BAD_PATHNAME:
        return ENOENT;
    case ERROR_MAX_THRDS_REACHED:
        return EAGAIN;
    case ERROR_LOCK_FAILED:
        return EACCES;
    case ERROR_ALREADY_EXISTS:
        return EEXIST;
    case ERROR_FILENAME_EXCED_RANGE:
        return ENOENT;
    case ERROR_NESTING_NOT_ALLOWED:
        return EAGAIN;
    case ERROR_NOT_ENOUGH_QUOTA:
        return ENOMEM;
    default:
        if (ERROR_WRITE_PROTECT <= winerrno && winerrno <= ERROR_SHARING_BUFFER_EXCEEDED)
            return EACCES;
        else if (ERROR_INVALID_STARTING_CODESEG <= winerrno && winerrno <= ERROR_INFLOOP_IN_RELOC_CHAIN)
            return ENOEXEC;
        else
            return EINVAL;
    }
}

static inline void *error0(void)
{
    errno = maperror(GetLastError());
    return 0;
}

static inline int error(void)
{
    errno = maperror(GetLastError());
    return -1;
}

/*
 * fcntl.h/unistd.h
 */

int open(const char *path, int oflag, ...)
{
    if ('/' == path[0] &&
        'd' == path[1] && 'e' == path[2] && 'v' == path[3] &&
        ('/' == path[4] || '\0' == path[4]))
    {
        errno = ENOENT;
        return -1;
    }

    static DWORD da[] = { GENERIC_READ, GENERIC_WRITE, GENERIC_READ | GENERIC_WRITE, 0 };
    static DWORD cd[] = { OPEN_EXISTING, OPEN_ALWAYS, TRUNCATE_EXISTING, CREATE_ALWAYS };
    DWORD DesiredAccess = 0 == (oflag & _O_APPEND) ?
        da[oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR)] :
        (da[oflag & (_O_RDONLY | _O_WRONLY | _O_RDWR)] & ~FILE_WRITE_DATA) | FILE_APPEND_DATA;
    DWORD CreationDisposition = (_O_CREAT | _O_EXCL) == (oflag & (_O_CREAT | _O_EXCL)) ?
        CREATE_NEW :
        cd[(oflag & (_O_CREAT | _O_TRUNC)) >> 8];

    PthreadCancelableIoEnter();

    HANDLE h = CreateFileA(path,
        DesiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0/* default security */,
        CreationDisposition, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, 0);

    PthreadCancelableIoLeave();

    if (INVALID_HANDLE_VALUE == h)
        return error();

    return (int)(intptr_t)h;
}

int close(int fd)
{
    HANDLE Handle = (HANDLE)(intptr_t)fd;
    BOOL Success;

    /*
     * POSIX requires close to be a cancelation point. But CancelSynchronousIo cannot
     * (I believe) cancel a CloseHandle. We frame it as cancelable I/O for symmetry,
     * and with the understanding that only the equivalent of pthread_cancel will be
     * performed.
     *
     * Note that CloseHandle should not block under normal circumstances, so this should
     * not be an issue.
     */
    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        Success = FALSE;
    }
    else
        Success = CloseHandle(Handle);
    
    PthreadCancelableIoLeave();

    return Success ? 0 : error();
}

ssize_t pread(int fd, void *buf, size_t nbyte, fuse_off_t offset)
{
    HANDLE Handle = (HANDLE)(intptr_t)fd;
    OVERLAPPED Overlapped = { 0 };
    DWORD BytesTransferred = 0;
    BOOL Success;

    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
        Success = FuseReadFile(Handle, buf, (DWORD)nbyte, &BytesTransferred);
    else
    {
        Overlapped.Offset = (DWORD)offset;
        Overlapped.OffsetHigh = (DWORD)(offset >> 32);
        Success = ReadFile(Handle, buf, (DWORD)nbyte, &BytesTransferred, &Overlapped);
    }

    PthreadCancelableIoLeave();

    return Success || ERROR_HANDLE_EOF == GetLastError() ? BytesTransferred : error();
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
    HANDLE Handle = (HANDLE)(intptr_t)fd;
    DWORD BytesTransferred = 0;
    BOOL Success;

    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
        Success = FuseReadFile(Handle, buf, (DWORD)nbyte, &BytesTransferred);
    else
        Success = ReadFile(Handle, buf, (DWORD)nbyte, &BytesTransferred, 0);

    PthreadCancelableIoLeave();

    return Success || ERROR_HANDLE_EOF == GetLastError() ? BytesTransferred : error();
}

ssize_t pwrite(int fd, const void *buf, size_t nbyte, fuse_off_t offset)
{
    HANDLE Handle = (HANDLE)(intptr_t)fd;
    OVERLAPPED Overlapped = { 0 };
    DWORD BytesTransferred = 0;
    BOOL Success;

    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
        Success = FuseWriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred);
    else
    {
        Overlapped.Offset = (DWORD)offset;
        Overlapped.OffsetHigh = (DWORD)(offset >> 32);
        Success = WriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred, &Overlapped);
    }

    PthreadCancelableIoLeave();

    return Success ? BytesTransferred : error();
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    HANDLE Handle = (HANDLE)(intptr_t)fd;
    DWORD BytesTransferred = 0;
    BOOL Success;

    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
        Success = FuseWriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred);
    else
        Success = WriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred, 0);

    PthreadCancelableIoLeave();

    return Success ? BytesTransferred : error();
}

ssize_t writev(int fd, const struct fuse_iovec *iov, int iovcnt)
{
    if (1 == iovcnt)
        return write(fd, iov[0].iov_base, iov[0].iov_len);

    HANDLE Handle = (HANDLE)(intptr_t)fd;
    __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) uint8_t stackbuf[64 * 1024];
    uint8_t *buf = stackbuf;
    size_t nbyte;
    DWORD BytesTransferred = 0;
    BOOL Success;

    nbyte = 0;
    for (int i = 0; iovcnt > i; i++)
        nbyte += iov[i].iov_len;
    
    if (sizeof stackbuf < nbyte)
    {
        buf = HeapAlloc(GetProcessHeap(), 0, nbyte);
        if (0 == buf)
        {
            errno = ENOMEM;
            return -1;
        }
    }

    nbyte = 0;
    for (int i = 0; iovcnt > i; i++)
    {
        memcpy(buf + nbyte, iov[i].iov_base, iov[i].iov_len);
        nbyte += iov[i].iov_len;
    }

    PthreadCancelableIoEnter();

    if (FuseFdIsHandle(Handle))
        Success = FuseWriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred);
    else
        Success = WriteFile(Handle, buf, (DWORD)nbyte, &BytesTransferred, 0);
    
    PthreadCancelableIoLeave();

    if (stackbuf != buf)
        HeapFree(GetProcessHeap(), 0, buf);

    return Success ? BytesTransferred : error();
}

/*
 * dlfcn.h
 */

static DWORD dlfcn_lasterr;
static char dlfcn_lasterrmsg[32];

void *dlopen(const char *path, int flag)
{
    HANDLE Handle;

    Handle = LoadLibraryA(path);
    if (0 == Handle)
    {
        dlfcn_lasterr = GetLastError();
        return 0;
    }

    return Handle;
}

int dlclose(void *handle)
{
    FreeLibrary(handle);
    return 0;
}

void *dlsym(void *handle, const char *name)
{
    FARPROC Proc;

    Proc = GetProcAddress(handle, name);
    if (0 == Proc)
    {
        dlfcn_lasterr = GetLastError();
        return 0;
    }

    return Proc;
}

char *dlerror(void)
{
    if (0 == dlfcn_lasterr)
        return 0;

    wsprintfA(dlfcn_lasterrmsg, "%s: %lu", __func__, dlfcn_lasterr);
    dlfcn_lasterr = 0;

    return dlfcn_lasterrmsg;
}

/*
 * pthread.h
 */

/* pthread_key_t */

struct pthread_key_rec
{
    pthread_key_t key;
    void (*dtor)(void *);
};

static struct pthread_key_rec pthread_key_recs[4];
static SRWLOCK pthread_key_lock = SRWLOCK_INIT;

static void NTAPI pthread_key_run_destructors(void)
{
    AcquireSRWLockExclusive(&pthread_key_lock);

    size_t i, n = sizeof pthread_key_recs / sizeof pthread_key_recs[0];
    for (i = 0; n > i; i++)
        if (0 != pthread_key_recs[i].dtor)
        {
            void *p = TlsGetValue(pthread_key_recs[i].key);
            if (0 != p)
                pthread_key_recs[i].dtor(p);
        }

    ReleaseSRWLockExclusive(&pthread_key_lock);
}

int pthread_key_create(pthread_key_t *keyp, void (*dtor)(void *))
{
    *keyp = 0;

    pthread_key_t key = TlsAlloc();
    if (TLS_OUT_OF_INDEXES == key)
        return ENOMEM;

    if (0 != dtor)
    {
        AcquireSRWLockExclusive(&pthread_key_lock);

        size_t i, n = sizeof pthread_key_recs / sizeof pthread_key_recs[0];
        for (i = 0; n > i; i++)
            if (0 == pthread_key_recs[i].dtor)
            {
                pthread_key_recs[i].key = key;
                pthread_key_recs[i].dtor = dtor;
            }

        ReleaseSRWLockExclusive(&pthread_key_lock);

        if (n <= i)
        {
            TlsFree(key);
            return ENOMEM;
        }
    }

    *keyp = key;
    return 0;
}

int pthread_key_delete(pthread_key_t key)
{
    AcquireSRWLockExclusive(&pthread_key_lock);

    size_t i, n = sizeof pthread_key_recs / sizeof pthread_key_recs[0];
    for (i = 0; n > i; i++)
        if (key == pthread_key_recs[i].key)
        {
            pthread_key_recs[i].key = 0;
            pthread_key_recs[i].dtor = 0;
        }

    ReleaseSRWLockExclusive(&pthread_key_lock);

    TlsFree(key);
    return 0;
}

/* pthread_t */

#if defined(PTHREAD_NO_CRT)
#define PthreadCreate(A, S, E, P, F, I) CreateThread(A, S, E, P, F, I)
#define PthreadExit(R)                  ExitThread(R)
#else
#include <process.h>
#define PthreadCreate(A, S, E, P, F, I) ((HANDLE)_beginthreadex(A, S, E, P, F, I))
#define PthreadExit(R)                  _endthreadex(R)
#endif

struct pthread_rec
{
    void *(*Start)(void *);
    void *Arg;
    HANDLE Thread;
    void *Retval;
    SRWLOCK CancelLock;
    HANDLE CancelEvent;
    BOOL Canceled;
    BOOL CancelableIoPending;
    int CancelState;
    LONG RefCount;
};

static pthread_key_t pthread_rec_key;
static INIT_ONCE pthread_rec_key_once;
static void pthread_rec_deref(void *p);

static BOOL WINAPI pthread_rec_key_init(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
    return 0 == pthread_key_create(&pthread_rec_key, pthread_rec_deref);
}

static struct pthread_rec *pthread_rec_create(void *(*start)(void *), void *arg)
{
    if (!InitOnceExecuteOnce(&pthread_rec_key_once, pthread_rec_key_init, 0, 0))
        return 0;

    struct pthread_rec *rec = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *rec);
    if (0 == rec)
        return 0;

    rec->Start = start;
    rec->Arg = arg;
    InitializeSRWLock(&rec->CancelLock);
    rec->CancelEvent = CreateEventW(0, TRUE, FALSE, 0);
    rec->CancelState = PTHREAD_CANCEL_ENABLE;
    rec->RefCount = 1;

    if (0 == rec->CancelEvent)
    {
        HeapFree(GetProcessHeap(), 0, rec);
        return 0;
    }

    return rec;
}

static void pthread_rec_deref(void *p)
{
    struct pthread_rec *rec = p;

    if (0 == InterlockedDecrement(&rec->RefCount))
    {
        if (0 != rec->Thread)
            CloseHandle(rec->Thread);
        if (0 != rec->CancelEvent)
            CloseHandle(rec->CancelEvent);
        HeapFree(GetProcessHeap(), 0, rec);
    }
}

static unsigned __stdcall pthread_entry(void *p)
{
    struct pthread_rec *rec = p;

    InterlockedIncrement(&rec->RefCount);
    pthread_setspecific(pthread_rec_key, rec);

    rec->Retval = rec->Start(rec->Arg);

    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg)
{
    *thread = 0;

    struct pthread_rec *rec = pthread_rec_create(start, arg);
    if (0 == rec)
        return ENOMEM;

    rec->Thread = PthreadCreate(0, 0 != attr ? *attr : 0, pthread_entry, rec, CREATE_SUSPENDED, 0);
    if (0 == rec->Thread)
    {
        pthread_rec_deref(rec);
        return EAGAIN;
    }
    ResumeThread(rec->Thread);

    *thread = rec;

    return 0;
}

int pthread_detach(pthread_t thread)
{
    struct pthread_rec *rec = thread;

    pthread_rec_deref(rec);

    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    struct pthread_rec *rec = thread;

    PthreadCancelableWaitForSingleObject(rec->Thread, INFINITE);
    *retval = rec->Retval;
    pthread_rec_deref(rec);

    return 0;
}

int pthread_cancel(pthread_t thread)
{
    struct pthread_rec *rec = thread;

    /*
     * 2-PHASE CANCELATION: PHASE 1
     *
     * Set the Canceled variable to TRUE and signal the CancelEvent. This allows the use of
     * pthread_cancel and PthreadCancelableWaitForSingleObject.
     *
     * SetEvent below acts as memory barrier. But note that thread may also be interrupted
     * after Canceled = TRUE, but before SetEvent. For this reason we must WaitForSingleObject
     * (which also acts as memory barriter) with an INFINITE timeout after checking Canceled.
     */
    rec->Canceled = TRUE;
    SetEvent(rec->CancelEvent);

    /*
     * 2-PHASE CANCELATION: PHASE 2
     *
     * Use CancelSynchronousIo in a retry loop. This allows the cancelation of any synchronous
     * I/O that has marked itself as cancelable, by framing itself with PthreadCancelableIoEnter
     * and PthreadCancelableIoLeave.
     *
     * The reason that CancelSynchronousIo has to be retried is because it may arrive just before
     * (or just after) the synchronous I/O call. Hence we retry if we see ERROR_NOT_FOUND.
     */
    for (int retry = 1;; retry++)
    {
        AcquireSRWLockExclusive(&rec->CancelLock);
        BOOL Done =
            !rec->CancelableIoPending ||
            CancelSynchronousIo(rec->Thread) ||
            ERROR_NOT_FOUND != GetLastError();
        ReleaseSRWLockExclusive(&rec->CancelLock);
        if (Done || 3 == retry)
            break;
        SwitchToThread();
    }

    return 0;
}

void pthread_testcancel(void)
{
    struct pthread_rec *rec = pthread_self();

    if (PTHREAD_CANCEL_ENABLE != rec->CancelState)
        return;

    if (rec->Canceled && WAIT_OBJECT_0 == WaitForSingleObject(rec->CancelEvent, INFINITE))
        pthread_exit(PTHREAD_CANCELED);
}

int pthread_setcancelstate(int state, int *oldstate)
{
    struct pthread_rec *rec = pthread_self();

    if (0 != oldstate)
        *oldstate = rec->CancelState;
    rec->CancelState = state;

    return 0;
}

void pthread_exit(void *retval)
{
    struct pthread_rec *rec = pthread_getspecific(pthread_rec_key);

    if (0 != rec)
        rec->Retval = retval;

    PthreadExit(0);
}

pthread_t pthread_self(void)
{
    struct pthread_rec *rec = pthread_getspecific(pthread_rec_key);

    if (0 == rec)
    {
        rec = pthread_rec_create(0, 0);
        if (0 == rec)
            goto fail;

        if (!DuplicateHandle(
            GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &rec->Thread,
            0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            pthread_rec_deref(rec);
            goto fail;
        }
    }

    return rec;

fail:
    /* POSIX says this cannot fail! Maybe we should raise or abort. */
    return 0;
}

DWORD PthreadCancelableWaitForSingleObject(HANDLE Handle, DWORD Timeout)
{
    struct pthread_rec *rec = pthread_self();
    HANDLE WaitObjects[2];
    DWORD WaitResult;

    if (PTHREAD_CANCEL_ENABLE == rec->CancelState)
    {
        /*
         * Place CancelEvent in slot 0 in the WaitObjects array.
         * This is so that if both objects are signaled, the CancelEvent
         * takes precedence.
         */
        WaitObjects[0] = rec->CancelEvent;
        WaitObjects[1] = Handle;
        WaitResult = WaitForMultipleObjects(2, WaitObjects, FALSE, Timeout);
        switch (WaitResult)
        {
        case WAIT_OBJECT_0:
            pthread_exit(PTHREAD_CANCELED);
            /* never reached; silence compiler warning */
            //SetLastError(...);
            return WAIT_FAILED;
        case WAIT_OBJECT_0 + 1:
            return WAIT_OBJECT_0;
        default:
            return WaitResult;
        }
    }
    else
        return WaitForSingleObject(Handle, Timeout);
}

VOID PthreadCancelableIo(BOOL Pending)
{
    struct pthread_rec *rec = pthread_self();

    if (PTHREAD_CANCEL_ENABLE != rec->CancelState)
        return;

    if (rec->Canceled && WAIT_OBJECT_0 == WaitForSingleObject(rec->CancelEvent, INFINITE))
        pthread_exit(PTHREAD_CANCELED);

    AcquireSRWLockExclusive(&rec->CancelLock);
    rec->CancelableIoPending = Pending;
    ReleaseSRWLockExclusive(&rec->CancelLock);
}

/*
 * stdlib.h
 */

char *realpath_ex(const char *path, char *resolved, int exists)
{
    char *result;

    if (0 == resolved)
    {
        result = malloc(PATH_MAX); /* sets errno */
        if (0 == result)
            return 0;
    }
    else
        result = resolved;

    if (
        (('A' <= path[0] && path[0] <= 'Z') || ('a' <= path[0] && path[0] <= 'z')) &&
        (':'  == path[1]) &&
        ('\0' == path[2]))
    {
        resolved[0] = path[0];
        resolved[1] = ':';
        resolved[2] = '\0';
        return resolved;
    }

    if ((
        ('\\' == path[0] || '/' == path[0]) &&
        ('\\' == path[1] || '/' == path[1]) &&
        ('?'  == path[2] || '.' == path[2]) &&
        ('\\' == path[3] || '/' == path[3])) &&
        (('A' <= path[4] && path[4] <= 'Z') || ('a' <= path[4] && path[4] <= 'z')) &&
        (':'  == path[5]) &&
        ('\0' == path[6]))
    {
        resolved[0] = '\\';
        resolved[1] = '\\';
        resolved[2] = path[2];
        resolved[3] = '\\';
        resolved[4] = path[4];
        resolved[5] = ':';
        resolved[6] = '\0';
        return resolved;
    }

    int err = 0;
    DWORD len = GetFullPathNameA(path, PATH_MAX, result, 0);
    if (0 == len)
        err = GetLastError();
    else if (PATH_MAX < len)
        err = ERROR_INVALID_PARAMETER;

    if (exists && 0 == err)
    {
        HANDLE h = CreateFileA(result,
            FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            0,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
        if (INVALID_HANDLE_VALUE != h)
            CloseHandle(h);
        else
            err = GetLastError();
    }

    if (0 != err)
    {
        if (result != resolved)
            free(result);

        errno = maperror(err);
        result = 0;
    }

    return result;
}

/*
 * sys/time.h
 */

int gettimeofday(struct timeval *tv, void *tz)
{
    struct fuse_timespec ts;
    if (0 != clock_gettime(CLOCK_REALTIME, &ts))
        return -1;
    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
    return 0;
}

/*
 * time.h
 */

int clock_gettime(clockid_t clockid, struct fuse_timespec *ts)
{
    switch (clockid)
    {
    case CLOCK_REALTIME:
        {
            INT64 CurrentTime;
            GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
            CurrentTime -= 116444736000000000LL;
            ts->tv_sec = (__int3264)(CurrentTime / 10000000);
            ts->tv_nsec = (__int3264)(CurrentTime % 10000000 * 100);
            return 0;
        }
    case CLOCK_MONOTONIC:
    default:
        errno = EINVAL;
        return -1;
    }
}

/*
 * DllMain
 */

BOOL WINAPI DllMain(HINSTANCE Instance, DWORD Reason, PVOID Reserved)
{
    switch (Reason)
    {
    case DLL_THREAD_DETACH:
        pthread_key_run_destructors();
        break;
    }

    return TRUE;
}
