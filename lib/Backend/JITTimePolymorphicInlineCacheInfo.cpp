//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimePolymorphicInlineCacheInfo::JITTimePolymorphicInlineCacheInfo()
{
    CompileAssert(sizeof(JITTimePolymorphicInlineCacheInfo) == sizeof(PolymorphicInlineCacheInfoIDL));
}

/* static */
void
JITTimePolymorphicInlineCacheInfo::InitializeEntryPointPolymorphicInlineCacheInfo(
    __in Recycler * recycler,
    __in Js::EntryPointPolymorphicInlineCacheInfo * runtimeInfo,
    __inout EntryPointPolymorphicInlineCacheInfoIDL * jitInfo)
{
    if (runtimeInfo == nullptr)
    {
        return;
    }
    Js::PolymorphicInlineCacheInfo * selfInfo = runtimeInfo->GetSelfInfo();
    JITTimePolymorphicInlineCacheInfo::InitializePolymorphicInlineCacheInfo(recycler, selfInfo, &jitInfo->selfInfo);

    SListCounted<Js::PolymorphicInlineCacheInfo*, Recycler> * inlineeList = runtimeInfo->GetInlineeInfo();
    jitInfo->inlineeInfoCount = inlineeList->Count();
    if (!inlineeList->Empty())
    {
        jitInfo->inlineeInfo = RecyclerNewArray(recycler, PolymorphicInlineCacheInfoIDL, inlineeList->Count());
        SListCounted<Js::PolymorphicInlineCacheInfo*, Recycler>::Iterator iter(inlineeList);
        uint i = 0;
        while (iter.Next())
        {
            Js::PolymorphicInlineCacheInfo * inlineeInfo = iter.Data();
            JITTimePolymorphicInlineCacheInfo::InitializePolymorphicInlineCacheInfo(recycler, inlineeInfo, &jitInfo->inlineeInfo[i]);
            ++i;
        }
        Assert(i == inlineeList->Count());
    }
}

/* static */
void
JITTimePolymorphicInlineCacheInfo::InitializePolymorphicInlineCacheInfo(
    __in Recycler * recycler,
    __in Js::PolymorphicInlineCacheInfo * runtimeInfo,
    __inout PolymorphicInlineCacheInfoIDL * jitInfo)
{
    jitInfo->polymorphicCacheUtilizationArray = runtimeInfo->GetUtilByteArray();
    jitInfo->functionBodyAddr = (intptr_t)runtimeInfo->GetFunctionBody();

    if (runtimeInfo->GetPolymorphicInlineCaches()->HasInlineCaches())
    {
        jitInfo->polymorphicInlineCacheCount = runtimeInfo->GetFunctionBody()->GetInlineCacheCount();
        jitInfo->polymorphicInlineCaches = RecyclerNewArrayZ(recycler, PolymorphicInlineCacheIDL*, jitInfo->polymorphicInlineCacheCount);
        for (uint j = 0; j < jitInfo->polymorphicInlineCacheCount; ++j)
        {
            Js::PolymorphicInlineCache * pic = runtimeInfo->GetPolymorphicInlineCaches()->GetInlineCache(j);
            if (pic != nullptr)
            {
                jitInfo->polymorphicInlineCaches[j] = RecyclerNewStructLeaf(recycler, PolymorphicInlineCacheIDL);
                jitInfo->polymorphicInlineCaches[j]->size = pic->GetSize();
                jitInfo->polymorphicInlineCaches[j]->addr = (intptr_t)pic;
                jitInfo->polymorphicInlineCaches[j]->inlineCachesAddr = (intptr_t)pic->GetInlineCaches();
            }
        }
    }
}

JITTimePolymorphicInlineCache *
JITTimePolymorphicInlineCacheInfo::GetInlineCache(uint index) const
{
    Assert(index < m_data.polymorphicInlineCacheCount);
    return (JITTimePolymorphicInlineCache *)m_data.polymorphicInlineCaches[index];
}

bool
JITTimePolymorphicInlineCacheInfo::HasInlineCaches() const
{
    return m_data.polymorphicInlineCacheCount != 0;
}

byte
JITTimePolymorphicInlineCacheInfo::GetUtil(uint index) const
{
    Assert(index < m_data.polymorphicInlineCacheCount);
    return m_data.polymorphicCacheUtilizationArray[index];
}

