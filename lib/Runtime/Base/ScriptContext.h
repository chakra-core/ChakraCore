//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(_WIN32) && defined(ENABLE_SCRIPT_PROFILING)
#include "activprof.h"
#endif

#if DBG || ENABLE_REGEX_CONFIG_OPTIONS || defined(PROFILE_STRINGS)
#define NEED_MISC_ALLOCATOR
#endif

#define BuiltInFunctionsScriptId 0

using namespace PlatformAgnostic;

class NativeCodeGenerator;
class BackgroundParser;
struct IActiveScriptDirect;
#ifdef ENABLE_BASIC_TELEMETRY
class ScriptContextTelemetry;
#endif
namespace Js
{
    class ScriptContext;
    class ScriptEditQuery;
    class MutationBreakpoint;
    class StringProfiler;
    class DebugContext;
    struct HaltCallback;
    struct DebuggerOptionsCallback;
    class ModuleRecordBase;
}

// Created for every source buffer passed by host.
// This structure has all the details.
class SRCINFO
{
    // We currently don't free SRCINFO object so we don't want to add extra variables here.
    // In future, when we do make it freeable and will be able to allocate more than one per Module,
    // we can move variables m_isGlobalFunc and m_isEval from FunctionBody.cpp here.
public:
    Field(SourceContextInfo *) sourceContextInfo;
    Field(ULONG) dlnHost;             // Line number passed to ParseScriptText
    Field(ULONG) ulColumnHost;        // Column number on the line where the parse script text started
    Field(ULONG) lnMinHost;           // Line offset of first host-supplied line
    Field(ULONG) ichMinHost;          // Range of host supplied characters
    Field(ULONG) ichLimHost;
    Field(ULONG) ulCharOffset;        // Char offset of the source text relative to the document. (Populated using IActiveScriptContext)
    Field(Js::ModuleID) moduleID;
    Field(ULONG) grfsi;

    static SRCINFO* Copy(Recycler* recycler, const SRCINFO* srcInfo)
    {
        SRCINFO* copySrcInfo = RecyclerNew(recycler, SRCINFO, *srcInfo);
        return copySrcInfo;
    }

    SRCINFO()
    {
    }
    SRCINFO(const SRCINFO& other)
        :sourceContextInfo(other.sourceContextInfo),
        dlnHost(other.dlnHost),
        ulColumnHost(other.ulColumnHost),
        lnMinHost(other.lnMinHost),
        ichMinHost(other.ichMinHost),
        ichLimHost(other.ichLimHost),
        ulCharOffset(other.ulCharOffset),
        moduleID(other.moduleID),
        grfsi(other.grfsi)
    {
    }
    SRCINFO(
        SourceContextInfo*sourceContextInfo,
        ULONG dlnHost,
        ULONG ulColumnHost,
        ULONG lnMinHost,
        ULONG ichMinHost,
        ULONG ichLimHost,
        ULONG ulCharOffset,
        Js::ModuleID moduleID,
        ULONG grfsi
    ):sourceContextInfo(sourceContextInfo),
        dlnHost(dlnHost),
        ulColumnHost(ulColumnHost),
        lnMinHost(lnMinHost),
        ichMinHost(ichMinHost),
        ichLimHost(ichLimHost),
        ulCharOffset(ulCharOffset),
        moduleID(moduleID),
        grfsi(grfsi)
    {
    }
};

struct CustomExternalObjectOperations
{
    size_t offsetOfOperationsUsage;
    DWORD operationFlagEquals;
    DWORD operationFlagStrictEquals;
};

enum ExternalJitData
{
    ExternalJitData_CustomExternalObjectOperations
};

enum LoadScriptFlag
{
    LoadScriptFlag_None         = 0x0,
    LoadScriptFlag_Expression   = 0x1,                  // the script returns a value.
    LoadScriptFlag_disableDeferredParse = 0x2,          // do not defer-parse the code.
    LoadScriptFlag_isByteCodeBufferForLibrary = 0x4,    // for bytecode buffer
    LoadScriptFlag_disableAsmJs = 0x8,                  // disable parse as asmjs. The code is not conform to asmjs requirement.
    LoadScriptFlag_Module = 0x10,                       // input script is module code.
    LoadScriptFlag_isFunction = 0x20,                   // input script is in a function scope, not global code.
    LoadScriptFlag_Utf8Source = 0x40,                   // input buffer is utf8 encoded.
    LoadScriptFlag_LibraryCode = 0x80,                  // for debugger, indicating 'not my code'
    LoadScriptFlag_ExternalArrayBuffer = 0x100          // for ExternalArrayBuffer
};

#ifdef INLINE_CACHE_STATS
// Used to store inline cache stats

struct InlineCacheData
{
    uint hits;
    uint misses;
    uint collisions;
    bool isGetCache;
    Js::PropertyId propertyId;

    InlineCacheData() : hits(0), misses(0), collisions(0), isGetCache(false), propertyId(Js::PropertyIds::_none) { }
};

#endif

class HostScriptContext
{
public:
    HostScriptContext(Js::ScriptContext* inScriptContext) { this->scriptContext = inScriptContext; }
    virtual void Delete() = 0;
    virtual HRESULT GetPreviousHostScriptContext(__deref_out HostScriptContext** ppUnkCaller) = 0;
    virtual HRESULT PushHostScriptContext() = 0;
    virtual void PopHostScriptContext() = 0;

    virtual HRESULT SetCaller(IUnknown *punkNew, IUnknown **ppunkPrev) = 0;
    virtual HRESULT GetDispatchExCaller(__deref_out void** dispatchExCaller) = 0;
    virtual void ReleaseDispatchExCaller(__in void* dispatchExCaler) = 0;
    virtual Js::ModuleRoot * GetModuleRoot(int moduleID) = 0;
    virtual HRESULT CheckCrossDomainScriptContext(__in Js::ScriptContext* scriptContext) = 0;

    virtual HRESULT GetHostContextUrl(__in DWORD_PTR hostSourceContext, __out BSTR& pUrl) = 0;
    virtual BOOL HasCaller() = 0;
    virtual void CleanDynamicCodeCache() = 0;
    virtual HRESULT VerifyDOMSecurity(Js::ScriptContext* targetContext, Js::Var obj) = 0;

    virtual HRESULT CheckEvalRestriction() = 0;
    virtual HRESULT HostExceptionFromHRESULT(HRESULT hr, Js::Var* outError) = 0;

    virtual HRESULT GetExternalJitData(ExternalJitData id, void *data) = 0;
    virtual HRESULT SetDispatchInvoke(Js::JavascriptMethod dispatchInvoke) = 0;
    virtual HRESULT ArrayBufferFromExternalObject(__in Js::RecyclableObject *obj,
        __out Js::ArrayBuffer **ppArrayBuffer) = 0;
    virtual Js::JavascriptError* CreateWinRTError(IErrorInfo* perrinfo, Js::RestrictedErrorStrings * proerrstr) = 0;
    virtual HRESULT EnqueuePromiseTask(Js::Var varTask) = 0;

    virtual HRESULT FetchImportedModule(Js::ModuleRecordBase* referencingModule, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord) = 0;
    virtual HRESULT FetchImportedModuleFromScript(DWORD_PTR dwReferencingSourceContext, LPCOLESTR specifier, Js::ModuleRecordBase** dependentModuleRecord) = 0;
    virtual HRESULT NotifyHostAboutModuleReady(Js::ModuleRecordBase* referencingModule, Js::Var exceptionVar) = 0;

    Js::ScriptContext* GetScriptContext() { return scriptContext; }

    virtual bool SetCrossSiteForFunctionType(Js::JavascriptFunction * function) = 0;
#if DBG_DUMP || defined(PROFILE_EXEC) || defined(PROFILE_MEM)
    virtual void EnsureParentInfo(Js::ScriptContext* scriptContext = NULL) = 0;
#endif
#if DBG
    virtual bool IsHostCrossSiteThunk(Js::JavascriptMethod address) = 0;
#endif
private:
    Js::ScriptContext* scriptContext;
};

#if ENABLE_TTD
typedef void (CALLBACK *JsTTDOnScriptLoadCallback)(FinalizableObject* hostContext, Js::FunctionBody* body, Js::Utf8SourceInfo* utf8SourceInfo, CompileScriptException* compileException, bool notify);
typedef uint32 (CALLBACK *JsTTDOnBPRegisterCallback)(void* hostRuntime, int64 bpID, Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* utf8SourceInfo, uint32 line, uint32 column, BOOL* isNewBP);
typedef void (CALLBACK *JsTTDOnBPDeleteCallback)(void* hostRuntime, uint32 bpID);
typedef void (CALLBACK *JsTTDOnBPClearDocumentCallback)(void* hostRuntime);

//A class that we use to pass in a functor from the host when we need to inform it about something we are doing
class HostScriptContextCallbackFunctor
{
public:
    FinalizableObject* HostContext;
    void* HostRuntime;

    JsTTDOnScriptLoadCallback pfOnScriptLoadCallback;

    JsTTDOnBPRegisterCallback pfOnBPRegisterCallback;
    JsTTDOnBPDeleteCallback pfOnBPDeleteCallback;
    JsTTDOnBPClearDocumentCallback pfOnBPClearDocumentCallback;

    HostScriptContextCallbackFunctor()
        : HostContext(nullptr), HostRuntime(nullptr), pfOnScriptLoadCallback(nullptr), pfOnBPRegisterCallback(nullptr), pfOnBPDeleteCallback(nullptr), pfOnBPClearDocumentCallback(nullptr)
    {
        ;
    }

    HostScriptContextCallbackFunctor(FinalizableObject* hostContext, void* hostRuntime, JsTTDOnScriptLoadCallback callbackOnScriptLoad, JsTTDOnBPRegisterCallback callbackOnBPRegister, JsTTDOnBPDeleteCallback callbackOnBPDelete, JsTTDOnBPClearDocumentCallback callbackOnBPClearDocument)
        : HostContext(hostContext), HostRuntime(hostRuntime), pfOnScriptLoadCallback(callbackOnScriptLoad), pfOnBPRegisterCallback(callbackOnBPRegister), pfOnBPDeleteCallback(callbackOnBPDelete), pfOnBPClearDocumentCallback(callbackOnBPClearDocument)
    {
        ;
    }
};
#endif

namespace Js
{

#pragma pack(push, 1)
    struct StackFrameInfo
    {
        StackFrameInfo() { }
        StackFrameInfo(DWORD_PTR _scriptContextID
            , UINT32 _sourceLocationLineNumber
            , UINT32 _sourceLocationColumnNumber
            , UINT32 _methodIDOrNameIndex
            , UINT8 _isFrameIndex)
            : scriptContextID(_scriptContextID)
            , sourceLocationLineNumber(_sourceLocationLineNumber)
            , sourceLocationColumnNumber(_sourceLocationColumnNumber)
            , methodIDOrNameIndex(_methodIDOrNameIndex)
            , isFrameIndex(_isFrameIndex)
        { }

