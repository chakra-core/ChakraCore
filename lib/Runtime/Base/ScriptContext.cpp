//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

// Parser Includes
#include "RegexCommon.h"
#include "DebugWriter.h"
#include "RegexStats.h"

#include "ByteCode/ByteCodeApi.h"
#include "Library/ProfileString.h"
#ifdef ENABLE_SCRIPT_DEBUGGING
#include "Debug/DiagHelperMethodWrapper.h"
#endif
#if PROFILE_DICTIONARY
#include "DictionaryStats.h"
#endif

#include "Base/ScriptContextProfiler.h"
#include "Base/EtwTrace.h"

#include "Language/InterpreterStackFrame.h"
#include "Language/SourceDynamicProfileManager.h"
#include "Language/JavascriptStackWalker.h"
#include "Language/AsmJsTypes.h"
#include "Language/AsmJsModule.h"
#ifdef ASMJS_PLAT
#include "Language/AsmJsEncoder.h"
#include "Language/AsmJsCodeGenerator.h"
#include "Language/AsmJsUtils.h"
#endif

#ifdef ENABLE_BASIC_TELEMETRY
#include "ScriptContextTelemetry.h"
#endif

namespace Js
{
    ScriptContext * ScriptContext::New(ThreadContext * threadContext)
    {
        AutoPtr<ScriptContext> scriptContext(HeapNew(ScriptContext, threadContext));
        scriptContext->InitializeAllocations();
        return scriptContext.Detach();
    }

#if ENABLE_NATIVE_CODEGEN
    CriticalSection JITPageAddrToFuncRangeCache::cs;
#endif

