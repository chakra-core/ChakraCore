//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "JsrtInternal.h"
#include "JsrtExternalObject.h"
#include "JsrtExternalArrayBuffer.h"
#include "jsrtHelper.h"

#include "JsrtSourceHolder.h"
#include "ByteCode/ByteCodeSerializer.h"
#include "Common/ByteSwap.h"
#include "Library/DataView.h"
#include "Library/JavascriptSymbol.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Codex/Utf8Helper.h"

// Parser Includes
#include "cmperr.h"     // For ERRnoMemory
#include "screrror.h"   // For CompileScriptException

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#include "TestHooksRt.h"
#endif

#if !ENABLE_TTD
#define PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(CTX) false

#define PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(CTX, ACTION_CODE)
#define PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(CTX, ACTION_CODE)
#define PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(RESULT)

//TODO: find and replace all of the occourences of this in jsrt.cpp
#define PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(CTX)
#endif

struct CodexHeapAllocatorInterface
{
public:
    static void* allocate(size_t size)
    {
        return HeapNewArray(char, size);
    }

    static void free(void* ptr, size_t count)
    {
        HeapDeleteArray(count, (char*) ptr);
    }
};

typedef utf8::NarrowWideConverter<CodexHeapAllocatorInterface, LPCSTR, LPWSTR> NarrowToWideChakraHeap;
typedef utf8::NarrowWideConverter<CodexHeapAllocatorInterface, LPCWSTR, LPSTR> WideToNarrowChakraHeap;

JsErrorCode CheckContext(JsrtContext *currentContext, bool verifyRuntimeState, bool allowInObjectBeforeCollectCallback)
{
    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    Assert(scriptContext != nullptr);
    Recycler *recycler = scriptContext->GetRecycler();
    ThreadContext *threadContext = scriptContext->GetThreadContext();

    // We don't need parameter check if it's checked in previous wrapper.
    if (verifyRuntimeState)
    {
        if (recycler && recycler->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (!allowInObjectBeforeCollectCallback && recycler && recycler->IsInObjectBeforeCollectCallback())
        {
            return JsErrorInObjectBeforeCollectCallback;
        }
        else if (threadContext->IsExecutionDisabled())
        {
            return JsErrorInDisabledState;
        }
        else if (scriptContext->IsInProfileCallback())
        {
            return JsErrorInProfileCallback;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        // Make sure we don't have an outstanding exception.
        if (scriptContext->GetThreadContext()->GetRecordedException() != nullptr)
        {
            return JsErrorInExceptionState;
        }
    }

    return JsNoError;
}

/////////////////////

//A create runtime function that we can funnel to for regular and record or debug aware creation
JsErrorCode CreateRuntimeCore(_In_ JsRuntimeAttributes attributes, _In_opt_ const byte* optRecordUri, size_t optRecordUriCount, _In_opt_ const byte* optDebugUri, size_t optDebugUriCount, _In_ UINT32 snapInterval, _In_ UINT32 snapHistoryLength, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    VALIDATE_ENTER_CURRENT_THREAD();

    return GlobalAPIWrapper([&]() -> JsErrorCode {
        PARAM_NOT_NULL(runtimeHandle);
        *runtimeHandle = nullptr;

        const JsRuntimeAttributes JsRuntimeAttributesAll =
            (JsRuntimeAttributes)(
            JsRuntimeAttributeDisableBackgroundWork |
            JsRuntimeAttributeAllowScriptInterrupt |
            JsRuntimeAttributeEnableIdleProcessing |
            JsRuntimeAttributeDisableEval |
            JsRuntimeAttributeDisableNativeCodeGeneration |
            JsRuntimeAttributeEnableExperimentalFeatures |
            JsRuntimeAttributeDispatchSetExceptionsToDebugger
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            | JsRuntimeAttributeSerializeLibraryByteCode
#endif
        );

        Assert((attributes & ~JsRuntimeAttributesAll) == 0);
        if ((attributes & ~JsRuntimeAttributesAll) != 0)
        {
            return JsErrorInvalidArgument;
        }
        CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 0, nullptr);
        AllocationPolicyManager * policyManager = HeapNew(AllocationPolicyManager, (attributes & JsRuntimeAttributeDisableBackgroundWork) == 0);
        bool enableExperimentalFeatures = (attributes & JsRuntimeAttributeEnableExperimentalFeatures) != 0;
        ThreadContext * threadContext = HeapNew(ThreadContext, policyManager, threadService, enableExperimentalFeatures);

        if (((attributes & JsRuntimeAttributeDisableBackgroundWork) != 0)
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            && !Js::Configuration::Global.flags.ConcurrentRuntime
#endif
            )
        {
            threadContext->OptimizeForManyInstances(true);
#if ENABLE_NATIVE_CODEGEN
            threadContext->EnableBgJit(false);
#endif
        }

        if (!threadContext->IsRentalThreadingEnabledInJSRT()
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            || Js::Configuration::Global.flags.DisableRentalThreading
#endif
            )
        {
            threadContext->SetIsThreadBound();
        }

        if (attributes & JsRuntimeAttributeAllowScriptInterrupt)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagCanDisableExecution);
        }

        if (attributes & JsRuntimeAttributeDisableEval)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagEvalDisabled);
        }

        if (attributes & JsRuntimeAttributeDisableNativeCodeGeneration)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagNoJIT);
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.PrimeRecycler)
        {
            threadContext->EnsureRecycler()->Prime();
        }
#endif

#if ENABLE_TTD
        if (optRecordUri != nullptr || optDebugUri != nullptr)
        {
            AssertMsg(optRecordUri == nullptr || optDebugUri == nullptr, "We should only have 1 but we fail on context create if both are set");

            threadContext->IsTTRecordRequested = (optRecordUri != nullptr);
            threadContext->IsTTDebugRequested = (optDebugUri != nullptr);

            const byte* ttdUri = (optRecordUri != nullptr) ? optRecordUri : optDebugUri;
            size_t ttdUriByteLength = (optRecordUri != nullptr) ? optRecordUriCount : optDebugUriCount;

            // This class will null-terminate the string, so the original count is not including the null terminator
            threadContext->TTDUri.SetUriValue(ttdUriByteLength, ttdUri);
            threadContext->TTSnapInterval = snapInterval;

            threadContext->TTSnapHistoryLength = max<uint32>(2, snapHistoryLength);
        }
#endif

        bool enableIdle = (attributes & JsRuntimeAttributeEnableIdleProcessing) == JsRuntimeAttributeEnableIdleProcessing;
        bool dispatchExceptions = (attributes & JsRuntimeAttributeDispatchSetExceptionsToDebugger) == JsRuntimeAttributeDispatchSetExceptionsToDebugger;

        JsrtRuntime * runtime = HeapNew(JsrtRuntime, threadContext, enableIdle, dispatchExceptions);
        threadContext->SetCurrentThreadId(ThreadContext::NoThread);
        *runtimeHandle = runtime->ToHandle();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        runtime->SetSerializeByteCodeForLibrary((attributes & JsRuntimeAttributeSerializeLibraryByteCode) != 0);
#endif
        return JsNoError;
    });
}

//A create context function that we can funnel to for regular and record or debug aware creation
JsErrorCode CreateContextCore(_In_ JsRuntimeHandle runtimeHandle, _In_ bool createUnderTimeTravel, _Out_ JsContextRef *newContext)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        PARAM_NOT_NULL(newContext);
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();

        if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

#if ENABLE_TTD
        if(createUnderTimeTravel)
        {
            if(!(threadContext->IsTTRecordRequested | threadContext->IsTTDebugRequested))
            {
                AssertMsg(false, "Can't create a context under TT if runtime is not set to support TT!!!");
                return JsErrorCategoryUsage;
            }

            if(threadContext->IsTTRecordRequested & threadContext->IsTTDebugRequested)
            {
                AssertMsg(false, "Can't set both of these at the same time!!!");
                return JsErrorCategoryUsage;
            }
        }
#endif

        JsrtContext * context = JsrtContext::New(runtime);

#if ENABLE_TTD
        if (createUnderTimeTravel)
        {
            HostScriptContextCallbackFunctor callbackFunctor(context, &JsrtContext::OnScriptLoad_TTDCallback);
            threadContext->BeginCtxTimeTravel(context->GetScriptContext(), callbackFunctor);

#if TTD_DYNAMIC_DECOMPILATION_WORK_AROUNDS
            if(threadContext->IsTTRecordRequested | threadContext->IsTTDebugRequested)
            {
                //
                //TODO: We currently force this into debug mode in record as well to make sure parsing/bytecode generation is same as during replay.
                //      Later we will want to be clever during inflate and not do this.
                //
#ifdef _WIN32
                context->GetScriptContext()->InitializeDebugging();
#else
                //
                //TODO: x-plat does not like some parts of initiallize debugging so just set the flag we need 
                //
                context->GetScriptContext()->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);
#endif
            }
#endif

            threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
            context->GetScriptContext()->InitializeCoreImage_TTD();
            threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
        }
#endif

        JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

        if (jsrtDebugManager != nullptr)
        {
            Js::ScriptContext* scriptContext = context->GetScriptContext();
            scriptContext->InitializeDebugging();

            Js::DebugContext* debugContext = scriptContext->GetDebugContext();
            debugContext->SetHostDebugContext(jsrtDebugManager);

            Js::ProbeContainer* probeContainer = debugContext->GetProbeContainer();
            probeContainer->InitializeInlineBreakEngine(jsrtDebugManager);
            probeContainer->InitializeDebuggerScriptOptionCallback(jsrtDebugManager);

            threadContext->GetDebugManager()->SetLocalsDisplayFlags(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods);
        }

        *newContext = (JsContextRef)context;
        return JsNoError;
    });
}

/////////////////////

CHAKRA_API JsCreateRuntime(_In_ JsRuntimeAttributes attributes, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    return CreateRuntimeCore(attributes, nullptr /*optRecordUri*/, 0 /*optRecordUriCount */, nullptr /*optRecordUri*/, 0 /* optDebugUriCount */, UINT_MAX /*optSnapInterval*/, UINT_MAX /*optLogLength*/, threadService, runtimeHandle);
}

template <CollectionFlags flags>
JsErrorCode JsCollectGarbageCommon(JsRuntimeHandle runtimeHandle)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();

        if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        Recycler* recycler = threadContext->EnsureRecycler();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (flags & CollectOverride_SkipStack)
        {
            Recycler::AutoEnterExternalStackSkippingGCMode autoGC(recycler);
            recycler->CollectNow<flags>();
        }
        else
#endif
        {
            recycler->CollectNow<flags>();
        }
        return JsNoError;
    });
}

CHAKRA_API JsCollectGarbage(_In_ JsRuntimeHandle runtimeHandle)
{
    return JsCollectGarbageCommon<CollectNowExhaustive>(runtimeHandle);
}

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
CHAKRA_API JsPrivateCollectGarbageSkipStack(_In_ JsRuntimeHandle runtimeHandle)
{
    return JsCollectGarbageCommon<CollectNowExhaustiveSkipStack>(runtimeHandle);
}
#endif

CHAKRA_API JsDisposeRuntime(_In_ JsRuntimeHandle runtimeHandle)
{
    return GlobalAPIWrapper([&] () -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();
        ThreadContextScope scope(threadContext);

        // We should not dispose if the runtime is being used.
        if (!scope.IsValid() ||
            scope.WasInUse() ||
            (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress()))
        {
            return JsErrorRuntimeInUse;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        // Invoke and clear the callbacks while the contexts and runtime are still available
        {
            Recycler* recycler = threadContext->GetRecycler();
            if (recycler != nullptr)
            {
                recycler->ClearObjectBeforeCollectCallbacks();
            }
        }

        if (runtime->GetJsrtDebugManager() != nullptr)
        {
            runtime->GetJsrtDebugManager()->ClearDebuggerObjects();
        }

        // Close any open Contexts.
        // We need to do this before recycler shutdown, because ScriptEngine->Close won't work then.
        runtime->CloseContexts();

        runtime->DeleteJsrtDebugManager();

#if defined(CHECK_MEMORY_LEAK) || defined(LEAK_REPORT)
        bool doFinalGC = false;

#if defined(LEAK_REPORT)
        if (Js::Configuration::Global.flags.IsEnabled(Js::LeakReportFlag))
        {
            doFinalGC = true;
        }
#endif

#if defined(CHECK_MEMORY_LEAK)
        if (Js::Configuration::Global.flags.CheckMemoryLeak)
        {
            doFinalGC = true;
        }
#endif

        if (doFinalGC)
        {
            Recycler *recycler = threadContext->GetRecycler();
            if (recycler)
            {
                recycler->EnsureNotCollecting();
                recycler->CollectNow<CollectNowFinalGC>();
                Assert(!recycler->CollectionInProgress());
            }
        }
#endif

        runtime->SetBeforeCollectCallback(nullptr, nullptr);
        threadContext->CloseForJSRT();
        HeapDelete(threadContext);

        HeapDelete(runtime);

        scope.Invalidate();

        return JsNoError;
    });
}

