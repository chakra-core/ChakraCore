//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimeConstructorCache::JITTimeConstructorCache(const Js::JavascriptFunction* constructor, Js::ConstructorCache* runtimeCache)
{
    Assert(constructor != nullptr);
    Assert(runtimeCache != nullptr);
    m_data.runtimeCacheAddr = (intptr_t)runtimeCache;
    m_data.runtimeCacheGuardAddr = (intptr_t)runtimeCache->GetAddressOfGuardValue();
    m_data.slotCount = runtimeCache->content.slotCount;
    m_data.inlineSlotCount = runtimeCache->content.inlineSlotCount;
    m_data.skipNewScObject = runtimeCache->content.skipDefaultNewObject;
    m_data.ctorHasNoExplicitReturnValue = runtimeCache->content.ctorHasNoExplicitReturnValue;
    m_data.typeIsFinal = runtimeCache->content.typeIsFinal;
    m_data.isUsed = false;

    if (runtimeCache->IsNormal())
    {
        JITType::BuildFromJsType(runtimeCache->content.type, (JITType*)&m_data.type);
    }
}

JITTimeConstructorCache::JITTimeConstructorCache(const JITTimeConstructorCache* other)
{
    Assert(other != nullptr);
    Assert(other->GetRuntimeCacheAddr() != 0);
    m_data.runtimeCacheAddr = other->GetRuntimeCacheAddr();
    m_data.runtimeCacheGuardAddr = other->GetRuntimeCacheGuardAddr();
    m_data.type = *(TypeIDL*)other->GetType();
    m_data.slotCount = other->GetSlotCount();
    m_data.inlineSlotCount = other->GetInlineSlotCount();
    m_data.skipNewScObject = other->SkipNewScObject();
    m_data.ctorHasNoExplicitReturnValue = other->CtorHasNoExplicitReturnValue();
    m_data.typeIsFinal = other->IsTypeFinal();
    m_data.isUsed = false;
}

JITTimeConstructorCache*
JITTimeConstructorCache::Clone(JitArenaAllocator* allocator) const
{
    JITTimeConstructorCache* clone = Anew(allocator, JITTimeConstructorCache, this);
    return clone;
}
const BVSparse<JitArenaAllocator>*
JITTimeConstructorCache::GetGuardedPropOps() const
{
    return m_guardedPropOps;
}

void
JITTimeConstructorCache::EnsureGuardedPropOps(JitArenaAllocator* allocator)
{
    if (m_guardedPropOps == nullptr)
    {
        m_guardedPropOps = Anew(allocator, BVSparse<JitArenaAllocator>, allocator);
    }
}

void
JITTimeConstructorCache::SetGuardedPropOp(uint propOpId)
{
    Assert(m_guardedPropOps != nullptr);
    m_guardedPropOps->Set(propOpId);
}

void
JITTimeConstructorCache::AddGuardedPropOps(const BVSparse<JitArenaAllocator>* propOps)
{
    Assert(m_guardedPropOps != nullptr);
    m_guardedPropOps->Or(propOps);
}

intptr_t
JITTimeConstructorCache::GetRuntimeCacheAddr() const
{
    return m_data.runtimeCacheAddr;
}

intptr_t
JITTimeConstructorCache::GetRuntimeCacheGuardAddr() const
{
    return m_data.runtimeCacheGuardAddr;
}

JITType *
JITTimeConstructorCache::GetType() const
{
    return (JITType*)&m_data.type;
}

int
JITTimeConstructorCache::GetSlotCount() const
{
    return m_data.slotCount;
}

int16
JITTimeConstructorCache::GetInlineSlotCount() const
{
    return m_data.inlineSlotCount;
}

bool
JITTimeConstructorCache::SkipNewScObject() const
{
    return m_data.skipNewScObject != FALSE;
}

bool
JITTimeConstructorCache::CtorHasNoExplicitReturnValue() const
{
    return m_data.ctorHasNoExplicitReturnValue != FALSE;
}

bool
JITTimeConstructorCache::IsTypeFinal() const
{
    return m_data.typeIsFinal != FALSE;
}

bool
JITTimeConstructorCache::IsUsed() const
{
    return m_data.isUsed != FALSE;
}

// TODO: OOP JIT, does this need to flow back?
void
JITTimeConstructorCache::SetUsed(bool val)
{
    m_data.isUsed = val;
}

JITTimeConstructorCacheIDL *
JITTimeConstructorCache::GetData()
{
    return &m_data;
}