        DWORD_PTR scriptContextID;
        UINT32 sourceLocationLineNumber;
        UINT32 sourceLocationColumnNumber;
        UINT32 methodIDOrNameIndex;
        UINT8  isFrameIndex;
    };
#pragma pack(pop)

#ifdef ENABLE_PROJECTION
    class ProjectionConfiguration
    {
    public:
        ProjectionConfiguration() : targetVersion(0)
        {
        }

        DWORD GetTargetVersion() const { return this->targetVersion; }
        void SetTargetVersion(DWORD version) { this->targetVersion = version; }

        bool IsTargetWindows8() const           { return this->targetVersion == NTDDI_WIN8; }
        bool IsTargetWindowsBlueOrLater() const { return this->targetVersion >= NTDDI_WINBLUE; }

    private:
        DWORD targetVersion;
    };
#endif // ENABLE_PROJECTION

    class ScriptConfiguration
    {
    public:
        ScriptConfiguration(const ThreadConfiguration * const threadConfig, const bool isOptimizedForManyInstances) :
#ifdef ENABLE_PROJECTION
            HostType(Configuration::Global.flags.HostType),
            WinRTConstructorAllowed(Configuration::Global.flags.WinRTConstructorAllowed),
#endif
            NoNative(Configuration::Global.flags.NoNative),
            isOptimizedForManyInstances(isOptimizedForManyInstances),
            threadConfig(threadConfig)
        {
        }

        // Version
        bool SupportsES3()                      const { return true; }
        bool SupportsES3Extensions()            const {
#ifdef ENABLE_PROJECTION
            return HostType != HostTypeApplication;
#else
            return true;
#endif
        }

#define FORWARD_THREAD_CONFIG(flag) inline bool flag() const { return threadConfig->flag(); }
#define FLAG(threadFlag, globalFlag) FORWARD_THREAD_CONFIG(threadFlag)
#define FLAG_RELEASE(threadFlag, globalFlag) FORWARD_THREAD_CONFIG(threadFlag)
#include "../Base/ThreadConfigFlagsList.h"
#undef FLAG_RELEASE
#undef FLAG
#undef FORWARD_THREAD_CONFIG

        bool SupportsCollectGarbage() const { return true; }

        void ForceNoNative() { this->NoNative = true; }
        void ForceNative() { this->NoNative = false; }
        bool IsNoNative() const { return this->NoNative; }

        void SetCanOptimizeGlobalLookupFlag(BOOL f){ this->fCanOptimizeGlobalLookup = f;}
        BOOL CanOptimizeGlobalLookup() const { return this->fCanOptimizeGlobalLookup;}
        bool IsOptimizedForManyInstances() const { return isOptimizedForManyInstances; }
        void CopyFrom(ScriptConfiguration& other)
        {
            this->NoNative = other.NoNative;
            this->fCanOptimizeGlobalLookup = other.fCanOptimizeGlobalLookup;
#ifdef ENABLE_PROJECTION
            this->HostType = other.HostType;
            this->WinRTConstructorAllowed = other.WinRTConstructorAllowed;
            this->projectionConfiguration = other.projectionConfiguration;
#endif
        }

#ifdef ENABLE_PROJECTION
        Number GetHostType() const    // Returns one of enum HostType values (see ConfigFlagsTable.h).
        {
            AssertMsg(this->HostType >= HostTypeMin && this->HostType <= HostTypeMax, "HostType value is out of valid range.");
            return this->HostType;
        }

        ProjectionConfiguration const * GetProjectionConfig() const
        {
            return &projectionConfiguration;
        }
        void SetHostType(int32 hostType) { this->HostType = hostType; }
        void SetWinRTConstructorAllowed(bool allowed) { this->WinRTConstructorAllowed = allowed; }
        void SetProjectionTargetVersion(DWORD version)
        {
            projectionConfiguration.SetTargetVersion(version);
        }
        bool IsWinRTEnabled()           const { return (GetHostType() == Js::HostTypeApplication) || (GetHostType() == Js::HostTypeWebview); }

        bool IsWinRTConstructorAllowed() const { return (GetHostType() != Js::HostTypeWebview) || this->WinRTConstructorAllowed; }
#endif
    private:

        // Per script configurations
        bool NoNative;
        BOOL fCanOptimizeGlobalLookup;
        const bool isOptimizedForManyInstances;
        const ThreadConfiguration * const threadConfig;

#ifdef ENABLE_PROJECTION
        Number HostType;    // One of enum HostType values (see ConfigFlagsTable.h).
        bool WinRTConstructorAllowed;  // whether allow constructor in webview host type. Also note that this is not a security feature.
        ProjectionConfiguration projectionConfiguration;
#endif
    };

    struct ScriptEntryExitRecord
    {
        bool hasCaller : 1;
        bool hasReentered : 1;
#if DBG_DUMP
        bool isCallRoot : 1;
#endif
#if DBG || defined(PROFILE_EXEC)
        bool leaveForHost : 1;
#endif
#if DBG
        bool leaveForAsyncHostOperation : 1;
#endif
#ifdef CHECK_STACKWALK_EXCEPTION
        bool ignoreStackWalkException: 1;
#endif
        Js::ImplicitCallFlags savedImplicitCallFlags;

        void * returnAddrOfScriptEntryFunction;
        void * frameIdOfScriptExitFunction; // the frameAddres in x86, the return address in amd64/arm_soc
        ScriptContext * scriptContext;
        struct ScriptEntryExitRecord * next;

#if defined(_M_IX86) && defined(DBG)
        void * scriptEntryFS0;
#endif
#ifdef EXCEPTION_CHECK
        ExceptionType handledExceptionType;
#endif
    };

#if ENABLE_NATIVE_CODEGEN
    /*
    * This class caches jitted func address ranges.
    * This is to facilitate WER scenarios to use this cache for checking native addresses.
    */
    class JITPageAddrToFuncRangeCache
    {
    public:
        typedef JsUtil::BaseDictionary<void *, uint, HeapAllocator> RangeMap;
        typedef JsUtil::BaseDictionary<void *, RangeMap*, HeapAllocator> JITPageAddrToFuncRangeMap;
        typedef JsUtil::BaseDictionary<void *, uint, HeapAllocator> LargeJITFuncAddrToSizeMap;

    private:
        JITPageAddrToFuncRangeMap * jitPageAddrToFuncRangeMap;
        LargeJITFuncAddrToSizeMap * largeJitFuncToSizeMap;

        static CriticalSection cs;

    public:
        JITPageAddrToFuncRangeCache() :jitPageAddrToFuncRangeMap(nullptr), largeJitFuncToSizeMap(nullptr) {}
        ~JITPageAddrToFuncRangeCache()
        {
            ClearCache();
        }
        void ClearCache();
        void AddFuncRange(void * address, uint bytes);
        void RemoveFuncRange(void * address);
        void * GetPageAddr(void * address);
        bool IsNativeAddr(void * address);
        JITPageAddrToFuncRangeMap * GetJITPageAddrToFuncRangeMap();
        LargeJITFuncAddrToSizeMap * GetLargeJITFuncAddrToSizeMap();
        static CriticalSection * GetCriticalSection() { return &cs; }
    };
#endif

    class ScriptContext : public ScriptContextBase, public ScriptContextInfo
    {
        friend class LowererMD;
        friend class RemoteScriptContext;
        friend class GlobalObject; // InitializeCache
        friend class SourceTextModuleRecord; // for module bytecode gen.

#if ENABLE_NATIVE_CODEGEN
    public:
        static DWORD GetThreadContextOffset() { return offsetof(ScriptContext, threadContext); }
        static DWORD GetOptimizationOverridesOffset() { return offsetof(ScriptContext, optimizationOverrides); }
        static DWORD GetRecyclerOffset() { return offsetof(ScriptContext, recycler); }
        static DWORD GetNumberAllocatorOffset() { return offsetof(ScriptContext, numberAllocator); }

        JITPageAddrToFuncRangeCache * GetJitFuncRangeCache();
    private:
        JITPageAddrToFuncRangeCache * jitFuncRangeCache;
#endif
    public:
        ScriptContext *next;
        ScriptContext *prev;
        bool IsRegistered() { return next != nullptr || prev != nullptr || threadContext->GetScriptContextList() == this; }
        union
        {
            int64 int64Val;
            float floatVal;
            double dbVal;
            AsmJsSIMDValue simdVal; // stores raw simd result for Asm interpreter
        } asmJsReturnValue;
        static DWORD GetAsmJsReturnValueOffset() { return offsetof(ScriptContext, asmJsReturnValue); }

        ScriptContextOptimizationOverrideInfo optimizationOverrides;

        Js::JavascriptMethod CurrentThunk;
        Js::JavascriptMethod CurrentCrossSiteThunk;
        Js::JavascriptMethod DeferredParsingThunk;
        Js::JavascriptMethod DeferredDeserializationThunk;
        Js::JavascriptMethod DispatchDefaultInvoke;
        Js::JavascriptMethod DispatchProfileInvoke;

#ifdef ENABLE_SCRIPT_DEBUGGING
        typedef HRESULT (*GetDocumentContextFunction)(
            Js::FunctionBody *pFunctionBody,
            IDebugDocumentContext **ppDebugDocumentContext);
        GetDocumentContextFunction GetDocumentContext;
#endif // ENABLE_SCRIPT_DEBUGGING

#ifdef ENABLE_SCRIPT_PROFILING
        typedef HRESULT (*CleanupDocumentContextFunction)(ScriptContext *pContext);
        CleanupDocumentContextFunction CleanupDocumentContext;
#endif

        const ScriptContextBase* GetScriptContextBase() const { return static_cast<const ScriptContextBase*>(this); }

        void RedeferFunctionBodies(ActiveFunctionSet *pActive, uint inactiveThreshold);
        bool DoUndeferGlobalFunctions() const;

        bool IsUndeclBlockVar(Var var) const { return this->javascriptLibrary->IsUndeclBlockVar(var); }

        void TrackPid(const PropertyRecord* propertyRecord);
        void TrackPid(PropertyId propertyId);

        bool IsTrackedPropertyId(Js::PropertyId propertyId);
        void InvalidateHostObjects()
        {
            AssertMsg(!isClosed, "Host Object invalidation should occur before the engine is fully closed. Figure our how isClosed got set beforehand.");
            isInvalidatedForHostObjects = true;
        }
        bool IsInvalidatedForHostObjects()
        {
            return isInvalidatedForHostObjects;
        }

#if ENABLE_NATIVE_CODEGEN
        void InitializeRemoteScriptContext();
#endif

#ifdef ENABLE_JS_ETW
        void EmitStackTraceEvent(__in UINT64 operationID, __in USHORT maxFrameCount, bool emitV2AsyncStackEvent);
        static ushort ProcessNameAndGetLength(Js::StringBuilder<ArenaAllocator>* nameBuffer, const WCHAR* name);
#endif

