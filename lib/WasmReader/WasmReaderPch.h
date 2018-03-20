//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// Parser Includes
#include "Common.h"
#include "Runtime.h"
#include "WasmReader.h"

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
#define WASM_TRACE_BODY_CHECK(phase) (GetFunctionBody() ? PHASE_TRACE(phase, GetFunctionBody()) : PHASE_TRACE1(phase))
#define DO_WASM_TRACE_BYTECODE WASM_TRACE_BODY_CHECK(Js::WasmBytecodePhase)
#define DO_WASM_TRACE_DECODER  WASM_TRACE_BODY_CHECK(Js::WasmReaderPhase)
#define DO_WASM_TRACE_SECTION  WASM_TRACE_BODY_CHECK(Js::WasmSectionPhase)
#else
#define TRACE_WASM(...)
#define DO_WASM_TRACE_BYTECODE (false)
#define DO_WASM_TRACE_DECODER (false)
#define DO_WASM_TRACE_SECTION (false)
#endif
#define TRACE_WASM_BYTECODE(...) TRACE_WASM(DO_WASM_TRACE_BYTECODE, __VA_ARGS__)
#define TRACE_WASM_DECODER(...) TRACE_WASM(DO_WASM_TRACE_DECODER, __VA_ARGS__)
#define TRACE_WASM_SECTION(...) TRACE_WASM(DO_WASM_TRACE_SECTION, __VA_ARGS__)

#include "WasmSection.h"
#include "WasmReaderBase.h"
#include "WasmBinaryReader.h"
#include "WasmCustomReader.h"
#include "WasmGlobal.h"
#include "WasmDataSegment.h"
#include "WasmElementSegment.h"
#include "WasmByteCodeGenerator.h"
#include "WasmSectionLimits.h"

#endif
