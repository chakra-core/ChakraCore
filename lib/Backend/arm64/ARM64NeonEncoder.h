//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// This file contains low-level emitter functions, where each emitter encodes
// exactly one VFP/Neon instruction.
//

#pragma once

typedef UCHAR NEON_REGISTER;

class NeonRegisterParam
{
    static const UCHAR REGISTER_SHIFT = 0;
    static const UCHAR REGISTER_MASK = 0xff;
    static const ULONG REGISTER_MASK_SHIFTED = REGISTER_MASK << REGISTER_SHIFT;

    static const UCHAR SIZE_SHIFT = 8;
    static const UCHAR SIZE_MASK = 0x07;
    static const ULONG SIZE_MASK_SHIFTED = SIZE_MASK << SIZE_SHIFT;

    NeonRegisterParam()
    {
    }

public:

    //
    // N.B. Derived class must initialize m_Encoded.
    //

    NeonRegisterParam(
        NEON_REGISTER Reg,
        UCHAR Size = 8
    )
        : m_Encoded(((Reg & REGISTER_MASK) << REGISTER_SHIFT) |
        (EncodeSize(Size) << SIZE_SHIFT))
    {
        AssertValidRegister(Reg);
        AssertValidSize(Size);
    }

    NEON_REGISTER
    Register() const
    {
        return (m_Encoded >> REGISTER_SHIFT) & REGISTER_MASK;
    }

    UCHAR
    RawRegister() const
    {
        return UCHAR(Register() - NEONREG_FIRST);
    }

    UCHAR
    SizeShift() const
    {
        return (m_Encoded >> SIZE_SHIFT) & SIZE_MASK;
    }

    UCHAR
    Size() const
    {
        return 1 << SizeShift();
    }

protected:
    static
    ULONG
    EncodeSize(
        UCHAR Size
        )
    {
        return (Size == 1) ? 0 :
            (Size == 2) ? 1 :
            (Size == 4) ? 2 :
            (Size == 8) ? 3 :
            4;
    }

    static
    void
    AssertValidRegister(
        NEON_REGISTER Reg
        )
    {
        UNREFERENCED_PARAMETER(Reg);
        Assert(Reg >= NEONREG_FIRST && Reg <= NEONREG_LAST);
    }

    static
    void
    AssertValidSize(
        UCHAR Size
        )
    {
        UNREFERENCED_PARAMETER(Size);
        Assert(Size == 4 || Size == 8 || Size == 16);
    }

    ULONG m_Encoded;
};

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
        Assert(NumElements == 1 || NumElements == 8 || NumElements == 16);
        return (NumElements == 1) ? SIZE_1B : (NumElements == 8) ? SIZE_8B : SIZE_16B;

    case 2:
        Assert(NumElements == 1 || NumElements == 4 || NumElements == 8);
        return (NumElements == 1) ? SIZE_1H : (NumElements == 4) ? SIZE_4H : SIZE_8H;

    case 4:
        Assert(NumElements == 1 || NumElements == 2 || NumElements == 4);
        return (NumElements == 1) ? SIZE_1S : (NumElements == 2) ? SIZE_2S : SIZE_4S;

    case 8:
        Assert(NumElements == 1 || NumElements == 2);
        return (NumElements == 1) ? SIZE_1D : SIZE_2D;

    case 16:
        Assert(NumElements == 1);
        return SIZE_1Q;

    default:
        Assert(!"Invalid element size passed to NeonSize.");
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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 3) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 3) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonAbs(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e20b800, 0x5e20b800);
}

inline
int
EmitNeonAddp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(SrcSize == SIZE_1D);
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x5e31b800);
}

inline
int
EmitNeonAddv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e31b800);
}

inline
int
EmitNeonCls(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e204800);
}

inline
int
EmitNeonClz(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e204800);
}

inline
int
EmitNeonCmeq0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e209800, 0x5e209800);
}

inline
int
EmitNeonCmge0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e208800, 0x7e208800);
}

inline
int
EmitNeonCmgt0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e208800, 0x5e208800);
}

inline
int
EmitNeonCmle0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e209800, 0x7e209800);
}

inline
int
EmitNeonCmlt0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e20a800, 0x5e20a800);
}

inline
int
EmitNeonCnt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e205800);
}

inline
int
EmitNeonNeg(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e20b800, 0x7e20b800);
}

inline
int
EmitNeonNot(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e205800);
}

inline
int
EmitNeonRbit(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e605800);
}

inline
int
EmitNeonRev16(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e201800);
}

inline
int
EmitNeonRev32(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e200800);
}

