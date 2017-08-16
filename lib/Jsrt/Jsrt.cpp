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
#include "Library/JavascriptExceptionMetadata.h"
#include "Library/JavascriptSymbol.h"
#include "Library/JavascriptPromise.h"
#include "Base/ThreadContextTlsEntry.h"
#include "Codex/Utf8Helper.h"

// Parser Includes
#include "cmperr.h"     // For ERRnoMemory
#include "screrror.h"   // For CompileScriptException

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#include "TestHooksRt.h"
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

JsErrorCode CheckContext(JsrtContext *currentContext, bool verifyRuntimeState,
    bool allowInObjectBeforeCollectCallback)
{
    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    // We don't need parameter check if it's checked in previous wrapper.
    if (verifyRuntimeState)
    {
        Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
        Assert(scriptContext != nullptr);
        Recycler *recycler = scriptContext->GetRecycler();
        ThreadContext *threadContext = scriptContext->GetThreadContext();

        if (recycler && recycler->IsHeapEnumInProgress())
        {
            return JsErrorHeapEnumInProgress;
        }
        else if (!allowInObjectBeforeCollectCallback &&
            recycler && recycler->IsInObjectBeforeCollectCallback())
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

#if ENABLE_TTD
void CALLBACK OnScriptLoad_TTDCallback(FinalizableObject* jsrtCtx, Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify)
{
    ((JsrtContext*)jsrtCtx)->OnScriptLoad_TTDCallback(body, utf8SourceInfo, compileException, notify);
}

uint32 CALLBACK OnBPRegister_TTDCallback(void* runtimeRcvr, int64 bpID, Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, uint32 line, uint32 column, BOOL* isNewBP)
{
    return ((JsrtRuntime*)runtimeRcvr)->BPRegister_TTD(bpID, scriptContext, utf8SourceInfo, line, column, isNewBP);
}

void CALLBACK OnBPDelete_TTDCallback(void* runtimeRcvr, uint32 bpID)
{
    ((JsrtRuntime*)runtimeRcvr)->BPDelete_TTD(bpID);
}

void CALLBACK OnBPClearDocument_TTDCallback(void* runtimeRcvr)
{
    ((JsrtRuntime*)runtimeRcvr)->BPClearDocument_TTD();
}
#endif

//A create context function that we can funnel to for regular and record or debug aware creation
JsErrorCode CreateContextCore(_In_ JsRuntimeHandle runtimeHandle, _In_ TTDRecorder& _actionEntryPopper, _In_ bool inRecordMode, _In_ bool activelyRecording, _In_ bool inReplayMode, _Out_ JsContextRef *newContext)
{
    JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext * threadContext = runtime->GetThreadContext();

    if(threadContext->GetRecycler() && threadContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if(threadContext->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    ThreadContextScope scope(threadContext);

    if(!scope.IsValid())
    {
        return JsErrorWrongThread;
    }

#if ENABLE_TTD
    TTD::NSLogEvents::EventLogEntry* createEvent = nullptr;
    if(activelyRecording)
    {
        createEvent = threadContext->TTDLog->RecordJsRTCreateScriptContext(_actionEntryPopper);
    }
#endif

    JsrtContext * context = JsrtContext::New(runtime);

#if ENABLE_TTD
    if(inRecordMode | inReplayMode)
    {
        Js::ScriptContext* scriptContext = context->GetScriptContext();
        HostScriptContextCallbackFunctor callbackFunctor((FinalizableObject*)context, (void*)runtime, &OnScriptLoad_TTDCallback, &OnBPRegister_TTDCallback, &OnBPDelete_TTDCallback, &OnBPClearDocument_TTDCallback);

#if ENABLE_TTD_DIAGNOSTICS_TRACING
        bool noNative = true;
        bool doDebug = true;
#else
        bool noNative = TTD_FORCE_NOJIT_MODE || threadContext->TTDLog->IsDebugModeFlagSet();
        bool doDebug = TTD_FORCE_DEBUG_MODE || threadContext->TTDLog->IsDebugModeFlagSet();
#endif

        threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecutionTTAction);
        if(inRecordMode)
        {
            threadContext->TTDContext->AddNewScriptContextRecord(context, scriptContext, callbackFunctor, noNative, doDebug);
        }
        else
        {
            threadContext->TTDContext->AddNewScriptContextReplay(context, scriptContext, callbackFunctor, noNative, doDebug);
        }

        threadContext->TTDLog->SetModeFlagsOnContext(scriptContext);
        threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecutionTTAction);
    }
#endif

    JsrtDebugManager* jsrtDebugManager = runtime->GetJsrtDebugManager();

    if(jsrtDebugManager != nullptr)
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

#if ENABLE_TTD
    if(activelyRecording)
    {
        threadContext->TTDLog->RecordJsRTCreateScriptContextResult(createEvent, context->GetScriptContext());
    }
#endif

    *newContext = (JsContextRef)context;
    return JsNoError;
}

#if ENABLE_TTD
void CALLBACK CreateExternalObject_TTDCallback(Js::ScriptContext* ctx, Js::Var* object)
{
    TTDAssert(object != nullptr, "This should always be a valid location");

    *object = JsrtExternalObject::Create(nullptr, nullptr, ctx);
}

void CALLBACK TTDDummyPromiseContinuationCallback(JsValueRef task, void *callbackState)
{
    TTDAssert(false, "This should never actually be invoked!!!");
}

void CALLBACK CreateJsRTContext_TTDCallback(void* runtimeHandle, Js::ScriptContext** result)
{
    JsContextRef newContext = nullptr;
    *result = nullptr;

    TTDRecorder dummyActionEntryPopper;
    JsErrorCode err = CreateContextCore(static_cast<JsRuntimeHandle>(runtimeHandle), dummyActionEntryPopper, false /*inRecordMode*/, false /*activelyRecording*/, true /*inReplayMode*/, &newContext);
    TTDAssert(err == JsNoError, "Shouldn't fail on us!!!");

    *result = static_cast<JsrtContext*>(newContext)->GetScriptContext();
    (*result)->GetLibrary()->SetNativeHostPromiseContinuationFunction((Js::JavascriptLibrary::PromiseContinuationCallback)TTDDummyPromiseContinuationCallback, nullptr);

    //To ensure we have a valid context active (when we next try and inflate into this context) set this as active by convention
    JsrtContext::TrySetCurrent(static_cast<JsrtContext*>(newContext));
}

void CALLBACK ReleaseJsRTContext_TTDCallback(FinalizableObject* jsrtCtx)
{
    static_cast<JsrtContext*>(jsrtCtx)->GetScriptContext()->GetThreadContext()->GetRecycler()->RootRelease(jsrtCtx);
    JsrtContext::OnReplayDisposeContext_TTDCallback(jsrtCtx);
}

void CALLBACK SetActiveJsRTContext_TTDCallback(void* runtimeHandle, Js::ScriptContext* ctx)
{
    JsrtRuntime * runtime = JsrtRuntime::FromHandle(static_cast<JsRuntimeHandle>(runtimeHandle));
    ThreadContext * threadContext = runtime->GetThreadContext();

    threadContext->TTDContext->SetActiveScriptContext(ctx);
    JsrtContext* runtimeCtx = (JsrtContext*)threadContext->TTDContext->GetRuntimeContextForScriptContext(ctx);
    JsrtContext::TrySetCurrent(runtimeCtx);
}
#endif

//A create runtime function that we can funnel to for regular and record or debug aware creation
JsErrorCode CreateRuntimeCore(_In_ JsRuntimeAttributes attributes,
    _In_opt_ const char* optTTUri, size_t optTTUriCount, bool isRecord, bool isReplay, bool isDebug,
    _In_ UINT32 snapInterval, _In_ UINT32 snapHistoryLength,
    _In_opt_ TTDOpenResourceStreamCallback openResourceStream, _In_opt_ JsTTDReadBytesFromStreamCallback readBytesFromStream,
    _In_opt_ JsTTDWriteBytesToStreamCallback writeBytesToStream, _In_opt_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
    _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    VALIDATE_ENTER_CURRENT_THREAD();

    PARAM_NOT_NULL(runtimeHandle);
    *runtimeHandle = nullptr;

    JsErrorCode runtimeResult = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
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

#if ENABLE_TTD
    if(runtimeResult != JsNoError)
    {
        return runtimeResult;
    }

    if(isRecord | isReplay | isDebug)
    {
        ThreadContext* threadContext = JsrtRuntime::FromHandle(*runtimeHandle)->GetThreadContext();

        if(isRecord && isReplay)
        {
            return JsErrorInvalidArgument; //A runtime can only be in 1 mode
        }

        if(isDebug && !isReplay)
        {
            return JsErrorInvalidArgument; //A debug runtime also needs to be in runtime mode (and we are going to be strict about it)
        }

        if(isReplay && optTTUri == nullptr)
        {
            return JsErrorInvalidArgument; //We must have a location to store data into
        }

        runtimeResult = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
            //Make sure the thread context recycler is allocated before we do anything else
            ThreadContextScope scope(threadContext);
            threadContext->EnsureRecycler();

            threadContext->InitTimeTravel(threadContext, *runtimeHandle, snapInterval, max<uint32>(2, snapHistoryLength));
            threadContext->InitHostFunctionsAndTTData(isRecord, isReplay, isDebug, optTTUriCount, optTTUri,
                openResourceStream, readBytesFromStream, writeBytesToStream, flushAndCloseStream,
                &CreateExternalObject_TTDCallback, &CreateJsRTContext_TTDCallback, &ReleaseJsRTContext_TTDCallback, &SetActiveJsRTContext_TTDCallback);

            return JsNoError;
        });
    }
#endif

    return runtimeResult;
}

/////////////////////

CHAKRA_API JsCreateRuntime(_In_ JsRuntimeAttributes attributes, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    return CreateRuntimeCore(attributes,
        nullptr /*optRecordUri*/, 0 /*optRecordUriCount */, false /*isRecord*/, false /*isReplay*/, false /*isDebug*/,
        UINT_MAX /*optSnapInterval*/, UINT_MAX /*optLogLength*/,
        nullptr, nullptr, nullptr, nullptr, /*TTD IO handlers*/
        threadService, runtimeHandle);
}