        void SetIsDiagnosticsScriptContext(bool set) { this->isDiagnosticsScriptContext = set; }
        bool IsDiagnosticsScriptContext() const { return this->isDiagnosticsScriptContext; }
        bool IsScriptContextInNonDebugMode() const;
        bool IsScriptContextInDebugMode() const;
        bool IsScriptContextInSourceRundownOrDebugMode() const;

#ifdef ENABLE_SCRIPT_DEBUGGING
        bool IsDebuggerRecording() const;
        void SetIsDebuggerRecording(bool isDebuggerRecording);
#endif

        bool IsRunningScript() const { return this->threadContext->GetScriptEntryExit() != nullptr; }

        typedef JsUtil::List<RecyclerWeakReference<Utf8SourceInfo>*, Recycler, false, Js::WeakRefFreeListedRemovePolicy> CalleeSourceList;
        RecyclerRootPtr<CalleeSourceList> calleeUtf8SourceInfoList;
        void AddCalleeSourceInfoToList(Utf8SourceInfo* sourceInfo);
        bool HaveCalleeSources() { return calleeUtf8SourceInfoList && !calleeUtf8SourceInfoList->Empty(); }

        template<class TMapFunction>
        void MapCalleeSources(TMapFunction map)
        {
            if (this->HaveCalleeSources())
            {
                calleeUtf8SourceInfoList->Map([&](uint i, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfoWeakRef)
                {
                    if (calleeUtf8SourceInfoList->IsItemValid(i))
                    {
                        Js::Utf8SourceInfo* sourceInfo = sourceInfoWeakRef->Get();
                        map(sourceInfo);
                    }
                });
            }
            if (calleeUtf8SourceInfoList)
            {
                calleeUtf8SourceInfoList.Unroot(this->GetRecycler());
            }
        }

#ifdef ASMJS_PLAT
        inline AsmJsCodeGenerator* GetAsmJsCodeGenerator() const{return asmJsCodeGenerator;}
        AsmJsCodeGenerator* InitAsmJsCodeGenerator();
#endif

        bool IsExceptionWrapperForBuiltInsEnabled();
        static bool IsExceptionWrapperForBuiltInsEnabled(ScriptContext* scriptContext);
        static bool IsExceptionWrapperForHelpersEnabled(ScriptContext* scriptContext);
        bool IsEnumerateNonUserFunctionsOnly() const { return m_enumerateNonUserFunctionsOnly; }
#ifdef ENABLE_SCRIPT_PROFILING
        bool IsTraceDomCall() const { return !!m_fTraceDomCall; }
#elif defined(ENABLE_SCRIPT_DEBUGGING)
        bool IsTraceDomCall() const { return false; }
#endif

        InlineCache * GetValueOfInlineCache() const { return valueOfInlineCache;}
        InlineCache * GetToStringInlineCache() const { return toStringInlineCache; }

    private:

        JavascriptFunction* GenerateRootFunction(ParseNodePtr parseTree, uint sourceIndex, Parser* parser, uint32 grfscr, CompileScriptException * pse, const char16 *rootDisplayName);

        typedef void (*EventHandler)(ScriptContext *);
        ScriptContext ** registeredPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext;

        ArenaAllocator generalAllocator;
#ifdef ENABLE_BASIC_TELEMETRY
        ArenaAllocator telemetryAllocator;
#endif

        ArenaAllocator dynamicProfileInfoAllocator;
        InlineCacheAllocator inlineCacheAllocator;
        CacheAllocator isInstInlineCacheAllocator;
        CacheAllocator forInCacheAllocator;

        ArenaAllocator* interpreterArena;
        ArenaAllocator* guestArena;

#ifdef ENABLE_SCRIPT_DEBUGGING
        ArenaAllocator* diagnosticArena;
#endif

#if ENABLE_NATIVE_CODEGEN
        PSCRIPTCONTEXT_HANDLE m_remoteScriptContextAddr;
#endif

        bool startupComplete; // Indicates if the heuristic startup phase for this script context is complete
        bool isInvalidatedForHostObjects;  // Indicates that we've invalidate all objects in the host so stop calling them.
#ifdef ENABLE_SCRIPT_DEBUGGING
        bool isEnumeratingRecyclerObjects; // Indicates this scriptContext is enumerating recycler objects. Used by recycler enumerating callbacks to filter out other unrelated scriptContexts.
#endif
        bool m_enumerateNonUserFunctionsOnly; // Indicates that recycler enumeration callback will consider only non-user functions (which are built-ins, external, winrt etc).

        ThreadContext* threadContext;
        TypeId  directHostTypeId;

        InlineCache * valueOfInlineCache;
        InlineCache * toStringInlineCache;

        typedef JsUtil::BaseHashSet<Js::PropertyId, ArenaAllocator> PropIdSetForConstProp;
        PropIdSetForConstProp * intConstPropsOnGlobalObject;
        PropIdSetForConstProp * intConstPropsOnGlobalUserObject;

        void * firstInterpreterFrameReturnAddress;
#ifdef SEPARATE_ARENA
        ArenaAllocator sourceCodeAllocator;
        ArenaAllocator regexAllocator;
#endif
#ifdef NEED_MISC_ALLOCATOR
        ArenaAllocator miscAllocator;
#endif

#if DBG
        JsUtil::BaseHashSet<void *, ArenaAllocator> bindRef;
        int m_iProfileSession;
#endif

#ifdef PROFILE_EXEC
        ScriptContextProfiler * profiler;
        bool isProfilerCreated;
        bool disableProfiler;
        bool ensureParentInfo;

        Profiler * CreateProfiler();
#endif
#ifdef PROFILE_MEM
        bool profileMemoryDump;
#endif
#ifdef PROFILE_STRINGS
        StringProfiler* stringProfiler;
#endif


public:
#ifdef PROFILE_TYPES
        int convertNullToSimpleCount;
        int convertNullToSimpleDictionaryCount;
        int convertNullToDictionaryCount;
        int convertDeferredToDictionaryCount;
        int convertDeferredToSimpleDictionaryCount;
        int convertSimpleToDictionaryCount;
        int convertSimpleToSimpleDictionaryCount;
        int convertPathToDictionaryCount1;
        int convertPathToDictionaryCount2;
        int convertPathToDictionaryCount3;
        int convertPathToDictionaryCount4;
        int convertPathToSimpleDictionaryCount;
        int convertSimplePathToPathCount;
        int convertSimpleDictionaryToDictionaryCount;
        int convertSimpleSharedDictionaryToNonSharedCount;
        int convertSimpleSharedToNonSharedCount;
        int simplePathTypeHandlerCount;
        int pathTypeHandlerCount;
        int promoteCount;
        int cacheCount;
        int branchCount;
        int maxPathLength;
        int typeCount[TypeIds_Limit];
        int instanceCount[TypeIds_Limit];
#endif


#ifdef PROFILE_BAILOUT_RECORD_MEMORY
        __int64 bailOutRecordBytes;
        __int64 bailOutOffsetBytes;
        __int64 codeSize;
#endif

#ifdef  PROFILE_OBJECT_LITERALS
        int objectLiteralInstanceCount;
        int objectLiteralPathCount;
        int objectLiteralCount[TypePath::MaxPathTypeHandlerLength];
        int objectLiteralSimpleDictionaryCount;
        uint32 objectLiteralMaxLength;
        int objectLiteralPromoteCount;
        int objectLiteralCacheCount;
        int objectLiteralBranchCount;
#endif
#if DBG_DUMP
        uint byteCodeDataSize;
        uint byteCodeAuxiliaryDataSize;
        uint byteCodeAuxiliaryContextDataSize;
        uint byteCodeHistogram[static_cast<uint>(OpCode::ByteCodeLast)];
        uint32 forinCache;
        uint32 forinNoCache;
#endif
#ifdef BGJIT_STATS
        uint interpretedCount;
        uint funcJITCount;
        uint loopJITCount;
        uint bytecodeJITCount;
        uint interpretedCallsHighPri;
        uint maxFuncInterpret;
        uint jitCodeUsed;
        uint funcJitCodeUsed;
        uint speculativeJitCount;
#endif
#if DBG
        // Count how many Out of Memory and Stack overflow exceptions happened during the execution
        uint oomExceptionCount = 0;
        uint soExceptionCount = 0;
#endif

#ifdef REJIT_STATS
        // Used to store bailout stats
        typedef JsUtil::BaseDictionary<uint, uint, ArenaAllocator> BailoutStatsMap;

        struct RejitStats
        {
            uint *m_rejitReasonCounts;
            BailoutStatsMap* m_bailoutReasonCounts;

            uint  m_totalRejits;
            uint  m_totalBailouts;

            RejitStats(ScriptContext *scriptContext) : m_totalRejits(0), m_totalBailouts(0)
            {
                m_rejitReasonCounts = AnewArrayZ(scriptContext->GeneralAllocator(), uint, NumRejitReasons);
                m_bailoutReasonCounts = Anew(scriptContext->GeneralAllocator(), BailoutStatsMap, scriptContext->GeneralAllocator());
            }
        };

        void LogDataForFunctionBody(Js::FunctionBody *body, uint idx, bool isRejit);

        void LogRejit(Js::FunctionBody *body, RejitReason reason);
        void LogBailout(Js::FunctionBody *body, uint kind);

        // Used to centrally collect stats for all function bodies.
        typedef JsUtil::WeaklyReferencedKeyDictionary<const Js::FunctionBody, RejitStats*> RejitStatsMap;
        RejitStatsMap* rejitStatsMap;

        BailoutStatsMap *bailoutReasonCounts;
        BailoutStatsMap *bailoutReasonCountsCap;
        uint *rejitReasonCounts;
        uint *rejitReasonCountsCap;
        void ClearBailoutReasonCountsMap();
        void ClearRejitReasonCountsArray();
#endif
#ifdef ENABLE_BASIC_TELEMETRY

    private:
        ScriptContextTelemetry* telemetry;
    public:
        ScriptContextTelemetry& GetTelemetry();
        bool HasTelemetry();

#endif
#ifdef INLINE_CACHE_STATS
        // This is a strongly referenced dictionary, since we want to know hit rates for dead caches.
        typedef JsUtil::BaseDictionary<const Js::PolymorphicInlineCache*, InlineCacheData*, Recycler> CacheDataMap;
        CacheDataMap *cacheDataMap;
        void LogCacheUsage(Js::PolymorphicInlineCache *cache, bool isGet, Js::PropertyId propertyId, bool hit, bool collision);
#endif

#ifdef FIELD_ACCESS_STATS
        typedef SList<FieldAccessStatsPtr, Recycler> FieldAccessStatsList;

