//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ARM64Encoder.h"
#include "LegalizeMD.h"

class Encoder;

// use this to encode the immediate field for a bitfield instruction
#define BITFIELD(lsb, width)    ((lsb) | ((width) << 16))

enum RelocType {
    RelocTypeBranch14,
    RelocTypeBranch19,
    RelocTypeBranch26,
    RelocTypeLabelAdr,
    RelocTypeLabelImmed,
    RelocTypeLabel
};

enum InstructionType {
    None    = 0,
    Vfp     = 3,
    A64     = 4,
};

#define RETURN_REG                  RegR0
#define FIRST_INT_ARG_REG           RegR0
#define LAST_INT_ARG_REG            RegR7
#define NUM_INT_ARG_REGS\
    ((LAST_INT_ARG_REG - FIRST_INT_ARG_REG) + 1)

#define FIRST_CALLEE_SAVED_GP_REG   RegR19
#define LAST_CALLEE_SAVED_GP_REG    RegR28
#define CALLEE_SAVED_GP_REG_COUNT\
    ((LAST_CALLEE_SAVED_GP_REG - FIRST_CALLEE_SAVED_GP_REG) + 1)

// Note that CATCH_OBJ_REG and ALT_LOCALS_PTR are implicitly referenced in
// arm64_CallEhFrame.asm and must be updated there as well if these are changed.
#define CATCH_OBJ_REG               RegR1
#define UNUSED_REG_FOR_STACK_ALIGN  RegR11
#define SP_ALLOC_SCRATCH_REG        RegR15
#define SCRATCH_REG                 RegR17
#define ALT_LOCALS_PTR              RegR28

#define RETURN_DBL_REG              RegD0
#define FIRST_CALLEE_SAVED_DBL_REG  RegD8
#define LAST_CALLEE_SAVED_DBL_REG   RegD15
#define CALLEE_SAVED_DOUBLE_REG_COUNT\
    ((LAST_CALLEE_SAVED_DBL_REG - FIRST_CALLEE_SAVED_DBL_REG) + 1)
#define FIRST_CALLEE_SAVED_DBL_REG_NUM  8
#define LAST_CALLEE_SAVED_DBL_REG_NUM   15


// See comment in LowerEntryInstr: even in a global function, we'll home x0 and x1
#define MIN_HOMED_PARAM_REGS 2

#define FRAME_REG           RegFP

//
// Opcode dope
//

#define DMOVE              0x0001
#define DLOAD              0x0002
#define DSTORE             0x0003
#define DMASK              0x0007
#define DHALFORSB          0x0020    // halfword or signed byte
#define DSUPDATE           0x0100
#define DSSUB              0x0200
#define DSPOST             0x0400
#define DSBIT              0x0800

#define D___               (0)
#define D__S               (DSBIT)
#define DM__               (DMOVE)
#define DL__               (DLOAD)
#define DLH_               (DLOAD | DHALFORSB)
#define DS__               (DSTORE)
#define DSH_               (DSTORE | DHALFORSB)
#define DLUP               (DLOAD | DSUPDATE | DSPOST)
#define DSUS               (DSTORE | DSUPDATE | DSSUB)

#define ISMOVE(o)          ((EncoderMD::GetOpdope(o) & DMASK) == DMOVE)
#define ISLOAD(o)          ((EncoderMD::GetOpdope(o) & DMASK) == DLOAD)
#define ISSTORE(o)         ((EncoderMD::GetOpdope(o) & DMASK) == DSTORE)

#define ISSHIFTERUPDATE(o) ((EncoderMD::GetOpdope(o) & DSUPDATE) != 0)
#define ISSHIFTERSUB(o)    ((EncoderMD::GetOpdope(o) & DSSUB) != 0)
#define ISSHIFTERPOST(o)   ((EncoderMD::GetOpdope(o) & DSPOST) != 0)

#define SETS_SBIT(o)       ((EncoderMD::GetOpdope(o) & DSBIT) != 0)

#define IS_CONST_0000003F(x) (((x) & ~0x0000003f) == 0)
#define IS_CONST_0000007F(x) (((x) & ~0x0000007f) == 0)
#define IS_CONST_000000FF(x) (((x) & ~0x000000ff) == 0)
#define IS_CONST_000003FF(x) (((x) & ~0x000003ff) == 0)
#define IS_CONST_00000FFF(x) (((x) & ~0x00000fff) == 0)
#define IS_CONST_0000FFFF(x) (((x) & ~0x0000ffff) == 0)
#define IS_CONST_000FFFFF(x) (((x) & ~0x000fffff) == 0)
#define IS_CONST_007FFFFF(x) (((x) & ~0x007fffff) == 0)
#define IS_CONST_00FFF000(x) (((x) & ~0x00fff000) == 0)
#define IS_CONST_003F003F(x) (((x) & ~0x003f003f) == 0)

