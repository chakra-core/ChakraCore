//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "ARM64LogicalImmediates.h"


#define IS_CONST_00001FFF(x) (((x) & ~0x00001fff) == 0)
#define IS_CONST_0003FFFF(x) (((x) & ~0x0003ffff) == 0)
#define IS_CONST_01FFFFFF(x) (((x) & ~0x01ffffff) == 0)

#define IS_CONST_NEG_14(x)   (((x) & ~0x00001fff) == ~0x00001fff)
#define IS_CONST_NEG_19(x)   (((x) & ~0x0003ffff) == ~0x0003ffff)
#define IS_CONST_NEG_26(x)   (((x) & ~0x01ffffff) == ~0x01ffffff)

#define IS_CONST_INT14(x)    (IS_CONST_00001FFF(x) || IS_CONST_NEG_14(x))
#define IS_CONST_INT19(x)    (IS_CONST_0003FFFF(x) || IS_CONST_NEG_19(x))
#define IS_CONST_INT26(x)    (IS_CONST_01FFFFFF(x) || IS_CONST_NEG_26(x))


//
// Core types
//

typedef UCHAR ARM64_REGISTER;

enum
{
    //
    // Base register set (0-49)
    //
    // PC is not represented since it's not numbered.
    //

    ARMREG_R0 = 0,
    ARMREG_R1 = 1,
    ARMREG_R2 = 2,
    ARMREG_R3 = 3,
    ARMREG_R4 = 4,
    ARMREG_R5 = 5,
    ARMREG_R6 = 6,
    ARMREG_R7 = 7,
    ARMREG_R8 = 8,
    ARMREG_R9 = 9,
    ARMREG_R10 = 10,
    ARMREG_R11 = 11,
    ARMREG_R12 = 12,
    ARMREG_R13 = 13,
    ARMREG_R14 = 14,
    ARMREG_R15 = 15,
    ARMREG_R16 = 16,
    ARMREG_R17 = 17,
    ARMREG_R18 = 18,
    ARMREG_R19 = 19,
    ARMREG_R20 = 20,
    ARMREG_R21 = 21,
    ARMREG_R22 = 22,
    ARMREG_R23 = 23,
    ARMREG_R24 = 24,
    ARMREG_R25 = 25,
    ARMREG_R26 = 26,
    ARMREG_R27 = 27,
    ARMREG_R28 = 28,
    ARMREG_FP = 29,
    ARMREG_LR = 30,
    ARMREG_SP = 31,
    ARMREG_ZR = ARMREG_SP,

    ARMREG_FIRST = ARMREG_R0,
    ARMREG_LAST = ARMREG_ZR,
    ARMREG_COUNT = ARMREG_LAST - ARMREG_FIRST + 1,

    ARMREG_INVALID = 49,

    //
    // NEON register set (50-99)
    //

    NEONREG_BASE = 50,
    NEONREG_Q0 = NEONREG_BASE + 0,
    NEONREG_Q1 = NEONREG_BASE + 1,
    NEONREG_Q2 = NEONREG_BASE + 2,
    NEONREG_Q3 = NEONREG_BASE + 3,
    NEONREG_Q4 = NEONREG_BASE + 4,
    NEONREG_Q5 = NEONREG_BASE + 5,
    NEONREG_Q6 = NEONREG_BASE + 6,
    NEONREG_Q7 = NEONREG_BASE + 7,
    NEONREG_Q8 = NEONREG_BASE + 8,
    NEONREG_Q9 = NEONREG_BASE + 9,
    NEONREG_Q10 = NEONREG_BASE + 10,
    NEONREG_Q11 = NEONREG_BASE + 11,
    NEONREG_Q12 = NEONREG_BASE + 12,
    NEONREG_Q13 = NEONREG_BASE + 13,
    NEONREG_Q14 = NEONREG_BASE + 14,
    NEONREG_Q15 = NEONREG_BASE + 15,
    NEONREG_Q16 = NEONREG_BASE + 16,
    NEONREG_Q17 = NEONREG_BASE + 17,
    NEONREG_Q18 = NEONREG_BASE + 18,
    NEONREG_Q19 = NEONREG_BASE + 19,
    NEONREG_Q20 = NEONREG_BASE + 20,
    NEONREG_Q21 = NEONREG_BASE + 21,
    NEONREG_Q22 = NEONREG_BASE + 22,
    NEONREG_Q23 = NEONREG_BASE + 23,
    NEONREG_Q24 = NEONREG_BASE + 24,
    NEONREG_Q25 = NEONREG_BASE + 25,
    NEONREG_Q26 = NEONREG_BASE + 26,
    NEONREG_Q27 = NEONREG_BASE + 27,
    NEONREG_Q28 = NEONREG_BASE + 28,
    NEONREG_Q29 = NEONREG_BASE + 29,
    NEONREG_Q30 = NEONREG_BASE + 30,
    NEONREG_Q31 = NEONREG_BASE + 31,

    NEONREG_FIRST = NEONREG_Q0,
    NEONREG_LAST = NEONREG_Q31,
    NEONREG_COUNT = NEONREG_LAST - NEONREG_FIRST + 1,

    NEONREG_D0  = NEONREG_Q0,
    NEONREG_D1  = NEONREG_Q1,
    NEONREG_D2  = NEONREG_Q2,
    NEONREG_D3  = NEONREG_Q3,
    NEONREG_D4  = NEONREG_Q4,
    NEONREG_D5  = NEONREG_Q5,
    NEONREG_D6  = NEONREG_Q6,
    NEONREG_D7  = NEONREG_Q7,
    NEONREG_D8  = NEONREG_Q8,
    NEONREG_D9  = NEONREG_Q9,
    NEONREG_D10 = NEONREG_Q10,
    NEONREG_D11 = NEONREG_Q11,
    NEONREG_D12 = NEONREG_Q12,
    NEONREG_D13 = NEONREG_Q13,
    NEONREG_D14 = NEONREG_Q14,
    NEONREG_D15 = NEONREG_Q15,
    NEONREG_D16 = NEONREG_Q16,
    NEONREG_D17 = NEONREG_Q17,
    NEONREG_D18 = NEONREG_Q18,
    NEONREG_D19 = NEONREG_Q19,
    NEONREG_D20 = NEONREG_Q20,
    NEONREG_D21 = NEONREG_Q21,
    NEONREG_D22 = NEONREG_Q22,
    NEONREG_D23 = NEONREG_Q23,
    NEONREG_D24 = NEONREG_Q24,
    NEONREG_D25 = NEONREG_Q25,
    NEONREG_D26 = NEONREG_Q26,
    NEONREG_D27 = NEONREG_Q27,
    NEONREG_D28 = NEONREG_Q28,
    NEONREG_D29 = NEONREG_Q29,
    NEONREG_D30 = NEONREG_Q30,
    NEONREG_D31 = NEONREG_Q31,

    NEONREG_INVALID = 99,
};

#define LAST_FLOAT_REG_ENCODE NEONREG_D31

//
// Condition code types.
//

enum
{
    COND_EQ = 0,  // Equal:             Z == 1
    COND_NE = 1,  // Not equal:         Z == 0
    COND_CS = 2,  // Carry set:         C == 1
    COND_CC = 3,  // Carry clear:       C == 0
    COND_MI = 4,  // Negative:          N == 1
    COND_PL = 5,  // Positive:          N == 0
    COND_VS = 6,  // Overflow:          V == 1
    COND_VC = 7,  // No overflow:       V == 0
    COND_HI = 8,  // Unsigned higher:   C == 1 and Z == 0
    COND_LS = 9,  // Unsigned lower:    C == 0 or Z == 1
    COND_GE = 10, // Signed GE:         N == V
    COND_LT = 11, // Signed LT:         N != V
    COND_GT = 12, // Signed GT:         Z == 0 and N == V
    COND_LE = 13, // Signed LE:         Z == 1 or N != V
    COND_AL = 14  // Always
};

//
// BRK types.
//

#define BrkOpcode(Code)  (0xd4200000 | ((Code) << 5))

enum
{
    ARM64_BREAK_DEBUG_BASE = 0xf000,
    ARM64_BREAKPOINT = ARM64_BREAK_DEBUG_BASE + 0,
    ARM64_ASSERT = ARM64_BREAK_DEBUG_BASE + 1,
    ARM64_DEBUG_SERVICE = ARM64_BREAK_DEBUG_BASE + 2,
    ARM64_FASTFAIL = ARM64_BREAK_DEBUG_BASE + 3,
    ARM64_DIVIDE_BY_0 = ARM64_BREAK_DEBUG_BASE + 4
};

//
// Special opcodes.
//

static const ULONG ARM64_OPCODE_NOP = 0xd503201f;
static const ULONG ARM64_OPCODE_DMB_ISH = 0xd5033bbf;
static const ULONG ARM64_OPCODE_DMB_ISHST = 0xd5033abf;
static const ULONG ARM64_OPCODE_DMB_ISHLD = 0xd50339bf;

//
// Conditional compare flags for CCMP and CCMN instructions.
//

enum
{
    CCMP_N = 0x8,
    CCMP_Z = 0x4,
    CCMP_C = 0x2,
    CCMP_V = 0x1
};

//
// Classes of branch encodings.
//

enum BRANCH_CLASS 
{
    CLASS_INVALID,
    CLASS_IMM26,    // B, inserted at bit 0
    CLASS_IMM19,    // B.cond/CBZ/CBNZ, inserted at bit 5
    CLASS_IMM14,    // TBZ/TBNZ, inserted at bit 5
};

