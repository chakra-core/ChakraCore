//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "LinearScanMDShared.h"
#include "LinearScanMD.h"

#if DBG_DUMP || ENABLE_DEBUG_CONFIG_OPTIONS
extern char const * const RegNames[];
extern char16 const * const RegNamesW[];
#endif

class OpHelperBlock;
struct FillBailOutState;
struct GlobalBailOutRecordDataTable;
struct GlobalBailOutRecordDataRow;

#define INT_REG_COUNT (FIRST_FLOAT_REG - 1)
#define FLOAT_REG_COUNT (RegNumCount - FIRST_FLOAT_REG)

struct StackSlot
{
    int32 offset;
    uint32 size;
    uint32 lastUse;
};

class LoweredBasicBlock
{
public:
    JsUtil::List<Lifetime*, JitArenaAllocator>  inlineeFrameLifetimes;
    typedef JsUtil::BaseDictionary<SymID, uint, JitArenaAllocator> InlineeFrameSymsDictionary;
    InlineeFrameSymsDictionary  inlineeFrameSyms;
    JsUtil::List<Func*, JitArenaAllocator>     inlineeStack;

    LoweredBasicBlock(JitArenaAllocator* allocator) : inlineeFrameLifetimes(allocator), inlineeStack(allocator), inlineeFrameSyms(allocator)
    {
    }

    static LoweredBasicBlock* New(JitArenaAllocator * allocator);
    void Delete(JitArenaAllocator* allocator);
    void Copy(LoweredBasicBlock* block);
    LoweredBasicBlock* Clone(JitArenaAllocator * allocator);
    bool HasData();
    bool Equals(LoweredBasicBlock* block);
};

class LinearScan
{
    friend class LinearScanMDShared;
    friend class LinearScanMD;

private:
    Func *              func;
    JitArenaAllocator *    tempAlloc;
    IR::Instr *         currentInstr;
    uint32              currentBlockNumber;
    BitVector           activeRegs;
    BitVector           int32Regs;
    uint32              numInt32Regs;
    BitVector           floatRegs;
    uint32              numFloatRegs;
    BitVector           callerSavedRegs;
    BitVector           calleeSavedRegs;
    BitVector           tempRegs;
    BitVector           instrUseRegs;
    BitVector           secondChanceRegs;
    BitVector           callSetupRegs;
    SList<Lifetime *> * activeLiveranges;
    SList<Lifetime *> * lifetimeList;
    uint                intRegUsedCount;
    uint                floatRegUsedCount;
    int                 loopNest;
    uint32              m_bailOutRecordCount;
    Loop *              curLoop;
    Region *            currentRegion;
    BVSparse<JitArenaAllocator> *liveOnBackEdgeSyms;
    GlobalBailOutRecordDataTable **globalBailOutRecordTables;
    uint             ** lastUpdatedRowIndices;      // A temporary array of indices keeping track of which entry on the global table was updated for a regSlot by the last bailout point
    LinearScanMD        linearScanMD;

    SList<OpHelperBlock> *  opHelperBlockList;
    SList<OpHelperBlock>::Iterator  opHelperBlockIter;
    OpHelperBlock   *  currentOpHelperBlock;
    uint                totalOpHelperFullVisitedLength;

    BitVector           opHelperSpilledRegs;
    SList<Lifetime *> * opHelperSpilledLiveranges;
    Lifetime *          tempRegLifetimes[RegNumCount];
    Lifetime *          regContent[RegNumCount];
    IR::LabelInstr *    lastLabel;

    SList<Lifetime *> * stackPackInUseLiveRanges;
    SList<StackSlot *> *stackSlotsFreeList;
    LoweredBasicBlock  *currentBlock;
#if DBG
    BitVector           nonAllocatableRegs;
#endif
public:
    LinearScan(Func *func) : func(func), currentBlockNumber(0), loopNest(0), intRegUsedCount(0), floatRegUsedCount(0), activeLiveranges(NULL),
        linearScanMD(func), opHelperSpilledLiveranges(NULL), currentOpHelperBlock(NULL),
        lastLabel(NULL), numInt32Regs(0), numFloatRegs(0), stackPackInUseLiveRanges(NULL), stackSlotsFreeList(NULL),
        totalOpHelperFullVisitedLength(0), curLoop(NULL), currentBlock(nullptr), currentRegion(nullptr), m_bailOutRecordCount(0),
        globalBailOutRecordTables(nullptr), lastUpdatedRowIndices(nullptr), bailIn(GeneratorBailIn(func, this))
    {
    }