        struct FieldAccessStatsEntry
        {
            Field(RecyclerWeakReference<FunctionBody>*) functionBodyWeakRef;
            Field(FieldAccessStatsList) stats;

            FieldAccessStatsEntry(RecyclerWeakReference<FunctionBody>* functionBodyWeakRef, Recycler* recycler)
                : functionBodyWeakRef(functionBodyWeakRef), stats(recycler) {}
        };

        typedef JsUtil::BaseDictionary<uint, FieldAccessStatsEntry*, Recycler> FieldAccessStatsByFunctionNumberMap;

        FieldAccessStatsByFunctionNumberMap* fieldAccessStatsByFunctionNumber;

        void RecordFieldAccessStats(FunctionBody* functionBody, FieldAccessStatsPtr fieldAccessStats);
#endif

#ifdef MISSING_PROPERTY_STATS
        int missingPropertyMisses;
        int missingPropertyHits;
        int missingPropertyCacheAttempts;

        void RecordMissingPropertyMiss();
        void RecordMissingPropertyHit();
        void RecordMissingPropertyCacheAttempt();
#endif

        bool IsIntConstPropertyOnGlobalObject(Js::PropertyId propId);
        void TrackIntConstPropertyOnGlobalObject(Js::PropertyId propId);
        bool IsIntConstPropertyOnGlobalUserObject(Js::PropertyId propertyId);
        void TrackIntConstPropertyOnGlobalUserObject(Js::PropertyId propertyId);

private:
        //
        // Regex globals
        //
#if ENABLE_REGEX_CONFIG_OPTIONS
        UnifiedRegex::DebugWriter* regexDebugWriter;
        UnifiedRegex::RegexStatsDatabase* regexStatsDatabase;
#endif
        UnifiedRegex::TrigramAlphabet* trigramAlphabet;
        UnifiedRegex::RegexStacks *regexStacks;

        JsUtil::Stack<Var>* operationStack;
        Recycler* recycler;
        RecyclerJavascriptNumberAllocator numberAllocator;

        ScriptConfiguration config;
        CharClassifier *charClassifier;

        // DisableJIT-TODO: Switch this to Dynamic thunk ifdef instead
#if ENABLE_NATIVE_CODEGEN
#if DYNAMIC_INTERPRETER_THUNK
        InterpreterThunkEmitter* interpreterThunkEmitter;
#endif
        BackgroundParser *backgroundParser;
#ifdef ASMJS_PLAT
        InterpreterThunkEmitter* asmJsInterpreterThunkEmitter;
        AsmJsCodeGenerator* asmJsCodeGenerator;
        typedef JsUtil::BaseDictionary<void *, SList<AsmJsScriptFunction *>*, ArenaAllocator> AsmFunctionMap;
        AsmFunctionMap* asmJsEnvironmentMap;
        ArenaAllocator* debugTransitionAlloc;
#endif
        NativeCodeGenerator* nativeCodeGen;
#endif

        DateTime::DaylightTimeHelper daylightTimeHelper;
        DateTime::Utility dateTimeUtility;

public:
        inline const WCHAR *const GetStandardName(size_t *nameLength, DateTime::YMD *ymd = NULL)
        {
            return dateTimeUtility.GetStandardName(nameLength, ymd);
        }

        inline const WCHAR *const GetDaylightName(size_t *nameLength, DateTime::YMD *ymd = NULL)
        {
            return dateTimeUtility.GetDaylightName(nameLength, ymd);
        }

private:
        HostScriptContext * hostScriptContext;
        HaltCallback* scriptEngineHaltCallback;
        EventHandler scriptStartEventHandler;
        EventHandler scriptEndEventHandler;
#ifdef FAULT_INJECTION
        EventHandler disposeScriptByFaultInjectionEventHandler;
#endif

        JsUtil::BaseDictionary<uint, JavascriptString *, ArenaAllocator> integerStringMap;

        double lastNumberToStringRadix10;
        double lastUtcTimeFromStr;

#if ENABLE_PROFILE_INFO
        bool referencesSharedDynamicSourceContextInfo;
#endif

        // We could delay the actual close after callRootLevel is 0.
        // this is to indicate the actual close is called once only.
        bool isScriptContextActuallyClosed;
        bool isFinalized;
        bool isEvalRestricted;
#if DBG
        bool isInitialized;
        bool isCloningGlobal;
#endif
        bool fastDOMenabled;
        bool hasUsedInlineCache;
        bool hasProtoOrStoreFieldInlineCache;
        bool hasIsInstInlineCache;
        bool deferredBody;
        bool isPerformingNonreentrantWork;
        bool isDiagnosticsScriptContext;   // mentions that current script context belongs to the diagnostics OM.

        size_t sourceSize;

        void CleanSourceListInternal(bool calledDuringMark);
        typedef JsUtil::List<RecyclerWeakReference<Utf8SourceInfo>*, Recycler, false, Js::FreeListedRemovePolicy> SourceList;
        RecyclerRootPtr<SourceList> sourceList;

#ifdef ENABLE_SCRIPT_DEBUGGING
        typedef void(*RaiseMessageToDebuggerFunctionType)(ScriptContext *, DEBUG_EVENT_INFO_TYPE, LPCWSTR, LPCWSTR);
        RaiseMessageToDebuggerFunctionType raiseMessageToDebuggerFunctionType;

        typedef void(*TransitionToDebugModeIfFirstSourceFn)(ScriptContext *, Utf8SourceInfo *);
        TransitionToDebugModeIfFirstSourceFn transitionToDebugModeIfFirstSourceFn;

        DebugContext* debugContext;
        CriticalSection debugContextCloseCS;
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        IActiveScriptProfilerHeapEnum* heapEnum;

        // Profiler Probes
        // In future these can be list of callbacks ?
        // Profiler Callback information
        IActiveScriptProfilerCallback *m_pProfileCallback;
        BOOL m_fTraceFunctionCall;
        BOOL m_fTraceNativeFunctionCall;
        DWORD m_dwEventMask;

        IActiveScriptProfilerCallback2 *m_pProfileCallback2;
        BOOL m_fTraceDomCall;
        BOOL m_inProfileCallback;
#endif // ENABLE_SCRIPT_PROFILING

        // List of weak reference dictionaries. We'll walk through them
        // and clean them up post-collection
        // IWeakReferenceDictionary objects are added to this list by calling
        // RegisterWeakReferenceDictionary. If you use JsUtil::WeakReferenceDictionary,
        // which also exposes the IWeakReferenceDictionary interface, it'll
        // automatically register the dictionary on the script context
        SListBase<JsUtil::IWeakReferenceDictionary*> weakReferenceDictionaryList;
        bool isWeakReferenceDictionaryListCleared;

        ScriptContext(ThreadContext* threadContext);
        void InitializeAllocations();
        void InitializePreGlobal();
        void InitializePostGlobal();
        void InitializeCache();

        bool isDebugContextInitialized;

        // Source Info
        void EnsureSourceContextInfoMap();
        void EnsureDynamicSourceContextInfoMap();

        void AddToEvalMapHelper(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction *pFuncScript);

        uint moduleSrcInfoCount;
#ifdef RUNTIME_DATA_COLLECTION
        time_t createTime;
#endif
        char16 const * url;

        void PrintStats();
        BOOL LeaveScriptStartCore(void * frameAddress, bool leaveForHost);

        void InternalClose();

    public:

#ifdef LEAK_REPORT
        LeakReport::UrlRecord * urlRecord;
        bool isRootTrackerScriptContext;
#endif

        DateTime::DaylightTimeHelper *GetDaylightTimeHelper() { return &daylightTimeHelper; }
        DateTime::Utility *GetDateUtility() { return &dateTimeUtility; }

        virtual bool IsClosed() const override { return isClosed; }
        void SetIsClosed();

        bool IsFinalized() const { return isFinalized; }
        void SetIsFinalized() { isFinalized = true; }
        bool IsActuallyClosed() const { return isScriptContextActuallyClosed; }
        void SetEvalRestriction(bool set) { this->isEvalRestricted = set; }
        bool IsEvalRestriction() const { return this->isEvalRestricted; }
#if ENABLE_NATIVE_CODEGEN
        bool IsClosedNativeCodeGenerator() const
        {
            return !nativeCodeGen || ::IsClosedNativeCodeGenerator(nativeCodeGen);
        }
#endif

        void SetHasUsedInlineCache(bool value)
        {
            hasUsedInlineCache = value;
#if DBG
            if(hasUsedInlineCache)
            {
                inlineCacheAllocator.Unlock();
            }
#endif
        }

        void SetDirectHostTypeId(TypeId typeId) {directHostTypeId = typeId; }
        TypeId GetDirectHostTypeId() const { return directHostTypeId; }

#if ENABLE_NATIVE_CODEGEN
        PSCRIPTCONTEXT_HANDLE GetRemoteScriptAddr(bool allowInitialize = true)
        {
#if ENABLE_OOP_NATIVE_CODEGEN
            if (!m_remoteScriptContextAddr && allowInitialize)
            {
                InitializeRemoteScriptContext();
            }
#endif
            return m_remoteScriptContextAddr;
        }
#endif

        char16 const * GetUrl() const { return url; }
        void SetUrl(BSTR bstr);
#ifdef RUNTIME_DATA_COLLECTION
        time_t GetCreateTime() const { return createTime; }
        uint GetAllocId() const { return allocId; }
#endif

#ifdef HEAP_ENUMERATION_VALIDATION
        bool IsInitialized() { return this->isInitialized; }
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
        bool IsDebugContextInitialized() const { return this->isDebugContextInitialized; }
        DebugContext* GetDebugContext() const;
        CriticalSection* GetDebugContextCloseCS() { return &debugContextCloseCS; }
#endif

        uint callCount;

        // if the current context is for webworker
        DWORD webWorkerId;

        static ScriptContext * New(ThreadContext * threadContext);

        ~ScriptContext();

#ifdef PROFILE_TYPES
        void ProfileTypes();
#endif

#ifdef PROFILE_OBJECT_LITERALS
        void ProfileObjectLiteral();
#endif

        //
        // Regex helpers
        //
#if ENABLE_REGEX_CONFIG_OPTIONS
        UnifiedRegex::RegexStatsDatabase* GetRegexStatsDatabase();
        UnifiedRegex::DebugWriter* GetRegexDebugWriter();
#endif
        RegexPatternMruMap* GetDynamicRegexMap() const;

        UnifiedRegex::TrigramAlphabet* GetTrigramAlphabet() { return trigramAlphabet; }

