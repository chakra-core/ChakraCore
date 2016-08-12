//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//This file contains definitions for data structures that track info about the runtime (ThreadContext and ScriptContexts) 
//that are needed by other parts of the TTD system

#if ENABLE_TTD

//default capcities for our core object maps/lists
#define TTD_CORE_OBJECT_COUNT 1028
#define TTD_CORE_FUNCTION_BODY_COUNT 512

namespace TTD
{
    //This struct stores the info for pending async mutations to array buffer objects
    struct TTDPendingAsyncBufferModification
    {
        Js::Var ArrayBufferVar; //the actual array buffer that is pending
        uint32 Index; //the start index that we are monitoring from
    };

    //This class implements the data structures and algorithms needed to manage the ScriptContext TTD runtime info -- it is a friend of ScriptContext
    //Basically we don't want to add a lot of size/complexity to the ScriptContext object/class if it isn't perf critical
    class ScriptContextTTD
    {
    private:
        Js::ScriptContext* m_ctx;

        //Keep track of roots (and local roots as needed)
        ObjectPinSet* m_ttdRootSet;
        ObjectPinSet* m_ttdLocalRootSet;
        JsUtil::BaseDictionary<TTD_LOG_PTR_ID, Js::RecyclableObject*, HeapAllocator> m_ttdRootTagIdMap;

        //List of pending async modifications to array buffers
        JsUtil::List<TTDPendingAsyncBufferModification, HeapAllocator> m_ttdPendingAsyncModList;

        //The lists containing the top-level code that is loaded in this context
        JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator> m_ttdTopLevelScriptLoad;
        JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator> m_ttdTopLevelNewFunction;
        JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator> m_ttdTopLevelEval;

        //need to add back pin set for functionBody to make sure they don't get collected on us
        TTD::FunctionBodyPinSet* m_ttdPinnedRootFunctionSet;
        JsUtil::BaseDictionary<Js::FunctionBody*, Js::FunctionBody*, HeapAllocator> m_ttdFunctionBodyParentMap;

    public:
        //
        //TODO: this results in a memory leak for programs with weak collections -- we should fix this
        //
        ObjectPinSet* TTDWeakReferencePinSet;

        ScriptContextTTD(Js::ScriptContext* ctx);
        ~ScriptContextTTD();

