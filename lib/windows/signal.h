/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#ifndef WINDOWS_SIGNAL_H_
#define WINDOWS_SIGNAL_H_

#include "libfuse_windows_internal.h"

#ifndef SIGHUP
#define SIGHUP                          1
#endif
#ifndef SIGINT
#define SIGINT                          2
#endif
#ifndef SIGQUIT
#define SIGQUIT                         3
#endif
#ifndef SIGPIPE
#define SIGPIPE                         13
#endif
#ifndef SIGTERM
#define SIGTERM                         15
#endif

#define sigaddset(set, sig)             (*(set) |=  (1 << (sig)), 0)
#define sigdelset(set, sig)             (*(set) &= ~(1 << (sig)), 0)
#define sigemptyset(set)                (*(set) =  0, 0)
#define sigfillset(set)                 (*(set) = ~0, 0)
#define sigismember(set, sig)           (!!(*(set) & (1 << (sig))))

#define SIG_SETMASK                     0
#define SIG_BLOCK                       1
#define SIG_UNBLOCK                     2

typedef uintptr_t sigset_t;

static inline
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset)
{
    return 0;
}

#endif /* WINDOWS_SIGNAL_H_ */
