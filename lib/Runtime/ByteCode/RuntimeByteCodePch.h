//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "Parser.h"

#include "Runtime.h"
#include "ByteCode/Symbol.h"
#include "ByteCode/Scope.h"
#include "ByteCode/FuncInfo.h"
#include "ByteCode/ScopeInfo.h"
#include "ByteCode/StatementReader.h"

#include "ByteCode/ByteCodeDumper.h"
#include "ByteCode/ByteCodeWriter.h"
#include "ByteCode/AsmJsByteCodeWriter.h"
#include "ByteCode/ByteCodeGenerator.h"

#include "ByteCode/OpCodeUtilAsmJs.h"
#include "Language/AsmJsTypes.h"

#include "ByteCode/ByteCodeApi.h"
#include "ByteCode/BackendOpCodeAttr.h"
#ifdef ENABLE_WASM
#include "WasmReader.h"
#endif
#include "Language/JavascriptStackWalker.h"
