//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //A class that manages inflation maps during heap state restoration
    class InflateMap
    {
    private:
        TTDIdentifierDictionary<TTD_PTR_ID, Js::DynamicTypeHandler*> m_handlerMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::Type*> m_typeMap;

        //The maps for script contexts and objects 
        TTDIdentifierDictionary<TTD_LOG_PTR_ID, Js::GlobalObject*> m_tagToGlobalObjectMap; //get the script context from here
        TTDIdentifierDictionary<TTD_PTR_ID, Js::RecyclableObject*> m_objectMap;

        //The maps for inflated function bodies
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FunctionBody*> m_functionBodyMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FrameDisplay*> m_environmentMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::Var*> m_slotArrayMap;

        //The maps for resolving debug scopes
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FunctionBody*> m_debuggerScopeHomeBodyMap;
        TTDIdentifierDictionary<TTD_PTR_ID, int32> m_debuggerScopeChainIndexMap;

        //A dictionary for the Promise related bits (not typesafe and a bit ugly but I prefer it to creating multiple additional collections)
        JsUtil::BaseDictionary<TTD_PTR_ID, void*, HeapAllocator> m_promiseDataMap;

        //A set we use to pin all the inflated objects live during/after the inflate process (to avoid accidental collection)
        RecyclerRootPtr<ObjectPinSet> m_inflatePinSet;
        RecyclerRootPtr<EnvironmentPinSet> m_environmentPinSet;
        RecyclerRootPtr<SlotArrayPinSet> m_slotArrayPinSet;

        //A set we use to keep some old objects alive during inflate in case we want to re-use them
        RecyclerRootPtr<ObjectPinSet> m_oldInflatePinSet;

        //Temp data structures for holding info during the inflate process
        TTDIdentifierDictionary<TTD_PTR_ID, Js::RecyclableObject*> m_oldObjectMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FunctionBody*> m_oldFunctionBodyMap;

        JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator> m_propertyReset;

    public:
        InflateMap();
        ~InflateMap();

        void PrepForInitialInflate(ThreadContext* threadContext, uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 dbgScopeCount, uint32 envCount, uint32 slotCount);
        void PrepForReInflate(uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 dbgScopeCount, uint32 envCount, uint32 slotCount);
        void CleanupAfterInflate();

        bool IsObjectAlreadyInflated(TTD_PTR_ID objid) const;
        bool IsFunctionBodyAlreadyInflated(TTD_PTR_ID fbodyid) const;

        Js::RecyclableObject* FindReusableObjectIfExists(TTD_PTR_ID objid) const;
        Js::FunctionBody* FindReusableFunctionBodyIfExists(TTD_PTR_ID fbodyid) const;

        //A version of FindReusableObjectIfExists but we haven't moved the last inflate objects to oldObjects yet so we need to look in a differnt location
        Js::RecyclableObject* FindReusableObject_WellKnowReuseCheck(TTD_PTR_ID objid) const;

        ////

        Js::DynamicTypeHandler* LookupHandler(TTD_PTR_ID handlerId) const;
        Js::Type* LookupType(TTD_PTR_ID typeId) const;

        Js::ScriptContext* LookupScriptContext(TTD_LOG_PTR_ID sctag) const;
        Js::RecyclableObject* LookupObject(TTD_PTR_ID objid) const;

        Js::FunctionBody* LookupFunctionBody(TTD_PTR_ID functionId) const;
        Js::FrameDisplay* LookupEnvironment(TTD_PTR_ID envid) const;
        Js::Var* LookupSlotArray(TTD_PTR_ID slotid) const;

        void LookupInfoForDebugScope(TTD_PTR_ID dbgScopeId, Js::FunctionBody** homeBody, int32* chainIndex) const;

        ////

        void AddDynamicHandler(TTD_PTR_ID handlerId, Js::DynamicTypeHandler* value);
        void AddType(TTD_PTR_ID typeId, Js::Type* value);

        void AddScriptContext(TTD_LOG_PTR_ID sctag, Js::ScriptContext* value);
        void AddObject(TTD_PTR_ID objid, Js::RecyclableObject* value);

        void AddInflationFunctionBody(TTD_PTR_ID functionId, Js::FunctionBody* value);
        void AddEnvironment(TTD_PTR_ID envId, Js::FrameDisplay* value);
        void AddSlotArray(TTD_PTR_ID slotId, Js::Var* value);

        void UpdateFBScopes(const NSSnapValues::SnapFunctionBodyScopeChain& scopeChainInfo, Js::FunctionBody* fb);

        ////

        //Get the inflate stack and property reset set to use for depends on processing
        JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& GetPropertyResetSet();

        Js::Var InflateTTDVar(TTDVar var) const;

        ////

        template <typename T>
        bool IsPromiseInfoDefined(TTD_PTR_ID ptrId) const
        {
            return this->m_promiseDataMap.ContainsKey(ptrId);
        }

        template <typename T>
        void AddInflatedPromiseInfo(TTD_PTR_ID ptrId, T* data)
        {
            this->m_promiseDataMap.AddNew(ptrId, data);
        }

        template <typename T>
        T* LookupInflatedPromiseInfo(TTD_PTR_ID ptrId) const
        {
            return (T*)this->m_promiseDataMap.Item(ptrId);
        }
    };

    //////////////////

