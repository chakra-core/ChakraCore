//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// TODO: ARM64: Scalar operations like FADD zero-extend!  FMOV is especially problematic for SIMD callers.

//
// This file contains low-level emitter functions, where each emitter encodes
// exactly one VFP/Neon instruction.
//

#pragma once

enum NEON_SIZE
{
    // scalar sizes, encoded as 0:Q:size
    SIZE_1B = 0,
    SIZE_1H = 1,
    SIZE_1S = 2,
    SIZE_1D = 3,
    SIZE_1Q = 4,

    // vector sizes, encoded as 1:Q:size
    SIZE_8B = 8,
    SIZE_4H = 9,
    SIZE_2S = 10,
    SIZE_16B = 12,
    SIZE_8H = 13,
    SIZE_4S = 14,
    SIZE_2D = 15
};

inline
NEON_SIZE
NeonSize(
    ULONG ElementSizeInBytes,
    ULONG NumElements
    )
{
    switch (ElementSizeInBytes)
    {
    case 1:
        NT_ASSERT(NumElements == 1 || NumElements == 8 || NumElements == 16);
        return (NumElements == 1) ? SIZE_1B : (NumElements == 8) ? SIZE_8B : SIZE_16B;

    case 2:
        NT_ASSERT(NumElements == 1 || NumElements == 4 || NumElements == 8);
        return (NumElements == 1) ? SIZE_1H : (NumElements == 4) ? SIZE_4H : SIZE_8H;

    case 4:
        NT_ASSERT(NumElements == 1 || NumElements == 2 || NumElements == 4);
        return (NumElements == 1) ? SIZE_1S : (NumElements == 2) ? SIZE_2S : SIZE_4S;

    case 8:
        NT_ASSERT(NumElements == 1 || NumElements == 2);
        return (NumElements == 1) ? SIZE_1D : SIZE_2D;

    case 16:
        NT_ASSERT(NumElements == 1);
        return SIZE_1Q;

    default:
        NT_ASSERT(!"Invalid element size passed to NeonSize.");
        return SIZE_1B;
    }
}

inline
NEON_SIZE
NeonSizeScalar(
    ULONG ElementSizeInBytes
    )
{
    return NeonSize(ElementSizeInBytes, 1);
}

inline
NEON_SIZE
NeonSizeFullVector(
    ULONG ElementSizeInBytes
    )
{
    return NeonSize(ElementSizeInBytes, 16 / ElementSizeInBytes);
}

inline
NEON_SIZE
NeonSizeHalfVector(
    ULONG ElementSizeInBytes
    )
{
    return NeonSize(ElementSizeInBytes, 8 / ElementSizeInBytes);
}

#define VALID_1B (1 << SIZE_1B)
#define VALID_1H (1 << SIZE_1H)
#define VALID_1S (1 << SIZE_1S)
#define VALID_1D (1 << SIZE_1D)
#define VALID_1Q (1 << SIZE_1Q)

#define VALID_8B (1 << SIZE_8B)
#define VALID_16B (1 << SIZE_16B)
#define VALID_4H (1 << SIZE_4H)
#define VALID_8H (1 << SIZE_8H)
#define VALID_2S (1 << SIZE_2S)
#define VALID_4S (1 << SIZE_4S)
#define VALID_2D (1 << SIZE_2D)

#define VALID_816B (VALID_8B | VALID_16B)
#define VALID_48H (VALID_4H | VALID_8H)
#define VALID_24S (VALID_2S | VALID_4S)

inline
bool
NeonSizeIsValid(
    NEON_SIZE Valid,
    ULONG ValidMask
    )
{
    return ((ValidMask >> int(Valid)) & 1) ? true : false;
}

inline
bool
NeonSizeIsScalar(
    NEON_SIZE Size
    )
{
    return (Size >= SIZE_1B && Size <= SIZE_1Q);
}

inline
bool
NeonSizeIsVector(
    NEON_SIZE Size
    )
{
    return (Size >= SIZE_8B && Size <= SIZE_2D);
}

inline
int
NeonElementSize(
    NEON_SIZE Size
    )
{
    return NeonSizeIsScalar(Size) ? Size : (Size & 3);
}

//
// Binary integer operations
//

inline
int
EmitNeonBinaryCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 3) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 3) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonAbs(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e20b800, 0x5e20b800);
}

inline
int
EmitNeonAddp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(SrcSize == SIZE_1D);
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x5e31b800);
}

inline
int
EmitNeonAddv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e31b800);
}

inline
int
EmitNeonCls(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e204800);
}

inline
int
EmitNeonClz(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e204800);
}

inline
int
EmitNeonCmeq0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e209800, 0x5e209800);
}

inline
int
EmitNeonCmge0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e208800, 0x7e208800);
}

inline
int
EmitNeonCmgt0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e208800, 0x5e208800);
}

inline
int
EmitNeonCmle0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e209800, 0x7e209800);
}

