//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

int const TySize[] = {
#define IRTYPE(ucname, baseType, size, bitSize, enRegOk, dname) size,
#include "IRTypeList.h"
#undef IRTYPE
};

enum IRBaseTypes : BYTE {
    IRBaseType_Illegal,
    IRBaseType_Int,
    IRBaseType_Uint,
    IRBaseType_Float,
    IRBaseType_Simd,
    IRBaseType_Var,
    IRBaseType_Condcode,
    IRBaseType_Misc
};

int const TyBaseType[] = {
#define IRTYPE(ucname, baseType, size, bitSize, enRegOk, dname) IRBaseType_ ## baseType,
#include "IRTypeList.h"
#undef IRTYPE
};

const char16 * const TyDumpName[] = {
#define IRTYPE(ucname, baseType, size, bitSize, enRegOk, dname) _u(#dname),
#include "IRTypeList.h"
#undef IRTYPE
};

bool IRType_IsSignedInt(IRType type) { return TyBaseType[type] == IRBaseType_Int; }
bool IRType_IsUnsignedInt(IRType type) { return TyBaseType[type] == IRBaseType_Uint; }
bool IRType_IsFloat(IRType type) { return TyBaseType[type] == IRBaseType_Float; }
bool IRType_IsNative(IRType type)
{
    return TyBaseType[type] > IRBaseType_Illegal && TyBaseType[type] < IRBaseType_Var;
}
bool IRType_IsNativeInt(IRType type)
{
    return TyBaseType[type] > IRBaseType_Illegal && TyBaseType[type] < IRBaseType_Float;
}
bool IRType_IsInt64(IRType type) { return type == TyInt64 || type == TyUint64; }

bool IRType_IsSimd(IRType type)
{
    return TyBaseType[type] == IRBaseType_Simd;
}

bool IRType_IsSimd128(IRType type)
{
    return type >= TySimd128F4 && type <= TySimd128D2;
}

IRType IRType_EnsureSigned(IRType type)
{
    CompileAssert(TyUint8 > TyInt8);
    CompileAssert((TyUint8 - TyInt8) == (TyUint16 - TyInt16));
    CompileAssert((TyUint8 - TyInt8) == (TyUint32 - TyInt32));
    CompileAssert((TyUint8 - TyInt8) == (TyUint64 - TyInt64));
    if (IRType_IsUnsignedInt(type))
    {
        IRType signedType = (IRType)(type - (TyUint8 - TyInt8));
        Assert(IRType_IsSignedInt(signedType));
        return signedType;
    }
    return type;
}

IRType IRType_EnsureUnsigned(IRType type)
{
    CompileAssert(TyUint8 > TyInt8);
    CompileAssert((TyUint8 - TyInt8) == (TyUint16 - TyInt16));
    CompileAssert((TyUint8 - TyInt8) == (TyUint32 - TyInt32));
    CompileAssert((TyUint8 - TyInt8) == (TyUint64 - TyInt64));
    if (IRType_IsSignedInt(type))
    {
        IRType unsignedType = (IRType)(type + (TyUint8 - TyInt8));
        Assert(IRType_IsUnsignedInt(unsignedType));
        return unsignedType;
    }
    return type;
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
void IRType_Dump(IRType type)
{
    Output::Print(TyDumpName[type]);
}
#endif