CHAKRA_API JsAddRef(_In_ JsRef ref, _Out_opt_ unsigned int *count)
{
    VALIDATE_JSREF(ref);
    if (count != nullptr)
    {
        *count = 0;
    }

    if (Js::TaggedNumber::Is(ref))
    {
        // The count is always one because these are never collected
        if (count)
        {
            *count = 1;
        }
        return JsNoError;
    }

    if (JsrtContext::Is(ref))
    {
        return GlobalAPIWrapper([&] () -> JsErrorCode
        {
            Recycler * recycler = static_cast<JsrtContext *>(ref)->GetRuntime()->GetThreadContext()->GetRecycler();
            recycler->RootAddRef(ref, count);
            return JsNoError;
        });
    }
    else
    {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if (threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }
        Recycler * recycler = threadContext->GetRecycler();
        return GlobalAPIWrapper([&] () -> JsErrorCode
        {

            // Note, some references may live in arena-allocated memory, so we need to do this check
            if (!recycler->IsValidObject(ref))
            {
                return JsNoError;
            }

#if ENABLE_TTD
            unsigned int lCount = 0;
            recycler->RootAddRef(ref, &lCount);
            if (count != nullptr)
            {
                *count = lCount;
            }

            Js::ScriptContext* scriptContext = JsrtContext::GetCurrent()->GetScriptContext();
            if ((lCount == 1) & scriptContext->ShouldPerformRootAddOrRemoveRefAction())
            {
                if (!scriptContext->GetThreadContext()->TTDLog->IsPropertyRecordRef(ref))
                {
                    Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(ref);
                    scriptContext->TTDContextInfo->AddTrackedRoot(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(obj), obj);

                    if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
                    {
                        scriptContext->GetThreadContext()->TTDLog->RecordJsRTAddRootRef(scriptContext, (Js::Var)ref);
                    }
                }
            }
#else
            recycler->RootAddRef(ref, count);
#endif

            return JsNoError;
        });
    }
}

CHAKRA_API JsRelease(_In_ JsRef ref, _Out_opt_ unsigned int *count)
{
    VALIDATE_JSREF(ref);
    if (count != nullptr)
    {
        *count = 0;
    }

    if (Js::TaggedNumber::Is(ref))
    {
        // The count is always one because these are never collected
        if (count)
        {
            *count = 1;
        }
        return JsNoError;
    }

    if (JsrtContext::Is(ref))
    {
        return GlobalAPIWrapper([&] () -> JsErrorCode
        {
            Recycler * recycler = static_cast<JsrtContext *>(ref)->GetRuntime()->GetThreadContext()->GetRecycler();
            recycler->RootRelease(ref, count);
            return JsNoError;
        });
    }
    else
    {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if (threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }
        Recycler * recycler = threadContext->GetRecycler();
        return GlobalAPIWrapper([&]() -> JsErrorCode
        {
            // Note, some references may live in arena-allocated memory, so we need to do this check
            if (!recycler->IsValidObject(ref))
            {
                return JsNoError;
            }

#if ENABLE_TTD
            unsigned int lCount = 0;
            recycler->RootRelease(ref, &lCount);
            if (count != nullptr)
            {
                *count = lCount;
            }

            //At shutdown we may get called when Context is destroyed so don't try and access it and don't do any TTD stuff
            if (JsrtContext::GetCurrent() != nullptr)
            {
                Js::ScriptContext* scriptContext = JsrtContext::GetCurrent()->GetScriptContext();
                if ((lCount == 0) & scriptContext->ShouldPerformRootAddOrRemoveRefAction())
                {
                    if (!scriptContext->GetThreadContext()->TTDLog->IsPropertyRecordRef(ref))
                    {
                        Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(ref);
                        scriptContext->TTDContextInfo->RemoveTrackedRoot(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(obj), obj);

                        if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
                        {
                            scriptContext->GetThreadContext()->TTDLog->RecordJsRTRemoveRootRef(scriptContext, (Js::Var)ref);
                        }
                    }
                }
            }
#else
            recycler->RootRelease(ref, count);
#endif

            return JsNoError;
        });
    }
}


CHAKRA_API JsSetObjectBeforeCollectCallback(_In_ JsRef ref, _In_opt_ void *callbackState, _In_ JsObjectBeforeCollectCallback objectBeforeCollectCallback)
{
    VALIDATE_JSREF(ref);

    if (Js::TaggedNumber::Is(ref))
    {
        return JsErrorInvalidArgument;
    }

    if (JsrtContext::Is(ref))
    {
        return GlobalAPIWrapper([&]() -> JsErrorCode
        {
            ThreadContext* threadContext = static_cast<JsrtContext *>(ref)->GetRuntime()->GetThreadContext();
            Recycler * recycler = threadContext->GetRecycler();
            recycler->SetObjectBeforeCollectCallback(ref, reinterpret_cast<Recycler::ObjectBeforeCollectCallback>(objectBeforeCollectCallback), callbackState,
                reinterpret_cast<Recycler::ObjectBeforeCollectCallbackWrapper>(JsrtCallbackState::ObjectBeforeCallectCallbackWrapper), threadContext);
            return JsNoError;
        });
    }
    else
    {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if (threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }
        Recycler * recycler = threadContext->GetRecycler();
        return GlobalAPIWrapper([&]() -> JsErrorCode
        {
            if (!recycler->IsValidObject(ref))
            {
                return JsErrorInvalidArgument;
            }

            recycler->SetObjectBeforeCollectCallback(ref, reinterpret_cast<Recycler::ObjectBeforeCollectCallback>(objectBeforeCollectCallback), callbackState,
                reinterpret_cast<Recycler::ObjectBeforeCollectCallbackWrapper>(JsrtCallbackState::ObjectBeforeCallectCallbackWrapper), threadContext);
            return JsNoError;
        });
    }
}

CHAKRA_API JsCreateContext(_In_ JsRuntimeHandle runtimeHandle, _Out_ JsContextRef *newContext)
{
    return CreateContextCore(runtimeHandle, false /*createUnderTimeTravel*/, newContext);
}

CHAKRA_API JsGetCurrentContext(_Out_ JsContextRef *currentContext)
{
    PARAM_NOT_NULL(currentContext);

    BEGIN_JSRT_NO_EXCEPTION
    {
      *currentContext = (JsContextRef)JsrtContext::GetCurrent();
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsSetCurrentContext(_In_ JsContextRef newContext)
{
    VALIDATE_ENTER_CURRENT_THREAD();

    return GlobalAPIWrapper([&] () -> JsErrorCode {
        JsrtContext *currentContext = JsrtContext::GetCurrent();
        Recycler* recycler = currentContext != nullptr ? currentContext->GetScriptContext()->GetRecycler() : nullptr;

        if (currentContext && recycler->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (currentContext && currentContext->GetRuntime()->GetThreadContext()->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        if (!JsrtContext::TrySetCurrent((JsrtContext *)newContext))
        {
            return JsErrorWrongThread;
        }

        return JsNoError;
    });
}

CHAKRA_API JsGetContextOfObject(_In_ JsValueRef object, _Out_ JsContextRef *context)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(context);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::RecyclableObject::Is(object))
        {
            RETURN_NO_EXCEPTION(JsErrorArgumentNotObject);
        }
        Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(object);
        *context = (JsContextRef)obj->GetScriptContext()->GetLibrary()->GetPinnedJsrtContextObject();
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetContextData(_In_ JsContextRef context, _Out_ void **data)
{
    VALIDATE_JSREF(context);
    PARAM_NOT_NULL(data);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!JsrtContext::Is(context))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        *data = static_cast<JsrtContext *>(context)->GetExternalData();
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsSetContextData(_In_ JsContextRef context, _In_ void *data)
{
    VALIDATE_JSREF(context);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!JsrtContext::Is(context))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        static_cast<JsrtContext *>(context)->SetExternalData(data);
    }
    END_JSRT_NO_EXCEPTION
}

void HandleScriptCompileError(Js::ScriptContext * scriptContext, CompileScriptException * se)
{
    HRESULT hr = se->ei.scode;
    if (hr == E_OUTOFMEMORY || hr == VBSERR_OutOfMemory || hr == VBSERR_OutOfStack || hr == ERRnoMemory)
    {
        Js::Throw::OutOfMemory();
    }

    PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

    Js::JavascriptError* error = Js::JavascriptError::CreateFromCompileScriptException(scriptContext, se);

    Js::JavascriptExceptionObject * exceptionObject = RecyclerNew(scriptContext->GetRecycler(),
        Js::JavascriptExceptionObject, error, scriptContext, nullptr);

    scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
}

CHAKRA_API JsGetUndefinedValue(_Out_ JsValueRef *undefinedValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(undefinedValue);

        *undefinedValue = scriptContext->GetLibrary()->GetUndefined();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetNullValue(_Out_ JsValueRef *nullValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(nullValue);

        *nullValue = scriptContext->GetLibrary()->GetNull();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetTrueValue(_Out_ JsValueRef *trueValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(trueValue);

        *trueValue = scriptContext->GetLibrary()->GetTrue();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetFalseValue(_Out_ JsValueRef *falseValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(falseValue);

        *falseValue = scriptContext->GetLibrary()->GetFalse();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsBoolToBoolean(_In_ bool value, _Out_ JsValueRef *booleanValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(booleanValue);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateBoolean(scriptContext, value, &__ttd_resultPtr));

        *booleanValue = value ? scriptContext->GetLibrary()->GetTrue() : scriptContext->GetLibrary()->GetFalse();

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(booleanValue);

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsBooleanToBool(_In_ JsValueRef value, _Out_ bool *boolValue)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(boolValue);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::JavascriptBoolean::Is(value))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        *boolValue = Js::JavascriptBoolean::FromVar(value)->GetValue() ? true : false;
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsConvertValueToBoolean(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTVarToBooleanConversion(scriptContext, (Js::Var)value, &__ttd_resultPtr));

        if (Js::JavascriptConversion::ToBool((Js::Var)value, scriptContext))
        {
            *result = scriptContext->GetLibrary()->GetTrue();
        }
        else
        {
            *result = scriptContext->GetLibrary()->GetFalse();
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsGetValueType(_In_ JsValueRef value, _Out_ JsValueType *type)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(type);

    BEGIN_JSRT_NO_EXCEPTION
    {
        Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(value);
        switch (typeId)
        {
        case Js::TypeIds_Undefined:
            *type = JsUndefined;
            break;
        case Js::TypeIds_Null:
            *type = JsNull;
            break;
        case Js::TypeIds_Boolean:
            *type = JsBoolean;
            break;
        case Js::TypeIds_Integer:
        case Js::TypeIds_Number:
        case Js::TypeIds_Int64Number:
        case Js::TypeIds_UInt64Number:
            *type = JsNumber;
            break;
        case Js::TypeIds_String:
            *type = JsString;
            break;
        case Js::TypeIds_Function:
            *type = JsFunction;
            break;
        case Js::TypeIds_Error:
            *type = JsError;
            break;
        case Js::TypeIds_Array:
        case Js::TypeIds_NativeIntArray:
#if ENABLE_COPYONACCESS_ARRAY
        case Js::TypeIds_CopyOnAccessNativeIntArray:
#endif
        case Js::TypeIds_NativeFloatArray:
        case Js::TypeIds_ES5Array:
            *type = JsArray;
            break;
        case Js::TypeIds_Symbol:
            *type = JsSymbol;
            break;
        case Js::TypeIds_ArrayBuffer:
            *type = JsArrayBuffer;
            break;
        case Js::TypeIds_DataView:
            *type = JsDataView;
            break;
        default:
            if (Js::TypedArrayBase::Is(typeId))
            {
                *type = JsTypedArray;
            }
            else
            {
                *type = JsObject;
            }
            break;
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsDoubleToNumber(_In_ double dbl, _Out_ JsValueRef *asValue)
{
    PARAM_NOT_NULL(asValue);
    if (Js::JavascriptNumber::TryToVarFastWithCheck(dbl, asValue)) {
      return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateNumber(scriptContext, dbl, &__ttd_resultPtr));

        *asValue = Js::JavascriptNumber::ToVarNoCheck(dbl, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(asValue);

        return JsNoError;
    });
}

CHAKRA_API JsIntToNumber(_In_ int intValue, _Out_ JsValueRef *asValue)
{
    PARAM_NOT_NULL(asValue);
    if (Js::JavascriptNumber::TryToVarFast(intValue, asValue))
    {
        return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

#if !INT32VAR
        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateInteger(scriptContext, intValue, &__ttd_resultPtr));
#endif

        *asValue = Js::JavascriptNumber::ToVar(intValue, scriptContext);

#if !INT32VAR
        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(asValue);
#endif

        return JsNoError;
    });
}

CHAKRA_API JsNumberToDouble(_In_ JsValueRef value, _Out_ double *asDouble)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(asDouble);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (Js::TaggedInt::Is(value))
        {
            *asDouble = Js::TaggedInt::ToDouble(value);
        }
        else if (Js::JavascriptNumber::Is_NoTaggedIntCheck(value))
        {
            *asDouble = Js::JavascriptNumber::GetValue(value);
        }
        else
        {
            *asDouble = 0;
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsNumberToInt(_In_ JsValueRef value, _Out_ int *asInt)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(asInt);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (Js::TaggedInt::Is(value))
        {
            *asInt = Js::TaggedInt::ToInt32(value);
        }
        else if (Js::JavascriptNumber::Is_NoTaggedIntCheck(value))
        {
            *asInt = Js::JavascriptConversion::ToInt32(Js::JavascriptNumber::GetValue(value));
        }
        else
        {
            *asInt = 0;
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsConvertValueToNumber(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTVarToNumberConversion(scriptContext, (Js::Var)value, &__ttd_resultPtr));

        *result = (JsValueRef)Js::JavascriptOperators::ToNumber((Js::Var)value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsGetStringLength(_In_ JsValueRef value, _Out_ int *length)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(length);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::JavascriptString::Is(value))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        *length = Js::JavascriptString::FromVar(value)->GetLengthAsSignedInt();
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsPointerToString(_In_reads_(stringLength) const wchar_t *stringValue, _In_ size_t stringLength, _Out_ JsValueRef *string)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(stringValue);
        PARAM_NOT_NULL(string);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateString(scriptContext, stringValue, stringLength, &__ttd_resultPtr));

        if (!Js::IsValidCharCount(stringLength))
        {
            Js::JavascriptError::ThrowOutOfMemoryError(scriptContext);
        }

        *string = Js::JavascriptString::NewCopyBuffer(stringValue, static_cast<charcount_t>(stringLength), scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(string);

        return JsNoError;
    });
}

CHAKRA_API JsPointerToStringUtf8(_In_reads_(stringLength) const char *stringValue, _In_ size_t stringLength, _Out_ JsValueRef *string)
{
    PARAM_NOT_NULL(stringValue);
    utf8::NarrowToWide wstr(stringValue, stringLength);
    if (!wstr)
    {
        return JsErrorOutOfMemory;
    }

    return JsPointerToString(wstr, wstr.Length(), string);
}

// TODO: The annotation of stringPtr is wrong.  Need to fix definition in chakrart.h
// The warning is '*stringPtr' could be '0' : this does not adhere to the specification for the function 'JsStringToPointer'.
#pragma warning(suppress:6387)
CHAKRA_API JsStringToPointer(_In_ JsValueRef stringValue, _Outptr_result_buffer_(*stringLength) const wchar_t **stringPtr, _Out_ size_t *stringLength)
{
    VALIDATE_JSREF(stringValue);
    PARAM_NOT_NULL(stringPtr);
    *stringPtr = nullptr;
    PARAM_NOT_NULL(stringLength);
    *stringLength = 0;

    if (!Js::JavascriptString::Is(stringValue))
    {
        return JsErrorInvalidArgument;
    }

    return GlobalAPIWrapper([&]() -> JsErrorCode {
        Js::JavascriptString *jsString = Js::JavascriptString::FromVar(stringValue);

        *stringPtr = jsString->GetSz();
        *stringLength = jsString->GetLength();
        return JsNoError;
    });
}

CHAKRA_API JsStringToPointerUtf8Copy(_In_ JsValueRef stringValue, _Outptr_result_buffer_(*stringLength) char **stringPtr, _Out_ size_t *stringLength)
{
    const wchar_t* wstr;
    size_t wstrLen;
    JsErrorCode err = JsStringToPointer(stringValue, &wstr, &wstrLen);
    if (err == JsNoError)
    {
        PARAM_NOT_NULL(stringPtr);
        PARAM_NOT_NULL(stringLength);
        *stringPtr = nullptr;
        *stringLength = 0;

        // xplat-todo: fix encoding. The result is cesu8.
        utf8::WideToNarrow str(wstr, wstrLen);
        if (str)
        {
            *stringPtr = str.Detach();
            *stringLength = str.Length();
        }
        else
        {
            err = JsErrorOutOfMemory;
        }
    }

    return err;
}

CHAKRA_API JsConvertValueToString(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTVarToStringConversion(scriptContext, (Js::Var)value, &__ttd_resultPtr));

        *result = (JsValueRef) Js::JavascriptConversion::ToString((Js::Var)value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsGetGlobalObject(_Out_ JsValueRef *globalObject)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(globalObject);

        *globalObject = (JsValueRef)scriptContext->GetGlobalObject();
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsCreateObject(_Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(object);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateBasicObject(scriptContext, &__ttd_resultPtr));

        *object = scriptContext->GetLibrary()->CreateObject();

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(object);

        return JsNoError;
    });
}

CHAKRA_API JsCreateExternalObject(_In_opt_ void *data, _In_opt_ JsFinalizeCallback finalizeCallback, _Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(object);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateExternalObject(scriptContext, &__ttd_resultPtr));

        *object = RecyclerNewFinalized(scriptContext->GetRecycler(), JsrtExternalObject, RecyclerNew(scriptContext->GetRecycler(), JsrtExternalType, scriptContext, finalizeCallback), data);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(object);

        return JsNoError;
    });
}