inline
int
EmitNeonCmlt0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e20a800, 0x5e20a800);
}

inline
int
EmitNeonCnt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e205800);
}

inline
int
EmitNeonNeg(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e20b800, 0x7e20b800);
}

inline
int
EmitNeonNot(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e205800);
}

inline
int
EmitNeonRbit(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e605800);
}

inline
int
EmitNeonRev16(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e201800);
}

inline
int
EmitNeonRev32(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e200800);
}

inline
int
EmitNeonRev64(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e200800);
}

inline
int
EmitNeonSadalp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e206800);
}

inline
int
EmitNeonSaddlp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e202800);
}

inline
int
EmitNeonSaddlv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e303800);
}

inline
int
EmitNeonShll(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x2e213800);
}

inline
int
EmitNeonShll2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x2e213800);
}

inline
int
EmitNeonSmaxv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e30a800);
}

inline
int
EmitNeonSminv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e31a800);
}

inline
int
EmitNeonSqabs(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e207800, 0x5e207800);
}

inline
int
EmitNeonSqneg(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e207800, 0x7e207800);
}

inline
int
EmitNeonSqxtn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x0e214800, 0x5e214800);
}

inline
int
EmitNeonSqxtn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x0e214800, 0x5e214800);
}

inline
int
EmitNeonSqxtun(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x2e212800, 0x7e212800);
}

inline
int
EmitNeonSqxtun2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x2e212800, 0x7e212800);
}

inline
int
EmitNeonSuqadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e203800, 0x5e203800);
}

inline
int
EmitNeonUadalp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e206800);
}

inline
int
EmitNeonUaddlp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e202800);
}

inline
int
EmitNeonUaddlv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e303800);
}

inline
int
EmitNeonUmaxv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30a800);
}

inline
int
EmitNeonUminv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e31a800);
}

inline
int
EmitNeonUqxtn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x2e214800, 0x7e214800);
}

inline
int
EmitNeonUqxtn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x2e214800, 0x7e214800);
}

inline
int
EmitNeonUrecpe(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1c800);
}

inline
int
EmitNeonUrsqrte(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1c800);
}

inline
int
EmitNeonUsqadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e203800, 0x7e203800);
}

inline
int
EmitNeonXtn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x0e212800);
}

inline
int
EmitNeonXtn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x0e212800);
}

//
// Binary FP operations
//

inline
int
EmitNeonFloatBinaryCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonFabs(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0f800, 0x1e20c000);
}

inline
int
EmitNeonFaddp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30d800);
}

inline
int
EmitNeonFcmeq0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0d800, 0x5ea0d800);
}

inline
int
EmitNeonFcmge0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0c800, 0x7ea0c800);
}

inline
int
EmitNeonFcmgt0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0c800, 0x5ea0c800);
}

inline
int
EmitNeonFcmle0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0d800, 0x7ea0d800);
}

inline
int
EmitNeonFcmlt0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0e800, 0x5ea0e800);
}

inline
int
EmitNeonFcvtas(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21c800, 0x5e21c800);
}

inline
int
EmitNeonFcvtau(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21c800, 0x7e21c800);
}

inline
int
EmitNeonFcvt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NEON_SIZE DestSize,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D));
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_1H | VALID_1S | VALID_1D));
    NT_ASSERT(SrcSize != DestSize);
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x1ea34000 ^ ((SrcSize & 2) << 22) ^ ((DestSize & 3) << 15));
}

inline
int
EmitNeonFcvtl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize + 1) & ~4), 0x0e217800);
}

inline
int
EmitNeonFcvtl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize + 1) | 4), 0x0e217800);
}

inline
int
EmitNeonFcvtms(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21b800, 0x5e21b800);
}

inline
int
EmitNeonFcvtmu(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21b800, 0x7e21b800);
}

inline
int
EmitNeonFcvtn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x0e216800);
}

inline
int
EmitNeonFcvtn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x4e216800);
}

inline
int
EmitNeonFcvtns(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21a800, 0x5e21a800);
}

inline
int
EmitNeonFcvtnu(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21a800, 0x7e21a800);
}

inline
int
EmitNeonFcvtps(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1a800, 0x5ea1a800);
}

inline
int
EmitNeonFcvtpu(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1a800, 0x7ea1a800);
}

inline
int
EmitNeonFcvtxn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x2e216800, 0x7e216800);
}

inline
int
EmitNeonFcvtxn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x2e216800, 0x7e216800);
}

inline
int
EmitNeonFcvtzs(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1b800, 0x5ea1b800);
}

inline
int
EmitNeonFcvtzu(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1b800, 0x7ea1b800);
}

inline
int
EmitNeonFmaxnmp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30c800);
}

inline
int
EmitNeonFmaxnmv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30c800);
}

inline
int
EmitNeonFmaxp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30f800);
}

inline
int
EmitNeonFmaxv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30f800);
}