        void SetTrigramAlphabet(UnifiedRegex::TrigramAlphabet * trigramAlphabet);

        UnifiedRegex::RegexStacks *RegexStacks();
        UnifiedRegex::RegexStacks *AllocRegexStacks();
        UnifiedRegex::RegexStacks *SaveRegexStacks();
        void RestoreRegexStacks(UnifiedRegex::RegexStacks *const contStack);

        void InitializeGlobalObject();
        bool IsIntlEnabled();
        JavascriptLibrary* GetLibrary() const { return javascriptLibrary; }
        Js::Cache* Cache() const{ return &this->javascriptLibrary->cache; }
        const JavascriptLibraryBase* GetLibraryBase() const { return javascriptLibrary->GetLibraryBase(); }
#if DBG
        BOOL IsCloningGlobal() const { return isCloningGlobal;}
#endif
        void PushObject(Var object);
        Var PopObject();
        BOOL CheckObject(Var object);

        inline bool IsHeapEnumInProgress() { return GetRecycler()->IsHeapEnumInProgress(); }

        bool IsInterpreted() { return config.IsNoNative(); }
        void ForceNoNative() { config.ForceNoNative(); }
        void ForceNative() { config.ForceNative(); }
        ScriptConfiguration const * GetConfig(void) const { return &config; }
        CharClassifier const * GetCharClassifier(void) const;

        ThreadContext * GetThreadContext() const { return threadContext; }

        static const int MaxEvalSourceSize = 400;

        bool IsInEvalMap(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction **ppFuncScript);
        void AddToEvalMap(FastEvalMapString const& key, BOOL isIndirect, ScriptFunction *pFuncScript);

        template <typename TCacheType>
        void CleanDynamicFunctionCache(TCacheType* cacheType);
        void CleanEvalMapCache(Js::EvalCacheTopLevelDictionary * cacheType);

        template <class TDelegate>
        void MapFunction(TDelegate mapper);

        template <class TDelegate>
        FunctionBody* FindFunction(TDelegate predicate);

        inline bool EnableEvalMapCleanup() { return CONFIG_FLAG(EnableEvalMapCleanup); };
        uint GetNextSourceContextId();

        bool IsInNewFunctionMap(EvalMapString const& key, FunctionInfo **ppFuncInfo);
        void AddToNewFunctionMap(EvalMapString const& key, FunctionInfo *pFuncInfo);

        SourceContextInfo * GetSourceContextInfo(DWORD_PTR hostSourceContext, IActiveScriptDataCache* profileDataCache);
        SourceContextInfo * GetSourceContextInfo(uint hash);
        SourceContextInfo * CreateSourceContextInfo(uint hash, DWORD_PTR hostSourceContext);
        SourceContextInfo * CreateSourceContextInfo(DWORD_PTR hostSourceContext, char16 const * url, size_t len,
            IActiveScriptDataCache* profileDataCache, char16 const * sourceMapUrl = nullptr, size_t sourceMapUrlLen = 0);

#if defined(LEAK_REPORT) || defined(CHECK_MEMORY_LEAK)
        void ClearSourceContextInfoMaps()
        {
#if ENABLE_PROFILE_INFO
              this->referencesSharedDynamicSourceContextInfo = false;
#endif
        }
#endif

#if ENABLE_PROFILE_INFO
#if DBG_DUMP || defined(DYNAMIC_PROFILE_STORAGE) || defined(RUNTIME_DATA_COLLECTION)
        void ClearDynamicProfileList()
        {
            if (this->Cache()->profileInfoList)
            {
                this->Cache()->profileInfoList->Reset();
                this->Cache()->profileInfoList = nullptr;
            }
        }

        DynamicProfileInfoList * GetProfileInfoList() { return this->Cache()->profileInfoList; }
#endif
#endif

        SRCINFO const * GetModuleSrcInfo(Js::ModuleID moduleID);
        SourceContextInfoMap* GetSourceContextInfoMap()
        {
            return this->Cache()->sourceContextInfoMap;
        }
        DynamicSourceContextInfoMap* GetDynamicSourceContextInfoMap()
        {
            return this->Cache()->dynamicSourceContextInfoMap;
        }

#if ENABLE_TTD
        //The host callback functor info
        HostScriptContextCallbackFunctor TTDHostCallbackFunctor;

        //The LogTag for this script context (the same as the tag for the global object but we put it here for fast lookup)
        TTD_LOG_PTR_ID ScriptContextLogTag;

        //Info about the core image for the context
        TTD::RuntimeContextInfo* TTDWellKnownInfo;

        //Additional runtime context that we only care about if TTD is running (or will be running)
        TTD::ScriptContextTTD* TTDContextInfo;

        //A flag indicating a TTDSnapshot is in progress
        bool TTDSnapshotOrInflateInProgress;

        //Check if a snapshot is in progress on this context
        bool IsTTDSnapshotOrInflateInProgress() const { return this->TTDSnapshotOrInflateInProgress; }

        //Memoized results for frequently used Record/Replay action checks
        bool TTDRecordOrReplayModeEnabled;
        bool TTDRecordModeEnabled;
        bool TTDReplayModeEnabled;

        bool TTDShouldPerformRecordOrReplayAction;
        bool TTDShouldPerformRecordAction;
        bool TTDShouldPerformReplayAction;

        bool TTDShouldPerformDebuggerAction;
        bool TTDShouldSuppressGetterInvocationForDebuggerEvaluation;

        //Check if the TTD system has been activated (and record/replay may or may not be enabled)
        bool IsTTDRecordOrReplayModeEnabled() const { return this->TTDRecordOrReplayModeEnabled; }

        //Check if the TTD Record system has been activated (and  may or may not be enabled)
        bool IsTTDRecordModeEnabled() const { return this->TTDRecordModeEnabled; }

        //Check if the TTD Replay system has been activated (and  may or may not be enabled)
        bool IsTTDReplayModeEnabled() const { return this->TTDReplayModeEnabled; }

        //Check if we are in (record OR replay) AND this code is being run on behalf of the user application
        bool ShouldPerformRecordOrReplayAction() const { return this->TTDShouldPerformRecordOrReplayAction; }

        //Use this to check specifically if we are in record AND this code is being run on behalf of the user application
        bool ShouldPerformRecordAction() const { return this->TTDShouldPerformRecordAction; }

        //Use this to check specifically if we are in replay mode AND this code is being run on behalf of the user application
        bool ShouldPerformReplayAction() const { return this->TTDShouldPerformReplayAction; }

        //Use this to check specifically if we are in debugging mode AND this code is being run on behalf of the user application
        bool ShouldPerformDebuggerAction() const { return this->TTDShouldPerformDebuggerAction; }

        //A special check to see if we are debugging and want to suppress the execution of getters (which may be triggered by displaying values in the debugger)
        bool ShouldSuppressGetterInvocationForDebuggerEvaluation() const { return this->TTDShouldSuppressGetterInvocationForDebuggerEvaluation; }

        //
        //TODO: this is currently called explicitly -- we need to fix up the core image computation and this will be eliminated then
        //
        //Initialize the core object image for TTD
        void InitializeCoreImage_TTD();
#endif

        void SetFirstInterpreterFrameReturnAddress(void * returnAddress) { firstInterpreterFrameReturnAddress = returnAddress;}
        void *GetFirstInterpreterFrameReturnAddress() { return firstInterpreterFrameReturnAddress;}

        void CleanupWeakReferenceDictionaries();

        void Initialize();
        bool Close(bool inDestructor);
        void MarkForClose();
#ifdef ENABLE_PROJECTION
        void SetHostType(int32 hostType) { config.SetHostType(hostType); }
        void SetWinRTConstructorAllowed(bool allowed) { config.SetWinRTConstructorAllowed(allowed); }
        void SetProjectionTargetVersion(DWORD version) { config.SetProjectionTargetVersion(version); }
#endif
        void SetCanOptimizeGlobalLookupFlag(BOOL f){ config.SetCanOptimizeGlobalLookupFlag(f);}
        BOOL CanOptimizeGlobalLookup(){ return config.CanOptimizeGlobalLookup();}

        bool IsFastDOMEnabled() { return fastDOMenabled; }
        void SetFastDOMenabled();
        BOOL VerifyAlive(BOOL isJSFunction = FALSE, ScriptContext* requestScriptContext = nullptr);
        void VerifyAliveWithHostContext(BOOL isJSFunction, HostScriptContext* requestHostScriptContext);
        void AddFunctionBodyToPropIdMap(FunctionBody* body);

        void BindReference(void* addr);

        void InitPropertyStringMap(int i);
        PropertyString* AddPropertyString2(const Js::PropertyRecord* propertyRecord);
        PropertyString* CachePropertyString2(const Js::PropertyRecord* propertyRecord);
        PropertyString* GetPropertyString2(char16 ch1, char16 ch2);
        void FindPropertyRecord(__in LPCWSTR pszPropertyName, __in int propertyNameLength, PropertyRecord const** propertyRecord);
        JsUtil::List<const RecyclerWeakReference<Js::PropertyRecord const>*>* FindPropertyIdNoCase(__in LPCWSTR pszPropertyName, __in int propertyNameLength);

        void FindPropertyRecord(JavascriptString* pstName, PropertyRecord const** propertyRecord);
        PropertyRecord const * GetPropertyName(PropertyId propertyId);
        PropertyRecord const * GetPropertyNameLocked(PropertyId propertyId);
        void GetOrAddPropertyRecord(JsUtil::CharacterBuffer<WCHAR> const& propName, PropertyRecord const** propertyRecord);
        template <size_t N> void GetOrAddPropertyRecord(const char16(&propertyName)[N], PropertyRecord const** propertyRecord)
        {
            GetOrAddPropertyRecord(propertyName, N - 1, propertyRecord);
        }
        PropertyId GetOrAddPropertyIdTracked(JsUtil::CharacterBuffer<WCHAR> const& propName);
        template <size_t N> PropertyId GetOrAddPropertyIdTracked(const char16(&propertyName)[N])
        {
            return GetOrAddPropertyIdTracked(propertyName, N - 1);
        }
        PropertyId GetOrAddPropertyIdTracked(__in_ecount(propertyNameLength) LPCWSTR pszPropertyName, __in int propertyNameLength);
        void GetOrAddPropertyRecord(__in_ecount(propertyNameLength) LPCWSTR pszPropertyName, __in int propertyNameLength, PropertyRecord const** propertyRecord);
        BOOL IsNumericPropertyId(PropertyId propertyId, uint32* value);

        void RegisterWeakReferenceDictionary(JsUtil::IWeakReferenceDictionary* weakReferenceDictionary);
        void ResetWeakReferenceDictionaryList() { weakReferenceDictionaryList.Reset(); }