        //Get all of the roots for a script context (roots are currently any recyclableObjects exposed to the host)
        void AddTrackedRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* newRoot);
        void RemoveTrackedRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* deleteRoot);
        const ObjectPinSet* GetRootSet() const;

        void AddLocalRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* newRoot);
        void ClearLocalRootsAndRefreshMap();
        const ObjectPinSet* GetLocalRootSet() const;

        void LoadInvertedRootMap(JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator>& objToLogIdMap) const;
        void ExtractSnapshotRoots(JsUtil::List<Js::Var, HeapAllocator>& roots);

        Js::RecyclableObject* LookupObjectForLogID(TTD_LOG_PTR_ID origId);
        void ClearRootsForSnapRestore();

        //Keep track of pending async ArrayBuffer modification
        void AddToAsyncPendingList(Js::ArrayBuffer* trgt, uint32 index);
        void GetFromAsyncPendingList(TTDPendingAsyncBufferModification* pendingInfo, byte* finalModPos);

        const JsUtil::List<TTDPendingAsyncBufferModification, HeapAllocator>& GetPendingAsyncModListForSnapshot() const;
        void ClearPendingAsyncModListForSnapRestore();

        //Get all of the root level sources evaluated in this script context (source text & root function returned)
        void GetLoadedSources(JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelScriptLoad, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelNewFunction, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelEval);

        //To support cases where we may get cached function bodies ('new Function' & eval) check if we already know of a top-level body
        bool IsBodyAlreadyLoadedAtTopLevel(Js::FunctionBody* body) const;

        //force parsing and load up the parent maps etc.
        void ProcessFunctionBodyOnLoad(Js::FunctionBody* body, Js::FunctionBody* parent);
        void RegisterLoadedScript(Js::FunctionBody* body, uint64 bodyCtrId);
        void RegisterNewScript(Js::FunctionBody* body, uint64 bodyCtrId);
        void RegisterEvalScript(Js::FunctionBody* body, uint64 bodyCtrId);

        //Lookup the parent bofy for a function body (or null for global code)
        Js::FunctionBody* ResolveParentBody(Js::FunctionBody* body) const;

        //
        //TODO: we need to fix this later since filenames are not 100% always unique
        //
        //Find the body with the filename from our top-level function bodies
        Js::FunctionBody* FindFunctionBodyByFileName(const char16* filename) const;

        void ClearLoadedSourcesForSnapshotRestore();
    };

    //////////////////

    //This class implements the data structures and algorithms needed to manage the ScriptContext core image 
    class RuntimeContextInfo
    {
    private:
        ////
        //code for performing well known object walk
        //A worklist to use for the core obj processing
        JsUtil::Queue<Js::RecyclableObject*, HeapAllocator> m_worklist;

        //A null string we use in a number of places
        UtilSupport::TTAutoString m_nullString;

        //A dictionary which contains the paths for "core" image objects and function bodies
        JsUtil::BaseDictionary<Js::RecyclableObject*, UtilSupport::TTAutoString*, HeapAllocator> m_coreObjToPathMap;
        JsUtil::BaseDictionary<Js::FunctionBody*, UtilSupport::TTAutoString*, HeapAllocator> m_coreBodyToPathMap;
        
        JsUtil::List<Js::RecyclableObject*, HeapAllocator> m_sortedObjectList;
        JsUtil::List<Js::FunctionBody*, HeapAllocator> m_sortedFunctionBodyList;
        
        //Build a path string based on a given name
        void BuildPathString(UtilSupport::TTAutoString, const char16* name, const char16* optaccessortag, UtilSupport::TTAutoString& into);

        //Ensure that when we do our core visit make sure that the properties always appear in the same order
        static void LoadAndOrderPropertyNames(Js::RecyclableObject* obj, JsUtil::List<const Js::PropertyRecord*, HeapAllocator>& propertyList);
        static bool PropertyNameCmp(const Js::PropertyRecord* p1, const Js::PropertyRecord* p2);
        ////
        
    public:
        RuntimeContextInfo();
        ~RuntimeContextInfo();

        //Mark all the well-known objects/values/types from this script context
        void MarkWellKnownObjects_TTD(MarkTable& marks) const;

        //Get the path name for a known path object (or function body)
        TTD_WELLKNOWN_TOKEN ResolvePathForKnownObject(Js::RecyclableObject* obj) const;
        TTD_WELLKNOWN_TOKEN ResolvePathForKnownFunctionBody(Js::FunctionBody* fbody) const;

        //Given a path name string lookup the coresponding object
        Js::RecyclableObject* LookupKnownObjectFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const;
        Js::FunctionBody* LookupKnownFunctionBodyFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const;

        //Walk the "known names" we use and fill the map with the objects at said names
        void GatherKnownObjectToPathMap(Js::ScriptContext* ctx);

        ////

        //Enqueue a root object in our core path walk
        void EnqueueRootPathObject(const char16* rootName, Js::RecyclableObject* obj);

        //Enqueue a child object that is stored at the given property in the parent 
        void EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const Js::PropertyRecord* prop, const char16* optacessortag = nullptr);
        void EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const char16* propName, const char16* optacessortag = nullptr);

        //Enqueue a child object that is stored at a special named location in the parent object
        void EnqueueNewFunctionBodyObject(Js::RecyclableObject* parent, Js::FunctionBody* fbody, const char16* name);

        //Build a path string based on a root path and an array index
        void BuildArrayIndexBuffer(uint32 arrayidx, UtilSupport::TTAutoString& res);

        //Build a path string based on a root path and an environment index
        void BuildEnvironmentIndexBuffer(uint32 envidx, UtilSupport::TTAutoString& res);

        //Build a path string based on an environment index and a slot index
        void BuildEnvironmentIndexAndSlotBuffer(uint32 envidx, uint32 slotidx, UtilSupport::TTAutoString& res);
    };

    //////////////////

    //Algorithms for sorting searching a list based on lexo-order from names in a map
    template <typename T>
    void SortDictIntoListOnNames(const JsUtil::BaseDictionary<T, UtilSupport::TTAutoString*, HeapAllocator>& objToNameMap, JsUtil::List<T, HeapAllocator>& sortedObjList, const UtilSupport::TTAutoString& nullString)
    {
        AssertMsg(sortedObjList.Count() == 0, "This should be empty.");

        objToNameMap.Map([&](T key, UtilSupport::TTAutoString* value)
        {
            sortedObjList.Add(key);
        });

        //now sort the list so the traversal order is stable
        //Rock a custom shell sort!!!!
        const int32 gaps[8] = { 701, 301, 132, 57, 23, 10, 4, 1 };

        int32 llen = sortedObjList.Count();
        for(uint32 gapi = 0; gapi < 8; ++gapi)
        {
            int32 gap = gaps[gapi];

            for(int32 i = gap; i < llen; i++)
            {
                T temp = sortedObjList.Item(i);
                const UtilSupport::TTAutoString* tempStr = objToNameMap.LookupWithKey(temp, nullptr);

                int32 j = 0;
                for(j = i; j >= gap && (wcscmp(objToNameMap.LookupWithKey(sortedObjList.Item(j - gap), nullptr)->GetStrValue(), tempStr->GetStrValue()) > 0); j -= gap)
                {
                    T shiftElem = sortedObjList.Item(j - gap);
                    sortedObjList.SetItem(j, shiftElem);
                }

                sortedObjList.SetItem(j, temp);
            }
        }
    }

    template <typename T, bool mustFind>
    int32 LookupPositionInDictNameList(const char16* key, const JsUtil::BaseDictionary<T, UtilSupport::TTAutoString*, HeapAllocator>& objToNameMap, const JsUtil::List<T, HeapAllocator>& sortedObjList, const UtilSupport::TTAutoString& nullString)
    {
        AssertMsg(sortedObjList.Count() != 0, "We are using this for matching so obviously no match and there is a problem.");

        int32 imin = 0;
        int32 imax = sortedObjList.Count() - 1;

        while(imin < imax)
        {
            int imid = (imin + imax) / 2;
            const UtilSupport::TTAutoString* imidStr = objToNameMap.LookupWithKey(sortedObjList.Item(imid), nullptr);
            AssertMsg(imid < imax && imidStr != nullptr, "Something went wrong with our indexing.");

            int32 scmpval = wcscmp(imidStr->GetStrValue(), key);
            if(scmpval < 0)
            {
                imin = imid + 1;
            }
            else
            {
                imax = imid;
            }

        }
        AssertMsg(imin == imax, "Something went wrong!!!"); 
        
        const UtilSupport::TTAutoString* resStr = objToNameMap.LookupWithKey(sortedObjList.Item(imin), nullptr);
        if(mustFind)
        {
            AssertMsg(wcscmp(resStr->GetStrValue(), key) == 0, "We are missing something");
            return imin;
        }
        else
        {
            return (wcscmp(resStr->GetStrValue(), key) == 0) ? imin : -1;
        }
    }
}

#endif