inline
int
EmitNeonFminnmp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7eb0c800);
}

inline
int
EmitNeonFminnmv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2eb0c800);
}

inline
int
EmitNeonFminp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7eb0f800);
}

inline
int
EmitNeonFminv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2eb0f800);
}

inline
int
EmitNeonFmov(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x1e204000);
}

inline
int
EmitNeonFmovImmediate(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    UCHAR Immediate,
    NEON_SIZE DestSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_1S | VALID_1D));
    return Emitter.EmitFourBytes(0x1e201000 | ((DestSize & 1) << 22) | (ULONG(Immediate) << 13) | Dest.RawRegister());
}

inline
int
EmitNeonFneg(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0f800, 0x1e214000);
}

inline
int
EmitNeonFrecpe(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1d800, 0x5ea1d800);
}

inline
int
EmitNeonFrecpx(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x5ea1f800);
}

inline
int
EmitNeonFrinta(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e218800, 0x1e264000);
}

inline
int
EmitNeonFrinti(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea19800, 0x1e27c000);
}

inline
int
EmitNeonFrintm(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e219800, 0x1e254000);
}

inline
int
EmitNeonFrintn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e218800, 0x1e244000);
}

inline
int
EmitNeonFrintp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea18800, 0x1e24c000);
}

inline
int
EmitNeonFrintx(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e219800, 0x1e274000);
}

inline
int
EmitNeonFrintz(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea19800, 0x1e25c000);
}

inline
int
EmitNeonFrsqrte(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1d800, 0x7ea1d800);
}

inline
int
EmitNeonFsqrt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1f800, 0x1e21c000);
}

inline
int
EmitNeonScvtf(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21d800, 0x5e21d800);
}

inline
int
EmitNeonUcvtf(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21d800, 0x7e21d800);
}

//
// Trinary integer operations
//

inline
int
EmitNeonTrinaryCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 3) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 3) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonAdd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e208400, 0x5e208400);
}

inline
int
EmitNeonAddhn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e204000);
}

inline
int
EmitNeonAddhn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e204000);
}

inline
int
EmitNeonAddp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20bc00);
}

inline
int
EmitNeonAnd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e201c00);
}

inline
int
EmitNeonBicRegister(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e601c00);
}

inline
int
EmitNeonBif(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ee01c00);
}

inline
int
EmitNeonBit(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea01c00);
}

inline
int
EmitNeonBsl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e601c00);
}

inline
int
EmitNeonCmeq(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e208c00, 0x7e208c00);
}

inline
int
EmitNeonCmge(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e203c00, 0x5e203c00);
}

inline
int
EmitNeonCmgt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e203400, 0x5e203400);
}

inline
int
EmitNeonCmhi(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e203400, 0x7e203400);
}

inline
int
EmitNeonCmhs(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e203c00, 0x7e203c00);
}

inline
int
EmitNeonCmtst(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e208c00, 0x5e208c00);
}

inline
int
EmitNeonEor(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e201c00);
}

inline
int
EmitNeonMla(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e209400);
}

inline
int
EmitNeonMls(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e209400);
}

inline
int
EmitNeonMov(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src, Src, SrcSize, 0x0ea01c00);
}

inline
int
EmitNeonMul(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e209c00);
}

inline
int
EmitNeonOrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ee01c00);
}

inline
int
EmitNeonOrrRegister(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea01c00);
}

inline
int
EmitNeonPmul(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e209c00);
}

inline
int
EmitNeonPmull(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_1D | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20e000, 0x0e20e000);
}

inline
int
EmitNeonPmull2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20e000, 0x0e20e000);
}

inline
int
EmitNeonRaddhn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) & ~4), 0x2e204000);
}

inline
int
EmitNeonRaddhn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) | 4), 0x2e204000);
}

inline
int
EmitNeonRsubhn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) & ~4), 0x2e206000);
}

inline
int
EmitNeonRsubhn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) | 4), 0x2e206000);
}

inline
int
EmitNeonSaba(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e207c00);
}

inline
int
EmitNeonSabal(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e205000);
}

inline
int
EmitNeonSabal2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e205000);
}

inline
int
EmitNeonSabd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e207400);
}

inline
int
EmitNeonSabdl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e207000);
}

inline
int
EmitNeonSabdl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e207000);
}

inline
int
EmitNeonSaddl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e200000);
}

inline
int
EmitNeonSaddl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e200000);
}

inline
int
EmitNeonSaddw(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e201000);
}

inline
int
EmitNeonSaddw2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e201000);
}

inline
int
EmitNeonShadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e200400);
}

inline
int
EmitNeonShsub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e202400);
}

inline
int
EmitNeonSmax(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e206400);
}

inline
int
EmitNeonSmaxp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20a400);
}

inline
int
EmitNeonSmin(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e206c00);
}

inline
int
EmitNeonSminp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20ac00);
}