inline
int
EmitNeonRev64(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e200800);
}

inline
int
EmitNeonSadalp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e206800);
}

inline
int
EmitNeonSaddlp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e202800);
}

inline
int
EmitNeonSaddlv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e303800);
}

inline
int
EmitNeonShll(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x2e213800);
}

inline
int
EmitNeonShll2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x2e213800);
}

inline
int
EmitNeonSmaxv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e30a800);
}

inline
int
EmitNeonSminv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e31a800);
}

inline
int
EmitNeonSqabs(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e207800, 0x5e207800);
}

inline
int
EmitNeonSqneg(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e207800, 0x7e207800);
}

inline
int
EmitNeonSqxtn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x0e214800, 0x5e214800);
}

inline
int
EmitNeonSqxtn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x0e214800, 0x5e214800);
}

inline
int
EmitNeonSqxtun(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x2e212800, 0x7e212800);
}

inline
int
EmitNeonSqxtun2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x2e212800, 0x7e212800);
}

inline
int
EmitNeonSuqadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e203800, 0x5e203800);
}

inline
int
EmitNeonUadalp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e206800);
}

inline
int
EmitNeonUaddlp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e202800);
}

inline
int
EmitNeonUaddlv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e303800);
}

inline
int
EmitNeonUmaxv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30a800);
}

inline
int
EmitNeonUminv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_4S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e31a800);
}

inline
int
EmitNeonUqxtn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x2e214800, 0x7e214800);
}

inline
int
EmitNeonUqxtn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x2e214800, 0x7e214800);
}

inline
int
EmitNeonUrecpe(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1c800);
}

inline
int
EmitNeonUrsqrte(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1c800);
}

inline
int
EmitNeonUsqadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e203800, 0x7e203800);
}

inline
int
EmitNeonXtn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) & ~4), 0x0e212800);
}

inline
int
EmitNeonXtn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize - 1) | 4), 0x0e212800);
}

//
// Binary FP operations
//

inline
int
EmitNeonFloatBinaryCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonFabs(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0f800, 0x1e20c000);
}

inline
int
EmitNeonFaddp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30d800);
}

inline
int
EmitNeonFcmeq0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0d800, 0x5ea0d800);
}

inline
int
EmitNeonFcmge0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0c800, 0x7ea0c800);
}

inline
int
EmitNeonFcmgt0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0c800, 0x5ea0c800);
}

inline
int
EmitNeonFcmle0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0d800, 0x7ea0d800);
}

inline
int
EmitNeonFcmlt0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea0e800, 0x5ea0e800);
}

inline
int
EmitNeonFcvtas(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21c800, 0x5e21c800);
}

inline
int
EmitNeonFcvtau(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21c800, 0x7e21c800);
}

inline
int
EmitNeonFcvt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NEON_SIZE DestSize,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D));
    Assert(NeonSizeIsValid(DestSize, VALID_1H | VALID_1S | VALID_1D));
    Assert(SrcSize != DestSize);
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x1ea34000 ^ ((SrcSize & 2) << 22) ^ ((DestSize & 3) << 15));
}

inline
int
EmitNeonFcvtl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize + 1) & ~4), 0x0e217800);
}

inline
int
EmitNeonFcvtl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE((SrcSize + 1) | 4), 0x0e217800);
}

inline
int
EmitNeonFcvtms(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21b800, 0x5e21b800);
}

inline
int
EmitNeonFcvtmu(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21b800, 0x7e21b800);
}

inline
int
EmitNeonFcvtn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x0e216800);
}

inline
int
EmitNeonFcvtn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x4e216800);
}

inline
int
EmitNeonFcvtns(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21a800, 0x5e21a800);
}

inline
int
EmitNeonFcvtnu(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21a800, 0x7e21a800);
}

inline
int
EmitNeonFcvtps(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1a800, 0x5ea1a800);
}

inline
int
EmitNeonFcvtpu(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1a800, 0x7ea1a800);
}

inline
int
EmitNeonFcvtxn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize & ~4), 0x2e216800, 0x7e216800);
}

inline
int
EmitNeonFcvtxn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, NEON_SIZE(SrcSize | 4), 0x2e216800, 0x7e216800);
}

inline
int
EmitNeonFcvtzs(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1b800, 0x5ea1b800);
}

inline
int
EmitNeonFcvtzu(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1b800, 0x7ea1b800);
}

inline
int
EmitNeonFmaxnmp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30c800);
}

inline
int
EmitNeonFmaxnmv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30c800);
}

inline
int
EmitNeonFmaxp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7e30f800);
}

