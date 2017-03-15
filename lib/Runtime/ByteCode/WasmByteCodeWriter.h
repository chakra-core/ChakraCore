//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
#include "ByteCodeWriter.h"
#include "IWasmByteCodeWriter.h"

struct WasmByteCodeWriter;

#define WASM_BYTECODE_WRITER
#define AsmJsByteCodeWriter WasmByteCodeWriter
#include "ByteCode/AsmJsByteCodeWriter.h"
#undef WASM_BYTECODE_WRITER
#undef AsmJsByteCodeWriter

#endif
