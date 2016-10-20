//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
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
WasmDataSegment::getDestAddr(Js::WebAssemblyModule* module) const
{
    if (initExpr.op == wbI32Const)
    {
        return initExpr.cnst.i32;
    }
    if (initExpr.var.num >= (uint) module->globals->Count())
    {
        throw WasmCompilationException(_u("global %d doesn't exist"), initExpr.var.num);
    }
    WasmGlobal* global = module->globals->Item(initExpr.var.num);
    Assert(global->GetReferenceType() == WasmGlobal::Const);

    if (global->GetType() != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("global %d must be i32"), initExpr.var.num);
    }
    return global->cnst.i32;
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