template <CollectionFlags flags>
JsErrorCode JsCollectGarbageCommon(JsRuntimeHandle runtimeHandle)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
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
    return GlobalAPIWrapper_NoRecord([&] () -> JsErrorCode {
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

        Js::ScriptContext *scriptContext;
        for (scriptContext = threadContext->GetScriptContextList(); scriptContext; scriptContext = scriptContext->next)
        {
            if (runtime->GetJsrtDebugManager() != nullptr)
            {
                runtime->GetJsrtDebugManager()->ClearDebugDocument(scriptContext);
            }
            scriptContext->MarkForClose();
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
        return GlobalAPIWrapper_NoRecord([&] () -> JsErrorCode
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
        return GlobalAPIWrapper([&] (TTDRecorder& _actionEntryPopper) -> JsErrorCode
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

            if((lCount == 1) && (threadContext->IsRuntimeInTTDMode()) && (!threadContext->TTDLog->IsPropertyRecordRef(ref)))
            {
                Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(ref);
                if(obj->GetScriptContext()->IsTTDRecordModeEnabled())
                {
                    if(obj->GetScriptContext()->ShouldPerformRecordAction())
                    {
                        threadContext->TTDLog->RecordJsRTAddRootRef(_actionEntryPopper, (Js::Var)ref);
                    }

                    threadContext->TTDContext->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(obj), obj);
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
        return GlobalAPIWrapper_NoRecord([&] () -> JsErrorCode
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
        return GlobalAPIWrapper([&](TTDRecorder& _actionEntryPopper) -> JsErrorCode
        {
            // Note, some references may live in arena-allocated memory, so we need to do this check
            if (!recycler->IsValidObject(ref))
            {
                return JsNoError;
            }

            recycler->RootRelease(ref, count);

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
        return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
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
        return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
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
    return GlobalAPIWrapper([&](TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PARAM_NOT_NULL(newContext);
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        bool inRecord = false;
        bool activelyRecording = false;
        bool inReplay = false;

#if ENABLE_TTD
        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();
        if(threadContext->IsRuntimeInTTDMode() && threadContext->TTDContext->GetActiveScriptContext() != nullptr)
        {
            Js::ScriptContext* currentCtx = threadContext->TTDContext->GetActiveScriptContext();
            inRecord = currentCtx->IsTTDRecordModeEnabled();
            activelyRecording = currentCtx->ShouldPerformRecordAction();
            inReplay = currentCtx->IsTTDReplayModeEnabled();
        }
#endif

        return CreateContextCore(runtimeHandle, _actionEntryPopper, inRecord, activelyRecording, inReplay, newContext);
    });
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

    return GlobalAPIWrapper([&] (TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        JsrtContext *currentContext = JsrtContext::GetCurrent();
        Recycler* recycler = currentContext != nullptr ? currentContext->GetScriptContext()->GetRecycler() : nullptr;

#if ENABLE_TTD
        Js::ScriptContext* newScriptContext = newContext != nullptr ? static_cast<JsrtContext*>(newContext)->GetScriptContext() : nullptr;
        Js::ScriptContext* oldScriptContext = currentContext != nullptr ? static_cast<JsrtContext*>(currentContext)->GetScriptContext() : nullptr;

        if(newScriptContext == nullptr)
        {
            if(oldScriptContext == nullptr)
            {
                ; //if newScriptContext and oldScriptContext are null then we don't worry about doing anything
            }
            else
            {
                if(oldScriptContext->IsTTDRecordModeEnabled())
                {
                    //already know newScriptContext != oldScriptContext so don't check again
                    if(oldScriptContext->ShouldPerformRecordAction())
                    {
                        oldScriptContext->GetThreadContext()->TTDLog->RecordJsRTSetCurrentContext(_actionEntryPopper, nullptr);
                    }

                    oldScriptContext->GetThreadContext()->TTDContext->SetActiveScriptContext(nullptr);
                }
            }
        }
        else
        {
            if(newScriptContext->IsTTDRecordModeEnabled())
            {
                if(newScriptContext != oldScriptContext && newScriptContext->ShouldPerformRecordAction())
                {
                    newScriptContext->GetThreadContext()->TTDLog->RecordJsRTSetCurrentContext(_actionEntryPopper, newScriptContext->GetGlobalObject());
                }

                newScriptContext->GetThreadContext()->TTDContext->SetActiveScriptContext(newScriptContext);
            }
        }
#endif

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
        *context = (JsContextRef)obj->GetScriptContext()->GetLibrary()->GetJsrtContext();
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

void HandleScriptCompileError(Js::ScriptContext * scriptContext, CompileScriptException * se, const WCHAR * sourceUrl)
{
    HRESULT hr = se->ei.scode;
    if (hr == E_OUTOFMEMORY || hr == VBSERR_OutOfMemory || hr == VBSERR_OutOfStack || hr == ERRnoMemory)
    {
        Js::Throw::OutOfMemory();
    }

    Js::JavascriptError* error = Js::JavascriptError::CreateFromCompileScriptException(scriptContext, se, sourceUrl);

    Js::JavascriptExceptionObject * exceptionObject = RecyclerNew(scriptContext->GetRecycler(),
        Js::JavascriptExceptionObject, error, scriptContext, nullptr);

    scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
}

CHAKRA_API JsGetUndefinedValue(_Out_ JsValueRef *undefinedValue)
{
    return ContextAPINoScriptWrapper_NoRecord([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(undefinedValue);

        *undefinedValue = scriptContext->GetLibrary()->GetUndefined();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetNullValue(_Out_ JsValueRef *nullValue)
{
    return ContextAPINoScriptWrapper_NoRecord([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(nullValue);

        *nullValue = scriptContext->GetLibrary()->GetNull();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetTrueValue(_Out_ JsValueRef *trueValue)
{
    return ContextAPINoScriptWrapper_NoRecord([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(trueValue);

        *trueValue = scriptContext->GetLibrary()->GetTrue();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsGetFalseValue(_Out_ JsValueRef *falseValue)
{
    return ContextAPINoScriptWrapper_NoRecord([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(falseValue);

        *falseValue = scriptContext->GetLibrary()->GetFalse();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsBoolToBoolean(_In_ bool value, _Out_ JsValueRef *booleanValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateBoolean, value);

        PARAM_NOT_NULL(booleanValue);

        *booleanValue = value ? scriptContext->GetLibrary()->GetTrue() : scriptContext->GetLibrary()->GetFalse();

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, booleanValue);

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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTVarToBooleanConversion, (Js::Var)value);

        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        if (Js::JavascriptConversion::ToBool((Js::Var)value, scriptContext))
        {
            *result = scriptContext->GetLibrary()->GetTrue();
        }
        else
        {
            *result = scriptContext->GetLibrary()->GetFalse();
        }

        //It is either true or false which we always track so no need to store result identity

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
    //If number is not heap allocated then we don't need to record/track the creation for time-travel
    if (Js::JavascriptNumber::TryToVarFastWithCheck(dbl, asValue))
    {
      return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateNumber, dbl);

        *asValue = Js::JavascriptNumber::ToVarNoCheck(dbl, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, asValue);

        return JsNoError;
    });
}

CHAKRA_API JsIntToNumber(_In_ int intValue, _Out_ JsValueRef *asValue)
{
    PARAM_NOT_NULL(asValue);
    //If number is not heap allocated then we don't need to record/track the creation for time-travel
    if (Js::JavascriptNumber::TryToVarFast(intValue, asValue))
    {
        return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
#if !INT32VAR
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateInteger, intValue);
#endif

        *asValue = Js::JavascriptNumber::ToVar(intValue, scriptContext);

#if !INT32VAR
        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, asValue);
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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTVarToNumberConversion, (Js::Var)value);

        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        *result = (JsValueRef)Js::JavascriptOperators::ToNumber((Js::Var)value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

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

CHAKRA_API JsPointerToString(_In_reads_(stringLength) const WCHAR *stringValue, _In_ size_t stringLength, _Out_ JsValueRef *string)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateString, stringValue, stringLength);

        PARAM_NOT_NULL(stringValue);
        PARAM_NOT_NULL(string);

        if (!Js::IsValidCharCount(stringLength))
        {
            Js::JavascriptError::ThrowOutOfMemoryError(scriptContext);
        }

        *string = Js::JavascriptString::NewCopyBuffer(stringValue, static_cast<charcount_t>(stringLength), scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, string);

        return JsNoError;
    });
}

// TODO: The annotation of stringPtr is wrong.  Need to fix definition in chakrart.h
// The warning is '*stringPtr' could be '0' : this does not adhere to the specification for the function 'JsStringToPointer'.
#pragma warning(suppress:6387)
CHAKRA_API JsStringToPointer(_In_ JsValueRef stringValue, _Outptr_result_buffer_(*stringLength) const WCHAR **stringPtr, _Out_ size_t *stringLength)
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

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        Js::JavascriptString *jsString = Js::JavascriptString::FromVar(stringValue);

        *stringPtr = jsString->GetSz();
        *stringLength = jsString->GetLength();
        return JsNoError;
    });
}

CHAKRA_API JsConvertValueToString(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTVarToStringConversion, (Js::Var)value);

        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        *result = (JsValueRef) Js::JavascriptConversion::ToString((Js::Var)value, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsGetGlobalObject(_Out_ JsValueRef *globalObject)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(globalObject);

        *globalObject = (JsValueRef)scriptContext->GetGlobalObject();
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

CHAKRA_API JsCreateObject(_Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateBasicObject);

        PARAM_NOT_NULL(object);

        *object = scriptContext->GetLibrary()->CreateObject();

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, object);

        return JsNoError;
    });
}

CHAKRA_API JsCreateExternalObject(_In_opt_ void *data, _In_opt_ JsFinalizeCallback finalizeCallback, _Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateExternalObject);

        PARAM_NOT_NULL(object);

        *object = JsrtExternalObject::Create(data, finalizeCallback, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, object);

        return JsNoError;
    });
}

CHAKRA_API JsConvertValueToObject(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTVarToObjectConversion, (Js::Var)value);

        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        *result = (JsValueRef)Js::JavascriptOperators::ToObject((Js::Var)value, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsGetPrototype(_In_ JsValueRef object, _Out_ JsValueRef *prototypeObject)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetPrototype, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(prototypeObject);

        *prototypeObject = (JsValueRef)Js::JavascriptOperators::OP_GetPrototype(object, scriptContext);
        Assert(*prototypeObject == nullptr || !Js::CrossSite::NeedMarshalVar(*prototypeObject, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, prototypeObject);

        return JsNoError;
    });
}

CHAKRA_API JsSetPrototype(_In_ JsValueRef object, _In_ JsValueRef prototypeObject)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTSetPrototype, object, prototypeObject);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_OBJECT_OR_NULL(prototypeObject, scriptContext);

        // We're not allowed to set this.
        if (object == scriptContext->GetLibrary()->GetObjectPrototype())
        {
            return JsErrorInvalidArgument;
        }

        Js::JavascriptObject::ChangePrototype(Js::RecyclableObject::FromVar(object), Js::RecyclableObject::FromVar(prototypeObject), true, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsInstanceOf(_In_ JsValueRef object, _In_ JsValueRef constructor, _Out_ bool *result) {
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTInstanceOf, object, constructor);

        VALIDATE_INCOMING_REFERENCE(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(constructor, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::RecyclableObject::FromVar(constructor)->HasInstance(object, scriptContext) ? true : false;

        return JsNoError;
    });
}

CHAKRA_API JsGetExtensionAllowed(_In_ JsValueRef object, _Out_ bool *value)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(value);
        *value = false;

        *value = Js::RecyclableObject::FromVar(object)->IsExtensible() != 0;

        return JsNoError;
    });
}

