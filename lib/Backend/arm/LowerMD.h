//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Lowerer;

///---------------------------------------------------------------------------
///
/// class LowererMD
///
///---------------------------------------------------------------------------

#ifdef DBG
#define INSERTDEBUGBREAK(instrInsert)\
    {\
        IR::Instr *int3 = IR::Instr::New(Js::OpCode::DEBUGBREAK, m_func);\
        instrInsert->InsertBefore(int3);\
    }
#else
#define INSERTDEBUGBREAK(instrInsert)
#endif

class LowererMD
{
public:
    static const int    MaxArgumentsToHelper = 16;

    LowererMD(Func *func) :
        m_func(func),
        helperCallArgsCount(0),
        helperCallDoubleArgsCount(0)
    {
    }

    static  bool            IsUnconditionalBranch(const IR::Instr *instr);
    static  bool            IsAssign(const IR::Instr *instr);
    static  bool            IsCall(const IR::Instr *instr);
    static  bool            IsIndirectBranch(const IR::Instr *instr);
    static  bool            IsReturnInstr(const IR::Instr *instr);
    static  void            InvertBranch(IR::BranchInstr *instr);
    static Js::OpCode       MDBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDUnsignedBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDCompareWithZeroBranchOpcode(Js::OpCode opcode);
    static Js::OpCode       MDConvertFloat64ToInt32Opcode(const RoundMode roundMode);
    static void             ChangeToAdd(IR::Instr *const instr, const bool needFlags);
    static void             ChangeToSub(IR::Instr *const instr, const bool needFlags);
    static void             ChangeToShift(IR::Instr *const instr, const bool needFlags);
    static const uint16     GetFormalParamOffset();

    static const Js::OpCode MDUncondBranchOpcode;
    static const Js::OpCode MDMultiBranchOpcode;
    static const Js::OpCode MDTestOpcode;
    static const Js::OpCode MDOrOpcode;
    static const Js::OpCode MDXorOpcode;
    static const Js::OpCode MDOverflowBranchOpcode;
    static const Js::OpCode MDNotOverflowBranchOpcode;
    static const Js::OpCode MDConvertFloat32ToFloat64Opcode;
    static const Js::OpCode MDConvertFloat64ToFloat32Opcode;
    static const Js::OpCode MDCallOpcode;
    static const Js::OpCode MDImulOpcode;
    static const Js::OpCode MDLea;

public:
            void            Init(Lowerer *lowerer);
            void            FinalLower();
            bool            FinalLowerAssign(IR::Instr* instr);
            IR::Opnd *      GenerateMemRef(intptr_t addr, IRType type, IR::Instr *instr, bool dontEncode = false);
            IR::Instr *     ChangeToHelperCall(IR::Instr * instr, IR::JnHelperMethod helperMethod, IR::LabelInstr *labelBailOut = nullptr,
                                               IR::Opnd *opndInstance = nullptr, IR::PropertySymOpnd * propSymOpnd = nullptr, bool isHelperContinuation = false);
            IR::Instr *     ChangeToHelperCallMem(IR::Instr * instr, IR::JnHelperMethod helperMethod);
    static  IR::Instr *     ChangeToAssign(IR::Instr * instr);
    static  IR::Instr *     ChangeToAssignNoBarrierCheck(IR::Instr * instr);
    static  IR::Instr *     ChangeToAssign(IR::Instr * instr, IRType type);
    static  IR::Instr *     ForceDstToReg(IR::Instr *instr);
    static  void            ImmedSrcToReg(IR::Instr * instr, IR::Opnd * newOpnd, int srcNum);

