//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Common.h"

#include "Runtime.h"
#if DBG
#ifdef ENABLE_WASM
#define TRACE_WASM_DECODER_PHASE(phase, ...) \
    if (PHASE_TRACE1(Js::phase##Phase)) \
    {\
        Output::Print(__VA_ARGS__); \
        Output::Print(_u("\n")); \
        Output::Flush(); \
    }
#define TRACE_WASM_DECODER(...) TRACE_WASM_DECODER_PHASE(WasmReader, __VA_ARGS__)
#define TRACE_WASM_DECODER(...) TRACE_WASM_DECODER_PHASE(WasmReader, __VA_ARGS__)
#define TRACE_WASM_LEB128(...) TRACE_WASM_DECODER_PHASE(WasmLEB128, __VA_ARGS__)
#else
#define TRACE_WASM_DECODER_PHASE(...)
#define TRACE_WASM_DECODER(...)
#define TRACE_WASM_DECODER(...)
#define TRACE_WASM_LEB128(...)
#endif

namespace Wasm
{
    // forward declarations
    struct WasmNode;
    struct SExprParseContext;
    class WasmFunctionInfo;
}

#include "ByteCode/ByteCodeWriter.h"
#include "ByteCode/AsmJsByteCodeWriter.h"

#include "WasmParseTree.h"

namespace Wasm
{
    typedef JsUtil::GrowingArray<WasmTypes::WasmType, ArenaAllocator> WasmTypeArray;
}

#include "WasmSignature.h"
#include "WasmDataSegment.h"
#include "ModuleInfo.h"
#include "WasmFunctionInfo.h"

#include "WasmSection.h"
#include "BaseWasmReader.h"

#include "SExprScanner.h"
#include "SExprParser.h"
#include "WasmBinaryReader.h"
#include "WasmRegisterSpace.h"
#include "WasmBytecodeGenerator.h"

// TODO (michhol): cleanup includes
#include "Bytecode/AsmJsByteCodeWriter.h"
#include "Bytecode/ByteCodeDumper.h"
#include "Bytecode/AsmJsByteCodeDumper.h"
#include "Language/AsmJSTypes.h"

#endif // ENABLE_WASM
