//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class ModuleInfo
{
private:
    struct Memory
    {
        Memory() : minSize(0)
        {
        }
        size_t minSize;
        size_t maxSize;
        bool exported;
    } m_memory;
public:
    ModuleInfo();

    bool InitializeMemory(size_t minSize, size_t maxSize, bool exported);

    const Memory * GetMemory() const;
};

} // namespace Wasm
