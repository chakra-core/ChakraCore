//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "jsrtInternal.h"
#include "JsrtExternalObject.h"
#include "JsrtExternalArrayBuffer.h"

#include "JsrtSourceHolder.h"
#include "ByteCode\ByteCodeSerializer.h"
#include "common\ByteSwap.h"
#include "Library\dataview.h"
#include "Library\JavascriptSymbol.h"
#include "Base\ThreadContextTLSEntry.h"

// Parser Includes
#include "cmperr.h"     // For ERRnoMemory
#include "screrror.h"   // For CompileScriptException

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
#include "TestHooksRt.h"
#endif

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

template <class Fn, class T>
T CallbackWrapper(Fn fn, T default)
{
    T result = default;
    try
    {
        AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

        result = fn();
    }
    catch (Js::OutOfMemoryException)
    {
    }
    catch (Js::StackOverflowException)
    {
    }
    catch (Js::ExceptionBase)
    {
        AssertMsg(false, "Unexpected engine exception.");
    }
    catch (...)
    {
        AssertMsg(false, "Unexpected non-engine exception.");
    }

    return result;
}

template <class Fn>
bool CallbackWrapper(Fn fn)
{
    return CallbackWrapper(fn, false);
}

/////////////////////

//A create runtime function that we can funnel to for regular and record or debug aware creation
JsErrorCode CreateRuntimeCore(_In_ JsRuntimeAttributes attributes, _In_opt_ wchar_t* optRecordUri, _In_opt_ wchar_t* optDebugUri, _In_ UINT32 snapInterval, _In_ UINT32 snapHistoryLength, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        PARAM_NOT_NULL(runtimeHandle);
        *runtimeHandle = nullptr;

#if ENABLE_TTD
        if(optRecordUri != nullptr || optDebugUri != nullptr)
        {
            attributes = static_cast<JsRuntimeAttributes>(attributes | JsRuntimeAttributeEnableExperimentalFeatures);
        }
#endif

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
        if((attributes & ~JsRuntimeAttributesAll) != 0)
        {
            return JsErrorInvalidArgument;
        }

        AllocationPolicyManager * policyManager = HeapNew(AllocationPolicyManager, (attributes & JsRuntimeAttributeDisableBackgroundWork) == 0);
        bool enableExperimentalFeatures = (attributes & JsRuntimeAttributeEnableExperimentalFeatures) != 0;
        ThreadContext * threadContext = HeapNew(ThreadContext, policyManager, threadService, enableExperimentalFeatures);

        if(((attributes & JsRuntimeAttributeDisableBackgroundWork) != 0)
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

        if(!threadContext->IsRentalThreadingEnabledInJSRT()
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            || Js::Configuration::Global.flags.DisableRentalThreading
#endif
            )
        {
            threadContext->SetIsThreadBound();
        }

        if(attributes & JsRuntimeAttributeAllowScriptInterrupt)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagCanDisableExecution);
        }

        if(attributes & JsRuntimeAttributeDisableEval)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagEvalDisabled);
        }

        if(attributes & JsRuntimeAttributeDisableNativeCodeGeneration)
        {
            threadContext->SetThreadContextFlag(ThreadContextFlagNoJIT);
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if(Js::Configuration::Global.flags.PrimeRecycler)
        {
            threadContext->EnsureRecycler()->Prime();
        }
#endif

#if ENABLE_TTD
        if(optRecordUri != nullptr || optDebugUri != nullptr)
        {
            AssertMsg(optRecordUri == nullptr || optDebugUri == nullptr, "We should only have 1 but we fail on context create if both are set");

            threadContext->IsTTRequested = true;
            threadContext->IsTTRecordRequested = (optRecordUri != nullptr);
            threadContext->IsTTDebugRequested = (optDebugUri != nullptr);

            wchar_t* uriOrig = (optRecordUri != nullptr) ? optRecordUri : optDebugUri;
            size_t uriOrigLength = wcslen(uriOrig) + 1;
            size_t uriOrigLengthBytes = uriOrigLength * sizeof(wchar_t);
            wchar_t* uriCopy = HeapNewArrayZ(wchar_t, uriOrigLength);
            js_memcpy_s(uriCopy, uriOrigLengthBytes, uriOrig, uriOrigLengthBytes);

            threadContext->TTDUri = uriCopy;
            threadContext->TTSnapInterval = snapInterval;
            threadContext->TTSnapHistoryLength = snapHistoryLength;
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
JsErrorCode CreateContextCore(_In_ JsRuntimeHandle runtimeHandle, _In_ bool createUnderTT, _Out_ JsContextRef *newContext)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        PARAM_NOT_NULL(newContext);
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

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
        if(createUnderTT & !threadContext->IsTTRequested)
        {
            AssertMsg(false, "Can't create a context under TT if runtime is not set to support TT!!!");
            return JsErrorCategoryUsage;
        }

        if(createUnderTT & (threadContext->IsTTRecordRequested & threadContext->IsTTDebugRequested))
        {
            AssertMsg(false, "Can't set both of these at the same time!!!");
            return JsErrorCategoryUsage;
        }
#endif

        JsrtContext * context = JsrtContext::New(runtime);

#if ENABLE_TTD
        if(createUnderTT && threadContext->IsTTRequested && !threadContext->IsTTDInitialized())
        {
#if ENABLE_TTD_DEBUGGING
            threadContext->SetThreadContextFlag(ThreadContextFlagNoJIT);
#endif

            wchar_t* ttdlogStr = nullptr;
            threadContext->TTDInitializeTTDUriFunction(threadContext->TTDUri, &ttdlogStr);

            threadContext->InitTimeTravel(ttdlogStr, threadContext->IsTTRecordRequested, threadContext->IsTTDebugRequested, threadContext->TTSnapInterval, threadContext->TTSnapHistoryLength);

            CoTaskMemFree(ttdlogStr);

            if(threadContext->IsTTDebugRequested && Js::Configuration::Global.flags.TTDCmdsFromFile != nullptr)
            {
                uint32 cmdFileLen = (uint32)wcslen(Js::Configuration::Global.flags.TTDCmdsFromFile) + 1;
                char* cmdFile = HeapNewArrayZ(char, cmdFileLen);

                for(uint32 i = 0; i < cmdFileLen; ++i)
                {
                    cmdFile[i] = (char)Js::Configuration::Global.flags.TTDCmdsFromFile[i];
                }

                FILE* stream;
                freopen_s(&stream, cmdFile, "r", stdin);

                HeapDeleteArray(cmdFileLen, cmdFile);
            }
        }

        if(createUnderTT)
        {
            HostScriptContextCallbackFunctor callbackFunctor(context, &JsrtContext::OnScriptLoad_TTDCallback);
            threadContext->BeginCtxTimeTravel(context->GetScriptContext(), callbackFunctor);

            if(threadContext->IsTTRecordRequested)
            {
                //
                //TODO: We currently force this into debug mode in record as well to make sure parsing/bytecode generation is same as during replay.
                //      Later we will want to be clever during inflate and not do this.
                //
                context->GetScriptContext()->GetDebugContext()->SetInDebugMode();
            }

            context->GetScriptContext()->InitializeCoreImage_TTD();
        }
#endif

        if(runtime->GetDebugObject() != nullptr)
        {
            context->GetScriptContext()->InitializeDebugging();
            context->GetScriptContext()->GetDebugContext()->GetProbeContainer()->InitializeInlineBreakEngine(runtime->GetDebugObject());
            context->GetScriptContext()->GetDebugContext()->GetProbeContainer()->InitializeDebuggerScriptOptionCallback(runtime->GetDebugObject());
            threadContext->GetDebugManager()->SetLocalsDisplayFlags(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods);
        }

        *newContext = (JsContextRef)context;
        return JsNoError;
    });
}