CHAKRA_API JsPreventExtension(_In_ JsValueRef object)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);

        Js::RecyclableObject::FromVar(object)->PreventExtensions();

        return JsNoError;
    });
}

CHAKRA_API JsGetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *value)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetProperty, (Js::PropertyRecord *)propertyId, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(value);
        *value = nullptr;

        *value = Js::JavascriptOperators::OP_GetProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext);
        Assert(*value == nullptr || !Js::CrossSite::NeedMarshalVar(*value, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, value);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertyDescriptor(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *propertyDescriptor)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetOwnPropertyInfo, (Js::PropertyRecord *)propertyId, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(propertyDescriptor);
        *propertyDescriptor = nullptr;

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

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, propertyDescriptor);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertyNames(_In_ JsValueRef object, _Out_ JsValueRef *propertyNames)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetOwnPropertyNamesInfo, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertyNames);
        *propertyNames = nullptr;

        *propertyNames = Js::JavascriptOperators::GetOwnPropertyNames(object, scriptContext);
        Assert(*propertyNames == nullptr || !Js::CrossSite::NeedMarshalVar(*propertyNames, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, propertyNames);

        return JsNoError;
    });
}

CHAKRA_API JsGetOwnPropertySymbols(_In_ JsValueRef object, _Out_ JsValueRef *propertySymbols)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetOwnPropertySymbolsInfo, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertySymbols);

        *propertySymbols = Js::JavascriptOperators::GetOwnPropertySymbols(object, scriptContext);
        Assert(*propertySymbols == nullptr || !Js::CrossSite::NeedMarshalVar(*propertySymbols, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, propertySymbols);

        return JsNoError;
    });
}

CHAKRA_API JsSetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef value, _In_ bool useStrictRules)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTSetProperty, object, (Js::PropertyRecord *)propertyId, value, useStrictRules);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

        Js::JavascriptOperators::OP_SetProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), value, scriptContext,
            nullptr, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);

        return JsNoError;
    });
}

CHAKRA_API JsHasProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ bool *hasProperty)
{
    VALIDATE_JSREF(object);
    if (!Js::JavascriptOperators::IsObject(object)) return JsErrorArgumentNotObject;

    auto internalHasProperty = [&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTHasProperty, (Js::PropertyRecord *)propertyId, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(hasProperty);
        *hasProperty = false;

        *hasProperty = Js::JavascriptOperators::OP_HasProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext) != 0;

        return JsNoError;
    };

    Js::RecyclableObject* robject = Js::RecyclableObject::FromVar(object);
    Js::TypeId typeId = Js::JavascriptOperators::GetTypeId(robject);
    while (typeId != Js::TypeIds_Null && typeId != Js::TypeIds_Proxy)
    {
        robject = robject->GetPrototype();
        typeId = Js::JavascriptOperators::GetTypeId(robject);
    }

    if (typeId == Js::TypeIds_Proxy)
    {
        return ContextAPIWrapper<JSRT_MAYBE_TRUE>(internalHasProperty);
    }
    else
    {
        return ContextAPINoScriptWrapper(internalHasProperty);
    }
}

CHAKRA_API JsDeleteProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ bool useStrictRules, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTDeleteProperty, object, (Js::PropertyRecord *)propertyId, useStrictRules);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        *result = Js::JavascriptOperators::OP_DeleteProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(),
            scriptContext, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsDefineProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef propertyDescriptor, _Out_ bool *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTDefineProperty, object, (Js::PropertyRecord *)propertyId, propertyDescriptor);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        VALIDATE_INCOMING_OBJECT(propertyDescriptor, scriptContext);
        PARAM_NOT_NULL(result);
        *result = false;

        Js::PropertyDescriptor propertyDescriptorValue;
        if (!Js::JavascriptOperators::ToPropertyDescriptor(propertyDescriptor, &propertyDescriptorValue, scriptContext))
        {
            return JsErrorInvalidArgument;
        }

        *result = Js::JavascriptOperators::DefineOwnPropertyDescriptor(
            Js::RecyclableObject::FromVar(object), ((Js::PropertyRecord *)propertyId)->GetPropertyId(), propertyDescriptorValue,
            true, scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsCreateArray(_In_ unsigned int length, _Out_ JsValueRef *result)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateBasicArray, length);

        PARAM_NOT_NULL(result);
        *result = nullptr;

        *result = scriptContext->GetLibrary()->CreateArray(length);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsCreateArrayBuffer(_In_ unsigned int byteLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateArrayBuffer, byteLength);

        PARAM_NOT_NULL(result);

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateArrayBuffer(byteLength);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

#ifdef _CHAKRACOREBUILD
CHAKRA_API JsCreateSharedArrayBufferWithSharedContent(_In_ JsSharedArrayBufferContentHandle sharedContents, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        PARAM_NOT_NULL(result);

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateSharedArrayBuffer((Js::SharedContents*)sharedContents);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsGetSharedArrayBufferContent(_In_ JsValueRef sharedArrayBuffer, _Out_ JsSharedArrayBufferContentHandle *sharedContents)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {

        PARAM_NOT_NULL(sharedContents);

        if (!Js::SharedArrayBuffer::Is(sharedArrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

        Js::SharedContents**& content = (Js::SharedContents**&)sharedContents;
        *content = Js::SharedArrayBuffer::FromVar(sharedArrayBuffer)->GetSharedContents();

        if (*content == nullptr)
        {
            return JsErrorFatal;
        }

        (*content)->AddRef();

        return JsNoError;
    });
}

CHAKRA_API JsReleaseSharedArrayBufferContentHandle(_In_ JsSharedArrayBufferContentHandle sharedContents)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        ((Js::SharedContents*)sharedContents)->Release();
        return JsNoError;
    });
}
#endif // _CHAKRACOREBUILD

CHAKRA_API JsCreateExternalArrayBuffer(_Pre_maybenull_ _Pre_writable_byte_size_(byteLength) void *data, _In_ unsigned int byteLength,
    _In_opt_ JsFinalizeCallback finalizeCallback, _In_opt_ void *callbackState, _Out_ JsValueRef *result)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateExternalArrayBuffer, reinterpret_cast<BYTE*>(data), byteLength);

        PARAM_NOT_NULL(result);

        if (data == nullptr && byteLength > 0)
        {
            return JsErrorInvalidArgument;
        }

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = Js::JsrtExternalArrayBuffer::New(
            reinterpret_cast<BYTE*>(data),
            byteLength,
            finalizeCallback,
            callbackState,
            library->GetArrayBufferType());

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

CHAKRA_API JsCreateTypedArray(_In_ JsTypedArrayType arrayType, _In_ JsValueRef baseArray, _In_ unsigned int byteOffset,
    _In_ unsigned int elementLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_REFERENCE(arrayBuffer, scriptContext);
        PARAM_NOT_NULL(result);

        if (!Js::ArrayBuffer::Is(arrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

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
    Js::ScriptContext* scriptContext = Js::RecyclableObject::FromVar(typedArray)->GetScriptContext();
    if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext) && arrayBuffer != nullptr)
    {
        scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetTypedArrayInfo(typedArray, *arrayBuffer);
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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateSymbol, description);

        PARAM_NOT_NULL(result);
        *result = nullptr;

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

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsHasIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _Out_ bool *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetIndex, index, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        *result = (JsValueRef)Js::JavascriptOperators::OP_GetElementI((Js::Var)object, (Js::Var)index, scriptContext);

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsSetIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _In_ JsValueRef value)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTSetIndex, object, index, value);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

        Js::JavascriptOperators::OP_SetElementI((Js::Var)object, (Js::Var)index, (Js::Var)value, scriptContext);

        return JsNoError;
    });
}

CHAKRA_API JsDeleteIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);

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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTEquals, object1, object2, false);

        VALIDATE_INCOMING_REFERENCE(object1, scriptContext);
        VALIDATE_INCOMING_REFERENCE(object2, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::JavascriptOperators::Equal((Js::Var)object1, (Js::Var)object2, scriptContext) != 0;
        return JsNoError;
    });
}

CHAKRA_API JsStrictEquals(_In_ JsValueRef object1, _In_ JsValueRef object2, _Out_ bool *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTEquals, object1, object2, true);

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

    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
#if ENABLE_TTD
        TTD::TTDJsRTFunctionCallActionPopperRecorder callInfoPopper;
        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            TTD::NSLogEvents::EventLogEntry* callEvent = scriptContext->GetThreadContext()->TTDLog->RecordJsRTCallFunction(_actionEntryPopper, scriptContext->GetThreadContext()->TTDRootNestingCount, function, cargs, args);
            callInfoPopper.InitializeForRecording(scriptContext, scriptContext->GetThreadContext()->TTDLog->GetCurrentWallTime(), callEvent);

            if(scriptContext->GetThreadContext()->TTDRootNestingCount == 0)
            {
                TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
                elog->ResetCallStackForTopLevelCall(elog->GetLastEventTime());

                TTD::ExecutionInfoManager* emanager = scriptContext->GetThreadContext()->TTDExecutionInfo;
                if(emanager != nullptr)
                {
                    emanager->ResetCallStackForTopLevelCall(elog->GetLastEventTime());
                }
            }
        }
#endif

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

        Js::Var varResult = jsFunction->CallRootFunction(jsArgs, scriptContext, true);
        if(result != nullptr)
        {
            *result = varResult;
            Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));
        }

