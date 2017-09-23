//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "CommonMinMemory.h"

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define MAKE_HR(errnum) (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_CONTROL, errnum))

// === C Runtime Header Files ===
#include <time.h>
#if defined(_UCRT)
#include <cmath>
#else
#include <math.h>
#endif

// Exceptions
#include "Exceptions/ExceptionBase.h"
#include "Exceptions/OutOfMemoryException.h"

#include "../Parser/rterror.h"

// Other Memory headers
#include "Memory/LeakReport.h"
#include "Memory/AutoPtr.h"

// Other core headers
#include "Core/FinalizableObject.h"
#include "Core/EtwTraceCore.h"
#include "Core/ProfileInstrument.h"
#include "Core/ProfileMemory.h"
#include "Core/StackBackTrace.h"

#ifdef _MSC_VER
#pragma warning(push)
#if defined(PROFILE_RECYCLER_ALLOC) || defined(HEAP_TRACK_ALLOC) || defined(ENABLE_DEBUG_CONFIG_OPTIONS)
#include <typeinfo.h>
#endif
#pragma warning(pop)
#endif

// Inl files
#include "Memory/Recycler.inl"
#include "Memory/MarkContext.inl"
#include "Memory/HeapBucket.inl"
#include "Memory/LargeHeapBucket.inl"
#include "Memory/HeapBlock.inl"
#include "Memory/HeapBlockMap.inl"


// Memory Protections
#ifdef _CONTROL_FLOW_GUARD
#define PAGE_EXECUTE_RW_TARGETS_INVALID   (PAGE_EXECUTE_READWRITE | PAGE_TARGETS_INVALID)
#define PAGE_EXECUTE_RW_TARGETS_NO_UPDATE (PAGE_EXECUTE_READWRITE | PAGE_TARGETS_NO_UPDATE)
#define PAGE_EXECUTE_RO_TARGETS_NO_UPDATE (PAGE_EXECUTE_READ      | PAGE_TARGETS_NO_UPDATE)
#else
#define PAGE_EXECUTE_RW_TARGETS_INVALID   (PAGE_EXECUTE_READWRITE)
#define PAGE_EXECUTE_RW_TARGETS_NO_UPDATE (PAGE_EXECUTE_READWRITE)
#define PAGE_EXECUTE_RO_TARGETS_NO_UPDATE (PAGE_EXECUTE_READ)
#endif
