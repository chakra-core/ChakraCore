//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

/***************************************************************************
NoReleaseAllocator - allocator that never releases until it is destroyed
***************************************************************************/
class NoReleaseAllocator
{
public:
    NoReleaseAllocator(DECLSPEC_GUARD_OVERFLOW int32 cbFirst = 256, DECLSPEC_GUARD_OVERFLOW int32 cbMax = 0x4000 /*16K*/);
    ~NoReleaseAllocator(void) { FreeAll(); }

    void *Alloc(DECLSPEC_GUARD_OVERFLOW int32 cb);
    void FreeAll();
    void Clear() { FreeAll(); }

private:
    struct NraBlock
    {
        NraBlock * pblkNext;
        // ... DATA ...
    };
    NraBlock * m_pblkList;
    int32 m_ibCur;
    int32 m_ibMax;
    int32 m_cbMinBlock;
    int32 m_cbMaxBlock;

#if DEBUG
    int32 m_cbTotRequested;    // total bytes requested
    int32 m_cbTotAlloced;    // total bytes allocated including headers
    int32 m_cblk;            // number of blocks including big blocks
    int32 m_cpvBig;            // each generates its own big block
    int32 m_cpvSmall;        // put in a common block
#endif //DEBUG
};