inline
int
EmitNeonFmaxv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e30f800);
}

inline
int
EmitNeonFminnmp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7eb0c800);
}

inline
int
EmitNeonFminnmv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2eb0c800);
}

inline
int
EmitNeonFminp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_2S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x7eb0f800);
}

inline
int
EmitNeonFminv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_4S));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2eb0f800);
}

inline
int
EmitNeonFmov(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x1e204000);
}

inline
int
EmitNeonFmovImmediate(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    UCHAR Immediate,
    NEON_SIZE DestSize
    )
{
    Assert(NeonSizeIsValid(DestSize, VALID_1S | VALID_1D));
    return Emitter.EmitFourBytes(0x1e201000 | ((DestSize & 1) << 22) | (ULONG(Immediate) << 13) | Dest.RawRegister());
}

inline
int
EmitNeonFneg(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea0f800, 0x1e214000);
}

inline
int
EmitNeonFrecpe(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea1d800, 0x5ea1d800);
}

inline
int
EmitNeonFrecpx(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0, 0x5ea1f800);
}

inline
int
EmitNeonFrinta(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e218800, 0x1e264000);
}

inline
int
EmitNeonFrinti(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea19800, 0x1e27c000);
}

inline
int
EmitNeonFrintm(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e219800, 0x1e254000);
}

inline
int
EmitNeonFrintn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e218800, 0x1e244000);
}

inline
int
EmitNeonFrintp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea18800, 0x1e24c000);
}

inline
int
EmitNeonFrintx(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e219800, 0x1e274000);
}

inline
int
EmitNeonFrintz(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0ea19800, 0x1e25c000);
}

inline
int
EmitNeonFrsqrte(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1d800, 0x7ea1d800);
}

inline
int
EmitNeonFsqrt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2ea1f800, 0x1e21c000);
}

inline
int
EmitNeonScvtf(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x0e21d800, 0x5e21d800);
}

inline
int
EmitNeonUcvtf(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatBinaryCommon(Emitter, Dest, Src, SrcSize, 0x2e21d800, 0x7e21d800);
}

//
// Trinary integer operations
//

inline
int
EmitNeonTrinaryCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 3) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 3) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonAdd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e208400, 0x5e208400);
}

inline
int
EmitNeonAddhn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e204000);
}

inline
int
EmitNeonAddhn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e204000);
}

inline
int
EmitNeonAddp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20bc00);
}

inline
int
EmitNeonAnd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e201c00);
}

inline
int
EmitNeonBicRegister(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e601c00);
}

inline
int
EmitNeonBif(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ee01c00);
}

inline
int
EmitNeonBit(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea01c00);
}

inline
int
EmitNeonBsl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e601c00);
}

inline
int
EmitNeonCmeq(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e208c00, 0x7e208c00);
}

inline
int
EmitNeonCmge(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e203c00, 0x5e203c00);
}

inline
int
EmitNeonCmgt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e203400, 0x5e203400);
}

inline
int
EmitNeonCmhi(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e203400, 0x7e203400);
}

inline
int
EmitNeonCmhs(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e203c00, 0x7e203c00);
}

inline
int
EmitNeonCmtst(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e208c00, 0x5e208c00);
}

inline
int
EmitNeonEor(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e201c00);
}

inline
int
EmitNeonMla(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e209400);
}

inline
int
EmitNeonMls(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e209400);
}

inline
int
EmitNeonMov(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src, Src, SrcSize, 0x0ea01c00);
}

inline
int
EmitNeonMul(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e209c00);
}

inline
int
EmitNeonOrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ee01c00);
}

inline
int
EmitNeonOrrRegister(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea01c00);
}

inline
int
EmitNeonPmul(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e209c00);
}

inline
int
EmitNeonPmull(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_1D | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20e000, 0x0e20e000);
}

inline
int
EmitNeonPmull2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20e000, 0x0e20e000);
}

inline
int
EmitNeonRaddhn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) & ~4), 0x2e204000);
}

inline
int
EmitNeonRaddhn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) | 4), 0x2e204000);
}

inline
int
EmitNeonRsubhn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) & ~4), 0x2e206000);
}

inline
int
EmitNeonRsubhn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE((SrcSize - 1) | 4), 0x2e206000);
}

inline
int
EmitNeonSaba(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e207c00);
}

inline
int
EmitNeonSabal(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e205000);
}

inline
int
EmitNeonSabal2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e205000);
}

inline
int
EmitNeonSabd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e207400);
}

inline
int
EmitNeonSabdl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e207000);
}