//A call function method that we can funnel to for regular and causal aware invokes
JsErrorCode CallFunctionCore(_In_ INT64 hostCallbackId, _In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_opt_ JsValueRef *result)
{
    if(result != nullptr)
    {
        *result = nullptr;
    }

    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_FUNCTION(function, scriptContext);

        if(cargs == 0 || args == nullptr) {
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
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            TTD::TTDRecordJsRTFunctionCallActionPopper actionPopper(threadContext->TTDLog, scriptContext);
            TTD::JsRTCallFunctionBeginAction* callAction = threadContext->TTDLog->RecordJsRTCallFunctionBegin(scriptContext, scriptContext->TTDRootNestingCount, hostCallbackId, actionPopper.GetStartTime(), jsFunction, cargs, jsArgs.Values);
            actionPopper.SetCallAction(callAction);

            if(scriptContext->TTDRootNestingCount == 0)
            {
                threadContext->TTDLog->ResetCallStackForTopLevelCall(callAction->GetEventTime(), hostCallbackId);
            }

            Js::Var varResult = jsFunction->CallRootFunction(jsArgs, scriptContext, true);
            if(result != nullptr)
            {
                *result = varResult;
                Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));
            }

            actionPopper.NormalReturn();
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

        if(result != nullptr)
        {
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
        }

        //put this here in the hope that after handling an event there is an idle period where we can work without blocking user work
        //May want to look into more sophisticated means for making this decision later
        if(threadContext->TTDLog != nullptr && scriptContext->TTDRootNestingCount == 0 && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            if(threadContext->TTDLog->IsTimeForSnapshot())
            {
                threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
                threadContext->TTDLog->DoSnapshotExtract();
                threadContext->TTDLog->PruneLogLength();
                threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
            }
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

/////////////////////

STDAPI_(JsErrorCode) JsCreateRuntime(_In_ JsRuntimeAttributes attributes, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtimeHandle)
{
    return CreateRuntimeCore(attributes, nullptr, nullptr, UINT_MAX, UINT_MAX, threadService, runtimeHandle);
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

STDAPI_(JsErrorCode) JsCollectGarbage(_In_ JsRuntimeHandle runtimeHandle)
{
    return JsCollectGarbageCommon<CollectNowExhaustive>(runtimeHandle);
}

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
STDAPI_(JsErrorCode) JsPrivateCollectGarbageSkipStack(_In_ JsRuntimeHandle runtimeHandle)
{
    return JsCollectGarbageCommon<CollectNowExhaustiveSkipStack>(runtimeHandle);
}
#endif

STDAPI_(JsErrorCode) JsDisposeRuntime(_In_ JsRuntimeHandle runtimeHandle)
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

        // Close any open Contexts.
        // We need to do this before recycler shutdown, because ScriptEngine->Close won't work then.
        runtime->CloseContexts();

        runtime->ClearDebugObject();

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

STDAPI_(JsErrorCode) JsAddRef(_In_ JsRef ref, _Out_opt_ unsigned int *count)
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
        return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode
        {
            Recycler * recycler = scriptContext->GetRecycler();

            // Note, some references may live in arena-allocated memory, so we need to do this check
            if (!recycler->IsValidObject(ref))
            {
                return JsNoError;
            }

            recycler->RootAddRef(ref, count);
            return JsNoError;
        },
        /*allowInObjectBeforeCollectCallback*/true);
    }
}

STDAPI_(JsErrorCode) JsRelease(_In_ JsRef ref, _Out_opt_ unsigned int *count)
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
        return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode
        {
            Recycler * recycler = scriptContext->GetRecycler();

            // Note, some references may live in arena-allocated memory, so we need to do this check
            if (!recycler->IsValidObject(ref))
            {
                return JsNoError;
            }
            recycler->RootRelease(ref, count);
            return JsNoError;
        },
        /*allowInObjectBeforeCollectCallback*/true);
    }
}

STDAPI_(JsErrorCode) JsSetObjectBeforeCollectCallback(_In_ JsRef ref, _In_opt_ void *callbackState, _In_ JsObjectBeforeCollectCallback objectBeforeCollectCallback)
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
            Recycler * recycler = static_cast<JsrtContext *>(ref)->GetRuntime()->GetThreadContext()->GetRecycler();
            recycler->SetObjectBeforeCollectCallback(ref, reinterpret_cast<Recycler::ObjectBeforeCollectCallback>(objectBeforeCollectCallback), callbackState);
            return JsNoError;
        });
    }
    else
    {
        return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode
        {
            Recycler * recycler = scriptContext->GetRecycler();
            if (!recycler->IsValidObject(ref))
            {
                return JsErrorInvalidArgument;
            }

            recycler->SetObjectBeforeCollectCallback(ref, reinterpret_cast<Recycler::ObjectBeforeCollectCallback>(objectBeforeCollectCallback), callbackState);
            return JsNoError;
        },
        /*allowInObjectBeforeCollectCallback*/true);
    }
}

STDAPI_(JsErrorCode) JsCreateContext(_In_ JsRuntimeHandle runtimeHandle, _Out_ JsContextRef *newContext)
{
    return CreateContextCore(runtimeHandle, false, newContext);
}

STDAPI_(JsErrorCode) JsGetCurrentContext(_Out_ JsContextRef *currentContext)
{
    PARAM_NOT_NULL(currentContext);

    BEGIN_JSRT_NO_EXCEPTION
    {
      *currentContext = (JsContextRef)JsrtContext::GetCurrent();
    }
    END_JSRT_NO_EXCEPTION
}