    ScriptContext::ScriptContext(ThreadContext* threadContext) :
        ScriptContextBase(),
        prev(nullptr),
        next(nullptr),
        interpreterArena(nullptr),
        moduleSrcInfoCount(0),
        // Regex globals
#if ENABLE_REGEX_CONFIG_OPTIONS
        regexStatsDatabase(0),
        regexDebugWriter(0),
#endif
        trigramAlphabet(nullptr),
        regexStacks(nullptr),
        config(threadContext->GetConfig(), threadContext->IsOptimizedForManyInstances()),
#if ENABLE_BACKGROUND_PARSING
        backgroundParser(nullptr),
#endif
#if ENABLE_NATIVE_CODEGEN
        nativeCodeGen(nullptr),
        m_domFastPathHelperMap(nullptr),
        m_remoteScriptContextAddr(nullptr),
        jitFuncRangeCache(nullptr),
#endif
        threadContext(threadContext),
        scriptStartEventHandler(nullptr),
        scriptEndEventHandler(nullptr),
#ifdef FAULT_INJECTION
        disposeScriptByFaultInjectionEventHandler(nullptr),
#endif
        integerStringMap(this->GeneralAllocator()),
        guestArena(nullptr),
#ifdef ENABLE_SCRIPT_DEBUGGING
        diagnosticArena(nullptr),
        raiseMessageToDebuggerFunctionType(nullptr),
        transitionToDebugModeIfFirstSourceFn(nullptr),
        debugContext(nullptr),
        isDebugContextInitialized(false),
        isEnumeratingRecyclerObjects(false),
#endif
        sourceSize(0),
        deferredBody(false),
        isScriptContextActuallyClosed(false),
        isFinalized(false),
        isEvalRestricted(false),
        isInvalidatedForHostObjects(false),
        fastDOMenabled(false),
        directHostTypeId(TypeIds_GlobalObject),
        isPerformingNonreentrantWork(false),
        isDiagnosticsScriptContext(false),
        m_enumerateNonUserFunctionsOnly(false),
        recycler(threadContext->EnsureRecycler()),
        CurrentThunk(DefaultEntryThunk),
        CurrentCrossSiteThunk(CrossSite::DefaultThunk),
        DeferredParsingThunk(DefaultDeferredParsingThunk),
        DeferredDeserializationThunk(DefaultDeferredDeserializeThunk),
        DispatchDefaultInvoke(nullptr),
        DispatchProfileInvoke(nullptr),
        m_pBuiltinFunctionIdMap(nullptr),
        hostScriptContext(nullptr),
        scriptEngineHaltCallback(nullptr),
#if DYNAMIC_INTERPRETER_THUNK
        interpreterThunkEmitter(nullptr),
#endif
#ifdef ASMJS_PLAT
        asmJsInterpreterThunkEmitter(nullptr),
        asmJsCodeGenerator(nullptr),
#endif
        generalAllocator(_u("SC-General"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
#ifdef ENABLE_BASIC_TELEMETRY
        telemetryAllocator(_u("SC-Telemetry"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
#endif
        dynamicProfileInfoAllocator(_u("SC-DynProfileInfo"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
#ifdef SEPARATE_ARENA
        sourceCodeAllocator(_u("SC-Code"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
        regexAllocator(_u("SC-Regex"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
#endif
#ifdef NEED_MISC_ALLOCATOR
        miscAllocator(_u("GC-Misc"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
#endif
        inlineCacheAllocator(_u("SC-InlineCache"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
        isInstInlineCacheAllocator(_u("SC-IsInstInlineCache"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
        forInCacheAllocator(_u("SC-ForInCache"), threadContext->GetPageAllocator(), Throw::OutOfMemory),
        hasUsedInlineCache(false),
        hasProtoOrStoreFieldInlineCache(false),
        hasIsInstInlineCache(false),
        registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext(nullptr),
        firstInterpreterFrameReturnAddress(nullptr),
        builtInLibraryFunctions(nullptr),
        isWeakReferenceDictionaryListCleared(false)
#if ENABLE_PROFILE_INFO
        , referencesSharedDynamicSourceContextInfo(false)
#endif
#if DBG
        , isInitialized(false)
        , isCloningGlobal(false)
        , bindRef(MiscAllocator())
#endif
#if ENABLE_TTD
        , TTDHostCallbackFunctor()
        , ScriptContextLogTag(TTD_INVALID_LOG_PTR_ID)
        , TTDWellKnownInfo(nullptr)
        , TTDContextInfo(nullptr)
        , TTDSnapshotOrInflateInProgress(false)
        , TTDRecordOrReplayModeEnabled(false)
        , TTDRecordModeEnabled(false)
        , TTDReplayModeEnabled(false)
        , TTDShouldPerformRecordOrReplayAction(false)
        , TTDShouldPerformRecordAction(false)
        , TTDShouldPerformReplayAction(false)
        , TTDShouldPerformDebuggerAction(false)
        , TTDShouldSuppressGetterInvocationForDebuggerEvaluation(false)
#endif
#ifdef REJIT_STATS
        , rejitStatsMap(nullptr)
        , bailoutReasonCounts(nullptr)
        , bailoutReasonCountsCap(nullptr)
        , rejitReasonCounts(nullptr)
        , rejitReasonCountsCap(nullptr)
#endif
#ifdef ENABLE_BASIC_TELEMETRY
        , telemetry(nullptr)
#endif
#ifdef INLINE_CACHE_STATS
        , cacheDataMap(nullptr)
#endif
#ifdef FIELD_ACCESS_STATS
        , fieldAccessStatsByFunctionNumber(nullptr)
#endif
        , webWorkerId(Js::Constants::NonWebWorkerContextId)
        , url(_u(""))
        , startupComplete(false)
#ifdef EDIT_AND_CONTINUE
        , activeScriptEditQuery(nullptr)
#endif
#ifdef RECYCLER_PERF_COUNTERS
        , bindReferenceCount(0)
#endif
        , nextPendingClose(nullptr)
#ifdef ENABLE_SCRIPT_PROFILING
        , heapEnum(nullptr)
        , m_fTraceDomCall(FALSE)
#endif
        , intConstPropsOnGlobalObject(nullptr)
        , intConstPropsOnGlobalUserObject(nullptr)
#ifdef PROFILE_STRINGS
        , stringProfiler(nullptr)
#endif
#ifdef PROFILE_BAILOUT_RECORD_MEMORY
        , codeSize(0)
        , bailOutRecordBytes(0)
        , bailOutOffsetBytes(0)
#endif
        , emptyStringPropertyId(Js::PropertyIds::_none)
    {
#ifdef ENABLE_SCRIPT_DEBUGGING
       // This may allocate memory and cause exception, but it is ok, as we all we have done so far
       // are field init and those dtor will be called if exception occurs
       threadContext->EnsureDebugManager();
#endif
       // Don't use throwing memory allocation in ctor, as exception in ctor doesn't cause the dtor to be called
       // potentially causing memory leaks
       BEGIN_NO_EXCEPTION;

#ifdef RUNTIME_DATA_COLLECTION
        createTime = time(nullptr);
#endif

#ifdef BGJIT_STATS
        interpretedCount = maxFuncInterpret = funcJITCount = bytecodeJITCount = interpretedCallsHighPri = jitCodeUsed = funcJitCodeUsed = loopJITCount = speculativeJitCount = 0;
#endif

#ifdef PROFILE_TYPES
        convertNullToSimpleCount = 0;
        convertNullToSimpleDictionaryCount = 0;
        convertNullToDictionaryCount = 0;
        convertDeferredToDictionaryCount = 0;
        convertDeferredToSimpleDictionaryCount = 0;
        convertSimpleToDictionaryCount = 0;
        convertSimpleToSimpleDictionaryCount = 0;
        convertPathToDictionaryCount1 = 0;
        convertPathToDictionaryCount2 = 0;
        convertPathToDictionaryCount3 = 0;
        convertPathToDictionaryCount4 = 0;
        convertPathToSimpleDictionaryCount = 0;
        convertSimplePathToPathCount = 0;
        convertSimpleDictionaryToDictionaryCount = 0;
        convertSimpleSharedDictionaryToNonSharedCount = 0;
        convertSimpleSharedToNonSharedCount = 0;
        simplePathTypeHandlerCount = 0;
        pathTypeHandlerCount = 0;
        promoteCount = 0;
        cacheCount = 0;
        branchCount = 0;
        maxPathLength = 0;
        memset(typeCount, 0, sizeof(typeCount));
        memset(instanceCount, 0, sizeof(instanceCount));
#endif

#ifdef PROFILE_OBJECT_LITERALS
        objectLiteralInstanceCount = 0;
        objectLiteralPathCount = 0;
        memset(objectLiteralCount, 0, sizeof(objectLiteralCount));
        objectLiteralSimpleDictionaryCount = 0;
        objectLiteralMaxLength = 0;
        objectLiteralPromoteCount = 0;
        objectLiteralCacheCount = 0;
        objectLiteralBranchCount = 0;
#endif
#if DBG_DUMP
        byteCodeDataSize = 0;
        byteCodeAuxiliaryDataSize = 0;
        byteCodeAuxiliaryContextDataSize = 0;
        memset(byteCodeHistogram, 0, sizeof(byteCodeHistogram));
#endif

#if DBG || defined(RUNTIME_DATA_COLLECTION)
        this->allocId = threadContext->GetScriptContextCount();
#endif
#if DBG
        this->hadProfiled = false;
#endif
#if DBG_DUMP
        forinCache = 0;
        forinNoCache = 0;
#endif

        callCount = 0;

        threadContext->GetHiResTimer()->Reset();

#ifdef PROFILE_EXEC
        profiler = nullptr;
        isProfilerCreated = false;
        disableProfiler = false;
        ensureParentInfo = false;
#endif

#ifdef PROFILE_MEM
        profileMemoryDump = true;
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        m_pProfileCallback = nullptr;
        m_pProfileCallback2 = nullptr;
        m_inProfileCallback = FALSE;
        CleanupDocumentContext = nullptr;
#endif

        // Do this after all operations that may cause potential exceptions. Note: InitialAllocations may still throw!
        numberAllocator.Initialize(this->GetRecycler());

#if DEBUG
        m_iProfileSession = -1;
#endif
#ifdef LEAK_REPORT
        this->urlRecord = nullptr;
        this->isRootTrackerScriptContext = false;
#endif

        PERF_COUNTER_INC(Basic, ScriptContext);
        PERF_COUNTER_INC(Basic, ScriptContextActive);

        END_NO_EXCEPTION;
    }

    void ScriptContext::InitializeAllocations()
    {
        this->charClassifier = Anew(GeneralAllocator(), CharClassifier, this);

        this->valueOfInlineCache = AllocatorNewZ(InlineCacheAllocator, GetInlineCacheAllocator(), InlineCache);
        this->toStringInlineCache = AllocatorNewZ(InlineCacheAllocator, GetInlineCacheAllocator(), InlineCache);

#ifdef REJIT_STATS
        rejitReasonCounts = AnewArrayZ(GeneralAllocator(), uint, NumRejitReasons);
        rejitReasonCountsCap = AnewArrayZ(GeneralAllocator(), uint, NumRejitReasons);
        bailoutReasonCounts = Anew(GeneralAllocator(), BailoutStatsMap, GeneralAllocator());
        bailoutReasonCountsCap = Anew(GeneralAllocator(), BailoutStatsMap, GeneralAllocator());
#endif

#ifdef ENABLE_BASIC_TELEMETRY
        this->telemetry = Anew(this->TelemetryAllocator(), ScriptContextTelemetry, *this);
#endif

#ifdef PROFILE_STRINGS
        if (Js::Configuration::Global.flags.ProfileStrings)
        {
            stringProfiler = Anew(MiscAllocator(), StringProfiler, threadContext->GetPageAllocator());
        }
#endif
        intConstPropsOnGlobalObject = Anew(GeneralAllocator(), PropIdSetForConstProp, GeneralAllocator());
        intConstPropsOnGlobalUserObject = Anew(GeneralAllocator(), PropIdSetForConstProp, GeneralAllocator());

#if ENABLE_NATIVE_CODEGEN
        m_domFastPathHelperMap = HeapNew(JITDOMFastPathHelperMap, &HeapAllocator::Instance, 17);
#endif
#ifdef ENABLE_SCRIPT_DEBUGGING
        this->debugContext = HeapNew(DebugContext, this);
#endif
    }

#ifdef ENABLE_SCRIPT_DEBUGGING
    void ScriptContext::EnsureClearDebugDocument()
    {
        if (this->sourceList)
        {
            this->sourceList->Map([=](uint i, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef) {
                Js::Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();
                if (sourceInfo)
                {
                    sourceInfo->ClearDebugDocument();
                }
            });
        }
    }
#endif

    void ScriptContext::ShutdownClearSourceLists()
    {
        if (this->sourceList)
        {
            // In the unclean shutdown case, we might not have destroyed the script context when
            // this is called- in which case, skip doing this work and simply release the source list
            // so that it doesn't show up as a leak. Since we're doing unclean shutdown, it's ok to
            // skip cleanup here for expediency.
            if (this->isClosed)
            {
                this->MapFunction([this](Js::FunctionBody* functionBody) {
                    Assert(functionBody->GetScriptContext() == this);
                    functionBody->CleanupSourceInfo(true);
                });
            }

#ifdef ENABLE_SCRIPT_DEBUGGING
            EnsureClearDebugDocument();
#endif

            // Don't need the source list any more so ok to release
            this->sourceList.Unroot(this->GetRecycler());
        }

        if (this->calleeUtf8SourceInfoList)
        {
            this->calleeUtf8SourceInfoList.Unroot(this->GetRecycler());
        }
    }

    ScriptContext::~ScriptContext()
    {
        // Take etw rundown lock on this thread context. We are going to change/destroy this scriptContext.
        AutoCriticalSection autocs(GetThreadContext()->GetFunctionBodyLock());

#if ENABLE_NATIVE_CODEGEN
        if (m_domFastPathHelperMap != nullptr)
        {
            HeapDelete(m_domFastPathHelperMap);
        }
#endif

        // TODO: Can we move this on Close()?
        ClearHostScriptContext();

        if (this->hasProtoOrStoreFieldInlineCache)
        {
            // TODO (PersistentInlineCaches): It really isn't necessary to clear inline caches in all script contexts.
            // Since this script context is being destroyed, the inline cache arena will also go away and release its
            // memory back to the page allocator.  Thus, we cannot leave this script context's inline caches on the
            // thread context's invalidation lists.  However, it should suffice to remove this script context's caches
            // without touching other script contexts' caches.  We could call some form of RemoveInlineCachesFromInvalidationLists()
            // on the inline cache allocator, which would walk all inline caches and zap values pointed to by strongRef.

            // clear out all inline caches to remove our proto inline caches from the thread context
            threadContext->ClearInlineCaches();

            ClearInlineCaches();
            Assert(!this->hasProtoOrStoreFieldInlineCache);
        }

        if (this->hasIsInstInlineCache)
        {
            // clear out all inline caches to remove our proto inline caches from the thread context
            threadContext->ClearIsInstInlineCaches();
            ClearIsInstInlineCaches();
            Assert(!this->hasIsInstInlineCache);
        }

        // Only call RemoveFromPendingClose if we are in a pending close state.
        if (isClosed && !isScriptContextActuallyClosed)
        {
            threadContext->RemoveFromPendingClose(this);
        }

        SetIsClosed();
        bool closed = Close(true);

        // JIT may access number allocator. Need to close the script context first,
        // which will close the native code generator and abort any current job on this generator.
        numberAllocator.Uninitialize();

        ShutdownClearSourceLists();

        if (regexStacks)
        {
            Adelete(RegexAllocator(), regexStacks);
            regexStacks = nullptr;
        }

        if (javascriptLibrary != nullptr)
        {
            javascriptLibrary->scriptContext = nullptr;
            javascriptLibrary = nullptr;
            if (closed)
            {
                // if we just closed, we haven't unpin the object yet.
                // We need to null out the script context in the global object first
                // before we unpin the global object so that script context dtor doesn't get called twice

#if ENABLE_NATIVE_CODEGEN
                Assert(this->IsClosedNativeCodeGenerator());
#endif
                if (!GetThreadContext()->IsJSRT())
                {
                    this->recycler->RootRelease(globalObject);
                }
            }
        }

        // Normally the JavascriptLibraryBase will unregister the scriptContext from the threadContext.
        // In cases where we don't finish initialization e.g. OOM, manually unregister the scriptContext.
        if (this->IsRegistered())
        {
            threadContext->UnregisterScriptContext(this);
        }

#if ENABLE_BACKGROUND_PARSING
        if (this->backgroundParser != nullptr)
        {
            BackgroundParser::Delete(this->backgroundParser);
            this->backgroundParser = nullptr;
        }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
        if (this->debugContext != nullptr)
        {
            Assert(this->debugContext->IsClosed());
            HeapDelete(this->debugContext);
            this->debugContext = nullptr;
        }
#endif

#if ENABLE_NATIVE_CODEGEN
        if (this->nativeCodeGen != nullptr)
        {
            DeleteNativeCodeGenerator(this->nativeCodeGen);
            nativeCodeGen = NULL;
        }
        if (jitFuncRangeCache != nullptr)
        {
            HeapDelete(jitFuncRangeCache);
            jitFuncRangeCache = nullptr;
        }
#endif

#if DYNAMIC_INTERPRETER_THUNK
        if (this->interpreterThunkEmitter != nullptr)
        {
            HeapDelete(interpreterThunkEmitter);
            this->interpreterThunkEmitter = NULL;
        }
#endif

#ifdef ASMJS_PLAT
        if (this->asmJsInterpreterThunkEmitter != nullptr)
        {
            HeapDelete(asmJsInterpreterThunkEmitter);
            this->asmJsInterpreterThunkEmitter = nullptr;
        }

        if (this->asmJsCodeGenerator != nullptr)
        {
            HeapDelete(asmJsCodeGenerator);
            this->asmJsCodeGenerator = NULL;
        }
#endif

        // In case there is something added to the list between close and dtor, just reset the list again
        this->weakReferenceDictionaryList.Reset();

#if ENABLE_NATIVE_CODEGEN
        if (m_remoteScriptContextAddr)
        {
            Assert(JITManager::GetJITManager()->IsOOPJITEnabled());
            if (JITManager::GetJITManager()->CleanupScriptContext(&m_remoteScriptContextAddr) == S_OK)
            {
                Assert(m_remoteScriptContextAddr == nullptr);
            }
            m_remoteScriptContextAddr = nullptr;
        }
#endif

        PERF_COUNTER_DEC(Basic, ScriptContext);
    }

    void ScriptContext::SetUrl(BSTR bstrUrl)
    {
        // Assumption: this method is never called multiple times
        Assert(this->url != nullptr && wcslen(this->url) == 0);

        charcount_t length = SysStringLen(bstrUrl) + 1; // Add 1 for the NULL.

        char16* urlCopy = AnewArray(this->GeneralAllocator(), char16, length);
        js_memcpy_s(urlCopy, (length - 1) * sizeof(char16), bstrUrl, (length - 1) * sizeof(char16));
        urlCopy[length - 1] = _u('\0');

        this->url = urlCopy;
#ifdef LEAK_REPORT
        if (Js::Configuration::Global.flags.IsEnabled(Js::LeakReportFlag))
        {
            this->urlRecord = LeakReport::LogUrl(urlCopy, this->globalObject);
        }
#endif
    }

    uint ScriptContext::GetNextSourceContextId()
    {

        Assert(this->Cache()->sourceContextInfoMap ||
            this->Cache()->dynamicSourceContextInfoMap);

        uint nextSourceContextId = 0;

        if (this->Cache()->sourceContextInfoMap)
        {
            nextSourceContextId = this->Cache()->sourceContextInfoMap->Count();
        }

        if (this->Cache()->dynamicSourceContextInfoMap)
        {
            nextSourceContextId += this->Cache()->dynamicSourceContextInfoMap->Count();
        }

        return nextSourceContextId + 1;
    }

    // Do most of the Close() work except the final release which could delete the scriptContext.
    void ScriptContext::InternalClose()
    {
        isScriptContextActuallyClosed = true;

        PERF_COUNTER_DEC(Basic, ScriptContextActive);

#if DBG_DUMP
        if (Js::Configuration::Global.flags.TraceWin8Allocations)
        {
            Output::Print(_u("MemoryTrace: ScriptContext Close\n"));
            Output::Flush();
        }
#endif
        JS_ETW_INTERNAL(EventWriteJSCRIPT_HOST_SCRIPT_CONTEXT_CLOSE(this));

#if ENABLE_TTD
        if(this->TTDWellKnownInfo != nullptr)
        {
            TT_HEAP_DELETE(TTD::RuntimeContextInfo, this->TTDWellKnownInfo);
            this->TTDWellKnownInfo = nullptr;
        }

        if(this->TTDContextInfo != nullptr)
        {
            TT_HEAP_DELETE(TTD::ScriptContextTTD, this->TTDContextInfo);
            this->TTDContextInfo = nullptr;
        }
#endif

#if ENABLE_NATIVE_CODEGEN
        if (nativeCodeGen != nullptr)
        {
            Assert(!isInitialized || this->globalObject != nullptr);
            CloseNativeCodeGenerator(this->nativeCodeGen);
        }
#endif
        {
            // Take lock on the function bodies to sync with the etw source rundown if any.
            AutoCriticalSection autocs(GetThreadContext()->GetFunctionBodyLock());
            if (this->sourceList)
            {
                bool hasFunctions = false;
                this->sourceList->MapUntil([&hasFunctions](int, RecyclerWeakReference<Utf8SourceInfo>* sourceInfoWeakRef) -> bool
                {
                    Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();
                    if (sourceInfo)
                    {
                        hasFunctions = sourceInfo->HasFunctions();
                    }

                    return hasFunctions;
                });

                if (hasFunctions)
                {
                    // We still need to walk through all the function bodies and call cleanup
                    // because otherwise ETW events might not get fired if a GC doesn't happen
                    // and the thread context isn't shut down cleanly (process detach case)
                    this->MapFunction([this](Js::FunctionBody* functionBody) {
                        Assert(functionBody->GetScriptContext() == nullptr || functionBody->GetScriptContext() == this);
                        functionBody->Cleanup(/* isScriptContextClosing */ true);
                    });
                }
            }
        }

        this->GetThreadContext()->SubSourceSize(this->GetSourceSize());

#if DYNAMIC_INTERPRETER_THUNK
        if (this->interpreterThunkEmitter != nullptr)
        {
            this->interpreterThunkEmitter->Close();
        }
#endif

#ifdef ASMJS_PLAT
        if (this->asmJsInterpreterThunkEmitter != nullptr)
        {
            this->asmJsInterpreterThunkEmitter->Close();
        }
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        // Stop profiling if present
        DeRegisterProfileProbe(S_OK, nullptr);
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
        this->EnsureClearDebugDocument();

        if (this->debugContext != nullptr)
        {
            if (this->debugContext->GetProbeContainer() != nullptr)
            {
                this->debugContext->GetProbeContainer()->UninstallInlineBreakpointProbe(nullptr);
                this->debugContext->GetProbeContainer()->UninstallDebuggerScriptOptionCallback();
            }

            // Guard the closing DebugContext as in meantime PDM might call OnBreakFlagChange
            AutoCriticalSection autoDebugContextCloseCS(&debugContextCloseCS);
            this->debugContext->Close();
            // Not deleting debugContext here as Close above will clear all memory debugContext allocated.
            // Actual deletion of debugContext will happen in ScriptContext destructor
        }

        if (this->diagnosticArena != nullptr)
        {
            HeapDelete(this->diagnosticArena);
            this->diagnosticArena = nullptr;
        }
#endif

        // Need to print this out before the native code gen is deleted
        // which will delete the codegenProfiler

#ifdef PROFILE_EXEC
        if (Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag))
        {
            if (isProfilerCreated)
            {
                this->ProfilePrint();
            }

            if (profiler != nullptr)
            {
                profiler->Release();
                profiler = nullptr;
            }
        }
#endif


#if ENABLE_PROFILE_INFO
        // Release this only after native code gen is shut down, as there may be
        // profile info allocated from the SourceDynamicProfileManager arena.
        // The first condition might not be true if the dynamic functions have already been freed by the time
        // ScriptContext closes
        if (referencesSharedDynamicSourceContextInfo)
        {
            // For the host provided dynamic code, we may not have added any dynamic context to the dynamicSourceContextInfoMap
            Assert(this->GetDynamicSourceContextInfoMap() != nullptr);
            this->GetThreadContext()->ReleaseSourceDynamicProfileManagers(this->GetUrl());
        }
#endif

        RECYCLER_PERF_COUNTER_SUB(BindReference, bindReferenceCount);

        if (this->interpreterArena)
        {
            ReleaseInterpreterArena();
            interpreterArena = nullptr;
        }

        if (this->guestArena)
        {
            ReleaseGuestArena();
            guestArena = nullptr;
        }

        builtInLibraryFunctions = nullptr;

        pActiveScriptDirect = nullptr;

        isWeakReferenceDictionaryListCleared = true;
        this->weakReferenceDictionaryList.Clear(this->GeneralAllocator());

        if (registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext != nullptr)
        {
            // UnregisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext may throw, set up the correct state first
            ScriptContext ** registeredScriptContext = registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext;
            ClearPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesCaches();
            Assert(registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext == nullptr);
            threadContext->UnregisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext(registeredScriptContext);
        }

#ifdef ENABLE_SCRIPT_DEBUGGING
        threadContext->ReleaseDebugManager();
#endif

        // This can be null if the script context initialization threw
        // and InternalClose gets called in the destructor code path
        if (javascriptLibrary != nullptr)
        {
            javascriptLibrary->CleanupForClose();
            javascriptLibrary->Uninitialize();

            this->ClearScriptContextCaches();
        }
    }

    bool ScriptContext::Close(bool inDestructor)
    {
        if (isScriptContextActuallyClosed)
            return false;

        InternalClose();

        if (!inDestructor && globalObject != nullptr)
        {
            //A side effect of releasing globalObject that this script context could be deleted, so the release call here
            //must be the last thing in close.
#if ENABLE_NATIVE_CODEGEN
            Assert(this->IsClosedNativeCodeGenerator());
#endif
            if (!GetThreadContext()->IsJSRT())
            {
                GetRecycler()->RootRelease(globalObject);
            }
            globalObject = nullptr;
        }

        // A script context closing is a signal to the thread context that it
        // needs to do an idle GC independent of what the heuristics are
        this->threadContext->SetForceOneIdleCollection();

        return true;
    }

    PropertyString* ScriptContext::GetPropertyString2(char16 ch1, char16 ch2)
    {
        if (ch1 < '0' || ch1 > 'z' || ch2 < '0' || ch2 > 'z')
        {
            return NULL;
        }
        const uint i = PropertyStringMap::PStrMapIndex(ch1);
        if (this->Cache()->propertyStrings[i] == NULL)
        {
            return NULL;
        }
        const uint j = PropertyStringMap::PStrMapIndex(ch2);
        return this->Cache()->propertyStrings[i]->strLen2[j];
    }

    void ScriptContext::FindPropertyRecord(JavascriptString *pstName, PropertyRecord const ** propertyRecord)
    {
        threadContext->FindPropertyRecord(pstName, propertyRecord);
    }

    void ScriptContext::FindPropertyRecord(__in LPCWSTR propertyName, __in int propertyNameLength, PropertyRecord const ** propertyRecord)
    {
        threadContext->FindPropertyRecord(propertyName, propertyNameLength, propertyRecord);
    }

    JsUtil::List<const RecyclerWeakReference<Js::PropertyRecord const>*>* ScriptContext::FindPropertyIdNoCase(__in LPCWSTR propertyName, __in int propertyNameLength)
    {
        return threadContext->FindPropertyIdNoCase(this, propertyName, propertyNameLength);
    }

    PropertyId ScriptContext::GetOrAddPropertyIdTracked(JsUtil::CharacterBuffer<WCHAR> const& propName)
    {
        Js::PropertyRecord const * propertyRecord = nullptr;
        threadContext->GetOrAddPropertyId(propName, &propertyRecord);

        this->TrackPid(propertyRecord);

        return propertyRecord->GetPropertyId();
    }

    void ScriptContext::GetOrAddPropertyRecord(JsUtil::CharacterBuffer<WCHAR> const& propertyName, PropertyRecord const ** propertyRecord)
    {
        threadContext->GetOrAddPropertyId(propertyName, propertyRecord);
    }

    PropertyId ScriptContext::GetOrAddPropertyIdTracked(__in_ecount(propertyNameLength) LPCWSTR propertyName, __in int propertyNameLength)
    {
        Js::PropertyRecord const * propertyRecord = nullptr;
        threadContext->GetOrAddPropertyId(propertyName, propertyNameLength, &propertyRecord);
        if (propertyNameLength == 2)
        {
            CachePropertyString2(propertyRecord);
        }
        this->TrackPid(propertyRecord);

        return propertyRecord->GetPropertyId();
    }

    void ScriptContext::GetOrAddPropertyRecord(__in_ecount(propertyNameLength) LPCWSTR propertyName, __in int propertyNameLength, PropertyRecord const ** propertyRecord)
    {
        threadContext->GetOrAddPropertyId(propertyName, propertyNameLength, propertyRecord);
        if (propertyNameLength == 2)
        {
            CachePropertyString2(*propertyRecord);
        }
    }

    BOOL ScriptContext::IsNumericPropertyId(PropertyId propertyId, uint32* value)
    {
        BOOL isNumericPropertyId = threadContext->IsNumericPropertyId(propertyId, value);

#if DEBUG
        PropertyRecord const * name = this->GetPropertyName(propertyId);

        if (name != nullptr)
        {
            // Symbol properties are not numeric - description should not be used.
            if (name->IsSymbol())
            {
                return false;
            }

            uint32 index;
            BOOL isIndex = JavascriptArray::GetIndex(name->GetBuffer(), &index);
            if (isNumericPropertyId != isIndex)
            {
                // WOOB 1137798: JavascriptArray::GetIndex does not handle embedded NULLs. So if we have a property
                // name "1234\0", JavascriptArray::GetIndex would incorrectly accepts it as an array index property
                // name.
                Assert((size_t)(name->GetLength()) != wcslen(name->GetBuffer()));
            }
            else if (isNumericPropertyId)
            {
                Assert((uint32)*value == index);
            }
        }
#endif

        return isNumericPropertyId;
    }

    void ScriptContext::RegisterWeakReferenceDictionary(JsUtil::IWeakReferenceDictionary* weakReferenceDictionary)
    {
        this->weakReferenceDictionaryList.Prepend(this->GeneralAllocator(), weakReferenceDictionary);
    }

    RecyclableObject *ScriptContext::GetMissingPropertyResult()
    {
        return GetLibrary()->GetUndefined();
    }

    RecyclableObject *ScriptContext::GetMissingItemResult()
    {
        return GetLibrary()->GetUndefined();
    }

    SRCINFO *ScriptContext::AddHostSrcInfo(SRCINFO const *pSrcInfo)
    {
        Assert(pSrcInfo != nullptr);

        return RecyclerNewZ(this->GetRecycler(), SRCINFO, *pSrcInfo);
    }

#ifdef PROFILE_TYPES
    void ScriptContext::ProfileTypes()
    {
        Output::Print(_u("===============================================================================\n"));
        Output::Print(_u("Types Profile %s\n"), this->url);
        Output::Print(_u("-------------------------------------------------------------------------------\n"));
        Output::Print(_u("Dynamic Type Conversions:\n"));
        Output::Print(_u("    Null to Simple                 %8d\n"), convertNullToSimpleCount);
        Output::Print(_u("    Deferred to SimpleMap          %8d\n"), convertDeferredToSimpleDictionaryCount);
        Output::Print(_u("    Simple to Map                  %8d\n"), convertSimpleToDictionaryCount);
        Output::Print(_u("    Simple to SimpleMap            %8d\n"), convertSimpleToSimpleDictionaryCount);
        Output::Print(_u("    Path to SimpleMap (set)        %8d\n"), convertPathToDictionaryCount1);
        Output::Print(_u("    Path to SimpleMap (delete)     %8d\n"), convertPathToDictionaryCount2);
        Output::Print(_u("    Path to SimpleMap (attribute)  %8d\n"), convertPathToDictionaryCount3);
        Output::Print(_u("    Path to SimpleMap              %8d\n"), convertPathToSimpleDictionaryCount);
        Output::Print(_u("    SimplePath to Path             %8d\n"), convertSimplePathToPathCount);
        Output::Print(_u("    Shared SimpleMap to non-shared %8d\n"), convertSimpleSharedDictionaryToNonSharedCount);
        Output::Print(_u("    Deferred to Map                %8d\n"), convertDeferredToDictionaryCount);
        Output::Print(_u("    Path to Map (accessor)         %8d\n"), convertPathToDictionaryCount4);
        Output::Print(_u("    SimpleMap to Map               %8d\n"), convertSimpleDictionaryToDictionaryCount);
        Output::Print(_u("    Path Cache Hits                %8d\n"), cacheCount);
        Output::Print(_u("    Path Branches                  %8d\n"), branchCount);
        Output::Print(_u("    Path Promotions                %8d\n"), promoteCount);
        Output::Print(_u("    Path Length (max)              %8d\n"), maxPathLength);
        Output::Print(_u("    SimplePathTypeHandlers         %8d\n"), simplePathTypeHandlerCount);
        Output::Print(_u("    PathTypeHandlers               %8d\n"), pathTypeHandlerCount);
        Output::Print(_u("\n"));
        Output::Print(_u("Type Statistics:                   %8s   %8s\n"), _u("Types"), _u("Instances"));
        Output::Print(_u("    Undefined                      %8d   %8d\n"), typeCount[TypeIds_Undefined], instanceCount[TypeIds_Undefined]);
        Output::Print(_u("    Null                           %8d   %8d\n"), typeCount[TypeIds_Null], instanceCount[TypeIds_Null]);
        Output::Print(_u("    Boolean                        %8d   %8d\n"), typeCount[TypeIds_Boolean], instanceCount[TypeIds_Boolean]);
        Output::Print(_u("    Integer                        %8d   %8d\n"), typeCount[TypeIds_Integer], instanceCount[TypeIds_Integer]);
        Output::Print(_u("    Number                         %8d   %8d\n"), typeCount[TypeIds_Number], instanceCount[TypeIds_Number]);
        Output::Print(_u("    String                         %8d   %8d\n"), typeCount[TypeIds_String], instanceCount[TypeIds_String]);
        Output::Print(_u("    Object                         %8d   %8d\n"), typeCount[TypeIds_Object], instanceCount[TypeIds_Object]);
        Output::Print(_u("    Function                       %8d   %8d\n"), typeCount[TypeIds_Function], instanceCount[TypeIds_Function]);
        Output::Print(_u("    Array                          %8d   %8d\n"), typeCount[TypeIds_Array], instanceCount[TypeIds_Array]);
        Output::Print(_u("    Date                           %8d   %8d\n"), typeCount[TypeIds_Date], instanceCount[TypeIds_Date] + instanceCount[TypeIds_WinRTDate]);
        Output::Print(_u("    Symbol                         %8d   %8d\n"), typeCount[TypeIds_Symbol], instanceCount[TypeIds_Symbol]);
        Output::Print(_u("    RegEx                          %8d   %8d\n"), typeCount[TypeIds_RegEx], instanceCount[TypeIds_RegEx]);
        Output::Print(_u("    Error                          %8d   %8d\n"), typeCount[TypeIds_Error], instanceCount[TypeIds_Error]);
        Output::Print(_u("    Proxy                          %8d   %8d\n"), typeCount[TypeIds_Proxy], instanceCount[TypeIds_Proxy]);
        Output::Print(_u("    BooleanObject                  %8d   %8d\n"), typeCount[TypeIds_BooleanObject], instanceCount[TypeIds_BooleanObject]);
        Output::Print(_u("    NumberObject                   %8d   %8d\n"), typeCount[TypeIds_NumberObject], instanceCount[TypeIds_NumberObject]);
        Output::Print(_u("    StringObject                   %8d   %8d\n"), typeCount[TypeIds_StringObject], instanceCount[TypeIds_StringObject]);
        Output::Print(_u("    SymbolObject                   %8d   %8d\n"), typeCount[TypeIds_SymbolObject], instanceCount[TypeIds_SymbolObject]);
        Output::Print(_u("    GlobalObject                   %8d   %8d\n"), typeCount[TypeIds_GlobalObject], instanceCount[TypeIds_GlobalObject]);
        Output::Print(_u("    Enumerator                     %8d   %8d\n"), typeCount[TypeIds_Enumerator], instanceCount[TypeIds_Enumerator]);
        Output::Print(_u("    Int8Array                      %8d   %8d\n"), typeCount[TypeIds_Int8Array], instanceCount[TypeIds_Int8Array]);
        Output::Print(_u("    Uint8Array                     %8d   %8d\n"), typeCount[TypeIds_Uint8Array], instanceCount[TypeIds_Uint8Array]);
        Output::Print(_u("    Uint8ClampedArray              %8d   %8d\n"), typeCount[TypeIds_Uint8ClampedArray], instanceCount[TypeIds_Uint8ClampedArray]);
        Output::Print(_u("    Int16Array                     %8d   %8d\n"), typeCount[TypeIds_Int16Array], instanceCount[TypeIds_Int16Array]);
        Output::Print(_u("    Int16Array                     %8d   %8d\n"), typeCount[TypeIds_Uint16Array], instanceCount[TypeIds_Uint16Array]);
        Output::Print(_u("    Int32Array                     %8d   %8d\n"), typeCount[TypeIds_Int32Array], instanceCount[TypeIds_Int32Array]);
        Output::Print(_u("    Uint32Array                    %8d   %8d\n"), typeCount[TypeIds_Uint32Array], instanceCount[TypeIds_Uint32Array]);
        Output::Print(_u("    Float32Array                   %8d   %8d\n"), typeCount[TypeIds_Float32Array], instanceCount[TypeIds_Float32Array]);
        Output::Print(_u("    Float64Array                   %8d   %8d\n"), typeCount[TypeIds_Float64Array], instanceCount[TypeIds_Float64Array]);
        Output::Print(_u("    DataView                       %8d   %8d\n"), typeCount[TypeIds_DataView], instanceCount[TypeIds_DataView]);
        Output::Print(_u("    ModuleRoot                     %8d   %8d\n"), typeCount[TypeIds_ModuleRoot], instanceCount[TypeIds_ModuleRoot]);
        Output::Print(_u("    HostObject                     %8d   %8d\n"), typeCount[TypeIds_HostObject], instanceCount[TypeIds_HostObject]);
        Output::Print(_u("    VariantDate                    %8d   %8d\n"), typeCount[TypeIds_VariantDate], instanceCount[TypeIds_VariantDate]);
        Output::Print(_u("    HostDispatch                   %8d   %8d\n"), typeCount[TypeIds_HostDispatch], instanceCount[TypeIds_HostDispatch]);
        Output::Print(_u("    Arguments                      %8d   %8d\n"), typeCount[TypeIds_Arguments], instanceCount[TypeIds_Arguments]);
        Output::Print(_u("    ActivationObject               %8d   %8d\n"), typeCount[TypeIds_ActivationObject], instanceCount[TypeIds_ActivationObject]);
        Output::Print(_u("    Map                            %8d   %8d\n"), typeCount[TypeIds_Map], instanceCount[TypeIds_Map]);
        Output::Print(_u("    Set                            %8d   %8d\n"), typeCount[TypeIds_Set], instanceCount[TypeIds_Set]);
        Output::Print(_u("    WeakMap                        %8d   %8d\n"), typeCount[TypeIds_WeakMap], instanceCount[TypeIds_WeakMap]);
        Output::Print(_u("    WeakSet                        %8d   %8d\n"), typeCount[TypeIds_WeakSet], instanceCount[TypeIds_WeakSet]);
        Output::Print(_u("    ArrayIterator                  %8d   %8d\n"), typeCount[TypeIds_ArrayIterator], instanceCount[TypeIds_ArrayIterator]);
        Output::Print(_u("    MapIterator                    %8d   %8d\n"), typeCount[TypeIds_MapIterator], instanceCount[TypeIds_MapIterator]);
        Output::Print(_u("    SetIterator                    %8d   %8d\n"), typeCount[TypeIds_SetIterator], instanceCount[TypeIds_SetIterator]);
        Output::Print(_u("    StringIterator                 %8d   %8d\n"), typeCount[TypeIds_StringIterator], instanceCount[TypeIds_StringIterator]);
        Output::Print(_u("    Generator                      %8d   %8d\n"), typeCount[TypeIds_Generator], instanceCount[TypeIds_Generator]);
#if !DBG
        Output::Print(_u("    ** Instance statistics only available on debug builds...\n"));
#endif
        Output::Flush();
    }
#endif


#ifdef PROFILE_OBJECT_LITERALS
    void ScriptContext::ProfileObjectLiteral()
    {
        Output::Print(_u("===============================================================================\n"));
        Output::Print(_u("    Object Lit Instances created.. %d\n"), objectLiteralInstanceCount);
        Output::Print(_u("    Object Lit Path Types......... %d\n"), objectLiteralPathCount);
        Output::Print(_u("    Object Lit Simple Map......... %d\n"), objectLiteralSimpleDictionaryCount);
        Output::Print(_u("    Object Lit Max # of properties %d\n"), objectLiteralMaxLength);
        Output::Print(_u("    Object Lit Promote count...... %d\n"), objectLiteralPromoteCount);
        Output::Print(_u("    Object Lit Cache Hits......... %d\n"), objectLiteralCacheCount);
        Output::Print(_u("    Object Lit Branch count....... %d\n"), objectLiteralBranchCount);

        for (int i = 0; i < TypePath::MaxPathTypeHandlerLength; i++)
        {
            if (objectLiteralCount[i] != 0)
            {
                Output::Print(_u("    Object Lit properties [ %2d] .. %d\n"), i, objectLiteralCount[i]);
            }
        }

        Output::Flush();
    }
#endif

    //
    // Regex helpers
    //

#if ENABLE_REGEX_CONFIG_OPTIONS
    UnifiedRegex::RegexStatsDatabase* ScriptContext::GetRegexStatsDatabase()
    {
        if (regexStatsDatabase == 0)
        {
            ArenaAllocator* allocator = MiscAllocator();
            regexStatsDatabase = Anew(allocator, UnifiedRegex::RegexStatsDatabase, allocator);
        }
        return regexStatsDatabase;
    }

    UnifiedRegex::DebugWriter* ScriptContext::GetRegexDebugWriter()
    {
        if (regexDebugWriter == 0)
        {
            ArenaAllocator* allocator = MiscAllocator();
            regexDebugWriter = Anew(allocator, UnifiedRegex::DebugWriter);
        }
        return regexDebugWriter;
    }
#endif

    void ScriptContext::RedeferFunctionBodies(ActiveFunctionSet *pActiveFuncs, uint inactiveThreshold)
    {
        Assert(!this->IsClosed());

        if (!this->IsScriptContextInNonDebugMode())
        {
            return;
        }

        // For each active function, collect call counts, update inactive counts, and redefer if appropriate.
        // In the redeferral case, we require 2 passes over the set of FunctionBody's.
        // This is because a function inlined in a non-redeferred function cannot itself be redeferred.
        // So we first need to close over the set of non-redeferrable functions, then go back and redefer
        // the eligible candidates.

        auto fn = [&](FunctionBody *functionBody) {
            bool exec = functionBody->InterpretedSinceCallCountCollection();
            functionBody->CollectInterpretedCounts();
            functionBody->MapEntryPoints([&](int index, FunctionEntryPointInfo *entryPointInfo) {
                if (!entryPointInfo->IsCleanedUp() && entryPointInfo->ExecutedSinceCallCountCollection())
                {
                    exec = true;
                }
                entryPointInfo->CollectCallCounts();
            });
            if (exec)
            {
                functionBody->SetInactiveCount(0);
            }
            else
            {
                functionBody->IncrInactiveCount(inactiveThreshold);
            }

            if (pActiveFuncs)
            {
                Assert(this->GetThreadContext()->DoRedeferFunctionBodies());
                bool doRedefer = functionBody->DoRedeferFunction(inactiveThreshold);
                if (!doRedefer)
                {
                    functionBody->UpdateActiveFunctionSet(pActiveFuncs, nullptr);
                }
            }
        };

        this->MapFunction(fn);

        if (!pActiveFuncs)
        {
            return;
        }

        auto fnRedefer = [&](FunctionBody * functionBody) {
            Assert(pActiveFuncs);
            if (!functionBody->IsActiveFunction(pActiveFuncs))
            {
                Assert(functionBody->DoRedeferFunction(inactiveThreshold));
                functionBody->RedeferFunction();
            }
            else
            {
                functionBody->ResetRedeferralAttributes();
            }
        };

        this->MapFunction(fnRedefer);
    }

    bool ScriptContext::DoUndeferGlobalFunctions() const
    {
        return CONFIG_FLAG(DeferTopLevelTillFirstCall) && !AutoSystemInfo::Data.IsLowMemoryProcess();
    }

    RegexPatternMruMap* ScriptContext::GetDynamicRegexMap() const
    {
        Assert(!isScriptContextActuallyClosed);
        Assert(Cache()->dynamicRegexMap);

        return Cache()->dynamicRegexMap;
    }

    void ScriptContext::SetTrigramAlphabet(UnifiedRegex::TrigramAlphabet * trigramAlphabet)
    {
        this->trigramAlphabet = trigramAlphabet;
    }

    UnifiedRegex::RegexStacks *ScriptContext::RegexStacks()
    {
        UnifiedRegex::RegexStacks * stacks = regexStacks;
        if (stacks)
        {
            return stacks;
        }
        return AllocRegexStacks();
    }

    UnifiedRegex::RegexStacks * ScriptContext::AllocRegexStacks()
    {
        Assert(this->regexStacks == nullptr);
        UnifiedRegex::RegexStacks * stacks = Anew(RegexAllocator(), UnifiedRegex::RegexStacks, threadContext->GetPageAllocator());
        this->regexStacks = stacks;
        return stacks;
    }

    UnifiedRegex::RegexStacks *ScriptContext::SaveRegexStacks()
    {
        Assert(regexStacks);

        const auto saved = regexStacks;
        regexStacks = nullptr;
        return saved;
    }

    void ScriptContext::RestoreRegexStacks(UnifiedRegex::RegexStacks *const stacks)
    {
        Assert(stacks);
        Assert(stacks != regexStacks);

        if (regexStacks)
        {
            Adelete(RegexAllocator(), regexStacks);
        }
        regexStacks = stacks;
    }

    Js::TempArenaAllocatorObject* ScriptContext::GetTemporaryAllocator(LPCWSTR name)
    {
        return this->threadContext->GetTemporaryAllocator(name);
    }

    void ScriptContext::ReleaseTemporaryAllocator(Js::TempArenaAllocatorObject* tempAllocator)
    {
        AssertMsg(tempAllocator != nullptr, "tempAllocator should not be null");

        this->threadContext->ReleaseTemporaryAllocator(tempAllocator);
    }

    Js::TempGuestArenaAllocatorObject* ScriptContext::GetTemporaryGuestAllocator(LPCWSTR name)
    {
        return this->threadContext->GetTemporaryGuestAllocator(name);
    }

    void ScriptContext::ReleaseTemporaryGuestAllocator(Js::TempGuestArenaAllocatorObject* tempGuestAllocator)
    {
        AssertMsg(tempGuestAllocator != nullptr, "tempAllocator should not be null");

        this->threadContext->ReleaseTemporaryGuestAllocator(tempGuestAllocator);
    }

    void ScriptContext::InitializeCache()
    {

#if ENABLE_PROFILE_INFO
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
        if (DynamicProfileInfo::NeedProfileInfoList())
        {
            this->Cache()->profileInfoList = RecyclerNew(this->GetRecycler(), DynamicProfileInfoList);
        }
#endif
#endif

        ClearArray(this->Cache()->propertyStrings, 80);
        this->Cache()->dynamicRegexMap =
            RegexPatternMruMap::New(
                recycler,
                REGEX_CONFIG_FLAG(DynamicRegexMruListSize) <= 0 ? 16 : REGEX_CONFIG_FLAG(DynamicRegexMruListSize));

        SourceContextInfo* sourceContextInfo = RecyclerNewStructZ(this->GetRecycler(), SourceContextInfo);
        sourceContextInfo->dwHostSourceContext = Js::Constants::NoHostSourceContext;
        sourceContextInfo->isHostDynamicDocument = false;
        sourceContextInfo->sourceContextId = Js::Constants::NoSourceContext;
        this->Cache()->noContextSourceContextInfo = sourceContextInfo;

        SRCINFO* srcInfo = RecyclerNewStructZ(this->GetRecycler(), SRCINFO);
        srcInfo->sourceContextInfo = this->Cache()->noContextSourceContextInfo;
        srcInfo->moduleID = kmodGlobal;
        this->Cache()->noContextGlobalSourceInfo = srcInfo;
    }

    void ScriptContext::InitializePreGlobal()
    {
        this->guestArena = this->GetRecycler()->CreateGuestArena(_u("Guest"), Throw::OutOfMemory);

#if ENABLE_BACKGROUND_PARSING
        if (PHASE_ON1(Js::ParallelParsePhase))
        {
            this->backgroundParser = BackgroundParser::New(this);
        }
#endif

#if ENABLE_NATIVE_CODEGEN
        // Create the native code gen before the profiler
        this->nativeCodeGen = NewNativeCodeGenerator(this);
        this->jitFuncRangeCache = HeapNew(JITPageAddrToFuncRangeCache);
#endif

#ifdef PROFILE_EXEC
        this->CreateProfiler();
#endif

        this->operationStack = Anew(GeneralAllocator(), JsUtil::Stack<Var>, GeneralAllocator());

        Tick::InitType();
    }

    void ScriptContext::Initialize()
    {
        SmartFPUControl defaultControl;

        InitializePreGlobal();

        InitializeGlobalObject();

        InitializePostGlobal();
    }

    void ScriptContext::InitializePostGlobal()
    {
#ifdef ENABLE_SCRIPT_DEBUGGING
        this->GetDebugContext()->Initialize();

        this->GetDebugContext()->GetProbeContainer()->Initialize(this);

        isDebugContextInitialized = true;
#endif

#if defined(_M_ARM32_OR_ARM64)
        // We need to ensure that the above write to the isDebugContextInitialized is visible to the debugger thread.
        MemoryBarrier();
#endif

        AssertMsg(this->CurrentThunk == DefaultEntryThunk, "Creating non default thunk while initializing");
        AssertMsg(this->DeferredParsingThunk == DefaultDeferredParsingThunk, "Creating non default thunk while initializing");
        AssertMsg(this->DeferredDeserializationThunk == DefaultDeferredDeserializeThunk, "Creating non default thunk while initializing");

#ifdef FIELD_ACCESS_STATS
        this->fieldAccessStatsByFunctionNumber = RecyclerNew(this->recycler, FieldAccessStatsByFunctionNumberMap, recycler);
        BindReference(this->fieldAccessStatsByFunctionNumber);
#endif

        if (!sourceList)
        {
            AutoCriticalSection critSec(threadContext->GetFunctionBodyLock());
            sourceList.Root(RecyclerNew(this->GetRecycler(), SourceList, this->GetRecycler()), this->GetRecycler());
        }

#if DYNAMIC_INTERPRETER_THUNK
        interpreterThunkEmitter = HeapNew(InterpreterThunkEmitter, this, SourceCodeAllocator(), this->GetThreadContext()->GetThunkPageAllocators());
#endif

#ifdef ASMJS_PLAT
        asmJsInterpreterThunkEmitter = HeapNew(InterpreterThunkEmitter, this, SourceCodeAllocator(), this->GetThreadContext()->GetThunkPageAllocators(),
            true);
#endif

        JS_ETW(EtwTrace::LogScriptContextLoadEvent(this));
        JS_ETW_INTERNAL(EventWriteJSCRIPT_HOST_SCRIPT_CONTEXT_START(this));

#ifdef PROFILE_EXEC
        if (profiler != nullptr)
        {
            this->threadContext->GetRecycler()->SetProfiler(profiler->GetProfiler(), profiler->GetBackgroundRecyclerProfiler());
        }
#endif

#if DBG
        this->javascriptLibrary->DumpLibraryByteCode();

        isInitialized = TRUE;
#endif
    }


#ifdef ASMJS_PLAT
    AsmJsCodeGenerator* ScriptContext::InitAsmJsCodeGenerator()
    {
        if( !asmJsCodeGenerator )
        {
            asmJsCodeGenerator = HeapNew( AsmJsCodeGenerator, this );
        }
        return asmJsCodeGenerator;
    }
#endif
    void ScriptContext::MarkForClose()
    {
        if (IsClosed())
        {
            return;
        }

        SaveStartupProfileAndRelease(true);
        SetIsClosed();

#ifdef LEAK_REPORT
        if (this->isRootTrackerScriptContext)
        {
            this->GetThreadContext()->ClearRootTrackerScriptContext(this);
        }
#endif

        if (!threadContext->IsInScript())
        {
            Close(FALSE);
        }
        else
        {
            threadContext->AddToPendingScriptContextCloseList(this);
        }
    }

    void ScriptContext::SetIsClosed()
    {
        if (!this->isClosed)
        {
            this->isClosed = true;

            if (this->javascriptLibrary)
            {
                JS_ETW(EtwTrace::LogSourceUnloadEvents(this));

#if ENABLE_PROFILE_INFO
#if DBG_DUMP
                DynamicProfileInfo::DumpScriptContext(this);
#endif
#ifdef RUNTIME_DATA_COLLECTION
                DynamicProfileInfo::DumpScriptContextToFile(this);
#endif
#endif

#if ENABLE_PROFILE_INFO
#ifdef DYNAMIC_PROFILE_STORAGE
                HRESULT hr = S_OK;
                BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
                {
                    DynamicProfileInfo::Save(this);
                }
                END_TRANSLATE_OOM_TO_HRESULT(hr);

                if (this->Cache()->sourceContextInfoMap)
                {
                    this->Cache()->sourceContextInfoMap->Map([&](DWORD_PTR dwHostSourceContext, SourceContextInfo * sourceContextInfo)
                    {
                        if (sourceContextInfo->sourceDynamicProfileManager)
                        {
                            sourceContextInfo->sourceDynamicProfileManager->ClearSavingData();
                        }
                    });
                }
#endif

#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
                this->ClearDynamicProfileList();
#endif
#endif
            }

#if ENABLE_NATIVE_CODEGEN
            if (m_remoteScriptContextAddr)
            {
                JITManager::GetJITManager()->CloseScriptContext(m_remoteScriptContextAddr);
            }
#endif
            this->PrintStats();
        }
    }

    void ScriptContext::InitializeGlobalObject()
    {
        GlobalObject * localGlobalObject = GlobalObject::New(this);
        GetRecycler()->RootAddRef(localGlobalObject);

        // Assigned the global Object after we have successfully AddRef (in case of OOM)
        globalObject = localGlobalObject;
        globalObject->Initialize(this);

        this->GetThreadContext()->RegisterScriptContext(this);
    }

#ifdef ENABLE_SCRIPT_DEBUGGING
    ArenaAllocator* ScriptContext::AllocatorForDiagnostics()
    {
        if (this->diagnosticArena == nullptr)
        {
            this->diagnosticArena = HeapNew(ArenaAllocator, _u("Diagnostic"), this->GetThreadContext()->GetDebugManager()->GetDiagnosticPageAllocator(), Throw::OutOfMemory);
        }
        Assert(this->diagnosticArena != nullptr);
        return this->diagnosticArena;
    }
#endif

    void ScriptContext::PushObject(Var object)
    {
        operationStack->Push(object);
    }

    Var ScriptContext::PopObject()
    {
        return operationStack->Pop();
    }

    BOOL ScriptContext::CheckObject(Var object)
    {
        return operationStack->Contains(object);
    }

    void ScriptContext::SetHostScriptContext(HostScriptContext *  hostScriptContext)
    {
        Assert(this->hostScriptContext == nullptr);
        this->hostScriptContext = hostScriptContext;
#ifdef PROFILE_EXEC
        this->ensureParentInfo = true;
#endif
    }

    //
    // Enables chakradiag to get the HaltCallBack pointer that is implemented by
    // the ScriptEngine.
    //
    void ScriptContext::SetScriptEngineHaltCallback(HaltCallback* scriptEngine)
    {
        Assert(this->scriptEngineHaltCallback == NULL);
        Assert(scriptEngine != NULL);
        this->scriptEngineHaltCallback = scriptEngine;
    }

    void ScriptContext::ClearHostScriptContext()
    {
        if (this->hostScriptContext != nullptr)
        {
            this->hostScriptContext->Delete();
#ifdef PROFILE_EXEC
            this->ensureParentInfo = false;
#endif
        }
    }

    IActiveScriptProfilerHeapEnum* ScriptContext::GetHeapEnum()
    {
        Assert(this->GetThreadContext());
        return this->GetThreadContext()->GetHeapEnum();
    }

    void ScriptContext::SetHeapEnum(IActiveScriptProfilerHeapEnum* newHeapEnum)
    {
        Assert(this->GetThreadContext());
        this->GetThreadContext()->SetHeapEnum(newHeapEnum);
    }

    void ScriptContext::ClearHeapEnum()
    {
        Assert(this->GetThreadContext());
        this->GetThreadContext()->ClearHeapEnum();
    }

    BOOL ScriptContext::VerifyAlive(BOOL isJSFunction, ScriptContext* requestScriptContext)
    {
        if (isClosed)
        {
            if (!requestScriptContext)
            {
                requestScriptContext = this;
            }

#if ENABLE_PROFILE_INFO
            if (!GetThreadContext()->RecordImplicitException())
            {
                return FALSE;
            }
#endif
            if (isJSFunction)
            {
                Js::JavascriptError::MapAndThrowError(requestScriptContext, JSERR_CantExecute);
            }
            else
            {
                Js::JavascriptError::MapAndThrowError(requestScriptContext, E_ACCESSDENIED);
            }
        }
        return TRUE;
    }

    void ScriptContext::VerifyAliveWithHostContext(BOOL isJSFunction, HostScriptContext* requestHostScriptContext)
    {
        if (requestHostScriptContext)
        {
            VerifyAlive(isJSFunction, requestHostScriptContext->GetScriptContext());
        }
        else
        {
            Assert(GetThreadContext()->IsJSRT() || !GetHostScriptContext()->HasCaller());
            VerifyAlive(isJSFunction, NULL);
        }
    }


    PropertyRecord const * ScriptContext::GetPropertyName(PropertyId propertyId)
    {
        return threadContext->GetPropertyName(propertyId);
    }

    PropertyRecord const * ScriptContext::GetPropertyNameLocked(PropertyId propertyId)
    {
        return threadContext->GetPropertyNameLocked(propertyId);
    }

    void ScriptContext::InitPropertyStringMap(int i)
    {
        this->Cache()->propertyStrings[i] = RecyclerNewStruct(GetRecycler(), PropertyStringMap);
        ClearArray(this->Cache()->propertyStrings[i]->strLen2, 80);
    }

    void ScriptContext::TrackPid(const PropertyRecord* propertyRecord)
    {
        if (IsBuiltInPropertyId(propertyRecord->GetPropertyId()) || propertyRecord->IsBound())
        {
            return;
        }

        if (-1 != this->GetLibrary()->EnsureReferencedPropertyRecordList()->AddNew(propertyRecord))
        {
            RECYCLER_PERF_COUNTER_INC(PropertyRecordBindReference);
        }
    }
    void ScriptContext::TrackPid(PropertyId propertyId)
    {
        if (IsBuiltInPropertyId(propertyId))
        {
            return;
        }
        const PropertyRecord* propertyRecord = this->GetPropertyName(propertyId);
        Assert(propertyRecord != nullptr);
        this->TrackPid(propertyRecord);
    }

    bool ScriptContext::IsTrackedPropertyId(Js::PropertyId propertyId)
    {
        if (IsBuiltInPropertyId(propertyId))
        {
            return true;
        }
        const PropertyRecord* propertyRecord = this->GetPropertyName(propertyId);
        Assert(propertyRecord != nullptr);
        if (propertyRecord->IsBound())
        {
            return true;
        }
        JavascriptLibrary::ReferencedPropertyRecordHashSet * referencedPropertyRecords
            = this->GetLibrary()->GetReferencedPropertyRecordList();
        return referencedPropertyRecords && referencedPropertyRecords->Contains(propertyRecord);
    }
    PropertyString* ScriptContext::AddPropertyString2(const Js::PropertyRecord* propString)
    {
        const char16* buf = propString->GetBuffer();
        const uint i = PropertyStringMap::PStrMapIndex(buf[0]);
        if (this->Cache()->propertyStrings[i] == NULL)
        {
            InitPropertyStringMap(i);
        }
        const uint j = PropertyStringMap::PStrMapIndex(buf[1]);
        if (this->Cache()->propertyStrings[i]->strLen2[j] == NULL && !isClosed)
        {
            this->Cache()->propertyStrings[i]->strLen2[j] = GetLibrary()->CreatePropertyString(propString);
            this->TrackPid(propString);
        }
        return this->Cache()->propertyStrings[i]->strLen2[j];
    }

    PropertyString* ScriptContext::CachePropertyString2(const PropertyRecord* propString)
    {
        Assert(propString->GetLength() == 2);
        const char16* propertyName = propString->GetBuffer();
        if ((propertyName[0] <= 'z') && (propertyName[1] <= 'z') && (propertyName[0] >= '0') && (propertyName[1] >= '0') && ((propertyName[0] > '9') || (propertyName[1] > '9')))
        {
            return AddPropertyString2(propString);
        }
        return NULL;
    }

    PropertyString* ScriptContext::TryGetPropertyString(PropertyId propertyId)
    {
        PropertyStringCacheMap* propertyStringMap = this->GetLibrary()->EnsurePropertyStringMap();

        RecyclerWeakReference<PropertyString>* stringReference = nullptr;
        if (propertyStringMap->TryGetValue(propertyId, &stringReference))
        {
            PropertyString *string = stringReference->Get();
            if (string != nullptr)
            {
                return string;
            }
        }

        return nullptr;
    }

    PropertyString* ScriptContext::GetPropertyString(PropertyId propertyId)
    {
        PropertyString *string = TryGetPropertyString(propertyId);
        if (string != nullptr)
        {
            return string;
        }

        PropertyStringCacheMap* propertyStringMap = this->GetLibrary()->EnsurePropertyStringMap();

        const Js::PropertyRecord* propertyName = this->GetPropertyName(propertyId);
        string = this->GetLibrary()->CreatePropertyString(propertyName);
        propertyStringMap->Item(propertyId, recycler->CreateWeakReferenceHandle(string));

        return string;
    }

    void ScriptContext::InvalidatePropertyStringCache(PropertyId propertyId, Type* type)
    {
        Assert(!isFinalized);
        PropertyStringCacheMap* propertyStringMap = this->javascriptLibrary->GetPropertyStringMap();
        if (propertyStringMap != nullptr)
        {
            PropertyString *string = nullptr;
            RecyclerWeakReference<PropertyString>* stringReference = nullptr;
            if (propertyStringMap->TryGetValue(propertyId, &stringReference))
            {
                string = stringReference->Get();
            }
            if (string)
            {
                PolymorphicInlineCache * cache = string->GetLdElemInlineCache();
                PropertyCacheOperationInfo info;
                if (cache->PretendTryGetProperty(type, &info))
                {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
                    if (PHASE_TRACE1(PropertyStringCachePhase))
                    {
                        Output::Print(_u("PropertyString '%s' : Invalidating LdElem cache for type %p\n"), string->GetString(), type);
                    }
#endif
                    cache->GetInlineCaches()[cache->GetInlineCacheIndexForType(type)].RemoveFromInvalidationListAndClear(this->GetThreadContext());
                }
                cache = string->GetStElemInlineCache();
                if (cache->PretendTrySetProperty(type, type, &info))
                {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
                    if (PHASE_TRACE1(PropertyStringCachePhase))
                    {
                        Output::Print(_u("PropertyString '%s' : Invalidating StElem cache for type %p\n"), string->GetString(), type);
                    }
#endif
                    cache->GetInlineCaches()[cache->GetInlineCacheIndexForType(type)].RemoveFromInvalidationListAndClear(this->GetThreadContext());
                }
            }
        }
    }

    void ScriptContext::CleanupWeakReferenceDictionaries()
    {
        if (!isWeakReferenceDictionaryListCleared)
        {
            SListBase<JsUtil::IWeakReferenceDictionary*>::Iterator iter(&this->weakReferenceDictionaryList);

            while (iter.Next())
            {
                JsUtil::IWeakReferenceDictionary* weakReferenceDictionary = iter.Data();

                weakReferenceDictionary->Cleanup();
            }
        }
    }

    JavascriptString* ScriptContext::GetIntegerString(Var aValue)
    {
        return this->GetIntegerString(TaggedInt::ToInt32(aValue));
    }

    JavascriptString* ScriptContext::GetIntegerString(uint value)
    {
        if (value <= INT_MAX)
        {
            return this->GetIntegerString((int)value);
        }
        return TaggedInt::ToString(value, this);
    }

    JavascriptString* ScriptContext::GetIntegerString(int value)
    {
        // Optimize for 0-9
        if (0 <= value && value <= 9)
        {
            return GetLibrary()->GetCharStringCache().GetStringForCharA('0' + static_cast<char>(value));
        }

        JavascriptString *string;

// TODO: (obastemur) Could this be dynamic instead of compile time?
#ifndef CC_LOW_MEMORY_TARGET // we don't need this on a target with low memory
        if (!this->integerStringMap.TryGetValue(value, &string))
        {
            // Add the string to hash table cache
            // Don't add if table is getting too full.  We'll be holding on to
            // too many strings, and table lookup will become too slow.
            // TODO: Long term running app, this cache doesn't provide much value?
            //       i.e. what is the importance of first 512 number to string calls?
            //       a solution; count the number of times we couldn't use cache
            //       after cache is full. If it's bigger than X ?? the discard the
            //       previous cache?
            if (this->integerStringMap.Count() > 512)
            {
#endif
                // Use recycler memory
                string = TaggedInt::ToString(value, this);

#ifndef CC_LOW_MEMORY_TARGET
            }
            else
            {
                char16 stringBuffer[22];

                int pos = TaggedInt::ToBuffer(value, stringBuffer, _countof(stringBuffer));
                string = JavascriptString::NewCopySzFromArena(stringBuffer + pos,
                    this, this->GeneralAllocator(), (_countof(stringBuffer) - 1) - pos);
                this->integerStringMap.AddNew(value, string);
            }
        }
#endif

        return string;
    }

    void ScriptContext::CheckEvalRestriction()
    {
        HRESULT hr = S_OK;
        Var domError = nullptr;
        HostScriptContext* hostScriptContext = this->GetHostScriptContext();

        BEGIN_LEAVE_SCRIPT(this)
        {
            if (!FAILED(hr = hostScriptContext->CheckEvalRestriction()))
            {
                return;
            }

            hr = hostScriptContext->HostExceptionFromHRESULT(hr, &domError);
        }
        END_LEAVE_SCRIPT(this);

        if (FAILED(hr))
        {
            Js::JavascriptError::MapAndThrowError(this, hr);
        }

        if (domError != nullptr)
        {
            JavascriptExceptionOperators::Throw(domError, this);
        }

        AssertMsg(false, "We should have thrown by now.");
        Js::JavascriptError::MapAndThrowError(this, E_FAIL);
    }

    ParseNode* ScriptContext::ParseScript(Parser* parser,
        const byte* script,
        size_t cb,
        SRCINFO const * pSrcInfo,
        CompileScriptException * pse,
        Utf8SourceInfo** ppSourceInfo,
        const char16 *rootDisplayName,
        LoadScriptFlag loadScriptFlag,
        uint* sourceIndex,
        Js::Var scriptSource)
    {
        if (pSrcInfo == nullptr)
        {
            pSrcInfo = this->Cache()->noContextGlobalSourceInfo;
        }

        LPUTF8 utf8Script = nullptr;
        size_t length = cb;
        size_t cbNeeded = 0;

        bool isLibraryCode = ((loadScriptFlag & LoadScriptFlag_LibraryCode) == LoadScriptFlag_LibraryCode);

        if ((loadScriptFlag & LoadScriptFlag_Utf8Source) != LoadScriptFlag_Utf8Source)
        {
            // Convert to UTF8 and then load that
            length = cb / sizeof(char16);
            if (!IsValidCharCount(length))
            {
                Js::Throw::OutOfMemory();
            }
            Assert(length < MAXLONG);

            // Allocate memory for the UTF8 output buffer.
            // We need at most 3 bytes for each Unicode code point.
            // The + 1 is to include the terminating NUL.
            // Nit:  Technically, we know that the NUL only needs 1 byte instead of
            // 3, but that's difficult to express in a SAL annotation for "EncodeInto".
            size_t cbUtf8Buffer = AllocSizeMath::Mul(AllocSizeMath::Add(length, 1), 3);

            utf8Script = RecyclerNewArrayLeafTrace(this->GetRecycler(), utf8char_t, cbUtf8Buffer);

            cbNeeded = utf8::EncodeIntoAndNullTerminate(utf8Script, (const char16*)script, static_cast<charcount_t>(length));

#if DBG_DUMP && defined(PROFILE_MEM)
            if(Js::Configuration::Global.flags.TraceMemory.IsEnabled(Js::ParsePhase) && Configuration::Global.flags.Verbose)
            {
                Output::Print(_u("Loading script.\n")
                    _u("  Unicode (in bytes)    %u\n")
                    _u("  UTF-8 size (in bytes) %u\n")
                    _u("  Expected savings      %d\n"), length * sizeof(char16), cbNeeded, length * sizeof(char16) - cbNeeded);
            }
#endif

            // Free unused bytes
            Assert(cbNeeded + 1 <= cbUtf8Buffer);
            *ppSourceInfo = Utf8SourceInfo::New(this, utf8Script, (int)length,
                cbNeeded, pSrcInfo, isLibraryCode);
        }
        else
        {
            // We do not own the memory passed into DefaultLoadScriptUtf8. We need to save it so we copy the memory.
            if(*ppSourceInfo == nullptr)
            {
#ifndef NTBUILD
                if (loadScriptFlag & LoadScriptFlag_ExternalArrayBuffer)
                {
                    *ppSourceInfo = Utf8SourceInfo::NewWithNoCopy(this,
                        script, (int)length, cb, pSrcInfo, isLibraryCode,
                        scriptSource);
                }
                else
#endif
                {
                    // the 'length' here is not correct - we will get the length from the parser - however parser hasn't done yet.
                    // Once the parser is done we will update the utf8sourceinfo's lenght correctly with parser's
                    *ppSourceInfo = Utf8SourceInfo::New(this, script,
                        (int)length, cb, pSrcInfo, isLibraryCode);
                }
            }
        }
        //
        // Parse and the JavaScript code
        //
        HRESULT hr;

        SourceContextInfo * sourceContextInfo = pSrcInfo->sourceContextInfo;

        // Invoke the parser, passing in the global function name, which we will then run to execute
        // the script.
        // TODO: yongqu handle non-global code.
        ULONG grfscr = fscrGlobalCode | ((loadScriptFlag & LoadScriptFlag_Expression) == LoadScriptFlag_Expression ? fscrReturnExpression : 0);
        if(((loadScriptFlag & LoadScriptFlag_disableDeferredParse) != LoadScriptFlag_disableDeferredParse) &&
            (length > Parser::GetDeferralThreshold(sourceContextInfo->IsSourceProfileLoaded())))
        {
            grfscr |= fscrDeferFncParse;
        }

        if((loadScriptFlag & LoadScriptFlag_disableAsmJs) == LoadScriptFlag_disableAsmJs)
        {
            grfscr |= fscrNoAsmJs;
        }

        if(PHASE_FORCE1(Js::EvalCompilePhase))
        {
            // pretend it is eval
            grfscr |= (fscrEval | fscrEvalCode);
        }

        if((loadScriptFlag & LoadScriptFlag_isByteCodeBufferForLibrary) == LoadScriptFlag_isByteCodeBufferForLibrary)
        {
            grfscr |= fscrNoPreJit;
        }

        if(((loadScriptFlag & LoadScriptFlag_Module) == LoadScriptFlag_Module) &&
            GetConfig()->IsES6ModuleEnabled())
        {
            grfscr |= fscrIsModuleCode;
        }

        if (isLibraryCode)
        {
            grfscr |= fscrIsLibraryCode;
        }

        ParseNodePtr parseTree;
        if((loadScriptFlag & LoadScriptFlag_Utf8Source) == LoadScriptFlag_Utf8Source)
        {
            hr = parser->ParseUtf8Source(&parseTree, script, cb, grfscr, pse,
                &sourceContextInfo->nextLocalFunctionId, sourceContextInfo);
        }
        else
        {
            hr = parser->ParseCesu8Source(&parseTree, utf8Script, cbNeeded, grfscr,
                pse, &sourceContextInfo->nextLocalFunctionId, sourceContextInfo);
        }

        if(FAILED(hr) || parseTree == nullptr)
        {
            return nullptr;
        }

        (*ppSourceInfo)->SetParseFlags(grfscr);

        //Make sure we have the body and text information available
        if ((loadScriptFlag & LoadScriptFlag_Utf8Source) != LoadScriptFlag_Utf8Source)
        {
            *sourceIndex = this->SaveSourceNoCopy(*ppSourceInfo, static_cast<charcount_t>((*ppSourceInfo)->GetCchLength()), /*isCesu8*/ true);
        }
        else
        {
            // Update the length.
            (*ppSourceInfo)->SetCchLength(parser->GetSourceIchLim());
            *sourceIndex = this->SaveSourceNoCopy(*ppSourceInfo, parser->GetSourceIchLim(), /* isCesu8*/ false);
        }

        return parseTree;
    }

    JavascriptFunction* ScriptContext::LoadScript(const byte* script, size_t cb,
        SRCINFO const * pSrcInfo, CompileScriptException * pse, Utf8SourceInfo** ppSourceInfo,
        const char16 *rootDisplayName, LoadScriptFlag loadScriptFlag, Js::Var scriptSource)
    {
        Assert(!this->threadContext->IsScriptActive());
        Assert(pse != nullptr);
        HRESULT hr = NOERROR;
        try
        {
            AUTO_NESTED_HANDLED_EXCEPTION_TYPE((ExceptionType)(ExceptionType_OutOfMemory | ExceptionType_StackOverflow));
            Js::AutoDynamicCodeReference dynamicFunctionReference(this);
            Parser parser(this);
            uint sourceIndex;
            JavascriptFunction * pFunction = nullptr;

            ParseNodePtr parseTree = ParseScript(&parser, script, cb, pSrcInfo,
                pse, ppSourceInfo, rootDisplayName, loadScriptFlag,
                &sourceIndex, scriptSource);

            if (parseTree != nullptr)
            {
                pFunction = GenerateRootFunction(parseTree, sourceIndex, &parser, (*ppSourceInfo)->GetParseFlags(), pse, rootDisplayName);
            }

            if (pse->ei.scode == JSERR_AsmJsCompileError)
            {
                Assert((loadScriptFlag & LoadScriptFlag_disableAsmJs) != LoadScriptFlag_disableAsmJs);

                pse->Free();

                loadScriptFlag = (LoadScriptFlag)(loadScriptFlag | LoadScriptFlag_disableAsmJs);
                return LoadScript(script, cb, pSrcInfo, pse, ppSourceInfo,
                    rootDisplayName, loadScriptFlag, scriptSource);
            }

#ifdef ENABLE_SCRIPT_PROFILING
            if (pFunction != nullptr && this->IsProfiling())
            {
                RegisterScript(pFunction->GetFunctionProxy());
            }
#else
            Assert(!this->IsProfiling());
#endif
            return pFunction;
        }
        catch (Js::OutOfMemoryException)
        {
            hr = E_OUTOFMEMORY;
        }
        catch (Js::StackOverflowException)
        {
            hr = VBSERR_OutOfStack;
        }
        pse->ProcessError(nullptr, hr, nullptr);
        return nullptr;
    }

    JavascriptFunction* ScriptContext::GenerateRootFunction(ParseNodePtr parseTree, uint sourceIndex, Parser* parser, uint32 grfscr, CompileScriptException * pse, const char16 *rootDisplayName)
    {
        HRESULT hr;

        // Get the source code to keep it alive during the bytecode generation process
        LPCUTF8 source = this->GetSource(sourceIndex)->GetSource(_u("ScriptContext::GenerateRootFunction"));
        Assert(source != nullptr); // Source should not have been reclaimed by now

        // Generate bytecode and native code
        ParseableFunctionInfo* body = NULL;
        hr = GenerateByteCode(parseTree, grfscr, this, &body, sourceIndex, false, parser, pse);

        this->GetSource(sourceIndex)->SetByteCodeGenerationFlags(grfscr);
        if(FAILED(hr))
        {
            return nullptr;
        }

        body->SetDisplayName(rootDisplayName);
        body->SetIsTopLevel(true);

        JavascriptFunction* rootFunction = javascriptLibrary->CreateScriptFunction(body);
        return rootFunction;
    }

    BOOL ScriptContext::ReserveStaticTypeIds(__in int first, __in int last)
    {
        return threadContext->ReserveStaticTypeIds(first, last);
    }

    TypeId ScriptContext::ReserveTypeIds(int count)
    {
        return threadContext->ReserveTypeIds(count);
    }

    TypeId ScriptContext::CreateTypeId()
    {
        return threadContext->CreateTypeId();
    }

    void ScriptContext::OnScriptStart(bool isRoot, bool isScript)
    {
        const bool isForcedEnter = 
#ifdef ENABLE_SCRIPT_DEBUGGING
            this->GetDebugContext() != nullptr ? this->GetDebugContext()->GetProbeContainer()->isForcedToEnterScriptStart : 
#endif
            false;
        if (this->scriptStartEventHandler != nullptr && ((isRoot && threadContext->GetCallRootLevel() == 1) || isForcedEnter))
        {
#ifdef ENABLE_SCRIPT_DEBUGGING
            if (this->GetDebugContext() != nullptr)
            {
                this->GetDebugContext()->GetProbeContainer()->isForcedToEnterScriptStart = false;
            }
#endif
            this->scriptStartEventHandler(this);
        }

#if ENABLE_NATIVE_CODEGEN
        //Blue 5491: Only start codegen if isScript. Avoid it if we are not really starting script and called from risky region such as catch handler.
        if (isScript)
        {
            NativeCodeGenEnterScriptStart(this->GetNativeCodeGenerator());
        }
#endif
    }

    void ScriptContext::OnScriptEnd(bool isRoot, bool isForcedEnd)
    {
        if ((isRoot && threadContext->GetCallRootLevel() == 1) || isForcedEnd)
        {
            if (this->scriptEndEventHandler != nullptr)
            {
                this->scriptEndEventHandler(this);
            }
        }
    }

#ifdef FAULT_INJECTION
    void ScriptContext::DisposeScriptContextByFaultInjection() {
        if (this->disposeScriptByFaultInjectionEventHandler != nullptr)
        {
            this->disposeScriptByFaultInjectionEventHandler(this);
        }
    }
#endif

    template <bool stackProbe, bool leaveForHost>
    bool ScriptContext::LeaveScriptStart(void * frameAddress)
    {
        ThreadContext * threadContext = this->threadContext;
        if (!threadContext->IsScriptActive())
        {
            // we should have enter always.
            AssertMsg(FALSE, "Leaving ScriptStart while script is not active.");
            return false;
        }

        // Make sure the host function will have at least 32k of stack available.
        if (stackProbe)
        {
            threadContext->ProbeStack(Js::Constants::MinStackCallout, this);
        }
        else
        {
            AssertMsg(ExceptionCheck::HasStackProbe(), "missing stack probe");
        }

        threadContext->LeaveScriptStart<leaveForHost>(frameAddress);
        return true;
    }

    template <bool leaveForHost>
    void ScriptContext::LeaveScriptEnd(void * frameAddress)
    {
        this->threadContext->LeaveScriptEnd<leaveForHost>(frameAddress);
    }

    // explicit instantiations
    template bool ScriptContext::LeaveScriptStart<true, true>(void * frameAddress);
    template bool ScriptContext::LeaveScriptStart<true, false>(void * frameAddress);
    template bool ScriptContext::LeaveScriptStart<false, true>(void * frameAddress);
    template void ScriptContext::LeaveScriptEnd<true>(void * frameAddress);
    template void ScriptContext::LeaveScriptEnd<false>(void * frameAddress);

    bool ScriptContext::EnsureInterpreterArena(ArenaAllocator **ppAlloc)
    {
        bool fNew = false;
        if (this->interpreterArena == nullptr)
        {
            this->interpreterArena = this->GetRecycler()->CreateGuestArena(_u("Interpreter"), Throw::OutOfMemory);
            fNew = true;
        }
        *ppAlloc = this->interpreterArena;
        return fNew;
    }

    void ScriptContext::ReleaseInterpreterArena()
    {
        AssertMsg(this->interpreterArena, "No interpreter arena to release");
        if (this->interpreterArena)
        {
            this->GetRecycler()->DeleteGuestArena(this->interpreterArena);
            this->interpreterArena = nullptr;
        }
    }


    void ScriptContext::ReleaseGuestArena()
    {
        AssertMsg(this->guestArena, "No guest arena to release");
        if (this->guestArena)
        {
            this->GetRecycler()->DeleteGuestArena(this->guestArena);
            this->guestArena = nullptr;
        }
    }

    void ScriptContext::SetScriptStartEventHandler(ScriptContext::EventHandler eventHandler)
    {
        AssertMsg(this->scriptStartEventHandler == nullptr, "Do not support multi-cast yet");
        this->scriptStartEventHandler = eventHandler;
    }
    void ScriptContext::SetScriptEndEventHandler(ScriptContext::EventHandler eventHandler)
    {
        AssertMsg(this->scriptEndEventHandler == nullptr, "Do not support multi-cast yet");
        this->scriptEndEventHandler = eventHandler;
    }

#ifdef FAULT_INJECTION
    void ScriptContext::SetDisposeDisposeByFaultInjectionEventHandler(ScriptContext::EventHandler eventHandler)
    {
        AssertMsg(this->disposeScriptByFaultInjectionEventHandler == nullptr, "Do not support multi-cast yet");
        this->disposeScriptByFaultInjectionEventHandler = eventHandler;
    }
#endif

    bool ScriptContext::SaveSourceCopy(Utf8SourceInfo* const sourceInfo, int cchLength, bool isCesu8, uint * index)
    {
        HRESULT hr = S_OK;
        BEGIN_TRANSLATE_OOM_TO_HRESULT
        {
            *index = this->SaveSourceCopy(sourceInfo, cchLength, isCesu8);
        }
        END_TRANSLATE_OOM_TO_HRESULT(hr);
        return hr == S_OK;
    }

    uint ScriptContext::SaveSourceCopy(Utf8SourceInfo* sourceInfo, int cchLength, bool isCesu8)
    {
        Utf8SourceInfo* newSource = Utf8SourceInfo::Clone(this, sourceInfo);

        return SaveSourceNoCopy(newSource, cchLength, isCesu8);
    }


    uint ScriptContext::SaveSourceNoCopy(Utf8SourceInfo* sourceInfo, int cchLength, bool isCesu8)
    {
        Assert(sourceInfo->GetScriptContext() == this);

#ifdef ENABLE_SCRIPT_DEBUGGING
        if (this->IsScriptContextInDebugMode() && !sourceInfo->GetIsLibraryCode() && !sourceInfo->IsInDebugMode())
        {
            sourceInfo->SetInDebugMode(true);
        }
#endif

        RecyclerWeakReference<Utf8SourceInfo>* sourceWeakRef = this->GetRecycler()->CreateWeakReferenceHandle<Utf8SourceInfo>(sourceInfo);
        sourceInfo->SetIsCesu8(isCesu8);
        {
            // We can be compiling new source code while rundown thread is reading from the list, causing AV on the reader thread
            // lock the list during write as well.
            AutoCriticalSection autocs(GetThreadContext()->GetFunctionBodyLock());
            return sourceList->SetAtFirstFreeSpot(sourceWeakRef);
        }
    }

    void ScriptContext::CloneSources(ScriptContext* sourceContext)
    {
        sourceContext->sourceList->Map([=](int index, RecyclerWeakReference<Utf8SourceInfo>* sourceInfo)
        {
            Utf8SourceInfo* info = sourceInfo->Get();
            if (info)
            {
                CloneSource(info);
            }
        });
    }

    uint ScriptContext::CloneSource(Utf8SourceInfo* info)
    {
        return this->SaveSourceCopy(info, info->GetCchLength(), info->GetIsCesu8());
    }

    Utf8SourceInfo* ScriptContext::GetSource(uint index)
    {
        Assert(this->sourceList->IsItemValid(index)); // This assert should be a subset of info != null- if info was null, in the last collect, we'd have invalidated the item
        Utf8SourceInfo* info = this->sourceList->Item(index)->Get();
        Assert(info != nullptr); // Should still be alive if this method is being called
        return info;
    }

    bool ScriptContext::IsItemValidInSourceList(int index)
    {
        return (index < this->sourceList->Count()) && this->sourceList->IsItemValid(index);
    }

    void ScriptContext::RecordException(JavascriptExceptionObject * exceptionObject, bool propagateToDebugger)
    {
        Assert(this->threadContext->GetRecordedException() == nullptr || GetThreadContext()->HasUnhandledException());
        this->threadContext->SetRecordedException(exceptionObject, propagateToDebugger);
#if DBG && ENABLE_DEBUG_STACK_BACK_TRACE
        exceptionObject->FillStackBackTrace();
#endif
    }

    void ScriptContext::RethrowRecordedException(JavascriptExceptionObject::HostWrapperCreateFuncType hostWrapperCreateFunc)
    {
        bool considerPassingToDebugger = false;
        JavascriptExceptionObject * exceptionObject = this->GetAndClearRecordedException(&considerPassingToDebugger);
        if (hostWrapperCreateFunc)
        {
            exceptionObject->SetHostWrapperCreateFunc(exceptionObject->GetScriptContext() != this ? hostWrapperCreateFunc : nullptr);
        }
        JavascriptExceptionOperators::RethrowExceptionObject(exceptionObject, this, considerPassingToDebugger);
    }

    Js::JavascriptExceptionObject * ScriptContext::GetAndClearRecordedException(bool *considerPassingToDebugger)
    {
        JavascriptExceptionObject * exceptionObject = this->threadContext->GetRecordedException();
        Assert(exceptionObject != nullptr);
        if (considerPassingToDebugger)
        {
            *considerPassingToDebugger = this->threadContext->GetPropagateException();
        }
        exceptionObject = exceptionObject->CloneIfStaticExceptionObject(this);
        this->threadContext->SetRecordedException(nullptr);
        return exceptionObject;
    }

    bool ScriptContext::IsInEvalMap(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction **ppFuncScript)
    {
        EvalCacheDictionary *dict = isIndirect ? this->Cache()->indirectEvalCacheDictionary : this->Cache()->evalCacheDictionary;
        if (dict == nullptr)
        {
            return false;
        }
#ifdef PROFILE_EVALMAP
        if (Configuration::Global.flags.ProfileEvalMap)
        {
            charcount_t len = key.str.GetLength();
            if (dict->TryGetValue(key, ppFuncScript))
            {
                Output::Print(_u("EvalMap cache hit:\t source size = %d\n"), len);
            }
            else
            {
                Output::Print(_u("EvalMap cache miss:\t source size = %d\n"), len);
            }
        }
#endif

        // If eval map cleanup is false, to preserve existing behavior, add it to the eval map MRU list
        bool success = dict->TryGetValue(key, ppFuncScript);

        if (success)
        {
            dict->NotifyAdd(key);
#ifdef VERBOSE_EVAL_MAP
#if DBG
            dict->DumpKeepAlives();
#endif
#endif
        }

        return success;
    }

    void ScriptContext::AddToEvalMap(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction *pfuncScript)
    {
        Assert(!pfuncScript->GetFunctionInfo()->IsGenerator());

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        Js::Utf8SourceInfo* utf8SourceInfo = pfuncScript->GetFunctionBody()->GetUtf8SourceInfo();
        if (this->IsScriptContextInDebugMode() && !utf8SourceInfo->GetIsLibraryCode() && !utf8SourceInfo->IsInDebugMode())
        {
            // Identifying if any non library function escaped for not being in debug mode.
            Throw::FatalInternalError();
        }
#endif

#if ENABLE_TTD
        if(!this->IsTTDRecordOrReplayModeEnabled())
        {
            this->AddToEvalMapHelper(key, isIndirect, pfuncScript);
        }
#else
        this->AddToEvalMapHelper(key, isIndirect, pfuncScript);
#endif
    }

    void ScriptContext::AddToEvalMapHelper(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction *pFuncScript)
    {
        EvalCacheDictionary *dict = isIndirect ? this->Cache()->indirectEvalCacheDictionary : this->Cache()->evalCacheDictionary;
        if (dict == nullptr)
        {
            EvalCacheTopLevelDictionary* evalTopDictionary = RecyclerNew(this->recycler, EvalCacheTopLevelDictionary, this->recycler);
            dict = RecyclerNew(this->recycler, EvalCacheDictionary, evalTopDictionary, recycler);
            if (isIndirect)
            {
                this->Cache()->indirectEvalCacheDictionary = dict;
            }
            else
            {
                this->Cache()->evalCacheDictionary = dict;
            }
        }

        dict->Add(key, pFuncScript);
    }

    bool ScriptContext::IsInNewFunctionMap(EvalMapString const& key, FunctionInfo **ppFuncInfo)
    {
        if (this->Cache()->newFunctionCache == nullptr)
        {
            return false;
        }

        // If eval map cleanup is false, to preserve existing behavior, add it to the eval map MRU list
        bool success = this->Cache()->newFunctionCache->TryGetValue(key, ppFuncInfo);
        if (success)
        {
            this->Cache()->newFunctionCache->NotifyAdd(key);
#ifdef VERBOSE_EVAL_MAP
#if DBG
            this->Cache()->newFunctionCache->DumpKeepAlives();
#endif
#endif
        }

        return success;
    }

    void ScriptContext::AddToNewFunctionMap(EvalMapString const& key, FunctionInfo *pFuncInfo)
    {
        if (this->Cache()->newFunctionCache == nullptr)
        {
            this->Cache()->newFunctionCache = RecyclerNew(this->recycler, NewFunctionCache, this->recycler);
        }
        this->Cache()->newFunctionCache->Add(key, pFuncInfo);
    }


    void ScriptContext::EnsureSourceContextInfoMap()
    {
        if (this->Cache()->sourceContextInfoMap == nullptr)
        {
            this->Cache()->sourceContextInfoMap = RecyclerNew(this->GetRecycler(), SourceContextInfoMap, this->GetRecycler());
        }
    }

    void ScriptContext::EnsureDynamicSourceContextInfoMap()
    {
        if (this->Cache()->dynamicSourceContextInfoMap == nullptr)
        {
            this->Cache()->dynamicSourceContextInfoMap = RecyclerNew(this->GetRecycler(), DynamicSourceContextInfoMap, this->GetRecycler());
        }
    }

    SourceContextInfo* ScriptContext::GetSourceContextInfo(uint hash)
    {
        SourceContextInfo * sourceContextInfo = nullptr;
        if (this->Cache()->dynamicSourceContextInfoMap && this->Cache()->dynamicSourceContextInfoMap->TryGetValue(hash, &sourceContextInfo))
        {
            return sourceContextInfo;
        }
        return nullptr;
    }

    SourceContextInfo* ScriptContext::CreateSourceContextInfo(uint hash, DWORD_PTR hostSourceContext)
    {
        EnsureDynamicSourceContextInfoMap();
        if (this->GetSourceContextInfo(hash) != nullptr)
        {
            return this->Cache()->noContextSourceContextInfo;
        }

        if (this->Cache()->dynamicSourceContextInfoMap->Count() > INMEMORY_CACHE_MAX_PROFILE_MANAGER)
        {
            OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Max of dynamic script profile info reached.\n"));
            return this->Cache()->noContextSourceContextInfo;
        }

        // This is capped so we can continue allocating in the arena
        SourceContextInfo * sourceContextInfo = RecyclerNewStructZ(this->GetRecycler(), SourceContextInfo);
        sourceContextInfo->sourceContextId = this->GetNextSourceContextId();
        sourceContextInfo->dwHostSourceContext = hostSourceContext;
        sourceContextInfo->isHostDynamicDocument = true;
        sourceContextInfo->hash = hash;
#if ENABLE_PROFILE_INFO
        sourceContextInfo->sourceDynamicProfileManager = this->threadContext->GetSourceDynamicProfileManager(this->GetUrl(), hash, &referencesSharedDynamicSourceContextInfo);
#endif

        // For the host provided dynamic code (if hostSourceContext is not NoHostSourceContext), do not add to dynamicSourceContextInfoMap
        if (hostSourceContext == Js::Constants::NoHostSourceContext)
        {
            this->Cache()->dynamicSourceContextInfoMap->Add(hash, sourceContextInfo);
        }
        return sourceContextInfo;
    }

    //
    // Makes a copy of the URL to be stored in the map.
    //
    SourceContextInfo * ScriptContext::CreateSourceContextInfo(DWORD_PTR sourceContext, char16 const * url, size_t len,
        IActiveScriptDataCache* profileDataCache, char16 const * sourceMapUrl /*= NULL*/, size_t sourceMapUrlLen /*= 0*/)
    {
        // Take etw rundown lock on this thread context. We are going to init/add to sourceContextInfoMap.
        AutoCriticalSection autocs(GetThreadContext()->GetFunctionBodyLock());

        EnsureSourceContextInfoMap();
        Assert(this->GetSourceContextInfo(sourceContext, profileDataCache) == nullptr);
        SourceContextInfo * sourceContextInfo = RecyclerNewStructZ(this->GetRecycler(), SourceContextInfo);
        sourceContextInfo->sourceContextId = this->GetNextSourceContextId();
        sourceContextInfo->dwHostSourceContext = sourceContext;
        sourceContextInfo->isHostDynamicDocument = false;
#if ENABLE_PROFILE_INFO
        sourceContextInfo->sourceDynamicProfileManager = nullptr;
#endif

        if (url != nullptr)
        {
            sourceContextInfo->url = CopyString(url, len, this->SourceCodeAllocator());
            JS_ETW(EtwTrace::LogSourceModuleLoadEvent(this, sourceContext, url));
        }
        if (sourceMapUrl != nullptr && sourceMapUrlLen != 0)
        {
            sourceContextInfo->sourceMapUrl = CopyString(sourceMapUrl, sourceMapUrlLen, this->SourceCodeAllocator());
        }

#if ENABLE_PROFILE_INFO
        if (!this->startupComplete)
        {
            sourceContextInfo->sourceDynamicProfileManager = SourceDynamicProfileManager::LoadFromDynamicProfileStorage(sourceContextInfo, this, profileDataCache);
            Assert(sourceContextInfo->sourceDynamicProfileManager != NULL);
        }

        this->Cache()->sourceContextInfoMap->Add(sourceContext, sourceContextInfo);
#endif
        return sourceContextInfo;
    }

    // static
    const char16* ScriptContext::CopyString(const char16* str, size_t charCount, ArenaAllocator* alloc)
    {
        size_t length = charCount + 1; // Add 1 for the NULL.
        char16* copy = AnewArray(alloc, char16, length);
        js_wmemcpy_s(copy, length, str, charCount);
        copy[length - 1] = _u('\0');
        return copy;
    }

    SourceContextInfo *  ScriptContext::GetSourceContextInfo(DWORD_PTR sourceContext, IActiveScriptDataCache* profileDataCache)
    {
        if (sourceContext == Js::Constants::NoHostSourceContext)
        {
            return this->Cache()->noContextSourceContextInfo;
        }

        // We only init sourceContextInfoMap, don't need to lock.
        EnsureSourceContextInfoMap();
        SourceContextInfo * sourceContextInfo = nullptr;
        if (this->Cache()->sourceContextInfoMap->TryGetValue(sourceContext, &sourceContextInfo))
        {
#if ENABLE_PROFILE_INFO
            if (profileDataCache &&
                sourceContextInfo->sourceDynamicProfileManager != nullptr &&
                !sourceContextInfo->sourceDynamicProfileManager->IsProfileLoadedFromWinInet() &&
                !this->startupComplete)
            {
                bool profileLoaded = sourceContextInfo->sourceDynamicProfileManager->LoadFromProfileCache(profileDataCache, sourceContextInfo->url);
                if (profileLoaded)
                {
                    JS_ETW(EventWriteJSCRIPT_PROFILE_LOAD(sourceContextInfo->dwHostSourceContext, this));
                }
            }
#endif
            return sourceContextInfo;
        }
        return nullptr;
    }

    SRCINFO const *
        ScriptContext::GetModuleSrcInfo(Js::ModuleID moduleID)
    {
            if (moduleSrcInfoCount <= moduleID)
            {
                uint newCount = moduleID + 4;  // Preallocate 4 more slots, moduleID don't usually grow much

                Field(SRCINFO const *)* newModuleSrcInfo = RecyclerNewArrayZ(this->GetRecycler(), Field(SRCINFO const*), newCount);
                CopyArray(newModuleSrcInfo, newCount, Cache()->moduleSrcInfo, moduleSrcInfoCount);
                Cache()->moduleSrcInfo = newModuleSrcInfo;
                moduleSrcInfoCount = newCount;
                Cache()->moduleSrcInfo[0] = this->Cache()->noContextGlobalSourceInfo;
            }

            SRCINFO const * si = Cache()->moduleSrcInfo[moduleID];
            if (si == nullptr)
            {
                SRCINFO * newSrcInfo = RecyclerNewStructZ(this->GetRecycler(), SRCINFO);
                newSrcInfo->sourceContextInfo = this->Cache()->noContextSourceContextInfo;
                newSrcInfo->moduleID = moduleID;
                Cache()->moduleSrcInfo[moduleID] = newSrcInfo;
                si = newSrcInfo;
            }
            return si;
    }

#if ENABLE_TTD
    void ScriptContext::InitializeCoreImage_TTD()
    {
        TTDAssert(this->TTDWellKnownInfo == nullptr, "This should only happen once!!!");

        this->TTDContextInfo = TT_HEAP_NEW(TTD::ScriptContextTTD, this);
        this->TTDWellKnownInfo = TT_HEAP_NEW(TTD::RuntimeContextInfo);

        BEGIN_ENTER_SCRIPT(this, true, true, true)
        {
            this->TTDWellKnownInfo->GatherKnownObjectToPathMap(this);
        }
        END_ENTER_SCRIPT
    }
#endif

#ifdef PROFILE_EXEC
    void
        ScriptContext::DisableProfiler()
    {
            disableProfiler = true;
    }

    Profiler *
        ScriptContext::CreateProfiler()
    {
            Assert(profiler == nullptr);
            if (Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag))
            {
                this->profiler = NoCheckHeapNew(ScriptContextProfiler);
                this->profiler->Initialize(GetThreadContext()->GetPageAllocator(), threadContext->GetRecycler());

#if ENABLE_NATIVE_CODEGEN
                CreateProfilerNativeCodeGen(this->nativeCodeGen, this->profiler);
#endif

                this->isProfilerCreated = true;
                Profiler * oldProfiler = this->threadContext->GetRecycler()->GetProfiler();
                this->threadContext->GetRecycler()->SetProfiler(this->profiler->GetProfiler(), this->profiler->GetBackgroundRecyclerProfiler());
                return oldProfiler;
            }
            return nullptr;
    }

    void
        ScriptContext::SetRecyclerProfiler()
    {
            Assert(Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag));
            AssertMsg(this->profiler != nullptr, "Profiler tag is supplied but the profiler pointer is NULL");

            if (this->ensureParentInfo)
            {
                this->hostScriptContext->EnsureParentInfo();
                this->ensureParentInfo = false;
            }

            this->GetRecycler()->SetProfiler(this->profiler->GetProfiler(), this->profiler->GetBackgroundRecyclerProfiler());
    }

    void
        ScriptContext::SetProfilerFromScriptContext(ScriptContext * scriptContext)
    {
            // this function needs to be called before any code gen happens so
            // that access to codegenProfiler won't have concurrency issues
            if (Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag))
            {
                Assert(this->profiler != nullptr);
                Assert(this->isProfilerCreated);
                Assert(scriptContext->profiler != nullptr);
                Assert(scriptContext->isProfilerCreated);


                scriptContext->profiler->ProfileMerge(this->profiler);

                this->profiler->Release();
                this->profiler = scriptContext->profiler;
                this->profiler->AddRef();
                this->isProfilerCreated = false;

#if ENABLE_NATIVE_CODEGEN
                SetProfilerFromNativeCodeGen(this->nativeCodeGen, scriptContext->GetNativeCodeGenerator());
#endif

                this->threadContext->GetRecycler()->SetProfiler(this->profiler->GetProfiler(), this->profiler->GetBackgroundRecyclerProfiler());
            }
    }

    void
        ScriptContext::ProfileBegin(Js::Phase phase)
    {
            AssertMsg((this->profiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
                "Profiler tag is supplied but the profiler pointer is NULL");
            if (this->profiler)
            {
                if (this->ensureParentInfo)
                {
                    this->hostScriptContext->EnsureParentInfo();
                    this->ensureParentInfo = false;
                }
                this->profiler->ProfileBegin(phase);
            }
    }

    void
        ScriptContext::ProfileEnd(Js::Phase phase)
    {
            AssertMsg((this->profiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
                "Profiler tag is supplied but the profiler pointer is NULL");
            if (this->profiler)
            {
                this->profiler->ProfileEnd(phase);
            }
    }

    void
        ScriptContext::ProfileSuspend(Js::Phase phase, Js::Profiler::SuspendRecord * suspendRecord)
    {
            AssertMsg((this->profiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
                "Profiler tag is supplied but the profiler pointer is NULL");
            if (this->profiler)
            {
                this->profiler->ProfileSuspend(phase, suspendRecord);
            }
    }

    void
        ScriptContext::ProfileResume(Js::Profiler::SuspendRecord * suspendRecord)
    {
            AssertMsg((this->profiler != nullptr) == Js::Configuration::Global.flags.IsEnabled(Js::ProfileFlag),
                "Profiler tag is supplied but the profiler pointer is NULL");
            if (this->profiler)
            {
                this->profiler->ProfileResume(suspendRecord);
            }
    }

    void
        ScriptContext::ProfilePrint()
    {
            if (disableProfiler)
            {
                return;
            }

            Assert(profiler != nullptr);
            recycler->EnsureNotCollecting();
            profiler->ProfilePrint(Js::Configuration::Global.flags.Profile.GetFirstPhase());
#if ENABLE_NATIVE_CODEGEN
            ProfilePrintNativeCodeGen(this->nativeCodeGen);
#endif
    }
#endif

#ifdef ENABLE_SCRIPT_PROFILING
    inline void ScriptContext::CoreSetProfileEventMask(DWORD dwEventMask)
    {
        AssertMsg(m_pProfileCallback != NULL, "Assigning the event mask when there is no callback");
        m_dwEventMask = dwEventMask;
        m_fTraceFunctionCall = (dwEventMask & PROFILER_EVENT_MASK_TRACE_SCRIPT_FUNCTION_CALL);
        m_fTraceNativeFunctionCall = (dwEventMask & PROFILER_EVENT_MASK_TRACE_NATIVE_FUNCTION_CALL);

        m_fTraceDomCall = (dwEventMask & PROFILER_EVENT_MASK_TRACE_DOM_FUNCTION_CALL);
    }

    HRESULT ScriptContext::RegisterProfileProbe(IActiveScriptProfilerCallback *pProfileCallback, DWORD dwEventMask, DWORD dwContext, RegisterExternalLibraryType RegisterExternalLibrary, JavascriptMethod dispatchInvoke)
    {
        if (m_pProfileCallback != NULL)
        {
            return ACTIVPROF_E_PROFILER_PRESENT;
        }

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RegisterProfileProbe\n"));
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("Info\nThunks Address :\n"));
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("DefaultEntryThunk : 0x%08X, CrossSite::DefaultThunk : 0x%08X, DefaultDeferredParsingThunk : 0x%08X\n"), DefaultEntryThunk, CrossSite::DefaultThunk, DefaultDeferredParsingThunk);
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ProfileEntryThunk : 0x%08X, CrossSite::ProfileThunk : 0x%08X, ProfileDeferredParsingThunk : 0x%08X, ProfileDeferredDeserializeThunk : 0x%08X,\n"), ProfileEntryThunk, CrossSite::ProfileThunk, ProfileDeferredParsingThunk, ProfileDeferredDeserializeThunk);
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptType :\n"));
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("PROFILER_SCRIPT_TYPE_USER : 0, PROFILER_SCRIPT_TYPE_DYNAMIC : 1, PROFILER_SCRIPT_TYPE_NATIVE : 2, PROFILER_SCRIPT_TYPE_DOM : 3\n"));

        HRESULT hr = pProfileCallback->Initialize(dwContext);
        if (SUCCEEDED(hr))
        {
            m_pProfileCallback = pProfileCallback;
            pProfileCallback->AddRef();
            CoreSetProfileEventMask(dwEventMask);
            if (m_fTraceDomCall)
            {
                if (FAILED(pProfileCallback->QueryInterface(&m_pProfileCallback2)))
                {
                    m_fTraceDomCall = FALSE;
                }
            }

            if (webWorkerId != Js::Constants::NonWebWorkerContextId)
            {
                IActiveScriptProfilerCallback3 * pProfilerCallback3;
                if (SUCCEEDED(pProfileCallback->QueryInterface(&pProfilerCallback3)))
                {
                    pProfilerCallback3->SetWebWorkerId(webWorkerId);
                    pProfilerCallback3->Release();
                    // Omitting the HRESULT since it is up to the callback to make use of the webWorker information.
                }
            }

#if DEBUG
            StartNewProfileSession();
#endif

#if ENABLE_NATIVE_CODEGEN
            NativeCodeGenerator *pNativeCodeGen = this->GetNativeCodeGenerator();
            AutoOptionalCriticalSection autoAcquireCodeGenQueue(GetNativeCodeGenCriticalSection(pNativeCodeGen));
#endif

            this->SetProfileMode(TRUE);

#if ENABLE_NATIVE_CODEGEN
            SetProfileModeNativeCodeGen(pNativeCodeGen, TRUE);
#endif

            // Register builtin functions
            if (m_fTraceNativeFunctionCall)
            {
                hr = this->RegisterBuiltinFunctions(RegisterExternalLibrary);
                if (FAILED(hr))
                {
                    return hr;
                }
            }

            this->RegisterAllScripts();

            // Set the dispatch profiler:
            this->SetDispatchProfile(TRUE, dispatchInvoke);

            // Update the function objects currently present in there.
            this->SetFunctionInRecyclerToProfileMode();
        }

        return hr;
    }

    HRESULT ScriptContext::SetProfileEventMask(DWORD dwEventMask)
    {
        if (m_pProfileCallback == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        return ACTIVPROF_E_UNABLE_TO_APPLY_ACTION;
    }

    HRESULT ScriptContext::DeRegisterProfileProbe(HRESULT hrReason, JavascriptMethod dispatchInvoke)
    {
        if (m_pProfileCallback == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::DeRegisterProfileProbe\n"));

#if ENABLE_NATIVE_CODEGEN
        // Acquire the code gen working queue - we are going to change the thunks
        NativeCodeGenerator *pNativeCodeGen = this->GetNativeCodeGenerator();
        Assert(pNativeCodeGen);
        {
            AutoOptionalCriticalSection lock(GetNativeCodeGenCriticalSection(pNativeCodeGen));

            this->SetProfileMode(FALSE);
            SetProfileModeNativeCodeGen(pNativeCodeGen, FALSE);

            // DisableJIT-TODO: Does need to happen even with JIT disabled?
            // Unset the dispatch profiler:
            if (dispatchInvoke != nullptr)
            {
                this->SetDispatchProfile(FALSE, dispatchInvoke);
            }
        }
#endif

        m_inProfileCallback = TRUE;
        HRESULT hr = m_pProfileCallback->Shutdown(hrReason);
        m_inProfileCallback = FALSE;
        m_pProfileCallback->Release();
        m_pProfileCallback = NULL;

        if (m_pProfileCallback2 != NULL)
        {
            m_pProfileCallback2->Release();
            m_pProfileCallback2 = NULL;
        }

#if DEBUG
        StopProfileSession();
#endif

        return hr;
    }

    void ScriptContext::SetProfileMode(BOOL fSet)
    {
#ifdef ENABLE_SCRIPT_PROFILING
        if (fSet)
        {
            AssertMsg(m_pProfileCallback != NULL, "In profile mode when there is no call back");
            this->CurrentThunk = ProfileEntryThunk;
            this->CurrentCrossSiteThunk = CrossSite::ProfileThunk;
            this->DeferredParsingThunk = ProfileDeferredParsingThunk;
            this->DeferredDeserializationThunk = ProfileDeferredDeserializeThunk;
            this->globalObject->EvalHelper = &Js::GlobalObject::ProfileModeEvalHelper;
#if DBG
            this->hadProfiled = true;
#endif
        }
        else
#endif
        {
            Assert(!fSet);
            this->CurrentThunk = DefaultEntryThunk;
            this->CurrentCrossSiteThunk = CrossSite::DefaultThunk;
            this->DeferredParsingThunk = DefaultDeferredParsingThunk;
            this->globalObject->EvalHelper = &Js::GlobalObject::DefaultEvalHelper;

            // In Debug mode/Fast F12 library is still needed for built-in wrappers.
            if (!(this->IsScriptContextInDebugMode() && this->IsExceptionWrapperForBuiltInsEnabled()))
            {
                this->javascriptLibrary->SetProfileMode(FALSE);
            }
        }
    }

    HRESULT ScriptContext::RegisterScript(Js::FunctionProxy * proxy, BOOL fRegisterScript /*default TRUE*/)
    {
        if (m_pProfileCallback == nullptr)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RegisterScript, fRegisterScript : %s, IsFunctionDefer : %s\n"), IsTrueOrFalse(fRegisterScript), IsTrueOrFalse(proxy->IsDeferred()));

        AssertMsg(proxy != nullptr, "Function body cannot be null when calling reporting");
        AssertMsg(proxy->GetScriptContext() == this, "wrong script context while reporting the function?");

        if (fRegisterScript)
        {
            // Register the script to the callback.
            // REVIEW: do we really need to undefer everything?
            HRESULT hr = proxy->EnsureDeserialized()->Parse()->ReportScriptCompiled();
            if (FAILED(hr))
            {
                return hr;
            }
        }

        return !proxy->IsDeferred() ? proxy->GetFunctionBody()->RegisterFunction(false) : S_OK;
    }

    HRESULT ScriptContext::RegisterAllScripts()
    {
        AssertMsg(m_pProfileCallback != nullptr, "Called register scripts when we don't have profile callback");

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RegisterAllScripts started\n"));

        // Future Work: Once Utf8SourceInfo can generate the debug document text without requiring a function body,
        // this code can be considerably simplified to doing the following:
        // - scriptContext->MapScript() : Fire script compiled for each script
        // - scriptContext->MapFunction(): Fire function compiled for each function
        this->MapScript([](Utf8SourceInfo* sourceInfo)
        {
            FunctionBody* functionBody = sourceInfo->GetAnyParsedFunction();
            if (functionBody)
            {
                functionBody->ReportScriptCompiled();
            }
        });

        // FunctionCompiled events for all functions.
        this->MapFunction([](Js::FunctionBody* pFuncBody)
        {
            if (!pFuncBody->GetIsTopLevel() && pFuncBody->GetIsGlobalFunc())
            {
                // This must be the dummy function, generated due to the deferred parsing.
                return;
            }

            pFuncBody->RegisterFunction(TRUE, TRUE); // Ignore potential failure (worst case is not profiling).
        });

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RegisterAllScripts ended\n"));
        return S_OK;
    }
#endif // ENABLE_SCRIPT_PROFILING

    // Shuts down and recreates the native code generator.  This is used when
    // attaching and detaching the debugger in order to clear the list of work
    // items that are pending in the JIT job queue.
    // Alloc first and then free so that the native code generator is at a different address
#if ENABLE_NATIVE_CODEGEN
    HRESULT ScriptContext::RecreateNativeCodeGenerator()
    {
        NativeCodeGenerator* oldCodeGen = this->nativeCodeGen;

        HRESULT hr = S_OK;
        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
            this->nativeCodeGen = NewNativeCodeGenerator(this);
        SetProfileModeNativeCodeGen(this->GetNativeCodeGenerator(), this->IsProfiling());
        END_TRANSLATE_OOM_TO_HRESULT(hr);

        // Delete the native code generator and recreate so that all jobs get cleared properly
        // and re-jitted.
        CloseNativeCodeGenerator(oldCodeGen);
        DeleteNativeCodeGenerator(oldCodeGen);

        return hr;
    }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
    HRESULT ScriptContext::OnDebuggerAttached()
    {
        OUTPUT_TRACE(Js::DebuggerPhase, _u("ScriptContext::OnDebuggerAttached: start 0x%p\n"), this);

        Js::StepController* stepController = &this->GetThreadContext()->GetDebugManager()->stepController;
        if (stepController->IsActive())
        {
            AssertMsg(stepController->GetActivatedContext() == nullptr, "StepController should not be active when we attach.");
            stepController->Deactivate(); // Defense in depth
        }

        bool shouldPerformSourceRundown = false;
        if (this->IsScriptContextInNonDebugMode())
        {
            // Today we do source rundown as a part of attach to support VS attaching without
            // first calling PerformSourceRundown.  PerformSourceRundown will be called once
            // by debugger host prior to attaching.
            this->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::SourceRundown);

            // Need to perform rundown only once.
            shouldPerformSourceRundown = true;
        }

        // Rundown on all existing functions and change their thunks so that they will go to debug mode once they are called.

        HRESULT hr = OnDebuggerAttachedDetached(/*attach*/ true);

        // Debugger attach/detach failure is catastrophic, take down the process
        DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);

        // Disable QC while functions are re-parsed as this can be time consuming
        AutoDisableInterrupt autoDisableInterrupt(this->threadContext, false /* explicitCompletion */);

        hr = this->GetDebugContext()->RundownSourcesAndReparse(shouldPerformSourceRundown, /*shouldReparseFunctions*/ true);

        if (this->IsClosed())
        {
            return hr;
        }

        // Debugger attach/detach failure is catastrophic, take down the process
        DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);

        HRESULT hrEntryPointUpdate = S_OK;
        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
#ifdef ASMJS_PLAT
            TempArenaAllocatorObject* tmpAlloc = GetTemporaryAllocator(_u("DebuggerTransition"));
            debugTransitionAlloc = tmpAlloc->GetAllocator();

            asmJsEnvironmentMap = Anew(debugTransitionAlloc, AsmFunctionMap, debugTransitionAlloc);
#endif

            // Still do the pass on the function's entrypoint to reflect its state with the functionbody's entrypoint.
            this->UpdateRecyclerFunctionEntryPointsForDebugger();

#ifdef ASMJS_PLAT
            auto asmEnvIter = asmJsEnvironmentMap->GetIterator();
            while (asmEnvIter.IsValid())
            {
                // we are attaching, change frame setup for asm.js frame to javascript frame
                SList<AsmJsScriptFunction *> * funcList = asmEnvIter.CurrentValue();
                Assert(!funcList->Empty());
                void* newEnv = AsmJsModuleInfo::ConvertFrameForJavascript(asmEnvIter.CurrentKey(), funcList->Head());
                funcList->Iterate([&](AsmJsScriptFunction * func)
                {
                    func->GetEnvironment()->SetItem(0, newEnv);
                });
                asmEnvIter.MoveNext();
            }

            // walk through and clean up the asm.js fields as a discrete step, because module might be multiply linked
            auto asmCleanupIter = asmJsEnvironmentMap->GetIterator();
            while (asmCleanupIter.IsValid())
            {
                SList<AsmJsScriptFunction *> * funcList = asmCleanupIter.CurrentValue();
                Assert(!funcList->Empty());
                funcList->Iterate([](AsmJsScriptFunction * func)
                {
                    func->SetModuleMemory(nullptr);
                    func->GetFunctionBody()->ResetAsmJsInfo();
                });
                asmCleanupIter.MoveNext();
            }

            ReleaseTemporaryAllocator(tmpAlloc);
#endif
        END_TRANSLATE_OOM_TO_HRESULT(hrEntryPointUpdate);

        if (hrEntryPointUpdate != S_OK)
        {
            // should only be here for OOM
            Assert(hrEntryPointUpdate == E_OUTOFMEMORY);
            return hrEntryPointUpdate;
        }

        OUTPUT_TRACE(Js::DebuggerPhase, _u("ScriptContext::OnDebuggerAttached: done 0x%p, hr = 0x%X\n"), this, hr);

        return hr;
    }

    // Reverts the script context state back to the state before debugging began.
    HRESULT ScriptContext::OnDebuggerDetached()
    {
        OUTPUT_TRACE(Js::DebuggerPhase, _u("ScriptContext::OnDebuggerDetached: start 0x%p\n"), this);

        Js::StepController* stepController = &this->GetThreadContext()->GetDebugManager()->stepController;
        if (stepController->IsActive())
        {
            // Normally step controller is deactivated on start of dispatch (step, async break, exception, etc),
            // and in the beginning of interpreter loop we check for step complete (can cause check whether current bytecode belong to stmt).
            // But since it holds to functionBody/statementMaps, we have to deactivate it as func bodies are going away/reparsed.
            stepController->Deactivate();
        }

        // Go through all existing functions and change their thunks back to using non-debug mode versions when called
        // and notify the script context that the debugger has detached to allow it to revert the runtime to the proper
        // state (JIT enabled).

        HRESULT hr = OnDebuggerAttachedDetached(/*attach*/ false);

        // Debugger attach/detach failure is catastrophic, take down the process
        DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);

        // Move the debugger into source rundown mode.
        this->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::SourceRundown);

        // Disable QC while functions are re-parsed as this can be time consuming
        AutoDisableInterrupt autoDisableInterrupt(this->threadContext, false /* explicitCompletion */);

        // Force a reparse so that indirect function caches are updated.
        hr = this->GetDebugContext()->RundownSourcesAndReparse(/*shouldPerformSourceRundown*/ false, /*shouldReparseFunctions*/ true);

        if (this->IsClosed())
        {
            return hr;
        }

        // Debugger attach/detach failure is catastrophic, take down the process
        DEBUGGER_ATTACHDETACH_FATAL_ERROR_IF_FAILED(hr);

        // Still do the pass on the function's entrypoint to reflect its state with the functionbody's entrypoint.
        this->UpdateRecyclerFunctionEntryPointsForDebugger();

        OUTPUT_TRACE(Js::DebuggerPhase, _u("ScriptContext::OnDebuggerDetached: done 0x%p, hr = 0x%X\n"), this, hr);

        return hr;
    }

    HRESULT ScriptContext::OnDebuggerAttachedDetached(bool attach)
    {

        // notify threadContext that debugger is attaching so do not do expire
        struct AutoRestore
        {
            AutoRestore(ThreadContext* threadContext)
                :threadContext(threadContext)
            {
                this->threadContext->GetDebugManager()->SetDebuggerAttaching(true);
            }
            ~AutoRestore()
            {
                this->threadContext->GetDebugManager()->SetDebuggerAttaching(false);
            }

        private:
            ThreadContext* threadContext;

        } autoRestore(this->GetThreadContext());

        // xplat-todo: (obastemur) Enable JIT on Debug mode
        // CodeGen entrypoint can be deleted before we are able to unregister
        // due to how we handle xdata on xplat, resetting the entrypoints below might affect CodeGen process.
        // it is safer (on xplat) to turn JIT off during Debug for now.
#ifdef _WIN32
        if (!Js::Configuration::Global.EnableJitInDebugMode())
#endif
        {
            if (attach)
            {
                // Now force nonative, so the job will not be put in jit queue.
                ForceNoNative();
            }
            else
            {
                // Take the runtime out of interpreted mode so the JIT
                // queue can be exercised.
                this->ForceNative();
            }
        }

        // Invalidate all the caches.
        this->threadContext->InvalidateAllProtoInlineCaches();
        this->threadContext->InvalidateAllStoreFieldInlineCaches();
        this->threadContext->InvalidateAllIsInstInlineCaches();

        if (!attach)
        {
            this->UnRegisterDebugThunk();

            // Remove all breakpoint probes
            this->GetDebugContext()->GetProbeContainer()->RemoveAllProbes();
        }

        HRESULT hr = S_OK;

#ifndef _WIN32
        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
        {
            // xplat eh_frame handling is a bit different than Windows
            // RecreateNativeCodeGenerator call below will be cleaning up
            // XDataAllocation and we won't be able to __DEREGISTER_FRAME

            // xplat-todo: make eh_frame handling better
            this->sourceList->Map([=](uint i, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef) {
                Js::Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();

                if (sourceInfo != nullptr)
                {
                    sourceInfo->MapFunction([](Js::FunctionBody* functionBody) {
                        functionBody->ResetEntryPoint();
                    });
                }
            });
        }
        END_TRANSLATE_OOM_TO_HRESULT(hr);

        if (FAILED(hr))
        {
            return hr;
        }
#endif

        if (!CONFIG_FLAG(ForceDiagnosticsMode))
        {
#if ENABLE_NATIVE_CODEGEN
            // Recreate the native code generator so that all pending
            // JIT work items will be cleared.
            hr = RecreateNativeCodeGenerator();
            if (FAILED(hr))
            {
                return hr;
            }
#endif
            if (attach)
            {
                // We need to transition to debug mode after the NativeCodeGenerator is cleared/closed. Since the NativeCodeGenerator will be working on a different thread - it may
                // be checking on the DebuggerState (from ScriptContext) while emitting code.
                this->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);
#if ENABLE_NATIVE_CODEGEN
                UpdateNativeCodeGeneratorForDebugMode(this->nativeCodeGen);
#endif
            }
        }
        else if (attach)
        {
            this->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);
        }

        BEGIN_TRANSLATE_OOM_TO_HRESULT_NESTED
        {
            // Remap all the function entry point thunks.
            this->sourceList->Map([=](uint i, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef) {
                Js::Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();

                if (sourceInfo != nullptr)
                {
                    if (!sourceInfo->GetIsLibraryCode())
                    {
                        sourceInfo->SetInDebugMode(attach);

                        sourceInfo->MapFunction([](Js::FunctionBody* functionBody) {
                            functionBody->SetEntryToDeferParseForDebugger();
                        });
                    }
#ifdef _WIN32
                    else
                    {
                        sourceInfo->MapFunction([](Js::FunctionBody* functionBody) {
                            functionBody->ResetEntryPoint();
                        });
                    }
#endif
                }
            });
        }
        END_TRANSLATE_OOM_TO_HRESULT(hr);

        if (FAILED(hr))
        {
            return hr;
        }

        if (attach)
        {
            this->RegisterDebugThunk();
        }

#if ENABLE_PROFILE_INFO
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
        // Reset the dynamic profile list
        if (this->Cache()->profileInfoList)
        {
            this->Cache()->profileInfoList->Reset();
        }
#endif
#endif
        return hr;
    }
#endif

#if defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
    // We use ProfileThunk under debugger.
    void ScriptContext::RegisterDebugThunk(bool calledDuringAttach /*= true*/)
    {
        if (this->IsExceptionWrapperForBuiltInsEnabled())
        {
            this->CurrentThunk = ProfileEntryThunk;
            this->CurrentCrossSiteThunk = CrossSite::ProfileThunk;
#if ENABLE_NATIVE_CODEGEN
            SetProfileModeNativeCodeGen(this->GetNativeCodeGenerator(), TRUE);
#endif

            // Set library to profile mode so that for built-ins all new instances of functions
            // are created with entry point set to the ProfileThunk.
            this->javascriptLibrary->SetProfileMode(true);
            this->javascriptLibrary->SetDispatchProfile(true, DispatchProfileInvoke);

            if (!calledDuringAttach)
            {
#ifdef ENABLE_SCRIPT_PROFILING
                m_fTraceDomCall = TRUE; // This flag is always needed in DebugMode to wrap external functions with DebugProfileThunk
#endif
                // Update the function objects currently present in there.
                this->SetFunctionInRecyclerToProfileMode(true/*enumerateNonUserFunctionsOnly*/);
            }
        }
    }

    void ScriptContext::UnRegisterDebugThunk()
    {
        if (!this->IsProfiling() && this->IsExceptionWrapperForBuiltInsEnabled())
        {
            this->CurrentThunk = DefaultEntryThunk;
            this->CurrentCrossSiteThunk = CrossSite::DefaultThunk;
#if ENABLE_NATIVE_CODEGEN
            SetProfileModeNativeCodeGen(this->GetNativeCodeGenerator(), FALSE);
#endif

            if (!this->IsProfiling())
            {
                this->javascriptLibrary->SetProfileMode(false);
                this->javascriptLibrary->SetDispatchProfile(false, DispatchDefaultInvoke);
            }
        }
    }
#endif // defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)

#ifdef ENABLE_SCRIPT_PROFILING
    HRESULT ScriptContext::RegisterBuiltinFunctions(RegisterExternalLibraryType RegisterExternalLibrary)
    {
        Assert(m_pProfileCallback != NULL);

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RegisterBuiltinFunctions\n"));

        HRESULT hr = S_OK;
        // Consider creating ProfileArena allocator instead of General allocator
        if (m_pBuiltinFunctionIdMap == NULL)
        {
            // Anew throws if it OOMs, so the caller into this function needs to handle that exception
            m_pBuiltinFunctionIdMap = Anew(GeneralAllocator(), BuiltinFunctionIdDictionary,
                GeneralAllocator(), 17);
        }

        this->javascriptLibrary->SetProfileMode(TRUE);

        if (FAILED(hr = OnScriptCompiled(BuiltInFunctionsScriptId, PROFILER_SCRIPT_TYPE_NATIVE, NULL)))
        {
            return hr;
        }

        if (FAILED(hr = this->javascriptLibrary->ProfilerRegisterBuiltIns()))
        {
            return hr;
        }

        // External Library
        if (RegisterExternalLibrary != NULL)
        {
            (*RegisterExternalLibrary)(this);
        }

        return hr;
    }
#endif // ENABLE_SCRIPT_PROFILING

#ifdef ENABLE_SCRIPT_DEBUGGING
    void ScriptContext::SetFunctionInRecyclerToProfileMode(bool enumerateNonUserFunctionsOnly/* = false*/)
    {
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::SetFunctionInRecyclerToProfileMode started (m_fTraceDomCall : %s)\n"), IsTrueOrFalse(IsTraceDomCall()));

        // Mark this script context isEnumeratingRecyclerObjects
        AutoEnumeratingRecyclerObjects enumeratingRecyclerObjects(this);

        m_enumerateNonUserFunctionsOnly = enumerateNonUserFunctionsOnly;

        this->recycler->EnumerateObjects(JavascriptLibrary::EnumFunctionClass, &ScriptContext::RecyclerEnumClassEnumeratorCallback);

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::SetFunctionInRecyclerToProfileMode ended\n"));
    }

    void ScriptContext::UpdateRecyclerFunctionEntryPointsForDebugger()
    {
        // Mark this script context isEnumeratingRecyclerObjects
        AutoEnumeratingRecyclerObjects enumeratingRecyclerObjects(this);

        this->recycler->EnumerateObjects(JavascriptLibrary::EnumFunctionClass, &ScriptContext::RecyclerFunctionCallbackForDebugger);
    }

#ifdef ASMJS_PLAT
    void ScriptContext::TransitionEnvironmentForDebugger(ScriptFunction * scriptFunction)
    {
        FunctionBody* functionBody = scriptFunction->GetFunctionBody();
#ifdef ENABLE_WASM
        // Wasm functions will not get reparsed, but the jitted code will be lost
        // Reset the entry point
        if (functionBody->IsWasmFunction())
        {
            // In case we are in debugger mode, make sure we use the non profiling thunk for this new entry point
            JavascriptMethod realThunk = CurrentThunk;
            CurrentThunk = AsmJsDefaultEntryThunk;
            functionBody->ResetEntryPoint();
            CurrentThunk = realThunk;

            bool isDeferred = functionBody->GetAsmJsFunctionInfo()->IsWasmDeferredParse();
            // Make sure the function and the function body are using the same entry point
            scriptFunction->ChangeEntryPoint(functionBody->GetDefaultEntryPointInfo(), AsmJsDefaultEntryThunk);
            WasmLibrary::SetWasmEntryPointToInterpreter(scriptFunction, isDeferred);
            // Reset jit status for this function
            functionBody->SetIsAsmJsFullJitScheduled(false);
            Assert(functionBody->HasValidEntryPoint());
        }
        else
#endif
        if (scriptFunction->GetScriptContext()->IsScriptContextInDebugMode())
        {
            if (functionBody->IsInDebugMode() &&
                scriptFunction->GetFunctionBody()->GetAsmJsFunctionInfo() != nullptr &&
                scriptFunction->GetFunctionBody()->GetAsmJsFunctionInfo()->GetModuleFunctionBody() != nullptr)
            {
                void* env = scriptFunction->GetEnvironment()->GetItem(0);
                SList<AsmJsScriptFunction*> * funcList = nullptr;
                if (asmJsEnvironmentMap->TryGetValue(env, &funcList))
                {
                    funcList->Push((AsmJsScriptFunction*)scriptFunction);
                }
                else
                {
                    SList<AsmJsScriptFunction*> * newList = Anew(debugTransitionAlloc, SList<AsmJsScriptFunction*>, debugTransitionAlloc);
                    asmJsEnvironmentMap->AddNew(env, newList);
                    newList->Push((AsmJsScriptFunction*)scriptFunction);
                }
            }
        }
    }
#endif

    /*static*/
    void ScriptContext::RecyclerFunctionCallbackForDebugger(void *address, size_t size)
    {
        JavascriptFunction *pFunction = (JavascriptFunction *)address;

        ScriptContext* scriptContext = pFunction->GetScriptContext();
        if (scriptContext == nullptr || scriptContext->IsClosed())
        {
            // Can't enumerate from closed scriptcontext
            return;
        }

        if (!scriptContext->IsEnumeratingRecyclerObjects())
        {
            return; // function not from enumerating script context
        }

        // Wrapped function are not allocated with the EnumClass bit
        Assert(pFunction->GetFunctionInfo() != &JavascriptExternalFunction::EntryInfo::WrappedFunctionThunk);

        JavascriptMethod entryPoint = pFunction->GetEntryPoint();
        FunctionInfo * info = pFunction->GetFunctionInfo();
        FunctionProxy * proxy = info->GetFunctionProxy();

        if (proxy == nullptr)
        {
            // Not a user defined function, we need to wrap them with try-catch for "continue after exception"
            if (!pFunction->IsScriptFunction() && IsExceptionWrapperForBuiltInsEnabled(scriptContext))
            {
#if defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
                if (scriptContext->IsScriptContextInDebugMode())
                {
                    // We are attaching.
                    // For built-ins, WinRT and DOM functions which are already in recycler, change entry points to route to debug/profile thunk.
                    ScriptContext::SetEntryPointToProfileThunk(pFunction);
                }
                else
                {
                    // We are detaching.
                    // For built-ins, WinRT and DOM functions which are already in recycler, restore entry points to original.
                    if (!scriptContext->IsProfiling())
                    {
                        ScriptContext::RestoreEntryPointFromProfileThunk(pFunction);
                    }
                    // If we are profiling, don't change anything.
                }
#else
                AssertMsg(false, "Debugging/Profiling needs to be enabled to change thunks");
#endif
            }

            return;
        }

        Assert(proxy->GetFunctionInfo() == info);

        if (!proxy->IsFunctionBody())
        {
            // REVIEW: why we still have function that is still deferred?
            return;
        }
        Assert(pFunction->IsScriptFunction());

        // Excluding the internal library code, which is not debuggable already
        if (!proxy->GetUtf8SourceInfo()->GetIsLibraryCode())
        {
            // Reset the constructor cache to default, so that it will not pick up the cached type, created before debugging.
            // Look bug: 301517
            pFunction->ResetConstructorCacheToDefault();
        }

        if (ScriptFunctionWithInlineCache::Is(pFunction))
        {
            ScriptFunctionWithInlineCache::FromVar(pFunction)->ClearInlineCacheOnFunctionObject();
        }

        // We should have force parsed the function, and have a function body
        FunctionBody * pBody = proxy->GetFunctionBody();

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (scriptContext->IsScriptContextInDebugMode() &&
            !proxy->GetUtf8SourceInfo()->GetIsLibraryCode() &&
#ifdef ENABLE_WASM
            !pBody->IsWasmFunction() &&
#endif
            !pBody->IsInDebugMode())
        {
            // Identifying if any function escaped for not being in debug mode. (This can be removed as a part of TFS : 935011)
            Throw::FatalInternalError();
        }
#endif

        ScriptFunction * scriptFunction = ScriptFunction::FromVar(pFunction);

#ifdef ASMJS_PLAT
        scriptContext->TransitionEnvironmentForDebugger(scriptFunction);
#endif

        JavascriptMethod newEntryPoint;
        if (CrossSite::IsThunk(entryPoint))
        {
            // Can't change from cross-site to non-cross-site, but still need to update the e.p.info on ScriptFunctionType.
            newEntryPoint = entryPoint;
        }
        else
        {
            newEntryPoint = pBody->GetDirectEntryPoint(pBody->GetDefaultFunctionEntryPointInfo());
        }

        scriptFunction->ChangeEntryPoint(pBody->GetDefaultFunctionEntryPointInfo(), newEntryPoint);
    }
#endif

#if defined(ENABLE_SCRIPT_PROFILING) || defined(ENABLE_SCRIPT_DEBUGGING)
    void ScriptContext::RecyclerEnumClassEnumeratorCallback(void *address, size_t size)
    {
        // TODO: we are assuming its function because for now we are enumerating only on functions
        // In future if the RecyclerNewEnumClass is used of Recyclable objects or Dynamic object, we would need a check if it is function
        JavascriptFunction *pFunction = (JavascriptFunction *)address;

        ScriptContext* scriptContext = pFunction->GetScriptContext();
        if (scriptContext == nullptr || scriptContext->IsClosed())
        {
            // Can't enumerate from closed scriptcontext
            return;
        }

        if (!scriptContext->IsEnumeratingRecyclerObjects())
        {
            return; // function not from enumerating script context
        }

        if (!scriptContext->IsTraceDomCall() && (pFunction->IsExternalFunction() || pFunction->IsWinRTFunction()))
        {
            return;
        }

        if (scriptContext->IsEnumerateNonUserFunctionsOnly() && pFunction->IsScriptFunction())
        {
            return;
        }

        // Wrapped function are not allocated with the EnumClass bit
        Assert(pFunction->GetFunctionInfo() != &JavascriptExternalFunction::EntryInfo::WrappedFunctionThunk);

        JavascriptMethod entryPoint = pFunction->GetEntryPoint();
        FunctionProxy *proxy = pFunction->GetFunctionProxy();

        if (proxy != NULL)
        {
#if defined(ENABLE_SCRIPT_PROFILING)
#if ENABLE_DEBUG_CONFIG_OPTIONS
            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
            OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::RecyclerEnumClassEnumeratorCallback\n"));
            OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("\tFunctionProxy : 0x%08X, FunctionNumber : %s, DeferredParseAttributes : %d, EntryPoint : 0x%08X"),
                (DWORD_PTR)proxy, proxy->GetDebugNumberSet(debugStringBuffer), proxy->GetAttributes(), (DWORD_PTR)entryPoint);
#if ENABLE_NATIVE_CODEGEN
            OUTPUT_TRACE(Js::ScriptProfilerPhase, _u(" (IsIntermediateCodeGenThunk : %s, isNative : %s)\n"),
                IsTrueOrFalse(IsIntermediateCodeGenThunk(entryPoint)), IsTrueOrFalse(scriptContext->IsNativeAddress(entryPoint)));
#endif
            OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("\n"));
#endif

#if ENABLE_NATIVE_CODEGEN
            if (!IsIntermediateCodeGenThunk(entryPoint) && entryPoint != DynamicProfileInfo::EnsureDynamicProfileInfoThunk)
#endif
            {
                OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("\t\tJs::ScriptContext::GetProfileModeThunk : 0x%08X\n"), (DWORD_PTR)Js::ScriptContext::GetProfileModeThunk(entryPoint));

                ScriptFunction * scriptFunction = ScriptFunction::FromVar(pFunction);
                scriptFunction->ChangeEntryPoint(proxy->GetDefaultEntryPointInfo(), Js::ScriptContext::GetProfileModeThunk(entryPoint));

#if ENABLE_NATIVE_CODEGEN && defined(ENABLE_SCRIPT_PROFILING)
                OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("\tUpdated entrypoint : 0x%08X (isNative : %s)\n"), (DWORD_PTR)pFunction->GetEntryPoint(), IsTrueOrFalse(scriptContext->IsNativeAddress(entryPoint)));
#endif
            }
        }
        else
        {
            ScriptContext::SetEntryPointToProfileThunk(pFunction);
        }
    }

    // static
    void ScriptContext::SetEntryPointToProfileThunk(JavascriptFunction* function)
    {
        JavascriptMethod entryPoint = function->GetEntryPoint();
        if (entryPoint == Js::CrossSite::DefaultThunk)
        {
            function->SetEntryPoint(Js::CrossSite::ProfileThunk);
        }
        else if (entryPoint != Js::CrossSite::ProfileThunk && entryPoint != ProfileEntryThunk)
        {
            function->SetEntryPoint(ProfileEntryThunk);
        }
    }

    // static
    void ScriptContext::RestoreEntryPointFromProfileThunk(JavascriptFunction* function)
    {
        JavascriptMethod entryPoint = function->GetEntryPoint();
        if (entryPoint == Js::CrossSite::ProfileThunk)
        {
            function->SetEntryPoint(Js::CrossSite::DefaultThunk);
        }
        else if (entryPoint == ProfileEntryThunk)
        {
            function->SetEntryPoint(function->GetFunctionInfo()->GetOriginalEntryPoint());
        }
    }

    JavascriptMethod ScriptContext::GetProfileModeThunk(JavascriptMethod entryPoint)
    {
    #if ENABLE_NATIVE_CODEGEN
        Assert(!IsIntermediateCodeGenThunk(entryPoint));
    #endif
        if (entryPoint == DefaultDeferredParsingThunk || entryPoint == ProfileDeferredParsingThunk)
        {
            return ProfileDeferredParsingThunk;
        }

        if (entryPoint == DefaultDeferredDeserializeThunk || entryPoint == ProfileDeferredDeserializeThunk)
        {
            return ProfileDeferredDeserializeThunk;
        }

        if (CrossSite::IsThunk(entryPoint))
        {
            return CrossSite::ProfileThunk;
        }
        return ProfileEntryThunk;
    }
#endif // defined(ENABLE_SCRIPT_PROFILING) || defiend(ENABLE_SCRIPT_DEBUGGING)

#if _M_IX86
    __declspec(naked)
        Var ScriptContext::ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
            // Register functions
            __asm
            {
                push ebp
                    mov ebp, esp
                    lea eax, [esp + 8]
                    push eax
                    call ScriptContext::ProfileModeDeferredParse
#ifdef _CONTROL_FLOW_GUARD
                    // verify that the call target is valid
                    mov  ecx, eax
                    call[__guard_check_icall_fptr]
                    mov eax, ecx
#endif
                    pop ebp
                    // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
                    jmp eax
            }
    }
#elif defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
    // Do nothing: the implementation of ScriptContext::ProfileModeDeferredParsingThunk is declared (appropriately decorated) in
    // Language\amd64\amd64_Thunks.asm and Language\arm\arm_Thunks.asm and Language\arm64\arm64_Thunks.asm respectively.
#else
    Var ScriptContext::ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        Js::Throw::NotImplemented();
        return nullptr;
    }
#endif

    Js::JavascriptMethod ScriptContext::ProfileModeDeferredParse(ScriptFunction ** functionRef)
    {
#if ENABLE_DEBUG_CONFIG_OPTIONS
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::ProfileModeDeferredParse FunctionNumber : %s, startEntrypoint : 0x%08X\n"), (*functionRef)->GetFunctionProxy()->GetDebugNumberSet(debugStringBuffer), (*functionRef)->GetEntryPoint());

        BOOL fParsed = FALSE;
        JavascriptMethod entryPoint = Js::JavascriptFunction::DeferredParseCore(functionRef, fParsed);

#ifdef ENABLE_SCRIPT_PROFILING
        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("\t\tIsParsed : %s, updatedEntrypoint : 0x%08X\n"), IsTrueOrFalse(fParsed), entryPoint);

        //To get the scriptContext we only need the functionProxy
        FunctionProxy *pRootBody = (*functionRef)->GetFunctionProxy();
        ScriptContext *pScriptContext = pRootBody->GetScriptContext();
        if (pScriptContext->IsProfiling() && !pRootBody->GetFunctionBody()->HasFunctionCompiledSent())
        {
            pScriptContext->RegisterScript(pRootBody, FALSE /*fRegisterScript*/);
        }

        // We can come to this function even though we have stopped profiling.
        Assert(!pScriptContext->IsProfiling() || (*functionRef)->GetFunctionBody()->GetProfileSession() == pScriptContext->GetProfileSession());
#endif

        return entryPoint;
    }

#if _M_IX86
    __declspec(naked)
        Var ScriptContext::ProfileModeDeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
            // Register functions
            __asm
            {
                    push ebp
                    mov ebp, esp
                    push[esp + 8]
                    call ScriptContext::ProfileModeDeferredDeserialize
#ifdef _CONTROL_FLOW_GUARD
                    // verify that the call target is valid
                    mov  ecx, eax
                    call[__guard_check_icall_fptr]
                    mov eax, ecx
#endif
                    pop ebp
                    // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
                    jmp eax
            }
    }
