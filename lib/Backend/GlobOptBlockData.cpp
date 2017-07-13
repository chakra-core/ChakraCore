//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

void
GlobOptBlockData::NullOutBlockData(GlobOpt* globOpt, Func* func)
{
    this->globOpt = globOpt;

    this->symToValueMap = nullptr;
    this->exprToValueMap = nullptr;
    this->liveFields = nullptr;
    this->maybeWrittenTypeSyms = nullptr;
    this->isTempSrc = nullptr;
    this->liveVarSyms = nullptr;
    this->liveInt32Syms = nullptr;
    this->liveLossyInt32Syms = nullptr;
    this->liveFloat64Syms = nullptr;
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms = nullptr;
    this->liveSimd128I4Syms = nullptr;
#endif
    this->hoistableFields = nullptr;
    this->argObjSyms = nullptr;
    this->maybeTempObjectSyms = nullptr;
    this->canStoreTempObjectSyms = nullptr;
    this->valuesToKillOnCalls = nullptr;
    this->inductionVariables = nullptr;
    this->availableIntBoundChecks = nullptr;
    this->callSequence = nullptr;
    this->startCallCount = 0;
    this->argOutCount = 0;
    this->totalOutParamCount = 0;
    this->inlinedArgOutCount = 0;
    this->hasCSECandidates = false;
    this->curFunc = func;

    this->stackLiteralInitFldDataMap = nullptr;

    this->capturedValues = nullptr;
    this->changedSyms = nullptr;

    this->OnDataUnreferenced();
}

void
GlobOptBlockData::InitBlockData(GlobOpt* globOpt, Func* func)
{
    this->globOpt = globOpt;

    JitArenaAllocator *const alloc = this->globOpt->alloc;

    this->symToValueMap = GlobHashTable::New(alloc, 64);
    this->exprToValueMap = ExprHashTable::New(alloc, 64);
    this->liveFields = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveArrayValues = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->isTempSrc = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveVarSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveLossyInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveFloat64Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveSimd128I4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
#endif
    this->hoistableFields = nullptr;
    this->argObjSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->maybeTempObjectSyms = nullptr;
    this->canStoreTempObjectSyms = nullptr;
    this->valuesToKillOnCalls = JitAnew(alloc, ValueSet, alloc);

    if(this->globOpt->DoBoundCheckHoist())
    {
        this->inductionVariables = this->globOpt->IsLoopPrePass() ? JitAnew(alloc, InductionVariableSet, alloc) : nullptr;
        this->availableIntBoundChecks = JitAnew(alloc, IntBoundCheckSet, alloc);
    }

    this->maybeWrittenTypeSyms = nullptr;
    this->callSequence = nullptr;
    this->startCallCount = 0;
    this->argOutCount = 0;
    this->totalOutParamCount = 0;
    this->inlinedArgOutCount = 0;
    this->hasCSECandidates = false;
    this->curFunc = func;

    this->stackLiteralInitFldDataMap = nullptr;

    this->changedSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);

    this->OnDataInitialized(alloc);
}

void
GlobOptBlockData::ReuseBlockData(GlobOptBlockData *fromData)
{
    this->globOpt = fromData->globOpt;

    // Reuse dead map
    this->symToValueMap = fromData->symToValueMap;
    this->exprToValueMap = fromData->exprToValueMap;
    this->liveFields = fromData->liveFields;
    this->liveArrayValues = fromData->liveArrayValues;
    this->maybeWrittenTypeSyms = fromData->maybeWrittenTypeSyms;
    this->isTempSrc = fromData->isTempSrc;
    this->liveVarSyms = fromData->liveVarSyms;
    this->liveInt32Syms = fromData->liveInt32Syms;
    this->liveLossyInt32Syms = fromData->liveLossyInt32Syms;
    this->liveFloat64Syms = fromData->liveFloat64Syms;
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms = fromData->liveSimd128F4Syms;
    this->liveSimd128I4Syms = fromData->liveSimd128I4Syms;
#endif
    if (this->globOpt->TrackHoistableFields())
    {
        this->hoistableFields = fromData->hoistableFields;
    }

    if (this->globOpt->TrackArgumentsObject())
    {
        this->argObjSyms = fromData->argObjSyms;
    }
    this->maybeTempObjectSyms = fromData->maybeTempObjectSyms;
    this->canStoreTempObjectSyms = fromData->canStoreTempObjectSyms;
    this->curFunc = fromData->curFunc;

    this->valuesToKillOnCalls = fromData->valuesToKillOnCalls;
    this->inductionVariables = fromData->inductionVariables;
    this->availableIntBoundChecks = fromData->availableIntBoundChecks;
    this->callSequence = fromData->callSequence;

    this->startCallCount = fromData->startCallCount;
    this->argOutCount = fromData->argOutCount;
    this->totalOutParamCount = fromData->totalOutParamCount;
    this->inlinedArgOutCount = fromData->inlinedArgOutCount;
    this->hasCSECandidates = fromData->hasCSECandidates;

    this->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap;

    this->changedSyms = fromData->changedSyms;
    this->changedSyms->ClearAll();

    this->OnDataReused(fromData);
}

void
GlobOptBlockData::CopyBlockData(GlobOptBlockData *fromData)
{
    this->globOpt = fromData->globOpt;

    this->symToValueMap = fromData->symToValueMap;
    this->exprToValueMap = fromData->exprToValueMap;
    this->liveFields = fromData->liveFields;
    this->liveArrayValues = fromData->liveArrayValues;
    this->maybeWrittenTypeSyms = fromData->maybeWrittenTypeSyms;
    this->isTempSrc = fromData->isTempSrc;
    this->liveVarSyms = fromData->liveVarSyms;
    this->liveInt32Syms = fromData->liveInt32Syms;
    this->liveLossyInt32Syms = fromData->liveLossyInt32Syms;
    this->liveFloat64Syms = fromData->liveFloat64Syms;
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms = fromData->liveSimd128F4Syms;
    this->liveSimd128I4Syms = fromData->liveSimd128I4Syms;
#endif
    this->hoistableFields = fromData->hoistableFields;
    this->argObjSyms = fromData->argObjSyms;
    this->maybeTempObjectSyms = fromData->maybeTempObjectSyms;
    this->canStoreTempObjectSyms = fromData->canStoreTempObjectSyms;
    this->curFunc = fromData->curFunc;
    this->valuesToKillOnCalls = fromData->valuesToKillOnCalls;
    this->inductionVariables = fromData->inductionVariables;
    this->availableIntBoundChecks = fromData->availableIntBoundChecks;
    this->callSequence = fromData->callSequence;
    this->startCallCount = fromData->startCallCount;
    this->argOutCount = fromData->argOutCount;
    this->totalOutParamCount = fromData->totalOutParamCount;
    this->inlinedArgOutCount = fromData->inlinedArgOutCount;
    this->hasCSECandidates = fromData->hasCSECandidates;

    this->changedSyms = fromData->changedSyms;

    this->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap;
    this->OnDataReused(fromData);
}

void
GlobOptBlockData::DeleteBlockData()
{
    JitArenaAllocator *const alloc = this->globOpt->alloc;

    this->symToValueMap->Delete();
    this->exprToValueMap->Delete();

    JitAdelete(alloc, this->liveFields);
    JitAdelete(alloc, this->liveArrayValues);
    if (this->maybeWrittenTypeSyms)
    {
        JitAdelete(alloc, this->maybeWrittenTypeSyms);
    }
    JitAdelete(alloc, this->isTempSrc);
    JitAdelete(alloc, this->liveVarSyms);
    JitAdelete(alloc, this->liveInt32Syms);
    JitAdelete(alloc, this->liveLossyInt32Syms);
    JitAdelete(alloc, this->liveFloat64Syms);
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    JitAdelete(alloc, this->liveSimd128F4Syms);
    JitAdelete(alloc, this->liveSimd128I4Syms);
#endif
    if (this->hoistableFields)
    {
        JitAdelete(alloc, this->hoistableFields);
    }
    if (this->argObjSyms)
    {
        JitAdelete(alloc, this->argObjSyms);
    }
    if (this->maybeTempObjectSyms)
    {
        JitAdelete(alloc, this->maybeTempObjectSyms);
        if (this->canStoreTempObjectSyms)
        {
            JitAdelete(alloc, this->canStoreTempObjectSyms);
        }
    }
    else
    {
        Assert(!this->canStoreTempObjectSyms);
    }

    JitAdelete(alloc, this->valuesToKillOnCalls);

    if(this->inductionVariables)
    {
        JitAdelete(alloc, this->inductionVariables);
    }
    if(this->availableIntBoundChecks)
    {
        JitAdelete(alloc, this->availableIntBoundChecks);
    }

    if (this->stackLiteralInitFldDataMap)
    {
        JitAdelete(alloc, this->stackLiteralInitFldDataMap);
    }

    JitAdelete(alloc, this->changedSyms);
    this->changedSyms = nullptr;

    this->OnDataDeleted();
}

