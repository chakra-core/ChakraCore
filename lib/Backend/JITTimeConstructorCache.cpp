//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

CompileAssert(sizeof(JITTimeConstructorCache) == sizeof(JITTimeConstructorCacheIDL));

JITTimeConstructorCache::JITTimeConstructorCache(const Js::JavascriptFunction* constructor, Js::ConstructorCache* runtimeCache)
{
    Assert(constructor != nullptr);
    Assert(runtimeCache != nullptr);
    m_data.runtimeCacheAddr = runtimeCache;
    m_data.runtimeCacheGuardAddr = const_cast<void*>(runtimeCache->GetAddressOfGuardValue());
    m_data.slotCount = runtimeCache->content.slotCount;
    m_data.inlineSlotCount = runtimeCache->content.inlineSlotCount;
    m_data.skipNewScObject = runtimeCache->content.skipDefaultNewObject;
    m_data.ctorHasNoExplicitReturnValue = runtimeCache->content.ctorHasNoExplicitReturnValue;
    m_data.typeIsFinal = runtimeCache->content.typeIsFinal;
    m_data.isUsed = false;
    m_data.guardedPropOps = 0;
    if (runtimeCache->IsNormal())
    {
        JITType::BuildFromJsType(runtimeCache->content.type, (JITType*)&m_data.type);
    }
}

JITTimeConstructorCache::JITTimeConstructorCache(const JITTimeConstructorCache* other)
{
    Assert(other != nullptr);
    Assert(other->GetRuntimeCacheAddr() != 0);
    m_data.runtimeCacheAddr = reinterpret_cast<void*>(other->GetRuntimeCacheAddr());
    m_data.runtimeCacheGuardAddr = reinterpret_cast<void*>(other->GetRuntimeCacheGuardAddr());
    m_data.type = *(TypeIDL*)PointerValue(other->GetType().t);
    m_data.slotCount = other->GetSlotCount();
    m_data.inlineSlotCount = other->GetInlineSlotCount();
    m_data.skipNewScObject = other->SkipNewScObject();
    m_data.ctorHasNoExplicitReturnValue = other->CtorHasNoExplicitReturnValue();
    m_data.typeIsFinal = other->IsTypeFinal();
    m_data.isUsed = false;
    m_data.guardedPropOps = 0; // REVIEW: OOP JIT should we copy these when cloning?
}

JITTimeConstructorCache*
JITTimeConstructorCache::Clone(JitArenaAllocator* allocator) const
{
    JITTimeConstructorCache* clone = Anew(allocator, JITTimeConstructorCache, this);
    return clone;
}
BVSparse<JitArenaAllocator>*
JITTimeConstructorCache::GetGuardedPropOps() const
{
    return (BVSparse<JitArenaAllocator>*)(m_data.guardedPropOps & ~(intptr_t)1);
}

void
JITTimeConstructorCache::EnsureGuardedPropOps(JitArenaAllocator* allocator)
{
    if (GetGuardedPropOps() == nullptr)
    {
        m_data.guardedPropOps = (intptr_t)Anew(allocator, BVSparse<JitArenaAllocator>, allocator);
        m_data.guardedPropOps |= 1; // tag it to prevent false positive after the arena address reuse in recycler
    }
}

void
JITTimeConstructorCache::SetGuardedPropOp(uint propOpId)
{
    Assert(GetGuardedPropOps() != nullptr);
    GetGuardedPropOps()->Set(propOpId);
}

void
JITTimeConstructorCache::AddGuardedPropOps(const BVSparse<JitArenaAllocator>* propOps)
{
    Assert(GetGuardedPropOps() != nullptr);
    GetGuardedPropOps()->Or(propOps);
}

intptr_t
JITTimeConstructorCache::GetRuntimeCacheAddr() const
{
    return reinterpret_cast<intptr_t>(PointerValue(m_data.runtimeCacheAddr));
}

intptr_t
JITTimeConstructorCache::GetRuntimeCacheGuardAddr() const
{
    return reinterpret_cast<intptr_t>(PointerValue(m_data.runtimeCacheGuardAddr));
}

JITTypeHolder
JITTimeConstructorCache::GetType() const
{
    return JITTypeHolder((JITType*)&m_data.type);
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

