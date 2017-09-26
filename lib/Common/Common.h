//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CommonMinMemory.h"

#ifdef _WIN32
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define MAKE_HR(errnum) (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, errnum))

// If we're using the PAL for C++ standard library compat,
// we don't need to include wchar for string handling
#ifndef USING_PAL_STDLIB
// === C Runtime Header Files ===
#include <wchar.h>
#include <stdarg.h>
#include <float.h>
#include <limits.h>
#if defined(_UCRT)
#include <cmath>
#else
#include <math.h>
#endif
#include <time.h>
#include <io.h>
#include <malloc.h>
#endif

#include "Common/GetCurrentFrameId.h"

namespace Js
{
    typedef int32 PropertyId;
    typedef uint32 ModuleID;
}

#define IsTrueOrFalse(value)     ((value) ? _u("True") : _u("False"))

// Header files
#include "Core/BinaryFeatureControl.h"
#include "TemplateParameter.h"

#include "Common/vtinfo.h"
#include "EnumClassHelp.h"
#include "Common/Tick.h"

#include "Common/IntMathCommon.h"
#include "Common/Int16Math.h"
#include "Common/Int32Math.h"
#include "Common/UInt16Math.h"
#include "Common/UInt32Math.h"
#include "Common/Int64Math.h"

template<typename T> struct IntMath { using Type = void; };
template<> struct IntMath<int16> { using Type = Int16Math; };
template<> struct IntMath<int32> { using Type = Int32Math; };
template<> struct IntMath<uint16> { using Type = UInt16Math; };
template<> struct IntMath<uint32> { using Type = UInt32Math; };
template<> struct IntMath<int64> { using Type = Int64Math; };

#include "Common/DateUtilities.h"
#include "Common/NumberUtilitiesBase.h"
#include "Common/NumberUtilities.h"
#include <Codex/Utf8Codex.h>

#include "Core/DelayLoadLibrary.h"
#include "Core/EtwTraceCore.h"

#include "Common/RejitReason.h"
#include "Common/ThreadService.h"

// Exceptions
#include "Exceptions/ExceptionBase.h"
#include "Exceptions/JavascriptException.h"
#include "Exceptions/OutOfMemoryException.h"
#include "Exceptions/OperationAbortedException.h"
#include "Exceptions/RejitException.h"
#include "Exceptions/ScriptAbortException.h"
#include "Exceptions/StackOverflowException.h"
#include "Exceptions/NotImplementedException.h"
#include "Exceptions/AsmJsParseException.h"

#include "Memory/AutoPtr.h"
#include "Memory/AutoAllocatorObjectPtr.h"
#include "Memory/LeakReport.h"

#include "DataStructures/DoublyLinkedListElement.h"
#include "DataStructures/DoublyLinkedList.h"
#include "DataStructures/SimpleHashTable.h"
#include "Memory/XDataAllocator.h"
#include "Memory/CustomHeap.h"

#include "Core/FinalizableObject.h"
#include "Memory/RecyclerRootPtr.h"
#include "Memory/RecyclerFastAllocator.h"
#include "Util/Pinned.h"

// Data Structures 2

#include "DataStructures/QuickSort.h"
#include "DataStructures/StringBuilder.h"
#include "DataStructures/WeakReferenceDictionary.h"
#include "DataStructures/LeafValueDictionary.h"
#include "DataStructures/Dictionary.h"
#include "DataStructures/List.h"
#include "DataStructures/Stack.h"
#include "DataStructures/Queue.h"
#include "DataStructures/CharacterBuffer.h"
#include "DataStructures/InternalString.h"
#include "DataStructures/Interval.h"
#include "DataStructures/InternalStringNoCaseComparer.h"
#include "DataStructures/SparseArray.h"
#include "DataStructures/GrowingArray.h"
#include "DataStructures/EvalMapString.h"
#include "DataStructures/RegexKey.h"

#include "Core/ICustomConfigFlags.h"
#include "Core/CmdParser.h"
#include "Core/ProfileInstrument.h"
#include "Core/ProfileMemory.h"
#include "Core/StackBackTrace.h"

#include "Common/Event.h"
#include "Common/Jobs.h"

#include "Common/vtregistry.h" // Depends on SimpleHashTable.h
#include "DataStructures/Cache.h" // Depends on config flags
#include "DataStructures/MruDictionary.h" // Depends on DoublyLinkedListElement

#include "Common/SmartFpuControl.h"



// This class is only used by AutoExp.dat
class AutoExpDummyClass
{
};

#pragma warning(push)
#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#ifdef _MSC_VER
#include <typeinfo.h>
#else
#include <typeinfo>
#endif
#endif
#pragma warning(pop)
