//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _M_X64
#include "Memory/amd64/XDataAllocator.h"
#elif defined(_M_ARM)
#include "Memory/arm/XDataAllocator.h"
#elif defined(_M_ARM64)
#include "Memory/arm64/XDataAllocator.h"
#endif

#if PDATA_ENABLED && defined(_WIN32)
struct FunctionTableNode
{
    SLIST_ENTRY itemEntry;
    XDataAllocation* xdata;
};

struct DelayDeletingFunctionTable
{
    static PSLIST_HEADER Head;
    static DelayDeletingFunctionTable Instance;
    static CriticalSection cs;

    DelayDeletingFunctionTable();
    ~DelayDeletingFunctionTable();

    static bool AddEntry(XDataAllocation* xdata);
    static void Clear();
    static bool IsEmpty();
    static void DeleteFunctionTable(XDataAllocation* xdata);
};
#endif