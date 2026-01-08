//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef CC_PAL_INC_CCLOCK_H
#define CC_PAL_INC_CCLOCK_H

#if defined(__IOS__) || defined(__ANDROID__)
#define CCLOCK_ALIGN __declspec(align(8))
#else
#define CCLOCK_ALIGN
#endif

class CCLOCK_ALIGN CCLock
{
    __declspec(align(sizeof(size_t)))
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
