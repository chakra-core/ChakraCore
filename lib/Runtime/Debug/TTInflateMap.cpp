//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

#define PATH_BUFFER_COUNT 256

namespace TTD
{
    InflateMap::InflateMap()
        : m_typeMap(), m_handlerMap(),
        m_tagToGlobalObjectMap(), m_objectMap(),
        m_functionBodyMap(), m_environmentMap(), m_slotArrayMap(), m_promiseDataMap(&HeapAllocator::Instance),
        m_debuggerScopeHomeBodyMap(), m_debuggerScopeChainIndexMap(),
        m_inflatePinSet(), m_environmentPinSet(), m_oldInflatePinSet(),
        m_oldObjectMap(), m_oldFunctionBodyMap(), m_propertyReset(&HeapAllocator::Instance)
    {
        ;
    }

    InflateMap::~InflateMap()
    {
        if(this->m_inflatePinSet != nullptr)
        {
            this->m_inflatePinSet.Unroot(this->m_inflatePinSet->GetAllocator());
        }

        if(this->m_environmentPinSet != nullptr)
        {
            this->m_environmentPinSet.Unroot(this->m_environmentPinSet->GetAllocator());
        }

        if(this->m_slotArrayPinSet != nullptr)
        {
            this->m_slotArrayPinSet.Unroot(this->m_slotArrayPinSet->GetAllocator());
        }

        if(this->m_oldInflatePinSet != nullptr)
        {
            this->m_oldInflatePinSet.Unroot(this->m_oldInflatePinSet->GetAllocator());
        }
    }

    void InflateMap::PrepForInitialInflate(ThreadContext* threadContext, uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 dbgScopeCount, uint32 envCount, uint32 slotCount)
    {
        this->m_typeMap.Initialize(typeCount);
        this->m_handlerMap.Initialize(handlerCount);
        this->m_tagToGlobalObjectMap.Initialize(ctxCount);
        this->m_objectMap.Initialize(objectCount);
        this->m_functionBodyMap.Initialize(bodyCount);
        this->m_debuggerScopeHomeBodyMap.Initialize(dbgScopeCount);
        this->m_debuggerScopeChainIndexMap.Initialize(dbgScopeCount);
        this->m_environmentMap.Initialize(envCount);
        this->m_slotArrayMap.Initialize(slotCount);
        this->m_promiseDataMap.Clear();

        Recycler * recycler = threadContext->GetRecycler();
        this->m_inflatePinSet.Root(RecyclerNew(recycler, ObjectPinSet, recycler, objectCount), recycler);
        this->m_environmentPinSet.Root(RecyclerNew(recycler, EnvironmentPinSet, recycler, objectCount), recycler);
        this->m_slotArrayPinSet.Root(RecyclerNew(recycler, SlotArrayPinSet, recycler, objectCount), recycler);
    }

    void InflateMap::PrepForReInflate(uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 dbgScopeCount, uint32 envCount, uint32 slotCount)
    {
        this->m_typeMap.Initialize(typeCount);
        this->m_handlerMap.Initialize(handlerCount);
        this->m_tagToGlobalObjectMap.Initialize(ctxCount);
        this->m_debuggerScopeHomeBodyMap.Initialize(dbgScopeCount);
        this->m_debuggerScopeChainIndexMap.Initialize(dbgScopeCount);
        this->m_environmentMap.Initialize(envCount);
        this->m_slotArrayMap.Initialize(slotCount);
        this->m_promiseDataMap.Clear();

        //We re-use these values (and reset things below) so we don't neet to initialize them here
        //m_objectMap
        //m_functionBodyMap

        //copy info we want to reuse into the old maps
        this->m_oldObjectMap.MoveDataInto(this->m_objectMap);
        this->m_oldFunctionBodyMap.MoveDataInto(this->m_functionBodyMap);

        //allocate the old pin set and fill it
        TTDAssert(this->m_oldInflatePinSet == nullptr, "Old pin set is not null.");
        Recycler* pinRecycler = this->m_inflatePinSet->GetAllocator();
        this->m_oldInflatePinSet.Root(RecyclerNew(pinRecycler, ObjectPinSet, pinRecycler, this->m_inflatePinSet->Count()), pinRecycler);

        for(auto iter = this->m_inflatePinSet->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            this->m_oldInflatePinSet->AddNew(iter.CurrentKey());
        }

        this->m_inflatePinSet->Clear();
        this->m_environmentPinSet->Clear();
        this->m_slotArrayPinSet->Clear();
    }

