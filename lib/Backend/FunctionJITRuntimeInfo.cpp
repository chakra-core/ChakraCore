//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

FunctionJITRuntimeInfo::FunctionJITRuntimeInfo(FunctionJITRuntimeIDL * data) : m_data(*data)
{
    CompileAssert(sizeof(FunctionJITRuntimeInfo) == sizeof(FunctionJITRuntimeIDL));
}

intptr_t
FunctionJITRuntimeInfo::GetClonedInlineCache(uint index) const
{
    Assert(index < m_data.clonedCacheCount);
    return m_data.clonedInlineCaches[index];
}

bool
FunctionJITRuntimeInfo::HasClonedInlineCaches() const
{
    return m_data.clonedInlineCaches != nullptr;
}