            IR::Instr *     LoadArgumentCount(IR::Instr * instr);
            IR::Instr *     LoadStackArgPtr(IR::Instr * instr);
            IR::Instr *     LoadHeapArguments(IR::Instr * instrArgs);
            IR::Instr *     LoadHeapArgsCached(IR::Instr * instr);
            IR::Instr *     LoadInputParamPtr(IR::Instr * instrInsert, IR::RegOpnd * optionalDstOpnd = nullptr);
            IR::Instr *     LoadInputParamCount(IR::Instr * instr, int adjust = 0, bool needFlags = false);
            IR::Instr *     LoadArgumentsFromFrame(IR::Instr * instr);
            IR::Instr *     LowerRet(IR::Instr * instr);
            IR::Instr *     LowerCondBranch(IR::Instr * instr);
            IR::Instr *     LoadFunctionObjectOpnd(IR::Instr *instr, IR::Opnd *&functionObjOpnd);
            IR::Instr *     GenerateSmIntPairTest(IR::Instr * instrInsert, IR::Opnd * opndSrc1, IR::Opnd * opndSrc2, IR::LabelInstr * labelFail);
#if DBG
    static  void            GenerateDebugBreak(IR::Instr * insertInstr);
#endif
            void            GenerateTaggedZeroTest( IR::Opnd * opndSrc, IR::Instr * instrInsert, IR::LabelInstr * labelHelper = nullptr);
            void            GenerateObjectPairTest(IR::Opnd * opndSrc1, IR::Opnd * opndSrc2, IR::Instr * insertInstr, IR::LabelInstr * labelTarget);
            bool            GenerateObjectTest(IR::Opnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr * labelTarget, bool fContinueLabel = false);
            bool            GenerateFastCmSrEqConst(IR::Instr *instr);
            void            GenerateFastCmXxI4(IR::Instr *instr);
            void            GenerateFastCmXxR8(IR::Instr *instr);
            void            GenerateFastCmXx(IR::Instr *instr);
            bool            GenerateFastCmXxTaggedInt(IR::Instr *instr, bool isInHelper = false);
            IR::Instr *     GenerateConvBool(IR::Instr *instr);
            void            GenerateClz(IR::Instr * instr);
            void            GenerateCtz(IR::Instr * instr) { Assert(UNREACHED); }
            void            GeneratePopCnt(IR::Instr * instr) { Assert(UNREACHED); }
            template <bool Saturate>
            void            GenerateTruncWithCheck(IR::Instr * instr) { Assert(UNREACHED); }
            void            GenerateFastDivByPow2(IR::Instr *instr);
            bool            GenerateFastAdd(IR::Instr * instrAdd);
            bool            GenerateFastSub(IR::Instr * instrSub);
            bool            GenerateFastMul(IR::Instr * instrMul);
            bool            GenerateFastAnd(IR::Instr * instrAnd);
            bool            GenerateFastXor(IR::Instr * instrXor);
            bool            GenerateFastOr(IR::Instr * instrOr);
            bool            GenerateFastNot(IR::Instr * instrNot);
            bool            GenerateFastNeg(IR::Instr * instrNeg);
            bool            GenerateFastDivAndRem(IR::Instr* instrDiv, IR::LabelInstr* bailOutLabel = false);
            bool            GenerateFastShiftLeft(IR::Instr * instrShift);
            bool            GenerateFastShiftRight(IR::Instr * instrShift);
            void            GenerateFastBrS(IR::BranchInstr *brInstr);
            void            GenerateFastInlineBuiltInCall(IR::Instr* instr, IR::JnHelperMethod helperMethod);
            void            HelperCallForAsmMathBuiltin(IR::Instr* instr, IR::JnHelperMethod helperMethodFloat, IR::JnHelperMethod helperMethodDouble) { Assert(UNREACHED); } // only for asm.js
            IR::Opnd *      CreateStackArgumentsSlotOpnd();
            void            GenerateSmIntTest(IR::Opnd *opndSrc, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::Instr **instrFirst = nullptr, bool fContinueLabel = false);
            IR::RegOpnd *   LoadNonnegativeIndex(IR::RegOpnd *indexOpnd, const bool skipNegativeCheck, IR::LabelInstr *const notTaggedIntLabel, IR::LabelInstr *const negativeLabel, IR::Instr *const insertBeforeInstr);
            IR::RegOpnd *   GenerateUntagVar(IR::RegOpnd * opnd, IR::LabelInstr * labelFail, IR::Instr * insertBeforeInstr, bool generateTagCheck = true);
            void            GenerateInt32ToVarConversion( IR::Opnd * opndSrc, IR::Instr * insertInstr );
            IR::Instr *     GenerateFastScopedFld(IR::Instr * instrScopedFld, bool isLoad);
            IR::Instr *     GenerateFastScopedLdFld(IR::Instr * instrLdFld);
            IR::Instr *     GenerateFastScopedStFld(IR::Instr * instrStFld);
            void            GenerateFastAbs(IR::Opnd *dst, IR::Opnd *src, IR::Instr *callInstr, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel);
            bool            TryGenerateFastMulAdd(IR::Instr * instrAdd, IR::Instr ** pInstrPrev);
            void            GenerateFloatTest(IR::RegOpnd * opndSrc, IR::Instr * insertInstr, IR::LabelInstr* labelHelper, const bool checkForNullInLoopBody = false);

