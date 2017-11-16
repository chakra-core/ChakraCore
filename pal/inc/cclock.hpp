//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef CC_PAL_INC_CCLOCK_H
#define CC_PAL_INC_CCLOCK_H

class CCLock
{
    char           mutexPtr[64]; // keep mutex implementation opaque to consumer (PAL vs non-PAL)

public:
    void Reset(bool shouldTrackThreadId = false);
    CCLock(bool shouldTrackThreadId = false)
    {
        *((size_t*)mutexPtr) = 0;
        Reset(shouldTrackThreadId);
    }

    ~CCLock();

    void Enter();
    bool TryEnter();
    void Leave();
    bool IsLocked() const;
};

#endif // CC_PAL_INC_CCLOCK_H