        BOOL ReserveStaticTypeIds(__in int first, __in int last);
        TypeId ReserveTypeIds(int count);
        TypeId CreateTypeId();

        template<WellKnownHostType wellKnownType>
        bool IsWellKnownHostType(Js::TypeId typeId) { return threadContext->IsWellKnownHostType<wellKnownType>(typeId); }
        void SetWellKnownHostTypeId(WellKnownHostType wellKnownType, Js::TypeId typeId) { threadContext->SetWellKnownHostTypeId(wellKnownType, typeId); }

        ParseNodePtr ParseScript(Parser* parser, const byte* script,
            size_t cb, SRCINFO const * pSrcInfo,
            CompileScriptException * pse, Utf8SourceInfo** ppSourceInfo,
            const char16 *rootDisplayName, LoadScriptFlag loadScriptFlag,
            uint* sourceIndex, Js::Var scriptSource = nullptr);

        JavascriptFunction* LoadScript(const byte* script, size_t cb,
            SRCINFO const * pSrcInfo,
            CompileScriptException * pse, Utf8SourceInfo** ppSourceInfo,
            const char16 *rootDisplayName, LoadScriptFlag loadScriptFlag,
            Js::Var scriptSource = nullptr);

        ArenaAllocator* GeneralAllocator() { return &generalAllocator; }

#ifdef ENABLE_BASIC_TELEMETRY
        ArenaAllocator* TelemetryAllocator() { return &telemetryAllocator; }
#endif

#ifdef SEPARATE_ARENA
        ArenaAllocator* SourceCodeAllocator() { return &sourceCodeAllocator; }
        ArenaAllocator* RegexAllocator() { return &regexAllocator; }
#else
        ArenaAllocator* SourceCodeAllocator() { return &generalAllocator; }
        ArenaAllocator* RegexAllocator() { return &generalAllocator; }
#endif
#ifdef NEED_MISC_ALLOCATOR
        ArenaAllocator* MiscAllocator() { return &miscAllocator; }
#endif
        InlineCacheAllocator* GetInlineCacheAllocator() { return &inlineCacheAllocator; }
        CacheAllocator* GetIsInstInlineCacheAllocator() { return &isInstInlineCacheAllocator; }
        CacheAllocator * ForInCacheAllocator() { return &forInCacheAllocator; }
        ArenaAllocator* DynamicProfileInfoAllocator() { return &dynamicProfileInfoAllocator; }

#ifdef ENABLE_SCRIPT_DEBUGGING
        ArenaAllocator* AllocatorForDiagnostics();
#endif

        Js::TempArenaAllocatorObject* GetTemporaryAllocator(LPCWSTR name);
        void ReleaseTemporaryAllocator(Js::TempArenaAllocatorObject* tempAllocator);
        Js::TempGuestArenaAllocatorObject* GetTemporaryGuestAllocator(LPCWSTR name);
        void ReleaseTemporaryGuestAllocator(Js::TempGuestArenaAllocatorObject* tempAllocator);

        bool EnsureInterpreterArena(ArenaAllocator **);
        void ReleaseInterpreterArena();

        ArenaAllocator* GetGuestArena() const
        {
            return guestArena;
        }

        void ReleaseGuestArena();

        Recycler* GetRecycler() const { return recycler; }
        RecyclerJavascriptNumberAllocator * GetNumberAllocator() { return &numberAllocator; }
#if ENABLE_NATIVE_CODEGEN
        NativeCodeGenerator * GetNativeCodeGenerator() const { return nativeCodeGen; }
#endif
#if ENABLE_BACKGROUND_PARSING
        BackgroundParser * GetBackgroundParser() const { return backgroundParser; }
#endif

        void OnScriptStart(bool isRoot, bool isScript);
        void OnScriptEnd(bool isRoot, bool isForcedEnd);

        template <bool stackProbe, bool leaveForHost>
        bool LeaveScriptStart(void * frameAddress);
        template <bool leaveForHost>
        void LeaveScriptEnd(void * frameAddress);

        HostScriptContext * GetHostScriptContext() const { return hostScriptContext; }
        void SetHostScriptContext(HostScriptContext *  hostScriptContext);
        void SetScriptEngineHaltCallback(HaltCallback* scriptEngine);
        void ClearHostScriptContext();

        IActiveScriptProfilerHeapEnum* GetHeapEnum();
        void SetHeapEnum(IActiveScriptProfilerHeapEnum* newHeapEnum);
        void ClearHeapEnum();

        void SetScriptStartEventHandler(EventHandler eventHandler);
        void SetScriptEndEventHandler(EventHandler eventHandler);
#ifdef FAULT_INJECTION
        void DisposeScriptContextByFaultInjection();
        void SetDisposeDisposeByFaultInjectionEventHandler(EventHandler eventHandler);
#endif
        EnumeratedObjectCache* GetEnumeratedObjectCache() { return &(this->Cache()->enumObjCache); }
        PropertyString* TryGetPropertyString(PropertyId propertyId);
        PropertyString* GetPropertyString(PropertyId propertyId);
        void InvalidatePropertyStringCache(PropertyId propertyId, Type* type);
        JavascriptString* GetIntegerString(Var aValue);
        JavascriptString* GetIntegerString(int value);
        JavascriptString* GetIntegerString(uint value);

        void CheckEvalRestriction();

        RecyclableObject* GetMissingPropertyResult();
        RecyclableObject* GetMissingItemResult();

        bool HasRecordedException() const { return threadContext->GetRecordedException() != nullptr; }
        Js::JavascriptExceptionObject * GetAndClearRecordedException(bool *considerPassingToDebugger = nullptr);
        void RecordException(Js::JavascriptExceptionObject * exceptionObject, bool propagateToDebugger = false);
        __declspec(noreturn) void RethrowRecordedException(JavascriptExceptionObject::HostWrapperCreateFuncType hostWrapperCreateFunc);

#if ENABLE_NATIVE_CODEGEN
        BOOL IsNativeAddress(void * codeAddr);
#endif

        uint SaveSourceCopy(Utf8SourceInfo* sourceInfo, int cchLength, bool isCesu8);
        bool SaveSourceCopy(Utf8SourceInfo* const sourceInfo, int cchLength, bool isCesu8, uint * index);

        uint SaveSourceNoCopy(Utf8SourceInfo* sourceInfo, int cchLength, bool isCesu8);

        void CloneSources(ScriptContext* sourceContext);
        Utf8SourceInfo* GetSource(uint sourceIndex);

        uint SourceCount() const { return (uint)sourceList->Count(); }
        void CleanSourceList() { CleanSourceListInternal(false); }
        SourceList* GetSourceList() const { return sourceList; }
        bool IsItemValidInSourceList(int index);

        template <typename TFunction>
        void MapScript(TFunction mapper)
        {
            this->sourceList->Map([mapper] (int, RecyclerWeakReference<Utf8SourceInfo>* sourceInfoWeakReference)
            {
                Utf8SourceInfo* strongRef = sourceInfoWeakReference->Get();

                if (strongRef)
                {
                    mapper(strongRef);
                }
            });
        }

#ifdef CHECK_STACKWALK_EXCEPTION
        void SetIgnoreStackWalkException() {threadContext->GetScriptEntryExit()->ignoreStackWalkException = true; }
#endif

        // For debugging scenarios where execution will go to debugging manager and come back to engine again, enforce the current EER to have
        // 'hasCaller' property set, which will enable the stack walking across frames.
        // Do not call this directly, look for ENFORCE_ENTRYEXITRECORD_HASCALLER macro.
        void EnforceEERHasCaller() { threadContext->GetScriptEntryExit()->hasCaller = true; }

#ifdef ENABLE_SCRIPT_DEBUGGING
        void SetRaiseMessageToDebuggerFunction(RaiseMessageToDebuggerFunctionType function)
        {
            raiseMessageToDebuggerFunctionType = function;
        }

        void RaiseMessageToDebugger(DEBUG_EVENT_INFO_TYPE messageType, LPCWSTR message, LPCWSTR url)
        {
            if (raiseMessageToDebuggerFunctionType != nullptr)
            {
                raiseMessageToDebuggerFunctionType(this, messageType, message, url);
            }
        }

        void SetTransitionToDebugModeIfFirstSourceFn(TransitionToDebugModeIfFirstSourceFn function)
        {
            transitionToDebugModeIfFirstSourceFn = function;
        }

        void TransitionToDebugModeIfFirstSource(Utf8SourceInfo *sourceInfo)
        {
            if (transitionToDebugModeIfFirstSourceFn != nullptr)
            {
                transitionToDebugModeIfFirstSourceFn(this, sourceInfo);
            }
        }
#endif

        void AddSourceSize(size_t sourceSize)
        {
            this->sourceSize += sourceSize;
            this->threadContext->AddSourceSize(sourceSize);
        }

        size_t GetSourceSize()
        {
            return this->sourceSize;
        }

        BOOL SetDeferredBody(BOOL set)
        {
            bool old = this->deferredBody;
            this->deferredBody = !!set;
            return old;
        }

        BOOL GetDeferredBody(void) const
        {
            return this->deferredBody;
        }

    private:
        Js::PropertyId emptyStringPropertyId;
    public:
        Js::PropertyId GetEmptyStringPropertyId()
        {
            if (emptyStringPropertyId == Js::PropertyIds::_none)
            {
                Js::PropertyRecord const * propertyRecord;
                this->GetOrAddPropertyRecord(_u(""), 0, &propertyRecord);
                emptyStringPropertyId = propertyRecord->GetPropertyId();
            }
            return emptyStringPropertyId;
        }

        void FreeFunctionEntryPoint(Js::JavascriptMethod codeAddress, Js::JavascriptMethod thunkAddress);

    private:
        uint CloneSource(Utf8SourceInfo* info);
    public:
        void RegisterProtoInlineCache(InlineCache *pCache, PropertyId propId);
        void InvalidateProtoCaches(const PropertyId propertyId);
        void InvalidateAllProtoCaches();
        void RegisterStoreFieldInlineCache(InlineCache *pCache, PropertyId propId);
        void InvalidateStoreFieldCaches(const PropertyId propertyId);
        void InvalidateAllStoreFieldCaches();
        void RegisterIsInstInlineCache(Js::IsInstInlineCache * cache, Js::Var function);
#if DBG
        bool IsIsInstInlineCacheRegistered(Js::IsInstInlineCache * cache, Js::Var function);
#endif
        void ClearInlineCaches();
        void ClearIsInstInlineCaches();
        void ClearForInCaches();
#ifdef PERSISTENT_INLINE_CACHES
        void ClearInlineCachesWithDeadWeakRefs();
#endif
        void ClearScriptContextCaches();
#if ENABLE_NATIVE_CODEGEN
        void RegisterConstructorCache(Js::PropertyId propertyId, Js::ConstructorCache* cache);
#endif

