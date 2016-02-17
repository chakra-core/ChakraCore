//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//This file contains definitions for data structures that track info about the runtime (ThreadContext and ScriptContexts) 
//that are needed by other parts of the TTD system

#if ENABLE_TTD

//A default capacity to use for our tag maps
#define TTD_TAG_MAP_DEFAULT_CAPACITY 512

//A default capacity to use for the 
#define TTD_PROPERTY_SET_DEFAULT_CAPACITY 1024

//default capcities for our core object maps/lists
#define TTD_CORE_OBJECT_COUNT 1028
#define TTD_CORE_FUNCTION_BODY_COUNT 512

namespace TTD
{
    //This class implements a data structure for tracking tagged object & pinned properties in the ThreadContext
    class RuntimeThreadInfo
    {
    private:
        //The counter we use for the tagging (and the counter we use for identity tagging)
        TTD_LOG_TAG m_logTagCtr;
        TTD_IDENTITY_TAG m_identityTagCtr;

        //Maps of objects that we log and track accross host/engine
        JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_TAG, ArenaAllocator> m_loggedObjectToTagMap;
        JsUtil::BaseDictionary<TTD_LOG_TAG, Js::RecyclableObject*, ArenaAllocator> m_tagToLoggedObjectMap;

        //Memory allocators that everyone uses
        ArenaAllocator* m_generalAllocator;
        ArenaAllocator* m_bulkAllocator;

        ArenaAllocator* m_taggingAllocator;

    public:
        RuntimeThreadInfo(ThreadContext* threadContext, ArenaAllocator* generalAllocator, ArenaAllocator* bulkAllocator, ArenaAllocator* taggingAllocator);
        ~RuntimeThreadInfo();

        ArenaAllocator* GetGeneralAllocator() { return this->m_generalAllocator; }
        ArenaAllocator* GetBulkAllocator() { return this->m_bulkAllocator; }

        //When reset the object tagging
        void GetTagsForSnapshot(TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const;
        void ResetTagsForRestore_TTD(TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag);

        //Set a specific tag for an object (snapshot inflate) -- if log tag is invalid then we don't add it to any of the tracked maps
        void SetObjectTrackingTagSnapAndInflate_TTD(TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag, Js::RecyclableObject* obj);

        //Add/remove an object to the tracked tag map 
        void TrackTagObject(Js::RecyclableObject* obj);
        void UnTrackTagObject(Js::RecyclableObject* obj);

        //Lookup objects for tags and tags for objects
        TTD_LOG_TAG LookupTagForObject(Js::RecyclableObject* obj) const;
        Js::RecyclableObject* LookupObjectForTag(TTD_LOG_TAG tag) const;

        //Mark all of the logged objects with their special tag
        void MarkLoggedObjects(MarkTable& marks) const;

        //Do any cross host/engine object tagging -- handles all cases so call this from all the JsRT APIs
        //Not enabled
        //Enabled but not running record/debug yet
        //Enabled and in record/debug 
        //Detached
        static void JsRTTagObject(ThreadContext* threadContext, Js::Var value);

#if ENABLE_TTD_IDENTITY_TRACING
        //Generate the next identity tag for an object
        TTD_IDENTITY_TAG GenNextObjectIdentityTag();

        //Generate the next identity tag for an object -- as a special value that we associate with the initial snapshot
        TTD_IDENTITY_TAG GenNextObjectIdentityTag_InitialSnapshot();

        //Return true if the tag is for an object seen during the initial snapshot
        static bool IsInitialSnapshotIdentityTag(TTD_IDENTITY_TAG identity);
#endif
    };

    //////////////////

    //This class implements the data structures and algorithms needed by the ScriptContext
    class RuntimeContextInfo
    {
    private:
        //The allocator to use for this context
        ArenaAllocator* m_shaddowAllocator;

        ////
        //code for performing well known object walk
        //A worklist to use for the core obj processing
        JsUtil::Queue<Js::RecyclableObject*, ArenaAllocator> m_worklist;

        //A null string we use in a number of places
        UtilSupport::TTAutoString m_nullString;

        //A dictionary which contains the paths for "core" image objects and function bodies
        JsUtil::BaseDictionary<Js::RecyclableObject*, UtilSupport::TTAutoString, ArenaAllocator> m_coreObjToPathMap;
        JsUtil::BaseDictionary<Js::FunctionBody*, UtilSupport::TTAutoString, ArenaAllocator> m_coreBodyToPathMap;
        