#elif defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
    // Do nothing: the implementation of ScriptContext::ProfileModeDeferredDeserializeThunk is declared (appropriately decorated) in
    // Language\amd64\amd64_Thunks.asm and Language\arm\arm_Thunks.asm respectively.
#endif

    Js::JavascriptMethod ScriptContext::ProfileModeDeferredDeserialize(ScriptFunction *function)
    {
#if ENABLE_DEBUG_CONFIG_OPTIONS
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::ProfileModeDeferredDeserialize FunctionNumber : %s\n"), function->GetFunctionProxy()->GetDebugNumberSet(debugStringBuffer));

        JavascriptMethod entryPoint = Js::JavascriptFunction::DeferredDeserialize(function);

#ifdef ENABLE_SCRIPT_PROFILING
        //To get the scriptContext; we only need the FunctionProxy
        FunctionProxy *pRootBody = function->GetFunctionProxy();
        ScriptContext *pScriptContext = pRootBody->GetScriptContext();
        if (pScriptContext->IsProfiling() && !pRootBody->GetFunctionBody()->HasFunctionCompiledSent())
        {
            pScriptContext->RegisterScript(pRootBody, FALSE /*fRegisterScript*/);
        }

        // We can come to this function even though we have stopped profiling.
        Assert(!pScriptContext->IsProfiling() || function->GetFunctionBody()->GetProfileSession() == pScriptContext->GetProfileSession());
#endif

        return entryPoint;
    }

