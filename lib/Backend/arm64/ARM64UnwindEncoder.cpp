//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"
#include "ARM64UnwindEncoder.h"

const OpcodeMatcher Arm64UnwindCodeGenerator::MovkOpcode            = { 0x1f800000, 0x12800000 };
const OpcodeMatcher Arm64UnwindCodeGenerator::BlrOpcode             = { 0xfffffc1f, 0xd63f0000 };
const OpcodeMatcher Arm64UnwindCodeGenerator::RetOpcode             = { 0xffffffff, 0xd65f03c0 };
const OpcodeMatcher Arm64UnwindCodeGenerator::SubSpSpX15Uxtx4Opcode = { 0xffffffff, 0xcb2f73ff };

const OpcodeList Arm64UnwindCodeGenerator::PrologOpcodes =
{
    { 0xff8003ff, 0xd10003ff },         // sub sp, sp, #imm
    { 0xffe0ffff, 0xcb4063ff },         // sub sp, sp, reg
    { 0xff8003ff, 0x910003fd },         // add fp, sp, #imm
    { 0xffc003e0, 0xa90003e0 },         // stp rt, rt2, [sp, #offs]
    { 0xffc003e0, 0xa98003e0 },         // stp rt, rt2, [sp, #offs]!
    { 0xffc003e0, 0xf90003e0 },         // str rt, [sp, #offs]
    { 0xffe00fe0, 0xf8000fe0 },         // str rt, [sp, #offs]!
    { 0xffc003e0, 0x6d0003e0 },         // stp dt, dt2, [sp, #offs]
    { 0xffc003e0, 0x6d8003e0 },         // stp dt, dt2, [sp, #offs]!
    { 0xffc003e0, 0xfd0003e0 },         // str dt, [sp, #offs]
    { 0xffe00fe0, 0xfc000fe0 },         // str dt, [sp, #offs]!
};

const OpcodeList Arm64UnwindCodeGenerator::EpilogOpcodes =
{
    { 0xff8003ff, 0x910003ff },         // add sp, sp, #imm
    { 0xffe0ffff, 0x8b2063ff },         // add sp, sp, reg
    { 0xff8003ff, 0xd10003bf },         // sub sp, fp, #imm
    { 0xffc003e0, 0xa94003e0 },         // ldp rt, rt2, [sp, #offs]
    { 0xffc003e0, 0xa8c003e0 },         // ldp rt, rt2, [sp], #offs
    { 0xffc003e0, 0xf94003e0 },         // ldr rt, [sp, #offs]
    { 0xffe00fe0, 0xf84007e0 },         // ldr rt, [sp], #offs
    { 0xffc003e0, 0x6d4003e0 },         // ldp dt, dt2, [sp, #offs]
    { 0xffc003e0, 0x6cc003e0 },         // ldp dt, dt2, [sp], #offs
    { 0xffc003e0, 0xfd4003e0 },         // ldr dt, [sp, #offs]
    { 0xffe00fe0, 0xfc4007e0 },         // ldr dt, [sp], #offs
};


Arm64XdataGenerator::Arm64XdataGenerator()
    : m_xdataBytes(0)
{
}

void Arm64XdataGenerator::SafeAppendDword(DWORD value)
{
    Assert(m_xdataBytes < sizeof(this->m_xdata));
    if (m_xdataBytes < sizeof(this->m_xdata))
    {
        this->m_xdata[this->m_xdataBytes / 4] = value;
    }
    this->m_xdataBytes += 4;
}