STDAPI_(JsErrorCode) JsSetCurrentContext(_In_ JsContextRef newContext)
{
    return GlobalAPIWrapper([&] () -> JsErrorCode {
        JsrtContext *currentContext = JsrtContext::GetCurrent();

        if (currentContext && currentContext->GetScriptContext()->GetRecycler()->IsHeapEnumInProgress())
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

STDAPI_(JsErrorCode) JsGetContextOfObject(_In_ JsValueRef object, _Out_ JsContextRef *context)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(context);

    BEGIN_JSRT_NO_EXCEPTION
    {
        if(!Js::RecyclableObject::Is(object))
        {
            RETURN_NO_EXCEPTION(JsErrorArgumentNotObject);
        }
        Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(object);
        *context = (JsContextRef)obj->GetScriptContext()->GetLibrary()->GetPinnedJsrtContextObject();
    }
    END_JSRT_NO_EXCEPTION
}

STDAPI_(JsErrorCode) JsGetContextData(_In_ JsContextRef context, _Out_ void **data)
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

STDAPI_(JsErrorCode) JsSetContextData(_In_ JsContextRef context, _In_ void *data)
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

#if ENABLE_TTD
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
    {
        AssertMsg(false, "Need to implement support here!!!");
    }
#endif

    Js::JavascriptError* error = Js::JavascriptError::CreateFromCompileScriptException(scriptContext, se);

    Js::JavascriptExceptionObject * exceptionObject = RecyclerNew(scriptContext->GetRecycler(),
        Js::JavascriptExceptionObject, error, scriptContext, nullptr);

    scriptContext->GetThreadContext()->SetRecordedException(exceptionObject);
}

STDAPI_(JsErrorCode) JsGetUndefinedValue(_Out_ JsValueRef *undefinedValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(undefinedValue);

        *undefinedValue = scriptContext->GetLibrary()->GetUndefined();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsGetNullValue(_Out_ JsValueRef *nullValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(nullValue);

        *nullValue = scriptContext->GetLibrary()->GetNull();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsGetTrueValue(_Out_ JsValueRef *trueValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(trueValue);

        *trueValue = scriptContext->GetLibrary()->GetTrue();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsGetFalseValue(_Out_ JsValueRef *falseValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(falseValue);

        *falseValue = scriptContext->GetLibrary()->GetFalse();

        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsBoolToBoolean(_In_ bool value, _Out_ JsValueRef *booleanValue)
{
    return ContextAPINoScriptWrapper([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(booleanValue);

        *booleanValue = value ? scriptContext->GetLibrary()->GetTrue() :
            scriptContext->GetLibrary()->GetFalse();
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsBooleanToBool(_In_ JsValueRef value, _Out_ bool *boolValue)
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

STDAPI_(JsErrorCode) JsConvertValueToBoolean(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        if (Js::JavascriptConversion::ToBool((Js::Var)value, scriptContext))
        {
            *result = scriptContext->GetLibrary()->GetTrue();
            return JsNoError;
        }
        else
        {
            *result = scriptContext->GetLibrary()->GetFalse();
            return JsNoError;
        }
    });
}

STDAPI_(JsErrorCode) JsGetValueType(_In_ JsValueRef value, _Out_ JsValueType *type)
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

STDAPI_(JsErrorCode) JsDoubleToNumber(_In_ double dbl, _Out_ JsValueRef *asValue)
{
    PARAM_NOT_NULL(asValue);
    if (Js::JavascriptNumber::TryToVarFastWithCheck(dbl, asValue)) {
      return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        *asValue = Js::JavascriptNumber::ToVarNoCheck(dbl, scriptContext);
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsIntToNumber(_In_ int intValue, _Out_ JsValueRef *asValue)
{
    PARAM_NOT_NULL(asValue);
    if (Js::JavascriptNumber::TryToVarFast(intValue, asValue))
    {
        return JsNoError;
    }

    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        *asValue = Js::JavascriptNumber::ToVar(intValue, scriptContext);
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsNumberToDouble(_In_ JsValueRef value, _Out_ double *asDouble)
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

STDAPI_(JsErrorCode) JsNumberToInt(_In_ JsValueRef value, _Out_ int *asInt)
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

STDAPI_(JsErrorCode) JsConvertValueToNumber(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

        *result = (JsValueRef)Js::JavascriptOperators::ToNumber((Js::Var)value, scriptContext);
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetStringLength(_In_ JsValueRef value, _Out_ int *length)
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

STDAPI_(JsErrorCode) JsPointerToString(_In_reads_(stringLength) const wchar_t *stringValue, _In_ size_t stringLength, _Out_ JsValueRef *string)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(stringValue);
        PARAM_NOT_NULL(string);

        if (!Js::IsValidCharCount(stringLength))
        {
            Js::JavascriptError::ThrowOutOfMemoryError(scriptContext);
        }

        *string = Js::JavascriptString::NewCopyBuffer(stringValue, static_cast<charcount_t>(stringLength), scriptContext);
        return JsNoError;
    });
}

// TODO: The annotation of stringPtr is wrong.  Need to fix definition in chakrart.h
// The warning is '*stringPtr' could be '0' : this does not adhere to the specification for the function 'JsStringToPointer'.
#pragma warning(suppress:6387)
STDAPI_(JsErrorCode) JsStringToPointer(_In_ JsValueRef stringValue, _Outptr_result_buffer_(*stringLength) const wchar_t **stringPtr, _Out_ size_t *stringLength)
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

STDAPI_(JsErrorCode) JsConvertValueToString(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

        *result = (JsValueRef) Js::JavascriptConversion::ToString((Js::Var)value, scriptContext);
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetGlobalObject(_Out_ JsValueRef *globalObject)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(globalObject);

        *globalObject = (JsValueRef)scriptContext->GetGlobalObject();
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

STDAPI_(JsErrorCode) JsCreateObject(_Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(object);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateBasicObject(scriptContext, true);
        }
#endif

        *object = scriptContext->GetLibrary()->CreateObject();

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *object);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateExternalObject(_In_opt_ void *data, _In_opt_ JsFinalizeCallback finalizeCallback, _Out_ JsValueRef *object)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(object);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateBasicObject(scriptContext, false);
        }
#endif

        *object = RecyclerNewFinalized(scriptContext->GetRecycler(), JsrtExternalObject, RecyclerNew(scriptContext->GetRecycler(), JsrtExternalType, scriptContext, finalizeCallback), data);

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *object);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsConvertValueToObject(_In_ JsValueRef value, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);
        PARAM_NOT_NULL(result);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTVarToObjectConversion(scriptContext, (Js::Var)value);
        }
#endif

        *result = (JsValueRef)Js::JavascriptOperators::ToObject((Js::Var)value, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetPrototype(_In_ JsValueRef object, _Out_ JsValueRef *prototypeObject)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(prototypeObject);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        *prototypeObject = (JsValueRef)Js::JavascriptOperators::OP_GetPrototype(object, scriptContext);
        Assert(*prototypeObject == nullptr || !Js::CrossSite::NeedMarshalVar(*prototypeObject, scriptContext));

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *prototypeObject);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsSetPrototype(_In_ JsValueRef object, _In_ JsValueRef prototypeObject)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_OBJECT_OR_NULL(prototypeObject, scriptContext);

        // We're not allowed to set this.
        if (object == scriptContext->GetLibrary()->GetObjectPrototype())
        {
            return JsErrorInvalidArgument;
        }

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTSetPrototype(scriptContext, object, prototypeObject);
        }
#endif

        Js::JavascriptObject::ChangePrototype(Js::RecyclableObject::FromVar(object), Js::RecyclableObject::FromVar(prototypeObject), true, scriptContext);

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsInstanceOf(_In_ JsValueRef object, _In_ JsValueRef constructor, _Out_ bool *result) {
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(constructor, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::RecyclableObject::FromVar(constructor)->HasInstance(object, scriptContext) ? true : false;

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetExtensionAllowed(_In_ JsValueRef object, _Out_ bool *value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(value);
        *value = nullptr;

        *value = Js::RecyclableObject::FromVar(object)->IsExtensible() != 0;

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsPreventExtension(_In_ JsValueRef object)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        Js::RecyclableObject::FromVar(object)->PreventExtensions();

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(value);
        *value = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetProperty(scriptContext, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), object);
        }
#endif

        *value = Js::JavascriptOperators::OP_GetProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext);
        Assert(*value == nullptr || !Js::CrossSite::NeedMarshalVar(*value, scriptContext));

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *value);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetOwnPropertyDescriptor(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *propertyDescriptor)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(propertyDescriptor);
        *propertyDescriptor = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetOwnPropertyInfo(scriptContext, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), object);
        }
#endif

        Js::PropertyDescriptor propertyDescriptorValue;
        if (Js::JavascriptOperators::GetOwnPropertyDescriptor(Js::RecyclableObject::FromVar(object), ((Js::PropertyRecord *)propertyId)->GetPropertyId(), scriptContext, &propertyDescriptorValue))
        {
            *propertyDescriptor = Js::JavascriptOperators::FromPropertyDescriptor(propertyDescriptorValue, scriptContext);

#if ENABLE_TTD
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *propertyDescriptor);
#endif

        }
        else
        {
            *propertyDescriptor = scriptContext->GetLibrary()->GetUndefined();
        }
        Assert(*propertyDescriptor == nullptr || !Js::CrossSite::NeedMarshalVar(*propertyDescriptor, scriptContext));

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetOwnPropertyNames(_In_ JsValueRef object, _Out_ JsValueRef *propertyNames)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertyNames);
        *propertyNames = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetOwnPropertiesInfo(scriptContext, true, object);
        }
#endif

        *propertyNames = Js::JavascriptOperators::GetOwnPropertyNames(object, scriptContext);
        Assert(*propertyNames == nullptr || !Js::CrossSite::NeedMarshalVar(*propertyNames, scriptContext));

#if ENABLE_TTD
        if(*propertyNames != nullptr)
        {
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *propertyNames);
        }
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetOwnPropertySymbols(_In_ JsValueRef object, _Out_ JsValueRef *propertySymbols)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        PARAM_NOT_NULL(propertySymbols);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetOwnPropertiesInfo(scriptContext, false, object);
        }
#endif

        *propertySymbols = Js::JavascriptOperators::GetOwnPropertySymbols(object, scriptContext);
        Assert(*propertySymbols == nullptr || !Js::CrossSite::NeedMarshalVar(*propertySymbols, scriptContext));

#if ENABLE_TTD
        if(*propertySymbols != nullptr)
        {
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *propertySymbols);
        }
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsSetProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef value, _In_ bool useStrictRules)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTSetProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), value, useStrictRules);
        }