CHAKRA_API JsConvertValueToObject(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTVarToObjectConversion(scriptContext, (Js::Var)value, &__ttd_resultPtr));

        *result = (JsValueRef)Js::JavascriptOperators::ToObject((Js::Var)value, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsGetPrototype(_In_ JsValueRef object, _Out_ JsValueRef *prototypeObject)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(prototypeObject);

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        *prototypeObject = (JsValueRef)Js::JavascriptOperators::OP_GetPrototype(object, scriptContext);
        Assert(*prototypeObject == nullptr || !Js::CrossSite::NeedMarshalVar(*prototypeObject, scriptContext));

        return JsNoError;
    });
}

CHAKRA_API JsSetPrototype(_In_ JsValueRef object, _In_ JsValueRef prototypeObject)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_OBJECT_OR_NULL(prototypeObject, scriptContext);

        // We're not allowed to set this.
        if (object == scriptContext->GetLibrary()->GetObjectPrototype())
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTSetPrototype(scriptContext, object, prototypeObject));

        Js::JavascriptObject::ChangePrototype(Js::RecyclableObject::FromVar(object), Js::RecyclableObject::FromVar(prototypeObject), true, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsInstanceOf(_In_ JsValueRef object, _In_ JsValueRef constructor, _Out_ bool *result) {
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(constructor, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::RecyclableObject::FromVar(constructor)->HasInstance(object, scriptContext) ? true : false;

        return JsNoError;
    });
}

CHAKRA_API JsGetExtensionAllowed(_In_ JsValueRef object, _Out_ bool *value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(value);
        *value = nullptr;

        *value = Js::RecyclableObject::FromVar(object)->IsExtensible() != 0;

        return JsNoError;
    });
}

CHAKRA_API JsPreventExtension(_In_ JsValueRef object)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        Js::RecyclableObject::FromVar(object)->PreventExtensions();

        return JsNoError;
    });
}

CHAKRA_API JsGetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(value);
        *value = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetProperty(scriptContext, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), object, &__ttd_resultPtr));

        *value = Js::JavascriptOperators::OP_GetProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext);
        Assert(*value == nullptr || !Js::CrossSite::NeedMarshalVar(*value, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(value);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertyDescriptor(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *propertyDescriptor)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(propertyDescriptor);
        *propertyDescriptor = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetOwnPropertyInfo(scriptContext, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), object, &__ttd_resultPtr));

        Js::PropertyDescriptor propertyDescriptorValue;
        if (Js::JavascriptOperators::GetOwnPropertyDescriptor(Js::RecyclableObject::FromVar(object), ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext, &propertyDescriptorValue))
        {
            *propertyDescriptor = Js::JavascriptOperators::FromPropertyDescriptor(propertyDescriptorValue, scriptContext);
        }
        else
        {
            *propertyDescriptor = scriptContext->GetLibrary()->GetUndefined();
        }
        Assert(*propertyDescriptor == nullptr || !Js::CrossSite::NeedMarshalVar(*propertyDescriptor, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(propertyDescriptor);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertyNames(_In_ JsValueRef object, _Out_ JsValueRef *propertyNames)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertyNames);
        *propertyNames = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetOwnPropertyNamesInfo(scriptContext, object, &__ttd_resultPtr));

        *propertyNames = Js::JavascriptOperators::GetOwnPropertyNames(object, scriptContext);
        Assert(*propertyNames == nullptr || !Js::CrossSite::NeedMarshalVar(*propertyNames, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(propertyNames);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertySymbols(_In_ JsValueRef object, _Out_ JsValueRef *propertySymbols)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertySymbols);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetOwnPropertySymbolsInfo(scriptContext, object, &__ttd_resultPtr));

        *propertySymbols = Js::JavascriptOperators::GetOwnPropertySymbols(object, scriptContext);
        Assert(*propertySymbols == nullptr || !Js::CrossSite::NeedMarshalVar(*propertySymbols, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(propertySymbols);

        return JsNoError;
    });
}

CHAKRA_API JsSetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef value, _In_ bool useStrictRules)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTSetProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), value, useStrictRules));

        Js::JavascriptOperators::OP_SetProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), value, scriptContext,
            nullptr, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);

        return JsNoError;
    });
}

