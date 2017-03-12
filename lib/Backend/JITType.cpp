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
    data->exists = true;
    data->addr = jsType;
    data->typeId = jsType->GetTypeId();
    data->libAddr = jsType->GetLibrary();
    data->protoAddr = jsType->GetPrototype();
    data->entrypointAddr = (intptr_t)jsType->GetEntryPoint();
    data->propertyCacheAddr = jsType->GetPropertyCache();
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
    return (intptr_t)PointerValue(m_data.addr);
}

intptr_t
JITType::GetPrototypeAddr() const
{
    return (intptr_t)PointerValue(m_data.protoAddr);
}

const JITTypeHandler*
JITType::GetTypeHandler() const
{
    return (const JITTypeHandler*)&m_data.handler;
}


template <class TAllocator>
JITTypeHolderBase<TAllocator>::JITTypeHolderBase() :
    t(nullptr)
{
}

template <class TAllocator>
JITTypeHolderBase<TAllocator>::JITTypeHolderBase(JITType * t) :
    t(t)
{
}

template <class TAllocator>
const JITType *
JITTypeHolderBase<TAllocator>::operator->() const
{
    return this->t;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator==(const JITTypeHolderBase& p) const
{
    if (this->t != nullptr && p != nullptr)
    {
        return this->t->GetAddr() == p->GetAddr();
    }
    return this->t == nullptr && p == nullptr;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator!=(const JITTypeHolderBase& p) const
{
    return !(*this == p);
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator==(const std::nullptr_t &p) const
{
    return this->t == nullptr;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator!=(const std::nullptr_t &p) const
{
    return this->t != nullptr;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator>(const JITTypeHolderBase& p) const
{
    if (this->t != nullptr && p != nullptr)
    {
        return this->t->GetAddr() > p->GetAddr();
    }
    return false;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator<=(const JITTypeHolderBase& p) const
{
    return !(*this > p);
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator>=(const JITTypeHolderBase& p) const
{
    if (this->t != nullptr && p != nullptr)
    {
        return this->t->GetAddr() >= p->GetAddr();
    }
    return false;
}

template <class TAllocator>
bool
JITTypeHolderBase<TAllocator>::operator<(const JITTypeHolderBase& p) const
{
    return !(*this >= p);
}

template class JITTypeHolderBase<void>;
template class JITTypeHolderBase<Recycler>;