inline
int
EmitNeonSabdl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e207000);
}

inline
int
EmitNeonSaddl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e200000);
}

inline
int
EmitNeonSaddl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e200000);
}

inline
int
EmitNeonSaddw(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e201000);
}

inline
int
EmitNeonSaddw2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e201000);
}

inline
int
EmitNeonShadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e200400);
}

inline
int
EmitNeonShsub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e202400);
}

inline
int
EmitNeonSmax(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e206400);
}

inline
int
EmitNeonSmaxp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20a400);
}

inline
int
EmitNeonSmin(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e206c00);
}

inline
int
EmitNeonSminp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20ac00);
}

inline
int
EmitNeonSmlal(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e208000);
}

inline
int
EmitNeonSmlal2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e208000);
}

inline
int
EmitNeonSmlsl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20a000);
}

inline
int
EmitNeonSmlsl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20a000);
}

inline
int
EmitNeonSmull(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20c000);
}

inline
int
EmitNeonSmull2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20c000);
}

inline
int
EmitNeonSqadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e200c00, 0x5e200c00);
}

inline
int
EmitNeonSqdmlal(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e209000, 0x5e209000);
}

inline
int
EmitNeonSqdmlal2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e209000);
}

inline
int
EmitNeonSqdmlsl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20b000, 0x5e20b00);
}

inline
int
EmitNeonSqdmlsl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20b000);
}

inline
int
EmitNeonSqdmulh(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20b400, 0x5e20b400);
}

inline
int
EmitNeonSqdmull(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e20d000, 0x5e20d000);
}

inline
int
EmitNeonSqdmull2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e20d000);
}

inline
int
EmitNeonSqrdmulh(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20b400, 0x7e20b400);
}

inline
int
EmitNeonSqrshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e205c00, 0x5e205c00);
}

inline
int
EmitNeonSqshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e204c00, 0x5e204c00);
}

inline
int
EmitNeonSqsub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e202c00, 0x5e202c00);
}

inline
int
EmitNeonSrhadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e201400);
}

inline
int
EmitNeonSrshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e205400, 0x5e205400);
}

inline
int
EmitNeonSshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e204400, 0x5e204400);
}

inline
int
EmitNeonSsubl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e202000);
}

inline
int
EmitNeonSsubl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e202000);
}

inline
int
EmitNeonSsubw(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e203000);
}

inline
int
EmitNeonSsubw2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e203000);
}

inline
int
EmitNeonSub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e208400, 0x7e208400);
}

inline
int
EmitNeonSubhn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x0e206000);
}

inline
int
EmitNeonSubhn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x0e206000);
}

inline
int
EmitNeonTrn1(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e002800);
}

inline
int
EmitNeonTrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e006800);
}

inline
int
EmitNeonUaba(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e207c00);
}

inline
int
EmitNeonUabal(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e205000);
}

inline
int
EmitNeonUabal2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e205000);
}

inline
int
EmitNeonUabd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e207400);
}

inline
int
EmitNeonUabdl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e207000);
}

inline
int
EmitNeonUabdl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e207000);
}

inline
int
EmitNeonUaddl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e200000);
}

inline
int
EmitNeonUaddl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e200000);
}

inline
int
EmitNeonUaddw(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e201000);
}

inline
int
EmitNeonUaddw2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e201000);
}

inline
int
EmitNeonUhadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e200400);
}

inline
int
EmitNeonUhsub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e202400);
}

inline
int
EmitNeonUmax(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e206400);
}

inline
int
EmitNeonUmaxp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20a400);
}

inline
int
EmitNeonUmin(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e206c00);
}

inline
int
EmitNeonUminp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20ac00);
}

inline
int
EmitNeonUmlal(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e208000);
}

inline
int
EmitNeonUmlal2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e208000);
}

inline
int
EmitNeonUmlsl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e20a000);
}

inline
int
EmitNeonUmlsl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e20a000);
}

inline
int
EmitNeonUmull(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e20c000);
}

inline
int
EmitNeonUmull2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e20c000);
}

inline
int
EmitNeonUqadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e200c00, 0x7e200c00);
}

inline
int
EmitNeonUqrshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e205c00, 0x7e205c00);
}

inline
int
EmitNeonUqshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e204c00, 0x7e204c00);
}

inline
int
EmitNeonUqsub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e202c00, 0x7e202c00);
}

inline
int
EmitNeonUrhadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e201400);
}

inline
int
EmitNeonUrshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e205400, 0x7e205400);
}

inline
int
EmitNeonUshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e204400, 0x7e204400);
}

