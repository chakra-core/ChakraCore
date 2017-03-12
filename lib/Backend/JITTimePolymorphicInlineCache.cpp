//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

JITTimePolymorphicInlineCache::JITTimePolymorphicInlineCache()
{
    CompileAssert(sizeof(JITTimePolymorphicInlineCache) == sizeof(PolymorphicInlineCacheIDL));
}

intptr_t
JITTimePolymorphicInlineCache::GetAddr() const
{
    return reinterpret_cast<intptr_t>(PointerValue(m_data.addr));
}

intptr_t
JITTimePolymorphicInlineCache::GetInlineCachesAddr() const
{
    return m_data.inlineCachesAddr;
}

uint16
JITTimePolymorphicInlineCache::GetSize() const
{
    return m_data.size;
}