    void                RegAlloc();
    JitArenaAllocator *    GetTempAlloc();

    static uint8        GetRegAttribs(RegNum reg);
    static IRType       GetRegType(RegNum reg);
    static bool         IsCalleeSaved(RegNum reg);
    static uint         GetUseSpillCost(uint loopNest, BOOL isInHelperBlock);
    bool                IsCallerSaved(RegNum reg) const;
    bool                IsAllocatable(RegNum reg) const;

private:
    void                Init();
    bool                SkipNumberedInstr(IR::Instr *instr);
    void                EndDeadLifetimes(IR::Instr *instr, bool isLoopBackEdge);
    void                EndDeadOpHelperLifetimes(IR::Instr *instr);
    void                AllocateNewLifetimes(IR::Instr *instr);
    RegNum              Spill(Lifetime *newLifetime, IR::RegOpnd *regOpnd, bool dontSpillCurrent, bool force);
    void                SpillReg(RegNum reg, bool forceSpill = false);
    RegNum              SpillLiveRange(Lifetime * spilledRange, IR::Instr *insertionInstr = NULL);
    void                ProcessEHRegionBoundary(IR::Instr * instr);
    void                AllocateStackSpace(Lifetime *spilledRange);
    RegNum              FindReg(Lifetime *newLifetime, IR::RegOpnd *regOpnd, bool force = false);
    BVIndex             GetPreferencedRegIndex(Lifetime *lifetime, BitVector freeRegs);
    void                AddToActive(Lifetime *liveRange);
    void                InsertStores(Lifetime *lifetime, RegNum reg, IR::Instr *insertionInstr);
    void                InsertStore(IR::Instr *instr, StackSym *sym, RegNum reg);
    void                InsertLoads(StackSym *sym, RegNum reg);
    IR::Instr*          InsertLoad(IR::Instr *instr, StackSym *sym, RegNum reg);
    void                SetDstReg(IR::Instr *instr);
    void                SetSrcRegs(IR::Instr *instr);
    void                SetUses(IR::Instr *instr, IR::Opnd *opnd);
    void                SetUse(IR::Instr *instr, IR::RegOpnd *opnd);
    void                PrepareForUse(Lifetime * lifetime);
    void                RecordUse(Lifetime * lifetime, IR::Instr * instr, IR::RegOpnd * regOpnd, bool isFromBailout = false);
    void                RecordLoopUse(Lifetime *lifetime, RegNum reg);
    void                RecordDef(Lifetime *const lifetime, IR::Instr *const instr, const uint32 useCountCost);
    void                SetReg(IR::RegOpnd *regOpnd);
    void                KillImplicitRegs(IR::Instr *instr);
    bool                CheckIfInLoop(IR::Instr *instr);
    uint                GetSpillCost(Lifetime * lifetime);
    bool                RemoveDeadStores(IR::Instr *instr);

    // This helper function is used to save bytecode stack sym value to memory / local slots on stack so that we can read it for the locals inspection.
    void                WriteThroughForLocal(IR::RegOpnd* regOpnd, Lifetime* lifetime, IR::Instr* instrInsertAfter);
    int32               GetStackOffset(Js::RegSlot regSlotId);
    bool                IsSymNonTempLocalVar(StackSym *sym);
    bool                NeedsWriteThrough(StackSym * sym);
    bool                NeedsWriteThroughForEH(StackSym * sym);

    bool                IsInLoop() { return this->loopNest != 0; }
    bool                IsInHelperBlock() const { return this->currentOpHelperBlock != NULL; }
    uint                HelperBlockStartInstrNumber() const;
    uint                HelperBlockEndInstrNumber() const;
    void                CheckOpHelper(IR::Instr * instr);
    void                InsertOpHelperSpillAndRestores();
    void                InsertOpHelperSpillAndRestores(OpHelperBlock const& opHelperBlock);