//
// Shift types used by register parameters
//

enum SHIFT_EXTEND_TYPE 
{
    SHIFT_LSL = 0,  // OK for AND/BIC/EON/EOR/ORN/ORR/TST/ADD/CMN/CMP/SUB
    SHIFT_LSR = 1,  // OK for AND/BIC/EON/EOR/ORN/ORR/TST/ADD/CMN/CMP/SUB
    SHIFT_ASR = 2,  // OK for AND/BIC/EON/EOR/ORN/ORR/TST/ADD/CMN/CMP/SUB
    SHIFT_ROR = 3,  // OK for AND/BIC/EON/EOR/ORN/ORR/TST
    SHIFT_NONE = 4,

    EXTEND_UXTB = 8,
    EXTEND_UXTH = 9,
    EXTEND_UXTW = 10,
    EXTEND_UXTX = 11,
    EXTEND_SXTB = 12,
    EXTEND_SXTH = 13,
    EXTEND_SXTW = 14,
    EXTEND_SXTX = 15,
};

//
// Bit shift for scale of indir access
//

enum INDEX_SCALE
{
    INDEX_SCALE_1 = 0,
    INDEX_SCALE_2 = 1,
    INDEX_SCALE_4 = 2,
    INDEX_SCALE_8 = 3,
};

static const BYTE RegEncode[] =
{
#define REGDAT(Name, Listing, Encoding, ...) Encoding,
#include "RegList.h"
#undef REGDAT
};

//
// Basic code emitter class
//

class Arm64CodeEmitter
{
public:
    Arm64CodeEmitter(
        PULONG BasePtr,
        ULONG BufferSizeInBytes
        ) 
        : m_BasePtr(BasePtr),
          m_Offset(0),
          m_MaxOffset(BufferSizeInBytes / 4)
        {
        }
    
    int
    EmitFourBytes(
        ULONG Data
        )
    {
        Assert(m_Offset < m_MaxOffset);
        m_BasePtr[m_Offset++] = Data;
        return 4;
    }

    ULONG
    GetEmittedByteCount() const
    {
        return m_Offset * 4;
    }

    PULONG
    GetEmitAreaBase() const
    {
        return m_BasePtr;
    }

private:
    PULONG m_BasePtr;
    ULONG m_Offset;
    ULONG m_MaxOffset;
};

//
// Derived emitter class that outputs to a local buffer
//

template<int MaxOpcodes = 4>
class Arm64LocalCodeEmitter : public Arm64CodeEmitter
{
public:
    Arm64LocalCodeEmitter() :
        Arm64CodeEmitter(&m_Buffer[0], MaxOpcodes * 4)
        {
        }
    
    DWORD
    Opcode(
        int Index = 0
        )
    {
        return m_Buffer[Index];
    }

private:
    ULONG m_Buffer[MaxOpcodes];
};

//
// Wrapper for an ARM64 register parameter, complete with shift type and amount
//

class Arm64RegisterParam
{
    static const UCHAR REGISTER_SHIFT = 0;
    static const UCHAR REGISTER_MASK = 0xff;
    static const ULONG REGISTER_MASK_SHIFTED = REGISTER_MASK << REGISTER_SHIFT;

    static const UCHAR SHIFT_TYPE_SHIFT = 8;
    static const UCHAR SHIFT_TYPE_MASK = 0x0f;
    static const ULONG SHIFT_TYPE_MASK_SHIFTED = SHIFT_TYPE_MASK << SHIFT_TYPE_SHIFT;
    static const ULONG SHIFT_NONE_SHIFTED = SHIFT_NONE << SHIFT_TYPE_SHIFT;

    static const UCHAR SHIFT_COUNT_SHIFT = 12;
    static const UCHAR SHIFT_COUNT_MASK = 0x3f;
    static const ULONG SHIFT_COUNT_MASK_SHIFTED = SHIFT_COUNT_MASK << SHIFT_COUNT_SHIFT;

protected:

    //
    // N.B. Derived class must initialize m_Encoded.
    //

    Arm64RegisterParam() { }

public:
    Arm64RegisterParam(
        ARM64_REGISTER Reg,
        SHIFT_EXTEND_TYPE Type = SHIFT_NONE,
        UCHAR Amount = 0
        )
        : m_Encoded(((Reg & REGISTER_MASK) << REGISTER_SHIFT) |
                    ((Type & SHIFT_TYPE_MASK) << SHIFT_TYPE_SHIFT) |
                    ((Amount & SHIFT_COUNT_MASK) << SHIFT_COUNT_SHIFT))
    {
        AssertValidRegister(Reg);
        AssertValidShift(Type, Amount);
    }

    ARM64_REGISTER
    Register() const
    {
        return (m_Encoded >> REGISTER_SHIFT) & REGISTER_MASK;
    }

    UCHAR
    RawRegister() const
    {
        return UCHAR(Register() - ARMREG_FIRST);
    }

    SHIFT_EXTEND_TYPE
    ShiftType() const
    {
        return SHIFT_EXTEND_TYPE((m_Encoded >> SHIFT_TYPE_SHIFT) & SHIFT_TYPE_MASK);
    }

    UCHAR
    ShiftCount() const
    {
        return (m_Encoded >> SHIFT_COUNT_SHIFT) & SHIFT_COUNT_MASK;
    }

    bool
    IsRegOnly() const
    {
        return ((m_Encoded & SHIFT_TYPE_MASK_SHIFTED) == SHIFT_NONE_SHIFTED);
    }

    bool
    IsExtended() const
    {
        return (ShiftType() >= EXTEND_UXTB);
    }

    ULONG
    Encode() const
    {
        Assert(!IsExtended());
        return (((ShiftType() & 3) << 22) |
                 (RawRegister() << 16) |
                 (ShiftCount() << 10));
    }

    ULONG
    EncodeExtended() const
    {
        Assert(IsExtended());
        return (((RawRegister() << 16) |
                 (ShiftType() & 7) << 13) |
                 (ShiftCount() << 10));
    }

protected:
    static
    void
    AssertValidRegister(
        ARM64_REGISTER Reg
        )
    {
        UNREFERENCED_PARAMETER(Reg);
        Assert(Reg >= ARMREG_FIRST && Reg <= ARMREG_LAST);
    }

    static
    void
    AssertValidShift(
        SHIFT_EXTEND_TYPE Type,
        UCHAR Amount
        )
    {
        UNREFERENCED_PARAMETER(Type);
        UNREFERENCED_PARAMETER(Amount);
        Assert(Type != SHIFT_NONE || Amount == 0);
        Assert(Type != SHIFT_LSL || (Amount >= 0 && Amount <= 63));
        Assert(Type != SHIFT_LSR || (Amount >= 0 && Amount <= 63));
        Assert(Type != SHIFT_ASR || (Amount >= 0 && Amount <= 63));
        Assert(Type != SHIFT_ROR || (Amount >= 0 && Amount <= 63));
        Assert(Type < EXTEND_UXTB || (Amount >= 0 && Amount <= 4));
    }

    ULONG m_Encoded;
};

//
// Wrapper for an ARM register parameter that can have no shift
//

class Arm64SimpleRegisterParam : public Arm64RegisterParam
{
public:
    Arm64SimpleRegisterParam(
        ARM64_REGISTER Reg
        )
        : Arm64RegisterParam(Reg)
    {
    }

    Arm64SimpleRegisterParam(
        const Arm64RegisterParam &Src
        )
        : Arm64RegisterParam(Src.Register())
    {
        Assert(Src.IsRegOnly());
    }

    bool
    IsRegOnly() const
    {
        return true;
    }

private:
    ULONG
    Encode() const
    {
        return 0;
    }
};

//
// Branch linker helper class
//

class ArmBranchLinker
{
    static const ULONG EMITTER_INVALID_OFFSET = 0x80000000;

public:
    ArmBranchLinker(
        ULONG OffsetFromEmitterBase = EMITTER_INVALID_OFFSET
        ) 
        : m_InstructionOffset(EMITTER_INVALID_OFFSET), 
          m_TargetOffset(OffsetFromEmitterBase), 
          m_BranchClass(CLASS_INVALID),
          m_Resolved(false)
    {
    }

    ArmBranchLinker(
        Arm64CodeEmitter &Emitter
        ) 
        : m_InstructionOffset(EMITTER_INVALID_OFFSET), 
          m_TargetOffset(Emitter.GetEmittedByteCount()), 
          m_BranchClass(CLASS_INVALID),
          m_Resolved(false)
    {
    }

    ~ArmBranchLinker()
    {
        if (HasInstruction() && HasTarget()) {
            Assert(m_Resolved);
        }
    }
    
    bool
    HasInstruction() const
    {
        return (m_InstructionOffset != EMITTER_INVALID_OFFSET);
    }
    
    bool
    HasTarget() const
    {
        return (m_TargetOffset != EMITTER_INVALID_OFFSET);
    }
    
    void
    Reset()
    {
        m_InstructionOffset = EMITTER_INVALID_OFFSET;
        m_TargetOffset = EMITTER_INVALID_OFFSET;
        m_BranchClass = CLASS_INVALID;
    }
    
    void
    SetInstructionOffset(
        ULONG InstructionOffset
        )
    {
        Assert(InstructionOffset % 4 == 0);
        m_InstructionOffset = InstructionOffset / 4;
    }

    void
    SetClass(
        BRANCH_CLASS Class
        )
    {
        m_BranchClass = Class;
    }

