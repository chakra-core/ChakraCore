//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{
    WasmElementSegment::WasmElementSegment(ArenaAllocator * alloc, const UINT32 index, const WasmNode initExpr, const UINT32 numElem) :
        m_alloc(alloc),
        m_index(index),
        m_offsetExpr(initExpr),
        m_numElem(numElem),
        m_offset(0),
        m_elemIdx(0),
        m_elems(nullptr)
    {}

    void
    WasmElementSegment::Init(const Js::WebAssemblyModule& module)
    {
        Assert(m_numElem > 0);
        m_elems = AnewArray(m_alloc, UINT32, m_numElem);
        memset(m_elems, Js::Constants::UninitializedValue, m_numElem * sizeof(UINT32));
    }

    void
    WasmElementSegment::AddElement(const UINT32 funcIndex, const Js::WebAssemblyModule& module)
    {
        if (m_elems == nullptr)
        {
            Init(module);
        }
        Assert(m_elemIdx < m_numElem);
        m_elems[m_elemIdx++] = funcIndex;
    }

    UINT32
    WasmElementSegment::GetElement(const UINT32 tableIndex) const
    {
        return m_elems[tableIndex];
    }

    uint32 WasmElementSegment::GetDestAddr(Js::WebAssemblyModule* module) const
    {
        return module->GetOffsetFromInit(m_offsetExpr);
    }

} // namespace Wasm

#endif // ENABLE_WASM