    void InflateMap::CleanupAfterInflate()
    {
        this->m_handlerMap.Unload();
        this->m_typeMap.Unload();
        this->m_tagToGlobalObjectMap.Unload();
        this->m_debuggerScopeHomeBodyMap.Unload();
        this->m_debuggerScopeChainIndexMap.Unload();
        this->m_environmentMap.Unload();
        this->m_slotArrayMap.Unload();
        this->m_promiseDataMap.Clear();

        //We re-use these values (and reset things later) so we don't want to unload them here
        //m_objectMap
        //m_functionBodyMap

        this->m_oldObjectMap.Unload();
        this->m_oldFunctionBodyMap.Unload();
        this->m_propertyReset.Clear();

        if(this->m_oldInflatePinSet != nullptr)
        {
            this->m_oldInflatePinSet.Unroot(this->m_oldInflatePinSet->GetAllocator());
        }
    }

    bool InflateMap::IsObjectAlreadyInflated(TTD_PTR_ID objid) const
    {
        return this->m_objectMap.Contains(objid);
    }

    bool InflateMap::IsFunctionBodyAlreadyInflated(TTD_PTR_ID fbodyid) const
    {
        return this->m_functionBodyMap.Contains(fbodyid);
    }

    Js::RecyclableObject* InflateMap::FindReusableObjectIfExists(TTD_PTR_ID objid) const
    {
        if(!this->m_oldObjectMap.IsValid())
        {
            return nullptr;
        }
        else
        {
            return this->m_oldObjectMap.LookupKnownItem(objid);
        }
    }

    Js::FunctionBody* InflateMap::FindReusableFunctionBodyIfExists(TTD_PTR_ID fbodyid) const
    {
        if(!this->m_oldFunctionBodyMap.IsValid())
        {
            return nullptr;
        }
        else
        {
            return this->m_oldFunctionBodyMap.LookupKnownItem(fbodyid);
        }
    }

    Js::RecyclableObject* InflateMap::FindReusableObject_WellKnowReuseCheck(TTD_PTR_ID objid) const
    {
        return this->m_objectMap.LookupKnownItem(objid);
    }

    Js::DynamicTypeHandler* InflateMap::LookupHandler(TTD_PTR_ID handlerId) const
    {
        return this->m_handlerMap.LookupKnownItem(handlerId);
    }

    Js::Type* InflateMap::LookupType(TTD_PTR_ID typeId) const
    {
        return this->m_typeMap.LookupKnownItem(typeId);
    }

    Js::ScriptContext* InflateMap::LookupScriptContext(TTD_LOG_PTR_ID sctag) const
    {
        return this->m_tagToGlobalObjectMap.LookupKnownItem(sctag)->GetScriptContext();
    }

    Js::RecyclableObject* InflateMap::LookupObject(TTD_PTR_ID objid) const
    {
        return this->m_objectMap.LookupKnownItem(objid);
    }

    Js::FunctionBody* InflateMap::LookupFunctionBody(TTD_PTR_ID functionId) const
    {
        return this->m_functionBodyMap.LookupKnownItem(functionId);
    }

    Js::FrameDisplay* InflateMap::LookupEnvironment(TTD_PTR_ID envid) const
    {
        return this->m_environmentMap.LookupKnownItem(envid);
    }

    Js::Var* InflateMap::LookupSlotArray(TTD_PTR_ID slotid) const
    {
        return this->m_slotArrayMap.LookupKnownItem(slotid);
    }

    void InflateMap::LookupInfoForDebugScope(TTD_PTR_ID dbgScopeId, Js::FunctionBody** homeBody, int32* chainIndex) const
    {
        *homeBody = this->m_debuggerScopeHomeBodyMap.LookupKnownItem(dbgScopeId);
        *chainIndex = this->m_debuggerScopeChainIndexMap.LookupKnownItem(dbgScopeId);
    }