    void
    SetInstructionAddressAndClass(
        Arm64CodeEmitter &Emitter,
        BRANCH_CLASS Class
        )
    {
        SetInstructionOffset(Emitter.GetEmittedByteCount());
        SetClass(Class);
    }
    
    void
    SetTarget(
        INT32 OffsetFromEmitterBase
        )
    {
        Assert(OffsetFromEmitterBase % 4 == 0);
        m_TargetOffset = OffsetFromEmitterBase / 4;
    }
    
    void 
    SetTarget(
        Arm64CodeEmitter &Emitter
        )
    {
        SetTarget(Emitter.GetEmittedByteCount());
        Resolve(Emitter);
    }

    void 
    SetTargetAbsolute(
        Arm64CodeEmitter &Emitter,
        PULONG Target
        )
    {
        ULONG_PTR Delta = ULONG_PTR(Target) - ULONG_PTR(Emitter.GetEmitAreaBase());
        Assert(INT32(Delta) == Delta);
        SetTarget(INT32(Delta));
        Resolve(Emitter);
    }

    void
    ResetTarget()
    {
        m_TargetOffset = EMITTER_INVALID_OFFSET;
    }
    
    void
    Resolve(
        PULONG EmitterBlockBase
        )
    {
        if (!HasInstruction() || !HasTarget()) {
            return;
        }
    
        PULONG InstructionAddress = EmitterBlockBase + m_InstructionOffset;
        *InstructionAddress = UpdateInstruction(*InstructionAddress);
        m_Resolved = true;
    }
    
    void
    Resolve(
        Arm64CodeEmitter &Emitter
        )
    {
        Resolve(Emitter.GetEmitAreaBase());
    }
    
    static void
    LinkRaw(
        PULONG ExistingInstruction,
        PULONG TargetInstruction
        )
    {
        ULONG Instruction = *ExistingInstruction;

        //
        // Determine instruction class from bits:
        //
        //    B      000101oo.oooooooo.oooooooo.oooooooo = IMM26
        //    BL     100101oo.oooooooo.oooooooo.oooooooo = IMM26
        //    B.cond 01010100.oooooooo.oooooooo.ooo0cccc = IMM19
        //    CBZ/NZ x011010x.oooooooo.oooooooo.ooorrrrr = IMM19
        //    TBZ/NZ b011011x.bbbbbooo.oooooooo.ooorrrrr = IMM14
        //

        BRANCH_CLASS Class;
        if ((Instruction & 0x7c000000) == 0x14000000) {
            Class = CLASS_IMM26;
        } else if ((Instruction & 0xff000010) == 0x54000000 || 
                   (Instruction & 0x7e000000) == 0x34000000) {
            Class = CLASS_IMM19;
        } else if ((Instruction & 0x7e000000) == 0x36000000) {
            Class = CLASS_IMM14;
        } else {
            Assert(FALSE);
            return;
        }
        
        Arm64CodeEmitter Emitter(ExistingInstruction, 4);
        ArmBranchLinker Linker;
        Linker.SetInstructionAddressAndClass(Emitter, Class);
        Linker.SetTarget(INT32(4 * (TargetInstruction - ExistingInstruction)));
        Linker.Resolve(Emitter);
    }
    
private:
    ULONG
    UpdateInstruction(
        ULONG Instruction
        )
    {
        INT32 Delta = INT32(m_TargetOffset - m_InstructionOffset);
        switch (m_BranchClass) {
        
        case CLASS_IMM26:
            Assert(((INT32(Delta << (32-26))) >> (32-26)) == Delta);
            Assert((Instruction & 0x03ffffff) == 0);
            Assert((Instruction & 0x7c000000) == 0x14000000);
            Instruction = (Instruction & 0xfc000000) | (Delta & 0x03ffffff);
            break;
        
        case CLASS_IMM19:
            Assert(((INT32(Delta << (32-19))) >> (32-19)) == Delta);
            Assert((Instruction & 0x00ffffe0) == 0);
            Assert((Instruction & 0xff000000) == 0x54000000 || (Instruction & 0x7e000000) == 0x34000000);
            Instruction = (Instruction & 0xff00001f) | ((Delta << 5) & 0x00ffffe0);
            break;
        
        case CLASS_IMM14:
            Assert(((INT32(Delta << (32-14))) >> (32-14)) == Delta);
            Assert((Instruction & 0x0007ffe0) == 0);
            Assert((Instruction & 0x7e000000) == 0x36000000);
            Instruction = (Instruction & 0xfff8001f) | ((Delta << 5) & 0x0007ffe0);
            break;
        }
        return Instruction;
    }

    ULONG m_InstructionOffset;
    INT32 m_TargetOffset;
    BRANCH_CLASS m_BranchClass;
    bool m_Resolved;
};

//
// NOP
//

inline
int
EmitNop(
    Arm64CodeEmitter &Emitter
    )
{
    return Emitter.EmitFourBytes(ARM64_OPCODE_NOP);
}

//
// DMB ISH
//

inline
int
EmitDmb(
    Arm64CodeEmitter &Emitter
    )
{
    return Emitter.EmitFourBytes(ARM64_OPCODE_DMB_ISH);
}

//
// DMB ISHST
//

inline
int
EmitDmbSt(
    Arm64CodeEmitter &Emitter
    )

{

    return Emitter.EmitFourBytes(ARM64_OPCODE_DMB_ISHST);
}

//
// DMB ISHLD
//

inline
int
EmitDmbLd(
    Arm64CodeEmitter &Emitter
    )

{

    return Emitter.EmitFourBytes(ARM64_OPCODE_DMB_ISHLD);
}

//
// BRK #Code
//

inline
int
EmitBrk(
    Arm64CodeEmitter &Emitter,
    USHORT Code
    )
{
    return Emitter.EmitFourBytes(BrkOpcode(Code));
}

inline
int
EmitDebugBreak(
    Arm64CodeEmitter &Emitter
    )
{
    return EmitBrk(Emitter, ARM64_BREAKPOINT);
}

inline
int
EmitFastFail(
    Arm64CodeEmitter &Emitter
    )
{
    return EmitBrk(Emitter, ARM64_FASTFAIL);
}

inline
int
EmitDiv0Exception(
    Arm64CodeEmitter &Emitter
    )
{
    return EmitBrk(Emitter, ARM64_DIVIDE_BY_0);
}

//
// MRS dest, systemreg
//

#define ARM64_NZCV              ARM64_SYSREG(3,3, 4, 2,0)   // Flags (EL0); arm64_x.h
#define ARM64_CNTVCT            ARM64_SYSREG(3,3,14, 0,2)   // Generic Timer virtual count

inline
int
EmitMrs(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG SystemReg 
    )
{

    Assert(SystemReg < (1 << 15));

    return Emitter.EmitFourBytes(0xd5300000 | (SystemReg << 5) | Dest.RawRegister());
}


//
// MSR systemreg, source
//

inline
int
EmitMsr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    ULONG SystemReg
    )
{

    Assert(SystemReg < (1 << 15));

    return Emitter.EmitFourBytes(0xd5100000 | (SystemReg << 5) | Source.RawRegister());
}

//
// B target
//

inline
ULONG
BranchOpcode(
    LONG Offset
    )

{

    Assert((Offset >= -(1 << 25)) && (Offset < (1 << 25)));
    Assert((Offset & 0x3) == 0);

    LONG OffsetInWords = (Offset / 4);

    return (0x14000000 | (OffsetInWords & ((1 << 26) - 1)));
}

//
// B target
// B.cond target
// CBZ source, target
// CBNZ source, target
// TBZ source, bit, target
// TBNZ source, bit, target
//

inline
int
EmitBranchCommon(
    Arm64CodeEmitter &Emitter,
    ArmBranchLinker &Linker,
    BRANCH_CLASS Class,
    UINT32 Opcode
    )
{
    Linker.SetInstructionAddressAndClass(Emitter, Class);
    int Result = Emitter.EmitFourBytes(Opcode);
    Linker.Resolve(Emitter);
    return Result;
}

inline
int
EmitBranch(
    Arm64CodeEmitter &Emitter,
    ArmBranchLinker &Linker,
    ULONG Condition = COND_AL
    )
{
    if (Condition == COND_AL) {
        return EmitBranchCommon(Emitter, Linker, CLASS_IMM26, 0x14000000);
    } else {
        return EmitBranchCommon(Emitter, Linker, CLASS_IMM19, 0x54000000 | (Condition & 15));
    }
}

inline
int
EmitBl(
    Arm64CodeEmitter &Emitter,
    ArmBranchLinker &Linker
    )
{
    return EmitBranchCommon(Emitter, Linker, CLASS_IMM26, 0x94000000);
}

inline
int
EmitCbz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ArmBranchLinker &Linker
    )
{
    return EmitBranchCommon(Emitter, Linker, CLASS_IMM19, 0x34000000 | Reg.RawRegister());
}

inline
int
EmitCbz64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ArmBranchLinker &Linker
    )
{
    return EmitBranchCommon(Emitter, Linker, CLASS_IMM19, 0xb4000000 | Reg.RawRegister());
}

inline
int
EmitCbnz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ArmBranchLinker &Linker
    )
{
    return EmitBranchCommon(Emitter, Linker, CLASS_IMM19, 0x35000000 | Reg.RawRegister());
}

inline
int
EmitCbnz64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ArmBranchLinker &Linker
    )
{
    return EmitBranchCommon(Emitter, Linker, CLASS_IMM19, 0xb5000000 | Reg.RawRegister());
}