inline
int
EmitNeonUsubl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e202000);
}

inline
int
EmitNeonUsubl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e202000);
}

inline
int
EmitNeonUsubw(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize & ~4), 0x2e203000);
}

inline
int
EmitNeonUsubw2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, NEON_SIZE(SrcSize | 4), 0x2e203000);
}

inline
int
EmitNeonUzp1(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e001800);
}

inline
int
EmitNeonUzp2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e005800);
}

inline
int
EmitNeonZip1(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e003800);
}

inline
int
EmitNeonZip2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e007800);
}

//
// Trinary FP operations
//

inline
int
EmitNeonFloatTrinaryCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | ((SrcSize & 1) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | ((SrcSize & 1) << 22) | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonFabd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0d400, 0x7ea0d400);
}

inline
int
EmitNeonFacge(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20ec00, 0x7e20ec00);
}

inline
int
EmitNeonFacgt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0ec00, 0x7ea0ec00);
}

inline
int
EmitNeonFadd(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20d400, 0x1e202800);
}

inline
int
EmitNeonFaddp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20d400);
}

inline
int
EmitNeonFcmeq(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    //
    // NaNs produce 0s (false)
    //

    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20e400, 0x5e20e400);
}

inline
int
EmitNeonFcmge(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20e400, 0x7e20e400);
}

inline
int
EmitNeonFcmgt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0e400, 0x7ea0e400);
}

inline
int
EmitNeonFcmp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, Src2, SrcSize, 0, 0x1e202000);
}

inline
int
EmitNeonFcmp0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, NEONREG_D0, SrcSize, 0, 0x1e202008);
}

inline
int
EmitNeonFcmpe(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, Src2, SrcSize, 0, 0x1e202010);
}

inline
int
EmitNeonFcmpe0(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Src1,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, NEONREG_D0, Src1, NEONREG_D0, SrcSize, 0, 0x1e204018);
}

inline
int
EmitNeonFdiv(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20fc00, 0x1e201800);
}

inline
int
EmitNeonFmax(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20f400, 0x1e204800);
}

inline
int
EmitNeonFmaxnm(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20c400, 0x1e206800);
}

inline
int
EmitNeonFmaxnmp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20c400);
}

inline
int
EmitNeonFmaxp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20f400);
}

inline
int
EmitNeonFmin(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0f400, 0x1e205800);
}

inline
int
EmitNeonFminnm(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0c400, 0x1e207800);
}

inline
int
EmitNeonFminnmp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0c400);
}

inline
int
EmitNeonFminp(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2ea0f400);
}

inline
int
EmitNeonFmla(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20cc00);
}

inline
int
EmitNeonFmls(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0cc00);
}

inline
int
EmitNeonFmul(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x2e20dc00, 0x1e200800);
}

inline
int
EmitNeonFmulx(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20dc00, 0x5e20dc00);
}

inline
int
EmitNeonFnmul(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0, 0x1e208800);
}

inline
int
EmitNeonFrecps(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0e20fc00, 0x5e20fc00);
}

inline
int
EmitNeonFrsqrts(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0fc00, 0x5ea0fc00);
}

inline
int
EmitNeonFsub(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D | VALID_24S | VALID_2D));
    return EmitNeonFloatTrinaryCommon(Emitter, Dest, Src1, Src2, SrcSize, 0x0ea0d400, 0x1e203800);
}

//
// Shift immediate forms
//

inline
int
EmitNeonShiftLeftImmediateCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{
    ULONG Size = SrcSize & 3;
    Assert(Immediate < (8U << Size));

    ULONG EffShift = Immediate + (8 << Size);

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonShiftRightImmediateCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize,
    ULONG VectorOpcode,
    ULONG ScalarOpcode = 0
    )
{
    ULONG Size = SrcSize & 3;
    Assert(Immediate <= (8U << Size));

    ULONG EffShift = (16 << Size) - Immediate;

    if (NeonSizeIsScalar(SrcSize)) {
        Assert(ScalarOpcode != 0);
        return Emitter.EmitFourBytes(ScalarOpcode | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else {
        Assert(VectorOpcode != 0);
        return Emitter.EmitFourBytes(VectorOpcode | (((SrcSize >> 2) & 1) << 30) | (EffShift << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitNeonRshrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f008c00);
}

inline
int
EmitNeonRshrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f008c00);
}

inline
int
EmitNeonShl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f005400, 0x5f005400);
}

inline
int
EmitNeonShrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f008400);
}

inline
int
EmitNeonShrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f008400);
}