    void InflateMap::AddDynamicHandler(TTD_PTR_ID handlerId, Js::DynamicTypeHandler* value)
    {
        this->m_handlerMap.AddItem(handlerId, value);
    }

    void InflateMap::AddType(TTD_PTR_ID typeId, Js::Type* value)
    {
        this->m_typeMap.AddItem(typeId, value);
    }

    void InflateMap::AddScriptContext(TTD_LOG_PTR_ID sctag, Js::ScriptContext* value)
    {
        Js::GlobalObject* globalObj = value->GetGlobalObject();

        this->m_tagToGlobalObjectMap.AddItem(sctag, globalObj);
        value->ScriptContextLogTag = sctag;
    }

    void InflateMap::AddObject(TTD_PTR_ID objid, Js::RecyclableObject* value)
    {
        this->m_objectMap.AddItem(objid, value);
        this->m_inflatePinSet->AddNew(value);
    }

    void InflateMap::AddInflationFunctionBody(TTD_PTR_ID functionId, Js::FunctionBody* value)
    {
        this->m_functionBodyMap.AddItem(functionId, value);
        //Function bodies are either root (and kept live by our root pin set in the script context/info) or are reachable from it
    }

    void InflateMap::AddEnvironment(TTD_PTR_ID envId, Js::FrameDisplay* value)
    {
        this->m_environmentMap.AddItem(envId, value);
        this->m_environmentPinSet->AddNew(value);
    }

    void InflateMap::AddSlotArray(TTD_PTR_ID slotId, Js::Var* value)
    {
        this->m_slotArrayMap.AddItem(slotId, value);
        this->m_slotArrayPinSet->AddNew(value);
    }

    void InflateMap::UpdateFBScopes(const NSSnapValues::SnapFunctionBodyScopeChain& scopeChainInfo, Js::FunctionBody* fb)
    {
        TTDAssert((int32)scopeChainInfo.ScopeCount == (fb->GetScopeObjectChain() != nullptr ? fb->GetScopeObjectChain()->pScopeChain->Count() : 0), "Mismatch in scope counts!!!");

        if(fb->GetScopeObjectChain() != nullptr)
        {
            Js::ScopeObjectChain* scChain = fb->GetScopeObjectChain();
            for(int32 i = 0; i < scChain->pScopeChain->Count(); ++i)
            {
                TTD_PTR_ID dbgScopeId = scopeChainInfo.ScopeArray[i];

                if(!this->m_debuggerScopeHomeBodyMap.Contains(dbgScopeId))
                {
                    this->m_debuggerScopeHomeBodyMap.AddItem(dbgScopeId, fb);
                    this->m_debuggerScopeChainIndexMap.AddItem(dbgScopeId, i);
                }
            }
        }
    }

    JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& InflateMap::GetPropertyResetSet()
    {
        return this->m_propertyReset;
    }

    Js::Var InflateMap::InflateTTDVar(TTDVar var) const
    {
        if(Js::TaggedNumber::Is(var))
        {
            return static_cast<Js::Var>(var);
        }
        else
        {
            return this->LookupObject(TTD_CONVERT_VAR_TO_PTR_ID(var));
        }
    }

    //////////////////
#if ENABLE_SNAPSHOT_COMPARE
    TTDComparePath::TTDComparePath()
        : m_prefix(nullptr), m_stepKind(StepKind::Empty), m_step()
    {
        ;
    }

    TTDComparePath::TTDComparePath(const TTDComparePath* prefix, StepKind stepKind, const PathEntry& nextStep)
        : m_prefix(prefix), m_stepKind(stepKind), m_step(nextStep)
    {
        ;
    }

    TTDComparePath::~TTDComparePath()
    {
        ;
    }