CHAKRA_API JsHasProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ bool *hasProperty)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(hasProperty);
        *hasProperty = nullptr;

        *hasProperty = Js::JavascriptOperators::OP_HasProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsDeleteProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ bool useStrictRules, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTDeleteProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), useStrictRules, &__ttd_resultPtr));

        *result = Js::JavascriptOperators::OP_DeleteProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(),
            scriptContext, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsDefineProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef propertyDescriptor, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        VALIDATE_INCOMING_OBJECT(propertyDescriptor, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        Js::PropertyDescriptor propertyDescriptorValue;
        if (!Js::JavascriptOperators::ToPropertyDescriptor(propertyDescriptor, &propertyDescriptorValue, scriptContext))
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTDefineProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), propertyDescriptor));

        *result = Js::JavascriptOperators::DefineOwnPropertyDescriptor(
            Js::RecyclableObject::FromVar(object), ((Js::PropertyRecord *)propertyId)->GetPropertyId(), propertyDescriptorValue,
            true, scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsCreateArray(_In_ unsigned int length, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);
        *result = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateBasicArray(scriptContext, length, &__ttd_resultPtr));

        *result = scriptContext->GetLibrary()->CreateArray(length);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsCreateArrayBuffer(_In_ unsigned int byteLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateArrayBuffer(scriptContext, byteLength, &__ttd_resultPtr));

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateArrayBuffer(byteLength);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsCreateExternalArrayBuffer(_Pre_maybenull_ _Pre_writable_byte_size_(byteLength) void *data, _In_ unsigned int byteLength,
    _In_opt_ JsFinalizeCallback finalizeCallback, _In_opt_ void *callbackState, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        if (data == nullptr && byteLength > 0)
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateExternalArrayBuffer(scriptContext, reinterpret_cast<BYTE*>(data), byteLength, &__ttd_resultPtr));

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = Js::JsrtExternalArrayBuffer::New(
            reinterpret_cast<BYTE*>(data),
            byteLength,
            finalizeCallback,
            callbackState,
            library->GetArrayBufferType());

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsCreateTypedArray(_In_ JsTypedArrayType arrayType, _In_ JsValueRef baseArray, _In_ unsigned int byteOffset,
    _In_ unsigned int elementLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if (baseArray != JS_INVALID_REFERENCE)
        {
            VALIDATE_INCOMING_REFERENCE(baseArray, scriptContext);
        }
        PARAM_NOT_NULL(result);

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();

        const bool fromArrayBuffer = (baseArray != JS_INVALID_REFERENCE && Js::ArrayBuffer::Is(baseArray));

        if (byteOffset != 0 && !fromArrayBuffer)
        {
            return JsErrorInvalidArgument;
        }

        if (elementLength != 0 && !(baseArray == JS_INVALID_REFERENCE || fromArrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        Js::JavascriptFunction* constructorFunc = nullptr;
        Js::Var values[4] =
        {
            library->GetUndefined(),
            baseArray != nullptr ? baseArray : Js::JavascriptNumber::ToVar(elementLength, scriptContext)
        };
        if (fromArrayBuffer)
        {
            values[2] = Js::JavascriptNumber::ToVar(byteOffset, scriptContext);
            values[3] = Js::JavascriptNumber::ToVar(elementLength, scriptContext);
        }

        Js::CallInfo info(Js::CallFlags_New, fromArrayBuffer ? 4 : 2);
        Js::Arguments args(info, values);

        switch (arrayType)
        {
        case JsArrayTypeInt8:
            constructorFunc = library->GetInt8ArrayConstructor();
            break;
        case JsArrayTypeUint8:
            constructorFunc = library->GetUint8ArrayConstructor();
            break;
        case JsArrayTypeUint8Clamped:
            constructorFunc = library->GetUint8ClampedArrayConstructor();
            break;
        case JsArrayTypeInt16:
            constructorFunc = library->GetInt16ArrayConstructor();
            break;
        case JsArrayTypeUint16:
            constructorFunc = library->GetUint16ArrayConstructor();
            break;
        case JsArrayTypeInt32:
            constructorFunc = library->GetInt32ArrayConstructor();
            break;
        case JsArrayTypeUint32:
            constructorFunc = library->GetUint32ArrayConstructor();
            break;
        case JsArrayTypeFloat32:
            constructorFunc = library->GetFloat32ArrayConstructor();
            break;
        case JsArrayTypeFloat64:
            constructorFunc = library->GetFloat64ArrayConstructor();
            break;
        default:
            return JsErrorInvalidArgument;
        }

        *result = Js::JavascriptFunction::CallAsConstructor(constructorFunc, /* overridingNewTarget = */nullptr, args, scriptContext);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsCreateDataView(_In_ JsValueRef arrayBuffer, _In_ unsigned int byteOffset, _In_ unsigned int byteLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(arrayBuffer, scriptContext);
        PARAM_NOT_NULL(result);

        if (!Js::ArrayBuffer::Is(arrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateDataView(Js::ArrayBuffer::FromVar(arrayBuffer), byteOffset, byteLength);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}


C_ASSERT(JsArrayTypeUint8         - Js::TypeIds_Uint8Array        == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeUint8Clamped  - Js::TypeIds_Uint8ClampedArray == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeInt16         - Js::TypeIds_Int16Array        == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeUint16        - Js::TypeIds_Uint16Array       == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeInt32         - Js::TypeIds_Int32Array        == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeUint32        - Js::TypeIds_Uint32Array       == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeFloat32       - Js::TypeIds_Float32Array      == JsArrayTypeInt8 - Js::TypeIds_Int8Array);
C_ASSERT(JsArrayTypeFloat64       - Js::TypeIds_Float64Array      == JsArrayTypeInt8 - Js::TypeIds_Int8Array);

inline JsTypedArrayType GetTypedArrayType(Js::TypeId typeId)
{
    Assert(Js::TypedArrayBase::Is(typeId));
    return static_cast<JsTypedArrayType>(typeId + (JsArrayTypeInt8 - Js::TypeIds_Int8Array));
}

CHAKRA_API JsGetTypedArrayInfo(_In_ JsValueRef typedArray, _Out_opt_ JsTypedArrayType *arrayType, _Out_opt_ JsValueRef *arrayBuffer,
    _Out_opt_ unsigned int *byteOffset, _Out_opt_ unsigned int *byteLength)
{
    VALIDATE_JSREF(typedArray);

    BEGIN_JSRT_NO_EXCEPTION
    {
        const Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(typedArray);

        if (!Js::TypedArrayBase::Is(typeId))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        if (arrayType != nullptr) {
            *arrayType = GetTypedArrayType(typeId);
        }

        Js::TypedArrayBase* typedArrayBase = Js::TypedArrayBase::FromVar(typedArray);
        if (arrayBuffer != nullptr) {
            *arrayBuffer = typedArrayBase->GetArrayBuffer();
        }

        if (byteOffset != nullptr) {
            *byteOffset = typedArrayBase->GetByteOffset();
        }

        if (byteLength != nullptr) {
            *byteLength = typedArrayBase->GetByteLength();
        }
    }

#if ENABLE_TTD
    Js::ScriptContext* scriptContext = JsrtContext::GetCurrent()->GetScriptContext();
    if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
    {
        if (arrayBuffer != nullptr)
        {
            BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
            {
                PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetTypedArrayInfo(scriptContext, typedArray, &__ttd_resultPtr));
                PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(arrayBuffer);
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }
    }
#endif

    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetArrayBufferStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer,
    _Out_ unsigned int *bufferLength)
{
    VALIDATE_JSREF(instance);
    PARAM_NOT_NULL(buffer);
    PARAM_NOT_NULL(bufferLength);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::ArrayBuffer::Is(instance))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        Js::ArrayBuffer* arrayBuffer = Js::ArrayBuffer::FromVar(instance);
        *buffer = arrayBuffer->GetBuffer();
        *bufferLength = arrayBuffer->GetByteLength();
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetTypedArrayStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer,
    _Out_ unsigned int *bufferLength, _Out_opt_ JsTypedArrayType *typedArrayType, _Out_opt_ int *elementSize)
{
    VALIDATE_JSREF(instance);
    PARAM_NOT_NULL(buffer);
    PARAM_NOT_NULL(bufferLength);

    BEGIN_JSRT_NO_EXCEPTION
    {
        const Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(instance);
        if (!Js::TypedArrayBase::Is(typeId))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        Js::TypedArrayBase* typedArrayBase = Js::TypedArrayBase::FromVar(instance);
        *buffer = typedArrayBase->GetByteBuffer();
        *bufferLength = typedArrayBase->GetByteLength();

        if (typedArrayType)
        {
            *typedArrayType = GetTypedArrayType(typeId);
        }

        if (elementSize)
        {
            switch (typeId)
            {
                case Js::TypeIds_Int8Array:
                    *elementSize = sizeof(int8);
                    break;
                case Js::TypeIds_Uint8Array:
                    *elementSize = sizeof(uint8);
                    break;
                case Js::TypeIds_Uint8ClampedArray:
                    *elementSize = sizeof(uint8);
                    break;
                case Js::TypeIds_Int16Array:
                    *elementSize = sizeof(int16);
                    break;
                case Js::TypeIds_Uint16Array:
                    *elementSize = sizeof(uint16);
                    break;
                case Js::TypeIds_Int32Array:
                    *elementSize = sizeof(int32);
                    break;
                case Js::TypeIds_Uint32Array:
                    *elementSize = sizeof(uint32);
                    break;
                case Js::TypeIds_Float32Array:
                    *elementSize = sizeof(float);
                    break;
                case Js::TypeIds_Float64Array:
                    *elementSize = sizeof(double);
                    break;
                default:
                    AssertMsg(FALSE, "invalid typed array type");
                    *elementSize = 1;
                    RETURN_NO_EXCEPTION(JsErrorFatal);
            }
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetDataViewStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer, _Out_ unsigned int *bufferLength)
{
    VALIDATE_JSREF(instance);
    PARAM_NOT_NULL(buffer);
    PARAM_NOT_NULL(bufferLength);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::DataView::Is(instance))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        Js::DataView* dataView = Js::DataView::FromVar(instance);
        *buffer = dataView->GetArrayBuffer()->GetBuffer() + dataView->GetByteOffset();
        *bufferLength = dataView->GetLength();
    }
    END_JSRT_NO_EXCEPTION
}


CHAKRA_API JsCreateSymbol(_In_ JsValueRef description, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);
        *result = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateSymbol(scriptContext, description, &__ttd_resultPtr));

        Js::JavascriptString* descriptionString;

        if (description != JS_INVALID_REFERENCE)
        {
            VALIDATE_INCOMING_REFERENCE(description, scriptContext);
            descriptionString = Js::JavascriptConversion::ToString(description, scriptContext);
        }
        else
        {
            descriptionString = scriptContext->GetLibrary()->GetEmptyString();
        }

        *result = scriptContext->GetLibrary()->CreateSymbol(descriptionString);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsHasIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        PARAM_NOT_NULL(result);
        *result = false;

        *result = Js::JavascriptOperators::OP_HasItem((Js::Var)object, (Js::Var)index, scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsGetIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetIndex(scriptContext, index, object, &__ttd_resultPtr));

        *result = (JsValueRef)Js::JavascriptOperators::OP_GetElementI((Js::Var)object, (Js::Var)index, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsSetIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _In_ JsValueRef value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_NORESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTSetIndex(scriptContext, object, index, value));

        Js::JavascriptOperators::OP_SetElementI((Js::Var)object, (Js::Var)index, (Js::Var)value, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsDeleteIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        Js::JavascriptOperators::OP_DeleteElementI((Js::Var)object, (Js::Var)index, scriptContext);

        return JsNoError;
    });
}

template <class T, bool clamped = false> struct TypedArrayTypeTraits { static const JsTypedArrayType cTypedArrayType; };
template<> struct TypedArrayTypeTraits<int8> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeInt8; };
template<> struct TypedArrayTypeTraits<uint8, false> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeUint8; };
template<> struct TypedArrayTypeTraits<uint8, true> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeUint8Clamped; };
template<> struct TypedArrayTypeTraits<int16> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeInt16; };
template<> struct TypedArrayTypeTraits<uint16> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeUint16; };
template<> struct TypedArrayTypeTraits<int32> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeInt32; };
template<> struct TypedArrayTypeTraits<uint32> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeUint32; };
template<> struct TypedArrayTypeTraits<float> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeFloat32; };
template<> struct TypedArrayTypeTraits<double> { static const JsTypedArrayType cTypedArrayType = JsTypedArrayType::JsArrayTypeFloat64; };

template <class T, bool clamped = false>
Js::ArrayObject* CreateTypedArray(Js::ScriptContext *scriptContext, void* data, unsigned int length)
{
    Js::JavascriptLibrary* library = scriptContext->GetLibrary();

    Js::ArrayBufferBase* arrayBuffer = RecyclerNew(
        scriptContext->GetRecycler(),
        Js::ExternalArrayBuffer,
        reinterpret_cast<BYTE*>(data),
        length * sizeof(T),
        library->GetArrayBufferType());

    return static_cast<Js::ArrayObject*>(Js::TypedArray<T, clamped>::Create(arrayBuffer, 0, length, library));
}

template <class T, bool clamped = false>
void GetObjectArrayData(Js::ArrayObject* objectArray, void** data, JsTypedArrayType* arrayType, uint* length)
{
    Js::TypedArray<T, clamped>* typedArray = Js::TypedArray<T, clamped>::FromVar(objectArray);
    *data = typedArray->GetArrayBuffer()->GetBuffer();
    *arrayType = TypedArrayTypeTraits<T, clamped>::cTypedArrayType;
    *length = typedArray->GetLength();
}

CHAKRA_API JsSetIndexedPropertiesToExternalData(
    _In_ JsValueRef object,
    _In_ void* data,
    _In_ JsTypedArrayType arrayType,
    _In_ unsigned int elementLength)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);

        // Don't support doing this on array or array-like object
        Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(object);
        if (!Js::DynamicType::Is(typeId)
            || Js::DynamicObject::IsAnyArrayTypeId(typeId)
            || (typeId >= Js::TypeIds_TypedArrayMin && typeId <= Js::TypeIds_TypedArrayMax)
            || typeId == Js::TypeIds_ArrayBuffer
            || typeId == Js::TypeIds_DataView
            || Js::RecyclableObject::FromVar(object)->IsExternal()
            )
        {
            return JsErrorInvalidArgument;
        }

        if (data == nullptr && elementLength > 0)
        {
            return JsErrorInvalidArgument;
        }

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        Js::ArrayObject* newTypedArray = nullptr;
        switch (arrayType)
        {
        case JsArrayTypeInt8:
            newTypedArray = CreateTypedArray<int8>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeUint8:
            newTypedArray = CreateTypedArray<uint8>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeUint8Clamped:
            newTypedArray = CreateTypedArray<uint8, true>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeInt16:
            newTypedArray = CreateTypedArray<int16>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeUint16:
            newTypedArray = CreateTypedArray<uint16>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeInt32:
            newTypedArray = CreateTypedArray<int32>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeUint32:
            newTypedArray = CreateTypedArray<uint32>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeFloat32:
            newTypedArray = CreateTypedArray<float>(scriptContext, data, elementLength);
            break;
        case JsArrayTypeFloat64:
            newTypedArray = CreateTypedArray<double>(scriptContext, data, elementLength);
            break;
        default:
            return JsErrorInvalidArgument;
        }

        Js::DynamicObject* dynamicObject = Js::DynamicObject::FromVar(object);
        dynamicObject->SetObjectArray(newTypedArray);

        return JsNoError;
    });
}

CHAKRA_API JsHasIndexedPropertiesExternalData(_In_ JsValueRef object, _Out_ bool *value)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(value);

    BEGIN_JSRT_NO_EXCEPTION
    {
        *value = false;

        if (Js::DynamicType::Is(Js::JavascriptOperators::GetTypeId(object)))
        {
            Js::DynamicObject* dynamicObject = Js::DynamicObject::FromVar(object);
            Js::ArrayObject* objectArray = dynamicObject->GetObjectArray();
            *value = (objectArray && !Js::DynamicObject::IsAnyArray(objectArray));
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetIndexedPropertiesExternalData(
    _In_ JsValueRef object,
    _Out_ void** buffer,
    _Out_ JsTypedArrayType* arrayType,
    _Out_ unsigned int* elementLength)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(buffer);
    PARAM_NOT_NULL(arrayType);
    PARAM_NOT_NULL(elementLength);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::DynamicType::Is(Js::JavascriptOperators::GetTypeId(object)))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        *buffer = nullptr;
        *arrayType = JsTypedArrayType();
        *elementLength = 0;

        Js::DynamicObject* dynamicObject = Js::DynamicObject::FromVar(object);
        Js::ArrayObject* objectArray = dynamicObject->GetObjectArray();
        if (!objectArray)
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        switch (Js::JavascriptOperators::GetTypeId(objectArray))
        {
        case Js::TypeIds_Int8Array:
            GetObjectArrayData<int8>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Uint8Array:
            GetObjectArrayData<uint8>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Uint8ClampedArray:
            GetObjectArrayData<uint8, true>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Int16Array:
            GetObjectArrayData<int16>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Uint16Array:
            GetObjectArrayData<uint16>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Int32Array:
            GetObjectArrayData<int32>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Uint32Array:
            GetObjectArrayData<uint32>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Float32Array:
            GetObjectArrayData<float>(objectArray, buffer, arrayType, elementLength);
            break;
        case Js::TypeIds_Float64Array:
            GetObjectArrayData<double>(objectArray, buffer, arrayType, elementLength);
            break;
        default:
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsEquals(_In_ JsValueRef object1, _In_ JsValueRef object2, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object1, scriptContext);
        VALIDATE_INCOMING_REFERENCE(object2, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::JavascriptOperators::Equal((Js::Var)object1, (Js::Var)object2, scriptContext) != 0;
        return JsNoError;
    });
}

CHAKRA_API JsStrictEquals(_In_ JsValueRef object1, _In_ JsValueRef object2, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object1, scriptContext);
        VALIDATE_INCOMING_REFERENCE(object2, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::JavascriptOperators::StrictEqual((Js::Var)object1, (Js::Var)object2, scriptContext) != 0;
        return JsNoError;
    });
}

CHAKRA_API JsHasExternalData(_In_ JsValueRef object, _Out_ bool *value)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(value);

    BEGIN_JSRT_NO_EXCEPTION
    {
        *value = JsrtExternalObject::Is(object);
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsGetExternalData(_In_ JsValueRef object, _Out_ void **data)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(data);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (JsrtExternalObject::Is(object))
        {
            *data = JsrtExternalObject::FromVar(object)->GetSlotData();
        }
        else
        {
            *data = nullptr;
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsSetExternalData(_In_ JsValueRef object, _In_opt_ void *data)
{
    VALIDATE_JSREF(object);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (JsrtExternalObject::Is(object))
        {
            JsrtExternalObject::FromVar(object)->SetSlotData(data);
        }
        else
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }
    }
    END_JSRT_NO_EXCEPTION
}

