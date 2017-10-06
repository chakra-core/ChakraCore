//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Instruction encoding macros.  Used by ARM instruction encoder.
//
#define INSTR_TYPE(x) (x)

//            opcode
//           /          layout
//          /          /         attrib             byte2
//         /          /         /                   /         form
//        /          /         /                   /         /          opbyte
//       /          /         /                   /         /          /
//      /          /         /                   /         /          /                  dope
//     /          /         /                   /         /          /                  /

MACRO(ADD,     Reg3,       0,              0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_ADD),  D___)
MACRO(ADDS,    Reg3,       OpSideEffect,   0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_ADD),  D__S)
MACRO(AND,     Reg3,       0,              0,  LEGAL_ALU3,     INSTR_TYPE(Forms_AND),  D___)
MACRO(ANDS,    Reg3,       0,              0,  LEGAL_ALU3,     INSTR_TYPE(Forms_AND),  D___)
MACRO(ASR,     Reg3,       0,              0,  LEGAL_SHIFT,    INSTR_TYPE(Forms_ASR),  D___)

MACRO(B,       Br,         OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_B),    D___)
MACRO(BR,      Br,         OpSideEffect,   0,  LEGAL_BREG,     INSTR_TYPE(Forms_BX),   D___)
MACRO(BL,      CallI,      OpSideEffect,   0,  LEGAL_CALL,     INSTR_TYPE(Forms_BL),   D___)
MACRO(BLR,     CallI,      OpSideEffect,   0,  LEGAL_BREG,     INSTR_TYPE(Forms_BLX),  D___)

MACRO(BIC,     Reg3,       OpSideEffect,   0,  LEGAL_ALU3,     INSTR_TYPE(Forms_BIC),  D___)

// B<C>: explicit and resolved in encoder
MACRO(BEQ,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BEQ),    D___)
MACRO(BNE,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BNE),    D___)
MACRO(BLT,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BLT),    D___)
MACRO(BLE,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BLE),    D___)
MACRO(BGT,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BGT),    D___)
MACRO(BGE,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BGE),    D___)

MACRO(BCS,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BCS),    D___)
MACRO(BCC,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BCC),    D___)
MACRO(BHI,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BHI),    D___)
MACRO(BLS,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BLS),    D___)
MACRO(BMI,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BMI),    D___)
MACRO(BPL,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BPL),    D___)
MACRO(BVS,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BVS),    D___)
MACRO(BVC,     BrReg2,     OpSideEffect,   0,  LEGAL_BLAB,     INSTR_TYPE(Forms_BVC),    D___)
MACRO(DEBUGBREAK,  Reg1,   OpSideEffect,   0,  LEGAL_NONE,     INSTR_TYPE(Forms_DBGBRK), D___)

MACRO(CLZ,     Reg2,       0,              0,  LEGAL_REG2,     INSTR_TYPE(Forms_CLZ),  D___)
MACRO(CMP,     Reg1,       OpSideEffect,   0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_CMP),  D___)
MACRO(CMN,     Reg1,       OpSideEffect,   0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_CMN),  D___)
// CMP src1, src, ASR #31/63
MACRO(CMP_ASR31,Reg1,      OpSideEffect,   0,  LEGAL_CMP_SH,   INSTR_TYPE(Forms_CMP_ASR31),D___)

MACRO(EOR,     Reg3,       0,              0,  LEGAL_ALU3,     INSTR_TYPE(Forms_EOR),  D___)
MACRO(EOR_ASR31,Reg3,      0,              0,  LEGAL_ALU3,     INSTR_TYPE(Forms_EOR_ASR31),D___)

// LDIMM: Load Immediate
MACRO(LDIMM,   Reg2,       0,              0,  LEGAL_LDIMM,    INSTR_TYPE(Forms_LDIMM), DM__)

// LDR: Load register from memory
MACRO(LDR,     Reg2,       0,              0,  LEGAL_LOAD,     INSTR_TYPE(Forms_LDRN), DL__)
MACRO(LDRS,    Reg2,       0,              0,  LEGAL_LOAD,     INSTR_TYPE(Forms_LDRN), DL__)
MACRO(LDP,     Reg3,       0,              0,  LEGAL_LOADP,    INSTR_TYPE(Forms_LDP),  DL__)
MACRO(LDP_POST,Reg3,       0,              0,  LEGAL_LOADP,    INSTR_TYPE(Forms_LDP),  DL__)

// LEA: Load Effective Address
MACRO(LEA,     Reg3,       0,              0,  LEGAL_LEA,      INSTR_TYPE(Forms_LEA),  D___)

// Shifter operands
MACRO(LSL,     Reg2,       0,              0,  LEGAL_SHIFT,    INSTR_TYPE(Forms_LSL), D___)
MACRO(LSR,     Reg2,       0,              0,  LEGAL_SHIFT,    INSTR_TYPE(Forms_LSR), D___)

MACRO(MOV,     Reg2,       0,              0,  LEGAL_ALU2,     INSTR_TYPE(Forms_MOV), DM__)
MACRO(MOVK,    Reg2,       0,              0,  LEGAL_MOVIMM16, INSTR_TYPE(Forms_MOVIMM), DM__)
MACRO(MOVN,    Reg2,       0,              0,  LEGAL_MOVIMM16, INSTR_TYPE(Forms_MOVIMM), DM__)
MACRO(MOVZ,    Reg2,       0,              0,  LEGAL_MOVIMM16, INSTR_TYPE(Forms_MOVIMM), DM__)

MACRO(MUL,     Reg3,       0,              0,  LEGAL_REG3,     INSTR_TYPE(Forms_MUL),  D___)

// SMULL dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
MACRO(SMULL,   Reg3,       0,            0,  LEGAL_REG3,     INSTR_TYPE(Forms_SMULL),D___)

