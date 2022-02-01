//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define ASSERT_INLINEE_FUNC(instr) Assert(instr->m_func->IsInlinee() ? (instr->m_func != this->m_func) : (instr->m_func == this->m_func))

enum IndirScale : BYTE {
    IndirScale1 = 0,
    IndirScale2 = 1,
    IndirScale4 = 2,
    IndirScale8 = 3
};

enum RoundMode : BYTE {
    RoundModeTowardZero = 0,
    RoundModeTowardInteger = 1,
    RoundModeHalfToEven = 2
};

#if DBG
enum HelperCallCheckState : BYTE {
    HelperCallCheckState_None = 0,
    HelperCallCheckState_ImplicitCallsBailout = 1 << 0,
    HelperCallCheckState_NoHelperCalls = 1 << 1
};

#endif
#if defined(_M_IX86) || defined(_M_AMD64)
#include "LowerMDShared.h"
#elif defined(_M_ARM32_OR_ARM64)
#include "LowerMD.h"
#endif

#define IR_HELPER_OP_FULL_OR_INPLACE(op) IR::HelperOp_##op##_Full, IR::HelperOp_##op##InPlace

///---------------------------------------------------------------------------
///
/// class Lowerer
///
///     Lower machine independent IR to machine dependent instrs.
///
///---------------------------------------------------------------------------

class Lowerer
{
    friend class LowererMD;
    friend class LowererMDArch;
    friend class Encoder;
    friend class Func;
    friend class ExternalLowerer;

public:
    Lowerer(Func * func) : m_func(func), m_lowererMD(func), nextStackFunctionOpnd(nullptr), outerMostLoopLabel(nullptr),
        initializedTempSym(nullptr), addToLiveOnBackEdgeSyms(nullptr), currentRegion(nullptr),
        m_lowerGeneratorHelper(LowerGeneratorHelper(func, this, this->m_lowererMD))
    {
#ifdef RECYCLER_WRITE_BARRIER_JIT
        m_func->m_lowerer = this;
#endif
#if DBG
        this->helperCallCheckState = HelperCallCheckState_None;
        this->oldHelperCallCheckState = HelperCallCheckState_None;
#endif
        
    }
    ~Lowerer()
    {
#ifdef RECYCLER_WRITE_BARRIER_JIT
        m_func->m_lowerer = nullptr;
#endif
    }

    void Lower();
    void LowerRange(IR::Instr *instrStart, IR::Instr *instrEnd, bool defaultDoFastPath, bool defaultDoLoopFastPath);
    void LowerPrologEpilog();
    void LowerPrologEpilogAsmJs();

    void DoInterruptProbes();

    static IR::Instr * TryShiftAdd(IR::Instr *instrAdd, IR::Opnd * opndFold, IR::Opnd * opndAdd);
    static IR::Instr *PeepShiftAdd(IR::Instr *instr);

    IR::Instr *PreLowerPeepInstr(IR::Instr *instr, IR::Instr **pInstrPrev);
    IR::Instr *PeepShl(IR::Instr *instr);
    IR::Instr *PeepBrBool(IR::Instr *instr);
    uint DoLoopProbeAndNumber(IR::BranchInstr *branchInstr);
    void InsertOneLoopProbe(IR::Instr *insertInstr, IR::LabelInstr *loopLabel);
    void FinalLower();
    void InsertLazyBailOutThunk();
    void EHBailoutPatchUp();
    inline Js::ScriptContext* GetScriptContext()
    {
        return m_func->GetScriptContext();
    }

    StackSym *      GetTempNumberSym(IR::Opnd * opnd, bool isTempTransferred);
    static bool     HasSideEffects(IR::Instr *instr);
    static bool     IsArgSaveRequired(Func *func);

#if DBG
    static bool     ValidOpcodeAfterLower(IR::Instr* instr, Func * func);
#endif
private:
    IR::Instr *     LowerNewRegEx(IR::Instr * instr);
    void            LowerNewScObjectSimple(IR::Instr *instr);
    void            LowerNewScObjectLiteral(IR::Instr *instr);
    IR::Instr *     LowerInitCachedFuncs(IR::Instr *instrInit);

