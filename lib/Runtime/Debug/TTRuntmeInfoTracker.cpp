//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    ScriptContextTTD::ScriptContextTTD(Js::ScriptContext* ctx)
        : m_ctx(ctx),
        m_ttdRootSet(nullptr), m_ttdLocalRootSet(nullptr), m_ttdRootTagIdMap(&HeapAllocator::Instance), m_ttdPendingAsyncModList(&HeapAllocator::Instance),
        m_ttdTopLevelScriptLoad(&HeapAllocator::Instance), m_ttdTopLevelNewFunction(&HeapAllocator::Instance), m_ttdTopLevelEval(&HeapAllocator::Instance),
        m_ttdPinnedRootFunctionSet(nullptr), m_ttdFunctionBodyParentMap(&HeapAllocator::Instance),
        TTDWeakReferencePinSet(nullptr)
    {
        Recycler* ctxRecycler = this->m_ctx->GetRecycler();

        this->m_ttdRootSet = RecyclerNew(ctxRecycler, TTD::ObjectPinSet, ctxRecycler);
        ctxRecycler->RootAddRef(this->m_ttdRootSet);

        this->m_ttdLocalRootSet = RecyclerNew(ctxRecycler, TTD::ObjectPinSet, ctxRecycler);
        ctxRecycler->RootAddRef(this->m_ttdLocalRootSet);

        this->m_ttdPinnedRootFunctionSet = RecyclerNew(ctxRecycler, TTD::FunctionBodyPinSet, ctxRecycler);
        ctxRecycler->RootAddRef(this->m_ttdPinnedRootFunctionSet);

        this->TTDWeakReferencePinSet = RecyclerNew(ctxRecycler, TTD::ObjectPinSet, ctxRecycler);
        ctxRecycler->RootAddRef(this->TTDWeakReferencePinSet);
    }

    ScriptContextTTD::~ScriptContextTTD()
    {
        if(this->m_ttdRootSet != nullptr)
        {
            this->m_ttdRootSet->GetAllocator()->RootRelease(this->m_ttdRootSet);
            this->m_ttdRootSet = nullptr;
        }

        if(this->m_ttdLocalRootSet != nullptr)
        {
            this->m_ttdLocalRootSet->GetAllocator()->RootRelease(this->m_ttdLocalRootSet);
            this->m_ttdLocalRootSet = nullptr;
        }

        this->m_ttdRootTagIdMap.Clear();

        this->m_ttdPendingAsyncModList.Clear();

        this->m_ttdTopLevelScriptLoad.Clear();
        this->m_ttdTopLevelNewFunction.Clear();
        this->m_ttdTopLevelEval.Clear();

        if(this->m_ttdPinnedRootFunctionSet != nullptr)
        {
            this->m_ttdPinnedRootFunctionSet->GetAllocator()->RootRelease(this->m_ttdPinnedRootFunctionSet);
            this->m_ttdPinnedRootFunctionSet = nullptr;
        }

        this->m_ttdFunctionBodyParentMap.Clear();

        if(this->TTDWeakReferencePinSet != nullptr)
        {
            this->TTDWeakReferencePinSet->GetAllocator()->RootRelease(this->TTDWeakReferencePinSet);
            this->TTDWeakReferencePinSet = nullptr;
        }
    }

    //Get all of the roots for a script context (roots are currently any recyclableObjects exposed to the host)
    void ScriptContextTTD::AddTrackedRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* newRoot)
    {
        //We provide special treatment for undefined, null, true, false, global and may add them multiple times 
        AssertMsg(!this->m_ttdRootSet->ContainsKey(newRoot) || (newRoot->GetTypeId() <= Js::TypeIds_Boolean) || (newRoot->GetTypeId() == Js::TypeIds_GlobalObject), "Should not have duplicate inserts.");

        this->m_ttdRootSet->AddNew(newRoot);
        this->m_ttdRootTagIdMap.Item(origId, newRoot);
    }

    void ScriptContextTTD::RemoveTrackedRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* deleteRoot)
    {
        AssertMsg(this->m_ttdRootSet->ContainsKey(deleteRoot), "Should not have delete elements that are not in the root set.");

        //We provide special treatment for undefined, null, true, false, global and want to ensure they are always in the root set and map
        Js::TypeId tid = deleteRoot->GetTypeId();
        if((tid > Js::TypeIds_Boolean) & (tid != Js::TypeIds_GlobalObject))
        {
            this->m_ttdRootSet->Remove(deleteRoot);
            if(!this->m_ttdLocalRootSet->ContainsKey(deleteRoot))
            {
                this->m_ttdRootTagIdMap.Remove(origId);
            }
        }
    }

    const ObjectPinSet* ScriptContextTTD::GetRootSet() const
    {
        return this->m_ttdRootSet;
    }

    void ScriptContextTTD::AddLocalRoot(TTD_LOG_PTR_ID origId, Js::RecyclableObject* newRoot)
    {
        this->m_ttdLocalRootSet->AddNew(newRoot);

        //if the pinned root set already has an entry then don't overwrite that one with a local entry (e.g. we will keep the current value)
        if(!this->m_ttdRootSet->ContainsKey(newRoot))
        {
            this->m_ttdRootTagIdMap.Item(origId, newRoot);
        }
    }

    void ScriptContextTTD::ClearLocalRootsAndRefreshMap()
    {
        this->m_ttdLocalRootSet->Clear();

        this->m_ttdRootTagIdMap.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_LOG_PTR_ID, Js::RecyclableObject*>& entry) -> bool
        {
            Js::RecyclableObject* obj = entry.Value();
            return !this->m_ttdRootSet->Contains(obj);
        });
    }

    const ObjectPinSet* ScriptContextTTD::GetLocalRootSet() const
    {
        return this->m_ttdLocalRootSet;
    }

    void ScriptContextTTD::LoadInvertedRootMap(JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator>& objToLogIdMap) const
    {
        for(auto iter = this->m_ttdRootTagIdMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            objToLogIdMap.AddNew(iter.CurrentValue(), iter.CurrentKey());
        }
    }

    void ScriptContextTTD::ExtractSnapshotRoots(JsUtil::List<Js::Var, HeapAllocator>& roots)
    {
        for(auto rootIter = this->m_ttdRootSet->GetIterator(); rootIter.IsValid(); rootIter.MoveNext())
        {
            Js::RecyclableObject* obj = rootIter.CurrentValue();
            roots.Add(obj);
        }

        for(auto localIter = this->m_ttdLocalRootSet->GetIterator(); localIter.IsValid(); localIter.MoveNext())
        {
            Js::RecyclableObject* obj = localIter.CurrentValue();
            if(!this->m_ttdRootSet->Contains(obj))
            {
                roots.Add(obj);
            }
        }
    }

    Js::RecyclableObject* ScriptContextTTD::LookupObjectForLogID(TTD_LOG_PTR_ID origId)
    {
        //Local root always has mappings for all the ids
        Js::RecyclableObject* res = this->m_ttdRootTagIdMap.LookupWithKey(origId, nullptr);
        AssertMsg(res != nullptr, "We lost an id somewhere");

        return res;
    }

    void ScriptContextTTD::ClearRootsForSnapRestore()
    {
        this->m_ttdRootSet->Clear();
        this->m_ttdLocalRootSet->Clear();

        this->m_ttdRootTagIdMap.Clear();
    }

    void ScriptContextTTD::AddToAsyncPendingList(Js::ArrayBuffer* trgt, uint32 index)
    {
        TTDPendingAsyncBufferModification pending = { trgt, index };
        this->m_ttdPendingAsyncModList.Add(pending);
    }

    void ScriptContextTTD::GetFromAsyncPendingList(TTDPendingAsyncBufferModification* pendingInfo, byte* finalModPos)
    {
        pendingInfo->ArrayBufferVar = nullptr;
        pendingInfo->Index = 0;

        const byte* currentBegin = nullptr;
        int32 pos = -1;
        for(int32 i = 0; i < this->m_ttdPendingAsyncModList.Count(); ++i)
        {
            const TTDPendingAsyncBufferModification& pi = this->m_ttdPendingAsyncModList.Item(i);
            const Js::ArrayBuffer* pbuff = Js::ArrayBuffer::FromVar(pi.ArrayBufferVar);
            const byte* pbuffBegin = pbuff->GetBuffer() + pi.Index;
            const byte* pbuffMax = pbuff->GetBuffer() + pbuff->GetByteLength();

            //if the final mod is less than the start of this buffer + index or off then end then this definitely isn't it so skip
            if(pbuffBegin > finalModPos || pbuffMax < finalModPos)
            {
                continue;
            }

            //it is in the right range so now we assume non-overlapping so we see if this pbuffBegin is closer than the current best
            AssertMsg(finalModPos != currentBegin, "We have something strange!!!");
            if(currentBegin == nullptr || finalModPos < currentBegin)
            {
                currentBegin = pbuffBegin;
                pos = (int32)i;
            }
        }
        AssertMsg(pos != -1, "Missing matching register!!!");

        *pendingInfo = this->m_ttdPendingAsyncModList.Item(pos);
        this->m_ttdPendingAsyncModList.RemoveAt(pos);
    }

    const JsUtil::List<TTDPendingAsyncBufferModification, HeapAllocator>& ScriptContextTTD::GetPendingAsyncModListForSnapshot() const
    {
        return this->m_ttdPendingAsyncModList;
    }

    void ScriptContextTTD::ClearPendingAsyncModListForSnapRestore()
    {
        this->m_ttdPendingAsyncModList.Clear();
    }

    void ScriptContextTTD::GetLoadedSources(JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelScriptLoad, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelNewFunction, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelEval)
    {
        AssertMsg(topLevelScriptLoad.Count() == 0 && topLevelNewFunction.Count() == 0 && topLevelEval.Count() == 0, "Should be empty when you call this.");

        topLevelScriptLoad.AddRange(this->m_ttdTopLevelScriptLoad);
        topLevelNewFunction.AddRange(this->m_ttdTopLevelNewFunction);
        topLevelEval.AddRange(this->m_ttdTopLevelEval);
    }

    bool ScriptContextTTD::IsBodyAlreadyLoadedAtTopLevel(Js::FunctionBody* body) const
    {
        return this->m_ttdPinnedRootFunctionSet->Contains(body);
    }

    void ScriptContextTTD::ProcessFunctionBodyOnLoad(Js::FunctionBody* body, Js::FunctionBody* parent)
    {
        //if this is a root (parent is null) then put this in the rootbody pin set so it isn't reclaimed on us
        if(parent == nullptr)
        {
            AssertMsg(!this->m_ttdPinnedRootFunctionSet->Contains(body), "We already added this function!!!");
            this->m_ttdPinnedRootFunctionSet->AddNew(body);
        }

        this->m_ttdFunctionBodyParentMap.AddNew(body, parent);

        for(uint32 i = 0; i < body->GetNestedCount(); ++i)
        {
            Js::ParseableFunctionInfo* pfiMid = body->GetNestedFunc(i)->EnsureDeserialized();
            Js::FunctionBody* currfb = TTD::JsSupport::ForceAndGetFunctionBody(pfiMid);

            this->ProcessFunctionBodyOnLoad(currfb, body);
        }
    }

    void ScriptContextTTD::RegisterLoadedScript(Js::FunctionBody* body, uint64 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelScriptLoad.Add(relation);
    }

    void ScriptContextTTD::RegisterNewScript(Js::FunctionBody* body, uint64 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelNewFunction.Add(relation);
    }

    void ScriptContextTTD::RegisterEvalScript(Js::FunctionBody* body, uint64 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelEval.Add(relation);
    }

    Js::FunctionBody* ScriptContextTTD::ResolveParentBody(Js::FunctionBody* body) const
    {
        return this->m_ttdFunctionBodyParentMap.LookupWithKey(body, nullptr);
    }

    Js::FunctionBody* ScriptContextTTD::FindFunctionBodyByFileName(const char16* filename) const
    {
        AssertMsg(filename != nullptr, "We don't want to set breakpoints in non-user code!!!");

        for(auto iter = this->m_ttdPinnedRootFunctionSet->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::FunctionBody* cfb = static_cast<Js::FunctionBody*>(iter.CurrentValue());

            const char16* curi = cfb->GetSourceContextInfo()->url;
            if(curi != nullptr && wcscmp(filename, curi) == 0)
            {
                return cfb;
            }
        }

        AssertMsg(false, "We should never get here!!!");
        return nullptr;
    }

    void ScriptContextTTD::ClearLoadedSourcesForSnapshotRestore()
    {
        this->m_ttdTopLevelScriptLoad.Clear();
        this->m_ttdTopLevelNewFunction.Clear();
        this->m_ttdTopLevelEval.Clear();

        this->m_ttdPinnedRootFunctionSet->Clear();

        this->m_ttdFunctionBodyParentMap.Clear();
    }

    //////////////////

    void RuntimeContextInfo::BuildPathString(UtilSupport::TTAutoString rootpath, const char16* name, const char16* optaccessortag, UtilSupport::TTAutoString& into)
    {
        into.Append(rootpath);
        into.Append(_u("."));
        into.Append(name);

        if(optaccessortag != nullptr)
        {
           into.Append(optaccessortag);
        }
    }

    void RuntimeContextInfo::LoadAndOrderPropertyNames(Js::RecyclableObject* obj, JsUtil::List<const Js::PropertyRecord*, HeapAllocator>& propertyList)
    {
        AssertMsg(propertyList.Count() == 0, "This should be empty.");

        Js::ScriptContext* ctx = obj->GetScriptContext();
        uint32 propcount = (uint32)obj->GetPropertyCount();

        //get all of the properties
        for(uint32 i = 0; i < propcount; ++i)
        {
            Js::PropertyIndex propertyIndex = (Js::PropertyIndex)i;
            Js::PropertyId propertyId = obj->GetPropertyId(propertyIndex);

            if((propertyId != Js::Constants::NoProperty) & (!Js::IsInternalPropertyId(propertyId)))
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
    
    RuntimeContextInfo::RuntimeContextInfo()
        : m_worklist(&HeapAllocator::Instance), m_nullString(),
        m_coreObjToPathMap(&HeapAllocator::Instance, TTD_CORE_OBJECT_COUNT), m_coreBodyToPathMap(&HeapAllocator::Instance, TTD_CORE_FUNCTION_BODY_COUNT), m_coreDbgScopeToPathMap(&HeapAllocator::Instance, TTD_CORE_FUNCTION_BODY_COUNT),
        m_sortedObjectList(&HeapAllocator::Instance, TTD_CORE_OBJECT_COUNT), m_sortedFunctionBodyList(&HeapAllocator::Instance, TTD_CORE_FUNCTION_BODY_COUNT), m_sortedDbgScopeList(&HeapAllocator::Instance, TTD_CORE_FUNCTION_BODY_COUNT)
    {
        ;
    }

    RuntimeContextInfo::~RuntimeContextInfo()
    {
        for(auto iter = this->m_coreObjToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            TT_HEAP_DELETE(UtilSupport::TTAutoString, iter.CurrentValue());
        }

        for(auto iter = this->m_coreBodyToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            TT_HEAP_DELETE(UtilSupport::TTAutoString, iter.CurrentValue());
        }

        for(auto iter = this->m_coreDbgScopeToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            TT_HEAP_DELETE(UtilSupport::TTAutoString, iter.CurrentValue());
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
        const UtilSupport::TTAutoString* res = this->m_coreObjToPathMap.LookupWithKey(obj, nullptr);
        AssertMsg(res != nullptr, "This isn't a well known object!");

        return res->GetStrValue();
    }

    TTD_WELLKNOWN_TOKEN RuntimeContextInfo::ResolvePathForKnownFunctionBody(Js::FunctionBody* fbody) const
    {
        const UtilSupport::TTAutoString* res = this->m_coreBodyToPathMap.LookupWithKey(fbody, nullptr);
        AssertMsg(res != nullptr, "This isn't a well known object!");

        return res->GetStrValue();
    }

    TTD_WELLKNOWN_TOKEN RuntimeContextInfo::ResolvePathForKnownDbgScopeIfExists(Js::DebuggerScope* dbgScope) const
    {
        const UtilSupport::TTAutoString* res = this->m_coreDbgScopeToPathMap.LookupWithKey(dbgScope, nullptr);

        if(res == nullptr)
        {
            return nullptr;
        }

        return res->GetStrValue();
    }

    Js::RecyclableObject* RuntimeContextInfo::LookupKnownObjectFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::RecyclableObject*, true>(pathIdString, this->m_coreObjToPathMap, this->m_sortedObjectList, this->m_nullString);
        AssertMsg(pos != -1, "This isn't a well known object!");

        return this->m_sortedObjectList.Item(pos);
    }

    Js::FunctionBody* RuntimeContextInfo::LookupKnownFunctionBodyFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::FunctionBody*, true>(pathIdString, this->m_coreBodyToPathMap, this->m_sortedFunctionBodyList, this->m_nullString);
        AssertMsg(pos != -1, "Missing function.");

        return (pos != -1) ? this->m_sortedFunctionBodyList.Item(pos) : nullptr;
    }

    Js::DebuggerScope* RuntimeContextInfo::LookupKnownDebuggerScopeFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::DebuggerScope*, true>(pathIdString, this->m_coreDbgScopeToPathMap, this->m_sortedDbgScopeList, this->m_nullString);
        AssertMsg(pos != -1, "Missing debug scope.");

        return (pos != -1) ? this->m_sortedDbgScopeList.Item(pos) : nullptr;
    }

    void RuntimeContextInfo::GatherKnownObjectToPathMap(Js::ScriptContext* ctx)
    {
        JsUtil::List<const Js::PropertyRecord*, HeapAllocator> propertyRecordList(&HeapAllocator::Instance);

        this->EnqueueRootPathObject(_u("global"), ctx->GetGlobalObject());
        this->EnqueueRootPathObject(_u("null"), ctx->GetLibrary()->GetNull());

        this->EnqueueRootPathObject(_u("_defaultAccessor"), ctx->GetLibrary()->GetDefaultAccessorFunction());

        this->EnqueueRootPathObject(_u("_stackTraceAccessor"), ctx->GetLibrary()->GetStackTraceAccessorFunction());
        this->EnqueueRootPathObject(_u("_throwTypeErrorAccessor"), ctx->GetLibrary()->GetThrowTypeErrorAccessorFunction());
        this->EnqueueRootPathObject(_u("_throwTypeErrorCallerAccessor"), ctx->GetLibrary()->GetThrowTypeErrorCallerAccessorFunction());
        this->EnqueueRootPathObject(_u("_throwTypeErrorCalleeAccessor"), ctx->GetLibrary()->GetThrowTypeErrorCalleeAccessorFunction());
        this->EnqueueRootPathObject(_u("_throwTypeErrorArgumentsAccessor"), ctx->GetLibrary()->GetThrowTypeErrorArgumentsAccessorFunction());

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
                        this->EnqueueNewPathVarAsNeeded(curr, getter, precord, _u(">"));
                    }

                    if(setter != nullptr && !Js::JavascriptOperators::IsUndefinedObject(setter))
                    {
                        AssertMsg(Js::JavascriptFunction::Is(setter), "The setter is not a function?");
                        this->EnqueueNewPathVarAsNeeded(curr, Js::RecyclableObject::FromVar(setter), precord, _u("<"));
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
                this->EnqueueNewPathVarAsNeeded(curr, proto, _u("_proto_"));
            }

            curr->ProcessCorePaths();
        }

        SortDictIntoListOnNames<Js::RecyclableObject*>(this->m_coreObjToPathMap, this->m_sortedObjectList, this->m_nullString);
        SortDictIntoListOnNames<Js::FunctionBody*>(this->m_coreBodyToPathMap, this->m_sortedFunctionBodyList, this->m_nullString);
        SortDictIntoListOnNames<Js::DebuggerScope*>(this->m_coreDbgScopeToPathMap, this->m_sortedDbgScopeList, this->m_nullString);
    }

    ////

    void RuntimeContextInfo::EnqueueRootPathObject(const char16* rootName, Js::RecyclableObject* obj)
    {
        this->m_worklist.Enqueue(obj);

        UtilSupport::TTAutoString* rootStr = TT_HEAP_NEW(UtilSupport::TTAutoString, rootName);

        AssertMsg(!this->m_coreObjToPathMap.ContainsKey(obj), "Already in map!!!");
        this->m_coreObjToPathMap.AddNew(obj, rootStr);
    }

    void RuntimeContextInfo::EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const Js::PropertyRecord* prop, const char16* optacessortag)
    {
        this->EnqueueNewPathVarAsNeeded(parent, val, prop->GetBuffer(), optacessortag);
    }

    void RuntimeContextInfo::EnqueueNewPathVarAsNeeded(Js::RecyclableObject* parent, Js::Var val, const char16* propName, const char16* optacessortag)
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
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.LookupWithKey(parent, nullptr);

            this->m_worklist.Enqueue(obj);

            UtilSupport::TTAutoString* tpath = TT_HEAP_NEW(UtilSupport::TTAutoString, *ppath);
            tpath->Append(_u("."));
            tpath->Append(propName);

            if(optacessortag != nullptr)
            {
                tpath->Append(optacessortag);
            }

            AssertMsg(!this->m_coreObjToPathMap.ContainsKey(obj), "Already in map!!!");
            this->m_coreObjToPathMap.AddNew(obj, tpath);
        }
    }

    void RuntimeContextInfo::EnqueueNewFunctionBodyObject(Js::RecyclableObject* parent, Js::FunctionBody* fbody, const char16* name)
    {
        if(!this->m_coreBodyToPathMap.ContainsKey(fbody))
        {
            fbody->EnsureDeserialized();
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.LookupWithKey(parent, nullptr);

            UtilSupport::TTAutoString* fpath = TT_HEAP_NEW(UtilSupport::TTAutoString, *ppath);

            fpath->Append(_u("."));
            fpath->Append(name);

            this->m_coreBodyToPathMap.AddNew(fbody, fpath);
        }
    }

    void RuntimeContextInfo::AddWellKnownDebuggerScopePath(Js::RecyclableObject* parent, Js::DebuggerScope* dbgScope, uint32 index)
    {
        if(!this->m_coreDbgScopeToPathMap.ContainsKey(dbgScope))
        {
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.LookupWithKey(parent, nullptr);

            UtilSupport::TTAutoString* scpath = TT_HEAP_NEW(UtilSupport::TTAutoString, *ppath);

            scpath->Append(_u(".!scope["));
            scpath->Append(index);
            scpath->Append(_u("]"));

            this->m_coreDbgScopeToPathMap.AddNew(dbgScope, scpath);
        }
    }

    void RuntimeContextInfo::BuildArrayIndexBuffer(uint32 arrayidx, UtilSupport::TTAutoString& res)
    {
        res.Append(_u("!arrayContents["));
        res.Append(arrayidx);
        res.Append(_u("]"));
    }

    void RuntimeContextInfo::BuildEnvironmentIndexBuffer(uint32 envidx, UtilSupport::TTAutoString& res)
    {
        res.Append(_u("!env["));
        res.Append(envidx);
        res.Append(_u("]"));
    }

    void RuntimeContextInfo::BuildEnvironmentIndexAndSlotBuffer(uint32 envidx, uint32 slotidx, UtilSupport::TTAutoString& res)
    {
        res.Append(_u("!env["));
        res.Append(envidx);
        res.Append(_u("].!slot["));
        res.Append(slotidx);
        res.Append(_u("]"));
    }
}

#endif