#ifdef ENABLE_SCRIPT_PROFILING
    BOOL ScriptContext::GetProfileInfo(
        JavascriptFunction* function,
        PROFILER_TOKEN &scriptId,
        PROFILER_TOKEN &functionId)
    {
        BOOL fCanProfile = (m_pProfileCallback != nullptr && m_fTraceFunctionCall);
        if (!fCanProfile)
        {
            return FALSE;
        }

        Js::FunctionInfo* functionInfo = function->GetFunctionInfo();
        if (functionInfo->GetAttributes() & FunctionInfo::DoNotProfile)
        {
            return FALSE;
        }

        Js::FunctionBody * functionBody = functionInfo->GetFunctionBody();
        if (functionBody == nullptr)
        {
            functionId = GetFunctionNumber(functionInfo->GetOriginalEntryPoint());
            if (functionId == -1)
            {
                // Dom Call
                return m_fTraceDomCall && (m_pProfileCallback2 != nullptr);
            }
            else
            {
                // Builtin function
                scriptId = BuiltInFunctionsScriptId;
                return m_fTraceNativeFunctionCall;
            }
        }
        else if (!functionBody->GetUtf8SourceInfo()->GetIsLibraryCode() || functionBody->IsPublicLibraryCode()) // user script or public library code
        {
            scriptId = (PROFILER_TOKEN)functionBody->GetUtf8SourceInfo()->GetSourceInfoId();
            functionId = functionBody->GetFunctionNumber();
            return TRUE;
        }

        return FALSE;
    }
