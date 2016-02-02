//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    RuntimeThreadInfo::RuntimeThreadInfo(ThreadContext* threadContext, ArenaAllocator* generalAllocator, ArenaAllocator* bulkAllocator, ArenaAllocator* taggingAllocator)
        : m_generalAllocator(generalAllocator), m_bulkAllocator(bulkAllocator), m_taggingAllocator(taggingAllocator),
        m_logTagCtr(TTD_INITIAL_LOG_TAG), m_identityTagCtr(TTD_INITIAL_IDENTITY_TAG),
        m_loggedObjectToTagMap(taggingAllocator, TTD_TAG_MAP_DEFAULT_CAPACITY), m_tagToLoggedObjectMap(taggingAllocator, TTD_TAG_MAP_DEFAULT_CAPACITY)
    {
        ;
    }

    RuntimeThreadInfo::~RuntimeThreadInfo()
    {
        ;
    }

    void RuntimeThreadInfo::GetTagsForSnapshot(TTD_LOG_TAG* logTag, TTD_IDENTITY_TAG* identityTag) const
    {
        *logTag = this->m_logTagCtr;
        *identityTag = this->m_identityTagCtr;
    }

    void RuntimeThreadInfo::ResetTagsForRestore_TTD(TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag)
    {
        this->m_logTagCtr = logTag;
        this->m_identityTagCtr = identityTag;

        this->m_loggedObjectToTagMap.Clear();
        this->m_tagToLoggedObjectMap.Clear();
    }

    void RuntimeThreadInfo::SetObjectTrackingTagSnapAndInflate_TTD(TTD_LOG_TAG logTag, TTD_IDENTITY_TAG identityTag, Js::RecyclableObject* obj)
    {
        if(logTag != TTD_INVALID_LOG_TAG)
        {
            this->m_loggedObjectToTagMap.AddNew(obj, logTag);
            this->m_tagToLoggedObjectMap.AddNew(logTag, obj);
        }

#if ENABLE_TTD_IDENTITY_TRACING
        if(Js::DynamicType::Is(obj->GetTypeId()))
        {
            Js::DynamicObject::FromVar(obj)->SetIdentity_TTD(identityTag);
        }
#else
        AssertMsg(identityTag == TTD_INVALID_IDENTITY_TAG, "Tracing is off how is this not the invalid value?");
#endif
    }

    void RuntimeThreadInfo::TrackTagObject(Js::RecyclableObject* obj)
    {
        //
        //TODO: this is a bit rough as we never clean the root set so we grow without bound here. 
        //

        bool isTracked = obj->GetScriptContext()->IsRootTrackedObject_TTD(obj);
        if(!isTracked)
        {
            TTD_LOG_TAG tag = this->m_logTagCtr;
            TTD_INCREMENT_LOG_TAG(this->m_logTagCtr);

            this->m_loggedObjectToTagMap.Add(obj, tag);
            this->m_tagToLoggedObjectMap.Add(tag, obj);

            obj->GetScriptContext()->AddTrackedRoot_TTD(obj);
        }
    }

    void RuntimeThreadInfo::UnTrackTagObject(Js::RecyclableObject* obj)
    {
        bool isTracked = obj->GetScriptContext()->IsRootTrackedObject_TTD(obj);
        if(isTracked)
        {
            TTD_LOG_TAG tag = this->m_loggedObjectToTagMap.LookupWithKey(obj, TTD_INVALID_LOG_TAG);

            this->m_loggedObjectToTagMap.Remove(obj);
            this->m_tagToLoggedObjectMap.Remove(tag);

            obj->GetScriptContext()->RemoveTrackedRoot_TTD(obj);
        }
    }

    TTD_LOG_TAG RuntimeThreadInfo::LookupTagForObject(Js::RecyclableObject* obj) const
    {
        TTD_LOG_TAG tag = this->m_loggedObjectToTagMap.LookupWithKey(obj, TTD_INVALID_LOG_TAG);
        AssertMsg(tag != TTD_INVALID_LOG_TAG, "Missing tag!!!");

        return tag;
    }

    Js::RecyclableObject* RuntimeThreadInfo::LookupObjectForTag(TTD_LOG_TAG tag) const
    {
        Js::RecyclableObject* obj = this->m_tagToLoggedObjectMap.LookupWithKey(tag, nullptr);
        AssertMsg(obj != nullptr, "Missing tag!!!");

        return obj;
    }

    void RuntimeThreadInfo::MarkLoggedObjects(MarkTable& marks) const
    {
        for(auto iter = this->m_loggedObjectToTagMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const Js::RecyclableObject* obj = iter.CurrentKey();
            if(marks.IsMarked(obj))
            {
                marks.MarkAddrWithSpecialInfo<MarkTableTag::LogTaggedObj>(obj);
            }
        }
    }

    void RuntimeThreadInfo::JsRTTagObject(ThreadContext* threadContext, Js::Var value)
    {
        //TTD is completely disabled so we aren't doing anything
        if(threadContext->TTDInfo == nullptr || !TTD::JsSupport::IsVarPtrValued(value))
        {
            return;
        }

        if(threadContext->TTDLog->ShouldTagForJsRT())
        {
            threadContext->TTDInfo->TrackTagObject(Js::RecyclableObject::FromVar(value));
        }
    }

