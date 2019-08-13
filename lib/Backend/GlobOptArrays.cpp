//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if ENABLE_DEBUG_CONFIG_OPTIONS

#define TESTTRACE_PHASE_INSTR(phase, instr, ...) \
    if(PHASE_TESTTRACE(phase, this->func)) \
    { \
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE]; \
        Output::Print( \
            _u("Testtrace: %s function %s (%s): "), \
            Js::PhaseNames[phase], \
            instr->m_func->GetJITFunctionBody()->GetDisplayName(), \
            instr->m_func->GetDebugNumberSet(debugStringBuffer)); \
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    }

#else

#define TESTTRACE_PHASE_INSTR(phase, instr, ...)

#endif

#if ENABLE_DEBUG_CONFIG_OPTIONS && DBG_DUMP

#define TRACE_TESTTRACE_PHASE_INSTR(phase, instr, ...) \
    TRACE_PHASE_INSTR(phase, instr, __VA_ARGS__); \
    TESTTRACE_PHASE_INSTR(phase, instr, __VA_ARGS__);

#else

#define TRACE_TESTTRACE_PHASE_INSTR(phase, instr, ...) TESTTRACE_PHASE_INSTR(phase, instr, __VA_ARGS__);

#endif

GlobOpt::ArraySrcOpt::~ArraySrcOpt()
{
    if (originalIndexOpnd != nullptr)
    {
        Assert(instr->m_opcode == Js::OpCode::IsIn);
        instr->ReplaceSrc1(originalIndexOpnd);
    }
}


bool GlobOpt::ArraySrcOpt::CheckOpCode()
{
    switch (instr->m_opcode)
    {
        // SIMD_JS
        case Js::OpCode::Simd128_LdArr_F4:
        case Js::OpCode::Simd128_LdArr_I4:
            // no type-spec for Asm.js
            if (globOpt->GetIsAsmJSFunc())
            {
                return false;
            }
            // fall through

        case Js::OpCode::LdElemI_A:
        case Js::OpCode::LdMethodElem:
            if (!instr->GetSrc1()->IsIndirOpnd())
            {
                return false;
            }

            baseOwnerIndir = instr->GetSrc1()->AsIndirOpnd();
            baseOpnd = baseOwnerIndir->GetBaseOpnd();

            // LdMethodElem is currently not profiled
            isProfilableLdElem = instr->m_opcode != Js::OpCode::LdMethodElem;
            needsBoundChecks = true;
            needsHeadSegmentLength = true;
            needsHeadSegment = true;
            isLoad = true;
            break;

            // SIMD_JS
        case Js::OpCode::Simd128_StArr_F4:
        case Js::OpCode::Simd128_StArr_I4:
            // no type-spec for Asm.js
            if (globOpt->GetIsAsmJSFunc())
            {
                return false;
            }
            // fall through

        case Js::OpCode::StElemI_A:
        case Js::OpCode::StElemI_A_Strict:
        case Js::OpCode::StElemC:
            if (!instr->GetDst()->IsIndirOpnd())
            {
                return false;
            }

            baseOwnerIndir = instr->GetDst()->AsIndirOpnd();
            baseOpnd = baseOwnerIndir->GetBaseOpnd();

            isProfilableStElem = instr->m_opcode != Js::OpCode::StElemC;
            needsBoundChecks = isProfilableStElem;
            needsHeadSegmentLength = true;
            needsHeadSegment = true;
            isStore = true;
            break;

        case Js::OpCode::InlineArrayPush:
        case Js::OpCode::InlineArrayPop:
        {
            IR::Opnd * thisOpnd = instr->GetSrc1();

            // Abort if it not a LikelyArray or Object with Array - No point in doing array check elimination.
            if (!thisOpnd->IsRegOpnd() || !thisOpnd->GetValueType().IsLikelyArrayOrObjectWithArray())
            {
                return false;
            }

            baseOwnerInstr = instr;
            baseOpnd = thisOpnd->AsRegOpnd();

            isLoad = instr->m_opcode == Js::OpCode::InlineArrayPop;
            isStore = instr->m_opcode == Js::OpCode::InlineArrayPush;
            needsLength = true;
            needsHeadSegmentLength = true;
            needsHeadSegment = true;
            break;
        }

        case Js::OpCode::LdLen_A:
            if (!instr->GetSrc1()->IsRegOpnd())
            {
                return false;
            }

            baseOpnd = instr->GetSrc1()->AsRegOpnd();
            if (baseOpnd->GetValueType().IsLikelyObject() && baseOpnd->GetValueType().GetObjectType() == ObjectType::ObjectWithArray)
            {
                return false;
            }

            baseOwnerInstr = instr;
            needsLength = true;
            break;

        case Js::OpCode::IsIn:
            if (!globOpt->DoArrayMissingValueCheckHoist())
            {
                return false;
            }

            if (!instr->GetSrc1()->IsRegOpnd() && !instr->GetSrc1()->IsIntConstOpnd())
            {
                return false;
            }

            if (!instr->GetSrc2()->IsRegOpnd())
            {
                return false;
            }

            baseOpnd = instr->GetSrc2()->AsRegOpnd();
            if (baseOpnd->GetValueType().IsLikelyObject() && baseOpnd->GetValueType().GetObjectType() == ObjectType::ObjectWithArray)
            {
                return false;
            }

            if (!baseOpnd->GetValueType().IsLikelyAnyArray() || (baseOpnd->GetValueType().IsLikelyArrayOrObjectWithArray() && !baseOpnd->GetValueType().HasNoMissingValues()))
            {
                return false;
            }

            baseOwnerInstr = instr;

            needsBoundChecks = true;
            needsHeadSegmentLength = true;
            needsHeadSegment = true;
            break;

        default:
            return false;
    }

    return true;
}

void GlobOpt::ArraySrcOpt::TypeSpecIndex()
{
    // Since this happens before type specialization, make sure that any necessary conversions are done, and that the index is int-specialized if possible such that the const flags are correct.
    if (!globOpt->IsLoopPrePass())
    {
        if (baseOwnerIndir)
        {
            globOpt->ToVarUses(instr, baseOwnerIndir, baseOwnerIndir == instr->GetDst(), nullptr);
        }
        else if (instr->m_opcode == Js::OpCode::IsIn && instr->GetSrc1()->IsRegOpnd())
        {
            // If the optimization is unable to eliminate the bounds checks, we need to restore the original var sym.
            Assert(originalIndexOpnd == nullptr);
            originalIndexOpnd = instr->GetSrc1()->Copy(func)->AsRegOpnd();
            globOpt->ToTypeSpecIndex(instr, instr->GetSrc1()->AsRegOpnd(), nullptr);
        }
    }

    if (baseOwnerIndir != nullptr)
    {
        indexOpnd = baseOwnerIndir->GetIndexOpnd();
    }
    else if (instr->m_opcode == Js::OpCode::IsIn)
    {
        indexOpnd = instr->GetSrc1();
    }

    if (indexOpnd != nullptr && indexOpnd->IsRegOpnd())
    {
        IR::RegOpnd * regOpnd = indexOpnd->AsRegOpnd();
        if (regOpnd->m_sym->IsTypeSpec())
        {
            Assert(regOpnd->m_sym->IsInt32());
            indexVarSym = regOpnd->m_sym->GetVarEquivSym(nullptr);
        }
        else
        {
            indexVarSym = regOpnd->m_sym;
        }

        indexValue = globOpt->CurrentBlockData()->FindValue(indexVarSym);
    }
}

void GlobOpt::ArraySrcOpt::UpdateValue(StackSym * newHeadSegmentSym, StackSym * newHeadSegmentLengthSym, StackSym * newLengthSym)
{
    Assert(baseValueType.GetObjectType() == newBaseValueType.GetObjectType());
    Assert(newBaseValueType.IsObject());
    Assert(baseValueType.IsLikelyArray() || !newLengthSym);

    if (!(newHeadSegmentSym || newHeadSegmentLengthSym || newLengthSym))
    {
        // We're not adding new information to the value other than changing the value type. Preserve any existing
        // information and just change the value type.
        globOpt->ChangeValueType(globOpt->currentBlock, baseValue, newBaseValueType, true);
        return;
    }

    // Merge the new syms into the value while preserving any existing information, and change the value type
    if (baseArrayValueInfo)
    {
        if (!newHeadSegmentSym)
        {
            newHeadSegmentSym = baseArrayValueInfo->HeadSegmentSym();
        }

        if (!newHeadSegmentLengthSym)
        {
            newHeadSegmentLengthSym = baseArrayValueInfo->HeadSegmentLengthSym();
        }

        if (!newLengthSym)
        {
            newLengthSym = baseArrayValueInfo->LengthSym();
        }

        Assert(!baseArrayValueInfo->HeadSegmentSym() || newHeadSegmentSym == baseArrayValueInfo->HeadSegmentSym());
        Assert(!baseArrayValueInfo->HeadSegmentLengthSym() || newHeadSegmentLengthSym == baseArrayValueInfo->HeadSegmentLengthSym());
        Assert(!baseArrayValueInfo->LengthSym() || newLengthSym == baseArrayValueInfo->LengthSym());
    }

    ArrayValueInfo *const newBaseArrayValueInfo =
        ArrayValueInfo::New(
            globOpt->alloc,
            newBaseValueType,
            newHeadSegmentSym,
            newHeadSegmentLengthSym,
            newLengthSym,
            baseValueInfo->GetSymStore());

    globOpt->ChangeValueInfo(globOpt->currentBlock, baseValue, newBaseArrayValueInfo);
};