#endif

        Js::JavascriptOperators::OP_SetProperty(object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), value, scriptContext,
            nullptr, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsHasProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _Out_ bool *hasProperty)
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

STDAPI_(JsErrorCode) JsDeleteProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ bool useStrictRules, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_PROPERTYID(propertyId);
        PARAM_NOT_NULL(result);
        *result = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTDeleteProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), useStrictRules);
        }
#endif

        *result = Js::JavascriptOperators::OP_DeleteProperty((Js::Var)object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(),
            scriptContext, useStrictRules ? Js::PropertyOperation_StrictMode : Js::PropertyOperation_None);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

#if ENABLE_TTD
        if(*result != nullptr)
        {
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
        }
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsDefineProperty(_In_ JsValueRef object, _In_ JsPropertyIdRef propertyId, _In_ JsValueRef propertyDescriptor, _Out_ bool *result)
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

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTDefineProperty(scriptContext, object, ((Js::PropertyRecord *)propertyId)->GetPropertyId(), propertyDescriptor);
        }
#endif

        *result = Js::JavascriptOperators::DefineOwnPropertyDescriptor(
            Js::RecyclableObject::FromVar(object), ((Js::PropertyRecord *)propertyId)->GetPropertyId(), propertyDescriptorValue,
            true, scriptContext) != 0;

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateArray(_In_ unsigned int length, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);
        *result = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateBasicClearArray(scriptContext, Js::TypeIds_Array, length);
        }
#endif

        *result = scriptContext->GetLibrary()->CreateArray(length);

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateArrayBuffer(_In_ unsigned int byteLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateArrayBuffer(byteLength);

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateExternalArrayBuffer(_Pre_maybenull_ _Pre_writable_byte_size_(byteLength) void *data, _In_ unsigned int byteLength,
    _In_opt_ JsFinalizeCallback finalizeCallback, _In_opt_ void *callbackState, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(result);

        if (data == nullptr && byteLength > 0)
        {
            return JsErrorInvalidArgument;
        }

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateArrayBuffer(scriptContext, reinterpret_cast<BYTE*>(data), byteLength);
        }
#endif

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = Js::JsrtExternalArrayBuffer::New(
            reinterpret_cast<BYTE*>(data),
            byteLength,
            finalizeCallback,
            callbackState,
            library->GetArrayBufferType());

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateTypedArray(_In_ JsTypedArrayType arrayType, _In_ JsValueRef baseArray, _In_ unsigned int byteOffset,
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

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

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

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        JS_ETW(EventWriteJSCRIPT_RECYCLER_ALLOCATE_OBJECT(*result));
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateDataView(_In_ JsValueRef arrayBuffer, _In_ unsigned int byteOffset, _In_ unsigned int byteLength, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(arrayBuffer, scriptContext);
        PARAM_NOT_NULL(result);

        if (!Js::ArrayBuffer::Is(arrayBuffer))
        {
            return JsErrorInvalidArgument;
        }

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        *result = library->CreateDataView(Js::ArrayBuffer::FromVar(arrayBuffer), byteOffset, byteLength);

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

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

STDAPI_(JsErrorCode) JsGetTypedArrayInfo(_In_ JsValueRef typedArray, _Out_opt_ JsTypedArrayType *arrayType, _Out_opt_ JsValueRef *arrayBuffer,
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

#if ENABLE_TTD
        JsrtContext* currentContext = JsrtContext::GetCurrent();
        ThreadContext* threadContext = currentContext->GetScriptContext()->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetTypedArrayInfo(currentContext->GetScriptContext(), arrayBuffer != nullptr, typedArray);
        }
#endif

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

#if ENABLE_TTD
        if(arrayBuffer != nullptr)
        {
            TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *arrayBuffer);
        }
#endif
    }
    END_JSRT_NO_EXCEPTION
}

STDAPI_(JsErrorCode) JsGetArrayBufferStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer,
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

STDAPI_(JsErrorCode) JsGetTypedArrayStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer,
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

STDAPI_(JsErrorCode) JsGetDataViewStorage(_In_ JsValueRef instance, _Outptr_result_bytebuffer_(*bufferLength) BYTE **buffer, _Out_ unsigned int *bufferLength)
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


STDAPI_(JsErrorCode) JsCreateSymbol(_In_ JsValueRef description, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
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
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsHasIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _Out_ bool *result)
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

STDAPI_(JsErrorCode) JsGetIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _Out_ JsValueRef *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        PARAM_NOT_NULL(result);
        *result = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTGetIndex(scriptContext, index, object);
        }
#endif

        *result = (JsValueRef)Js::JavascriptOperators::OP_GetElementI((Js::Var)object, (Js::Var)index, scriptContext);

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsSetIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index, _In_ JsValueRef value)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);
        VALIDATE_INCOMING_REFERENCE(value, scriptContext);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTSetIndex(scriptContext, object, index, value);
        }
#endif

        Js::JavascriptOperators::OP_SetElementI((Js::Var)object, (Js::Var)index, (Js::Var)value, scriptContext);

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsDeleteIndexedProperty(_In_ JsValueRef object, _In_ JsValueRef index)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_OBJECT(object, scriptContext);
        VALIDATE_INCOMING_REFERENCE(index, scriptContext);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

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

    Js::ArrayBuffer* arrayBuffer = RecyclerNew(
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

STDAPI_(JsErrorCode) JsSetIndexedPropertiesToExternalData(
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

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

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

STDAPI_(JsErrorCode) JsHasIndexedPropertiesExternalData(_In_ JsValueRef object, _Out_ bool *value)
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

STDAPI_(JsErrorCode) JsGetIndexedPropertiesExternalData(
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

STDAPI_(JsErrorCode) JsEquals(_In_ JsValueRef object1, _In_ JsValueRef object2, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object1, scriptContext);
        VALIDATE_INCOMING_REFERENCE(object2, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::JavascriptOperators::Equal((Js::Var)object1, (Js::Var)object2, scriptContext) != 0;
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsStrictEquals(_In_ JsValueRef object1, _In_ JsValueRef object2, _Out_ bool *result)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(object1, scriptContext);
        VALIDATE_INCOMING_REFERENCE(object2, scriptContext);
        PARAM_NOT_NULL(result);

        *result = Js::JavascriptOperators::StrictEqual((Js::Var)object1, (Js::Var)object2, scriptContext) != 0;
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsHasExternalData(_In_ JsValueRef object, _Out_ bool *value)
{
    VALIDATE_JSREF(object);
    PARAM_NOT_NULL(value);

    BEGIN_JSRT_NO_EXCEPTION
    {
        *value = JsrtExternalObject::Is(object);
    }
    END_JSRT_NO_EXCEPTION
}

STDAPI_(JsErrorCode) JsGetExternalData(_In_ JsValueRef object, _Out_ void **data)
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

STDAPI_(JsErrorCode) JsSetExternalData(_In_ JsValueRef object, _In_opt_ void *data)
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

STDAPI_(JsErrorCode) JsCallFunction(_In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_opt_ JsValueRef *result)
{
    return CallFunctionCore(-1, function, args, cargs, result);
}

STDAPI_(JsErrorCode) JsConstructObject(_In_ JsValueRef function, _In_reads_(cargs) JsValueRef *args, _In_ ushort cargs, _Out_ JsValueRef *result)
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

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTConstructCall(scriptContext, jsFunction, cargs, args);
        }
#endif

        *result = Js::JavascriptFunction::CallAsConstructor(jsFunction, /* overridingNewTarget = */nullptr, jsArgs, scriptContext);
        Assert(*result == nullptr || !Js::CrossSite::NeedMarshalVar(*result, scriptContext));

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateFunction(_In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext *scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateFunction(scriptContext, false, nullptr);
        }
#endif

        Js::JavascriptExternalFunction *externalFunction = scriptContext->GetLibrary()->CreateStdCallExternalFunction((Js::StdCallJavascriptMethod)nativeFunction, 0, callbackState);
        *function = (JsValueRef)externalFunction;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *function);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateNamedFunction(_In_ JsValueRef name, _In_ JsNativeFunction nativeFunction, _In_opt_ void *callbackState, _Out_ JsValueRef *function)
{
    return ContextAPIWrapper<true>([&](Js::ScriptContext *scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(name, scriptContext);
        PARAM_NOT_NULL(nativeFunction);
        PARAM_NOT_NULL(function);
        *function = nullptr;


#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            threadContext->TTDLog->RecordJsRTAllocateFunction(scriptContext, true, name);
        }
#endif

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

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *function);
#endif

        return JsNoError;
    });
}