     static void            EmitInt4Instr(IR::Instr *instr);
            void            EmitLoadVar(IR::Instr *instr, bool isFromUint32 = false, bool isHelper = false);
            bool            EmitLoadInt32(IR::Instr *instr, bool conversionFromObjectAllowed, bool bailOutOnHelper = false, IR::LabelInstr * labelBailOut = nullptr);

     static void            LowerInt4NegWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
     static void            LowerInt4AddWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
     static void            LowerInt4SubWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
     static void            LowerInt4MulWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel);
            void            LowerInt4RemWithBailOut(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel) const;
            void            GenerateNumberAllocation(IR::RegOpnd * opndDst, IR::Instr * instrInsert, bool isHelper);
            void            GenerateFastRecyclerAlloc(size_t allocSize, IR::RegOpnd* newObjDst, IR::Instr* insertionPointInstr, IR::LabelInstr* allocHelperLabel, IR::LabelInstr* allocDoneLabel);
            void            SaveDoubleToVar(IR::RegOpnd * dstOpnd, IR::RegOpnd *opndFloat, IR::Instr *instrOrig, IR::Instr *instrInsert, bool isHelper = false);
            void            EmitLoadFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, IR::Instr * instrBailOut = nullptr, IR::LabelInstr * labelBailOut = nullptr);
            IR::Instr *     LoadCheckedFloat(IR::RegOpnd *opndOrig, IR::RegOpnd *opndFloat, IR::LabelInstr *labelInline, IR::LabelInstr *labelHelper, IR::Instr *instrInsert, const bool checkForNullInLoopBody = false);

            void LoadFloatValue(IR::RegOpnd * javascriptNumber, IR::RegOpnd * opndFloat, IR::LabelInstr * labelHelper, IR::Instr * instrInsert, const bool checkFornullptrInLoopBody = false);

            IR::Instr *     LowerCallHelper(IR::Instr *instrCall);

            IR::LabelInstr *GetBailOutStackRestoreLabel(BailOutInfo * bailOutInfo, IR::LabelInstr * exitTargetInstr);
            StackSym *      GetImplicitParamSlotSym(Js::ArgSlot argSlot);
     static StackSym *      GetImplicitParamSlotSym(Js::ArgSlot argSlot, Func * func);

            IR::Instr *     LowerDivI4AndBailOnReminder(IR::Instr * instr, IR::LabelInstr * bailOutLabel);
