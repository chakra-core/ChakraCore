//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum ErrorReason
{
    JavascriptDispatch_OUTOFMEMORY = 1,
    Fatal_Internal_Error = 2,
    Fatal_Debug_Heap_OUTOFMEMORY = 3,
    Fatal_Amd64StackWalkerOutOfContexts = 4,
    // Unused = 5,
    Fatal_Binary_Inconsistency = 6,
    WriteBarrier_OUTOFMEMORY = 7,
    CustomHeap_MEMORYCORRUPTION = 8,
    LargeHeapBlock_Metadata_Corrupt = 9,
    Fatal_Version_Inconsistency = 10,
    MarkStack_OUTOFMEMORY = 11,
    EnterScript_FromDOM_NoScriptScope = 12,
    Fatal_FailedToBox_OUTOFMEMORY = 13,
    Fatal_Recycler_MemoryCorruption = 14,
    Fatal_Debugger_AttachDetach_Failure = 15,
    Fatal_EntryExitRecordCorruption = 16,
    Fatal_UnexpectedExceptionHandling = 17,
    Fatal_RpcFailure = 18,
    Fatal_JsReentrancy_Error = 19,
    Fatal_TTDAbort = 20,
    Fatal_Failed_API_Result = 21,
    Fatal_OutOfMemory = 22,
    // Unused = 23,
    Fatal_JsBuiltIn_Error = 24,
    Fatal_XDataRegistration = 25,
};

extern "C" void ReportFatalException(
    _In_ ULONG_PTR context,
    _In_ HRESULT exceptionCode,
    _In_ ErrorReason reasonCode,
    _In_ ULONG_PTR scenario);

// We can have other error handle code path with
// unique call stack so we can collect data in Dr. Watson.
void JavascriptDispatch_OOM_fatal_error(
    _In_ ULONG_PTR context);

void CustomHeap_BadPageState_unrecoverable_error(
    _In_ ULONG_PTR context);

void Amd64StackWalkerOutOfContexts_unrecoverable_error(
    _In_ ULONG_PTR context);

void FailedToBox_OOM_unrecoverable_error(
    _In_ ULONG_PTR context);

#if defined(RECYCLER_WRITE_BARRIER) && defined(TARGET_64)
void X64WriteBarrier_OOM_unrecoverable_error();
#endif

void DebugHeap_OOM_fatal_error();

void MarkStack_OOM_unrecoverable_error();

void Binary_Inconsistency_fatal_error();
void Version_Inconsistency_fatal_error();
void EntryExitRecord_Corrupted_unrecoverable_error();
void UnexpectedExceptionHandling_fatal_error();

#ifdef LARGEHEAPBLOCK_ENCODING
void LargeHeapBlock_Metadata_Corrupted(
    _In_ ULONG_PTR context, _In_ unsigned char calculatedCheckSum);
#endif

void FromDOM_NoScriptScope_unrecoverable_error();
void Debugger_AttachDetach_unrecoverable_error(HRESULT hr);
void RpcFailure_unrecoverable_error(HRESULT hr);
void OutOfMemory_unrecoverable_error();
void RecyclerSingleAllocationLimit_unrecoverable_error();
void MemGCSingleAllocationLimit_unrecoverable_error();

void OutOfMemoryTooManyPinnedObjects_unrecoverable_error();
void OutOfMemoryTooManyClosedContexts_unrecoverable_error();
void OutOfMemoryAllocationPolicy_unrecoverable_error();

void OutOfMemoryTooManyPinnedObjects_unrecoverable_error_visible();
void OutOfMemoryTooManyClosedContexts_unrecoverable_error_visible();
void OutOfMemoryAllocationPolicy_unrecoverable_error_visible();

void OutOfMemoryTooManyPinnedObjects_unrecoverable_error_notvisible();
void OutOfMemoryTooManyClosedContexts_unrecoverable_error_notvisible();
void OutOfMemoryAllocationPolicy_unrecoverable_error_notvisible();

void XDataRegistration_unrecoverable_error(HRESULT hr, ULONG_PTR scenario);

inline void OutOfMemoryTooManyPinnedObjects_unrecoverable_error(BYTE visibility)
{
    switch (visibility)
    {
    case 1:
        OutOfMemoryTooManyPinnedObjects_unrecoverable_error_visible();
        break;
    case 2:
        OutOfMemoryTooManyPinnedObjects_unrecoverable_error_notvisible();
        break;
    default:
        OutOfMemoryTooManyPinnedObjects_unrecoverable_error();
        break;
    }
}

inline void OutOfMemoryTooManyClosedContexts_unrecoverable_error(BYTE visibility)
{
    switch (visibility)
    {
    case 1:
        OutOfMemoryTooManyClosedContexts_unrecoverable_error_visible();
        break;
    case 2:
        OutOfMemoryTooManyClosedContexts_unrecoverable_error_notvisible();
        break;
    default:
        OutOfMemoryTooManyClosedContexts_unrecoverable_error();
        break;
    }
}

inline void OutOfMemoryAllocationPolicy_unrecoverable_error(BYTE visibility)
{
    switch (visibility)
    {
    case 1:
        OutOfMemoryAllocationPolicy_unrecoverable_error_visible();
        break;
    case 2:
        OutOfMemoryAllocationPolicy_unrecoverable_error_notvisible();
        break;
    default:
        OutOfMemoryAllocationPolicy_unrecoverable_error();
        break;
    }
}

#ifndef DISABLE_SEH
// RtlReportException is available on Vista and up, but we cannot use it for OOB release.
// Use UnhandleExceptionFilter to let the default handler handles it.
inline LONG FatalExceptionFilter(
    _In_ LPEXCEPTION_POINTERS lpep, 
    _In_ void * addressToBlame = nullptr)
{
    if (addressToBlame != nullptr)
    {
        lpep->ExceptionRecord->ExceptionAddress = addressToBlame;
    }

    LONG rc = UnhandledExceptionFilter(lpep);

    // re == EXCEPTION_EXECUTE_HANDLER means there is no debugger attached, let's terminate
    // the process. Otherwise give control to the debugger.
    // Note: in case when postmortem debugger is registered but no actual debugger attached,
    //       rc will be 0 (and EXCEPTION_EXECUTE_HANDLER is 1), so it acts as if there is debugger attached.
    if (rc == EXCEPTION_EXECUTE_HANDLER)
    {
        TerminateProcess(GetCurrentProcess(), (UINT)DBG_TERMINATE_PROCESS);
    }
    else
    {
        // However, if debugger was not attached for some reason, terminate the process.
        if (!IsDebuggerPresent())
        {
            TerminateProcess(GetCurrentProcess(), (UINT)DBG_TERMINATE_PROCESS);
        }
        DebugBreak();
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
#endif // DISABLE_SEH


template<class Fn>
static STDMETHODIMP DebugApiWrapper(Fn fn)
{
    // If an assertion or AV is hit, it triggers a SEH exception. SEH exceptions escaped here will be eaten by PDM. To prevent assertions
    // from getting unnoticed, we install a SEH exception filter and crash the process.
#if ENABLE_DEBUG_API_WRAPPER
    __try
    {
#endif
        return fn();
#if ENABLE_DEBUG_API_WRAPPER
    }
    __except(FatalExceptionFilter(GetExceptionInformation()))
    {
    }
    return E_FAIL;
#endif
}