        JsUtil::List<Js::RecyclableObject*, ArenaAllocator> m_sortedObjectList;
        JsUtil::List<Js::FunctionBody*, ArenaAllocator> m_sortedFunctionBodyList;
        
        //Build a path string based on a given name
        void BuildPathString(UtilSupport::TTAutoString, LPCWSTR name, LPCWSTR optaccessortag, UtilSupport::TTAutoString& into);

        //Ensure that when we do our core visit make sure that the properties always appear in the same order
        static void LoadAndOrderPropertyNames(Js::RecyclableObject* obj, JsUtil::List<const Js::PropertyRecord*, ArenaAllocator>& propertyList);
        static bool PropertyNameCmp(const Js::PropertyRecord* p1, const Js::PropertyRecord* p2);
        ////
        
    public:
        RuntimeContextInfo(ArenaAllocator* allocator);
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
        void EnqueueRootPathObject(LPCWSTR rootName, Js::RecyclableObject* obj);

        //Enqueue a child object that is stored at the given property in the parent 
        void EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const Js::PropertyRecord* prop, LPCWSTR optacessortag = nullptr);
        void EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, LPCWSTR propName, LPCWSTR optacessortag = nullptr);

        //Enqueue a child object that is stored at a special named location in the parent object
        void EnqueueNewFunctionBodyObject(Js::RecyclableObject* parent, Js::FunctionBody* fbody, LPCWSTR name);

        //Build a path string based on a root path and an array index
        UtilSupport::TTAutoString BuildArrayIndexBuffer(uint32 arrayidx);

        //Build a path string based on a root path and an environment index
        UtilSupport::TTAutoString BuildEnvironmentIndexBuffer(uint32 envidx);

        //Build a path string based on a root path and an environment index and the associated body
        UtilSupport::TTAutoString BuildEnvironmentIndexBodyBuffer(uint32 envidx);

        //Build a path string based on an environment index and a slot index
        UtilSupport::TTAutoString BuildEnvironmentIndexAndSlotBuffer(uint32 envidx, uint32 slotidx);
    };

    //////////////////

    //Algorithms for sorting searching a list based on lexo-order from names in a map
    template <typename T>
    void SortDictIntoListOnNames(const JsUtil::BaseDictionary<T, UtilSupport::TTAutoString, ArenaAllocator>& objToNameMap, JsUtil::List<T, ArenaAllocator>& sortedObjList, const UtilSupport::TTAutoString& nullString)
    {
        AssertMsg(sortedObjList.Count() == 0, "This should be empty.");

        objToNameMap.Map([&](T key, UtilSupport::TTAutoString value)
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
                const UtilSupport::TTAutoString& tempStr = objToNameMap.LookupWithKey(temp, nullString);

                int32 j = 0;
                for(j = i; j >= gap && (wcscmp(objToNameMap.LookupWithKey(sortedObjList.Item(j - gap), nullString).GetStrValue(), tempStr.GetStrValue()) > 0); j -= gap)
                {
                    T shiftElem = sortedObjList.Item(j - gap);
                    sortedObjList.SetItem(j, shiftElem);
                }

                sortedObjList.SetItem(j, temp);
            }
        }
    }

    template <typename T, bool mustFind>
    int32 LookupPositionInDictNameList(LPCWSTR key, const JsUtil::BaseDictionary<T, UtilSupport::TTAutoString, ArenaAllocator>& objToNameMap, const JsUtil::List<T, ArenaAllocator>& sortedObjList, const UtilSupport::TTAutoString& nullString)
    {
        AssertMsg(sortedObjList.Count() != 0, "We are using this for matching so obviously no match and there is a problem.");

        int32 imin = 0;
        int32 imax = sortedObjList.Count() - 1;

        while(imin < imax)
        {
            int imid = (imin + imax) / 2;
            const UtilSupport::TTAutoString& imidStr = objToNameMap.LookupWithKey(sortedObjList.Item(imid), nullString);
            AssertMsg(imid < imax && !imidStr.IsNullString(), "Something went wrong with our indexing.");

            int32 scmpval = wcscmp(imidStr.GetStrValue(), key);
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
        
        const UtilSupport::TTAutoString& resStr = objToNameMap.LookupWithKey(sortedObjList.Item(imin), nullString);
        if(mustFind)
        {
            AssertMsg(wcscmp(resStr.GetStrValue(), key) == 0, "We are missing something");
            return imin;
        }
        else
        {
            return (wcscmp(resStr.GetStrValue(), key) == 0) ? imin : -1;
        }
    }
}

#endif