#define IS_CONST_NEG_7(x)    (((x) & ~0x0000003f) == ~0x0000003f)
#define IS_CONST_NEG_8(x)    (((x) & ~0x0000007f) == ~0x0000007f)
#define IS_CONST_NEG_9(x)    (((x) & ~0x000000ff) == ~0x000000ff)
#define IS_CONST_NEG_12(x)   (((x) & ~0x000007ff) == ~0x000007ff)
#define IS_CONST_NEG_21(x)   (((x) & ~0x000fffff) == ~0x000fffff)
#define IS_CONST_NEG_24(x)   (((x) & ~0x007fffff) == ~0x007fffff)

#define IS_CONST_INT7(x)     (IS_CONST_0000003F(x) || IS_CONST_NEG_7(x))
#define IS_CONST_INT8(x)     (IS_CONST_0000007F(x) || IS_CONST_NEG_8(x))
#define IS_CONST_INT9(x)     (IS_CONST_000000FF(x) || IS_CONST_NEG_9(x))
#define IS_CONST_INT12(x)    (IS_CONST_00000FFF(x) || IS_CONST_NEG_12(x))
#define IS_CONST_INT21(x)    (IS_CONST_000FFFFF(x) || IS_CONST_NEG_21(x))
#define IS_CONST_INT24(x)    (IS_CONST_007FFFFF(x) || IS_CONST_NEG_24(x))

#define IS_CONST_UINT6(x)    IS_CONST_0000003F(x)
#define IS_CONST_UINT7(x)    IS_CONST_0000007F(x)
#define IS_CONST_UINT8(x)    IS_CONST_000000FF(x)
#define IS_CONST_UINT10(x)   IS_CONST_000003FF(x)
#define IS_CONST_UINT12(x)   IS_CONST_00000FFF(x)
#define IS_CONST_UINT16(x)   IS_CONST_0000FFFF(x)
#define IS_CONST_UINT12LSL12(x) IS_CONST_00FFF000(x)
#define IS_CONST_UINT6UINT6(x) IS_CONST_003F003F(x)

///---------------------------------------------------------------------------
///
/// class EncoderReloc
///
///---------------------------------------------------------------------------

class EncodeReloc
{
public:
    static void     New(EncodeReloc **pHead, RelocType relocType, BYTE *offset, IR::Instr *relocInstr, ArenaAllocator *alloc);

public:
    EncodeReloc *   m_next;
    RelocType       m_relocType;
    BYTE *          m_consumerOffset;  // offset in instruction stream
    IR::Instr *     m_relocInstr;
};



///---------------------------------------------------------------------------
///
/// class EncoderMD
///
///---------------------------------------------------------------------------

class EncoderMD
{
public:
    EncoderMD(Func * func) : m_func(func) { }
    ptrdiff_t       Encode(IR::Instr * instr, BYTE *pc, BYTE* beginCodeAddress = nullptr);
    void            Init(Encoder *encoder);
    void            ApplyRelocs(size_t codeBufferAddress, size_t codeSize, uint* bufferCRC, BOOL isBrShorteningSucceeded, bool isFinalBufferValidation = false);
    static bool     TryConstFold(IR::Instr *instr, IR::RegOpnd *regOpnd);
    static bool     TryFold(IR::Instr *instr, IR::RegOpnd *regOpnd);
    BYTE            GetRegEncode(IR::RegOpnd *regOpnd);
    BYTE            GetFloatRegEncode(IR::RegOpnd *regOpnd);
    static BYTE     GetRegEncode(RegNum reg);
    static uint32   GetOpdope(IR::Instr *instr);
    static uint32   GetOpdope(Js::OpCode op);

    static bool     IsLoad(IR::Instr *instr)
    {
        return ISLOAD(instr->m_opcode);
    }

    static bool     IsStore(IR::Instr *instr)
    {
        return ISSTORE(instr->m_opcode);
    }

    static bool     IsShifterUpdate(IR::Instr *instr)
    {
        return ISSHIFTERUPDATE(instr->m_opcode);
    }

    static bool     IsShifterSub(IR::Instr *instr)
    {
        return ISSHIFTERSUB(instr->m_opcode);
    }

    static bool     IsShifterPost(IR::Instr *instr)
    {
        return ISSHIFTERPOST(instr->m_opcode);
    }

    static bool     SetsSBit(IR::Instr *instr)
    {
        return SETS_SBIT(instr->m_opcode);
    }

    static DWORD BranchOffset_26(int64 x);
    
    void            AddLabelReloc(BYTE* relocAddress);

    static bool     CanEncodeLogicalConst(IntConstType constant, int size);
    // ToDo (SaAgarwa) Copied from ARM32 to compile. Validate is this correct
    static bool     CanEncodeLoadStoreOffset(int32 offset) { return IS_CONST_UINT12(offset); }
    static void     BaseAndOffsetFromSym(IR::SymOpnd *symOpnd, RegNum *pBaseReg, int32 *pOffset, Func * func);
    void            EncodeInlineeCallInfo(IR::Instr *instr, uint32 offset);
private:
    Func *          m_func;
    Encoder *       m_encoder;
    BYTE *          m_pc;
    EncodeReloc *   m_relocList;
private:
    ULONG           GenerateEncoding(IR::Instr* instr, BYTE *pc);
    bool            CanonicalizeInstr(IR::Instr *instr);
    void            CanonicalizeLea(IR::Instr * instr);
    bool            DecodeMemoryOpnd(IR::Opnd* opnd, ARM64_REGISTER &baseRegResult, ARM64_REGISTER &indexRegResult, BYTE &indexScale, int32 &offset);
    static bool     EncodeLogicalConst(IntConstType constant, DWORD * result, int size);

