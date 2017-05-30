//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef CC_PAL_INC_CCSPINLOCK_H
#define CC_PAL_INC_CCSPINLOCK_H

template<bool shouldTrackThreadId>
class CCSpinLock
{
    unsigned int  enterCount;
    unsigned int  spinCount;
    char          lock;
    DWORD         threadId;
public:
    // TODO: (obastemur) tune this / don't use constant here?
    void Reset(unsigned int spinCount = 11)
    {
        this->enterCount = 0;
        this->lock = 0;
        this->threadId = 0;
        this->spinCount = spinCount;
    }

    CCSpinLock(unsigned int spinCount)
    {
        Reset(spinCount);
    }

    CCSpinLock() { Reset(); }

    void Enter();
    bool TryEnter();
    void Leave();
    bool IsLocked() const;
};

#endif // CC_PAL_INC_CCSPINLOCK_H
