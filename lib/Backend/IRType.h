//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum IRType : BYTE
{
#define IRTYPE(Name, BaseType, Bytes, Bits, EnRegOk, DumpName)  Ty ## Name,
#include "IRTypeList.h"
#undef IRTYPE

};

enum IRBaseType : BYTE
{
#define IRBASETYPE(Name, BaseType, Bytes, Align, Bits, EnRegOk, DumpName)  TyBase ## Name,
#include "IRBaseTypeList.h"
#undef IRTYPE

};

extern int const TySize[];

extern bool IRType_IsSignedInt(IRType type);
extern bool IRType_IsUnsignedInt(IRType type);
extern bool IRType_IsFloat(IRType type);
extern bool IRType_IsNative(IRType type);
extern bool IRType_IsNativeInt(IRType type);
extern bool IRType_IsNativeIntOrVar(IRType type);
extern bool IRType_IsInt64(IRType type);
extern bool IRType_IsSimd(IRType type);
extern bool IRType_IsSimd128(IRType type);
extern IRType IRType_EnsureSigned(IRType type);
extern IRType IRType_EnsureUnsigned(IRType type);

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
extern void IRType_Dump(IRType type);
#endif

#ifdef _M_AMD64
    #include "amd64/machvalues.h"
#elif defined(_M_IX86)
    #include "i386/machvalues.h"
#elif defined(_M_ARM)
    #include "arm/machvalues.h"
#elif defined(_M_ARM64)
    #include "arm64/machvalues.h"
#endif