void SetErrorMessage(Js::ScriptContext *scriptContext, JsValueRef newError, JsValueRef message)
{
    Js::JavascriptOperators::OP_SetProperty(newError, Js::PropertyIds::message, message, scriptContext);
}

STDAPI_(JsErrorCode) JsCreateError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        JsValueRef newError = scriptContext->GetLibrary()->CreateError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateRangeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        newError = scriptContext->GetLibrary()->CreateRangeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateReferenceError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        newError = scriptContext->GetLibrary()->CreateReferenceError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateSyntaxError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        newError = scriptContext->GetLibrary()->CreateSyntaxError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateTypeError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        newError = scriptContext->GetLibrary()->CreateTypeError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsCreateURIError(_In_ JsValueRef message, _Out_ JsValueRef *error)
{
    return ContextAPIWrapper<true>([&] (Js::ScriptContext * scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(message, scriptContext);
        PARAM_NOT_NULL(error);
        *error = nullptr;

        JsValueRef newError;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        newError = scriptContext->GetLibrary()->CreateURIError();
        SetErrorMessage(scriptContext, newError, message);
        *error = newError;

#if ENABLE_TTD
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *error);
#endif

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsHasException(_Out_ bool *hasException)
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

STDAPI_(JsErrorCode) JsGetAndClearException(_Out_ JsValueRef *exception)
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

#if ENABLE_TTD
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
    {
        threadContext->TTDLog->RecordJsRTGetAndClearException(scriptContext);
    }
#endif

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
    BEGIN_JS_RUNTIME_CALL_NOT_SCRIPT(currentContext->GetScriptContext())
    {
        TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *exception);
    }
    END_JS_RUNTIME_CALL(currentContext->GetScriptContext())
#endif

    return JsNoError;
}

STDAPI_(JsErrorCode) JsSetException(_In_ JsValueRef exception)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext* scriptContext) -> JsErrorCode {
        VALIDATE_INCOMING_REFERENCE(exception, scriptContext);

        Js::JavascriptExceptionObject *exceptionObject;

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        exceptionObject = RecyclerNew(scriptContext->GetRecycler(), Js::JavascriptExceptionObject, exception, scriptContext, nullptr);

        JsrtContext * context = JsrtContext::GetCurrent();
        JsrtRuntime * runtime = context->GetRuntime();

        scriptContext->RecordException(exceptionObject, runtime->DispatchExceptions());

        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsGetRuntimeMemoryUsage(_In_ JsRuntimeHandle runtimeHandle, _Out_ size_t * memoryUsage)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(memoryUsage);
    *memoryUsage = 0;

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    *memoryUsage = allocPolicyManager->GetUsage();

    return JsNoError;
}

STDAPI_(JsErrorCode) JsSetRuntimeMemoryLimit(_In_ JsRuntimeHandle runtimeHandle, _In_ size_t memoryLimit)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    allocPolicyManager->SetLimit(memoryLimit);

    return JsNoError;
}

STDAPI_(JsErrorCode) JsGetRuntimeMemoryLimit(_In_ JsRuntimeHandle runtimeHandle, _Out_ size_t * memoryLimit)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(memoryLimit);
    *memoryLimit = 0;

    ThreadContext * threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    *memoryLimit = allocPolicyManager->GetLimit();

    return JsNoError;
}

C_ASSERT(JsMemoryAllocate == AllocationPolicyManager::MemoryAllocateEvent::MemoryAllocate);
C_ASSERT(JsMemoryFree == AllocationPolicyManager::MemoryAllocateEvent::MemoryFree);
C_ASSERT(JsMemoryFailure == AllocationPolicyManager::MemoryAllocateEvent::MemoryFailure);
C_ASSERT(JsMemoryFailure == AllocationPolicyManager::MemoryAllocateEvent::MemoryMax);

STDAPI_(JsErrorCode) JsSetRuntimeMemoryAllocationCallback(_In_ JsRuntimeHandle runtime, _In_opt_ void *callbackState, _In_ JsMemoryAllocationCallback allocationCallback)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtime);

    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtime)->GetThreadContext();
    AllocationPolicyManager * allocPolicyManager = threadContext->GetAllocationPolicyManager();

    allocPolicyManager->SetMemoryAllocationCallback(callbackState, (AllocationPolicyManager::PageAllocatorMemoryAllocationCallback)allocationCallback);

    return JsNoError;
}

STDAPI_(JsErrorCode) JsSetRuntimeBeforeCollectCallback(_In_ JsRuntimeHandle runtime, _In_opt_ void *callbackState, _In_ JsBeforeCollectCallback beforeCollectCallback)
{
    return GlobalAPIWrapper([&]() -> JsErrorCode {
        VALIDATE_INCOMING_RUNTIME_HANDLE(runtime);

        JsrtRuntime::FromHandle(runtime)->SetBeforeCollectCallback(beforeCollectCallback, callbackState);
        return JsNoError;
    });
}

STDAPI_(JsErrorCode) JsDisableRuntimeExecution(_In_ JsRuntimeHandle runtimeHandle)
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

STDAPI_(JsErrorCode) JsEnableRuntimeExecution(_In_ JsRuntimeHandle runtimeHandle)
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

STDAPI_(JsErrorCode) JsIsRuntimeExecutionDisabled(_In_ JsRuntimeHandle runtimeHandle, _Out_ bool *isDisabled)
{
    VALIDATE_INCOMING_RUNTIME_HANDLE(runtimeHandle);
    PARAM_NOT_NULL(isDisabled);
    *isDisabled = false;

    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtimeHandle)->GetThreadContext();
    *isDisabled = threadContext->IsExecutionDisabled();
    return JsNoError;
}

STDAPI_(JsErrorCode) JsGetPropertyIdFromName(_In_z_ const wchar_t *name, _Out_ JsPropertyIdRef *propertyId)
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

STDAPI_(JsErrorCode) JsGetPropertyIdFromSymbol(_In_ JsValueRef symbol, _Out_ JsPropertyIdRef *propertyId)
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

STDAPI_(JsErrorCode) JsGetSymbolFromPropertyId(_In_ JsPropertyIdRef propertyId, _Out_ JsValueRef *symbol)
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
STDAPI_(JsErrorCode) JsGetPropertyNameFromId(_In_ JsPropertyIdRef propertyId, _Outptr_result_z_ const wchar_t **name)
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

STDAPI_(JsErrorCode) JsGetPropertyIdType(_In_ JsPropertyIdRef propertyId, _Out_ JsPropertyIdType* propertyIdType)
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


STDAPI_(JsErrorCode) JsGetRuntime(_In_ JsContextRef context, _Out_ JsRuntimeHandle *runtime)
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

STDAPI_(JsErrorCode) JsIdle(_Out_opt_ unsigned int *nextIdleTick)
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

STDAPI_(JsErrorCode) JsSetPromiseContinuationCallback(_In_ JsPromiseContinuationCallback promiseContinuationCallback, _In_opt_ void *callbackState)
{
    return ContextAPINoScriptWrapper([&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(promiseContinuationCallback);

#if ENABLE_TTD
        ThreadContext* threadContext = scriptContext->GetThreadContext();
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            AssertMsg(false, "Need to implement support here!!!");
        }
#endif

        scriptContext->GetLibrary()->SetNativeHostPromiseContinuationFunction((Js::JavascriptLibrary::PromiseContinuationCallback)promiseContinuationCallback, callbackState);
        return JsNoError;
    },
    /*allowInObjectBeforeCollectCallback*/true);
}