void Arm64XdataGenerator::Generate(PULONG prologStart, PULONG prologEnd, PULONG epilogStart, PULONG epilogEnd, PULONG functionEnd, ULONG exceptionHandlerRva, ULONG exceptionData)
{
    Assert(prologStart != NULL);
    Assert(prologEnd != NULL);
    Assert(prologEnd >= prologStart);
    Assert(functionEnd != NULL);
    Assert(functionEnd >= prologEnd);

    if (epilogStart != NULL)
    {
        Assert(epilogEnd != NULL);
        Assert(epilogStart >= prologEnd);
        Assert(epilogEnd >= epilogStart);
        Assert(functionEnd >= epilogEnd);
    }

    // single records are capped to 0x40000 instructions, or 1MB total code size
    // future: if needed, break functions into parts
    ptrdiff_t functionLength = functionEnd - prologStart;
    if (functionLength >= 0x40000)
    {
        // forcenative can hit this, so instead of asserting abort jit
        throw Js::OperationAbortedException();
    }

    // first generate the codes for prolog
    Arm64UnwindCodeGenerator generator;
    BYTE unwindCodes[maxOpcodeBytes * 2];
    ULONG prologCodeBytes = generator.GeneratePrologCodes(&unwindCodes[0], sizeof(unwindCodes), prologStart, prologEnd);

    // then for the epilog
    PBYTE epilogCodes = &unwindCodes[prologCodeBytes];
    ULONG epilogCodeBytes = 0;
    if (epilogStart != NULL)
    {
        epilogCodeBytes = generator.GenerateEpilogCodes(epilogCodes, sizeof(unwindCodes) - prologCodeBytes, epilogStart, epilogEnd);
    }

    // see if the epilog codes can point into the prolog codes, then compute the total size
    bool epilogSubstring = (epilogCodeBytes > 0 && epilogCodeBytes <= prologCodeBytes && memcmp(epilogCodes, &unwindCodes[prologCodeBytes - epilogCodeBytes], epilogCodeBytes) == 0);
    ULONG epilogCodesStart = epilogSubstring ? (prologCodeBytes - epilogCodeBytes) : prologCodeBytes;
    ULONG totalCodeSize = epilogCodesStart + epilogCodeBytes;
    ULONG codeWords = (totalCodeSize + 3) / 4;

    // determine if the epilog is at the very end or not
    bool epilogAtEnd = (epilogEnd == functionEnd) && (epilogCodesStart < 32);
    ULONG epilogCount = epilogAtEnd ? epilogCodesStart : 1;

    // determine if there is an exception handler
    bool hasExceptionHandler = (exceptionHandlerRva != 0);

    // create the header -- currentely limited to "small" functions (up to 1MB in size)
    this->m_xdataBytes = 0;
    if (codeWords < 32)
    {
        this->SafeAppendDword((codeWords << 27) | (epilogCount << 22) | (epilogAtEnd << 21) | (hasExceptionHandler << 20) | (0 << 18) | ULONG(functionLength));
    }
    else
    {
        this->SafeAppendDword((0 << 27) | (0 << 22) | (epilogAtEnd << 21) | (hasExceptionHandler << 20) | (0 << 18) | ULONG(functionLength));
        this->SafeAppendDword((codeWords << 16) | epilogCount);
    }

    // if the epilog is not at the end, append the single epilog scope
    if (!epilogAtEnd)
    {
        this->SafeAppendDword((epilogCodesStart << 22) | ULONG(epilogStart - prologStart));
    }

    // next append the prolog codes
    for (ULONG curWord = 0; curWord < codeWords; curWord++)
    {
        this->SafeAppendDword(PULONG(&unwindCodes[0])[curWord]);
    }

    // finally append the exception handler & data
    if (hasExceptionHandler)
    {
        this->SafeAppendDword(exceptionHandlerRva);
        this->SafeAppendDword(exceptionData);
    }
}


Arm64UnwindCodeGenerator::Arm64UnwindCodeGenerator()
    : m_lastPair(0),
      m_lastPairOffset(0)
{
}

int Arm64UnwindCodeGenerator::RebaseReg(int reg)
{
    Assert(reg >= 19 && reg <= 30);
    return reg - 19;
}

int Arm64UnwindCodeGenerator::RebaseRegPair(int reg1, int reg2)
{
    int result = RebaseReg(reg1);
    RebaseReg(reg2);
    Assert(reg1 + 1 == reg2);
    return result;
}

int Arm64UnwindCodeGenerator::RebaseFpReg(int reg)
{
    Assert(reg >= 8 && reg <= 16);
    return reg - 8;
}

