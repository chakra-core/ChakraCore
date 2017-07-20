//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CommonBasic.h"

// === C Runtime Header Files ===
#ifndef USING_PAL_STDLIB
#pragma warning(push)
#pragma warning(disable: 4995) /* 'function': name was marked as #pragma deprecated */
#include <stdio.h>
#pragma warning(pop)
#ifdef _WIN32
#include <intrin.h>
#endif
#endif

// === Core Header Files ===
// In Debug mode, the PALs definition of max and min are insufficient
// since some of our code expects the template min-max instead, so
// including that here
#if defined(DBG) && !defined(_MSC_VER)
#pragma push_macro("NO_PAL_MINMAX")
#pragma push_macro("_Post_equal_to")
#pragma push_macro("_Post_satisfies_")
#define NO_PAL_MINMAX
#define _Post_equal_to_(x)
#define _Post_satisfies_(x)
#endif

#include "Core/CommonMinMax.h"

// Restore the macros
#if defined(DBG) && !defined(_MSC_VER)
#pragma pop_macro("NO_PAL_MINMAX")
#pragma pop_macro("_Post_equal_to")
#pragma pop_macro("_Post_satisfies_")
#endif

#include "EnumHelp.h"
#include "Core/Assertions.h"
#include "Core/SysInfo.h"

#include "Core/PerfCounter.h"
#include "Core/PerfCounterSet.h"

#include "Common/MathUtil.h"
#include "Core/AllocSizeMath.h"
#include "Core/FaultInjection.h"

#include "Core/BasePtr.h"
#include "Core/AutoFile.h"
#include "Core/Output.h"

// === Basic Memory Header Files ===
namespace Memory
{
    class ArenaAllocator;
    class Recycler;
}
using namespace Memory;
#include "Memory/Allocator.h"
#include "Memory/HeapAllocator.h"
#include "Memory/RecyclerPointers.h"

// === Data structures Header Files ===
#include "DataStructures/QuickSort.h"
#include "DataStructures/DefaultContainerLockPolicy.h"
#include "DataStructures/Comparer.h"
#include "DataStructures/SizePolicy.h"
#include "DataStructures/BitVector.h"
#include "DataStructures/SList.h"
#include "DataStructures/DList.h"
#include "DataStructures/KeyValuePair.h"
#include "DataStructures/BaseDictionary.h"
#include "DataStructures/DictionaryEntry.h"

// === Configurations Header ===
#include "Core/ConfigFlagsTable.h"
#include "Core/GlobalSecurityPolicy.h"

// === Page/Arena Memory Header Files ===
#include "Memory/SectionAllocWrapper.h"
#include "Memory/VirtualAllocWrapper.h"
#include "Memory/MemoryTracking.h"
#include "Memory/AllocationPolicyManager.h"
#include "Memory/PageAllocator.h"
#include "Memory/ArenaAllocator.h"