inline
int
EmitTbz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ULONG Bit,
    ArmBranchLinker &Linker
    )
{
    //Assert(Bit >= 0 && Bit <= 63);

    return EmitBranchCommon(Emitter, Linker, CLASS_IMM14,
            0x36000000 | ((Bit >> 5) << 31) | ((Bit & 0x1f) << 19) | Reg.RawRegister());
}

inline
int
EmitTbnz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg,
    ULONG Bit,
    ArmBranchLinker &Linker
    )
{
    //Assert(Bit >= 0 && Bit <= 63);

    return EmitBranchCommon(Emitter, Linker, CLASS_IMM14,
            0x37000000 | ((Bit >> 5) << 31) | ((Bit & 0x1f) << 19) | Reg.RawRegister());
}

inline
int
EmitBr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg
    )
{
    return Emitter.EmitFourBytes(0xd61f0000 | (Reg.RawRegister() << 5));
}

inline
int
EmitBlr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg
    )
{
    return Emitter.EmitFourBytes(0xd63f0000 | (Reg.RawRegister() << 5));
}

inline
int
EmitRet(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Reg
    )
{
    return Emitter.EmitFourBytes(0xd65f0000 | (Reg.RawRegister() << 5));
}

//
// ADD dest, source1, source2 [with optional shift/extension]
// ADDS dest, source1, source2 [with optional shift/extension]
// SUB dest, source1, source2 [with optional shift/extension]
// SUBS dest, source1, source2 [with optional shift/extension]
// CMP source1, source2 [with optional shift/extension]
//

inline
int
EmitAddSubRegisterCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2,
    UINT32 Opcode,
    UINT32 ExtendedOpcode
    )
{
    Assert(Src2.ShiftType() != SHIFT_ROR);

    //
    // ADD/SUB (shifted register)
    //

    if (Src2.IsExtended()) {
        Assert(ExtendedOpcode != 0);
        return Emitter.EmitFourBytes(ExtendedOpcode | Src2.EncodeExtended() | (Src1.RawRegister() << 5) | Dest.RawRegister());
    } else {
        return Emitter.EmitFourBytes(Opcode | Src2.Encode() | (Src1.RawRegister() << 5) | Dest.RawRegister());
    }
}

inline
int
EmitAddRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0x0b000000, 0x0b200000);
}

inline
int
EmitAddRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0x8b000000, 0x8b200000);
}

inline
int
EmitAddsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0x2b000000, 0x2b200000);
}

inline
int
EmitAddsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0xab000000, 0x8b200000);
}

inline
int
EmitSubRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0x4b000000, 0x4b200000);
}

inline
int
EmitSubRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0xcb000000, 0xcb200000);
}

inline
int
EmitSubsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0x6b000000, 0x6b200000);
}

inline
int
EmitSubsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAddSubRegisterCommon(Emitter, Dest, Src1, Src2, 0xeb000000, 0xeb200000);
}

inline
int
EmitCmpRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitSubsRegister(Emitter, ARMREG_ZR, Src1, Src2);
}

inline
int
EmitCmpRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitSubsRegister64(Emitter, ARMREG_ZR, Src1, Src2);
}

//
// ADC dest, source1, source2
// ADCS dest, source1, source2
// SBC dest, source1, source2
// SBCS dest, source1, source2
//

inline
int
EmitAdcSbcRegisterCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    UINT32 Opcode
    )

{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitAdcRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0x1a000000);
}

inline
int
EmitAdcRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0x9a000000);
}

inline
int
EmitAdcsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0x3a000000);
}

inline
int
EmitAdcsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0xba000000);
}

inline
int
EmitSbcRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0x5a000000);
}

inline
int
EmitSbcRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0xda000000);
}

inline
int
EmitSbcsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0x7a000000);
}

inline
int
EmitSbcsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitAdcSbcRegisterCommon(Emitter, Dest, Src1, Src2, 0xfa000000);
}

//
// MADD dest, source1, source2, source3
// MSUB dest, source1, source2, source3
// SMADDL dest, source1, source2, source3
// UMADDL dest, source1, source2, source3
// SMSUBL dest, source1, source2, source3
// UMSUBL dest, source1, source2, source3
// MUL dest, source1, source2
// SMULL dest, source1, source2
// UMULL dest, source1, source2
//

inline
int
EmitMaddMsubCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Src3.RawRegister() << 10) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitMadd(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x1b000000);
}

inline
int
EmitMadd64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9b000000);
}

inline
int
EmitMsub(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x1b008000);
}

inline
int
EmitMsub64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9b008000);
}

inline
int
EmitSmaddl(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9b200000);
}

inline
int
EmitUmaddl(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9ba00000);
}

inline
int
EmitSmsubl(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9b208000);
}

inline
int
EmitUmsubl(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    Arm64SimpleRegisterParam Src3
    )
{
    return EmitMaddMsubCommon(Emitter, Dest, Src1, Src2, Src3, 0x9ba08000);
}

inline
int
EmitMul(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitMadd(Emitter, Dest, Src1, Src2, ARMREG_ZR);
}

inline
int
EmitMul64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitMadd64(Emitter, Dest, Src1, Src2, ARMREG_ZR);
}

inline
int
EmitSmull(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitSmaddl(Emitter, Dest, Src1, Src2, ARMREG_ZR);
}

inline
int
EmitUmull(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitUmaddl(Emitter, Dest, Src1, Src2, ARMREG_ZR);
}

//
// SDIV dest, source1, source2
// UDIV dest, source1, source2
//

inline
int
EmitDivideCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitSdiv(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitDivideCommon(Emitter, Dest, Src1, Src2, 0x1ac00c00);
}

inline
int
EmitSdiv64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitDivideCommon(Emitter, Dest, Src1, Src2, 0x9ac00c00);
}

inline
int
EmitUdiv(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitDivideCommon(Emitter, Dest, Src1, Src2, 0x1ac00800);
}

inline
int
EmitUdiv64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitDivideCommon(Emitter, Dest, Src1, Src2, 0x9ac00800);
}

//
// AND dest, source1, source2 [with optional shift]
// ANDS dest, source1, source2 [with optional shift]
// BIC dest, source1, source2 [with optional shift]
// BICS dest, source1, source2 [with optional shift]
// EON dest, source1, source2 [with optional shift]
// EOR dest, source1, source2 [with optional shift]
// ORR dest, source1, source2 [with optional shift]
// ORN dest, source1, source2 [with optional shift]
// TST source1, source2 [with optional shift]
// MOV dest, source
//

inline
int
EmitLogicalRegisterCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | Src2.Encode() | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitAndRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x0a000000);
}

inline
int
EmitAndRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x8a000000);
}

inline
int
EmitAndsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x6a000000);
}

inline
int
EmitAndsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xea000000);
}

inline
int
EmitBicRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x0a200000);
}

inline
int
EmitBicRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x8a200000);
}

inline
int
EmitBicsRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x6a200000);
}

inline
int
EmitBicsRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xea200000);
}

inline
int
EmitEonRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x4a200000);
}

inline
int
EmitEonRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xca200000);
}

inline
int
EmitEorRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x4a000000);
}

inline
int
EmitEorRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xca000000);
}

inline
int
EmitOrnRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x2a200000);
}

inline
int
EmitOrnRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xaa200000);
}

inline
int
EmitOrrRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0x2a000000);
}

inline
int
EmitOrrRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitLogicalRegisterCommon(Emitter, Dest, Src1, Src2, 0xaa000000);
}

inline
int
EmitTestRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAndsRegister(Emitter, ARMREG_ZR, Src1, Src2);
}

inline
int
EmitTestRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64RegisterParam Src2
    )
{
    return EmitAndsRegister64(Emitter, ARMREG_ZR, Src1, Src2);
}

inline
int
EmitMovRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
)
{
    return EmitOrrRegister(Emitter, Dest, ARMREG_ZR, Src);
}

inline
int
EmitMovRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
)
{
    return EmitOrrRegister64(Emitter, Dest, ARMREG_ZR, Src);
}

inline
int
EmitMvnRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
)
{
    return EmitOrnRegister(Emitter, Dest, ARMREG_ZR, Src);
}

inline
int
EmitMvnRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
)
{
    return EmitOrnRegister64(Emitter, Dest, ARMREG_ZR, Src);
}

//
// ASR dest, source1, source2
// LSL dest, source1, source2
// LSR dest, source1, source2
// ROR dest, source1, source2
//

inline
int
EmitShiftRegisterCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    UINT32 Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitAsrRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x1ac02800);
}

inline
int
EmitAsrRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x9ac02800);
}

inline
int
EmitLslRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x1ac02000);
}

inline
int
EmitLslRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x9ac02000);
}

inline
int
EmitLsrRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x1ac02400);
}

inline
int
EmitLsrRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x9ac02400);
}

inline
int
EmitRorRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x1ac02c00);
}

inline
int
EmitRorRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2
    )
{
    return EmitShiftRegisterCommon(Emitter, Dest, Src1, Src2, 0x9ac02c00);
}

//
// BFM dest, source, immr, imms
// UBFM dest, source, immr, imms
// SBFM dest, source, immr, imms
// SXTB dest, source
// UXTB dest, source
// SXTH dest, source
// UXTH dest, source
// BFI dest, source, lsb, count
// BFXIL dest, source, lsb, count
// UBFX dest, source, lsb, count
// SBFX dest, source, lsb, count
//

inline
int
EmitBitfieldCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (ULONG(Immr) << 16) | (ULONG(Imms) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitBfm(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0x33000000);
}

inline
int
EmitBfm64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0xb3400000);
}

