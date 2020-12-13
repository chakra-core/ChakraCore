//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#ifdef ASMJS_PLAT
namespace OpCodeAttrAsmJs
{
    enum OpCodeAttrEnum
    {
        None = 0,
        OpNoFallThrough = 1 << 0, // Opcode doesn't fallthrough in flow  and its always jump to the return from this opcode.
        OpHasMultiSizeLayout = 1 << 1,
        OpHasProfiled = 1 << 2,
        OpProfiled = 1 << 3
    };

    static const int OpcodeAttributesAsmJs[] =
    {
#define DEF_OP(name, jnLayout, attrib, ...) attrib,
#include "ByteCode/OpCodeListAsmJs.h"
#undef DEF_OP
    };

    static const int ExtendedOpcodeAttributesAsmJs[] =
    {
#define DEF_OP(name, jnLayout, attrib, ...) attrib,
#include "ByteCode/ExtendedOpCodeListAsmJs.h"
#undef DEF_OP
    };


    static const int GetOpCodeAttributes( Js::OpCodeAsmJs op )
    {
        uint opIndex = (uint)op;
        if (opIndex <= (uint)Js::OpCodeAsmJs::MaxByteSizedOpcodes)
        {
            AnalysisAssert(opIndex < _countof(OpcodeAttributesAsmJs));
            return OpcodeAttributesAsmJs[opIndex];
        }
        opIndex -= ( Js::OpCodeAsmJs::MaxByteSizedOpcodes + 1 );
        AnalysisAssert(opIndex < _countof(ExtendedOpcodeAttributesAsmJs));
        return ExtendedOpcodeAttributesAsmJs[opIndex];
    }

#define CheckHasFlag(flag) (!!(GetOpCodeAttributes(opcode) & flag))
#define CheckNoHasFlag(flag) (!(GetOpCodeAttributes(opcode) & flag))


    bool HasFallThrough( Js::OpCodeAsmJs opcode )
    {
        return CheckNoHasFlag( OpNoFallThrough );
    }

    bool HasMultiSizeLayout( Js::OpCodeAsmJs opcode )
    {
        return CheckHasFlag( OpHasMultiSizeLayout );
    }

    bool HasProfiledOp(Js::OpCodeAsmJs opcode)
    {
        return ((GetOpCodeAttributes(opcode) & OpHasProfiled) != 0);
    }
    bool IsProfiledOp(Js::OpCodeAsmJs opcode)
    {
        return ((GetOpCodeAttributes(opcode) & OpProfiled) != 0);
    }

}; // OpCodeAttrAsmJs
#endif
