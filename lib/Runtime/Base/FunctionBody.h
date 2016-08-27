//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "AuxPtrs.h"
#include "CompactCounters.h"

struct CodeGenWorkItem;
class SourceContextInfo;
struct DeferredFunctionStub;
#ifdef DYNAMIC_PROFILE_MUTATOR
class DynamicProfileMutator;
class DynamicProfileMutatorImpl;
#endif
#define MAX_FUNCTION_BODY_DEBUG_STRING_SIZE 42 //11*3+8+1

namespace Js
{
#pragma region Class Forward Declarations
    class ByteCodeBufferReader;
    class ByteCodeBufferBuilder;
    class ByteCodeCache;
    class ScopeInfo;
    class SmallSpanSequence;
    struct StatementLocation;
    class SmallSpanSequenceIter;
    struct StatementData;
    struct PropertyIdOnRegSlotsContainer;

    struct InlineCache;
    struct PolymorphicInlineCache;
    struct IsInstInlineCache;
    class ScopeObjectChain;
    class EntryPointInfo;
    class FunctionProxy;
    class ParseableFunctionInfo;
    class FunctionBody;

    class DebuggerScopeProperty;
    class DebuggerScope;
    class FunctionEntryPointInfo;

#ifndef TEMP_DISABLE_ASMJS
    class AsmJsFunctionInfo;
    class AmsJsModuleInfo;
#endif
    class ArrayBuffer;
    class FunctionCodeGenRuntimeData;
#pragma endregion

    typedef JsUtil::BaseDictionary<Js::PropertyId, const Js::PropertyRecord*, RecyclerNonLeafAllocator, PowerOf2SizePolicy, DefaultComparer, JsUtil::SimpleDictionaryEntry> PropertyRecordList;
    typedef JsUtil::BaseHashSet<void*, Recycler, PowerOf2SizePolicy> TypeRefSet;

     // Definition of scopes such as With, Catch and Block which will be used further in the debugger for additional look-ups.
    enum DiagExtraScopesType
    {
        DiagUnknownScope,           // Unknown scope set when deserializing bytecode and the scope is not yet known.
        DiagWithScope,              // With scope.
        DiagCatchScopeDirect,       // Catch scope in regslot
        DiagCatchScopeInSlot,       // Catch scope in slot array
        DiagCatchScopeInObject,     // Catch scope in scope object
        DiagBlockScopeDirect,       // Block scope in regslot
        DiagBlockScopeInSlot,       // Block scope in slot array
        DiagBlockScopeInObject,     // Block scope in activation object
        DiagBlockScopeRangeEnd,     // Used to end a block scope range.
        DiagParamScope,             // The scope represents symbols at formals
        DiagParamScopeInObject,     // The scope represents symbols at formals and formal scope in activation object
    };

    class PropertyGuard
    {
        friend class PropertyGuardValidator;
    private:
        intptr_t value; // value is address of Js::Type
    public:
        static PropertyGuard* New(Recycler* recycler) { return RecyclerNewLeaf(recycler, Js::PropertyGuard); }
        PropertyGuard() : value(1) {}
        PropertyGuard(intptr_t value) : value(value) { Assert(this->value != 0); }

        inline static size_t const GetSizeOfValue() { return sizeof(((PropertyGuard*)0)->value); }
        inline static size_t const GetOffsetOfValue() { return offsetof(PropertyGuard, value); }

        intptr_t GetValue() const { return this->value; }
        bool IsValid() { return this->value != 0; }
        void SetValue(intptr_t value) { Assert(value != 0); this->value = value; }
        intptr_t const* GetAddressOfValue() { return &this->value; }
        void Invalidate() { this->value = 0; }
    };

    class PropertyGuardValidator
    {
        // Required by EquivalentTypeGuard::SetType.
        CompileAssert(offsetof(PropertyGuard, value) == 0);
        CompileAssert(offsetof(ConstructorCache, guard.value) == offsetof(PropertyGuard, value));
    };

    class JitIndexedPropertyGuard : public Js::PropertyGuard
    {
    private:
        int index;

    public:
        JitIndexedPropertyGuard(intptr_t value, int index):
            Js::PropertyGuard(value), index(index) {}

        int GetIndex() const { return this->index; }
    };

    class JitTypePropertyGuard : public Js::JitIndexedPropertyGuard
    {
    public:
        JitTypePropertyGuard(intptr_t typeAddr, int index):
            JitIndexedPropertyGuard(typeAddr, index) {}

        intptr_t GetTypeAddr() const { return this->GetValue(); }

    };

    struct TypeGuardTransferEntry
    {
        PropertyId propertyId;
        JitIndexedPropertyGuard* guards[0];

        TypeGuardTransferEntry(): propertyId(Js::Constants::NoProperty) {}
    };

    class FakePropertyGuardWeakReference: public RecyclerWeakReference<Js::PropertyGuard>
    {
    public:
        static FakePropertyGuardWeakReference* New(Recycler* recycler, Js::PropertyGuard* guard)
        {
            Assert(guard != nullptr);
            return RecyclerNewLeaf(recycler, Js::FakePropertyGuardWeakReference, guard);
        }
        FakePropertyGuardWeakReference(const Js::PropertyGuard* guard)
        {
            this->strongRef = (char*)guard;
            this->strongRefHeapBlock = &CollectedRecyclerWeakRefHeapBlock::Instance;
        }

        void Zero()
        {
            Assert(this->strongRef != nullptr);
            this->strongRef = nullptr;
        }
    };

    struct CtorCacheGuardTransferEntry
    {
        PropertyId propertyId;
        intptr_t caches[0];

        CtorCacheGuardTransferEntry(): propertyId(Js::Constants::NoProperty) {}
    };


#define EQUIVALENT_TYPE_CACHE_SIZE (EQUIVALENT_TYPE_CACHE_SIZE_IDL)

    struct EquivalentTypeCache
    {
        Js::Type* types[EQUIVALENT_TYPE_CACHE_SIZE];
        PropertyGuard *guard;
        TypeEquivalenceRecord record;
        uint nextEvictionVictim;
        bool isLoadedFromProto;
        bool hasFixedValue;

        EquivalentTypeCache(): nextEvictionVictim(EQUIVALENT_TYPE_CACHE_SIZE) {}
        bool ClearUnusedTypes(Recycler *recycler);
        void SetGuard(PropertyGuard *theGuard) { this->guard = theGuard; }
        void SetIsLoadedFromProto() { this->isLoadedFromProto = true; }
        bool IsLoadedFromProto() const { return this->isLoadedFromProto; }
        void SetHasFixedValue() { this->hasFixedValue = true; }
        bool HasFixedValue() const { return this->hasFixedValue; }

        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            Assert(false); // not implemented yet
        }
    };

    class JitEquivalentTypeGuard : public JitIndexedPropertyGuard
    {
        // This pointer is allocated from background thread first, and then transferred to recycler,
        // so as to keep the cached types alive.
        EquivalentTypeCache* cache;
        uint32 objTypeSpecFldId;

    public:
        JitEquivalentTypeGuard(intptr_t typeAddr, int index, uint32 objTypeSpecFldId):
            JitIndexedPropertyGuard(typeAddr, index), cache(nullptr), objTypeSpecFldId(objTypeSpecFldId) {}

        intptr_t GetTypeAddr() const { return this->GetValue(); }

        void SetTypeAddr(const intptr_t typeAddr)
        {
            this->SetValue(typeAddr);
        }

        uint32 GetObjTypeSpecFldId() const
        {
            return this->objTypeSpecFldId;
        }

        Js::EquivalentTypeCache* GetCache() const
        {
            return this->cache;
        }

        void SetCache(Js::EquivalentTypeCache* cache)
        {
            this->cache = cache;
        }

        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            // cache will be recycler allocated pointer
        }
    };

#pragma region Inline Cache Info class declarations
    class PolymorphicCacheUtilizationArray
    {
    private:
        byte *utilArray;

    public:
        PolymorphicCacheUtilizationArray()
            : utilArray(nullptr)
        {
        }
        void EnsureUtilArray(Recycler * const recycler, Js::FunctionBody * functionBody);
        byte* GetByteArray() { return utilArray; }
        void SetUtil(Js::FunctionBody* functionBody, uint index, byte util);
        byte GetUtil(Js::FunctionBody* functionBody, uint index);
    };

    class PolymorphicInlineCacheInfo sealed
    {
    private:
        InlineCachePointerArray<PolymorphicInlineCache> polymorphicInlineCaches;
        PolymorphicCacheUtilizationArray polymorphicCacheUtilizationArray;
        FunctionBody * functionBody;

    public:
        PolymorphicInlineCacheInfo(FunctionBody * functionBody)
            : functionBody(functionBody)
        {
        }

        InlineCachePointerArray<PolymorphicInlineCache> * GetPolymorphicInlineCaches() { return &polymorphicInlineCaches; }
        PolymorphicCacheUtilizationArray * GetUtilArray() { return &polymorphicCacheUtilizationArray; }
        byte * GetUtilByteArray() { return polymorphicCacheUtilizationArray.GetByteArray(); }
        FunctionBody * GetFunctionBody() const { return functionBody; }
    };

    class EntryPointPolymorphicInlineCacheInfo sealed
    {
    private:
        PolymorphicInlineCacheInfo selfInfo;
        SListCounted<PolymorphicInlineCacheInfo*, Recycler> inlineeInfo;

        static void SetPolymorphicInlineCache(PolymorphicInlineCacheInfo * polymorphicInlineCacheInfo, FunctionBody * functionBody, uint index, PolymorphicInlineCache * polymorphicInlineCache, byte polyCacheUtil);

    public:
        EntryPointPolymorphicInlineCacheInfo(FunctionBody * functionBody);

        PolymorphicInlineCacheInfo * GetSelfInfo() { return &selfInfo; }
        PolymorphicInlineCacheInfo * EnsureInlineeInfo(Recycler * recycler, FunctionBody * inlineeFunctionBody);
        PolymorphicInlineCacheInfo * GetInlineeInfo(FunctionBody * inlineeFunctionBody);
        SListCounted<PolymorphicInlineCacheInfo*, Recycler> * GetInlineeInfo() { return &this->inlineeInfo; }

        void SetPolymorphicInlineCache(FunctionBody * functionBody, uint index, PolymorphicInlineCache * polymorphicInlineCache, bool isInlinee, byte polyCacheUtil);

        template <class Fn>
        void MapInlinees(Fn fn)
        {
            SListCounted<PolymorphicInlineCacheInfo*, Recycler>::Iterator iter(&inlineeInfo);
            while (iter.Next())
            {
                fn(iter.Data());
            }
        }
    };
#pragma endregion

#ifdef FIELD_ACCESS_STATS
    struct FieldAccessStats
    {
        uint totalInlineCacheCount;
        uint noInfoInlineCacheCount;
        uint monoInlineCacheCount;
        uint emptyMonoInlineCacheCount;
        uint polyInlineCacheCount;
        uint nullPolyInlineCacheCount;
        uint emptyPolyInlineCacheCount;
        uint ignoredPolyInlineCacheCount;
        uint highUtilPolyInlineCacheCount;
        uint lowUtilPolyInlineCacheCount;
        uint equivPolyInlineCacheCount;
        uint nonEquivPolyInlineCacheCount;
        uint disabledPolyInlineCacheCount;
        uint clonedMonoInlineCacheCount;
        uint clonedPolyInlineCacheCount;

        FieldAccessStats() :
            totalInlineCacheCount(0), noInfoInlineCacheCount(0), monoInlineCacheCount(0), emptyMonoInlineCacheCount(0),
            polyInlineCacheCount(0), nullPolyInlineCacheCount(0), emptyPolyInlineCacheCount(0), ignoredPolyInlineCacheCount(0),
            highUtilPolyInlineCacheCount(0), lowUtilPolyInlineCacheCount(0),
            equivPolyInlineCacheCount(0), nonEquivPolyInlineCacheCount(0), disabledPolyInlineCacheCount(0),
            clonedMonoInlineCacheCount(0), clonedPolyInlineCacheCount(0) {}

        void Add(FieldAccessStats* other);
    };

    typedef FieldAccessStats* FieldAccessStatsPtr;
#else
    typedef void* FieldAccessStatsPtr;
#endif