inline
int
EmitSbfm(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0x13000000);
}

inline
int
EmitSbfm64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0x93400000);
}

inline
int
EmitUbfm(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0x53000000);
}

inline
int
EmitUbfm64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immr,
    ULONG64 Imms
    )
{
    return EmitBitfieldCommon(Emitter, Dest, Src, Immr, Imms, 0xd3400000);
}

//
// SXTB dest, source
// SXTH dest, source
// UXTB dest, source
// UXTH dest, source
//

inline
int
EmitSxtb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitSbfm(Emitter, Dest, Src, 0, 7);
}

inline
int
EmitSxtb64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitSbfm64(Emitter, Dest, Src, 0, 7);
}

inline
int
EmitSxth(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitSbfm(Emitter, Dest, Src, 0, 15);
}

inline
int
EmitSxth64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitSbfm64(Emitter, Dest, Src, 0, 15);
}

inline
int
EmitUxtb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitUbfm(Emitter, Dest, Src, 0, 7);
}

inline
int
EmitUxtb64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitUbfm64(Emitter, Dest, Src, 0, 7);
}

inline
int
EmitUxth(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitUbfm(Emitter, Dest, Src, 0, 15);
}

inline
int
EmitUxth64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitUbfm64(Emitter, Dest, Src, 0, 15);
}

inline
int
EmitBfi(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 31);
    Assert(Width >= 1 && Width <= 32);
    Assert(Lsb + Width <= 32);

    return EmitBfm(Emitter, Dest, Src, (32 - Lsb) % 32, Width - 1);
}

inline
int
EmitBfi64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 63);
    Assert(Width >= 1 && Width <= 64);
    Assert(Lsb + Width <= 64);

    return EmitBfm64(Emitter, Dest, Src, (64 - Lsb) % 64, Width - 1);
}

inline
int
EmitBfxil(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 31);
    Assert(Width >= 1 && Width <= 32);
    Assert(Lsb + Width <= 32);

    return EmitBfm(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

inline
int
EmitBfxil64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 63);
    Assert(Width >= 1 && Width <= 64);
    Assert(Lsb + Width <= 64);

    return EmitBfm64(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

inline
int
EmitSbfx(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 31);
    Assert(Width >= 1 && Width <= 32);
    Assert(Lsb + Width <= 32);

    return EmitSbfm(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

inline
int
EmitSbfx64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 63);
    Assert(Width >= 1 && Width <= 64);
    Assert(Lsb + Width <= 64);

    return EmitSbfm64(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

inline
int
EmitUbfx(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 31);
    Assert(Width >= 1 && Width <= 32);
    Assert(Lsb + Width <= 32);

    return EmitUbfm(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

inline
int
EmitUbfx64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Lsb,
    ULONG Width
    )
{
    Assert(Lsb <= 63);
    Assert(Width >= 1 && Width <= 64);
    Assert(Lsb + Width <= 64);

    return EmitUbfm64(Emitter, Dest, Src, Lsb, Lsb + Width - 1);
}

//
// ASR dest, source, immediate
// LSL dest, source, immediate
// LSR dest, source, immediate
//

inline
int
EmitAsrImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 32);
    return EmitSbfm(Emitter, Dest, Src, Immediate, 31);
}

inline
int
EmitAsrImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 64);
    return EmitSbfm64(Emitter, Dest, Src, Immediate, 63);
}

inline
int
EmitLslImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 32);
    return EmitUbfm(Emitter, Dest, Src, 32 - Immediate, 31 - Immediate);
}

inline
int
EmitLslImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 64);
    return EmitUbfm64(Emitter, Dest, Src, 64 - Immediate, 63 - Immediate);
}

inline
int
EmitLsrImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 32);
    return EmitUbfm(Emitter, Dest, Src, Immediate, 31);
}

inline
int
EmitLsrImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    Assert(Immediate < 64);
    return EmitUbfm64(Emitter, Dest, Src, Immediate, 63);
}

//
// EXTR dest, source1, source2, shift
// ROR dest, source, immediate
//

inline
int
EmitExtr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Shift
    )
{
    return Emitter.EmitFourBytes(0x13800000 | (Src2.RawRegister() << 16) | ((Shift & 0x3f) << 10) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitExtr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Shift
    )
{
    return Emitter.EmitFourBytes(0x93c00000 | (Src2.RawRegister() << 16) | ((Shift & 0x3f) << 10) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitRorImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    //Assert(Immediate >= 0 && Immediate < 32);
    return EmitExtr(Emitter, Dest, Src, Src, Immediate);
}

inline
int
EmitRorImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    //Assert(Immediate >= 0 && Immediate < 64);
    return EmitExtr64(Emitter, Dest, Src, Src, Immediate);
}

//
// CSEL dest, src1, src2, cond
// CSINC dest, src1, src2, cond
// CSINV dest, src1, src2, cond
// CSNEG dest, src1, src2, cond
// CINC dest, src1, cond
// CSET dest, cond
// CSETM dest, cond
// CNEG dest, src1, cond
//

inline
int
EmitConditionalCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Condition << 12) | (Src1.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitCsel(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x1a800000);
}

inline
int
EmitCsel64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x9a800000);
}

inline
int
EmitCsinc(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x1a800400);
}

inline
int
EmitCsinc64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x9a800400);
}

inline
int
EmitCsinv(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x5a800000);
}

inline
int
EmitCsinv64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0xda800000);
}

inline
int
EmitCsneg(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0x5a800400);
}

inline
int
EmitCsneg64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Condition
    )
{
    return EmitConditionalCommon(Emitter, Dest, Src1, Src2, Condition, 0xda800400);
}

inline
int
EmitCinc(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsinc(Emitter, Dest, Src, Src, Condition ^ 1);
}

inline
int
EmitCinc64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsinc64(Emitter, Dest, Src, Src, Condition ^ 1);
}

inline
int
EmitCset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Condition
    )
{
    return EmitCsinc(Emitter, Dest, ARMREG_ZR, ARMREG_ZR, Condition ^ 1);
}

inline
int
EmitCset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Condition
    )
{
    return EmitCsinc64(Emitter, Dest, ARMREG_ZR, ARMREG_ZR, Condition ^ 1);
}

inline
int
EmitCinv(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsinv(Emitter, Dest, Src, Src, Condition ^ 1);
}

inline
int
EmitCinv64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsinv64(Emitter, Dest, Src, Src, Condition ^ 1);
}

inline
int
EmitCsetm(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Condition
    )
{
    return EmitCsinv(Emitter, Dest, ARMREG_ZR, ARMREG_ZR, Condition ^ 1);
}

inline
int
EmitCsetm64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Condition
    )
{
    return EmitCsinv64(Emitter, Dest, ARMREG_ZR, ARMREG_ZR, Condition ^ 1);
}

inline
int
EmitCneg(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsneg(Emitter, Dest, Src, Src, Condition ^ 1);
}

inline
int
EmitCneg64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Condition
    )
{
    return EmitCsneg64(Emitter, Dest, Src, Src, Condition ^ 1);
}

//
// CCMN src1, src2, nzcv, cond
// CCMP src1, src2, nzcv, cond
//

inline
int
EmitConditionalCompareRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Nzcv,
    ULONG Condition,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src2.RawRegister() << 16) | (Condition << 12) | (Src1.RawRegister() << 5) | Nzcv);
}

inline
int
EmitCcmnRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Nzcv,
    ULONG Condition
    )
{
    return EmitConditionalCompareRegister(Emitter, Src1, Src2, Nzcv, Condition, 0x3A400000);
}

inline
int
EmitCcmnRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Nzcv,
    ULONG Condition
    )
{
    return EmitConditionalCompareRegister(Emitter, Src1, Src2, Nzcv, Condition, 0xBA400000);
}

inline
int
EmitCcmpRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Nzcv,
    ULONG Condition
    )
{
    return EmitConditionalCompareRegister(Emitter, Src1, Src2, Nzcv, Condition, 0x7A400000);
}

inline
int
EmitCcmpRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src1,
    Arm64SimpleRegisterParam Src2,
    ULONG Nzcv,
    ULONG Condition
    )
{
    return EmitConditionalCompareRegister(Emitter, Src1, Src2, Nzcv, Condition, 0xFA400000);
}

//
// Load an immediate value in the most efficient way possible.
//

inline
int
EmitMovImmediateCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift,
    ULONG Opcode
    )
{
    Assert(Shift % 16 == 0);
    Assert(Shift / 16 < 4);
    return Emitter.EmitFourBytes(Opcode | ((Shift / 16) << 21) | ((Immediate & 0xffff) << 5) | Dest.RawRegister());
}

inline
int
EmitMovk(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0x72800000);
}

inline
int
EmitMovk64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0xf2800000);
}

inline
int
EmitMovn(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0x12800000);
}

inline
int
EmitMovn64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0x92800000);
}

inline
int
EmitMovz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0x52800000);
}

inline
int
EmitMovz64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate,
    ULONG Shift
    )
{
    return EmitMovImmediateCommon(Emitter, Dest, Immediate, Shift, 0xd2800000);
}