void GlobOptBlockData::CloneBlockData(BasicBlock *const toBlockContext, BasicBlock *const fromBlock)
{
    GlobOptBlockData *const fromData = &fromBlock->globOptData;

    this->globOpt = fromData->globOpt;

    JitArenaAllocator *const alloc = this->globOpt->alloc;

    this->symToValueMap = fromData->symToValueMap->Copy();
    this->exprToValueMap = fromData->exprToValueMap->Copy();

    // Clone the values as well to allow for flow-sensitive ValueInfo
    this->globOpt->CloneValues(toBlockContext, this, fromData);

    if(this->globOpt->DoBoundCheckHoist())
    {
        this->globOpt->CloneBoundCheckHoistBlockData(toBlockContext, this, fromBlock, fromData);
    }

    this->liveFields = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveFields->Copy(fromData->liveFields);

    this->liveArrayValues = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveArrayValues->Copy(fromData->liveArrayValues);

    if (fromData->maybeWrittenTypeSyms)
    {
        this->maybeWrittenTypeSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
        this->maybeWrittenTypeSyms->Copy(fromData->maybeWrittenTypeSyms);
    }

    this->isTempSrc = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->isTempSrc->Copy(fromData->isTempSrc);

    this->liveVarSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveVarSyms->Copy(fromData->liveVarSyms);

    this->liveInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveInt32Syms->Copy(fromData->liveInt32Syms);

    this->liveLossyInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveLossyInt32Syms->Copy(fromData->liveLossyInt32Syms);

    this->liveFloat64Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveFloat64Syms->Copy(fromData->liveFloat64Syms);
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveSimd128F4Syms->Copy(fromData->liveSimd128F4Syms);

    this->liveSimd128I4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->liveSimd128I4Syms->Copy(fromData->liveSimd128I4Syms);
#endif
    if (this->globOpt->TrackHoistableFields())
    {
        if (fromData->hoistableFields)
        {
            this->hoistableFields = fromData->hoistableFields->CopyNew(alloc);
        }
    }

    if (this->globOpt->TrackArgumentsObject() && fromData->argObjSyms)
    {
        this->argObjSyms = fromData->argObjSyms->CopyNew(alloc);
    }
    if (fromData->maybeTempObjectSyms && !fromData->maybeTempObjectSyms->IsEmpty())
    {
        this->maybeTempObjectSyms = fromData->maybeTempObjectSyms->CopyNew(alloc);
        if (fromData->canStoreTempObjectSyms && !fromData->canStoreTempObjectSyms->IsEmpty())
        {
            this->canStoreTempObjectSyms = fromData->canStoreTempObjectSyms->CopyNew(alloc);
        }
    }
    else
    {
        Assert(fromData->canStoreTempObjectSyms == nullptr || fromData->canStoreTempObjectSyms->IsEmpty());
    }

    this->curFunc = fromData->curFunc;
    if (fromData->callSequence != nullptr)
    {
        this->callSequence = JitAnew(alloc, SListBase<IR::Opnd *>);
        fromData->callSequence->CopyTo(alloc, *(this->callSequence));
    }
    else
    {
        this->callSequence = nullptr;
    }

    this->startCallCount = fromData->startCallCount;
    this->argOutCount = fromData->argOutCount;
    this->totalOutParamCount = fromData->totalOutParamCount;
    this->inlinedArgOutCount = fromData->inlinedArgOutCount;
    this->hasCSECandidates = fromData->hasCSECandidates;

    // Although we don't need the data on loop pre pass, we need to do it for the loop header
    // because we capture the loop header bailout on loop prepass
    if (fromData->stackLiteralInitFldDataMap != nullptr &&
        (!this->globOpt->IsLoopPrePass() || (toBlockContext->isLoopHeader && toBlockContext->loop == this->globOpt->rootLoopPrePass)))
    {
        this->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap->Clone();
    }
    else
    {
        this->stackLiteralInitFldDataMap = nullptr;
    }

    this->changedSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    this->changedSyms->Copy(fromData->changedSyms);

    Assert(fromData->HasData());
    this->OnDataInitialized(alloc);
}

void GlobOptBlockData::RemoveUnavailableCandidates(PRECandidatesList * candidates)
{
    // In case of multiple back-edges to the loop, make sure the candidates are still valid.
    FOREACH_SLIST_ENTRY_EDITING(GlobHashBucket*, candidate, candidates, iter)
    {
        Value *candidateValue = candidate->element;
        PropertySym *candidatePropertySym = candidate->value->AsPropertySym();
        ValueNumber valueNumber = candidateValue->GetValueNumber();
        Sym *symStore = candidateValue->GetValueInfo()->GetSymStore();

        Value *blockValue = this->FindValue(candidatePropertySym);

        if (blockValue && blockValue->GetValueNumber() == valueNumber
            && blockValue->GetValueInfo()->GetSymStore() == symStore)
        {
            Value *symStoreValue = this->FindValue(symStore);

            if (symStoreValue && symStoreValue->GetValueNumber() == valueNumber)
            {
                continue;
            }
        }

        iter.RemoveCurrent();
    } NEXT_SLIST_ENTRY_EDITING;
}

template <typename CapturedList, typename CapturedItemsAreEqual>
void
GlobOptBlockData::MergeCapturedValues(
    SListBase<CapturedList> * toList,
    SListBase<CapturedList> * fromList,
    CapturedItemsAreEqual itemsAreEqual)
{
    typename SListBase<CapturedList>::Iterator iterTo(toList);
    typename SListBase<CapturedList>::Iterator iterFrom(fromList);
    bool hasTo = iterTo.Next();
    bool hasFrom = fromList == nullptr ? false : iterFrom.Next();

    // to be conservative, only copy the captured value for common sym Ids
    // in from and to CapturedList, mark all non-common sym Ids for re-capture
    while (hasFrom && hasTo)
    {
        Sym * symFrom = iterFrom.Data().Key();
        Sym * symTo = iterTo.Data().Key();

        if (symFrom->m_id < symTo->m_id)
        {
            this->changedSyms->Set(symFrom->m_id);
            hasFrom = iterFrom.Next();
        }
        else if(symFrom->m_id > symTo->m_id)
        {
            this->changedSyms->Set(symTo->m_id);
            hasTo = iterTo.Next();
        }
        else
        {
            if (!itemsAreEqual(&iterFrom.Data(), &iterTo.Data()))
            {
                this->changedSyms->Set(symTo->m_id);
            }

            hasFrom = iterFrom.Next();
            hasTo = iterTo.Next();
        }
    }
    bool hasRemain = hasFrom || hasTo;
    if (hasRemain)
    {
        typename SListBase<CapturedList>::Iterator iterRemain(hasFrom ? iterFrom : iterTo);
        do
        {
            Sym * symRemain = iterRemain.Data().Key();
            this->changedSyms->Set(symRemain->m_id);
            hasRemain = iterRemain.Next();
        } while (hasRemain);
    }
}

