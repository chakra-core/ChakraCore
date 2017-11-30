//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Arm64XdataGenerator
{
public:
    Arm64XdataGenerator();

    void Generate(PULONG prologStart, PULONG prologEnd, PULONG epilogStart, PULONG epilogEnd, PULONG functionEnd, ULONG exceptionHandlerRva = 0, ULONG exceptionData = 0);

    const VOID *GetXdata() const { return &this->m_xdata[0]; }
    ULONG GetXdataBytes() const { return this->m_xdataBytes; }

    // xdata opcodes map 1:1 with instructions; compute an estimated maximum 
    // instructions for prolog/epilog:
    //     4: 2xMOV + BL + SUB for allocation (worst case assumes _chkstk call)
    //     4: STP for homing parameters (x0-x7)
    //     6: STP for saving integer non-vols in pairs (x19-x28 plus fp/lr)
    //     4: STP for saving FP non-vols in pairs (d8-d16)
    //   =18, round up to 20 instructions for flotsam (extra no-ops)
    // Assume 2 bytes/opcode on average (one opcode is 4 bytes; many are only 1)
    // Plus a factor of 2 because prolog and epilog can each have a list
    static const ULONG maxOpcodeBytes = 20 * 2 * 2;

    // xdata consists of a few header words plus opcodes for prolog and epilog:
    //     8: up to 2 header words
    //     4: possibly 1 exception scope
    //   MOB: opcode bytes
    //     8: exception handler + data
    static const ULONG maxXdataSize = 8 + 4 + maxOpcodeBytes + 8;

private:
    void SafeAppendDword(DWORD value);

    ULONG m_xdata[(maxXdataSize + 3) / 4];
    ULONG m_xdataBytes;
};

struct OpcodeMatcher
{
    bool Matches(ULONG opcode) const { return (opcode & this->mask) == this->pattern; }

    ULONG   mask;
    ULONG   pattern;
};

struct OpcodeList
{
    OpcodeMatcher   subSpSpImm;         // sub sp, sp, #imm / add sp, sp, #imm [epilog]
    OpcodeMatcher   subSpSpReg;         // sub sp, sp, reg / add sp, sp, reg [epilog]
    OpcodeMatcher   addFpSpImm;         // add fp, sp, #imm
    OpcodeMatcher   stpRtRt2SpOffs;     // stp rt, rt2, [sp, #offs] / ldp rt, rt2, [sp, #offs]
    OpcodeMatcher   stpRtRt2SpOffsBang; // stp rt, rt2, [sp, #offs]! / ldp rt, rt2, [sp], #offs
    OpcodeMatcher   strRtSpOffs;        // str rt, [sp, #offs] / ldr rt, [sp, #offs]
    OpcodeMatcher   strRtSpOffsBang;    // str rt, [sp, #offs]! / ldr rt, [sp], #offs
    OpcodeMatcher   stpDtDt2SpOffs;     // stp dt, dt2, [sp, #offs] / ldp dt, dt2, [sp, #offs]
    OpcodeMatcher   stpDtDt2SpOffsBang; // stp dt, dt2, [sp, #offs]! / ldp dt, dt2, [sp], #offs
    OpcodeMatcher   strDtSpOffs;        // str dt, [sp, #offs] / ldr dt, [sp, #offs]
    OpcodeMatcher   strDtSpOffsBang;    // str dt, [sp, #offs]! / ldr dt, [sp], #offs
};

class Arm64UnwindCodeGenerator
{
    // opcodes as defined by the Windows ARM64 Exception Data document
    static const ULONG op_alloc_s = 0x00;           // 000xxxxx: allocate small stack with size < 512 (2^5 * 16).
    static const ULONG op_save_r19r20_x = 0x20;     // 001zzzzz: save <r19,r20> pair at [sp-#Z*8]!, pre-indexed offset >= -248
    static const ULONG op_save_fplr = 0x40;         // 01zzzzzz: save <r29,lr> pair at [sp+#Z*8],  offset <= 504.
    static const ULONG op_save_fplr_x = 0x80;       // 10zzzzzz: save <r29,lr> pair at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
    static const ULONG op_alloc_m = 0xc000;         // 11000xxx|xxxxxxxx: allocate large stack with size < 32k (2^11 * 16).
    static const ULONG op_save_regp = 0xc800;       // 110010xx|xzzzzzz: save r(19+#X) pair at [sp+#Z*8], offset <= 504
    static const ULONG op_save_regp_x = 0xcc00;     // 110011xx|xxzzzzzz: save pair r(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
    static const ULONG op_save_reg = 0xd000;        // 110100xx|xxzzzzzz: save reg r(19+#X) at [sp+#Z*8], offset <=504
    static const ULONG op_save_reg_x = 0xd400;      // 1101010x|xxxzzzzz: save reg r(19+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
    static const ULONG op_save_lrpair = 0xd600;     // 1101011x|xxzzzzzz: save pair <r19+2*#X,rl> at [sp+#Z*8], offset <= 504
    static const ULONG op_save_fregp = 0xd800;      // 1101100x|xxzzzzzz: save pair d(8+#X) at [sp+#Z*8], offset <=504
    static const ULONG op_save_fregp_x = 0xda00;    // 1101101x|xxzzzzzz: save pair d(8+#X), at [sp-(#Z+1)*8]!, pre-indexed offset >= -512
    static const ULONG op_save_freg = 0xdc00;       // 1101110x|xxzzzzzz: save reg d(9+#X) at [sp+#Z*8], offset <=504
    static const ULONG op_save_freg_x = 0xde00;     // 11011110|xxxzzzzz: save reg d(8+#X) at [sp-(#Z+1)*8]!, pre-indexed offset >= -256
    static const ULONG op_alloc_l = 0xe0000000;     // 11100000|xxxxxxxx|xxxxxxxx|xxxxxxxx: allocate large stack with size < 256M
    static const ULONG op_set_fp = 0xe1;            // 11100001: set up r29: with: mov r29,sp
    static const ULONG op_add_fp = 0xe200;          // 11100010|xxxxxxxx: set up r29 with: add r29,sp,#x*8
    static const ULONG op_nop = 0xe3;               // 11100011: no unwind operation is required.
    static const ULONG op_end = 0xe4;               // 11100100: end of unwind code
    static const ULONG op_end_c = 0xe5;             // 11100101: end of unwind code in current chained scope.
    static const ULONG op_save_next = 0xe6;         // 11100110: save next non-volatile Int or FP register pair.**

