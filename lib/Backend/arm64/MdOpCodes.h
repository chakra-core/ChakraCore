//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Instruction encoding macros.  Used by ARM instruction encoder.
//
#define UNUSED 0

//            opcode
//           /           layout
//          /           /          attrib          byte2
//         /           /          /               /         form
//        /           /          /               /         /               unused
//       /           /          /               /         /               /
//      /           /          /               /         /               /         dope
//     /           /          /               /         /               /         /

MACRO(ADD,        Reg3,       0,              UNUSED,   LEGAL_ADDSUB,   UNUSED,   D___)
MACRO(ADDS,       Reg3,       OpSideEffect,   UNUSED,   LEGAL_ADDSUB,   UNUSED,   D__S)
MACRO(ADR,        Reg3,       0,              UNUSED,   LEGAL_LABEL,    UNUSED,   D___)
MACRO(AND,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(ANDS,       Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D__S)
MACRO(ASR,        Reg3,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(B,          Br,         OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BEQ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BNE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLT,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BGT,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BGE,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BCS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BCC,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BHI,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BLS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BMI,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BPL,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BVS,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BVC,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_BLAB,     UNUSED,   D___)
MACRO(BFI,        Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(BFXIL,      Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(BIC,        Reg3,       OpSideEffect,   UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(BL,         CallI,      OpSideEffect,   UNUSED,   LEGAL_CALL,     UNUSED,   D___)
MACRO(BLR,        CallI,      OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(BR,         Br,         OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(CBZ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_CBZ,      UNUSED,   D___)
MACRO(CBNZ,       BrReg2,     OpSideEffect,   UNUSED,   LEGAL_CBZ,      UNUSED,   D___)
MACRO(CLZ,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(CMP,        Reg1,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
MACRO(CMN,        Reg1,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
// CMP src1, src2, SXTW -- used in multiply overflow checks
MACRO(CMP_SXTW,   Reg1,       OpSideEffect,   UNUSED,   LEGAL_REG3_ND,  UNUSED,   D__S)
// CSELcc src1, src2 -- select src1 if cc or src2 if not
MACRO(CSELEQ,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(CSELNE,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(CSELLT,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// CSNEGPL src1, src2 -- select src1 if PL or -src1 if not; used in integer absolute value
MACRO(CSNEGPL,    Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(DEBUGBREAK, Reg1,       OpSideEffect,   UNUSED,   LEGAL_NONE,     UNUSED,   D___)
MACRO(EOR,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
// EOR src1, src2, ASR #31/63 -- used in floating-point-to-integer overflow checks
MACRO(EOR_ASR31,  Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(LDIMM,      Reg2,       0,              UNUSED,   LEGAL_LDIMM,    UNUSED,   DM__)
MACRO(LDP,        Reg3,       0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(LDP_POST,   Reg3,       0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(LDR,        Reg2,       0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LDRS,       Reg2,       0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(LEA,        Reg3,       0,              UNUSED,   LEGAL_LEA,      UNUSED,   D___)
MACRO(LSL,        Reg2,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(LSR,        Reg2,       0,              UNUSED,   LEGAL_SHIFT,    UNUSED,   D___)
MACRO(MOV,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
// Alias of MOV that won't get optimized out when src and dst are the same.
MACRO(MOV_TRUNC,  Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(MOVK,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MOVN,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MOVZ,       Reg2,       0,              UNUSED,   LEGAL_LDIMM_S,    UNUSED,   DM__)
MACRO(MRS_FPCR,   Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)
MACRO(MRS_FPSR,   Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)
MACRO(MSR_FPCR,   Reg2,       0,              UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(MSR_FPSR,   Reg2,       0,              UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(MSUB,       Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(MUL,        Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(MVN,        Reg2,       0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(NOP,        Empty,      0,              UNUSED,   LEGAL_NONE,     UNUSED,   D___)
MACRO(ORR,        Reg3,       0,              UNUSED,   LEGAL_ALU3,     UNUSED,   D___)
MACRO(PLD,        Reg2,       0,              UNUSED,   LEGAL_PLD,      UNUSED,   DL__)
MACRO(REM,        Reg3,       OpSideEffect,   UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(RET,        Reg2,       OpSideEffect,   UNUSED,   LEGAL_REG2_ND,  UNUSED,   D___)
MACRO(SBFX,       Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)
MACRO(SDIV,       Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SMADDL,     Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SMULL,      Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(STP,        Reg3,       0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DL__)
MACRO(STP_PRE,    Reg3,       0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DL__)
MACRO(STR,        Reg2,       0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(SUB,        Reg3,       0,              UNUSED,   LEGAL_ADDSUB,   UNUSED,   D___)
// SUB dst, src1, src2 LSL #4 -- used in prologs with _chkstk calls
MACRO(SUB_LSL4,   Reg3,       0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(SUBS,       Reg3,       OpSideEffect,   UNUSED,   LEGAL_ADDSUB,   UNUSED,   D__S)
MACRO(TBNZ,       BrReg2,     OpSideEffect,   UNUSED,   LEGAL_TBZ,      UNUSED,   D___)
MACRO(TBZ,        BrReg2,     OpSideEffect,   UNUSED,   LEGAL_TBZ,      UNUSED,   D___)
MACRO(TST,        Reg2,       OpSideEffect,   UNUSED,   LEGAL_PSEUDO,   UNUSED,   D__S)
MACRO(UBFX,       Reg3,       0,              UNUSED,   LEGAL_BITFIELD, UNUSED,   D___)

// Pseudo-op that loads the size of the arg out area. A special op with no src is used so that the
// actual arg out size can be fixed up by the encoder.
MACRO(LDARGOUTSZ, Reg1,       0,              UNUSED,   LEGAL_REG1,     UNUSED,   D___)

//VFP instructions:
MACRO(FABS,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FADD,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
// FCSELcc src1, src2 -- select src1 if cc or src2 if not
MACRO(FCSELEQ,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FCSELNE,     Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FCMP,        Reg1,      OpSideEffect,   UNUSED,   LEGAL_REG3_ND,  UNUSED,   D___)
MACRO(FCVT,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTM,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTN,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTP,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FCVTZ,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FDIV,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FLDR,        Reg2,      0,              UNUSED,   LEGAL_LOAD,     UNUSED,   DL__)
MACRO(FLDP,        Reg2,      0,              UNUSED,   LEGAL_LOADP,    UNUSED,   DL__)
MACRO(FMIN,        Reg2,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FMAX,        Reg2,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FMOV,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(FMOV_GEN,    Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   DM__)
MACRO(FMUL,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FNEG,        Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FRINTM,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FRINTP,      Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FSUB,        Reg3,      0,              UNUSED,   LEGAL_REG3,     UNUSED,   D___)
MACRO(FSQRT,       Reg2,      0,              UNUSED,   LEGAL_REG2,     UNUSED,   D___)
MACRO(FSTR,        Reg2,      0,              UNUSED,   LEGAL_STORE,    UNUSED,   DS__)
MACRO(FSTP,        Reg2,      0,              UNUSED,   LEGAL_STOREP,   UNUSED,   DS__)
