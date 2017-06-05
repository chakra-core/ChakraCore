//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{

class WasmDataSegment
{
public:
    WasmDataSegment(ArenaAllocator* alloc, WasmNode initExpr, uint32 _source_size, const byte* _data);
    WasmNode GetOffsetExpr() const { return m_initExpr; }
    uint32 GetSourceSize() const;
    const byte* GetData() const;

private:
    ArenaAllocator* m_alloc;
    WasmNode m_initExpr;
    uint32 m_sourceSize;
    const byte* m_data;
};

} // namespace Wasm