CHAKRA_API JsCallFunction(_In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_opt_ JsValueRef *result)
{
    if(result != nullptr)
    {
        *result = nullptr;
    }

    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_FUNCTION(function, scriptContext);

        if(cargs == 0 || args == nullptr) 
        {
            return JsErrorInvalidArgument;
        }

        for(int index = 0; index < cargs; index++)
        {
            VALIDATE_INCOMING_REFERENCE(args[index], scriptContext);
        }

        Js::JavascriptFunction *jsFunction = Js::JavascriptFunction::FromVar(function);
        Js::CallInfo callInfo(cargs);
        Js::Arguments jsArgs(callInfo, reinterpret_cast<Js::Var *>(args));

#if ENABLE_TTD
        double ttdStartTime = -1.0;

        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            TTD::NSLogEvents::EventLogEntry* callEvent = scriptContext->GetThreadContext()->TTDLog->RecordJsRTCallFunction(scriptContext, scriptContext->TTDRootNestingCount, jsFunction, cargs, jsArgs.Values);
            TTD::TTDRecordJsRTFunctionCallActionPopper actionPopper(scriptContext, callEvent);

            if(scriptContext->TTDRootNestingCount == 0)
            {
                TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
                elog->ResetCallStackForTopLevelCall(elog->GetLastEventTime());

                ttdStartTime = elog->GetCurrentWallTime();
            }

            Js::Var varResult = jsFunction->CallRootFunction(jsArgs, scriptContext, true);
            if(result != nullptr)
            {
                *result = varResult;
                Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));
            }

            actionPopper.NormalReturn(result != nullptr ? *result : nullptr);
        }
        else
        {
            Js::Var varResult = jsFunction->CallRootFunction(jsArgs, scriptContext, true);
            if(result != nullptr)
            {
                *result = varResult;
                Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));
            }
        }

        //Update the time elapsed since a snapshot if needed
        if(ttdStartTime >= 0.0)
        {
            TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;

            double ttdEndTime = elog->GetCurrentWallTime();
            elog->IncrementElapsedSnapshotTime(ttdEndTime - ttdStartTime);
        }
#else
        Js::Var varResult = jsFunction->CallRootFunction(jsArgs, scriptContext, true);
        if(result != nullptr)
        {
            *result = varResult;
            Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));
        }
#endif

        return JsNoError;
    });
}

CHAKRA_API JsConstructObject(_In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_FUNCTION(function, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        if (cargs == 0 || args == nullptr)
        {
            return JsErrorInvalidArgument;
        }

        for (int index = 0; index < cargs; index++)
        {
            VALIDATE_INCOMING_REFERENCE(args[index], scriptContext);
        }

        Js::JavascriptFunction *jsFunction = Js::JavascriptFunction::FromVar(function);
        Js::CallInfo callInfo(Js::CallFlags::CallFlags_New, cargs);
        Js::Arguments jsArgs(callInfo, reinterpret_cast<Js::Var *>(args));

        //
        //TODO: we will want to look at this at some point -- either treat as "top-level" call or maybe constructors are fast so we can just jump back to previous "real" code
        //AssertMsg(!Js::ScriptFunction::Is(jsFunction) || execContext->TTDRootNestingCount != 0, "This will cause user code to execute and we need to add support for that as a top-level call source!!!!");
        //

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTConstructCall(scriptContext, jsFunction, cargs, args, &__ttd_resultPtr));

        *result = Js::JavascriptFunction::CallAsConstructor(jsFunction, /* overridingNewTarget = */nullptr, jsArgs, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(result);

        return JsNoError;
    });
}

CHAKRA_API JsCreateFunction(_In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateFunction(scriptContext, false, nullptr, &__ttd_resultPtr));

        Js::JavascriptExternalFunction *externalFunction = scriptContext->GetLibrary()->CreateStdCallExternalFunction((Js::StdCallJavascriptMethod)nativeFunction, 0, callbackState);
        *function = (JsValueRef)externalFunction;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(function);

        return JsNoError;
    });
}

CHAKRA_API JsCreateNamedFunction(_In_ JsValueRef name, _In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(name, scriptContext);
        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTAllocateFunction(scriptContext, true, name, &__ttd_resultPtr));

        if (name != JS_INVALID_REFERENCE)
        {
            name = Js::JavascriptConversion::ToString(name, scriptContext);
        }
        else
        {
            name = scriptContext->GetLibrary()->GetEmptyString();
        }

        Js::JavascriptExternalFunction *externalFunction = scriptContext->GetLibrary()->CreateStdCallExternalFunction((Js::StdCallJavascriptMethod)nativeFunction, Js::JavascriptString::FromVar(name), callbackState);
        *function = (JsValueRef)externalFunction;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(function);

        return JsNoError;
    });
}

void SetErrorMessage(Js::ScriptContext *scriptContext, JsValueRef newError, JsValueRef message)
{
    Js::JavascriptOperators::OP_SetProperty(newError, Js::PropertyIds::message, message, scriptContext);
}

CHAKRA_API JsCreateError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateError(scriptContext, message, &__ttd_resultPtr));

        JsValueRef newError = scriptContext->GetLibrary()->CreateError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateRangeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateRangeError(scriptContext, message, &__ttd_resultPtr));

        newError = scriptContext->GetLibrary()->CreateRangeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateReferenceError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateReferenceError(scriptContext, message, &__ttd_resultPtr));

        newError = scriptContext->GetLibrary()->CreateReferenceError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateSyntaxError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateSyntaxError(scriptContext, message, &__ttd_resultPtr));

        newError = scriptContext->GetLibrary()->CreateSyntaxError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateTypeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateTypeError(scriptContext, message, &__ttd_resultPtr));

        newError = scriptContext->GetLibrary()->CreateTypeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateURIError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTCreateURIError(scriptContext, message, &__ttd_resultPtr));

        newError = scriptContext->GetLibrary()->CreateURIError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(error);

        return JsNoError;
    });
}

CHAKRA_API JsHasException(_Out_ bool *hasException)
{
    PARAM_NOT_NULL(hasException);
    *hasException = false;

    JsrtContext *currentContext = JsrtContext::GetCurrent();

    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    Assert(scriptContext != nullptr);

    if (scriptContext->GetRecycler() && scriptContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (scriptContext->GetThreadContext()->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    if (scriptContext->GetThreadContext()->IsExecutionDisabled())
    {
        return JsErrorInDisabledState;
    }

    *hasException = scriptContext->HasRecordedException();

    return JsNoError;
}

CHAKRA_API JsGetAndClearException(_Out_ JsValueRef *exception)
{
    PARAM_NOT_NULL(exception);
    *exception = nullptr;

    JsrtContext *currentContext = JsrtContext::GetCurrent();

    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    Assert(scriptContext != nullptr);

    if (scriptContext->GetRecycler() && scriptContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (scriptContext->GetThreadContext()->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    if (scriptContext->GetThreadContext()->IsExecutionDisabled())
    {
        return JsErrorInDisabledState;
    }

    HRESULT hr = S_OK;
    Js::JavascriptExceptionObject *recordedException = nullptr;

    BEGIN_TRANSLATE_OOM_TO_HRESULT
      recordedException = scriptContext->GetAndClearRecordedException();
    END_TRANSLATE_OOM_TO_HRESULT(hr)

    if (hr == E_OUTOFMEMORY)
    {
        recordedException = scriptContext->GetThreadContext()->GetRecordedException();
    }
    if (recordedException == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    *exception = recordedException->GetThrownObject(nullptr);
    if (*exception == nullptr)
    {
        return JsErrorInvalidArgument;
    }

#if ENABLE_TTD
    BEGIN_JS_RUNTIME_CALL_NOT_SCRIPT(scriptContext)
    {
        PERFORM_JSRT_TTD_RECORD_ACTION_WRESULT(scriptContext, scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetAndClearException(scriptContext, &__ttd_resultPtr));
        PERFORM_JSRT_TTD_RECORD_ACTION_PROCESS_RESULT(exception);
    }
    END_JS_RUNTIME_CALL(scriptContext)
#endif

    return JsNoError;
}

CHAKRA_API JsSetException(_In_ JsValueRef exception)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(exception, scriptContext);

        Js::JavascriptExceptionObject *exceptionObject;

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        exceptionObject = RecyclerNew(scriptContext->GetRecycler(), Js::JavascriptExceptionObject, exception, scriptContext, nullptr);

        JsrtContext * context = JsrtContext::GetCurrent();
        JsrtRuntime * runtime = context->GetRuntime();

        scriptContext->RecordException(exceptionObject, runtime->DispatchExceptions());

        return JsNoError;
    });
}

CHAKRA_API JsGetRuntimeMemoryUsage(_In_ JsRuntimeHandle runtimeHandle, _Out_ size_t * memoryUsage)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(memoryUsage);
    *memoryUsage = 0;

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    *memoryUsage = allocPolicyManager->GetUsage();

    return JsNoError;
}

CHAKRA_API JsSetRuntimeMemoryLimit(_In_ JsRuntimeHandle runtimeHandle, _In_ size_t memoryLimit)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    allocPolicyManager->SetLimit(memoryLimit);

    return JsNoError;
}

CHAKRA_API JsGetRuntimeMemoryLimit(_In_ JsRuntimeHandle runtimeHandle, _Out_ size_t * memoryLimit)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(memoryLimit);
    *memoryLimit = 0;

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    *memoryLimit = allocPolicyManager->GetLimit();

    return JsNoError;
}

C_ASSERT(JsMemoryAllocate == (_JsMemoryEventType) AllocationPolicyManager::MemoryAllocateEvent::MemoryAllocate);
C_ASSERT(JsMemoryFree == (_JsMemoryEventType) AllocationPolicyManager::MemoryAllocateEvent::MemoryFree);
C_ASSERT(JsMemoryFailure == (_JsMemoryEventType) AllocationPolicyManager::MemoryAllocateEvent::MemoryFailure);
C_ASSERT(JsMemoryFailure == (_JsMemoryEventType) AllocationPolicyManager::MemoryAllocateEvent::MemoryMax);

CHAKRA_API JsSetRuntimeMemoryAllocationCallback(_In_ JsRuntimeHandle runtime, _In_opt_ void *callbackState, _In_ JsMemoryAllocationCallback allocationCallback)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtime);

    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtime)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    allocPolicyManager->SetMemoryAllocationCallback(callbackState, (AllocationPolicyManager::PageAllocatorMemoryAllocationCallback)allocationCallback);

    return JsNoError;
}

CHAKRA_API JsSetRuntimeBeforeCollectCallback(_In_ JsRuntimeHandle runtime, _In_opt_ void *callbackState, _In_ JsBeforeCollectCallback beforeCollectCallback)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtime);

        JsrtRuntime::FromHandle(runtime)->SetBeforeCollectCallback(beforeCollectCallback, callbackState);
        return JsNoError;
    });
}

CHAKRA_API JsDisableRuntimeExecution(_In_ JsRuntimeHandle runtimeHandle)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    if (!threadContext->TestThreadContextFlag(ThreadContextFlagCanDisableExecution))
    {
        return JsErrorCannotDisableExecution;
    }

    if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (threadContext->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    threadContext->DisableExecution();
    return JsNoError;
}

CHAKRA_API JsEnableRuntimeExecution(_In_ JsRuntimeHandle runtimeHandle)
{
    return GlobalAPIWrapper([&] () -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
        if (!threadContext->TestThreadContextFlag(ThreadContextFlagCanDisableExecution))
        {
            return JsNoError;
        }

        if (threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (threadContext->IsInThreadServiceCallback())
        {
            return JsErrorInThreadServiceCallback;
        }

        ThreadContextScope scope(threadContext);

        if (!scope.IsValid())
        {
            return JsErrorWrongThread;
        }

        threadContext->EnableExecution();
        return JsNoError;
    });
}

CHAKRA_API JsIsRuntimeExecutionDisabled(_In_ JsRuntimeHandle runtimeHandle, _Out_ bool *isDisabled)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(isDisabled);
    *isDisabled = false;

    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    *isDisabled = threadContext->IsExecutionDisabled();
    return JsNoError;
}

CHAKRA_API JsGetPropertyIdFromName(_In_z_ const wchar_t *name, _Out_ JsPropertyIdRef *propertyId)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(name);
        PARAM_NOT_NULL(propertyId);
        *propertyId = nullptr;

        size_t cPropertyNameLength = wcslen(name);

        if (cPropertyNameLength <= INT_MAX)
        {
            scriptContext->GetOrAddPropertyRecord(name, static_cast<int>(cPropertyNameLength), (Js::PropertyRecord const **)propertyId);

            return JsNoError;
        }
        else
        {
            return JsErrorOutOfMemory;
        }
    });
}

CHAKRA_API JsGetPropertyIdFromNameUtf8(_In_z_ const char *name, _Out_ JsPropertyIdRef *propertyId)
{
    // xplat-todo: should pass in utf8 length
    PARAM_NOT_NULL(name);
    utf8::NarrowToWide wname(name);
    if (!wname)
    {
      return JsErrorOutOfMemory;
    }

    // xplat-todo: does following accept embedded null?
    return JsGetPropertyIdFromName(wname, propertyId);
}

CHAKRA_API JsGetPropertyIdFromSymbol(_In_ JsValueRef symbol, _Out_ JsPropertyIdRef *propertyId)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(symbol, scriptContext);
        PARAM_NOT_NULL(propertyId);
        *propertyId = nullptr;

        if (!Js::JavascriptSymbol::Is(symbol))
        {
            return JsErrorPropertyNotSymbol;
        }

        *propertyId = (JsPropertyIdRef)Js::JavascriptSymbol::FromVar(symbol)->GetValue();
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetSymbolFromPropertyId(_In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *symbol)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(symbol);
        *symbol = nullptr;

        Js::PropertyRecord const * propertyRecord = (Js::PropertyRecord const *)propertyId;
        if (!propertyRecord->IsSymbol())
        {
            return JsErrorPropertyNotSymbol;
        }

        *symbol = scriptContext->GetLibrary()->CreateSymbol(propertyRecord);
        return JsNoError;
    });
}