inline
int
EmitLoadImmediate32(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate
    )
{

    ULONG Word0 = Immediate & 0xffff;
    ULONG Word1 = (Immediate >> 16) & 0xffff;

    if (Word0 != 0 && Word1 != 0) {

        //
        // Try to encode as a bitfield. If possible, use ORR immediate.
        //

        ULONG BitfieldResult = FindArm64LogicalImmediateEncoding(Immediate, 4);
        if (BitfieldResult != ARM64_LOGICAL_IMMEDIATE_NO_ENCODING) {
            return Emitter.EmitFourBytes(0x32000000 | (BitfieldResult << 10) | (31 << 5) | Dest.RawRegister());
        }

        //
        // Use MOVN (+MOVK) if one of the two halves is 0xffff
        //

        if (Word1 == 0xffff) {
            return Emitter.EmitFourBytes(0x12800000 | (0 << 21) | ((Word0 ^ 0xffff) << 5) | Dest.RawRegister());
        } else if (Word0 == 0xffff) {
            return Emitter.EmitFourBytes(0x12800000 | (1 << 21) | ((Word1 ^ 0xffff) << 5) | Dest.RawRegister());
        }
    }

    //
    // Otherwise use MOVZ (+MOVK).
    //

    ULONG Opcode = 0x52800000;
    int Result = 0;
    if (Word0 != 0 || Word1 == 0) {
        Result += Emitter.EmitFourBytes(Opcode | (0 << 21) | (Word0 << 5) | Dest.RawRegister());
        Opcode = 0x72800000;
    }
    if (Word1 != 0) {
        Result += Emitter.EmitFourBytes(Opcode | (1 << 21) | (Word1 << 5) | Dest.RawRegister());
    }
    return Result;
}

inline
int
EmitLoadImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG64 Immediate
    )
{

    //
    // Use a 32-bit form if the upper 32 bits are 0.
    //

    if ((Immediate >> 32) == 0) {
        return EmitLoadImmediate32(Emitter, Dest, ULONG(Immediate));
    }

    //
    // Try to encode as a bitfield. If possible, use ORR immediate.
    //

    ULONG BitfieldResult = FindArm64LogicalImmediateEncoding(Immediate, 8);
    if (BitfieldResult != ARM64_LOGICAL_IMMEDIATE_NO_ENCODING) {
        return Emitter.EmitFourBytes(0xb2000000 | (BitfieldResult << 10) | (31 << 5) | Dest.RawRegister());
    }

    //
    // Break into words and count the number of 0's and ffff's
    //

    ULONG Word0 = Immediate & 0xffff;
    ULONG Word1 = (Immediate >> 16) & 0xffff;
    ULONG Word2 = (Immediate >> 32) & 0xffff;
    ULONG Word3 = (Immediate >> 48) & 0xffff;
    int NumZero = (Word0 == 0) + (Word1 == 0) + (Word2 == 0) + (Word3 == 0);
    int NumOnes = (Word0 == 0xffff) + (Word1 == 0xffff) + (Word2 == 0xffff) + (Word3 == 0xffff);

    //
    // Use MOVZ/MOVN (+MOVK)
    //

    ULONG WordMask = (NumOnes > NumZero) ? 0xffff : 0x0000;
    ULONG WordXor = WordMask;
    ULONG Opcode = (WordMask == 0xffff) ? 0x92800000 : 0xd2800000;
    int Result = 0;
    if (Word0 != WordMask) {
        Result += Emitter.EmitFourBytes(Opcode | (0 << 21) | ((Word0 ^ WordXor) << 5) | Dest.RawRegister());
        Opcode = 0xf2800000;
        WordXor = 0;
    }
    if (Word1 != WordMask) {
        Result += Emitter.EmitFourBytes(Opcode | (1 << 21) | ((Word1 ^ WordXor) << 5) | Dest.RawRegister());
        Opcode = 0xf2800000;
        WordXor = 0;
    }
    if (Word2 != WordMask) {
        Result += Emitter.EmitFourBytes(Opcode | (2 << 21) | ((Word2 ^ WordXor) << 5) | Dest.RawRegister());
        Opcode = 0xf2800000;
        WordXor = 0;
    }
    if (Word3 != WordMask) {
        Result += Emitter.EmitFourBytes(Opcode | (3 << 21) | ((Word3 ^ WordXor) << 5) | Dest.RawRegister());
    }
    return Result;
}

inline
int
EmitLoadImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    ULONG Immediate
    )

{
    return EmitLoadImmediate32(Emitter, Dest, Immediate);
}

//
// ADR dest, offset
// ADRP dest, pageoffs
//

inline
int
EmitAdrAdrp(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    LONG Offset,
    ULONG Opcode
)
{

    Assert(Offset >= -(1 << 21) && Offset < (1 << 21));
    return Emitter.EmitFourBytes(Opcode | ((Offset & 3) << 29) | (((Offset >> 2) & 0x7ffff) << 5) | Dest.RawRegister());
}

inline
int
EmitAdr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    LONG Offset
)
{
    return EmitAdrAdrp(Emitter, Dest, Offset, 0x10000000);
}

inline
int
EmitAdrp(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    LONG PageOffset
)
{
    return EmitAdrAdrp(Emitter, Dest, PageOffset, 0x90000000);
}

//
// ADD dest, source, immediate
// ADDS dest, source, immediate
// SUB dest, source, immediate
// SUBS dest, source, immediate
// CMP dest, immediate
//

inline
int
EmitAddSubImmediateCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    LONG Immediate,
    bool SetFlags,
    bool IsAdd,
    ULONG OpcodeHighBit
    )
{
    //
    // If the immediate is zero, use the requested ADD/SUB to compute the carry bit.
    //

    if (Immediate == 0) {

        if (IsAdd) {
            return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x11000000 | (0 << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
        } else {
            return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x51000000 | (0 << 22) | (Src.RawRegister() << 5) | Dest.RawRegister());
        }
    }

    //
    // If fits within 12 bits, use ADD/SUB directly.
    //

    if ((Immediate & 0xfff) == Immediate) {
        return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x11000000 | (0 << 22) | ((Immediate & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else if ((-Immediate & 0xfff) == -Immediate) {
        return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x51000000 | (0 << 22) | ((-Immediate & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }

    //
    // If fits within upper 12 bits, use ADD/SUB directly.
    //

    if ((Immediate & 0xfff000) == Immediate) {
        return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x11000000 | (1 << 22) | (((Immediate >> 12) & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
    } else if ((-Immediate & 0xfff000) == -Immediate) {
        return Emitter.EmitFourBytes(OpcodeHighBit | (SetFlags << 29) | 0x51000000 | (1 << 22) | (((-Immediate >> 12) & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
    }

    //
    // If fits within upper 24 bits, and flags not needed, do a pair.
    //

    if (!SetFlags) {

        if ((Immediate & 0xffffff) == Immediate) {
            int Result = Emitter.EmitFourBytes(OpcodeHighBit | 0x11000000 | (1 << 22) | (((Immediate >> 12) & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
            return Result + Emitter.EmitFourBytes(OpcodeHighBit | 0x11000000 | (0 << 22) | ((Immediate & 0xfff) << 10) | (Dest.RawRegister() << 5) | Dest.RawRegister());
        } else if ((-Immediate & 0xffffff) == -Immediate) {
            int Result = Emitter.EmitFourBytes(OpcodeHighBit | 0x51000000 | (1 << 22) | (((-Immediate >> 12) & 0xfff) << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
            return Result + Emitter.EmitFourBytes(OpcodeHighBit | 0x51000000 | (0 << 22) | ((-Immediate & 0xfff) << 10) | (Dest.RawRegister() << 5) | Dest.RawRegister());
        }
    }

    //
    // Otherwise fail.
    //
    AssertMsg(false, "EmitAddSubImmediateCommon failed to emit");
    return 0;
}

inline
int
EmitAddImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitAddSubImmediateCommon(Emitter, Dest, Src, Immediate, false, true, 0);
}

inline
int
EmitAddImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    if (LONG(Immediate) == Immediate) {
        return EmitAddSubImmediateCommon(Emitter, Dest, Src, LONG(Immediate), false, true, 0x80000000);
    }

    AssertMsg(false, "EmitAddImmediate64 failed to emit");
    return 0;
}

inline
int
EmitAddsImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitAddSubImmediateCommon(Emitter, Dest, Src, Immediate, true, true, 0);
}

inline
int
EmitAddsImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    if (LONG(Immediate) == Immediate) {
        return EmitAddSubImmediateCommon(Emitter, Dest, Src, LONG(Immediate), true, true, 0x80000000);
    }

    AssertMsg(false, "EmitAddsImmediate64 failed to emit");
    return 0;
}

inline
int
EmitSubImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitAddSubImmediateCommon(Emitter, Dest, Src, -LONG(Immediate), false, false, 0);
}

inline
int
EmitSubImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    if (-LONG(Immediate) == -LONG64(Immediate)) {
        return EmitAddSubImmediateCommon(Emitter, Dest, Src, -LONG(Immediate), false, false, 0x80000000);
    }

    AssertMsg(false, "EmitSubImmediate64 failed to emit");
    return 0;
}

inline
int
EmitSubsImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitAddSubImmediateCommon(Emitter, Dest, Src, -LONG(Immediate), true, false, 0);
}

inline
int
EmitSubsImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    if (-LONG(Immediate) == -LONG64(Immediate)) {
        return EmitAddSubImmediateCommon(Emitter, Dest, Src, -LONG(Immediate), true, false, 0x80000000);
    }

    AssertMsg(false, "EmitSubsImmediate64 failed to emit");
    return 0;
}

inline
int
EmitCmpImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitSubsImmediate(Emitter, ARMREG_ZR, Src, Immediate);
}

inline
int
EmitCmpImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitSubsImmediate64(Emitter, ARMREG_ZR, Src, Immediate);
}


//
// ADC/SBC immediate don't really exist. Returning 0 here forces them
// to go down the load-immediate-into-register path.
//

inline
int EmitAdcImmediate(...) { return 0; }
inline
int EmitAdcImmediate64(...) { return 0; }
inline
int EmitAdcsImmediate(...) { return 0; }
inline
int EmitAdcsImmediate64(...) { return 0; }
inline
int EmitSbcImmediate(...) { return 0; }
inline
int EmitSbcImmediate64(...) { return 0; }
inline
int EmitSbcsImmediate(...) { return 0; }
inline
int EmitSbcsImmediate64(...) { return 0; }

//
// AND dest, source, immediate
// ANDS dest, source, immediate
// ORR dest, source, immediate
// EOR dest, source, immediate
// TST source, immediate
//

inline
int
EmitLogicalImmediateCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate,
    ULONG Opcode,
    ULONG NotRegisterOpcode,
    ULONG Size
    )
{
    ULONG BitfieldResult = FindArm64LogicalImmediateEncoding(Immediate, Size);
    if (BitfieldResult == ARM64_LOGICAL_IMMEDIATE_NO_ENCODING) {

        //
        // Special case: -1 can't be encoded, but the NOT form of the opcode
        // can use ZR to produce the same result.
        //

        if (NotRegisterOpcode != 0 &&
            ((Size == 4 && ULONG(Immediate) == ULONG(-1)) ||
             (Size == 8 && ULONG64(Immediate) == ULONG64(-1)))) {

            return Emitter.EmitFourBytes(NotRegisterOpcode | (31 << 16) | (Src.RawRegister() << 5) | Dest.RawRegister());
        }

        AssertMsg(false, "EmitLogicalImmediateCommon failed to emit");
        return 0;
    }

    return Emitter.EmitFourBytes(Opcode | (BitfieldResult << 10) | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitAndImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0x12000000, 0x0a200000, 4);
}

inline
int
EmitAndImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0x92000000, 0x8a200000, 8);
}

inline
int
EmitAndsImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0x72000000, 0x6a200000, 4);
}