inline
int
EmitNeonSli(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f005400, 0x7f005400);
}

inline
int
EmitNeonSqrshrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f009c00, 0x5f009c00);
}

inline
int
EmitNeonSqrshrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f009c00);
}

inline
int
EmitNeonSqrshrun(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f008c00, 0x7f008c00);
}

inline
int
EmitNeonSqrshrun2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f008c00);
}

inline
int
EmitNeonSqshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f007400, 0x5f007400);
}

inline
int
EmitNeonSqshlu(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f006400, 0x7f006400);
}

inline
int
EmitNeonSqshrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x0f009400, 0x5f009400);
}

inline
int
EmitNeonSqshrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x0f009400);
}

inline
int
EmitNeonSri(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f004400, 0x7f004400);
}

inline
int
EmitNeonSrshr(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f002400, 0x5f002400);
}

inline
int
EmitNeonSrsra(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f003400, 0x5f003400);
}

inline
int
EmitNeonSshll(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize & ~4), 0x0f00a400);
}

inline
int
EmitNeonSshll2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize | 4), 0x0f00a400);
}

inline
int
EmitNeonSshr(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f000400, 0x5f000400);
}

inline
int
EmitNeonSsra(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x0f001400, 0x5f001400);
}

inline
int
EmitNeonSxtl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize & ~4), 0x0f00a400);
}

inline
int
EmitNeonSxtl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize | 4), 0x0f00a400);
}

inline
int
EmitNeonUqrshrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f009c00, 0x7f009c00);
}

inline
int
EmitNeonUqrshrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f009c00);
}

inline
int
EmitNeonUqshl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f007400, 0x7f007400);
}

inline
int
EmitNeonUqshrn(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1H | VALID_1S | VALID_1D | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) & ~4), 0x2f009400, 0x7f009400);
}

inline
int
EmitNeonUqshrn2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE((SrcSize - 1) | 4), 0x2f009400);
}

inline
int
EmitNeonUrshr(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f002400, 0x7f002400);
}

inline
int
EmitNeonUrsra(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f003400, 0x7f003400);
}

inline
int
EmitNeonUshll(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize & ~4), 0x2f00a400);
}

inline
int
EmitNeonUshll2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, Immediate, NEON_SIZE(SrcSize | 4), 0x2f00a400);
}

inline
int
EmitNeonUshr(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f000400, 0x7f000400);
}

inline
int
EmitNeonUsra(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftRightImmediateCommon(Emitter, Dest, Src, Immediate, SrcSize, 0x2f001400, 0x7f001400);
}

inline
int
EmitNeonUxtl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize & ~4), 0x2f00a400);
}

inline
int
EmitNeonUxtl2(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_16B | VALID_8H | VALID_4S | VALID_2D));
    return EmitNeonShiftLeftImmediateCommon(Emitter, Dest, Src, 0, NEON_SIZE(SrcSize | 4), 0x2f00a400);
}

//
// FCVT* Rx,V.T
//

inline
int
EmitNeonConvertScalarCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | ((SrcSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonFcvtmsGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e300000);
}

inline
int
EmitNeonFcvtmsGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e300000);
}

inline
int
EmitNeonFcvtmuGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e310000);
}

inline
int
EmitNeonFcvtmuGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e310000);
}

inline
int
EmitNeonFcvtnsGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e200000);
}

inline
int
EmitNeonFcvtnsGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e200000);
}

inline
int
EmitNeonFcvtnuGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e210000);
}

inline
int
EmitNeonFcvtnuGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e210000);
}

inline
int
EmitNeonFcvtpsGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e280000);
}

inline
int
EmitNeonFcvtpsGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e280000);
}

inline
int
EmitNeonFcvtpuGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e290000);
}

inline
int
EmitNeonFcvtpuGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e290000);
}

inline
int
EmitNeonFcvtzsGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e380000);
}

inline
int
EmitNeonFcvtzsGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e380000);
}

inline
int
EmitNeonFcvtzuGen(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x1e390000);
}

inline
int
EmitNeonFcvtzuGen64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
)
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SrcSize, 0x9e390000);
}

inline
int
EmitNeonFmovToGeneral(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    Assert(NeonSizeIsValid(SrcSize, VALID_1S));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S, 0x1e260000);
}

//
// e.g. FMOV X6, D0
//

inline
int
EmitNeonFmovToGeneral64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    Assert(NeonSizeIsValid(SrcSize, VALID_1D | VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1D, 0x9e260000);
}

//
// e.g. FMOV X7, V0.D[1]
//