#endif // ENABLE_SCRIPT_PROFILING

    bool ScriptContext::IsForceNoNative()
    {
        bool forceNoNative = false;
        if (this->IsScriptContextInSourceRundownOrDebugMode())
        {
            forceNoNative = this->IsInterpreted();
        }
        else if (!Js::Configuration::Global.EnableJitInDebugMode())
        {
            forceNoNative = true;
            this->ForceNoNative();
        }
        return forceNoNative;
    }

#ifdef ENABLE_SCRIPT_DEBUGGING
    void ScriptContext::InitializeDebugging()
    {
        if (!this->IsScriptContextInDebugMode()) // If we already in debug mode, we would have done below changes already.
        {
            this->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);
            if (this->IsScriptContextInDebugMode())
            {
                // Note: for this we need final IsInDebugMode and NativeCodeGen initialized,
                //       and inside EnsureScriptContext, which seems appropriate as well,
                //       it's too early as debugger manager is not registered, thus IsDebuggerEnvironmentAvailable is false.
                this->RegisterDebugThunk(false/*calledDuringAttach*/);

                // TODO: for launch scenario for external and WinRT functions it might be too late to register debug thunk here,
                //       as we need the thunk registered before FunctionInfo's for built-ins, that may throw, are created.
                //       Need to verify. If that's the case, one way would be to enumerate and fix all external/winRT thunks here.
            }
        }
    }