#if ENABLE_TTD_IDENTITY_TRACING
    TTD_IDENTITY_TAG RuntimeThreadInfo::GenNextObjectIdentityTag()
    {
        TTD_IDENTITY_TAG resTag = this->m_identityTagCtr;
        TTD_INCREMENT_IDENTITY_TAG(this->m_identityTagCtr);

        return resTag;
    }

    TTD_IDENTITY_TAG RuntimeThreadInfo::GenNextObjectIdentityTag_InitialSnapshot()
    {
        return this->GenNextObjectIdentityTag() * -1;
    }

    bool RuntimeThreadInfo::IsInitialSnapshotIdentityTag(TTD_IDENTITY_TAG identity)
    {
        return identity < 0;
    }
#endif
    //////////////////

    void RuntimeContextInfo::BuildPathString(UtilSupport::TTAutoString rootpath, LPCWSTR name, LPCWSTR optaccessortag, UtilSupport::TTAutoString& into)
    {
        into.Append(rootpath);
        into.Append(L".");
        into.Append(name);

        if(optaccessortag != nullptr)
        {
           into.Append(optaccessortag);
        }
    }

    void RuntimeContextInfo::LoadAndOrderPropertyNames(Js::RecyclableObject* obj, JsUtil::List<const Js::PropertyRecord*, ArenaAllocator>& propertyList)
    {
        AssertMsg(propertyList.Count() == 0, "This should be empty.");

        Js::ScriptContext* ctx = obj->GetScriptContext();
        uint32 propcount = (uint32)obj->GetPropertyCount();

        //get all of the properties
        for(uint32 i = 0; i < propcount; ++i)
        {
            Js::PropertyIndex propertyIndex = (Js::PropertyIndex)i;
            Js::PropertyId propertyId = obj->GetPropertyId(propertyIndex);

            if(!Js::IsInternalPropertyId(propertyId))
            {
                AssertMsg(obj->HasOwnProperty(propertyId), "We are assuming this is own property count.");

                propertyList.Add(ctx->GetPropertyName(propertyId));
            }
        }

        //now sort the list so the traversal order is stable
        //Rock a custom shell sort!!!!
        const int32 gaps[6] = { 132, 57, 23, 10, 4, 1 };

        int32 llen = propertyList.Count();
        for(uint32 gapi = 0; gapi < 6; ++gapi)
        {
            int32 gap = gaps[gapi];

            for(int32 i = gap; i < llen; i++)
            {
                const Js::PropertyRecord* temp = propertyList.Item(i);

                int32 j = 0;
                for(j = i; j >= gap && PropertyNameCmp(propertyList.Item(j - gap), temp); j -= gap)
                {
                    const Js::PropertyRecord* shiftElem = propertyList.Item(j - gap);
                    propertyList.SetItem(j, shiftElem);
                }

                propertyList.SetItem(j, temp);
            }
        }
    }

    bool RuntimeContextInfo::PropertyNameCmp(const Js::PropertyRecord* p1, const Js::PropertyRecord* p2)
    {
        if(p1->GetLength() != p2->GetLength())
        {
            return p1->GetLength() > p2->GetLength();
        }
        else
        {
            return wcscmp(p1->GetBuffer(), p2->GetBuffer()) > 0;
        }
    }
    
    RuntimeContextInfo::RuntimeContextInfo(ArenaAllocator* allocator)
        : m_shaddowAllocator(allocator), m_worklist(allocator), m_nullString(),
        m_coreObjToPathMap(allocator, TTD_CORE_OBJECT_COUNT), m_coreBodyToPathMap(allocator, TTD_CORE_FUNCTION_BODY_COUNT),
        m_sortedObjectList(allocator, TTD_CORE_OBJECT_COUNT), m_sortedFunctionBodyList(allocator, TTD_CORE_FUNCTION_BODY_COUNT)
    {
        ;
    }

    RuntimeContextInfo::~RuntimeContextInfo()
    {
        for(auto iter = this->m_coreObjToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const_cast<TTD::UtilSupport::TTAutoString&>(iter.CurrentValue()).Clear();
        }

        for(auto iter = this->m_coreBodyToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            const_cast<TTD::UtilSupport::TTAutoString&>(iter.CurrentValue()).Clear();
        }
    }

    //Mark all the well-known objects/values/types from this script context
    void RuntimeContextInfo::MarkWellKnownObjects_TTD(MarkTable& marks) const
    {
        for(int32 i = 0; i < this->m_sortedObjectList.Count(); ++i)
        {
            Js::RecyclableObject* obj = this->m_sortedObjectList.Item(i);
            marks.MarkAddrWithSpecialInfo<MarkTableTag::JsWellKnownObj>(obj);
        }

        for(int32 i = 0; i < this->m_sortedFunctionBodyList.Count(); ++i)
        {
            Js::FunctionBody* body = this->m_sortedFunctionBodyList.Item(i);
            marks.MarkAddrWithSpecialInfo<MarkTableTag::JsWellKnownObj>(body);
        }
    }

    TTD_WELLKNOWN_TOKEN RuntimeContextInfo::ResolvePathForKnownObject(Js::RecyclableObject* obj) const
    {
        const UtilSupport::TTAutoString& res = this->m_coreObjToPathMap.LookupWithKey(obj, this->m_nullString);
        
        return (!res.IsNullString()) ? res.GetStrValue() : TTD_INVALID_WELLKNOWN_TOKEN;
    }

    TTD_WELLKNOWN_TOKEN RuntimeContextInfo::ResolvePathForKnownFunctionBody(Js::FunctionBody* fbody) const
    {
        const UtilSupport::TTAutoString& res = this->m_coreBodyToPathMap.LookupWithKey(fbody, this->m_nullString);
        
        return (!res.IsNullString()) ? res.GetStrValue() : TTD_INVALID_WELLKNOWN_TOKEN;
    }

    Js::RecyclableObject* RuntimeContextInfo::LookupKnownObjectFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::RecyclableObject*, true>(pathIdString, this->m_coreObjToPathMap, this->m_sortedObjectList, this->m_nullString);
        
        return (pos != -1) ? this->m_sortedObjectList.Item(pos) : nullptr;
    }

    Js::FunctionBody* RuntimeContextInfo::LookupKnownFunctionBodyFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::FunctionBody*, true>(pathIdString, this->m_coreBodyToPathMap, this->m_sortedFunctionBodyList, this->m_nullString);
        AssertMsg(pos != -1, "Missing function.");

        return (pos != -1) ? this->m_sortedFunctionBodyList.Item(pos) : nullptr;
    }

    void RuntimeContextInfo::GatherKnownObjectToPathMap(Js::ScriptContext* ctx)
    {
        JsUtil::List<const Js::PropertyRecord*, ArenaAllocator> propertyRecordList(this->m_shaddowAllocator);

        this->EnqueueRootPathObject(L"global", ctx->GetGlobalObject());
        this->EnqueueRootPathObject(L"null", ctx->GetLibrary()->GetNull());

        this->EnqueueRootPathObject(L"_defaultAccessor", ctx->GetLibrary()->GetDefaultAccessorFunction());

        //DEBUG
        uint32 counter = 0;

        while(!this->m_worklist.Empty())
        {
            Js::RecyclableObject* curr = this->m_worklist.Dequeue();

            counter++;

            ////
            //Handle the standard properties for all object types

            //load propArray with all property names
            propertyRecordList.Clear();
            LoadAndOrderPropertyNames(curr, propertyRecordList);

            //access each property and process the target objects as needed
            for(int32 i = 0; i < propertyRecordList.Count(); ++i)
            {
                const Js::PropertyRecord* precord = propertyRecordList.Item(i);

                Js::Var getter = nullptr;
                Js::Var setter = nullptr;
                if(curr->GetAccessors(precord->GetPropertyId(), &getter, &setter, ctx))
                {
                    if(getter != nullptr && !Js::JavascriptOperators::IsUndefinedObject(getter))
                    {
                        AssertMsg(Js::JavascriptFunction::Is(getter), "The getter is not a function?");
                        this->EnqueueNewPathVarAsNeeded(curr, getter, precord, L">");
                    }

                    if(setter != nullptr && !Js::JavascriptOperators::IsUndefinedObject(setter))
                    {
                        AssertMsg(Js::JavascriptFunction::Is(setter), "The setter is not a function?");
                        this->EnqueueNewPathVarAsNeeded(curr, Js::RecyclableObject::FromVar(setter), precord, L"<");
                    }
                }
                else
                {
                    Js::Var pitem = nullptr;
                    BOOL isproperty = Js::JavascriptOperators::GetOwnProperty(curr, precord->GetPropertyId(), &pitem, ctx);
                    AssertMsg(isproperty, "Not sure what went wrong.");

                    this->EnqueueNewPathVarAsNeeded(curr, pitem, precord, nullptr);
                }
            }

            //shouldn't have any dynamic array valued properties
            AssertMsg(!Js::DynamicType::Is(curr->GetTypeId()) || (Js::DynamicObject::FromVar(curr))->GetObjectArray() == nullptr || (Js::DynamicObject::FromVar(curr))->GetObjectArray()->GetLength() == 0, "Shouldn't have any dynamic array valued properties at this point.");

            Js::RecyclableObject* proto = curr->GetPrototype();
            bool skipProto = (proto == nullptr) || Js::JavascriptOperators::IsUndefinedOrNullType(proto->GetTypeId());
            if(!skipProto)
            {
                this->EnqueueNewPathVarAsNeeded(curr, proto, L"_proto_");
            }

            if(Js::GlobalObject::Is(curr))
            {
                //Place some other values in known locations (they may lazily appear later but we want to force them now)
                this->EnqueueRootPathObject(L"__Shadow_DefaultAccessorFunction", ctx->GetLibrary()->GetDefaultAccessorFunction());
            }

            curr->ProcessCorePaths();
        }

        SortDictIntoListOnNames<Js::RecyclableObject*>(this->m_coreObjToPathMap, this->m_sortedObjectList, this->m_nullString);
        SortDictIntoListOnNames<Js::FunctionBody*>(this->m_coreBodyToPathMap, this->m_sortedFunctionBodyList, this->m_nullString);
    }

    ////

    void RuntimeContextInfo::EnqueueRootPathObject(LPCWSTR rootName, Js::RecyclableObject* obj)
    {
        this->m_worklist.Enqueue(obj);

        UtilSupport::TTAutoString rootStr(rootName);
        this->m_coreObjToPathMap.AddNew(obj, rootStr);
    }

    void RuntimeContextInfo::EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const Js::PropertyRecord* prop, LPCWSTR optacessortag)
    {
        this->EnqueueNewPathVarAsNeeded(parent, val, prop->GetBuffer(), optacessortag);
    }

    void RuntimeContextInfo::EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, LPCWSTR propName, LPCWSTR optacessortag)
    {
        if(JsSupport::IsVarTaggedInline(val))
        {
            return;
        }

        if(JsSupport::IsVarPrimitiveKind(val) && !Js::GlobalObject::Is(parent))
        {
            return; //we keep primitives from global object only -- may need others but this is a simple way to start to get undefined, null, infy, etc.
        }

        Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(val);
        if(!this->m_coreObjToPathMap.ContainsKey(obj))
        {   
            const UtilSupport::TTAutoString& ppath = this->m_coreObjToPathMap.LookupWithKey(parent, this->m_nullString);

            this->m_worklist.Enqueue(obj);

            UtilSupport::TTAutoString tpath(ppath);
            tpath.Append(L".");
            tpath.Append(propName);

            if(optacessortag != nullptr)
            {
                tpath.Append(optacessortag);
            }

            this->m_coreObjToPathMap.AddNew(obj, tpath);
        }
    }

    void RuntimeContextInfo::EnqueueNewFunctionBodyObject(Js::RecyclableObject* parent, Js::FunctionBody* fbody, LPCWSTR name)
    {
        if(!this->m_coreBodyToPathMap.ContainsKey(fbody))
        {
            const UtilSupport::TTAutoString& ppath = this->m_coreObjToPathMap.LookupWithKey(parent, this->m_nullString);

            UtilSupport::TTAutoString fpath(ppath);

            fpath.Append(L".");
            fpath.Append(name);

            this->m_coreBodyToPathMap.AddNew(fbody, fpath);
        }
    }

    UtilSupport::TTAutoString RuntimeContextInfo::BuildArrayIndexBuffer(uint32 arrayidx)
    {
        UtilSupport::TTAutoString res(L"!arrayContents[");
        res.Append(arrayidx);
        res.Append(L"]");

        return res;
    }

    UtilSupport::TTAutoString RuntimeContextInfo::BuildEnvironmentIndexBuffer(uint32 envidx)
    {
        UtilSupport::TTAutoString res(L"!env[");
        res.Append(envidx);
        res.Append(L"]");

        return res;
    }

    UtilSupport::TTAutoString RuntimeContextInfo::BuildEnvironmentIndexBodyBuffer(uint32 envidx)
    {
        UtilSupport::TTAutoString res(L"!env[");
        res.Append(envidx);
        res.Append(L"].!body");

        return res;
    }

    UtilSupport::TTAutoString RuntimeContextInfo::BuildEnvironmentIndexAndSlotBuffer(uint32 envidx, uint32 slotidx)
    {
        UtilSupport::TTAutoString res(L"!env[");
        res.Append(envidx);
        res.Append(L"].!slot[");
        res.Append(slotidx);
        res.Append(L"]");

        return res;
    }
}

#endif