void
GlobOptBlockData::MergeBlockData(
    BasicBlock *toBlock,
    BasicBlock *fromBlock,
    BVSparse<JitArenaAllocator> *const symsRequiringCompensation,
    BVSparse<JitArenaAllocator> *const symsCreatedForMerge,
    bool forceTypeSpecOnLoopHeader)
{
    GlobOptBlockData *fromData = &(fromBlock->globOptData);

    if(this->globOpt->DoBoundCheckHoist())
    {
        // Do this before merging values so that it can see whether a sym's value was changed on one side or the other
        this->globOpt->MergeBoundCheckHoistBlockData(toBlock, this, fromBlock, fromData);
    }

    bool isLoopBackEdge = toBlock->isLoopHeader;
    this->MergeValueMaps(toBlock, fromBlock, symsRequiringCompensation, symsCreatedForMerge);

    this->globOpt->InsertCloneStrs(toBlock, this, fromData);

    this->liveFields->And(fromData->liveFields);
    this->liveArrayValues->And(fromData->liveArrayValues);
    this->isTempSrc->And(fromData->isTempSrc);
    this->hasCSECandidates &= fromData->hasCSECandidates;

    if (this->capturedValues == nullptr)
    {
        this->capturedValues = fromData->capturedValues;
        this->changedSyms->Or(fromData->changedSyms);
    }
    else
    {
        this->MergeCapturedValues(
            &this->capturedValues->constantValues,
            fromData->capturedValues == nullptr ? nullptr : &fromData->capturedValues->constantValues,
            [&](ConstantStackSymValue * symValueFrom, ConstantStackSymValue * symValueTo)
            {
                return symValueFrom->Value().IsEqual(symValueTo->Value());
            });

        this->MergeCapturedValues(
            &this->capturedValues->copyPropSyms,
            fromData->capturedValues == nullptr ? nullptr : &fromData->capturedValues->copyPropSyms,
            [&](CopyPropSyms * copyPropSymFrom, CopyPropSyms * copyPropSymTo)
            {
                if (copyPropSymFrom->Value()->m_id == copyPropSymTo->Value()->m_id)
                {
                    Value * val = fromData->FindValue(copyPropSymFrom->Key());
                    Value * copyVal = fromData->FindValue(copyPropSymTo->Key());
                    return (val != nullptr && copyVal != nullptr &&
                        val->GetValueNumber() == copyVal->GetValueNumber());
                }
                return false;
            });
    }

    if (fromData->maybeWrittenTypeSyms)
    {
        if (this->maybeWrittenTypeSyms == nullptr)
        {
            this->maybeWrittenTypeSyms = JitAnew(this->globOpt->alloc, BVSparse<JitArenaAllocator>, this->globOpt->alloc);
            this->maybeWrittenTypeSyms->Copy(fromData->maybeWrittenTypeSyms);
        }
        else
        {
            this->maybeWrittenTypeSyms->Or(fromData->maybeWrittenTypeSyms);
        }
    }

    {
        // - Keep the var sym live if any of the following is true:
        //     - The var sym is live on both sides
        //     - The var sym is the only live sym that contains the lossless value of the sym on a side (that is, the lossless
        //       int32 sym is not live, and the float64 sym is not live on that side), and the sym of any type is live on the
        //       other side
        //     - On a side, the var and float64 syms are live, the lossless int32 sym is not live, the sym's merged value is
        //       likely int, and the sym of any type is live on the other side. Since the value is likely int, it may be
        //       int-specialized (with lossless conversion) later. Keeping only the float64 sym live requires doing a lossless
        //       conversion from float64 to int32, with bailout if the value of the float is not a true 32-bit integer. Checking
        //       that is costly, and if the float64 sym is converted back to var, it does not become a tagged int, causing a
        //       guaranteed bailout if a lossless conversion to int happens later. Keep the var sym live to preserve its
        //       tagged-ness so that it can be int-specialized while avoiding unnecessary bailouts.
        // - Keep the int32 sym live if it's live on both sides
        //     - Mark the sym as lossy if it's lossy on any side
        // - Keep the float64 sym live if it's live on a side and the sym of a specialized lossless type is live on the other
        //   side
        //
        // fromData.temp =
        //     (fromData.var - (fromData.int32 - fromData.lossyInt32)) &
        //     (this.var | this.int32 | this.float64)
        // this.temp =
        //     (this.var - (this.int32 - this.lossyInt32)) &
        //     (fromData.var | fromData.int32 | fromData.float64)
        // this.var =
        //     (fromData.var & this.var) |
        //     (fromData.temp - fromData.float64) |
        //     (this.temp - this.float64) |
        //     (fromData.temp & fromData.float64 | this.temp & this.float64) & (value ~ int)
        //
        // this.float64 =
        //     fromData.float64 & ((this.int32 - this.lossyInt32) | this.float64) |
        //     this.float64 & ((fromData.int32 - fromData.lossyInt32) | fromData.float64)
        // this.int32 &= fromData.int32
        // this.lossyInt32 = (fromData.lossyInt32 | this.lossyInt32) & this.int32

        BVSparse<JitArenaAllocator> tempBv1(this->globOpt->tempAlloc);
        BVSparse<JitArenaAllocator> tempBv2(this->globOpt->tempAlloc);

        if (isLoopBackEdge && forceTypeSpecOnLoopHeader)
        {
            Loop *const loop = toBlock->loop;

            // Force to lossless int32:
            // forceLosslessInt32 =
            //     ((fromData.int32 - fromData.lossyInt32) - (this.int32 - this.lossyInt32)) &
            //     loop.likelyIntSymsUsedBeforeDefined &
            //     this.var
            tempBv1.Minus(fromData->liveInt32Syms, fromData->liveLossyInt32Syms);
            tempBv2.Minus(this->liveInt32Syms, this->liveLossyInt32Syms);
            tempBv1.Minus(&tempBv2);
            tempBv1.And(loop->likelyIntSymsUsedBeforeDefined);
            tempBv1.And(this->liveVarSyms);
            this->liveInt32Syms->Or(&tempBv1);
            this->liveLossyInt32Syms->Minus(&tempBv1);

            if(this->globOpt->DoLossyIntTypeSpec())
            {
                // Force to lossy int32:
                // forceLossyInt32 = (fromData.int32 - this.int32) & loop.symsUsedBeforeDefined & this.var
                tempBv1.Minus(fromData->liveInt32Syms, this->liveInt32Syms);
                tempBv1.And(loop->symsUsedBeforeDefined);
                tempBv1.And(this->liveVarSyms);
                this->liveInt32Syms->Or(&tempBv1);
                this->liveLossyInt32Syms->Or(&tempBv1);
            }

            // Force to float64:
            // forceFloat64 =
            //     fromData.float64 & loop.forceFloat64 |
            //     (fromData.float64 - this.float64) & loop.likelyNumberSymsUsedBeforeDefined
            tempBv1.And(fromData->liveFloat64Syms, loop->forceFloat64SymsOnEntry);
            this->liveFloat64Syms->Or(&tempBv1);
            tempBv1.Minus(fromData->liveFloat64Syms, this->liveFloat64Syms);
            tempBv1.And(loop->likelyNumberSymsUsedBeforeDefined);
            this->liveFloat64Syms->Or(&tempBv1);

#ifdef ENABLE_SIMDJS
            // Force to Simd128 type:
            // if live on the backedge and we are hoisting the operand.
            // or if live on the backedge only and used before def in the loop.
            tempBv1.And(fromData->liveSimd128F4Syms, loop->forceSimd128F4SymsOnEntry);
            this->liveSimd128F4Syms->Or(&tempBv1);
            tempBv1.Minus(fromData->liveSimd128F4Syms, this->liveSimd128F4Syms);
            tempBv1.And(loop->likelySimd128F4SymsUsedBeforeDefined);
            this->liveSimd128F4Syms->Or(&tempBv1);

            tempBv1.And(fromData->liveSimd128I4Syms, loop->forceSimd128I4SymsOnEntry);
            this->liveSimd128I4Syms->Or(&tempBv1);
            tempBv1.Minus(fromData->liveSimd128I4Syms, this->liveSimd128I4Syms);
            tempBv1.And(loop->likelySimd128I4SymsUsedBeforeDefined);
            this->liveSimd128I4Syms->Or(&tempBv1);
#endif
        }

#ifdef ENABLE_SIMDJS
        BVSparse<JitArenaAllocator> simdSymsToVar(this->globOpt->tempAlloc);
        {
            // SIMD_JS
            // If we have simd128 type-spec sym live as one type on one side, but not of same type on the other, we look at the merged ValueType.
            // If it's Likely the simd128 type, we choose to keep the type-spec sym (compensate with a FromVar), if the following is true:
            //     - We are not in jitLoopBody. Introducing a FromVar for compensation extends bytecode syms lifetime. If the value
            //       is actually dead, and we enter the loop-body after bailing out from SimpleJit, the value will not be restored in
            //       the bailout code.
            //     - Value was never Undefined/Null. Avoid unboxing of possibly uninitialized values.
            //     - Not loop back-edge. To keep unboxed value, the value has to be used-before def in the loop-body. This is done
            //       separately in forceSimd128*SymsOnEntry and included in loop-header.

            // Live syms as F4 on one edge only
            tempBv1.Xor(fromData->liveSimd128F4Syms, this->liveSimd128F4Syms);
            FOREACH_BITSET_IN_SPARSEBV(id, &tempBv1)
            {
                StackSym *const stackSym = this->globOpt->func->m_symTable->FindStackSym(id);
                Assert(stackSym);
                Value *const value = this->FindValue(stackSym);
                ValueInfo * valueInfo = value ? value->GetValueInfo() : nullptr;

                // There are two possible representations for Simd128F4 Value: F4 or Var.
                // If the merged ValueType is LikelySimd128F4, then on the edge where F4 is dead,  Var must be alive.
                // Unbox to F4 type-spec sym.
                if (
                    valueInfo && valueInfo->IsLikelySimd128Float32x4() &&
                    !valueInfo->HasBeenUndefined() && !valueInfo->HasBeenNull() &&
                    !isLoopBackEdge && !this->globOpt->func->IsLoopBody()
                   )
                {
                    this->liveSimd128F4Syms->Set(id);
                }
                else
                {
                    // If live on both edges, box it.
                    if (fromData->IsLive(stackSym) && this->IsLive(stackSym))
                    {
                        simdSymsToVar.Set(id);
                    }
                    // kill F4 sym
                    this->liveSimd128F4Syms->Clear(id);
                }
            } NEXT_BITSET_IN_SPARSEBV;

            // Same for I4
            tempBv1.Xor(fromData->liveSimd128I4Syms, this->liveSimd128I4Syms);
            FOREACH_BITSET_IN_SPARSEBV(id, &tempBv1)
            {
                StackSym *const stackSym = this->globOpt->func->m_symTable->FindStackSym(id);
                Assert(stackSym);
                Value *const value = this->FindValue(stackSym);
                ValueInfo * valueInfo = value ? value->GetValueInfo() : nullptr;
                if (
                    valueInfo && valueInfo->IsLikelySimd128Int32x4() &&
                    !valueInfo->HasBeenUndefined() && !valueInfo->HasBeenNull() &&
                    !isLoopBackEdge && !this->globOpt->func->IsLoopBody()
                   )
                {
                    this->liveSimd128I4Syms->Set(id);
                }
                else
                {
                    if (fromData->IsLive(stackSym) && this->IsLive(stackSym))
                    {
                        simdSymsToVar.Set(id);
                    }
                    this->liveSimd128I4Syms->Clear(id);
                }
            } NEXT_BITSET_IN_SPARSEBV;
        }
#endif

        {
            BVSparse<JitArenaAllocator> tempBv3(this->globOpt->tempAlloc);

            // fromData.temp =
            //     (fromData.var - (fromData.int32 - fromData.lossyInt32)) &
            //     (this.var | this.int32 | this.float64)
            tempBv2.Minus(fromData->liveInt32Syms, fromData->liveLossyInt32Syms);
            tempBv1.Minus(fromData->liveVarSyms, &tempBv2);
            tempBv2.Or(this->liveVarSyms, this->liveInt32Syms);
            tempBv2.Or(this->liveFloat64Syms);
            tempBv1.And(&tempBv2);

            // this.temp =
            //     (this.var - (this.int32 - this.lossyInt32)) &
            //     (fromData.var | fromData.int32 | fromData.float64)
            tempBv3.Minus(this->liveInt32Syms, this->liveLossyInt32Syms);
            tempBv2.Minus(this->liveVarSyms, &tempBv3);
            tempBv3.Or(fromData->liveVarSyms, fromData->liveInt32Syms);
            tempBv3.Or(fromData->liveFloat64Syms);
            tempBv2.And(&tempBv3);

            {
                BVSparse<JitArenaAllocator> tempBv4(this->globOpt->tempAlloc);

                // fromData.temp & fromData.float64 | this.temp & this.float64
                tempBv3.And(&tempBv1, fromData->liveFloat64Syms);
                tempBv4.And(&tempBv2, this->liveFloat64Syms);
                tempBv3.Or(&tempBv4);
            }

            //     (fromData.temp - fromData.float64) |
            //     (this.temp - this.float64)
            tempBv1.Minus(fromData->liveFloat64Syms);
            tempBv2.Minus(this->liveFloat64Syms);
            tempBv1.Or(&tempBv2);

            // this.var =
            //     (fromData.var & this.var) |
            //     (fromData.temp - fromData.float64) |
            //     (this.temp - this.float64)
            this->liveVarSyms->And(fromData->liveVarSyms);
            this->liveVarSyms->Or(&tempBv1);

            // this.var |=
            //     (fromData.temp & fromData.float64 | this.temp & this.float64) & (value ~ int)
            FOREACH_BITSET_IN_SPARSEBV(id, &tempBv3)
            {
                StackSym *const stackSym = this->globOpt->func->m_symTable->FindStackSym(id);
                Assert(stackSym);
                Value *const value = this->FindValue(stackSym);
                if(value)
                {
                    ValueInfo *const valueInfo = value->GetValueInfo();
                    if(valueInfo->IsInt() || (valueInfo->IsLikelyInt() && this->globOpt->DoAggressiveIntTypeSpec()))
                    {
                        this->liveVarSyms->Set(id);
                    }
                }
            } NEXT_BITSET_IN_SPARSEBV;

#ifdef ENABLE_SIMDJS
            // SIMD_JS
            // Simd syms that need boxing
            this->liveVarSyms->Or(&simdSymsToVar);
#endif
        }

        //     fromData.float64 & ((this.int32 - this.lossyInt32) | this.float64)
        tempBv1.Minus(this->liveInt32Syms, this->liveLossyInt32Syms);
        tempBv1.Or(this->liveFloat64Syms);
        tempBv1.And(fromData->liveFloat64Syms);

        //     this.float64 & ((fromData.int32 - fromData.lossyInt32) | fromData.float64)
        tempBv2.Minus(fromData->liveInt32Syms, fromData->liveLossyInt32Syms);
        tempBv2.Or(fromData->liveFloat64Syms);
        tempBv2.And(this->liveFloat64Syms);

        // this.float64 =
        //     fromData.float64 & ((this.int32 - this.lossyInt32) | this.float64) |
        //     this.float64 & ((fromData.int32 - fromData.lossyInt32) | fromData.float64)
        this->liveFloat64Syms->Or(&tempBv1, &tempBv2);

        // this.int32 &= fromData.int32
        // this.lossyInt32 = (fromData.lossyInt32 | this.lossyInt32) & this.int32
        this->liveInt32Syms->And(fromData->liveInt32Syms);
        this->liveLossyInt32Syms->Or(fromData->liveLossyInt32Syms);
        this->liveLossyInt32Syms->And(this->liveInt32Syms);
    }

    if (this->globOpt->TrackHoistableFields() && this->globOpt->HasHoistableFields(fromData))
    {
        if (this->hoistableFields)
        {
            this->hoistableFields->Or(fromData->hoistableFields);
        }
        else
        {
            this->hoistableFields = fromData->hoistableFields->CopyNew(this->globOpt->alloc);
        }
    }

    if (this->globOpt->TrackArgumentsObject())
    {
        if (!this->argObjSyms->Equal(fromData->argObjSyms))
        {
            this->globOpt->CannotAllocateArgumentsObjectOnStack();
        }
    }

    if (fromData->maybeTempObjectSyms && !fromData->maybeTempObjectSyms->IsEmpty())
    {
        if (this->maybeTempObjectSyms)
        {
            this->maybeTempObjectSyms->Or(fromData->maybeTempObjectSyms);
        }
        else
        {
            this->maybeTempObjectSyms = fromData->maybeTempObjectSyms->CopyNew(this->globOpt->alloc);
        }

        if (fromData->canStoreTempObjectSyms && !fromData->canStoreTempObjectSyms->IsEmpty())
        {
            if (this->canStoreTempObjectSyms)
            {
                // Both need to be temp object
                this->canStoreTempObjectSyms->And(fromData->canStoreTempObjectSyms);
            }
        }
        else if (this->canStoreTempObjectSyms)
        {
            this->canStoreTempObjectSyms->ClearAll();
        }
    }
    else
    {
        Assert(!fromData->canStoreTempObjectSyms || fromData->canStoreTempObjectSyms->IsEmpty());
        if (this->canStoreTempObjectSyms)
        {
            this->canStoreTempObjectSyms->ClearAll();
        }
    }

    Assert(this->curFunc == fromData->curFunc);
    Assert((this->callSequence == nullptr && fromData->callSequence == nullptr) || this->callSequence->Equals(*(fromData->callSequence)));
    Assert(this->startCallCount == fromData->startCallCount);
    Assert(this->argOutCount == fromData->argOutCount);
    Assert(this->totalOutParamCount == fromData->totalOutParamCount);
    Assert(this->inlinedArgOutCount == fromData->inlinedArgOutCount);

    // stackLiteralInitFldDataMap is a union of the stack literal from two path.
    // Although we don't need the data on loop prepass, we need to do it for the loop header
    // because we capture the loop header bailout on loop prepass.
    if (fromData->stackLiteralInitFldDataMap != nullptr &&
        (!this->globOpt->IsLoopPrePass() || (toBlock->isLoopHeader && toBlock->loop == this->globOpt->rootLoopPrePass)))
    {
        if (this->stackLiteralInitFldDataMap == nullptr)
        {
            this->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap->Clone();
        }
        else
        {
            StackLiteralInitFldDataMap * toMap = this->stackLiteralInitFldDataMap;
            fromData->stackLiteralInitFldDataMap->Map([toMap](StackSym * stackSym, StackLiteralInitFldData const& data)
            {
                if (toMap->AddNew(stackSym, data) == -1)
                {
                    // If there is an existing data for the stackSym, both path should match
                    DebugOnly(StackLiteralInitFldData const * currentData);
                    Assert(toMap->TryGetReference(stackSym, &currentData));
                    Assert(currentData->currentInitFldCount == data.currentInitFldCount);
                    Assert(currentData->propIds == data.propIds);
                }
            });
        }
    }
}