JsErrorCode RunScriptCore(INT64 hostCallbackId, const wchar_t *script, JsSourceContext sourceContext, const wchar_t *sourceUrl, bool parseOnly, JsParseScriptAttributes parseAttributes, bool isSourceModule, JsValueRef *result)
{
    Js::JavascriptFunction *scriptFunction;
    CompileScriptException se;
    LoadScriptFlag loadScriptFlag = LoadScriptFlag_None;

    JsErrorCode errorCode = ContextAPINoScriptWrapper(
        [&](Js::ScriptContext * scriptContext) -> JsErrorCode {
        PARAM_NOT_NULL(script);
        PARAM_NOT_NULL(sourceUrl);

        SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(sourceContext, nullptr);

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
            /* ichLimHost          */ static_cast<ULONG>(wcslen(script)), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
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

        scriptFunction = scriptContext->LoadScript((const byte*)script, wcslen(script)* sizeof(wchar_t), &si, &se, &utf8SourceInfo, Js::Constants::GlobalCode, loadScriptFlag);

#if ENABLE_TTD
        //
        //TODO: We may (probably?) want to use the debugger source rundown functionality here instead
        //
        if(scriptContext->GetThreadContext()->TTDLog != nullptr)
        {
            //Make sure we have the body and text information available
            Js::FunctionBody* globalBody = TTD::JsSupport::ForceAndGetFunctionBody(scriptFunction->GetParseableFunctionInfo());

            TTD::NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo* tbfi = HeapNewStruct(TTD::NSSnapValues::TopLevelScriptLoadFunctionBodyResolveInfo);
            TTD::NSSnapValues::ExtractTopLevelLoadedFunctionBodyInfo_InScriptContext(tbfi, globalBody, kmodGlobal, globalBody->GetUtf8SourceInfo()->GetSourceInfoId(), script, (uint32)wcslen(script), loadScriptFlag);
            scriptContext->m_ttdTopLevelScriptLoad.Add(tbfi);

            //walk global body to (1) add functions to pin set (2) build parent map
            BEGIN_JS_RUNTIME_CALL(scriptContext);
            {
                scriptContext->ProcessFunctionBodyOnLoad(globalBody, nullptr);
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
        if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
        {
            //
            //TODO: Module support not implemented yet
            //
            AssertMsg(!isSourceModule, "Modules not implemented in TTD yet!!!");

            threadContext->TTDLog->RecordJsRTCodeParse(scriptContext, loadScriptFlag, scriptFunction, script, sourceUrl);
        }

        TTD::RuntimeThreadInfo::JsRTTagObject(scriptContext->GetThreadContext(), scriptFunction);
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
            ThreadContext* threadContext = scriptContext->GetThreadContext();
            if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
            {
                TTD::TTDRecordJsRTFunctionCallActionPopper actionPopper(threadContext->TTDLog, scriptContext);
                TTD::JsRTCallFunctionBeginAction* callAction = threadContext->TTDLog->RecordJsRTCallFunctionBegin(scriptContext, scriptContext->TTDRootNestingCount, -1, actionPopper.GetStartTime(), scriptFunction, args.Info.Count, args.Values);
                actionPopper.SetCallAction(callAction);

                if(scriptContext->TTDRootNestingCount == 0)
                {
                    threadContext->TTDLog->ResetCallStackForTopLevelCall(callAction->GetEventTime(), hostCallbackId);
                }

                Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);
                if(result != nullptr)
                {
                    *result = varResult;
                }

                actionPopper.NormalReturn();
            }
            else
            {
            Js::Var varResult = scriptFunction->CallRootFunction(args, scriptContext, true);
                if(result != nullptr)
                {
                    *result = varResult;
                }
            }

#if ENABLE_TTD
            if(result != nullptr)
            {
                TTD::RuntimeThreadInfo::JsRTTagObject(threadContext, *result);
            }
#endif

            //Put this here in the hope that after handling an event there is an idle period where we can work without blocking user work
            //May want to look into more sophisticated means for making this decision later
            if(threadContext->TTDLog != nullptr && scriptContext->TTDRootNestingCount == 0 && threadContext->TTDLog->ShouldPerformRecordAction())
            {
                if(threadContext->TTDLog->IsTimeForSnapshot())
                {
                    threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
                    threadContext->TTDLog->DoSnapshotExtract();
                    threadContext->TTDLog->PruneLogLength();
                    threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
                }
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

STDAPI_(JsErrorCode) JsParseScript(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(-1, script, sourceContext, sourceUrl, true, JsParseScriptAttributeNone, false /*isModule*/, result);
}

STDAPI_(JsErrorCode) JsParseScriptWithFlags(_In_z_ const wchar_t *script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _In_ JsParseScriptAttributes parseAttributes, _Out_ JsValueRef *result)
{
    return RunScriptCore(-1, script, sourceContext, sourceUrl, true, parseAttributes, false /*isModule*/, result);
}

STDAPI_(JsErrorCode) JsRunScript(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(-1, script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, false /*isModule*/, result);
}

STDAPI_(JsErrorCode) JsExperimentalApiRunModule(_In_z_ const wchar_t * script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunScriptCore(-1, script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, true, result);
}

JsErrorCode JsSerializeScriptCore(const wchar_t *script, BYTE *functionTable, int functionTableSize, unsigned char *buffer, unsigned long *bufferSize)
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

        if (scriptContext->IsInDebugMode())
        {
            return JsErrorCannotSerializeDebugScript;
        }

        SourceContextInfo * sourceContextInfo = scriptContext->GetSourceContextInfo(JS_SOURCE_CONTEXT_NONE, nullptr);
        Assert(sourceContextInfo != nullptr);

        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ static_cast<ULONG>(wcslen(script)), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };
        bool isSerializeByteCodeForLibrary = false;
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        isSerializeByteCodeForLibrary = JsrtContext::GetCurrent()->GetRuntime()->IsSerializeByteCodeForLibrary();
#endif

        Js::Utf8SourceInfo* sourceInfo = nullptr;
        LoadScriptFlag loadScriptFlag = LoadScriptFlag_disableDeferredParse;
        if (isSerializeByteCodeForLibrary)
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_isByteCodeBufferForLibrary);
        }
        else
        {
            loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_Expression);
        }
        function = scriptContext->LoadScript((const byte*)script, wcslen(script)* sizeof(wchar_t), &si, &se, &sourceInfo, Js::Constants::GlobalCode, loadScriptFlag);
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
        size_t cSourceCodeLength = sourceInfo->GetCbLength(L"JsSerializeScript");

        // trucation of code length can lead to accessing random memory. Reject the call.
        if (cSourceCodeLength > DWORD_MAX)
        {
            return JsErrorOutOfMemory;
        }

        LPCUTF8 utf8Code = sourceInfo->GetSource(L"JsSerializeScript");
        DWORD dwFlags = 0;
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        dwFlags = JsrtContext::GetCurrent()->GetRuntime()->IsSerializeByteCodeForLibrary() ? GENERATE_BYTE_CODE_BUFFER_LIBRARY : 0;
#endif

        BEGIN_TEMP_ALLOCATOR(tempAllocator, scriptContext, L"ByteCodeSerializer");
        HRESULT hr = Js::ByteCodeSerializer::SerializeToBuffer(scriptContext, tempAllocator, static_cast<DWORD>(cSourceCodeLength), utf8Code, 0, nullptr, functionBody, functionBody->GetHostSrcInfo(), false, &buffer, bufferSize, dwFlags);
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

STDAPI_(JsErrorCode) JsSerializeScript(_In_z_ const wchar_t *script, _Out_writes_to_opt_(*bufferSize, *bufferSize) unsigned char *buffer,
    _Inout_ unsigned long *bufferSize)
{
    return JsSerializeScriptCore(script, nullptr, 0, buffer, bufferSize);
}

JsErrorCode RunSerializedScriptCore(const wchar_t *script, JsSerializedScriptLoadSourceCallback scriptLoadCallback,
    JsSerializedScriptUnloadCallback scriptUnloadCallback, unsigned char *buffer, JsSourceContext sourceContext,
    const wchar_t *sourceUrl, bool parseOnly, JsValueRef *result)
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
        LPUTF8 utf8Source = nullptr;
        size_t utf8Length = 0;
        size_t length = 0;

        if (script != nullptr)
        {
            Assert(scriptLoadCallback == nullptr);
            Assert(scriptUnloadCallback == nullptr);
            Js::JsrtSourceHolder::ScriptToUtf8(scriptContext, script, &utf8Source, &utf8Length, &length);
        }
        else
        {
            PARAM_NOT_NULL(scriptLoadCallback);
            PARAM_NOT_NULL(scriptUnloadCallback);
            sourceHolder = RecyclerNewFinalized(scriptContext->GetRecycler(), Js::JsrtSourceHolder, scriptLoadCallback, scriptUnloadCallback, sourceContext);
        }

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
            /* ichLimHost          */ static_cast<ULONG>(length), // OK to truncate since this is used to limit sourceText in debugDocument/compilation errors.
            /* ulCharOffset        */ 0,
            /* mod                 */ kmodGlobal,
            /* grfsi               */ 0
        };

        ulong flags = 0;

        if (CONFIG_FLAG(CreateFunctionProxy) && !scriptContext->IsProfiling())
        {
            flags = fscrAllowFunctionProxy;
        }

        hsi = scriptContext->AddHostSrcInfo(&si);

        if (utf8Source != nullptr)
        {
            hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, utf8Source, hsi, buffer, nullptr, &functionBody);
        }
        else
        {
            hr = Js::ByteCodeSerializer::DeserializeFromBuffer(scriptContext, flags, sourceHolder, hsi, buffer, nullptr, &functionBody);
        }

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