#pragma prefast(suppress:6101, "Prefast doesn't see through the lambda")
CHAKRA_API JsGetPropertyNameFromId(_In_ JsPropertyIdRef propertyId, _Outptr_result_z_ const wchar_t **name)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(name);
        *name = nullptr;

        Js::PropertyRecord const * propertyRecord = (Js::PropertyRecord const *)propertyId;

        if (propertyRecord->IsSymbol())
        {
            return JsErrorPropertyNotString;
        }

        *name = propertyRecord->GetBuffer();
        return JsNoError;
    });
}

CHAKRA_API JsGetPropertyNameFromIdUtf8Copy(_In_ JsPropertyIdRef propertyId, _Outptr_result_z_ char **name)
{
    const wchar_t *wname = nullptr;

    JsErrorCode err = JsGetPropertyNameFromId(propertyId, &wname);

    if (err == JsNoError)
    {
        // xplat-todo: fix encoding. The result is cesu8.
        utf8::WideToNarrow str(wname, wcslen(wname));
        if (str)
        {
            *name = str.Detach();
        }
        else
        {
            err = JsErrorOutOfMemory;
        }
    }

    return err;
}

CHAKRA_API JsGetPropertyIdType(_In_ JsPropertyIdRef propertyId, _Out_ JsPropertyIdType* propertyIdType)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        VALIDATE_INCOMING_PROPERTYID(propertyId);

        Js::PropertyRecord const * propertyRecord = (Js::PropertyRecord const *)propertyId;

        if (propertyRecord->IsSymbol())
        {
            *propertyIdType = JsPropertyIdTypeSymbol;
        }
        else
        {
            *propertyIdType = JsPropertyIdTypeString;
        }
        return JsNoError;
    });
}


CHAKRA_API JsGetRuntime(_In_ JsContextRef context, _Out_ JsRuntimeHandle *runtime)
{
    VALIDATE_JSREF(context);
    PARAM_NOT_NULL(runtime);

    *runtime = nullptr;

    if (!JsrtContext::Is(context))
    {
        return JsErrorInvalidArgument;
    }

    *runtime = static_cast<JsrtContext *>(context)->GetRuntime();
    return JsNoError;
}

CHAKRA_API JsIdle(_Out_opt_ unsigned int *nextIdleTick)
{
    PARAM_NOT_NULL(nextIdleTick);

    return ContextAPINoScriptWrapper(
        [&] (Js::ScriptContext * scriptContext) -> JsErrorCode {

            *nextIdleTick = 0;

            if (scriptContext->GetThreadContext()->GetRecycler() && scriptContext->GetThreadContext()->GetRecycler()->IsHeapEnumInProgress())
            {
                return JsErrorHeapEnumInProgress;
            }
            else if (scriptContext->GetThreadContext()->IsInThreadServiceCallback())
            {
                return JsErrorInThreadServiceCallback;
            }

            JsrtContext * context = JsrtContext::GetCurrent();
            JsrtRuntime * runtime = context->GetRuntime();

            if (!runtime->UseIdle())
            {
                return JsErrorIdleNotEnabled;
            }

            unsigned int ticks = runtime->Idle();

            *nextIdleTick = ticks;

            return JsNoError;
    });
}

CHAKRA_API JsSetPromiseContinuationCallback(_In_ JsPromiseContinuationCallback promiseContinuationCallback, _In_opt_ void *callbackState)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(promiseContinuationCallback);

        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        scriptContext->GetLibrary()->SetNativeHostPromiseContinuationFunction((Js::JavascriptLibrary::PromiseContinuationCallback)promiseContinuationCallback, callbackState);
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

JsErrorCode RunScriptCore(const byte *script, size_t cb, LoadScriptFlag loadScriptFlag, JsSourceContext sourceContext, const wchar_t *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes, bool isSourceModule, JsValueRef *result)
{
    Js::JavascriptFunction *scriptFunction;
    CompileScriptException se;

#if ENABLE_TTD
    uint64 bodyCtrId = 0;
#endif

    JsErrorCode errorCode = ContextAPINoScriptWrapper(
        [&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(script);
        PARAM_NOT_NULL(sourceUrl);

        SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);

        if (sourceContextInfo == nullptr)
        {
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, sourceUrl, wcslen(sourceUrl), nullptr);
        }

        const int chsize = (loadScriptFlag & LoadScriptFlag_Utf8Source) ? sizeof(utf8char_t) : sizeof(wchar_t);
        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(cb / chsize), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };

        Js::Utf8SourceInfo* utf8SourceInfo = nullptr;
        if (result != nullptr)
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_Expression);
        }
        bool isLibraryCode = (parseAttributes & JsParseScriptAttributeLibraryCode) == JsParseScriptAttributeLibraryCode;
        if (isLibraryCode)
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_LibraryCode);
        }
        if (isSourceModule)
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_Module);
        }
        scriptFunction = scriptContext->LoadScript(script, cb, &si, &se, &utf8SourceInfo, Js::Constants::GlobalCode, loadScriptFlag);

#if ENABLE_TTD
        //
        //TODO: We may (probably?) want to use the debugger source rundown functionality here instead
        //
        if (scriptContext->ShouldPerformRecordTopLevelFunction())
        {
            //Make sure we have the body and text information available
            Js::FunctionBody* globalBody = TTD::JsSupport::ForceAndGetFunctionBody(scriptFunction->GetParseableFunctionInfo());

            const TTD::NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* tbfi = scriptContext->GetThreadContext()->TTDLog->AddScriptLoad(globalBody, kmodGlobal, globalBody->GetUtf8SourceInfo()->GetSourceInfoId(), script, (uint32)cb, loadScriptFlag);
            bodyCtrId = tbfi->TopLevelBase.TopLevelBodyCtr;

            //walk global body to (1) add functions to pin set (2) build parent map
            BEGIN_JS_RUNTIME_CALL(scriptContext);
            {
                scriptContext->TTDContextInfo->ProcessFunctionBodyOnLoad(globalBody, nullptr);
                scriptContext->TTDContextInfo->RegisterLoadedScript(globalBody, tbfi->TopLevelBase.TopLevelBodyCtr);
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }
#endif

        JsrtContext * context = JsrtContext::GetCurrent();
        context->OnScriptLoad(scriptFunction, utf8SourceInfo, &se);

        return JsNoError;
    });

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        if (scriptFunction == nullptr)
        {
            HandleScriptCompileError(scriptContext, &se);
            return JsErrorScriptCompile;
        }

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            //
            //TODO: Module support not implemented yet
            //
            AssertMsg(!isSourceModule, "Modules not implemented in TTD yet!!!");

            bool isUtf8 = ((loadScriptFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source);
            threadContext->TTDLog->RecordJsRTCodeParse(scriptContext, bodyCtrId, loadScriptFlag, scriptFunction, isUtf8, script, (uint32)cb, sourceUrl, scriptFunction);
        }
#endif
        if (parseOnly)
        {
            PARAM_NOT_NULL(result);
            *result = scriptFunction;
        }
        else
        {
            Js::Arguments args(0, nullptr);
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            Js::Var varThis;
            if (PHASE_FORCE1(Js::EvalCompilePhase))
            {
                varThis = Js::JavascriptOperators::OP_GetThis(scriptContext->GetLibrary()->GetUndefined(), kmodGlobal, scriptContext);
                args.Info.Flags = (Js::CallFlags)Js::CallFlags::CallFlags_Eval;
                args.Info.Count = 1;
                args.Values = &varThis;
            }
#endif

#if ENABLE_TTD
            double ttdStartTime = -1.0;

            if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
            {
                TTD::NSLogEvents::EventLogEntry* callEvent = scriptContext->GetThreadContext()->TTDLog->RecordJsRTCallFunction(scriptContext, scriptContext->TTDRootNestingCount, scriptFunction, args.Info.Count, args.Values);
                TTD::TTDRecordJsRTFunctionCallActionPopper actionPopper(scriptContext, callEvent);

                if (scriptContext->TTDRootNestingCount == 0)
                {
                    TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
                    elog->ResetCallStackForTopLevelCall(elog->GetLastEventTime());

                    ttdStartTime = elog->GetCurrentWallTime();
                }

                Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);
                if (result != nullptr)
                {
                    *result = varResult;
                }

                actionPopper.NormalReturn(result != nullptr ? *result : nullptr);
            }
            else
            {
                Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);

                if (result != nullptr)
                {
                    *result = varResult;
                }
            }

            //Update the time elapsed since a snapshot if needed
            if (ttdStartTime >= 0.0)
            {
                TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;

                double ttdEndTime = elog->GetCurrentWallTime();
                elog->IncrementElapsedSnapshotTime(ttdEndTime - ttdStartTime);
            }
#else
            Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);
            if (result != nullptr)
            {
                *result = varResult;
            }
#endif
        }
        return JsNoError;
    });
}

JsErrorCode RunScriptCore(const char *script, JsSourceContext sourceContext, const char *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes, bool isSourceModule, JsValueRef *result)
{
    utf8::NarrowToWide url((LPCSTR)sourceUrl);
    if (!url)
    {
        return JsErrorOutOfMemory;
    }

    return RunScriptCore(reinterpret_cast<const byte*>(script), strlen(script), LoadScriptFlag_Utf8Source, sourceContext, url, parseOnly, parseAttributes, isSourceModule, result);
}

JsErrorCode RunScriptCore(const wchar_t *script, JsSourceContext sourceContext, const wchar_t *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes, bool isSourceModule, JsValueRef *result)
{
    return RunScriptCore(reinterpret_cast<const byte*>(script), wcslen(script) * sizeof(wchar_t), LoadScriptFlag_None, sourceContext, sourceUrl, parseOnly, parseAttributes, isSourceModule, result);
}

#ifdef _WIN32
CHAKRA_API JsParseScript(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, true, JsParseScriptAttributeNone, false /*isModule*/, result);
}

CHAKRA_API JsParseScriptWithAttributes(
    _In_z_ const wchar_t *script,
    _In_ JsSourceContext sourceContext,
    _In_z_ const wchar_t *sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, true, parseAttributes, false /*isModule*/, result);
}

CHAKRA_API JsRunScript(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, false /*isModule*/, result);
}

CHAKRA_API JsExperimentalApiRunModule(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, true, result);
}
#endif

JsErrorCode JsSerializeScriptCore(const byte *script, size_t cb, LoadScriptFlag loadScriptFlag, BYTE *functionTable, int functionTableSize, unsigned char *buffer, unsigned int *bufferSize)
{
    Js::JavascriptFunction *function;
    CompileScriptException se;

    JsErrorCode errorCode = ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(script);
        PARAM_NOT_NULL(bufferSize);

        if (*bufferSize > 0)
        {
            PARAM_NOT_NULL(buffer);
            ZeroMemory(buffer, *bufferSize);
        }

        if (scriptContext->IsScriptContextInDebugMode())
        {
            return JsErrorCannotSerializeDebugScript;
        }

        SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(JS_SOURCE_CONTEXT_NONE, nullptr);
        Assert(sourceContextInfo != nullptr);

        const int chsize = (loadScriptFlag & LoadScriptFlag_Utf8Source) ? sizeof(utf8char_t) : sizeof(wchar_t);
        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(cb / chsize), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };
        bool isSerializeByteCodeForLibrary = false;
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        isSerializeByteCodeForLibrary = JsrtContext::GetCurrent()->GetRuntime()->IsSerializeByteCodeForLibrary();
#endif

        Js::Utf8SourceInfo* sourceInfo = nullptr;
        loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_disableDeferredParse);
        if (isSerializeByteCodeForLibrary)
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_isByteCodeBufferForLibrary);
        }
        else
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_Expression);
        }
        function = scriptContext->LoadScript(script, cb, &si, &se, &sourceInfo, Js::Constants::GlobalCode, loadScriptFlag);
        return JsNoError;
    });

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        if (function == nullptr)
        {
            HandleScriptCompileError(scriptContext, &se);
            return JsErrorScriptCompile;
        }
        // Could we have a deserialized function in this case?
        // If we are going to serialize it, a check isn't to expensive
        if (CONFIG_FLAG(ForceSerialized) && function->GetFunctionProxy() != nullptr) {
            function->GetFunctionProxy()->EnsureDeserialized();
        }
        Js::FunctionBody *functionBody = function->GetFunctionBody();
        const Js::Utf8SourceInfo *sourceInfo = functionBody->GetUtf8SourceInfo();
        size_t cSourceCodeLength = sourceInfo->GetCbLength(_u("JsSerializeScript"));

        // truncation of code length can lead to accessing random memory. Reject the call.
        if (cSourceCodeLength > DWORD_MAX)
        {
            return JsErrorOutOfMemory;
        }

        LPCUTF8 utf8Code = sourceInfo->GetSource(_u("JsSerializeScript"));
        DWORD dwFlags = 0;
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        dwFlags = JsrtContext::GetCurrent()->GetRuntime()->IsSerializeByteCodeForLibrary() ? GENERATE_BYTE_CODE_BUFFER_LIBRARY : 0;
#endif

        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, _u("ByteCodeSerializer"));
        // We cast buffer size to DWORD* because on Windows, DWORD = unsigned long = unsigned int
        // On 64-bit clang on linux, this is not true, unsigned long is larger than unsigned int
        // However, the PAL defines DWORD for us on linux as unsigned int so the cast is safe here.
        HRESULT hr = Js::ByteCodeSerializer::SerializeToBuffer(scriptContext, tempAllocator, static_cast<DWORD>(cSourceCodeLength), utf8Code, functionBody, functionBody->GetHostSrcInfo(), false, &buffer, (DWORD*) bufferSize, dwFlags);
        END_TEMP_ALLOCATOR(tempAllocator, scriptContext);

        if (SUCCEEDED(hr))
        {
            return JsNoError;
        }
        else
        {
            return JsErrorScriptCompile;
        }
    });

}

