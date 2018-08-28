//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonMemoryPch.h"
#include "Memory/XDataAllocator.h"
#if PDATA_ENABLED && defined(_WIN32)
#include "Core/DelayLoadLibrary.h"
#include <malloc.h>

CriticalSection DelayDeletingFunctionTable::cs;
PSLIST_HEADER DelayDeletingFunctionTable::Head = nullptr;
DelayDeletingFunctionTable DelayDeletingFunctionTable::Instance;

DelayDeletingFunctionTable::DelayDeletingFunctionTable()
{
    Head = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);
    if (Head)
    {
        InitializeSListHead(Head);
    }
}
DelayDeletingFunctionTable::~DelayDeletingFunctionTable()
{
    Clear();
    if (Head)
    {
        DebugOnly(SLIST_ENTRY* entry = InterlockedPopEntrySList(Head));
        Assert(entry == nullptr);
        _aligned_free(Head);
        Head = nullptr;
    }
}


bool DelayDeletingFunctionTable::AddEntry(FunctionTableHandle ft)
{
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
    return false;
}

void DelayDeletingFunctionTable::Clear()
{
    if (Head)
    {
        // add lock here to prevent in case one thread popped the entry and before deleting
        // another thread is registering function table for the same address
        AutoCriticalSection autoCS(&cs); 

        SLIST_ENTRY* entry = InterlockedPopEntrySList(Head);
        while (entry)
        {
            FunctionTableNode* list = (FunctionTableNode*)entry;
            DeleteFunctionTable(list->functionTable);
            _aligned_free(entry);
            entry = InterlockedPopEntrySList(Head);
        }
    }
}

bool DelayDeletingFunctionTable::IsEmpty()
{
    return QueryDepthSList(Head) == 0;
}

void DelayDeletingFunctionTable::DeleteFunctionTable(void* functionTable)
{
    NtdllLibrary::Instance->DeleteGrowableFunctionTable(functionTable);
}

#endif
