//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

// Make sure all basic opcode with no one byte layout fits in a byte.
#define MACRO(opcode, layout, attr) CompileAssert((uint)Js::OpCode::opcode <= BYTE_MAX);
#define MACRO_WMS(opcode, layout, attr) CompileAssert((uint)Js::OpCode::opcode <= BYTE_MAX);

// Make sure all extended opcode needs two bytes.
#define MACRO_EXTEND(opcode, layout, attr) CompileAssert((uint)Js::OpCode::opcode > BYTE_MAX);
#define MACRO_EXTEND_WMS(opcode, layout, attr)  CompileAssert((uint)Js::OpCode::opcode > BYTE_MAX);

#include "OpCodes.h"

#ifdef ASMJS_PLAT
// Make sure all basic opcode with no one byte layout fits in a byte.
#define MACRO(opcode, layout, attr) CompileAssert((uint)Js::OpCodeAsmJs::opcode <= BYTE_MAX);
#define MACRO_WMS(opcode, layout, attr) CompileAssert((uint)Js::OpCodeAsmJs::opcode <= BYTE_MAX);

// Make sure all extended opcode needs two bytes.
#define MACRO_EXTEND(opcode, layout, attr) CompileAssert((uint)Js::OpCodeAsmJs::opcode > BYTE_MAX);
#define MACRO_EXTEND_WMS(opcode, layout, attr)  CompileAssert((uint)Js::OpCodeAsmJs::opcode > BYTE_MAX);

#include "OpCodesAsmJs.h"
#endif