    // maximum instructions in the internal buffer, figuring:
    //   +4 for worst-case stack allocation (via chkstk)
    //   +6 for saving integer non-volatiles
    //   +4 for saving FP non-volatiles
    //   +4 for homing paramters
    //   +1 for setting up/recovering frame pointer
    //   +1 for return
    //  +12 for random things and to be safe and to make a nice power of 2
    static const ULONG MAX_INSTRUCTIONS = 32;

public:
    Arm64UnwindCodeGenerator();

    // generate the code for a prolog or epilog as appropriate
    ULONG GeneratePrologCodes(PBYTE buffer, ULONG bufferSize, PULONG &prologStart, PULONG &prologEnd);
    ULONG GenerateEpilogCodes(PBYTE buffer, ULONG bufferSize, PULONG &epilogStart, PULONG &epilogEnd);

private:
    // internal helpers
    ULONG GenerateSingleOpcode(PULONG pOpcode, PULONG pRegionStart, const OpcodeList &opcodeList);
    ULONG TrimNops(PULONG opcodeList, ULONG numOpcodes);
    void ReverseCodes(PULONG opcodeList, ULONG numOpcodes);
    ULONG64 FindRegisterImmediate(int regNum, PULONG registerStart, PULONG regionEnd);
    ULONG EmitFinalCodes(PBYTE buffer, ULONG bufferSize, PULONG opcodes, ULONG count);

    // encode an opcode and parameters, up to 4 bytes depending on the opcode
    ULONG SafeEncode(ULONG opcode, ULONG params);

    // encode an opcode and parameters for a register pair saving operation
    ULONG SafeEncodeRegPair(ULONG opcode, ULONG params, int regBase, int rebasedOffset);

    // rebase registers and offets as needed for opcodes
    int RebaseReg(int reg);
    int RebaseRegPair(int reg1, int reg2);
    int RebaseFpReg(int reg);
    int RebaseFpRegPair(int reg1, int reg2);
    int RebaseOffset(int offset, int maxOffset);

    // specific encoders
    ULONG EncodeAlloc(ULONG bytes);
    ULONG EncodeStoreReg(int reg, int offset);
    ULONG EncodeStoreRegPredec(int reg, int offset);
    ULONG EncodeStorePair(int reg1, int reg2, int offset);
    ULONG EncodeStorePairPredec(int reg1, int reg2, int offset);
    ULONG EncodeStoreFpReg(int reg, int offset);
    ULONG EncodeStoreFpRegPredec(int reg, int offset);
    ULONG EncodeStoreFpPair(int reg1, int reg2, int offset);
    ULONG EncodeStoreFpPairPredec(int reg1, int reg2, int offset);
    ULONG EncodeAddFp(int offset);

    // simple inline encoders
    ULONG EncodeSetFp() { return this->SafeEncode(op_set_fp, 0); }
    ULONG EncodeNop() { return this->SafeEncode(op_nop, 0); }
    ULONG EncodeEnd() { return this->SafeEncode(op_end, 0); }

    // save_next tracking
    int m_lastPair;
    int m_lastPairOffset;

    // immediate tracking
    int64 m_pendingImmediate;
    int m_pendingImmediateReg;

    // fixed opcodes
    static const OpcodeMatcher MovkOpcode;
    static const OpcodeMatcher BlrOpcode;
    static const OpcodeMatcher RetOpcode;
    static const OpcodeMatcher SubSpSpX15Uxtx4Opcode;

    // specific opcodes for prologs vs. epilogs
    static const OpcodeList PrologOpcodes;
    static const OpcodeList EpilogOpcodes;
};