    public:
        void RegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext();
    private:
        void DoRegisterPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesScriptContext();
    public:
        void ClearPrototypeChainEnsuredToHaveOnlyWritableDataPropertiesCaches();

    public:
        JavascriptString * GetLastNumberToStringRadix10(double value);
        void SetLastNumberToStringRadix10(double value, JavascriptString * str);
        bool GetLastUtcTimeFromStr(JavascriptString * str, double& dbl);
        void SetLastUtcTimeFromStr(JavascriptString * str, double value);
        bool IsNoContextSourceContextInfo(SourceContextInfo *sourceContextInfo) const
        {
            return sourceContextInfo == this->Cache()->noContextSourceContextInfo;
        }

        BOOL IsProfiling()
        {
#ifdef ENABLE_SCRIPT_PROFILING
            return (m_pProfileCallback != nullptr);
#else
            return FALSE;
#endif
        }

        BOOL IsInProfileCallback()
        {
#ifdef ENABLE_SCRIPT_PROFILING
            return m_inProfileCallback;
#else
            return FALSE;
#endif
        }

        SRCINFO *AddHostSrcInfo(SRCINFO const *pSrcInfo);

#if DBG
        SourceContextInfo const * GetNoContextSourceContextInfo() const { return this->Cache()->noContextSourceContextInfo; }
        bool hadProfiled;
        bool HadProfiled() const { return hadProfiled; }

#ifdef ENABLE_SCRIPT_PROFILING
        int GetProfileSession()
        {
            AssertMsg(m_pProfileCallback != nullptr, "Asking for profile session when we aren't in one.");
            return m_iProfileSession;
        }

        void StartNewProfileSession()
        {
            AssertMsg(m_pProfileCallback != nullptr, "New Session when the profiler isn't set to any callback.");
            m_iProfileSession++;
        }

        void StopProfileSession()
        {
            AssertMsg(m_pProfileCallback == nullptr, "How to stop when there is still the callback out there");
        }
#endif // ENABLE_SCRIPT_PROFILING
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        void CoreSetProfileEventMask(DWORD dwEventMask);
        typedef HRESULT(*RegisterExternalLibraryType)(Js::ScriptContext *pScriptContext);
        HRESULT RegisterProfileProbe(IActiveScriptProfilerCallback *pProfileCallback, DWORD dwEventMask, DWORD dwContext, RegisterExternalLibraryType RegisterExternalLibrary, JavascriptMethod dispatchInvoke);
        HRESULT DeRegisterProfileProbe(HRESULT hrReason, JavascriptMethod dispatchInvoke);
        HRESULT RegisterLibraryFunction(const char16 *pwszObjectName, const char16 *pwszFunctionName, Js::PropertyId functionPropertyId, JavascriptMethod entryPoint);
        HRESULT RegisterBuiltinFunctions(RegisterExternalLibraryType RegisterExternalLibrary);
        HRESULT SetProfileEventMask(DWORD dwEventMask);

        HRESULT RegisterScript(Js::FunctionProxy *pFunctionBody, BOOL fRegisterScript = TRUE);

        // Register static and dynamic scripts
        HRESULT RegisterAllScripts();
#endif

#ifdef ENABLE_SCRIPT_DEBUGGING
        // Iterate through utf8sourceinfo and clear debug document if they are there.
        void EnsureClearDebugDocument();
        void UpdateRecyclerFunctionEntryPointsForDebugger();
        static void RecyclerFunctionCallbackForDebugger(void *address, size_t size);
        void SetFunctionInRecyclerToProfileMode(bool enumerateNonUserFunctionsOnly = false);
#ifdef ASMJS_PLAT
        void TransitionEnvironmentForDebugger(ScriptFunction * scriptFunction);
#endif
#endif

        // To be called directly only when the thread context is shutting down
        void ShutdownClearSourceLists();

#if defined(ENABLE_SCRIPT_PROFILING) || defined(ENABLE_SCRIPT_DEBUGGING)
        void RegisterDebugThunk(bool calledDuringAttach = true);
        void UnRegisterDebugThunk();
        static void SetEntryPointToProfileThunk(JavascriptFunction* function);
        static void RestoreEntryPointFromProfileThunk(JavascriptFunction* function);
        static void RecyclerEnumClassEnumeratorCallback(void *address, size_t size);
#endif

#if ENABLE_NATIVE_CODEGEN
        HRESULT RecreateNativeCodeGenerator();
#endif
        bool IsForceNoNative();

#ifdef ENABLE_SCRIPT_DEBUGGING
        HRESULT OnDebuggerAttached();
        HRESULT OnDebuggerDetached();
        HRESULT OnDebuggerAttachedDetached(bool attach);
        void InitializeDebugging();
        bool IsEnumeratingRecyclerObjects() const { return isEnumeratingRecyclerObjects; }
    private:
        class AutoEnumeratingRecyclerObjects
        {
        public:
            AutoEnumeratingRecyclerObjects(ScriptContext* scriptContext):
                m_scriptContext(scriptContext)
            {
                Assert(!m_scriptContext->IsEnumeratingRecyclerObjects());
                m_scriptContext->isEnumeratingRecyclerObjects = true;
            }

            ~AutoEnumeratingRecyclerObjects()
            {
                Assert(m_scriptContext->IsEnumeratingRecyclerObjects());
                m_scriptContext->isEnumeratingRecyclerObjects = false;
            }

        private:
            ScriptContext* m_scriptContext;
        };
#endif        

#ifdef EDIT_AND_CONTINUE
    private:
        ScriptEditQuery* activeScriptEditQuery;

        void BeginScriptEditEnumFunctions(ScriptEditQuery* scriptEditQuery) { Assert(!activeScriptEditQuery); activeScriptEditQuery = scriptEditQuery; }
        void EndScriptEditEnumFunctions() { Assert(activeScriptEditQuery); activeScriptEditQuery = nullptr; }
    public:
        ScriptEditQuery* GetActiveScriptEditQuery() const { return activeScriptEditQuery; }

        class AutoScriptEditEnumFunctions
        {
        public:
            AutoScriptEditEnumFunctions(ScriptContext* scriptContext, ScriptEditQuery* scriptEditQuery) : m_scriptContext(scriptContext)
            {
                scriptContext->BeginScriptEditEnumFunctions(scriptEditQuery);
            }
            ~AutoScriptEditEnumFunctions() { m_scriptContext->EndScriptEditEnumFunctions(); }
        private:
            ScriptContext* m_scriptContext;
        };
#endif

    private:
        typedef JsUtil::BaseDictionary<JavascriptMethod, Js::PropertyId, ArenaAllocator, PrimeSizePolicy> BuiltinFunctionIdDictionary;
        BuiltinFunctionIdDictionary *m_pBuiltinFunctionIdMap;
        Js::PropertyId GetFunctionNumber(JavascriptMethod entryPoint);

        static const char16* CopyString(const char16* str, size_t charCount, ArenaAllocator* alloc);

#ifdef ENABLE_JS_ETW
        static charcount_t AppendWithEscapeCharacters(Js::StringBuilder<ArenaAllocator>* stringBuilder, const WCHAR* sourceString, charcount_t sourceStringLen, WCHAR escapeChar, WCHAR charToEscape);
#endif

    public:
#if DYNAMIC_INTERPRETER_THUNK
        JavascriptMethod GetNextDynamicAsmJsInterpreterThunk(PVOID* ppDynamicInterpreterThunk);
        JavascriptMethod GetNextDynamicInterpreterThunk(PVOID* ppDynamicInterpreterThunk);
#if DBG
        BOOL IsDynamicInterpreterThunk(JavascriptMethod address);
#endif
        void ReleaseDynamicInterpreterThunk(BYTE* address, bool addtoFreeList);
        void ReleaseDynamicAsmJsInterpreterThunk(BYTE* address, bool addtoFreeList);
#endif

        static Var DebugProfileProbeThunk(RecyclableObject* function, CallInfo callInfo, ...);
        static JavascriptMethod ProfileModeDeferredParse(ScriptFunction **function);
        static Var ProfileModeDeferredParsingThunk(RecyclableObject* function, CallInfo callInfo, ...);

        // Thunks for deferred deserialization of function bodies from the byte code cache
        static JavascriptMethod ProfileModeDeferredDeserialize(ScriptFunction* function);
        static Var ProfileModeDeferredDeserializeThunk(RecyclableObject* function, CallInfo callInfo, ...);

#if defined(ENABLE_SCRIPT_DEBUGGING) || defined(ENABLE_SCRIPT_PROFILING)
        static Var ProfileModeThunk_DebugModeWrapper(JavascriptFunction* function, ScriptContext* scriptContext, JavascriptMethod entryPoint, Arguments& args);
        static JavascriptMethod GetProfileModeThunk(JavascriptMethod entryPoint);
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        void SetProfileMode(BOOL fSet);
        BOOL GetProfileInfo(
            JavascriptFunction* function,
            PROFILER_TOKEN &scriptId,
            PROFILER_TOKEN &functionId);

        HRESULT OnScriptCompiled(PROFILER_TOKEN scriptId, PROFILER_SCRIPT_TYPE type, IUnknown *pIDebugDocumentContext);
        HRESULT OnFunctionCompiled(
            PROFILER_TOKEN functionId,
            PROFILER_TOKEN scriptId,
            const WCHAR *pwszFunctionName,
            const WCHAR *pwszFunctionNameHint,
            IUnknown *pIDebugDocumentContext);
        HRESULT OnFunctionEnter(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId);
        HRESULT OnFunctionExit(PROFILER_TOKEN scriptId, PROFILER_TOKEN functionId);

        static HRESULT FunctionExitSenderThunk(PROFILER_TOKEN functionId, PROFILER_TOKEN scriptId, ScriptContext *pScriptContext);
        static HRESULT FunctionExitByNameSenderThunk(const char16 *pwszFunctionName, ScriptContext *pScriptContext);

        bool SetDispatchProfile(bool fSet, JavascriptMethod dispatchInvoke);
        HRESULT OnDispatchFunctionEnter(const WCHAR *pwszFunctionName);
        HRESULT OnDispatchFunctionExit(const WCHAR *pwszFunctionName);

#endif // ENABLE_SCRIPT_PROFILING
       
