//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmDataSegment::WasmDataSegment(ArenaAllocator * alloc, WasmNode ie, uint32 _source_size, byte* _data) :
    m_alloc(alloc),
    initExpr(ie),
    source_size(_source_size),
    data(_data)
{
}

uint32
WasmDataSegment::getDestAddr(WasmModule* module) const
{
    return module->GetOffsetFromInit(initExpr);
}

uint32
WasmDataSegment::getSourceSize() const
{
    return source_size;
}

byte*
WasmDataSegment::getData() const
{
    return data;
}

} // namespace Wasm

#endif // ENABLE_WASM