#pragma region Entry point class declarations
    class ProxyEntryPointInfo:  public ExpirableObject
    {
    public:
        // These are public because we don't manage them nor their consistency;
        // the user of this class does.
        Js::JavascriptMethod jsMethod;

        ProxyEntryPointInfo(Js::JavascriptMethod jsMethod, ThreadContext* context = nullptr):
            ExpirableObject(context),
            jsMethod(jsMethod)
        {
        }
        static DWORD GetAddressOffset() { return offsetof(ProxyEntryPointInfo, jsMethod); }
        virtual void Expire()
        {
            AssertMsg(false, "Expire called on object that doesn't support expiration");
        }

        virtual void EnterExpirableCollectMode()
        {
            AssertMsg(false, "EnterExpirableCollectMode called on object that doesn't support expiration");
        }

        virtual bool IsFunctionEntryPointInfo() const { return false; }
    };


    struct TypeGuardTransferData
    {
        unsigned int propertyGuardCount;
        TypeGuardTransferEntryIDL* entries;
    };

    struct CtorCacheTransferData
    {
        unsigned int ctorCachesCount;
        CtorCacheTransferEntryIDL ** entries;
    };



    // Not thread safe.
    // Note that instances of this class are read from and written to from the
    // main and JIT threads.
    class EntryPointInfo : public ProxyEntryPointInfo
    {
    private:
        enum State : BYTE
        {
            NotScheduled,       // code gen has not been scheduled
            CodeGenPending,     // code gen job has been scheduled
            CodeGenQueued,      // code gen has been queued and all the code gen data has been gathered.
            CodeGenRecorded,    // backend completed, but job still pending
            CodeGenDone,        // code gen job successfully completed
            JITCapReached,      // workitem created but JIT cap reached
            PendingCleanup,     // workitem needs to be cleaned up but couldn't for some reason- it'll be cleaned up at the next opportunity
            CleanedUp           // the entry point has been cleaned up
        };

#if ENABLE_NATIVE_CODEGEN
        class JitTransferData
        {
            friend EntryPointInfo;

        private:
            TypeRefSet* jitTimeTypeRefs;

            PinnedTypeRefsIDL* runtimeTypeRefs;


            int propertyGuardCount;
            // This is a dynamically sized array of dynamically sized TypeGuardTransferEntries.  It's heap allocated by the JIT
            // thread and lives until entry point is installed, at which point it is explicitly freed.
            TypeGuardTransferEntry* propertyGuardsByPropertyId;
            size_t propertyGuardsByPropertyIdPlusSize;

            // This is a dynamically sized array of dynamically sized CtorCacheGuardTransferEntry.  It's heap allocated by the JIT
            // thread and lives until entry point is installed, at which point it is explicitly freed.
            CtorCacheGuardTransferEntry* ctorCacheGuardsByPropertyId;
            size_t ctorCacheGuardsByPropertyIdPlusSize;

            int equivalentTypeGuardCount;
            int lazyBailoutPropertyCount;
            // This is a dynamically sized array of JitEquivalentTypeGuards. It's heap allocated by the JIT thread and lives
            // until entry point is installed, at which point it is explicitly freed. We need it during installation so as to
            // swap the cache associated with each guard from the heap to the recycler (so the types in the cache are kept alive).
            JitEquivalentTypeGuard** equivalentTypeGuards;
            Js::PropertyId* lazyBailoutProperties;
            NativeCodeData* jitTransferRawData;
            EquivalentTypeGuardOffsets* equivalentTypeGuardOffsets;
            TypeGuardTransferData typeGuardTransferData;
            CtorCacheTransferData ctorCacheTransferData;

            bool falseReferencePreventionBit;
            bool isReady;

        public:
            JitTransferData():
                jitTimeTypeRefs(nullptr), runtimeTypeRefs(nullptr),
                propertyGuardCount(0), propertyGuardsByPropertyId(nullptr), propertyGuardsByPropertyIdPlusSize(0),
                ctorCacheGuardsByPropertyId(nullptr), ctorCacheGuardsByPropertyIdPlusSize(0),
                equivalentTypeGuardCount(0), equivalentTypeGuards(nullptr), jitTransferRawData(nullptr),
                falseReferencePreventionBit(true), isReady(false), lazyBailoutProperties(nullptr), lazyBailoutPropertyCount(0){}

            void SetRawData(NativeCodeData* rawData) { jitTransferRawData = rawData; }
            void AddJitTimeTypeRef(void* typeRef, Recycler* recycler);

            int GetRuntimeTypeRefCount() { return this->runtimeTypeRefs ? this->runtimeTypeRefs->count : 0; }
            void** GetRuntimeTypeRefs() { return this->runtimeTypeRefs ? (void**)this->runtimeTypeRefs->typeRefs : nullptr; }
            void SetRuntimeTypeRefs(PinnedTypeRefsIDL* pinnedTypeRefs) { this->runtimeTypeRefs = pinnedTypeRefs;}

            JitEquivalentTypeGuard** GetEquivalentTypeGuards() const { return this->equivalentTypeGuards; }
            void SetEquivalentTypeGuards(JitEquivalentTypeGuard** guards, int count)
            {
                this->equivalentTypeGuardCount = count;
                this->equivalentTypeGuards = guards;
            }
            void SetLazyBailoutProperties(Js::PropertyId* properties, int count)
            {
                this->lazyBailoutProperties = properties;
                this->lazyBailoutPropertyCount = count;
            }
            void SetEquivalentTypeGuardOffsets(EquivalentTypeGuardOffsets* offsets)
            {
                equivalentTypeGuardOffsets = offsets;
            }
            void SetTypeGuardTransferData(JITOutputIDL* data)
            {
                typeGuardTransferData.entries = data->typeGuardEntries;
                typeGuardTransferData.propertyGuardCount = data->propertyGuardCount;
            }
            void SetCtorCacheTransferData(JITOutputIDL * data)
            {
                ctorCacheTransferData.entries = data->ctorCacheEntries;
                ctorCacheTransferData.ctorCachesCount = data->ctorCachesCount;
            }
            bool GetIsReady() { return this->isReady; }
            void SetIsReady() { this->isReady = true; }

        private:
            void EnsureJitTimeTypeRefs(Recycler* recycler);
        };

        NativeCodeData * inProcJITNaticeCodedata;
        char* nativeDataBuffer;
        CodeGenNumberChunk * numberChunks;

        SmallSpanSequence *nativeThrowSpanSequence;
        typedef JsUtil::BaseHashSet<RecyclerWeakReference<FunctionBody>*, Recycler, PowerOf2SizePolicy> WeakFuncRefSet;
        WeakFuncRefSet *weakFuncRefSet;
        // Need to keep strong references to the guards here so they don't get collected while the entry point is alive.
        typedef JsUtil::BaseDictionary<Js::PropertyId, PropertyGuard*, Recycler, PowerOf2SizePolicy> SharedPropertyGuardDictionary;
        SharedPropertyGuardDictionary* sharedPropertyGuards;
        typedef JsUtil::List<LazyBailOutRecord, HeapAllocator> BailOutRecordMap;
        BailOutRecordMap* bailoutRecordMap;

        // This array holds fake weak references to type property guards. We need it to zero out the weak references when the
        // entry point is finalized and the guards are about to be freed. Otherwise, if one of the guards was to be invalidated
        // from the thread context, we would AV trying to access freed memory. Note that the guards themselves are allocated by
        // NativeCodeData::Allocator and are kept alive by the data field. The weak references are recycler allocated, and so
        // the array must be recycler allocated also, so that the recycler doesn't collect the weak references.
        FakePropertyGuardWeakReference** propertyGuardWeakRefs;
        EquivalentTypeCache* equivalentTypeCaches;
        EntryPointInfo ** registeredEquivalentTypeCacheRef;

        int propertyGuardCount;
        int equivalentTypeCacheCount;
#endif
        CodeGenWorkItem * workItem;
        Js::JavascriptMethod nativeAddress;
        ptrdiff_t codeSize;
        bool isAsmJsFunction; // true if entrypoint is for asmjs function
        uintptr_t  mModuleAddress; //asm Module address

#ifdef FIELD_ACCESS_STATS
        FieldAccessStatsPtr fieldAccessStats;
#endif

    protected:
        JavascriptLibrary* library;
#if ENABLE_NATIVE_CODEGEN
        typedef JsUtil::List<NativeOffsetInlineeFramePair, HeapAllocator> InlineeFrameMap;
        InlineeFrameMap*  inlineeFrameMap;
        unsigned int inlineeFrameOffsetArrayOffset;
        unsigned int inlineeFrameOffsetArrayCount;
#endif
#if ENABLE_DEBUG_STACK_BACK_TRACE
        StackBackTrace*    cleanupStack;
#endif

    public:
        enum CleanupReason
        {
            NotCleanedUp,
            CodeGenFailedOOM,
            CodeGenFailedStackOverflow,
            CodeGenFailedAborted,
            CodeGenFailedExceedJITLimit,
            CodeGenFailedUnknown,
            NativeCodeInstallFailure,
            CleanUpForFinalize
        };
        uint frameHeight;

        char** GetNativeDataBufferRef() { return &nativeDataBuffer; }
        char* GetNativeDataBuffer() { return nativeDataBuffer; }
        void SetInProcJITNativeCodeData(NativeCodeData* nativeCodeData) { inProcJITNaticeCodedata = nativeCodeData; }

        void SetNumberChunks(CodeGenNumberChunk * chunks) { numberChunks = chunks; }

    private:
#if ENABLE_NATIVE_CODEGEN
        typedef SListCounted<ConstructorCache*, Recycler> ConstructorCacheList;
        ConstructorCacheList* constructorCaches;

        EntryPointPolymorphicInlineCacheInfo * polymorphicInlineCacheInfo;

        // This field holds any recycler allocated references that must be kept alive until
        // we install the entry point.  It is freed at that point, so anything that must survive
        // until the EntryPointInfo itself goes away, must be copied somewhere else.
        JitTransferData* jitTransferData;

        // If we pin types this array contains strong references to types, otherwise it holds weak references.
        void **runtimeTypeRefs;

#if _M_AMD64 || _M_ARM32_OR_ARM64
        XDataInfo * xdataInfo;
#endif

        uint32 pendingPolymorphicCacheState;
#endif
        State state; // Single state member so users can query state w/o a lock
#if ENABLE_DEBUG_CONFIG_OPTIONS
        CleanupReason cleanupReason;
#endif
        BYTE   pendingInlinerVersion;
        bool   isLoopBody;

        bool   hasJittedStackClosure;
#if ENABLE_NATIVE_CODEGEN
        ImplicitCallFlags pendingImplicitCallFlags;
#endif

    public:
        virtual void Finalize(bool isShutdown) override;
        virtual bool IsFunctionEntryPointInfo() const override { return true; }

    protected:
        EntryPointInfo(Js::JavascriptMethod method, JavascriptLibrary* library, void* validationCookie, ThreadContext* context = nullptr, bool isLoopBody = false) :
            ProxyEntryPointInfo(method, context),
#if ENABLE_NATIVE_CODEGEN
            nativeThrowSpanSequence(nullptr), workItem(nullptr), weakFuncRefSet(nullptr),
            jitTransferData(nullptr), sharedPropertyGuards(nullptr), propertyGuardCount(0), propertyGuardWeakRefs(nullptr),
            equivalentTypeCacheCount(0), equivalentTypeCaches(nullptr), constructorCaches(nullptr), state(NotScheduled), inProcJITNaticeCodedata(nullptr),
            numberChunks(nullptr), polymorphicInlineCacheInfo(nullptr), runtimeTypeRefs(nullptr),
            isLoopBody(isLoopBody), hasJittedStackClosure(false), registeredEquivalentTypeCacheRef(nullptr), bailoutRecordMap(nullptr),
#endif
            library(library), codeSize(0), nativeAddress(nullptr), isAsmJsFunction(false), validationCookie(validationCookie)
#if ENABLE_DEBUG_STACK_BACK_TRACE
            , cleanupStack(nullptr)
#endif
#if ENABLE_DEBUG_CONFIG_OPTIONS
            , cleanupReason(NotCleanedUp)
#endif
#if DBG_DUMP | defined(VTUNE_PROFILING)
            , nativeOffsetMaps(&HeapAllocator::Instance)
#endif
#ifdef FIELD_ACCESS_STATS
            , fieldAccessStats(nullptr)
#endif
        {}

        virtual void ReleasePendingWorkItem() {};

        virtual void OnCleanup(bool isShutdown) = 0;

#ifdef PERF_COUNTERS
        virtual void OnRecorded() = 0;
#endif
    private:
        State GetState() const
        {
            Assert(this->state >= NotScheduled && this->state <= CleanedUp);
            return this->state;
        }

    public:
        ScriptContext* GetScriptContext();

        virtual FunctionBody *GetFunctionBody() const = 0;
#if ENABLE_NATIVE_CODEGEN
        EntryPointPolymorphicInlineCacheInfo * EnsurePolymorphicInlineCacheInfo(Recycler * recycler, FunctionBody * functionBody);
        EntryPointPolymorphicInlineCacheInfo * GetPolymorphicInlineCacheInfo() { return polymorphicInlineCacheInfo; }

        JitTransferData* GetJitTransferData() { return this->jitTransferData; }
        JitTransferData* EnsureJitTransferData(Recycler* recycler);
#if defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
        XDataInfo* GetXDataInfo() { return this->xdataInfo; }
        void SetXDataInfo(XDataInfo* xdataInfo) { this->xdataInfo = xdataInfo; }
#endif

#ifdef FIELD_ACCESS_STATS
        FieldAccessStats* GetFieldAccessStats() { return this->fieldAccessStats; }
        FieldAccessStats* EnsureFieldAccessStats(Recycler* recycler);
#endif

        void PinTypeRefs(ScriptContext* scriptContext);
        void InstallGuards(ScriptContext* scriptContext);
#endif

        void Cleanup(bool isShutdown, bool captureCleanupStack);

#if ENABLE_DEBUG_STACK_BACK_TRACE
        void CaptureCleanupStackTrace();
#endif

        bool IsNotScheduled() const
        {
            return this->GetState() == NotScheduled;
        }

        bool IsCodeGenPending() const
        {
            return this->GetState() == CodeGenPending;
        }

        bool IsNativeCode() const
        {
#if ENABLE_NATIVE_CODEGEN
            return this->GetState() == CodeGenRecorded ||
                this->GetState() == CodeGenDone;
#else
            return false;
#endif
        }

        bool IsCodeGenDone() const
        {
#if ENABLE_NATIVE_CODEGEN
            return this->GetState() == CodeGenDone;
#else
            return false;
#endif
        }

        bool IsCodeGenQueued() const
        {
#if ENABLE_NATIVE_CODEGEN
            return this->GetState() == CodeGenQueued;
#else
            return false;
#endif
        }

        bool IsJITCapReached() const
        {
#if ENABLE_NATIVE_CODEGEN
            return this->GetState() == JITCapReached;
#else
            return false;
#endif
        }

        bool IsCleanedUp() const
        {
            return this->GetState() == CleanedUp;
        }

        bool IsPendingCleanup() const
        {
            return this->GetState() == PendingCleanup;
        }

        void SetPendingCleanup()
        {
            this->state = PendingCleanup;
        }

#if ENABLE_DEBUG_CONFIG_OPTIONS
        void SetCleanupReason(CleanupReason reason)
        {
            this->cleanupReason = reason;
        }
#endif

        bool IsLoopBody() const
        {
            return this->isLoopBody;
        }

#if ENABLE_NATIVE_CODEGEN
        bool HasJittedStackClosure() const
        {
            return this->hasJittedStackClosure;
        }

        void SetHasJittedStackClosure()
        {
            this->hasJittedStackClosure = true;
        }
#endif

#ifndef TEMP_DISABLE_ASMJS
        void SetModuleAddress(uintptr_t moduleAddress)
        {
            Assert(this->GetIsAsmJSFunction());
            Assert(moduleAddress);
            mModuleAddress = moduleAddress;
        }

        uintptr_t GetModuleAddress()const
        {
            Assert(this->GetIsAsmJSFunction());
            Assert(mModuleAddress); // module address should not be null
            return mModuleAddress;
        }
#endif

        void Reset(bool resetStateToNotScheduled = true);

#if ENABLE_NATIVE_CODEGEN
        void SetCodeGenPending(CodeGenWorkItem * workItem)
        {
            Assert(this->GetState() == NotScheduled || this->GetState() == CleanedUp);
            Assert(workItem != nullptr);
            this->workItem = workItem;
            this->state = CodeGenPending;
        }

        void SetCodeGenPending()
        {
            Assert(this->GetState() == CodeGenQueued);
            this->state = CodeGenPending;
        }

        void SetCodeGenQueued()
        {
            Assert(this->GetState() == CodeGenPending);
            this->state = CodeGenQueued;
        }

        void RevertToNotScheduled()
        {
            Assert(this->GetState() == CodeGenPending);
            Assert(this->workItem != nullptr);
            this->workItem = nullptr;
            this->state = NotScheduled;
        }

        void SetCodeGenPendingWithStackAllocatedWorkItem()
        {
            Assert(this->GetState() == NotScheduled || this->GetState() == CleanedUp);
            this->workItem = nullptr;
            this->state = CodeGenPending;
        }

        void SetCodeGenRecorded(Js::JavascriptMethod nativeAddress, ptrdiff_t codeSize)
        {
            Assert(this->GetState() == CodeGenQueued);
            Assert(codeSize > 0);
            this->nativeAddress = nativeAddress;
            this->codeSize = codeSize;
            this->state = CodeGenRecorded;

#ifdef PERF_COUNTERS
            this->OnRecorded();
#endif
        }

        void SetCodeGenDone()
        {
            Assert(this->GetState() == CodeGenRecorded);
            this->state = CodeGenDone;
            this->workItem = nullptr;
        }

        void SetJITCapReached()
        {
            Assert(this->GetState() == CodeGenQueued);
            this->state = JITCapReached;
            this->workItem = nullptr;
        }

        SmallSpanSequence* GetNativeThrowSpanSequence() const
        {
            Assert(this->GetState() != NotScheduled);
            Assert(this->GetState() != CleanedUp);
            return nativeThrowSpanSequence;
        }

        void SetNativeThrowSpanSequence(SmallSpanSequence* seq)
        {
            Assert(this->GetState() == CodeGenQueued);
            Assert(this->nativeThrowSpanSequence == nullptr);

            nativeThrowSpanSequence = seq;
        }

        bool IsInNativeAddressRange(DWORD_PTR codeAddress) {
            return (IsNativeCode() &&
                codeAddress >= GetNativeAddress() &&
                codeAddress < GetNativeAddress() + GetCodeSize());
        }
#endif

        DWORD_PTR GetNativeAddress() const
        {
            // need the assert to skip for asmjsFunction as nativeAddress can be interpreter too for asmjs
            Assert(this->GetState() == CodeGenRecorded || this->GetState() == CodeGenDone || this->isAsmJsFunction);

            // !! this is illegal, however (by design) `IsInNativeAddressRange` (right above) needs it
            return reinterpret_cast<DWORD_PTR>(this->nativeAddress);
        }

        ptrdiff_t GetCodeSize() const
        {
            Assert(this->GetState() == CodeGenRecorded || this->GetState() == CodeGenDone);
            return codeSize;
        }

        CodeGenWorkItem * GetWorkItem() const
        {
            State state = this->GetState();
            Assert(state != NotScheduled || this->workItem == nullptr);
            Assert(state == CleanedUp && this->workItem == nullptr ||
                state != CleanedUp);

            if (state == PendingCleanup)
            {
                return nullptr;
            }

            return this->workItem;
        }

#ifndef TEMP_DISABLE_ASMJS
        // set code size, used by TJ to set the code size
        void SetCodeSize(ptrdiff_t size)
        {
            Assert(isAsmJsFunction);
            this->codeSize = size;
        }

        void SetNativeAddress(Js::JavascriptMethod address)
        {
            Assert(isAsmJsFunction);
            this->nativeAddress = address;
        }

        void SetIsAsmJSFunction(bool value)
        {
            this->isAsmJsFunction = value;
        }
#endif

        bool GetIsAsmJSFunction()const
        {
            return this->isAsmJsFunction;
        }

#ifndef TEMP_DISABLE_ASMJS
        void SetTJCodeGenDone()
        {
            Assert(isAsmJsFunction);
            this->state = CodeGenDone;
            this->workItem = nullptr;
        }
#endif

#if ENABLE_NATIVE_CODEGEN
        void AddWeakFuncRef(RecyclerWeakReference<FunctionBody> *weakFuncRef, Recycler *recycler);
        WeakFuncRefSet *EnsureWeakFuncRefSet(Recycler *recycler);

        void EnsureIsReadyToCall();
        void ProcessJitTransferData();
        void ResetOnLazyBailoutFailure();
        void OnNativeCodeInstallFailure();
        virtual void ResetOnNativeCodeInstallFailure() = 0;

        Js::PropertyGuard* RegisterSharedPropertyGuard(Js::PropertyId propertyId, ScriptContext* scriptContext);
        Js::PropertyId* GetSharedPropertyGuardsWithLock(ArenaAllocator * alloc, unsigned int& count) const;

        bool TryGetSharedPropertyGuard(Js::PropertyId propertyId, Js::PropertyGuard*& guard);
        void RecordTypeGuards(int propertyGuardCount, TypeGuardTransferEntry* typeGuardTransferRecord, size_t typeGuardTransferPlusSize);
        void RecordCtorCacheGuards(CtorCacheGuardTransferEntry* ctorCacheTransferRecord, size_t ctorCacheTransferPlusSize);
        void FreePropertyGuards();
        void FreeJitTransferData();
        void RegisterEquivalentTypeCaches();
        void UnregisterEquivalentTypeCaches();
        bool ClearEquivalentTypeCaches();

        void RegisterConstructorCache(Js::ConstructorCache* constructorCache, Recycler* recycler);
        uint GetConstructorCacheCount() const { return this->constructorCaches != nullptr ? this->constructorCaches->Count() : 0; }
        uint32 GetPendingPolymorphicCacheState() const { return this->pendingPolymorphicCacheState; }
        void SetPendingPolymorphicCacheState(uint32 state) { this->pendingPolymorphicCacheState = state; }
        BYTE GetPendingInlinerVersion() const { return this->pendingInlinerVersion; }
        void SetPendingInlinerVersion(BYTE version) { this->pendingInlinerVersion = version; }
        ImplicitCallFlags GetPendingImplicitCallFlags() const { return this->pendingImplicitCallFlags; }
        void SetPendingImplicitCallFlags(ImplicitCallFlags flags) { this->pendingImplicitCallFlags = flags; }
        virtual void Invalidate(bool prolongEntryPoint) { Assert(false); }
        void RecordBailOutMap(JsUtil::List<LazyBailOutRecord, ArenaAllocator>* bailoutMap);
        void RecordInlineeFrameMap(JsUtil::List<NativeOffsetInlineeFramePair, ArenaAllocator>* tempInlineeFrameMap);
        void RecordInlineeFrameOffsetsInfo(unsigned int offsetsArrayOffset, unsigned int offsetsArrayCount);
        InlineeFrameRecord* FindInlineeFrame(void* returnAddress);
        bool HasInlinees() { return this->frameHeight > 0; }
        void DoLazyBailout(BYTE** addressOfReturnAddress, Js::FunctionBody* functionBody, const PropertyRecord* propertyRecord);
#endif
#if DBG_DUMP
     public:
#elif defined(VTUNE_PROFILING)
     private:
#endif
#if DBG_DUMP || defined(VTUNE_PROFILING)
         // NativeOffsetMap is public for DBG_DUMP, private for VTUNE_PROFILING
         struct NativeOffsetMap
         {
            uint32 statementIndex;
            regex::Interval nativeOffsetSpan;
         };

     private:
         JsUtil::List<NativeOffsetMap, HeapAllocator> nativeOffsetMaps;
     public:
         void RecordNativeMap(uint32 offset, uint32 statementIndex);

         int GetNativeOffsetMapCount() const;
#endif

#if DBG_DUMP && ENABLE_NATIVE_CODEGEN
         void DumpNativeOffsetMaps();
         void DumpNativeThrowSpanSequence();
         NativeOffsetMap* GetNativeOffsetMap(int index)
         {
             Assert(index >= 0);
             Assert(index < GetNativeOffsetMapCount());

             return &nativeOffsetMaps.Item(index);
         }
#endif

#ifdef VTUNE_PROFILING

     public:
         uint PopulateLineInfo(void* pLineInfo, FunctionBody* body);

#endif

    protected:
        void* validationCookie;
    };

    class FunctionEntryPointInfo : public EntryPointInfo
    {
    public:
        FunctionProxy * functionProxy;
        FunctionEntryPointInfo* nextEntryPoint;

        // The offset on the native stack, from which the locals are located (Populated at RegAlloc phase). Used for debug purpose.
        int32 localVarSlotsOffset;
        // The offset which stores that any of the locals are changed from the debugger.
        int32 localVarChangedOffset;
        uint entryPointIndex;

        uint8 callsCount;
        uint8 lastCallsCount;
        bool nativeEntryPointProcessed;

    private:
        ExecutionMode jitMode;
        FunctionEntryPointInfo* mOldFunctionEntryPointInfo; // strong ref to oldEntryPointInfo(Int or TJ) in asm to ensure we don't collect it before JIT is completed
        bool       mIsTemplatizedJitMode; // true only if in TJ mode, used only for debugging
    public:
        FunctionEntryPointInfo(FunctionProxy * functionInfo, Js::JavascriptMethod method, ThreadContext* context, void* validationCookie);

#ifndef TEMP_DISABLE_ASMJS
        //AsmJS Support

        void SetOldFunctionEntryPointInfo(FunctionEntryPointInfo* entrypointInfo);
        FunctionEntryPointInfo* GetOldFunctionEntryPointInfo()const;
        void SetIsTJMode(bool value);
        bool GetIsTJMode()const;
        //End AsmJS Support
#endif

        virtual FunctionBody *GetFunctionBody() const override;
#if ENABLE_NATIVE_CODEGEN
        ExecutionMode GetJitMode() const;
        void SetJitMode(const ExecutionMode jitMode);

        virtual void Invalidate(bool prolongEntryPoint) override;
        virtual void Expire() override;
        virtual void EnterExpirableCollectMode() override;
        virtual void ResetOnNativeCodeInstallFailure() override;
        static const uint8 GetDecrCallCountPerBailout()
        {
            return (uint8)CONFIG_FLAG(CallsToBailoutsRatioForRejit) + 1;
        }
#endif

        virtual void OnCleanup(bool isShutdown) override;

        virtual void ReleasePendingWorkItem() override;

#ifdef PERF_COUNTERS
        virtual void OnRecorded() override;
#endif

    };

    class LoopEntryPointInfo : public EntryPointInfo
    {
    public:
        LoopHeader* loopHeader;
        uint jittedLoopIterationsSinceLastBailout; // number of times the loop iterated in the jitted code before bailing out 
        uint totalJittedLoopIterations; // total number of times the loop has iterated in the jitted code for this entry point for a particular invocation of the loop
        LoopEntryPointInfo(LoopHeader* loopHeader, Js::JavascriptLibrary* library, void* validationCookie) :
            EntryPointInfo(nullptr, library, validationCookie, /*threadContext*/ nullptr, /*isLoopBody*/ true),
            loopHeader(loopHeader),
            jittedLoopIterationsSinceLastBailout(0),
            totalJittedLoopIterations(0),
            mIsTemplatizedJitMode(false)
#ifdef BGJIT_STATS
            ,used(false)
#endif
        { }

        virtual FunctionBody *GetFunctionBody() const override;

        virtual void OnCleanup(bool isShutdown) override;

#if ENABLE_NATIVE_CODEGEN
        virtual void ResetOnNativeCodeInstallFailure() override;
        static const uint8 GetDecrLoopCountPerBailout()
        {
            return (uint8)CONFIG_FLAG(LoopIterationsToBailoutsRatioForRejit) + 1;
        }
#endif

#ifndef TEMP_DISABLE_ASMJS
        void SetIsTJMode(bool value)
        {
            Assert(this->GetIsAsmJSFunction());
            mIsTemplatizedJitMode = value;
        }

        bool GetIsTJMode()const
        {
            return mIsTemplatizedJitMode;
        };
#endif

#ifdef PERF_COUNTERS
        virtual void OnRecorded() override;
#endif

#ifdef BGJIT_STATS
        bool IsUsed() const
        {
            return this->used;
        }

        void MarkAsUsed()
        {
            this->used = true;
        }
#endif
    private:
#ifdef BGJIT_STATS
        bool used;
#endif
        bool       mIsTemplatizedJitMode;
    };

    typedef RecyclerWeakReference<FunctionEntryPointInfo> FunctionEntryPointWeakRef;

    typedef SynchronizableList<FunctionEntryPointWeakRef*, JsUtil::List<FunctionEntryPointWeakRef*>> FunctionEntryPointList;
    typedef SynchronizableList<LoopEntryPointInfo*, JsUtil::List<LoopEntryPointInfo*>> LoopEntryPointList;

