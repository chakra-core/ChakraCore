//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "jsrtHelper.h"
#include "JsrtExternalObject.h"
#include "Types/PathTypeHandler.h"

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

bool JsrtExternalObject::Is(Js::Var value)
{
    if (Js::TaggedNumber::Is(value))
    {
        return false;
    }

    return (VirtualTableInfo<JsrtExternalObject>::HasVirtualTable(value)) ||
        (VirtualTableInfo<Js::CrossSiteObject<JsrtExternalObject>>::HasVirtualTable(value));
}

JsrtExternalObject * JsrtExternalObject::FromVar(Js::Var value)
{
    AssertOrFailFast(Is(value));
    return static_cast<JsrtExternalObject *>(value);
}

JsrtExternalObject * JsrtExternalObject::UnsafeFromVar(Js::Var value)
{
    Assert(Is(value));
    return static_cast<JsrtExternalObject *>(value);
}

void JsrtExternalObject::Mark(Recycler * recycler) 
{
    JsTraceCallback traceCallback = this->GetExternalType()->GetJsTraceCallback();
    Assert(nullptr != traceCallback);
    JsrtCallbackState scope(nullptr);
    traceCallback(this->GetSlotData());
}

void JsrtExternalObject::Finalize(bool isShutdown)
{
    JsFinalizeCallback finalizeCallback = this->GetExternalType()->GetJsFinalizeCallback();
    Assert(this->GetExternalType()->GetJsTraceCallback() != nullptr || finalizeCallback != nullptr);
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
        ? this->u.slot
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

void* JsrtExternalObject::GetInlineSlots() const
{
    return this->slotType == SlotType::External
        ? nullptr
        : (void*)((uintptr_t)this + sizeof(JsrtExternalObject));
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
