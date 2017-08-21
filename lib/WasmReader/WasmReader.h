//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Common.h"

#include "Runtime.h"
#include "Language/WAsmjsUtils.h"
#ifdef ENABLE_WASM
#if ENABLE_DEBUG_CONFIG_OPTIONS
#define TRACE_WASM(condition, ...) \
    if (condition)\
    {\
        Output::Print(__VA_ARGS__); \
        Output::Print(_u("\n")); \
        Output::Flush(); \
    }

// Level of tracing
#define DO_WASM_TRACE_ALL       PHASE_TRACE1(Js::WasmPhase)
#define DO_WASM_TRACE_BYTECODE  DO_WASM_TRACE_ALL || PHASE_TRACE(Js::WasmBytecodePhase, GetFunctionBody())
#define DO_WASM_TRACE_DECODER   DO_WASM_TRACE_ALL || PHASE_TRACE1(Js::WasmReaderPhase)
  #define DO_WASM_TRACE_SECTION DO_WASM_TRACE_DECODER || PHASE_TRACE1(Js::WasmSectionPhase)
#define DO_WASM_TRACE_LEB128    PHASE_TRACE1(Js::WasmLEB128Phase)
#else
#define TRACE_WASM(...)
#define DO_WASM_TRACE_ALL (false)
#define DO_WASM_TRACE_BYTECODE (false)
#define DO_WASM_TRACE_DECODER (false)
#define DO_WASM_TRACE_SECTION (false)
#define DO_WASM_TRACE_LEB128 (false)
#endif
#define TRACE_WASM_BYTECODE(...) TRACE_WASM(DO_WASM_TRACE_BYTECODE, __VA_ARGS__)
#define TRACE_WASM_DECODER(...) TRACE_WASM(DO_WASM_TRACE_DECODER, __VA_ARGS__)
#define TRACE_WASM_SECTION(...) TRACE_WASM(DO_WASM_TRACE_SECTION, __VA_ARGS__)
#define TRACE_WASM_LEB128(...) TRACE_WASM(DO_WASM_TRACE_LEB128, __VA_ARGS__)

namespace Wasm
{
    // forward declarations
    struct WasmNode;
    struct SExprParseContext;
    class WasmFunctionInfo;
}

#include "WasmParseTree.h"

namespace Wasm
{
    typedef WasmTypes::WasmType Local;
}

#include "WasmReaderBase.h"
#include "WasmSignature.h"
#include "WasmDataSegment.h"
#include "WasmElementSegment.h"
#include "WasmFunctionInfo.h"

#include "WasmSection.h"

#include "WasmBinaryReader.h"
#include "WasmCustomReader.h"
#include "WasmByteCodeGenerator.h"

#include "WasmGlobal.h"

// TODO (michhol): cleanup includes
#include "ByteCode/ByteCodeDumper.h"
#include "ByteCode/AsmJsByteCodeDumper.h"
#include "Language/AsmJsTypes.h"

#endif // ENABLE_WASM
