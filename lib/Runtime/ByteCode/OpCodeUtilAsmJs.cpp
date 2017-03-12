//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

#ifdef ASMJS_PLAT
namespace Js
{
#if DBG_DUMP || ENABLE_DEBUG_CONFIG_OPTIONS
    char16 const * const OpCodeUtilAsmJs::OpCodeAsmJsNames[] =
    {
#define DEF_OP(x, y, ...) _u("") STRINGIZEW(x) _u(""),
#include "OpCodeListAsmJs.h"
#undef DEF_OP
    };

    char16 const * const OpCodeUtilAsmJs::ExtendedOpCodeAsmJsNames[] =
    {
#define DEF_OP(x, y, ...) _u("") STRINGIZEW(x) _u(""),
#include "ExtendedOpCodeListAsmJs.h"
#undef DEF_OP
    };

    char16 const * OpCodeUtilAsmJs::GetOpCodeName(OpCodeAsmJs op)
    {
        if (op <= Js::OpCodeAsmJs::MaxByteSizedOpcodes)
        {
            Assert(op < _countof(OpCodeAsmJsNames));
            __analysis_assume(op < _countof(OpCodeAsmJsNames));
            return OpCodeAsmJsNames[(int)op];
        }
        else if (op < Js::OpCodeAsmJs::ByteCodeLast)
        {
            uint opIndex = op - (Js::OpCodeAsmJs::MaxByteSizedOpcodes + 1);
            Assert(opIndex < _countof(ExtendedOpCodeAsmJsNames));
            __analysis_assume(opIndex < _countof(ExtendedOpCodeAsmJsNames));
            return ExtendedOpCodeAsmJsNames[opIndex];
        }
        return _u("<NotAvail>");
    }

#else
    wchar const * OpCodeUtilAsmJs::GetOpCodeName(OpCodeAsmJs op)
    {
        return _u("<NotAvail>");
    }
#endif

    OpLayoutTypeAsmJs const OpCodeUtilAsmJs::OpCodeAsmJsLayouts[] =
    {
#define DEF_OP(x, y, ...) OpLayoutTypeAsmJs::y,
#include "OpCodeListAsmJs.h"
    };

    OpLayoutTypeAsmJs const OpCodeUtilAsmJs::ExtendedOpCodeAsmJsLayouts[] =
    {
#define DEF_OP(x, y, ...) OpLayoutTypeAsmJs::y,
#include "ExtendedOpCodeListAsmJs.h"
    };

    OpLayoutTypeAsmJs OpCodeUtilAsmJs::GetOpCodeLayout(OpCodeAsmJs op)
    {
        if ((uint)op <= (uint)Js::OpCodeAsmJs::MaxByteSizedOpcodes)
        {
            Assert(op < _countof(OpCodeAsmJsLayouts));
            __analysis_assume(op < _countof(OpCodeAsmJsLayouts));
            return OpCodeAsmJsLayouts[(uint)op];
        }

        uint opIndex = op - (Js::OpCodeAsmJs::MaxByteSizedOpcodes + 1);
        Assert(opIndex < _countof(ExtendedOpCodeAsmJsLayouts));
        __analysis_assume(opIndex < _countof(ExtendedOpCodeAsmJsLayouts));
        return ExtendedOpCodeAsmJsLayouts[opIndex];
    }

    bool OpCodeUtilAsmJs::IsValidByteCodeOpcode(OpCodeAsmJs op)
    {
        // These OpCodes must have the same value for asm.js and normal javascript.
        // This CompileAssert will make sure to update both lists if any changes are made
        CompileAssert((uint)OpCodeAsmJs::EndOfBlock                 == (uint)OpCode::EndOfBlock);
        CompileAssert((uint)OpCodeAsmJs::ExtendedOpcodePrefix       == (uint)OpCode::ExtendedOpcodePrefix);
        CompileAssert((uint)OpCodeAsmJs::MediumLayoutPrefix         == (uint)OpCode::MediumLayoutPrefix);
        CompileAssert((uint)OpCodeAsmJs::ExtendedMediumLayoutPrefix == (uint)OpCode::ExtendedMediumLayoutPrefix);
        CompileAssert((uint)OpCodeAsmJs::LargeLayoutPrefix          == (uint)OpCode::LargeLayoutPrefix);
        CompileAssert((uint)OpCodeAsmJs::ExtendedLargeLayoutPrefix  == (uint)OpCode::ExtendedLargeLayoutPrefix);
        CompileAssert((uint)OpCodeAsmJs::Nop                        == (uint)OpCode::Nop);
        CompileAssert((uint)OpCodeAsmJs::InvalidOpCode              == (uint)OpCode::InvalidOpCode);

        CompileAssert((int)Js::OpCodeAsmJs::MaxByteSizedOpcodes + 1 + _countof(OpCodeUtilAsmJs::ExtendedOpCodeAsmJsLayouts) == (int)Js::OpCodeAsmJs::ByteCodeLast);
        return op < _countof(OpCodeAsmJsLayouts)
            || (op > Js::OpCodeAsmJs::MaxByteSizedOpcodes && op < Js::OpCodeAsmJs::ByteCodeLast);
    }

    bool OpCodeUtilAsmJs::IsValidOpcode(OpCodeAsmJs op)
    {
        return IsValidByteCodeOpcode(op)
            || (op > Js::OpCodeAsmJs::ByteCodeLast && op < Js::OpCodeAsmJs::Count);
    }
};
#endif
