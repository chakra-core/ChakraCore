//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    InflateMap::InflateMap()
        : m_typeMap(), m_handlerMap(),
        m_tagToGlobalObjectMap(), m_objectMap(),
        m_functionBodyMap(), m_environmentMap(), m_slotArrayMap(), m_promiseDataMap(&HeapAllocator::Instance),
        m_inflatePinSet(nullptr), m_oldInflatePinSet(nullptr),
        m_oldObjectMap(), m_oldFunctionBodyMap(), m_propertyReset(&HeapAllocator::Instance)
    {
        ;
    }

    InflateMap::~InflateMap()
    {
        if(this->m_inflatePinSet != nullptr)
        {
            this->m_inflatePinSet->GetAllocator()->RootRelease(this->m_inflatePinSet);
            this->m_inflatePinSet = nullptr;
        }

        if(this->m_oldInflatePinSet != nullptr)
        {
            this->m_oldInflatePinSet->GetAllocator()->RootRelease(this->m_oldInflatePinSet);
            this->m_oldInflatePinSet = nullptr;
        }
    }

    void InflateMap::PrepForInitialInflate(ThreadContext* threadContext, uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 envCount, uint32 slotCount)
    {
        this->m_typeMap.Initialize(typeCount);
        this->m_handlerMap.Initialize(handlerCount);
        this->m_tagToGlobalObjectMap.Initialize(ctxCount);
        this->m_objectMap.Initialize(objectCount);
        this->m_functionBodyMap.Initialize(bodyCount);
        this->m_environmentMap.Initialize(envCount);
        this->m_slotArrayMap.Initialize(slotCount);
        this->m_promiseDataMap.Clear();

        this->m_inflatePinSet = RecyclerNew(threadContext->GetRecycler(), ReferencePinSet, threadContext->GetRecycler(), objectCount);
        threadContext->GetRecycler()->RootAddRef(this->m_inflatePinSet);
    }

    void InflateMap::PrepForReInflate(uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 envCount, uint32 slotCount)
    {
        this->m_typeMap.Initialize(typeCount);
        this->m_handlerMap.Initialize(handlerCount);
        this->m_tagToGlobalObjectMap.Initialize(ctxCount);
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
        AssertMsg(this->m_oldInflatePinSet == nullptr, "Old pin set is not null.");
        Recycler* pinRecycler = this->m_inflatePinSet->GetAllocator();
        this->m_oldInflatePinSet = RecyclerNew(pinRecycler, ReferencePinSet, pinRecycler, this->m_inflatePinSet->Count());
        pinRecycler->RootAddRef(this->m_oldInflatePinSet);

        for(auto iter = this->m_inflatePinSet->GetIterator(); iter.IsValid(); iter.MoveNext())
        {
            this->m_oldInflatePinSet->AddNew(iter.CurrentKey());
        }
        this->m_inflatePinSet->Clear();
    }

    void InflateMap::CleanupAfterInflate()
    {
        this->m_handlerMap.Unload();
        this->m_typeMap.Unload();
        this->m_tagToGlobalObjectMap.Unload();
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
            this->m_oldInflatePinSet->GetAllocator()->RootRelease(this->m_oldInflatePinSet);
            this->m_oldInflatePinSet = nullptr;
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

    Js::DynamicTypeHandler* InflateMap::LookupHandler(TTD_PTR_ID handlerId) const
    {
        return this->m_handlerMap.LookupKnownItem(handlerId);
    }

    Js::Type* InflateMap::LookupType(TTD_PTR_ID typeId) const
    {
        return this->m_typeMap.LookupKnownItem(typeId);
    }

    Js::ScriptContext* InflateMap::LookupScriptContext(TTD_LOG_TAG sctag) const
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

    void InflateMap::AddDynamicHandler(TTD_PTR_ID handlerId, Js::DynamicTypeHandler* value)
    {
        this->m_handlerMap.AddItem(handlerId, value);
    }

    void InflateMap::AddType(TTD_PTR_ID typeId, Js::Type* value)
    {
        this->m_typeMap.AddItem(typeId, value);
    }

    void InflateMap::AddScriptContext(TTD_LOG_TAG sctag, Js::ScriptContext* value)
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
        this->m_inflatePinSet->AddNew(value);
    }

    void InflateMap::AddEnvironment(TTD_PTR_ID envId, Js::FrameDisplay* value)
    {
        this->m_environmentMap.AddItem(envId, value);
    }

    void InflateMap::AddSlotArray(TTD_PTR_ID slotId, Js::Var* value)
    {
        this->m_slotArrayMap.AddItem(slotId, value);
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
}

#endif