CHAKRA_API JsSerializeScript(_In_z_ const wchar_t *script, _Out_writes_to_opt_(*bufferSize, *bufferSize) unsigned char *buffer,
    _Inout_ unsigned int *bufferSize)
{
    return JsSerializeScriptCore((const byte*)script, wcslen(script) * sizeof(wchar_t), LoadScriptFlag_None, nullptr, 0, buffer, bufferSize);
}

template <typename TLoadCallback, typename TUnloadCallback>
JsErrorCode RunSerializedScriptCore(
    TLoadCallback scriptLoadCallback, TUnloadCallback scriptUnloadCallback,
    JsSourceContext scriptLoadSourceContext, // only used by scriptLoadCallback
    unsigned char *buffer,
    JsSourceContext sourceContext, const wchar_t *sourceUrl,
    bool parseOnly, JsValueRef *result)
{
    Js::JavascriptFunction *function;
    JsErrorCode errorCode = ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if (result != nullptr)
        {
            *result = nullptr;
        }

        PARAM_NOT_NULL(buffer);
        PARAM_NOT_NULL(sourceUrl);

        Js::ISourceHolder *sourceHolder = nullptr;
        PARAM_NOT_NULL(scriptLoadCallback);
        PARAM_NOT_NULL(scriptUnloadCallback);
        typedef Js::JsrtSourceHolder<TLoadCallback, TUnloadCallback> TSourceHolder;
        sourceHolder = RecyclerNewFinalized(scriptContext->GetRecycler(), TSourceHolder, scriptLoadCallback, scriptUnloadCallback, scriptLoadSourceContext);

        SourceContextInfo *sourceContextInfo;
        SRCINFO *hsi;
        Js::FunctionBody *functionBody = nullptr;

        HRESULT hr;

        sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);

        if (sourceContextInfo == nullptr)
        {
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, sourceUrl, wcslen(sourceUrl), nullptr);
        }

        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ 0, // xplat-todo: need to compute this?
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };

        uint32 flags = 0;

        if (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling())
        {
            flags = fscrAllowFunctionProxy;
        }

        hsi = scriptContext->AddHostSrcInfo(&si);
        hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, sourceHolder, hsi, buffer, nullptr, &functionBody);

        if (FAILED(hr))
        {
            return JsErrorBadSerializedScript;
        }

        function = scriptContext->GetLibrary()->CreateScriptFunction(functionBody);

        JsrtContext * context = JsrtContext::GetCurrent();
        context->OnScriptLoad(function, functionBody->GetUtf8SourceInfo(), nullptr);

        return JsNoError;
    });

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        if (parseOnly)
        {
            PARAM_NOT_NULL(result);
            *result = function;
        }
        else
        {
            Js::Var varResult = function->CallRootFunction(Js::Arguments(0, nullptr), scriptContext, true);
            if (result != nullptr)
            {
                *result = varResult;
            }
        }
        return JsNoError;
    });
}

#ifdef _WIN32

static bool CHAKRA_CALLBACK DummyScriptLoadSourceCallback(_In_ JsSourceContext sourceContext, _Outptr_result_z_ const wchar_t** scriptBuffer)
{
    // sourceContext is actually the script source pointer
    *scriptBuffer = reinterpret_cast<const wchar_t*>(sourceContext);
    return true;
}

static void CHAKRA_CALLBACK DummyScriptUnloadCallback(_In_ JsSourceContext sourceContext)
{
    // Do nothing
}

CHAKRA_API JsParseSerializedScript(_In_z_ const wchar_t * script, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        DummyScriptLoadSourceCallback, DummyScriptUnloadCallback,
        reinterpret_cast<JsSourceContext>(script), // use script source pointer as scriptLoadSourceContext
        buffer, sourceContext, sourceUrl, true, result);
}

CHAKRA_API JsRunSerializedScript(_In_z_ const wchar_t * script, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        DummyScriptLoadSourceCallback, DummyScriptUnloadCallback,
        reinterpret_cast<JsSourceContext>(script), // use script source pointer as scriptLoadSourceContext
        buffer, sourceContext, sourceUrl, false, result);
}

CHAKRA_API JsParseSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback, _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        scriptLoadCallback, scriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, sourceContext, sourceUrl, true, result);
}

CHAKRA_API JsRunSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback, _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_opt_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        scriptLoadCallback, scriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, sourceContext, sourceUrl, false, result);
}
#endif // _WIN32

CHAKRA_API JsParseScriptUtf8(
    _In_z_ const char *script,
    _In_ JsSourceContext sourceContext,
    _In_z_ const char *sourceUrl,
    _Out_ JsValueRef *result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, /*parseOnly*/true,
            JsParseScriptAttributeNone, /*isSourceModule*/false, result);
}

CHAKRA_API JsParseScriptWithAttributesUtf8(
    _In_z_ const char *script,
    _In_ JsSourceContext sourceContext,
    _In_z_ const char *sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, true, parseAttributes, false, result);
}

CHAKRA_API JsRunScriptUtf8(
    _In_z_ const char *script,
    _In_ JsSourceContext sourceContext,
    _In_z_ const char *sourceUrl,
    _Out_ JsValueRef *result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, false, result);
}

CHAKRA_API JsSerializeScriptUtf8(
    _In_z_ const char *script,
    _Out_writes_to_opt_(*bufferSize, *bufferSize) ChakraBytePtr buffer,
    _Inout_ unsigned int *bufferSize)
{
    return JsSerializeScriptCore((const byte*)script, strlen(script), LoadScriptFlag_Utf8Source, nullptr, 0, buffer, bufferSize);
}

CHAKRA_API JsRunSerializedScriptUtf8(
    _In_ JsSerializedScriptLoadUtf8SourceCallback scriptLoadCallback,
    _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback,
    _In_ ChakraBytePtr buffer,
    _In_ JsSourceContext sourceContext,
    _In_z_ const char *sourceUrl,
    _Out_opt_ JsValueRef * result)
{
    utf8::NarrowToWide url(sourceUrl);
    if (!url)
    {
        return JsErrorOutOfMemory;
    }

    return RunSerializedScriptCore(
        scriptLoadCallback, scriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, sourceContext, url, false, result);
}

CHAKRA_API JsStringFree(_In_ char* stringValue)
{
    if (stringValue == nullptr)
    {
        return JsErrorNullArgument;
    }

    // Utf8Helper uses malloc_allocator
    free(stringValue);

    return JsNoError;
}

/////////////////////

CHAKRA_API JsTTDCreateRecordRuntime(_In_ JsRuntimeAttributes attributes, _In_reads_(infoUriCount) const byte* infoUri, _In_ size_t infoUriCount,
    _In_ size_t snapInterval, _In_ size_t snapHistoryLength,
    _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    if(snapInterval > UINT32_MAX || snapHistoryLength > UINT32_MAX)
    {
        return JsErrorInvalidArgument;
    }

    return CreateRuntimeCore(attributes, infoUri, infoUriCount, nullptr, 0, (uint32)snapInterval, (uint32)snapHistoryLength, threadService, runtime);
#endif
}

CHAKRA_API JsTTDCreateDebugRuntime(_In_ JsRuntimeAttributes attributes, _In_reads_(infoUriCount) const byte* infoUri, _In_ size_t infoUriCount,
    _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return CreateRuntimeCore(attributes, nullptr, 0, infoUri, infoUriCount, UINT_MAX, UINT_MAX, threadService, runtime);
#endif
}

CHAKRA_API JsTTDCreateContext(_In_ JsRuntimeHandle runtime, _Out_ JsContextRef *newContext)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return CreateContextCore(runtime, true, newContext);
#endif
}

CHAKRA_API JsTTDSetIOCallbacks(_In_ JsRuntimeHandle runtime,
    _In_ JsTTDInitializeForWriteLogStreamCallback writeInitializeFunction, _In_ TTDOpenResourceStreamCallback openResourceStream,
    _In_ JsTTDReadBytesFromStreamCallback readBytesFromStream, _In_ JsTTDWriteBytesToStreamCallback writeBytesToStream, _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtime)->GetThreadContext();

    if (writeInitializeFunction == nullptr || openResourceStream == nullptr
        || readBytesFromStream == nullptr || writeBytesToStream == nullptr || flushAndCloseStream == nullptr)
    {
        return JsErrorNullArgument;
    }

    threadContext->TTDWriteInitializeFunction = writeInitializeFunction;
    threadContext->TTDStreamFunctions.pfGetResourceStream = openResourceStream;
    threadContext->TTDStreamFunctions.pfReadBytesFromStream = readBytesFromStream;
    threadContext->TTDStreamFunctions.pfWriteBytesToStream = writeBytesToStream;
    threadContext->TTDStreamFunctions.pfFlushAndCloseStream = flushAndCloseStream;

    return GlobalAPIWrapper([&]() -> JsErrorCode {
        ThreadContextScope scope(threadContext);

        //Make sure the thread context recycler is allocated before we do anything else
        threadContext->EnsureRecycler();

#if ENABLE_TTD_DEBUGGING
        threadContext->SetThreadContextFlag(ThreadContextFlagNoJIT);
#endif

        threadContext->InitTimeTravel(threadContext->IsTTRecordRequested, threadContext->IsTTDebugRequested);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDStartTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->TTDLog == nullptr)
        {
            AssertMsg(false, "Need to create in TTD mode.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDDetached())
        {
            AssertMsg(false, "Cannot re-start TTD after detach.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDActive())
        {
            AssertMsg(false, "Already started TTD.");
            return JsErrorCategoryUsage;
        }

        threadContext->TTDLog->SetGlobalMode(TTD::TTDMode::RecordEnabled);

        threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);

        threadContext->TTDLog->DoSnapshotExtract();

        threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDStopTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->TTDLog == nullptr)
        {
            AssertMsg(false, "Need to create in TTD mode.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDDetached())
        {
            AssertMsg(false, "Already stopped TTD.");
            return JsErrorCategoryUsage;
        }

        if (!scriptContext->IsTTDActive())
        {
            AssertMsg(false, "TTD was never started.");
            return JsErrorCategoryUsage;
        }

        threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);

        threadContext->EmitTTDLogIfNeeded();
        threadContext->EndCtxTimeTravel(scriptContext);

        threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDEmitTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->TTDLog == nullptr)
        {
            AssertMsg(false, "Need to create in TTD mode.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDDetached())
        {
            AssertMsg(false, "Already stopped TTD.");
            return JsErrorCategoryUsage;
        }

        if (!scriptContext->IsTTDActive())
        {
            AssertMsg(false, "TTD was never started.");
            return JsErrorCategoryUsage;
        }

        threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);

        threadContext->EmitTTDLogIfNeeded();

        threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDStartTimeTravelDebugging()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->TTDLog == nullptr)
        {
            AssertMsg(false, "Need to create in TTD mode.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDDetached())
        {
            AssertMsg(false, "Cannot re-start TTD after detach.");
            return JsErrorCategoryUsage;
        }

        if (scriptContext->IsTTDActive())
        {
            AssertMsg(false, "Already started TTD.");
            return JsErrorCategoryUsage;
        }

        scriptContext->GetThreadContext()->TTDLog->SetIntoDebuggingMode();

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();

        if (threadContext->TTDLog != nullptr)
        {
            threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDReStartTimeTravelAfterRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if (threadContext->TTDLog != nullptr)
        {
            threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDNotifyYield()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            scriptContext->GetThreadContext()->TTDLog->RecordJsRTEventLoopYieldPoint(scriptContext);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDHostExit(_In_ int statusCode)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {

        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            scriptContext->GetThreadContext()->TTDLog->RecordJsRTHostExitProcess(scriptContext, statusCode);
        }

        return JsNoError;
});
#endif
}

CHAKRA_API JsTTDRawBufferCopySyncIndirect(_In_ JsValueRef dst, _In_ size_t dstIndex, _In_ JsValueRef src, _In_ size_t srcIndex, _In_ size_t count)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            if(dstIndex > UINT32_MAX || srcIndex > UINT32_MAX || count > UINT32_MAX)
            {
                return JsErrorInvalidArgument;
            }

            scriptContext->GetThreadContext()->TTDLog->RecordJsRTRawBufferCopySync(scriptContext, dst, (uint32)dstIndex, src, (uint32)srcIndex, (uint32)count);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDRawBufferModifySyncIndirect(_In_ JsValueRef buffer, _In_ size_t index, _In_ size_t count)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if (PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            if(index > UINT32_MAX || count > UINT32_MAX)
            {
                return JsErrorInvalidArgument;
            }

            scriptContext->GetThreadContext()->TTDLog->RecordJsRTRawBufferModifySync(scriptContext, buffer, (uint32)index, (uint32)count);
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDRawBufferAsyncModificationRegister(_In_ JsValueRef instance, _In_ byte* initialModPos)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsValueRef addRefObj = nullptr;
    JsErrorCode addRefResult = ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if (scriptContext->ShouldPerformAsyncBufferModAction())
        {
            addRefObj = scriptContext->GetThreadContext()->TTDLog->RecordJsRTRawBufferAsyncModificationRegister(scriptContext, instance, initialModPos);
        }

        return JsNoError;
    });

    if(addRefResult != JsNoError)
    {
        return addRefResult;
    }

    //We need to root add ref so we can find this during replay!!!
    if(addRefObj == nullptr)
    {
        return JsNoError;
    }
    else
    {
        return JsAddRef(addRefObj, nullptr);
    }
#endif
}

CHAKRA_API JsTTDRawBufferAsyncModifyComplete(_In_ byte* finalModPos)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsValueRef releaseObj = nullptr;
    JsErrorCode releaseStatus = ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        if(scriptContext->ShouldPerformAsyncBufferModAction())
        {
            releaseObj = scriptContext->GetThreadContext()->TTDLog->RecordJsRTRawBufferAsyncModifyComplete(scriptContext, finalModPos);
        }

        return JsNoError;
    });

    if(releaseStatus != JsNoError)
    {
        return releaseStatus;
    }

    //We need to root release ref so we can free this in replay if needed!!!
    if(releaseObj == nullptr)
    {
        return JsNoError;
    }
    else
    {
        return JsRelease(releaseObj, nullptr);
    }

