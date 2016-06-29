//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Common.h"

#include "Runtime.h"
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
#define DO_WASM_TRACE_DECODER   DO_WASM_TRACE_ALL || PHASE_TRACE1(Js::WasmReaderPhase)
  #define DO_WASM_TRACE_SECTION DO_WASM_TRACE_DECODER || PHASE_TRACE1(Js::WasmSectionPhase)
#define DO_WASM_TRACE_LEB128    DO_WASM_TRACE_ALL || PHASE_TRACE1(Js::WasmLEB128Phase)

#define TRACE_WASM_DECODER(...) TRACE_WASM(DO_WASM_TRACE_DECODER, __VA_ARGS__)
#define TRACE_WASM_SECTION(...) TRACE_WASM(DO_WASM_TRACE_SECTION, __VA_ARGS__)
#define TRACE_WASM_LEB128(...) TRACE_WASM(DO_WASM_TRACE_LEB128, __VA_ARGS__)
#else
#define DO_WASM_TRACE_ALL
#define DO_WASM_TRACE_DECODER
#define DO_WASM_TRACE_SECTION
#define DO_WASM_TRACE_LEB128

#define TRACE_WASM(...)
#define TRACE_WASM_DECODER(...)
#define TRACE_WASM_SECTION(...)
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
    struct Local
    {
        WasmTypes::WasmType t;
        char16* name;

        Local(WasmTypes::WasmType _t) : t(_t), name(nullptr) {}
        Local() : t(WasmTypes::Limit), name(nullptr) {}
    };

    typedef JsUtil::GrowingArray<Local, ArenaAllocator> WasmTypeArray;
}

#include "WasmSignature.h"
#include "WasmDataSegment.h"
#include "WasmFunctionInfo.h"
#include "ModuleInfo.h"

#include "WasmSection.h"

#include "WasmBinaryReader.h"
#include "WasmRegisterSpace.h"
#include "WasmBytecodeGenerator.h"

// TODO (michhol): cleanup includes
#include "Bytecode/AsmJsByteCodeWriter.h"
#include "Bytecode/ByteCodeDumper.h"
#include "Bytecode/AsmJsByteCodeDumper.h"
#include "Language/AsmJSTypes.h"

#endif // ENABLE_WASM