inline
int
EmitNeonSmlal(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e208000);
}

inline
int
EmitNeonSmlal2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e208000);
}

inline
int
EmitNeonSmlsl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20a000);
}

inline
int
EmitNeonSmlsl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20a000);
}

inline
int
EmitNeonSmull(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20c000);
}

inline
int
EmitNeonSmull2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20c000);
}

inline
int
EmitNeonSqadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e200c00, 0x5e200c00);
}

inline
int
EmitNeonSqdmlal(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e209000, 0x5e209000);
}

inline
int
EmitNeonSqdmlal2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e209000);
}

inline
int
EmitNeonSqdmlsl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20b000, 0x5e20b00);
}

inline
int
EmitNeonSqdmlsl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20b000);
}

inline
int
EmitNeonSqdmulh(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20b400, 0x5e20b400);
}

inline
int
EmitNeonSqdmull(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20d000, 0x5e20d000);
}

inline
int
EmitNeonSqdmull2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20d000);
}

inline
int
EmitNeonSqrdmulh(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20b400, 0x7e20b400);
}

inline
int
EmitNeonSqrshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e205c00, 0x5e205c00);
}

inline
int
EmitNeonSqshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e204c00, 0x5e204c00);
}

inline
int
EmitNeonSqsub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e202c00, 0x5e202c00);
}

inline
int
EmitNeonSrhadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e201400);
}

inline
int
EmitNeonSrshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e205400, 0x5e205400);
}

inline
int
EmitNeonSshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e204400, 0x5e204400);
}

inline
int
EmitNeonSsubl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e202000);
}

inline
int
EmitNeonSsubl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e202000);
}

inline
int
EmitNeonSsubw(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e203000);
}

inline
int
EmitNeonSsubw2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e203000);
}

inline
int
EmitNeonSub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e208400, 0x7e208400);
}

inline
int
EmitNeonSubhn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e206000);
}

inline
int
EmitNeonSubhn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e206000);
}

inline
int
EmitNeonTrn1(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e002800);
}

inline
int
EmitNeonTrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e006800);
}

inline
int
EmitNeonUaba(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e207c00);
}

inline
int
EmitNeonUabal(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e205000);
}

inline
int
EmitNeonUabal2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e205000);
}

inline
int
EmitNeonUabd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e207400);
}

inline
int
EmitNeonUabdl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e207000);
}

inline
int
EmitNeonUabdl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e207000);
}

inline
int
EmitNeonUaddl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e200000);
}

inline
int
EmitNeonUaddl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e200000);
}

inline
int
EmitNeonUaddw(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e201000);
}

inline
int
EmitNeonUaddw2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e201000);
}

inline
int
EmitNeonUhadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e200400);
}

inline
int
EmitNeonUhsub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e202400);
}

inline
int
EmitNeonUmax(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e206400);
}

inline
int
EmitNeonUmaxp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20a400);
}

inline
int
EmitNeonUmin(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e206c00);
}

inline
int
EmitNeonUminp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20ac00);
}

inline
int
EmitNeonUmlal(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e208000);
}

inline
int
EmitNeonUmlal2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e208000);
}

inline
int
EmitNeonUmlsl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e20a000);
}

inline
int
EmitNeonUmlsl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e20a000);
}

inline
int
EmitNeonUmull(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e20c000);
}

inline
int
EmitNeonUmull2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e20c000);
}

inline
int
EmitNeonUqadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e200c00, 0x7e200c00);
}

inline
int
EmitNeonUqrshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e205c00, 0x7e205c00);
}

inline
int
EmitNeonUqshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e204c00, 0x7e204c00);
}

inline
int
EmitNeonUqsub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e202c00, 0x7e202c00);
}

inline
int
EmitNeonUrhadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e201400);
}

inline
int
EmitNeonUrshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e205400, 0x7e205400);
}

inline
int
EmitNeonUshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e204400, 0x7e204400);
}

inline
int
EmitNeonUsubl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e202000);
}

inline
int
EmitNeonUsubl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e202000);
}

inline
int
EmitNeonUsubw(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e203000);
}

inline
int
EmitNeonUsubw2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e203000);
}

inline
int
EmitNeonUzp1(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e001800);
}

inline
int
EmitNeonUzp2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e005800);
}

inline
int
EmitNeonZip1(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e003800);
}

inline
int
EmitNeonZip2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e007800);
}

//
// Trinary FP operations
//

inline
int
EmitNeonFloatTrinaryCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 1) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 1) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonFabd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0d400, 0x7ea0d400);
}

inline
int
EmitNeonFacge(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20ec00, 0x7e20ec00);
}

inline
int
EmitNeonFacgt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0ec00, 0x7ea0ec00);
}

inline
int
EmitNeonFadd(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20d400, 0x1e202800);
}

inline
int
EmitNeonFaddp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20d400);
}