    void TTDComparePath::WritePathToConsole(ThreadContext* threadContext, bool printNewline, _Out_writes_z_(buffLength) char16* namebuff, charcount_t namebuffLength) const
    {
        if(this->m_prefix != nullptr)
        {
            this->m_prefix->WritePathToConsole(threadContext, false, namebuff, namebuffLength);
        }

        if(this->m_stepKind == StepKind::PropertyData || this->m_stepKind == StepKind::PropertyGetter || this->m_stepKind == StepKind::PropertySetter)
        {
            const Js::PropertyRecord* pRecord = threadContext->GetPropertyName((Js::PropertyId)this->m_step.IndexOrPID);
            js_memcpy_s(namebuff, namebuffLength * sizeof(char16), pRecord->GetBuffer(), pRecord->GetLength() * sizeof(char16));

            // Don't allow the null to be written past the end of the buffer.
            namebuff[min(namebuffLength - 1, pRecord->GetLength())] = _u('\0');
        }

        bool isFirst = (this->m_prefix == nullptr);
        switch(this->m_stepKind)
        {
        case StepKind::Empty:
            break;
        case StepKind::Root:
            wprintf(_u("root#%I64i"), this->m_step.IndexOrPID);
            break;
        case StepKind::PropertyData:
            wprintf(_u("%ls%ls"), (isFirst ? _u("") : _u(".")), namebuff);
            break;
        case StepKind::PropertyGetter:
            wprintf(_u("%ls<%ls"), (isFirst ? _u("") : _u(".")), namebuff);
            break;
        case StepKind::PropertySetter:
            wprintf(_u("%ls>%ls"), (isFirst ? _u("") : _u(".")), namebuff);
            break;
        case StepKind::Array:
            wprintf(_u("[%I64i]"), this->m_step.IndexOrPID);
            break;
        case StepKind::Scope:
            wprintf(_u("%ls_scope[%I64i]"), (isFirst ? _u("") : _u(".")), this->m_step.IndexOrPID);
            break;
        case StepKind::SlotArray:
            wprintf(_u("%ls_slots[%I64i]"), (isFirst ? _u("") : _u(".")), this->m_step.IndexOrPID);
            break;
        case StepKind::FunctionBody:
            wprintf(_u("%ls%ls"), (isFirst ? _u("") : _u(".")), this->m_step.OptName);
            break;
        case StepKind::Special:
            wprintf(_u("%ls_%ls"), (isFirst ? _u("") : _u(".")), this->m_step.OptName);
            break;
        case StepKind::SpecialArray:
            wprintf(_u("%ls_%ls[%I64i]"), (isFirst ? _u("") : _u(".")), this->m_step.OptName, this->m_step.IndexOrPID);
            break;
        default:
            TTDAssert(false, "Unknown tag in switch statement!!!");
            break;
        }

        if(printNewline)
        {
            wprintf(_u("\n"));
        }
    }