STDAPI_(JsErrorCode) JsParseSerializedScript(_In_z_ const wchar_t * script, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(script, nullptr, nullptr, buffer, sourceContext, sourceUrl, true, result);
}

STDAPI_(JsErrorCode) JsRunSerializedScript(_In_z_ const wchar_t * script, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext,
    _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(script, nullptr, nullptr, buffer, sourceContext, sourceUrl, false, result);
}

STDAPI_(JsErrorCode) JsParseSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback, _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef * result)
{
    return RunSerializedScriptCore(nullptr, scriptLoadCallback, scriptUnloadCallback, buffer, sourceContext, sourceUrl, true, result);
}

STDAPI_(JsErrorCode) JsRunSerializedScriptWithCallback(_In_ JsSerializedScriptLoadSourceCallback scriptLoadCallback, _In_ JsSerializedScriptUnloadCallback scriptUnloadCallback, _In_ unsigned char *buffer, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_opt_ JsValueRef * result)
{
    return RunSerializedScriptCore(nullptr, scriptLoadCallback, scriptUnloadCallback, buffer, sourceContext, sourceUrl, false, result);
}

/////////////////////

#if DBG || ENABLE_DEBUG_CONFIG_OPTIONS

STDAPI_(JsErrorCode) JsTTDCreateRecordRuntime(_In_ JsRuntimeAttributes attributes, _In_ wchar_t* infoUri, _In_ UINT32 snapInterval, _In_ UINT32 snapHistoryLength, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
    return CreateRuntimeCore(attributes, infoUri, nullptr, snapInterval, snapHistoryLength, threadService, runtime);
}

STDAPI_(JsErrorCode) JsTTDCreateDebugRuntime(_In_ JsRuntimeAttributes attributes, _In_ wchar_t* infoUri, _In_opt_ JsThreadServiceCallback threadService, _Out_ JsRuntimeHandle *runtime)
{
    return CreateRuntimeCore(attributes, nullptr, infoUri, UINT_MAX, UINT_MAX, threadService, runtime);
}

STDAPI_(JsErrorCode) JsTTDCreateContext(_In_ JsRuntimeHandle runtime, _Out_ JsContextRef *newContext)
{
    return CreateContextCore(runtime, true, newContext);
}

STDAPI_(JsErrorCode) JsTTDRunScript(_In_ INT64 hostCallbackId, _In_z_ const wchar_t *script, _In_ JsSourceContext sourceContext, _In_z_ const wchar_t *sourceUrl, _Out_ JsValueRef *result)
{
    return RunScriptCore(hostCallbackId, script, sourceContext, sourceUrl, false, JsParseScriptAttributeNone, false /*isModule*/, result);
}

STDAPI_(JsErrorCode) JsTTDCallFunction(_In_ INT64 hostCallbackId, _In_ JsValueRef function, _In_reads_(argumentCount) JsValueRef *arguments, _In_ unsigned short argumentCount, _Out_opt_ JsValueRef *result)
{
    return CallFunctionCore(hostCallbackId, function, arguments, argumentCount, result);
}

