//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{

    class WasmElementSegment
    {
    public:
        WasmElementSegment(ArenaAllocator* alloc, const uint32 index, const WasmNode initExpr, const uint32 numElem);
        void AddElement(const uint32 funcIndex);
        uint32 GetElement(const uint32 tableIndex) const;
        uint32 GetNumElements() const { return m_numElem; }
        WasmNode GetOffsetExpr() const { return m_offsetExpr; }
    private:
        ArenaAllocator* m_alloc;
        uint32 m_index;
        const WasmNode m_offsetExpr;
        uint32 m_numElem;
        uint32 m_offset;
        uint32 m_elemIdx;
        uint32* m_elems;

        void Init();
    };
} // Namespace Wasm
