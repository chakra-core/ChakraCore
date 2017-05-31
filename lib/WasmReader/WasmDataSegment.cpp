//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmDataSegment::WasmDataSegment(ArenaAllocator* alloc, WasmNode ie, uint32 _source_size, const byte* _data) :
    m_alloc(alloc),
    m_initExpr(ie),
    m_sourceSize(_source_size),
    m_data(_data)
{
}

uint32 WasmDataSegment::GetSourceSize() const
{
    return m_sourceSize;
}

const byte* WasmDataSegment::GetData() const
{
    return m_data;
}

} // namespace Wasm

#endif // ENABLE_WASM