// SMADDL dst, dst, src1, src2. src1 and src2 are 32-bit. dst is 64-bit.
MACRO(SMADDL,  Reg3,       0,            0,  LEGAL_REG3,     INSTR_TYPE(Forms_SMLAL),D___)

// MSUB dst, src1, src2: Multiply and Subtract. We use 3 registers: dst = src1 - src2 * dst
MACRO(MSUB,    Reg3,       0,              0,  LEGAL_REG3,     INSTR_TYPE(Forms_MLS),  D___)

// MVN: Move NOT (register); Format 4
MACRO(MVN,     Reg2,       0,              0,  LEGAL_ALU2,     INSTR_TYPE(Forms_MVN),  D___)

// NOP: No-op
MACRO(NOP,     Empty,      0,              0,  LEGAL_NONE,     INSTR_TYPE(Forms_NOP),   D___)

MACRO(ORR,     Reg3,       0,              0,  LEGAL_ALU3,     INSTR_TYPE(Forms_ORR),  D___)

MACRO(PLD,     Reg2,       0,              0,  LEGAL_LOAD,     INSTR_TYPE(Forms_PLD),  DL__)

// RET: pseudo-op for function return
MACRO(RET,     Reg2,       OpSideEffect,   0,  LEGAL_REG2,     INSTR_TYPE(Forms_RET),  D___)

// REM: pseudo-op
MACRO(REM,     Reg3,       OpSideEffect,   0,  LEGAL_REG3,     INSTR_TYPE(Forms_REM),  D___)

// SDIV: Signed divide
MACRO(SDIV,    Reg3,       0,              0,  LEGAL_REG3,     INSTR_TYPE(Forms_SDIV),  D___)

// STR: Store register to memory
MACRO(STR,     Reg2,       0,              0,  LEGAL_STORE,    INSTR_TYPE(Forms_STRN), DS__)
MACRO(STP,     Reg3,       0,              0,  LEGAL_STOREP,   INSTR_TYPE(Forms_STP),  DL__)
MACRO(STP_PRE, Reg3,       0,              0,  LEGAL_STOREP,   INSTR_TYPE(Forms_STP),  DL__)

MACRO(SUB,     Reg3,       0,              0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_SUB), D___)
MACRO(SUBS,    Reg3,       OpSideEffect,   0,  LEGAL_ADDSUB,   INSTR_TYPE(Forms_SUB), D__S)

MACRO(TIOFLW,  Reg1,       OpSideEffect,   0,  LEGAL_CMP1,     INSTR_TYPE(Forms_TIOFLW), D___)
MACRO(TST,     Reg2,       OpSideEffect,   0,  LEGAL_CMP,      INSTR_TYPE(Forms_TST),  D___)

// Pseudo-op that loads the size of the arg out area. A special op with no src is used so that the
// actual arg out size can be fixed up by the encoder.
MACRO(LDARGOUTSZ,Reg1,     0,              0,  LEGAL_REG1,     INSTR_TYPE(Forms_LDIMM), D___)

// Pseudo-op: dst = EOR src, src ASR #31
MACRO(CLRSIGN, Reg2,       0,              0,  LEGAL_REG2,     INSTR_TYPE(Forms_CLRSIGN), D___)

// Pseudo-op: dst = SUB src1, src2 ASR #31
MACRO(SBCMPLNT, Reg3,      0,              0,  LEGAL_REG3,     INSTR_TYPE(Forms_SBCMPLNT), D___)


//VFP instructions:
MACRO(VABS,     Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VABS),   D___)
MACRO(VADDF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VADDF64),   D___)
MACRO(VCMPF64,  Reg1,      OpSideEffect,   0,  LEGAL_CMP_SH,    INSTR_TYPE(Forms_VCMPF64),   D___)
MACRO(VCVTF64F32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64F32),   D___)
MACRO(VCVTF32F64, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF32F64),   D___)
MACRO(VCVTF64S32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64S32),   D___)
MACRO(VCVTF64U32, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTF64U32),   D___)
MACRO(VCVTS32F64, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTS32F64),   D___)
MACRO(VCVTRS32F64, Reg2,   0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VCVTRS32F64),   D___)
MACRO(VDIVF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VDIVF64),   D___)
MACRO(VLDR,     Reg2,      0,              0,  LEGAL_VLOAD,     INSTR_TYPE(Forms_VLDR),   DL__)
MACRO(VLDR32,   Reg2,      0,              0,  LEGAL_VLOAD,     INSTR_TYPE(Forms_VLDR32), DL__) //single precision float load
MACRO(VMOV,     Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VMOV),   DM__)
MACRO(VMOVARMVFP, Reg2,    0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VMOVARMVFP),   DM__)
MACRO(VMRS,     Empty,     OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMRS),   D___)
MACRO(VMRSR,    Reg1,      OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMRSR),   D___)
MACRO(VMSR,     Reg1,      OpSideEffect,   0,  LEGAL_NONE,      INSTR_TYPE(Forms_VMSR),   D___)
MACRO(VMULF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VMULF64),   D___)
MACRO(VNEGF64,  Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VNEGF64),   D___)
MACRO(VSUBF64,  Reg3,      0,              0,  LEGAL_REG3,      INSTR_TYPE(Forms_VSUBF64),   D___)
MACRO(VSQRT,    Reg2,      0,              0,  LEGAL_REG2,      INSTR_TYPE(Forms_VSQRT),   D___)
MACRO(VSTR,     Reg2,      0,              0,  LEGAL_VSTORE,    INSTR_TYPE(Forms_VSTR),   DS__)
MACRO(VSTR32,   Reg2,      0,              0,  LEGAL_VSTORE,    INSTR_TYPE(Forms_VSTR32), DS__) //single precision float store
