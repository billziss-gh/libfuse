/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_SEMAPHORE_H_
#define WINDOWS_SEMAPHORE_H_

#include "libfuse_windows_internal.h"

/*
 * sem_t
 */

#define SEM_VALUE_MAX                   LONG_MAX

typedef struct _sem_t
{
    HANDLE waitobj;
    LONG count;
} sem_t;

static inline
int sem_init(sem_t *sem, int pshared, unsigned value)
{
    if (SEM_VALUE_MAX < value)
        return EINVAL;

    sem->waitobj = CreateSemaphore(0, 0, 1000000000, 0);
    if (0 == sem->waitobj)
        return ENOMEM;

    sem->count = (LONG)value;

    return 0;
}

static inline
int sem_destroy(sem_t *sem)
{
    CloseHandle(sem->waitobj);

    return 0;
}

static inline
int sem_post(sem_t *sem)
{
    if (0 >= InterlockedIncrement(&sem->count))
        ReleaseSemaphore(sem->waitobj, 1, 0);

    return 0;
}

static inline
int sem_wait(sem_t *sem)
{
    if (0 > InterlockedDecrement(&sem->count))
        PthreadCancelableWaitForSingleObject(sem->waitobj, INFINITE);

    return 0;
}

#endif /* WINDOWS_SEMAPHORE_H_ */