#if ENABLE_TTD
        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            _actionEntryPopper.SetResult(result);
        }
#endif

        return JsNoError;
    });
}

CHAKRA_API JsConstructObject(_In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTConstructCall, function, cargs, args);

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
        //TTDAssert(!Js::ScriptFunction::Is(jsFunction) || execContext->GetThreadContext()->TTDRootNestingCount != 0, "This will cause user code to execute and we need to add support for that as a top-level call source!!!!");
        //

        *result = Js::JavascriptFunction::CallAsConstructor(jsFunction, /* overridingNewTarget = */nullptr, jsArgs, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, result);

        return JsNoError;
    });
}

CHAKRA_API JsCreateFunction(_In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateFunction, false, nullptr);

        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;

        Js::JavascriptExternalFunction *externalFunction = scriptContext->GetLibrary()->CreateStdCallExternalFunction((Js::StdCallJavascriptMethod)nativeFunction, 0, callbackState);
        *function = (JsValueRef)externalFunction;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, function);

        return JsNoError;
    });
}

CHAKRA_API JsCreateNamedFunction(_In_ JsValueRef name, _In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTAllocateFunction, true, name);

        VALIDATE_INCOMING_REFERENCE(name, scriptContext);
        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;

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

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, function);

        return JsNoError;
    });
}

void SetErrorMessage(Js::ScriptContext *scriptContext, JsValueRef newError, JsValueRef message)
{
    Js::JavascriptOperators::OP_SetProperty(newError, Js::PropertyIds::message, message, scriptContext);
}

CHAKRA_API JsCreateError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateRangeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateRangeError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateRangeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateReferenceError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateReferenceError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateReferenceError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateSyntaxError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateSyntaxError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateSyntaxError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateTypeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateTypeError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateTypeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

        return JsNoError;
    });
}

CHAKRA_API JsCreateURIError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&] (Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTCreateURIError, message);

        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError = scriptContext->GetLibrary()->CreateURIError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, error);

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
    Recycler *recycler = scriptContext->GetRecycler();
    ThreadContext *threadContext = scriptContext->GetThreadContext();

#ifndef JSRT_VERIFY_RUNTIME_STATE
    if (recycler && recycler->IsInObjectBeforeCollectCallback())
    {
        return JsErrorInObjectBeforeCollectCallback;
    }
#endif

    if (recycler && recycler->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (threadContext->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    if (threadContext->IsExecutionDisabled())
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
      if (scriptContext->HasRecordedException())
      {
          recordedException = scriptContext->GetAndClearRecordedException();
      }
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

#if ENABLE_TTD
    if(hr != E_OUTOFMEMORY)
    {
        TTD::TTDJsRTActionResultAutoRecorder _actionEntryPopper;

        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetAndClearException);
        PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, exception);
    }
#endif

    if (*exception == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    return JsNoError;
}

CHAKRA_API JsSetException(_In_ JsValueRef exception)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        JsrtContext * context = JsrtContext::GetCurrent();
        JsrtRuntime * runtime = context->GetRuntime();

        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTSetException, exception, runtime->DispatchExceptions());

        VALIDATE_INCOMING_REFERENCE(exception, scriptContext);

        Js::JavascriptExceptionObject *exceptionObject;
        exceptionObject = RecyclerNew(scriptContext->GetRecycler(), Js::JavascriptExceptionObject, exception, scriptContext, nullptr);

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
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
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
    return GlobalAPIWrapper_NoRecord([&] () -> JsErrorCode {
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

CHAKRA_API JsGetPropertyIdFromName(_In_z_ const WCHAR *name, _Out_ JsPropertyIdRef *propertyId)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
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

CHAKRA_API JsGetPropertyIdFromSymbol(_In_ JsValueRef symbol, _Out_ JsPropertyIdRef *propertyId)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetPropertyIdFromSymbol, symbol);

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
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

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
CHAKRA_API JsGetPropertyNameFromId(_In_ JsPropertyIdRef propertyId, _Outptr_result_z_ const WCHAR **name)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
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

CHAKRA_API JsGetPropertyIdType(_In_ JsPropertyIdRef propertyId, _Out_ JsPropertyIdType* propertyIdType)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
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

    return ContextAPINoScriptWrapper_NoRecord([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {

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

CHAKRA_API JsSetPromiseContinuationCallback(_In_opt_ JsPromiseContinuationCallback promiseContinuationCallback, _In_opt_ void *callbackState)
{
    return ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        scriptContext->GetLibrary()->SetNativeHostPromiseContinuationFunction((Js::JavascriptLibrary::PromiseContinuationCallback) promiseContinuationCallback, callbackState);
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

JsErrorCode RunScriptCore(JsValueRef scriptSource, const byte *script, size_t cb,
    LoadScriptFlag loadScriptFlag, JsSourceContext sourceContext,
    const WCHAR *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes,
    bool isSourceModule, JsValueRef *result)
{
    Js::JavascriptFunction *scriptFunction;
    CompileScriptException se;

    JsErrorCode errorCode = ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PARAM_NOT_NULL(script);
        PARAM_NOT_NULL(sourceUrl);

        SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);

        if (sourceContextInfo == nullptr)
        {
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, sourceUrl, wcslen(sourceUrl), nullptr);
        }

        const int chsize = (loadScriptFlag & LoadScriptFlag_Utf8Source) ?
                            sizeof(utf8char_t) : sizeof(WCHAR);

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

#if ENABLE_TTD
        TTD::NSLogEvents::EventLogEntry* parseEvent = nullptr;
        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            parseEvent = scriptContext->GetThreadContext()->TTDLog->RecordJsRTCodeParse(_actionEntryPopper,
              loadScriptFlag, ((loadScriptFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source),
              script, (uint32)cb, sourceContext, sourceUrl);
        }
#endif

        scriptFunction = scriptContext->LoadScript(script, cb,
            &si, &se, &utf8SourceInfo,
            Js::Constants::GlobalCode, loadScriptFlag, scriptSource);

#if ENABLE_TTD
        if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
        {
            _actionEntryPopper.SetResult((Js::Var*)&scriptFunction);
        }

        //
        //TODO: We may (probably?) want to use the debugger source rundown functionality here instead
        //
        if (scriptFunction != nullptr && scriptContext->IsTTDRecordModeEnabled())
        {
            //Make sure we have the body and text information available
            Js::FunctionBody* globalBody = TTD::JsSupport::ForceAndGetFunctionBody(scriptFunction->GetParseableFunctionInfo());

            const TTD::NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* tbfi = scriptContext->GetThreadContext()->TTDLog->AddScriptLoad(globalBody, kmodGlobal, sourceContext, script, (uint32)cb, loadScriptFlag);
            if(parseEvent != nullptr)
            {
                TTD::NSLogEvents::JsRTCodeParseAction_SetBodyCtrId(parseEvent, tbfi->TopLevelBase.TopLevelBodyCtr);
            }

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

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        if (scriptFunction == nullptr)
        {
            PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

            HandleScriptCompileError(scriptContext, &se, sourceUrl);
            return JsErrorScriptCompile;
        }

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
            TTD::TTDJsRTFunctionCallActionPopperRecorder callInfoPopper;
            if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
            {
                TTD::NSLogEvents::EventLogEntry* callEvent = scriptContext->GetThreadContext()->TTDLog->RecordJsRTCallFunction(_actionEntryPopper, scriptContext->GetThreadContext()->TTDRootNestingCount, scriptFunction, args.Info.Count, args.Values);
                callInfoPopper.InitializeForRecording(scriptContext, scriptContext->GetThreadContext()->TTDLog->GetCurrentWallTime(), callEvent);

                if(scriptContext->GetThreadContext()->TTDRootNestingCount == 0)
                {
                    TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
                    elog->ResetCallStackForTopLevelCall(elog->GetLastEventTime());

                    TTD::ExecutionInfoManager* emanager = scriptContext->GetThreadContext()->TTDExecutionInfo;
                    if(emanager != nullptr)
                    {
                        emanager->ResetCallStackForTopLevelCall(elog->GetLastEventTime());
                    }
                }
            }
#endif

            Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);
            if (result != nullptr)
            {
                *result = varResult;
            }

#if ENABLE_TTD
            if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext))
            {
                _actionEntryPopper.SetResult(result);
            }
#endif
        }
        return JsNoError;
    });
}

JsErrorCode RunScriptCore(const char *script, JsSourceContext sourceContext,
    const char *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes,
    bool isSourceModule, JsValueRef *result)
{
    utf8::NarrowToWide url((LPCSTR)sourceUrl);
    if (!url)
    {
        return JsErrorOutOfMemory;
    }

    return RunScriptCore(nullptr, reinterpret_cast<const byte*>(script), strlen(script),
        LoadScriptFlag_Utf8Source, sourceContext, url, parseOnly, parseAttributes,
        isSourceModule, result);
}

JsErrorCode RunScriptCore(const WCHAR *script, JsSourceContext sourceContext,
    const WCHAR *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes,
    bool isSourceModule, JsValueRef *result)
{
    return RunScriptCore(nullptr, reinterpret_cast<const byte*>(script),
        wcslen(script) * sizeof(WCHAR),
        LoadScriptFlag_None, sourceContext, sourceUrl, parseOnly,
        parseAttributes, isSourceModule, result);
}

#ifdef _WIN32
CHAKRA_API JsParseScript(_In_z_ const WCHAR * script, _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, true,
        JsParseScriptAttributeNone, false /*isModule*/, result);
}

CHAKRA_API JsParseScriptWithAttributes(
    _In_z_ const WCHAR *script,
    _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, true,
        parseAttributes, false /*isModule*/, result);
}

CHAKRA_API JsRunScript(_In_z_ const WCHAR * script, _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, false,
        JsParseScriptAttributeNone, false /*isModule*/, result);
}

CHAKRA_API JsExperimentalApiRunModule(_In_z_ const WCHAR * script,
    _In_ JsSourceContext sourceContext, _In_z_ const WCHAR *sourceUrl,
    _Out_ JsValueRef * result)
{
    return RunScriptCore(script, sourceContext, sourceUrl, false,
        JsParseScriptAttributeNone, true, result);
}
#endif