inline
int
EmitAndsImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0xf2000000, 0xea200000, 8);
}

inline
int
EmitOrrImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0x32000000, 0x2a200000, 4);
}

inline
int
EmitOrrImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0xb2000000, 0xaa200000, 8);
}

inline
int
EmitEorImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0x52000000, 0x4a200000, 4);
}

inline
int
EmitEorImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitLogicalImmediateCommon(Emitter, Dest, Src, Immediate, 0xd2000000, 0xca200000, 8);
}

inline
int
EmitTestImmediate(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src,
    ULONG Immediate
    )
{
    return EmitAndsImmediate(Emitter, ARMREG_ZR, Src, Immediate);
}

inline
int
EmitTestImmediate64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Src,
    ULONG64 Immediate
    )
{
    return EmitAndsImmediate64(Emitter, ARMREG_ZR, Src, Immediate);
}

//
// CLZ dest, source
// RBIT dest, source
// REV dest, source
// REV16 dest, source
//

inline
int
EmitReverseCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Src.RawRegister() << 5) | Dest.RawRegister());
}

inline
int
EmitClz(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0x5ac01000);
}

inline
int
EmitClz64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0xdac01000);
}

inline
int
EmitRbit(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0x5ac00000);
}

inline
int
EmitRbit64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0xdac00000);
}

inline
int
EmitRev(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0x5ac00800);
}

inline
int
EmitRev64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )

{
    return EmitReverseCommon(Emitter, Dest, Src, 0xdac00c00);
}

inline
int
EmitRev16(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0x5ac00400);
}

inline
int
EmitRev1664(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0xdac00400);
}

inline
int
EmitRev3264(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Src
    )
{
    return EmitReverseCommon(Emitter, Dest, Src, 0xdac00800);
}

//
// LDRB dest, [addr, reg[, lsl #shift]]
// LDRSB dest, [addr, reg[, lsl #shift]]
// LDRH dest, [addr, reg[, lsl #shift]]
// LDRSH dest, [addr, reg[, lsl #shift]]
// LDR dest, [addr, reg[, lsl #shift]]
// STRB src, [addr, reg[, lsl #shift]]
// STRH src, [addr, reg[, lsl #shift]]
// STR src, [addr, reg[, lsl #shift]]
//

inline
int
EmitLdrStrRegisterCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam SrcDest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index,
    int AccessShift,
    UINT32 Opcode
    )
{

    Assert(Index.IsExtended() || Index.ShiftType() == SHIFT_LSL || Index.ShiftType() == SHIFT_NONE);

    //
    // Choose an extend type, mapping NONE/LSL to UXTX.
    //

    int ExtendType = Index.ShiftType() & 7;
    if (!Index.IsExtended()) {
        ExtendType = EXTEND_UXTX & 7;
    }

    //
    // Determine shift amount or return 0 if it can't be encoded.
    //

    ULONG Amount = 0;
    if (Index.ShiftType() != SHIFT_NONE && Index.ShiftCount() == AccessShift) {
        Amount = 1;
    } else if (Index.ShiftCount() != 0) {
        AssertMsg(false, "EmitLdrStrRegisterCommon failed to emit");
        return 0;
    }

    return Emitter.EmitFourBytes(Opcode | (Index.RawRegister() << 16) | (ExtendType << 13) | (Amount << 12) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
}

inline
int
EmitLdrbRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_1, 0x38600800);
}

inline
int
EmitLdrsbRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_1, 0x38e00800);
}

inline
int
EmitLdrsbRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_1, 0x38a00800);
}

inline
int
EmitLdrhRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_2, 0x78600800);
}

inline
int
EmitLdrshRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_2, 0x78e00800);
}

inline
int
EmitLdrshRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_2, 0x78a00800);
}

inline
int
EmitLdrRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_4, 0xb8600800);
}

inline
int
EmitLdrswRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_4, 0xb8a00800);
}

inline
int
EmitLdrRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Dest, Addr, Index, INDEX_SCALE_8, 0xf8600800);
}

inline
int
EmitStrbRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Source, Addr, Index, INDEX_SCALE_1, 0x38200800);
}

inline
int
EmitStrhRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Source, Addr, Index, INDEX_SCALE_2, 0x78200800);
}

inline
int
EmitStrRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Source, Addr, Index, INDEX_SCALE_4, 0xb8200800);
}

inline
int
EmitStrRegister64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, Source, Addr, Index, INDEX_SCALE_8, 0xf8200800);
}

inline
int
EmitPrfmRegister(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Addr,
    Arm64RegisterParam Index
    )
{
    return EmitLdrStrRegisterCommon(Emitter, ARMREG_R0 /* PLDL1KEEP */, Addr, Index, INDEX_SCALE_4, 0xf8a00800);
}

//
// LDRB dest, [addr, #offset]
// LDRSB dest, [addr, #offset]
// LDRH dest, [addr, #offset]
// LDRSH dest, [addr, #offset]
// LDR dest, [addr, #offset]
// STRB src, [addr, #offset]
// STRH src, [addr, #offset]
// STR src, [addr, #offset]
//

inline
int
EmitLdrStrOffsetCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam SrcDest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset,
    ULONG AccessShift,
    ULONG Opcode,
    ULONG OpcodeUnscaled
    )
{
    if (Opcode != 0) {
        LONG EncodeOffset = Offset >> AccessShift;
        if ((EncodeOffset << AccessShift) == Offset && (EncodeOffset & 0xfff) == EncodeOffset) {
            return Emitter.EmitFourBytes(Opcode | ((EncodeOffset & 0xfff) << 10) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
        }
    }

    if (OpcodeUnscaled != 0 && Offset >= -0x100 && Offset <= 0xff) {
        return Emitter.EmitFourBytes(OpcodeUnscaled | ((Offset & 0x1ff) << 12) | (Addr.RawRegister() << 5) | SrcDest.RawRegister());
    }

    AssertMsg(false, "EmitLdrStrOffsetCommon failed to emit");
    return 0;
}

inline
int
EmitLdrbOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 0, 0x39400000, 0x38400000);
}

inline
int
EmitLdrsbOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 0, 0x39c00000, 0x38c00000);
}

inline
int
EmitLdrsbOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 0, 0x39800000, 0x38800000);
}

inline
int
EmitLdrhOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 1, 0x79400000, 0x78400000);
}

inline
int
EmitLdrhOffsetPostIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    if (Offset == 0) {
        return EmitLdrhOffset(Emitter, Dest, Addr, 0);
    } else {
        return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 1, 0, 0x78400400);
    }
}

inline
int
EmitLdrshOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 1, 0x79c00000, 0x78c00000);
}

inline
int
EmitLdrshOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 1, 0x79800000, 0x78800000);
}

inline
int
EmitLdrOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 2, 0xb9400000, 0xb8400000);
}

inline
int
EmitLdrOffsetPostIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    if (Offset == 0) {
        return EmitLdrOffset(Emitter, Dest, Addr, 0);
    } else {
        return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 2, 0, 0xb8400400);
    }
}

inline
int
EmitLdrswOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 2, 0xb9800000, 0xb8800000);
}

inline
int
EmitLdrOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 3, 0xf9400000, 0xf8400000);
}

inline
int
EmitLdrOffsetPostIndex64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    if (Offset == 0) {
        return EmitLdrOffset(Emitter, Dest, Addr, 0);
    } else {
        return EmitLdrStrOffsetCommon(Emitter, Dest, Addr, Offset, 3, 0, 0xf8400400);
    }
}