void
GlobOptBlockData::MergeValueMaps(
    BasicBlock *toBlock,
    BasicBlock *fromBlock,
    BVSparse<JitArenaAllocator> *const symsRequiringCompensation,
    BVSparse<JitArenaAllocator> *const symsCreatedForMerge)
{
    GlobOptBlockData *fromData = &(fromBlock->globOptData);
    bool isLoopBackEdge = toBlock->isLoopHeader;
    Loop *loop = toBlock->loop;
    bool isLoopPrepass = (loop && this->globOpt->prePassLoop == loop);

    Assert(this->globOpt->valuesCreatedForMerge->Count() == 0);
    DebugOnly(ValueSetByValueNumber mergedValues(this->globOpt->tempAlloc, 64));

    BVSparse<JitArenaAllocator> *const mergedValueTypesTrackedForKills = this->globOpt->tempBv;
    Assert(mergedValueTypesTrackedForKills->IsEmpty());
    this->valuesToKillOnCalls->Clear(); // the tracking will be reevaluated based on merged value types

    GlobHashTable *thisTable = this->symToValueMap;
    GlobHashTable *otherTable = fromData->symToValueMap;
    for (uint i = 0; i < thisTable->tableSize; i++)
    {
        SListBase<GlobHashBucket>::Iterator iter2(&otherTable->table[i]);
        iter2.Next();
        FOREACH_SLISTBASE_ENTRY_EDITING(GlobHashBucket, bucket, &thisTable->table[i], iter)
        {
            while (iter2.IsValid() && bucket.value->m_id < iter2.Data().value->m_id)
            {
                iter2.Next();
            }
            Value *newValue = nullptr;

            if (iter2.IsValid() && bucket.value->m_id == iter2.Data().value->m_id)
            {
                newValue =
                    this->MergeValues(
                        bucket.element,
                        iter2.Data().element,
                        iter2.Data().value,
                        isLoopBackEdge,
                        symsRequiringCompensation,
                        symsCreatedForMerge);
            }
            if (newValue == nullptr)
            {
                iter.RemoveCurrent(thisTable->alloc);
                continue;
            }
            else
            {
#if DBG
                // Ensure that only one value per value number is produced by merge. Byte-code constant values are reused in
                // multiple blocks without cloning, so exclude those value numbers.
                {
                    Value *const previouslyMergedValue = mergedValues.Lookup(newValue->GetValueNumber());
                    if (previouslyMergedValue)
                    {
                        if (!this->globOpt->byteCodeConstantValueNumbersBv->Test(newValue->GetValueNumber()))
                        {
                            Assert(newValue == previouslyMergedValue);
                        }
                    }
                    else
                    {
                        mergedValues.Add(newValue);
                    }
                }
#endif

                this->globOpt->TrackMergedValueForKills(newValue, this, mergedValueTypesTrackedForKills);
                bucket.element = newValue;
            }
            iter2.Next();
        } NEXT_SLISTBASE_ENTRY_EDITING;

        if (isLoopPrepass && !this->globOpt->rootLoopPrePass->allFieldsKilled)
        {
            while (iter2.IsValid())
            {
                iter2.Next();
            }
        }
    }

    this->globOpt->valuesCreatedForMerge->Clear();
    DebugOnly(mergedValues.Reset());
    mergedValueTypesTrackedForKills->ClearAll();

    this->exprToValueMap->And(fromData->exprToValueMap);

    this->globOpt->ProcessValueKills(toBlock, this);

    bool isLastLoopBackEdge = false;

    if (isLoopBackEdge)
    {
        this->globOpt->ProcessValueKillsForLoopHeaderAfterBackEdgeMerge(toBlock, this);

        BasicBlock *lastBlock = nullptr;
        FOREACH_PREDECESSOR_BLOCK(pred, toBlock)
        {
            Assert(!lastBlock || pred->GetBlockNum() > lastBlock->GetBlockNum());
            lastBlock = pred;
        }NEXT_PREDECESSOR_BLOCK;
        isLastLoopBackEdge = (lastBlock == fromBlock);
    }
}