inline
int
EmitNeonFcmeq(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    //
    // NaNs produce 0s (false)
    //

    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20e400, 0x5e20e400);
}

inline
int
EmitNeonFcmge(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20e400, 0x7e20e400);
}

inline
int
EmitNeonFcmgt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0e400, 0x7ea0e400);
}

inline
int
EmitNeonFcmp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, Src2, SrcSize, 0, 0x1e202000);
}

inline
int
EmitNeonFcmp0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, NEONREG_D0, SrcSize, 0, 0x1e202008);
}

inline
int
EmitNeonFcmpe(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, Src2, SrcSize, 0, 0x1e202010);
}

inline
int
EmitNeonFcmpe0(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, NEONREG_D0, SrcSize, 0, 0x1e204018);
}

inline
int
EmitNeonFdiv(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20fc00, 0x1e201800);
}

inline
int
EmitNeonFmax(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20f400, 0x1e204800);
}

inline
int
EmitNeonFmaxnm(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20c400, 0x1e206800);
}

inline
int
EmitNeonFmaxnmp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20c400);
}

inline
int
EmitNeonFmaxp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20f400);
}

inline
int
EmitNeonFmin(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0f400, 0x1e205800);
}

inline
int
EmitNeonFminnm(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0c400, 0x1e207800);
}

inline
int
EmitNeonFminnmp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0c400);
}

inline
int
EmitNeonFminp(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0f400);
}

inline
int
EmitNeonFmla(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20cc00);
}

inline
int
EmitNeonFmls(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0cc00);
}

inline
int
EmitNeonFmul(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20dc00, 0x1e200800);
}

inline
int
EmitNeonFmulx(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20dc00, 0x5e20dc00);
}

inline
int
EmitNeonFnmul(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0, 0x1e208800);
}

inline
int
EmitNeonFrecps(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20fc00, 0x5e20fc00);
}

inline
int
EmitNeonFrsqrts(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0fc00, 0x5ea0fc00);
}

inline
int
EmitNeonFsub(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0d400, 0x1e203800);
}

//
// Shift immediate forms
//

inline
int
EmitNeonShiftLeftImmediateCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{
    ULONG Size = SrcSize & 3;
    NT_ASSERT(Immediate < (8U << Size));

    ULONG EffShift = Immediate + (8 << Size);

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonShiftRightImmediateCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{
    ULONG Size = SrcSize & 3;
    NT_ASSERT(Immediate >= 0 && Immediate <= (8U << Size));

    ULONG EffShift = (16 << Size) - Immediate;

    if (NeonSizeIsScalar(SrcSize)) {
        NT_ASSERT(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        NT_ASSERT(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonRshrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f008c00);
}

inline
int
EmitNeonRshrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f008c00);
}

inline
int
EmitNeonShl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f005400, 0x5f005400);
}

inline
int
EmitNeonShrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f008400);
}

inline
int
EmitNeonShrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f008400);
}

inline
int
EmitNeonSli(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f005400, 0x7f005400);
}

inline
int
EmitNeonSqrshrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f009c00, 0x5f009c00);
}

inline
int
EmitNeonSqrshrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f009c00);
}

inline
int
EmitNeonSqrshrun(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f008c00, 0x7f008c00);
}

inline
int
EmitNeonSqrshrun2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f008c00);
}

inline
int
EmitNeonSqshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f007400, 0x5f007400);
}

inline
int
EmitNeonSqshlu(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f006400, 0x7f006400);
}

inline
int
EmitNeonSqshrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f009400, 0x5f009400);
}

inline
int
EmitNeonSqshrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f009400);
}

inline
int
EmitNeonSri(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f004400, 0x7f004400);
}

inline
int
EmitNeonSrshr(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f002400, 0x5f002400);
}

inline
int
EmitNeonSrsra(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f003400, 0x5f003400);
}

inline
int
EmitNeonSshll(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize & ~4), 0x0f00a400);
}

inline
int
EmitNeonSshll2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize | 4), 0x0f00a400);
}

inline
int
EmitNeonSshr(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f000400, 0x5f000400);
}

inline
int
EmitNeonSsra(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f001400, 0x5f001400);
}

inline
int
EmitNeonSxtl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize & ~4), 0x0f00a400);
}

inline
int
EmitNeonSxtl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize | 4), 0x0f00a400);
}

inline
int
EmitNeonUqrshrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f009c00, 0x7f009c00);
}

inline
int
EmitNeonUqrshrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f009c00);
}

inline
int
EmitNeonUqshl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f007400, 0x7f007400);
}

inline
int
EmitNeonUqshrn(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f009400, 0x7f009400);
}

inline
int
EmitNeonUqshrn2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f009400);
}

inline
int
EmitNeonUrshr(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f002400, 0x7f002400);
}

inline
int
EmitNeonUrsra(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f003400, 0x7f003400);
}

inline
int
EmitNeonUshll(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize & ~4), 0x2f00a400);
}

