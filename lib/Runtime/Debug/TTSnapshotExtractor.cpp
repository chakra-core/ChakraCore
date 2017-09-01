//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    void SnapshotExtractor::MarkVisitHandler(Js::DynamicTypeHandler* handler)
    {
        this->m_marks.MarkAndTestAddr<MarkTableTag::TypeHandlerTag>(handler);
    }

    void SnapshotExtractor::MarkVisitType(Js::Type* type)
    {
        //Must ensure this is de-serialized before you call this

        if(this->m_marks.MarkAndTestAddr<MarkTableTag::TypeTag>(type))
        {
            if(Js::DynamicType::Is(type->GetTypeId()))
            {
                Js::DynamicTypeHandler* handler = (static_cast<Js::DynamicType*>(type))->GetTypeHandler();

                this->MarkVisitHandler(handler);
            }

            Js::RecyclableObject* proto = type->GetPrototype();
            if(proto != nullptr)
            {
                this->MarkVisitVar(proto);
            }
        }
    }

    void SnapshotExtractor::MarkVisitStandardProperties(Js::RecyclableObject* obj)
    {
        TTDAssert(Js::DynamicType::Is(obj->GetTypeId()) || obj->GetPropertyCount() == 0, "Only dynamic objects should have standard properties.");

        if(Js::DynamicType::Is(obj->GetTypeId()))
        {
            Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(obj);

            dynObj->GetDynamicType()->GetTypeHandler()->MarkObjectSlots_TTD(this, dynObj);

            Js::ArrayObject* parray = dynObj->GetObjectArray();
            if(parray != nullptr)
            {
                this->MarkVisitVar(parray);
            }
        }
    }

    void SnapshotExtractor::ExtractHandlerIfNeeded(Js::DynamicTypeHandler* handler, ThreadContext* threadContext)
    {
        if(this->m_marks.IsMarked(handler))
        {
            NSSnapType::SnapHandler* sHandler = this->m_pendingSnap->GetNextAvailableHandlerEntry();
            handler->ExtractSnapHandler(sHandler, threadContext, this->m_pendingSnap->GetSnapshotSlabAllocator());

            this->m_idToHandlerMap.AddItem(sHandler->HandlerId, sHandler);
            this->m_marks.ClearMark(handler);
        }
    }

    void SnapshotExtractor::ExtractTypeIfNeeded(Js::Type* jstype, ThreadContext* threadContext)
    {
        if(this->m_marks.IsMarked(jstype))
        {
            if(Js::DynamicType::Is(jstype->GetTypeId()))
            {
                this->ExtractHandlerIfNeeded(static_cast<Js::DynamicType*>(jstype)->GetTypeHandler(), threadContext);
            }

            NSSnapType::SnapHandler* sHandler = nullptr;
            if(Js::DynamicType::Is(jstype->GetTypeId()))
            {
                Js::DynamicTypeHandler* dhandler = static_cast<const Js::DynamicType*>(jstype)->GetTypeHandler();

                TTD_PTR_ID handlerId = TTD_CONVERT_TYPEINFO_TO_PTR_ID(dhandler);
                sHandler = this->m_idToHandlerMap.LookupKnownItem(handlerId);
            }

            NSSnapType::SnapType* sType = this->m_pendingSnap->GetNextAvailableTypeEntry();
            jstype->ExtractSnapType(sType, sHandler, this->m_pendingSnap->GetSnapshotSlabAllocator());

            this->m_idToTypeMap.AddItem(sType->TypePtrId, sType);
            this->m_marks.ClearMark(jstype);
        }
    }

    void SnapshotExtractor::ExtractSlotArrayIfNeeded(Js::ScriptContext* ctx, Js::Var* scope)
    {
        if(this->m_marks.IsMarked(scope))
        {
            NSSnapValues::SlotArrayInfo* slotInfo = this->m_pendingSnap->GetNextAvailableSlotArrayEntry();

            Js::ScopeSlots slots(scope);
            slotInfo->SlotId = TTD_CONVERT_VAR_TO_PTR_ID(scope);
            slotInfo->ScriptContextLogId = ctx->ScriptContextLogTag;

            slotInfo->SlotCount = slots.GetCount();
            slotInfo->Slots = this->m_pendingSnap->GetSnapshotSlabAllocator().SlabAllocateArray<TTDVar>(slotInfo->SlotCount);

            for(uint32 j = 0; j < slotInfo->SlotCount; ++j)
            {
                slotInfo->Slots[j] = slots.Get(j);
            }

            if(slots.IsFunctionScopeSlotArray())
            {
                Js::FunctionBody* fb = slots.GetFunctionInfo()->GetFunctionBody();

                slotInfo->isFunctionBodyMetaData = true;
                slotInfo->OptFunctionBodyId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(fb);
                slotInfo->OptDebugScopeId = TTD_INVALID_PTR_ID;
                slotInfo->OptWellKnownDbgScope = TTD_INVALID_WELLKNOWN_TOKEN;

                Js::PropertyId* propertyIds = fb->GetPropertyIdsForScopeSlotArray();
                slotInfo->PIDArray = this->m_pendingSnap->GetSnapshotSlabAllocator().SlabAllocateArray<Js::PropertyId>(slotInfo->SlotCount);
                js_memcpy_s(slotInfo->PIDArray, sizeof(Js::PropertyId) * slotInfo->SlotCount, propertyIds, sizeof(Js::PropertyId) * slots.GetCount());
            }
            else
            {
                Js::DebuggerScope* dbgScope = slots.GetDebuggerScope();
                slotInfo->isFunctionBodyMetaData = false;
                slotInfo->OptFunctionBodyId = TTD_INVALID_PTR_ID;

                TTD_WELLKNOWN_TOKEN wellKnownToken = ctx->TTDWellKnownInfo->ResolvePathForKnownDbgScopeIfExists(dbgScope);
                if(wellKnownToken == TTD_INVALID_WELLKNOWN_TOKEN)
                {
                    slotInfo->OptDebugScopeId = TTD_CONVERT_DEBUGSCOPE_TO_PTR_ID(dbgScope);
                    slotInfo->OptWellKnownDbgScope = TTD_INVALID_WELLKNOWN_TOKEN;
                }
                else
                {
                    slotInfo->OptDebugScopeId = TTD_INVALID_PTR_ID;
                    slotInfo->OptWellKnownDbgScope = this->m_pendingSnap->GetSnapshotSlabAllocator().CopyRawNullTerminatedStringInto(wellKnownToken);
                }

                slotInfo->PIDArray = this->m_pendingSnap->GetSnapshotSlabAllocator().SlabAllocateArray<Js::PropertyId>(slotInfo->SlotCount);

                for(uint32 j = 0; j < slotInfo->SlotCount; ++j)
                {
                    slotInfo->PIDArray[j] = dbgScope->GetPropertyIdForSlotIndex_TTD(j);
                }
            }

            this->m_marks.ClearMark(scope);
        }
    }

    void SnapshotExtractor::ExtractScopeIfNeeded(Js::ScriptContext* ctx, Js::FrameDisplay* environment)
    {
        if(this->m_marks.IsMarked(environment))
        {
            TTDAssert(environment->GetLength() > 0, "This doesn't make sense");

            NSSnapValues::ScriptFunctionScopeInfo* funcScopeInfo = this->m_pendingSnap->GetNextAvailableFunctionScopeEntry();
            funcScopeInfo->ScopeId = TTD_CONVERT_ENV_TO_PTR_ID(environment);
            funcScopeInfo->ScriptContextLogId = ctx->ScriptContextLogTag;

            funcScopeInfo->ScopeCount = environment->GetLength();
            funcScopeInfo->ScopeArray = this->m_pendingSnap->GetSnapshotSlabAllocator().SlabAllocateArray<NSSnapValues::ScopeInfoEntry>(funcScopeInfo->ScopeCount);

            for(uint16 i = 0; i < funcScopeInfo->ScopeCount; ++i)
            {
                void* scope = environment->GetItem(i);
                NSSnapValues::ScopeInfoEntry* entryInfo = (funcScopeInfo->ScopeArray + i);

                entryInfo->Tag = environment->GetScopeType(scope);
                switch(entryInfo->Tag)
                {
                case Js::ScopeType::ScopeType_ActivationObject:
                case Js::ScopeType::ScopeType_WithScope:
                    entryInfo->IDValue = TTD_CONVERT_VAR_TO_PTR_ID((Js::Var)scope);
                    break;
                case Js::ScopeType::ScopeType_SlotArray:
                {
                    this->ExtractSlotArrayIfNeeded(ctx, (Js::Var*)scope);

                    entryInfo->IDValue = TTD_CONVERT_SLOTARRAY_TO_PTR_ID((Js::Var*)scope);
                    break;
                }
                default:
                    TTDAssert(false, "Unknown scope kind");
                    entryInfo->IDValue = TTD_INVALID_PTR_ID;
                    break;
                }
            }

            this->m_marks.ClearMark(environment);
        }
    }

    void SnapshotExtractor::ExtractScriptFunctionEnvironmentIfNeeded(Js::ScriptFunction* function)
    {
        Js::FrameDisplay* environment = function->GetEnvironment();
        if(environment->GetLength() != 0)
        {
            this->ExtractScopeIfNeeded(function->GetScriptContext(), environment);
        }
    }

    void SnapshotExtractor::UnloadDataFromExtractor()
    {
        this->m_marks.Clear();
        this->m_worklist.Clear();

        this->m_idToHandlerMap.Unload();
        this->m_idToTypeMap.Unload();

        this->m_pendingSnap = nullptr;
    }

    SnapshotExtractor::SnapshotExtractor()
        : m_marks(), m_worklist(&HeapAllocator::Instance),
        m_idToHandlerMap(), m_idToTypeMap(),
        m_pendingSnap(nullptr),
        m_snapshotsTakenCount(0),
        m_totalMarkMillis(0.0), m_totalExtractMillis(0.0),
        m_maxMarkMillis(0.0), m_maxExtractMillis(0.0),
        m_lastMarkMillis(0.0), m_lastExtractMillis(0.0)
    {
        ;
    }

    SnapshotExtractor::~SnapshotExtractor()
    {
        this->UnloadDataFromExtractor();
    }

    SnapShot* SnapshotExtractor::GetPendingSnapshot()
    {
        TTDAssert(this->m_pendingSnap != nullptr, "Should only call if we are extracting a snapshot");

        return this->m_pendingSnap;
    }

    SlabAllocator& SnapshotExtractor::GetActiveSnapshotSlabAllocator()
    {
        TTDAssert(this->m_pendingSnap != nullptr, "Should only call if we are extracting a snapshot");

        return this->m_pendingSnap->GetSnapshotSlabAllocator();
    }

    void SnapshotExtractor::MarkVisitVar(Js::Var var)
    {
        TTDAssert(var != nullptr, "I don't think this should happen but not 100% sure.");
        TTDAssert(Js::JavascriptOperators::GetTypeId(var) < Js::TypeIds_Limit || Js::RecyclableObject::FromVar(var)->CanHaveInterceptors(), "Not cool.");

        //We don't need to visit tagged things
        if(JsSupport::IsVarTaggedInline(var))
        {
            return;
        }

        if(JsSupport::IsVarPrimitiveKind(var))
        {
            if(this->m_marks.MarkAndTestAddr<MarkTableTag::PrimitiveObjectTag>(var))
            {
                Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(var);
                this->MarkVisitType(obj->GetType());
            }
        }
        else
        {
            TTDAssert(JsSupport::IsVarComplexKind(var), "Shouldn't be anything else");

            if(this->m_marks.MarkAndTestAddr<MarkTableTag::CompoundObjectTag>(var))
            {
                Js::RecyclableObject* obj = Js::RecyclableObject::FromVar(var);

                //do this here instead of in mark visit type as it wants the dynamic object as well
                if(Js::DynamicType::Is(obj->GetTypeId()))
                {
                    Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(obj);
                    if(dynObj->GetDynamicType()->GetTypeHandler()->IsDeferredTypeHandler())
                    {
                        dynObj->GetDynamicType()->GetTypeHandler()->EnsureObjectReady(dynObj);
                    }
                }
                this->MarkVisitType(obj->GetType());

                this->m_worklist.Enqueue(obj);
            }
        }
    }

    void SnapshotExtractor::MarkFunctionBody(Js::FunctionBody* fb)
    {
        if(this->m_marks.MarkAndTestAddr<MarkTableTag::FunctionBodyTag>(fb))
        {
            Js::FunctionBody* currfb = fb->GetScriptContext()->TTDContextInfo->ResolveParentBody(fb);

            while(currfb != nullptr && this->m_marks.MarkAndTestAddr<MarkTableTag::FunctionBodyTag>(currfb))
            {
                currfb = currfb->GetScriptContext()->TTDContextInfo->ResolveParentBody(currfb);
            }
        }
    }

    void SnapshotExtractor::MarkScriptFunctionScopeInfo(Js::FrameDisplay* environment)
    {
        if(this->m_marks.MarkAndTestAddr<MarkTableTag::EnvironmentTag>(environment))
        {
            uint32 scopeCount = environment->GetLength();

            for(uint32 i = 0; i < scopeCount; ++i)
            {
                void* scope = environment->GetItem(i);

                switch(environment->GetScopeType(scope))
                {
                case Js::ScopeType::ScopeType_ActivationObject:
                case Js::ScopeType::ScopeType_WithScope:
                {
                    this->MarkVisitVar((Js::Var)scope);
                    break;
                }
                case Js::ScopeType::ScopeType_SlotArray:
                {
                    if(this->m_marks.MarkAndTestAddr<MarkTableTag::SlotArrayTag>(scope))
                    {
                        Js::ScopeSlots slotArray = (Js::Var*)scope;
                        uint slotArrayCount = slotArray.GetCount();

                        if(slotArray.IsFunctionScopeSlotArray())
                        {
                            this->MarkFunctionBody(slotArray.GetFunctionInfo()->GetFunctionBody());
                        }

                        for(uint j = 0; j < slotArrayCount; j++)
                        {
                            Js::Var sval = slotArray.Get(j);
                            this->MarkVisitVar(sval);
                        }
                    }
                    break;
                }
                default:
                    TTDAssert(false, "Unknown scope kind");
                }
            }
        }
    }

    void SnapshotExtractor::BeginSnapshot(ThreadContext* threadContext, double gcTime)
    {
        TTDAssert((this->m_pendingSnap == nullptr) & this->m_worklist.Empty(), "Something went wrong.");

        this->m_pendingSnap = TT_HEAP_NEW(SnapShot, gcTime);
    }

    void SnapshotExtractor::DoMarkWalk(ThreadContext* threadContext)
    {
        TTDTimer timer;
        double startTime = timer.Now();

        //Add the global roots
        for(auto iter = threadContext->TTDContext->GetRootTagToObjectMap().GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::Var root = iter.CurrentValue();
            this->MarkVisitVar(root);
        }

        while(!this->m_worklist.Empty())
        {
            Js::RecyclableObject* nobj = this->m_worklist.Dequeue();
            TTDAssert(JsSupport::IsVarComplexKind(nobj), "Should only be these two options");

            this->MarkVisitStandardProperties(nobj);
            nobj->MarkVisitKindSpecificPtrs(this);
        }

        //Mark all of the well known objects/types
        for(int32 i = 0; i < threadContext->TTDContext->GetTTDContexts().Count(); ++i)
        {
            threadContext->TTDContext->GetTTDContexts().Item(i)->TTDWellKnownInfo->MarkWellKnownObjects_TTD(this->m_marks);
        }

        double endTime = timer.Now();
        this->m_pendingSnap->MarkTime = (endTime - startTime) / 1000.0;
    }

    void SnapshotExtractor::EvacuateMarkedIntoSnapshot(ThreadContext* threadContext, JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies)
    {
        TTDTimer timer;
        double startTime = timer.Now();

        SnapShot* snap = this->m_pendingSnap;
        SlabAllocator& alloc = this->m_pendingSnap->GetSnapshotSlabAllocator();

        //invert the root map for extracting
        JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator> objToLogIdMap(&HeapAllocator::Instance);
        threadContext->TTDContext->LoadInvertedRootMap(objToLogIdMap);

        //We extract all the global code function bodies with the context so clear their marks now
        for(int32 i = 0; i < threadContext->TTDContext->GetTTDContexts().Count(); ++i)
        {
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelScriptLoad(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelNewFunction(&HeapAllocator::Instance);
            JsUtil::List<TopLevelFunctionInContextRelation, HeapAllocator> topLevelEval(&HeapAllocator::Instance);

            Js::ScriptContext* ctx = threadContext->TTDContext->GetTTDContexts().Item(i);
            ctx->TTDContextInfo->GetLoadedSources(nullptr, topLevelScriptLoad, topLevelNewFunction, topLevelEval);

            for(int32 j = 0; j < topLevelScriptLoad.Count(); ++j)
            {
                Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(topLevelScriptLoad.Item(j).ContextSpecificBodyPtrId);
                if(this->m_marks.IsMarked(body))
                {
                    liveTopLevelBodies.Add(body);
                    this->m_marks.ClearMark(body);
                }
            }

            for(int32 j = 0; j < topLevelNewFunction.Count(); ++j)
            {
                Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(topLevelNewFunction.Item(j).ContextSpecificBodyPtrId);
                if(this->m_marks.IsMarked(body))
                {
                    liveTopLevelBodies.Add(body);
                    this->m_marks.ClearMark(body);
                }
            }

            for(int32 j = 0; j < topLevelEval.Count(); ++j)
            {
                Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(topLevelEval.Item(j).ContextSpecificBodyPtrId);
                if(this->m_marks.IsMarked(body))
                {
                    liveTopLevelBodies.Add(body);
                    this->m_marks.ClearMark(body);
                }
            }
        }

        UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& snpCtxs = this->m_pendingSnap->GetContextList();
        for(int32 i = 0; i < threadContext->TTDContext->GetTTDContexts().Count(); ++i)
        {
            NSSnapValues::SnapContext* snpCtx = snpCtxs.NextOpenEntry();
            NSSnapValues::ExtractScriptContext(snpCtx, threadContext->TTDContext->GetTTDContexts().Item(i), objToLogIdMap, liveTopLevelBodies, snap->GetSnapshotSlabAllocator());
        }

        //extract the thread context symbol map info
        JsUtil::BaseDictionary<Js::HashedCharacterBuffer<char16>*, const Js::PropertyRecord*, Recycler, PowerOf2SizePolicy, Js::PropertyRecordStringHashComparer>* tcSymbolRegistrationMap = threadContext->GetSymbolRegistrationMap_TTD();
        UnorderedArrayList<Js::PropertyId, TTD_ARRAY_LIST_SIZE_XSMALL>& tcSymbolMapInfo = this->m_pendingSnap->GetTCSymbolMapInfoList();
        for(auto iter = tcSymbolRegistrationMap->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::PropertyId* tcpid = tcSymbolMapInfo.NextOpenEntry();
            *tcpid = iter.CurrentValue()->GetPropertyId();
        }

        this->m_idToHandlerMap.Initialize(this->m_marks.GetCountForTag<MarkTableTag::TypeHandlerTag>());
        this->m_idToTypeMap.Initialize(this->m_marks.GetCountForTag<MarkTableTag::TypeTag>());

        //walk all the marked objects
        this->m_marks.InitializeIter();
        MarkTableTag tag = this->m_marks.GetTagValue();
        while(tag != MarkTableTag::Clear)
        {
            switch(tag & MarkTableTag::AllKindMask)
            {
            case MarkTableTag::TypeHandlerTag:
                this->ExtractHandlerIfNeeded(this->m_marks.GetPtrValue<Js::DynamicTypeHandler*>(), threadContext);
                break;
            case MarkTableTag::TypeTag:
                this->ExtractTypeIfNeeded(this->m_marks.GetPtrValue<Js::Type*>(), threadContext);
                break;
            case MarkTableTag::PrimitiveObjectTag:
            {
                this->ExtractTypeIfNeeded(this->m_marks.GetPtrValue<Js::RecyclableObject*>()->GetType(), threadContext);
                NSSnapValues::ExtractSnapPrimitiveValue(snap->GetNextAvailablePrimitiveObjectEntry(), this->m_marks.GetPtrValue<Js::RecyclableObject*>(), this->m_marks.GetTagValueIsWellKnown(), this->m_idToTypeMap, alloc);
                break;
            }
            case MarkTableTag::CompoundObjectTag:
            {
                this->ExtractTypeIfNeeded(this->m_marks.GetPtrValue<Js::RecyclableObject*>()->GetType(), threadContext);
                if(Js::ScriptFunction::Is(this->m_marks.GetPtrValue<Js::RecyclableObject*>()))
                {
                    this->ExtractScriptFunctionEnvironmentIfNeeded(this->m_marks.GetPtrValue<Js::ScriptFunction*>());
                }
                NSSnapObjects::ExtractCompoundObject(snap->GetNextAvailableCompoundObjectEntry(), this->m_marks.GetPtrValue<Js::RecyclableObject*>(), this->m_marks.GetTagValueIsWellKnown(), this->m_idToTypeMap, alloc);
                break;
            }
            case MarkTableTag::FunctionBodyTag:
                NSSnapValues::ExtractFunctionBodyInfo(snap->GetNextAvailableFunctionBodyResolveInfoEntry(), this->m_marks.GetPtrValue<Js::FunctionBody*>(), this->m_marks.GetTagValueIsWellKnown(), alloc);
                break;
            case MarkTableTag::EnvironmentTag:
            case MarkTableTag::SlotArrayTag:
                break; //should be handled with the associated script function
            default:
                TTDAssert(false, "If this isn't true then we have an unknown tag");
                break;
            }

            this->m_marks.MoveToNextAddress();
            tag = this->m_marks.GetTagValue();
        }

        //Extract the roots
        ThreadContextTTD* txctx = threadContext->TTDContext;
        UnorderedArrayList<NSSnapValues::SnapRootInfoEntry, TTD_ARRAY_LIST_SIZE_MID>& rootlist = this->m_pendingSnap->GetRootList();
        for(auto iter = threadContext->TTDContext->GetRootTagToObjectMap().GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            NSSnapValues::SnapRootInfoEntry* spe = rootlist.NextOpenEntry();
            spe->LogObject = TTD_CONVERT_VAR_TO_PTR_ID(iter.CurrentValue());
            spe->LogId = iter.CurrentKey();
            spe->MaybeLongLivedRoot = txctx->ResolveIsLongLivedForExtract(spe->LogId);
        }

        if(threadContext->TTDContext->GetActiveScriptContext() == nullptr)
        {
            this->m_pendingSnap->SetActiveScriptContext(TTD_INVALID_LOG_PTR_ID);
        }
        else
        {
            TTD_LOG_PTR_ID ctxId = threadContext->TTDContext->GetActiveScriptContext()->ScriptContextLogTag;
            this->m_pendingSnap->SetActiveScriptContext(ctxId);
        }

        double endTime = timer.Now();
        snap->ExtractTime = (endTime - startTime) / 1000.0;
    }

    SnapShot* SnapshotExtractor::CompleteSnapshot()
    {
        SnapShot* snap = this->m_pendingSnap;
        this->UnloadDataFromExtractor();

        this->m_snapshotsTakenCount++;
        this->m_totalMarkMillis += snap->MarkTime;
        this->m_totalExtractMillis += snap->ExtractTime;

        if(this->m_maxMarkMillis < snap->MarkTime)
        {
            this->m_maxMarkMillis = snap->MarkTime;
        }

        if(this->m_maxExtractMillis < snap->ExtractTime)
        {
            this->m_maxExtractMillis = snap->ExtractTime;
        }

        this->m_lastMarkMillis = snap->MarkTime;
        this->m_lastExtractMillis = snap->ExtractTime;

        return snap;
    }

    void SnapshotExtractor::DoResetWeakCollectionPinSet(ThreadContext* threadContext)
    {
        //Add the roots
        for(auto iter = threadContext->TTDContext->GetRootTagToObjectMap().GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::Var root = iter.CurrentValue();
            this->MarkVisitVar(root);
        }

        while(!this->m_worklist.Empty())
        {
            Js::RecyclableObject* nobj = this->m_worklist.Dequeue();
            TTDAssert(JsSupport::IsVarComplexKind(nobj), "Should only be these two options");

            this->MarkVisitStandardProperties(nobj);
            nobj->MarkVisitKindSpecificPtrs(this);
        }

        this->UnloadDataFromExtractor();
    }
}

#endif