void GlobOpt::ArraySrcOpt::CheckVirtualArrayBounds()
{
#if ENABLE_FAST_ARRAYBUFFER
    if (baseValueType.IsLikelyOptimizedVirtualTypedArray() && !Js::IsSimd128LoadStore(instr->m_opcode) /*Always extract bounds for SIMD */)
    {
        if (isProfilableStElem ||
            !instr->IsDstNotAlwaysConvertedToInt32() ||
            ((baseValueType.GetObjectType() == ObjectType::Float32VirtualArray ||
                baseValueType.GetObjectType() == ObjectType::Float64VirtualArray) &&
                !instr->IsDstNotAlwaysConvertedToNumber()
                )
            )
        {
            // Unless we're in asm.js (where it is guaranteed that virtual typed array accesses cannot read/write beyond 4GB),
            // check the range of the index to make sure we won't access beyond the reserved memory beforing eliminating bounds
            // checks in jitted code.
            if (!globOpt->GetIsAsmJSFunc() && baseOwnerIndir)
            {
                if (indexOpnd)
                {
                    IntConstantBounds idxConstantBounds;
                    if (indexValue && indexValue->GetValueInfo()->TryGetIntConstantBounds(&idxConstantBounds))
                    {
                        BYTE indirScale = Lowerer::GetArrayIndirScale(baseValueType);
                        int32 upperBound = idxConstantBounds.UpperBound();
                        int32 lowerBound = idxConstantBounds.LowerBound();
                        if (lowerBound >= 0 && ((static_cast<uint64>(upperBound) << indirScale) < MAX_ASMJS_ARRAYBUFFER_LENGTH))
                        {
                            eliminatedLowerBoundCheck = true;
                            eliminatedUpperBoundCheck = true;
                            canBailOutOnArrayAccessHelperCall = false;
                        }
                    }
                }
            }
            else
            {
                if (baseOwnerIndir == nullptr)
                {
                    Assert(instr->m_opcode == Js::OpCode::InlineArrayPush ||
                        instr->m_opcode == Js::OpCode::InlineArrayPop ||
                        instr->m_opcode == Js::OpCode::LdLen_A ||
                        instr->m_opcode == Js::OpCode::IsIn);
                }

                eliminatedLowerBoundCheck = true;
                eliminatedUpperBoundCheck = true;
                canBailOutOnArrayAccessHelperCall = false;
            }
        }
    }
#endif
}

void GlobOpt::ArraySrcOpt::TryEliminiteBoundsCheck()
{
    AnalysisAssert(indexOpnd != nullptr || baseOwnerIndir != nullptr);
    Assert(needsHeadSegmentLength);

    // Bound checks can be separated from the instruction only if it can bail out instead of making a helper call when a
    // bound check fails. And only if it would bail out, can we use a bound check to eliminate redundant bound checks later
    // on that path.
    doExtractBoundChecks = (headSegmentLengthIsAvailable || doHeadSegmentLengthLoad) && canBailOutOnArrayAccessHelperCall;

    // Get the index value
    if (indexOpnd != nullptr && indexOpnd->IsRegOpnd())
    {
        if (indexOpnd->AsRegOpnd()->m_sym->IsTypeSpec())
        {
            Assert(indexVarSym);
            Assert(indexValue);
            AssertVerify(indexValue->GetValueInfo()->TryGetIntConstantBounds(&indexConstantBounds));
            Assert(indexOpnd->GetType() == TyInt32 || indexOpnd->GetType() == TyUint32);
            Assert(
                (indexOpnd->GetType() == TyUint32) ==
                ValueInfo::IsGreaterThanOrEqualTo(
                    indexValue,
                    indexConstantBounds.LowerBound(),
                    indexConstantBounds.UpperBound(),
                    nullptr,
                    0,
                    0));

            if (indexOpnd->GetType() == TyUint32)
            {
                eliminatedLowerBoundCheck = true;
            }
        }
        else
        {
            doExtractBoundChecks = false; // Bound check instruction operates only on int-specialized operands
            if (!indexValue || !indexValue->GetValueInfo()->TryGetIntConstantBounds(&indexConstantBounds))
            {
                return;
            }

            if (ValueInfo::IsGreaterThanOrEqualTo(
                indexValue,
                indexConstantBounds.LowerBound(),
                indexConstantBounds.UpperBound(),
                nullptr,
                0,
                0))
            {
                eliminatedLowerBoundCheck = true;
            }
        }
        if (!eliminatedLowerBoundCheck &&
            ValueInfo::IsLessThan(
                indexValue,
                indexConstantBounds.LowerBound(),
                indexConstantBounds.UpperBound(),
                nullptr,
                0,
                0))
        {
            eliminatedUpperBoundCheck = true;
            doExtractBoundChecks = false;
            return;
        }
    }
    else
    {
        const int32 indexConstantValue = indexOpnd ? indexOpnd->AsIntConstOpnd()->AsInt32() : baseOwnerIndir->GetOffset();
        if (indexConstantValue < 0)
        {
            eliminatedUpperBoundCheck = true;
            doExtractBoundChecks = false;
            return;
        }

        if (indexConstantValue == INT32_MAX)
        {
            eliminatedLowerBoundCheck = true;
            doExtractBoundChecks = false;
            return;
        }

        indexConstantBounds = IntConstantBounds(indexConstantValue, indexConstantValue);
        eliminatedLowerBoundCheck = true;
    }

    if (!headSegmentLengthIsAvailable)
    {
        return;
    }

    headSegmentLengthValue = globOpt->CurrentBlockData()->FindValue(baseArrayValueInfo->HeadSegmentLengthSym());
    if (!headSegmentLengthValue)
    {
        if (doExtractBoundChecks)
        {
            headSegmentLengthConstantBounds = IntConstantBounds(0, Js::SparseArraySegmentBase::MaxLength);
        }

        return;
    }

    AssertVerify(headSegmentLengthValue->GetValueInfo()->TryGetIntConstantBounds(&headSegmentLengthConstantBounds));

    if (ValueInfo::IsLessThanOrEqualTo(
        indexValue,
        indexConstantBounds.LowerBound(),
        indexConstantBounds.UpperBound(),
        headSegmentLengthValue,
        headSegmentLengthConstantBounds.LowerBound(),
        headSegmentLengthConstantBounds.UpperBound(),
        -1
    ))
    {
        eliminatedUpperBoundCheck = true;
        if (eliminatedLowerBoundCheck)
        {
            doExtractBoundChecks = false;
        }
    }
}

void GlobOpt::ArraySrcOpt::CheckLoops()
{
    if (!doArrayChecks && !doHeadSegmentLoad && !doHeadSegmentLengthLoad && !doLengthLoad)
    {
        return;
    }

    // Find the loops out of which array checks and head segment loads need to be hoisted
    for (Loop *loop = globOpt->currentBlock->loop; loop; loop = loop->parent)
    {
        const JsArrayKills loopKills(loop->jsArrayKills);
        Value *baseValueInLoopLandingPad = nullptr;
        if (((isLikelyJsArray || isLikelyVirtualTypedArray) && loopKills.KillsValueType(newBaseValueType)) ||
            !globOpt->OptIsInvariant(baseOpnd->m_sym, globOpt->currentBlock, loop, baseValue, true, true, &baseValueInLoopLandingPad) ||
            !(doArrayChecks || baseValueInLoopLandingPad->GetValueInfo()->IsObject()))
        {
            break;
        }

        // The value types should be the same, except:
        //     - The value type in the landing pad is a type that can merge to a specific object type. Typically, these
        //       cases will use BailOnNoProfile, but that can be disabled due to excessive bailouts. Those value types
        //       merge aggressively to the other side's object type, so the value type may have started off as
        //       Uninitialized, [Likely]Undefined|Null, [Likely]UninitializedObject, etc., and changed in the loop to an
        //       array type during a prepass.
        //     - StElems in the loop can kill the no-missing-values info.
        //     - The native array type may be made more conservative based on profile data by an instruction in the loop.
#if DBG
        if (!baseValueInLoopLandingPad->GetValueInfo()->CanMergeToSpecificObjectType())
        {
            ValueType landingPadValueType = baseValueInLoopLandingPad->GetValueInfo()->Type();
            Assert(landingPadValueType.IsSimilar(baseValueType)
                || (landingPadValueType.IsLikelyNativeArray() && landingPadValueType.Merge(baseValueType).IsSimilar(baseValueType))
                || (baseValueType.IsLikelyNativeArray() && baseValueType.Merge(landingPadValueType).IsSimilar(landingPadValueType))
            );
        }
#endif

        if (doArrayChecks)
        {
            hoistChecksOutOfLoop = loop;
        }

        if (isLikelyJsArray && loopKills.KillsArrayHeadSegments())
        {
            Assert(loopKills.KillsArrayHeadSegmentLengths());
            if (!(doArrayChecks || doLengthLoad))
            {
                break;
            }
        }
        else
        {
            if (doHeadSegmentLoad || headSegmentIsAvailable)
            {
                // If the head segment is already available, we may need to rehoist the value including other
                // information. So, need to track the loop out of which the head segment length can be hoisted even if
                // the head segment length is not being loaded here.
                hoistHeadSegmentLoadOutOfLoop = loop;
            }

            if (isLikelyJsArray
                ? loopKills.KillsArrayHeadSegmentLengths()
                : loopKills.KillsTypedArrayHeadSegmentLengths())
            {
                if (!(doArrayChecks || doHeadSegmentLoad || doLengthLoad))
                {
                    break;
                }
            }
            else if (doHeadSegmentLengthLoad || headSegmentLengthIsAvailable)
            {
                // If the head segment length is already available, we may need to rehoist the value including other
                // information. So, need to track the loop out of which the head segment length can be hoisted even if
                // the head segment length is not being loaded here.
                hoistHeadSegmentLengthLoadOutOfLoop = loop;
            }
        }

        if (isLikelyJsArray && loopKills.KillsArrayLengths())
        {
            if (!(doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad))
            {
                break;
            }
        }
        else if (doLengthLoad || lengthIsAvailable)
        {
            // If the length is already available, we may need to rehoist the value including other information. So,
            // need to track the loop out of which the head segment length can be hoisted even if the length is not
            // being loaded here.
            hoistLengthLoadOutOfLoop = loop;
        }
    }
}

