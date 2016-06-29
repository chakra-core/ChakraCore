//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITType::JITType()
{
}

JITType::JITType(TypeIDL * data) :
    m_data(*data)
{
    CompileAssert(sizeof(JITType) == sizeof(TypeIDL));
}

/* static */
void
JITType::BuildFromJsType(__in Js::Type * jsType, __out JITType * jitType)
{
    TypeIDL * data = jitType->GetData();
    data->addr = (intptr_t)jsType;
    data->typeId = jsType->GetTypeId();
    data->libAddr = (intptr_t)jsType->GetLibrary();
    data->protoAddr = (intptr_t)jsType->GetPrototype();
    data->entrypointAddr = (intptr_t)jsType->GetEntryPoint();
    data->propertyCacheAddr = (intptr_t)jsType->GetPropertyCache();
    if (Js::DynamicType::Is(jsType->GetTypeId()))
    {
        Js::DynamicType * dynamicType = static_cast<Js::DynamicType*>(jsType);

        data->isShared = dynamicType->GetIsShared();

        Js::DynamicTypeHandler * handler = dynamicType->GetTypeHandler();
        data->handler.isObjectHeaderInlinedTypeHandler = handler->IsObjectHeaderInlinedTypeHandler();
        data->handler.isLocked = handler->GetIsLocked();
        data->handler.inlineSlotCapacity = handler->GetInlineSlotCapacity();
        data->handler.offsetOfInlineSlots = handler->GetOffsetOfInlineSlots();
        data->handler.slotCapacity = handler->GetSlotCapacity();
    }
}

bool
JITType::IsShared() const
{
    Assert(Js::DynamicType::Is(GetTypeId()));
    return m_data.isShared != FALSE;
}

Js::TypeId
JITType::GetTypeId() const
{
    return (Js::TypeId)m_data.typeId;
}

TypeIDL *
JITType::GetData()
{
    return &m_data;
}

intptr_t
JITType::GetAddr() const
{
    Assert(m_data.addr > 99999);
    return m_data.addr;
}

intptr_t
JITType::GetPrototypeAddr() const
{
    return m_data.protoAddr;
}

const JITTypeHandler*
JITType::GetTypeHandler() const
{
    return (const JITTypeHandler*)&m_data.handler;
}

bool
JITType::operator!=(const JITType& other) const
{
    return this->GetAddr() != other.GetAddr();
}

bool
JITType::operator==(const JITType& other) const
{
    return this->GetAddr() == other.GetAddr();
}