inline
int
EmitNeonUshll2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize | 4), 0x2f00a400);
}

inline
int
EmitNeonUshr(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f000400, 0x7f000400);
}

inline
int
EmitNeonUsra(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f001400, 0x7f001400);
}

inline
int
EmitNeonUxtl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize & ~4), 0x2f00a400);
}

inline
int
EmitNeonUxtl2(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize | 4), 0x2f00a400);
}

//
// FCVT* Rx,V.T
//

inline
int
EmitNeonConvertScalarCommon(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonFcvtzs(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e380000);
}

inline
int
EmitNeonFcvtzs64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e380000);
}

inline
int
EmitNeonFcvtzu(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e390000);
}

inline
int
EmitNeonFcvtzu64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e390000);
}

inline
int
EmitNeonFmovToGeneral(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S, 0x1e260000);
}

//
// e.g. FMOV X6, D0
//

inline
int
EmitNeonFmovToGeneral64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1D | VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1D, 0x9e260000);
}

//
// e.g. FMOV X7, V0.D[1]
//

inline
int
EmitNeonFmovToGeneralHigh64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_2D));  // TODO: Should this be VALID_1D?
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S /* SIZE_1D */, 0x9eae0000);
}

//
// SCVTF* V.T,D
//

inline
int
EmitNeonConvertScalarCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DstSize,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | ((DstSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonFmovFromGeneral(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_1S));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S, 0x1e270000);
}

//
// e.g. FMOV D0, X6
//

inline
int
EmitNeonFmovFromGeneral64(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_1D | VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1D, 0x9e270000);
}

//
// e.g. FMOV V0.d[1], X7
//

inline
int
EmitNeonFmovFromGeneralHigh64(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S /* SIZE_1D */, 0x9eaf0000);
}

inline
int
EmitNeonScvtf(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x1e220000);
}

inline
int
EmitNeonScvtf64(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x9e220000);
}

inline
int
EmitNeonUcvtf(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x1e230000);
}

inline
int
EmitNeonUcvtf64(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x9e230000);
}

//
// DUP V.T,V.T[index]
//

inline
int
EmitNeonMovElementCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize,
    ULONG Opcode
    )
{
    ULONG Size = SrcSize & 3;
    NT_ASSERT((SrcIndex << Size) < 16);

    SrcIndex = ((SrcIndex << 1) | 1) << Size;

    return Emitter.EmitFourBytes(Opcode | (SrcIndex << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonDupElement(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE DestSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonMovElementCommon(Emitter, Dest, Src, SrcIndex, DestSize, 0x0e000400 | (((DestSize >> 2) & 1) << 30));
}

inline
int
EmitNeonDup(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonMovElementCommon(Emitter, Dest, NeonRegisterParam(NEONREG_D0 + Src.RawRegister()), 0, DestSize, 0x0e000c00 | (((DestSize >> 2) & 1) << 30));
}

inline
int
EmitNeonIns(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG DestIndex,
    ArmSimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D));
    return EmitNeonMovElementCommon(Emitter, Dest, NeonRegisterParam(NEONREG_D0 + Src.RawRegister()), DestIndex, DestSize, 0x4e001c00);
}

inline
int
EmitNeonSmov(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x0e002c00);
}

inline
int
EmitNeonSmov64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x4e002c00);
}

inline
int
EmitNeonUmov(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x0e003c00);
}

inline
int
EmitNeonUmov64(
    VPCCompilerCodeEmitter &Emitter,
    ArmSimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x4e003c00);
}

//
// INS V.T[index],V.T[index]
//

