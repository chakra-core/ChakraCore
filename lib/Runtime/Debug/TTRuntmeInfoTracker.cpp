//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    void ThreadContextTTD::AddNewScriptContext_Helper(Js::ScriptContext* ctx, HostScriptContextCallbackFunctor& callbackFunctor, bool noNative, bool debugMode)
    {
        ////
        //First just setup the standard things needed for a script context
        ctx->TTDHostCallbackFunctor = callbackFunctor;
        if(noNative)
        {
            ctx->ForceNoNative();
        }

        if(debugMode)
        {
#ifdef _WIN32
            ctx->InitializeDebugging();
#else
            //
            //TODO: x-plat does not like some parts of initiallize debugging so just set the flag we need 
            //
            ctx->GetDebugContext()->SetDebuggerMode(Js::DebuggerMode::Debugging);
#endif
        }

        ctx->InitializeCoreImage_TTD();

        TTDAssert(!this->m_contextList.Contains(ctx), "We should only be adding at creation time!!!");
        this->m_contextList.Add(ctx);
    }

    void ThreadContextTTD::CleanRecordWeakRootMap()
    {
        this->m_ttdRecordRootWeakMap->Map([&](Js::RecyclableObject* key, bool, const RecyclerWeakReference<Js::RecyclableObject>*)
        {
            ; //nop just map to force the clean
        });
    }

    ThreadContextTTD::ThreadContextTTD(ThreadContext* threadContext, void* runtimeHandle, uint32 snapInterval, uint32 snapHistoryLength)
        : m_threadCtx(threadContext), m_runtimeHandle(runtimeHandle), m_contextCreatedOrDestoyedInReplay(false),
        SnapInterval(snapInterval), SnapHistoryLength(snapHistoryLength),
        m_activeContext(nullptr), m_contextList(&HeapAllocator::Instance), m_ttdContextToExternalRefMap(&HeapAllocator::Instance),
        m_ttdRootTagToObjectMap(&HeapAllocator::Instance), m_ttdMayBeLongLivedRoot(&HeapAllocator::Instance), 
        m_ttdRecordRootWeakMap(),m_ttdReplayRootPinSet(),
        TTDataIOInfo({ 0 }), TTDExternalObjectFunctions({ 0 })
    {
        Recycler* tctxRecycler = this->m_threadCtx->GetRecycler();

        this->m_ttdRecordRootWeakMap.Root(RecyclerNew(tctxRecycler, RecordRootMap, tctxRecycler), tctxRecycler);
        this->m_ttdReplayRootPinSet.Root(RecyclerNew(tctxRecycler, ObjectPinSet, tctxRecycler), tctxRecycler);
    }

    ThreadContextTTD::~ThreadContextTTD()
    {
        for(auto iter = this->m_ttdContextToExternalRefMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            this->m_threadCtx->GetRecycler()->RootRelease(iter.CurrentValue());
        }
        this->m_ttdContextToExternalRefMap.Clear();

        this->m_activeContext = nullptr;
        this->m_contextList.Clear();

        this->m_ttdRootTagToObjectMap.Clear();
        this->m_ttdMayBeLongLivedRoot.Clear();

        if(this->m_ttdRecordRootWeakMap != nullptr)
        {
            this->m_ttdRecordRootWeakMap.Unroot(this->m_threadCtx->GetRecycler());
        }

        if(this->m_ttdReplayRootPinSet != nullptr)
        {
            this->m_ttdReplayRootPinSet.Unroot(this->m_threadCtx->GetRecycler());
        }
    }

    ThreadContext* ThreadContextTTD::GetThreadContext()
    {
        return this->m_threadCtx;
    }

    void* ThreadContextTTD::GetRuntimeHandle()
    {
        return this->m_runtimeHandle;
    }

    FinalizableObject* ThreadContextTTD::GetRuntimeContextForScriptContext(Js::ScriptContext* ctx)
    {
        return this->m_ttdContextToExternalRefMap.Lookup(ctx, nullptr);
    }

    bool ThreadContextTTD::ContextCreatedOrDestoyedInReplay() const
    {
        return this->m_contextCreatedOrDestoyedInReplay;
    }

    void ThreadContextTTD::ResetContextCreatedOrDestoyedInReplay()
    {
        this->m_contextCreatedOrDestoyedInReplay = false;
    }

    const JsUtil::List<Js::ScriptContext*, HeapAllocator>& ThreadContextTTD::GetTTDContexts() const
    {
        return this->m_contextList;
    }

    void ThreadContextTTD::AddNewScriptContextRecord(FinalizableObject* externalCtx, Js::ScriptContext* ctx, HostScriptContextCallbackFunctor& callbackFunctor, bool noNative, bool debugMode)
    {
        this->AddNewScriptContext_Helper(ctx, callbackFunctor, noNative, debugMode);

        this->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetGlobalObject()), ctx->GetGlobalObject());
        ctx->ScriptContextLogTag = TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetGlobalObject());

        this->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetLibrary()->GetUndefined()), ctx->GetLibrary()->GetUndefined());
        this->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetLibrary()->GetNull()), ctx->GetLibrary()->GetNull());
        this->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetLibrary()->GetTrue()), ctx->GetLibrary()->GetTrue());
        this->AddRootRef_Record(TTD_CONVERT_OBJ_TO_LOG_PTR_ID(ctx->GetLibrary()->GetFalse()), ctx->GetLibrary()->GetFalse());
    }

    void ThreadContextTTD::AddNewScriptContextReplay(FinalizableObject* externalCtx, Js::ScriptContext* ctx, HostScriptContextCallbackFunctor& callbackFunctor, bool noNative, bool debugMode)
    {
        this->AddNewScriptContext_Helper(ctx, callbackFunctor, noNative, debugMode);

        this->m_contextCreatedOrDestoyedInReplay = true;

        this->m_threadCtx->GetRecycler()->RootAddRef(externalCtx);
        this->m_ttdContextToExternalRefMap.Add(ctx, externalCtx);
    }

    void ThreadContextTTD::SetActiveScriptContext(Js::ScriptContext* ctx)
    {
        TTDAssert(ctx == nullptr || this->m_contextList.Contains(ctx), "Missing value!!!");

        this->m_activeContext = ctx;
    }

    Js::ScriptContext* ThreadContextTTD::GetActiveScriptContext()
    {
        return this->m_activeContext;
    }

    void ThreadContextTTD::NotifyCtxDestroyInRecord(Js::ScriptContext* ctx)
    {
        if(this->m_contextList.Contains(ctx))
        {
            this->m_contextList.Remove(ctx);
        }
    }

    void ThreadContextTTD::ClearContextsForSnapRestore(JsUtil::List<FinalizableObject*, HeapAllocator>& deadCtxs)
    {
        for(int32 i = 0; i < this->m_contextList.Count(); ++i)
        {
            Js::ScriptContext* ctx = this->m_contextList.Item(i);
            FinalizableObject* externalCtx = this->m_ttdContextToExternalRefMap.Item(ctx);

            deadCtxs.Add(externalCtx);
        }
        this->m_ttdContextToExternalRefMap.Clear();
        this->m_contextList.Clear();

        this->m_activeContext = nullptr;
    }

    void ThreadContextTTD::ClearLocalRootsAndRefreshMap_Replay()
    {
        JsUtil::BaseHashSet<Js::RecyclableObject*, HeapAllocator> image(&HeapAllocator::Instance);

        this->m_ttdRootTagToObjectMap.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_LOG_PTR_ID, Js::RecyclableObject*>& entry) -> bool
        {
            bool localonly = !this->m_ttdMayBeLongLivedRoot.LookupWithKey(entry.Key(), false);

            if(!localonly)
            {
                image.AddNew(entry.Value());
            }

            return localonly;
        });

        this->m_ttdReplayRootPinSet->MapAndRemoveIf([&](Js::RecyclableObject* value) -> bool
        {
            return !image.Contains(value);
        });
    }

    void ThreadContextTTD::LoadInvertedRootMap(JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator>& objToLogIdMap) const
    {
        for(auto iter = this->m_ttdRootTagToObjectMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            objToLogIdMap.AddNew(iter.CurrentValue(), iter.CurrentKey());
        }
    }

    Js::RecyclableObject* ThreadContextTTD::LookupObjectForLogID(TTD_LOG_PTR_ID origId)
    {
        //Local root always has mappings for all the ids
        return this->m_ttdRootTagToObjectMap.Item(origId);
    }

    void ThreadContextTTD::ClearRootsForSnapRestore()
    {
        this->m_ttdRootTagToObjectMap.Clear();
        this->m_ttdMayBeLongLivedRoot.Clear();

        this->m_ttdReplayRootPinSet->Clear();
    }

    void ThreadContextTTD::ForceSetRootInfoInRestore(TTD_LOG_PTR_ID logid, Js::RecyclableObject* obj, bool maybeLongLived)
    {
        this->m_ttdRootTagToObjectMap.Item(logid, obj);
        if(maybeLongLived)
        {
            this->m_ttdMayBeLongLivedRoot.Item(logid, maybeLongLived);
        }

        this->m_ttdReplayRootPinSet->AddNew(obj);
    }

    void ThreadContextTTD::SyncRootsBeforeSnapshot_Record()
    {
        this->CleanRecordWeakRootMap();

        this->m_ttdRootTagToObjectMap.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_LOG_PTR_ID, Js::RecyclableObject*>& entry) -> bool
        {
            return !this->m_ttdRecordRootWeakMap->Lookup(entry.Value(), false);
        });

        this->m_ttdMayBeLongLivedRoot.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_LOG_PTR_ID, bool>& entry) -> bool
        {
            return !this->m_ttdRootTagToObjectMap.ContainsKey(entry.Key());
        });
    }

    void ThreadContextTTD::SyncCtxtsAndRootsWithSnapshot_Replay(uint32 liveContextCount, TTD_LOG_PTR_ID* liveContextIdArray, uint32 liveRootCount, TTD_LOG_PTR_ID* liveRootIdArray)
    {
        //First sync up the context list -- releasing any contexts that are not also there in our initial recording
        JsUtil::BaseHashSet<TTD_LOG_PTR_ID, HeapAllocator> lctxSet(&HeapAllocator::Instance);
        for(uint32 i = 0; i < liveContextCount; ++i)
        {
            lctxSet.AddNew(liveContextIdArray[i]);
        }

        int32 ctxpos = 0;
        while(ctxpos < this->m_contextList.Count())
        {
            Js::ScriptContext* ctx = this->m_contextList.Item(ctxpos);

            if(lctxSet.Contains(ctx->ScriptContextLogTag))
            {
                ctxpos++;
            }
            else
            {
                this->m_contextCreatedOrDestoyedInReplay = true;

                this->m_contextList.Remove(ctx);

                FinalizableObject* externalCtx = this->m_ttdContextToExternalRefMap.Item(ctx);
                this->m_ttdContextToExternalRefMap.Remove(ctx);

                this->m_threadCtx->GetRecycler()->RootRelease(externalCtx);

                //no inc of ctxpos since list go shorter!!!
            }
        }

        //Now sync up the root list wrt. long lived roots that we recorded 
        JsUtil::BaseHashSet<TTD_LOG_PTR_ID, HeapAllocator> refInfoMap(&HeapAllocator::Instance);
        for(uint32 i = 0; i < liveRootCount; ++i)
        {
            refInfoMap.AddNew(liveRootIdArray[i]);
        }

        this->m_ttdMayBeLongLivedRoot.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_LOG_PTR_ID, bool>& entry) -> bool
        {
            return !refInfoMap.Contains(entry.Key());
        });

        this->ClearLocalRootsAndRefreshMap_Replay();
    }

    Js::ScriptContext* ThreadContextTTD::LookupContextForScriptId(TTD_LOG_PTR_ID ctxId) const
    {
        for(int i = 0; i < this->m_contextList.Count(); ++i)
        {
            if(this->m_contextList.Item(i)->ScriptContextLogTag == ctxId)
            {
                return this->m_contextList.Item(i);
            }
        }

        return nullptr;
    }

    ScriptContextTTD::ScriptContextTTD(Js::ScriptContext* ctx)
        : m_ctx(ctx),
        m_ttdPendingAsyncModList(&HeapAllocator::Instance),
        m_ttdTopLevelScriptLoad(&HeapAllocator::Instance), m_ttdTopLevelNewFunction(&HeapAllocator::Instance), m_ttdTopLevelEval(&HeapAllocator::Instance),
        m_ttdFunctionBodyParentMap(&HeapAllocator::Instance)
    {
        Recycler* ctxRecycler = this->m_ctx->GetRecycler();

        this->m_ttdPinnedRootFunctionSet.Root(RecyclerNew(ctxRecycler, TTD::FunctionBodyPinSet, ctxRecycler), ctxRecycler);
        this->TTDWeakReferencePinSet.Root(RecyclerNew(ctxRecycler, TTD::ObjectPinSet, ctxRecycler), ctxRecycler);
    }

    ScriptContextTTD::~ScriptContextTTD()
    {
        this->m_ttdPendingAsyncModList.Clear();

        this->m_ttdTopLevelScriptLoad.Clear();
        this->m_ttdTopLevelNewFunction.Clear();
        this->m_ttdTopLevelEval.Clear();

        if(this->m_ttdPinnedRootFunctionSet != nullptr)
        {
            this->m_ttdPinnedRootFunctionSet.Unroot(this->m_ttdPinnedRootFunctionSet->GetAllocator());
        }

        this->m_ttdFunctionBodyParentMap.Clear();

        if(this->TTDWeakReferencePinSet != nullptr)
        {
            this->TTDWeakReferencePinSet.Unroot(this->TTDWeakReferencePinSet->GetAllocator());
        }
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
            TTDAssert(finalModPos != currentBegin, "We have something strange!!!");
            if(currentBegin == nullptr || finalModPos < currentBegin)
            {
                currentBegin = pbuffBegin;
                pos = (int32)i;
            }
        }
        TTDAssert(pos != -1, "Missing matching register!!!");

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

    void ScriptContextTTD::GetLoadedSources(const JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>* onlyLiveTopLevelBodies, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelScriptLoad, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelNewFunction, JsUtil::List<TTD::TopLevelFunctionInContextRelation, HeapAllocator>& topLevelEval)
    {
        TTDAssert(topLevelScriptLoad.Count() == 0 && topLevelNewFunction.Count() == 0 && topLevelEval.Count() == 0, "Should be empty when you call this.");

        for(auto iter = this->m_ttdTopLevelScriptLoad.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            if(onlyLiveTopLevelBodies == nullptr || onlyLiveTopLevelBodies->Contains(body))
            {
                topLevelScriptLoad.Add(iter.CurrentValue());
            }
        }

        for(auto iter = this->m_ttdTopLevelNewFunction.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            if(onlyLiveTopLevelBodies == nullptr || onlyLiveTopLevelBodies->Contains(body))
            {
                topLevelNewFunction.Add(iter.CurrentValue());
            }
        }

        for(auto iter = this->m_ttdTopLevelEval.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            if(onlyLiveTopLevelBodies == nullptr || onlyLiveTopLevelBodies->Contains(body))
            {
                topLevelEval.Add(iter.CurrentValue());
            }
        }
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
            TTDAssert(!this->m_ttdPinnedRootFunctionSet->Contains(body), "We already added this function!!!");
            this->m_ttdPinnedRootFunctionSet->AddNew(body);
        }

        this->m_ttdFunctionBodyParentMap.AddNew(body, parent);

        for(uint32 i = 0; i < body->GetNestedCount(); ++i)
        {
            Js::ParseableFunctionInfo* pfiMid = body->GetNestedFunctionForExecution(i);
            Js::FunctionBody* currfb = TTD::JsSupport::ForceAndGetFunctionBody(pfiMid);

            this->ProcessFunctionBodyOnLoad(currfb, body);
        }
    }

    void ScriptContextTTD::ProcessFunctionBodyOnUnLoad(Js::FunctionBody* body, Js::FunctionBody* parent)
    {
        //if this is a root (parent is null) then put this in the rootbody pin set so it isn't reclaimed on us
        if(parent == nullptr)
        {
            TTDAssert(this->m_ttdPinnedRootFunctionSet->Contains(body), "We already added this function!!!");
            this->m_ttdPinnedRootFunctionSet->Remove(body);
        }

        this->m_ttdFunctionBodyParentMap.Remove(body);

        for(uint32 i = 0; i < body->GetNestedCount(); ++i)
        {
            Js::ParseableFunctionInfo* pfiMid = body->GetNestedFunctionForExecution(i);
            Js::FunctionBody* currfb = TTD::JsSupport::ForceAndGetFunctionBody(pfiMid);

            this->ProcessFunctionBodyOnUnLoad(currfb, body);
        }
    }

    void ScriptContextTTD::RegisterLoadedScript(Js::FunctionBody* body, uint32 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelScriptLoad.Add(relation.ContextSpecificBodyPtrId, relation);
    }

    void ScriptContextTTD::RegisterNewScript(Js::FunctionBody* body, uint32 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelNewFunction.Add(relation.ContextSpecificBodyPtrId, relation);
    }

    void ScriptContextTTD::RegisterEvalScript(Js::FunctionBody* body, uint32 bodyCtrId)
    {
        TTD::TopLevelFunctionInContextRelation relation;
        relation.TopLevelBodyCtr = bodyCtrId;
        relation.ContextSpecificBodyPtrId = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(body);

        this->m_ttdTopLevelEval.Add(relation.ContextSpecificBodyPtrId, relation);
    }

    void ScriptContextTTD::CleanUnreachableTopLevelBodies(const JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies)
    {
        //don't clear top-level loaded bodies

        this->m_ttdTopLevelNewFunction.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_PTR_ID, TTD::TopLevelFunctionInContextRelation>& entry) -> bool {
            Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(entry.Value().ContextSpecificBodyPtrId);
            if(liveTopLevelBodies.Contains(body))
            {
                return false;
            }
            else
            {
                this->ProcessFunctionBodyOnUnLoad(body, nullptr);
                return true;
            }
        });

        this->m_ttdTopLevelEval.MapAndRemoveIf([&](JsUtil::SimpleDictionaryEntry<TTD_PTR_ID, TTD::TopLevelFunctionInContextRelation>& entry) -> bool {
            Js::FunctionBody* body = TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(entry.Value().ContextSpecificBodyPtrId);
            if(liveTopLevelBodies.Contains(body))
            {
                return false;
            }
            else
            {
                this->ProcessFunctionBodyOnUnLoad(body, nullptr);
                return true;
            }
        });
    }

    Js::FunctionBody* ScriptContextTTD::ResolveParentBody(Js::FunctionBody* body) const
    {
        //Want to return null if this has no parent (or this is an internal function and we don't care about parents)
        return this->m_ttdFunctionBodyParentMap.LookupWithKey(body, nullptr);
    }

    uint32 ScriptContextTTD::FindTopLevelCtrForBody(Js::FunctionBody* body) const
    {
        Js::FunctionBody* rootBody = body;
        while(this->ResolveParentBody(rootBody) != nullptr)
        {
            rootBody = this->ResolveParentBody(rootBody);
        }

        TTD_PTR_ID trgtid = TTD_CONVERT_FUNCTIONBODY_TO_PTR_ID(rootBody);

        for(auto iter = this->m_ttdTopLevelScriptLoad.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().ContextSpecificBodyPtrId == trgtid)
            {
                return iter.CurrentValue().TopLevelBodyCtr;
            }
        }

        for(auto iter = this->m_ttdTopLevelNewFunction.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().ContextSpecificBodyPtrId == trgtid)
            {
                return iter.CurrentValue().TopLevelBodyCtr;
            }
        }

        for(auto iter = this->m_ttdTopLevelEval.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().ContextSpecificBodyPtrId == trgtid)
            {
                return iter.CurrentValue().TopLevelBodyCtr;
            }
        }

        TTDAssert(false, "We are missing a top-level function reference.");
        return 0;
    }

    Js::FunctionBody* ScriptContextTTD::FindRootBodyByTopLevelCtr(uint32 bodyCtrId) const
    {
        for(auto iter = this->m_ttdTopLevelScriptLoad.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().TopLevelBodyCtr == bodyCtrId)
            {
                return TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            }
        }

        for(auto iter = this->m_ttdTopLevelNewFunction.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().TopLevelBodyCtr == bodyCtrId)
            {
                return TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            }
        }

        for(auto iter = this->m_ttdTopLevelEval.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            if(iter.CurrentValue().TopLevelBodyCtr == bodyCtrId)
            {
                return TTD_COERCE_PTR_ID_TO_FUNCTIONBODY(iter.CurrentValue().ContextSpecificBodyPtrId);
            }
        }

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
        TTDAssert(propertyList.Count() == 0, "This should be empty.");

        Js::ScriptContext* ctx = obj->GetScriptContext();
        uint32 propcount = (uint32)obj->GetPropertyCount();

        //get all of the properties
        for(uint32 i = 0; i < propcount; ++i)
        {
            Js::PropertyIndex propertyIndex = (Js::PropertyIndex)i;
            Js::PropertyId propertyId = obj->GetPropertyId(propertyIndex);

            if((propertyId != Js::Constants::NoProperty) & (!Js::IsInternalPropertyId(propertyId)))
            {
                TTDAssert(obj->HasOwnProperty(propertyId), "We are assuming this is own property count.");

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
        const UtilSupport::TTAutoString* res = this->m_coreObjToPathMap.Item(obj);

        return res->GetStrValue();
    }

    TTD_WELLKNOWN_TOKEN RuntimeContextInfo::ResolvePathForKnownFunctionBody(Js::FunctionBody* fbody) const
    {
        const UtilSupport::TTAutoString* res = this->m_coreBodyToPathMap.Item(fbody);

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
        TTDAssert(pos != -1, "This isn't a well known object!");

        return this->m_sortedObjectList.Item(pos);
    }

    Js::FunctionBody* RuntimeContextInfo::LookupKnownFunctionBodyFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::FunctionBody*, true>(pathIdString, this->m_coreBodyToPathMap, this->m_sortedFunctionBodyList, this->m_nullString);
        TTDAssert(pos != -1, "Missing function.");

        return (pos != -1) ? this->m_sortedFunctionBodyList.Item(pos) : nullptr;
    }

    Js::DebuggerScope* RuntimeContextInfo::LookupKnownDebuggerScopeFromPath(TTD_WELLKNOWN_TOKEN pathIdString) const
    {
        int32 pos = LookupPositionInDictNameList<Js::DebuggerScope*, true>(pathIdString, this->m_coreDbgScopeToPathMap, this->m_sortedDbgScopeList, this->m_nullString);
        TTDAssert(pos != -1, "Missing debug scope.");

        return (pos != -1) ? this->m_sortedDbgScopeList.Item(pos) : nullptr;
    }

    void RuntimeContextInfo::GatherKnownObjectToPathMap(Js::ScriptContext* ctx)
    {
        JsUtil::List<const Js::PropertyRecord*, HeapAllocator> propertyRecordList(&HeapAllocator::Instance);

        this->EnqueueRootPathObject(_u("global"), ctx->GetGlobalObject());
        this->EnqueueRootPathObject(_u("null"), ctx->GetLibrary()->GetNull());

        this->EnqueueRootPathObject(_u("_defaultAccessor"), ctx->GetLibrary()->GetDefaultAccessorFunction());

        if(ctx->GetConfig()->IsErrorStackTraceEnabled())
        {
            this->EnqueueRootPathObject(_u("_stackTraceAccessor"), ctx->GetLibrary()->GetStackTraceAccessorFunction());
        }

        this->EnqueueRootPathObject(_u("_throwTypeErrorRestrictedPropertyAccessor"), ctx->GetLibrary()->GetThrowTypeErrorRestrictedPropertyAccessorFunction());

        if(ctx->GetConfig()->IsES6PromiseEnabled())
        {
            this->EnqueueRootPathObject(_u("_identityFunction"), ctx->GetLibrary()->GetIdentityFunction());
            this->EnqueueRootPathObject(_u("_throwerFunction"), ctx->GetLibrary()->GetThrowerFunction());
        }

        this->EnqueueRootPathObject(_u("_arrayIteratorPrototype"), ctx->GetLibrary()->GetArrayIteratorPrototype());
        this->EnqueueRootPathObject(_u("_mapIteratorPrototype"), ctx->GetLibrary()->GetMapIteratorPrototype());
        this->EnqueueRootPathObject(_u("_setIteratorPrototype"), ctx->GetLibrary()->GetSetIteratorPrototype());
        this->EnqueueRootPathObject(_u("_stringIteratorPrototype"), ctx->GetLibrary()->GetStringIteratorPrototype());

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
                        TTDAssert(Js::JavascriptFunction::Is(getter), "The getter is not a function?");
                        this->EnqueueNewPathVarAsNeeded(curr, getter, precord, _u(">"));
                    }

                    if(setter != nullptr && !Js::JavascriptOperators::IsUndefinedObject(setter))
                    {
                        TTDAssert(Js::JavascriptFunction::Is(setter), "The setter is not a function?");
                        this->EnqueueNewPathVarAsNeeded(curr, Js::RecyclableObject::FromVar(setter), precord, _u("<"));
                    }
                }
                else
                {
                    Js::Var pitem = nullptr;
                    BOOL isproperty = Js::JavascriptOperators::GetOwnProperty(curr, precord->GetPropertyId(), &pitem, ctx);
                    TTDAssert(isproperty, "Not sure what went wrong.");

                    this->EnqueueNewPathVarAsNeeded(curr, pitem, precord, nullptr);
                }
            }

            //shouldn't have any dynamic array valued properties
            TTDAssert(!Js::DynamicType::Is(curr->GetTypeId()) || (Js::DynamicObject::FromVar(curr))->GetObjectArray() == nullptr || (Js::DynamicObject::FromVar(curr))->GetObjectArray()->GetLength() == 0, "Shouldn't have any dynamic array valued properties at this point.");

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

        TTDAssert(!this->m_coreObjToPathMap.ContainsKey(obj), "Already in map!!!");
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
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.Item(parent);

            this->m_worklist.Enqueue(obj);

            UtilSupport::TTAutoString* tpath = TT_HEAP_NEW(UtilSupport::TTAutoString, *ppath);
            tpath->Append(_u("."));
            tpath->Append(propName);

            if(optacessortag != nullptr)
            {
                tpath->Append(optacessortag);
            }

            TTDAssert(!this->m_coreObjToPathMap.ContainsKey(obj), "Already in map!!!");
            this->m_coreObjToPathMap.AddNew(obj, tpath);
        }
    }

    void RuntimeContextInfo::EnqueueNewFunctionBodyObject(Js::RecyclableObject* parent, Js::FunctionBody* fbody, const char16* name)
    {
        if(!this->m_coreBodyToPathMap.ContainsKey(fbody))
        {
            fbody->EnsureDeserialized();
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.Item(parent);

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
            const UtilSupport::TTAutoString* ppath = this->m_coreObjToPathMap.Item(parent);

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