Value *
GlobOptBlockData::MergeValues(
    Value *toDataValue,
    Value *fromDataValue,
    Sym *fromDataSym,
    bool isLoopBackEdge,
    BVSparse<JitArenaAllocator> *const symsRequiringCompensation,
    BVSparse<JitArenaAllocator> *const symsCreatedForMerge)
{
    // Same map
    if (toDataValue == fromDataValue)
    {
        return toDataValue;
    }

    const ValueNumberPair sourceValueNumberPair(toDataValue->GetValueNumber(), fromDataValue->GetValueNumber());
    const bool sameValueNumber = sourceValueNumberPair.First() == sourceValueNumberPair.Second();

    ValueInfo *newValueInfo =
        this->MergeValueInfo(
            toDataValue,
            fromDataValue,
            fromDataSym,
            isLoopBackEdge,
            sameValueNumber,
            symsRequiringCompensation,
            symsCreatedForMerge);

    if (newValueInfo == nullptr)
    {
        return nullptr;
    }

    if (sameValueNumber && newValueInfo == toDataValue->GetValueInfo())
    {
        return toDataValue;
    }

    // There may be other syms in toData that haven't been merged yet, referring to the current toData value for this sym. If
    // the merge produced a new value info, don't corrupt the value info for the other sym by changing the same value. Instead,
    // create one value per source value number pair per merge and reuse that for new value infos.
    Value *newValue = this->globOpt->valuesCreatedForMerge->Lookup(sourceValueNumberPair, nullptr);
    if(newValue)
    {
        Assert(sameValueNumber == (newValue->GetValueNumber() == toDataValue->GetValueNumber()));

        // This is an exception where Value::SetValueInfo is called directly instead of GlobOpt::ChangeValueInfo, because we're
        // actually generating new value info through merges.
        newValue->SetValueInfo(newValueInfo);
    }
    else
    {
        newValue = this->globOpt->NewValue(sameValueNumber ? sourceValueNumberPair.First() : this->globOpt->NewValueNumber(), newValueInfo);
        this->globOpt->valuesCreatedForMerge->Add(sourceValueNumberPair, newValue);
    }

    // Set symStore if same on both paths.
    if (toDataValue->GetValueInfo()->GetSymStore() == fromDataValue->GetValueInfo()->GetSymStore())
    {
        this->globOpt->SetSymStoreDirect(newValueInfo, toDataValue->GetValueInfo()->GetSymStore());
    }

    return newValue;
}

ValueInfo *
GlobOptBlockData::MergeValueInfo(
    Value *toDataVal,
    Value *fromDataVal,
    Sym *fromDataSym,
    bool isLoopBackEdge,
    bool sameValueNumber,
    BVSparse<JitArenaAllocator> *const symsRequiringCompensation,
    BVSparse<JitArenaAllocator> *const symsCreatedForMerge)
{
    ValueInfo *const toDataValueInfo = toDataVal->GetValueInfo();
    ValueInfo *const fromDataValueInfo = fromDataVal->GetValueInfo();

    // Same value
    if (toDataValueInfo == fromDataValueInfo)
    {
        return toDataValueInfo;
    }

    if (toDataValueInfo->IsJsType() || fromDataValueInfo->IsJsType())
    {
        Assert(toDataValueInfo->IsJsType() && fromDataValueInfo->IsJsType());
        return this->MergeJsTypeValueInfo(toDataValueInfo->AsJsType(), fromDataValueInfo->AsJsType(), isLoopBackEdge, sameValueNumber);
    }

    ValueType newValueType(toDataValueInfo->Type().Merge(fromDataValueInfo->Type()));
    if (newValueType.IsLikelyInt())
    {
        return ValueInfo::MergeLikelyIntValueInfo(this->globOpt->alloc, toDataVal, fromDataVal, newValueType);
    }
    if(newValueType.IsLikelyAnyOptimizedArray())
    {
        if(newValueType.IsLikelyArrayOrObjectWithArray() &&
            toDataValueInfo->IsLikelyArrayOrObjectWithArray() &&
            fromDataValueInfo->IsLikelyArrayOrObjectWithArray())
        {
            // Value type merge for missing values is aggressive by default (for profile data) - if either side likely has no
            // missing values, then the merged value type also likely has no missing values. This is because arrays often start
            // off having missing values but are eventually filled up. In GlobOpt however, we need to be conservative because
            // the existence of a value type that likely has missing values indicates that it is more likely for it to have
            // missing values than not. Also, StElems that are likely to create missing values are tracked in profile data and
            // will update value types to say they are now likely to have missing values, and that needs to be propagated
            // conservatively.
            newValueType =
                newValueType.SetHasNoMissingValues(
                    toDataValueInfo->HasNoMissingValues() && fromDataValueInfo->HasNoMissingValues());

            if(toDataValueInfo->HasIntElements() != fromDataValueInfo->HasIntElements() ||
                toDataValueInfo->HasFloatElements() != fromDataValueInfo->HasFloatElements())
            {
                // When merging arrays with different native storage types, make the merged value type a likely version to force
                // array checks to be done again and cause a conversion and/or bailout as necessary
                newValueType = newValueType.ToLikely();
            }
        }

        if(!(newValueType.IsObject() && toDataValueInfo->IsArrayValueInfo() && fromDataValueInfo->IsArrayValueInfo()))
        {
            return ValueInfo::New(this->globOpt->alloc, newValueType);
        }

        return
            this->MergeArrayValueInfo(
                newValueType,
                toDataValueInfo->AsArrayValueInfo(),
                fromDataValueInfo->AsArrayValueInfo(),
                fromDataSym,
                symsRequiringCompensation,
                symsCreatedForMerge);
    }

    // Consider: If both values are VarConstantValueInfo with the same value, we could
    // merge them preserving the value.
    return ValueInfo::New(this->globOpt->alloc, newValueType);
}