#endif

    // Combined profile/debug wrapper thunk.
    // - used when we profile to send profile events
    // - used when we debug, only used for built-in functions
    // - used when we profile and debug
    Var ScriptContext::DebugProfileProbeThunk(RecyclableObject* callable, CallInfo callInfo, ...)
    {
#if defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
        RUNTIME_ARGUMENTS(args, callInfo);

        Assert(!AsmJsScriptFunction::IsWasmScriptFunction(callable));
        JavascriptFunction* function = JavascriptFunction::FromVar(callable);
        ScriptContext* scriptContext = function->GetScriptContext();

        // We can come here when profiling is not on
        // e.g. User starts profiling, we update all thinks and then stop profiling - we don't update thunk
        // So we still get this call
#if defined(ENABLE_SCRIPT_PROFILING)
        bool functionEnterEventSent = false;
        char16 *pwszExtractedFunctionName = NULL;
        size_t functionNameLen = 0;
        const char16 *pwszFunctionName = NULL;
        HRESULT hrOfEnterEvent = S_OK;

        PROFILER_TOKEN scriptId = -1;
        PROFILER_TOKEN functionId = -1;
        const bool isProfilingUserCode = scriptContext->GetThreadContext()->IsProfilingUserCode();
        const bool isUserCode = !function->IsLibraryCode();

        const bool fProfile = (isUserCode || isProfilingUserCode) // Only report user code or entry library code
            && scriptContext->GetProfileInfo(function, scriptId, functionId);

        if (fProfile)
        {
            Js::FunctionBody *pBody = function->GetFunctionBody();
            if (pBody != nullptr && !pBody->HasFunctionCompiledSent())
            {
                pBody->RegisterFunction(false/*changeThunk*/);
            }

#if DEBUG
            { // scope

                Assert(scriptContext->IsProfiling());

                if (pBody && pBody->GetProfileSession() != pBody->GetScriptContext()->GetProfileSession())
                {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    OUTPUT_TRACE_DEBUGONLY(Js::ScriptProfilerPhase, _u("ScriptContext::ProfileProbeThunk, ProfileSession does not match (%d != %d), functionNumber : %s, functionName : %s\n"),
                        pBody->GetProfileSession(), pBody->GetScriptContext()->GetProfileSession(), pBody->GetDebugNumberSet(debugStringBuffer), pBody->GetDisplayName());
                }
                AssertMsg(pBody == NULL || pBody->GetProfileSession() == pBody->GetScriptContext()->GetProfileSession(), "Function info wasn't reported for this profile session");
            }
#endif // DEBUG

            if (functionId == -1)
            {
                Var sourceString = function->GetSourceString();

                // SourceString will be null for the Js::BoundFunction, don't throw Enter/Exit notification in that case.
                if (sourceString != NULL)
                {
                    if (TaggedInt::Is(sourceString))
                    {
                        PropertyId nameId = TaggedInt::ToInt32(sourceString);
                        pwszFunctionName = scriptContext->GetPropertyString(nameId)->GetSz();
                    }
                    else
                    {
                        // it is string because user had called in toString extract name from it
                        Assert(JavascriptString::Is(sourceString));
                        const char16 *pwszToString = ((JavascriptString *)sourceString)->GetSz();
                        const char16 *pwszNameStart = wcsstr(pwszToString, _u(" "));
                        const char16 *pwszNameEnd = wcsstr(pwszToString, _u("("));
                        if (pwszNameStart == nullptr || pwszNameEnd == nullptr || ((int)(pwszNameEnd - pwszNameStart) <= 0))
                        {
                            functionNameLen = ((JavascriptString *)sourceString)->GetLength() + 1;
                            pwszExtractedFunctionName = HeapNewArray(char16, functionNameLen);
                            wcsncpy_s(pwszExtractedFunctionName, functionNameLen, pwszToString, _TRUNCATE);
                        }
                        else
                        {
                            functionNameLen = pwszNameEnd - pwszNameStart;
                            AssertMsg(functionNameLen < INT_MAX, "Allocating array with zero or negative length?");
                            pwszExtractedFunctionName = HeapNewArray(char16, functionNameLen);
                            wcsncpy_s(pwszExtractedFunctionName, functionNameLen, pwszNameStart + 1, _TRUNCATE);
                        }
                        pwszFunctionName = pwszExtractedFunctionName;
                    }

                    functionEnterEventSent = true;
                    Assert(pwszFunctionName != NULL);
                    hrOfEnterEvent = scriptContext->OnDispatchFunctionEnter(pwszFunctionName);
                }
            }
            else
            {
                hrOfEnterEvent = scriptContext->OnFunctionEnter(scriptId, functionId);
            }

            scriptContext->GetThreadContext()->SetIsProfilingUserCode(isUserCode); // Update IsProfilingUserCode state
        }
#endif // ENABLE_SCRIPT_PROFILING

        Var aReturn = NULL;
        JavascriptMethod origEntryPoint = function->GetFunctionInfo()->GetOriginalEntryPoint();

        if (scriptContext->IsEvalRestriction())
        {
            if (origEntryPoint == Js::GlobalObject::EntryEval)
            {
                origEntryPoint = Js::GlobalObject::EntryEvalRestrictedMode;
            }
            else if (origEntryPoint == Js::JavascriptFunction::NewInstance)
            {
                origEntryPoint = Js::JavascriptFunction::NewInstanceRestrictedMode;
            }
            else if (origEntryPoint == Js::JavascriptGeneratorFunction::NewInstance)
            {
                origEntryPoint = Js::JavascriptGeneratorFunction::NewInstanceRestrictedMode;
            }
            else if (origEntryPoint == Js::JavascriptFunction::NewAsyncFunctionInstance)
            {
                origEntryPoint = Js::JavascriptFunction::NewAsyncFunctionInstanceRestrictedMode;
            }
        }

        __TRY_FINALLY_BEGIN // SEH is not guaranteed, see the implementation
        {
            Assert(!function->IsScriptFunction() || function->GetFunctionProxy());

            // No need to wrap script functions, also can't if the wrapper is already on the stack.
            // Treat "library code" script functions, such as Intl, as built-ins:
            // use the wrapper when calling them, and do not reset the wrapper when calling them.
            bool isDebugWrapperEnabled = scriptContext->IsScriptContextInDebugMode() && IsExceptionWrapperForBuiltInsEnabled(scriptContext);
            bool useDebugWrapper =
                isDebugWrapperEnabled &&
                function->IsLibraryCode() &&
                !AutoRegisterIgnoreExceptionWrapper::IsRegistered(scriptContext->GetThreadContext());

            OUTPUT_VERBOSE_TRACE(Js::DebuggerPhase, _u("DebugProfileProbeThunk: calling function: %s isWrapperRegistered=%d useDebugWrapper=%d\n"),
                function->GetFunctionInfo()->HasBody() ? function->GetFunctionBody()->GetDisplayName() : _u("built-in/library"), AutoRegisterIgnoreExceptionWrapper::IsRegistered(scriptContext->GetThreadContext()), useDebugWrapper);

            if (scriptContext->IsDebuggerRecording())
            {
                scriptContext->GetDebugContext()->GetProbeContainer()->StartRecordingCall();
            }

            if (useDebugWrapper)
            {
                // For native use wrapper and bail out on to ignore exception.
                // Extract try-catch out of hot path in normal profile mode (presence of try-catch in a function is bad for perf).
                aReturn = ProfileModeThunk_DebugModeWrapper(function, scriptContext, origEntryPoint, args);
            }
            else
            {
                if (isDebugWrapperEnabled && !function->IsLibraryCode())
                {
                    // We want to ignore exception and continue into closest user/script function down on the stack.
                    // Thus, if needed, reset the wrapper for the time of this call,
                    // so that if there is library/helper call after script function, it will use try-catch.
                    // Can't use smart/destructor object here because of __try__finally.
                    ThreadContext* threadContext = scriptContext->GetThreadContext();
                    bool isOrigWrapperPresent = threadContext->GetDebugManager()->GetDebuggingFlags()->IsBuiltInWrapperPresent();
                    if (isOrigWrapperPresent)
                    {
                        threadContext->GetDebugManager()->GetDebuggingFlags()->SetIsBuiltInWrapperPresent(false);
                    }
                    __TRY_FINALLY_BEGIN // SEH is not guaranteed, see the implementation
                    {
                        aReturn = JavascriptFunction::CallFunction<true>(function, origEntryPoint, args);
                    }
                    __FINALLY
                    {
                        threadContext->GetDebugManager()->GetDebuggingFlags()->SetIsBuiltInWrapperPresent(isOrigWrapperPresent);
                    }
                    __TRY_FINALLY_END
                }
                else
                {
                    // Can we update return address to a thunk that sends Exit event and then jmp to entry instead of Calling it.
                    // Saves stack space and it might be something we would be doing anyway for handling profile.Start/stop
                    // which can come anywhere on the stack.
                    aReturn = JavascriptFunction::CallFunction<true>(function, origEntryPoint, args);
                }
            }
        }
        __FINALLY
        {
#if defined(ENABLE_SCRIPT_PROFILING)
            if (fProfile)
            {
                if (hrOfEnterEvent != ACTIVPROF_E_PROFILER_ABSENT)
                {
                    if (functionId == -1)
                    {
                        // Check whether we have sent the Enter event or not.
                        if (functionEnterEventSent)
                        {
                            scriptContext->OnDispatchFunctionExit(pwszFunctionName);
                            if (pwszExtractedFunctionName != NULL)
                            {
                                HeapDeleteArray(functionNameLen, pwszExtractedFunctionName);
                            }
                        }
                    }
                    else
                    {
                        scriptContext->OnFunctionExit(scriptId, functionId);
                    }
                }

                scriptContext->GetThreadContext()->SetIsProfilingUserCode(isProfilingUserCode); // Restore IsProfilingUserCode state
            }
#endif

            if (scriptContext->IsDebuggerRecording())
            {
                scriptContext->GetDebugContext()->GetProbeContainer()->EndRecordingCall(aReturn, function);
            }
        }
        __TRY_FINALLY_END

        return aReturn;
#else
        return nullptr;
#endif // defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
    }

#if defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
    // Part of ProfileModeThunk which is called in debug mode (debug or debug & profile).
    Var ScriptContext::ProfileModeThunk_DebugModeWrapper(JavascriptFunction* function, ScriptContext* scriptContext, JavascriptMethod entryPoint, Arguments& args)
    {
        AutoRegisterIgnoreExceptionWrapper autoWrapper(scriptContext->GetThreadContext());

        Var aReturn = HelperOrLibraryMethodWrapper<true>(scriptContext, [=] {
            return JavascriptFunction::CallFunction<true>(function, entryPoint, args);
        });

        return aReturn;
    }
#endif

#ifdef ENABLE_SCRIPT_PROFILING
    HRESULT ScriptContext::OnScriptCompiled(PROFILER_TOKEN scriptId, PROFILER_SCRIPT_TYPE type, IUnknown *pIDebugDocumentContext)
    {
        // TODO : can we do a delay send of these events or can we send an event before doing all this stuff that could calculate overhead?
        Assert(m_pProfileCallback != NULL);

        OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::OnScriptCompiled scriptId : %d, ScriptType : %d\n"), scriptId, type);

        HRESULT hr = S_OK;

        if ((type == PROFILER_SCRIPT_TYPE_NATIVE && m_fTraceNativeFunctionCall) ||
            (type != PROFILER_SCRIPT_TYPE_NATIVE && m_fTraceFunctionCall))
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback->ScriptCompiled(scriptId, type, pIDebugDocumentContext);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }

    HRESULT ScriptContext::OnFunctionCompiled(
        PROFILER_TOKEN functionId,
        PROFILER_TOKEN scriptId,
        const WCHAR *pwszFunctionName,
        const WCHAR *pwszFunctionNameHint,
        IUnknown *pIDebugDocumentContext)
    {
        Assert(m_pProfileCallback != NULL);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (scriptId != BuiltInFunctionsScriptId || Js::Configuration::Global.flags.Verbose)
        {
            OUTPUT_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::OnFunctionCompiled scriptId : %d, functionId : %d, FunctionName : %s, FunctionNameHint : %s\n"), scriptId, functionId, pwszFunctionName, pwszFunctionNameHint);
        }
#endif

        HRESULT hr = S_OK;

        if ((scriptId == BuiltInFunctionsScriptId && m_fTraceNativeFunctionCall) ||
            (scriptId != BuiltInFunctionsScriptId && m_fTraceFunctionCall))
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback->FunctionCompiled(functionId, scriptId, pwszFunctionName, pwszFunctionNameHint, pIDebugDocumentContext);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }

    HRESULT ScriptContext::OnFunctionEnter(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId)
    {
        if (m_pProfileCallback == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        OUTPUT_VERBOSE_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::OnFunctionEnter scriptId : %d, functionId : %d\n"), scriptId, functionId);

        HRESULT hr = S_OK;

        if ((scriptId == BuiltInFunctionsScriptId && m_fTraceNativeFunctionCall) ||
            (scriptId != BuiltInFunctionsScriptId && m_fTraceFunctionCall))
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback->OnFunctionEnter(scriptId, functionId);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }

    HRESULT ScriptContext::OnFunctionExit(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId)
    {
        if (m_pProfileCallback == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        OUTPUT_VERBOSE_TRACE(Js::ScriptProfilerPhase, _u("ScriptContext::OnFunctionExit scriptId : %d, functionId : %d\n"), scriptId, functionId);

        HRESULT hr = S_OK;

        if ((scriptId == BuiltInFunctionsScriptId && m_fTraceNativeFunctionCall) ||
            (scriptId != BuiltInFunctionsScriptId && m_fTraceFunctionCall))
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback->OnFunctionExit(scriptId, functionId);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }

    HRESULT ScriptContext::FunctionExitSenderThunk(PROFILER_TOKEN functionId, PROFILER_TOKEN scriptId, ScriptContext *pScriptContext)
    {
        return pScriptContext->OnFunctionExit(scriptId, functionId);
    }

    HRESULT ScriptContext::FunctionExitByNameSenderThunk(const char16 *pwszFunctionName, ScriptContext *pScriptContext)
    {
        return pScriptContext->OnDispatchFunctionExit(pwszFunctionName);
    }
#endif // ENABLE_SCRIPT_PROFILING

    Js::PropertyId ScriptContext::GetFunctionNumber(JavascriptMethod entryPoint)
    {
        return (m_pBuiltinFunctionIdMap == NULL) ? -1 : m_pBuiltinFunctionIdMap->Lookup(entryPoint, -1);
    }

#if defined(ENABLE_SCRIPT_PROFILING)
    HRESULT ScriptContext::RegisterLibraryFunction(const char16 *pwszObjectName, const char16 *pwszFunctionName, Js::PropertyId functionPropertyId, JavascriptMethod entryPoint)
    {
#if DEBUG
        const char16 *pwszObjectNameFromProperty = const_cast<char16 *>(GetPropertyName(functionPropertyId)->GetBuffer());
        if (GetPropertyName(functionPropertyId)->IsSymbol())
        {
            // The spec names functions whose property is a well known symbol as the description from the symbol
            // wrapped in square brackets, so verify by skipping past first bracket
            Assert(!wcsncmp(pwszFunctionName + 1, pwszObjectNameFromProperty, wcslen(pwszObjectNameFromProperty)));
            Assert(wcslen(pwszFunctionName) == wcslen(pwszObjectNameFromProperty) + 2);
        }
        else
        {
            Assert(!wcscmp(pwszFunctionName, pwszObjectNameFromProperty));
        }
        Assert(m_pBuiltinFunctionIdMap != NULL);
#endif

        // Create the propertyId as object.functionName if it is not global function
        // the global functions would be recognized by just functionName
        // e.g. with functionName, toString, depending on objectName, it could be Object.toString, or Date.toString
        char16 szTempName[70];
        if (pwszObjectName != NULL)
        {
            // Create name as "object.function"
            swprintf_s(szTempName, 70, _u("%s.%s"), pwszObjectName, pwszFunctionName);
            functionPropertyId = GetOrAddPropertyIdTracked(szTempName, (uint)wcslen(szTempName));
        }

        Js::PropertyId cachedFunctionId;
        bool keyFound = m_pBuiltinFunctionIdMap->TryGetValue(entryPoint, &cachedFunctionId);

        if (keyFound)
        {
            // Entry point is already in the map
            if (cachedFunctionId != functionPropertyId)
            {
                // This is the scenario where we could be using same function for multiple builtin functions
                // e.g. Error.toString, WinRTError.toString etc.
                // We would ignore these extra entrypoints because while profiling, identifying which object's toString is too costly for its worth
                return S_OK;
            }

            // else is the scenario where map was created by earlier profiling session and we are yet to send function compiled for this session
        }
        else
        {
#if DBG
            m_pBuiltinFunctionIdMap->MapUntil([&](JavascriptMethod, Js::PropertyId propertyId) -> bool
            {
                if (functionPropertyId == propertyId)
                {
                    Assert(false);
                    return true;
                }
                return false;
            });
#endif

            // throws, this must always be in a function that handles OOM
            m_pBuiltinFunctionIdMap->Add(entryPoint, functionPropertyId);
        }

        // Use name with "Object." if its not a global function
        if (pwszObjectName != NULL)
        {
            return OnFunctionCompiled(functionPropertyId, BuiltInFunctionsScriptId, szTempName, NULL, NULL);
        }
        else
        {
            return OnFunctionCompiled(functionPropertyId, BuiltInFunctionsScriptId, pwszFunctionName, NULL, NULL);
        }
    }
#endif // ENABLE_SCRIPT_PROFILING

    void ScriptContext::BindReference(void * addr)
    {
        Assert(!this->isClosed);
        Assert(this->guestArena);
        Assert(recycler->IsValidObject(addr));
#if DBG
        Assert(!bindRef.ContainsKey(addr));     // Make sure we don't bind the same pointer twice
        bindRef.AddNew(addr);
#endif
        javascriptLibrary->BindReference(addr);

#ifdef RECYCLER_PERF_COUNTERS
        this->bindReferenceCount++;
        RECYCLER_PERF_COUNTER_INC(BindReference);
#endif
    }

#ifdef PROFILE_STRINGS
    StringProfiler* ScriptContext::GetStringProfiler()
    {
        return stringProfiler;
    }
#endif

    void ScriptContext::FreeFunctionEntryPoint(Js::JavascriptMethod codeAddress, Js::JavascriptMethod thunkAddress)
    {
#if ENABLE_NATIVE_CODEGEN
        FreeNativeCodeGenAllocation(this, codeAddress, thunkAddress);
#endif
    }

    void ScriptContext::RegisterProtoInlineCache(InlineCache *pCache, PropertyId propId)
    {
        hasProtoOrStoreFieldInlineCache = true;
#if DBG
        this->inlineCacheAllocator.Unlock();
#endif
        threadContext->RegisterProtoInlineCache(pCache, propId);
    }

    void ScriptContext::InvalidateProtoCaches(const PropertyId propertyId)
    {
        threadContext->InvalidateProtoInlineCaches(propertyId);
        // Because setter inline caches get registered in the store field chain, we must invalidate that
        // chain whenever we invalidate the proto chain.
        threadContext->InvalidateStoreFieldInlineCaches(propertyId);
#if ENABLE_NATIVE_CODEGEN
        threadContext->InvalidatePropertyGuards(propertyId);
#endif
        threadContext->InvalidateProtoTypePropertyCaches(propertyId);
    }

    void ScriptContext::InvalidateAllProtoCaches()
    {
        threadContext->InvalidateAllProtoInlineCaches();
        // Because setter inline caches get registered in the store field chain, we must invalidate that
        // chain whenever we invalidate the proto chain.
        threadContext->InvalidateAllStoreFieldInlineCaches();
#if ENABLE_NATIVE_CODEGEN
        threadContext->InvalidateAllPropertyGuards();
#endif
        threadContext->InvalidateAllProtoTypePropertyCaches();
    }

    void ScriptContext::RegisterStoreFieldInlineCache(InlineCache *pCache, PropertyId propId)
    {
        hasProtoOrStoreFieldInlineCache = true;
#if DBG
        this->inlineCacheAllocator.Unlock();
#endif
        threadContext->RegisterStoreFieldInlineCache(pCache, propId);
    }

    void ScriptContext::InvalidateStoreFieldCaches(const PropertyId propertyId)
    {
        threadContext->InvalidateStoreFieldInlineCaches(propertyId);
#if ENABLE_NATIVE_CODEGEN
        threadContext->InvalidatePropertyGuards(propertyId);
#endif
    }

    void ScriptContext::InvalidateAllStoreFieldCaches()
    {
        threadContext->InvalidateAllStoreFieldInlineCaches();
    }

    void ScriptContext::RegisterIsInstInlineCache(Js::IsInstInlineCache * cache, Js::Var function)
    {
        Assert(JavascriptFunction::FromVar(function)->GetScriptContext() == this);
        hasIsInstInlineCache = true;
#if DBG
        this->isInstInlineCacheAllocator.Unlock();
#endif
        threadContext->RegisterIsInstInlineCache(cache, function);
    }

#if DBG
    bool ScriptContext::IsIsInstInlineCacheRegistered(Js::IsInstInlineCache * cache, Js::Var function)
    {
        return threadContext->IsIsInstInlineCacheRegistered(cache, function);
    }
#endif

    void ScriptContext::CleanSourceListInternal(bool calledDuringMark)
    {
        bool fCleanupDocRequired = false;
        for (int i = 0; i < sourceList->Count(); i++)
        {
            if (this->sourceList->IsItemValid(i))
            {
                RecyclerWeakReference<Utf8SourceInfo>* sourceInfoWeakRef = this->sourceList->Item(i);
                Utf8SourceInfo* strongRef = nullptr;

                if (calledDuringMark)
                {
                    strongRef = sourceInfoWeakRef->FastGet();
                }
                else
                {
                    strongRef = sourceInfoWeakRef->Get();
                }

                if (strongRef == nullptr)
                {
                    this->sourceList->RemoveAt(i);
                    fCleanupDocRequired = true;
                }
            }
        }

#ifdef ENABLE_SCRIPT_PROFILING
        // If the sourceList got changed, in we need to refresh the nondebug document list in the profiler mode.
        if (fCleanupDocRequired && m_pProfileCallback != NULL)
        {
            Assert(CleanupDocumentContext != NULL);
            CleanupDocumentContext(this);
        }
#endif // ENABLE_SCRIPT_PROFILING
    }

    void ScriptContext::ClearScriptContextCaches()
    {
        // Prevent reentrancy for the following work, which is not required to be done on every call to this function including
        // reentrant calls
        if (this->isPerformingNonreentrantWork || !this->hasUsedInlineCache)
        {
            return;
        }

        class AutoCleanup
        {
        private:
            ScriptContext *const scriptContext;

        public:
            AutoCleanup(ScriptContext *const scriptContext) : scriptContext(scriptContext)
            {
                scriptContext->isPerformingNonreentrantWork = true;
            }

            ~AutoCleanup()
            {
                scriptContext->isPerformingNonreentrantWork = false;
            }
        } autoCleanup(this);

        if (this->isScriptContextActuallyClosed)
        {
            return;
        }
        Assert(this->guestArena);

        if (EnableEvalMapCleanup())
        {
            // The eval map is not re-entrant, so make sure it's not in the middle of adding an entry
            // Also, don't clean the eval map if the debugger is attached
            if (!this->IsScriptContextInDebugMode())
            {
                if (this->Cache()->evalCacheDictionary != nullptr)
                {
                    this->CleanDynamicFunctionCache<Js::EvalCacheTopLevelDictionary>(this->Cache()->evalCacheDictionary->GetDictionary());
                }
                if (this->Cache()->indirectEvalCacheDictionary != nullptr)
                {
                    this->CleanDynamicFunctionCache<Js::EvalCacheTopLevelDictionary>(this->Cache()->indirectEvalCacheDictionary->GetDictionary());
                }
                if (this->Cache()->newFunctionCache != nullptr)
                {
                    this->CleanDynamicFunctionCache<Js::NewFunctionCache>(this->Cache()->newFunctionCache);
                }
                if (this->hostScriptContext != nullptr)
                {
                    this->hostScriptContext->CleanDynamicCodeCache();
                }

            }
        }

        if (REGEX_CONFIG_FLAG(DynamicRegexMruListSize) > 0)
        {
            GetDynamicRegexMap()->RemoveRecentlyUnusedItems();
        }

        CleanSourceListInternal(true);
    }

void ScriptContext::ClearInlineCaches()
{
    if (this->hasUsedInlineCache)
    {
        this->inlineCacheAllocator.ZeroAll();
        this->hasUsedInlineCache = false;
        this->hasProtoOrStoreFieldInlineCache = false;
    }

    DebugOnly(this->inlineCacheAllocator.CheckIsAllZero(true));
}

void ScriptContext::ClearIsInstInlineCaches()
{
    if (this->hasIsInstInlineCache)
    {
        isInstInlineCacheAllocator.ZeroAll();
        this->hasIsInstInlineCache = false;
    }

    DebugOnly(isInstInlineCacheAllocator.CheckIsAllZero(true));
}

void ScriptContext::ClearForInCaches()
{
    forInCacheAllocator.ZeroAll();
    DebugOnly(forInCacheAllocator.CheckIsAllZero(false));
}


#ifdef PERSISTENT_INLINE_CACHES
void ScriptContext::ClearInlineCachesWithDeadWeakRefs()
{
    if (this->hasUsedInlineCache)
    {
        this->inlineCacheAllocator.ClearCachesWithDeadWeakRefs(this->recycler);
        Assert(this->inlineCacheAllocator.HasNoDeadWeakRefs(this->recycler));
    }
}
#endif

#if ENABLE_NATIVE_CODEGEN
void ScriptContext::RegisterConstructorCache(Js::PropertyId propertyId, Js::ConstructorCache* cache)
{
    this->threadContext->RegisterConstructorCache(propertyId, cache);
}

JITPageAddrToFuncRangeCache *
ScriptContext::GetJitFuncRangeCache()
{
    return jitFuncRangeCache;
}
#endif

void ScriptContext::RegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext()
{
    Assert(!IsClosed());

    if (registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext == nullptr)
    {
        DoRegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext();
    }
}

    void ScriptContext::DoRegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext()
    {
        Assert(!IsClosed());
        Assert(registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext == nullptr);

        // this call may throw OOM
        registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext = threadContext->RegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext(this);
    }

    void ScriptContext::ClearPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesCaches()
    {
        Assert(registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext != nullptr);
        if (!isFinalized)
        {
            javascriptLibrary->NoPrototypeChainsAreEnsuredToHaveOnlyWritableDataProperties();
        }

        // Caller will unregister the script context from the thread context
        registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext = nullptr;
    }

    JavascriptString * ScriptContext::GetLastNumberToStringRadix10(double value)
    {
        if (value == lastNumberToStringRadix10)
        {
            return Cache()->lastNumberToStringRadix10String;
        }
        return nullptr;
    }

    void
        ScriptContext::SetLastNumberToStringRadix10(double value, JavascriptString * str)
    {
            lastNumberToStringRadix10 = value;
            Cache()->lastNumberToStringRadix10String = str;
    }

    bool ScriptContext::GetLastUtcTimeFromStr(JavascriptString * str, double& dbl)
    {
        Assert(str != nullptr);
        if (str != Cache()->lastUtcTimeFromStrString)
        {
            if (Cache()->lastUtcTimeFromStrString == nullptr
                || !JavascriptString::Equals(str, Cache()->lastUtcTimeFromStrString))
            {
                return false;
            }
        }
        dbl = lastUtcTimeFromStr;
        return true;
    }

    void
    ScriptContext::SetLastUtcTimeFromStr(JavascriptString * str, double value)
    {
            lastUtcTimeFromStr = value;
            Cache()->lastUtcTimeFromStrString = str;
    }

#if ENABLE_NATIVE_CODEGEN
    BOOL ScriptContext::IsNativeAddress(void * codeAddr)
    {
        return this->GetThreadContext()->IsNativeAddress(codeAddr, this);
    }
#endif

#ifdef ENABLE_SCRIPT_PROFILING
    bool ScriptContext::SetDispatchProfile(bool fSet, JavascriptMethod dispatchInvoke)
    {
        if (!fSet)
        {
            this->javascriptLibrary->SetDispatchProfile(false, dispatchInvoke);
            return true;
        }
        else if (m_fTraceDomCall)
        {
            this->javascriptLibrary->SetDispatchProfile(true, dispatchInvoke);
            return true;
        }
        return false;
    }

    HRESULT ScriptContext::OnDispatchFunctionEnter(const WCHAR *pwszFunctionName)
    {
        if (m_pProfileCallback2 == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        HRESULT hr = S_OK;

        if (m_fTraceDomCall)
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback2->OnFunctionEnterByName(pwszFunctionName, PROFILER_SCRIPT_TYPE_DOM);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }

    HRESULT ScriptContext::OnDispatchFunctionExit(const WCHAR *pwszFunctionName)
    {
        if (m_pProfileCallback2 == NULL)
        {
            return ACTIVPROF_E_PROFILER_ABSENT;
        }

        HRESULT hr = S_OK;

        if (m_fTraceDomCall)
        {
            m_inProfileCallback = TRUE;
            hr = m_pProfileCallback2->OnFunctionExitByName(pwszFunctionName, PROFILER_SCRIPT_TYPE_DOM);
            m_inProfileCallback = FALSE;
        }
        return hr;
    }
#endif // ENABLE_SCRIPT_PROFILING

    void ScriptContext::SetBuiltInLibraryFunction(JavascriptMethod entryPoint, JavascriptFunction* function)
    {
        if (!isClosed)
        {
            if (builtInLibraryFunctions == NULL)
            {
                Assert(this->recycler);

                builtInLibraryFunctions = RecyclerNew(this->recycler, BuiltInLibraryFunctionMap, this->recycler);
                Cache()->builtInLibraryFunctions = builtInLibraryFunctions;
            }

            builtInLibraryFunctions->Item(entryPoint, function);
        }
    }

#if ENABLE_NATIVE_CODEGEN
    void ScriptContext::InitializeRemoteScriptContext()
    {
        Assert(JITManager::GetJITManager()->IsOOPJITEnabled());

        if (!JITManager::GetJITManager()->IsConnected())
        {
            return;
        }
        ScriptContextDataIDL contextData;
        contextData.nullAddr = (intptr_t)GetLibrary()->GetNull();
        contextData.undefinedAddr = (intptr_t)GetLibrary()->GetUndefined();
        contextData.trueAddr = (intptr_t)GetLibrary()->GetTrue();
        contextData.falseAddr = (intptr_t)GetLibrary()->GetFalse();
        contextData.undeclBlockVarAddr = (intptr_t)GetLibrary()->GetUndeclBlockVar();
        contextData.scriptContextAddr = (intptr_t)this;
        contextData.emptyStringAddr = (intptr_t)GetLibrary()->GetEmptyString();
        contextData.negativeZeroAddr = (intptr_t)GetLibrary()->GetNegativeZero();
        contextData.numberTypeStaticAddr = (intptr_t)GetLibrary()->GetNumberTypeStatic();
        contextData.stringTypeStaticAddr = (intptr_t)GetLibrary()->GetStringTypeStatic();
        contextData.objectTypeAddr = (intptr_t)GetLibrary()->GetObjectType();
        contextData.objectHeaderInlinedTypeAddr = (intptr_t)GetLibrary()->GetObjectHeaderInlinedType();
        contextData.regexTypeAddr = (intptr_t)GetLibrary()->GetRegexType();
        contextData.arrayConstructorAddr = (intptr_t)GetLibrary()->GetArrayConstructor();
        contextData.arrayTypeAddr = (intptr_t)GetLibrary()->GetArrayType();
        contextData.nativeIntArrayTypeAddr = (intptr_t)GetLibrary()->GetNativeIntArrayType();
        contextData.nativeFloatArrayTypeAddr = (intptr_t)GetLibrary()->GetNativeFloatArrayType();
        contextData.charStringCacheAddr = (intptr_t)&GetLibrary()->GetCharStringCache();
        contextData.libraryAddr = (intptr_t)GetLibrary();
        contextData.globalObjectAddr = (intptr_t)GetLibrary()->GetGlobalObject();
        contextData.builtinFunctionsBaseAddr = (intptr_t)GetLibrary()->GetBuiltinFunctions();
        contextData.sideEffectsAddr = optimizationOverrides.GetAddressOfSideEffects();
        contextData.arraySetElementFastPathVtableAddr = (intptr_t)optimizationOverrides.GetAddressOfArraySetElementFastPathVtable();
        contextData.intArraySetElementFastPathVtableAddr = (intptr_t)optimizationOverrides.GetAddressOfIntArraySetElementFastPathVtable();
        contextData.floatArraySetElementFastPathVtableAddr = (intptr_t)optimizationOverrides.GetAddressOfFloatArraySetElementFastPathVtable();
        contextData.recyclerAddr = (intptr_t)GetRecycler();
        contextData.recyclerAllowNativeCodeBumpAllocation = GetRecycler()->AllowNativeCodeBumpAllocation();
        contextData.numberAllocatorAddr = (intptr_t)GetNumberAllocator();
#ifdef RECYCLER_MEMORY_VERIFY
        contextData.isRecyclerVerifyEnabled = (boolean)recycler->VerifyEnabled();
        contextData.recyclerVerifyPad = recycler->GetVerifyPad();
#else
        // TODO: OOP JIT, figure out how to have this only in debug build
        contextData.isRecyclerVerifyEnabled = FALSE;
        contextData.recyclerVerifyPad = 0;
#endif
#ifdef ENABLE_SCRIPT_DEBUGGING
        contextData.debuggingFlagsAddr = GetDebuggingFlagsAddr();
        contextData.debugStepTypeAddr = GetDebugStepTypeAddr();
        contextData.debugFrameAddressAddr = GetDebugFrameAddressAddr();
        contextData.debugScriptIdWhenSetAddr = GetDebugScriptIdWhenSetAddr();
#endif
        contextData.numberAllocatorAddr = (intptr_t)GetNumberAllocator();
#ifdef ENABLE_SIMDJS
        contextData.isSIMDEnabled = GetConfig()->IsSimdjsEnabled();
#endif
        CompileAssert(VTableValue::Count == VTABLE_COUNT); // need to update idl when this changes

        auto vtblAddresses = GetLibrary()->GetVTableAddresses();
        for (unsigned int i = 0; i < VTableValue::Count; i++)
        {
            contextData.vtableAddresses[i] = vtblAddresses[i];
        }

        bool allowPrereserveAlloc = true;
#if !_M_X64_OR_ARM64
        if (this->webWorkerId != Js::Constants::NonWebWorkerContextId)
        {
            allowPrereserveAlloc = false;
        }
#endif
#ifndef _CONTROL_FLOW_GUARD
        allowPrereserveAlloc = false;
#endif
        // The EnsureJITThreadContext() call could fail if the JIT Server process has died. In such cases, we should not try to do anything further in the client process.
        if (this->GetThreadContext()->EnsureJITThreadContext(allowPrereserveAlloc))
        {
            HRESULT hr = JITManager::GetJITManager()->InitializeScriptContext(&contextData, this->GetThreadContext()->GetRemoteThreadContextAddr(), &m_remoteScriptContextAddr);
            JITManager::HandleServerCallResult(hr, RemoteCallType::StateUpdate);
        }
    }
#endif

    intptr_t ScriptContext::GetNullAddr() const
    {
        return (intptr_t)GetLibrary()->GetNull();
    }

    intptr_t ScriptContext::GetUndefinedAddr() const
    {
        return (intptr_t)GetLibrary()->GetUndefined();
    }

    intptr_t ScriptContext::GetTrueAddr() const
    {
        return (intptr_t)GetLibrary()->GetTrue();
    }

    intptr_t ScriptContext::GetFalseAddr() const
    {
        return (intptr_t)GetLibrary()->GetFalse();
    }

    intptr_t ScriptContext::GetUndeclBlockVarAddr() const
    {
        return (intptr_t)GetLibrary()->GetUndeclBlockVar();
    }

    intptr_t ScriptContext::GetEmptyStringAddr() const
    {
        return (intptr_t)GetLibrary()->GetEmptyString();
    }

    intptr_t ScriptContext::GetNegativeZeroAddr() const
    {
        return (intptr_t)GetLibrary()->GetNegativeZero();
    }

    intptr_t ScriptContext::GetNumberTypeStaticAddr() const
    {
        return (intptr_t)GetLibrary()->GetNumberTypeStatic();
    }

    intptr_t ScriptContext::GetStringTypeStaticAddr() const
    {
        return (intptr_t)GetLibrary()->GetStringTypeStatic();
    }

    intptr_t ScriptContext::GetObjectTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetObjectType();
    }

    intptr_t ScriptContext::GetObjectHeaderInlinedTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetObjectHeaderInlinedType();
    }

    intptr_t ScriptContext::GetRegexTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetRegexType();
    }

    intptr_t ScriptContext::GetArrayTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetArrayType();
    }

    intptr_t ScriptContext::GetNativeIntArrayTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetNativeIntArrayType();
    }

    intptr_t ScriptContext::GetNativeFloatArrayTypeAddr() const
    {
        return (intptr_t)GetLibrary()->GetNativeFloatArrayType();
    }

    intptr_t ScriptContext::GetArrayConstructorAddr() const
    {
        return (intptr_t)GetLibrary()->GetArrayConstructor();
    }

    intptr_t ScriptContext::GetCharStringCacheAddr() const
    {
        return (intptr_t)&GetLibrary()->GetCharStringCache();
    }

    intptr_t ScriptContext::GetSideEffectsAddr() const
    {
        return optimizationOverrides.GetAddressOfSideEffects();
    }

    intptr_t ScriptContext::GetArraySetElementFastPathVtableAddr() const
    {
        return optimizationOverrides.GetArraySetElementFastPathVtableAddr();
    }

    intptr_t ScriptContext::GetIntArraySetElementFastPathVtableAddr() const
    {
        return optimizationOverrides.GetIntArraySetElementFastPathVtableAddr();
    }

    intptr_t ScriptContext::GetFloatArraySetElementFastPathVtableAddr() const
    {
        return optimizationOverrides.GetFloatArraySetElementFastPathVtableAddr();
    }

    intptr_t ScriptContext::GetBuiltinFunctionsBaseAddr() const
    {
        return (intptr_t)GetLibrary()->GetBuiltinFunctions();
    }

    intptr_t ScriptContext::GetLibraryAddr() const
    {
        return (intptr_t)GetLibrary();
    }

    intptr_t ScriptContext::GetGlobalObjectAddr() const
    {
        return (intptr_t)GetLibrary()->GetGlobalObject();
    }

    intptr_t ScriptContext::GetGlobalObjectThisAddr() const
    {
        return (intptr_t)GetLibrary()->GetGlobalObject()->ToThis();
    }

    intptr_t ScriptContext::GetNumberAllocatorAddr() const
    {
        return (intptr_t)&numberAllocator;
    }

    intptr_t ScriptContext::GetRecyclerAddr() const
    {
        return (intptr_t)GetRecycler();
    }

