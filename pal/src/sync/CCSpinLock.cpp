//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "pal/thread.hpp"
#include "pal/cs.hpp"
#include "pal_assert.h"

#include <sched.h>
#include <pthread.h>

template<bool shouldTrackThreadId>
void CCSpinLock<shouldTrackThreadId>::Enter()
{
    size_t currentThreadId = -1, spin = 0;
    if (shouldTrackThreadId)
    {
        currentThreadId = GetCurrentThreadId();
        if (this->threadId == currentThreadId)
        {
            _ASSERTE(this->enterCount > 0);
            this->enterCount++;
            return;
        }
    }

    while(__sync_lock_test_and_set(&lock, 1))
    {
        while(this->lock)
        {
            if (++spin > this->spinCount)
            {
                sched_yield();
                spin = 0;
            }
        }
    }

    if (shouldTrackThreadId)
    {
        this->threadId = GetCurrentThreadId();
        _ASSERTE(this->enterCount == 0);
        this->enterCount = 1;
    }
}
template void CCSpinLock<false>::Enter();
template void CCSpinLock<true>::Enter();

template<bool shouldTrackThreadId>
bool CCSpinLock<shouldTrackThreadId>::TryEnter()
{
    int spin = 0;
    bool locked = true;

    while(__sync_lock_test_and_set(&lock, 1))
    {
        while(this->lock)
        {
            if (++spin > this->spinCount)
            {
               locked = false;
               goto FAIL;
            }
        }
    }

FAIL:

    if (shouldTrackThreadId)
    {
        size_t currentThreadId = GetCurrentThreadId();
        if (locked)
        {
            this->threadId = currentThreadId;
            _ASSERTE(this->enterCount == 0);
            this->enterCount = 1;
        }
        else if (threadId == currentThreadId)
        {
            _ASSERTE(this->enterCount > 0);
            this->enterCount++;
            locked = true;
        }
    }

    return locked;
}
template bool CCSpinLock<false>::TryEnter();
template bool CCSpinLock<true>::TryEnter();

template<bool shouldTrackThreadId>
void CCSpinLock<shouldTrackThreadId>::Leave()
{
    if (shouldTrackThreadId)
    {
        _ASSERTE(threadId == GetCurrentThreadId() && "Something is terribly wrong.");
        _ASSERTE(enterCount > 0);
        if (--enterCount == 0)
        {
            this->threadId = 0;
        }
        else
        {
            return;
        }
    }
    __sync_lock_release(&lock);
}
template void CCSpinLock<false>::Leave();
template void CCSpinLock<true>::Leave();

template<bool shouldTrackThreadId>
bool CCSpinLock<shouldTrackThreadId>::IsLocked() const
{
    return this->threadId == GetCurrentThreadId();
}
template bool CCSpinLock<false>::IsLocked() const;
template bool CCSpinLock<true>::IsLocked() const;