JsTypeValueInfo*
GlobOptBlockData::MergeJsTypeValueInfo(JsTypeValueInfo * toValueInfo, JsTypeValueInfo * fromValueInfo, bool isLoopBackEdge, bool sameValueNumber)
{
    Assert(toValueInfo != fromValueInfo);

    // On loop back edges we must be conservative and only consider type values which are invariant throughout the loop.
    // That's because in dead store pass we can't correctly track object pointer assignments (o = p), and we may not
    // be able to register correct type checks for the right properties upstream. If we ever figure out how to enhance
    // the dead store pass to track this info we could go more aggressively, as below.
    if (isLoopBackEdge && !sameValueNumber)
    {
        return nullptr;
    }

    const JITTypeHolder toType = toValueInfo->GetJsType();
    const JITTypeHolder fromType = fromValueInfo->GetJsType();
    const JITTypeHolder mergedType = toType == fromType ? toType : JITTypeHolder(nullptr);

    Js::EquivalentTypeSet* toTypeSet = toValueInfo->GetJsTypeSet();
    Js::EquivalentTypeSet* fromTypeSet = fromValueInfo->GetJsTypeSet();
    Js::EquivalentTypeSet* mergedTypeSet = (toTypeSet != nullptr && fromTypeSet != nullptr && Js::EquivalentTypeSet::AreIdentical(toTypeSet, fromTypeSet)) ? toTypeSet : nullptr;

#if DBG_DUMP
    if (PHASE_TRACE(Js::ObjTypeSpecPhase, this->globOpt->func) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, this->globOpt->func))
    {
        Output::Print(_u("ObjTypeSpec: Merging type value info:\n"));
        Output::Print(_u("    from (shared %d): "), fromValueInfo->GetIsShared());
        fromValueInfo->Dump();
        Output::Print(_u("\n    to (shared %d): "), toValueInfo->GetIsShared());
        toValueInfo->Dump();
    }
#endif

    if (mergedType == toType && mergedTypeSet == toTypeSet)
    {
#if DBG_DUMP
        if (PHASE_TRACE(Js::ObjTypeSpecPhase, this->globOpt->func) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, this->globOpt->func))
        {
            Output::Print(_u("\n    result (shared %d): "), toValueInfo->GetIsShared());
            toValueInfo->Dump();
            Output::Print(_u("\n"));
        }
#endif
        return toValueInfo;
    }

    if (mergedType == nullptr && mergedTypeSet == nullptr)
    {
        // No info, so don't bother making a value.
        return nullptr;
    }

    if (toValueInfo->GetIsShared())
    {
        JsTypeValueInfo* mergedValueInfo = JsTypeValueInfo::New(this->globOpt->alloc, mergedType, mergedTypeSet);
#if DBG_DUMP
        if (PHASE_TRACE(Js::ObjTypeSpecPhase, this->globOpt->func) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, this->globOpt->func))
        {
            Output::Print(_u("\n    result (shared %d): "), mergedValueInfo->GetIsShared());
            mergedValueInfo->Dump();
            Output::Print(_u("\n"));
        }
#endif
        return mergedValueInfo;
    }
    else
    {
        toValueInfo->SetJsType(mergedType);
        toValueInfo->SetJsTypeSet(mergedTypeSet);
#if DBG_DUMP
        if (PHASE_TRACE(Js::ObjTypeSpecPhase, this->globOpt->func) || PHASE_TRACE(Js::EquivObjTypeSpecPhase, this->globOpt->func))
        {
            Output::Print(_u("\n    result (shared %d): "), toValueInfo->GetIsShared());
            toValueInfo->Dump();
            Output::Print(_u("\n"));
        }
#endif
        return toValueInfo;
    }
}

ValueInfo *GlobOptBlockData::MergeArrayValueInfo(
    const ValueType mergedValueType,
    const ArrayValueInfo *const toDataValueInfo,
    const ArrayValueInfo *const fromDataValueInfo,
    Sym *const arraySym,
    BVSparse<JitArenaAllocator> *const symsRequiringCompensation,
    BVSparse<JitArenaAllocator> *const symsCreatedForMerge)
{
    Assert(mergedValueType.IsAnyOptimizedArray());
    Assert(toDataValueInfo);
    Assert(fromDataValueInfo);
    Assert(toDataValueInfo != fromDataValueInfo);
    Assert(arraySym);
    Assert(!symsRequiringCompensation == this->globOpt->IsLoopPrePass());
    Assert(!symsCreatedForMerge == this->globOpt->IsLoopPrePass());

    // Merge the segment and segment length syms. If we have the segment and/or the segment length syms available on both sides
    // but in different syms, create a new sym and record that the array sym requires compensation. Compensation will be
    // inserted later to initialize this new sym from all predecessors of the merged block.

    StackSym *newHeadSegmentSym;
    if(toDataValueInfo->HeadSegmentSym() && fromDataValueInfo->HeadSegmentSym())
    {
        if(toDataValueInfo->HeadSegmentSym() == fromDataValueInfo->HeadSegmentSym())
        {
            newHeadSegmentSym = toDataValueInfo->HeadSegmentSym();
        }
        else
        {
            Assert(!this->globOpt->IsLoopPrePass());
            Assert(symsRequiringCompensation);
            symsRequiringCompensation->Set(arraySym->m_id);
            Assert(symsCreatedForMerge);
            if(symsCreatedForMerge->Test(toDataValueInfo->HeadSegmentSym()->m_id))
            {
                newHeadSegmentSym = toDataValueInfo->HeadSegmentSym();
            }
            else
            {
                newHeadSegmentSym = StackSym::New(TyMachPtr, this->globOpt->func);
                symsCreatedForMerge->Set(newHeadSegmentSym->m_id);
            }
        }
    }
    else
    {
        newHeadSegmentSym = nullptr;
    }

    StackSym *newHeadSegmentLengthSym;
    if(toDataValueInfo->HeadSegmentLengthSym() && fromDataValueInfo->HeadSegmentLengthSym())
    {
        if(toDataValueInfo->HeadSegmentLengthSym() == fromDataValueInfo->HeadSegmentLengthSym())
        {
            newHeadSegmentLengthSym = toDataValueInfo->HeadSegmentLengthSym();
        }
        else
        {
            Assert(!this->globOpt->IsLoopPrePass());
            Assert(symsRequiringCompensation);
            symsRequiringCompensation->Set(arraySym->m_id);
            Assert(symsCreatedForMerge);
            if(symsCreatedForMerge->Test(toDataValueInfo->HeadSegmentLengthSym()->m_id))
            {
                newHeadSegmentLengthSym = toDataValueInfo->HeadSegmentLengthSym();
            }
            else
            {
                newHeadSegmentLengthSym = StackSym::New(TyUint32, this->globOpt->func);
                symsCreatedForMerge->Set(newHeadSegmentLengthSym->m_id);
            }
        }
    }
    else
    {
        newHeadSegmentLengthSym = nullptr;
    }

    StackSym *newLengthSym;
    if(toDataValueInfo->LengthSym() && fromDataValueInfo->LengthSym())
    {
        if(toDataValueInfo->LengthSym() == fromDataValueInfo->LengthSym())
        {
            newLengthSym = toDataValueInfo->LengthSym();
        }
        else
        {
            Assert(!this->globOpt->IsLoopPrePass());
            Assert(symsRequiringCompensation);
            symsRequiringCompensation->Set(arraySym->m_id);
            Assert(symsCreatedForMerge);
            if(symsCreatedForMerge->Test(toDataValueInfo->LengthSym()->m_id))
            {
                newLengthSym = toDataValueInfo->LengthSym();
            }
            else
            {
                newLengthSym = StackSym::New(TyUint32, this->globOpt->func);
                symsCreatedForMerge->Set(newLengthSym->m_id);
            }
        }
    }
    else
    {
        newLengthSym = nullptr;
    }

    if(newHeadSegmentSym || newHeadSegmentLengthSym || newLengthSym)
    {
        return ArrayValueInfo::New(this->globOpt->alloc, mergedValueType, newHeadSegmentSym, newHeadSegmentLengthSym, newLengthSym);
    }

    if(symsRequiringCompensation)
    {
        symsRequiringCompensation->Clear(arraySym->m_id);
    }
    return ValueInfo::New(this->globOpt->alloc, mergedValueType);
}