#pragma endregion

    struct LoopHeader
    {
    private:
        LoopEntryPointList* entryPoints;

    public:
        uint startOffset;
        uint endOffset;
        uint interpretCount;
        uint profiledLoopCounter;
        bool isNested;
        bool isInTry;
        FunctionBody * functionBody;

#if DBG_DUMP
        uint nativeCount;
#endif
        static const uint NoLoop = (uint)-1;

        static const uint GetOffsetOfProfiledLoopCounter() { return offsetof(LoopHeader, profiledLoopCounter); }
        static const uint GetOffsetOfInterpretCount() { return offsetof(LoopHeader, interpretCount); }

        bool Contains(Js::LoopHeader * loopHeader) const
        {
            return (this->startOffset <= loopHeader->startOffset && loopHeader->endOffset <= this->endOffset);
        }

        bool Contains(uint offset) const
        {
            return this->startOffset <= offset && offset < this->endOffset;
        }

        Js::JavascriptMethod GetCurrentEntryPoint() const
        {
            LoopEntryPointInfo * entryPoint = GetCurrentEntryPointInfo();

            if (entryPoint != nullptr)
            {
                return this->entryPoints->Item(this->GetCurrentEntryPointIndex())->jsMethod;
            }

            return nullptr;
        }

        LoopEntryPointInfo * GetCurrentEntryPointInfo() const
        {
            Assert(this->entryPoints->Count() > 0);
            return this->entryPoints->Item(this->GetCurrentEntryPointIndex());
        }

        uint GetByteCodeCount()
        {
            return (endOffset - startOffset);
        }

        int GetCurrentEntryPointIndex() const
        {
           return this->entryPoints->Count() - 1;
        }

        LoopEntryPointInfo * GetEntryPointInfo(int index) const
        {
            return this->entryPoints->Item(index);
        }

        template <class Fn>
        void MapEntryPoints(Fn fn) const
        {
            if (this->entryPoints) // ETW rundown may call this before entryPoints initialization
            {
                this->entryPoints->Map([&](int index, LoopEntryPointInfo * entryPoint)
                {
                    if (entryPoint != nullptr)
                    {
                        fn(index, entryPoint);
                    }
                });
            }
        }

        template <class DebugSite, class Fn>
        HRESULT MapEntryPoints(DebugSite site, Fn fn) const // external debugging version
        {
            return Map(site, this->entryPoints, [&](int index, LoopEntryPointInfo * entryPoint)
            {
                if (entryPoint != nullptr)
                {
                    fn(index, entryPoint);
                }
            });
        }

        void Init(FunctionBody * functionBody);

#if ENABLE_NATIVE_CODEGEN
        int CreateEntryPoint();
        void ReleaseEntryPoints();
#endif

        void ResetInterpreterCount()
        {
            this->interpretCount = 0;
        }
        void ResetProfiledLoopCounter()
        {
            this->profiledLoopCounter = 0;
        }

    };

    class FunctionProxy;

    typedef FunctionProxy** FunctionProxyArray;
    typedef FunctionProxy** FunctionProxyPtrPtr;

    //
    // FunctionProxy represents a user defined function
    // This could be either from a source file or the byte code cache
    // The function need not have been compiled yet- it could be parsed or compiled
    // at a later time
    //
    class FunctionProxy : public FunctionInfo
    {
        static CriticalSection GlobalLock;
    public:
        static CriticalSection* GetLock() { return &GlobalLock; }
        typedef RecyclerWeakReference<DynamicType> FunctionTypeWeakRef;
        typedef JsUtil::List<FunctionTypeWeakRef*, Recycler, false, WeakRefFreeListedRemovePolicy> FunctionTypeWeakRefList;

    protected:
        FunctionProxy(JavascriptMethod entryPoint, Attributes attributes,
            LocalFunctionId functionId, ScriptContext* scriptContext, Utf8SourceInfo* utf8SourceInfo, uint functionNumber);
        DEFINE_VTABLE_CTOR_NO_REGISTER(FunctionProxy, FunctionInfo);

        enum class AuxPointerType : uint8 {
            DeferredStubs = 0,
            CachedSourceString = 1,
            AsmJsFunctionInfo = 2,
            AsmJsModuleInfo = 3,
            StatementMaps = 4,
            StackNestedFuncParent = 5,
            SimpleJitEntryPointInfo = 6,
            FunctionObjectTypeList = 7,           // Script function types not including the deferred prototype type
            CodeGenGetSetRuntimeData = 8,
            PropertyIdOnRegSlotsContainer = 9,    // This is used for showing locals for the current frame.
            LoopHeaderArray = 10,
            CodeGenRuntimeData = 11,
            PolymorphicInlineCachesHead = 12,     // DList of all polymorphic inline caches that aren't finalized yet
            PropertyIdsForScopeSlotArray = 13,    // For SourceInfo
            PolymorphicCallSiteInfoHead  = 14,
            AuxBlock = 15,                        // Optional auxiliary information
            AuxContextBlock = 16,                 // Optional auxiliary context specific information
            ReferencedPropertyIdMap = 17,
            LiteralRegexes = 18,
            ObjLiteralTypes = 19,
            ScopeInfo = 20,
            FormalsPropIdArray = 21,

            Max,
            Invalid = 0xff
        };

        typedef AuxPtrs<FunctionProxy, AuxPointerType> AuxPtrsT;
        friend AuxPtrsT;
        WriteBarrierPtr<AuxPtrsT> auxPtrs;
        void* GetAuxPtr(AuxPointerType e) const;
        void* GetAuxPtrWithLock(AuxPointerType e) const;
        void SetAuxPtr(AuxPointerType e, void* ptr);

    public:
        enum SetDisplayNameFlags
        {
            SetDisplayNameFlagsNone = 0,
            SetDisplayNameFlagsDontCopy = 1,
            SetDisplayNameFlagsRecyclerAllocated = 2
        };

        Recycler* GetRecycler() const;
        uint32 GetSourceContextId() const;
        char16* GetDebugNumberSet(wchar(&bufferToWriteTo)[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]) const;
        bool GetIsTopLevel() { return m_isTopLevel; }
        void SetIsTopLevel(bool set) { m_isTopLevel = set; }
        bool GetIsAnonymousFunction() const { return this->GetDisplayName() == Js::Constants::AnonymousFunction; }
        void Copy(FunctionProxy* other);
        ParseableFunctionInfo* EnsureDeserialized();
        ScriptContext* GetScriptContext() const;
        Utf8SourceInfo* GetUtf8SourceInfo() const { return this->m_utf8SourceInfo; }
        void SetUtf8SourceInfo(Utf8SourceInfo* utf8SourceInfo) { m_utf8SourceInfo = utf8SourceInfo; }
        void SetReferenceInParentFunction(FunctionProxyPtrPtr reference);
        void UpdateReferenceInParentFunction(FunctionProxy* newFunctionInfo);
        bool IsInDebugMode() const { return this->m_utf8SourceInfo->IsInDebugMode(); }

        DWORD_PTR GetSecondaryHostSourceContext() const;
        DWORD_PTR GetHostSourceContext() const;
        SourceContextInfo * GetSourceContextInfo() const;
        SRCINFO const * GetHostSrcInfo() const;

        uint GetFunctionNumber() const { return m_functionNumber; }

        virtual void Finalize(bool isShutdown) override;

        void UpdateFunctionBodyImpl(FunctionBody* body);
        bool IsFunctionBody() const;
        ProxyEntryPointInfo* GetDefaultEntryPointInfo() const;
        ScriptFunctionType * GetDeferredPrototypeType() const;
        ScriptFunctionType * EnsureDeferredPrototypeType();
        JavascriptMethod GetDirectEntryPoint(ProxyEntryPointInfo* entryPoint) const;

        // Function object type list methods
        FunctionTypeWeakRefList* EnsureFunctionObjectTypeList();
        void RegisterFunctionObjectType(DynamicType* functionType);
        template <typename Fn>
        void MapFunctionObjectTypes(Fn func);

        static uint GetOffsetOfDeferredPrototypeType() { return offsetof(Js::FunctionProxy, deferredPrototypeType); }
        static Js::ScriptFunctionType * EnsureFunctionProxyDeferredPrototypeType(FunctionProxy * proxy)
        {
            return proxy->EnsureDeferredPrototypeType();
        }

        void SetIsPublicLibraryCode() { m_isPublicLibraryCode = true; }
        bool IsPublicLibraryCode() const { return m_isPublicLibraryCode; }

#if DBG
        bool HasValidEntryPoint() const;
#ifdef ENABLE_SCRIPT_PROFILING
        bool HasValidProfileEntryPoint() const;
#endif
        bool HasValidNonProfileEntryPoint() const;
#endif
        virtual void SetDisplayName(const char16* displayName, uint displayNameLength, uint displayShortNameOffset, SetDisplayNameFlags flags = SetDisplayNameFlagsNone) = 0;
        virtual const char16* GetDisplayName() const = 0;
        virtual uint GetDisplayNameLength() const = 0;
        virtual uint GetShortDisplayNameOffset() const = 0;
        static const char16* WrapWithBrackets(const char16* name, charcount_t sz, ScriptContext* scriptContext);

        // Used only in the library function stringify (toString, DiagGetValueString).
        // If we need more often to give the short name, we should create a member variable which points to the short name
        // this is also now being used for function.name.
        const char16* GetShortDisplayName(charcount_t * shortNameLength);

        bool IsJitLoopBodyPhaseEnabled() const
        {
            // Consider: Allow JitLoopBody in generator functions for loops that do not yield.
            return !PHASE_OFF(JITLoopBodyPhase, this) && !PHASE_OFF(FullJitPhase, this) && !this->IsGenerator();
        }

        bool IsJitLoopBodyPhaseForced() const
        {
            return
                IsJitLoopBodyPhaseEnabled() &&
                (
                    PHASE_FORCE(JITLoopBodyPhase, this)
                #ifdef ENABLE_PREJIT
                    || Configuration::Global.flags.Prejit
                #endif
                );
        }

        ULONG GetHostStartLine() const;
        ULONG GetHostStartColumn() const;

    protected:
        // Static method(s)
        static void SetDisplayName(const char16* srcName, WriteBarrierPtr<const char16>* destName, uint displayNameLength, ScriptContext * scriptContext, SetDisplayNameFlags flags = SetDisplayNameFlagsNone);
        static bool SetDisplayName(const char16* srcName, const char16** destName, uint displayNameLength, ScriptContext * scriptContext, SetDisplayNameFlags flags = SetDisplayNameFlagsNone);
        static bool IsConstantFunctionName(const char16* srcName);

    protected:
        NoWriteBarrierPtr<ScriptContext>  m_scriptContext;   // Memory context for this function body
        WriteBarrierPtr<Utf8SourceInfo> m_utf8SourceInfo;
        // WriteBarrier-TODO: Consider changing this to NoWriteBarrierPtr, and skip tagging- also, tagging is likely unnecessary since that pointer in question is likely not resolvable
        FunctionProxyPtrPtr m_referenceInParentFunction; // Reference to nested function reference to this function in the parent function body (tagged to not be actual reference)
        WriteBarrierPtr<ScriptFunctionType> deferredPrototypeType;
        WriteBarrierPtr<ProxyEntryPointInfo> m_defaultEntryPointInfo; // The default entry point info for the function proxy

        NoWriteBarrierField<uint> m_functionNumber;  // Per thread global function number

        bool m_isTopLevel : 1; // Indicates that this function is top-level function, currently being used in script profiler and debugger
        bool m_isPublicLibraryCode: 1; // Indicates this function is public boundary library code that should be visible in JS stack
        void CleanupFunctionProxyCounters()
        {
            PERF_COUNTER_DEC(Code, TotalFunction);
        }

        ULONG ComputeAbsoluteLineNumber(ULONG relativeLineNumber) const;
        ULONG ComputeAbsoluteColumnNumber(ULONG relativeLineNumber, ULONG relativeColumnNumber) const;
        ULONG GetLineNumberInHostBuffer(ULONG relativeLineNumber) const;

    private:
        ScriptFunctionType * AllocDeferredPrototypeType();
    };

    // Represents a function from the byte code cache which will
    // be deserialized upon use
    class DeferDeserializeFunctionInfo: public FunctionProxy
    {
        friend struct ByteCodeSerializer;

    private:
        DeferDeserializeFunctionInfo(int nestedFunctionCount, LocalFunctionId functionId, ByteCodeCache* byteCodeCache, const byte* serializedFunction, Utf8SourceInfo* sourceInfo, ScriptContext* scriptContext, uint functionNumber, const char16* displayName, uint displayNameLength, uint displayShortNameOffset, NativeModule *nativeModule, Attributes attributes);
    public:
        static DeferDeserializeFunctionInfo* New(ScriptContext* scriptContext, int nestedFunctionCount, LocalFunctionId functionId, ByteCodeCache* byteCodeCache, const byte* serializedFunction, Utf8SourceInfo* utf8SourceInfo, const char16* displayName, uint displayNameLength, uint displayShortNameOffset, NativeModule *nativeModule, Attributes attributes);

        virtual void Finalize(bool isShutdown) override;
        FunctionBody* Deserialize();

        virtual const char16* GetDisplayName() const override;
        void SetDisplayName(const char16* displayName);
        virtual void SetDisplayName(const char16* displayName, uint displayNameLength, uint displayShortNameOffset, SetDisplayNameFlags flags = SetDisplayNameFlagsNone) override;
        virtual uint GetDisplayNameLength() const { return m_displayNameLength; }
        virtual uint GetShortDisplayNameOffset() const { return m_displayShortNameOffset; }
        LPCWSTR GetSourceInfo(int& lineNumber, int& columnNumber) const;
    private:
        const byte* m_functionBytes;
        ByteCodeCache* m_cache;
        const char16 * m_displayName;  // Optional name
        uint m_displayNameLength;
        uint m_displayShortNameOffset;
        NativeModule *m_nativeModule;
    };

    class ParseableFunctionInfo: public FunctionProxy
    {
        friend class ByteCodeBufferReader;

    protected:
        ParseableFunctionInfo(JavascriptMethod method, int nestedFunctionCount, LocalFunctionId functionId, Utf8SourceInfo* sourceInfo, ScriptContext* scriptContext, uint functionNumber, const char16* displayName, uint m_displayNameLength, uint displayShortNameOffset, Attributes attributes, Js::PropertyRecordList* propertyRecordList);

    public:
        struct NestedArray
        {
            NestedArray(uint32 count) :nestedCount(count) {}
            uint32 nestedCount;
            FunctionProxy* functionProxyArray[0];
        };
        template<typename Fn>
        void ForEachNestedFunc(Fn fn)
        {
            NestedArray* nestedArray = GetNestedArray();
            if (nestedArray != nullptr)
            {
                for (uint i = 0; i < nestedArray->nestedCount; i++)
                {
                    if (!fn(nestedArray->functionProxyArray[i], i))
                    {
                        break;
                    }
                }
            }
        }

        NestedArray* GetNestedArray() const { return nestedArray; }
        uint GetNestedCount() const { return nestedArray == nullptr ? 0 : nestedArray->nestedCount; }

    public:
        static ParseableFunctionInfo* New(ScriptContext* scriptContext, int nestedFunctionCount, LocalFunctionId functionId, Utf8SourceInfo* utf8SourceInfo, const char16* displayName, uint m_displayNameLength, uint displayShortNameOffset, Js::PropertyRecordList* propertyRecordList, Attributes attributes);

        DEFINE_VTABLE_CTOR_NO_REGISTER(ParseableFunctionInfo, FunctionProxy);
        FunctionBody* Parse(ScriptFunction ** functionRef = nullptr, bool isByteCodeDeserialization = false);
#ifndef TEMP_DISABLE_ASMJS
        FunctionBody* ParseAsmJs(Parser * p, __out CompileScriptException * se, __out ParseNodePtr * ptree);
#endif
        virtual uint GetDisplayNameLength() const { return m_displayNameLength; }
        virtual uint GetShortDisplayNameOffset() const { return m_displayShortNameOffset; }
        bool GetIsDeclaration() const { return m_isDeclaration; }
        void SetIsDeclaration(const bool is) { m_isDeclaration = is; }
        bool GetIsAccessor() const { return m_isAccessor; }
        void SetIsAccessor(const bool is) { m_isAccessor = is; }
        bool GetIsGlobalFunc() const { return m_isGlobalFunc; }
        void SetIsStaticNameFunction(const bool is) { m_isStaticNameFunction = is; }
        bool GetIsStaticNameFunction() const { return m_isStaticNameFunction; }
        void SetIsNamedFunctionExpression(const bool is) { m_isNamedFunctionExpression = is; }
        bool GetIsNamedFunctionExpression() const { return m_isNamedFunctionExpression; }
        void SetIsNameIdentifierRef (const bool is) { m_isNameIdentifierRef  = is; }
        bool GetIsNameIdentifierRef () const { return m_isNameIdentifierRef ; }

        // Fake global ->
        //    1) new Function code's global code
        //    2) global code generated from the reparsing deferred parse function
        bool IsFakeGlobalFunc(uint32 flags) const;

        void SetIsGlobalFunc(bool is) { m_isGlobalFunc = is; }
        bool GetIsStrictMode() const { return m_isStrictMode; }
        void SetIsStrictMode() { m_isStrictMode = true; }
        bool GetIsAsmjsMode() const { return m_isAsmjsMode; }
        void SetIsAsmjsMode(bool value)
        {
            m_isAsmjsMode = value;
    #if DBG
            if (value)
            {
                m_wasEverAsmjsMode = true;
            }
    #endif
        }

        bool GetHasImplicitArgIns() { return m_hasImplicitArgIns; }
        void SetHasImplicitArgIns(bool has) { m_hasImplicitArgIns = has; }
        uint32 GetGrfscr() const;
        void SetGrfscr(uint32 grfscr);

        ///----------------------------------------------------------------------------
        ///
        /// ParseableFunctionInfo::GetInParamsCount
        ///
        /// GetInParamsCount() returns the number of "in parameters" that have
        /// currently been declared for this function:
        /// - If this is "RegSlot_VariableCount", the function takes a variable number
        ///   of parameters.
        ///
        /// Consider: Change to store type information about parameters- names, type,
        /// direction, etc.
        ///
        ///----------------------------------------------------------------------------
        ArgSlot GetInParamsCount() const { return m_inParamCount; }

        void SetInParamsCount(ArgSlot newInParamCount);
        ArgSlot GetReportedInParamsCount() const;
        void SetReportedInParamsCount(ArgSlot newReportedInParamCount);
        void ResetInParams();
        ScopeInfo* GetScopeInfo() const { return static_cast<ScopeInfo*>(this->GetAuxPtr(AuxPointerType::ScopeInfo)); }
        void SetScopeInfo(ScopeInfo* scopeInfo) {  this->SetAuxPtr(AuxPointerType::ScopeInfo, scopeInfo); }
        PropertyId GetOrAddPropertyIdTracked(JsUtil::CharacterBuffer<WCHAR> const& propName);
        bool IsTrackedPropertyId(PropertyId pid);
        Js::PropertyRecordList* GetBoundPropertyRecords() { return this->m_boundPropertyRecords; }
        void SetBoundPropertyRecords(Js::PropertyRecordList* boundPropertyRecords)
        {
            Assert(this->m_boundPropertyRecords == nullptr);
            this->m_boundPropertyRecords = boundPropertyRecords;
        }
        void ClearBoundPropertyRecords()
        {
            this->m_boundPropertyRecords = nullptr;
        }
        virtual ParseableFunctionInfo* Clone(ScriptContext *scriptContext, uint sourceIndex = Js::Constants::InvalidSourceIndex);
        ParseableFunctionInfo* CopyFunctionInfoInto(ScriptContext *scriptContext, Js::ParseableFunctionInfo* functionInfo, uint sourceIndex = Js::Constants::InvalidSourceIndex);
        void CloneSourceInfo(ScriptContext* scriptContext, const ParseableFunctionInfo& other, ScriptContext* othersScriptContext, uint sourceIndex);

        void SetInitialDefaultEntryPoint();
        void SetDeferredParsingEntryPoint();

        void SetEntryPoint(ProxyEntryPointInfo* entryPoint, Js::JavascriptMethod jsMethod) {
            entryPoint->jsMethod = jsMethod;
        }

        bool IsDynamicScript() const;

        uint LengthInBytes() const { return m_cbLength; }
        uint StartOffset() const;
        ULONG GetLineNumber() const;
        ULONG GetColumnNumber() const;
        template <class T>
        LPCWSTR GetSourceName(const T& sourceContextInfo) const;
        template <class T>
        static LPCWSTR GetSourceName(const T& sourceContextInfo, bool m_isEval, bool m_isDynamicFunction);
        LPCWSTR GetSourceName() const;
        ULONG GetRelativeLineNumber() const { return m_lineNumber; }
        ULONG GetRelativeColumnNumber() const { return m_columnNumber; }
        uint GetSourceIndex() const;
        LPCUTF8 GetSource(const  char16* reason = nullptr) const;
        charcount_t LengthInChars() const { return m_cchLength; }
        charcount_t StartInDocument() const;
        bool IsEval() const { return m_isEval; }
        bool IsDynamicFunction() const;
        bool GetDontInline() { return m_dontInline; }
        void SetDontInline(bool is) { m_dontInline = is; }
        LPCUTF8 GetStartOfDocument(const char16* reason = nullptr) const;
        bool IsReparsed() const { return m_reparsed; }
        void SetReparsed(bool set) { m_reparsed = set; }
        bool GetExternalDisplaySourceName(BSTR* sourceName);

        void SetDoBackendArgumentsOptimization(bool set)
        {
            if (m_doBackendArgumentsOptimization)
            {
                m_doBackendArgumentsOptimization = set;
            }
        }

        bool GetDoBackendArgumentsOptimization()
        {
            return m_doBackendArgumentsOptimization;
        }

        void SetUsesArgumentsObject(bool set)
        {
            if (!m_usesArgumentsObject)
            {
                m_usesArgumentsObject = set;
            }
        }

        bool GetUsesArgumentsObject()
        {
            return m_usesArgumentsObject;
        }

        bool IsFunctionParsed()
        {
            return !IsDeferredParseFunction() || m_hasBeenParsed;
        }

        void SetFunctionParsed(bool hasBeenParsed)
        {
            m_hasBeenParsed = hasBeenParsed;
        }

        void SetSourceInfo(uint sourceIndex, ParseNodePtr node, bool isEval, bool isDynamicFunction);
        void Copy(FunctionBody* other);

        const char16* GetExternalDisplayName() const;

        //
        // Algorithm to retrieve a function body's external display name. Template supports both
        // local FunctionBody and ScriptDAC (debugging) scenarios.
        //
        template <class T>
        static const char16* GetExternalDisplayName(const T* funcBody)
        {
            Assert(funcBody != nullptr);
            Assert(funcBody->GetDisplayName() != nullptr);

            return funcBody->GetDisplayName();
        }

        virtual const char16* GetDisplayName() const override;
        void SetDisplayName(const char16* displayName);
        virtual void SetDisplayName(const char16* displayName, uint displayNameLength, uint displayShortNameOffset, SetDisplayNameFlags flags = SetDisplayNameFlagsNone) override;

        virtual void Finalize(bool isShutdown) override;

        Var GetCachedSourceString() { return this->GetAuxPtr(AuxPointerType::CachedSourceString); }
        void SetCachedSourceString(Var sourceString)
        {
            Assert(this->GetCachedSourceString() == nullptr);
            this->SetAuxPtr(AuxPointerType::CachedSourceString, sourceString);
        }

        FunctionProxyArray GetNestedFuncArray();
        FunctionProxy* GetNestedFunc(uint index);
        FunctionProxyPtrPtr GetNestedFuncReference(uint index);
        ParseableFunctionInfo* GetNestedFunctionForExecution(uint index);
        void SetNestedFunc(FunctionProxy* nestedFunc, uint index, uint32 flags);
        void ClearNestedFunctionParentFunctionReference();

        void SetCapturesThis() { attributes = (Attributes)(attributes | Attributes::CapturesThis); }
        bool GetCapturesThis() { return (attributes & Attributes::CapturesThis) != 0; }

        void BuildDeferredStubs(ParseNode *pnodeFnc);
        DeferredFunctionStub *GetDeferredStubs() const { return static_cast<DeferredFunctionStub *>(this->GetAuxPtr(AuxPointerType::DeferredStubs)); }
        void SetDeferredStubs(DeferredFunctionStub *stub) { this->SetAuxPtr(AuxPointerType::DeferredStubs, stub); }
        void RegisterFuncToDiag(ScriptContext * scriptContext, char16 const * pszTitle);
    protected:
        static HRESULT MapDeferredReparseError(HRESULT& hrParse, const CompileScriptException& se);

        bool m_hasBeenParsed : 1;       // Has function body been parsed- true for actual function bodies, false for deferparse
        bool m_isDeclaration : 1;
        bool m_isAccessor : 1;          // Function is a property getter or setter
        bool m_isStaticNameFunction : 1;
        bool m_isNamedFunctionExpression : 1;
        bool m_isNameIdentifierRef  : 1;
        bool m_isClassMember : 1;
        bool m_isStrictMode : 1;
        bool m_isAsmjsMode : 1;
        bool m_isAsmJsFunction : 1;
        bool m_isGlobalFunc : 1;
        bool m_doBackendArgumentsOptimization : 1;
        bool m_usesArgumentsObject : 1;
        bool m_isEval : 1;              // Source code is in 'eval'
        bool m_isDynamicFunction : 1;   // Source code is in 'Function'
        bool m_hasImplicitArgIns : 1;
        bool m_dontInline : 1;            // Used by the JIT's inliner

        // Indicates if the function has been reparsed for debug attach/detach scenario.
        bool m_reparsed : 1;

        // This field is not required for deferred parsing but because our thunks can't handle offsets > 128 bytes
        // yet, leaving this here for now. We can look at optimizing the function info and function proxy structures some
        // more and also fix our thunks to handle 8 bit offsets

        NoWriteBarrierField<bool> m_utf8SourceHasBeenSet;          // start of UTF8-encoded source
        NoWriteBarrierField<uint> m_sourceIndex;             // index into the scriptContext's list of saved sources
#if DYNAMIC_INTERPRETER_THUNK
        void* m_dynamicInterpreterThunk;  // Unique 'thunk' for every interpreted function - used for ETW symbol decoding.
#endif
        NoWriteBarrierField<uint> m_cbStartOffset;         // pUtf8Source is this many bytes from the start of the scriptContext's source buffer.

        // This is generally the same as m_cchStartOffset unless the buffer has a BOM

#define DEFINE_PARSEABLE_FUNCTION_INFO_FIELDS 1
#define CURRENT_ACCESS_MODIFIER protected:
#include "SerializableFunctionFields.h"

        ULONG m_lineNumber;
        ULONG m_columnNumber;
        WriteBarrierPtr<const char16> m_displayName;  // Optional name
        uint m_displayNameLength;
        WriteBarrierPtr<PropertyRecordList> m_boundPropertyRecords;
        WriteBarrierPtr<NestedArray> nestedArray;

    public:
#if DBG
        bool m_wasEverAsmjsMode; // has m_isAsmjsMode ever been true
        NoWriteBarrierField<Js::LocalFunctionId> deferredParseNextFunctionId;
#endif
#if DBG
        NoWriteBarrierField<UINT> scopeObjectSize; // If the scope is an activation object - its size
#endif
    };

    //
    // Algorithm to retrieve a function body's source name (url). Template supports both
    // local FunctionBody and ScriptDAC (debugging) scenarios.
    //
    template <class T>
    LPCWSTR ParseableFunctionInfo::GetSourceName(const T& sourceContextInfo) const
    {
        return GetSourceName<T>(sourceContextInfo, this->m_isEval, this->m_isDynamicFunction);
    }

    template <class T>
    LPCWSTR ParseableFunctionInfo::GetSourceName(const T& sourceContextInfo, bool m_isEval, bool m_isDynamicFunction)
    {
        if (sourceContextInfo->IsDynamic())
        {
            if (m_isEval)
            {
                return Constants::EvalCode;
            }
            else if (m_isDynamicFunction)
            {
                return Constants::FunctionCode;
            }
            else
            {
                return Constants::UnknownScriptCode;
            }
        }
        else
        {
            return sourceContextInfo->url;
        }
    }

    class FunctionBody : public ParseableFunctionInfo
    {
        DEFINE_VTABLE_CTOR_NO_REGISTER(FunctionBody, ParseableFunctionInfo);

        friend class ByteCodeBufferBuilder;
        friend class ByteCodeBufferReader;
        public:
            // same as MachDouble, used in the Func.h
            static const uint DIAGLOCALSLOTSIZE = 8;

            enum class CounterFields : uint8
            {
                VarCount                                = 0,
                ConstantCount                           = 1,
                OutParamMaxDepth                        = 2,
                ByteCodeCount                           = 3,
                ByteCodeWithoutLDACount                 = 4,
                ByteCodeInLoopCount                     = 5,
                LoopCount                               = 6,
                InlineCacheCount                        = 7,
                RootObjectLoadInlineCacheStart          = 8,
                RootObjectLoadMethodInlineCacheStart    = 9,
                RootObjectStoreInlineCacheStart         = 10,
                IsInstInlineCacheCount                  = 11,
                ReferencedPropertyIdCount               = 12,
                ObjLiteralCount                         = 13,
                LiteralRegexCount                       = 14,
                InnerScopeCount                         = 15,

                // Following counters uses ((uint32)-1) as default value
                LocalClosureRegister                    = 16,
                ParamClosureRegister                    = 17,
                LocalFrameDisplayRegister               = 18,
                EnvRegister                             = 19,
                ThisRegisterForEventHandler             = 20,
                FirstInnerScopeRegister                 = 21,
                FuncExprScopeRegister                   = 22,
                FirstTmpRegister                        = 23,

                Max
            };

            typedef CompactCounters<FunctionBody> CounterT;
            CounterT counters;

            uint32 GetCountField(FunctionBody::CounterFields fieldEnum) const
            {
                return counters.Get(fieldEnum);
            }
            uint32 SetCountField(FunctionBody::CounterFields fieldEnum, uint32 val)
            {
                return counters.Set(fieldEnum, val, this);
            }
            uint32 IncreaseCountField(FunctionBody::CounterFields fieldEnum)
            {
                return counters.Increase(fieldEnum, this);
            }

            struct StatementMap
            {
                StatementMap() : isSubexpression(false) {}

                static StatementMap * New(Recycler* recycler)
                {
                    return RecyclerNew(recycler, StatementMap);
                }

                regex::Interval sourceSpan;
                regex::Interval byteCodeSpan;
                bool isSubexpression;
            };

            // The type of StatementAdjustmentRecord.
            // A bitmask that can be OR'ed of multiple values of the enum.
            enum StatementAdjustmentType : ushort
            {
                SAT_None = 0,

                // Specifies an adjustment for next statement when going from current to next.
                // Used for transitioning from current stmt to next during normal control-flow,
                // such as offset of Br after if-block when there is else block present,
                // when throw happens inside if and we ignore exceptions (next statement in the list
                // would be 'else' but we need to pass flow control to Br target rather than entering 'else').
                SAT_FromCurrentToNext = 0x01,

                // Specifies an adjustment for beginning of next statement.
                // If there is adjustment record, the statement following it starts at specified offset and not at offset specified in statementMap.
                // Used for set next statement from arbitrary location.
                SAT_NextStatementStart = 0x02,

                SAT_All = SAT_FromCurrentToNext | SAT_NextStatementStart
            };

            class StatementAdjustmentRecord
            {
                uint m_byteCodeOffset;
                StatementAdjustmentType m_adjustmentType;
            public:
                StatementAdjustmentRecord();
                StatementAdjustmentRecord(StatementAdjustmentType type, int byteCodeOffset);
                StatementAdjustmentRecord(const StatementAdjustmentRecord& other);
                uint GetByteCodeOffset();
                StatementAdjustmentType GetAdjustmentType();
            };

            // Offset and entry/exit of a block that must be processed in new interpreter frame rather than current.
            // Used for try and catch blocks.
            class CrossFrameEntryExitRecord
            {
                uint m_byteCodeOffset;
                // true means enter, false means exit.
                bool m_isEnterBlock;
            public:
                CrossFrameEntryExitRecord();
                CrossFrameEntryExitRecord(uint byteCodeOffset, bool isEnterBlock);
                CrossFrameEntryExitRecord(const CrossFrameEntryExitRecord& other);
                uint GetByteCodeOffset() const;
                bool GetIsEnterBlock();
            };

            typedef JsUtil::List<Js::FunctionBody::StatementMap*> StatementMapList;

            // Note: isLeaf = true template param below means that recycler should not be used to dispose the items.
            typedef JsUtil::List<StatementAdjustmentRecord, Recycler, /* isLeaf = */ true> StatementAdjustmentRecordList;
            typedef JsUtil::List<CrossFrameEntryExitRecord, Recycler, /* isLeaf = */ true> CrossFrameEntryExitRecordList;

            // Contains recorded at bytecode generation time information about statements and try-catch blocks.
            // Used by debugger.
            struct AuxStatementData
            {
                // Contains statement adjustment data:
                // For given bytecode, following statement needs an adjustment, see StatementAdjustmentType for details.
                StatementAdjustmentRecordList* m_statementAdjustmentRecords;

                // Contain data about entry/exit of blocks that cause processing in different interpreter stack frame, such as try or catch.
                CrossFrameEntryExitRecordList* m_crossFrameBlockEntryExisRecords;

                AuxStatementData();
            };

            class SourceInfo
            {
                friend class RemoteFunctionBody;
                friend class ByteCodeBufferReader;
                friend class ByteCodeBufferBuilder;

            public:
                SmallSpanSequence * pSpanSequence;

                RegSlot         frameDisplayRegister;   // this register slot cannot be 0 so we use that sentinel value to indicate invalid
                RegSlot         objectRegister;         // this register slot cannot be 0 so we use that sentinel value to indicate invalid
                WriteBarrierPtr<ScopeObjectChain> pScopeObjectChain;
                WriteBarrierPtr<ByteBlock> m_probeBackingBlock; // NULL if no Probes, otherwise a copy of the unmodified the byte-codeblock //Delay
                int32 m_probeCount;             // The number of installed probes (such as breakpoints).

                // List of bytecode offset for the Branch bytecode.
                WriteBarrierPtr<AuxStatementData> m_auxStatementData;

                SourceInfo():
                    frameDisplayRegister(0),
                    objectRegister(0),
                    pScopeObjectChain(nullptr),
                    m_probeBackingBlock(nullptr),
                    m_probeCount(0),
                    m_auxStatementData(nullptr),
                    pSpanSequence(nullptr)
                {
                }
            };

    private:
        WriteBarrierPtr<ByteBlock> byteCodeBlock;               // Function byte-code for script functions
        WriteBarrierPtr<FunctionEntryPointList> entryPoints;
        WriteBarrierPtr<Var> m_constTable;
        WriteBarrierPtr<void*> inlineCaches;
        InlineCachePointerArray<PolymorphicInlineCache> polymorphicInlineCaches; // Contains the latest polymorphic inline caches
        WriteBarrierPtr<PropertyId> cacheIdToPropertyIdMap;

#if DBG
#define InlineCacheTypeNone         0x00
#define InlineCacheTypeInlineCache  0x01
#define InlineCacheTypeIsInst       0x02
            WriteBarrierPtr<byte> m_inlineCacheTypes;
#endif
    public:
        PropertyId * GetCacheIdToPropertyIdMap()
        {
            return cacheIdToPropertyIdMap;
        }
        static DWORD GetAsmJsTotalLoopCountOffset() { return offsetof(FunctionBody, m_asmJsTotalLoopCount); }
#if DBG
        int m_DEBUG_executionCount;     // Count of outstanding on InterpreterStackFrame
        bool m_nativeEntryPointIsInterpreterThunk; // NativeEntry entry point is in fact InterpreterThunk.
                                                   // Set by bgjit in OutOfMemory scenario during codegen.
#endif

//#if ENABLE_DEBUG_CONFIG_OPTIONS //TODO: need this?
        NoWriteBarrierField<uint> regAllocStoreCount;
        NoWriteBarrierField<uint> regAllocLoadCount;
        NoWriteBarrierField<uint> callCountStats;
//#endif

        // >>>>>>WARNING! WARNING!<<<<<<<<<<
        //
        // If you add compile-time attributes to this set, be sure to add them to the attributes that are
        // copied in FunctionBody::Clone
        //
        SourceInfo m_sourceInfo; // position of the source

        // Data needed by profiler:
        NoWriteBarrierField<uint> m_uScriptId; // Delay //Script Block it belongs to. This is function no. of the global function created by engine for each block
#if DBG
        NoWriteBarrierField<int> m_iProfileSession; // Script profile session the meta data of this function is reported to.
#endif // DEBUG

        // R0 is reserved for the return value, R1 for the root object
        static const RegSlot ReturnValueRegSlot = 0;
        static const RegSlot RootObjectRegSlot = 1;
        static const RegSlot FirstRegSlot = 1;
        // This value be set on the stack (on a particular offset), when the frame value got changed.
        static const int LocalsChangeDirtyValue = 1;

        enum FunctionBodyFlags : byte
        {
            Flags_None                     = 0x00,
            Flags_StackNestedFunc          = 0x01,
            Flags_HasOrParentHasArguments  = 0x02,
            Flags_HasTry                   = 0x04,
            Flags_HasThis                  = 0x08,
            Flags_NonUserCode              = 0x10,
            Flags_HasOnlyThisStatements    = 0x20,
            Flags_HasNoExplicitReturnValue = 0x40,   // Returns undefined, i.e. has no return statements or return with no expression
            Flags_HasRestParameter         = 0x80
        };

#define DEFINE_FUNCTION_BODY_FIELDS 1
#define CURRENT_ACCESS_MODIFIER public:
#include "SerializableFunctionFields.h"

    private:
        bool m_tag : 1;                     // Used to tag the low bit to prevent possible GC false references
        bool m_nativeEntryPointUsed : 1;    // Code might have been generated but not yet used.
        bool hasDoneLoopBodyCodeGen : 1;    // Code generated for loop body, but not necessary available to execute yet.
        bool m_isFuncRegistered : 1;
        bool m_isFuncRegisteredToDiag : 1; // Mentions the function's context is registered with diagprobe.
        bool funcEscapes : 1;
        bool m_hasBailoutInstrInJittedCode : 1; // Indicates whether function has bailout instructions. Valid only if hasDoneCodeGen is true
        bool m_pendingLoopHeaderRelease : 1; // Indicates whether loop headers need to be released
        bool hasExecutionDynamicProfileInfo : 1;

        bool cleanedUp: 1;
        bool sourceInfoCleanedUp: 1;
        bool dontRethunkAfterBailout : 1;
        bool disableInlineApply : 1;
        bool disableInlineSpread : 1;
        bool hasHotLoop: 1;
        bool wasCalledFromLoop : 1;
        bool hasNestedLoop : 1;
        bool recentlyBailedOutOfJittedLoopBody : 1;
        bool m_firstFunctionObject: 1;
        bool m_inlineCachesOnFunctionObject: 1;
        // Used for the debug re-parse. Saves state of function on the first parse, and restores it on a reparse. The state below is either dependent on
        // the state of the script context, or on other factors like whether it was defer parsed or not.
        bool m_hasSetIsObject : 1;
        // Used for the debug purpose, this info will be stored (in the non-debug mode), when a function has all locals marked as non-local-referenced.
        // So when we got to no-refresh debug mode, and try to re-use the same function body we can then enforce all locals to be non-local-referenced.
        bool m_hasAllNonLocalReferenced : 1;
        bool m_hasFunExprNameReference : 1;
        bool m_ChildCallsEval : 1;
        bool m_CallsEval : 1;
        bool m_hasReferenceableBuiltInArguments : 1;
        bool m_isParamAndBodyScopeMerged : 1;

        // Used in the debug purpose. This is to avoid setting all locals to non-local-referenced, multiple times for each child function.
        bool m_hasDoneAllNonLocalReferenced : 1;

        // Used by the script profiler, once the function compiled is sent this will be set to true.
        bool m_hasFunctionCompiledSent : 1;

        bool m_isFromNativeCodeModule : 1;
        bool m_isPartialDeserializedFunction : 1;
        bool m_isAsmJsScheduledForFullJIT : 1;
        bool m_hasLocalClosureRegister : 1;
        bool m_hasParamClosureRegister : 1;
        bool m_hasLocalFrameDisplayRegister : 1;
        bool m_hasEnvRegister : 1;
        bool m_hasThisRegisterForEventHandler : 1;
        bool m_hasFirstInnerScopeRegister : 1;
        bool m_hasFuncExprScopeRegister : 1;
        bool m_hasFirstTmpRegister : 1;
#if DBG
        bool m_isSerialized : 1;
#endif
#ifdef PERF_COUNTERS
        bool m_isDeserializedFunction : 1;
#endif
#if DBG
        // Indicates that nested functions can be allocated on the stack (but may not be)
        bool m_canDoStackNestedFunc : 1;
#endif

#if DBG
        bool initializedExecutionModeAndLimits : 1;
#endif

#ifdef IR_VIEWER
        // whether IR Dump is enabled for this function (used by parseIR)
        bool m_isIRDumpEnabled : 1;
        WriteBarrierPtr<Js::DynamicObject> m_irDumpBaseObject;
#endif /* IR_VIEWER */

        NoWriteBarrierField<uint8> bailOnMisingProfileCount;
        NoWriteBarrierField<uint8> bailOnMisingProfileRejitCount;

        NoWriteBarrierField<byte> inlineDepth; // Used by inlining to avoid recursively inlining functions excessively

        NoWriteBarrierField<ExecutionMode> executionMode;
        NoWriteBarrierField<uint16> interpreterLimit;
        NoWriteBarrierField<uint16> autoProfilingInterpreter0Limit;
        NoWriteBarrierField<uint16> profilingInterpreter0Limit;
        NoWriteBarrierField<uint16> autoProfilingInterpreter1Limit;
        NoWriteBarrierField<uint16> simpleJitLimit;
        NoWriteBarrierField<uint16> profilingInterpreter1Limit;
        NoWriteBarrierField<uint16> fullJitThreshold;
        NoWriteBarrierField<uint16> fullJitRequeueThreshold;
        NoWriteBarrierField<uint16> committedProfiledIterations;

        NoWriteBarrierField<uint> m_depth; // Indicates how many times the function has been entered (so increases by one on each recursive call, decreases by one when we're done)

        uint32 interpretedCount;
        uint32 loopInterpreterLimit;
        uint32 debuggerScopeIndex;
        uint32 savedPolymorphicCacheState;

        // >>>>>>WARNING! WARNING!<<<<<<<<<<
        //
        // If you add compile-time attributes to the above set, be sure to add them to the attributes that are
        // copied in FunctionBody::Clone
        //

        NoWriteBarrierPtr<Js::ByteCodeCache> byteCodeCache;  // Not GC allocated so naked pointer
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        static bool shareInlineCaches;
#endif
        WriteBarrierPtr<FunctionEntryPointInfo> defaultFunctionEntryPointInfo;

#if ENABLE_PROFILE_INFO 
        WriteBarrierPtr<DynamicProfileInfo> dynamicProfileInfo;
#endif


        // select dynamic profile info saved off when we codegen and later
        // used for rejit decisions (see bailout.cpp)
        NoWriteBarrierField<BYTE> savedInlinerVersion;
#if ENABLE_NATIVE_CODEGEN
        NoWriteBarrierField<ImplicitCallFlags> savedImplicitCallsFlags;
#endif

        FunctionBody(ScriptContext* scriptContext, const char16* displayName, uint displayNameLength, uint displayShortNameOffset, uint nestedCount, Utf8SourceInfo* sourceInfo,
            uint uFunctionNumber, uint uScriptId, Js::LocalFunctionId functionId, Js::PropertyRecordList* propRecordList, Attributes attributes
#ifdef PERF_COUNTERS
            , bool isDeserializedFunction = false
#endif
            );

        void SetNativeEntryPoint(FunctionEntryPointInfo* entryPointInfo, JavascriptMethod originalEntryPoint, Var directEntryPoint);
#if DYNAMIC_INTERPRETER_THUNK
        void GenerateDynamicInterpreterThunk();
#endif
        void CloneByteCodeInto(ScriptContext * scriptContext, FunctionBody *newFunctionBody, uint sourceIndex);
        Js::JavascriptMethod GetEntryPoint(ProxyEntryPointInfo* entryPoint) const { return entryPoint->jsMethod; }
        void CaptureDynamicProfileState(FunctionEntryPointInfo* entryPointInfo);
#if ENABLE_DEBUG_CONFIG_OPTIONS
        void DumpRegStats(FunctionBody *funcBody);
#endif

    public:
        FunctionBody(ByteCodeCache* cache, Utf8SourceInfo* sourceInfo, ScriptContext* scriptContext):
            ParseableFunctionInfo((JavascriptMethod) nullptr, 0, (LocalFunctionId) 0, sourceInfo, scriptContext, 0, nullptr, 0, 0, None, nullptr)
        {
            // Dummy constructor- does nothing
            // Must be stack allocated
            // Used during deferred bytecode serialization
        }

        static FunctionBody * NewFromRecycler(Js::ScriptContext * scriptContext, const char16 * displayName, uint displayNameLength, uint displayShortNameOffset, uint nestedCount,
            Utf8SourceInfo* sourceInfo, uint uScriptId, Js::LocalFunctionId functionId, Js::PropertyRecordList* boundPropertyRecords, Attributes attributes
#ifdef PERF_COUNTERS
            , bool isDeserializedFunction
#endif
            );
        static FunctionBody * NewFromRecycler(Js::ScriptContext * scriptContext, const char16 * displayName, uint displayNameLength, uint displayShortNameOffset, uint nestedCount,
            Utf8SourceInfo* sourceInfo, uint uFunctionNumber, uint uScriptId, Js::LocalFunctionId functionId, Js::PropertyRecordList* boundPropertyRecords, Attributes attributes
#ifdef PERF_COUNTERS
            , bool isDeserializedFunction
#endif
            );

        FunctionEntryPointInfo * GetEntryPointInfo(int index) const;
        FunctionEntryPointInfo * TryGetEntryPointInfo(int index) const;

        Js::RootObjectBase * LoadRootObject() const;
        Js::RootObjectBase * GetRootObject() const;
        ByteBlock* GetAuxiliaryData() const { return static_cast<ByteBlock*>(this->GetAuxPtr(AuxPointerType::AuxBlock)); }
        ByteBlock* GetAuxiliaryDataWithLock() const { return static_cast<ByteBlock*>(this->GetAuxPtrWithLock(AuxPointerType::AuxBlock)); }
        void SetAuxiliaryData(ByteBlock* auxBlock) { this->SetAuxPtr(AuxPointerType::AuxBlock, auxBlock); }
        ByteBlock* GetAuxiliaryContextData()const { return static_cast<ByteBlock*>(this->GetAuxPtr(AuxPointerType::AuxContextBlock)); }
        ByteBlock* GetAuxiliaryContextDataWithLock()const { return static_cast<ByteBlock*>(this->GetAuxPtrWithLock(AuxPointerType::AuxContextBlock)); }
        void SetAuxiliaryContextData(ByteBlock* auxContextBlock) { this->SetAuxPtr(AuxPointerType::AuxContextBlock, auxContextBlock); }
        void SetFormalsPropIdArray(PropertyIdArray * propIdArray);
        PropertyIdArray* GetFormalsPropIdArray(bool checkForNull = true);
        Var GetFormalsPropIdArrayOrNullObj();
        ByteBlock* GetByteCode();
        ByteBlock* GetOriginalByteCode(); // Returns original bytecode without probes (such as BPs).
        Js::ByteCodeCache * GetByteCodeCache() const { return this->byteCodeCache; }
        void SetByteCodeCache(Js::ByteCodeCache *byteCodeCache)
        {
            if (byteCodeCache != nullptr)
            {
                this->byteCodeCache = byteCodeCache;
            }
        }
#if DBG
        void SetIsSerialized(bool serialized) { m_isSerialized = serialized; }
        bool GetIsSerialized()const { return m_isSerialized; }
#endif
        uint GetByteCodeCount() const { return GetCountField(CounterFields::ByteCodeCount); }
        void SetByteCodeCount(uint count) { SetCountField(CounterFields::ByteCodeCount, count); }
        uint GetByteCodeWithoutLDACount() const { return GetCountField(CounterFields::ByteCodeWithoutLDACount); }
        void SetByteCodeWithoutLDACount(uint count) { SetCountField(CounterFields::ByteCodeWithoutLDACount, count); }
        uint GetByteCodeInLoopCount() const { return GetCountField(CounterFields::ByteCodeInLoopCount); }
        void SetByteCodeInLoopCount(uint count) { SetCountField(CounterFields::ByteCodeInLoopCount, count); }
        uint16 GetEnvDepth() const { return m_envDepth; }
        void SetEnvDepth(uint16 depth) { m_envDepth = depth; }

        void SetEnvRegister(RegSlot reg);
        void MapAndSetEnvRegister(RegSlot reg);
        RegSlot GetEnvRegister() const;
        void SetThisRegisterForEventHandler(RegSlot reg);
        void MapAndSetThisRegisterForEventHandler(RegSlot reg);
        RegSlot GetThisRegisterForEventHandler() const;

        void SetLocalClosureRegister(RegSlot reg);
        void MapAndSetLocalClosureRegister(RegSlot reg);
        RegSlot GetLocalClosureRegister() const;
        void SetParamClosureRegister(RegSlot reg);
        void MapAndSetParamClosureRegister(RegSlot reg);
        RegSlot GetParamClosureRegister() const;

        void SetLocalFrameDisplayRegister(RegSlot reg);
        void MapAndSetLocalFrameDisplayRegister(RegSlot reg);
        RegSlot GetLocalFrameDisplayRegister() const;
        void SetFirstInnerScopeRegister(RegSlot reg);
        void MapAndSetFirstInnerScopeRegister(RegSlot reg);
        RegSlot GetFirstInnerScopeRegister() const;
        void SetFuncExprScopeRegister(RegSlot reg);
        void MapAndSetFuncExprScopeRegister(RegSlot reg);
        RegSlot GetFuncExprScopeRegister() const;

        bool HasScopeObject() const { return hasScopeObject; }
        void SetHasScopeObject(bool has) { hasScopeObject = has; }
        uint GetInnerScopeCount() const { return GetCountField(CounterFields::InnerScopeCount); }
        void SetInnerScopeCount(uint count) { SetCountField(CounterFields::InnerScopeCount, count); }
        bool HasCachedScopePropIds() const { return hasCachedScopePropIds; }
        void SetHasCachedScopePropIds(bool has) { hasCachedScopePropIds = has; }

        uint32 GetInterpretedCount() const { return interpretedCount; }
        uint32 SetInterpretedCount(uint32 val) { return interpretedCount = val; }
        uint32 IncreaseInterpretedCount() { return interpretedCount++; }

        uint32 GetLoopInterpreterLimit() const { return loopInterpreterLimit; }
        uint32 SetLoopInterpreterLimit(uint32 val) { return loopInterpreterLimit = val; }

        // Gets the next index for tracking debugger scopes (increments the internal counter as well).
        uint32 GetNextDebuggerScopeIndex() { return debuggerScopeIndex++; }
        void SetDebuggerScopeIndex(uint32 index) { debuggerScopeIndex = index; }

        size_t GetLoopBodyName(uint loopNumber, _Out_writes_opt_z_(sizeInChars) WCHAR* displayName, _In_ size_t sizeInChars);

        void AllocateLoopHeaders();
        void ReleaseLoopHeaders();
        Js::LoopHeader * GetLoopHeader(uint index) const;
        Js::LoopHeader * GetLoopHeaderWithLock(uint index) const;
        Js::Var GetLoopHeaderArrayPtr() const
        {
            Assert(this->GetLoopHeaderArray() != nullptr);
            return this->GetLoopHeaderArray();
        }
#ifndef TEMP_DISABLE_ASMJS
        void SetIsAsmJsFullJitScheduled(bool val){ m_isAsmJsScheduledForFullJIT = val; }
        bool GetIsAsmJsFullJitScheduled(){ return m_isAsmJsScheduledForFullJIT; }
        uint32 GetAsmJSTotalLoopCount() const
        {
            return m_asmJsTotalLoopCount;
        }

        void SetIsAsmJsFunction(bool isAsmJsFunction)
        {
            m_isAsmJsFunction = isAsmJsFunction;
        }
#endif

        const bool GetIsAsmJsFunction() const
        {
            return m_isAsmJsFunction;
        }

#ifndef TEMP_DISABLE_ASMJS
        bool IsHotAsmJsLoop()
        {
            // Negative MinTemplatizedJitLoopRunCount treats all loops as hot asm loop
            if (CONFIG_FLAG(MinTemplatizedJitLoopRunCount) < 0 || m_asmJsTotalLoopCount > static_cast<uint>(CONFIG_FLAG(MinTemplatizedJitLoopRunCount)))
            {
                return true;
            }
            return false;
        }
#endif

    private:
        void ResetLoops();

    public:
        static bool Is(void* ptr);
        uint GetScriptId() const { return m_uScriptId; }

        void* GetAddressOfScriptId() const
        {
            return (void*)&m_uScriptId;
        }


        static uint *GetJittedLoopIterationsSinceLastBailoutAddress(EntryPointInfo* info)
        {
            LoopEntryPointInfo* entryPoint = (LoopEntryPointInfo*)info;
            return &entryPoint->jittedLoopIterationsSinceLastBailout;
        }

        FunctionEntryPointInfo* GetDefaultFunctionEntryPointInfo() const;
        void SetDefaultFunctionEntryPointInfo(FunctionEntryPointInfo* entryPointInfo, const JavascriptMethod originalEntryPoint);

        FunctionEntryPointInfo *GetSimpleJitEntryPointInfo() const;
        void SetSimpleJitEntryPointInfo(FunctionEntryPointInfo *const entryPointInfo);

    private:
        void VerifyExecutionMode(const ExecutionMode executionMode) const;
    public:
        ExecutionMode GetDefaultInterpreterExecutionMode() const;
        ExecutionMode GetExecutionMode() const;
        ExecutionMode GetInterpreterExecutionMode(const bool isPostBailout);
        void SetExecutionMode(const ExecutionMode executionMode);
    private:
        bool IsInterpreterExecutionMode() const;

    public:
        bool TryTransitionToNextExecutionMode();
        void TryTransitionToNextInterpreterExecutionMode();
        void SetIsSpeculativeJitCandidate();
        bool TryTransitionToJitExecutionMode();
        void TransitionToSimpleJitExecutionMode();
        void TransitionToFullJitExecutionMode();

    private:
        void VerifyExecutionModeLimits();
        void InitializeExecutionModeAndLimits();
    public:
        void ReinitializeExecutionModeAndLimits();
    private:
        void SetFullJitThreshold(const uint16 newFullJitThreshold, const bool skipSimpleJit = false);
        void CommitExecutedIterations();
        void CommitExecutedIterations(uint16 &limit, const uint executedIterations);

    private:
        uint16 GetSimpleJitExecutedIterations() const;
    public:
        void ResetSimpleJitLimitAndCallCount();
    private:
        void SetSimpleJitCallCount(const uint16 simpleJitLimit) const;
        void ResetSimpleJitCallCount();
    public:
        uint16 GetProfiledIterations() const;

    public:
        void OnFullJitDequeued(const FunctionEntryPointInfo *const entryPointInfo);

    public:
        void TraceExecutionMode(const char *const eventDescription = nullptr) const;
        void TraceInterpreterExecutionMode() const;
    private:
        void DoTraceExecutionMode(const char *const eventDescription) const;

    public:
        bool DoSimpleJit() const;
        bool DoSimpleJitWithLock() const;
        bool DoSimpleJitDynamicProfile() const;

    private:
        bool DoInterpreterProfile() const;
        bool DoInterpreterProfileWithLock() const;
        bool DoInterpreterAutoProfile() const;

    public:
        bool WasCalledFromLoop() const;
        void SetWasCalledFromLoop();

    public:
        bool RecentlyBailedOutOfJittedLoopBody() const;
        void SetRecentlyBailedOutOfJittedLoopBody(const bool value);

    private:
        static uint16 GetMinProfileIterations();
    public:
        static uint16 GetMinFunctionProfileIterations();
    private:
        static uint GetMinLoopProfileIterations(const uint loopInterpreterLimit);
    public:
        uint GetLoopProfileThreshold(const uint loopInterpreterLimit) const;
    private:
        static uint GetReducedLoopInterpretCount();
    public:
        uint GetLoopInterpretCount(LoopHeader* loopHeader) const;

    private:
        static bool DoObjectHeaderInlining();
        static bool DoObjectHeaderInliningForConstructors();
    public:
        static bool DoObjectHeaderInliningForConstructor(const uint32 inlineSlotCapacity);
    private:
        static bool DoObjectHeaderInliningForObjectLiterals();
    public:
        static bool DoObjectHeaderInliningForObjectLiteral(const uint32 inlineSlotCapacity);
        static bool DoObjectHeaderInliningForObjectLiteral(const PropertyIdArray *const propIds);
        static bool DoObjectHeaderInliningForEmptyObjects();

    public:
#if DBG
        int GetProfileSession() { return m_iProfileSession; }
#endif
        virtual void Finalize(bool isShutdown) override;

        void Cleanup(bool isScriptContextClosing);
        void CleanupSourceInfo(bool isScriptContextClosing);
        template<bool IsScriptContextShutdown>
        void CleanUpInlineCaches();
        void CleanupRecyclerData(bool isRecyclerShutdown, bool doEntryPointCleanupCaptureStack);

#ifdef PERF_COUNTERS
        void CleanupPerfCounter();
#endif

        virtual void Dispose(bool isShutdown) override { }

        bool HasRejit() const
        {
            if(this->entryPoints)
            {
                return this->entryPoints->Count() > 1;
            }
            return false;
        }

#pragma region SourceInfo Methods
        void CopySourceInfo(ParseableFunctionInfo* originalFunctionInfo);
        void FinishSourceInfo();
        RegSlot GetFrameDisplayRegister() const;
        void SetFrameDisplayRegister(RegSlot frameDisplayRegister);

        RegSlot GetObjectRegister() const;
        void SetObjectRegister(RegSlot objectRegister);
        bool HasObjectRegister() const { return GetObjectRegister() != 0; }
        ScopeObjectChain *GetScopeObjectChain() const;
        void SetScopeObjectChain(ScopeObjectChain *pScopeObjectChain);

        // fetch the Catch scope object which encloses the passed bytecode offset, returns NULL otherwise
        Js::DebuggerScope * GetDiagCatchScopeObjectAt(int byteCodeOffset);

        ByteBlock *GetProbeBackingBlock();
        void SetProbeBackingBlock(ByteBlock* probeBackingBlock);

        bool HasLineBreak() const;
        bool HasLineBreak(charcount_t start, charcount_t end) const;

        bool HasGeneratedFromByteCodeCache() const { return this->byteCodeCache != nullptr; }

        bool EndsAfter(size_t offset) const;

        void TrackLoad(int ichMin);

        SmallSpanSequence* GetStatementMapSpanSequence() const { return m_sourceInfo.pSpanSequence; }
        void RecordStatementMap(StatementMap* statementMap);
        void RecordStatementMap(SmallSpanSequenceIter &iter, StatementData * data);
        void RecordLoad(int ichMin, int bytecodeAfterLoad);
        DebuggerScope* RecordStartScopeObject(DiagExtraScopesType scopeType, int start, RegSlot scopeLocation, int* index = nullptr);
        void RecordEndScopeObject(DebuggerScope* currentScope, int end);
        DebuggerScope* AddScopeObject(DiagExtraScopesType scopeType, int start, RegSlot scopeLocation);
        bool TryGetDebuggerScopeAt(int index, DebuggerScope*& debuggerScope);

        StatementMapList * GetStatementMaps() const { return static_cast<StatementMapList *>(this->GetAuxPtrWithLock(AuxPointerType::StatementMaps)); }
        void SetStatementMaps(StatementMapList *pStatementMaps) { this->SetAuxPtr(AuxPointerType::StatementMaps, pStatementMaps); }

        FunctionCodeGenRuntimeData ** GetCodeGenGetSetRuntimeData() const { return static_cast<FunctionCodeGenRuntimeData**>(this->GetAuxPtr(AuxPointerType::CodeGenGetSetRuntimeData)); }
        FunctionCodeGenRuntimeData ** GetCodeGenGetSetRuntimeDataWithLock() const { return static_cast<FunctionCodeGenRuntimeData**>(this->GetAuxPtrWithLock(AuxPointerType::CodeGenGetSetRuntimeData)); }
        void SetCodeGenGetSetRuntimeData(FunctionCodeGenRuntimeData** codeGenGetSetRuntimeData) { this->SetAuxPtr(AuxPointerType::CodeGenGetSetRuntimeData, codeGenGetSetRuntimeData); }

        FunctionCodeGenRuntimeData ** GetCodeGenRuntimeData() const { return static_cast<FunctionCodeGenRuntimeData**>(this->GetAuxPtr(AuxPointerType::CodeGenRuntimeData)); }
        FunctionCodeGenRuntimeData ** GetCodeGenRuntimeDataWithLock() const { return static_cast<FunctionCodeGenRuntimeData**>(this->GetAuxPtrWithLock(AuxPointerType::CodeGenRuntimeData)); }
        void SetCodeGenRuntimeData(FunctionCodeGenRuntimeData** codeGenRuntimeData) { this->SetAuxPtr(AuxPointerType::CodeGenRuntimeData, codeGenRuntimeData); }

        static StatementMap * GetNextNonSubexpressionStatementMap(StatementMapList *statementMapList, int & startingAtIndex);
        static StatementMap * GetPrevNonSubexpressionStatementMap(StatementMapList *statementMapList, int & startingAtIndex);
        void RecordStatementAdjustment(uint offset, StatementAdjustmentType adjType);
        void RecordCrossFrameEntryExitRecord(uint byteCodeOffset, bool isEnterBlock);

        // Find out an offset falls within the range. returns TRUE if found.
        BOOL GetBranchOffsetWithin(uint start, uint end, StatementAdjustmentRecord* record);
        bool GetLineCharOffset(int byteCodeOffset, ULONG* line, LONG* charOffset, bool canAllocateLineCache = true);
        bool GetLineCharOffsetFromStartChar(int startCharOfStatement, ULONG* _line, LONG* _charOffset, bool canAllocateLineCache = true);

        // Given bytecode position, returns the start position of the statement and length of the statement.
        bool GetStatementIndexAndLengthAt(int byteCodeOffset, UINT32* statementIndex, UINT32* statementLength);

        // skip any utf-8/utf-16 byte-order-mark. Returns the number of chars skipped.
        static charcount_t SkipByteOrderMark(__in_bcount_z(4) LPCUTF8& documentStart)
        {
            charcount_t retValue = 0;

            Assert(documentStart != nullptr);

            if (documentStart[0] == 0xEF &&
                documentStart[1] == 0xBB &&
                documentStart[2] == 0xBF)
            {
                // UTF-8     - EF BB BF
                // 3 bytes skipped - reports one char skipped
                documentStart += 3;
                retValue = 1;
            }
            else if ((documentStart[0] == 0xFF && documentStart[1] == 0xFE) ||
                    (documentStart[0] == 0xFE && documentStart[1] == 0xFF))
            {
                // UTF-16 LE - FF FE
                // UTF-16 BE - FE FF
                // 2 bytes skipped - reports one char skipped
                documentStart += 2;
                retValue = 1;
            }

            return retValue;
        }

        StatementMap* GetMatchingStatementMapFromByteCode(int byteCodeOffset, bool ignoreSubexpressions = false);
        int GetEnclosingStatementIndexFromByteCode(int byteCodeOffset, bool ignoreSubexpressions = false);
        StatementMap* GetEnclosingStatementMapFromByteCode(int byteCodeOffset, bool ignoreSubexpressions = false);
        StatementMap* GetMatchingStatementMapFromSource(int byteCodeOffset, int* pMapIndex = nullptr);
        void RecordFrameDisplayRegister(RegSlot slot);
        void RecordObjectRegister(RegSlot slot);

        CrossFrameEntryExitRecordList* GetCrossFrameEntryExitRecords();

#ifdef VTUNE_PROFILING
        uint GetStartOffset(uint statementIndex) const;
        ULONG GetSourceLineNumber(uint statementIndex);
#endif

#pragma endregion

        // Field accessors
        bool GetHasBailoutInstrInJittedCode() const { return this->m_hasBailoutInstrInJittedCode; }
        void SetHasBailoutInstrInJittedCode(bool hasBailout) { this->m_hasBailoutInstrInJittedCode = hasBailout; }
        bool GetCanReleaseLoopHeaders() const { return (this->m_depth == 0); }
        void SetPendingLoopHeaderRelease(bool pendingLoopHeaderRelease) { this->m_pendingLoopHeaderRelease = pendingLoopHeaderRelease; }

        bool GetIsFromNativeCodeModule() const { return m_isFromNativeCodeModule; }
        void SetIsFromNativeCodeModule(bool isFromNativeCodeModule) { m_isFromNativeCodeModule = isFromNativeCodeModule; }

        uint GetLoopNumber(LoopHeader const * loopHeader) const;
        uint GetLoopNumberWithLock(LoopHeader const * loopHeader) const;
        bool GetHasAllocatedLoopHeaders() { return this->GetLoopHeaderArray() != nullptr; }
        Js::LoopHeader* GetLoopHeaderArray() const { return static_cast<Js::LoopHeader*>(this->GetAuxPtr(AuxPointerType::LoopHeaderArray)); }
        Js::LoopHeader* GetLoopHeaderArrayWithLock() const { return static_cast<Js::LoopHeader*>(this->GetAuxPtrWithLock(AuxPointerType::LoopHeaderArray)); }
        void SetLoopHeaderArray(Js::LoopHeader* loopHeaderArray) { this->SetAuxPtr(AuxPointerType::LoopHeaderArray, loopHeaderArray); }

#if ENABLE_NATIVE_CODEGEN
        Js::JavascriptMethod GetLoopBodyEntryPoint(Js::LoopHeader * loopHeader, int entryPointIndex);
        void SetLoopBodyEntryPoint(Js::LoopHeader * loopHeader, EntryPointInfo* entryPointInfo, Js::JavascriptMethod entryPoint);
#endif

        void RestoreOldDefaultEntryPoint(FunctionEntryPointInfo* oldEntryPoint, JavascriptMethod oldOriginalEntryPoint, FunctionEntryPointInfo* newEntryPoint);
        FunctionEntryPointInfo* CreateNewDefaultEntryPoint();
        void AddEntryPointToEntryPointList(FunctionEntryPointInfo* entryPoint);

        // Kind of entry point for original entry point
        BOOL IsInterpreterThunk() const;
        BOOL IsDynamicInterpreterThunk() const;
        BOOL IsNativeOriginalEntryPoint() const;
        bool IsSimpleJitOriginalEntryPoint() const;

#if DYNAMIC_INTERPRETER_THUNK
        static BYTE GetOffsetOfDynamicInterpreterThunk() { return offsetof(FunctionBody, m_dynamicInterpreterThunk); }
        void* GetDynamicInterpreterEntryPoint() const
        {
            return m_dynamicInterpreterThunk;
        }
        bool HasInterpreterThunkGenerated() const
        {
            return m_dynamicInterpreterThunk != nullptr;
        }

        DWORD GetDynamicInterpreterThunkSize() const;
#endif

        bool GetHasHotLoop() const { return hasHotLoop; };
        void SetHasHotLoop();

        bool GetHasNestedLoop() const { return hasNestedLoop; };
        void SetHasNestedLoop(bool nest) { hasNestedLoop = nest; };

        bool IsInlineApplyDisabled();
        void InitDisableInlineApply();
        void SetDisableInlineApply(bool set);

        bool IsInlineSpreadDisabled()  const  { return disableInlineSpread; }
        void InitDisableInlineSpread()        { disableInlineSpread = this->functionId != Js::Constants::NoFunctionId && PHASE_OFF(Js::InlinePhase, this); }
        void SetDisableInlineSpread(bool set) { disableInlineSpread = set; }

        bool CheckCalleeContextForInlining(FunctionProxy* calleeFunctionProxy);
#if DBG
        bool HasValidSourceInfo();
#endif
#if DYNAMIC_INTERPRETER_THUNK
        JavascriptMethod EnsureDynamicInterpreterThunk(FunctionEntryPointInfo* entryPointInfo);
#endif

        void SetCheckCodeGenEntryPoint(FunctionEntryPointInfo* entryPointInfo, JavascriptMethod entryPoint);

#if ENABLE_NATIVE_CODEGEN
        typedef void (*SetNativeEntryPointFuncType)(FunctionEntryPointInfo* entryPointInfo, Js::FunctionBody * functionBody, Js::JavascriptMethod entryPoint);
        static void DefaultSetNativeEntryPoint(FunctionEntryPointInfo* entryPointInfo, FunctionBody * functionBody, JavascriptMethod entryPoint);
        static void ProfileSetNativeEntryPoint(FunctionEntryPointInfo* entryPointInfo, FunctionBody * functionBody, JavascriptMethod entryPoint);

        bool GetNativeEntryPointUsed() const { return m_nativeEntryPointUsed; }
        void SetNativeEntryPointUsed(bool nativeEntryPointUsed) { this->m_nativeEntryPointUsed = nativeEntryPointUsed; }
#endif

        bool GetIsFuncRegistered() { return m_isFuncRegistered; }
        void SetIsFuncRegistered(bool isRegistered) { m_isFuncRegistered = isRegistered; }

        bool GetHasLoops() const { return this->GetLoopCount() != 0; }
        uint IncrLoopCount() { return this->IncreaseCountField(CounterFields::LoopCount); }
        uint GetLoopCount() const { return this->GetCountField(CounterFields::LoopCount); }
        uint SetLoopCount(uint count) { return this->SetCountField(CounterFields::LoopCount, count); }

        bool AllocProfiledDivOrRem(ProfileId* profileId) { if (this->profiledDivOrRemCount != Constants::NoProfileId) { *profileId = this->profiledDivOrRemCount++; return true; } return false; }
        ProfileId GetProfiledDivOrRemCount() { return this->profiledDivOrRemCount; }

        bool AllocProfiledSwitch(ProfileId* profileId) { if (this->profiledSwitchCount != Constants::NoProfileId) { *profileId = this->profiledSwitchCount++; return true; } return false; }
        ProfileId GetProfiledSwitchCount() { return this->profiledSwitchCount; }

        bool AllocProfiledCallSiteId(ProfileId* profileId) { if (this->profiledCallSiteCount != Constants::NoProfileId) { *profileId = this->profiledCallSiteCount++; return true; } return false; }
        ProfileId GetProfiledCallSiteCount() const { return this->profiledCallSiteCount; }
        void SetProfiledCallSiteCount(ProfileId callSiteId)  { this->profiledCallSiteCount = callSiteId; }

        bool AllocProfiledArrayCallSiteId(ProfileId* profileId) { if (this->profiledArrayCallSiteCount != Constants::NoProfileId) { *profileId = this->profiledArrayCallSiteCount++; return true; } return false; }
        ProfileId GetProfiledArrayCallSiteCount() const { return this->profiledArrayCallSiteCount; }

        bool AllocProfiledReturnTypeId(ProfileId* profileId) { if (this->profiledReturnTypeCount != Constants::NoProfileId) { *profileId = this->profiledReturnTypeCount++; return true; } return false; }
        ProfileId GetProfiledReturnTypeCount() const { return this->profiledReturnTypeCount; }

        bool AllocProfiledSlotId(ProfileId* profileId) { if (this->profiledSlotCount != Constants::NoProfileId) { *profileId = this->profiledSlotCount++; return true; } return false; }
        ProfileId GetProfiledSlotCount() const { return this->profiledSlotCount; }

        ProfileId AllocProfiledLdElemId(ProfileId* profileId) { if (this->profiledLdElemCount != Constants::NoProfileId) { *profileId = this->profiledLdElemCount++; return true; } return false; }
        ProfileId GetProfiledLdElemCount() const { return this->profiledLdElemCount; }

        bool AllocProfiledStElemId(ProfileId* profileId) { if (this->profiledStElemCount != Constants::NoProfileId) { *profileId = this->profiledStElemCount++; return true; } return false; }
        ProfileId GetProfiledStElemCount() const { return this->profiledStElemCount; }

        uint GetProfiledFldCount() const { return this->GetInlineCacheCount(); }

        ArgSlot GetProfiledInParamsCount() const { return this->GetInParamsCount() > 1? this->GetInParamsCount() - 1 : 0; }

        bool IsPartialDeserializedFunction() { return this->m_isPartialDeserializedFunction; }
#ifdef PERF_COUNTERS
        bool IsDeserializedFunction() { return this->m_isDeserializedFunction; }
#endif

#ifdef IR_VIEWER
        bool IsIRDumpEnabled() const { return this->m_isIRDumpEnabled; }
        void SetIRDumpEnabled(bool enabled) { this->m_isIRDumpEnabled = enabled; }
        Js::DynamicObject * GetIRDumpBaseObject();
#endif /* IR_VIEWER */

#if ENABLE_NATIVE_CODEGEN
        void SetPolymorphicCallSiteInfoHead(PolymorphicCallSiteInfo *polyCallSiteInfo) { this->SetAuxPtr(AuxPointerType::PolymorphicCallSiteInfoHead, polyCallSiteInfo); }
        PolymorphicCallSiteInfo * GetPolymorphicCallSiteInfoHead() { return static_cast<PolymorphicCallSiteInfo *>(this->GetAuxPtr(AuxPointerType::PolymorphicCallSiteInfoHead)); }
#endif

        PolymorphicInlineCache * GetPolymorphicInlineCachesHead() { return static_cast<PolymorphicInlineCache *>(this->GetAuxPtr(AuxPointerType::PolymorphicInlineCachesHead)); }
        void SetPolymorphicInlineCachesHead(PolymorphicInlineCache * cache) { this->SetAuxPtr(AuxPointerType::PolymorphicInlineCachesHead, cache); }

        bool PolyInliningUsingFixedMethodsAllowedByConfigFlags(FunctionBody* topFunctionBody)
        {
            return  !PHASE_OFF(Js::InlinePhase, this) && !PHASE_OFF(Js::InlinePhase, topFunctionBody) &&
                !PHASE_OFF(Js::PolymorphicInlinePhase, this) && !PHASE_OFF(Js::PolymorphicInlinePhase, topFunctionBody) &&
                !PHASE_OFF(Js::FixedMethodsPhase, this) && !PHASE_OFF(Js::FixedMethodsPhase, topFunctionBody) &&
                !PHASE_OFF(Js::PolymorphicInlineFixedMethodsPhase, this) && !PHASE_OFF(Js::PolymorphicInlineFixedMethodsPhase, topFunctionBody);
        }

        void SetScopeSlotArraySizes(uint scopeSlotCount, uint scopeSlotCountForParamScope)
        {
            this->scopeSlotArraySize = scopeSlotCount;
            this->paramScopeSlotArraySize = scopeSlotCountForParamScope;
        }

        Js::PropertyId * GetPropertyIdsForScopeSlotArray() const { return static_cast<Js::PropertyId *>(this->GetAuxPtr(AuxPointerType::PropertyIdsForScopeSlotArray)); }
        void SetPropertyIdsForScopeSlotArray(Js::PropertyId * propertyIdsForScopeSlotArray, uint scopeSlotCount, uint scopeSlotCountForParamScope = 0)
        {
            SetScopeSlotArraySizes(scopeSlotCount, scopeSlotCountForParamScope);
            this->SetAuxPtr(AuxPointerType::PropertyIdsForScopeSlotArray, propertyIdsForScopeSlotArray);
        }

        Js::PropertyIdOnRegSlotsContainer * GetPropertyIdOnRegSlotsContainer() const
        {
            return static_cast<Js::PropertyIdOnRegSlotsContainer *>(this->GetAuxPtr(AuxPointerType::PropertyIdOnRegSlotsContainer));
        }
        Js::PropertyIdOnRegSlotsContainer * GetPropertyIdOnRegSlotsContainerWithLock() const
        {
            return static_cast<Js::PropertyIdOnRegSlotsContainer *>(this->GetAuxPtrWithLock(AuxPointerType::PropertyIdOnRegSlotsContainer));
        }
        void SetPropertyIdOnRegSlotsContainer(Js::PropertyIdOnRegSlotsContainer *propertyIdOnRegSlotsContainer)
        {
            this->SetAuxPtr(AuxPointerType::PropertyIdOnRegSlotsContainer, propertyIdOnRegSlotsContainer);
        }
    private:
        void ResetProfileIds();

        void SetFlags(bool does, FunctionBodyFlags newFlags)
        {
            if (does)
            {
                flags = (FunctionBodyFlags)(flags | newFlags);
            }
            else
            {
                flags = (FunctionBodyFlags)(flags & ~newFlags);
            }
        }
    public:
        FunctionBodyFlags GetFlags() { return this->flags; }

        bool GetHasThis() const { return (flags & Flags_HasThis) != 0; }
        void SetHasThis(bool has) { SetFlags(has, Flags_HasThis); }

        static bool GetHasTry(FunctionBodyFlags flags) { return (flags & Flags_HasTry) != 0; }
        bool GetHasTry() const { return GetHasTry(this->flags); }
        void SetHasTry(bool has) { SetFlags(has, Flags_HasTry); }

        bool GetHasFinally() const { return m_hasFinally; }
        void SetHasFinally(bool has){ m_hasFinally = has; }

        bool GetFuncEscapes() const { return funcEscapes; }
        void SetFuncEscapes(bool does) { funcEscapes = does; }

        static bool GetHasOrParentHasArguments(FunctionBodyFlags flags) { return (flags & Flags_HasOrParentHasArguments) != 0; }
        bool GetHasOrParentHasArguments() const { return GetHasOrParentHasArguments(this->flags); }
        void SetHasOrParentHasArguments(bool has) { SetFlags(has, Flags_HasOrParentHasArguments); }

        static bool DoStackNestedFunc(FunctionBodyFlags flags) { return (flags & Flags_StackNestedFunc) != 0; }
        bool DoStackNestedFunc() const { return DoStackNestedFunc(flags); }
        void SetStackNestedFunc(bool does) { SetFlags(does, Flags_StackNestedFunc); }
#if DBG
        bool CanDoStackNestedFunc() const { return m_canDoStackNestedFunc; }
        void SetCanDoStackNestedFunc() { m_canDoStackNestedFunc = true; }
#endif
        RecyclerWeakReference<FunctionBody> * GetStackNestedFuncParent();
        FunctionBody * GetStackNestedFuncParentStrongRef();
        FunctionBody * GetAndClearStackNestedFuncParent();
        void ClearStackNestedFuncParent();
        void SetStackNestedFuncParent(FunctionBody * parentFunctionBody);
#if defined(_M_IX86) || defined(_M_X64)
        bool DoStackClosure() const
        {
            return DoStackNestedFunc() && GetNestedCount() != 0 && !PHASE_OFF(StackClosurePhase, this) && scopeSlotArraySize != 0 && GetEnvDepth() != (uint16)-1;
        }
#else
        bool DoStackClosure() const
        {
            return false;
        }
#endif
        bool DoStackFrameDisplay() const { return DoStackClosure(); }
        bool DoStackScopeSlots() const { return DoStackClosure(); }

        bool IsNonUserCode() const { return (flags & Flags_NonUserCode) != 0; }
        void SetIsNonUserCode(bool set);

        bool GetHasNoExplicitReturnValue() { return (flags & Flags_HasNoExplicitReturnValue) != 0; }
        void SetHasNoExplicitReturnValue(bool has) { SetFlags(has, Flags_HasNoExplicitReturnValue); }

        bool GetHasOnlyThisStmts() const { return (flags & Flags_HasOnlyThisStatements) != 0; }
        void SetHasOnlyThisStmts(bool has) { SetFlags(has, Flags_HasOnlyThisStatements); }

        bool GetIsFirstFunctionObject() const { return m_firstFunctionObject; }
        void SetIsNotFirstFunctionObject() { m_firstFunctionObject = false; }

        bool GetInlineCachesOnFunctionObject() { return m_inlineCachesOnFunctionObject; }
        void SetInlineCachesOnFunctionObject(bool has) { m_inlineCachesOnFunctionObject = has; }

        static bool GetHasRestParameter(FunctionBodyFlags flags) { return (flags & Flags_HasRestParameter) != 0; }
        bool GetHasRestParameter() const { return GetHasRestParameter(this->flags); }
        void SetHasRestParameter() { SetFlags(true, Flags_HasRestParameter); }

        bool NeedScopeObjectForArguments(bool hasNonSimpleParams)
        {
            Assert(HasReferenceableBuiltInArguments());
            // We can avoid creating a scope object with arguments present if:
            bool dontNeedScopeObject =
                // Either we are in strict mode, or have strict mode formal semantics from a non-simple parameter list, and
                (GetIsStrictMode() || hasNonSimpleParams)
                // Neither of the scopes are objects
                && !HasScopeObject();
            
            return 
                // Regardless of the conditions above, we won't need a scope object if there aren't any formals.
                (GetInParamsCount() > 1 || GetHasRestParameter())
                && !dontNeedScopeObject;
        }

        uint GetNumberOfRecursiveCallSites();
        bool CanInlineRecursively(uint depth, bool tryAggressive = true);
    public:
        bool CanInlineAgain() const
        {
            // Block excessive recursive inlining of the same function
            return inlineDepth < static_cast<byte>(max(1, min(0xff, CONFIG_FLAG(MaxFuncInlineDepth))));
        }

        void OnBeginInlineInto()
        {
            ++inlineDepth;
        }

        void OnEndInlineInto()
        {
            --inlineDepth;
        }

        uint8 IncrementBailOnMisingProfileCount() { return ++bailOnMisingProfileCount; }
        void ResetBailOnMisingProfileCount() { bailOnMisingProfileCount = 0; }
        uint8 IncrementBailOnMisingProfileRejitCount() { return ++bailOnMisingProfileRejitCount; }
        uint32 GetFrameHeight(EntryPointInfo* entryPointInfo) const;
        void SetFrameHeight(EntryPointInfo* entryPointInfo, uint32 frameHeight);

        RegSlot GetLocalsCount();
        RegSlot GetConstantCount() const { return this->GetCountField(CounterFields::ConstantCount); }
        void CheckAndSetConstantCount(RegSlot cNewConstants);
        void SetConstantCount(RegSlot cNewConstants);
        RegSlot GetVarCount();
        void SetVarCount(RegSlot cNewVars);
        void CheckAndSetVarCount(RegSlot cNewVars);
        RegSlot MapRegSlot(RegSlot reg)
        {
            if (this->RegIsConst(reg))
            {
                reg = CONSTREG_TO_REGSLOT(reg);
                Assert(reg < this->GetConstantCount());
            }
            else
            {
                reg += this->GetConstantCount();
            }

            return reg;
        }
        bool RegIsConst(RegSlot reg) { return reg > REGSLOT_TO_CONSTREG(this->GetConstantCount()); }

        uint32 GetNonTempLocalVarCount();
        uint32 GetFirstNonTempLocalIndex();
        uint32 GetEndNonTempLocalIndex();
        bool IsNonTempLocalVar(uint32 varIndex);
        bool GetSlotOffset(RegSlot slotId, int32 * slotOffset, bool allowTemp = false);

        RegSlot GetOutParamMaxDepth();
        void SetOutParamMaxDepth(RegSlot cOutParamsDepth);
        void CheckAndSetOutParamMaxDepth(RegSlot cOutParamsDepth);

        RegSlot GetYieldRegister();

        RegSlot GetFirstTmpRegister() const;
        void SetFirstTmpRegister(RegSlot reg);

        RegSlot GetFirstTmpReg();
        void SetFirstTmpReg(RegSlot firstTmpReg);
        RegSlot GetTempCount();

        Js::ModuleID GetModuleID() const;

        void CreateConstantTable();
        void RecordNullObject(RegSlot location);
        void RecordUndefinedObject(RegSlot location);
        void RecordTrueObject(RegSlot location);
        void RecordFalseObject(RegSlot location);
        void RecordIntConstant(RegSlot location, unsigned int val);
        void RecordStrConstant(RegSlot location, LPCOLESTR psz, uint32 cch);
        void RecordFloatConstant(RegSlot location, double d);
        void RecordNullDisplayConstant(RegSlot location);
        void RecordStrictNullDisplayConstant(RegSlot location);
        void InitConstantSlots(Var *dstSlots);
        Var GetConstantVar(RegSlot location);
        Js::Var* GetConstTable() const { return this->m_constTable; }
        void SetConstTable(Js::Var* constTable) { this->m_constTable = constTable; }
        void CloneConstantTable(FunctionBody *newFunc);

        void MarkScript(ByteBlock * pblkByteCode, ByteBlock * pblkAuxiliaryData, ByteBlock* auxContextBlock,
            uint byteCodeCount, uint byteCodeInLoopCount, uint byteCodeWithoutLDACount);

        void         BeginExecution();
        void         EndExecution();
        SourceInfo * GetSourceInfo() { return &this->m_sourceInfo; }

        bool InstallProbe(int offset);
        bool UninstallProbe(int offset);
        bool ProbeAtOffset(int offset, OpCode* pOriginalOpcode);

        ParseableFunctionInfo * Clone(ScriptContext *scriptContext, uint sourceIndex = Js::Constants::InvalidSourceIndex) override;

        static bool ShouldShareInlineCaches() { return CONFIG_FLAG(ShareInlineCaches); }

        uint GetInlineCacheCount() const { return GetCountField(CounterFields::InlineCacheCount); }
        void SetInlineCacheCount(uint count) { SetCountField(CounterFields::InlineCacheCount, count); }

        uint GetRootObjectLoadInlineCacheStart() const { return GetCountField(CounterFields::RootObjectLoadInlineCacheStart); }
        void SetRootObjectLoadInlineCacheStart(uint count) { SetCountField(CounterFields::RootObjectLoadInlineCacheStart, count); }

        uint GetRootObjectLoadMethodInlineCacheStart() const { return GetCountField(CounterFields::RootObjectLoadMethodInlineCacheStart); }
        void SetRootObjectLoadMethodInlineCacheStart(uint count) { SetCountField(CounterFields::RootObjectLoadMethodInlineCacheStart, count); }

        uint GetRootObjectStoreInlineCacheStart() const { return GetCountField(CounterFields::RootObjectStoreInlineCacheStart); }
        void SetRootObjectStoreInlineCacheStart(uint count) { SetCountField(CounterFields::RootObjectStoreInlineCacheStart, count); }

        uint GetIsInstInlineCacheCount() const { return GetCountField(CounterFields::IsInstInlineCacheCount); }
        void SetIsInstInlineCacheCount(uint count) { SetCountField(CounterFields::IsInstInlineCacheCount, count); }

        uint GetReferencedPropertyIdCount() const { return GetCountField(CounterFields::ReferencedPropertyIdCount); }
        void SetReferencedPropertyIdCount(uint count) { SetCountField(CounterFields::ReferencedPropertyIdCount, count); }

        uint GetObjLiteralCount() const { return GetCountField(CounterFields::ObjLiteralCount); }
        void SetObjLiteralCount(uint count) { SetCountField(CounterFields::ObjLiteralCount, count); }
        uint IncObjLiteralCount() { return IncreaseCountField(CounterFields::ObjLiteralCount); }

        uint GetLiteralRegexCount() const { return GetCountField(CounterFields::LiteralRegexCount); }
        void SetLiteralRegexCount(uint count) { SetCountField(CounterFields::LiteralRegexCount, count); }
        uint IncLiteralRegexCount() { return IncreaseCountField(CounterFields::LiteralRegexCount); }


        void AllocateInlineCache();
        InlineCache * GetInlineCache(uint index);
        bool CanFunctionObjectHaveInlineCaches();
        void** GetInlineCaches();

#if DBG
        byte* GetInlineCacheTypes();
#endif
        IsInstInlineCache * GetIsInstInlineCache(uint index);
        PolymorphicInlineCache * GetPolymorphicInlineCache(uint index);
        PolymorphicInlineCache * CreateNewPolymorphicInlineCache(uint index, PropertyId propertyId, InlineCache * inlineCache);
        PolymorphicInlineCache * CreateBiggerPolymorphicInlineCache(uint index, PropertyId propertyId);

    private:
        void ResetInlineCaches();
        PolymorphicInlineCache * CreatePolymorphicInlineCache(uint index, uint16 size);
        uint32 m_asmJsTotalLoopCount;
    public:
        void CreateCacheIdToPropertyIdMap();
        void CreateCacheIdToPropertyIdMap(uint rootObjectLoadInlineCacheStart, uint rootObjectLoadMethodInlineCacheStart, uint rootObjectStoreInlineCacheStart,
            uint totalFieldAccessInlineCacheCount, uint isInstInlineCacheCount);
        void SetPropertyIdForCacheId(uint cacheId, PropertyId propertyId);
        PropertyId GetPropertyIdFromCacheId(uint cacheId)
        {
            Assert(this->cacheIdToPropertyIdMap);
            Assert(cacheId < this->GetInlineCacheCount());
            return this->cacheIdToPropertyIdMap[cacheId];
        }
#if DBG
        void VerifyCacheIdToPropertyIdMap();
#endif
        PropertyId* GetReferencedPropertyIdMap() const { return static_cast<PropertyId*>(this->GetAuxPtr(AuxPointerType::ReferencedPropertyIdMap)); }
        PropertyId* GetReferencedPropertyIdMapWithLock() const { return static_cast<PropertyId*>(this->GetAuxPtrWithLock(AuxPointerType::ReferencedPropertyIdMap)); }
        void SetReferencedPropertyIdMap(PropertyId* propIdMap) { this->SetAuxPtr(AuxPointerType::ReferencedPropertyIdMap, propIdMap); }
        void CreateReferencedPropertyIdMap(uint referencedPropertyIdCount);
        void CreateReferencedPropertyIdMap();
        PropertyId GetReferencedPropertyIdWithMapIndex(uint mapIndex);
        PropertyId GetReferencedPropertyIdWithMapIndexWithLock(uint mapIndex);
        void SetReferencedPropertyIdWithMapIndex(uint mapIndex, PropertyId propertyId);
        PropertyId GetReferencedPropertyId(uint index);
        PropertyId GetReferencedPropertyIdWithLock(uint index);
#if DBG
        void VerifyReferencedPropertyIdMap();
#endif
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        void DumpFullFunctionName();
        void DumpFunctionId(bool pad);
        uint GetTraceFunctionNumber() const;
#endif

    public:
        uint NewObjectLiteral();
        void AllocateObjectLiteralTypeArray();
        DynamicType ** GetObjectLiteralTypeRef(uint index);
        DynamicType ** GetObjectLiteralTypeRefWithLock(uint index);
        uint NewLiteralRegex();
        void AllocateLiteralRegexArray();
        UnifiedRegex::RegexPattern **GetLiteralRegexes() const { return static_cast<UnifiedRegex::RegexPattern **>(this->GetAuxPtr(AuxPointerType::LiteralRegexes)); }
        UnifiedRegex::RegexPattern **GetLiteralRegexesWithLock() const { return static_cast<UnifiedRegex::RegexPattern **>(this->GetAuxPtrWithLock(AuxPointerType::LiteralRegexes)); }
        void SetLiteralRegexs(UnifiedRegex::RegexPattern ** literalRegexes) { this->SetAuxPtr(AuxPointerType::LiteralRegexes, literalRegexes); }
        UnifiedRegex::RegexPattern *GetLiteralRegex(const uint index);
        UnifiedRegex::RegexPattern *GetLiteralRegexWithLock(const uint index);
#ifndef TEMP_DISABLE_ASMJS
        AsmJsFunctionInfo* GetAsmJsFunctionInfo()const { return static_cast<AsmJsFunctionInfo*>(this->GetAuxPtr(AuxPointerType::AsmJsFunctionInfo)); }
        AsmJsFunctionInfo* GetAsmJsFunctionInfoWithLock()const { return static_cast<AsmJsFunctionInfo*>(this->GetAuxPtrWithLock(AuxPointerType::AsmJsFunctionInfo)); }
        AsmJsFunctionInfo* AllocateAsmJsFunctionInfo();
        AsmJsModuleInfo* GetAsmJsModuleInfo()const { return static_cast<AsmJsModuleInfo*>(this->GetAuxPtr(AuxPointerType::AsmJsModuleInfo)); }
        AsmJsModuleInfo* GetAsmJsModuleInfoWithLock()const { return static_cast<AsmJsModuleInfo*>(this->GetAuxPtrWithLock(AuxPointerType::AsmJsModuleInfo)); }
        void ResetAsmJsInfo()
        {
            SetAuxPtr(AuxPointerType::AsmJsFunctionInfo, nullptr);
            SetAuxPtr(AuxPointerType::AsmJsModuleInfo, nullptr);
        }
        bool IsAsmJSModule()const{ return this->GetAsmJsFunctionInfo() != nullptr; }
        AsmJsModuleInfo* AllocateAsmJsModuleInfo();
#endif
        void SetLiteralRegex(const uint index, UnifiedRegex::RegexPattern *const pattern);
        DynamicType** GetObjectLiteralTypes() const { return static_cast<DynamicType**>(this->GetAuxPtr(AuxPointerType::ObjLiteralTypes)); }
    private:
        void ResetLiteralRegexes();
        void ResetObjectLiteralTypes();
        DynamicType** GetObjectLiteralTypesWithLock() const { return static_cast<DynamicType**>(this->GetAuxPtrWithLock(AuxPointerType::ObjLiteralTypes)); }
        void SetObjectLiteralTypes(DynamicType** objLiteralTypes) { this->SetAuxPtr(AuxPointerType::ObjLiteralTypes, objLiteralTypes); };
    public:

        void ResetByteCodeGenState();
        void ResetByteCodeGenVisitState();

        void FindClosestStatements(int32 characterOffset, StatementLocation *firstStatementLocation, StatementLocation *secondStatementLocation);
#if ENABLE_NATIVE_CODEGEN
        const FunctionCodeGenRuntimeData *GetInlineeCodeGenRuntimeData(const ProfileId profiledCallSiteId) const;
        const FunctionCodeGenRuntimeData *GetInlineeCodeGenRuntimeDataForTargetInlinee(const ProfileId profiledCallSiteId, FunctionBody *inlineeFuncBody) const;
        FunctionCodeGenRuntimeData *EnsureInlineeCodeGenRuntimeData(
            Recycler *const recycler,
            __in_range(0, profiledCallSiteCount - 1) const ProfileId profiledCallSiteId,
            FunctionBody *const inlinee);
        const FunctionCodeGenRuntimeData *GetLdFldInlineeCodeGenRuntimeData(const InlineCacheIndex inlineCacheIndex) const;
        FunctionCodeGenRuntimeData *EnsureLdFldInlineeCodeGenRuntimeData(
            Recycler *const recycler,
            const InlineCacheIndex inlineCacheIndex,
            FunctionBody *const inlinee);

        void LoadDynamicProfileInfo();
        bool HasExecutionDynamicProfileInfo() const { return hasExecutionDynamicProfileInfo; }
        bool HasDynamicProfileInfo() const { return dynamicProfileInfo != nullptr; }
        bool NeedEnsureDynamicProfileInfo() const;
        DynamicProfileInfo * GetDynamicProfileInfo() const { Assert(HasExecutionDynamicProfileInfo()); return dynamicProfileInfo; }
        DynamicProfileInfo * GetAnyDynamicProfileInfo() const { Assert(HasDynamicProfileInfo()); return dynamicProfileInfo; }
        DynamicProfileInfo * EnsureDynamicProfileInfo();
        DynamicProfileInfo * AllocateDynamicProfile();
        BYTE GetSavedInlinerVersion() const;
        uint32 GetSavedPolymorphicCacheState() const;
        void SetSavedPolymorphicCacheState(uint32 state);
        ImplicitCallFlags GetSavedImplicitCallsFlags() const;
        bool HasNonBuiltInCallee();

        void RecordNativeThrowMap(SmallSpanSequenceIter& iter, uint32 offset, uint32 statementIndex, EntryPointInfo* entryPoint, uint loopNum);
        void SetNativeThrowSpanSequence(SmallSpanSequence *seq, uint loopNum, LoopEntryPointInfo* entryPoint);

        BOOL GetMatchingStatementMapFromNativeAddress(DWORD_PTR codeAddress, StatementData &data, uint loopNum, FunctionBody *inlinee = nullptr);
        BOOL GetMatchingStatementMapFromNativeOffset(DWORD_PTR codeAddress, uint32 offset, StatementData &data, uint loopNum, FunctionBody *inlinee = nullptr);

        FunctionEntryPointInfo * GetEntryPointFromNativeAddress(DWORD_PTR codeAddress);
        LoopEntryPointInfo * GetLoopEntryPointInfoFromNativeAddress(DWORD_PTR codeAddress, uint loopNum) const;
#endif

        void InsertSymbolToRegSlotList(JsUtil::CharacterBuffer<WCHAR> const& propName, RegSlot reg, RegSlot totalRegsCount);
        void InsertSymbolToRegSlotList(RegSlot reg, PropertyId propertyId, RegSlot totalRegsCount);
        void SetPropertyIdsOfFormals(PropertyIdArray * formalArgs);
        PropertyIdArray * AllocatePropertyIdArrayForFormals(uint32 size, uint32 count, byte extraSlots);

        bool DontRethunkAfterBailout() const { return dontRethunkAfterBailout; }
        void SetDontRethunkAfterBailout() { dontRethunkAfterBailout = true; }
        void ClearDontRethunkAfterBailout() { dontRethunkAfterBailout = false; }

        void SaveState(ParseNodePtr pnode);
        void RestoreState(ParseNodePtr pnode);

        // Used for the debug purpose, this info will be stored (in the non-debug mode), when a function has all locals marked as non-local-referenced.
        // So when we got to no-refresh debug mode, and try to re-use the same function body we can then enforce all locals to be non-local-referenced.
        bool HasAllNonLocalReferenced() const { return m_hasAllNonLocalReferenced; }
        void SetAllNonLocalReferenced(bool set) { m_hasAllNonLocalReferenced = set; }

        bool HasSetIsObject() const { return m_hasSetIsObject; }
        void SetHasSetIsObject(bool set) { m_hasSetIsObject = set; }

        bool HasFuncExprNameReference() const { return m_hasFunExprNameReference; }
        void SetFuncExprNameReference(bool value) { m_hasFunExprNameReference = value; }

        bool GetChildCallsEval() const { return m_ChildCallsEval; }
        void SetChildCallsEval(bool value) { m_ChildCallsEval = value; }

        bool GetCallsEval() const { return m_CallsEval; }
        void SetCallsEval(bool set) { m_CallsEval = set; }

        bool HasReferenceableBuiltInArguments() const { return m_hasReferenceableBuiltInArguments; }
        void SetHasReferenceableBuiltInArguments(bool value) { m_hasReferenceableBuiltInArguments = value; }

        bool IsParamAndBodyScopeMerged() const { return m_isParamAndBodyScopeMerged; }
        void SetParamAndBodyScopeNotMerged() { m_isParamAndBodyScopeMerged = false; }

        // Used for the debug purpose. This is to avoid setting all locals to non-local-referenced, multiple time for each child function.
        bool HasDoneAllNonLocalReferenced() const { return m_hasDoneAllNonLocalReferenced; }
        void SetHasDoneAllNonLocalReferenced(bool set) { m_hasDoneAllNonLocalReferenced = set; }

        // Once the function compiled is sent m_hasFunctionCompiledSent will be set to 'true'. The below check will be used only to determine during ProfileModeDeferredParse function.
        bool HasFunctionCompiledSent() const { return m_hasFunctionCompiledSent; }
        void SetHasFunctionCompiledSent(bool set) { m_hasFunctionCompiledSent = set; }

#if DBG_DUMP
        void DumpStatementMaps();
        void Dump();
        void PrintStatementSourceLine(uint statementIndex);
        void PrintStatementSourceLineFromStartOffset(uint cchStartOffset);
        void DumpScopes();
#endif

        uint GetStatementStartOffset(const uint statementIndex);

#ifdef IR_VIEWER
        void GetSourceLineFromStartOffset(const uint startOffset, LPCUTF8 *sourceBegin, LPCUTF8 *sourceEnd,
            ULONG * line, LONG * col);
        void GetStatementSourceInfo(const uint statementIndex, LPCUTF8 *sourceBegin, LPCUTF8 *sourceEnd,
            ULONG * line, LONG * col);
#endif

#if ENABLE_TTD
        void GetSourceLineFromStartOffset_TTD(const uint startOffset, ULONG* line, LONG* col);
#endif

#ifdef ENABLE_SCRIPT_PROFILING
        HRESULT RegisterFunction(BOOL fChangeMode, BOOL fOnlyCurrent = FALSE);
        HRESULT ReportScriptCompiled();
        HRESULT ReportFunctionCompiled();
        void SetEntryToProfileMode();
#endif

        void CheckAndRegisterFuncToDiag(ScriptContext *scriptContext);
        void SetEntryToDeferParseForDebugger();
        void ResetEntryPoint();
        void CleanupToReparse();
        void AddDeferParseAttribute();
        void RemoveDeferParseAttribute();
#if DBG
        void MustBeInDebugMode();
#endif

        static bool IsDummyGlobalRetStatement(const regex::Interval *sourceSpan)
        {
            Assert(sourceSpan != nullptr);
            return sourceSpan->begin == 0 && sourceSpan->end == 0;
        }

        static void GetShortNameFromUrl(__in LPCWSTR pchUrl, _Out_writes_z_(cchBuffer) LPWSTR pchShortName, __in size_t cchBuffer);

        template<class Fn>
        void MapLoopHeaders(Fn fn) const
        {
            Js::LoopHeader* loopHeaderArray = this->GetLoopHeaderArray();
            if(loopHeaderArray)
            {
                uint loopCount = this->GetLoopCount();
                for(uint i = 0; i < loopCount; i++)
                {
                    fn(i , &loopHeaderArray[i]);
                }
            }
        }
        template<class Fn>
        void MapLoopHeadersWithLock(Fn fn) const
        {
            Js::LoopHeader* loopHeaderArray = this->GetLoopHeaderArrayWithLock();
            if (loopHeaderArray)
            {
                uint loopCount = this->GetLoopCount();
                for (uint i = 0; i < loopCount; i++)
                {
                    fn(i, &loopHeaderArray[i]);
                }
            }
        }

        template <class Fn>
        void MapEntryPoints(Fn fn) const
        {
            if (this->entryPoints)
            {
                this->entryPoints->Map([&fn] (int index, RecyclerWeakReference<FunctionEntryPointInfo>* entryPoint) {
                    FunctionEntryPointInfo* strongRef = entryPoint->Get();
                    if (strongRef)
                    {
                        fn(index, strongRef);
                    }
                });
            }
        }

        bool DoJITLoopBody() const
        {
            return IsJitLoopBodyPhaseEnabled() && this->GetLoopHeaderArrayWithLock() != nullptr;
        }

        bool ForceJITLoopBody() const
        {
            return IsJitLoopBodyPhaseForced() && !this->GetHasTry();
        }

        bool IsGeneratorAndJitIsDisabled()
        {
            return this->IsGenerator() && !(CONFIG_ISENABLED(Js::JitES6GeneratorsFlag) && !this->GetHasTry());
        }

        FunctionBodyFlags * GetAddressOfFlags() { return &this->flags; }
        Js::RegSlot GetRestParamRegSlot();

    public:
        void RecordConstant(RegSlot location, Var var);

    private:
        inline  void            CheckEmpty();
        inline  void            CheckNotExecuting();

        BOOL               GetMatchingStatementMap(StatementData &data, int statementIndex, FunctionBody *inlinee);

#if ENABLE_NATIVE_CODEGEN
        int                GetStatementIndexFromNativeOffset(SmallSpanSequence *pThrowSpanSequence, uint32 nativeOffset);
        int                GetStatementIndexFromNativeAddress(SmallSpanSequence *pThrowSpanSequence, DWORD_PTR codeAddress, DWORD_PTR nativeBaseAddress);
#endif

        void EnsureAuxStatementData();
        StatementAdjustmentRecordList* GetStatementAdjustmentRecords();

#ifdef DYNAMIC_PROFILE_MUTATOR
        friend class DynamicProfileMutator;
        friend class DynamicProfileMutatorImpl;
#endif
        friend class RemoteFunctionBody;
    };

    typedef SynchronizableList<FunctionBody*, JsUtil::List<FunctionBody*, ArenaAllocator, false, Js::FreeListedRemovePolicy> > FunctionBodyList;

    struct ScopeSlots
    {
    public:
        static uint const MaxEncodedSlotCount = Constants::UShortMaxValue;

        // The slot index is at the same location as the vtable, so that we can distinguish between scope slot and frame display
        static uint const EncodedSlotCountSlotIndex = 0;
        static uint const ScopeMetadataSlotIndex = 1;    // Either a FunctionBody* or DebuggerScope*
        static uint const FirstSlotIndex = 2;
    public:
        ScopeSlots(Var* slotArray) : slotArray(slotArray)
        {
        }

        bool IsFunctionScopeSlotArray()
        {
            return FunctionBody::Is(slotArray[ScopeMetadataSlotIndex]);
        }

        FunctionBody* GetFunctionBody()
        {
            Assert(IsFunctionScopeSlotArray());
            return (FunctionBody*)(slotArray[ScopeMetadataSlotIndex]);
        }

        DebuggerScope* GetDebuggerScope()
        {
            Assert(!IsFunctionScopeSlotArray());
            return (DebuggerScope*)(slotArray[ScopeMetadataSlotIndex]);
        }

        Var GetScopeMetadataRaw() const
        {
            return slotArray[ScopeMetadataSlotIndex];
        }

        void SetScopeMetadata(Var scopeMetadataObj)
        {
            slotArray[ScopeMetadataSlotIndex] = scopeMetadataObj;
        }

        uint GetCount() const
        {
            return ::Math::PointerCastToIntegralTruncate<uint>(slotArray[EncodedSlotCountSlotIndex]);
        }

        void SetCount(uint count)
        {
            slotArray[EncodedSlotCountSlotIndex] = (Var)min<uint>(count, ScopeSlots::MaxEncodedSlotCount);
        }

        Var Get(uint i) const
        {
            Assert(i < GetCount());
            return slotArray[i + FirstSlotIndex];
        }

        void Set(uint i, Var value)
        {
            Assert(i < GetCount());
            slotArray[i + FirstSlotIndex] = value;
        }

        template<class Fn>
        void Map(Fn fn)
        {
            uint count = GetCount();
            for(uint i = 0; i < count; i++)
            {
                fn(GetSlot[i]);
            }
        }

        // The first pointer sized value in the object for scope slots is the count, while it is a vtable
        // for Activation object or with scope (a recyclable object)
        // VTable values are always > 64K because they are a pointer, hence anything less than that implies
        // a slot array.
        // CONSIDER: Use TaggedInt instead of range of slot count to distinguish slot array with others.
        static bool Is(void* object)
        {
            size_t slotCount = *((size_t*)object);
            if(slotCount <= MaxEncodedSlotCount)
            {
                return true;
            }
            return false;
        }

    private:
        Var* slotArray;
    };


    enum ScopeType
    {
        ScopeType_ActivationObject,
        ScopeType_SlotArray,
        ScopeType_WithScope
    };

    // A FrameDisplay encodes a FunctionObject's scope chain. It is an array of scopes, where each scope can be either an inline slot array
    // or a RecyclableObject. A FrameDisplay for a given FunctionObject will consist of the FrameDisplay from it's enclosing scope, with any additional
    // scopes prepended. Due to with statements etc. a function may introduce multiple scopes to the FrameDisplay.
    struct FrameDisplay
    {
        FrameDisplay(uint16 len, bool strictMode = false) :
            tag(true),
            length(len),
            strictMode(strictMode)
#if _M_X64
            , unused(0)
#endif
        {
        }

        void SetTag(bool tag) { this->tag = tag; }
        void SetItem(uint index, void* item);
        void *GetItem(uint index);
        uint16 GetLength() const { return length; }
        void SetLength(uint16 len) { this->length = len; }

        bool   GetStrictMode() const { return strictMode; }
        void   SetStrictMode(bool flag) { this->strictMode = flag; }

        void** GetDataAddress() { return (void**)&this->scopes; }
        static uint32 GetOffsetOfStrictMode() { return offsetof(FrameDisplay, strictMode); }
        static uint32 GetOffsetOfLength() { return offsetof(FrameDisplay, length); }
        static uint32 GetOffsetOfScopes() { return offsetof(FrameDisplay, scopes); }
        static ScopeType GetScopeType(void* scope);

    private:
        bool tag;              // Tag it so that the NativeCodeGenerator::IsValidVar would not think this is var
        bool strictMode;
        uint16 length;

#if defined(_M_X64_OR_ARM64)
        uint32 unused;
#endif
        void* scopes[];
    };
