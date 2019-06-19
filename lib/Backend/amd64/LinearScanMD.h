//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class OpHelperBlock;
class LinearScan;
class BailOutRecord;
class BranchBailOutRecord;

class LinearScanMD : public LinearScanMDShared
{
private:
    StackSym ** helperSpillSlots;
    Func      * func;
    uint32      maxOpHelperSpilledLiveranges;
    BitVector   byteableRegsBv;
    StackSym   *xmmSymTable128[XMM_REGCOUNT];
    StackSym   *xmmSymTable64[XMM_REGCOUNT];
    StackSym   *xmmSymTable32[XMM_REGCOUNT];

public:
    LinearScanMD(Func *func);

    StackSym   *EnsureSpillSymForXmmReg(RegNum reg, Func *func, IRType type);
    BitVector   FilterRegIntSizeConstraints(BitVector regsBv, BitVector sizeUsageBv) const;
    bool        FitRegIntSizeConstraints(RegNum reg, BitVector sizeUsageBv) const;
    bool        FitRegIntSizeConstraints(RegNum reg, IRType type) const;
    bool        IsAllocatable(RegNum reg, Func *func) const { return true; }
    uint        UnAllocatableRegCount(Func *func) const { return 2; /* RSP, RBP */ }
    void        LegalizeDef(IR::Instr * instr) { /* This is a nop for amd64 */ }
    void        LegalizeUse(IR::Instr * instr, IR::Opnd * opnd) { /* A nop for amd64 */ }
    void        LegalizeConstantUse(IR::Instr * instr, IR::Opnd * opnd);
    void        InsertOpHelperSpillAndRestores(SList<OpHelperBlock> *opHelperBlockList);
    void        EndOfHelperBlock(uint32 helperSpilledLiveranges);
    void        GenerateBailOut(IR::Instr * instr,
                                __in_ecount(registerSaveSymsCount) StackSym ** registerSaveSyms,
                                uint registerSaveSymsCount);
    IR::Instr* GenerateBailInForGeneratorYield(IR::Instr* resumeLabelInstr, BailOutInfo* bailOutInfo);

private:
    static void SaveAllRegisters(BailOutRecord *const bailOutRecord);
public:
    static void SaveAllRegistersAndBailOut(BailOutRecord *const bailOutRecord);
    static void SaveAllRegistersAndBranchBailOut(BranchBailOutRecord *const bailOutRecord, const BOOL condition);
    static RegNum GetParamReg(IR::SymOpnd *symOpnd, Func *func);

    static uint GetRegisterSaveSlotCount() {
        return RegisterSaveSlotCount;
    }

    static uint GetRegisterSaveIndex(RegNum reg);
    static RegNum GetRegisterFromSaveIndex(uint offset);

    // All regs, including XMMs, plus extra slot space for XMMs (1 XMM = 2 Vars)
    static const uint RegisterSaveSlotCount = RegNumCount + XMM_REGCOUNT;
private:
    void        InsertOpHelperSpillsAndRestores(const OpHelperBlock& opHelperBlock);

    class GeneratorBailIn {

        // We need to rely on 2 registers `rax` and `rcx` to generate the bail-in code.
        // At this point, since `rax` already has the address of the generator's interpreter frame,
        // we can easily get the symbols' values through something like: mov dst [rax + appropriate offset]
        //
        // There are 4 types of symbols that we have to deal with:
        //  - symbols that are currently on the stack at this point. We need 2 instructions:
        //     - Load the value to `rcx`: mov rcx [rax + offset]
        //     - Finally load the value to its stack slot: mov [rbp + stack offset] rcx
        //  - symbol that is in rax
        //  - symbol that is in rcx
        //  - symbols that are in the other registers. We only need 1 instruction:
        //     - mov reg [rax + offset]
        //
        // Since restoring symbols on the stack might mess up values that will be in rax/rcx,
        // and we want to maintain the invariant that rax has to hold the value of the interpreter
        // frame, we need to restore the symbols in the following order:
        //  - symbols in stack
        //  - symbols in registers
        //  - symbol in rax
        //
        // The following 3 instructions indicate the insertion points for the above cases:
        struct BailInInsertionPoint
        {
            IR::Instr* raxRestoreInstr = nullptr;
            IR::Instr* instrInsertStackSym = nullptr;
            IR::Instr* instrInsertRegSym = nullptr;
        };

        // There are symbols that we don't need to restore such as constant values,
        // ScriptFunction's Environment (through LdEnv) and the address pointing to for-in enumerator
        // on the interpreter frame because their values are already loaded before (or as part) of the
        // generator resume jump table. In such cases, they could already be in either rax or rcx.
        // So we would need to save their values (and restore afterwards) before generating the bail-in code.
        struct SaveInitializedRegister
        {
            bool rax = false;
            bool rcx = false;
        };

        Func* const func;
        LinearScanMD* const linearScanMD;
        const JITTimeFunctionBody* const jitFnBody;
        BVSparse<JitArenaAllocator> initializedRegs;
        IR::RegOpnd* const raxRegOpnd;
        IR::RegOpnd* const rcxRegOpnd;

        bool NeedsReloadingValueWhenBailIn(StackSym* sym) const;
        uint32 GetOffsetFromInterpreterStackFrame(Js::RegSlot regSlot) const;
        IR::SymOpnd* CreateGeneratorObjectOpnd() const;

        void InsertSaveAndRestore(IR::Instr* start, IR::Instr* end, IR::RegOpnd* reg);

        void InsertRestoreRegSymbol(
            StackSym* stackSym,
            IR::Opnd* srcOpnd,
            BailInInsertionPoint& insertionPoint
        );

        void InsertRestoreStackSymbol(
            StackSym* stackSym,
            IR::Opnd* srcOpnd,
            BailInInsertionPoint& insertionPoint
        );

        void InsertRestoreSymbols(
            BVSparse<JitArenaAllocator>* symbols,
            BailInInsertionPoint& insertionPoint,
            SaveInitializedRegister& saveInitializedReg
        );

    public:
        GeneratorBailIn(Func* func, LinearScanMD* linearScanMD);
        IR::Instr* GenerateBailIn(IR::Instr* resumeLabelInstr, BailOutInfo* bailOutInfo);
    };

    GeneratorBailIn bailIn;
};