STDAPI_(JsErrorCode) JsTTDSetDebuggerForReplay()
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    if(currentContext->GetRuntime()->GetThreadContext()->TTDLog == nullptr)
    {
        AssertMsg(false, "Need to create in TTD mode.");
        return JsErrorCategoryUsage;
    }

    currentContext->GetScriptContext()->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDSetIOCallbacks(_In_ JsRuntimeHandle runtime, _In_ JsTTDInitializeTTDUriCallback ttdInitializeUriFunction, _In_ JsTTDInitializeForWriteLogStreamCallback writeInitializeFunction, _In_ JsTTDGetLogStreamCallback getLogStreamInfo, _In_ JsTTDGetSnapshotStreamCallback getSnapshotStreamInfo, _In_ JsTTDGetSrcCodeStreamCallback getSrcCodeStreamInfo, _In_ JsTTDReadBytesFromStreamCallback readBytesFromStream, _In_ JsTTDWriteBytesToStreamCallback writeBytesToStream, _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream)
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    ThreadContext* threadContext = JsrtRuntime::FromHandle(runtime)->GetThreadContext();

    threadContext->TTDInitializeTTDUriFunction = ttdInitializeUriFunction;
    threadContext->TTDWriteInitializeFunction = writeInitializeFunction;
    threadContext->TTDStreamFunctions.pfGetLogStream = getLogStreamInfo;
    threadContext->TTDStreamFunctions.pfGetSnapshotStream = getSnapshotStreamInfo;
    threadContext->TTDStreamFunctions.pfGetSrcCodeStream = getSrcCodeStreamInfo;
    threadContext->TTDStreamFunctions.pfReadBytesFromStream = readBytesFromStream;
    threadContext->TTDStreamFunctions.pfWriteBytesToStream = writeBytesToStream;
    threadContext->TTDStreamFunctions.pfFlushAndCloseStream = flushAndCloseStream;

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDStartTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Need to create in TTD mode.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDDetached())
    {
        AssertMsg(false, "Cannot re-start TTD after detach.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDActive())
    {
        AssertMsg(false, "Already started TTD.");
        return JsErrorCategoryUsage;
    }

    threadContext->TTDLog->SetGlobalMode(TTD::TTDMode::RecordEnabled);

    threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
    BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
    {
        threadContext->TTDLog->DoSnapshotExtract();
    }
    END_JS_RUNTIME_CALL(scriptContext);
    threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDStopTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Need to create in TTD mode.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDDetached())
    {
        AssertMsg(false, "Already stopped TTD.");
        return JsErrorCategoryUsage;
    }

    if(!threadContext->TTDLog->IsTTDActive())
    {
        AssertMsg(false, "TTD was never started.");
        return JsErrorCategoryUsage;
    }

    threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
    BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
    {
        threadContext->EmitTTDLogIfNeeded();
        threadContext->EndCtxTimeTravel(scriptContext);
    }
    END_JS_RUNTIME_CALL(scriptContext);
    threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDEmitTimeTravelRecording()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Need to create in TTD mode.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDDetached())
    {
        AssertMsg(false, "Already stopped TTD.");
        return JsErrorCategoryUsage;
    }

    if(!threadContext->TTDLog->IsTTDActive())
    {
        AssertMsg(false, "TTD was never started.");
        return JsErrorCategoryUsage;
    }

    threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
    BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
    {
        threadContext->EmitTTDLogIfNeeded();
    }
    END_JS_RUNTIME_CALL(scriptContext);
    threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDStartTimeTravelDebugging()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();
    if(threadContext->TTDLog == nullptr)
    {
        AssertMsg(false, "Need to create in TTD mode.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDDetached())
    {
        AssertMsg(false, "Cannot re-start TTD after detach.");
        return JsErrorCategoryUsage;
    }

    if(threadContext->TTDLog->IsTTDActive())
    {
        AssertMsg(false, "Already started TTD.");
        return JsErrorCategoryUsage;
    }

    scriptContext->GetThreadContext()->TTDLog->SetIntoDebuggingMode();

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    if(currentContext->GetRuntime()->GetThreadContext()->TTDLog != nullptr)
    {
        currentContext->GetRuntime()->GetThreadContext()->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
    }

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDReStartTimeTravelAfterRuntimeOperation()
{
#if !ENABLE_TTD
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    if(currentContext->GetRuntime()->GetThreadContext()->TTDLog != nullptr)
    {
        currentContext->GetRuntime()->GetThreadContext()->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);
    }

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDNotifyHostCallbackCreatedOrCanceled(bool isCancel, bool isRepeating, JsValueRef function, INT64 createdCallbackId)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    ThreadContext* threadContext = scriptContext->GetThreadContext();

    if(threadContext->TTDLog != nullptr && threadContext->TTDLog->ShouldPerformRecordAction())
    {
        Js::JavascriptFunction *jsFunction = (function != nullptr) ? Js::JavascriptFunction::FromVar(function) : nullptr;

        threadContext->TTDLog->RecordJsRTCallbackOperation(scriptContext, isCancel, isRepeating, jsFunction, createdCallbackId);
    }

    return JsNoError;
#endif
}

STDAPI_(JsErrorCode) JsTTDPrepContextsForTopLevelEventMove(JsRuntimeHandle runtimeHandle, INT64 targetEventTime, INT64* targetStartSnapTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    if(scriptContext->GetThreadContext()->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorCategoryUsage;
    }

    //Make sure we don't have any pending recorded exceptions
    if(scriptContext->HasRecordedException())
    {
        scriptContext->GetAndClearRecordedException(nullptr);
    }

    //a special indicator to use the time from the argument flag or log diagnostic report
    if(targetEventTime == -2)
    {
        targetEventTime = scriptContext->GetThreadContext()->TTDLog->GetKthEventTime(Js::Configuration::Global.flags.TTDStartEvent);
    }

    bool createFreshCtxs = false;
    *targetStartSnapTime = scriptContext->GetThreadContext()->TTDLog->FindSnapTimeForEventTime(targetEventTime, &createFreshCtxs);

    if(createFreshCtxs)
    {
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));

            JsrtRuntime * runtime = JsrtRuntime::FromHandle(runtimeHandle);
            ThreadContext * threadContext = runtime->GetThreadContext();

            ThreadContextScope scope(threadContext);
            AssertMsg(scope.IsValid(), "Hmm not cool");

            JsrtContext* oldContext = JsrtContext::GetCurrent();

            JsrtContext* context = JsrtContext::New(runtime);
            JsrtContext::TrySetCurrent(context);

            oldContext->Dispose(false);

            threadContext->TTDLog->UpdateInflateMapForFreshScriptContexts();

            HostScriptContextCallbackFunctor callbackFunctor(context, &JsrtContext::OnScriptLoad_TTDCallback);
            threadContext->BeginCtxTimeTravel(context->GetScriptContext(), callbackFunctor);
            context->GetScriptContext()->GetDebugContext()->SetInDebugMode();

            //initialize the core image but we need to disable debugging while this happens
            threadContext->TTDLog->PushMode(TTD::TTDMode::ExcludedExecution);
            context->GetScriptContext()->InitializeCoreImage_TTD();
            threadContext->TTDLog->PopMode(TTD::TTDMode::ExcludedExecution);

            context->GetScriptContext()->InitializeDebuggingActionsAsNeeded_TTD();

            context->GetScriptContext()->InitializeDebugging();
            context->GetScriptContext()->GetDebugContext()->GetProbeContainer()->InitializeInlineBreakEngine(runtime->GetDebugObject());
            context->GetScriptContext()->GetDebugContext()->GetProbeContainer()->InitializeDebuggerScriptOptionCallback(runtime->GetDebugObject());
            threadContext->GetDebugManager()->SetLocalsDisplayFlags(Js::DebugManager::LocalsDisplayFlags::LocalsDisplayFlags_NoGroupMethods);
        }
        catch(...)
        {
            //something went horribly wrong

            *targetStartSnapTime = -1;
        }
    }

    return (*targetStartSnapTime != -1) ? JsNoError : JsErrorFatal;
#endif
}

STDAPI_(JsErrorCode) JsTTDMoveToTopLevelEvent(INT64 snapshotTime, INT64 eventTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    if(scriptContext->GetThreadContext()->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorCategoryUsage;
    }

    TTD::EventLog* elog = scriptContext->GetThreadContext()->TTDLog;
    JsErrorCode res = JsNoError;

    //a special indicator to use the time from the argument flag
    if(eventTime == -2)
    {
        eventTime = scriptContext->GetThreadContext()->TTDLog->GetKthEventTime(Js::Configuration::Global.flags.TTDStartEvent);
    }

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

    return res;
#endif
}

STDAPI_(JsErrorCode) JsTTDReplayExecution(INT64* rootEventTime)
{
#if !ENABLE_TTD_DEBUGGING
    return JsErrorCategoryUsage;
#else
    JsrtContext *currentContext = JsrtContext::GetCurrent();
    Js::ScriptContext* scriptContext = currentContext->GetScriptContext();
    if(scriptContext->GetThreadContext()->TTDLog == nullptr)
    {
        AssertMsg(false, "Should only happen in TT debugging mode.");
        return JsErrorCategoryUsage;
    }

    //If the log has a BP requested then we should set the actual bp here
    if(scriptContext->GetThreadContext()->TTDLog->HasPendingTTDBP())
    {
        TTD::TTDebuggerSourceLocation bpLocation; 
        scriptContext->GetThreadContext()->TTDLog->GetPendingTTDBPInfo(bpLocation);

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
        Js::BreakpointProbe* probe = debugDocument->FindBreakpointId(statement);
        if(probe == nullptr)
        {
            BEGIN_JS_RUNTIME_CALLROOT_EX(scriptContext, false)
            {
                probe = debugDocument->SetBreakPoint(statement, BREAKPOINT_ENABLED);
                AssertMsg(probe != nullptr, "We have a bad line or something for setting a breakpoint.");
            }
            END_JS_RUNTIME_CALL(scriptContext);
        }

        scriptContext->GetThreadContext()->TTDLog->SetActiveBP(probe->GetId(), bpLocation);

        //Finally clear the pending BP info so we don't get confused later
        scriptContext->GetThreadContext()->TTDLog->ClearPendingTTDBPInfo();
    }

    JsErrorCode res = JsNoError;
    try
    {
        scriptContext->GetThreadContext()->TTDLog->ReplayFullTrace();
    }
    catch(TTD::TTDebuggerAbortException abortException)
    {
        //if the debugger bails out with a move time request set info on the requested event time here
        //rest of breakpoint info should have been set by the debugger callback before aborting
        if(abortException.IsEventTimeMove() || abortException.IsTopLevelException())
        {
            *rootEventTime = abortException.GetTargetEventTime();
        }
        else
        {
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

#endif //ENABLE_TTD