    TTDCompareMap::TTDCompareMap(ThreadContext* threadContext)
        : StrictCrossSite(false), H1PtrIdWorklist(&HeapAllocator::Instance), H1PtrToH2PtrMap(&HeapAllocator::Instance), SnapObjCmpVTable(nullptr), H1PtrToPathMap(&HeapAllocator::Instance), 
        CurrentPath(nullptr), CurrentH1Ptr(TTD_INVALID_PTR_ID), CurrentH2Ptr(TTD_INVALID_PTR_ID), Context(threadContext),
        //
        H1ValueMap(&HeapAllocator::Instance), H1SlotArrayMap(&HeapAllocator::Instance), H1FunctionScopeInfoMap(&HeapAllocator::Instance),
        H1FunctionTopLevelLoadMap(&HeapAllocator::Instance), H1FunctionTopLevelNewMap(&HeapAllocator::Instance), H1FunctionTopLevelEvalMap(&HeapAllocator::Instance),
        H1FunctionBodyMap(&HeapAllocator::Instance), H1ObjectMap(&HeapAllocator::Instance), H1PendingAsyncModBufferSet(&HeapAllocator::Instance),
        //
        H2ValueMap(&HeapAllocator::Instance), H2SlotArrayMap(&HeapAllocator::Instance), H2FunctionScopeInfoMap(&HeapAllocator::Instance),
        H2FunctionTopLevelLoadMap(&HeapAllocator::Instance), H2FunctionTopLevelNewMap(&HeapAllocator::Instance), H2FunctionTopLevelEvalMap(&HeapAllocator::Instance),
        H2FunctionBodyMap(&HeapAllocator::Instance), H2ObjectMap(&HeapAllocator::Instance), H2PendingAsyncModBufferSet(&HeapAllocator::Instance)
    {
        this->StrictCrossSite = !threadContext->TTDLog->IsDebugModeFlagSet();

        this->PathBuffer = TT_HEAP_ALLOC_ARRAY_ZERO(char16, PATH_BUFFER_COUNT);

        this->SnapObjCmpVTable = TT_HEAP_ALLOC_ARRAY_ZERO(fPtr_AssertSnapEquivAddtlInfo, (int32)NSSnapObjects::SnapObjectType::Limit);

        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapScriptFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapScriptFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapExternalFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapExternalFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapRuntimeRevokerFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapRevokerFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapBoundFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapBoundFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject] = &NSSnapObjects::AssertSnapEquiv_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapHeapArgumentsObject>;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject] = &NSSnapObjects::AssertSnapEquiv_SnapHeapArgumentsInfo<NSSnapObjects::SnapObjectType::SnapES5HeapArgumentsObject>;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapBoxedValueObject] = &NSSnapObjects::AssertSnapEquiv_SnapBoxedValue;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapDateObject] = &NSSnapObjects::AssertSnapEquiv_SnapDate;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapRegexObject] = &NSSnapObjects::AssertSnapEquiv_SnapRegexInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapArrayObject] = &NSSnapObjects::AssertSnapEquiv_SnapArrayInfo<TTDVar, NSSnapObjects::SnapObjectType::SnapArrayObject>;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject] = &NSSnapObjects::AssertSnapEquiv_SnapArrayInfo<int, NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject] = &NSSnapObjects::AssertSnapEquiv_SnapArrayInfo<double, NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapES5ArrayObject] = &NSSnapObjects::AssertSnapEquiv_SnapES5ArrayInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapArrayBufferObject] = &NSSnapObjects::AssertSnapEquiv_SnapArrayBufferInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapTypedArrayObject] = &NSSnapObjects::AssertSnapEquiv_SnapTypedArrayInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapSetObject] = &NSSnapObjects::AssertSnapEquiv_SnapSetInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapMapObject] = &NSSnapObjects::AssertSnapEquiv_SnapMapInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapProxyObject] = &NSSnapObjects::AssertSnapEquiv_SnapProxyInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapPromiseObject] = &NSSnapObjects::AssertSnapEquiv_SnapPromiseInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapPromiseResolveOrRejectFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapPromiseResolveOrRejectFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapPromiseReactionTaskFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapPromiseReactionTaskFunctionInfo;
        this->SnapObjCmpVTable[(int32)NSSnapObjects::SnapObjectType::SnapPromiseAllResolveElementFunctionObject] = &NSSnapObjects::AssertSnapEquiv_SnapPromiseAllResolveElementFunctionInfo;
    }

    TTDCompareMap::~TTDCompareMap()
    {
        TT_HEAP_FREE_ARRAY(char16, this->PathBuffer, PATH_BUFFER_COUNT);

        TT_HEAP_FREE_ARRAY(TTD::fPtr_AssertSnapEquivAddtlInfo, this->SnapObjCmpVTable, (int32)NSSnapObjects::SnapObjectType::Limit);

        //delete all the compare paths
        for(auto iter = this->H1PtrToPathMap.GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            TT_HEAP_DELETE(TTDComparePath, iter.CurrentValue());
        }
    }

    void TTDCompareMap::DiagnosticAssert(bool condition)
    {
        if(!condition)
        {
            if(this->CurrentPath != nullptr)
            {
                wprintf(_u("Snap1 ptrid: *0x%I64x\n"), this->CurrentH1Ptr);
                wprintf(_u("Snap2 ptrid: *0x%I64x\n"), this->CurrentH2Ptr);
                this->CurrentPath->WritePathToConsole(this->Context, true, this->PathBuffer, PATH_BUFFER_COUNT);
            }
        }

        TTDAssert(condition, "Diagnostic compare assertion failed!!!");
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_Helper(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next)
    {
        if(h1PtrId == TTD_INVALID_PTR_ID || h2PtrId == TTD_INVALID_PTR_ID)
        {
            this->DiagnosticAssert(h1PtrId == TTD_INVALID_PTR_ID && h2PtrId == TTD_INVALID_PTR_ID);
        }
        else if(this->H1PtrToH2PtrMap.ContainsKey(h1PtrId))
        {
            this->DiagnosticAssert(this->H1PtrToH2PtrMap.Item(h1PtrId) == h2PtrId);
        }
        else if(this->H1ValueMap.ContainsKey(h1PtrId))
        {
            this->DiagnosticAssert(this->H2ValueMap.ContainsKey(h2PtrId));

            const NSSnapValues::SnapPrimitiveValue* v1 = this->H1ValueMap.Item(h1PtrId);
            const NSSnapValues::SnapPrimitiveValue* v2 = this->H2ValueMap.Item(h2PtrId);
            NSSnapValues::AssertSnapEquiv(v1, v2, *this);
        }
        else
        {
            this->H1PtrIdWorklist.Enqueue(h1PtrId);

            TTDComparePath* objPath = TT_HEAP_NEW(TTDComparePath, this->CurrentPath, stepKind, next);
            this->H1PtrToPathMap.AddNew(h1PtrId, objPath);

            this->H1PtrToH2PtrMap.AddNew(h1PtrId, h2PtrId);
        }
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_Scope(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, uint32 index)
    {
        TTDComparePath::PathEntry next{ index, nullptr };
        this->CheckConsistentAndAddPtrIdMapping_Helper(h1PtrId, h2PtrId, TTDComparePath::StepKind::Scope, next);
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_FunctionBody(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId)
    {
        TTDComparePath::PathEntry next{ -1, _u("!body") };
        this->CheckConsistentAndAddPtrIdMapping_Helper(h1PtrId, h2PtrId, TTDComparePath::StepKind::FunctionBody, next);
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_Special(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, const char16* specialField)
    {
        TTDComparePath::PathEntry next{ -1, specialField };
        this->CheckConsistentAndAddPtrIdMapping_Helper(h1PtrId, h2PtrId, TTDComparePath::StepKind::Special, next);
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_Root(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId, TTD_LOG_PTR_ID tag)
    {
        TTDComparePath::PathEntry next{ (uint32)tag, nullptr };
        this->CheckConsistentAndAddPtrIdMapping_Helper(h1PtrId, h2PtrId, TTDComparePath::StepKind::Root, next);
    }

    void TTDCompareMap::CheckConsistentAndAddPtrIdMapping_NoEnqueue(TTD_PTR_ID h1PtrId, TTD_PTR_ID h2PtrId)
    {
        if(h1PtrId == TTD_INVALID_PTR_ID || h2PtrId == TTD_INVALID_PTR_ID)
        {
            this->DiagnosticAssert(h1PtrId == TTD_INVALID_PTR_ID && h2PtrId == TTD_INVALID_PTR_ID);
        }
        else if(this->H1PtrToH2PtrMap.ContainsKey(h1PtrId))
        {
            this->DiagnosticAssert(this->H1PtrToH2PtrMap.Item(h1PtrId) == h2PtrId);
        }
        else
        {
            this->H1PtrToH2PtrMap.AddNew(h1PtrId, h2PtrId);
        }
    }

    void TTDCompareMap::GetNextCompareInfo(TTDCompareTag* tag, TTD_PTR_ID* h1PtrId, TTD_PTR_ID* h2PtrId)
    {
        if(this->H1PtrIdWorklist.Empty())
        {
            *tag = TTDCompareTag::Done;
            *h1PtrId = TTD_INVALID_PTR_ID;
            *h2PtrId = TTD_INVALID_PTR_ID;
        }
        else
        {
            *h1PtrId = this->H1PtrIdWorklist.Dequeue();
            *h2PtrId = this->H1PtrToH2PtrMap.Item(*h1PtrId);

            this->CurrentPath = this->H1PtrToPathMap.Item(*h1PtrId);
            this->CurrentH1Ptr = *h1PtrId;
            this->CurrentH2Ptr = *h2PtrId;

            if(this->H1SlotArrayMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2SlotArrayMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::SlotArray;
            }
            else if(this->H1FunctionScopeInfoMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2FunctionScopeInfoMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::FunctionScopeInfo;
            }
            else if(this->H1FunctionTopLevelLoadMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2FunctionTopLevelLoadMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::TopLevelLoadFunction;
            }
            else if(this->H1FunctionTopLevelNewMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2FunctionTopLevelNewMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::TopLevelNewFunction;
            }
            else if(this->H1FunctionTopLevelEvalMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2FunctionTopLevelEvalMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::TopLevelEvalFunction;
            }
            else if(this->H1FunctionBodyMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2FunctionBodyMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::FunctionBody;
            }
            else if(this->H1ObjectMap.ContainsKey(*h1PtrId))
            {
                this->DiagnosticAssert(this->H2ObjectMap.ContainsKey(*h2PtrId));
                *tag = TTDCompareTag::SnapObject;
            }
            else
            {
                TTDAssert(!this->H1ValueMap.ContainsKey(*h1PtrId), "Should be comparing by value!!!");
                TTDAssert(false, "Id not found in any of the maps!!!");
                *tag = TTDCompareTag::Done;
            }
        }
    }

    void TTDCompareMap::GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::SlotArrayInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::SlotArrayInfo** val2)
    {
        TTDAssert(compareTag == TTDCompareTag::SlotArray, "Should be a type");
        *val1 = this->H1SlotArrayMap.Item(h1PtrId);
        *val2 = this->H2SlotArrayMap.Item(h2PtrId);
    }

    void TTDCompareMap::GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::ScriptFunctionScopeInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::ScriptFunctionScopeInfo** val2)
    {
        TTDAssert(compareTag == TTDCompareTag::FunctionScopeInfo, "Should be a type");
        *val1 = this->H1FunctionScopeInfoMap.Item(h1PtrId);
        *val2 = this->H2FunctionScopeInfoMap.Item(h2PtrId);
    }

    void TTDCompareMap::GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, uint64* val1, TTD_PTR_ID h2PtrId, uint64* val2)
    {
        if(compareTag == TTDCompareTag::TopLevelLoadFunction)
        {
            *val1 = this->H1FunctionTopLevelLoadMap.Item(h1PtrId);
            *val2 = this->H2FunctionTopLevelLoadMap.Item(h2PtrId);
        }
        else if(compareTag == TTDCompareTag::TopLevelNewFunction)
        {
            *val1 = this->H1FunctionTopLevelNewMap.Item(h1PtrId);
            *val2 = this->H2FunctionTopLevelNewMap.Item(h2PtrId);
        }
        else
        {
            TTDAssert(compareTag == TTDCompareTag::TopLevelEvalFunction, "Should be a type");
            *val1 = this->H1FunctionTopLevelEvalMap.Item(h1PtrId);
            *val2 = this->H2FunctionTopLevelEvalMap.Item(h2PtrId);
        }
    }

    void TTDCompareMap::GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapValues::FunctionBodyResolveInfo** val1, TTD_PTR_ID h2PtrId, const NSSnapValues::FunctionBodyResolveInfo** val2)
    {
        TTDAssert(compareTag == TTDCompareTag::FunctionBody, "Should be a type");
        *val1 = this->H1FunctionBodyMap.Item(h1PtrId);
        *val2 = this->H2FunctionBodyMap.Item(h2PtrId);
    }

    void TTDCompareMap::GetCompareValues(TTDCompareTag compareTag, TTD_PTR_ID h1PtrId, const NSSnapObjects::SnapObject** val1, TTD_PTR_ID h2PtrId, const NSSnapObjects::SnapObject** val2)
    {
        TTDAssert(compareTag == TTDCompareTag::SnapObject, "Should be a type");
        *val1 = this->H1ObjectMap.Item(h1PtrId);
        *val2 = this->H2ObjectMap.Item(h2PtrId);
    }
#endif
}

#endif