#pragma region Function Body helper classes
#pragma region Debugging related source classes
    // Contains only the beginning part of the statement. This will mainly used in SmallSpanSequence which will further be compressed
    // and stored in the buffer
    struct StatementData
    {
        StatementData()
            : sourceBegin(0),
            bytecodeBegin(0)
        {
        }

        int sourceBegin;
        int bytecodeBegin;
    };

    struct StatementLocation
    {
        Js::FunctionBody* function;
        regex::Interval statement;
        regex::Interval bytecodeSpan;
    };

    // Small span in the Statement buffer of the SmallSpanSequence
    struct SmallSpan
    {
        ushort sourceBegin;
        ushort bytecodeBegin;

        SmallSpan(uint32 val)
        {
            sourceBegin = (ushort)(val >> 16);
            bytecodeBegin = (ushort)(val & 0x0000FFFF);
        }

        operator unsigned int()
        {
            return (uint32)sourceBegin << 16 | bytecodeBegin;
        }
    };

    // Iterator which contains the state at particular index. These values will used when fetching next item from
    // SmallSpanSequence
    class SmallSpanSequenceIter
    {
        friend class SmallSpanSequence;

    public:
        SmallSpanSequenceIter()
            : accumulatedIndex(-1),
            accumulatedSourceBegin(0),
            accumulatedBytecodeBegin(0),
            indexOfActualOffset(0)
        {

        }

        // Below are used for fast access when the last access happened nearby.
        // so the actual index would be accumulatedIndex / 2 + (remainder for which byte).
        int accumulatedIndex;
        int accumulatedSourceBegin;
        int accumulatedBytecodeBegin;

        int indexOfActualOffset;
    };

    struct ThrowMapEntry
    {
        uint32 nativeBufferOffset;
        uint32 statementIndex;
    };

    // This class compacts the range of the statement to BYTEs instead of ints.
    // Instead of having start and end as int32s we will have them stored in bytes, and they will be
    // treated as start offset and end offset.
    // For simplicity, this class should be heap allocated, since it can be allocated from either the background
    // or main thread.
    class SmallSpanSequence
    {
    private:
        BOOL GetRangeAt(int index, SmallSpanSequenceIter &iter, int * pCountOfMissed, StatementData & data);
        ushort GetDiff(int current, int prev);

    public:

        // Each item in the list contains two set of begins (one for bytecode and for sourcespan).
        // The  allowed valued for source and bytecode span is in between SHORT_MAX - 1 to SHORT_MIN (inclusive).
        // otherwise its a miss
        JsUtil::GrowingUint32HeapArray * pStatementBuffer;

        // Contains list of values which are missed in StatementBuffer.
        JsUtil::GrowingUint32HeapArray * pActualOffsetList;

        // The first value of the sequence
        int baseValue;

        SmallSpanSequence();

        ~SmallSpanSequence()
        {
            Cleanup();
        }

        void Cleanup()
        {
            if (pStatementBuffer != nullptr)
            {
                HeapDelete(pStatementBuffer);
                pStatementBuffer = nullptr;
            }

            if (pActualOffsetList != nullptr)
            {
                HeapDelete(pActualOffsetList);
                pActualOffsetList = nullptr;
            }
        }

        // Trys to match passed bytecode in the statement, and returns the statement which includes that.
        BOOL GetMatchingStatementFromBytecode(int bytecode, SmallSpanSequenceIter &iter, StatementData & data);

        // Record the statement data in the statement buffer in the compressed manner.
        BOOL RecordARange(SmallSpanSequenceIter &iter, StatementData * data);

        // Reset the accumulator's state and value.
        void Reset(SmallSpanSequenceIter &iter);

        uint32 Count() const { return pStatementBuffer ? pStatementBuffer->Count() : 0; }

        BOOL Item(int index, SmallSpanSequenceIter &iter, StatementData &data);

        // Below function will not change any state, so it will not alter accumulated index and value
        BOOL Seek(int index, StatementData & data);

        SmallSpanSequence * Clone();
    };
