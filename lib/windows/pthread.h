/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_PTHREAD_H_
#define WINDOWS_PTHREAD_H_

#include "libfuse_windows_internal.h"

/*
 * pthread_mutex_t
 */

#define PTHREAD_MUTEX_INITIALIZER       SRWLOCK_INIT

typedef SRWLOCK pthread_mutex_t;
typedef DWORD pthread_mutexattr_t;

static inline
int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
    *attr = 0;
    return 0;
}

static inline
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
    return 0;
}

static inline
int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type)
{
    *type = (size_t)*attr;
    return 0;
}

static inline
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
    *attr = (DWORD)type;
    return 0;
}

static inline
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    InitializeSRWLock(mutex);
    return 0;
}

static inline
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    return 0;
}

static inline
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    AcquireSRWLockExclusive(mutex);
    return 0;
}

static inline
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    ReleaseSRWLockExclusive(mutex);
    return 0;
}

/*
 * pthread_cond_t
 */

typedef CONDITION_VARIABLE pthread_cond_t;
typedef DWORD pthread_condattr_t;

static inline
int pthread_condattr_init(pthread_condattr_t *attr)
{
    *attr = 0;
    return 0;
}

static inline
int pthread_condattr_destroy(pthread_condattr_t *attr)
{
    return 0;
}

static inline
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    InitializeConditionVariable(cond);
    return 0;
}

static inline
int pthread_cond_destroy(pthread_cond_t *cond)
{
    return 0;
}

static inline
int pthread_cond_signal(pthread_cond_t *cond)
{
    WakeConditionVariable(cond);
    return 0;
}

static inline
int pthread_cond_broadcast(pthread_cond_t *cond)
{
    WakeAllConditionVariable(cond);
    return 0;
}

/* NOTE: pthread_cond_wait is not a cancellation point as required by POSIX */
static inline
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!SleepConditionVariableSRW(cond, mutex, INFINITE, 0))
        return EINVAL;
    return 0;
}

/* NOTE: pthread_cond_timedwait is not a cancellation point as required by POSIX */
static inline
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
    const struct fuse_timespec *ts)
{
    INT64 CurrentTime, AbsTime;
    DWORD Timeout;

    GetSystemTimeAsFileTime((PFILETIME)&CurrentTime);
    AbsTime = (INT64)ts->tv_sec * 10000000 + (INT64)ts->tv_nsec / 100 + 116444736000000000LL;

    if (CurrentTime >= AbsTime)
        return ETIMEDOUT;

    Timeout = (DWORD)(AbsTime / 1000000);

    if (!SleepConditionVariableSRW(cond, mutex, Timeout, 0))
        switch (GetLastError())
        {
        case ERROR_TIMEOUT:
            return ETIMEDOUT;
        default:
            return EINVAL;
        }

    return 0;
}

/*
 * pthread_key_t
 */

typedef DWORD pthread_key_t;

int pthread_key_create(pthread_key_t *key, void (*dtor)(void *));
int pthread_key_delete(pthread_key_t key);

static inline
void *pthread_getspecific(pthread_key_t key)
{
    return TlsGetValue(key);
}

static inline
int pthread_setspecific(pthread_key_t key, const void *value)
{
    TlsSetValue(key, (void *)value);
    return 0;
}

/*
 * pthread_t
 */

#define PTHREAD_CANCEL_ENABLE           0
#define PTHREAD_CANCEL_DISABLE          1
#define PTHREAD_CANCELED                ((void *)-1)

typedef void *pthread_t;
typedef DWORD pthread_attr_t;

static inline
int pthread_attr_init(pthread_attr_t *attr)
{
    *attr = 0;
    return 0;
}

static inline
int pthread_attr_destroy(pthread_attr_t *attr)
{
    return 0;
}

static inline
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
    *stacksize = (size_t)*attr;
    return 0;
}

static inline
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
    *attr = (DWORD)stacksize;
    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
int pthread_detach(pthread_t thread);
int pthread_join(pthread_t thread, void **retval);
int pthread_cancel(pthread_t thread);
void pthread_testcancel(void);
int pthread_setcancelstate(int state, int *oldstate);
void pthread_exit(void *retval);
pthread_t pthread_self(void);

#endif /* WINDOWS_PTHREAD_H_ */
