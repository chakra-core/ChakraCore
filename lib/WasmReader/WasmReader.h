//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Common.h"

#include "Runtime.h"

#ifdef ENABLE_WASM

namespace Wasm
{
    // forward declarations
    struct WasmNode;
    struct SExprParseContext;
    class WasmFunctionInfo;
}

#include "ByteCode\ByteCodeWriter.h"
#include "ByteCode\AsmJsByteCodeWriter.h"

#include "WasmParseTree.h"
#include "WasmFunctionInfo.h"

#include "BaseWasmReader.h"

#include "SExprScanner.h"
#include "SExprParser.h"
#include "WasmBinaryReader.h"
#include "WasmRegisterSpace.h"
#include "WasmBytecodeGenerator.h"

#endif // ENABLE_WASM