JsErrorCode JsSerializeScriptCore(const byte *script, size_t cb,
    LoadScriptFlag loadScriptFlag, BYTE *functionTable, int functionTableSize,
    unsigned char *buffer, unsigned int *bufferSize, JsValueRef scriptSource)
{
    Js::JavascriptFunction *function;
    CompileScriptException se;

    JsErrorCode errorCode = ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
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

        const int chsize = (loadScriptFlag & LoadScriptFlag_Utf8Source) ? sizeof(utf8char_t) : sizeof(WCHAR);
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
        function = scriptContext->LoadScript(script, cb, &si, &se, &sourceInfo,
            Js::Constants::GlobalCode, loadScriptFlag, scriptSource);
        return JsNoError;
    });

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
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
        HRESULT hr = Js::ByteCodeSerializer::SerializeToBuffer(scriptContext,
            tempAllocator, static_cast<DWORD>(cSourceCodeLength), utf8Code,
            functionBody, functionBody->GetHostSrcInfo(), false, &buffer,
            (DWORD*) bufferSize, dwFlags);
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

CHAKRA_API JsSerializeScript(_In_z_ const WCHAR *script, _Out_writes_to_opt_(*bufferSize,
    *bufferSize) unsigned char *buffer,
    _Inout_ unsigned int *bufferSize)
{
    return JsSerializeScriptCore((const byte*)script, wcslen(script) * sizeof(WCHAR),
        LoadScriptFlag_None, nullptr, 0, buffer, bufferSize, nullptr);
}

template <typename TLoadCallback, typename TUnloadCallback>
JsErrorCode RunSerializedScriptCore(
    TLoadCallback scriptLoadCallback, TUnloadCallback scriptUnloadCallback,
    JsSourceContext scriptLoadSourceContext, // only used by scriptLoadCallback
    unsigned char *buffer, JsValueRef bufferVal,
    JsSourceContext sourceContext, const WCHAR *sourceUrl,
    bool parseOnly, JsValueRef *result)
{
    Js::JavascriptFunction *function;
    JsErrorCode errorCode = ContextAPINoScriptWrapper_NoRecord([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
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
        sourceHolder = RecyclerNewFinalized(scriptContext->GetRecycler(), TSourceHolder,
            scriptLoadCallback, scriptUnloadCallback, scriptLoadSourceContext, bufferVal);

        SourceContextInfo *sourceContextInfo;
        SRCINFO *hsi;
        Field(Js::FunctionBody*) functionBody = nullptr;

        HRESULT hr;

        sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);

        if (sourceContextInfo == nullptr)
        {
            sourceContextInfo = scriptContext->CreateSourceContextInfo(sourceContext, sourceUrl,
                wcslen(sourceUrl), nullptr);
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
        hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, sourceHolder,
            hsi, buffer, nullptr, &functionBody);

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

    return ContextAPIWrapper_NoRecord<false>([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
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

static void CHAKRA_CALLBACK DummyScriptUnloadCallback(_In_ JsSourceContext sourceContext)
{
    // Do nothing
}

#ifdef _WIN32
static bool CHAKRA_CALLBACK DummyScriptLoadSourceCallback(_In_ JsSourceContext sourceContext, _Outptr_result_z_ const WCHAR** scriptBuffer)
{
    // sourceContext is actually the script source pointer
    *scriptBuffer = reinterpret_cast<const WCHAR*>(sourceContext);
    return true;
}

CHAKRA_API JsParseSerializedScript(_In_z_ const WCHAR * script, _In_ unsigned char *buffer,
    _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl,
    _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        DummyScriptLoadSourceCallback, DummyScriptUnloadCallback,
        reinterpret_cast<JsSourceContext>(script), // use script source pointer as scriptLoadSourceContext
        buffer, nullptr, sourceContext, sourceUrl, true, result);
}

CHAKRA_API JsRunSerializedScript(_In_z_ const WCHAR * script, _In_ unsigned char *buffer,
    _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl,
    _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        DummyScriptLoadSourceCallback, DummyScriptUnloadCallback,
        reinterpret_cast<JsSourceContext>(script), // use script source pointer as scriptLoadSourceContext
        buffer, nullptr, sourceContext, sourceUrl, false, result);
}

CHAKRA_API JsParseSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback,
    _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback,
    _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        scriptLoadCallback, scriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, nullptr, sourceContext, sourceUrl, true, result);
}

CHAKRA_API JsRunSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback,
    _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback,
    _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const WCHAR *sourceUrl, _Out_opt_ JsValueRef * result)
{
    return RunSerializedScriptCore(
        scriptLoadCallback, scriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, nullptr, sourceContext, sourceUrl, false, result);
}
#endif // _WIN32

/////////////////////

CHAKRA_API JsTTDCreateRecordRuntime(_In_ JsRuntimeAttributes attributes, _In_ size_t snapInterval, _In_ size_t snapHistoryLength,
    _In_ TTDOpenResourceStreamCallback openResourceStream, _In_ JsTTDWriteBytesToStreamCallback writeBytesToStream, _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
    _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    if(snapInterval > UINT32_MAX || snapHistoryLength > UINT32_MAX)
    {
        return JsErrorInvalidArgument;
    }

    return CreateRuntimeCore(attributes, nullptr, 0, true, false, false, (uint32)snapInterval, (uint32)snapHistoryLength,
        openResourceStream, nullptr, writeBytesToStream, flushAndCloseStream,
        threadService, runtime);
#endif
}

CHAKRA_API JsTTDCreateReplayRuntime(_In_ JsRuntimeAttributes attributes, _In_reads_(infoUriCount) const char* infoUri, _In_ size_t infoUriCount, _In_ bool enableDebugging,
    _In_ TTDOpenResourceStreamCallback openResourceStream, _In_ JsTTDReadBytesFromStreamCallback readBytesFromStream, _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
    _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else

    return CreateRuntimeCore(attributes, infoUri, infoUriCount, false, true, enableDebugging, UINT_MAX, UINT_MAX,
        openResourceStream, readBytesFromStream, nullptr, flushAndCloseStream,
        threadService, runtime);
#endif
}

CHAKRA_API JsTTDCreateContext(_In_ JsRuntimeHandle runtimeHandle, _In_ bool useRuntimeTTDMode, _Out_ JsContextRef *newContext)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        PARAM_NOT_NULL(newContext);
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

        JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
        ThreadContext * threadContext = runtime->GetThreadContext();
        TTDAssert(threadContext->IsRuntimeInTTDMode(), "Need to create in TTD Mode.");

        bool inRecord = false;
        bool activelyRecording = false;
        bool inReplay = false;
        TTDRecorder dummyActionEntryPopper;
        if(useRuntimeTTDMode)
        {
            threadContext->TTDLog->GetModesForExplicitContextCreate(inRecord, activelyRecording, inReplay);
        }

        return CreateContextCore(runtimeHandle, dummyActionEntryPopper, inRecord, activelyRecording, inReplay, newContext);
    });
#endif
}

CHAKRA_API JsTTDNotifyContextDestroy(_In_ JsContextRef context)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
    if(threadContext && threadContext->IsRuntimeInTTDMode())
    {
        Js::ScriptContext* ctx = static_cast<JsrtContext*>(context)->GetScriptContext();
        threadContext->TTDContext->NotifyCtxDestroyInRecord(ctx);
    }

    return JsNoError;
#endif
}

CHAKRA_API JsTTDStart()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    TTDAssert(cCheck == JsNoError, "Must have valid context when starting TTD.");

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    TTDAssert(scriptContext->IsTTDRecordOrReplayModeEnabled(), "Need to create in TTD Record Mode.");
#if ENABLE_NATIVE_CODEGEN
    TTDAssert(JITManager::GetJITManager() == nullptr || !JITManager::GetJITManager()->IsOOPJITEnabled(), "TTD cannot run with OOP JIT yet!!!");
#endif
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
    {
        if(scriptContext->IsTTDRecordModeEnabled())
        {
            scriptContext->GetThreadContext()->TTDLog->DoSnapshotExtract();
        }

        //Want to verify that we are at top-level of dispatch
        scriptContext->GetThreadContext()->TTDLog->PushMode(TTD::TTDMode::CurrentlyEnabled);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDStop()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    TTDAssert(cCheck == JsNoError, "Must have valid context when starting TTD.");

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    TTDAssert(scriptContext->IsTTDRecordOrReplayModeEnabled(), "Need to create in TTD mode.");

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
    {
        scriptContext->GetThreadContext()->TTDLog->PopMode(TTD::TTDMode::CurrentlyEnabled);

        if(scriptContext->IsTTDRecordModeEnabled())
        {
            scriptContext->GetThreadContext()->TTDLog->UnloadAllLogData();
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    TTDAssert(cCheck == JsNoError, "Must have valid context when changing debugger mode.");

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();

    if(threadContext->IsRuntimeInTTDMode())
    {
        threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecutionDebuggerAction);
    }

    return JsNoError;
#endif
}

CHAKRA_API JsTTDReStartTimeTravelAfterRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    TTDAssert(cCheck == JsNoError, "Must have valid context when changing debugger mode.");

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();

    if(threadContext->IsRuntimeInTTDMode())
    {
        threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecutionDebuggerAction);
    }

    return JsNoError;
#endif
}

CHAKRA_API JsTTDNotifyYield()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    if(cCheck != JsNoError)
    {
        return JsNoError; //we are ok just aren't going to do any TTD related work
    }

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
    {
        if(scriptContext->IsTTDRecordModeEnabled())
        {
            scriptContext->GetThreadContext()->TTDLog->RecordJsRTEventLoopYieldPoint();
        }

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDNotifyLongLivedReferenceAdd(_In_ JsValueRef value)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    return GlobalAPIWrapper([&](TTDRecorder& _actionEntryPopper) -> JsErrorCode
    {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if(threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }

        Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(value);
        if(obj->GetScriptContext()->IsTTDRecordModeEnabled())
        {
            if(obj->GetScriptContext()->ShouldPerformRecordAction())
            {
                threadContext->TTDLog->RecordJsRTAddWeakRootRef(_actionEntryPopper, (Js::Var)value);
            }

            threadContext->TTDContext->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(obj), obj);
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
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTHostExitProcess, statusCode);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDRawBufferCopySyncIndirect(_In_ JsValueRef dst, _In_ size_t dstIndex, _In_ JsValueRef src, _In_ size_t srcIndex, _In_ size_t count)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    if(dstIndex > UINT32_MAX || srcIndex > UINT32_MAX || count > UINT32_MAX)
    {
        return JsErrorInvalidArgument;
    }

    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTRawBufferCopySync, dst, (uint32)dstIndex, src, (uint32)srcIndex, (uint32)count);

        return JsNoError;
    });