#pragma endregion

    // This container represent the property ids for the locals which are placed at direct slot
    // and list of formals args if user has not used the arguments object in the script for the current function
    struct PropertyIdOnRegSlotsContainer
    {
        PropertyId * propertyIdsForRegSlots;
        uint length;

        PropertyIdArray * propertyIdsForFormalArgs;

        PropertyIdOnRegSlotsContainer();
        static PropertyIdOnRegSlotsContainer * New(Recycler * recycler);

        void CreateRegSlotsArray(Recycler * recycler, uint _length);
        void SetFormalArgs(PropertyIdArray * formalArgs);

        // Helper methods
        void Insert(RegSlot reg, PropertyId propId);
        void FetchItemAt(uint index, FunctionBody *pFuncBody, __out PropertyId *pPropId, __out RegSlot *pRegSlot);
        // Whether reg belongs to non-temp locals
        bool IsRegSlotFormal(RegSlot reg);
    };

    // Flags for the DebuggerScopeProperty object.
    typedef int DebuggerScopePropertyFlags;
    const int DebuggerScopePropertyFlags_None                   = 0x000000000;
    const int DebuggerScopePropertyFlags_Const                  = 0x000000001;
    const int DebuggerScopePropertyFlags_CatchObject            = 0x000000002;
    const int DebuggerScopePropertyFlags_WithObject             = 0x000000004;
    const int DebuggerScopePropertyFlags_ForInOrOfCollection    = 0x000000008;

    // Used to store local property info for with/catch objects, lets, or consts
    // that are needed for the debugger.
    class DebuggerScopeProperty
    {
    public:
        Js::PropertyId propId;              // The property ID of the scope variable.
        RegSlot location;                   // Contains the location of the scope variable (regslot, slotarray, direct).
        int byteCodeInitializationOffset;   // The byte code offset used when comparing let/const variables for dead zone exclusion debugger side.
        DebuggerScopePropertyFlags flags;   // Flags for the property.

        bool IsConst() const { return (flags & DebuggerScopePropertyFlags_Const) != 0; }
        bool IsCatchObject() const { return (flags & DebuggerScopePropertyFlags_CatchObject) != 0; }
        bool IsWithObject() const { return (flags & DebuggerScopePropertyFlags_WithObject) != 0; }
        bool IsForInOrForOfCollectionScope() const { return (flags & DebuggerScopePropertyFlags_ForInOrOfCollection) != 0; }

    public:
        // Determines if the current property is in a dead zone.  Note that the property makes
        // no assumptions about what scope it's in, that is determined by DebuggerScope.
        // byteCodeOffset - The current offset in bytecode that the debugger is at.
        bool IsInDeadZone(int byteCodeOffset) const
        {
            if (IsForInOrForOfCollectionScope())
            {
                // These are let/const loop variables of a for-in or for-of loop
                // in the scope for the collection expression.  They are always
                // in TDZ in this scope, never initialized by the bytecode.
                return true;
            }

            if (this->byteCodeInitializationOffset == Constants::InvalidByteCodeOffset && !(IsCatchObject() || IsWithObject()))
            {
                AssertMsg(false, "Debug let/const property never had its initialization point updated.  This indicates that a Ld or St operation in ByteCodeGenerator was missed that needs to have DebuggerScope::UpdatePropertyInitializationOffset() added to it.");
                return false;
            }

            return byteCodeOffset < this->byteCodeInitializationOffset;
        }
    };

    // Used to track with, catch, and block scopes for the debugger to determine context.
    class DebuggerScope
    {
    public:
        typedef JsUtil::List<DebuggerScopeProperty> DebuggerScopePropertyList;

        DebuggerScope(Recycler* recycler, DiagExtraScopesType scopeType, RegSlot scopeLocation, int rangeBegin)
            : scopeType(scopeType),
              scopeProperties(nullptr),
              parentScope(nullptr),
              siblingScope(nullptr),
              scopeLocation(scopeLocation),
              recycler(recycler)
        {
            this->range.begin = rangeBegin;
            this->range.end = -1;
        }

        DebuggerScope * GetSiblingScope(RegSlot location, FunctionBody *functionBody);
        void AddProperty(RegSlot location, Js::PropertyId propertyId, DebuggerScopePropertyFlags flags);
        bool GetPropertyIndex(Js::PropertyId propertyId, int& i);
        bool HasProperty(Js::PropertyId propertyId);

        bool IsOffsetInScope(int offset) const;
        bool Contains(Js::PropertyId propertyId, RegSlot location) const;
        bool IsBlockScope() const;
        bool IsBlockObjectScope() const
        {
            return this->scopeType == Js::DiagBlockScopeInObject;
        }
        bool IsCatchScope() const;
        bool IsWithScope() const;
        bool IsSlotScope() const;
        bool IsParamScope() const;
        bool HasProperties() const;
        bool IsAncestorOf(const DebuggerScope* potentialChildScope);
        bool AreAllPropertiesInDeadZone(int byteCodeOffset) const;
        RegSlot GetLocation() const { Assert(IsOwnScope()); return scopeLocation; }
        bool IsOwnScope() const { return scopeLocation != Js::Constants::NoRegister; }
        bool TryGetProperty(Js::PropertyId propertyId, RegSlot location, DebuggerScopeProperty* outScopeProperty) const;
        bool TryGetValidProperty(Js::PropertyId propertyId, RegSlot location, int offset, DebuggerScopeProperty* outScopeProperty, bool* isInDeadZone) const;
        bool UpdatePropertyInitializationOffset(RegSlot location, Js::PropertyId propertyId, int byteCodeOffset, bool isFunctionDeclaration = false);
        void UpdateDueToByteCodeRegeneration(DiagExtraScopesType scopeType, int start, RegSlot scopeLocation);
        void UpdatePropertiesInForInOrOfCollectionScope();

        void SetParentScope(DebuggerScope* parentScope) { this->parentScope = parentScope; }
        DebuggerScope* GetParentScope() const { return parentScope; }
        DebuggerScope* FindCommonAncestor(DebuggerScope* debuggerScope);
        int GetEnd() const { return range.end; }
        int GetStart() const { return range.begin; }

        void SetScopeLocation(RegSlot scopeLocation) { this->scopeLocation = scopeLocation; }

        void SetBegin(int begin);
        void SetEnd(int end);
#if DBG
        void Dump();
        PCWSTR GetDebuggerScopeTypeString(DiagExtraScopesType scopeType);
#endif

    public:
        // The list of scope properties in this scope object.
        // For with scope:  Has 1 property that represents the scoped object.
        // For catch scope: Has 1 property that represents the exception object.
        // For block scope: Has 0-n properties that represent let/const variables in that scope.
        DebuggerScopePropertyList* scopeProperties;
        DiagExtraScopesType scopeType; // The type of scope being represented (With, Catch, or Block scope).
        DebuggerScope* siblingScope;  // Valid only when current scope is slot/activationobject and symbols are on direct regslot
        static const int InvalidScopeIndex = -1;
    private:
        int GetScopeDepth() const;
        bool UpdatePropertyInitializationOffsetInternal(RegSlot location, Js::PropertyId propertyId, int byteCodeOffset, bool isFunctionDeclaration = false);
        void EnsurePropertyListIsAllocated();

    private:
        DebuggerScope* parentScope;
        regex::Interval range; // The start and end byte code writer offsets used when comparing where the debugger is currently stopped at (breakpoint location).
        RegSlot scopeLocation;
        Recycler* recycler;
    };

    class ScopeObjectChain
    {
    public:

        typedef JsUtil::List<DebuggerScope*> ScopeObjectChainList;

        ScopeObjectChain(Recycler* recycler)
            : pScopeChain(nullptr)
        {
            pScopeChain = RecyclerNew(recycler, ScopeObjectChainList, recycler);
        }

        // This function will return DebuggerScopeProperty when the property is found and correctly in the range.
        // If the property is found, but the scope is not in the range, it will return false, but the out param (isPropertyInDebuggerScope) will set to true,
        // and isConst will be updated.
        // If the property is not found at all, it will return false, and isPropertyInDebuggerScope will be false.
        bool TryGetDebuggerScopePropertyInfo(PropertyId propertyId, RegSlot location, int offset, bool* isPropertyInDebuggerScope, bool *isConst, bool* isInDeadZone);

        // List of all Scope Objects in a function. Scopes are added to this list as when they are created in bytecode gen part.
        ScopeObjectChainList* pScopeChain;
    };
#pragma endregion
} // namespace Js