#ifdef ENABLE_SCRIPT_DEBUGGING
    intptr_t ScriptContext::GetDebuggingFlagsAddr() const
    {
        return this->threadContext->GetDebugManager()->GetDebuggingFlagsAddr();
    }

    intptr_t ScriptContext::GetDebugStepTypeAddr() const
    {
        return (intptr_t)this->threadContext->GetDebugManager()->stepController.GetAddressOfStepType();
    }

    intptr_t ScriptContext::GetDebugFrameAddressAddr() const
    {
        return (intptr_t)this->threadContext->GetDebugManager()->stepController.GetAddressOfFrameAddress();
    }

    intptr_t ScriptContext::GetDebugScriptIdWhenSetAddr() const
    {
        return (intptr_t)this->threadContext->GetDebugManager()->stepController.GetAddressOfScriptIdWhenSet();
    }
#endif

    bool ScriptContext::GetRecyclerAllowNativeCodeBumpAllocation() const
    {
        return GetRecycler()->AllowNativeCodeBumpAllocation();
    }

#ifdef ENABLE_SIMDJS
    bool ScriptContext::IsSIMDEnabled() const
    {
        return GetConfig()->IsSimdjsEnabled();
    }
#endif

    bool ScriptContext::IsPRNGSeeded() const
    {
        return GetLibrary()->IsPRNGSeeded();
    }

    intptr_t ScriptContext::GetAddr() const
    {
        return (intptr_t)this;
    }

#if ENABLE_NATIVE_CODEGEN
    void ScriptContext::AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper)
    {
        m_domFastPathHelperMap->Add(funcInfoAddr, helper);
    }

    IR::JnHelperMethod ScriptContext::GetDOMFastPathHelper(intptr_t funcInfoAddr)
    {
        IR::JnHelperMethod helper = IR::HelperInvalid;

        m_domFastPathHelperMap->LockResize();
        m_domFastPathHelperMap->TryGetValue(funcInfoAddr, &helper);
        m_domFastPathHelperMap->UnlockResize();

        return helper;
    }
#endif

    intptr_t ScriptContext::GetVTableAddress(VTableValue vtableType) const
    {
        Assert(vtableType < VTableValue::Count);
        return GetLibrary()->GetVTableAddresses()[vtableType];
    }

    bool ScriptContext::IsRecyclerVerifyEnabled() const
    {
#ifdef RECYCLER_MEMORY_VERIFY
        return recycler->VerifyEnabled() != FALSE;
#else
        return false;
#endif
    }

    uint ScriptContext::GetRecyclerVerifyPad() const
    {
#ifdef RECYCLER_MEMORY_VERIFY
        return recycler->GetVerifyPad();
#else
        return 0;
#endif
    }

    JavascriptFunction* ScriptContext::GetBuiltInLibraryFunction(JavascriptMethod entryPoint)
    {
        JavascriptFunction * function = NULL;
        if (builtInLibraryFunctions)
        {
            builtInLibraryFunctions->TryGetValue(entryPoint, &function);
        }
        return function;
    }

#if ENABLE_PROFILE_INFO
    template<template<typename> class BarrierT>
    void ScriptContext::AddDynamicProfileInfo(
        FunctionBody * functionBody, BarrierT<DynamicProfileInfo>& dynamicProfileInfo)
    {
        Assert(functionBody->GetScriptContext() == this);
        Assert(functionBody->HasValidSourceInfo());

        DynamicProfileInfo * newDynamicProfileInfo = dynamicProfileInfo;
        // If it is a dynamic script - we should create a profile info bound to the threadContext for its lifetime.
        SourceContextInfo* sourceContextInfo = functionBody->GetSourceContextInfo();
        SourceDynamicProfileManager* profileManager = sourceContextInfo->sourceDynamicProfileManager;
        if (sourceContextInfo->IsDynamic())
        {
            if (profileManager != nullptr)
            {
                // There is an in-memory cache and dynamic profile info is coming from there
                if (newDynamicProfileInfo == nullptr)
                {
                    newDynamicProfileInfo = DynamicProfileInfo::New(this->GetRecycler(), functionBody, true /* persistsAcrossScriptContexts */);
                    profileManager->UpdateDynamicProfileInfo(functionBody->GetLocalFunctionId(), newDynamicProfileInfo);
                    dynamicProfileInfo = newDynamicProfileInfo;
                }
                profileManager->MarkAsExecuted(functionBody->GetLocalFunctionId());
                newDynamicProfileInfo->UpdateFunctionInfo(functionBody, this->GetRecycler());
            }
            else
            {
                if (newDynamicProfileInfo == nullptr)
                {
                    newDynamicProfileInfo = functionBody->AllocateDynamicProfile();
                }
                dynamicProfileInfo = newDynamicProfileInfo;
            }
        }
        else
        {
            if (newDynamicProfileInfo == nullptr)
            {
                newDynamicProfileInfo = functionBody->AllocateDynamicProfile();
                dynamicProfileInfo = newDynamicProfileInfo;
            }
            Assert(functionBody->GetInterpretedCount() == 0);
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)

            if (this->Cache()->profileInfoList)
            {
                this->Cache()->profileInfoList->Prepend(this->GetRecycler(), newDynamicProfileInfo);
            }
#endif
            if (!startupComplete)
            {
                Assert(profileManager);
                profileManager->MarkAsExecuted(functionBody->GetLocalFunctionId());
            }
        }
        Assert(dynamicProfileInfo != nullptr);
    }
    template void  ScriptContext::AddDynamicProfileInfo<WriteBarrierPtr>(FunctionBody *, WriteBarrierPtr<DynamicProfileInfo>&);
#endif

    CharClassifier const * ScriptContext::GetCharClassifier(void) const
    {
        return this->charClassifier;
    }

    void ScriptContext::OnStartupComplete()
    {
        JS_ETW(EventWriteJSCRIPT_ON_STARTUP_COMPLETE(this));

        SaveStartupProfileAndRelease();
    }

    void ScriptContext::SaveStartupProfileAndRelease(bool isSaveOnClose)
    {
        // No need to save profiler info in JSRT scenario at this time.
        if (GetThreadContext()->IsJSRT())
        {
            return;
        }
        if (!startupComplete && this->Cache()->sourceContextInfoMap)
        {
#if ENABLE_PROFILE_INFO
            this->Cache()->sourceContextInfoMap->Map([&](DWORD_PTR dwHostSourceContext, SourceContextInfo* info)
            {
                Assert(info->sourceDynamicProfileManager);
                uint bytesWritten = info->sourceDynamicProfileManager->SaveToProfileCacheAndRelease(info);
                if (bytesWritten > 0)
                {
                    JS_ETW(EventWriteJSCRIPT_PROFILE_SAVE(info->dwHostSourceContext, this, bytesWritten, isSaveOnClose));
                    OUTPUT_TRACE(Js::DynamicProfilePhase, _u("Profile saving succeeded\n"));
                }
            });
#endif
        }
        startupComplete = true;
    }

    void ScriptContext::SetFastDOMenabled()
    {
        fastDOMenabled = true; Assert(globalObject->GetDirectHostObject() != NULL);
    }

#if DYNAMIC_INTERPRETER_THUNK
    JavascriptMethod ScriptContext::GetNextDynamicAsmJsInterpreterThunk(PVOID* ppDynamicInterpreterThunk)
    {
#ifdef ASMJS_PLAT
        return (JavascriptMethod)this->asmJsInterpreterThunkEmitter->GetNextThunk(ppDynamicInterpreterThunk);
#else
        __debugbreak();
        return nullptr;
#endif
    }

    JavascriptMethod ScriptContext::GetNextDynamicInterpreterThunk(PVOID* ppDynamicInterpreterThunk)
    {
        return (JavascriptMethod)this->interpreterThunkEmitter->GetNextThunk(ppDynamicInterpreterThunk);
    }

#if DBG
    BOOL ScriptContext::IsDynamicInterpreterThunk(JavascriptMethod address)
    {
        return this->interpreterThunkEmitter->IsInHeap((void*)address);
    }
#endif

    void ScriptContext::ReleaseDynamicInterpreterThunk(BYTE* address, bool addtoFreeList)
    {
        this->interpreterThunkEmitter->Release(address, addtoFreeList);
    }

    void ScriptContext::ReleaseDynamicAsmJsInterpreterThunk(BYTE* address, bool addtoFreeList)
    {
#ifdef ASMJS_PLAT
        this->asmJsInterpreterThunkEmitter->Release(address, addtoFreeList);
#else
        Assert(UNREACHED);
#endif
    }
#endif

    bool ScriptContext::IsExceptionWrapperForBuiltInsEnabled()
    {
        return ScriptContext::IsExceptionWrapperForBuiltInsEnabled(this);
    }

    // static
    bool ScriptContext::IsExceptionWrapperForBuiltInsEnabled(ScriptContext* scriptContext)
    {
        Assert(scriptContext);
        return CONFIG_FLAG(EnableContinueAfterExceptionWrappersForBuiltIns);
    }

    bool ScriptContext::IsExceptionWrapperForHelpersEnabled(ScriptContext* scriptContext)
    {
        Assert(scriptContext);
        return  CONFIG_FLAG(EnableContinueAfterExceptionWrappersForHelpers);
    }

    void ScriptContextBase::SetGlobalObject(GlobalObject *globalObject)
    {
#if DBG
        ScriptContext* scriptContext = static_cast<ScriptContext*>(this);
        Assert(scriptContext->IsCloningGlobal() && !this->globalObject);
#endif
        this->globalObject = globalObject;
    }

    void ConvertKey(const FastEvalMapString& src, EvalMapString& dest)
    {
        dest.str = src.str;
        dest.strict = src.strict;
        dest.moduleID = src.moduleID;
        dest.hash = TAGHASH((hash_t)dest.str);
    }

    void ScriptContext::PrintStats()
    {

#ifdef PROFILE_TYPES
        if (Configuration::Global.flags.ProfileTypes)
        {
            ProfileTypes();
        }
#endif

#ifdef PROFILE_BAILOUT_RECORD_MEMORY
        if (Configuration::Global.flags.ProfileBailOutRecordMemory)
        {
            Output::Print(_u("CodeSize: %6d\nBailOutRecord Size: %6d\nLocalOffsets Size: %6d\n"), codeSize, bailOutRecordBytes, bailOutOffsetBytes);
        }
#endif

#ifdef PROFILE_OBJECT_LITERALS
        if (Configuration::Global.flags.ProfileObjectLiteral)
        {
            ProfileObjectLiteral();
        }
#endif

#ifdef PROFILE_STRINGS
        if (stringProfiler != nullptr)
        {
            stringProfiler->PrintAll();
            Adelete(MiscAllocator(), stringProfiler);
            stringProfiler = nullptr;
        }
#endif

#ifdef PROFILE_MEM
        if (profileMemoryDump && MemoryProfiler::IsTraceEnabled())
        {
            MemoryProfiler::PrintAll();
#ifdef PROFILE_RECYCLER_ALLOC
            if (Js::Configuration::Global.flags.TraceMemory.IsEnabled(Js::AllPhase)
                || Js::Configuration::Global.flags.TraceMemory.IsEnabled(Js::RunPhase))
            {
                GetRecycler()->PrintAllocStats();
            }
#endif
        }
#endif
#if DBG_DUMP
        if (PHASE_STATS1(Js::ByteCodePhase))
        {
            Output::Print(_u(" Total Bytecode size: <%d, %d, %d> = %d\n"),
                byteCodeDataSize,
                byteCodeAuxiliaryDataSize,
                byteCodeAuxiliaryContextDataSize,
                byteCodeDataSize + byteCodeAuxiliaryDataSize + byteCodeAuxiliaryContextDataSize);
        }

        if (Configuration::Global.flags.BytecodeHist)
        {
            Output::Print(_u("ByteCode Histogram\n"));
            Output::Print(_u("\n"));

            uint total = 0;
            uint unique = 0;
            for (int j = 0; j < (int)OpCode::ByteCodeLast; j++)
            {
                total += byteCodeHistogram[j];
                if (byteCodeHistogram[j] > 0)
                {
                    unique++;
                }
            }
            Output::Print(_u("%9u                     Total executed ops\n"), total);
            Output::Print(_u("\n"));

            uint max = UINT_MAX;
            double pctcume = 0.0;

            while (true)
            {
                uint upper = 0;
                int index = -1;
                for (int j = 0; j < (int)OpCode::ByteCodeLast; j++)
                {
                    if (OpCodeUtil::IsValidOpcode((OpCode)j) && byteCodeHistogram[j] > upper && byteCodeHistogram[j] < max)
                    {
                        index = j;
                        upper = byteCodeHistogram[j];
                    }
                }

                if (index == -1)
                {
                    break;
                }

                max = byteCodeHistogram[index];

                for (OpCode j = (OpCode)0; j < OpCode::ByteCodeLast; j++)
                {
                    if (OpCodeUtil::IsValidOpcode(j) && max == byteCodeHistogram[(int)j])
                    {
                        double pct = ((double)max) / total;
                        pctcume += pct;

                        Output::Print(_u("%9u  %5.1lf  %5.1lf  %04x %s\n"), max, pct * 100, pctcume * 100, j, OpCodeUtil::GetOpCodeName(j));
                    }
                }
            }
            Output::Print(_u("\n"));
            Output::Print(_u("Unique opcodes: %d\n"), unique);
        }

#endif

#if ENABLE_NATIVE_CODEGEN
#ifdef BGJIT_STATS
        // We do not care about small script contexts without much activity - unless t
        if (PHASE_STATS1(Js::BGJitPhase) && (this->interpretedCount > 50 || Js::Configuration::Global.flags.IsEnabled(Js::ForceFlag)))
        {

#define MAX_BUCKETS 15
            uint loopJitCodeUsed = 0;
            uint bucketSize1 = 20;
            uint bucketSize2 = 100;
            uint size1CutOffbucketId = 4;
            uint totalBuckets[MAX_BUCKETS] = { 0 };
            uint nativeCodeBuckets[MAX_BUCKETS] = { 0 };
            uint usedNativeCodeBuckets[MAX_BUCKETS] = { 0 };
            uint rejits[MAX_BUCKETS] = { 0 };
            uint zeroInterpretedFunctions = 0;
            uint oneInterpretedFunctions = 0;
            uint nonZeroBytecodeFunctions = 0;
            Output::Print(_u("Script Context: 0x%p Url: %s\n"), this, this->url);

            FunctionBody* anyFunctionBody = this->FindFunction([](FunctionBody* body) { return body != nullptr; });

            if (anyFunctionBody)
            {
                OUTPUT_VERBOSE_STATS(Js::BGJitPhase, _u("Function list\n"));
                OUTPUT_VERBOSE_STATS(Js::BGJitPhase, _u("===============================\n"));
                OUTPUT_VERBOSE_STATS(Js::BGJitPhase, _u("%-24s, %-8s, %-10s, %-10s, %-10s, %-10s, %-10s\n"), _u("Function"), _u("InterpretedCount"), _u("ByteCodeInLoopSize"), _u("ByteCodeSize"), _u("IsJitted"), _u("IsUsed"), _u("NativeCodeSize"));

                this->MapFunction([&](FunctionBody* body)
                {
                    bool isNativeCode = false;

                    // Filtering interpreted count lowers a lot of noise
                    if (body->GetInterpretedCount() > 1 || Js::Configuration::Global.flags.IsEnabled(Js::ForceFlag))
                    {
                        body->MapEntryPoints([&](uint entryPointIndex, FunctionEntryPointInfo* entryPoint)
                        {
                            char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                            char rejit = entryPointIndex > 0 ? '*' : ' ';
                            isNativeCode = entryPoint->IsNativeCode() | isNativeCode;
                            OUTPUT_VERBOSE_STATS(Js::BGJitPhase, _u("%-20s %16s %c, %8d , %10d , %10d, %-10s, %-10s, %10d\n"),
                                body->GetExternalDisplayName(),
                                body->GetDebugNumberSet(debugStringBuffer),
                                rejit,
                                body->GetInterpretedCount(),
                                body->GetByteCodeInLoopCount(),
                                body->GetByteCodeCount(),
                                entryPoint->IsNativeCode() ? _u("Jitted") : _u("Interpreted"),
                                body->GetNativeEntryPointUsed() ? _u("Used") : _u("NotUsed"),
                                entryPoint->IsNativeCode() ? entryPoint->GetCodeSize() : 0);
                        });
                    }
                    if (body->GetInterpretedCount() == 0)
                    {
                        zeroInterpretedFunctions++;
                        if (body->GetByteCodeCount() > 0)
                        {
                            nonZeroBytecodeFunctions++;
                        }
                    }
                    else if (body->GetInterpretedCount() == 1)
                    {
                        oneInterpretedFunctions++;
                    }


                    // Generate a histogram using interpreted counts.
                    uint bucket;
                    uint intrpCount = body->GetInterpretedCount();
                    if (intrpCount < 100)
                    {
                        bucket = intrpCount / bucketSize1;
                    }
                    else if (intrpCount < 1000)
                    {
                        bucket = size1CutOffbucketId  + intrpCount / bucketSize2;
                    }
                    else
                    {
                        bucket = MAX_BUCKETS - 1;
                    }

                    // Explicitly assume that the bucket count is less than the following counts (which are all equal)
                    // This is because min will return MAX_BUCKETS - 1 if the count exceeds MAX_BUCKETS - 1.
                    __analysis_assume(bucket < MAX_BUCKETS);

                    totalBuckets[bucket]++;
                    if (isNativeCode)
                    {
                        nativeCodeBuckets[bucket]++;
                        if (body->GetNativeEntryPointUsed())
                        {
                            usedNativeCodeBuckets[bucket]++;
                        }
                        if (body->HasRejit())
                        {
                            rejits[bucket]++;
                        }
                    }

                    body->MapLoopHeaders([&](uint loopNumber, LoopHeader* header)
                    {
                        char16 loopBodyName[256];
                        body->GetLoopBodyName(loopNumber, loopBodyName, _countof(loopBodyName));
                        header->MapEntryPoints([&](int index, LoopEntryPointInfo * entryPoint)
                        {
                            if (entryPoint->IsNativeCode())
                            {
                                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                                char rejit = index > 0 ? '*' : ' ';
                                OUTPUT_VERBOSE_STATS(Js::BGJitPhase, _u("%-20s %16s %c, %8d , %10d , %10d, %-10s, %-10s, %10d\n"),
                                    loopBodyName,
                                    body->GetDebugNumberSet(debugStringBuffer),
                                    rejit,
                                    header->interpretCount,
                                    header->GetByteCodeCount(),
                                    header->GetByteCodeCount(),
                                    _u("Jitted"),
                                    entryPoint->IsUsed() ? _u("Used") : _u("NotUsed"),
                                    entryPoint->GetCodeSize());
                                if (entryPoint->IsUsed())
                                {
                                    loopJitCodeUsed++;
                                }
                            }
                        });
                    });
                });
            }

            Output::Print(_u("**  SpeculativelyJitted: %6d FunctionsJitted: %6d JittedUsed: %6d Usage:%f ByteCodesJitted: %6d JitCodeUsed: %6d Usage: %f \n"),
                speculativeJitCount, funcJITCount, funcJitCodeUsed, ((float)(funcJitCodeUsed) / funcJITCount) * 100, bytecodeJITCount, jitCodeUsed, ((float)(jitCodeUsed) / bytecodeJITCount) * 100);
            Output::Print(_u("** LoopJITCount: %6d LoopJitCodeUsed: %6d Usage: %f\n"),
                loopJITCount, loopJitCodeUsed, ((float)loopJitCodeUsed / loopJITCount) * 100);
            Output::Print(_u("** TotalInterpretedCalls: %6d MaxFuncInterp: %6d  InterpretedHighPri: %6d \n"),
                interpretedCount, maxFuncInterpret, interpretedCallsHighPri);
            Output::Print(_u("** ZeroInterpretedFunctions: %6d OneInterpretedFunctions: %6d ZeroInterpretedWithNonZeroBytecode: %6d \n "), zeroInterpretedFunctions, oneInterpretedFunctions, nonZeroBytecodeFunctions);
            Output::Print(_u("** %-24s : %-10s %-10s %-10s %-10s %-10s\n"), _u("InterpretedCounts"), _u("Total"), _u("NativeCode"), _u("Used"), _u("Usage"), _u("Rejits"));
            uint low = 0;
            uint high = 0;
            for (uint i = 0; i < _countof(totalBuckets); i++)
            {
                low = high;
                if (i <= size1CutOffbucketId)
                {
                    high = low + bucketSize1;
                }
                else if (i < (_countof(totalBuckets) - 1))
                {
                    high = low + bucketSize2;               }
                else
                {
                    high = 100000;
                }
                Output::Print(_u("** %10d - %10d : %10d %10d %10d %7.2f %10d\n"), low, high, totalBuckets[i], nativeCodeBuckets[i], usedNativeCodeBuckets[i], ((float)usedNativeCodeBuckets[i] / nativeCodeBuckets[i]) * 100, rejits[i]);
            }
            Output::Print(_u("\n\n"));
        }
#undef MAX_BUCKETS
#endif

#ifdef REJIT_STATS
        if (PHASE_STATS1(Js::ReJITPhase))
        {
            uint totalBailouts = 0;
            uint totalRejits = 0;
            WCHAR buf[256];

            // Dump bailout data.
            Output::Print(_u("%-40s %6s\n"), _u("Bailout Reason,"), _u("Count"));

            bailoutReasonCounts->Map([&totalBailouts](uint kind, uint val) {
                WCHAR buf[256];
                totalBailouts += val;
                if (val != 0)
                {
                    swprintf_s(buf, _u("%S,"), GetBailOutKindName((IR::BailOutKind)kind));
                    Output::Print(_u("%-40s %6d\n"), buf, val);
                }
            });


            Output::Print(_u("%-40s %6d\n"), _u("TOTAL,"), totalBailouts);
            Output::Print(_u("\n\n"));

            // Dump rejit data.
            Output::Print(_u("%-40s %6s\n"), _u("Rejit Reason,"), _u("Count"));
            for (uint i = 0; i < NumRejitReasons; ++i)
            {
                totalRejits += rejitReasonCounts[i];
                if (rejitReasonCounts[i] != 0)
                {
                    swprintf_s(buf, _u("%S,"), RejitReasonNames[i]);
                    Output::Print(_u("%-40s %6d\n"), buf, rejitReasonCounts[i]);
                }
            }
            Output::Print(_u("%-40s %6d\n"), _u("TOTAL,"), totalRejits);
            Output::Print(_u("\n\n"));

            // If in verbose mode, dump data for each FunctionBody
            if (
#if defined(FLAG) || defined(FLAG_REGOVR_EXP)
                Configuration::Global.flags.Verbose &&
#endif
                rejitStatsMap != nullptr)
            {
                // Aggregated data
                Output::Print(_u("%-30s %14s %14s\n"), _u("Function (#),"), _u("Bailout Count,"), _u("Rejit Count"));
                rejitStatsMap->Map([](Js::FunctionBody const *body, RejitStats *stats, RecyclerWeakReference<const Js::FunctionBody> const*) {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    for (uint i = 0; i < NumRejitReasons; ++i)
                        stats->m_totalRejits += stats->m_rejitReasonCounts[i];

                    stats->m_bailoutReasonCounts->Map([stats](uint kind, uint val) {
                        stats->m_totalBailouts += val;
                    });

                    WCHAR buf[256];

                    swprintf_s(buf, _u("%s (%s),"), body->GetExternalDisplayName(), (const_cast<Js::FunctionBody*>(body))->GetDebugNumberSet(debugStringBuffer)); //TODO Kount
                    Output::Print(_u("%-30s %14d, %14d\n"), buf, stats->m_totalBailouts, stats->m_totalRejits);

                });
                Output::Print(_u("\n\n"));

                // Per FunctionBody data
                rejitStatsMap->Map([](Js::FunctionBody const *body, RejitStats *stats, RecyclerWeakReference<const Js::FunctionBody> const *) {
                    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
                    WCHAR buf[256];

                    swprintf_s(buf, _u("%s (%s),"), body->GetExternalDisplayName(), (const_cast<Js::FunctionBody*>(body))->GetDebugNumberSet(debugStringBuffer)); //TODO Kount
                    Output::Print(_u("%-30s\n\n"), buf);

                    // Dump bailout data
                    if (stats->m_totalBailouts != 0)
                    {
                        Output::Print(_u("%10sBailouts:\n"), _u(""));

                        stats->m_bailoutReasonCounts->Map([](uint kind, uint val) {
                            if (val != 0)
                            {
                                WCHAR buf[256];
                                swprintf_s(buf, _u("%S,"), GetBailOutKindName((IR::BailOutKind)kind));
                                Output::Print(_u("%10s%-40s %6d\n"), _u(""), buf, val);
                            }
                        });
                    }
                    Output::Print(_u("\n"));

                    // Dump rejit data.
                    if (stats->m_totalRejits != 0)
                    {
                        Output::Print(_u("%10sRejits:\n"), _u(""));
                        for (uint i = 0; i < NumRejitReasons; ++i)
                        {
                            if (stats->m_rejitReasonCounts[i] != 0)
                            {
                                swprintf_s(buf, _u("%S,"), RejitReasonNames[i]);
                                Output::Print(_u("%10s%-40s %6d\n"), _u(""), buf, stats->m_rejitReasonCounts[i]);
                            }
                        }
                        Output::Print(_u("\n\n"));
                    }
                });

            }
        }

        this->ClearBailoutReasonCountsMap();
        this->ClearRejitReasonCountsArray();
#endif

#ifdef FIELD_ACCESS_STATS
    if (PHASE_STATS1(Js::ObjTypeSpecPhase))
    {
        FieldAccessStats globalStats;
        if (this->fieldAccessStatsByFunctionNumber != nullptr)
        {
            this->fieldAccessStatsByFunctionNumber->Map([&globalStats](uint functionNumber, FieldAccessStatsEntry* entry)
            {
                FieldAccessStats functionStats;
                entry->stats.Map([&functionStats](FieldAccessStatsPtr entryPointStats)
                {
                    functionStats.Add(entryPointStats);
                });

                if (PHASE_VERBOSE_STATS1(Js::ObjTypeSpecPhase))
                {
                    FunctionBody* functionBody = entry->functionBodyWeakRef->Get();
                    const char16* functionName = functionBody != nullptr ? functionBody->GetDisplayName() : _u("<unknown>");
                    Output::Print(_u("FieldAccessStats: function %s (#%u): inline cache stats:\n"), functionName, functionNumber);
                    Output::Print(_u("    overall: total %u, no profile info %u\n"), functionStats.totalInlineCacheCount, functionStats.noInfoInlineCacheCount);
                    Output::Print(_u("    mono: total %u, empty %u, cloned %u\n"),
                        functionStats.monoInlineCacheCount, functionStats.emptyMonoInlineCacheCount, functionStats.clonedMonoInlineCacheCount);
                    Output::Print(_u("    poly: total %u (high %u, low %u), null %u, empty %u, ignored %u, disabled %u, equivalent %u, non-equivalent %u, cloned %u\n"),
                        functionStats.polyInlineCacheCount, functionStats.highUtilPolyInlineCacheCount, functionStats.lowUtilPolyInlineCacheCount,
                        functionStats.nullPolyInlineCacheCount, functionStats.emptyPolyInlineCacheCount, functionStats.ignoredPolyInlineCacheCount, functionStats.disabledPolyInlineCacheCount,
                        functionStats.equivPolyInlineCacheCount, functionStats.nonEquivPolyInlineCacheCount, functionStats.clonedPolyInlineCacheCount);
                }

                globalStats.Add(&functionStats);
            });
        }

        Output::Print(_u("FieldAccessStats: totals\n"));
        Output::Print(_u("    overall: total %u, no profile info %u\n"), globalStats.totalInlineCacheCount, globalStats.noInfoInlineCacheCount);
        Output::Print(_u("    mono: total %u, empty %u, cloned %u\n"),
            globalStats.monoInlineCacheCount, globalStats.emptyMonoInlineCacheCount, globalStats.clonedMonoInlineCacheCount);
        Output::Print(_u("    poly: total %u (high %u, low %u), null %u, empty %u, ignored %u, disabled %u, equivalent %u, non-equivalent %u, cloned %u\n"),
            globalStats.polyInlineCacheCount, globalStats.highUtilPolyInlineCacheCount, globalStats.lowUtilPolyInlineCacheCount,
            globalStats.nullPolyInlineCacheCount, globalStats.emptyPolyInlineCacheCount, globalStats.ignoredPolyInlineCacheCount, globalStats.disabledPolyInlineCacheCount,
            globalStats.equivPolyInlineCacheCount, globalStats.nonEquivPolyInlineCacheCount, globalStats.clonedPolyInlineCacheCount);
    }
#endif

#ifdef MISSING_PROPERTY_STATS
    if (PHASE_STATS1(Js::MissingPropertyCachePhase))
    {
        Output::Print(_u("MissingPropertyStats: hits = %d, misses = %d, cache attempts = %d.\n"),
            this->missingPropertyHits, this->missingPropertyMisses, this->missingPropertyCacheAttempts);
    }
#endif


#ifdef INLINE_CACHE_STATS
        if (PHASE_STATS1(Js::PolymorphicInlineCachePhase))
        {
            Output::Print(_u("%s,%s,%s,%s,%s,%s,%s,%s,%s\n"), _u("Function"), _u("Property"), _u("Kind"), _u("Accesses"), _u("Misses"), _u("Miss Rate"), _u("Collisions"), _u("Collision Rate"), _u("Slot Count"));
            cacheDataMap->Map([this](Js::PolymorphicInlineCache const *cache, InlineCacheData *data) {
                cache->PrintStats(data);
            });
        }
#endif

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (regexStatsDatabase != 0)
            regexStatsDatabase->Print(GetRegexDebugWriter());
#endif
        OUTPUT_STATS(Js::EmitterPhase, _u("Script Context: 0x%p Url: %s\n"), this, this->url);
        OUTPUT_STATS(Js::EmitterPhase, _u("  Total thread committed code size = %d\n"), this->GetThreadContext()->GetCodeSize());

        OUTPUT_STATS(Js::ParsePhase, _u("Script Context: 0x%p Url: %s\n"), this, this->url);
        OUTPUT_STATS(Js::ParsePhase, _u("  Total ThreadContext source size %d\n"), this->GetThreadContext()->GetSourceSize());
#endif

#ifdef ENABLE_BASIC_TELEMETRY
        if (this->telemetry != nullptr)
        {
            // If an exception (e.g. out-of-memory) happens during InitializeAllocations then `this->telemetry` will be null and the Close method will still be called, hence this guard expression.
            this->telemetry->OutputPrint();
        }
#endif

        Output::Flush();
    }
    void ScriptContext::SetNextPendingClose(ScriptContext * nextPendingClose) {
        this->nextPendingClose = nextPendingClose;
    }

