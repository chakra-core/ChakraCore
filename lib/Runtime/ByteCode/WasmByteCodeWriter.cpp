//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

#ifdef ENABLE_WASM
#include "WasmByteCodeWriter.h"

#define WASM_BYTECODE_WRITER
#define AsmJsByteCodeWriter WasmByteCodeWriter
#include "ByteCode/AsmJsByteCodeWriter.cpp"
#undef WASM_BYTECODE_WRITER
#undef AsmJsByteCodeWriter

namespace Js
{
void WasmByteCodeWriter::Create() 
{
    ByteCodeWriter::Create();
}
void WasmByteCodeWriter::End()
{
    ByteCodeWriter::End();
}
void WasmByteCodeWriter::Reset()
{
    ByteCodeWriter::Reset();
}
void WasmByteCodeWriter::Begin(FunctionBody* functionWrite, ArenaAllocator* alloc)
{
    ByteCodeWriter::Begin(functionWrite, alloc, true, true, false);
}
ByteCodeLabel WasmByteCodeWriter::DefineLabel()
{
    return ByteCodeWriter::DefineLabel();
}
}

#endif