inline
int
EmitNeonInsElement(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG DestIndex,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));

    ULONG Size = SrcSize & 3;
    NT_ASSERT((DestIndex << Size) < 16);
    NT_ASSERT((SrcIndex << Size) < 16);

    DestIndex = ((DestIndex << 1) | 1) << Size;
    SrcIndex <<= Size;

    return Emitter.EmitFourBytes(0x6e000400 | (DestIndex << 16) | (SrcIndex << 11) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

//
// FCSEL V.T,V.T,V.T,condition
//

inline
int
EmitNeonFcsel(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    ULONG Condition,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return Emitter.EmitFourBytes(0x1e200c00 | ((SrcSize & 1) << 22) | (Src2.RawRegister() << 16) | ((Condition & 15) << 12) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

//
// MOVI V.T,#x
//

inline
ULONG
ComputeNeonImmediate(
    ULONG64 Immediate,
    NEON_SIZE DestSize
    )
{

    //
    // Expand the immediate
    //

    ULONG OpSize = DestSize & 3;
    if (OpSize == 0) {
        Immediate &= 0xff;
        Immediate |= Immediate << 8;
    }
    if (OpSize <= 1) {
        Immediate &= 0xffff;
        Immediate |= Immediate << 16;
    }
    if (OpSize <= 2) {
        Immediate &= 0xffffffff;
        Immediate |= Immediate << 32;
    }
    bool Replicated2x = (((Immediate >> 32) & 0xffffffff) == (Immediate & 0xffffffff));
    bool Replicated4x = Replicated2x && (((Immediate >> 16) & 0xffff) == (Immediate & 0xffff));

    //
    // Determine CMODE and Op
    //

    ULONG Op = 0;
    ULONG Cmode;
    ULONG EncImmediate;
    if (Replicated2x && (Immediate & 0xffffff00) == 0) {
        Cmode = 0;
        EncImmediate = Immediate & 0xff;
    } else if (Replicated2x && (Immediate & 0xffff00ff) == 0) {
        Cmode = 2;
        EncImmediate = (Immediate >> 8) & 0xff;
    } else if (Replicated2x && (Immediate & 0xff00ffff) == 0) {
        Cmode = 4;
        EncImmediate = (Immediate >> 16) & 0xff;
    } else if (Replicated2x && (Immediate & 0x00ffffff) == 0) {
        Cmode = 6;
        EncImmediate = (Immediate >> 24) & 0xff;
    } else if (Replicated4x && (Immediate & 0xff00) == 0) {
        Cmode = 8;
        EncImmediate = Immediate & 0xff;
    } else if (Replicated4x && (Immediate & 0x00ff) == 0) {
        Cmode = 10;
        EncImmediate = (Immediate >> 8) & 0xff;
    } else if (Replicated2x && (Immediate & 0xffff00ff) == 0x000000ff) {
        Cmode = 12;
        EncImmediate = (Immediate >> 8) & 0xff;
    } else if (Replicated2x && (Immediate & 0xff00ffff) == 0x0000ffff) {
        Cmode = 13;
        EncImmediate = (Immediate >> 16) & 0xff;
    } else if (Replicated4x && ((Immediate >> 8) & 0xff) == (Immediate & 0xff)) {
        Cmode = 14;
        EncImmediate = Immediate & 0xff;
    } else if (Replicated2x && ((Immediate & 0x7e07ffff) == 0x40000000 || (Immediate & 0x7e07ffffull) == 0x3e000000)) {
        Cmode = 15;
        EncImmediate = (((Immediate >> 31) & 1) << 7) | (((Immediate >> 29) & 1) << 6) | ((Immediate >> 19) & 0x3f);
    } else if ((Immediate & 0x7fc0ffffffffffffull) == 0x4000000000000000ull ||
               (Immediate & 0x7fc0ffffffffffffull) == 0x3fc0000000000000ull) {
        Cmode = 15;
        Op = 1;
        EncImmediate = (((Immediate >> 63) & 1) << 7) | (((Immediate >> 61) & 1) << 6) | ((Immediate >> 48) & 0x3f);
    } else {
        EncImmediate = 0;
        for (int Index = 0; Index < 8; Index++) {
            ULONG Temp = (Immediate >> (8 * Index)) & 0xff;
            if (Temp == 0xff) {
                EncImmediate |= 1 << Index;
            } else if (Temp != 0) {
                return 1;
            }
        }
        Cmode = 14;
        Op = 1;
    }

    NT_ASSERT(EncImmediate < 256);
    return (Op << 29) | (((EncImmediate >> 5) & 7) << 16) | (Cmode << 12) | ((EncImmediate & 0x1f) << 5);
}

inline
int
EmitNeonMovi(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG64 Immediate,
    NEON_SIZE DestSize
    )
{
    NT_ASSERT(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D | VALID_1D));

    ULONG EncImmediate = ComputeNeonImmediate(Immediate, DestSize);
    if (EncImmediate != 1) {
        return Emitter.EmitFourBytes(0x0f000400 | (((DestSize >> 2) & 1) << 30) | EncImmediate | Dest.RawRegister());
    }

    EncImmediate = ComputeNeonImmediate(~Immediate, DestSize);
    if (EncImmediate != 1) {
        return Emitter.EmitFourBytes(0x2f000400 | (((DestSize >> 2) & 1) << 30) | EncImmediate | Dest.RawRegister());
    }

    NT_ASSERT(false);
    return 0;
}

//
// TBL Vd.T,Vn.16B,Vm.T
//
//

inline
int
EmitNeonTbl(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NeonRegisterParam Indices,
    NEON_SIZE Size
    )
{

    if (Size == SIZE_8B) {
        return Emitter.EmitFourBytes(0x0e000000 | (Indices.RawRegister() << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());

    } else {

        NT_ASSERT(Size == SIZE_16B);

        return Emitter.EmitFourBytes(0x4e000000 | (Indices.RawRegister() << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

//
// EXT Vd.T,Vn.T,Vm.T,#x
//

inline
int
EmitNeonExt(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcSize, VALID_816B));
    NT_ASSERT(((SrcSize == SIZE_8B) && (Immediate < 8)) ||
              ((SrcSize == SIZE_16B) && (Immediate < 16)));

    return Emitter.EmitFourBytes(0x2e000000 | (((SrcSize >> 2) & 1) << 30) | (Src2.RawRegister() << 16) | (Immediate << 11) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

//
// LDR (immediate, SIMD&FP) - Unsigned offset form
//

inline
int
EmitNeonLdrStrOffsetCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    NEON_SIZE SrcDestSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset,
    ULONG Opcode,
    ULONG OpcodeUnscaled
    )
{
    NT_ASSERT(NeonSizeIsScalar(SrcDestSize));

    ULONG SizeBits = ((SrcDestSize & 3) << 30) | ((SrcDestSize >> 2) << 23);

    if (Opcode != 0) {
        LONG EncodeOffset = Offset >> SrcDestSize;
        if ((EncodeOffset << SrcDestSize) == Offset && (EncodeOffset & 0xfff) == EncodeOffset) {
            return Emitter.EmitFourBytes(Opcode | SizeBits | ((EncodeOffset & 0xfff) << 10) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
        }
    }

    if (OpcodeUnscaled != 0 && Offset >= -0x100 && Offset <= 0xff) {
        return Emitter.EmitFourBytes(OpcodeUnscaled | SizeBits | ((Offset & 0x1ff) << 12) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
    }

    return 0;
}

inline
int
EmitNeonLdrOffset(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NEON_SIZE DestSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdrStrOffsetCommon(Emitter, Dest, DestSize, Addr, Offset, 0x3d400000, 0x3c400000);
}

inline
int
EmitNeonStrOffset(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Source,
    NEON_SIZE SourceSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdrStrOffsetCommon(Emitter, Source, SourceSize, Addr, Offset, 0x3d000000, 0x3c000000);
}

//
// LDP (immediate, SIMD&FP) - Unsigned offset form
//

inline
int
EmitNeonLdpStpOffsetCommon(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam SrcDest1,
    NeonRegisterParam SrcDest2,
    NEON_SIZE SrcDestSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset,
    ULONG Opcode
    )
{
    NT_ASSERT(NeonSizeIsValid(SrcDestSize, VALID_1S | VALID_1D | VALID_1Q));

    ULONG Opc = (SrcDestSize - 2);

    LONG EncodeOffset = Offset >> SrcDestSize;
    if ((EncodeOffset << SrcDestSize) == Offset && EncodeOffset >= -0x40 && EncodeOffset <= 0x3f) {
        return Emitter.EmitFourBytes(Opcode | (Opc << 30) | ((EncodeOffset & 0x7f) << 15) | (SrcDest2.RawRegister() << 10) | (Addr.RawRegister() << 5) | SrcDest1.RawRegister());
    }

    return 0;
}

inline
int
EmitNeonLdpOffset(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest1,
    NeonRegisterParam Dest2,
    NEON_SIZE DestSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdpStpOffsetCommon(Emitter, Dest1, Dest2, DestSize, Addr, Offset, 0x2d400000);
}

inline
int
EmitNeonStpOffset(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Source1,
    NeonRegisterParam Source2,
    NEON_SIZE SourceSize,
    ArmSimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdpStpOffsetCommon(Emitter, Source1, Source2, SourceSize, Addr, Offset, 0x2d000000);
}

//
// LD1/ST1 (immediate, SIMD&FP) - Unsigned offset form
//

inline
int
EmitNeonLd1St1Common(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    ArmSimpleRegisterParam Addr,
    NEON_SIZE SrcDestSize,
    ULONG Opcode
    )
{
    ULONG QSSize = Index << (SrcDestSize & 3);
    if (SrcDestSize == SIZE_1D) {
        QSSize |= 1;
    }

    NT_ASSERT(QSSize < 16);
    ULONG Op = (SrcDestSize == SIZE_1B) ? 0 : (SrcDestSize == SIZE_1H) ? 2 : 4;

    return Emitter.EmitFourBytes(Opcode | ((QSSize >> 3) << 30) | (Op << 13) | ((QSSize & 7) << 10) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
}

inline
int
EmitNeonLd1(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    ArmSimpleRegisterParam Addr,
    NEON_SIZE SrcDestSize
    )
{
    return EmitNeonLd1St1Common(Emitter, SrcDest, Index, Addr, SrcDestSize, 0x0d400000);
}

inline
int
EmitNeonSt1(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    ArmSimpleRegisterParam Addr,
    NEON_SIZE SrcDestSize
    )
{
    return EmitNeonLd1St1Common(Emitter, SrcDest, Index, Addr, SrcDestSize, 0x0d000000);
}

//
// AES
//

inline
int
EmitNeonAesD(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    NT_ASSERT(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e285800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesE(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    NT_ASSERT(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e284800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesImc(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    NT_ASSERT(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e287800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesMc(
    VPCCompilerCodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    NT_ASSERT(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e286800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}