#if ENABLE_SNAPSHOT_COMPARE
    enum class TTDCompareTag
    {
        Done,
        //Values should not be matched by ptr -- should be immediately matched by value
        SlotArray,
        FunctionScopeInfo,

        TopLevelLoadFunction,
        TopLevelNewFunction,
        TopLevelEvalFunction,
        FunctionBody,
        SnapObject
    };

    class TTDCompareMap;
    typedef void(*fPtr_AssertSnapEquivAddtlInfo)(const NSSnapObjects::SnapObject* v1, const NSSnapObjects::SnapObject* v2, TTDCompareMap& compareMap);

    class TTDComparePath
    {
    public:
        enum class StepKind
        {
            Empty,
            Root,
            PropertyData,
            PropertyGetter,
            PropertySetter,
            Array,
            Scope,
            SlotArray,
            FunctionBody,
            Special,
            SpecialArray
        };

        struct PathEntry
        {
            int64 IndexOrPID;
            const char16* OptName;
        };

    private:
        const StepKind m_stepKind;
        const TTDComparePath* m_prefix;
        const PathEntry m_step;

    public:
        TTDComparePath();
        TTDComparePath(const TTDComparePath* prefix, StepKind stepKind, const PathEntry& nextStep);

        ~TTDComparePath();

        void WritePathToConsole(ThreadContext* threadContext, bool printNewline, _Out_writes_z_(namebuffLength) char16* namebuff, charcount_t namebuffLength) const;
    };

    //A class that we use to manage all the dictionaries we need when comparing 2 snapshots
    class TTDCompareMap
    {
    public:
        bool StrictCrossSite;

        JsUtil::Queue<TTD_PTR_ID, HeapAllocator> H1PtrIdWorklist;
        JsUtil::BaseDictionary<TTD_PTR_ID, TTD_PTR_ID, HeapAllocator> H1PtrToH2PtrMap;

        TTDComparePath* CurrentPath;
        TTD_PTR_ID CurrentH1Ptr;
        TTD_PTR_ID CurrentH2Ptr;
        ThreadContext* Context;
        char16* PathBuffer;
        JsUtil::BaseDictionary<TTD_PTR_ID, TTDComparePath*, HeapAllocator> H1PtrToPathMap;

        fPtr_AssertSnapEquivAddtlInfo* SnapObjCmpVTable;

        ////
        //H1 Maps
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::SnapPrimitiveValue*, HeapAllocator> H1ValueMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::SlotArrayInfo*, HeapAllocator> H1SlotArrayMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::ScriptFunctionScopeInfo*, HeapAllocator> H1FunctionScopeInfoMap;

        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H1FunctionTopLevelLoadMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H1FunctionTopLevelNewMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H1FunctionTopLevelEvalMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::FunctionBodyResolveInfo*, HeapAllocator> H1FunctionBodyMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapObjects::SnapObject*, HeapAllocator> H1ObjectMap;

        JsUtil::BaseHashSet<TTD_PTR_ID, HeapAllocator> H1PendingAsyncModBufferSet;

        ////
        //H2 Maps
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::SnapPrimitiveValue*, HeapAllocator> H2ValueMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::SlotArrayInfo*, HeapAllocator> H2SlotArrayMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::ScriptFunctionScopeInfo*, HeapAllocator> H2FunctionScopeInfoMap;

        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H2FunctionTopLevelLoadMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H2FunctionTopLevelNewMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, uint64, HeapAllocator> H2FunctionTopLevelEvalMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapValues::FunctionBodyResolveInfo*, HeapAllocator> H2FunctionBodyMap;
        JsUtil::BaseDictionary<TTD_PTR_ID, const NSSnapObjects::SnapObject*, HeapAllocator> H2ObjectMap;

        JsUtil::BaseHashSet<TTD_PTR_ID, HeapAllocator> H2PendingAsyncModBufferSet;

        ////
        //Code

        TTDCompareMap(ThreadContext* threadContext);
        ~TTDCompareMap();

        //Assert the condition must be true and report if not
        void DiagnosticAssert(bool condition);

        //Check that the given mapping either (1) does not exist or (2) is consistent -- if needed add the mapping and to worklist as well
        void CheckConsistentAndAddPtrIdMapping_Helper(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next);

        void CheckConsistentAndAddPtrIdMapping_Scope(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, uint32 index);
        void CheckConsistentAndAddPtrIdMapping_FunctionBody(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId);
        void CheckConsistentAndAddPtrIdMapping_Special(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, const char16* specialField);
        void CheckConsistentAndAddPtrIdMapping_Root(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, TTD_LOG_PTR_ID tag);

        //Check if the given mapping is consistent but do not enqueue or try to lookup ptr id in any of the maps (used mainly for heap allocated promise info that may be shared)
        void CheckConsistentAndAddPtrIdMapping_NoEnqueue(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId);

        void GetNextCompareInfo(TTDCompareTag* tag, TTD_PTR_ID* h1PtrId, TTD_PTR_ID* h2PtrId);

        void GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::SlotArrayInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::SlotArrayInfo** val2);
        void GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::ScriptFunctionScopeInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::ScriptFunctionScopeInfo** val2);

        void GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, uint64* val1, TTD_PTR_ID h2PtrId, uint64* val2);
        void GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::FunctionBodyResolveInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::FunctionBodyResolveInfo** val2);
        void GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapObjects::SnapObject** val1, TTD_PTR_ID h2PtrId, const NSSnapObjects::SnapObject** val2);
    };
#endif
}

#endif