    void                AssignActiveReg(Lifetime * lifetime, RegNum reg);
    void                AssignTempReg(Lifetime * lifetime, RegNum reg);
    RegNum              GetAssignedTempReg(Lifetime * lifetime, IRType type);

    void                AddOpHelperSpilled(Lifetime * liveRange);
    void                RemoveOpHelperSpilled(Lifetime * lifetime);
    void                SetCantOpHelperSpill(Lifetime * lifetime);

    static void         AddLiveRange(SList<Lifetime *> * list, Lifetime * liverange);
    static Lifetime *   RemoveRegLiveRange(SList<Lifetime *> * list, RegNum reg);

    GlobalBailOutRecordDataTable * EnsureGlobalBailOutRecordTable(Func *func);

    static void         SetBitVectorIfTypeSpec(StackSym * sym, Js::RegSlot regSlot, BVFixed * intSyms, BVFixed * floatSyms);
    void                FillBailOutRecord(IR::Instr * instr);
    void                FillBailOutOffset(int * offset, StackSym * stackSym, FillBailOutState * state, IR::Instr * instr);
    int                 SaveSymbolToReg(RegNum reg, FillBailOutState * state, StackSym * stackSym);
    void                FillStackLiteralBailOutRecord(IR::Instr * instr, BailOutInfo * bailOutInfo, struct FuncBailOutData * funcBailOutData, uint funcCount);
    template <typename Fn>
    void                ForEachStackLiteralBailOutInfo(IR::Instr * instr, BailOutInfo * bailOutInfo, FuncBailOutData * funcBailOutData, uint funcCount, Fn fn);
    void                ProcessSecondChanceBoundary(IR::BranchInstr *branchInstr);
    void                ProcessSecondChanceBoundaryHelper(IR::BranchInstr *branchInstr, IR::LabelInstr *branchLabel);
    void                ProcessSecondChanceBoundary(IR::LabelInstr *labelInstr);
    void                InsertSecondChanceCompensation(Lifetime ** branchRegContent, Lifetime **labelRegContent,
                                                       IR::BranchInstr *branchInstr, IR::LabelInstr *labelInstr);
    IR::Instr *         EnsureAirlock(bool needsAirlock, bool *pHasAirlock, IR::Instr *insertionInstr, IR::Instr **pInsertionStartInstr, IR::BranchInstr *branchInstr, IR::LabelInstr *labelInstr);
    bool                NeedsLoopBackEdgeCompensation(Lifetime *lifetime, IR::LabelInstr *loopTopLabel);
    void                ReconcileRegContent(Lifetime ** branchRegContent, Lifetime **labelRegContent,
                                            IR::BranchInstr *branchInstr, IR::LabelInstr *labelInstr,
                                            Lifetime *lifetime, RegNum reg, BitVector *thrashedRegs, IR::Instr *insertionInstr, IR::Instr* insertionStartInstr);
    void                AvoidCompensationConflicts(IR::LabelInstr *labelInstr, IR::BranchInstr *branchInstr,
                                            Lifetime *labelRegContent[], Lifetime *branchRegContent[],
                                            IR::Instr **pInsertionInstr, IR::Instr **pInsertionStartInstr, bool needsAirlock, bool *pHasAirlock);
    RegNum              SecondChanceAllocation(Lifetime *lifetime, bool force);
    void                SecondChanceAllocateToReg(Lifetime *lifetime, RegNum reg);
    IR::Instr *         InsertAirlock(IR::BranchInstr *branchInstr, IR::LabelInstr *labelInstr);
    void                SaveRegContent(IR::Instr *instr);
    bool                RegsAvailable(IRType type);
    void                SpillInlineeArgs(IR::Instr* instr);
    void                TrackInlineeArgLifetimes(IR::Instr* instr);

