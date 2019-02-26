//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "jsrtHelper.h"
#include "JsrtExternalObject.h"
#include "Types/PathTypeHandler.h"

#ifdef _CHAKRACOREBUILD
JsrtExternalType::JsrtExternalType(Js::ScriptContext* scriptContext, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype)
    : Js::DynamicType(
        scriptContext,
        Js::TypeIds_Object,
        prototype,
        nullptr,
        Js::PathTypeHandlerNoAttr::New(scriptContext, scriptContext->GetLibrary()->GetRootPath(), 0, 0, 0, true, true),
        true,
        true)
    , jsTraceCallback(traceCallback)
    , jsFinalizeCallback(finalizeCallback)
{
    this->flags |= TypeFlagMask_JsrtExternal;
}
#endif
JsrtExternalType::JsrtExternalType(Js::ScriptContext* scriptContext, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype)
    : Js::DynamicType(
        scriptContext,
        Js::TypeIds_Object,
        prototype,
        nullptr,
        Js::PathTypeHandlerNoAttr::New(scriptContext, scriptContext->GetLibrary()->GetRootPath(), 0, 0, 0, true, true),
        true,
        true)
#ifdef _CHAKRACOREBUILD
    , jsTraceCallback(nullptr)
#endif
    , jsFinalizeCallback(finalizeCallback)
{
    this->flags |= TypeFlagMask_JsrtExternal;
}

JsrtExternalObject::JsrtExternalObject(JsrtExternalType * type, void *data, uint inlineSlotSize) :
    Js::DynamicObject(type, true /* initSlots*/)
{
    if (inlineSlotSize != 0)
    {
        this->slotType = SlotType::Inline;
        this->u.inlineSlotSize = inlineSlotSize;
        if (data)
        {
            memcpy_s(this->GetInlineSlots(), inlineSlotSize, data, inlineSlotSize);
        }
    }
    else
    {
        this->slotType = SlotType::External;
        this->u.slot = data;
    }
}

JsrtExternalObject::JsrtExternalObject(JsrtExternalObject* instance, bool deepCopy) :
    Js::DynamicObject(instance, deepCopy)
{
    if (instance->GetInlineSlotSize() != 0)
    {
        this->slotType = SlotType::Inline;
        this->u.inlineSlotSize = instance->GetInlineSlotSize();
        if (instance->GetInlineSlots())
        {
            memcpy_s(this->GetInlineSlots(), this->GetInlineSlotSize(), instance->GetInlineSlots(), instance->GetInlineSlotSize());
        }
    }
    else
    {
        this->slotType = SlotType::External;
        this->u.slot = instance->GetInlineSlots();
    }
}

#ifdef _CHAKRACOREBUILD
/* static */
JsrtExternalObject* JsrtExternalObject::Create(void *data, uint inlineSlotSize, JsTraceCallback traceCallback, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype, Js::ScriptContext *scriptContext, JsrtExternalType * type)
{
    if (prototype == nullptr)
    {
        prototype = scriptContext->GetLibrary()->GetObjectPrototype();
    }
    if (type == nullptr)
    {
        type = scriptContext->GetLibrary()->GetCachedJsrtExternalType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype));

        if (type == nullptr)
        {
            type = RecyclerNew(scriptContext->GetRecycler(), JsrtExternalType, scriptContext, traceCallback, finalizeCallback, prototype);
            scriptContext->GetLibrary()->CacheJsrtExternalType(reinterpret_cast<uintptr_t>(traceCallback), reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype), type);
        }
    }

    Assert(type->IsJsrtExternal());

    JsrtExternalObject * externalObject;
    if (traceCallback != nullptr)
    {
        externalObject = RecyclerNewTrackedPlus(scriptContext->GetRecycler(), inlineSlotSize, JsrtExternalObject, static_cast<JsrtExternalType*>(type), data, inlineSlotSize);
    }
    else if (finalizeCallback != nullptr) 
    {
        externalObject = RecyclerNewFinalizedPlus(scriptContext->GetRecycler(), inlineSlotSize, JsrtExternalObject, static_cast<JsrtExternalType*>(type), data, inlineSlotSize);
    }
    else
    {
        externalObject = RecyclerNewPlus(scriptContext->GetRecycler(), inlineSlotSize, JsrtExternalObject, static_cast<JsrtExternalType*>(type), data, inlineSlotSize);
    }

    return externalObject;
}
#endif
/* static */
JsrtExternalObject* JsrtExternalObject::Create(void *data, uint inlineSlotSize, JsFinalizeCallback finalizeCallback, Js::RecyclableObject * prototype, Js::ScriptContext *scriptContext, JsrtExternalType * type)
{
    if (prototype == nullptr)
    {
        prototype = scriptContext->GetLibrary()->GetObjectPrototype();
    }
    if (type == nullptr)
    {
#ifdef _CHAKRACOREBUILD
        type = scriptContext->GetLibrary()->GetCachedJsrtExternalType(0, reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype));
#else
        type = scriptContext->GetLibrary()->GetCachedJsrtExternalType(reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype));
