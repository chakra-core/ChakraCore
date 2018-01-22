//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"
#include "Memory/XDataAllocator.h"
#if PDATA_ENABLED && defined(_WIN32)
#include "Core/DelayLoadLibrary.h"
#include <malloc.h>
#endif

PSLIST_HEADER DelayDeletingFunctionTable::Head = nullptr;
DelayDeletingFunctionTable DelayDeletingFunctionTable::Instance;

DelayDeletingFunctionTable::DelayDeletingFunctionTable()
{
#if PDATA_ENABLED && defined(_WIN32)
    Head = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
    if (Head)
    {
        InitializeSListHead(Head);
    }
#endif
}
DelayDeletingFunctionTable::~DelayDeletingFunctionTable()
{
#if PDATA_ENABLED && defined(_WIN32)
    Clear();
    if (Head)
    {
        DebugOnly(SLIST_ENTRY* entry = InterlockedPopEntrySList(Head));
        Assert(entry == nullptr);
        _aligned_free(Head);
        Head = nullptr;
    }
#endif
}


bool DelayDeletingFunctionTable::AddEntry(FunctionTableHandle ft)
{
#if PDATA_ENABLED && defined(_WIN32)
    if (Head)
    {
        FunctionTableNode* node = (FunctionTableNode*)_aligned_malloc(sizeof(FunctionTableNode), MEMORY_ALLOCATION_ALIGNMENT);
        if (node)
        {
            node->functionTable = ft;
            InterlockedPushEntrySList(Head, &(node->itemEntry));
            return true;
        }
    }
#endif
    return false;
}

void DelayDeletingFunctionTable::Clear()
{
#if PDATA_ENABLED && defined(_WIN32)
    if (Head)
    {
        SLIST_ENTRY* entry = InterlockedPopEntrySList(Head);
        while (entry)
        {
            FunctionTableNode* list = (FunctionTableNode*)entry;
            DeleteFunctionTable(list->functionTable);
            _aligned_free(entry);
            entry = InterlockedPopEntrySList(Head);
        }
    }
#endif
}

bool DelayDeletingFunctionTable::IsEmpty()
{
#if PDATA_ENABLED && defined(_WIN32)
    return QueryDepthSList(Head) == 0;
#else
    return true;
#endif
}

void DelayDeletingFunctionTable::DeleteFunctionTable(void* functionTable)
{
#if PDATA_ENABLED && defined(_WIN32)
    NtdllLibrary::Instance->DeleteGrowableFunctionTable(functionTable);
#endif
}