void GlobOpt::ArraySrcOpt::DoArrayChecks()
{
    TRACE_TESTTRACE_PHASE_INSTR(Js::ArrayCheckHoistPhase, instr, _u("Separating array checks with bailout\n"));

    IR::Instr *bailOnNotArray = IR::Instr::New(Js::OpCode::BailOnNotArray, instr->m_func);
    bailOnNotArray->SetSrc1(baseOpnd);
    bailOnNotArray->GetSrc1()->SetIsJITOptimizedReg(true);
    const IR::BailOutKind bailOutKind = newBaseValueType.IsLikelyNativeArray() ? IR::BailOutOnNotNativeArray : IR::BailOutOnNotArray;

    if (hoistChecksOutOfLoop)
    {
        Assert(!(isLikelyJsArray && hoistChecksOutOfLoop->jsArrayKills.KillsValueType(newBaseValueType)));

        TRACE_PHASE_INSTR(
            Js::ArrayCheckHoistPhase,
            instr,
            _u("Hoisting array checks with bailout out of loop %u to landing pad block %u\n"),
            hoistChecksOutOfLoop->GetLoopNumber(),
            hoistChecksOutOfLoop->landingPad->GetBlockNum());
        TESTTRACE_PHASE_INSTR(Js::ArrayCheckHoistPhase, instr, _u("Hoisting array checks with bailout out of loop\n"));

        Assert(hoistChecksOutOfLoop->bailOutInfo);
        globOpt->EnsureBailTarget(hoistChecksOutOfLoop);
        InsertInstrInLandingPad(bailOnNotArray, hoistChecksOutOfLoop);
        bailOnNotArray = bailOnNotArray->ConvertToBailOutInstr(hoistChecksOutOfLoop->bailOutInfo, bailOutKind);
    }
    else
    {
        bailOnNotArray->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(bailOnNotArray);
        globOpt->GenerateBailAtOperation(&bailOnNotArray, bailOutKind);
        shareableBailOutInfo = bailOnNotArray->GetBailOutInfo();
        shareableBailOutInfoOriginalOwner = bailOnNotArray;
    }

    baseValueType = newBaseValueType;
    baseOpnd->SetValueType(newBaseValueType);
}

void GlobOpt::ArraySrcOpt::DoLengthLoad()
{
    Assert(baseValueType.IsArray());
    Assert(newLengthSym);

    TRACE_TESTTRACE_PHASE_INSTR(Js::Phase::ArrayLengthHoistPhase, instr, _u("Separating array length load\n"));

    // Create an initial value for the length
    globOpt->CurrentBlockData()->liveVarSyms->Set(newLengthSym->m_id);
    Value *const lengthValue = globOpt->NewIntRangeValue(0, INT32_MAX, false);
    globOpt->CurrentBlockData()->SetValue(lengthValue, newLengthSym);

    // SetValue above would have set the sym store to newLengthSym. This sym won't be used for copy-prop though, so
    // remove it as the sym store.
    globOpt->SetSymStoreDirect(lengthValue->GetValueInfo(), nullptr);

    // length = [array + offsetOf(length)]
    IR::Instr *const loadLength =
        IR::Instr::New(
            Js::OpCode::LdIndir,
            IR::RegOpnd::New(newLengthSym, newLengthSym->GetType(), instr->m_func),
            IR::IndirOpnd::New(
                baseOpnd,
                Js::JavascriptArray::GetOffsetOfLength(),
                newLengthSym->GetType(),
                instr->m_func),
            instr->m_func);

    loadLength->GetDst()->SetIsJITOptimizedReg(true);
    loadLength->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->SetIsJITOptimizedReg(true);

    // BailOnNegative length (BailOutOnIrregularLength)
    IR::Instr *bailOnIrregularLength = IR::Instr::New(Js::OpCode::BailOnNegative, instr->m_func);
    bailOnIrregularLength->SetSrc1(loadLength->GetDst());

    const IR::BailOutKind bailOutKind = IR::BailOutOnIrregularLength;
    if (hoistLengthLoadOutOfLoop)
    {
        Assert(!hoistLengthLoadOutOfLoop->jsArrayKills.KillsArrayLengths());

        TRACE_PHASE_INSTR(
            Js::Phase::ArrayLengthHoistPhase,
            instr,
            _u("Hoisting array length load out of loop %u to landing pad block %u\n"),
            hoistLengthLoadOutOfLoop->GetLoopNumber(),
            hoistLengthLoadOutOfLoop->landingPad->GetBlockNum());
        TESTTRACE_PHASE_INSTR(Js::Phase::ArrayLengthHoistPhase, instr, _u("Hoisting array length load out of loop\n"));

        Assert(hoistLengthLoadOutOfLoop->bailOutInfo);
        globOpt->EnsureBailTarget(hoistLengthLoadOutOfLoop);
        InsertInstrInLandingPad(loadLength, hoistLengthLoadOutOfLoop);
        InsertInstrInLandingPad(bailOnIrregularLength, hoistLengthLoadOutOfLoop);
        bailOnIrregularLength = bailOnIrregularLength->ConvertToBailOutInstr(hoistLengthLoadOutOfLoop->bailOutInfo, bailOutKind);

        // Hoist the length value
        for (InvariantBlockBackwardIterator it(
                globOpt,
                globOpt->currentBlock,
                hoistLengthLoadOutOfLoop->landingPad,
                baseOpnd->m_sym,
                baseValue->GetValueNumber());
            it.IsValid();
            it.MoveNext())
        {
            BasicBlock *const block = it.Block();
            block->globOptData.liveVarSyms->Set(newLengthSym->m_id);
            Assert(!block->globOptData.FindValue(newLengthSym));
            Value *const lengthValueCopy = globOpt->CopyValue(lengthValue, lengthValue->GetValueNumber());
            block->globOptData.SetValue(lengthValueCopy, newLengthSym);
            globOpt->SetSymStoreDirect(lengthValueCopy->GetValueInfo(), nullptr);
        }
    }
    else
    {
        loadLength->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(loadLength);
        bailOnIrregularLength->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(bailOnIrregularLength);
        if (shareableBailOutInfo)
        {
            ShareBailOut();
            bailOnIrregularLength = bailOnIrregularLength->ConvertToBailOutInstr(shareableBailOutInfo, bailOutKind);
        }
        else
        {
            globOpt->GenerateBailAtOperation(&bailOnIrregularLength, bailOutKind);
            shareableBailOutInfo = bailOnIrregularLength->GetBailOutInfo();
            shareableBailOutInfoOriginalOwner = bailOnIrregularLength;
        }
    }
}

void GlobOpt::ArraySrcOpt::DoHeadSegmentLengthLoad()
{
    Assert(!isLikelyJsArray || newHeadSegmentSym || baseArrayValueInfo && baseArrayValueInfo->HeadSegmentSym());
    Assert(newHeadSegmentLengthSym);
    Assert(!headSegmentLengthValue);

    TRACE_TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Separating array segment length load\n"));

    // Create an initial value for the head segment length
    globOpt->CurrentBlockData()->liveVarSyms->Set(newHeadSegmentLengthSym->m_id);
    headSegmentLengthValue = globOpt->NewIntRangeValue(0, Js::SparseArraySegmentBase::MaxLength, false);
    headSegmentLengthConstantBounds = IntConstantBounds(0, Js::SparseArraySegmentBase::MaxLength);
    globOpt->CurrentBlockData()->SetValue(headSegmentLengthValue, newHeadSegmentLengthSym);

    // SetValue above would have set the sym store to newHeadSegmentLengthSym. This sym won't be used for copy-prop
    // though, so remove it as the sym store.
    globOpt->SetSymStoreDirect(headSegmentLengthValue->GetValueInfo(), nullptr);

    StackSym *const headSegmentSym = isLikelyJsArray ? newHeadSegmentSym ? newHeadSegmentSym : baseArrayValueInfo->HeadSegmentSym() : nullptr;

    IR::Instr *const loadHeadSegmentLength =
        IR::Instr::New(
            Js::OpCode::LdIndir,
            IR::RegOpnd::New(newHeadSegmentLengthSym, newHeadSegmentLengthSym->GetType(), instr->m_func),
            IR::IndirOpnd::New(
                isLikelyJsArray ? IR::RegOpnd::New(headSegmentSym, headSegmentSym->GetType(), instr->m_func) : baseOpnd,
                isLikelyJsArray
                ? Js::SparseArraySegmentBase::GetOffsetOfLength()
                : Lowerer::GetArrayOffsetOfLength(baseValueType),
                newHeadSegmentLengthSym->GetType(),
                instr->m_func),
            instr->m_func);

    loadHeadSegmentLength->GetDst()->SetIsJITOptimizedReg(true);
    loadHeadSegmentLength->GetSrc1()->AsIndirOpnd()->GetBaseOpnd()->SetIsJITOptimizedReg(true);

    // We don't check the head segment length for negative (very large uint32) values. For JS arrays, the bound checks
    // cover that. For typed arrays, we currently don't allocate array buffers with more than 1 GB elements.
    if (hoistHeadSegmentLengthLoadOutOfLoop)
    {
        Assert(
            !(
                isLikelyJsArray
                ? hoistHeadSegmentLengthLoadOutOfLoop->jsArrayKills.KillsArrayHeadSegmentLengths()
                : hoistHeadSegmentLengthLoadOutOfLoop->jsArrayKills.KillsTypedArrayHeadSegmentLengths()
                ));

        TRACE_PHASE_INSTR(
            Js::ArraySegmentHoistPhase,
            instr,
            _u("Hoisting array segment length load out of loop %u to landing pad block %u\n"),
            hoistHeadSegmentLengthLoadOutOfLoop->GetLoopNumber(),
            hoistHeadSegmentLengthLoadOutOfLoop->landingPad->GetBlockNum());
        TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Hoisting array segment length load out of loop\n"));

        InsertInstrInLandingPad(loadHeadSegmentLength, hoistHeadSegmentLengthLoadOutOfLoop);

        // Hoist the head segment length value
        for (InvariantBlockBackwardIterator it(
                globOpt,
                globOpt->currentBlock,
                hoistHeadSegmentLengthLoadOutOfLoop->landingPad,
                baseOpnd->m_sym,
                baseValue->GetValueNumber());
            it.IsValid();
            it.MoveNext())
        {
            BasicBlock *const block = it.Block();
            block->globOptData.liveVarSyms->Set(newHeadSegmentLengthSym->m_id);
            Assert(!block->globOptData.FindValue(newHeadSegmentLengthSym));
            Value *const headSegmentLengthValueCopy = globOpt->CopyValue(headSegmentLengthValue, headSegmentLengthValue->GetValueNumber());
            block->globOptData.SetValue(headSegmentLengthValueCopy, newHeadSegmentLengthSym);
            globOpt->SetSymStoreDirect(headSegmentLengthValueCopy->GetValueInfo(), nullptr);
        }
    }
    else
    {
        loadHeadSegmentLength->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(loadHeadSegmentLength);
        instr->loadedArrayHeadSegmentLength = true;
    }
}