#endif

        if (type == nullptr)
        {
            type = RecyclerNew(scriptContext->GetRecycler(), JsrtExternalType, scriptContext, finalizeCallback, prototype);
#ifdef _CHAKRACOREBUILD
            scriptContext->GetLibrary()->CacheJsrtExternalType(0, reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype), type);
#else
            scriptContext->GetLibrary()->CacheJsrtExternalType(reinterpret_cast<uintptr_t>(finalizeCallback), reinterpret_cast<uintptr_t>(prototype), type);
#endif
        }
    }

    Assert(type->IsJsrtExternal());

    JsrtExternalObject * externalObject;
    if (finalizeCallback != nullptr)
    {
        externalObject = RecyclerNewFinalizedPlus(scriptContext->GetRecycler(), inlineSlotSize, JsrtExternalObject, static_cast<JsrtExternalType*>(type), data, inlineSlotSize);
    }
    else
    {
        externalObject = RecyclerNewPlus(scriptContext->GetRecycler(), inlineSlotSize, JsrtExternalObject, static_cast<JsrtExternalType*>(type), data, inlineSlotSize);
    }

    return externalObject;
}

JsrtExternalObject* JsrtExternalObject::Copy(bool deepCopy)
{
    Recycler* recycler = this->GetRecycler();
    JsrtExternalType* type = this->GetExternalType();
    int inlineSlotSize = this->GetInlineSlotSize();

#ifdef _CHAKRACOREBUILD
    if (type->GetJsTraceCallback() != nullptr)
    {
        return RecyclerNewTrackedPlus(recycler, inlineSlotSize, JsrtExternalObject, this, deepCopy);
    }
#endif

    if (type->GetJsFinalizeCallback() != nullptr)
    {
        return RecyclerNewFinalizedPlus(recycler, inlineSlotSize, JsrtExternalObject, this, deepCopy);
    }

    return RecyclerNewPlus(recycler, inlineSlotSize, JsrtExternalObject, this, deepCopy);
}

void JsrtExternalObject::Mark(Recycler * recycler) 
{
#ifdef _CHAKRACOREBUILD
    recycler->SetNeedExternalWrapperTracing();
    JsTraceCallback traceCallback = this->GetExternalType()->GetJsTraceCallback();
    Assert(nullptr != traceCallback);
    JsrtCallbackState scope(nullptr);
    traceCallback(this->GetSlotData());
#else
    Assert(UNREACHED);
#endif
}

void JsrtExternalObject::Finalize(bool isShutdown)
{
    JsFinalizeCallback finalizeCallback = this->GetExternalType()->GetJsFinalizeCallback();
#ifdef _CHAKRACOREBUILD
    Assert(this->GetExternalType()->GetJsTraceCallback() != nullptr || finalizeCallback != nullptr);
#else
    Assert(finalizeCallback != nullptr);
#endif
    if (nullptr != finalizeCallback)
    {
        JsrtCallbackState scope(nullptr);
        finalizeCallback(this->GetSlotData());
    }
}

void JsrtExternalObject::Dispose(bool isShutdown)
{
}

void * JsrtExternalObject::GetSlotData() const
{
    return this->slotType == SlotType::External
        ? unsafe_write_barrier_cast<void *>(this->u.slot)
        : GetInlineSlots();
}

void JsrtExternalObject::SetSlotData(void * data)
{
    this->slotType = SlotType::External;
    this->u.slot = data;
}

int JsrtExternalObject::GetInlineSlotSize() const
{
    return this->slotType == SlotType::External
        ? 0
        : this->u.inlineSlotSize;
}

void * JsrtExternalObject::GetInlineSlots() const
{
    return this->slotType == SlotType::External
        ? nullptr
        : (void *)((uintptr_t)this + sizeof(JsrtExternalObject));
}

Js::DynamicType* JsrtExternalObject::DuplicateType()
{
    return RecyclerNew(this->GetScriptContext()->GetRecycler(), JsrtExternalType,
        this->GetExternalType());
}

#if ENABLE_TTD
TTD::NSSnapObjects::SnapObjectType JsrtExternalObject::GetSnapTag_TTD() const
{
    return TTD::NSSnapObjects::SnapObjectType::SnapExternalObject;
}

void JsrtExternalObject::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
{
    TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<void*, TTD::NSSnapObjects::SnapObjectType::SnapExternalObject>(objData, nullptr);
}
#endif