    IR::Instr *     LowerNewScObject(IR::Instr *instr, bool callCtor, bool hasArgs, bool isBaseClassConstructorNewScObject = false);
    IR::Instr *     LowerNewScObjArray(IR::Instr *instr);
    IR::Instr *     LowerNewScObjArrayNoArg(IR::Instr *instr);
    bool            TryLowerNewScObjectWithFixedCtorCache(IR::Instr* newObjInstr, IR::RegOpnd* newObjDst, IR::LabelInstr* helperOrBailoutLabel, IR::LabelInstr* callCtorLabel,
                        bool& skipNewScObj, bool& returnNewScObj, bool& emitHelper);
    void            GenerateRecyclerAllocAligned(IR::JnHelperMethod allocHelper, size_t allocSize, IR::RegOpnd* newObjDst, IR::Instr* insertionPointInstr, bool inOpHelper = false);
    IR::Instr *     LowerGetNewScObject(IR::Instr *const instr);
    void            LowerGetNewScObjectCommon(IR::RegOpnd *const resultObjOpnd, IR::RegOpnd *const constructorReturnOpnd, IR::RegOpnd *const newObjOpnd, IR::Instr *insertBeforeInstr);
    IR::Instr *     LowerUpdateNewScObjectCache(IR::Instr * updateInstr, IR::Opnd *dst, IR::Opnd *src1, const bool isCtorFunction);
    bool            GenerateLdFldWithCachedType(IR::Instr * instrLdFld, bool* continueAsHelperOut, IR::LabelInstr** labelHelperOut, IR::RegOpnd** typeOpndOut);
    bool            GenerateCheckFixedFld(IR::Instr * instrChkFld);
    void            GenerateCheckObjType(IR::Instr * instrChkObjType);
    void            LowerAdjustObjType(IR::Instr * instrAdjustObjType);
    bool            GenerateNonConfigurableLdRootFld(IR::Instr * instrLdFld);
    IR::Instr *     LowerProfiledLdFld(IR::JitProfilingInstr *instr);
    void            LowerProfiledBeginSwitch(IR::JitProfilingInstr *instr);
    void            LowerFunctionExit(IR::Instr* funcExit);
    void            LowerFunctionEntry(IR::Instr* funcEntry);
    void            LowerFunctionBodyCallCountChange(IR::Instr *const insertBeforeInstr);
    IR::Instr*      LowerProfiledNewArray(IR::JitProfilingInstr* instr, bool hasArgs);
    IR::Instr *     LowerProfiledLdSlot(IR::JitProfilingInstr *instr);
    void            LowerProfileLdSlot(IR::Opnd *const valueOpnd, Func *const ldSlotFunc, const Js::ProfileId profileId, IR::Instr *const insertBeforeInstr);
    void            LowerProfiledBinaryOp(IR::JitProfilingInstr* instr, IR::JnHelperMethod meth);
    IR::Instr *     LowerLdFld(IR::Instr *instr, IR::JnHelperMethod helperMethod, IR::JnHelperMethod polymorphicHelperMethod, bool useInlineCache, IR::LabelInstr *labelBailOut = nullptr, bool isHelper = false);
    template<bool isRoot>
    IR::Instr*      GenerateCompleteLdFld(IR::Instr* instr, bool emitFastPath, IR::JnHelperMethod monoHelperAfterFastPath, IR::JnHelperMethod polyHelperAfterFastPath,
                        IR::JnHelperMethod monoHelperWithoutFastPath, IR::JnHelperMethod polyHelperWithoutFastPath);
    IR::Instr *     LowerScopedLdFld(IR::Instr *instr, IR::JnHelperMethod helperMethod, bool withInlineCache);
    IR::Instr *     LowerScopedLdInst(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerDelFld(IR::Instr *instr, IR::JnHelperMethod helperMethod, bool useInlineCache, bool strictMode);
    IR::Instr *     LowerIsInst(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerScopedDelFld(IR::Instr *instr, IR::JnHelperMethod helperMethod, bool withInlineCache, bool strictMode);
    IR::Instr *     LowerNewScFunc(IR::Instr *instr);
    IR::Instr *     LowerNewScGenFunc(IR::Instr *instr);
    IR::Instr *     LowerNewScFuncHomeObj(IR::Instr *instr);
    IR::Instr *     LowerNewScGenFuncHomeObj(IR::Instr *instr);
    IR::Instr *     LowerStPropIdArrFromVar(IR::Instr *instr);
    IR::Instr *     LowerRestify(IR::Instr *instr);
    IR::Instr*      GenerateCompleteStFld(IR::Instr* instr, bool emitFastPath, IR::JnHelperMethod monoHelperAfterFastPath, IR::JnHelperMethod polyHelperAfterFastPath,
                        IR::JnHelperMethod monoHelperWithoutFastPath, IR::JnHelperMethod polyHelperWithoutFastPath, bool withPutFlags, Js::PropertyOperationFlags flags);
    bool            GenerateStFldWithCachedType(IR::Instr * instrStFld, bool* continueAsHelperOut, IR::LabelInstr** labelHelperOut, IR::RegOpnd** typeOpndOut);
    bool            GenerateStFldWithCachedFinalType(IR::Instr * instrStFld, IR::PropertySymOpnd *propertySymOpnd);
    IR::RegOpnd *   GenerateCachedTypeCheck(IR::Instr *instrInsert, IR::PropertySymOpnd *propertySymOpnd,
                        IR::LabelInstr* labelObjCheckFailed, IR::LabelInstr *labelTypeCheckFailed, IR::LabelInstr *labelSecondChance = nullptr);
    void            GenerateCachedTypeWithoutPropertyCheck(IR::Instr *instrInsert, IR::PropertySymOpnd *propertySymOpnd, IR::Opnd *typeOpnd, IR::LabelInstr *labelTypeCheckFailed);
    IR::RegOpnd *   GeneratePolymorphicTypeIndex(IR::RegOpnd * typeOpnd, Js::PropertyGuard * typeCheckGuard, IR::Instr * instrInsert);
    void            GenerateLeaOfOOPData(IR::RegOpnd * regOpnd, void * address, int32 offset, IR::Instr * instrInsert);
    IR::Opnd *      GenerateIndirOfOOPData(void * address, int32 offset, IR::Instr * instrInsert);
    bool            GenerateFixedFieldGuardCheck(IR::Instr *insertPointInstr, IR::PropertySymOpnd *propertySymOpnd, IR::LabelInstr *labelBailOut);
    Js::JitTypePropertyGuard* CreateTypePropertyGuardForGuardedProperties(JITTypeHolder type, IR::PropertySymOpnd* propertySymOpnd);
    Js::JitEquivalentTypeGuard* CreateEquivalentTypeGuardAndLinkToGuardedProperties(IR::PropertySymOpnd* propertySymOpnd);
    bool            LinkCtorCacheToGuardedProperties(JITTimeConstructorCache* cache);
    template<typename LinkFunc>
    bool            LinkGuardToGuardedProperties(const BVSparse<JitArenaAllocator>* guardedPropOps, LinkFunc link);
    bool            GeneratePropertyGuardCheck(IR::Instr *insertPointInstr, IR::PropertySymOpnd *propertySymOpnd, IR::LabelInstr *labelBailOut);
    IR::Instr *     GeneratePropertyGuardCheckBailoutAndLoadType(IR::Instr *insertInstr);
    void            GenerateFieldStoreWithTypeChange(IR::Instr * instrStFld, IR::PropertySymOpnd *propertySymOpnd, JITTypeHolder initialType, JITTypeHolder finalType);
    void            GenerateDirectFieldStore(IR::Instr* instrStFld, IR::PropertySymOpnd* propertySymOpnd);
    void            GenerateAdjustSlots(IR::Instr * instrStFld, IR::PropertySymOpnd *propertySymOpnd, JITTypeHolder initialType, JITTypeHolder finalType);
    bool            GenerateAdjustBaseSlots(IR::Instr * instrStFld, IR::RegOpnd *baseOpnd, JITTypeHolder initialType, JITTypeHolder finalType);
    void            PinTypeRef(JITTypeHolder type, void* typeRef, IR::Instr* instr, Js::PropertyId propertyId);
    IR::RegOpnd *   GenerateIsBuiltinRecyclableObject(IR::RegOpnd *regOpnd, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, bool checkObjectAndDynamicObject = true, IR::LabelInstr *labelFastExternal = nullptr, bool isInHelper = false);
    void            GenerateIsDynamicObject(IR::RegOpnd *regOpnd, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, bool fContinueLabel = false);
    void            GenerateIsRecyclableObject(IR::RegOpnd *regOpnd, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, bool checkObjectAndDynamicObject = true);
    bool            GenerateLdThisCheck(IR::Instr * instr);
    bool            GenerateFastIsInst(IR::Instr * instr);
    void            GenerateFastArrayIsIn(IR::Instr * instr);
    void            GenerateFastObjectIsIn(IR::Instr * instr);

    void GenerateProtoLdFldFromFlagInlineCache(
        IR::Instr * insertBeforeInstr,
        IR::Opnd * opndDst,
        IR::RegOpnd * opndInlineCache,
        IR::LabelInstr * labelFallThru,
        bool isInlineSlot);
    void GenerateLocalLdFldFromFlagInlineCache(
        IR::Instr * insertBeforeInstr,
        IR::RegOpnd * opndBase,
        IR::Opnd * opndDst,
        IR::RegOpnd * opndInlineCache,
        IR::LabelInstr * labelFallThru,
        bool isInlineSlot);

    IR::LabelInstr* EnsureEpilogueLabel() const;

    void            GenerateFlagProtoCheck(IR::Instr * insertBeforeInstr, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFail);
    void            GenerateFlagInlineCacheCheck(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelNext);
    bool            GenerateFastLdMethodFromFlags(IR::Instr * instrLdFld);

    void            EnsureStackFunctionListStackSym();
    void            EnsureZeroLastStackFunctionNext();
    void            AllocStackClosure();
    IR::Instr *     GenerateNewStackScFunc(IR::Instr * newScFuncInstr, IR::RegOpnd ** ppEnvOpnd);
    void            GenerateStackScriptFunctionInit(StackSym * stackSym, Js::FunctionInfoPtrPtr nestedInfo);
    void            GenerateScriptFunctionInit(IR::RegOpnd * regOpnd, IR::Opnd * vtableAddressOpnd,
                        Js::FunctionInfoPtrPtr nestedInfo, IR::Opnd * envOpnd, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateStackScriptFunctionInit(IR::RegOpnd * regOpnd, Js::FunctionInfoPtrPtr nestedInfo, IR::Opnd * envOpnd, IR::Instr * insertBeforeInstr);
    IR::Instr *     LowerProfiledStFld(IR::JitProfilingInstr * instr, Js::PropertyOperationFlags flags);
    IR::Instr *     LowerStFld(IR::Instr * stFldInstr, IR::JnHelperMethod helperMethod, IR::JnHelperMethod polymorphicHelperMethod, bool withInlineCache, IR::LabelInstr *ppBailOutLabel = nullptr, bool isHelper = false, bool withPutFlags = false, Js::PropertyOperationFlags flags = Js::PropertyOperation_None);
    void            MapStFldHelper(IR::PropertySymOpnd * propertySymOpnd, IR::JnHelperMethod &helperMethod, IR::JnHelperMethod &polymorphicHelperMethod);
    IR::Instr *     LowerScopedStFld(IR::Instr * stFldInstr, IR::JnHelperMethod helperMethod, bool withInlineCache,
                                bool withPropertyOperationFlags = false, Js::PropertyOperationFlags flags = Js::PropertyOperation_None);
    void            LowerProfiledLdElemI(IR::JitProfilingInstr *const instr);
    void            LowerProfiledStElemI(IR::JitProfilingInstr *const instr, const Js::PropertyOperationFlags flags);
    IR::Instr *     LowerStElemI(IR::Instr *instr, Js::PropertyOperationFlags flags, bool isHelper, IR::JnHelperMethod helperMethod = IR::HelperOp_SetElementI);
    IR::Instr *     LowerLdElemI(IR::Instr *instr, IR::JnHelperMethod helperMethod, bool isHelper);
    void            LowerLdLen(IR::Instr *const instr, const bool isHelper);

    IR::Instr *     LowerMemOp(IR::Instr * instr);
    IR::Instr *     LowerMemset(IR::Instr * instr, IR::RegOpnd * helperRet);
    IR::Instr *     LowerMemcopy(IR::Instr * instr, IR::RegOpnd * helperRet);

    IR::Instr *     LowerWasmArrayBoundsCheck(IR::Instr * instr, IR::Opnd *addrOpnd);
    IR::Instr *     LowerLdArrViewElem(IR::Instr * instr);
    IR::Instr *     LowerStArrViewElem(IR::Instr * instr);
    IR::Instr *     LowerStAtomicsWasm(IR::Instr * instr);
    IR::Instr *     LowerLdAtomicsWasm(IR::Instr * instr);
    IR::Instr *     LowerLdArrViewElemWasm(IR::Instr * instr);
    IR::Instr *     LowerArrayDetachedCheck(IR::Instr * instr);
    IR::Instr *     LowerDeleteElemI(IR::Instr *instr, bool strictMode);
    IR::Instr *     LowerStElemC(IR::Instr *instr);
    void            LowerLdArrHead(IR::Instr *instr);
    IR::Instr*      AddSlotArrayCheck(PropertySym *propertySym, IR::Instr* instr);
    IR::Instr *     LowerStSlot(IR::Instr *instr);
    IR::Instr *     LowerStSlotChkUndecl(IR::Instr *instr);
    void            LowerStLoopBodyCount(IR::Instr* instr);
#if !FLOATVAR
    IR::Instr *     LowerStSlotBoxTemp(IR::Instr *instr);
#endif
    void            LowerLdSlot(IR::Instr *instr);
    IR::Instr *     LowerChkUndecl(IR::Instr *instr);
    void            GenUndeclChk(IR::Instr *insertInsert, IR::Opnd *opnd);
    IR::Instr *     LoadPropertySymAsArgument(IR::Instr *instr, IR::Opnd *fieldSrc);
    IR::Instr *     LoadFunctionBodyAsArgument(IR::Instr *instr, IR::IntConstOpnd * functionBodySlotOpnd, IR::RegOpnd * envOpnd);
    IR::Instr *     LoadHelperTemp(IR::Instr * instr, IR::Instr * instrInsert);
    IR::Instr *     LowerLoadVar(IR::Instr *instr, IR::Opnd *opnd);
    void            LoadArgumentCount(IR::Instr *const instr);
    void            LoadStackArgPtr(IR::Instr *const instr);
    IR::Instr *     InsertLoadStackAddress(StackSym *sym, IR::Instr * instrInsert, IR::RegOpnd *optionalDstOpnd = nullptr);
    void            LoadArgumentsFromFrame(IR::Instr *const instr);
    IR::Instr *     LowerUnaryHelper(IR::Instr *instr, IR::JnHelperMethod helperMethod, IR::Opnd* opndBailoutArg = nullptr);
    IR::Instr *     LowerUnaryHelperMem(IR::Instr *instr, IR::JnHelperMethod helperMethod, IR::Opnd* opndBailoutArg = nullptr);
    IR::Instr *     LowerUnaryHelperMemWithFuncBody(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerUnaryHelperMemWithFunctionInfo(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBinaryHelperMemWithFuncBody(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerUnaryHelperMemWithTemp(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerUnaryHelperMemWithTemp2(IR::Instr *instr, IR::JnHelperMethod helperMethod, IR::JnHelperMethod helperMethodWithTemp);
    IR::Instr *     LowerUnaryHelperMemWithBoolReference(IR::Instr *instr, IR::JnHelperMethod helperMethod, bool useBoolForBailout);
    IR::Instr *     LowerBinaryHelperMem(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBinaryHelperMemWithTemp(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBinaryHelperMemWithTemp2(IR::Instr *instr, IR::JnHelperMethod helperMethod, IR::JnHelperMethod helperMethodWithTemp);
    IR::Instr *     LowerBinaryHelperMemWithTemp3(IR::Instr *instr, IR::JnHelperMethod helperMethod,
        IR::JnHelperMethod helperMethodWithTemp, IR::JnHelperMethod helperMethodLeftDead);
    IR::Instr *     LowerAddLeftDeadForString(IR::Instr *instr);
    IR::Instr *     LowerBinaryHelper(IR::Instr *instr, IR::JnHelperMethod helperMethod);
#ifdef ENABLE_WASM
    IR::Instr *     LowerCheckWasmSignature(IR::Instr * instr);
    IR::Instr *     LowerLdWasmFunc(IR::Instr* instr);
    IR::Instr *     LowerGrowWasmMemory(IR::Instr* instr);
#endif
    IR::Instr *     LowerInitCachedScope(IR::Instr * instr);
    IR::Instr *     LowerBrBReturn(IR::Instr * instr, IR::JnHelperMethod helperMethod, bool isHelper);
    IR::Instr *     LowerBrBMem(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBrOnObject(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerStrictBrOrCm(IR::Instr * instr, IR::JnHelperMethod helperMethod, bool noMathFastPath, bool isBranch, bool isHelper = true);
    IR::Instr *     LowerBrCMem(IR::Instr * instr, IR::JnHelperMethod helperMethod, bool noMathFastPath, bool isHelper = true);
    IR::Instr *     LowerBrFncApply(IR::Instr * instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBrProperty(IR::Instr * instr, IR::JnHelperMethod helperMethod);
    IR::Instr *     LowerBrOnClassConstructor(IR::Instr *instr, IR::JnHelperMethod helperMethod);
    IR::Instr*      LowerMultiBr(IR::Instr * instr, IR::JnHelperMethod helperMethod);
    IR::Instr*      LowerMultiBr(IR::Instr * instr);
    IR::Instr *     LowerElementUndefined(IR::Instr * instr, IR::JnHelperMethod helper);
    IR::Instr *     LowerElementUndefinedMem(IR::Instr * instr, IR::JnHelperMethod helper);
    IR::Instr *     LowerElementUndefinedScoped(IR::Instr * instr, IR::JnHelperMethod helper);
    IR::Instr *     LowerElementUndefinedScopedMem(IR::Instr * instr, IR::JnHelperMethod helper);
    IR::Instr *     LowerLdElemUndef(IR::Instr * instr);
    IR::Instr *     LowerRestParameter(IR::Opnd *formalsOpnd, IR::Opnd *dstOpnd, IR::Opnd *excessOpnd, IR::Instr *instr, IR::RegOpnd *generatorArgsPtrOpnd);
    IR::Instr *     LowerArgIn(IR::Instr *instr);
    IR::Instr *     LowerArgInAsmJs(IR::Instr *instr);
    IR::Instr *     LowerProfiledNewScArray(IR::JitProfilingInstr* arrInstr);
    IR::Instr *     LowerNewScArray(IR::Instr *arrInstr);
    IR::Instr *     LowerNewScIntArray(IR::Instr *arrInstr);
    IR::Instr *     LowerNewScFltArray(IR::Instr *arrInstr);
    IR::Instr *     LowerArraySegmentVars(IR::Instr *instr);
    IR::Instr *     LowerEqualityBranch(IR::Instr* instr, IR::JnHelperMethod helper);
    IR::Instr *     LowerEqualityCompare(IR::Instr* instr, IR::JnHelperMethod helper);
    template <typename ArrayType>
    BOOL            IsSmallObject(uint32 length);
    void            GenerateProfiledNewScIntArrayFastPath(IR::Instr *instr, Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, intptr_t weakFuncRef);
    void            GenerateArrayInfoIsNativeIntArrayTest(IR::Instr * instr,  Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, IR::LabelInstr * helperLabel);
    void            GenerateProfiledNewScFloatArrayFastPath(IR::Instr *instr, Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, intptr_t weakFuncRef);
    void            GenerateArrayInfoIsNativeFloatAndNotIntArrayTest(IR::Instr * instr,  Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, IR::LabelInstr * helperLabel);
    bool            IsEmitTempDst(IR::Opnd *opnd);
    bool            IsEmitTempSrc(IR::Opnd *opnd);
    bool            IsNullOrUndefRegOpnd(IR::RegOpnd *opnd) const;
    bool            IsConstRegOpnd(IR::RegOpnd *opnd) const;
    IR::Opnd *      GetConstRegOpnd(IR::RegOpnd *opnd, IR::Instr *instr);
    IR::Instr *     GenerateRuntimeError(IR::Instr * insertBeforeInstr, Js::MessageId errorCode, IR::JnHelperMethod helper = IR::JnHelperMethod::HelperOp_RuntimeTypeError);
    bool            InlineBuiltInLibraryCall(IR::Instr *callInstr);
    void            LowerInlineBuiltIn(IR::Instr* instr);
    intptr_t GetObjRefForBuiltInTarget(IR::RegOpnd * opnd);
    bool            TryGenerateFastCmSrXx(IR::Instr * instr);
    bool            TryGenerateFastBrEq(IR::Instr * instr);
    bool            TryGenerateFastBrNeq(IR::Instr * instr);
    bool            TryGenerateFastBrSrXx(IR::Instr * instr, IR::RegOpnd * srcReg1, IR::RegOpnd * srcReg2, IR::Instr ** pInstrPrev, bool noMathFastPath);
    IR::BranchInstr* GenerateFastBrConst(IR::BranchInstr *branchInstr, IR::Opnd * constOpnd, bool isEqual);
    bool            GenerateFastCondBranch(IR::BranchInstr * instrBranch, bool *pIsHelper);
    void            GenerateBooleanNegate(IR::Instr * instr, IR::Opnd * srcBool, IR::Opnd * dst);
    bool            GenerateJSBooleanTest(IR::RegOpnd * regSrc, IR::Instr * insertInstr, IR::LabelInstr * labelTarget, bool fContinueLabel = false);
    bool            GenerateFastEqBoolInt(IR::Instr * instr, bool *pIsHelper, bool isInHelper);
    bool            GenerateFastBrOrCmEqDefinite(IR::Instr * instrBranch, IR::JnHelperMethod helperMethod, bool *pNeedHelper, bool isBranch, bool isInHelper);
    bool            GenerateFastBrEqLikely(IR::BranchInstr * instrBranch, bool *pNeedHelper, bool isInHelper);
    bool            GenerateFastBooleanAndObjectEqLikely(IR::Instr * instr, IR::Opnd *src1, IR::Opnd *src2, IR::LabelInstr * labelHelper, IR::LabelInstr * labelEqualLikely, bool *pNeedHelper, bool isInHelper);
    bool            GenerateFastCmEqLikely(IR::Instr * instr, bool *pNeedHelper, bool isInHelper);
    bool            GenerateFastBrBool(IR::BranchInstr *const instr);
    bool            GenerateFastStringCheck(IR::Instr *instr, IR::RegOpnd *srcReg1, IR::RegOpnd *srcReg2, bool isEqual, bool isStrict, IR::LabelInstr *labelHelper, IR::LabelInstr *labelBranchSuccess, IR::LabelInstr *labelBranchFail);
    bool            GenerateFastBrOrCmString(IR::Instr* instr);
    void            GenerateDynamicLoadPolymorphicInlineCacheSlot(IR::Instr * instrInsert, IR::RegOpnd * inlineCacheOpnd, IR::Opnd * objectTypeOpnd);
    static IR::Instr *LoadFloatFromNonReg(IR::Opnd * opndOrig, IR::Opnd * regOpnd, IR::Instr * instrInsert);
    void            LoadInt32FromUntaggedVar(IR::Instr *const instrLoad);
    bool            GetValueFromIndirOpnd(IR::IndirOpnd *indirOpnd, IR::Opnd **pValueOpnd, IntConstType *pValue);
    void            GenerateFastBrOnObject(IR::Instr *instr);

    void            GenerateObjectTypeTest(IR::RegOpnd *srcReg, IR::Instr *instrInsert, IR::LabelInstr *labelHelper);
    static          IR::LabelInstr* InsertContinueAfterExceptionLabelForDebugger(Func* func, IR::Instr* insertAfterInstr, bool isHelper);
    void            GenerateObjectHeaderInliningTest(IR::RegOpnd *baseOpnd, IR::LabelInstr * target, IR::Instr *insertBeforeInstr);

// Static tables that will be used by the GetArray* methods below
private:
    static const VTableValue VtableAddresses[static_cast<ValueType::TSize>(ObjectType::Count)];
    static const uint32 OffsetsOfHeadSegment[static_cast<ValueType::TSize>(ObjectType::Count)];
    static const uint32 OffsetsOfLength[static_cast<ValueType::TSize>(ObjectType::Count)];
    static const IRType IndirTypes[static_cast<ValueType::TSize>(ObjectType::Count)];
    static const BYTE IndirScales[static_cast<ValueType::TSize>(ObjectType::Count)];

private:
    static VTableValue GetArrayVtableAddress(const ValueType valueType, bool getVirtual = false);

public:
    static uint32   GetArrayOffsetOfHeadSegment(const ValueType valueType);
    static uint32   GetArrayOffsetOfLength(const ValueType valueType);
    static IRType   GetArrayIndirType(const ValueType valueType);
    static BYTE     GetArrayIndirScale(const ValueType valueType);
    static int      SimdGetElementCountFromBytes(ValueType arrValueType, uint8 dataWidth);
private:
    bool            ShouldGenerateArrayFastPath(const IR::Opnd *const arrayOpnd, const bool supportsObjectsWithArrays, const bool supportsTypedArrays, const bool requiresSse2ForFloatArrays) const;
    IR::RegOpnd *   LoadObjectArray(IR::RegOpnd *const baseOpnd, IR::Instr *const insertBeforeInstr);
    IR::RegOpnd *   GenerateArrayTest(IR::RegOpnd *const baseOpnd, IR::LabelInstr *const isNotObjectLabel, IR::LabelInstr *const isNotArrayLabel, IR::Instr *const insertBeforeInstr, const bool forceFloat, const bool isStore = false, const bool allowDefiniteArray = false);
    void            GenerateIsEnabledArraySetElementFastPathCheck(IR::LabelInstr * isDisabledLabel, IR::Instr * const insertBeforeInstr);
    void            GenerateIsEnabledIntArraySetElementFastPathCheck(IR::LabelInstr * isDisabledLabel, IR::Instr * const insertBeforeInstr);
    void            GenerateIsEnabledFloatArraySetElementFastPathCheck(IR::LabelInstr * isDisabledLabel, IR::Instr * const insertBeforeInstr);
    void            GenerateStringTest(IR::RegOpnd *srcReg, IR::Instr *instrInsert, IR::LabelInstr * failLabel, IR::LabelInstr * succeedLabel = nullptr, bool generateObjectCheck = true);
    void            GenerateSymbolTest(IR::RegOpnd *srcReg, IR::Instr *instrInsert, IR::LabelInstr * failLabel, IR::LabelInstr * succeedLabel = nullptr, bool generateObjectCheck = true);
    void            GeneratePropertyStringTest(IR::RegOpnd *srcReg, IR::Instr *instrInsert, IR::LabelInstr *labelHelper, bool usePoison);
    IR::RegOpnd *   GenerateUntagVar(IR::RegOpnd * opnd, IR::LabelInstr * labelFail, IR::Instr * insertBeforeInstr, bool generateTagCheck = true);
    void            GenerateNotZeroTest( IR::Opnd * opndSrc, IR::LabelInstr * labelZero, IR::Instr * instrInsert);
    IR::Opnd *      CreateOpndForSlotAccess(IR::Opnd * opnd);
    IR::RegOpnd *   GetRegOpnd(IR::Opnd * opnd, IR::Instr * insertInstr, Func * func, IRType type);

    void            GenerateSwitchStringLookup(IR::Instr * instr);
    void            GenerateSingleCharStrJumpTableLookup(IR::Instr * instr);
    void            LowerJumpTableMultiBranch(IR::MultiBranchInstr * multiBrInstr, IR::RegOpnd * indexOpnd);

    void            LowerConvNum(IR::Instr *instrLoad, bool noMathFastPath);

    void            InsertBitTestBranch(IR::Opnd * bitMaskOpnd, IR::Opnd * bitIndex, bool jumpIfBitOn, IR::LabelInstr * targetLabel, IR::Instr * insertBeforeInstr);
    void            GenerateGetSingleCharString(IR::RegOpnd * charCodeOpnd, IR::Opnd * resultOpnd, IR::LabelInstr * labelHelper, IR::LabelInstr * doneLabel, IR::Instr * instr, bool isCodePoint);
    void            GenerateFastBrBReturn(IR::Instr * instr);

public:
    static IR::Instr *HoistIndirOffset(IR::Instr* instr, IR::IndirOpnd *indirOpnd, RegNum regNum);
    static IR::Instr *HoistIndirOffsetAsAdd(IR::Instr* instr, IR::IndirOpnd *orgOpnd, IR::Opnd *baseOpnd, int offset, RegNum regNum);
    static IR::Instr *HoistIndirIndexOpndAsAdd(IR::Instr* instr, IR::IndirOpnd *orgOpnd, IR::Opnd *baseOpnd, IR::Opnd *indexOpnd, RegNum regNum);
    static IR::Instr *HoistSymOffset(IR::Instr *instr, IR::SymOpnd *symOpnd, RegNum baseReg, uint32 offset, RegNum regNum);
    static IR::Instr *HoistSymOffsetAsAdd(IR::Instr* instr, IR::SymOpnd *orgOpnd, IR::Opnd *baseOpnd, int offset, RegNum regNum);

    static IR::LabelInstr *     InsertLabel(const bool isHelper, IR::Instr *const insertBeforeInstr);

    static IR::Instr *          InsertMove(IR::Opnd *dst, IR::Opnd *src, IR::Instr *const insertBeforeInstr, bool generateWriteBarrier = true);
    static IR::Instr *          InsertMoveWithBarrier(IR::Opnd *dst, IR::Opnd *src, IR::Instr *const insertBeforeInstr);
#if _M_X64
    static IR::Instr *          InsertMoveBitCast(IR::Opnd *const dst, IR::Opnd *const src1, IR::Instr *const insertBeforeInstr);
#endif
    static IR::BranchInstr *    InsertBranch(const Js::OpCode opCode, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::BranchInstr *    InsertBranch(const Js::OpCode opCode, const bool isUnsigned, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertCompare(IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
           IR::BranchInstr *    InsertCompareBranch(IR::Opnd *const compareSrc1, IR::Opnd *const compareSrc2, Js::OpCode branchOpCode, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr, const bool ignoreNaN = false);
           IR::BranchInstr *    InsertCompareBranch(IR::Opnd *compareSrc1, IR::Opnd *compareSrc2, Js::OpCode branchOpCode, const bool isUnsigned, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr, const bool ignoreNaN = false);
    static IR::Instr *          InsertTest(IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
    static IR::BranchInstr *    InsertTestBranch(IR::Opnd *const testSrc1, IR::Opnd *const testSrc2, const Js::OpCode branchOpCode, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::BranchInstr *    InsertTestBranch(IR::Opnd *const testSrc1, IR::Opnd *const testSrc2, const Js::OpCode branchOpCode, const bool isUnsigned, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertAdd(const bool needFlags, IR::Opnd *const dst, IR::Opnd *src1, IR::Opnd *src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertSub(const bool needFlags, IR::Opnd *const dst, IR::Opnd *src1, IR::Opnd *src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertLea(IR::RegOpnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          ChangeToLea(IR::Instr *const instr);
    static IR::Instr *          InsertXor(IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertAnd(IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertOr(IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertShift(const Js::OpCode opCode, const bool needFlags, IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertShiftBranch(const Js::OpCode shiftOpCode, IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, const Js::OpCode branchOpCode, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertShiftBranch(const Js::OpCode shiftOpCode, IR::Opnd *const dst, IR::Opnd *const src1, IR::Opnd *const src2, const Js::OpCode branchOpCode, const bool isUnsigned, IR::LabelInstr *const target, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertConvertFloat32ToFloat64(IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr);
    static IR::Instr *          InsertConvertFloat64ToFloat32(IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr);

public:
    static void InsertDecUInt32PreventOverflow(IR::Opnd *const dst, IR::Opnd *const src, IR::Instr *const insertBeforeInstr, IR::Instr * *const onOverflowInsertBeforeInstrRef = nullptr);
    static void InsertAddWithOverflowCheck(const bool needFlags, IR::Opnd *const dst, IR::Opnd *src1, IR::Opnd *src2, IR::Instr *const insertBeforeInstr, IR::Instr **const onOverflowInsertBeforeInstrRef);

    void InsertFloatCheckForZeroOrNanBranch(IR::Opnd *const src, const bool branchOnZeroOrNan, IR::LabelInstr *const target, IR::LabelInstr *const fallthroughLabel, IR::Instr *const insertBeforeInstr);

public:
    static IR::HelperCallOpnd*  CreateHelperCallOpnd(IR::JnHelperMethod helperMethod, int helperArgCount, Func* func);
    static IR::Opnd *           GetMissingItemOpnd(IRType type, Func *func);
    static IR::Opnd *           GetMissingItemOpndForAssignment(IRType type, Func *func);
    static IR::Opnd *           GetMissingItemOpndForCompare(IRType type, Func *func);
    static IR::Opnd *           GetImplicitCallFlagsOpnd(Func * func);
    inline static IR::IntConstOpnd* MakeCallInfoConst(ushort flags, int32 argCount, Func* func) {
        argCount = Js::CallInfo::GetArgCountWithoutExtraArgs((Js::CallFlags)flags, (uint16)argCount);
#ifdef _M_X64
        // This was defined differently for x64
        Js::CallInfo callInfo = Js::CallInfo((Js::CallFlags)flags, (unsigned __int16)argCount);
        return IR::IntConstOpnd::New(*((IntConstType *)((void *)&callInfo)), TyInt32, func, true);
#else
        AssertMsg(!(argCount & 0xFF000000), "Too many arguments"); //final 8 bits are for flags
        AssertMsg(!(flags & ~0xFF), "Flags are invalid!"); //8 bits for flags
        return IR::IntConstOpnd::New(argCount | (flags << 24), TyMachReg, func, true);
#endif
    }
    static void InsertAndLegalize(IR::Instr * instr, IR::Instr* insertBeforeInstr);
private:
    IR::IndirOpnd* GenerateFastElemICommon(
        _In_ IR::Instr* elemInstr,
        _In_ bool isStore,
        _In_ IR::IndirOpnd* indirOpnd,
        _In_ IR::LabelInstr* labelHelper,
        _In_ IR::LabelInstr* labelCantUseArray,
        _In_opt_ IR::LabelInstr* labelFallthrough,
        _Out_ bool* pIsTypedArrayElement,
        _Out_ bool* pIsStringIndex,
        _Out_opt_ bool* emitBailoutRef,
        _Outptr_opt_result_maybenull_ IR::Opnd** maskOpnd,
        _Outptr_opt_result_maybenull_ IR::LabelInstr** pLabelSegmentLengthIncreased = nullptr,
        _In_ bool checkArrayLengthOverflow = true,
        _In_ bool forceGenerateFastPath = false,
        _In_ bool returnLength = false,
        _In_opt_ IR::LabelInstr* bailOutLabelInstr = nullptr,
        _Out_opt_ bool* indirOpndOverflowed = nullptr,
        _In_ Js::FldInfoFlags flags = Js::FldInfo_NoInfo);

    IR::IndirOpnd * GenerateFastElemIIntIndexCommon(
        IR::Instr * ldElem,
        bool isStore,
        IR::IndirOpnd * indirOpnd,
        IR::LabelInstr * labelHelper,
        IR::LabelInstr * labelCantUseArray,
        IR::LabelInstr *labelFallthrough,
        bool * pIsTypedArrayElement,
        bool *emitBailoutRef,
        IR::LabelInstr **pLabelSegmentLengthIncreased,
        bool checkArrayLengthOverflow,
        IR::Opnd** maskOpnd,
        bool forceGenerateFastPath = false,
        bool returnLength = false,
        IR::LabelInstr *bailOutLabelInstr = nullptr,
        bool * indirOpndOverflowed = nullptr);

    IR::IndirOpnd* GenerateFastElemIStringIndexCommon(
        _In_ IR::Instr* elemInstr,
        _In_ bool isStore,
        _In_ IR::IndirOpnd* indirOpnd,
        _In_ IR::LabelInstr* labelHelper,
        _In_ Js::FldInfoFlags flags);

    IR::IndirOpnd* GenerateFastElemISymbolIndexCommon(
        _In_ IR::Instr* elemInstr,
        _In_ bool isStore,
        _In_ IR::IndirOpnd* indirOpnd,
        _In_ IR::LabelInstr* labelHelper,
        _In_ Js::FldInfoFlags flags);

    IR::IndirOpnd* GenerateFastElemISymbolOrStringIndexCommon(
        _In_ IR::Instr* instrInsert,
        _In_ IR::RegOpnd* indexOpnd,
        _In_ IR::RegOpnd* baseOpnd,
        _In_ const uint32 inlineCacheOffset,
        _In_ const uint32 hitRateOffset,
        _In_ IR::LabelInstr* labelHelper,
        _In_ Js::FldInfoFlags flags);

    void GenerateLookUpInIndexCache(
        _In_ IR::Instr* instrInsert,
        _In_ IR::RegOpnd* indexOpnd,
        _In_ IR::RegOpnd* baseOpnd,
        _In_opt_ IR::RegOpnd* opndSlotArray,
        _In_opt_ IR::RegOpnd* opndSlotIndex,
        _In_ const uint32 inlineCacheOffset,
        _In_ const uint32 hitRateOffset,
        _In_ IR::LabelInstr* labelHelper,
        _In_ Js::FldInfoFlags flags = Js::FldInfo_NoInfo);

    template <bool CheckLocal, bool CheckInlineSlot, bool DoAdd>
    void GenerateLookUpInIndexCacheHelper(
        _In_ IR::Instr* instrInsert,
        _In_ IR::RegOpnd* baseOpnd,
        _In_opt_ IR::RegOpnd* opndSlotArray,
        _In_opt_ IR::RegOpnd* opndSlotIndex,
        _In_ IR::RegOpnd* objectTypeOpnd,
        _In_ IR::RegOpnd* inlineCacheOpnd,
        _In_ IR::LabelInstr* doneLabel,
        _In_ IR::LabelInstr* helperLabel,
        _Outptr_ IR::LabelInstr** nextLabel,
        _Outptr_ IR::BranchInstr** branchToPatch,
        _Inout_ IR::RegOpnd** taggedTypeOpnd);

    void            GenerateFastIsInSymbolOrStringIndex(IR::Instr * instrInsert, IR::RegOpnd *indexOpnd, IR::RegOpnd *baseOpnd, IR::Opnd *dest, uint32 inlineCacheOffset, const uint32 hitRateOffset, IR::LabelInstr * labelHelper, IR::LabelInstr * labelDone);
    IR::BranchInstr* InsertMissingItemCompareBranch(IR::Opnd* compareSrc, Js::OpCode opcode, IR::LabelInstr* target, IR::Instr* insertBeforeInstr);
    bool            GenerateFastLdElemI(IR::Instr *& ldElem, bool *instrIsInHelperBlockRef);
    bool            GenerateFastStElemI(IR::Instr *& StElem, bool *instrIsInHelperBlockRef);
    bool            GenerateFastLdLen(IR::Instr *ldLen, bool *instrIsInHelperBlockRef);
    bool            GenerateFastCharAt(Js::BuiltinFunction index, IR::Opnd *dst, IR::Opnd *srcStr, IR::Opnd *srcIndex, IR::Instr *callInstr, IR::Instr *insertInstr,
        IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel);
    bool            GenerateFastInlineGlobalObjectParseInt(IR::Instr *instr);
    bool            GenerateFastInlineStringFromCharCode(IR::Instr* instr);
    bool            GenerateFastInlineStringFromCodePoint(IR::Instr* instr);
    void            GenerateFastInlineStringCodePointAt(IR::Instr* doneLabel, Func* func, IR::Opnd *strLength, IR::Opnd *srcIndex, IR::RegOpnd *lowerChar, IR::RegOpnd *strPtr);
    bool            GenerateFastInlineStringCharCodeAt(IR::Instr* instr, Js::BuiltinFunction index);
    bool            GenerateFastInlineStringReplace(IR::Instr* instr);
    void            GenerateFastInlineIsArray(IR::Instr * instr);
    void            GenerateFastInlineHasOwnProperty(IR::Instr * instr);
    void            GenerateFastInlineArrayPush(IR::Instr * instr);
    void            GenerateFastInlineArrayPop(IR::Instr * instr);
    // TODO: Cleanup
    // void            GenerateFastInlineStringSplitMatch(IR::Instr * instr);
    void            GenerateFastInlineMathImul(IR::Instr* instr);
    void            GenerateFastInlineMathClz(IR::Instr* instr);
    void            GenerateCtz(IR::Instr* instr);
    void            GeneratePopCnt(IR::Instr* instr);
    template <bool Saturate> void GenerateTruncWithCheck(_In_ IR::Instr* instr);
    void            GenerateFastInlineMathFround(IR::Instr* instr);
    // void            GenerateFastInlineRegExpExec(IR::Instr * instr);
    bool            GenerateFastPush(IR::Opnd *baseOpndParam, IR::Opnd *src, IR::Instr *callInstr, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel, IR::LabelInstr * bailOutLabelHelper, bool returnLength = false);
    bool            GenerateFastReplace(IR::Opnd* strOpnd, IR::Opnd* src1, IR::Opnd* src2, IR::Instr *callInstr, IR::Instr *insertInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel);
    bool            ShouldGenerateStringReplaceFastPath(IR::Instr * instr, IntConstType argCount);
    bool            GenerateFastPop(IR::Opnd *baseOpndParam, IR::Instr *callInstr, IR::LabelInstr *labelHelper, IR::LabelInstr *doneLabel, IR::LabelInstr * bailOutLabelHelper);
    bool            GenerateFastStringLdElem(IR::Instr * ldElem, IR::LabelInstr * labelHelper, IR::LabelInstr * labelFallThru);
    IR::Instr *     LowerCallDirect(IR::Instr * instr);
    IR::Instr *     GenerateDirectCall(IR::Instr* inlineInstr, IR::Opnd* funcObj, ushort callflags);
    IR::Instr *     GenerateFastInlineBuiltInMathRandom(IR::Instr* instr);
    IR::Instr *     GenerateHelperToArrayPushFastPath(IR::Instr * instr, IR::LabelInstr * bailOutLabelHelper);
    IR::Instr *     GenerateHelperToArrayPopFastPath(IR::Instr * instr, IR::LabelInstr * doneLabel, IR::LabelInstr * bailOutLabelHelper);
    IR::Instr *     LowerCondBranchCheckBailOut(IR::BranchInstr * instr, IR::Instr * helperCall, bool isHelper);
    IR::Instr *     LowerBailOnEqualOrNotEqual(IR::Instr * instr, IR::BranchInstr * branchInstr = nullptr, IR::LabelInstr * labelBailOut = nullptr, IR::PropertySymOpnd * propSymOpnd = nullptr, bool isHelper = false);
    void            LowerBailOnNegative(IR::Instr *const instr);
    void            LowerBailoutCheckAndLabel(IR::Instr *instr, bool onEqual, bool isHelper);
    IR::Instr *     LowerBailOnNotSpreadable(IR::Instr *instr);
    IR::Instr *     LowerBailOnNotPolymorphicInlinee(IR::Instr * instr);
    IR::Instr *     LowerBailOnNotStackArgs(IR::Instr * instr);
    IR::Instr *     LowerBailOnNotObject(IR::Instr *instr, IR::BranchInstr *branchInstr = nullptr, IR::LabelInstr *labelBailOut = nullptr);
    IR::Instr *     LowerCheckIsFuncObj(IR::Instr *instr, bool checkFuncInfo = false);
    IR::Instr *     LowerBailOnTrue(IR::Instr *instr, IR::LabelInstr *labelBailOut = nullptr);
    IR::Instr *     LowerBailOnNotBuiltIn(IR::Instr *instr, IR::BranchInstr *branchInstr = nullptr, IR::LabelInstr *labelBailOut = nullptr);
    IR::Instr *     LowerBailOnNotInteger(IR::Instr *instr, IR::BranchInstr *branchInstr = nullptr, IR::LabelInstr *labelBailOut = nullptr);
    IR::Instr *     LowerBailOnIntMin(IR::Instr *instr, IR::BranchInstr *branchInstr = nullptr, IR::LabelInstr *labelBailOut = nullptr);
    void            LowerBailOnNotString(IR::Instr *instr);
    IR::Instr *     LowerBailForDebugger(IR::Instr* instr, bool isInsideHelper = false);
    IR::Instr *     LowerBailOnException(IR::Instr* instr);
    void            LowerReinterpretPrimitive(IR::Instr* instr);
    IR::Instr *     LowerBailOnEarlyExit(IR::Instr* instr);

    void            LowerOneBailOutKind(IR::Instr *const instr, const IR::BailOutKind bailOutKindToLower, const bool isInHelperBlock, const bool preserveBailOutKindInInstr = false);

    void            SplitBailOnNotArray(IR::Instr *const instr, IR::Instr * *const bailOnNotArrayRef, IR::Instr * *const bailOnMissingValueRef);
    IR::RegOpnd *   LowerBailOnNotArray(IR::Instr *const instr);
    void            LowerBailOnMissingValue(IR::Instr *const instr, IR::RegOpnd *const arrayOpnd);
    void            LowerBailOnInvalidatedArrayHeadSegment(IR::Instr *const instr, const bool isInHelperBlock);
    void            LowerBailOnInvalidatedArrayLength(IR::Instr *const instr, const bool isInHelperBlock);
    void            LowerBailOnCreatedMissingValue(IR::Instr *const instr, const bool isInHelperBlock);
    void            LowerBoundCheck(IR::Instr *const instr);
    IR::Opnd*       GetFuncObjectOpnd(IR::Instr* insertBeforeInstr);
    IR::Instr*      LoadFuncExpression(IR::Instr *instrFuncExpr);

    IR::Instr *     LowerBailTarget(IR::Instr * instr);
    IR::Instr *     SplitBailOnImplicitCall(IR::Instr *& instr);
    IR::Instr *     SplitBailOnImplicitCall(IR::Instr * instr, IR::Instr * helperCall, IR::Instr * insertBeforeInstr);

    IR::Instr *     SplitBailForDebugger(IR::Instr* instr);

    IR::Instr *     SplitBailOnResultCondition(IR::Instr *const instr) const;
    void            LowerBailOnResultCondition(IR::Instr *const instr, IR::LabelInstr * *const bailOutLabel, IR::LabelInstr * *const skipBailOutLabel);
    void            PreserveSourcesForBailOnResultCondition(IR::Instr *const instr, IR::LabelInstr *const skipBailOutLabel) const;
    void            LowerInstrWithBailOnResultCondition(IR::Instr *const instr, const IR::BailOutKind bailOutKind, IR::LabelInstr *const bailOutLabel, IR::LabelInstr *const skipBailOutLabel) const;
    void            GenerateObjectTestAndTypeLoad(IR::Instr *instrLdSt, IR::RegOpnd *opndBase, IR::RegOpnd *opndType, IR::LabelInstr *labelHelper);
    void            InsertMoveForPolymorphicCacheIndex(IR::Instr * instr, BailOutInfo * bailOutInfo, int bailOutRecordOffset, uint polymorphicCacheIndexValue);
    IR::LabelInstr *GenerateBailOut(IR::Instr * instr, IR::BranchInstr * branchInstr = nullptr, IR::LabelInstr * labelBailOut = nullptr, IR::LabelInstr * collectRuntimeStatsLabel = nullptr);
    void            GenerateJumpToEpilogForBailOut(BailOutInfo * bailOutInfo, IR::Instr *instrAfter, IR::LabelInstr *exitTargetInstr);
    void            GenerateThrow(IR::Opnd* errorCode, IR::Instr * instr);
    void            LowerDivI4(IR::Instr * const instr);
    void            LowerRemI4(IR::Instr * const instr);
    void            LowerTrapIfZero(IR::Instr * const instr);
    void            LowerTrapIfMinIntOverNegOne(IR::Instr * const instr);
    IR::Instr*      LowerTrapIfUnalignedAccess(IR::Instr * const instr);
    void            LowerDivI4Common(IR::Instr * const instr);
    void            LowerRemR8(IR::Instr * const instr);
    void            LowerRemR4(IR::Instr * const instr);

    IR::Instr*      LowerInlineeStart(IR::Instr * instr);
    void            LowerInlineeEnd(IR::Instr * instr);

    static
    IR::SymOpnd*    LoadCallInfo(IR::Instr * instrInsert);

    IR::Instr *     LowerCallIDynamic(IR::Instr * callInstr, ushort callFlags);
    IR::Opnd*       GenerateArgOutForInlineeStackArgs(IR::Instr* callInstr, IR::Instr* stackArgsInstr);
    IR::Opnd*       GenerateArgOutForStackArgs(IR::Instr* callInstr, IR::Instr* stackArgsInstr);

    void            GenerateLoadStackArgumentByIndex(IR::Opnd *dst, IR::RegOpnd *indexOpnd, IR::Instr *instr, int32 offset, Func *func);
    bool            GenerateFastStackArgumentsLdElemI(IR::Instr* ldElem);
    IR::IndirOpnd*  GetArgsIndirOpndForInlinee(IR::Instr* ldElem, IR::Opnd* valueOpnd);
    IR::IndirOpnd*  GetArgsIndirOpndForTopFunction(IR::Instr* ldElem, IR::Opnd* valueOpnd);
    void            GenerateCheckForArgumentsLength(IR::Instr* ldElem, IR::LabelInstr* labelCreateHeapArgs, IR::Opnd* actualParamOpnd, IR::Opnd* valueOpnd, Js::OpCode opcode);
    bool            GenerateFastArgumentsLdElemI(IR::Instr* ldElem, IR::LabelInstr *labelFallThru);
    bool            GenerateFastRealStackArgumentsLdLen(IR::Instr *ldLen);
    bool            GenerateFastArgumentsLdLen(IR::Instr *ldLen, IR::LabelInstr* labelFallThru);
    static uint16   GetFormalParamOffset() { /*formal start after frame pointer, return address, function object, callInfo*/ return 4;};

    IR::RegOpnd*    GenerateFunctionTypeFromFixedFunctionObject(IR::Instr *callInstr, IR::Opnd* functionObjOpnd);

    bool GenerateFastLdFld(IR::Instr * const instrLdFld, IR::JnHelperMethod helperMethod, IR::JnHelperMethod polymorphicHelperMethod,
        IR::LabelInstr ** labelBailOut, IR::RegOpnd* typeOpnd, bool* pIsHelper, IR::LabelInstr** pLabelHelper);
    void GenerateAuxSlotAdjustmentRequiredCheck(IR::Instr * instrToInsertBefore, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelHelper);
    void GenerateSetObjectTypeFromInlineCache(IR::Instr * instrToInsertBefore, IR::RegOpnd * opndBase, IR::RegOpnd * opndInlineCache, bool isTypeTagged);
    bool GenerateFastStFld(IR::Instr * const instrStFld, IR::JnHelperMethod helperMethod, IR::JnHelperMethod polymorphicHelperMethod,
        IR::LabelInstr ** labelBailOut, IR::RegOpnd* typeOpnd, bool* pIsHelper, IR::LabelInstr** pLabelHelper, bool withPutFlags = false, Js::PropertyOperationFlags flags = Js::PropertyOperation_None);
    void            GenerateAuxSlotPtrLoad(IR::PropertySymOpnd *propertySymOpnd, IR::Instr *insertInstr);

    bool            GenerateFastStFldForCustomProperty(IR::Instr *const instr, IR::LabelInstr * *const labelHelperRef);

    void            RelocateCallDirectToHelperPath(IR::Instr* argoutInlineSpecialized, IR::LabelInstr* labelHelper);

    bool            TryGenerateFastBrOrCmTypeOf(IR::Instr *instr, IR::Instr **prev, bool isNeqOp, bool *pfNoLower);
    void            GenerateFastBrTypeOf(IR::Instr *branch, IR::RegOpnd *object, IR::IntConstOpnd *typeIdOpnd, IR::Instr *typeOf, bool *pfNoLower, bool isNeqOp);
    void            GenerateFastCmTypeOf(IR::Instr *compare, IR::RegOpnd *object, IR::IntConstOpnd *typeIdOpnd, IR::Instr *typeOf, bool *pfNoLower, bool isNeqOp);
    void            GenerateFalsyObjectTest(IR::Instr *insertInstr, IR::RegOpnd *typeOpnd, Js::TypeId typeIdToCheck, IR::LabelInstr* target, IR::LabelInstr* done, bool isNeqOp);
    void            GenerateFalsyObjectTest(IR::Instr * insertInstr, IR::RegOpnd * typeOpnd, IR::LabelInstr * falsyLabel);

    void            GenerateJavascriptOperatorsIsConstructorGotoElse(IR::Instr *instrInsert, IR::RegOpnd *instanceRegOpnd, IR::LabelInstr *labelTrue, IR::LabelInstr *labelFalse);
    void            GenerateRecyclableObjectGetPrototypeNullptrGoto(IR::Instr *instrInsert, IR::RegOpnd *instanceRegOpnd, IR::LabelInstr *labelReturnNullptr);
    void            GenerateRecyclableObjectIsElse(IR::Instr *instrInsert, IR::RegOpnd *instanceRegOpnd, IR::LabelInstr *labelFalse);
    void            GenerateLdHomeObj(IR::Instr* instr);
    void            GenerateLdHomeObjProto(IR::Instr* instr);
    void            GenerateLdFuncObj(IR::Instr* instr);
    void            GenerateLdFuncObjProto(IR::Instr* instr);
    void            GenerateLoadNewTarget(IR::Instr* instrInsert);
    void            GenerateCheckForCallFlagNew(IR::Instr* instrInsert);
    void            GenerateGetCurrentFunctionObject(IR::Instr * instr);
    IR::Opnd *      GetInlineCacheFromFuncObjectForRuntimeUse(IR::Instr * instr, IR::PropertySymOpnd * propSymOpnd, bool isHelper);

    IR::Instr *     LowerNewClassConstructor(IR::Instr * instr);

    IR::RegOpnd *   GenerateGetImmutableOrScriptUnreferencedString(IR::RegOpnd * strOpnd, IR::Instr * insertBeforeInstr, IR::JnHelperMethod helperMethod, bool loweringCloneStr = false, bool reloadDst = true);
    void            LowerNewConcatStrMulti(IR::Instr * instr);
    void            LowerNewConcatStrMultiBE(IR::Instr * instr);
    void            LowerSetConcatStrMultiItem(IR::Instr * instr);
    void            LowerConvStr(IR::Instr * instr);
    void            LowerCoerseStr(IR::Instr * instr);
    void            LowerCoerseRegex(IR::Instr * instr);
    void            LowerCoerseStrOrRegex(IR::Instr * instr);
    void            LowerConvPrimStr(IR::Instr * instr);
    void            LowerConvStrCommon(IR::JnHelperMethod helper, IR::Instr * instr);

    void            LowerConvPropertyKey(IR::Instr* instr);

    void            GenerateRecyclerAlloc(IR::JnHelperMethod allocHelper, size_t allocSize, IR::RegOpnd* newObjDst, IR::Instr* insertionPointInstr, bool inOpHelper = false);

    template <typename ArrayType>
    IR::RegOpnd *   GenerateArrayAllocHelper(IR::Instr *instr, uint32 * psize, Js::ArrayCallSiteInfo * arrayInfo, bool * pIsHeadSegmentZeroed, bool isArrayObjCtor, bool isNoArgs);
    template <typename ArrayType>
    IR::RegOpnd *   GenerateArrayLiteralsAlloc(IR::Instr *instr, uint32 * psize, Js::ArrayCallSiteInfo * arrayInfo, bool * pIsHeadSegmentZeroed);
    template <typename ArrayType>
    IR::RegOpnd *   GenerateArrayObjectsAlloc(IR::Instr *instr, uint32 * psize, Js::ArrayCallSiteInfo * arrayInfo, bool * pIsHeadSegmentZeroed, bool isNoArgs);

    template <typename ArrayType>
    IR::RegOpnd *   GenerateArrayAlloc(IR::Instr *instr, IR::Opnd * sizeOpnd, Js::ArrayCallSiteInfo * arrayInfo);

    bool            GenerateProfiledNewScObjArrayFastPath(IR::Instr *instr, Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, intptr_t weakFuncRef, uint32 length, IR::LabelInstr* labelDone, bool isNoArgs);

    template <typename ArrayType>
    bool            GenerateProfiledNewScObjArrayFastPath(IR::Instr *instr, Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, intptr_t weakFuncRef, IR::LabelInstr* helperLabel, IR::LabelInstr* labelDone, IR::Opnd* lengthOpnd, uint32 offsetOfCallSiteIndex, uint32 offsetOfWeakFuncRef);
    bool            GenerateProfiledNewScArrayFastPath(IR::Instr *instr, Js::ArrayCallSiteInfo * arrayInfo, intptr_t arrayInfoAddr, intptr_t weakFuncRef, uint32 length);
     
    void            GenerateMemInit(IR::RegOpnd * opnd, int32 offset, int32 value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateMemInit(IR::RegOpnd * opnd, int32 offset, uint32 value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateMemInitNull(IR::RegOpnd * opnd, int32 offset, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateMemInit(IR::RegOpnd * opnd, int32 offset, IR::Opnd * value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateMemInit(IR::RegOpnd * opnd, IR::RegOpnd * offset, IR::Opnd * value, IR::Instr * insertBeforeInstr, bool isZeroed = false);
    void            GenerateRecyclerMemInit(IR::RegOpnd * opnd, int32 offset, int32 value, IR::Instr * insertBeforeInstr);
    void            GenerateRecyclerMemInit(IR::RegOpnd * opnd, int32 offset, uint32 value, IR::Instr * insertBeforeInstr);
    void            GenerateRecyclerMemInitNull(IR::RegOpnd * opnd, int32 offset, IR::Instr * insertBeforeInstr);
    void            GenerateRecyclerMemInit(IR::RegOpnd * opnd, int32 offset, IR::Opnd * value, IR::Instr * insertBeforeInstr);
    void            GenerateMemCopy(IR::Opnd * dst, IR::Opnd * src, uint32 size, IR::Instr * insertBeforeInstr);

    void            GenerateDynamicObjectAlloc(IR::Instr * newObjInstr, uint inlineSlotCount, uint slotCount, IR::RegOpnd * newObjDst, IR::Opnd * typeSrc);
    bool            GenerateSimplifiedInt4Rem(IR::Instr *const remInstr, IR::LabelInstr *const skipBailOutLabel = nullptr) const;
    IR::Instr*      GenerateCallProfiling(Js::ProfileId profileId, Js::InlineCacheIndex inlineCacheIndex, IR::Opnd* retval, IR::Opnd*calleeFunctionObjOpnd, IR::Opnd* callInfo, bool returnTypeOnly, IR::Instr*callInstr, IR::Instr*insertAfter);
    IR::Opnd*       GetImplicitCallFlagsOpnd();
    IR::Opnd*       CreateClearImplicitCallFlagsOpnd();

    void GenerateFlagInlineCacheCheckForGetterSetter(IR::Instr * insertBeforeInstr, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelNext);
    void GenerateLdFldFromFlagInlineCache(IR::Instr * insertBeforeInstr, IR::RegOpnd * opndBase, IR::Opnd * opndDst, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFallThru, bool isInlineSlot);
    static IR::BranchInstr * GenerateLocalInlineCacheCheck(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelNext, bool checkTypeWithoutProperty = false);
    static IR::BranchInstr * GenerateProtoInlineCacheCheck(IR::Instr * instrLdSt, IR::RegOpnd * opndType, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelNext);
    static void GenerateLdFldFromLocalInlineCache(IR::Instr * instrLdFld, IR::RegOpnd * opndBase, IR::Opnd * opndDst, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFallThru, bool isInlineSlot);
    static void GenerateLdFldFromProtoInlineCache(IR::Instr * instrLdFld, IR::RegOpnd * opndBase, IR::Opnd * opndDst, IR::RegOpnd * opndInlineCache, IR::LabelInstr * labelFallThru, bool isInlineSlot);

    IR::Instr *         LoadScriptContext(IR::Instr *instr);
    IR::Instr *         LoadFunctionBody(IR::Instr * instr);
    IR::Opnd *          LoadFunctionBodyOpnd(IR::Instr *instr);
    IR::Opnd *          LoadFunctionInfoOpnd(IR::Instr *instr);
    IR::Opnd *          LoadScriptContextOpnd(IR::Instr *instr);
    IR::Opnd *          LoadScriptContextValueOpnd(IR::Instr * instr, ScriptContextValue valueType);
    IR::Opnd *          LoadLibraryValueOpnd(IR::Instr * instr, LibraryValue valueType);
    IR::Opnd *          LoadVTableValueOpnd(IR::Instr * instr, VTableValue vtableType);
    IR::Opnd *          LoadOptimizationOverridesValueOpnd(IR::Instr *instr, OptimizationOverridesValue valueType);
    IR::Opnd *          LoadNumberAllocatorValueOpnd(IR::Instr *instr, NumberAllocatorValue valueType);
    IR::Opnd *          LoadIsInstInlineCacheOpnd(IR::Instr *instr, uint inlineCacheIndex);
    IR::Opnd *          LoadRuntimeInlineCacheOpnd(IR::Instr * instr, IR::PropertySymOpnd * sym, bool isHelper = false);

    void            LowerSpreadArrayLiteral(IR::Instr *instr);
    IR::Instr*      LowerSpreadCall(IR::Instr *instr, Js::CallFlags callFlags, bool setupProfiledVersion = false);
    void            LowerInlineSpreadArgOutLoopUsingRegisters(IR::Instr *callInstr, IR::RegOpnd *indexOpnd, IR::RegOpnd *arrayElementsStartOpnd);
    IR::Instr*      LowerCallIDynamicSpread(IR::Instr * callInstr, ushort callFlags);

    void            LowerNewScopeSlots(IR::Instr * instr, bool doStackSlots);
    void            LowerLdFrameDisplay(IR::Instr * instr, bool doStackDisplay);
    void            LowerLdInnerFrameDisplay(IR::Instr * instr);

    IR::AddrOpnd *  CreateFunctionBodyOpnd(Func *const func) const;
    IR::AddrOpnd *  CreateFunctionBodyOpnd(Js::FunctionBody *const functionBody) const;

    bool            GenerateRecyclerOrMarkTempAlloc(IR::Instr * instr, IR::RegOpnd * dstOpnd, IR::JnHelperMethod allocHelper, size_t allocSize, IR::SymOpnd ** tempObjectSymOpnd);
    IR::SymOpnd *   GenerateMarkTempAlloc(IR::RegOpnd *const dstOpnd, const size_t allocSize, IR::Instr *const insertBeforeInstr);
    void            LowerBrFncCachedScopeEq(IR::Instr *instr);

    IR::Instr*      InsertLoweredRegionStartMarker(IR::Instr* instrToInsertBefore);
    IR::Instr*      RemoveLoweredRegionStartMarker(IR::Instr* startMarkerInstr);

    void                    ConvertArgOpndIfGeneratorFunction(IR::Instr *instrArgIn, IR::RegOpnd *generatorArgsPtrOpnd);
    static IR::RegOpnd *    LoadGeneratorArgsPtr(IR::Instr *instrInsert);
    static IR::Instr *      LoadGeneratorObject(IR::Instr *instrInsert);

    IR::Opnd *      LoadSlotArrayWithCachedLocalType(IR::Instr * instrInsert, IR::PropertySymOpnd *propertySymOpnd);
    IR::Opnd *      LoadSlotArrayWithCachedProtoType(IR::Instr * instrInsert, IR::PropertySymOpnd *propertySymOpnd);
    IR::Instr *     LowerLdAsmJsEnv(IR::Instr *instr);
    IR::Instr *     LowerLdEnv(IR::Instr *instr);
    IR::Instr *     LowerLdSuper(IR::Instr *instr, IR::JnHelperMethod helperOpCode);
    IR::Instr *     LowerLdNativeCodeData(IR::Instr *instr);
    IR::Instr *     LowerFrameDisplayCheck(IR::Instr * instr);
    IR::Instr *     LowerSlotArrayCheck(IR::Instr * instr);
    static void     InsertObjectPoison(IR::Opnd* poisonedOpnd, IR::BranchInstr* branchInstr, IR::Instr* insertInstr, bool isForStore);

    IR::RegOpnd *   LoadIndexFromLikelyFloat(IR::RegOpnd *indexOpnd, const bool skipNegativeCheck, IR::LabelInstr *const notTaggedIntLabel, IR::LabelInstr *const negativeLabel, IR::Instr *const insertBeforeInstr);

    void MarkConstantAddressRegOpndLiveOnBackEdge(IR::LabelInstr * loopTop);
#if DBG
    static void LegalizeVerifyRange(IR::Instr * instrStart, IR::Instr * instrLast);
    void        ReconcileWithLowererStateOnHelperCall(IR::Instr * callInstr, IR::JnHelperMethod helperMethod);
    void        ClearAndSaveImplicitCallCheckOnHelperCallCheckState();
    void        RestoreImplicitCallCheckOnHelperCallCheckState();
    IR::Instr*  LowerCheckLowerIntBound(IR::Instr * instr);
    IR::Instr*  LowerCheckUpperIntBound(IR::Instr * instr);
#endif

    IR::Instr *     LowerGetCachedFunc(IR::Instr *instr);
    IR::Instr *     LowerCommitScope(IR::Instr *instr);
    IR::Instr*      LowerTry(IR::Instr* instr, bool tryCatch);
    IR::Instr *     LowerCatch(IR::Instr *instr);
    IR::Instr *     LowerLeave(IR::Instr *instr, IR::LabelInstr * targetInstr, bool fromFinalLower, bool isOrphanedLeave = false);
    void            InsertReturnThunkForRegion(Region* region, IR::LabelInstr* restoreLabel);
    void            SetHasBailedOut(IR::Instr * bailoutInstr);
    IR::Instr*      EmitEHBailoutStackRestore(IR::Instr * bailoutInstr);
    void            EmitSaveEHBailoutReturnValueAndJumpToRetThunk(IR::Instr * instr);
    void            EmitRestoreReturnValueFromEHBailout(IR::LabelInstr * restoreLabel, IR::LabelInstr * epilogLabel);   
    void            LowerInitForInEnumerator(IR::Instr * instr);
    void            AllocStackForInObjectEnumeratorArray();
    IR::RegOpnd *   GenerateForInEnumeratorLoad(IR::Opnd * forInEnumeratorOpnd, IR::Instr * insertBeforeInstr);
    IR::Opnd *      GetForInEnumeratorFieldOpnd(IR::Opnd * forInEnumeratorOpnd, uint fieldOffset, IRType type);

    void            GenerateInitForInEnumeratorFastPath(IR::Instr * instr, Js::EnumeratorCache * forInCache);
    void            GenerateHasObjectArrayCheck(IR::RegOpnd * objectOpnd, IR::RegOpnd * typeOpnd, IR::LabelInstr * hasObjectArray, IR::Instr * insertBeforeInstr);

    IR::LabelInstr* InsertLoopTopLabel(IR::Instr * insertBeforeInstr);
    IR::Instr *     AddBailoutToHelperCallInstr(IR::Instr * helperCallInstr, BailOutInfo * bailoutInfo, IR::BailOutKind  bailoutKind, IR::Instr * primaryBailoutInstr);

    IR::Instr* InsertObjectCheck(IR::RegOpnd *funcOpnd, IR::Instr *insertBeforeInstr, IR::BailOutKind bailOutKind, BailOutInfo *bailOutInfo);
    IR::Instr* InsertFunctionTypeIdCheck(IR::RegOpnd *funcOpnd, IR::Instr *insertBeforeInstr, IR::BailOutKind bailOutKind, BailOutInfo *bailOutInfo);
    IR::Instr* InsertFunctionInfoCheck(IR::RegOpnd *funcOpnd, IR::Instr *insertBeforeInstr, IR::AddrOpnd *inlinedFuncInfo, IR::BailOutKind bailOutKind, BailOutInfo *bailOutInfo);

public:
    static IRType   GetImplicitCallFlagsType()
    {
        static_assert(sizeof(Js::ImplicitCallFlags) == 1, "If this size changes, change TyUint8 in the line below to the right type.");
        return TyUint8;
    }
    static IRType   GetFldInfoFlagsType()
    {
        static_assert(sizeof(Js::FldInfoFlags) == 1, "If this size changes, change TyUint8 in the line below to the right type.");
        return TyUint8;
    }

    static bool     IsSpreadCall(IR::Instr *instr);
    static
    IR::Instr*      GetLdSpreadIndicesInstr(IR::Instr *instr);
    static bool ShouldDoLazyFixedTypeBailout(Func* func) { return func->ShouldDoLazyBailOut() && PHASE_ON1(Js::LazyFixedTypeBailoutPhase); }
    static bool ShouldDoLazyFixedDataBailout(Func* func) { return func->ShouldDoLazyBailOut() && !PHASE_OFF1(Js::LazyFixedDataBailoutPhase); }
    LowererMD * GetLowererMD() { return &m_lowererMD; }
private:
    Func *          m_func;
    LowererMD       m_lowererMD;
    JitArenaAllocator *m_alloc;
    IR::Opnd       * nextStackFunctionOpnd;
    IR::LabelInstr * outerMostLoopLabel;
    BVSparse<JitArenaAllocator> * initializedTempSym;
    BVSparse<JitArenaAllocator> * addToLiveOnBackEdgeSyms;
    Region *        currentRegion;

#if DBG
    HelperCallCheckState helperCallCheckState;
    HelperCallCheckState oldHelperCallCheckState;
    Js::OpCode m_currentInstrOpCode;
#endif

    //
    // Generator
    //
    class LowerGeneratorHelper
    {
        Func* const func;
        LowererMD &lowererMD;
        Lowerer* const lowerer;

        IR::LabelInstr* epilogueForReturnStatements = nullptr;
        IR::LabelInstr* epilogueForBailOut = nullptr;

        void EnsureEpilogueLabels();
        IR::SymOpnd* CreateResumeYieldOpnd() const;

    public:
        LowerGeneratorHelper(Func* func, Lowerer* lowerer, LowererMD &lowererMD);

        // Insert code to set generator->interpreterFrame to nullptr so that we know the
        // generator has finished executing and has no more yield points.
        // This will be inserted at the epilogue of the jitted function.
        void InsertNullOutGeneratorFrameInEpilogue(IR::LabelInstr* epilogueLabel);

        // Normally, after every bail out, we would do a jump to the epilogue and pop off the current frame.
        // However, in the case of generator, we also want to null out the interpreter frame to signal that
        // the generator is completed in the epilogue (i.e: there are no more yield points). This makes
        // jumping to the epilogue after the bailout call returns not possible because we wouldn't know if
        // the jump was because we actually want to return or because we have just bailed out.
        //
        // To deal with this, generators will have two kinds of epilogue label:
        //  - one that nulls out the generator's interpreter frame
        //  - one that doesn't
        //
        // Both of them share the register restore code, only the jump point differs:
        //
        // $Label_GeneratorEpilogueFrameNullOut: (intended for return statement)
        //    null out generator's interpreter frame
        // $Label_GeneratorEpilogueNoFrameNullOut: (intended for jumping after bailout call returns)
        //    pop ...
        //    pop ...
        //    ret
        //
        IR::LabelInstr* GetEpilogueForReturnStatements();
        IR::LabelInstr* GetEpilogueForBailOut();

        // Introduce a BailOutNoSave label if there were yield points that were elided due to optimizations.
        // They could still be hit if an active generator object had been paused at such a yield point when
        // the function body was JITed. So safe guard such a case by having the native code simply jump back
        // to the interpreter for such yield points.
        IR::LabelInstr* InsertBailOutForElidedYield();

        void LowerGeneratorResumeJumpTable(IR::Instr* jumpTableInstr);
        void LowerCreateInterpreterStackFrameForGenerator(IR::Instr* instr);
        void LowerYield(IR::Instr* instr);
        void LowerGeneratorResumeYield(IR::Instr* instr);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        void LowerGeneratorTraceBailIn(IR::Instr* instr);
#endif
    };

    LowerGeneratorHelper m_lowerGeneratorHelper;
};