void GlobOpt::ArraySrcOpt::DoExtractBoundChecks()
{
    Assert(!(eliminatedLowerBoundCheck && eliminatedUpperBoundCheck));
    Assert(baseOwnerIndir != nullptr || indexOpnd != nullptr);
    Assert(indexOpnd == nullptr || indexOpnd->IsIntConstOpnd() || indexOpnd->AsRegOpnd()->m_sym->IsTypeSpec());
    Assert(doHeadSegmentLengthLoad || headSegmentLengthIsAvailable);
    Assert(canBailOutOnArrayAccessHelperCall);
    Assert(!isStore || instr->m_opcode == Js::OpCode::StElemI_A || instr->m_opcode == Js::OpCode::StElemI_A_Strict || Js::IsSimd128LoadStore(instr->m_opcode));

    headSegmentLengthSym = headSegmentLengthIsAvailable ? baseArrayValueInfo->HeadSegmentLengthSym() : newHeadSegmentLengthSym;
    Assert(headSegmentLengthSym);
    Assert(headSegmentLengthValue);

    if (globOpt->DoBoundCheckHoist())
    {
        if (indexVarSym)
        {
            TRACE_PHASE_INSTR_VERBOSE(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Determining array bound check hoistability for index s%u\n"),
                indexVarSym->m_id);
        }
        else
        {
            TRACE_PHASE_INSTR_VERBOSE(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Determining array bound check hoistability for index %d\n"),
                indexConstantBounds.LowerBound());
        }

        globOpt->DetermineArrayBoundCheckHoistability(
            !eliminatedLowerBoundCheck,
            !eliminatedUpperBoundCheck,
            lowerBoundCheckHoistInfo,
            upperBoundCheckHoistInfo,
            isLikelyJsArray,
            indexVarSym,
            indexValue,
            indexConstantBounds,
            headSegmentLengthSym,
            headSegmentLengthValue,
            headSegmentLengthConstantBounds,
            hoistHeadSegmentLengthLoadOutOfLoop,
            failedToUpdateCompatibleLowerBoundCheck,
            failedToUpdateCompatibleUpperBoundCheck);
    }

    if (!eliminatedLowerBoundCheck)
    {
        DoLowerBoundCheck();
    }

    if (!eliminatedUpperBoundCheck)
    {
        DoUpperBoundCheck();
    }
}

void GlobOpt::ArraySrcOpt::DoLowerBoundCheck()
{
    eliminatedLowerBoundCheck = true;

    Assert(indexVarSym);
    Assert(indexOpnd);
    Assert(indexValue);

    GlobOpt::ArrayLowerBoundCheckHoistInfo &hoistInfo = lowerBoundCheckHoistInfo;
    if (hoistInfo.HasAnyInfo())
    {
        BasicBlock *hoistBlock;
        if (hoistInfo.CompatibleBoundCheckBlock())
        {
            hoistBlock = hoistInfo.CompatibleBoundCheckBlock();

            TRACE_PHASE_INSTR(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Hoisting array lower bound check into existing bound check instruction in block %u\n"),
                hoistBlock->GetBlockNum());
            TESTTRACE_PHASE_INSTR(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Hoisting array lower bound check into existing bound check instruction\n"));
        }
        else
        {
            Assert(hoistInfo.Loop());
            BasicBlock *const landingPad = hoistInfo.Loop()->landingPad;
            hoistBlock = landingPad;

            StackSym *indexIntSym;
            if (hoistInfo.IndexSym() && hoistInfo.IndexSym()->IsVar())
            {
                if (!landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()))
                {
                    // Int-specialize the index sym, as the BoundCheck instruction requires int operands. Specialize
                    // it in this block if it is invariant, as the conversion will be hoisted along with value
                    // updates.
                    BasicBlock *specializationBlock = hoistInfo.Loop()->landingPad;
                    IR::Instr *specializeBeforeInstr = nullptr;
                    if (!globOpt->CurrentBlockData()->IsInt32TypeSpecialized(hoistInfo.IndexSym()) &&
                        globOpt->OptIsInvariant(
                            hoistInfo.IndexSym(),
                            globOpt->currentBlock,
                            hoistInfo.Loop(),
                            globOpt->CurrentBlockData()->FindValue(hoistInfo.IndexSym()),
                            false,
                            true))
                    {
                        specializationBlock = globOpt->currentBlock;
                        specializeBeforeInstr = insertBeforeInstr;
                    }

                    Assert(globOpt->tempBv->IsEmpty());
                    globOpt->tempBv->Set(hoistInfo.IndexSym()->m_id);
                    globOpt->ToInt32(globOpt->tempBv, specializationBlock, false, specializeBeforeInstr);
                    globOpt->tempBv->ClearAll();
                    Assert(landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()));
                }

                indexIntSym = hoistInfo.IndexSym()->GetInt32EquivSym(nullptr);
                Assert(indexIntSym);
            }
            else
            {
                indexIntSym = hoistInfo.IndexSym();
                Assert(!indexIntSym || indexIntSym->GetType() == TyInt32 || indexIntSym->GetType() == TyUint32);
            }

            if (hoistInfo.IndexSym())
            {
                Assert(hoistInfo.Loop()->bailOutInfo);
                globOpt->EnsureBailTarget(hoistInfo.Loop());

                bool needsMagnitudeAdjustment = false;
                if (hoistInfo.LoopCount())
                {
                    // Generate the loop count and loop count based bound that will be used for the bound check
                    if (!hoistInfo.LoopCount()->HasBeenGenerated())
                    {
                        globOpt->GenerateLoopCount(hoistInfo.Loop(), hoistInfo.LoopCount());
                    }
                    needsMagnitudeAdjustment = (hoistInfo.MaxMagnitudeChange() > 0)
                        ? (hoistInfo.IndexOffset() < hoistInfo.MaxMagnitudeChange())
                        : (hoistInfo.IndexOffset() > hoistInfo.MaxMagnitudeChange());

                    globOpt->GenerateSecondaryInductionVariableBound(
                        hoistInfo.Loop(),
                        indexVarSym->GetInt32EquivSym(nullptr),
                        hoistInfo.LoopCount(),
                        hoistInfo.MaxMagnitudeChange(),
                        needsMagnitudeAdjustment,
                        hoistInfo.IndexSym());
                }

                IR::Opnd* lowerBound = IR::IntConstOpnd::New(0, TyInt32, instr->m_func, true);
                IR::Opnd* upperBound = IR::RegOpnd::New(indexIntSym, TyInt32, instr->m_func);
                int offset = needsMagnitudeAdjustment ? (hoistInfo.IndexOffset() - hoistInfo.Offset()) : hoistInfo.Offset();
                upperBound->SetIsJITOptimizedReg(true);

                // 0 <= indexSym + offset (src1 <= src2 + dst)
                IR::Instr *const boundCheck = globOpt->CreateBoundsCheckInstr(
                    lowerBound,
                    upperBound,
                    offset,
                    hoistInfo.IsLoopCountBasedBound()
                    ? IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck
                    : IR::BailOutOnFailedHoistedBoundCheck,
                    hoistInfo.Loop()->bailOutInfo,
                    hoistInfo.Loop()->bailOutInfo->bailOutFunc);

                InsertInstrInLandingPad(boundCheck, hoistInfo.Loop());

                TRACE_PHASE_INSTR(
                    Js::Phase::BoundCheckHoistPhase,
                    instr,
                    _u("Hoisting array lower bound check out of loop %u to landing pad block %u, as (0 <= s%u + %d)\n"),
                    hoistInfo.Loop()->GetLoopNumber(),
                    landingPad->GetBlockNum(),
                    hoistInfo.IndexSym()->m_id,
                    hoistInfo.Offset());
                TESTTRACE_PHASE_INSTR(
                    Js::Phase::BoundCheckHoistPhase,
                    instr,
                    _u("Hoisting array lower bound check out of loop\n"));

                // Record the bound check instruction as available
                const IntBoundCheck boundCheckInfo(
                    ZeroValueNumber,
                    hoistInfo.IndexValueNumber(),
                    boundCheck,
                    landingPad);
                {
                    const bool added = globOpt->CurrentBlockData()->availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                    Assert(added || failedToUpdateCompatibleLowerBoundCheck);
                }
                for (InvariantBlockBackwardIterator it(globOpt, globOpt->currentBlock, landingPad, nullptr);
                    it.IsValid();
                    it.MoveNext())
                {
                    const bool added = it.Block()->globOptData.availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                    Assert(added || failedToUpdateCompatibleLowerBoundCheck);
                }
            }
        }

        // Update values of the syms involved in the bound check to reflect the bound check
        if (hoistBlock != globOpt->currentBlock && hoistInfo.IndexSym() && hoistInfo.Offset() != INT32_MIN)
        {
            for (InvariantBlockBackwardIterator it(
                    globOpt,
                    globOpt->currentBlock->next,
                    hoistBlock,
                    hoistInfo.IndexSym(),
                    hoistInfo.IndexValueNumber(),
                    true);
                it.IsValid();
                it.MoveNext())
            {
                Value *const value = it.InvariantSymValue();
                IntConstantBounds constantBounds;
                AssertVerify(value->GetValueInfo()->TryGetIntConstantBounds(&constantBounds, true));

                ValueInfo *const newValueInfo =
                    globOpt->UpdateIntBoundsForGreaterThanOrEqual(
                        value,
                        constantBounds,
                        nullptr,
                        IntConstantBounds(-hoistInfo.Offset(), -hoistInfo.Offset()),
                        false);

                if (newValueInfo)
                {
                    globOpt->ChangeValueInfo(nullptr, value, newValueInfo);
                    if (it.Block() == globOpt->currentBlock && value == indexValue)
                    {
                        AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
                    }
                }
            }
        }
    }
    else
    {
        IR::Opnd* lowerBound = IR::IntConstOpnd::New(0, TyInt32, instr->m_func, true);
        IR::Opnd* upperBound = indexOpnd;
        upperBound->SetIsJITOptimizedReg(true);
        const int offset = 0;

        IR::Instr *boundCheck;
        if (shareableBailOutInfo)
        {
            ShareBailOut();
            boundCheck = globOpt->CreateBoundsCheckInstr(
                lowerBound,
                upperBound,
                offset,
                IR::BailOutOnArrayAccessHelperCall,
                shareableBailOutInfo,
                shareableBailOutInfo->bailOutFunc);
        }
        else
        {
            boundCheck = globOpt->CreateBoundsCheckInstr(
                lowerBound,
                upperBound,
                offset,
                instr->m_func);
        }


        boundCheck->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(boundCheck);
        if (!shareableBailOutInfo)
        {
            globOpt->GenerateBailAtOperation(&boundCheck, IR::BailOutOnArrayAccessHelperCall);
            shareableBailOutInfo = boundCheck->GetBailOutInfo();
            shareableBailOutInfoOriginalOwner = boundCheck;
        }

        TRACE_PHASE_INSTR(
            Js::Phase::BoundCheckEliminationPhase,
            instr,
            _u("Separating array lower bound check, as (0 <= s%u)\n"),
            indexVarSym->m_id);

        TESTTRACE_PHASE_INSTR(
            Js::Phase::BoundCheckEliminationPhase,
            instr,
            _u("Separating array lower bound check\n"));

        if (globOpt->DoBoundCheckHoist())
        {
            // Record the bound check instruction as available
            const bool added =
                globOpt->CurrentBlockData()->availableIntBoundChecks->AddNew(
                    IntBoundCheck(ZeroValueNumber, indexValue->GetValueNumber(), boundCheck, globOpt->currentBlock)) >= 0;
            Assert(added || failedToUpdateCompatibleLowerBoundCheck);
        }
    }

    // Update the index value to reflect the bound check
    ValueInfo *const newValueInfo =
        globOpt->UpdateIntBoundsForGreaterThanOrEqual(
            indexValue,
            indexConstantBounds,
            nullptr,
            IntConstantBounds(0, 0),
            false);

    if (newValueInfo)
    {
        globOpt->ChangeValueInfo(nullptr, indexValue, newValueInfo);
        AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
    }
}