inline
int
EmitNeonFmovToGeneralHigh64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{
    UNREFERENCED_PARAMETER(SrcSize);
    Assert(NeonSizeIsValid(SrcSize, VALID_2D));  // TODO: Should this be VALID_1D?
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S /* SIZE_1D */, 0x9eae0000);
}

//
// SCVTF* V.T,D
//

inline
int
EmitNeonConvertScalarCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DstSize,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | ((DstSize & 1) << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonFmovFromGeneral(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    Assert(NeonSizeIsValid(DestSize, VALID_1S));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S, 0x1e270000);
}

//
// e.g. FMOV D0, X6
//

inline
int
EmitNeonFmovFromGeneral64(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    Assert(NeonSizeIsValid(DestSize, VALID_1D | VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1D, 0x9e270000);
}

//
// e.g. FMOV V0.d[1], X7
//

inline
int
EmitNeonFmovFromGeneralHigh64(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    UNREFERENCED_PARAMETER(DestSize);
    Assert(NeonSizeIsValid(DestSize, VALID_2D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, SIZE_1S /* SIZE_1D */, 0x9eaf0000);
}

inline
int
EmitNeonScvtf(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    Assert(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x1e220000);
}

inline
int
EmitNeonScvtf64(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    Assert(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x9e220000);
}

inline
int
EmitNeonUcvtf(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    Assert(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x1e230000);
}

inline
int
EmitNeonUcvtf64(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DstSize
    )
{
    Assert(NeonSizeIsValid(DstSize, VALID_1S | VALID_1D));
    return EmitNeonConvertScalarCommon(Emitter, Dest, Src, DstSize, 0x9e230000);
}

//
// DUP V.T,V.T[index]
//

inline
int
EmitNeonMovElementCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize,
    ULONG Opcode
    )
{
    ULONG Size = SrcSize & 3;
    Assert((SrcIndex << Size) < 16);

    SrcIndex = ((SrcIndex << 1) | 1) << Size;

    return Emitter.EmitFourBytes(Opcode | (SrcIndex << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonDupElement(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE DestSize
    )
{
    Assert(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonMovElementCommon(Emitter, Dest, Src, SrcIndex, DestSize, 0x0e000400 | (((DestSize >> 2) & 1) << 30));
}

inline
int
EmitNeonDup(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    Assert(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));
    return EmitNeonMovElementCommon(Emitter, Dest, NeonRegisterParam(NEONREG_D0 + Src.RawRegister()), 0, DestSize, 0x0e000c00 | (((DestSize >> 2) & 1) << 30));
}

inline
int
EmitNeonIns(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG DestIndex,
    Arm64SimpleRegisterParam Src,
    NEON_SIZE DestSize
    )
{
    Assert(NeonSizeIsValid(DestSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D));
    return EmitNeonMovElementCommon(Emitter, Dest, NeonRegisterParam(NEONREG_D0 + Src.RawRegister()), DestIndex, DestSize, 0x4e001c00);
}

inline
int
EmitNeonSmov(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x0e002c00);
}

inline
int
EmitNeonSmov64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x4e002c00);
}

inline
int
EmitNeonUmov(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x0e003c00);
}

inline
int
EmitNeonUmov64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1B | VALID_1H | VALID_1S | VALID_1D));
    return EmitNeonMovElementCommon(Emitter, NeonRegisterParam(NEONREG_D0 + Dest.RawRegister()), Src, SrcIndex, SrcSize, 0x4e003c00);
}

//
// INS V.T[index],V.T[index]
//

inline
int
EmitNeonInsElement(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG DestIndex,
    NeonRegisterParam Src,
    ULONG SrcIndex,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D));

    ULONG Size = SrcSize & 3;
    Assert((DestIndex << Size) < 16);
    Assert((SrcIndex << Size) < 16);

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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    ULONG Condition,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_1S | VALID_1D));
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

    Assert(EncImmediate < 256);
    return (Op << 29) | (((EncImmediate >> 5) & 7) << 16) | (Cmode << 12) | ((EncImmediate & 0x1f) << 5);
}

inline
int
EmitNeonMovi(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    ULONG64 Immediate,
    NEON_SIZE DestSize
    )
{
    Assert(NeonSizeIsValid(DestSize, VALID_816B | VALID_48H | VALID_24S | VALID_2D | VALID_1D));

    ULONG EncImmediate = ComputeNeonImmediate(Immediate, DestSize);
    if (EncImmediate != 1) {
        return Emitter.EmitFourBytes(0x0f000400 | (((DestSize >> 2) & 1) << 30) | EncImmediate | Dest.RawRegister());
    }

    EncImmediate = ComputeNeonImmediate(~Immediate, DestSize);
    if (EncImmediate != 1) {
        return Emitter.EmitFourBytes(0x2f000400 | (((DestSize >> 2) & 1) << 30) | EncImmediate | Dest.RawRegister());
    }

    Assert(false);
    return 0;
}