    // General 1-operand instructions (BR, RET)
    template<typename _RegFunc64> int EmitOp1Register64(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc64 reg64);

    // General 2-operand instructions (CLZ, MOV)
    template<typename _RegFunc32, typename _RegFunc64> int EmitOp2Register(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64);

    // General 3-operand instructions (ADD, AND, SUB, etc) follow a very standard pattern
    template<typename _RegFunc32, typename _RegFunc64> int EmitOp3Register(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64);
    template<typename _RegFunc32, typename _RegFunc64> int EmitOp3RegisterShifted(Arm64CodeEmitter &Emitter, IR::Instr* instr, SHIFT_EXTEND_TYPE shiftType, int shiftAmount, _RegFunc32 reg32, _RegFunc64 reg64);
    template<typename _ImmFunc32, typename _ImmFunc64> int EmitOp3Immediate(Arm64CodeEmitter &Emitter, IR::Instr* instr, _ImmFunc32 imm32, _ImmFunc64 imm64);
    template<typename _RegFunc32, typename _RegFunc64, typename _ImmFunc32, typename _ImmFunc64> int EmitOp3RegisterOrImmediate(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64, _ImmFunc32 imm32, _ImmFunc64 imm64);
    template<typename _RegFunc32, typename _RegFunc64, typename _ImmFunc32, typename _ImmFunc64> int EmitOp3RegisterOrImmediateExtendSPReg(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64, _ImmFunc32 imm32, _ImmFunc64 imm64);
    template<typename _RegFunc32, typename _RegFunc64, typename _ImmFunc32, typename _ImmFunc64> int EmitOp3RegisterOrImmediateExtendSP(Arm64CodeEmitter &Emitter, IR::Instr* instr, _RegFunc32 reg32, _RegFunc64 reg64, _ImmFunc32 imm32, _ImmFunc64 imm64);

    // Load/store operations
    int EmitPrefetch(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd);
    template<typename _RegFunc8, typename _RegFunc16, typename _RegFunc32, typename _RegFunc64, typename _OffFunc8, typename _OffFunc16, typename _OffFunc32, typename _OffFunc64>
        int EmitLoadStore(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDstOpnd, _RegFunc8 reg8, _RegFunc16 reg16, _RegFunc32 reg32, _RegFunc64 reg64, _OffFunc8 off8, _OffFunc16 off16, _OffFunc32 off32, _OffFunc64 off64);
    template<typename _OffFunc32, typename _OffFunc64> int EmitLoadStorePair(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDst1Opnd, IR::Opnd* srcDst2Opnd, _OffFunc32 off32, _OffFunc64 off64);

    // Branch operations
    template<typename _Emitter> int EmitUnconditionalBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter);
    int             EmitConditionalBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, int condition);
    template<typename _Emitter, typename _Emitter64> int EmitCompareAndBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter, _Emitter64 emitter64);
    template<typename _Emitter> int EmitTestAndBranch(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter);

    // Misc operations
    template<typename _Emitter, typename _Emitter64> int EmitMovConstant(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Emitter emitter, _Emitter64 emitter64);
    template<typename _Emitter, typename _Emitter64> int EmitBitfield(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter, _Emitter64 emitter64);
    template<typename _Emitter, typename _Emitter64> int EmitConditionalSelect(Arm64CodeEmitter &Emitter, IR::Instr *instr, int condition, _Emitter emitter, _Emitter64 emitter64);

    // Floating point instructions
    template<typename _Emitter> int EmitOp2FpRegister(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter);
    template<typename _Emitter> int EmitOp2FpRegister(Arm64CodeEmitter &Emitter, IR::Opnd* opnd1, IR::Opnd* opnd2, _Emitter emitter);
    template<typename _Emitter> int EmitOp3FpRegister(Arm64CodeEmitter &Emitter, IR::Instr *instr, _Emitter emitter);
    template<typename _LoadStoreFunc> int EmitLoadStoreFp(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDstOpnd, _LoadStoreFunc loadStore);
    template<typename _LoadStoreFunc> int EmitLoadStoreFpPair(Arm64CodeEmitter &Emitter, IR::Instr* instr, IR::Opnd* memOpnd, IR::Opnd* srcDst1Opnd, IR::Opnd* srcDst2Opnd, _LoadStoreFunc loadStore);
    template<typename _Int32Func, typename _Uint32Func, typename _Int64Func, typename _Uint64Func> int EmitConvertToInt(Arm64CodeEmitter &Emitter, IR::Instr* instr, _Int32Func toInt32, _Uint32Func toUint32, _Int64Func toInt64, _Uint64Func toUint64);
    template<typename _Emitter> int EmitConditionalSelectFp(Arm64CodeEmitter &Emitter, IR::Instr *instr, int condition, _Emitter emitter);
};