void
GlobOptBlockData::TrackArgumentsSym(IR::RegOpnd const* opnd)
{
    if(!this->curFunc->argObjSyms)
    {
        this->curFunc->argObjSyms = JitAnew(this->globOpt->alloc, BVSparse<JitArenaAllocator>, this->globOpt->alloc);
    }
    this->curFunc->argObjSyms->Set(opnd->m_sym->m_id);
    this->argObjSyms->Set(opnd->m_sym->m_id);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (PHASE_TESTTRACE(Js::StackArgOptPhase, this->globOpt->func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(_u("Created a new alias s%d for arguments object in function %s(%s) topFunc %s(%s)\n"),
            opnd->m_sym->m_id,
            this->curFunc->GetJITFunctionBody()->GetDisplayName(),
            this->curFunc->GetDebugNumberSet(debugStringBuffer),
            this->globOpt->func->GetJITFunctionBody()->GetDisplayName(),
            this->globOpt->func->GetDebugNumberSet(debugStringBuffer2)
            );
        Output::Flush();
    }
#endif
}

void
GlobOptBlockData::ClearArgumentsSym(IR::RegOpnd const* opnd)
{
    // We blindly clear so need to check func has argObjSyms
    if (this->curFunc->argObjSyms)
    {
        this->curFunc->argObjSyms->Clear(opnd->m_sym->m_id);
    }
    this->argObjSyms->Clear(opnd->m_sym->m_id);
}

BOOL
GlobOptBlockData::TestAnyArgumentsSym() const
{
    return this->argObjSyms->TestEmpty();
}

BOOL
GlobOptBlockData::IsArgumentsSymID(SymID id) const
{
    return this->argObjSyms->Test(id);
}

BOOL
GlobOptBlockData::IsArgumentsOpnd(IR::Opnd const* opnd) const
{
    SymID id = 0;
    if (opnd->IsRegOpnd())
    {
        id = opnd->AsRegOpnd()->m_sym->m_id;
        return this->IsArgumentsSymID(id);
    }
    else if (opnd->IsSymOpnd())
    {
        Sym const *sym = opnd->AsSymOpnd()->m_sym;
        if (sym && sym->IsPropertySym())
        {
            PropertySym const *propertySym = sym->AsPropertySym();
            id = propertySym->m_stackSym->m_id;
            return this->IsArgumentsSymID(id);
        }
        return false;
    }
    else if (opnd->IsIndirOpnd())
    {
        IR::RegOpnd const *indexOpnd = opnd->AsIndirOpnd()->GetIndexOpnd();
        IR::RegOpnd const *baseOpnd = opnd->AsIndirOpnd()->GetBaseOpnd();
        return this->IsArgumentsSymID(baseOpnd->m_sym->m_id) || (indexOpnd && this->IsArgumentsSymID(indexOpnd->m_sym->m_id));
    }
    AssertMsg(false, "Unknown type");
    return false;
}

Value*
GlobOptBlockData::FindValue(Sym *sym)
{
    Assert(this->symToValueMap);

    if (sym->IsStackSym() && sym->AsStackSym()->IsTypeSpec())
    {
        sym = sym->AsStackSym()->GetVarEquivSym(this->globOpt->func);
    }
    else if (sym->IsPropertySym())
    {
        return this->FindPropertyValue(sym->m_id);
    }

    if (sym->IsStackSym() && sym->AsStackSym()->IsFromByteCodeConstantTable())
    {
        return this->globOpt->byteCodeConstantValueArray->Get(sym->m_id);
    }
    else
    {
        return FindValueFromMapDirect(sym->m_id);
    }
}

Value *
GlobOptBlockData::FindPropertyValue(SymID symId)
{
    Assert(this->globOpt->func->m_symTable->Find(symId)->IsPropertySym());
    if (!this->liveFields->Test(symId))
    {
        Assert(!this->globOpt->IsHoistablePropertySym(symId));
        return nullptr;
    }
    return FindValueFromMapDirect(symId);
}

Value *
GlobOptBlockData::FindObjectTypeValue(SymID typeSymId)
{
    Assert(this->globOpt->func->m_symTable->Find(typeSymId)->IsStackSym());
    if (!this->liveFields->Test(typeSymId))
    {
        return nullptr;
    }
    return FindObjectTypeValueNoLivenessCheck(typeSymId);
}

Value *
GlobOptBlockData::FindObjectTypeValueNoLivenessCheck(StackSym* typeSym)
{
    return FindObjectTypeValueNoLivenessCheck(typeSym->m_id);
}

Value *
GlobOptBlockData::FindObjectTypeValueNoLivenessCheck(SymID typeSymId)
{
    Value* value = this->FindValueFromMapDirect(typeSymId);
    Assert(value == nullptr || value->GetValueInfo()->IsJsType());
    return value;
}

Value *
GlobOptBlockData::FindObjectTypeValue(StackSym* typeSym)
{
    return FindObjectTypeValue(typeSym->m_id);
}

Value *
GlobOptBlockData::FindFuturePropertyValue(PropertySym *const propertySym)
{
    Assert(propertySym);

    // Try a direct lookup based on this sym
    Value *const value = this->FindValue(propertySym);
    if(value)
    {
        return value;
    }

    if(PHASE_OFF(Js::CopyPropPhase, this->globOpt->func))
    {
        // Need to use copy-prop info to backtrack
        return nullptr;
    }

    // Try to get the property object's value
    StackSym *const objectSym = propertySym->m_stackSym;
    Value *objectValue = this->FindValue(objectSym);
    if(!objectValue)
    {
        if(!objectSym->IsSingleDef())
        {
            return nullptr;
        }

        switch(objectSym->m_instrDef->m_opcode)
        {
        case Js::OpCode::Ld_A:
        case Js::OpCode::LdSlotArr:
        case Js::OpCode::LdSlot:
            // Allow only these op-codes for tracking the object sym's value transfer backwards. Other transfer op-codes
            // could be included here if this function is used in scenarios that need them.
            break;

        default:
            return nullptr;
        }

        // Try to get the property object's value from the src of the definition
        IR::Opnd *const objectTransferSrc = objectSym->m_instrDef->GetSrc1();
        if(!objectTransferSrc)
        {
            return nullptr;
        }
        if(objectTransferSrc->IsRegOpnd())
        {
            objectValue = this->FindValue(objectTransferSrc->AsRegOpnd()->m_sym);
        }
        else if(objectTransferSrc->IsSymOpnd())
        {
            Sym *const objectTransferSrcSym = objectTransferSrc->AsSymOpnd()->m_sym;
            if(objectTransferSrcSym->IsStackSym())
            {
                objectValue = this->FindValue(objectTransferSrcSym);
            }
            else
            {
                // About to make a recursive call, so when jitting in the foreground, probe the stack
                if(!this->globOpt->func->IsBackgroundJIT())
                {
                    PROBE_STACK(this->globOpt->func->GetScriptContext(), Js::Constants::MinStackDefault);
                }
                objectValue = FindFuturePropertyValue(objectTransferSrcSym->AsPropertySym());
            }
        }
        else
        {
            return nullptr;
        }
        if(!objectValue)
        {
            return nullptr;
        }
    }

    // Try to use the property object's copy-prop sym and the property ID to find a mapped property sym, and get its value
    StackSym *const objectCopyPropSym = this->GetCopyPropSym(nullptr, objectValue);
    if(!objectCopyPropSym)
    {
        return nullptr;
    }
    PropertySym *const propertyCopyPropSym = PropertySym::Find(objectCopyPropSym->m_id, propertySym->m_propertyId, this->globOpt->func);
    if(!propertyCopyPropSym)
    {
        return nullptr;
    }
    return this->FindValue(propertyCopyPropSym);
}

Value *
GlobOptBlockData::FindValueFromMapDirect(SymID symId)
{
    Value ** valuePtr = this->symToValueMap->Get(symId);

    if (valuePtr == nullptr)
    {
        return 0;
    }

    return (*valuePtr);
}

StackSym *
GlobOptBlockData::GetCopyPropSym(Sym * sym, Value * value)
{
    ValueInfo *valueInfo = value->GetValueInfo();
    Sym * copySym = valueInfo->GetSymStore();

    if (!copySym)
    {
        return nullptr;
    }

    // Only copy prop stackSym, as a propertySym wouldn't improve anything.
    // SingleDef info isn't flow sensitive, so make sure the symbol is actually live.
    if (copySym->IsStackSym() && copySym != sym)
    {
        Assert(!copySym->AsStackSym()->IsTypeSpec());
        Value *copySymVal = this->FindValue(valueInfo->GetSymStore());
        if (copySymVal && copySymVal->GetValueNumber() == value->GetValueNumber())
        {
            if (valueInfo->IsVarConstant() && !this->IsLive(copySym))
            {
                // Because the addrConstantToValueMap isn't flow-based, the symStore of
                // varConstants may not be live.
                return nullptr;
            }
            return copySym->AsStackSym();
        }
    }
    return nullptr;
}

void
GlobOptBlockData::ClearSymValue(Sym* sym)
{
    this->symToValueMap->Clear(sym->m_id);
}

void
GlobOptBlockData::MarkTempLastUse(IR::Instr *instr, IR::RegOpnd *regOpnd)
{
    if (OpCodeAttr::NonTempNumberSources(instr->m_opcode))
    {
        // Turn off bit if opcode could cause the src to be aliased.
        this->isTempSrc->Clear(regOpnd->m_sym->m_id);
    }
    else if (this->isTempSrc->Test(regOpnd->m_sym->m_id))
    {
        // We just mark things that are temp in the globopt phase.
        // The backwards phase will turn this off if it is not the last use.
        // The isTempSrc is freed at the end of each block, which is why the backwards phase can't
        // just use it.
        if (!PHASE_OFF(Js::BackwardPhase, this->globOpt->func) && !this->globOpt->IsLoopPrePass())
        {
            regOpnd->m_isTempLastUse = true;
        }
    }
}

Value *
GlobOptBlockData::InsertNewValue(Value *val, IR::Opnd *opnd)
{
    return this->SetValue(val, opnd);
}

void
GlobOptBlockData::SetChangedSym(SymID symId)
{
    // this->currentBlock might not be the one which contain the changing symId,
    // like hoisting invariant, but more changed symId is overly conservative and safe.
    // symId in the hoisted to block is marked as JITOptimizedReg so it does't affect bailout.
    if (this->changedSyms)
    {
        this->changedSyms->Set(symId);
        if (this->capturedValuesCandidate != nullptr)
        {
            this->globOpt->changedSymsAfterIncBailoutCandidate->Set(symId);
        }
    }
    // else could be hit only in MergeValues and it is handled by MergeCapturedValues
}

void
GlobOptBlockData::SetValue(Value *val, Sym * sym)
{
    ValueInfo *valueInfo = val->GetValueInfo();

    sym = this->globOpt->SetSymStore(valueInfo, sym);
    bool isStackSym = sym->IsStackSym();

    if (isStackSym && sym->AsStackSym()->IsFromByteCodeConstantTable())
    {
        // Put the constants in a global array. This will minimize the per-block info.
        this->globOpt->byteCodeConstantValueArray->Set(sym->m_id, val);
        this->globOpt->byteCodeConstantValueNumbersBv->Set(val->GetValueNumber());
    }
    else
    {
        this->SetValueToHashTable(this->symToValueMap, val, sym);
        if (isStackSym && sym->AsStackSym()->HasByteCodeRegSlot())
        {
            this->SetChangedSym(sym->m_id);
        }
    }
}

Value *
GlobOptBlockData::SetValue(Value *val, IR::Opnd *opnd)
{
    if (opnd)
    {
        Sym *sym;
        switch (opnd->GetKind())
        {
        case IR::OpndKindSym:
            sym = opnd->AsSymOpnd()->m_sym;
            break;

        case IR::OpndKindReg:
            sym = opnd->AsRegOpnd()->m_sym;
            break;

        default:
            sym = nullptr;
        }
        if (sym)
        {
            this->SetValue(val, sym);
        }
    }

    return val;
}

void
GlobOptBlockData::SetValueToHashTable(GlobHashTable *valueNumberMap, Value *val, Sym *sym)
{
    Value **pValue = valueNumberMap->FindOrInsertNew(sym);
    *pValue = val;
}

void
GlobOptBlockData::MakeLive(StackSym *sym, const bool lossy)
{
    Assert(sym);
    Assert(this);

    if(sym->IsTypeSpec())
    {
        const SymID varSymId = sym->GetVarEquivSym(this->globOpt->func)->m_id;
        if(sym->IsInt32())
        {
            this->liveInt32Syms->Set(varSymId);
            if(lossy)
            {
                this->liveLossyInt32Syms->Set(varSymId);
            }
            else
            {
                this->liveLossyInt32Syms->Clear(varSymId);
            }
            return;
        }

        if (sym->IsFloat64())
        {
            this->liveFloat64Syms->Set(varSymId);
            return;
        }
#ifdef ENABLE_SIMDJS
        // SIMD_JS
        if (sym->IsSimd128F4())
        {
            this->liveSimd128F4Syms->Set(varSymId);
            return;
        }

        if (sym->IsSimd128I4())
        {
            this->liveSimd128I4Syms->Set(varSymId);
            return;
        }
#endif
    }

    this->liveVarSyms->Set(sym->m_id);
}

bool
GlobOptBlockData::IsLive(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return
        sym &&
        (
        this->liveVarSyms->Test(sym->m_id) ||
        this->liveInt32Syms->Test(sym->m_id) ||
        this->liveFloat64Syms->Test(sym->m_id)
#ifdef ENABLE_SIMDJS
            || this->liveSimd128F4Syms->Test(sym->m_id) || this->liveSimd128I4Syms->Test(sym->m_id)
#endif
        );
}

bool
GlobOptBlockData::IsTypeSpecialized(Sym const * sym) const
{
    return this->IsInt32TypeSpecialized(sym) || this->IsFloat64TypeSpecialized(sym)
#ifdef ENABLE_SIMDJS
        || this->IsSimd128TypeSpecialized(sym)
#endif
        ;
}

bool
GlobOptBlockData::IsSwitchInt32TypeSpecialized(IR::Instr const * instr) const
{
    return GlobOpt::IsSwitchOptEnabled(instr->m_func->GetTopFunc())
        && instr->GetSrc1()->IsRegOpnd()
        && this->IsInt32TypeSpecialized(instr->GetSrc1()->AsRegOpnd()->m_sym);
}

bool
GlobOptBlockData::IsInt32TypeSpecialized(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && this->liveInt32Syms->Test(sym->m_id) && !this->liveLossyInt32Syms->Test(sym->m_id);
}

bool
GlobOptBlockData::IsFloat64TypeSpecialized(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && this->liveFloat64Syms->Test(sym->m_id);
}

#ifdef ENABLE_SIMDJS
// SIMD_JS
bool
GlobOptBlockData::IsSimd128TypeSpecialized(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (this->liveSimd128F4Syms->Test(sym->m_id) || this->liveSimd128I4Syms->Test(sym->m_id));
}

bool
GlobOptBlockData::IsSimd128TypeSpecialized(IRType type, Sym const * sym) const
{
    switch (type)
    {
    case TySimd128F4:
        return this->IsSimd128F4TypeSpecialized(sym);
    case TySimd128I4:
        return this->IsSimd128I4TypeSpecialized(sym);
    default:
        Assert(UNREACHED);
        return false;
    }
}

bool
GlobOptBlockData::IsSimd128F4TypeSpecialized(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (this->liveSimd128F4Syms->Test(sym->m_id));
}

bool
GlobOptBlockData::IsSimd128I4TypeSpecialized(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (this->liveSimd128I4Syms->Test(sym->m_id));
}

bool
GlobOptBlockData::IsLiveAsSimd128(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return
        sym &&
        (
        this->liveSimd128F4Syms->Test(sym->m_id) ||
        this->liveSimd128I4Syms->Test(sym->m_id)
        );
}

bool
GlobOptBlockData::IsLiveAsSimd128F4(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && this->liveSimd128F4Syms->Test(sym->m_id);
}

bool
GlobOptBlockData::IsLiveAsSimd128I4(Sym const * sym) const
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && this->liveSimd128I4Syms->Test(sym->m_id);
}
#endif