void GlobOpt::ArraySrcOpt::DoUpperBoundCheck()
{
    eliminatedUpperBoundCheck = true;

    GlobOpt::ArrayUpperBoundCheckHoistInfo &hoistInfo = upperBoundCheckHoistInfo;
    if (hoistInfo.HasAnyInfo())
    {
        BasicBlock *hoistBlock;
        if (hoistInfo.CompatibleBoundCheckBlock())
        {
            hoistBlock = hoistInfo.CompatibleBoundCheckBlock();

            TRACE_PHASE_INSTR(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Hoisting array upper bound check into existing bound check instruction in block %u\n"),
                hoistBlock->GetBlockNum());

            TESTTRACE_PHASE_INSTR(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Hoisting array upper bound check into existing bound check instruction\n"));
        }
        else
        {
            Assert(hoistInfo.Loop());
            BasicBlock *const landingPad = hoistInfo.Loop()->landingPad;
            hoistBlock = landingPad;

            StackSym *indexIntSym;
            if (hoistInfo.IndexSym() && hoistInfo.IndexSym()->IsVar())
            {
                if (!landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()))
                {
                    // Int-specialize the index sym, as the BoundCheck instruction requires int operands. Specialize it
                    // in this block if it is invariant, as the conversion will be hoisted along with value updates.
                    BasicBlock *specializationBlock = hoistInfo.Loop()->landingPad;
                    IR::Instr *specializeBeforeInstr = nullptr;
                    if (!globOpt->CurrentBlockData()->IsInt32TypeSpecialized(hoistInfo.IndexSym()) &&
                        globOpt->OptIsInvariant(
                            hoistInfo.IndexSym(),
                            globOpt->currentBlock,
                            hoistInfo.Loop(),
                            globOpt->CurrentBlockData()->FindValue(hoistInfo.IndexSym()),
                            false,
                            true))
                    {
                        specializationBlock = globOpt->currentBlock;
                        specializeBeforeInstr = insertBeforeInstr;
                    }

                    Assert(globOpt->tempBv->IsEmpty());
                    globOpt->tempBv->Set(hoistInfo.IndexSym()->m_id);
                    globOpt->ToInt32(globOpt->tempBv, specializationBlock, false, specializeBeforeInstr);
                    globOpt->tempBv->ClearAll();
                    Assert(landingPad->globOptData.IsInt32TypeSpecialized(hoistInfo.IndexSym()));
                }

                indexIntSym = hoistInfo.IndexSym()->GetInt32EquivSym(nullptr);
                Assert(indexIntSym);
            }
            else
            {
                indexIntSym = hoistInfo.IndexSym();
                Assert(!indexIntSym || indexIntSym->GetType() == TyInt32 || indexIntSym->GetType() == TyUint32);
            }

            Assert(hoistInfo.Loop()->bailOutInfo);
            globOpt->EnsureBailTarget(hoistInfo.Loop());

                bool needsMagnitudeAdjustment = false;
            if (hoistInfo.LoopCount())
            {
                // Generate the loop count and loop count based bound that will be used for the bound check
                if (!hoistInfo.LoopCount()->HasBeenGenerated())
                {
                    globOpt->GenerateLoopCount(hoistInfo.Loop(), hoistInfo.LoopCount());
                }
                    needsMagnitudeAdjustment = (hoistInfo.MaxMagnitudeChange() > 0)
                        ? (hoistInfo.IndexOffset() < hoistInfo.MaxMagnitudeChange())
                        : (hoistInfo.IndexOffset() > hoistInfo.MaxMagnitudeChange());
                globOpt->GenerateSecondaryInductionVariableBound(
                    hoistInfo.Loop(),
                    indexVarSym->GetInt32EquivSym(nullptr),
                    hoistInfo.LoopCount(),
                    hoistInfo.MaxMagnitudeChange(),
                        needsMagnitudeAdjustment,
                    hoistInfo.IndexSym());
            }

            IR::Opnd* lowerBound = indexIntSym
                ? static_cast<IR::Opnd *>(IR::RegOpnd::New(indexIntSym, TyInt32, instr->m_func))
                : IR::IntConstOpnd::New(
                    hoistInfo.IndexConstantBounds().LowerBound(),
                    TyInt32,
                    instr->m_func);

            lowerBound->SetIsJITOptimizedReg(true);
            IR::Opnd* upperBound = IR::RegOpnd::New(headSegmentLengthSym, headSegmentLengthSym->GetType(), instr->m_func);
            upperBound->SetIsJITOptimizedReg(true);

                int offset = needsMagnitudeAdjustment ? (hoistInfo.IndexOffset() + hoistInfo.Offset()) : hoistInfo.Offset();

            // indexSym <= headSegmentLength + offset (src1 <= src2 + dst)
            IR::Instr *const boundCheck = globOpt->CreateBoundsCheckInstr(
                lowerBound,
                upperBound,
                    offset,
                hoistInfo.IsLoopCountBasedBound()
                ? IR::BailOutOnFailedHoistedLoopCountBasedBoundCheck
                : IR::BailOutOnFailedHoistedBoundCheck,
                hoistInfo.Loop()->bailOutInfo,
                hoistInfo.Loop()->bailOutInfo->bailOutFunc);

            InsertInstrInLandingPad(boundCheck, hoistInfo.Loop());

            if (indexIntSym)
            {
                TRACE_PHASE_INSTR(
                    Js::Phase::BoundCheckHoistPhase,
                    instr,
                    _u("Hoisting array upper bound check out of loop %u to landing pad block %u, as (s%u <= s%u + %d)\n"),
                    hoistInfo.Loop()->GetLoopNumber(),
                    landingPad->GetBlockNum(),
                    hoistInfo.IndexSym()->m_id,
                    headSegmentLengthSym->m_id,
                        offset);
            }
            else
            {
                TRACE_PHASE_INSTR(
                    Js::Phase::BoundCheckHoistPhase,
                    instr,
                    _u("Hoisting array upper bound check out of loop %u to landing pad block %u, as (%d <= s%u + %d)\n"),
                    hoistInfo.Loop()->GetLoopNumber(),
                    landingPad->GetBlockNum(),
                    hoistInfo.IndexConstantBounds().LowerBound(),
                    headSegmentLengthSym->m_id,
                        offset);
            }

            TESTTRACE_PHASE_INSTR(
                Js::Phase::BoundCheckHoistPhase,
                instr,
                _u("Hoisting array upper bound check out of loop\n"));

            // Record the bound check instruction as available
            const IntBoundCheck boundCheckInfo(
                hoistInfo.IndexValue() ? hoistInfo.IndexValueNumber() : ZeroValueNumber,
                hoistInfo.HeadSegmentLengthValue()->GetValueNumber(),
                boundCheck,
                landingPad);
            {
                const bool added = globOpt->CurrentBlockData()->availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                Assert(added || failedToUpdateCompatibleUpperBoundCheck);
            }
            for (InvariantBlockBackwardIterator it(globOpt, globOpt->currentBlock, landingPad, nullptr);
                it.IsValid();
                it.MoveNext())
            {
                const bool added = it.Block()->globOptData.availableIntBoundChecks->AddNew(boundCheckInfo) >= 0;
                Assert(added || failedToUpdateCompatibleUpperBoundCheck);
            }
        }

        // Update values of the syms involved in the bound check to reflect the bound check
        Assert(!hoistInfo.Loop() || hoistBlock != globOpt->currentBlock);
        if (hoistBlock != globOpt->currentBlock)
        {
            for (InvariantBlockBackwardIterator it(globOpt, globOpt->currentBlock->next, hoistBlock, nullptr, InvalidValueNumber, true);
                it.IsValid();
                it.MoveNext())
            {
                BasicBlock *const block = it.Block();

                Value *leftValue;
                IntConstantBounds leftConstantBounds;
                if (hoistInfo.IndexSym())
                {
                    leftValue = block->globOptData.FindValue(hoistInfo.IndexSym());
                    if (!leftValue || leftValue->GetValueNumber() != hoistInfo.IndexValueNumber())
                    {
                        continue;
                    }

                    AssertVerify(leftValue->GetValueInfo()->TryGetIntConstantBounds(&leftConstantBounds, true));
                }
                else
                {
                    leftValue = nullptr;
                    leftConstantBounds = hoistInfo.IndexConstantBounds();
                }

                Value *const rightValue = block->globOptData.FindValue(headSegmentLengthSym);
                if (!rightValue)
                {
                    continue;
                }

                Assert(rightValue->GetValueNumber() == headSegmentLengthValue->GetValueNumber());
                IntConstantBounds rightConstantBounds;
                AssertVerify(rightValue->GetValueInfo()->TryGetIntConstantBounds(&rightConstantBounds));

                ValueInfo *const newValueInfoForLessThanOrEqual =
                    globOpt->UpdateIntBoundsForLessThanOrEqual(
                        leftValue,
                        leftConstantBounds,
                        rightValue,
                        rightConstantBounds,
                        hoistInfo.Offset(),
                        false);
                if (newValueInfoForLessThanOrEqual)
                {
                    globOpt->ChangeValueInfo(nullptr, leftValue, newValueInfoForLessThanOrEqual);
                    AssertVerify(newValueInfoForLessThanOrEqual->TryGetIntConstantBounds(&leftConstantBounds, true));
                    if (block == globOpt->currentBlock && leftValue == indexValue)
                    {
                        Assert(newValueInfoForLessThanOrEqual->IsInt());
                        indexConstantBounds = leftConstantBounds;
                    }

                }
                if (hoistInfo.Offset() != INT32_MIN)
                {
                    ValueInfo *const newValueInfoForGreaterThanOrEqual =
                        globOpt->UpdateIntBoundsForGreaterThanOrEqual(
                            rightValue,
                            rightConstantBounds,
                            leftValue,
                            leftConstantBounds,
                            -hoistInfo.Offset(),
                            false);
                    if (newValueInfoForGreaterThanOrEqual)
                    {
                        globOpt->ChangeValueInfo(nullptr, rightValue, newValueInfoForGreaterThanOrEqual);
                        if (block == globOpt->currentBlock)
                        {
                            Assert(rightValue == headSegmentLengthValue);
                            AssertVerify(newValueInfoForGreaterThanOrEqual->TryGetIntConstantBounds(&headSegmentLengthConstantBounds));
                        }
                    }
                }
            }
        }
    }
    else
    {
        IR::Opnd * lowerBound = indexOpnd ? indexOpnd : IR::IntConstOpnd::New(baseOwnerIndir->GetOffset(), TyInt32, instr->m_func);

        lowerBound->SetIsJITOptimizedReg(true);
        IR::Opnd* upperBound = IR::RegOpnd::New(headSegmentLengthSym, headSegmentLengthSym->GetType(), instr->m_func);
        upperBound->SetIsJITOptimizedReg(true);
        const int offset = -1;
        IR::Instr *boundCheck;

        // index <= headSegmentLength - 1 (src1 <= src2 + dst)
        if (shareableBailOutInfo)
        {
            ShareBailOut();
            boundCheck = globOpt->CreateBoundsCheckInstr(
                lowerBound,
                upperBound,
                offset,
                IR::BailOutOnArrayAccessHelperCall,
                shareableBailOutInfo,
                shareableBailOutInfo->bailOutFunc);
        }
        else
        {
            boundCheck = globOpt->CreateBoundsCheckInstr(
                lowerBound,
                upperBound,
                offset,
                instr->m_func);
        }

        boundCheck->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(boundCheck);
        if (!shareableBailOutInfo)
        {
            globOpt->GenerateBailAtOperation(&boundCheck, IR::BailOutOnArrayAccessHelperCall);
            shareableBailOutInfo = boundCheck->GetBailOutInfo();
            shareableBailOutInfoOriginalOwner = boundCheck;
        }

        instr->extractedUpperBoundCheckWithoutHoisting = true;

        if (indexOpnd != nullptr && indexOpnd->IsRegOpnd())
        {
            TRACE_PHASE_INSTR(
                Js::Phase::BoundCheckEliminationPhase,
                instr,
                _u("Separating array upper bound check, as (s%u < s%u)\n"),
                indexVarSym->m_id,
                headSegmentLengthSym->m_id);
        }
        else
        {
            TRACE_PHASE_INSTR(
                Js::Phase::BoundCheckEliminationPhase,
                instr,
                _u("Separating array upper bound check, as (%d < s%u)\n"),
                indexOpnd ? indexOpnd->AsIntConstOpnd()->AsInt32() : baseOwnerIndir->GetOffset(),
                headSegmentLengthSym->m_id);
        }
        TESTTRACE_PHASE_INSTR(
            Js::Phase::BoundCheckEliminationPhase,
            instr,
            _u("Separating array upper bound check\n"));

        if (globOpt->DoBoundCheckHoist())
        {
            // Record the bound check instruction as available
            const bool added =
                globOpt->CurrentBlockData()->availableIntBoundChecks->AddNew(
                    IntBoundCheck(
                        indexValue ? indexValue->GetValueNumber() : ZeroValueNumber,
                        headSegmentLengthValue->GetValueNumber(),
                        boundCheck,
                        globOpt->currentBlock)) >= 0;
            Assert(added || failedToUpdateCompatibleUpperBoundCheck);
        }
    }

    // Update the index and head segment length values to reflect the bound check
    ValueInfo *newValueInfo =
        globOpt->UpdateIntBoundsForLessThan(
            indexValue,
            indexConstantBounds,
            headSegmentLengthValue,
            headSegmentLengthConstantBounds,
            false);

    if (newValueInfo)
    {
        globOpt->ChangeValueInfo(nullptr, indexValue, newValueInfo);
        AssertVerify(newValueInfo->TryGetIntConstantBounds(&indexConstantBounds));
    }

    newValueInfo =
        globOpt->UpdateIntBoundsForGreaterThan(
            headSegmentLengthValue,
            headSegmentLengthConstantBounds,
            indexValue,
            indexConstantBounds,
            false);

    if (newValueInfo)
    {
        globOpt->ChangeValueInfo(nullptr, headSegmentLengthValue, newValueInfo);
    }
}

