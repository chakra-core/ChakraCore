//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>
#include <assert_only.h>
#include <cclock.hpp>
#include <errno.h>
// do not include PAL

void CCLock::Reset(bool shouldTrackThreadId)
{
    Assert(sizeof(pthread_mutex_t) <= sizeof(this->mutexPtr));

    if (*((size_t*)mutexPtr) != 0)
    {
        return; // already initialized
    }

    int err;
    pthread_mutexattr_t mtconf;
    if (shouldTrackThreadId)
    {
        err = pthread_mutexattr_init(&mtconf); Assert(err == 0);
        err = pthread_mutexattr_settype(&mtconf, PTHREAD_MUTEX_RECURSIVE); Assert(err == 0);
    }

    pthread_mutex_t *mutex = (pthread_mutex_t*)this->mutexPtr;
    err = pthread_mutex_init(mutex, shouldTrackThreadId ? &mtconf : NULL); Assert(err == 0);

    if (shouldTrackThreadId)
    {
        err = pthread_mutexattr_destroy(&mtconf); Assert(err == 0);
    }
}

CCLock::~CCLock()
{
    pthread_mutex_t *mutex = (pthread_mutex_t*)this->mutexPtr;
    pthread_mutex_destroy(mutex);
    *((size_t*)mutexPtr) = 0;
}

void CCLock::Enter()
{
    pthread_mutex_t *mutex = (pthread_mutex_t*)this->mutexPtr;
    int err = pthread_mutex_lock(mutex);
    AssertMsg(err == 0 || *((size_t*)mutexPtr) == 0, "Mutex Enter has failed");
}

void CCLock::Leave()
{
    pthread_mutex_t *mutex = (pthread_mutex_t*)this->mutexPtr;
    int err = pthread_mutex_unlock(mutex);
    AssertMsg(err == 0 || *((size_t*)mutexPtr) == 0, "Mutex Leave has failed");
}

bool CCLock::TryEnter()
{
    pthread_mutex_t *mutex = (pthread_mutex_t*)this->mutexPtr;
    int err = pthread_mutex_trylock(mutex);
    AssertMsg(err == 0 || err == EBUSY || *((size_t*)mutexPtr) == 0, "Mutex TryEnter has failed");

    if (err != EBUSY)
    {
        return true;
    }
    return false;
}

bool CCLock::IsLocked() const
{
#ifndef DEBUG
    fprintf(stderr, "you shouldn't need this for release build, It's expensive!\n");
    abort();
#endif
    return true; // there is no good way to do this, return true for CC Assert stuff.
                 // if you need to know whether it is locked or not, you may try
                 // TryEnter -> Leave couple. If it's not locked, TryEnter should succeed
}