//
// TBL Vd.T,Vn.16B,Vm.T
//
//

inline
int
EmitNeonTbl(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NeonRegisterParam Indices,
    NEON_SIZE Size
    )
{

    if (Size == SIZE_8B) {
        return Emitter.EmitFourBytes(0x0e000000 | (Indices.RawRegister() << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());

    } else {

        Assert(Size == SIZE_16B);

        return Emitter.EmitFourBytes(0x4e000000 | (Indices.RawRegister() << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }
}

//
// EXT Vd.T,Vn.T,Vm.T,#x
//

inline
int
EmitNeonExt(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src1,
    NeonRegisterParam Src2,
    ULONG Immediate,
    NEON_SIZE SrcSize
    )
{
    Assert(NeonSizeIsValid(SrcSize, VALID_816B));
    Assert(((SrcSize == SIZE_8B) && (Immediate < 8)) ||
              ((SrcSize == SIZE_16B) && (Immediate < 16)));

    return Emitter.EmitFourBytes(0x2e000000 | (((SrcSize >> 2) & 1) << 30) | (Src2.RawRegister() << 16) | (Immediate << 11) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

//
// LDR (immediate, SIMD&FP) - Unsigned offset form
//

inline
int
EmitNeonLdrStrOffsetCommon(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    NEON_SIZE SrcDestSize,
    Arm64SimpleRegisterParam Addr,
    LONG Offset,
    ULONG Opcode,
    ULONG OpcodeUnscaled
    )
{
    Assert(NeonSizeIsScalar(SrcDestSize));

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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NEON_SIZE DestSize,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdrStrOffsetCommon(Emitter, Dest, DestSize, Addr, Offset, 0x3d400000, 0x3c400000);
}

inline
int
EmitNeonStrOffset(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Source,
    NEON_SIZE SourceSize,
    Arm64SimpleRegisterParam Addr,
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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam SrcDest1,
    NeonRegisterParam SrcDest2,
    NEON_SIZE SrcDestSize,
    Arm64SimpleRegisterParam Addr,
    LONG Offset,
    ULONG Opcode
    )
{
    Assert(NeonSizeIsValid(SrcDestSize, VALID_1S | VALID_1D | VALID_1Q));

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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest1,
    NeonRegisterParam Dest2,
    NEON_SIZE DestSize,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitNeonLdpStpOffsetCommon(Emitter, Dest1, Dest2, DestSize, Addr, Offset, 0x2d400000);
}

inline
int
EmitNeonStpOffset(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Source1,
    NeonRegisterParam Source2,
    NEON_SIZE SourceSize,
    Arm64SimpleRegisterParam Addr,
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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    Arm64SimpleRegisterParam Addr,
    NEON_SIZE SrcDestSize,
    ULONG Opcode
    )
{
    ULONG QSSize = Index << (SrcDestSize & 3);
    if (SrcDestSize == SIZE_1D) {
        QSSize |= 1;
    }

    Assert(QSSize < 16);
    ULONG Op = (SrcDestSize == SIZE_1B) ? 0 : (SrcDestSize == SIZE_1H) ? 2 : 4;

    return Emitter.EmitFourBytes(Opcode | ((QSSize >> 3) << 30) | (Op << 13) | ((QSSize & 7) << 10) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
}

inline
int
EmitNeonLd1(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    Arm64SimpleRegisterParam Addr,
    NEON_SIZE SrcDestSize
    )
{
    return EmitNeonLd1St1Common(Emitter, SrcDest, Index, Addr, SrcDestSize, 0x0d400000);
}

inline
int
EmitNeonSt1(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam SrcDest,
    LONG Index,
    Arm64SimpleRegisterParam Addr,
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
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    Assert(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e285800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesE(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    Assert(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e284800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesImc(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    Assert(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e287800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitNeonAesMc(
    Arm64CodeEmitter &Emitter,
    NeonRegisterParam Dest,
    NeonRegisterParam Src,
    NEON_SIZE SrcSize
    )
{

    UNREFERENCED_PARAMETER(SrcSize);

    Assert(SrcSize == SIZE_16B);

    return Emitter.EmitFourBytes(0x4e286800 | (Src.RawRegister() << 5) | Dest.RawRegister());
}
