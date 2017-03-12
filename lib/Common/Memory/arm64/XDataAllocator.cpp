//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// This one works only for ARM64
#include "CommonMemoryPch.h"
#if !defined(_M_ARM64)
CompileAssert(false)
#endif

#include "XDataAllocator.h"
#include "Core/DelayLoadLibrary.h"

XDataAllocator::XDataAllocator(BYTE* address, uint size)
{
    __debugbreak();
}


void XDataAllocator::Delete()
{
    __debugbreak();
}

bool XDataAllocator::Initialize(void* segmentStart, void* segmentEnd)
{
    __debugbreak();
    return true;
}

bool XDataAllocator::Alloc(ULONG_PTR functionStart, DWORD functionSize, ushort pdataCount, ushort xdataSize, SecondaryAllocation* allocation)
{
    __debugbreak();
    return false;
}


void XDataAllocator::Release(const SecondaryAllocation& allocation)
{
    __debugbreak();
}

/* static */
void XDataAllocator::Register(XDataAllocation * xdataInfo, intptr_t functionStart, DWORD functionSize)
{
    __debugbreak();
}

/* static */
void XDataAllocator::Unregister(XDataAllocation * xdataInfo)
{
    __debugbreak();
}

bool XDataAllocator::CanAllocate()
{
    __debugbreak();
    return true;
}