#endif
}

CHAKRA_API JsTTDRawBufferModifySyncIndirect(_In_ JsValueRef buffer, _In_ size_t index, _In_ size_t count)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    if(index > UINT32_MAX || count > UINT32_MAX)
    {
        return JsErrorInvalidArgument;
    }

    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTRawBufferModifySync, buffer, (uint32)index, (uint32)count);

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
    JsErrorCode addRefResult = ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        if (scriptContext->IsTTDRecordModeEnabled())
        {
            TTDAssert(Js::ArrayBuffer::Is(instance), "Not array buffer object!!!");
            Js::ArrayBuffer* dstBuff = Js::ArrayBuffer::FromVar(instance);
            addRefObj = dstBuff;

            TTDAssert(dstBuff->GetBuffer() <= initialModPos && initialModPos < dstBuff->GetBuffer() + dstBuff->GetByteLength(), "Not array buffer object!!!");
            TTDAssert(initialModPos - dstBuff->GetBuffer() < UINT32_MAX, "This is really big!!!");
            ptrdiff_t index = initialModPos - Js::ArrayBuffer::FromVar(instance)->GetBuffer();

            scriptContext->TTDContextInfo->AddToAsyncPendingList(dstBuff, (uint32)index);

            PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTRawBufferAsyncModificationRegister, instance, (uint32)index);
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
    JsErrorCode releaseStatus = ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        if (scriptContext->IsTTDRecordModeEnabled())
        {
            TTD::TTDPendingAsyncBufferModification pendingAsyncInfo = { 0 };
            scriptContext->TTDContextInfo->GetFromAsyncPendingList(&pendingAsyncInfo, finalModPos);

            Js::ArrayBuffer* dstBuff = Js::ArrayBuffer::FromVar(pendingAsyncInfo.ArrayBufferVar);
            releaseObj = dstBuff;

            PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTRawBufferAsyncModifyComplete, pendingAsyncInfo, finalModPos);
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

CHAKRA_API JsTTDCheckAndAssertIfTTDRunning(_In_ const char* msg)
{
#if ENABLE_TTD
    JsrtContext* context = JsrtContext::GetCurrent();
    TTDAssert(context == nullptr || !context->GetScriptContext()->ShouldPerformRecordAction(), msg);
#endif
    return JsNoError;
}

CHAKRA_API JsTTDGetSnapTimeTopLevelEventMove(_In_ JsRuntimeHandle runtimeHandle,
   _In_ JsTTDMoveMode moveMode, _In_opt_ uint32_t kthEvent,
   _Inout_ int64_t* targetEventTime, _Out_ int64_t* targetStartSnapTime,
   _Out_opt_ int64_t* targetEndSnapTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtRuntime* runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext* threadContext = runtime->GetThreadContext();

    *targetStartSnapTime = -1;
    if(targetEndSnapTime != nullptr)
    {
        *targetEndSnapTime = -1;
    }

    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

    //If we requested a move to a specific event then extract the event count and try to find it
    if((moveMode & JsTTDMoveMode::JsTTDMoveFirstEvent) == JsTTDMoveMode::JsTTDMoveFirstEvent)
    {
        *targetEventTime = threadContext->TTDLog->GetFirstEventTimeInLog();
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else if((moveMode & JsTTDMoveMode::JsTTDMoveLastEvent) == JsTTDMoveMode::JsTTDMoveLastEvent)
    {
        *targetEventTime = threadContext->TTDLog->GetLastEventTimeInLog();
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else if((moveMode & JsTTDMoveMode::JsTTDMoveKthEvent) == JsTTDMoveMode::JsTTDMoveKthEvent)
    {
        *targetEventTime = threadContext->TTDLog->GetKthEventTimeInLog(kthEvent);
        if(*targetEventTime == -1)
        {
            return JsErrorCategoryUsage;
        }
    }
    else
    {
        ;
    }

#ifdef __APPLE__
    //TODO: Explicit cast of ptr since compiler gets confused -- resolve in PAL later
    static_assert(sizeof(int64_t) == sizeof(int64), "int64_t and int64 size mis-match");
    *targetStartSnapTime = threadContext->TTDLog->FindSnapTimeForEventTime(*targetEventTime, (int64*)targetEndSnapTime);
#else
    *targetStartSnapTime = threadContext->TTDLog->FindSnapTimeForEventTime(*targetEventTime, targetEndSnapTime);
#endif

    return JsNoError;
#endif
}

CHAKRA_API JsTTDGetSnapShotBoundInterval(_In_ JsRuntimeHandle runtimeHandle, _In_ int64_t targetEventTime, _Out_ int64_t* startSnapTime, _Out_ int64_t* endSnapTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtRuntime* runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext* threadContext = runtime->GetThreadContext();
    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

#ifdef __APPLE__
    //TODO: Explicit cast of ptr since compiler gets confused -- resolve in PAL later
    static_assert(sizeof(int64_t) == sizeof(int64), "int64_t and int64 size mis-match");
    threadContext->TTDLog->GetSnapShotBoundInterval(targetEventTime, (int64*)startSnapTime, (int64*)endSnapTime);
#else
    threadContext->TTDLog->GetSnapShotBoundInterval(targetEventTime, startSnapTime, endSnapTime);
#endif

    return JsNoError;
#endif
}

CHAKRA_API JsTTDGetPreviousSnapshotInterval(_In_ JsRuntimeHandle runtimeHandle, _In_ int64_t currentSnapStartTime, _Out_ int64_t* previousSnapTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext * threadContext = runtime->GetThreadContext();
    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

    *previousSnapTime = threadContext->TTDLog->GetPreviousSnapshotInterval(currentSnapStartTime);

    return JsNoError;
#endif
}

#if ENABLE_TTD
//Helper method for resetting breakpoint info around snapshot inflate
JsErrorCode TTDHandleBreakpointInfoAndInflate(int64_t snapTime, JsrtRuntime* runtime, ThreadContext* threadContext)
{
    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
    {
        if(threadContext->TTDLog->IsDebugModeFlagSet())
        {
            threadContext->TTDExecutionInfo->LoadPreservedBPInfo(threadContext);
        }

        threadContext->TTDLog->DoSnapshotInflate(snapTime);

        threadContext->TTDLog->ResetCallStackForTopLevelCall(-1);
        if(threadContext->TTDExecutionInfo != nullptr)
        {
            threadContext->TTDExecutionInfo->ResetCallStackForTopLevelCall(-1);
        }

        return JsNoError;
    });
}
#endif

CHAKRA_API JsTTDPreExecuteSnapShotInterval(_In_ JsRuntimeHandle runtimeHandle, _In_ int64_t startSnapTime, _In_ int64_t endSnapTime, _In_ JsTTDMoveMode moveMode, _Out_ int64_t* newTargetEventTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else

    *newTargetEventTime = -1;

    JsrtRuntime* runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext* threadContext = runtime->GetThreadContext();
    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

    TTD::EventLog* elog = threadContext->TTDLog;
    TTD::ExecutionInfoManager* emanager = threadContext->TTDExecutionInfo;
    JsErrorCode res = JsNoError;

    JsErrorCode inflateStatus = TTDHandleBreakpointInfoAndInflate(startSnapTime, runtime, threadContext);
    if(inflateStatus != JsNoError)
    {
        return inflateStatus;
    }

    //If we are in the "active" segment set the continue breakpoint
    if((moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalForContinueInActiveBreakpointSegment) == JsTTDMoveMode::JsTTDMoveScanIntervalForContinueInActiveBreakpointSegment)
    {
        GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
        {
            emanager->SetBPInfoForActiveSegmentContinueScan(threadContext->TTDContext);

            return JsNoError;
        });
    }

    elog->PushMode(TTD::TTDMode::DebuggerSuppressBreakpoints);
    elog->PushMode(TTD::TTDMode::DebuggerLogBreakpoints);
    try
    {
        if(endSnapTime == -1)
        {
            elog->ReplayRootEventsToTime(TTD_EVENT_MAXTIME);
        }
        else
        {
            elog->ReplayRootEventsToTime(endSnapTime);
        }
    }
    catch(TTD::TTDebuggerAbortException abortException)
    {
        //If we hit the end of the log or we hit a terminal exception that is fine -- anything else is a problem
        if(!abortException.IsEndOfLog() && !abortException.IsTopLevelException())
        {
            res = JsErrorFatal;
        }
    }
    catch(...) //we are replaying something that should be known to execute successfully so encountering any error is very bad
    {
        res = JsErrorFatal;
        TTDAssert(false, "Unexpected fatal Error");
    }
    elog->PopMode(TTD::TTDMode::DebuggerLogBreakpoints);
    elog->PopMode(TTD::TTDMode::DebuggerSuppressBreakpoints);

    //If we are in the "active" segment un-set the continue breakpoint
    if((moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalForContinueInActiveBreakpointSegment) == JsTTDMoveMode::JsTTDMoveScanIntervalForContinueInActiveBreakpointSegment)
    {
        GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
        {
            emanager->ClearBPInfoForActiveSegmentContinueScan(threadContext->TTDContext);

            return JsNoError;
        });
    }

    if((moveMode & JsTTDMoveMode::JsTTDMoveScanIntervalForContinue) == JsTTDMoveMode::JsTTDMoveScanIntervalForContinue)
    {
        bool bpFound = emanager->TryFindAndSetPreviousBP();
        if(bpFound)
        {
            *newTargetEventTime = emanager->GetPendingTTDBPTargetEventTime();
        }
    }

    return res;
#endif
}

CHAKRA_API JsTTDMoveToTopLevelEvent(_In_ JsRuntimeHandle runtimeHandle, _In_ JsTTDMoveMode moveMode, _In_ int64_t snapshotTime, _In_ int64_t eventTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else

    JsrtRuntime* runtime = JsrtRuntime::FromHandle(runtimeHandle);
    ThreadContext* threadContext = runtime->GetThreadContext();
    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

    TTD::EventLog* elog = threadContext->TTDLog;
    JsErrorCode res = JsNoError;

    JsErrorCode inflateStatus = TTDHandleBreakpointInfoAndInflate(snapshotTime, runtime, threadContext);
    if(inflateStatus != JsNoError)
    {
        return inflateStatus;
    }

    elog->PushMode(TTD::TTDMode::DebuggerSuppressBreakpoints);
    try
    {
        elog->ReplayRootEventsToTime(eventTime);

        elog->DoRtrSnapIfNeeded();
    }
    catch(...) //we are replaying something that should be known to execute successfully so encountering any error is very bad
    {
        res = JsErrorFatal;
        TTDAssert(false, "Unexpected fatal Error");
    }
    elog->PopMode(TTD::TTDMode::DebuggerSuppressBreakpoints);

    return res;
#endif
}

