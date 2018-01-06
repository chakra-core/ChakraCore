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

struct FunctionTableNode
{
#ifdef _WIN32
    SLIST_ENTRY itemEntry;
    FunctionTableHandle functionTable;

#endif
};

struct DelayDeletingFunctionTable
{
    static PSLIST_HEADER Head;
    static DelayDeletingFunctionTable Instance;

    DelayDeletingFunctionTable();
    ~DelayDeletingFunctionTable();

    static bool AddEntry(FunctionTableHandle ft);
    static void Clear();
    static bool IsEmpty();
};