public:
            IR::Instr *         LowerCall(IR::Instr * callInstr, Js::ArgSlot argCount);
            IR::Instr *         LowerCallI(IR::Instr * callInstr, ushort callFlags, bool isHelper = false, IR::Instr* insertBeforeInstrForCFG = nullptr);
            int32               LowerCallArgs(IR::Instr * callInstr, IR::Instr * stackParamInsert, ushort callFlags, Js::ArgSlot extraParams = 1 /* for function object */, IR::IntConstOpnd **callInfoOpndRef = nullptr);
            int32               LowerCallArgs(IR::Instr * callInstr, ushort callFlags, Js::ArgSlot extraParams = 1 /* for function object */, IR::IntConstOpnd **callInfoOpndRef = nullptr) { return LowerCallArgs(callInstr, callInstr, callFlags, extraParams, callInfoOpndRef); }
            IR::Instr *         LowerStartCall(IR::Instr * instr);
            IR::Instr *         LowerAsmJsCallI(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
            IR::Instr *         LowerAsmJsCallE(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
            IR::Instr *         LowerWasmArrayBoundsCheck(IR::Instr * instr, IR::Opnd *addrOpnd) { Assert(UNREACHED); return nullptr; }
            void                LowerAtomicStore(IR::Opnd * dst, IR::Opnd * src1, IR::Instr * insertBeforeInstr) { Assert(UNREACHED); }
            void                LowerAtomicLoad(IR::Opnd* dst, IR::Opnd* src1, IR::Instr* insertBeforeInstr) { Assert(UNREACHED); }
            IR::Instr *         LowerAsmJsStElemHelper(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
            IR::Instr *         LowerAsmJsLdElemHelper(IR::Instr * callInstr) { Assert(UNREACHED); return nullptr; }
            IR::Instr *         LowerCallIDynamic(IR::Instr *callInstr, IR::Instr*saveThisArgOutInstr, IR::Opnd *argsLength, ushort callFlags, IR::Instr * insertBeforeInstrForCFG = nullptr);
            IR::Instr *         LoadHelperArgument(IR::Instr * instr, IR::Opnd * opndArg);
            IR::Instr *         LoadDynamicArgument(IR::Instr * instr, uint argNumber = 1);
            IR::Instr *         LoadDynamicArgumentUsingLength(IR::Instr *instr);
            IR::Instr *         LoadDoubleHelperArgument(IR::Instr * instr, IR::Opnd * opndArg);
            IR::Instr *         LoadFloatHelperArgument(IR::Instr * instr, IR::Opnd * opndArg) { Assert(UNREACHED); return nullptr; } // only used for asm.js right now
            IR::Instr *         LowerToFloat(IR::Instr *instr);
     static IR::BranchInstr *   LowerFloatCondBranch(IR::BranchInstr *instrBranch, bool ignoreNaN = false);
            void                ConvertFloatToInt32(IR::Opnd* intOpnd, IR::Opnd* floatOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone, IR::Instr * instInsert);
            void                CheckOverflowOnFloatToInt32(IR::Instr* instr, IR::Opnd* intOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone);
            void                EmitLoadVarNoCheck(IR::RegOpnd * dst, IR::RegOpnd * src, IR::Instr *instrLoad, bool isFromUint32, bool isHelper);
            void                EmitIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void                EmitUIntToFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void                EmitFloatToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert, IR::Instr * instrBailOut = nullptr, IR::LabelInstr * labelBailOut = nullptr);
            void                EmitFloat32ToFloat64(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert) { Assert(UNREACHED); }
            void                EmitInt64toFloat(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert) {  Assert(UNREACHED); }
            static IR::Instr *  InsertConvertFloat64ToInt32(const RoundMode roundMode, IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr);
            void                EmitIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void                EmitUIntToLong(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void                EmitLongToInt(IR::Opnd *dst, IR::Opnd *src, IR::Instr *instrInsert);
            void                EmitSignExtend(IR::Instr * instr) { Assert(UNREACHED); }
            void                EmitReinterpretPrimitive(IR::Opnd* dst, IR::Opnd* src, IR::Instr* insertBeforeInstr) { Assert(UNREACHED); }
            void                EmitLoadFloatFromNumber(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr);
            IR::LabelInstr*     EmitLoadFloatCommon(IR::Opnd *dst, IR::Opnd *src, IR::Instr *insertInstr, bool needHelperLabel);
            static IR::Instr *  LoadFloatZero(IR::Opnd * opndDst, IR::Instr * instrInsert);
            static IR::Instr *  LoadFloatValue(IR::Opnd * opndDst, double value, IR::Instr * instrInsert);

            IR::Instr *         LowerEntryInstr(IR::EntryInstr * entryInstr);
            IR::Instr *         LowerExitInstr(IR::ExitInstr * exitInstr);
            IR::Instr *         LowerExitInstrAsmJs(IR::ExitInstr * exitInstr) { Assert(UNREACHED); return nullptr; }
            IR::Instr *         LoadNewScObjFirstArg(IR::Instr * instr, IR::Opnd * dst, ushort extraArgs = 0);
            IR::Instr *         LowerTry(IR::Instr *instr, IR::JnHelperMethod helperMethod);
            IR::Instr *         LowerLeaveNull(IR::Instr *instr);
            IR::LabelInstr *    EnsureEpilogLabel();
            IR::Instr *         LowerEHRegionReturn(IR::Instr * insertBeforeInstr, IR::Opnd * targetOpnd);
            void                FinishArgLowering();
            IR::Opnd *          GetOpndForArgSlot(Js::ArgSlot argSlot, IR::Opnd * argOpnd = nullptr);
            bool                GenerateStackAllocation(IR::Instr *instr, uint32 allocSize, uint32 probeSize);
            void                GenerateStackDeallocation(IR::Instr *instr, uint32 allocSize);
            void                GenerateStackProbe(IR::Instr *instr, bool afterProlog);
            IR::Opnd*           GenerateArgOutForStackArgs(IR::Instr* callInstr, IR::Instr* stackArgsInstr);

            template <bool verify = false>
            static void         Legalize(IR::Instr *const instr);

            void                GenerateFastInlineBuiltInMathAbs(IR::Instr *callInstr);
            void                GenerateFastInlineBuiltInMathFloor(IR::Instr *callInstr);
            void                GenerateFastInlineBuiltInMathCeil(IR::Instr *callInstr);
            void                GenerateFastInlineBuiltInMathRound(IR::Instr *callInstr);
            static RegNum       GetRegStackPointer() { return RegSP; }
            static RegNum       GetRegFramePointer() { return RegR11; }
            static RegNum       GetRegReturn(IRType type) { return IRType_IsFloat(type) ? RegNOREG : RegR0; }
            static RegNum       GetRegArgI4(int32 argNum) { return RegNOREG; }
            static RegNum       GetRegArgR8(int32 argNum) { return RegNOREG; }
            static Js::OpCode   GetLoadOp(IRType type) { return type == TyFloat64? Js::OpCode::VLDR : ((type == TyFloat32)? Js::OpCode::VLDR32 : Js::OpCode::LDR); }
            static Js::OpCode   GetStoreOp(IRType type) { return type == TyFloat64? Js::OpCode::VSTR : ((type == TyFloat32)? Js::OpCode::VSTR32 : Js::OpCode::STR); }
            static Js::OpCode   GetMoveOp(IRType type) { return IRType_IsFloat(type) ? Js::OpCode::VMOV : Js::OpCode::MOV; }

            static BYTE         GetDefaultIndirScale()
            {
                return IndirScale4;
            }

            // -4 is to avoid alignment issues popping up, we are conservative here.
            // We might check for IsSmallStack first to push R4 register & then align.
            static bool         IsSmallStack(uint32 size)   { return (size < (PAGESIZE - 4));}

            static void GenerateLoadTaggedType(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndTaggedType);
            static void GenerateLoadPolymorphicInlineCacheSlot(IR::Instr * instrLdSt, IR::RegOpnd * opndInlineCache, IR::RegOpnd * opndType, uint polymorphicInlineCacheSize);
            static void GenerateStFldFromLocalInlineCache(IR::Instr * instrStFld, IR::RegOpnd * opndBase, IR::Opnd * opndSrc, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFallThru, bool isInlineSlot);
            void        GenerateFunctionObjectTest(IR::Instr * callInstr, IR::RegOpnd  *functionOpnd, bool isHelper, IR::LabelInstr* continueAfterExLabel = nullptr);

            static IR::Instr * ChangeToWriteBarrierAssign(IR::Instr * assignInstr, const Func* func);

            int                 GetHelperArgsCount() { return this->helperCallArgsCount; }
            void                ResetHelperArgsCount() { this->helperCallArgsCount = 0; }

            void                LowerInlineSpreadArgOutLoop(IR::Instr *callInstr, IR::RegOpnd *indexOpnd, IR::RegOpnd *arrayElementsStartOpnd);
            void                LowerTypeof(IR::Instr * typeOfInstr);
            void                GenerateMemInit(IR::RegOpnd * opnd, int32 offset, size_t value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
            IR::BranchInstr*    InsertMissingItemCompareBranch(IR::Opnd* compareSrc, IR::Opnd* missingItemOpnd, Js::OpCode opcode, IR::LabelInstr* target, IR::Instr* insertBeforeInstr);
private:
    IR::Opnd* IsOpndNegZero(IR::Opnd* opnd, IR::Instr* instr);

    void GenerateAssignForBuiltinArg(
        RegNum dstReg,
        IR::Opnd* srcOpnd,
        IR::Instr* instr);

    IR::Instr*  GeneratePreCall(IR::Instr * callInstr, IR::Opnd  *functionOpnd);
    void        SetMaxArgSlots(Js::ArgSlot actualCount /*including this*/);
// Data
protected:
    Func *          m_func;
    Lowerer *       m_lowerer;

    //
    // Support to load helper arguments.
    //
    static const int    MaxDoubleArgumentsToHelper = 8;
    // Only 8 double values can be passed through double registers, rest has to go through stack and
    // need to following a different calling convention. We should never hit that case as there is no helper call with more
    // than 8 double arguments.

    uint16              helperCallArgsCount;        //consists of both integer & double arguments
    uint16              helperCallDoubleArgsCount;  //consists of only double arguments
    IR::Opnd *          helperCallArgs[MaxArgumentsToHelper];

    void                FlipHelperCallArgsOrder();
};