CHAKRA_API JsTTDReplayExecution(_Inout_ JsTTDMoveMode* moveMode, _Out_ int64_t* rootEventTime)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    JsErrorCode cCheck = CheckContext(currentContext, JSRT_MAYBE_TRUE);
    TTDAssert(cCheck == JsNoError, "This shouldn't happen!!!");

    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    TTDAssert(threadContext->IsRuntimeInTTDMode(), "Should only happen in TT debugging mode.");

    TTD::EventLog* elog = threadContext->TTDLog;
    TTD::ExecutionInfoManager* emanager = threadContext->TTDExecutionInfo;

    if(emanager != nullptr)
    {
        JsErrorCode bpstatus = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode
        {
            if((*moveMode & JsTTDMoveMode::JsTTDMoveBreakOnEntry) == JsTTDMoveMode::JsTTDMoveBreakOnEntry)
            {
                emanager->SetBreakOnFirstUserCode();
            }

            //Set the active BP info from the manager (so we will hit the BP in step back operations)
            emanager->SetActiveBPInfoAsNeeded(threadContext->TTDContext);

            return JsNoError;
        });

        if(bpstatus != JsNoError)
        {
            return bpstatus;
        }
    }

    *moveMode = JsTTDMoveMode::JsTTDMoveNone;
    *rootEventTime = -1;
    JsErrorCode res = JsNoError;
    try
    {
        elog->ReplayRootEventsToTime(TTD_EVENT_MAXTIME);
    }
    catch(TTD::TTDebuggerAbortException abortException)
    {
        //if the debugger bails out with a move time request set info on the requested event time here
        //rest of breakpoint info should have been set by the debugger callback before aborting
        if (abortException.IsEventTimeMove() || abortException.IsTopLevelException())
        {
            *moveMode = (JsTTDMoveMode)abortException.GetMoveMode();
            *rootEventTime = abortException.GetTargetEventTime();

            //Check if we are tracking execution and, if so, set the exception locaiton so we can access it later
            if(emanager != nullptr && abortException.IsTopLevelException())
            {
                emanager->SetPendingTTDUnhandledException();
            }
        }

        res = abortException.IsTopLevelException() ? JsErrorCategoryScript : JsNoError;
    }
    catch(...)
    {
        res = JsErrorFatal;
        TTDAssert(false, "Unexpected fatal Error");
    }

    return res;
#endif
}

#ifdef _CHAKRACOREBUILD

template <class SrcChar, class DstChar>
static void CastCopy(const SrcChar* src, DstChar* dst, size_t count)
{
    const SrcChar* end = src + count;
    while (src < end)
    {
        *dst++ = static_cast<DstChar>(*src++);
    }
}

CHAKRA_API JsCreateString(
    _In_ const char *content,
    _In_ size_t length,
    _Out_ JsValueRef *value)
{
    PARAM_NOT_NULL(content);

    utf8::NarrowToWide wstr(content, length);
    if (!wstr)
    {
        return JsErrorOutOfMemory;
    }

    return JsPointerToString(wstr, wstr.Length(), value);
}

CHAKRA_API JsCreateStringUtf16(
    _In_ const uint16_t *content,
    _In_ size_t length,
    _Out_ JsValueRef *value)
{
    PARAM_NOT_NULL(content);

    return JsPointerToString(
        reinterpret_cast<const char16*>(content), length, value);
}


template <class CopyFunc>
JsErrorCode WriteStringCopy(
    JsValueRef value,
    int start,
    int length,
    _Out_opt_ size_t* written,
    const CopyFunc& copyFunc)
{
    if (written)
    {
        *written = 0;  // init to 0 for default
    }

    const char16* str = nullptr;
    size_t strLength = 0;
    JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    if (start < 0 || (size_t)start > strLength)
    {
        return JsErrorInvalidArgument;  // start out of range, no chars written
    }

    size_t count = min(static_cast<size_t>(length), strLength - start);
    if (count == 0)
    {
        return JsNoError;  // no chars written
    }

    errorCode = copyFunc(str + start, count, written);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    if (written)
    {
        *written = count;
    }

    return JsNoError;
}

CHAKRA_API JsCopyStringUtf16(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ uint16_t* buffer,
    _Out_opt_ size_t* written)
{
    PARAM_NOT_NULL(value);
    VALIDATE_JSREF(value);

    return WriteStringCopy(value, start, length, written,
        [buffer](const char16* src, size_t count, size_t *needed)
        {
            if (buffer)
            {
                memmove(buffer, src, sizeof(char16) * count);
            }
            return JsNoError;
        });
}

CHAKRA_API JsCopyString(
    _In_ JsValueRef value,
    _Out_opt_ char* buffer,
    _In_ size_t bufferSize,
    _Out_opt_ size_t* length)
{
    PARAM_NOT_NULL(value);
    VALIDATE_JSREF(value);

    const char16* str = nullptr;
    size_t strLength = 0;
    JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    utf8::WideToNarrow utf8Str(str, strLength, buffer, bufferSize);
    if (length)
    {
        *length = utf8Str.Length();
    }

    return JsNoError;
}

_ALWAYSINLINE JsErrorCode CompileRun(
    JsValueRef scriptVal,
    JsSourceContext sourceContext,
    JsValueRef sourceUrl,
    JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result,
    bool parseOnly)
{
    PARAM_NOT_NULL(scriptVal);
    VALIDATE_JSREF(scriptVal);
    PARAM_NOT_NULL(sourceUrl);

    bool isExternalArray = Js::ExternalArrayBuffer::Is(scriptVal),
         isString = false;
    bool isUtf8   = !(parseAttributes & JsParseScriptAttributeArrayBufferIsUtf16Encoded);

    LoadScriptFlag scriptFlag = LoadScriptFlag_None;
    const byte* script;
    size_t cb;
    const WCHAR *url;

    if (isExternalArray)
    {
        script = ((Js::ExternalArrayBuffer*)(scriptVal))->GetBuffer();

        cb = ((Js::ExternalArrayBuffer*)(scriptVal))->GetByteLength();

        scriptFlag = (LoadScriptFlag)(isUtf8 ?
            LoadScriptFlag_ExternalArrayBuffer | LoadScriptFlag_Utf8Source :
            LoadScriptFlag_ExternalArrayBuffer);
    }
    else
    {
        isString = Js::JavascriptString::Is(scriptVal);
        if (!isString)
        {
            return JsErrorInvalidArgument;
        }
    }

    JsErrorCode error = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        if (isString)
        {
            Js::JavascriptString* jsString = Js::JavascriptString::FromVar(scriptVal);
            script = (const byte*)jsString->GetSz();

            // JavascriptString is 2 bytes (WCHAR/char16)
            cb = jsString->GetLength() * sizeof(WCHAR);
        }

        if (!Js::JavascriptString::Is(sourceUrl))
        {
            return JsErrorInvalidArgument;
        }

        url = Js::JavascriptString::FromVar(sourceUrl)->GetSz();

        return JsNoError;

    });

    if (error != JsNoError)
    {
        return error;
    }

    return RunScriptCore(scriptVal, script, cb, scriptFlag,
        sourceContext, url, parseOnly, parseAttributes, false, result);
}

CHAKRA_API JsParse(
    _In_ JsValueRef scriptVal,
    _In_ JsSourceContext sourceContext,
    _In_ JsValueRef sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return CompileRun(scriptVal, sourceContext, sourceUrl, parseAttributes,
        result, true);
}

CHAKRA_API JsRun(
    _In_ JsValueRef scriptVal,
    _In_ JsSourceContext sourceContext,
    _In_ JsValueRef sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return CompileRun(scriptVal, sourceContext, sourceUrl, parseAttributes,
        result, false);
}

CHAKRA_API JsCreatePropertyId(
    _In_z_ const char *name,
    _In_ size_t length,
    _Out_ JsPropertyIdRef *propertyId)
{
    PARAM_NOT_NULL(name);
    utf8::NarrowToWide wname(name, length);
    if (!wname)
    {
        return JsErrorOutOfMemory;
    }

    return JsGetPropertyIdFromName(wname, propertyId);
}

CHAKRA_API JsCopyPropertyId(
    _In_ JsPropertyIdRef propertyId,
    _Out_ char* buffer,
    _In_ size_t bufferSize,
    _Out_ size_t* length)
{
    PARAM_NOT_NULL(propertyId);

    const char16* str = nullptr;
    JsErrorCode errorCode = JsGetPropertyNameFromId(propertyId, &str);

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    utf8::WideToNarrow utf8Str(str);
    if (!buffer)
    {
        if (length)
        {
            *length = utf8Str.Length();
        }
    }
    else
    {
        size_t count = min(bufferSize, utf8Str.Length());
        // Try to copy whole characters if buffer size insufficient
        auto maxFitChars = utf8::ByteIndexIntoCharacterIndex(
            (LPCUTF8)(const char*)utf8Str, count,
            utf8::DecodeOptions::doChunkedEncoding);
        count = utf8::CharacterIndexToByteIndex(
            (LPCUTF8)(const char*)utf8Str, utf8Str.Length(), maxFitChars);

        memmove(buffer, utf8Str, sizeof(char) * count);
        if (length)
        {
            *length = count;
        }
    }

    return JsNoError;
}