#endif
}

#if ENABLE_TTD_DEBUGGING
static void CALLBACK TTDDummyPromiseContinuationCallback(JsValueRef task, void *callbackState)
{
    AssertMsg(false, "This should never actually be invoked!!!");
}
#endif

CHAKRA_API JsTTDGetSnapTimeTopLevelEventMove(_In_ JsRuntimeHandle runtimeHandle, 
    _In_ JsTTDMoveMode moveMode, _Inout_ int64_t* targetEventTime, 
    _Out_ bool* createFreshCxts, _Out_ int64_t* targetStartSnapTime, _Out_opt_ int64_t* targetEndSnapTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext * threadContext = runtime->GetThreadContext();

    *createFreshCxts = false;
    *targetStartSnapTime = -1;
    if(targetEndSnapTime != nullptr)
    {
        *targetEndSnapTime = -1;
    }

    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorFatal;
    }

    //If we requested a move to a specific event then extract the event count and try to find it
    bool scanJMC = (moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalBeforeDebugExecute) == JsTTDMoveMode::JsTTDMoveScanIntervalBeforeDebugExecute;

    if((moveMode & JsTTDMoveMode::JsTTDMoveFirstEvent) == JsTTDMoveMode::JsTTDMoveFirstEvent)
    {
        *targetEventTime = threadContext->TTDLog->GetFirstEventTime(scanJMC);
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else if((moveMode & JsTTDMoveMode::JsTTDMoveLastEvent) == JsTTDMoveMode::JsTTDMoveLastEvent)
    {
        *targetEventTime = threadContext->TTDLog->GetLastEventTime(scanJMC);
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else if((moveMode & JsTTDMoveMode::JsTTDMoveKthEvent) == JsTTDMoveMode::JsTTDMoveKthEvent)
    {
        uint32 kthEvent = (uint32)(((int64)moveMode) >> 32);
        *targetEventTime = threadContext->TTDLog->GetKthEventTime(kthEvent);
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else
    {
        ;
    }

    bool rtrok = (moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalBeforeDebugExecute) == JsTTDMoveMode::JsTTDMoveScanIntervalBeforeDebugExecute;
    *targetStartSnapTime = threadContext->TTDLog->FindSnapTimeForEventTime(*targetEventTime, rtrok, createFreshCxts, targetEndSnapTime);

    return JsNoError;
#endif
}

CHAKRA_API JsTTDPrepContextsForTopLevelEventMove(_In_ JsRuntimeHandle runtimeHandle, _In_ bool createFreshCtxs)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext * threadContext = runtime->GetThreadContext();

   if (!createFreshCtxs)
    {
        //We are continuing to step around using the same context so make sure we don't have any pending recorded exceptions
        JsrtContext* context = JsrtContext::GetCurrent();
        AssertMsg(context != nullptr, "This should always be set if we get here.");

        if(context->GetScriptContext()->HasRecordedException())
        {
            context->GetScriptContext()->GetAndClearRecordedException(nullptr);
        }
    }
    else 
    {
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

            ThreadContextScope scope(threadContext);
            AssertMsg(scope.IsValid(), "Hmm not cool");

            bool canDeleteOldCtx = threadContext->TTDLog->UpdateInflateMapForFreshScriptContexts();
            JsrtContext* oldContext = JsrtContext::GetCurrent();

            JsrtContext* context = nullptr;
            JsContextRef jsContextRef = nullptr;
            JsErrorCode ok = JsTTDCreateContext(runtime, &jsContextRef);
            if(ok != JsNoError)
            {
                AssertMsg(false, "Failed to create new ScriptContext");
                return JsErrorFatal;
            }

            context = reinterpret_cast<JsrtContext*>(jsContextRef); //we know this is it so just he-man cast it
            JsrtContext::TrySetCurrent(context);

            if(oldContext != nullptr && canDeleteOldCtx)
            {
                oldContext->Dispose(false);
            }

            JsSetPromiseContinuationCallback(TTDDummyPromiseContinuationCallback, nullptr);
        }
        catch(...)
        {
            //something went horribly wrong
            AssertMsg(false, "Unexpected fatal Error");
            return JsErrorFatal;
        }
    }

    return JsNoError;
#endif
}

CHAKRA_API JsTTDPreExecuteSnapShotInterval(_In_ int64_t startSnapTime, _In_ int64_t endSnapTime, _In_ JsTTDMoveMode moveMode)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else

    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, true);
    if(cCheck != JsNoError)
    {
        return cCheck;
    }

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext * threadContext = scriptContext->GetThreadContext();

    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorFatal;
    }

    TTD::EventLog* elog = threadContext->TTDLog;
    JsErrorCode res = JsNoError;

    elog->ClearBPScanList();
    elog->PushMode(TTD::TTDMode::DebuggerSuppressBreakpoints);
    elog->PushMode(TTD::TTDMode::DebuggerLogBreakpoints);
    try
    {
        elog->PushMode(TTD::TTDMode::ExcludedExecution);
        BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
        {
            elog->DoSnapshotInflate(startSnapTime);
        }
        END_JS_RUNTIME_CALL(scriptContext);
        elog->PopMode(TTD::TTDMode::ExcludedExecution);

        if(endSnapTime == -1)
        {
            elog->ReplayFullTrace();
        }
        else
        {
            elog->ReplayToTime(endSnapTime);
        }
    }
    catch(TTD::TTDebuggerAbortException abortException)
    {
        //
        //TODO: we need to clear out the exception state if needed otherwise when we try to re-enter execution we will fail
        //

        //If we hit the end of the log or we hit a terminal exception that is fine -- anything else is a problem
        if(!abortException.IsEndOfLog() && !abortException.IsTopLevelException())
        {
            res = JsErrorFatal;
        }
    }
    catch(...) //we are replaying something that should be known to execute successfully so encountering any error is very bad
    {
        res = JsErrorFatal;
        AssertMsg(false, "Unexpected fatal Error");
    }
    elog->PopMode(TTD::TTDMode::DebuggerLogBreakpoints);
    elog->PopMode(TTD::TTDMode::DebuggerSuppressBreakpoints);

    if((moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalForContinue) == JsTTDMoveMode::JsTTDMoveScanIntervalForContinue)
    {
        elog->TryFindAndSetPreviousBP();
    }
    elog->ClearBPScanList();

    return res;
#endif
}

CHAKRA_API JsTTDMoveToTopLevelEvent(_In_ JsTTDMoveMode moveMode, _In_ int64_t snapshotTime, _In_ int64_t eventTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else

    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, true);
    if(cCheck != JsNoError)
    {
        return cCheck;
    }

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext * threadContext = scriptContext->GetThreadContext();

    if (threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorFatal;
    }

    TTD::EventLog* elog = threadContext->TTDLog;
    JsErrorCode res = JsNoError;

    elog->PushMode(TTD::TTDMode::DebuggerSuppressBreakpoints);
    try
    {
        elog->PushMode(TTD::TTDMode::ExcludedExecution);
        BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
        {
            elog->DoSnapshotInflate(snapshotTime);
        }
        END_JS_RUNTIME_CALL(scriptContext);
        elog->PopMode(TTD::TTDMode::ExcludedExecution);

        elog->ReplayToTime(eventTime);

        elog->PushMode(TTD::TTDMode::ExcludedExecution);
        BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
        {
            elog->DoRtrSnapIfNeeded();
        }
        END_JS_RUNTIME_CALL(scriptContext);
        elog->PopMode(TTD::TTDMode::ExcludedExecution);
    }
    catch(...) //we are replaying something that should be known to execute successfully so encountering any error is very bad
    {
        res = JsErrorFatal;
        AssertMsg(false, "Unexpected fatal Error");
    }
    elog->PopMode(TTD::TTDMode::DebuggerSuppressBreakpoints);

    return res;
#endif
}

CHAKRA_API JsTTDReplayExecution(_Inout_ JsTTDMoveMode* moveMode, _Inout_ int64_t* rootEventTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, true);
    if(cCheck != JsNoError)
    {
        return cCheck;
    }

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();

    if (threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorCategoryUsage;
    }

    TTD::EventLog* elog = threadContext->TTDLog;

    if((*moveMode & JsTTDMoveMode::JsTTDMoveBreakOnEntry) == JsTTDMoveMode::JsTTDMoveBreakOnEntry)
    {
        elog->SetBreakOnFirstUserCode();
    }

    //reset any breakpoints that we preserved accross a TTD move
    if(elog->HasPendingTTDBP() || elog->GetRestoreBPListAfterContextRecreate().Count() != 0)
    {
        GlobalAPIWrapper([&]() -> JsErrorCode {
            JsrtDebugManager* jsrtDebugManager = currentContext->GetRuntime()->GetJsrtDebugManager();

            const JsUtil::List<TTD::TTDebuggerSourceLocation, HeapAllocator>& bplist = elog->GetRestoreBPListAfterContextRecreate();
            for(int32 i = 0; i < bplist.Count(); ++i)
            {
                const TTD::TTDebuggerSourceLocation& bpLocation = bplist.Item(i);

                Js::FunctionBody* body = bpLocation.ResolveAssociatedSourceInfo(scriptContext);
                Js::Utf8SourceInfo* utf8SourceInfo = body->GetUtf8SourceInfo();

                bool isNewBP = false;
                jsrtDebugManager->SetBreakpointHelper_TTD(scriptContext, utf8SourceInfo, bpLocation.GetLine(), bpLocation.GetColumn(), &isNewBP);
            }
            elog->UnLoadBPListAfterMoveForContextRecreate();

            //Handle the pending BP as/if needed
            if(elog->HasPendingTTDBP())
            {
                TTD::TTDebuggerSourceLocation bpLocation;
                elog->GetPendingTTDBPInfo(bpLocation);

                Js::FunctionBody* body = bpLocation.ResolveAssociatedSourceInfo(scriptContext);
                Js::Utf8SourceInfo* utf8SourceInfo = body->GetUtf8SourceInfo();

                bool isNewBP = false;
                Js::BreakpointProbe* probe = jsrtDebugManager->SetBreakpointHelper_TTD(scriptContext, utf8SourceInfo, bpLocation.GetLine(), bpLocation.GetColumn(),  &isNewBP);

                if(probe != nullptr)
                {
                    elog->SetActiveBP(probe->GetId(), isNewBP, bpLocation);
                }

                //Finally clear the pending BP info so we don't get confused later
                elog->ClearPendingTTDBPInfo();
            }

            return JsNoError;
        });
    }

    //If the log has a BP requested then we should set the actual bp here
    if(elog->HasPendingTTDBP())
    {
        TTD::TTDebuggerSourceLocation bpLocation;
        elog->GetPendingTTDBPInfo(bpLocation);

        Js::FunctionBody* body = bpLocation.ResolveAssociatedSourceInfo(scriptContext);
        Js::Utf8SourceInfo* utf8SourceInfo = body->GetUtf8SourceInfo();

        charcount_t charPosition;
        charcount_t byteOffset;
        utf8SourceInfo->GetCharPositionForLineInfo((charcount_t)bpLocation.GetLine(), &charPosition, &byteOffset);
        long ibos = charPosition + bpLocation.GetColumn() + 1;

        Js::DebugDocument* debugDocument = utf8SourceInfo->GetDebugDocument();

        Js::StatementLocation statement;
        BOOL stmtok = debugDocument->GetStatementLocation(ibos, &statement);
        AssertMsg(stmtok, "We have a bad line for setting a breakpoint.");

        // Don't see a use case for supporting multiple breakpoints at same location.
        // If a breakpoint already exists, just return that
        Js::BreakpointProbe* probe = debugDocument->FindBreakpoint(statement);
        bool isNewBP = (probe == nullptr);

        if(probe == nullptr)
        {
            BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
            {
                probe = debugDocument->SetBreakPoint(statement, BREAKPOINT_ENABLED);
                AssertMsg(probe != nullptr, "We have a bad line or something for setting a breakpoint.");
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }

        elog->SetActiveBP(probe->GetId(), isNewBP, bpLocation);

        //Finally clear the pending BP info so we don't get confused later
        elog->ClearPendingTTDBPInfo();
    }

    JsErrorCode res = JsNoError;
    try
    {
        elog->ReplayFullTrace();
    }
    catch(TTD::TTDebuggerAbortException abortException)
    {
        //if the debugger bails out with a move time request set info on the requested event time here
        //rest of breakpoint info should have been set by the debugger callback before aborting
        if (abortException.IsEventTimeMove() || abortException.IsTopLevelException())
        {
            *moveMode = (JsTTDMoveMode)abortException.GetMoveMode();
            *rootEventTime = abortException.GetTargetEventTime();

            if(abortException.IsTopLevelException())
            {
                //
                //TODO: we need to clear out the exception state if needed otherwise when we try to re-enter execution we will fail
                //

                bool markedAsJustMyCode = false;
                TTD::TTDebuggerSourceLocation throwLocation;
                elog->GetLastExecutedTimeAndPositionForDebugger(&markedAsJustMyCode, throwLocation);

                elog->SetPendingTTDBPInfo(throwLocation);
            }
        }
        else
        {
            *moveMode = JsTTDMoveMode::JsTTDMoveNone;
            *rootEventTime = -1;
        }

        res = abortException.IsTopLevelException() ? JsErrorCategoryScript : JsNoError;
    }
    catch(...)
    {
        res = JsErrorFatal;
        AssertMsg(false, "Unexpected fatal Error");
    }

    return res;
#endif
}