int Arm64UnwindCodeGenerator::RebaseFpRegPair(int reg1, int reg2)
{
    int result = RebaseFpReg(reg1);
    RebaseFpReg(reg2);
    Assert(reg1 + 1 == reg2);
    return result;
}

int Arm64UnwindCodeGenerator::RebaseOffset(int offset, int maxOffset)
{
    Assert(offset >= 0);
    Assert(offset % 8 == 0);
    offset /= 8;
    Assert(offset < maxOffset);
    return offset;
}

ULONG Arm64UnwindCodeGenerator::SafeEncode(ULONG opcode, ULONG params)
{
    // reset tracking for save_next opcode
    m_lastPair = 0;
    m_lastPairOffset = 0;
    return opcode | params;
}

ULONG Arm64UnwindCodeGenerator::SafeEncodeRegPair(ULONG opcode, ULONG params, int regBase, int rebasedOffset)
{
    // only valid for those specific opcodes which can precede a save_next opcode
    Assert(opcode == op_save_r19r20_x || opcode == op_save_regp || opcode == op_save_regp_x || opcode == op_save_fregp || opcode == op_save_fregp_x);
        
    // if this is the next register in sequence and the offset matches, encode as save next instead
    if (m_lastPair + 2 == regBase && m_lastPairOffset + 2 == rebasedOffset)
    {
        opcode = op_save_next;
        params = 0;
    }

    // update the next register tracking
    m_lastPair = regBase;
    m_lastPairOffset = rebasedOffset;
    return opcode | params;
}