void
GlobOptBlockData::KillStateForGeneratorYield()
{
    /*
    TODO[generators][ianhall]: Do a ToVar on any typespec'd syms before the bailout so that we can enable typespec in generators without bailin having to restore typespec'd values
    FOREACH_BITSET_IN_SPARSEBV(symId, this->liveInt32Syms)
    {
        this->ToVar(instr, , this->globOpt->currentBlock, , );
    }
    NEXT_BITSET_IN_SPARSEBV;

    FOREACH_BITSET_IN_SPARSEBV(symId, this->liveInt32Syms)
    {
        this->ToVar(instr, , this->globOpt->currentBlock, , );
    }
    NEXT_BITSET_IN_SPARSEBV;
    */

    FOREACH_GLOBHASHTABLE_ENTRY(bucket, this->symToValueMap)
    {
        ValueType type = bucket.element->GetValueInfo()->Type().ToLikely();
        bucket.element = this->globOpt->NewGenericValue(type);
    }
    NEXT_GLOBHASHTABLE_ENTRY;

    this->exprToValueMap->ClearAll();
    this->liveFields->ClearAll();
    this->liveArrayValues->ClearAll();
    if (this->maybeWrittenTypeSyms)
    {
        this->maybeWrittenTypeSyms->ClearAll();
    }
    this->isTempSrc->ClearAll();
    this->liveInt32Syms->ClearAll();
    this->liveLossyInt32Syms->ClearAll();
    this->liveFloat64Syms->ClearAll();
#ifdef ENABLE_SIMDJS
    // SIMD_JS
    this->liveSimd128F4Syms->ClearAll();
    this->liveSimd128I4Syms->ClearAll();
#endif
    if (this->hoistableFields)
    {
        this->hoistableFields->ClearAll();
    }
    // Keep this->liveVarSyms as is
    // Keep this->argObjSyms as is

    // MarkTemp should be disabled for generator functions for now
    Assert(this->maybeTempObjectSyms == nullptr || this->maybeTempObjectSyms->IsEmpty());
    Assert(this->canStoreTempObjectSyms == nullptr || this->canStoreTempObjectSyms->IsEmpty());

    this->valuesToKillOnCalls->Clear();
    if (this->inductionVariables)
    {
        this->inductionVariables->Clear();
    }
    if (this->availableIntBoundChecks)
    {
        this->availableIntBoundChecks->Clear();
    }

    // Keep bailout data as is
    this->hasCSECandidates = false;
}

#if DBG_DUMP
void
GlobOptBlockData::DumpSymToValueMap() const
{
    if (this->symToValueMap != nullptr)
    {
        this->symToValueMap->Dump(GlobOptBlockData::DumpSym);
    }
}

void
GlobOptBlockData::DumpSym(Sym *sym)
{
    ((Sym const*)sym)->Dump();
}
#endif
