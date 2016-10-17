//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
class OpCodeUtil
{
public:
    static char16 const * GetOpCodeName(OpCode op);

    static bool IsCallOp(OpCode op);
    static bool IsProfiledCallOp(OpCode op);
    static bool IsProfiledCallOpWithICIndex(OpCode op);
    static bool IsProfiledConstructorCall(OpCode op);
    static bool IsProfiledReturnTypeCallOp(OpCode op);

    // OpCode conversion functions
    static void ConvertOpToNonProfiled(OpCode& op);
    static void ConvertNonCallOpToProfiled(OpCode& op);
    static void ConvertNonCallOpToProfiledWithICIndex(OpCode& op);
    static void ConvertNonCallOpToNonProfiled(OpCode& op);
    static void ConvertNonCallOpToNonProfiledWithICIndex(OpCode& op);

    static OpCode ConvertProfiledCallOpToNonProfiled(OpCode op);
    static OpCode ConvertProfiledReturnTypeCallOpToNonProfiled(OpCode op);
    static OpCode ConvertCallOpToProfiled(OpCode op, bool withICIndex = false);
    static OpCode ConvertCallOpToProfiledReturnType(OpCode op);

    static bool IsValidByteCodeOpcode(OpCode op);
    static bool IsValidOpcode(OpCode op);
    static bool IsPrefixOpcode(OpCode op);
    static const bool IsSmallEncodedOpcode(OpCode op)
    {
        return op <= Js::OpCode::MaxByteSizedOpcodes;
    }
    static const uint EncodedSize(OpCode op, LayoutSize layoutSize)
    {
        // Simple case, only 1 byte
        // Small OpCode with Medium or Large layout: 1 extra byte for the prefix
        // Extended OpCode: Prefix + 2 bytes for the opcode
        return IsSmallEncodedOpcode(op) ? layoutSize == SmallLayout ? 1 : 2 : 3;
    }

    static OpLayoutType GetOpCodeLayout(OpCode op);
private:
#if DBG_DUMP || ENABLE_DEBUG_CONFIG_OPTIONS
    static char16 const * const OpCodeNames[(int)Js::OpCode::MaxByteSizedOpcodes + 1];
    static char16 const * const ExtendedOpCodeNames[];
    static char16 const * const BackendOpCodeNames[];
#endif
    static OpLayoutType const OpCodeLayouts[];
    static OpLayoutType const ExtendedOpCodeLayouts[];
    static OpLayoutType const BackendOpCodeLayouts[];
#if DBG
    static OpCode DebugConvertProfiledCallToNonProfiled(OpCode op);
    static OpCode DebugConvertProfiledReturnTypeCallToNonProfiled(OpCode op);
#endif
};
};