#ifdef ENABLE_MUTATION_BREAKPOINT
    bool ScriptContext::HasMutationBreakpoints()
    {
        if (this->GetDebugContext() != nullptr && this->GetDebugContext()->GetProbeContainer() != nullptr)
        {
            return this->GetDebugContext()->GetProbeContainer()->HasMutationBreakpoints();
        }
        return false;
    }

    void ScriptContext::InsertMutationBreakpoint(Js::MutationBreakpoint *mutationBreakpoint)
    {
        this->GetDebugContext()->GetProbeContainer()->InsertMutationBreakpoint(mutationBreakpoint);
    }
#endif

#ifdef REJIT_STATS
    void ScriptContext::LogDataForFunctionBody(Js::FunctionBody *body, uint idx, bool isRejit)
    {
        if (rejitStatsMap == NULL)
        {
            rejitStatsMap = RecyclerNew(this->recycler, RejitStatsMap, this->recycler);
            BindReference(rejitStatsMap);
        }

        RejitStats *stats = NULL;
        if (!rejitStatsMap->TryGetValue(body, &stats))
        {
            stats = Anew(GeneralAllocator(), RejitStats, this);
            rejitStatsMap->Item(body, stats);
        }

        if (isRejit)
        {
            stats->m_rejitReasonCounts[idx]++;
        }
        else
        {
            if (!stats->m_bailoutReasonCounts->ContainsKey(idx))
            {
                stats->m_bailoutReasonCounts->Item(idx, 1);
            }
            else
            {
                uint val = stats->m_bailoutReasonCounts->Item(idx);
                ++val;
                stats->m_bailoutReasonCounts->Item(idx, val);
            }
        }
    }
    void ScriptContext::LogRejit(Js::FunctionBody *body, RejitReason reason)
    {
        byte reasonIndex = static_cast<byte>(reason);
        Assert(reasonIndex < NumRejitReasons);
        rejitReasonCounts[reasonIndex]++;

#if defined(FLAG) || defined(FLAG_REGOVR_EXP)
        if (Js::Configuration::Global.flags.Verbose)
        {
            LogDataForFunctionBody(body, reasonIndex, true);
        }
#endif
    }
    void ScriptContext::LogBailout(Js::FunctionBody *body, uint kind)
    {
        if (!bailoutReasonCounts->ContainsKey(kind))
        {
            bailoutReasonCounts->Item(kind, 1);
        }
        else
        {
            uint val = bailoutReasonCounts->Item(kind);
            ++val;
            bailoutReasonCounts->Item(kind, val);
        }

#if defined(FLAG) || defined(FLAG_REGOVR_EXP)
        if (Js::Configuration::Global.flags.Verbose)
        {
            LogDataForFunctionBody(body, kind, false);
        }
#endif
    }
    void ScriptContext::ClearBailoutReasonCountsMap()
    {
        if (this->bailoutReasonCounts != nullptr)
        {
            this->bailoutReasonCounts->Clear();
        }
        if (this->bailoutReasonCountsCap != nullptr)
        {
            this->bailoutReasonCountsCap->Clear();
        }
    }
    void ScriptContext::ClearRejitReasonCountsArray()
    {
        if (this->rejitReasonCounts != nullptr)
        {
            for (UINT16 i = 0; i < NumRejitReasons; i++)
            {
                this->rejitReasonCounts[i] = 0;
            }
        }
        if (this->rejitReasonCountsCap != nullptr)
        {
            for (UINT16 i = 0; i < NumRejitReasons; i++)
            {
                this->rejitReasonCountsCap[i] = 0;
            }
        }
    }
#endif

#ifdef ENABLE_BASIC_TELEMETRY
    ScriptContextTelemetry& ScriptContext::GetTelemetry()
    {
        return *this->telemetry;
    }
    bool ScriptContext::HasTelemetry()
    {
        return this->telemetry != nullptr;
    }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
    DebugContext* ScriptContext::GetDebugContext() const
    {
        Assert(this->debugContext != nullptr);

        if (this->debugContext->IsClosed())
        {
            // Once DebugContext is closed we should assume it's not there
            // The actual deletion of debugContext happens in ScriptContext destructor
            return nullptr;
        }

        return this->debugContext;
    }
#endif

    bool ScriptContext::IsScriptContextInNonDebugMode() const
    {
#ifdef ENABLE_SCRIPT_DEBUGGING
        if (this->GetDebugContext() != nullptr)
        {
            return this->GetDebugContext()->IsDebugContextInNonDebugMode();
        }
#endif
        return true;
    }


    bool ScriptContext::IsScriptContextInDebugMode() const
    {
#ifdef ENABLE_SCRIPT_DEBUGGING
        if (this->GetDebugContext() != nullptr)
        {
            return this->GetDebugContext()->IsDebugContextInDebugMode();
        }
#endif
        return false;
    }

    bool ScriptContext::IsScriptContextInSourceRundownOrDebugMode() const
    {
#ifdef ENABLE_SCRIPT_DEBUGGING
        if (this->GetDebugContext() != nullptr)
        {
            return this->GetDebugContext()->IsDebugContextInSourceRundownOrDebugMode();
        }
#endif
        return false;
    }

#ifdef ENABLE_SCRIPT_DEBUGGING
    bool ScriptContext::IsDebuggerRecording() const
    {
        if (this->GetDebugContext() != nullptr)
        {
            return this->GetDebugContext()->IsDebuggerRecording();
        }
        return false;
    }

    void ScriptContext::SetIsDebuggerRecording(bool isDebuggerRecording)
    {
        if (this->GetDebugContext() != nullptr)
        {
            this->GetDebugContext()->SetIsDebuggerRecording(isDebuggerRecording);
        }
    }
#endif

    bool ScriptContext::IsIntlEnabled()
    {
#ifdef ENABLE_INTL_OBJECT
        if (GetConfig()->IsIntlEnabled())
        {
#ifdef INTL_WINGLOB
            // This will try to load globalization dlls if not already loaded.
            Js::DelayLoadWindowsGlobalization* globLibrary = GetThreadContext()->GetWindowsGlobalizationLibrary();
            return globLibrary->HasGlobalizationDllLoaded();
#else
            return true;
#endif
        }
#endif
        return false;
    }

#ifdef INLINE_CACHE_STATS
    void ScriptContext::LogCacheUsage(Js::PolymorphicInlineCache *cache, bool isGetter, Js::PropertyId propertyId, bool hit, bool collision)
    {
        if (cacheDataMap == NULL)
        {
            cacheDataMap = RecyclerNew(this->recycler, CacheDataMap, this->recycler);
            BindReference(cacheDataMap);
        }

        InlineCacheData *data = NULL;
        if (!cacheDataMap->TryGetValue(cache, &data))
        {
            data = Anew(GeneralAllocator(), InlineCacheData);
            cacheDataMap->Item(cache, data);
            data->isGetCache = isGetter;
            data->propertyId = propertyId;
        }

        Assert(data->isGetCache == isGetter);
        Assert(data->propertyId == propertyId);

        if (hit)
        {
            data->hits++;
        }
        else
        {
            data->misses++;
        }
        if (collision)
        {
            data->collisions++;
        }
    }
#endif

#ifdef FIELD_ACCESS_STATS
    void ScriptContext::RecordFieldAccessStats(FunctionBody* functionBody, FieldAccessStatsPtr fieldAccessStats)
    {
        Assert(fieldAccessStats != nullptr);

        if (!PHASE_STATS1(Js::ObjTypeSpecPhase))
        {
            return;
        }

        FieldAccessStatsEntry* entry;
        if (!this->fieldAccessStatsByFunctionNumber->TryGetValue(functionBody->GetFunctionNumber(), &entry))
        {
            RecyclerWeakReference<FunctionBody>* functionBodyWeakRef;
            this->recycler->FindOrCreateWeakReferenceHandle(functionBody, &functionBodyWeakRef);
            entry = RecyclerNew(this->recycler, FieldAccessStatsEntry, functionBodyWeakRef, this->recycler);

            this->fieldAccessStatsByFunctionNumber->AddNew(functionBody->GetFunctionNumber(), entry);
        }

        entry->stats.Prepend(fieldAccessStats);
    }
#endif

#ifdef MISSING_PROPERTY_STATS
    void ScriptContext::RecordMissingPropertyMiss()
    {
        this->missingPropertyMisses++;
    }

    void ScriptContext::RecordMissingPropertyHit()
    {
        this->missingPropertyHits++;
    }

    void ScriptContext::RecordMissingPropertyCacheAttempt()
    {
        this->missingPropertyCacheAttempts++;
    }
#endif

    bool ScriptContext::IsIntConstPropertyOnGlobalObject(Js::PropertyId propId)
    {
        return intConstPropsOnGlobalObject->ContainsKey(propId);
    }

    void ScriptContext::TrackIntConstPropertyOnGlobalObject(Js::PropertyId propertyId)
    {
        intConstPropsOnGlobalObject->AddNew(propertyId);
    }

    bool ScriptContext::IsIntConstPropertyOnGlobalUserObject(Js::PropertyId propertyId)
    {
        return intConstPropsOnGlobalUserObject->ContainsKey(propertyId) != NULL;
    }

    void ScriptContext::TrackIntConstPropertyOnGlobalUserObject(Js::PropertyId propertyId)
    {
        intConstPropsOnGlobalUserObject->AddNew(propertyId);
    }

    void ScriptContext::AddCalleeSourceInfoToList(Utf8SourceInfo* sourceInfo)
    {
        Assert(sourceInfo);

        RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef = nullptr;
        this->GetRecycler()->FindOrCreateWeakReferenceHandle(sourceInfo, &sourceInfoWeakRef);
        Assert(sourceInfoWeakRef);

        if (!calleeUtf8SourceInfoList)
        {
            Recycler *recycler = this->GetRecycler();
            calleeUtf8SourceInfoList.Root(RecyclerNew(recycler, CalleeSourceList, recycler), recycler);
        }

        if (!calleeUtf8SourceInfoList->Contains(sourceInfoWeakRef))
        {
            calleeUtf8SourceInfoList->Add(sourceInfoWeakRef);
        }
    }

#ifdef ENABLE_JS_ETW
    void ScriptContext::EmitStackTraceEvent(__in UINT64 operationID, __in USHORT maxFrameCount, bool emitV2AsyncStackEvent)
    {
        // If call root level is zero, there is no EntryExitRecord and the stack walk will fail.
        if (GetThreadContext()->GetCallRootLevel() == 0)
        {
            return;
        }

        Assert(EventEnabledJSCRIPT_STACKTRACE() || EventEnabledJSCRIPT_ASYNCCAUSALITY_STACKTRACE_V2() || PHASE_TRACE1(Js::StackFramesEventPhase));
        BEGIN_TEMP_ALLOCATOR(tempAllocator, this, _u("StackTraceEvent"))
        {
            JsUtil::List<StackFrameInfo, ArenaAllocator> stackFrames(tempAllocator);
            Js::JavascriptStackWalker walker(this);
            unsigned short nameBufferLength = 0;
            Js::StringBuilder<ArenaAllocator> nameBuffer(tempAllocator);
            nameBuffer.Reset();

            OUTPUT_TRACE(Js::StackFramesEventPhase, _u("\nPosting stack trace via ETW:\n"));

            ushort frameCount = walker.WalkUntil((ushort)maxFrameCount, [&](Js::JavascriptFunction* function, ushort frameIndex) -> bool
            {
                ULONG lineNumber = 0;
                LONG columnNumber = 0;
                UINT32 methodIdOrNameId = 0;
                UINT8 isFrameIndex = 0; // FALSE
                const WCHAR* name = nullptr;
                if (function->IsScriptFunction() && !function->IsLibraryCode())
                {
                    Js::FunctionBody * functionBody = function->GetFunctionBody();
                    functionBody->GetLineCharOffset(walker.GetByteCodeOffset(), &lineNumber, &columnNumber);
                    methodIdOrNameId = EtwTrace::GetFunctionId(functionBody);
                    name = functionBody->GetExternalDisplayName();
                }
                else
                {
                    if (function->IsScriptFunction())
                    {
                        name = function->GetFunctionBody()->GetExternalDisplayName();
                    }
                    else
                    {
                        name = walker.GetCurrentNativeLibraryEntryName();
                    }

                    ushort nameLen = ProcessNameAndGetLength(&nameBuffer, name);

                    methodIdOrNameId = nameBufferLength;

                    // Keep track of the current length of the buffer. The next nameIndex will be at this position (+1 for each '\\', '\"', and ';' character added above).
                    nameBufferLength += nameLen;

                    isFrameIndex = 1; // TRUE;
                }

                StackFrameInfo frame((DWORD_PTR)function->GetScriptContext(),
                    (UINT32)lineNumber,
                    (UINT32)columnNumber,
                    methodIdOrNameId,
                    isFrameIndex);

                OUTPUT_TRACE(Js::StackFramesEventPhase, _u("Frame : (%s : %u) (%s), LineNumber : %u, ColumnNumber : %u\n"),
                    (isFrameIndex == 1) ? (_u("NameBufferIndex")) : (_u("MethodID")),
                    methodIdOrNameId,
                    name,
                    lineNumber,
                    columnNumber);

                stackFrames.Add(frame);

                return false;
            });

            Assert(frameCount == (ushort)stackFrames.Count());

            if (frameCount > 0) // No need to emit event if there are no script frames.
            {
                auto nameBufferString = nameBuffer.Detach();

                if (nameBufferLength > 0)
                {
                    // Account for the terminating null character.
                    nameBufferLength++;
                }

                if (emitV2AsyncStackEvent)
                {
                    JS_ETW(EventWriteJSCRIPT_ASYNCCAUSALITY_STACKTRACE_V2(operationID, frameCount, nameBufferLength, sizeof(StackFrameInfo), &stackFrames.Item(0), nameBufferString));
                }
                else
                {
                    JS_ETW(EventWriteJSCRIPT_STACKTRACE(operationID, frameCount, nameBufferLength, sizeof(StackFrameInfo), &stackFrames.Item(0), nameBufferString));
                }
            }
        }
        END_TEMP_ALLOCATOR(tempAllocator, this);

        OUTPUT_FLUSH();
    }

    // Info:        Append sourceString to stringBuilder after escaping charToEscape with escapeChar.
    //              "SomeBadly\0Formed\0String" => "SomeBadly\\\0Formed\\\0String"
    // Parameters:  stringBuilder - The Js::StringBuilder to which we should append sourceString.
    //              sourceString - The string we want to escape and append to stringBuilder.
    //              sourceStringLen - Length of sourceString.
    //              escapeChar - Char to use for escaping.
    //              charToEscape - The char which we should escape with escapeChar.
    // Returns:     Count of chars written to stringBuilder.
    charcount_t ScriptContext::AppendWithEscapeCharacters(Js::StringBuilder<ArenaAllocator>* stringBuilder, const WCHAR* sourceString, charcount_t sourceStringLen, WCHAR escapeChar, WCHAR charToEscape)
    {
        const WCHAR* charToEscapePtr = wcschr(sourceString, charToEscape);
        charcount_t charsPadding = 0;

        // Only escape characters if sourceString contains one.
        if (charToEscapePtr)
        {
            charcount_t charsWritten = 0;
            charcount_t charsToAppend = 0;

            while (charToEscapePtr)
            {
                charsToAppend = static_cast<charcount_t>(charToEscapePtr - sourceString) - charsWritten;

                stringBuilder->Append(sourceString + charsWritten, charsToAppend);
                stringBuilder->Append(escapeChar);
                stringBuilder->Append(charToEscape);

                // Keep track of this extra escapeChar character so we can update the buffer length correctly below.
                charsPadding++;

                // charsWritten is a count of the chars from sourceString which have been written - not count of chars Appended to stringBuilder.
                charsWritten += charsToAppend + 1;

                // Find next charToEscape.
                charToEscapePtr++;
                charToEscapePtr = wcschr(charToEscapePtr, charToEscape);
            }

            // Append the final part of the string if there is any left after the final charToEscape.
            if (charsWritten != sourceStringLen)
            {
                charsToAppend = sourceStringLen - charsWritten;
                stringBuilder->Append(sourceString + charsWritten, charsToAppend);
            }
        }
        else
        {
            stringBuilder->AppendSz(sourceString);
        }

        return sourceStringLen + charsPadding;
    }

    /*static*/
    ushort ScriptContext::ProcessNameAndGetLength(Js::StringBuilder<ArenaAllocator>* nameBuffer, const WCHAR* name)
    {
        Assert(nameBuffer);
        Assert(name);

        ushort nameLen = (ushort)wcslen(name);

        // Surround each function name with quotes and escape any quote characters in the function name itself with '\\'.
        nameBuffer->Append('\"');

        // Adjust nameLen based on any escape characters we added to escape the '\"' in name.
        nameLen = (unsigned short)AppendWithEscapeCharacters(nameBuffer, name, nameLen, '\\', '\"');

        nameBuffer->AppendCppLiteral(_u("\";"));

        // Add 3 padding characters here - one for initial '\"' character, too.
        nameLen += 3;

        return nameLen;
    }
#endif

    Field(Js::Var)* ScriptContext::GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex)
    {
        Js::SourceTextModuleRecord* moduleRecord = this->GetModuleRecord(moduleIndex);
        Assert(moduleRecord != nullptr);

        // Require caller to also provide the intended access slot so we can do bounds check now.
        if (moduleRecord->GetLocalExportCount() + 1 <= slotIndex)
        {
            Js::Throw::FatalInternalError();
        }

        return moduleRecord->GetLocalExportSlots();
    }

#if ENABLE_NATIVE_CODEGEN
    void JITPageAddrToFuncRangeCache::ClearCache()
    {
        if (jitPageAddrToFuncRangeMap != nullptr)
        {
            jitPageAddrToFuncRangeMap->Map(
                [](void* key, RangeMap* value) {
                HeapDelete(value);
            });

            HeapDelete(jitPageAddrToFuncRangeMap);
            jitPageAddrToFuncRangeMap = nullptr;
        }

        if (largeJitFuncToSizeMap != nullptr)
        {
            HeapDelete(largeJitFuncToSizeMap);
            largeJitFuncToSizeMap = nullptr;
        }
    }

    void JITPageAddrToFuncRangeCache::AddFuncRange(void * address, uint bytes)
    {
        AutoCriticalSection autocs(GetCriticalSection());

        if (bytes <= AutoSystemInfo::PageSize)
        {
            if (jitPageAddrToFuncRangeMap == nullptr)
            {
                jitPageAddrToFuncRangeMap = HeapNew(JITPageAddrToFuncRangeMap, &HeapAllocator::Instance);
            }

            void * pageAddr = GetPageAddr(address);
            RangeMap * rangeMap = nullptr;
            bool isPageAddrFound = jitPageAddrToFuncRangeMap->TryGetValue(pageAddr, &rangeMap);
            if (rangeMap == nullptr)
            {
                Assert(!isPageAddrFound);
                rangeMap = HeapNew(RangeMap, &HeapAllocator::Instance);
                jitPageAddrToFuncRangeMap->Add(pageAddr, rangeMap);
            }
            uint byteCount = 0;
            Assert(!rangeMap->TryGetValue(address, &byteCount));
            rangeMap->Add(address, bytes);
        }
        else
        {
            if (largeJitFuncToSizeMap == nullptr)
            {
                largeJitFuncToSizeMap = HeapNew(LargeJITFuncAddrToSizeMap, &HeapAllocator::Instance);
            }

            uint byteCount = 0;
            Assert(!largeJitFuncToSizeMap->TryGetValue(address, &byteCount));
            largeJitFuncToSizeMap->Add(address, bytes);
        }
    }

    void* JITPageAddrToFuncRangeCache::GetPageAddr(void * address)
    {
        return (void*)((uintptr_t)address & ~(AutoSystemInfo::PageSize - 1));
    }

    void JITPageAddrToFuncRangeCache::RemoveFuncRange(void * address)
    {
        AutoCriticalSection autocs(GetCriticalSection());

        void * pageAddr = GetPageAddr(address);

        RangeMap * rangeMap = nullptr;
        uint bytes = 0;
        if (jitPageAddrToFuncRangeMap && jitPageAddrToFuncRangeMap->TryGetValue(pageAddr, &rangeMap))
        {
            Assert(rangeMap->Count() != 0);
            rangeMap->Remove(address);

            if (rangeMap->Count() == 0)
            {
                HeapDelete(rangeMap);
                rangeMap = nullptr;
                jitPageAddrToFuncRangeMap->Remove(pageAddr);
            }
            return;
        }
        else if (largeJitFuncToSizeMap && largeJitFuncToSizeMap->TryGetValue(address, &bytes))
        {
            largeJitFuncToSizeMap->Remove(address);
        }
        else
        {
            AssertMsg(false, "Page address not found to remove the func range");
        }
    }

    bool JITPageAddrToFuncRangeCache::IsNativeAddr(void * address)
    {
        AutoCriticalSection autocs(GetCriticalSection());

        void * pageAddr = GetPageAddr(address);
        RangeMap * rangeMap = nullptr;
        if (jitPageAddrToFuncRangeMap && jitPageAddrToFuncRangeMap->TryGetValue(pageAddr, &rangeMap))
        {
            if (rangeMap->MapUntil(
                [&](void* key, uint value) {
                return (key <= address && (uintptr_t)address < ((uintptr_t)key + value));
            }))
            {
                return true;
            }
        }

        return largeJitFuncToSizeMap && largeJitFuncToSizeMap->MapUntil(
            [&](void *key, uint value) {
            return (key <= address && (uintptr_t)address < ((uintptr_t)key + value));
        });
    }

    JITPageAddrToFuncRangeCache::JITPageAddrToFuncRangeMap * JITPageAddrToFuncRangeCache::GetJITPageAddrToFuncRangeMap()
    {
        return jitPageAddrToFuncRangeMap;
    }

    JITPageAddrToFuncRangeCache::LargeJITFuncAddrToSizeMap * JITPageAddrToFuncRangeCache::GetLargeJITFuncAddrToSizeMap()
    {
        return largeJitFuncToSizeMap;
    }
#endif

} // End namespace Js
