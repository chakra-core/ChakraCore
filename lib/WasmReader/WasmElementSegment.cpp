//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{
    WasmElementSegment::WasmElementSegment(ArenaAllocator* alloc, const uint32 index, const WasmNode initExpr, const uint32 numElem) :
        m_alloc(alloc),
        m_index(index),
        m_offsetExpr(initExpr),
        m_numElem(numElem),
        m_offset(0),
        m_elemIdx(0),
        m_elems(nullptr)
    {}

    void WasmElementSegment::Init()
    {
        Assert(m_numElem > 0);
        m_elems = AnewArray(m_alloc, uint32, m_numElem);
        memset(m_elems, Js::Constants::UninitializedValue, m_numElem * sizeof(uint32));
    }

    void WasmElementSegment::AddElement(const uint32 funcIndex)
    {
        if (m_elems == nullptr)
        {
            Init();
        }
        Assert(m_elemIdx < m_numElem);
        m_elems[m_elemIdx++] = funcIndex;
    }

    uint32 WasmElementSegment::GetElement(const uint32 tableIndex) const
    {
        Assert(m_elems != nullptr);
        return m_elems[tableIndex];
    }
} // namespace Wasm

#endif // ENABLE_WASM
