//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Internal  Listing  Machine
// Name      Name     Encode    Type        BitVec
//------------------------------------------------------------------------

#ifndef REGDAT
#define REGDAT(Name, Listing,    Encode,    Type,    BitVec)
#endif

// Illegal registers - must be first and have a value of 0

//            Internal Name
//           /      Listing Name
//          /      /        Machine Encode
//         /      /        /
//        /      /        /          Type
//       /      /        /          /           BitVec
//      /      /        /          /           /
#ifdef _WIN32
REGDAT(NOREG, noreg,    0xf,    TyIllegal,    RA_DONTALLOCATE)
REGDAT(RAX,   rax,      0,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RCX,   rcx,      1,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RDX,   rdx,      2,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RBX,   rbx,      3,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(RSP,   rsp,      4,      TyInt64,      RA_DONTALLOCATE)
REGDAT(RBP,   rbp,      5,      TyInt64,      RA_DONTALLOCATE)
REGDAT(RSI,   rsi,      6,      TyInt64,      RA_CALLEESAVE)
REGDAT(RDI,   rdi,      7,      TyInt64,      RA_CALLEESAVE)
REGDAT(R8,    r8,       0,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R9,    r9,       1,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R10,   r10,      2,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R11,   r11,      3,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R12,   r12,      4,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R13,   r13,      5,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R14,   r14,      6,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R15,   r15,      7,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)

REGDAT(XMM0,  xmm0,     0,      TyFloat64,    0)
REGDAT(XMM1,  xmm1,     1,      TyFloat64,    0)
REGDAT(XMM2,  xmm2,     2,      TyFloat64,    0)
REGDAT(XMM3,  xmm3,     3,      TyFloat64,    0)
REGDAT(XMM4,  xmm4,     4,      TyFloat64,    0)
REGDAT(XMM5,  xmm5,     5,      TyFloat64,    0)
REGDAT(XMM6,  xmm6,     6,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM7,  xmm7,     7,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM8,  xmm8,     0,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM9,  xmm9,     1,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM10, xmm10,    2,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM11, xmm11,    3,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM12, xmm12,    4,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM13, xmm13,    5,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM14, xmm14,    6,      TyFloat64,    RA_CALLEESAVE)
REGDAT(XMM15, xmm15,    7,      TyFloat64,    RA_CALLEESAVE)

#else  // System V x64
REGDAT(NOREG, noreg,    0xf,    TyIllegal,    RA_DONTALLOCATE)
REGDAT(RAX,   rax,      0,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RCX,   rcx,      1,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RDX,   rdx,      2,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(RBX,   rbx,      3,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(RSP,   rsp,      4,      TyInt64,      RA_DONTALLOCATE)
REGDAT(RBP,   rbp,      5,      TyInt64,      RA_DONTALLOCATE)
REGDAT(RSI,   rsi,      6,      TyInt64,      RA_CALLERSAVE)
REGDAT(RDI,   rdi,      7,      TyInt64,      RA_CALLERSAVE)
REGDAT(R8,    r8,       0,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R9,    r9,       1,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R10,   r10,      2,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R11,   r11,      3,      TyInt64,      RA_CALLERSAVE | RA_BYTEABLE)
REGDAT(R12,   r12,      4,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R13,   r13,      5,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R14,   r14,      6,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)
REGDAT(R15,   r15,      7,      TyInt64,      RA_CALLEESAVE | RA_BYTEABLE)

REGDAT(XMM0,  xmm0,     0,      TyFloat64,    0)
REGDAT(XMM1,  xmm1,     1,      TyFloat64,    0)
REGDAT(XMM2,  xmm2,     2,      TyFloat64,    0)
REGDAT(XMM3,  xmm3,     3,      TyFloat64,    0)
REGDAT(XMM4,  xmm4,     4,      TyFloat64,    0)
REGDAT(XMM5,  xmm5,     5,      TyFloat64,    0)
REGDAT(XMM6,  xmm6,     6,      TyFloat64,    0)
REGDAT(XMM7,  xmm7,     7,      TyFloat64,    0)
REGDAT(XMM8,  xmm8,     0,      TyFloat64,    0)
REGDAT(XMM9,  xmm9,     1,      TyFloat64,    0)
REGDAT(XMM10, xmm10,    2,      TyFloat64,    0)
REGDAT(XMM11, xmm11,    3,      TyFloat64,    0)
REGDAT(XMM12, xmm12,    4,      TyFloat64,    0)
REGDAT(XMM13, xmm13,    5,      TyFloat64,    0)
REGDAT(XMM14, xmm14,    6,      TyFloat64,    0)
REGDAT(XMM15, xmm15,    7,      TyFloat64,    0)
#endif  // !_WIN32

#ifndef REG_INT_ARG
#define REG_INT_ARG(Index, Name)
#endif

#ifndef REG_XMM_ARG
#define REG_XMM_ARG(Index, Name)
#endif

#ifdef _WIN32
REG_INT_ARG(0, RCX)
REG_INT_ARG(1, RDX)
REG_INT_ARG(2, R8)
REG_INT_ARG(3, R9)

REG_XMM_ARG(0, XMM0)
REG_XMM_ARG(1, XMM1)
REG_XMM_ARG(2, XMM2)
REG_XMM_ARG(3, XMM3)

#else  // System V x64
REG_INT_ARG(0, RDI)
REG_INT_ARG(1, RSI)
REG_INT_ARG(2, RDX)
REG_INT_ARG(3, RCX)
REG_INT_ARG(4, R8)
REG_INT_ARG(5, R9)

REG_XMM_ARG(0, XMM0)
REG_XMM_ARG(1, XMM1)
REG_XMM_ARG(2, XMM2)
REG_XMM_ARG(3, XMM3)
REG_XMM_ARG(4, XMM4)
REG_XMM_ARG(5, XMM5)
REG_XMM_ARG(6, XMM6)
REG_XMM_ARG(7, XMM7)
#endif  // !_WIN32

#undef REGDAT
#undef REG_INT_ARG
#undef REG_XMM_ARG
