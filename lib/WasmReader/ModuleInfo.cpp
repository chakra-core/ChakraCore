//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

namespace Wasm
{

ModuleInfo::ModuleInfo() :
    m_memory()
{
}

bool
ModuleInfo::InitializeMemory(size_t minSize, size_t maxSize, bool exported)
{
    if (m_memory.minSize != 0)
    {
        return false;
    }

    if (maxSize < minSize)
    {
        return false;
    }

    if (minSize == 0 || minSize % AutoSystemInfo::PageSize != 0)
    {
        return false;
    }

    if (maxSize % AutoSystemInfo::PageSize != 0)
    {
        return false;
    }

    m_memory.minSize = minSize;
    m_memory.maxSize = maxSize;
    m_memory.exported = exported;

    return true;
}

const ModuleInfo::Memory *
ModuleInfo::GetMemory() const
{
    return &m_memory;
}

} // namespace Wasm