void GlobOpt::ArraySrcOpt::UpdateHoistedValueInfo()
{
    // Iterate up to the root loop's landing pad until all necessary value info is updated
    uint hoistItemCount =
        static_cast<uint>(!!hoistChecksOutOfLoop) +
        !!hoistHeadSegmentLoadOutOfLoop +
        !!hoistHeadSegmentLengthLoadOutOfLoop +
        !!hoistLengthLoadOutOfLoop;

    if (hoistItemCount == 0)
    {
        return;
    }

    AnalysisAssert(globOpt->currentBlock->loop != nullptr);

    Loop * rootLoop = nullptr;
    for (Loop *loop = globOpt->currentBlock->loop; loop; loop = loop->parent)
    {
        rootLoop = loop;
    }

    AnalysisAssert(rootLoop != nullptr);

    ValueInfo *valueInfoToHoist = baseValueInfo;
    bool removeHeadSegment, removeHeadSegmentLength, removeLength;
    if (baseArrayValueInfo)
    {
        removeHeadSegment = baseArrayValueInfo->HeadSegmentSym() && !hoistHeadSegmentLoadOutOfLoop;
        removeHeadSegmentLength = baseArrayValueInfo->HeadSegmentLengthSym() && !hoistHeadSegmentLengthLoadOutOfLoop;
        removeLength = baseArrayValueInfo->LengthSym() && !hoistLengthLoadOutOfLoop;
    }
    else
    {
        removeLength = removeHeadSegmentLength = removeHeadSegment = false;
    }

    for (InvariantBlockBackwardIterator it(
            globOpt,
            globOpt->currentBlock,
            rootLoop->landingPad,
            baseOpnd->m_sym,
            baseValue->GetValueNumber());
        it.IsValid();
        it.MoveNext())
    {
        if (removeHeadSegment || removeHeadSegmentLength || removeLength)
        {
            // Remove information that shouldn't be there anymore, from the value info
            valueInfoToHoist =
                valueInfoToHoist->AsArrayValueInfo()->Copy(
                    globOpt->alloc,
                    !removeHeadSegment,
                    !removeHeadSegmentLength,
                    !removeLength);
            removeLength = removeHeadSegmentLength = removeHeadSegment = false;
        }

        BasicBlock *const block = it.Block();
        Value *const blockBaseValue = it.InvariantSymValue();
        globOpt->HoistInvariantValueInfo(valueInfoToHoist, blockBaseValue, block);

        // See if we have completed hoisting value info for one of the items
        if (hoistChecksOutOfLoop && block == hoistChecksOutOfLoop->landingPad)
        {
            // All other items depend on array checks, so we can just stop here
            hoistChecksOutOfLoop = nullptr;
            break;
        }

        if (hoistHeadSegmentLoadOutOfLoop && block == hoistHeadSegmentLoadOutOfLoop->landingPad)
        {
            hoistHeadSegmentLoadOutOfLoop = nullptr;
            if (--hoistItemCount == 0)
            {
                break;
            }

            if (valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->HeadSegmentSym())
            {
                removeHeadSegment = true;
            }
        }

        if (hoistHeadSegmentLengthLoadOutOfLoop && block == hoistHeadSegmentLengthLoadOutOfLoop->landingPad)
        {
            hoistHeadSegmentLengthLoadOutOfLoop = nullptr;
            if (--hoistItemCount == 0)
            {
                break;
            }

            if (valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->HeadSegmentLengthSym())
            {
                removeHeadSegmentLength = true;
            }
        }

        if (hoistLengthLoadOutOfLoop && block == hoistLengthLoadOutOfLoop->landingPad)
        {
            hoistLengthLoadOutOfLoop = nullptr;
            if (--hoistItemCount == 0)
            {
                break;
            }

            if (valueInfoToHoist->IsArrayValueInfo() && valueInfoToHoist->AsArrayValueInfo()->LengthSym())
            {
                removeLength = true;
            }
        }
    }
}