ULONG Arm64UnwindCodeGenerator::EncodeAlloc(ULONG bytes)
{
    Assert(bytes % 16 == 0);
    ULONG immed = bytes / 16;

    if (immed < 0x20)
    {
        // alloc_s         000xxxxx: allocate small stack with size < 512 (2^5 * 16).
        return this->SafeEncode(op_alloc_s, immed & 0x1f);
    }
    else if (immed < 0x800)
    {
        // alloc_m         11000xxx|xxxxxxxx: allocate large stack with size < 32k (2^11 * 16).
        return this->SafeEncode(op_alloc_m, immed & 0x3ff);
    }
    else
    {
        // alloc_l         11100000|xxxxxxxx|xxxxxxxx|xxxxxxxx: allocate large stack with size < 256M
        return this->SafeEncode(op_alloc_l, immed & 0xffffff);
    }
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreReg(int reg, int offset)
{
    Assert(offset >= 0);
    int immed = this->RebaseOffset(offset, 0x40);
    int regBase = this->RebaseReg(reg);

    // save_reg        110100xx|xxzzzzzz: save reg r(19+#X) at [sp+#Z*8], offset <=504
    return this->SafeEncode(op_save_reg, ((regBase << 6) & 0x3c0) | (immed & 0x3f));
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreRegPredec(int reg, int offset)
{
    Assert(offset < 0);
    int immed = this->RebaseOffset(-offset, 0x20);
    int regBase = this->RebaseReg(reg);

    // save_reg_x      1101010x|xxxzzzzz: save reg r(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
    return this->SafeEncode(op_save_reg_x, ((regBase << 5) & 0x1e0) | (immed & 0x1f));
}

ULONG Arm64UnwindCodeGenerator::EncodeStorePair(int reg1, int reg2, int offset)
{
    Assert(offset >= 0);
    int immed = this->RebaseOffset(offset, 0x40);

    // special case for LR as second register
    if (reg2 == 30)
    {
        if (reg1 == 29)
        {
            // save_fplr       01zzzzzz: save <r29,lr> pair at [sp+#Z*8],  offset <= 504.
            return this->SafeEncode(op_save_fplr, immed & 0x3f);
        }
        else
        {
            // save_lrpair     1101011x|xxzzzzzz: save pair <r19+2*#X,rl> at [sp+#Z*8], offset <= 504
            int regBase = this->RebaseReg(reg1);
            Assert(regBase % 2 == 0);
            regBase /= 2;
            return this->SafeEncode(op_save_lrpair, ((regBase << 6) & 0x1c0) | (immed & 0x3f));
        }
    }
    else
    {
        // save_regp       110010xx|xxzzzzzz: save r(19+#X) pair at [sp+#Z*8], offset <= 504
        int regBase = this->RebaseRegPair(reg1, reg2);
        return this->SafeEncodeRegPair(op_save_regp, ((regBase << 6) & 0x3c0) | (immed & 0x3f), regBase, immed);
    }
}

ULONG Arm64UnwindCodeGenerator::EncodeStorePairPredec(int reg1, int reg2, int offset)
{
    Assert(offset < 0);
    int immed = this->RebaseOffset(-offset, 0x40);
    int regBase = this->RebaseRegPair(reg1, reg2);

    // special case for FP/LR
    if (reg1 == 29)
    {
        // save_fplr_x     10zzzzzz: save <r29,lr> pair at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
        return this->SafeEncode(op_save_fplr_x, immed & 0x3f);
    }
    else if (reg1 == 19 && immed < 0x20)
    {
        // save_r19r20_x   001zzzzz: save <r19,r20> pair at [sp-#Z*8]!, pre-indexed offset >= -248
        return this->SafeEncodeRegPair(op_save_r19r20_x, immed & 0x1f, this->RebaseReg(19), 0);
    }
    else
    {
        // save_regp_x     110011xx|xxzzzzzz: save pair r(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
        return this->SafeEncodeRegPair(op_save_regp_x, ((regBase << 6) & 0x3c0) | (immed & 0x3f), regBase, 0);
    }
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreFpReg(int reg, int offset)
{
    Assert(offset >= 0);
    int immed = this->RebaseOffset(offset, 0x40);
    int regBase = this->RebaseFpReg(reg);

    // save_freg       1101110x|xxzzzzzz: save reg d(9+#X) at [sp+#Z*8], offset <=504
    return this->SafeEncode(op_save_freg, ((regBase << 6) & 0x1c0) | (immed & 0x3f));
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreFpRegPredec(int reg, int offset)
{
    Assert(offset < 0);
    int immed = this->RebaseOffset(-offset, 0x20);
    int regBase = this->RebaseFpReg(reg);

    // save_freg_x     11011110|xxxzzzzz: save reg d(8+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
    return this->SafeEncode(op_save_freg_x, ((regBase << 5) & 0xe0) | (immed & 0x1f));
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreFpPair(int reg1, int reg2, int offset)
{
    Assert(offset >= 0);
    int immed = this->RebaseOffset(offset, 0x40);
    int regBase = this->RebaseFpRegPair(reg1, reg2);

    // save_fregp      1101100x|xxzzzzzz: save pair d(8+#X) at [sp+#Z*8], offset <=504
    return this->SafeEncodeRegPair(op_save_fregp, ((regBase << 6) & 0x1c0) | (immed & 0x3f), 32 + regBase, immed);
}

ULONG Arm64UnwindCodeGenerator::EncodeStoreFpPairPredec(int reg1, int reg2, int offset)
{
    Assert(offset < 0);
    int immed = this->RebaseOffset(offset, 0x40);
    int regBase = this->RebaseFpRegPair(reg1, reg2);

    // save_fregp_x    1101101x|xxzzzzzz: save pair d(8+#X), at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
    return this->SafeEncodeRegPair(op_save_fregp_x, ((regBase << 6) & 0x1c0) | (immed & 0x3f), 32 + regBase, 0);
}

ULONG Arm64UnwindCodeGenerator::EncodeAddFp(int offset)
{
    Assert(offset > 0);
    int immed = this->RebaseOffset(offset, 0x100);
    return this->SafeEncode(op_add_fp, immed & 0xff);
}

ULONG Arm64UnwindCodeGenerator::GenerateSingleOpcode(PULONG opcodePtr, PULONG regionStart, const OpcodeList &opcodeList)
{
    ULONG opcode = *opcodePtr;

    // SUB SP, SP, #imm / ADD SP, SP, #imm
    if (opcodeList.subSpSpImm.Matches(opcode))
    {
        int shift = (opcode >> 22) & 1;
        ULONG bytes = ((opcode >> 10) & 0xfff) << (12 * shift);
        return this->EncodeAlloc(bytes);
    }

    // SUB SP, SP, reg / ADD SP, SP, reg
    else if (opcodeList.subSpSpReg.Matches(opcode))
    {
        int regNum = (opcode >> 16) & 31;
        ULONG64 immediate = this->FindRegisterImmediate(regNum, regionStart, opcodePtr);
        Assert(immediate < 0xffffff);
        return this->EncodeAlloc(ULONG(immediate));
    }

    // ADD FP, SP, #imm
    else if (opcodeList.addFpSpImm.Matches(opcode))
    {
        int shift = (opcode >> 22) & 1;
        ULONG bytes = ((opcode >> 10) & 0xfff) << (12 * shift);
        if (bytes != 0)
        {
            return this->EncodeAddFp(bytes);
        }
        else
        {
            return this->EncodeSetFp();
        }
    }

    // STP rT, rT2, [SP, #offs] / LDP rT, rT2, [SP, #offs]
    else if (opcodeList.stpRtRt2SpOffs.Matches(opcode))
    {
        int rt = opcode & 31;
        int rt2 = (opcode >> 10) & 31;
        int offset = (int32(opcode << 10) >> 25) << 3;

        // special case for homing parameters / volatile registers
        if (rt < 19 && rt2 < 19)
        {
            return this->EncodeNop();
        }
        else
        {
            return this->EncodeStorePair(rt, rt2, offset);
        }
    }

    // STP rT, rT2, [SP, #offs]! / LDP rT, rT2, [SP], #offs
    else if (opcodeList.stpRtRt2SpOffsBang.Matches(opcode))
    {
        int rt = opcode & 31;
        int rt2 = (opcode >> 10) & 31;
        int offset = (int32(opcode << 10) >> 25) << 3;
        return this->EncodeStorePairPredec(rt, rt2, offset);
    }

    // STR rT, [SP, #offs] / LDR rT, [SP, #offs]
    else if (opcodeList.strRtSpOffs.Matches(opcode))
    {
        int rt = opcode & 31;
        int offset = ((opcode >> 10) & 0xfff) << 3;
        return this->EncodeStoreReg(rt, offset);
    }

    // STR rT, [SP, #offs]! / LDR rT, [SP], #offs
    else if (opcodeList.strRtSpOffsBang.Matches(opcode))
    {
        int rt = opcode & 31;
        int offset = int32(opcode << 11) >> 23;
        return this->EncodeStoreRegPredec(rt, offset);
    }

    // STP dT, dT2, [SP, #offs] / LDP dT, dT2, [SP, #offs]
    else if (opcodeList.stpDtDt2SpOffs.Matches(opcode))
    {
        int rt = opcode & 31;
        int rt2 = (opcode >> 10) & 31;
        int offset = (int32(opcode << 10) >> 25) << 3;
        return this->EncodeStoreFpPair(rt, rt2, offset);
    }

    // STP dT, dT2, [SP, #offs]! / LDP dT, dT2, [SP], #offs
    else if (opcodeList.stpDtDt2SpOffsBang.Matches(opcode))
    {
        int rt = opcode & 31;
        int rt2 = (opcode >> 10) & 31;
        int offset = (int32(opcode << 10) >> 25) << 3;
        return this->EncodeStoreFpPairPredec(rt, rt2, offset);
    }

    // STR dT, [SP, #offs] / LDR dT, [SP, #offs]
    else if (opcodeList.strDtSpOffs.Matches(opcode))
    {
        int rt = opcode & 31;
        int offset = ((opcode >> 10) & 0xfff) << 3;
        return this->EncodeStoreFpReg(rt, offset);
    }

    // STR dT, [SP, #offs]! / LDR dT, [SP], #offs
    else if (opcodeList.strDtSpOffsBang.Matches(opcode))
    {
        int rt = opcode & 31;
        int offset = int32(opcode << 11) >> 23;
        return this->EncodeStoreFpRegPredec(rt, offset);
    }

    // SUB SP, SP, x15 UXTX #4
    else if (SubSpSpX15Uxtx4Opcode.Matches(opcode))
    {
        ULONG64 immediate = this->FindRegisterImmediate(15, regionStart, opcodePtr);
        Assert(immediate < 0xffffff / 16);
        return this->EncodeAlloc(ULONG(immediate) * 16);
    }

    // BLR <chkstk> / RET / MOVK/MOVN/MOVZ
    else if (BlrOpcode.Matches(opcode) || RetOpcode.Matches(opcode) || MovkOpcode.Matches(opcode))
    {
        // fall through to encode a NOP, but avoid the assert
    }

    // unexpected opcode!
    else
    {
        Assert(false);
    }

    // by default just return an encoded NOP
    return this->EncodeNop();
}

ULONG Arm64UnwindCodeGenerator::TrimNops(PULONG opcodeList, ULONG numOpcodes)
{
    ULONG count;
    for (count = 0; count < numOpcodes; count++)
    {
        if (opcodeList[numOpcodes - 1 - count] != op_nop)
        {
            break;
        }
    }
    return count;
}

void Arm64UnwindCodeGenerator::ReverseCodes(PULONG opcodeList, ULONG numOpcodes)
{
    // simple swap-reversal
    for (int index = numOpcodes / 2 - 1; index >= 0; index--)
    {
        ULONG temp = opcodeList[numOpcodes - 1 - index];
        opcodeList[numOpcodes - 1 - index] = opcodeList[index];
        opcodeList[index] = temp;
    }
}

ULONG Arm64UnwindCodeGenerator::EmitFinalCodes(PBYTE buffer, ULONG bufferSize, PULONG opcodes, ULONG count)
{
    ULONG outputIndex = 0;
    for (ULONG opIndex = 0; opIndex < count; opIndex++)
    {
        ULONG opcode = opcodes[opIndex];

        // trim unneeded bytes from the top
        int numBytes = 4;
        while ((opcode & 0xff000000) == 0)
        {
            opcode <<= 8;
            numBytes -= 1;
        }

        // output the final bytes
        for (int curByte = 0; curByte < numBytes; curByte++)
        {
            if (outputIndex < bufferSize)
            {
                buffer[outputIndex] = (opcode >> 24) & 0xff;
            }
            opcode <<= 8;
            outputIndex += 1;
        }
    }
    return outputIndex;
}

ULONG64 Arm64UnwindCodeGenerator::FindRegisterImmediate(int regNum, PULONG regionStart, PULONG regionEnd)
{
    // scan forward, looking for opcodes that assemble immediate values
    ULONG64 pendingImmediate = 0;
    int pendingImmediateReg = -1;
    bool foundImmediate = false;
    for (PULONG opcodePtr = regionStart; opcodePtr < regionEnd; opcodePtr++)
    {
        ULONG opcode = *opcodePtr;

        // MOVZ/MOVK/MOVN reg
        if (MovkOpcode.Matches(opcode))
        {
            // MOVK (opc == 3) should only ever modify a previously-written register
            int opc = (opcode >> 29) & 3;
            int rd = opcode & 31;
            if (opc == 3)
            {
                Assert(pendingImmediateReg == rd);
            }
            pendingImmediateReg = rd;

            // Skip the rest if this isn't our target register
            if (rd != regNum)
            {
                continue;
            }
            foundImmediate = true;

            // MOVN (opc == 0) sign-extends
            int64 val = (opcode >> 5) & 0xffff;
            if (opc == 0)
            {
                val = int16(val);
            }

            // apply shift
            int shift = 16 * ((opcode >> 21) & 3);
            val <<= shift;

            // 32-bit operations clear the upper 32 bits
            if ((opcode & 0x80000000) == 0)
            {
                val &= 0xffffffff;
            }

            // MOVK (opc == 3) inserts into existing register; MOVN/MOVZ replace the whole value
            if (opc == 3)
            {
                pendingImmediate = (pendingImmediate & ~(0xffff << shift)) | val;
            }
            else
            {
                pendingImmediate = val;
            }
        }
    }

    // make sure we found something
    Assert(foundImmediate);
    return pendingImmediate;
}

ULONG Arm64UnwindCodeGenerator::GeneratePrologCodes(PBYTE buffer, ULONG bufferSize, PULONG &prologStart, PULONG &prologEnd)
{
    Assert(prologStart != NULL);
    Assert(prologEnd != NULL);
    Assert(prologStart <= prologEnd);

    // verify against internal buffer size; truncate if out of bounds
    ULONG numOpcodes = ULONG(prologEnd - prologStart);
    Assert(numOpcodes <= MAX_INSTRUCTIONS);
    if (numOpcodes > MAX_INSTRUCTIONS)
    {
        prologEnd = prologStart + MAX_INSTRUCTIONS;
        numOpcodes = MAX_INSTRUCTIONS;
    }

    // iterate over all prolog opcodes in forward order to produce the list of unwind opcodes
    ULONG opcodeList[MAX_INSTRUCTIONS + 1];
    for (ULONG opIndex = 0; opIndex < numOpcodes; opIndex++)
    {
        opcodeList[opIndex] = this->GenerateSingleOpcode(&prologStart[opIndex], prologStart, PrologOpcodes);
    }

    // trim out any trailing nops (they can be safely ignored)
    int trimmed = this->TrimNops(opcodeList, numOpcodes);
    prologEnd -= trimmed;
    numOpcodes -= trimmed;

    // reverse the codes and append an end
    this->ReverseCodes(opcodeList, numOpcodes);

    // don't trim initial NOPs because we want the function start to remain as-is

    // append an end and return the final size 
    opcodeList[numOpcodes++] = this->EncodeEnd();
    return this->EmitFinalCodes(buffer, bufferSize, opcodeList, numOpcodes);
}

ULONG Arm64UnwindCodeGenerator::GenerateEpilogCodes(PBYTE buffer, ULONG bufferSize, PULONG &epilogStart, PULONG &epilogEnd)
{
    Assert(epilogStart != NULL);
    Assert(epilogEnd != NULL);
    Assert(epilogStart <= epilogEnd);

    // verify against internal buffer size; truncate if out of bounds
    ULONG numOpcodes = ULONG(epilogEnd - epilogStart);
    Assert(numOpcodes <= MAX_INSTRUCTIONS);
    if (numOpcodes > MAX_INSTRUCTIONS)
    {
        epilogStart = epilogEnd - MAX_INSTRUCTIONS;
        numOpcodes = MAX_INSTRUCTIONS;
    }

    // iterate over all epilog opcodes in reverse order to produce the list of unwind opcodes
    ULONG opcodeList[MAX_INSTRUCTIONS + 1];
    for (ULONG opIndex = 0; opIndex < numOpcodes; opIndex++)
    {
        opcodeList[opIndex] = this->GenerateSingleOpcode(&epilogStart[numOpcodes - 1 - opIndex], epilogStart, EpilogOpcodes);
    }

    // trim out any trailing nops (they can be safely ignored)
    int trimmed = this->TrimNops(opcodeList, numOpcodes);
    epilogStart += trimmed;
    numOpcodes -= trimmed;

    // reverse the codes and append an end
    this->ReverseCodes(opcodeList, numOpcodes);

    // trim out any newly trailing nops
    trimmed = this->TrimNops(opcodeList, numOpcodes);
    epilogEnd -= trimmed;
    numOpcodes -= trimmed;

    // but add back in the implicit RET
    epilogEnd += 1;

    // append an end and return the final size 
    opcodeList[numOpcodes++] = this->EncodeEnd();
    return this->EmitFinalCodes(buffer, bufferSize, opcodeList, numOpcodes);
}