CHAKRA_API JsSerialize(
    _In_ JsValueRef scriptVal,
    _Out_ JsValueRef *bufferVal,
    _In_ JsParseScriptAttributes parseAttributes)
{
    PARAM_NOT_NULL(scriptVal);
    PARAM_NOT_NULL(bufferVal);
    VALIDATE_JSREF(scriptVal);

    *bufferVal = nullptr;

    bool isExternalArray = Js::ExternalArrayBuffer::Is(scriptVal),
         isString = false;
    bool isUtf8   = !(parseAttributes & JsParseScriptAttributeArrayBufferIsUtf16Encoded);
    if (!isExternalArray)
    {
        isString = Js::JavascriptString::Is(scriptVal);
        if (!isString)
        {
            return JsErrorInvalidArgument;
        }
    }

    LoadScriptFlag scriptFlag;
    const byte* script = isExternalArray ?
        ((Js::ExternalArrayBuffer*)(scriptVal))->GetBuffer() :
        (const byte*)((Js::JavascriptString*)(scriptVal))->GetSz();
    const size_t cb = isExternalArray ?
        ((Js::ExternalArrayBuffer*)(scriptVal))->GetByteLength() :
        ((Js::JavascriptString*)(scriptVal))->GetLength();

    if (isExternalArray && isUtf8)
    {
        scriptFlag = (LoadScriptFlag) (LoadScriptFlag_ExternalArrayBuffer | LoadScriptFlag_Utf8Source);
    }
    else if (isUtf8)
    {
        scriptFlag = (LoadScriptFlag) (LoadScriptFlag_Utf8Source);
    }
    else
    {
        scriptFlag = LoadScriptFlag_None;
    }

    unsigned int bufferSize = 0;
    JsErrorCode errorCode = JsSerializeScriptCore(script, cb, scriptFlag, nullptr,
        0, nullptr, &bufferSize, scriptVal);

    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    if (bufferSize == 0)
    {
        return JsErrorScriptCompile;
    }

    if ((errorCode = JsCreateArrayBuffer(bufferSize, bufferVal)) == JsNoError)
    {
        byte* buffer = ((Js::ArrayBuffer*)(*bufferVal))->GetBuffer();
        errorCode = JsSerializeScriptCore(script, cb, scriptFlag, nullptr,
            0, buffer, &bufferSize, scriptVal);
    }

    return errorCode;
}

CHAKRA_API JsParseSerialized(
    _In_ JsValueRef bufferVal,
    _In_ JsSerializedLoadScriptCallback scriptLoadCallback,
    _In_ JsSourceContext sourceContext,
    _In_ JsValueRef sourceUrl,
    _Out_ JsValueRef *result)
{
    PARAM_NOT_NULL(bufferVal);
    PARAM_NOT_NULL(sourceUrl);

    const WCHAR *url;

    if (Js::JavascriptString::Is(sourceUrl))
    {
        url = ((Js::JavascriptString*)(sourceUrl))->GetSz();
    }
    else
    {
        return JsErrorInvalidArgument;
    }

    // JsParseSerialized only accepts ArrayBuffer (incl. ExternalArrayBuffer)
    if (!Js::ExternalArrayBuffer::Is(bufferVal))
    {
        return JsErrorInvalidArgument;
    }

    byte* buffer = Js::ArrayBuffer::FromVar(bufferVal)->GetBuffer();

    return RunSerializedScriptCore(
      scriptLoadCallback, DummyScriptUnloadCallback,
      sourceContext,// use the same user provided sourceContext as scriptLoadSourceContext
      buffer, bufferVal, sourceContext, url, true, result);
}

CHAKRA_API JsRunSerialized(
    _In_ JsValueRef bufferVal,
    _In_ JsSerializedLoadScriptCallback scriptLoadCallback,
    _In_ JsSourceContext sourceContext,
    _In_ JsValueRef sourceUrl,
    _Out_ JsValueRef *result)
{
    PARAM_NOT_NULL(bufferVal);
    const WCHAR *url;

    if (sourceUrl && Js::JavascriptString::Is(sourceUrl))
    {
        url = ((Js::JavascriptString*)(sourceUrl))->GetSz();
    }
    else
    {
        return JsErrorInvalidArgument;
    }

    // JsParseSerialized only accepts ArrayBuffer (incl. ExternalArrayBuffer)
    if (!Js::ExternalArrayBuffer::Is(bufferVal))
    {
        return JsErrorInvalidArgument;
    }

    byte* buffer = Js::ArrayBuffer::FromVar(bufferVal)->GetBuffer();

    return RunSerializedScriptCore(
        scriptLoadCallback, DummyScriptUnloadCallback,
        sourceContext, // use the same user provided sourceContext as scriptLoadSourceContext
        buffer, bufferVal, sourceContext, url, false, result);
}

CHAKRA_API JsCreatePromise(_Out_ JsValueRef *promise, _Out_ JsValueRef *resolve, _Out_ JsValueRef *reject)
{
    return ContextAPIWrapper<JSRT_MAYBE_TRUE>([&](Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION_NOT_IMPLEMENTED(scriptContext);

        PARAM_NOT_NULL(promise);
        PARAM_NOT_NULL(resolve);
        PARAM_NOT_NULL(reject);

        *promise = nullptr;
        *resolve = nullptr;
        *reject = nullptr;

        Js::JavascriptPromiseResolveOrRejectFunction *jsResolve = nullptr;
        Js::JavascriptPromiseResolveOrRejectFunction *jsReject = nullptr;
        Js::JavascriptPromise *jsPromise = scriptContext->GetLibrary()->CreatePromise();
        Js::JavascriptPromise::InitializePromise(jsPromise, &jsResolve, &jsReject, scriptContext);

        *promise = (JsValueRef)jsPromise;
        *resolve = (JsValueRef)jsResolve;
        *reject = (JsValueRef)jsReject;

        return JsNoError;
    });
}

CHAKRA_API JsCreateWeakReference(
    _In_ JsValueRef value,
    _Out_ JsWeakRef* weakRef)
{
    VALIDATE_JSREF(value);
    PARAM_NOT_NULL(weakRef);
    *weakRef = nullptr;

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        ThreadContext* threadContext = ThreadContext::GetContextForCurrentThread();
        if (threadContext == nullptr)
        {
            return JsErrorNoCurrentContext;
        }

        Recycler* recycler = threadContext->GetRecycler();
        if (recycler->IsInObjectBeforeCollectCallback())
        {
            return JsErrorInObjectBeforeCollectCallback;
        }

        recycler->FindOrCreateWeakReferenceHandle<char>(
            reinterpret_cast<char*>(value),
            reinterpret_cast<Memory::RecyclerWeakReference<char>**>(weakRef));
        return JsNoError;
    });
}

CHAKRA_API JsGetWeakReferenceValue(
    _In_ JsWeakRef weakRef,
    _Out_ JsValueRef* value)
{
    VALIDATE_JSREF(weakRef);
    PARAM_NOT_NULL(value);
    *value = JS_INVALID_REFERENCE;

    return GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        Memory::RecyclerWeakReference<char>* recyclerWeakReference =
            reinterpret_cast<Memory::RecyclerWeakReference<char>*>(weakRef);
        *value = reinterpret_cast<JsValueRef>(recyclerWeakReference->Get());
        return JsNoError;
    });
}

CHAKRA_API JsGetAndClearExceptionWithMetadata(_Out_ JsValueRef *metadata)
{
    PARAM_NOT_NULL(metadata);
    *metadata = nullptr;

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
        if (scriptContext->HasRecordedException())
        {
            recordedException = scriptContext->GetAndClearRecordedException();
        }
    END_TRANSLATE_OOM_TO_HRESULT(hr)

    if (hr == E_OUTOFMEMORY)
    {
        recordedException = scriptContext->GetThreadContext()->GetRecordedException();
    }
    if (recordedException == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    Js::Var exception = recordedException->GetThrownObject(nullptr);

    if (exception == nullptr)
    {
        // TODO: How does this early bailout impact TTD?
        return JsErrorInvalidArgument;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        Js::Var exceptionMetadata = Js::JavascriptExceptionMetadata::CreateMetadataVar(scriptContext);
        Js::JavascriptOperators::OP_SetProperty(exceptionMetadata, Js::PropertyIds::exception, exception, scriptContext);

        Js::FunctionBody *functionBody = recordedException->GetFunctionBody();
        if (functionBody == nullptr)
        {
            // This is probably a parse error. We can get the error location metadata from the thrown object.
            Js::JavascriptExceptionMetadata::PopulateMetadataFromCompileException(exceptionMetadata, exception, scriptContext);
        }
        else
        {
            if (!Js::JavascriptExceptionMetadata::PopulateMetadataFromException(exceptionMetadata, recordedException, scriptContext))
            {
                return JsErrorInvalidArgument;
            }
        }

        *metadata = exceptionMetadata;

#if ENABLE_TTD
        if (hr != E_OUTOFMEMORY)
        {
            PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetAndClearExceptionWithMetadata);
            PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, metadata);
        }
#endif


        return JsNoError;
    });
}

CHAKRA_API JsHasOwnProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ bool *hasOwnProperty)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTHasOwnProperty, (Js::PropertyRecord *)propertyId, object);

        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(hasOwnProperty);
        *hasOwnProperty = false;

        *hasOwnProperty = Js::JavascriptOperators::OP_HasOwnProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext) != 0;

        return JsNoError;
    });
}

CHAKRA_API JsCopyStringOneByte(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ char* buffer,
    _Out_opt_ size_t* written)
{
    PARAM_NOT_NULL(value);
    VALIDATE_JSREF(value);
    return WriteStringCopy(value, start, length, written,
        [buffer](const char16* src, size_t count, size_t *needed)
    {
        if (buffer)
        {
            for (size_t i = 0; i < count; i++)
            {
                buffer[i] = (char)src[i];
            }
        }
        return JsNoError;
    });
}

CHAKRA_API JsGetDataViewInfo(
    _In_ JsValueRef dataView,
    _Out_opt_ JsValueRef *arrayBuffer,
    _Out_opt_ unsigned int *byteOffset,
    _Out_opt_ unsigned int *byteLength)
{
    VALIDATE_JSREF(dataView);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if (!Js::DataView::Is(dataView))
        {
            RETURN_NO_EXCEPTION(JsErrorInvalidArgument);
        }

        Js::DataView* dv = Js::DataView::FromVar(dataView);
        if (arrayBuffer != nullptr) {
            *arrayBuffer = dv->GetArrayBuffer();
        }

        if (byteOffset != nullptr) {
            *byteOffset = dv->GetByteOffset();
        }

        if (byteLength != nullptr) {
            *byteLength = dv->GetLength();
        }
    }

#if ENABLE_TTD
    Js::ScriptContext* scriptContext = Js::RecyclableObject::FromVar(dataView)->GetScriptContext();
    if(PERFORM_JSRT_TTD_RECORD_ACTION_CHECK(scriptContext) && arrayBuffer != nullptr)
    {
        scriptContext->GetThreadContext()->TTDLog->RecordJsRTGetDataViewInfo(dataView, *arrayBuffer);
    }
#endif

    END_JSRT_NO_EXCEPTION
}

#endif // _CHAKRACOREBUILD