        void OnStartupComplete();
        void SaveStartupProfileAndRelease(bool isSaveOnClose = false);

#if ENABLE_PROFILE_INFO
        template<template<typename> class BarrierT>
        void AddDynamicProfileInfo(FunctionBody * functionBody, BarrierT<DynamicProfileInfo>& dynamicProfileInfo);
#endif
#if DBG || defined(RUNTIME_DATA_COLLECTION)
        uint allocId;
#endif

#ifdef PROFILE_EXEC
        void DisableProfiler();
        void SetRecyclerProfiler();
        void SetProfilerFromScriptContext(ScriptContext * scriptContext);
        void ProfileBegin(Js::Phase);
        void ProfileEnd(Js::Phase);
        void ProfileSuspend(Js::Phase, Js::Profiler::SuspendRecord * suspendRecord);
        void ProfileResume(Js::Profiler::SuspendRecord * suspendRecord);
        void ProfilePrint();
        bool IsProfilerCreated() const { return isProfilerCreated; }
#endif

#ifdef PROFILE_MEM
        void DisableProfileMemoryDumpOnDelete() { profileMemoryDump = false; }
#endif

#ifdef PROFILE_STRINGS
        StringProfiler * GetStringProfiler(); // May be null if string profiling not enabled
#endif

    public:
        virtual intptr_t GetNullAddr() const override;
        virtual intptr_t GetUndefinedAddr() const override;
        virtual intptr_t GetTrueAddr() const override;
        virtual intptr_t GetFalseAddr() const override;
        virtual intptr_t GetUndeclBlockVarAddr() const override;
        virtual intptr_t GetEmptyStringAddr() const override;
        virtual intptr_t GetNegativeZeroAddr() const override;
        virtual intptr_t GetNumberTypeStaticAddr() const override;
        virtual intptr_t GetStringTypeStaticAddr() const override;
        virtual intptr_t GetObjectTypeAddr() const override;
        virtual intptr_t GetObjectHeaderInlinedTypeAddr() const override;
        virtual intptr_t GetRegexTypeAddr() const override;
        virtual intptr_t GetArrayTypeAddr() const override;
        virtual intptr_t GetNativeIntArrayTypeAddr() const override;
        virtual intptr_t GetNativeFloatArrayTypeAddr() const override;
        virtual intptr_t GetArrayConstructorAddr() const override;
        virtual intptr_t GetCharStringCacheAddr() const override;
        virtual intptr_t GetSideEffectsAddr() const override;
        virtual intptr_t GetArraySetElementFastPathVtableAddr() const override;
        virtual intptr_t GetIntArraySetElementFastPathVtableAddr() const override;
        virtual intptr_t GetFloatArraySetElementFastPathVtableAddr() const override;
        virtual intptr_t GetLibraryAddr() const override;
        virtual intptr_t GetGlobalObjectAddr() const override;
        virtual intptr_t GetGlobalObjectThisAddr() const override;
        virtual intptr_t GetNumberAllocatorAddr() const override;
        virtual intptr_t GetRecyclerAddr() const override;
        virtual bool GetRecyclerAllowNativeCodeBumpAllocation() const override;
#ifdef ENABLE_SIMDJS
        virtual bool IsSIMDEnabled() const override;
#endif
        virtual bool IsPRNGSeeded() const override;
        virtual intptr_t GetBuiltinFunctionsBaseAddr() const override;

#ifdef ENABLE_SCRIPT_DEBUGGING
        virtual intptr_t GetDebuggingFlagsAddr() const override;
        virtual intptr_t GetDebugStepTypeAddr() const override;
        virtual intptr_t GetDebugFrameAddressAddr() const override;
        virtual intptr_t GetDebugScriptIdWhenSetAddr() const override;
#endif

#if ENABLE_NATIVE_CODEGEN
        virtual void AddToDOMFastPathHelperMap(intptr_t funcInfoAddr, IR::JnHelperMethod helper) override;
        virtual IR::JnHelperMethod GetDOMFastPathHelper(intptr_t funcInfoAddr) override;
#endif

        virtual intptr_t GetAddr() const override;

        virtual intptr_t GetVTableAddress(VTableValue vtableType) const override;

        virtual bool IsRecyclerVerifyEnabled() const override;
        virtual uint GetRecyclerVerifyPad() const override;

        virtual Field(Js::Var)* GetModuleExportSlotArrayAddress(uint moduleIndex, uint slotIndex) override;

        Js::SourceTextModuleRecord* GetModuleRecord(uint moduleId) const
        {
            return javascriptLibrary->GetModuleRecord(moduleId);
        }

        void SetBuiltInLibraryFunction(JavascriptMethod entryPoint, JavascriptFunction* function);
        JavascriptFunction* GetBuiltInLibraryFunction(JavascriptMethod entryPoint);

    private:
        BuiltInLibraryFunctionMap* builtInLibraryFunctions;

#if ENABLE_NATIVE_CODEGEN
        JITDOMFastPathHelperMap * m_domFastPathHelperMap;
#endif

#ifdef RECYCLER_PERF_COUNTERS
        size_t bindReferenceCount;
#endif

        ScriptContext * nextPendingClose;
    public:
        void SetNextPendingClose(ScriptContext * nextPendingClose);
        inline ScriptContext * GetNextPendingClose() const { return nextPendingClose; }

#ifdef ENABLE_MUTATION_BREAKPOINT
        // Keep track of all breakpoints in order to properly clean up on debugger detach
        bool HasMutationBreakpoints();
        void InsertMutationBreakpoint(Js::MutationBreakpoint *mutationBreakpoint);
#endif
    };

    class AutoDynamicCodeReference
    {
    public:
        AutoDynamicCodeReference(ScriptContext* scriptContext):
          m_scriptContext(scriptContext)
          {
              scriptContext->GetLibrary()->BeginDynamicFunctionReferences();
          }

          ~AutoDynamicCodeReference()
          {
              m_scriptContext->GetLibrary()->EndDynamicFunctionReferences();
          }

    private:
        ScriptContext* m_scriptContext;
    };

    template <typename TCacheType>
    void ScriptContext::CleanDynamicFunctionCache(TCacheType* cacheType)
    {
        // Remove eval map functions that haven't been recently used
        // TODO: Metric based on allocation size too? So don't clean if there hasn't been much allocated?

        cacheType->Clean([this](const typename TCacheType::KeyType& key, typename TCacheType::ValueType value) {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(DumpEvalStringOnRemoval))
            {
                Output::Print(_u("EvalMap: Removing Dynamic Function String from dynamic function cache: %s\n"), key.str.GetBuffer()); Output::Flush();
            }
#endif
        });
    }

    template <class TDelegate>
    void ScriptContext::MapFunction(TDelegate mapper)
    {
        if (this->sourceList)
        {
            this->sourceList->Map([&mapper](int, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfo)
            {
                Utf8SourceInfo* sourceInfoStrongRef = sourceInfo->Get();
                if (sourceInfoStrongRef)
                {
                    sourceInfoStrongRef->MapFunction(mapper);
                }
            });
        }
    }

    template <class TDelegate>
    FunctionBody* ScriptContext::FindFunction(TDelegate predicate)
    {
        FunctionBody* functionBody = nullptr;

        this->sourceList->MapUntil([&functionBody, &predicate](int, RecyclerWeakReference<Js::Utf8SourceInfo>* sourceInfo) -> bool
        {
            Utf8SourceInfo* sourceInfoStrongRef = sourceInfo->Get();
            if (sourceInfoStrongRef)
            {
                functionBody = sourceInfoStrongRef->FindFunction(predicate);
                if (functionBody)
                {
                    return true;
                }
            }

            return false;
        });

        return functionBody;
    }

    class AutoProfilingPhase
    {
    public:
        AutoProfilingPhase(ScriptContext* scriptcontext, Js::Phase phase) : scriptcontext(scriptcontext), phase(phase), isPhaseComplete(false)
        {
    #ifdef PROFILE_EXEC
            scriptcontext->ProfileBegin(phase);
    #endif
        }

        ~AutoProfilingPhase()
        {
            if(!this->isPhaseComplete)
            {
                EndProfile();
            }
        }

        void EndProfile()
        {
            this->isPhaseComplete = true;
#ifdef PROFILE_EXEC
            scriptcontext->ProfileEnd(phase);
#endif
        }
    private:
        ScriptContext* scriptcontext;
        Js::Phase phase;
        bool isPhaseComplete;
    };

#ifdef ENABLE_SCRIPT_DEBUGGING
    // Set up a scope in which we will initialize library JS code (like Intl.js),
    // which should not be treated as user-level JS code.
    // We should not profile and should not log debugger information in such a scope.
    class AutoInitLibraryCodeScope
    {
    private:
        ScriptContext * const scriptContext;
        const bool oldIsProfilingUserCode;
        const bool oldIsDebuggerRecording;

    public:
        AutoInitLibraryCodeScope(ScriptContext *scriptContext) :
            scriptContext(scriptContext),
            oldIsProfilingUserCode(scriptContext->GetThreadContext()->IsProfilingUserCode()),
            oldIsDebuggerRecording(scriptContext->IsDebuggerRecording())
        {
            this->scriptContext->GetThreadContext()->SetIsProfilingUserCode(false);
            this->scriptContext->SetIsDebuggerRecording(false);
        }

        ~AutoInitLibraryCodeScope()
        {
            this->scriptContext->GetThreadContext()->SetIsProfilingUserCode(this->oldIsProfilingUserCode);
            this->scriptContext->SetIsDebuggerRecording(this->oldIsDebuggerRecording);
        }
    };
#endif
}

#define BEGIN_TEMP_ALLOCATOR(allocator, scriptContext, name) \
    Js::TempArenaAllocatorObject *temp##allocator = scriptContext->GetTemporaryAllocator(name); \
    ArenaAllocator * allocator = temp##allocator->GetAllocator();

#define END_TEMP_ALLOCATOR(allocator, scriptContext) \
    scriptContext->ReleaseTemporaryAllocator(temp##allocator);

#define DECLARE_TEMP_ALLOCATOR(allocator) \
    Js::TempArenaAllocatorObject *temp##allocator = nullptr; \
    ArenaAllocator * allocator = nullptr;

#define ACQUIRE_TEMP_ALLOCATOR(allocator, scriptContext, name) \
    temp##allocator = scriptContext->GetTemporaryAllocator(name); \
    allocator = temp##allocator->GetAllocator();

#define RELEASE_TEMP_ALLOCATOR(allocator, scriptContext) \
    if (temp##allocator) \
    scriptContext->ReleaseTemporaryAllocator(temp##allocator);

#define DECLARE_TEMP_GUEST_ALLOCATOR(allocator) \
    Js::TempGuestArenaAllocatorObject *tempGuest##allocator = nullptr; \
    ArenaAllocator * allocator = nullptr;

#define ACQUIRE_TEMP_GUEST_ALLOCATOR(allocator, scriptContext, name) \
    tempGuest##allocator = scriptContext->GetTemporaryGuestAllocator(name); \
    allocator = tempGuest##allocator->GetAllocator();

#define RELEASE_TEMP_GUEST_ALLOCATOR(allocator, scriptContext) \
    if (tempGuest##allocator) \
    scriptContext->ReleaseTemporaryGuestAllocator(tempGuest##allocator);