void GlobOpt::ArraySrcOpt::InsertInstrInLandingPad(IR::Instr *const instr, Loop *const hoistOutOfLoop)
{
    if (hoistOutOfLoop->bailOutInfo->bailOutInstr)
    {
        instr->SetByteCodeOffset(hoistOutOfLoop->bailOutInfo->bailOutInstr);
        hoistOutOfLoop->bailOutInfo->bailOutInstr->InsertBefore(instr);
    }
    else
    {
        instr->SetByteCodeOffset(hoistOutOfLoop->landingPad->GetLastInstr());
        hoistOutOfLoop->landingPad->InsertAfter(instr);
    }
};

void GlobOpt::ArraySrcOpt::ShareBailOut()
{
    Assert(shareableBailOutInfo);
    if (shareableBailOutInfo->bailOutInstr != shareableBailOutInfoOriginalOwner)
    {
        return;
    }

    Assert(shareableBailOutInfoOriginalOwner->GetBailOutInfo() == shareableBailOutInfo);
    IR::Instr *const sharedBailOut = shareableBailOutInfoOriginalOwner->ShareBailOut();
    Assert(sharedBailOut->GetBailOutInfo() == shareableBailOutInfo);
    shareableBailOutInfoOriginalOwner = nullptr;
    sharedBailOut->Unlink();
    insertBeforeInstr->InsertBefore(sharedBailOut);
    insertBeforeInstr = sharedBailOut;
}

void GlobOpt::ArraySrcOpt::InsertHeadSegmentLoad()
{
    TRACE_TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Separating array segment load\n"));

    Assert(newHeadSegmentSym);
    IR::RegOpnd *const headSegmentOpnd = IR::RegOpnd::New(newHeadSegmentSym, newHeadSegmentSym->GetType(), instr->m_func);
    headSegmentOpnd->SetIsJITOptimizedReg(true);

    IR::RegOpnd *const jitOptimizedBaseOpnd = baseOpnd->Copy(instr->m_func)->AsRegOpnd();
    jitOptimizedBaseOpnd->SetIsJITOptimizedReg(true);

    IR::Instr *loadObjectArray;
    if (baseValueType.GetObjectType() == ObjectType::ObjectWithArray)
    {
        loadObjectArray =
            IR::Instr::New(
                Js::OpCode::LdIndir,
                headSegmentOpnd,
                IR::IndirOpnd::New(
                    jitOptimizedBaseOpnd,
                    Js::DynamicObject::GetOffsetOfObjectArray(),
                    jitOptimizedBaseOpnd->GetType(),
                    instr->m_func),
                instr->m_func);
    }
    else
    {
        loadObjectArray = nullptr;
    }

    IR::Instr *const loadHeadSegment =
        IR::Instr::New(
            Js::OpCode::LdIndir,
            headSegmentOpnd,
            IR::IndirOpnd::New(
                loadObjectArray ? headSegmentOpnd : jitOptimizedBaseOpnd,
                Lowerer::GetArrayOffsetOfHeadSegment(baseValueType),
                headSegmentOpnd->GetType(),
                instr->m_func),
            instr->m_func);

    if (hoistHeadSegmentLoadOutOfLoop)
    {
        Assert(!(isLikelyJsArray && hoistHeadSegmentLoadOutOfLoop->jsArrayKills.KillsArrayHeadSegments()));

        TRACE_PHASE_INSTR(
            Js::ArraySegmentHoistPhase,
            instr,
            _u("Hoisting array segment load out of loop %u to landing pad block %u\n"),
            hoistHeadSegmentLoadOutOfLoop->GetLoopNumber(),
            hoistHeadSegmentLoadOutOfLoop->landingPad->GetBlockNum());

        TESTTRACE_PHASE_INSTR(Js::ArraySegmentHoistPhase, instr, _u("Hoisting array segment load out of loop\n"));

        if (loadObjectArray)
        {
            InsertInstrInLandingPad(loadObjectArray, hoistHeadSegmentLoadOutOfLoop);
        }

        InsertInstrInLandingPad(loadHeadSegment, hoistHeadSegmentLoadOutOfLoop);
    }
    else
    {
        if (loadObjectArray)
        {
            loadObjectArray->SetByteCodeOffset(instr);
            insertBeforeInstr->InsertBefore(loadObjectArray);
        }

        loadHeadSegment->SetByteCodeOffset(instr);
        insertBeforeInstr->InsertBefore(loadHeadSegment);
        instr->loadedArrayHeadSegment = true;
    }
}