inline
int
EmitStrbOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 0, 0x39000000, 0x38000000);
}

inline
int
EmitStrhOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 1, 0x79000000, 0x78000000);
}

inline
int
EmitStrhOffsetPreIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    if (Offset == 0) {
        return EmitStrhOffset(Emitter, Source, Addr, Offset);
    } else {
        return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 1, 0, 0x78000c00);
    }
}

inline
int
EmitStrOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 2, 0xb9000000, 0xb8000000);
}

inline
int
EmitStrOffsetPreIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    if (Offset == 0) {
        return EmitStrOffset(Emitter, Source, Addr, 0);
    } else {
        return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 2, 0, 0xb8000c00);
    }
}

inline
int
EmitStrOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, Source, Addr, Offset, 3, 0xf9000000, 0xf8000000);
}

inline
int
EmitPrfmOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdrStrOffsetCommon(Emitter, ARMREG_R0 /* PLDL1KEEP */, Addr, Offset, 2, 0xf9800000, 0xf8800000);
}

//
// LDP wDest1, wDest2, [xAddr, Offset]
// STP wDest1, wDest2, [xAddr, Offset]
//

inline
int
EmitLdpStpOffsetCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam SrcDest1,
    Arm64SimpleRegisterParam SrcDest2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset,
    LONG AccessShift,
    ULONG Opcode
    )
{
    LONG EncodeOffset = Offset >> AccessShift;
    if ((EncodeOffset << AccessShift) == Offset && EncodeOffset >= -0x40 && EncodeOffset <= 0x3f) {
        return Emitter.EmitFourBytes(Opcode | ((EncodeOffset & 0x7f) << 15) | (SrcDest2.RawRegister() << 10) | (Addr.RawRegister() << 5) | SrcDest1.RawRegister());
    }

    AssertMsg(false, "EmitLdpStpOffsetCommon failed to emit");
    return 0;
}

inline
int
EmitLdpOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    Assert(Dest1.RawRegister() != Dest2.RawRegister());
    return EmitLdpStpOffsetCommon(Emitter, Dest1, Dest2, Addr, Offset, 2, 0x29400000);
}

inline
int
EmitLdpOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    Assert(Dest1.RawRegister() != Dest2.RawRegister());
    return EmitLdpStpOffsetCommon(Emitter, Dest1, Dest2, Addr, Offset, 3, 0xa9400000);
}

inline
int
EmitLdpOffsetPostIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
)
{
    if (Offset == 0) {
        return EmitLdpOffset(Emitter, Dest1, Dest2, Addr, 0);
    } else {
        return EmitLdpStpOffsetCommon(Emitter, Dest1, Dest2, Addr, Offset, 2, 0x28c00000);
    }
}

inline
int
EmitLdpOffsetPostIndex64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
)
{
    if (Offset == 0) {
        return EmitLdpOffset64(Emitter, Dest1, Dest2, Addr, 0);
    } else {
        return EmitLdpStpOffsetCommon(Emitter, Dest1, Dest2, Addr, Offset, 3, 0xa8c00000);
    }
}

inline
int
EmitStpOffset(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdpStpOffsetCommon(Emitter, Source1, Source2, Addr, Offset, 2, 0x29000000);
}

inline
int
EmitStpOffset64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
    )
{
    return EmitLdpStpOffsetCommon(Emitter, Source1, Source2, Addr, Offset, 3, 0xa9000000);
}

inline
int
EmitStpOffsetPreIndex(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
)
{
    if (Offset == 0) {
        return EmitStpOffset(Emitter, Source1, Source2, Addr, 0);
    } else {
        return EmitLdpStpOffsetCommon(Emitter, Source1, Source2, Addr, Offset, 2, 0x29800000);
    }
}

inline
int
EmitStpOffsetPreIndex64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr,
    LONG Offset
)
{
    if (Offset == 0) {
        return EmitStpOffset64(Emitter, Source1, Source2, Addr, 0);
    } else {
        return EmitLdpStpOffsetCommon(Emitter, Source1, Source2, Addr, Offset, 3, 0xa9800000);
    }
}

//
// LDARB dest, [addr]
// LDARH dest, [addr]
// LDAR dest, [addr]
//
// LDXRB dest, [addr]
// LDXRH dest, [addr]
// LDXR dest, [addr]
//
// LDAXRB dest, [addr]
// LDAXRH dest, [addr]
// LDAXR dest, [addr]
//
// STLRB dest, [addr]
// STLRH dest, [addr]
// STLR dest, [addr]
//

inline
int
EmitLdaStlCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Addr.RawRegister() << 5) | Dest.RawRegister());
}

#define ARM64_OPCODE_LDARB  0x08dffc00
#define ARM64_OPCODE_LDARH  0x48dffc00
#define ARM64_OPCODE_LDAR   0x88dffc00
#define ARM64_OPCODE_LDAR64 0xc8dffc00

inline
int
EmitLdarb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDARB);
}

inline
int
EmitLdarh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDARH);
}

inline
int
EmitLdar(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAR);
}

inline
int
EmitLdar64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAR64);
}

#define ARM64_OPCODE_LDXRB   0x085f7c00
#define ARM64_OPCODE_LDXRH   0x485f7c00
#define ARM64_OPCODE_LDXR    0x885f7c00
#define ARM64_OPCODE_LDXR64  0xc85f7c00

inline
int
EmitLdxrb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDXRB);
}

inline
int
EmitLdxrh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDXRH);
}

inline
int
EmitLdxr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDXR);
}

inline
int
EmitLdxr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDXR64);
}

#define ARM64_OPCODE_LDAXRB  0x085ffc00
#define ARM64_OPCODE_LDAXRH  0x485ffc00
#define ARM64_OPCODE_LDAXR   0x885ffc00
#define ARM64_OPCODE_LDAXR64 0xc85ffc00

inline
int
EmitLdaxrb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAXRB);
}

inline
int
EmitLdaxrh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAXRH);
}

inline
int
EmitLdaxr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAXR);
}

inline
int
EmitLdaxr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Dest, Addr, ARM64_OPCODE_LDAXR64);
}

#define ARM64_OPCODE_STLRB  0x089ffc00
#define ARM64_OPCODE_STLRH  0x489ffc00
#define ARM64_OPCODE_STLR   0x889ffc00
#define ARM64_OPCODE_STLR64 0xc89ffc00

inline
int
EmitStlrb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Source, Addr, ARM64_OPCODE_STLRB);
}

inline
int
EmitStlrh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Source, Addr, ARM64_OPCODE_STLRH);
}

inline
int
EmitStlr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Source, Addr, ARM64_OPCODE_STLR);
}

inline
int
EmitStlr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaStlCommon(Emitter, Source, Addr, ARM64_OPCODE_STLR64);
}

//
// STXRB status, src, [addr]
// STXRH status, src, [addr]
// STXR status, src, [addr]
//
// STLXRB status, src, [addr]
// STLXRH status, src, [addr]
// STLXR status, src, [addr]
//

inline
int
EmitStlxrCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Status.RawRegister() << 16) | (Addr.RawRegister() << 5) | Source.RawRegister());
}

#define ARM64_OPCODE_STXRB   0x08007c00
#define ARM64_OPCODE_STXRH   0x48007c00
#define ARM64_OPCODE_STXR    0x88007c00
#define ARM64_OPCODE_STXR64  0xc8007c00

inline
int
EmitStxrb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STXRB);
}

inline
int
EmitStxrh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STXRH);
}

inline
int
EmitStxr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STXR);
}

inline
int
EmitStxr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STXR64);
}

#define ARM64_OPCODE_STLXRB  0x0800fc00
#define ARM64_OPCODE_STLXRH  0x4800fc00
#define ARM64_OPCODE_STLXR   0x8800fc00
#define ARM64_OPCODE_STLXR64 0xc800fc00

inline
int
EmitStlxrb(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STLXRB);
}

inline
int
EmitStlxrh(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STLXRH);
}

inline
int
EmitStlxr(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STLXR);
}

inline
int
EmitStlxr64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxrCommon(Emitter, Status, Source, Addr, ARM64_OPCODE_STLXR64);
}

//
// LDAXP dest1, dest2, [addr]
//

inline
int
EmitLdaxpCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Dest2.RawRegister() << 10) | (Addr.RawRegister() << 5) | Dest1.RawRegister());
}

inline
int
EmitLdaxp(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaxpCommon(Emitter, Dest1, Dest2, Addr, 0x887f8000);
}

inline
int
EmitLdaxp64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Dest1,
    Arm64SimpleRegisterParam Dest2,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitLdaxpCommon(Emitter, Dest1, Dest2, Addr, 0xc87f8000);
}

//
// STLXP status, src1, src2, [addr]
//

inline
int
EmitStlxpCommon(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr,
    ULONG Opcode
    )
{
    return Emitter.EmitFourBytes(Opcode | (Status.RawRegister() << 16) | (Source2.RawRegister() << 10) | (Addr.RawRegister() << 5) | Source1.RawRegister());
}

inline
int
EmitStlxp(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxpCommon(Emitter, Status, Source1, Source2, Addr, 0x88208000);
}

inline
int
EmitStlxp64(
    Arm64CodeEmitter &Emitter,
    Arm64SimpleRegisterParam Status,
    Arm64SimpleRegisterParam Source1,
    Arm64SimpleRegisterParam Source2,
    Arm64SimpleRegisterParam Addr
    )
{
    return EmitStlxpCommon(Emitter, Status, Source1, Source2, Addr, 0xc8208000);
}
