//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "JsrtPch.h"
#include "jsrtHelper.h"
#include "JsrtExternalObject.h"
#include "Types/PathTypeHandler.h"

JsrtExternalType::JsrtExternalType(Js::ScriptContext* scriptContext, JsFinalizeCallback finalizeCallback)
    : Js::DynamicType(
        scriptContext,
        Js::TypeIds_Object,
        scriptContext->GetLibrary()->GetObjectPrototype(),
        nullptr,
        Js::SimplePathTypeHandler::New(scriptContext, scriptContext->GetLibrary()->GetRootPath(), 0, 0, 0, true, true),
        true,
        true)
        , jsFinalizeCallback(finalizeCallback)
{
    this->flags |= TypeFlagMask_JsrtExternal;
}

JsrtExternalObject::JsrtExternalObject(JsrtExternalType * type, void *data) :
    slot(data),
    Js::DynamicObject(type, false/* initSlots*/)
{
}

/* static */
JsrtExternalObject* JsrtExternalObject::Create(void *data, JsFinalizeCallback finalizeCallback, Js::ScriptContext *scriptContext)
{
    Js::DynamicType * dynamicType = scriptContext->GetLibrary()->GetCachedJsrtExternalType(reinterpret_cast<uintptr_t>(finalizeCallback));

    if (dynamicType == nullptr)
    {
        dynamicType = RecyclerNew(scriptContext->GetRecycler(), JsrtExternalType, scriptContext, finalizeCallback);
        scriptContext->GetLibrary()->CacheJsrtExternalType(reinterpret_cast<uintptr_t>(finalizeCallback), dynamicType);
    }

    Assert(dynamicType->IsJsrtExternal());
    Assert(dynamicType->GetIsShared());

    return RecyclerNewFinalized(scriptContext->GetRecycler(), JsrtExternalObject, static_cast<JsrtExternalType*>(dynamicType), data);
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
    Assert(Is(value));
    return static_cast<JsrtExternalObject *>(value);
}

void JsrtExternalObject::Finalize(bool isShutdown)
{
    JsFinalizeCallback finalizeCallback = this->GetExternalType()->GetJsFinalizeCallback();
    if (nullptr != finalizeCallback)
    {
        JsrtCallbackState scope(nullptr);
        finalizeCallback(this->slot);
    }
}

void JsrtExternalObject::Dispose(bool isShutdown)
{
}

void * JsrtExternalObject::GetSlotData() const
{
    return this->slot;
}

void JsrtExternalObject::SetSlotData(void * data)
{
    this->slot = data;
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