void GlobOpt::ArraySrcOpt::Optimize()
{
    if (!CheckOpCode())
    {
        return;
    }

    Assert(!(baseOwnerInstr && baseOwnerIndir));
    Assert(!needsHeadSegmentLength || needsHeadSegment);

    TypeSpecIndex();

    if (isProfilableStElem && !globOpt->IsLoopPrePass())
    {
        // If the dead-store pass decides to add the bailout kind IR::BailOutInvalidatedArrayHeadSegment, and the fast path is
        // generated, it may bail out before the operation is done, so this would need to be a pre-op bailout.
        if (instr->HasBailOutInfo())
        {
            Assert(instr->GetByteCodeOffset() != Js::Constants::NoByteCodeOffset && instr->GetBailOutInfo()->bailOutOffset <= instr->GetByteCodeOffset());

            const IR::BailOutKind bailOutKind = instr->GetBailOutKind();
            Assert(!(bailOutKind & ~IR::BailOutKindBits) || (bailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp);

            if (!(bailOutKind & ~IR::BailOutKindBits))
            {
                instr->SetBailOutKind(bailOutKind + IR::BailOutOnImplicitCallsPreOp);
            }
        }
        else
        {
            globOpt->GenerateBailAtOperation(&instr, IR::BailOutOnImplicitCallsPreOp);
        }
    }

    baseValue = globOpt->CurrentBlockData()->FindValue(baseOpnd->m_sym);
    if (baseValue == nullptr)
    {
        return;
    }

    baseValueInfo = baseValue->GetValueInfo();
    baseValueType = baseValueInfo->Type();

    baseOpnd->SetValueType(baseValueType);

    if (!baseValueType.IsLikelyAnyOptimizedArray() ||
        !globOpt->DoArrayCheckHoist(baseValueType, globOpt->currentBlock->loop, instr) ||
        (baseOwnerIndir && !globOpt->ShouldExpectConventionalArrayIndexValue(baseOwnerIndir)))
    {
        return;
    }

    isLikelyJsArray = !baseValueType.IsLikelyTypedArray();
    Assert(isLikelyJsArray == baseValueType.IsLikelyArrayOrObjectWithArray());
    Assert(!isLikelyJsArray == baseValueType.IsLikelyOptimizedTypedArray());

    if (!isLikelyJsArray && instr->m_opcode == Js::OpCode::LdMethodElem)
    {
        // Fast path is not generated in this case since the subsequent call will throw
        return;
    }

    isLikelyVirtualTypedArray = baseValueType.IsLikelyOptimizedVirtualTypedArray();
    Assert(!(isLikelyJsArray && isLikelyVirtualTypedArray));

    newBaseValueType = baseValueType.ToDefiniteObject();
    if (isLikelyJsArray && newBaseValueType.HasNoMissingValues() && !globOpt->DoArrayMissingValueCheckHoist())
    {
        newBaseValueType = newBaseValueType.SetHasNoMissingValues(false);
    }

    Assert((newBaseValueType == baseValueType) == baseValueType.IsObject());

    if (globOpt->IsLoopPrePass())
    {
        if (newBaseValueType != baseValueType)
        {
            if (globOpt->IsSafeToTransferInPrePass(baseOpnd, baseValue))
            {
                UpdateValue(nullptr, nullptr, nullptr);
            }
            else if (isLikelyJsArray && globOpt->IsOperationThatLikelyKillsJsArraysWithNoMissingValues(instr) && baseValueInfo->HasNoMissingValues())
            {
                globOpt->ChangeValueType(nullptr, baseValue, baseValueInfo->Type().SetHasNoMissingValues(false), true);
            }
        }

        // For javascript arrays and objects with javascript arrays:
        //   - Implicit calls need to be disabled and calls cannot be allowed in the loop since the array vtable may be changed
        //     into an ES5 array.
        // For typed arrays:
        //   - A typed array's array buffer may be transferred to a web worker as part of an implicit call, in which case the
        //     typed array's length is set to zero. Implicit calls need to be disabled if the typed array's head segment length
        //     is going to be loaded and used later.
        // Since we don't know if the loop has kills after this instruction, the kill information may not be complete. If a kill
        // is found later, this information will be updated to not require disabling implicit calls.
        const bool kills = isLikelyJsArray ? globOpt->rootLoopPrePass->jsArrayKills.KillsValueType(newBaseValueType) : globOpt->rootLoopPrePass->jsArrayKills.KillsTypedArrayHeadSegmentLengths();
        if (!kills)
        {
            globOpt->rootLoopPrePass->needImplicitCallBailoutChecksForJsArrayCheckHoist = true;
        }

        return;
    }

    if (baseValueInfo->IsArrayValueInfo())
    {
        baseArrayValueInfo = baseValueInfo->AsArrayValueInfo();
    }

    doArrayChecks = !baseValueType.IsObject();

    doArraySegmentHoist =
        globOpt->DoArraySegmentHoist(baseValueType) &&
        instr->m_opcode != Js::OpCode::StElemC;

    headSegmentIsAvailable =
        baseArrayValueInfo &&
        baseArrayValueInfo->HeadSegmentSym();

    doHeadSegmentLoad =
        doArraySegmentHoist &&
        needsHeadSegment && !headSegmentIsAvailable;

    doArraySegmentLengthHoist =
        doArraySegmentHoist &&
        (isLikelyJsArray || globOpt->DoTypedArraySegmentLengthHoist(globOpt->currentBlock->loop));

    headSegmentLengthIsAvailable =
        baseArrayValueInfo &&
        baseArrayValueInfo->HeadSegmentLengthSym();

    doHeadSegmentLengthLoad =
        doArraySegmentLengthHoist &&
        (needsHeadSegmentLength || (!isLikelyJsArray && needsLength)) &&
        !headSegmentLengthIsAvailable;

    lengthIsAvailable =
        baseArrayValueInfo &&
        baseArrayValueInfo->LengthSym();

    doLengthLoad =
        globOpt->DoArrayLengthHoist() &&
        needsLength &&
        !lengthIsAvailable &&
        baseValueType.IsLikelyArray() &&
        globOpt->DoLdLenIntSpec(instr->m_opcode == Js::OpCode::LdLen_A ? instr : nullptr, baseValueType);

    newHeadSegmentSym = doHeadSegmentLoad ? StackSym::New(TyMachPtr, instr->m_func) : nullptr;
    newHeadSegmentLengthSym = doHeadSegmentLengthLoad ? StackSym::New(TyUint32, instr->m_func) : nullptr;
    newLengthSym = doLengthLoad ? StackSym::New(TyUint32, instr->m_func) : nullptr;

    if (Js::IsSimd128LoadStore(instr->m_opcode) || instr->m_opcode == Js::OpCode::IsIn)
    {
        // SIMD_JS
        // simd load/store never call helper
        canBailOutOnArrayAccessHelperCall = true;
    }
    else
    {
        canBailOutOnArrayAccessHelperCall =
            (isProfilableLdElem || isProfilableStElem) &&
            globOpt->DoEliminateArrayAccessHelperCall() &&
            !(
                instr->IsProfiledInstr() &&
                (
                    isProfilableLdElem
                    ? instr->AsProfiledInstr()->u.ldElemInfo->LikelyNeedsHelperCall()
                    : instr->AsProfiledInstr()->u.stElemInfo->LikelyNeedsHelperCall()
                    )
                );
    }

    CheckVirtualArrayBounds();

    if (needsBoundChecks && globOpt->DoBoundCheckElimination())
    {
        TryEliminiteBoundsCheck();
    }

    if (doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad || doLengthLoad || doExtractBoundChecks)
    {
        CheckLoops();

        insertBeforeInstr = instr->GetInsertBeforeByteCodeUsesInstr();

        if (doArrayChecks)
        {
            DoArrayChecks();
        }

        if (doLengthLoad)
        {
            DoLengthLoad();
        }

        if (doHeadSegmentLoad && isLikelyJsArray)
        {
            // For javascript arrays, the head segment is required to load the head segment length
            InsertHeadSegmentLoad();
        }

        if (doHeadSegmentLengthLoad)
        {
            DoHeadSegmentLengthLoad();
        }

        if (doExtractBoundChecks)
        {
            DoExtractBoundChecks();
        }

        if (doHeadSegmentLoad && !isLikelyJsArray)
        {
            // For typed arrays, load the length first, followed by the bound checks, and then load the head segment. This
            // allows the length sym to become dead by the time of the head segment load, freeing up the register for use by the
            // head segment sym.
            InsertHeadSegmentLoad();
        }

        if (doArrayChecks || doHeadSegmentLoad || doHeadSegmentLengthLoad || doLengthLoad)
        {
            UpdateValue(newHeadSegmentSym, newHeadSegmentLengthSym, newLengthSym);
            baseValueInfo = baseValue->GetValueInfo();
            baseArrayValueInfo = baseValueInfo->IsArrayValueInfo() ? baseValueInfo->AsArrayValueInfo() : nullptr;

            UpdateHoistedValueInfo();
        }
    }

    IR::ArrayRegOpnd * baseArrayOpnd;
    if (baseArrayValueInfo != nullptr)
    {
        // Update the opnd to include the associated syms
        baseArrayOpnd =
            baseArrayValueInfo->CreateOpnd(
                baseOpnd,
                needsHeadSegment,
                needsHeadSegmentLength || (!isLikelyJsArray && needsLength),
                needsLength,
                eliminatedLowerBoundCheck,
                eliminatedUpperBoundCheck,
                instr->m_func);

        if (baseOwnerInstr != nullptr)
        {
            if (baseOwnerInstr->GetSrc1() == baseOpnd)
            {
                baseOwnerInstr->ReplaceSrc1(baseArrayOpnd);
            }
            else
            {
                Assert(baseOwnerInstr->GetSrc2() == baseOpnd);
                baseOwnerInstr->ReplaceSrc2(baseArrayOpnd);
            }
        }
        else
        {
            Assert(baseOwnerIndir);
            Assert(baseOwnerIndir->GetBaseOpnd() == baseOpnd);
            baseOwnerIndir->ReplaceBaseOpnd(baseArrayOpnd);
        }

        baseOpnd = baseArrayOpnd;
    }
    else
    {
        baseArrayOpnd = nullptr;
    }

    globOpt->ProcessNoImplicitCallArrayUses(baseOpnd, baseArrayOpnd, instr, isLikelyJsArray, isLoad || isStore || instr->m_opcode == Js::OpCode::IsIn);

    const auto OnEliminated = [&](const Js::Phase phase, const char *const eliminatedLoad)
    {
        TRACE_TESTTRACE_PHASE_INSTR(phase, instr, _u("Eliminating array %S\n"), eliminatedLoad);
    };

    OnEliminated(Js::Phase::ArrayCheckHoistPhase, "checks");
    if (baseArrayOpnd)
    {
        if (baseArrayOpnd->HeadSegmentSym())
        {
            OnEliminated(Js::Phase::ArraySegmentHoistPhase, "head segment load");
        }

        if (baseArrayOpnd->HeadSegmentLengthSym())
        {
            OnEliminated(Js::Phase::ArraySegmentHoistPhase, "head segment length load");
        }

        if (baseArrayOpnd->LengthSym())
        {
            OnEliminated(Js::Phase::ArrayLengthHoistPhase, "length load");
        }

        if (baseArrayOpnd->EliminatedLowerBoundCheck())
        {
            OnEliminated(Js::Phase::BoundCheckEliminationPhase, "lower bound check");
        }

        if (baseArrayOpnd->EliminatedUpperBoundCheck())
        {
            OnEliminated(Js::Phase::BoundCheckEliminationPhase, "upper bound check");
        }
    }

    if (instr->m_opcode == Js::OpCode::IsIn)
    {
        if (eliminatedLowerBoundCheck && eliminatedUpperBoundCheck)
        {
            TRACE_TESTTRACE_PHASE_INSTR(Js::Phase::BoundCheckEliminationPhase, instr, _u("Eliminating IsIn\n"));

            globOpt->CaptureByteCodeSymUses(instr);

            instr->m_opcode = Js::OpCode::Ld_A;

            IR::AddrOpnd * addrOpnd = IR::AddrOpnd::New(func->GetScriptContextInfo()->GetTrueAddr(), IR::AddrOpndKindDynamicVar, func, true);
            addrOpnd->SetValueType(ValueType::Boolean);
            instr->ReplaceSrc1(addrOpnd);
            instr->FreeSrc2();
            originalIndexOpnd->Free(func);
            originalIndexOpnd = nullptr;

            src1Val = globOpt->GetVarConstantValue(instr->GetSrc1()->AsAddrOpnd());
            src2Val = nullptr;
        }

        return;
    }

    if (!canBailOutOnArrayAccessHelperCall)
    {
        return;
    }

    // Bail out instead of generating a helper call. This helps to remove the array reference when the head segment and head
    // segment length are available, reduces code size, and allows bound checks to be separated.
    if (instr->HasBailOutInfo())
    {
        const IR::BailOutKind bailOutKind = instr->GetBailOutKind();
        Assert(
            !(bailOutKind & ~IR::BailOutKindBits) ||
            (bailOutKind & ~IR::BailOutKindBits) == IR::BailOutOnImplicitCallsPreOp);
        instr->SetBailOutKind(bailOutKind & IR::BailOutKindBits | IR::BailOutOnArrayAccessHelperCall);
    }
    else
    {
        globOpt->GenerateBailAtOperation(&instr, IR::BailOutOnArrayAccessHelperCall);
    }
}