    uint                GetRemainingHelperLength(Lifetime *const lifetime);
    uint                CurrentOpHelperVisitedLength(IR::Instr *const currentInstr) const;
    IR::Instr *         TryHoistLoad(IR::Instr *instr, Lifetime *lifetime);
    bool                ClearLoopExitIfRegUnused(Lifetime *lifetime, RegNum reg, IR::BranchInstr *branchInstr, Loop *loop);

#if DBG
    void                CheckInvariants() const;
#endif
#if DBG_DUMP
    void                PrintStats() const;
#endif
#if ENABLE_DEBUG_CONFIG_OPTIONS
    IR::Instr *         GetIncInsertionPoint(IR::Instr *instr);
    void                DynamicStatsInstrument();
#endif

    void                ProcessLazyBailOut(IR::Instr *instr);

    static IR::Instr *  InsertMove(IR::Opnd *dst, IR::Opnd *src, IR::Instr *const insertBeforeInstr);
    static IR::Instr *  InsertLea(IR::RegOpnd *dst, IR::Opnd *src, IR::Instr *const insertBeforeInstr);

    class GeneratorBailIn {
        // We need to rely on 2 registers `rax` and `rcx` on x64, or `eax` and `ecx` on x86 to generate the bail-in code.
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
            IR::Instr* raxRestoreInstr;
            IR::Instr* instrInsertStackSym;
            IR::Instr* instrInsertRegSym;
        };

        // Represents a symbol that needs to be restored when bailing in.
        // In normal cases, for a given symbol, whatever bytecode registers that the
        // symbol has will map directly to the backend symbol with the same id.
        // However, due to copy-prop, sometimes we would use a different symbol for a given value.
        // This struct keep track of that fact and generate the restore instruction accordingly.
        // Additionally, for symbols that are constant but not in the bytecode constant table, we have
        // to reload the symbol's value directly.
        struct BailInSymbol
        {
            const SymID fromByteCodeRegSlot;
            const SymID toBackendId;
            const bool restoreConstDirectly : 1;
            const Js::Var constValue;
            BailInSymbol(SymID fromByteCodeRegSlot, SymID toBackendId, bool restoreConstDirectly = false, Js::Var constValue = nullptr):
                fromByteCodeRegSlot(fromByteCodeRegSlot),
                toBackendId(toBackendId),
                restoreConstDirectly(restoreConstDirectly),
                constValue(constValue) {}
        };

        Func* const func;
        LinearScan* const linearScan;
        const JITTimeFunctionBody* const jitFnBody;
        BVSparse<JitArenaAllocator> initializedRegs;
        SListBase<BailInSymbol>* bailInSymbols;

        // Registers needed in the bail-in code.
        // The register allocator will have to spill these
        // so that we are free to use them.
        static constexpr int regNum = 2;
        const RegNum regs[regNum];
        IR::RegOpnd* const interpreterFrameRegOpnd;
        IR::RegOpnd* const tempRegOpnd;

        bool NeedsReloadingSymWhenBailingIn(StackSym* sym) const;
        uint32 GetOffsetFromInterpreterStackFrame(Js::RegSlot regSlot) const;
        IR::SymOpnd* CreateGeneratorObjectOpnd() const;

        // Insert instructions to restore symbols in the `bailInSymbols` list
        void InsertRestoreSymbols(BailInInsertionPoint& insertionPoint);

        // Fill `bailInSymbols` list with all of the symbols that need to be restored
        void BuildBailInSymbolList(
            const BVSparse<JitArenaAllocator>& byteCodeUpwardExposedUses,
            const BVSparse<JitArenaAllocator>& upwardExposedUses,
            const CapturedValues& capturedValues
        );

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        void InsertBailInTrace(BVSparse<JitArenaAllocator>* symbols, IR::Instr* insertBeforeInstr);
#endif
    public:
        GeneratorBailIn(Func* func, LinearScan* linearScan);
        ~GeneratorBailIn();
        IR::Instr* GenerateBailIn(IR::GeneratorBailInInstr* bailInInstr);
        // Spill all registers that we need in order to generate the bail-in code
        void SpillRegsForBailIn();
    };

    GeneratorBailIn bailIn;
};
