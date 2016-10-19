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
    WasmDataSegment(ArenaAllocator * alloc, WasmNode initExpr, uint32 _source_size, byte* _data);
    uint32 getDestAddr(WasmModule* module) const;
    uint32 getSourceSize() const;
    byte* getData() const;

private:
    ArenaAllocator * m_alloc;
    WasmNode initExpr;
    uint32 source_size;
    byte* data;
};

} // namespace Wasm
