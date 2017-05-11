#include "Backend.h"

void
GlobOpt::NulloutBlockData(GlobOptBlockData *data)
{
    data->globOpt = this;

    data->symToValueMap = nullptr;
    data->exprToValueMap = nullptr;
    data->liveFields = nullptr;
    data->maybeWrittenTypeSyms = nullptr;
    data->isTempSrc = nullptr;
    data->liveVarSyms = nullptr;
    data->liveInt32Syms = nullptr;
    data->liveLossyInt32Syms = nullptr;
    data->liveFloat64Syms = nullptr;
    // SIMD_JS
    data->liveSimd128F4Syms = nullptr;
    data->liveSimd128I4Syms = nullptr;

    data->hoistableFields = nullptr;
    data->argObjSyms = nullptr;
    data->maybeTempObjectSyms = nullptr;
    data->canStoreTempObjectSyms = nullptr;
    data->valuesToKillOnCalls = nullptr;
    data->inductionVariables = nullptr;
    data->availableIntBoundChecks = nullptr;
    data->callSequence = nullptr;
    data->startCallCount = 0;
    data->argOutCount = 0;
    data->totalOutParamCount = 0;
    data->inlinedArgOutCount = 0;
    data->hasCSECandidates = false;
    data->curFunc = this->func;

    data->stackLiteralInitFldDataMap = nullptr;

    data->capturedValues = nullptr;
    data->changedSyms = nullptr;

    data->OnDataUnreferenced();
}

void
GlobOpt::InitBlockData(GlobOptBlockData* data)
{
    data->globOpt = this;

    JitArenaAllocator *const alloc = this->alloc;

    data->symToValueMap = GlobHashTable::New(alloc, 64);
    data->exprToValueMap = ExprHashTable::New(alloc, 64);
    data->liveFields = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveArrayValues = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->isTempSrc = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveVarSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveLossyInt32Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveFloat64Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);

    // SIMD_JS
    data->liveSimd128F4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->liveSimd128I4Syms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);

    data->hoistableFields = nullptr;
    data->argObjSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);
    data->maybeTempObjectSyms = nullptr;
    data->canStoreTempObjectSyms = nullptr;
    data->valuesToKillOnCalls = JitAnew(alloc, ValueSet, alloc);

    if(DoBoundCheckHoist())
    {
        data->inductionVariables = IsLoopPrePass() ? JitAnew(alloc, InductionVariableSet, alloc) : nullptr;
        data->availableIntBoundChecks = JitAnew(alloc, IntBoundCheckSet, alloc);
    }

    data->maybeWrittenTypeSyms = nullptr;
    data->callSequence = nullptr;
    data->startCallCount = 0;
    data->argOutCount = 0;
    data->totalOutParamCount = 0;
    data->inlinedArgOutCount = 0;
    data->hasCSECandidates = false;
    data->curFunc = this->func;

    data->stackLiteralInitFldDataMap = nullptr;

    data->changedSyms = JitAnew(alloc, BVSparse<JitArenaAllocator>, alloc);

    data->OnDataInitialized(alloc);
}

void
GlobOpt::ReuseBlockData(GlobOptBlockData *toData, GlobOptBlockData *fromData)
{
    toData->globOpt = fromData->globOpt;

    // Reuse dead map
    toData->symToValueMap = fromData->symToValueMap;
    toData->exprToValueMap = fromData->exprToValueMap;
    toData->liveFields = fromData->liveFields;
    toData->liveArrayValues = fromData->liveArrayValues;
    toData->maybeWrittenTypeSyms = fromData->maybeWrittenTypeSyms;
    toData->isTempSrc = fromData->isTempSrc;
    toData->liveVarSyms = fromData->liveVarSyms;
    toData->liveInt32Syms = fromData->liveInt32Syms;
    toData->liveLossyInt32Syms = fromData->liveLossyInt32Syms;
    toData->liveFloat64Syms = fromData->liveFloat64Syms;

    // SIMD_JS
    toData->liveSimd128F4Syms = fromData->liveSimd128F4Syms;
    toData->liveSimd128I4Syms = fromData->liveSimd128I4Syms;

    if (TrackHoistableFields())
    {
        toData->hoistableFields = fromData->hoistableFields;
    }

    if (TrackArgumentsObject())
    {
        toData->argObjSyms = fromData->argObjSyms;
    }
    toData->maybeTempObjectSyms = fromData->maybeTempObjectSyms;
    toData->canStoreTempObjectSyms = fromData->canStoreTempObjectSyms;
    toData->curFunc = fromData->curFunc;

    toData->valuesToKillOnCalls = fromData->valuesToKillOnCalls;
    toData->inductionVariables = fromData->inductionVariables;
    toData->availableIntBoundChecks = fromData->availableIntBoundChecks;
    toData->callSequence = fromData->callSequence;

    toData->startCallCount = fromData->startCallCount;
    toData->argOutCount = fromData->argOutCount;
    toData->totalOutParamCount = fromData->totalOutParamCount;
    toData->inlinedArgOutCount = fromData->inlinedArgOutCount;
    toData->hasCSECandidates = fromData->hasCSECandidates;

    toData->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap;

    toData->changedSyms = fromData->changedSyms;
    toData->changedSyms->ClearAll();

    toData->OnDataReused(fromData);
}

void
GlobOpt::CopyBlockData(GlobOptBlockData *toData, GlobOptBlockData *fromData)
{
    toData->globOpt = fromData->globOpt;

    toData->symToValueMap = fromData->symToValueMap;
    toData->exprToValueMap = fromData->exprToValueMap;
    toData->liveFields = fromData->liveFields;
    toData->liveArrayValues = fromData->liveArrayValues;
    toData->maybeWrittenTypeSyms = fromData->maybeWrittenTypeSyms;
    toData->isTempSrc = fromData->isTempSrc;
    toData->liveVarSyms = fromData->liveVarSyms;
    toData->liveInt32Syms = fromData->liveInt32Syms;
    toData->liveLossyInt32Syms = fromData->liveLossyInt32Syms;
    toData->liveFloat64Syms = fromData->liveFloat64Syms;

    // SIMD_JS
    toData->liveSimd128F4Syms = fromData->liveSimd128F4Syms;
    toData->liveSimd128I4Syms = fromData->liveSimd128I4Syms;

    toData->hoistableFields = fromData->hoistableFields;
    toData->argObjSyms = fromData->argObjSyms;
    toData->maybeTempObjectSyms = fromData->maybeTempObjectSyms;
    toData->canStoreTempObjectSyms = fromData->canStoreTempObjectSyms;
    toData->curFunc = fromData->curFunc;
    toData->valuesToKillOnCalls = fromData->valuesToKillOnCalls;
    toData->inductionVariables = fromData->inductionVariables;
    toData->availableIntBoundChecks = fromData->availableIntBoundChecks;
    toData->callSequence = fromData->callSequence;
    toData->startCallCount = fromData->startCallCount;
    toData->argOutCount = fromData->argOutCount;
    toData->totalOutParamCount = fromData->totalOutParamCount;
    toData->inlinedArgOutCount = fromData->inlinedArgOutCount;
    toData->hasCSECandidates = fromData->hasCSECandidates;

    toData->changedSyms = fromData->changedSyms;

    toData->stackLiteralInitFldDataMap = fromData->stackLiteralInitFldDataMap;
    toData->OnDataReused(fromData);
}

void
GlobOpt::DeleteBlockData(GlobOptBlockData *data)
{
    JitArenaAllocator *const alloc = this->alloc;

    data->symToValueMap->Delete();
    data->exprToValueMap->Delete();

    JitAdelete(alloc, data->liveFields);
    JitAdelete(alloc, data->liveArrayValues);
    if (data->maybeWrittenTypeSyms)
    {
        JitAdelete(alloc, data->maybeWrittenTypeSyms);
    }
    JitAdelete(alloc, data->isTempSrc);
    JitAdelete(alloc, data->liveVarSyms);
    JitAdelete(alloc, data->liveInt32Syms);
    JitAdelete(alloc, data->liveLossyInt32Syms);
    JitAdelete(alloc, data->liveFloat64Syms);

    // SIMD_JS
    JitAdelete(alloc, data->liveSimd128F4Syms);
    JitAdelete(alloc, data->liveSimd128I4Syms);

    if (data->hoistableFields)
    {
        JitAdelete(alloc, data->hoistableFields);
    }
    if (data->argObjSyms)
    {
        JitAdelete(alloc, data->argObjSyms);
    }
    if (data->maybeTempObjectSyms)
    {
        JitAdelete(alloc, data->maybeTempObjectSyms);
        if (data->canStoreTempObjectSyms)
        {
            JitAdelete(alloc, data->canStoreTempObjectSyms);
        }
    }
    else
    {
        Assert(!data->canStoreTempObjectSyms);
    }

    JitAdelete(alloc, data->valuesToKillOnCalls);

    if(data->inductionVariables)
    {
        JitAdelete(alloc, data->inductionVariables);
    }
    if(data->availableIntBoundChecks)
    {
        JitAdelete(alloc, data->availableIntBoundChecks);
    }

    if (data->stackLiteralInitFldDataMap)
    {
        JitAdelete(alloc, data->stackLiteralInitFldDataMap);
    }

    JitAdelete(alloc, data->changedSyms);
    data->changedSyms = nullptr;

    data->OnDataDeleted();
}

void
GlobOpt::CleanUpValueMaps()
{
    // Don't do cleanup if it's been done recently.
    // Landing pad could get optimized twice...
    // We want the same info out the first and second time. So always cleanup.
    // Increasing the cleanup threshold count for asmjs to 500
    uint cleanupCount = (!GetIsAsmJSFunc()) ? CONFIG_FLAG(GoptCleanupThreshold) : CONFIG_FLAG(AsmGoptCleanupThreshold);
    if (!this->currentBlock->IsLandingPad() && this->instrCountSinceLastCleanUp < cleanupCount)
    {
        return;
    }
    this->instrCountSinceLastCleanUp = 0;

    GlobHashTable *thisTable = this->currentBlock->globOptData.symToValueMap;
    BVSparse<JitArenaAllocator> deadSymsBv(this->tempAlloc);
    BVSparse<JitArenaAllocator> keepAliveSymsBv(this->tempAlloc);
    BVSparse<JitArenaAllocator> availableValueNumbers(this->tempAlloc);
    availableValueNumbers.Copy(byteCodeConstantValueNumbersBv);
    BVSparse<JitArenaAllocator> *upwardExposedUses = this->currentBlock->upwardExposedUses;
    BVSparse<JitArenaAllocator> *upwardExposedFields = this->currentBlock->upwardExposedFields;
    bool isInLoop = !!this->currentBlock->loop;

    BVSparse<JitArenaAllocator> symsInCallSequence(this->tempAlloc);
    SListBase<IR::Opnd *> * callSequence = this->currentBlock->globOptData.callSequence;
    if (callSequence && !callSequence->Empty())
    {
        FOREACH_SLISTBASE_ENTRY(IR::Opnd *, opnd, callSequence)
        {
            StackSym * sym = opnd->GetStackSym();
            symsInCallSequence.Set(sym->m_id);
        }
    }
    NEXT_SLISTBASE_ENTRY;

    for (uint i = 0; i < thisTable->tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY_EDITING(GlobHashBucket, bucket, &thisTable->table[i], iter)
        {
            bool isSymUpwardExposed = upwardExposedUses->Test(bucket.value->m_id) || upwardExposedFields->Test(bucket.value->m_id);
            if (!isSymUpwardExposed && symsInCallSequence.Test(bucket.value->m_id))
            {
                // Don't remove/shrink sym-value pair if the sym is referenced in callSequence even if the sym is dead according to backward data flow.
                // This is possible in some edge cases that an infinite loop is involved when evaluating parameter for a function (between StartCall and Call),
                // there is no backward data flow into the infinite loop block, but non empty callSequence still populates to it in this (forward) pass
                // which causes error when looking up value for the syms in callSequence (cannot find the value).
                // It would cause error to fill out the bailout information for the loop blocks.
                // Remove dead syms from callSequence has some risk because there are various associated counters which need to be consistent.
                continue;
            }
            // Make sure symbol was created before backward pass.
            // If symbols isn't upward exposed, mark it as dead.
            // If a symbol was copy-prop'd in a loop prepass, the upwardExposedUses info could be wrong.  So wait until we are out of the loop before clearing it.
            if ((SymID)bucket.value->m_id <= this->maxInitialSymID && !isSymUpwardExposed
                && (!isInLoop || !this->prePassCopyPropSym->Test(bucket.value->m_id)))
            {
                Value *val = bucket.element;
                ValueInfo *valueInfo = val->GetValueInfo();

                Sym * sym = bucket.value;
                Sym *symStore = valueInfo->GetSymStore();

                if (symStore && symStore == bucket.value)
                {
                    // Keep constants around, as we don't know if there will be further uses
                    if (!bucket.element->GetValueInfo()->IsVarConstant() && !bucket.element->GetValueInfo()->HasIntConstantValue())
                    {
                        // Symbol may still be a copy-prop candidate.  Wait before deleting it.
                        deadSymsBv.Set(bucket.value->m_id);

                        // Make sure the type sym is added to the dead syms vector as well, because type syms are
                        // created in backward pass and so their symIds > maxInitialSymID.
                        if (sym->IsStackSym() && sym->AsStackSym()->HasObjectTypeSym())
                        {
                            deadSymsBv.Set(sym->AsStackSym()->GetObjectTypeSym()->m_id);
                        }
                    }
                    availableValueNumbers.Set(val->GetValueNumber());
                }
                else
                {
                    // Make sure the type sym is added to the dead syms vector as well, because type syms are
                    // created in backward pass and so their symIds > maxInitialSymID. Perhaps we could remove
                    // it explicitly here, but would it work alright with the iterator?
                    if (sym->IsStackSym() && sym->AsStackSym()->HasObjectTypeSym())
                    {
                        deadSymsBv.Set(sym->AsStackSym()->GetObjectTypeSym()->m_id);
                    }

                    // Not a copy-prop candidate; delete it right away.
                    iter.RemoveCurrent(thisTable->alloc);
                    this->currentBlock->globOptData.liveInt32Syms->Clear(sym->m_id);
                    this->currentBlock->globOptData.liveLossyInt32Syms->Clear(sym->m_id);
                    this->currentBlock->globOptData.liveFloat64Syms->Clear(sym->m_id);
                }
            }
            else
            {
                Sym * sym = bucket.value;

                if (sym->IsPropertySym() && !this->currentBlock->globOptData.liveFields->Test(sym->m_id))
                {
                    // Remove propertySyms which are not live anymore.
                    iter.RemoveCurrent(thisTable->alloc);
                    this->currentBlock->globOptData.liveInt32Syms->Clear(sym->m_id);
                    this->currentBlock->globOptData.liveLossyInt32Syms->Clear(sym->m_id);
                    this->currentBlock->globOptData.liveFloat64Syms->Clear(sym->m_id);
                }
                else
                {
                    // Look at the copy-prop candidate. We don't want to get rid of the data for a symbol which is
                    // a copy-prop candidate.
                    Value *val = bucket.element;
                    ValueInfo *valueInfo = val->GetValueInfo();

                    Sym *symStore = valueInfo->GetSymStore();

                    if (symStore && symStore != bucket.value)
                    {
                        keepAliveSymsBv.Set(symStore->m_id);
                        if (symStore->IsStackSym() && symStore->AsStackSym()->HasObjectTypeSym())
                        {
                            keepAliveSymsBv.Set(symStore->AsStackSym()->GetObjectTypeSym()->m_id);
                        }
                    }
                    availableValueNumbers.Set(val->GetValueNumber());
                }
            }
        } NEXT_SLISTBASE_ENTRY_EDITING;
    }

    deadSymsBv.Minus(&keepAliveSymsBv);

    // Now cleanup exprToValueMap table
    ExprHashTable *thisExprTable = this->currentBlock->globOptData.exprToValueMap;
    bool oldHasCSECandidatesValue = this->currentBlock->globOptData.hasCSECandidates;  // Could be false if none need bailout.
    this->currentBlock->globOptData.hasCSECandidates = false;

    for (uint i = 0; i < thisExprTable->tableSize; i++)
    {
        FOREACH_SLISTBASE_ENTRY_EDITING(ExprHashBucket, bucket, &thisExprTable->table[i], iter)
        {
            ExprHash hash = bucket.value;
            ValueNumber src1ValNum = hash.GetSrc1ValueNumber();
            ValueNumber src2ValNum = hash.GetSrc2ValueNumber();

            // If src1Val or src2Val are not available anymore, no point keeping this CSE candidate
            bool removeCurrent = false;
            if ((src1ValNum && !availableValueNumbers.Test(src1ValNum))
                || (src2ValNum && !availableValueNumbers.Test(src2ValNum)))
            {
                removeCurrent = true;
            }
            else
            {
                // If we are keeping this value, make sure we also keep the symStore in the value table
                removeCurrent = true; // Remove by default, unless it's set to false later below.
                Value *val = bucket.element;
                if (val)
                {
                    Sym *symStore = val->GetValueInfo()->GetSymStore();
                    if (symStore)
                    {
                        Value *symStoreVal = this->FindValue(this->currentBlock->globOptData.symToValueMap, symStore);

                        if (symStoreVal && symStoreVal->GetValueNumber() == val->GetValueNumber())
                        {
                            removeCurrent = false;
                            deadSymsBv.Clear(symStore->m_id);
                            if (symStore->IsStackSym() && symStore->AsStackSym()->HasObjectTypeSym())
                            {
                                deadSymsBv.Clear(symStore->AsStackSym()->GetObjectTypeSym()->m_id);
                            }
                        }
                    }
                }
            }

            if(removeCurrent)
            {
                iter.RemoveCurrent(thisExprTable->alloc);
            }
            else
            {
                this->currentBlock->globOptData.hasCSECandidates = oldHasCSECandidatesValue;
            }
        } NEXT_SLISTBASE_ENTRY_EDITING;
    }

    FOREACH_BITSET_IN_SPARSEBV(dead_id, &deadSymsBv)
    {
        thisTable->Clear(dead_id);
    }
    NEXT_BITSET_IN_SPARSEBV;

    if (!deadSymsBv.IsEmpty())
    {
        if (this->func->IsJitInDebugMode())
        {
            // Do not remove non-temp local vars from liveVarSyms (i.e. do not let them become dead).
            // We will need to restore all initialized/used so far non-temp local during bail out.
            // (See BackwardPass::ProcessBailOutInfo)
            Assert(this->func->m_nonTempLocalVars);
            BVSparse<JitArenaAllocator> tempBv(this->tempAlloc);
            tempBv.Minus(&deadSymsBv, this->func->m_nonTempLocalVars);
            this->currentBlock->globOptData.liveVarSyms->Minus(&tempBv);
#if DBG
            tempBv.And(this->currentBlock->globOptData.liveInt32Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveInt32Syms?");
            tempBv.And(this->currentBlock->globOptData.liveLossyInt32Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveLossyInt32Syms?");
            tempBv.And(this->currentBlock->globOptData.liveFloat64Syms, this->func->m_nonTempLocalVars);
            AssertMsg(tempBv.IsEmpty(), "Type spec is disabled under debugger. How come did we get a non-temp local in liveFloat64Syms?");
#endif
        }
        else
        {
            this->currentBlock->globOptData.liveVarSyms->Minus(&deadSymsBv);
        }

        this->currentBlock->globOptData.liveInt32Syms->Minus(&deadSymsBv);
        this->currentBlock->globOptData.liveLossyInt32Syms->Minus(&deadSymsBv);
        this->currentBlock->globOptData.liveFloat64Syms->Minus(&deadSymsBv);
    }

    JitAdelete(this->alloc, upwardExposedUses);
    this->currentBlock->upwardExposedUses = nullptr;
    JitAdelete(this->alloc, upwardExposedFields);
    this->currentBlock->upwardExposedFields = nullptr;
    if (this->currentBlock->cloneStrCandidates)
    {
        JitAdelete(this->alloc, this->currentBlock->cloneStrCandidates);
        this->currentBlock->cloneStrCandidates = nullptr;
    }
}

PRECandidatesList * GlobOpt::RemoveUnavailableCandidates(BasicBlock *block, PRECandidatesList *candidates, JitArenaAllocator *alloc)
{
    // In case of multiple back-edges to the loop, make sure the candidates are still valid.
    FOREACH_SLIST_ENTRY_EDITING(GlobHashBucket*, candidate, (SList<GlobHashBucket*>*)candidates, iter)
    {
        Value *candidateValue = candidate->element;
        PropertySym *candidatePropertySym = candidate->value->AsPropertySym();
        ValueNumber valueNumber = candidateValue->GetValueNumber();
        Sym *symStore = candidateValue->GetValueInfo()->GetSymStore();

        Value *blockValue = this->FindValue(block->globOptData.symToValueMap, candidatePropertySym);

        if (blockValue && blockValue->GetValueNumber() == valueNumber
            && blockValue->GetValueInfo()->GetSymStore() == symStore)
        {
            Value *symStoreValue = this->FindValue(block->globOptData.symToValueMap, symStore);

            if (symStoreValue && symStoreValue->GetValueNumber() == valueNumber)
            {
                continue;
            }
        }

        iter.RemoveCurrent();
    } NEXT_SLIST_ENTRY_EDITING;

    return candidates;
}

Value *
GlobOpt::MergeValues(
    Value *toDataValue,
    Value *fromDataValue,
    Sym *fromDataSym,
    GlobOptBlockData *toData,
    GlobOptBlockData *fromData,
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
            fromData,
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
    Value *newValue = valuesCreatedForMerge->Lookup(sourceValueNumberPair, nullptr);
    if(newValue)
    {
        Assert(sameValueNumber == (newValue->GetValueNumber() == toDataValue->GetValueNumber()));

        // This is an exception where Value::SetValueInfo is called directly instead of GlobOpt::ChangeValueInfo, because we're
        // actually generating new value info through merges.
        newValue->SetValueInfo(newValueInfo);
    }
    else
    {
        newValue = NewValue(sameValueNumber ? sourceValueNumberPair.First() : NewValueNumber(), newValueInfo);
        valuesCreatedForMerge->Add(sourceValueNumberPair, newValue);
    }

    // Set symStore if same on both paths.
    if (toDataValue->GetValueInfo()->GetSymStore() == fromDataValue->GetValueInfo()->GetSymStore())
    {
        this->SetSymStoreDirect(newValueInfo, toDataValue->GetValueInfo()->GetSymStore());
    }

    return newValue;
}

ValueInfo *
GlobOpt::MergeValueInfo(
    Value *toDataVal,
    Value *fromDataVal,
    Sym *fromDataSym,
    GlobOptBlockData *fromData,
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
        return MergeJsTypeValueInfo(toDataValueInfo->AsJsType(), fromDataValueInfo->AsJsType(), isLoopBackEdge, sameValueNumber);
    }

    ValueType newValueType(toDataValueInfo->Type().Merge(fromDataValueInfo->Type()));
    if (newValueType.IsLikelyInt())
    {
        return MergeLikelyIntValueInfo(toDataVal, fromDataVal, newValueType);
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
            return ValueInfo::New(alloc, newValueType);
        }

        return
            MergeArrayValueInfo(
                newValueType,
                toDataValueInfo->AsArrayValueInfo(),
                fromDataValueInfo->AsArrayValueInfo(),
                fromDataSym,
                symsRequiringCompensation,
                symsCreatedForMerge);
    }

    // Consider: If both values are VarConstantValueInfo with the same value, we could
    // merge them preserving the value.
    return ValueInfo::New(this->alloc, newValueType);
}

void
GlobOpt::TrackArgumentsSym(IR::RegOpnd const* opnd)
{
    if(!currentBlock->globOptData.curFunc->argObjSyms)
    {
        currentBlock->globOptData.curFunc->argObjSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    }
    currentBlock->globOptData.curFunc->argObjSyms->Set(opnd->m_sym->m_id);
    currentBlock->globOptData.argObjSyms->Set(opnd->m_sym->m_id);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (PHASE_TESTTRACE(Js::StackArgOptPhase, this->func))
    {
        char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        char16 debugStringBuffer2[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
        Output::Print(_u("Created a new alias s%d for arguments object in function %s(%s) topFunc %s(%s)\n"),
            opnd->m_sym->m_id,
            currentBlock->globOptData.curFunc->GetJITFunctionBody()->GetDisplayName(),
            currentBlock->globOptData.curFunc->GetDebugNumberSet(debugStringBuffer),
            this->func->GetJITFunctionBody()->GetDisplayName(),
            this->func->GetDebugNumberSet(debugStringBuffer2)
            );
        Output::Flush();
    }
#endif
}

BOOLEAN
GlobOpt::IsArgumentsSymID(SymID id, const GlobOptBlockData& blockData) const
{
    return blockData.argObjSyms->Test(id);
}

BOOLEAN
GlobOpt::IsArgumentsOpnd(IR::Opnd const* opnd) const
{
    SymID id = 0;
    if (opnd->IsRegOpnd())
    {
        id = opnd->AsRegOpnd()->m_sym->m_id;
        return IsArgumentsSymID(id, this->currentBlock->globOptData);
    }
    else if (opnd->IsSymOpnd())
    {
        Sym const *sym = opnd->AsSymOpnd()->m_sym;
        if (sym && sym->IsPropertySym())
        {
            PropertySym const *propertySym = sym->AsPropertySym();
            id = propertySym->m_stackSym->m_id;
            return IsArgumentsSymID(id, this->currentBlock->globOptData);
        }
        return false;
    }
    else if (opnd->IsIndirOpnd())
    {
        IR::RegOpnd const *indexOpnd = opnd->AsIndirOpnd()->GetIndexOpnd();
        IR::RegOpnd const *baseOpnd = opnd->AsIndirOpnd()->GetBaseOpnd();
        return IsArgumentsSymID(baseOpnd->m_sym->m_id, this->currentBlock->globOptData) || (indexOpnd && IsArgumentsSymID(indexOpnd->m_sym->m_id, this->currentBlock->globOptData));
    }
    AssertMsg(false, "Unknown type");
    return false;
}

void
GlobOpt::ClearArgumentsSym(IR::RegOpnd const* opnd)
{
    // We blindly clear so need to check func has argObjSyms
    if (currentBlock->globOptData.curFunc->argObjSyms)
    {
        currentBlock->globOptData.curFunc->argObjSyms->Clear(opnd->m_sym->m_id);
    }
    currentBlock->globOptData.argObjSyms->Clear(opnd->m_sym->m_id);
}

BOOLEAN
GlobOpt::TestAnyArgumentsSym() const
{
    return currentBlock->globOptData.argObjSyms->TestEmpty();
}

Value*
GlobOpt::FindValue(GlobOptBlockData* blockData, Sym *sym)
{
    return FindValue(blockData->symToValueMap, sym);
}

Value*
GlobOpt::FindValue(GlobHashTable *valueNumberMap, Sym *sym)
{
    Assert(valueNumberMap);

    if (sym->IsStackSym() && sym->AsStackSym()->IsTypeSpec())
    {
        sym = sym->AsStackSym()->GetVarEquivSym(this->func);
    }
    else if (sym->IsPropertySym())
    {
        return FindPropertyValue(valueNumberMap, sym->m_id);
    }

    if (sym->IsStackSym() && sym->AsStackSym()->IsFromByteCodeConstantTable())
    {
        return this->byteCodeConstantValueArray->Get(sym->m_id);
    }
    else
    {
        return FindValueFromHashTable(valueNumberMap, sym->m_id);
    }
}

ValueNumber
GlobOpt::FindValueNumber(GlobHashTable *valueNumberMap, Sym *sym)
{
    Value *val = FindValue(valueNumberMap, sym);

    return val->GetValueNumber();
}

Value *
GlobOpt::FindPropertyValue(GlobHashTable *valueNumberMap, SymID symId)
{
    Assert(this->func->m_symTable->Find(symId)->IsPropertySym());
    if (!this->currentBlock->globOptData.liveFields->Test(symId))
    {
        Assert(!IsHoistablePropertySym(symId));
        return nullptr;
    }
    return FindValueFromHashTable(valueNumberMap, symId);
}

ValueNumber
GlobOpt::FindPropertyValueNumber(GlobHashTable *valueNumberMap, SymID symId)
{
    Value *val = FindPropertyValue(valueNumberMap, symId);

    return val->GetValueNumber();
}

Value *
GlobOpt::FindObjectTypeValue(StackSym* typeSym)
{
    return FindObjectTypeValue(typeSym, this->currentBlock->globOptData.symToValueMap);
}

Value *
GlobOpt::FindObjectTypeValue(StackSym* typeSym, BasicBlock* block)
{
    return FindObjectTypeValue(typeSym->m_id, block);
}

Value *
GlobOpt::FindObjectTypeValue(SymID typeSymId, BasicBlock* block)
{
    return FindObjectTypeValue(typeSymId, block->globOptData.symToValueMap, block->globOptData.liveFields);
}

Value *
GlobOpt::FindObjectTypeValue(StackSym* typeSym, GlobHashTable *valueNumberMap)
{
    return FindObjectTypeValue(typeSym->m_id, valueNumberMap);
}

Value *
GlobOpt::FindObjectTypeValue(SymID typeSymId, GlobHashTable *valueNumberMap)
{
    return FindObjectTypeValue(typeSymId, valueNumberMap, this->currentBlock->globOptData.liveFields);
}

Value *
GlobOpt::FindObjectTypeValue(StackSym* typeSym, GlobHashTable *valueNumberMap, BVSparse<JitArenaAllocator>* liveFields)
{
    return FindObjectTypeValue(typeSym->m_id, valueNumberMap, liveFields);
}

Value *
GlobOpt::FindObjectTypeValue(SymID typeSymId, GlobHashTable *valueNumberMap, BVSparse<JitArenaAllocator>* liveFields)
{
    Assert(this->func->m_symTable->Find(typeSymId)->IsStackSym());
    if (!liveFields->Test(typeSymId))
    {
        return nullptr;
    }
    Value* value = FindValueFromHashTable(valueNumberMap, typeSymId);
    Assert(value == nullptr || value->GetValueInfo()->IsJsType());
    return value;
}

Value *
GlobOpt::FindFuturePropertyValue(PropertySym *const propertySym)
{
    Assert(propertySym);

    // Try a direct lookup based on this sym
    Value *const value = FindValue(&this->currentBlock->globOptData, propertySym);
    if(value)
    {
        return value;
    }

    if(PHASE_OFF(Js::CopyPropPhase, func))
    {
        // Need to use copy-prop info to backtrack
        return nullptr;
    }

    // Try to get the property object's value
    StackSym *const objectSym = propertySym->m_stackSym;
    Value *objectValue = FindValue(&this->currentBlock->globOptData, objectSym);
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
            objectValue = FindValue(&this->currentBlock->globOptData, objectTransferSrc->AsRegOpnd()->m_sym);
        }
        else if(objectTransferSrc->IsSymOpnd())
        {
            Sym *const objectTransferSrcSym = objectTransferSrc->AsSymOpnd()->m_sym;
            if(objectTransferSrcSym->IsStackSym())
            {
                objectValue = FindValue(&this->currentBlock->globOptData, objectTransferSrcSym);
            }
            else
            {
                // About to make a recursive call, so when jitting in the foreground, probe the stack
                if(!func->IsBackgroundJIT())
                {
                    PROBE_STACK(func->GetScriptContext(), Js::Constants::MinStackDefault);
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
    StackSym *const objectCopyPropSym = GetCopyPropSym(nullptr, objectValue);
    if(!objectCopyPropSym)
    {
        return nullptr;
    }
    PropertySym *const propertyCopyPropSym = PropertySym::Find(objectCopyPropSym->m_id, propertySym->m_propertyId, func);
    if(!propertyCopyPropSym)
    {
        return nullptr;
    }
    return FindValue(&this->currentBlock->globOptData, propertyCopyPropSym);
}

Value *
GlobOpt::FindValueFromHashTable(GlobHashTable *valueNumberMap, SymID symId)
{
    Value ** valuePtr = valueNumberMap->Get(symId);

    if (valuePtr == nullptr)
    {
        return 0;
    }

    return (*valuePtr);
}

StackSym *
GlobOpt::GetCopyPropSym(Sym * sym, Value * value)
{
    return GetCopyPropSym(this->currentBlock, sym, value);
}

StackSym *
GlobOpt::GetCopyPropSym(BasicBlock * block, Sym * sym, Value * value)
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
        Value *copySymVal = this->FindValue(block->globOptData.symToValueMap, valueInfo->GetSymStore());
        if (copySymVal && copySymVal->GetValueNumber() == value->GetValueNumber())
        {
            if (valueInfo->IsVarConstant() && !GlobOpt::IsLive(copySym, block))
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
GlobOpt::MarkTempLastUse(IR::Instr *instr, IR::RegOpnd *regOpnd)
{
    if (OpCodeAttr::NonTempNumberSources(instr->m_opcode))
    {
        // Turn off bit if opcode could cause the src to be aliased.
        this->currentBlock->globOptData.isTempSrc->Clear(regOpnd->m_sym->m_id);
    }
    else if (this->currentBlock->globOptData.isTempSrc->Test(regOpnd->m_sym->m_id))
    {
        // We just mark things that are temp in the globopt phase.
        // The backwards phase will turn this off if it is not the last use.
        // The isTempSrc is freed at the end of each block, which is why the backwards phase can't
        // just use it.
        if (!PHASE_OFF(Js::BackwardPhase, this->func) && !this->IsLoopPrePass())
        {
            regOpnd->m_isTempLastUse = true;
        }
    }
}

Value *
GlobOpt::InsertNewValue(Value *val, IR::Opnd *opnd)
{
    return this->InsertNewValue(&this->currentBlock->globOptData, val, opnd);
}

Value *
GlobOpt::InsertNewValue(GlobOptBlockData *blockData, Value *val, IR::Opnd *opnd)
{
    return this->SetValue(blockData, val, opnd);
}

void
GlobOpt::SetValueToHashTable(GlobHashTable *valueNumberMap, Value *val, Sym *sym)
{
    Value **pValue = valueNumberMap->FindOrInsertNew(sym);
    *pValue = val;
}

Sym *
GlobOpt::SetSymStore(ValueInfo *valueInfo, Sym *sym)
{
    if (sym->IsStackSym())
    {
        StackSym *stackSym = sym->AsStackSym();

        if (stackSym->IsTypeSpec())
        {
            stackSym = stackSym->GetVarEquivSym(this->func);
            sym = stackSym;
        }
    }
    if (valueInfo->GetSymStore() == nullptr || valueInfo->GetSymStore()->IsPropertySym())
    {
        SetSymStoreDirect(valueInfo, sym);
    }

    return sym;
}

void
GlobOpt::SetSymStoreDirect(ValueInfo * valueInfo, Sym * sym)
{
    Sym * prevSymStore = valueInfo->GetSymStore();
    if (prevSymStore && prevSymStore->IsStackSym() &&
        prevSymStore->AsStackSym()->HasByteCodeRegSlot())
    {
        this->SetChangedSym(prevSymStore->m_id);
    }
    valueInfo->SetSymStore(sym);
}

void
GlobOpt::SetChangedSym(SymID symId)
{
    // this->currentBlock might not be the one which contain the changing symId,
    // like hoisting invariant, but more changed symId is overly conservative and safe.
    // symId in the hoisted to block is marked as JITOptimizedReg so it does't affect bailout.
    GlobOptBlockData * globOptData = &this->currentBlock->globOptData;
    if (globOptData->changedSyms)
    {
        globOptData = &this->currentBlock->globOptData;

        globOptData->changedSyms->Set(symId);
        if (globOptData->capturedValuesCandidate != nullptr)
        {
            this->changedSymsAfterIncBailoutCandidate->Set(symId);
        }
    }
    // else could be hit only in MergeValues and it is handled by MergeCapturedValues
}

void
GlobOpt::SetValue(GlobOptBlockData *blockData, Value *val, Sym * sym)
{
    ValueInfo *valueInfo = val->GetValueInfo();

    sym = this->SetSymStore(valueInfo, sym);
    bool isStackSym = sym->IsStackSym();

    if (isStackSym && sym->AsStackSym()->IsFromByteCodeConstantTable())
    {
        // Put the constants in a global array. This will minimize the per-block info.
        this->byteCodeConstantValueArray->Set(sym->m_id, val);
        this->byteCodeConstantValueNumbersBv->Set(val->GetValueNumber());
    }
    else
    {
        SetValueToHashTable(blockData->symToValueMap, val, sym);
        if (isStackSym && sym->AsStackSym()->HasByteCodeRegSlot())
        {
            this->SetChangedSym(sym->m_id);
        }
    }
}

Value *
GlobOpt::SetValue(GlobOptBlockData *blockData, Value *val, IR::Opnd *opnd)
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
            SetValue(blockData, val, sym);
        }
    }

    return val;
}

BOOL
GlobOpt::IsInt32TypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsInt32TypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsSwitchInt32TypeSpecialized(IR::Instr const * instr, BasicBlock const * block)
{
    return IsSwitchOptEnabled(instr->m_func->GetTopFunc()) && instr->GetSrc1()->IsRegOpnd() &&
                                    IsInt32TypeSpecialized(instr->GetSrc1()->AsRegOpnd()->m_sym, block);
}

BOOL
GlobOpt::IsInt32TypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && data->liveInt32Syms->Test(sym->m_id) && !data->liveLossyInt32Syms->Test(sym->m_id);
}

BOOL
GlobOpt::IsFloat64TypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsFloat64TypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsFloat64TypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && data->liveFloat64Syms->Test(sym->m_id);
}

// SIMD_JS
BOOL
GlobOpt::IsSimd128TypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsSimd128TypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsSimd128TypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (data->liveSimd128F4Syms->Test(sym->m_id) || data->liveSimd128I4Syms->Test(sym->m_id));
}

BOOL
GlobOpt::IsSimd128TypeSpecialized(IRType type, Sym const * sym, BasicBlock const * block)
{
    return IsSimd128TypeSpecialized(type, sym, &block->globOptData);
}

BOOL
GlobOpt::IsSimd128TypeSpecialized(IRType type, Sym const * sym, GlobOptBlockData const * data)
{
    switch (type)
    {
    case TySimd128F4:
        return IsSimd128F4TypeSpecialized(sym, data);
    case TySimd128I4:
        return IsSimd128I4TypeSpecialized(sym, data);
    default:
        Assert(UNREACHED);
        return false;
    }
}

BOOL
GlobOpt::IsSimd128F4TypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsSimd128F4TypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsSimd128F4TypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (data->liveSimd128F4Syms->Test(sym->m_id));
}

BOOL
GlobOpt::IsSimd128I4TypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsSimd128I4TypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsSimd128I4TypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && (data->liveSimd128I4Syms->Test(sym->m_id));
}

BOOL
GlobOpt::IsLiveAsSimd128(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return
        sym &&
        (
        data->liveSimd128F4Syms->Test(sym->m_id) ||
        data->liveSimd128I4Syms->Test(sym->m_id)
        );
}

BOOL
GlobOpt::IsLiveAsSimd128F4(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && data->liveSimd128F4Syms->Test(sym->m_id);
}

BOOL
GlobOpt::IsLiveAsSimd128I4(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return sym && data->liveSimd128I4Syms->Test(sym->m_id);
}

BOOL
GlobOpt::IsTypeSpecialized(Sym const * sym, BasicBlock const * block)
{
    return IsTypeSpecialized(sym, &block->globOptData);
}

BOOL
GlobOpt::IsTypeSpecialized(Sym const * sym, GlobOptBlockData const * data)
{
    return IsInt32TypeSpecialized(sym, data) || IsFloat64TypeSpecialized(sym, data) || IsSimd128TypeSpecialized(sym, data);
}

BOOL
GlobOpt::IsLive(Sym const * sym, BasicBlock const * block)
{
    return IsLive(sym, &block->globOptData);
}

BOOL
GlobOpt::IsLive(Sym const * sym, GlobOptBlockData const * data)
{
    sym = StackSym::GetVarEquivStackSym_NoCreate(sym);
    return
        sym &&
        (
        data->liveVarSyms->Test(sym->m_id) ||
        data->liveInt32Syms->Test(sym->m_id) ||
        data->liveFloat64Syms->Test(sym->m_id) ||
        data->liveSimd128F4Syms->Test(sym->m_id) ||
        data->liveSimd128I4Syms->Test(sym->m_id)
        );
}

void
GlobOpt::MakeLive(StackSym *const sym, GlobOptBlockData *const blockData, const bool lossy) const
{
    Assert(sym);
    Assert(blockData);

    if(sym->IsTypeSpec())
    {
        const SymID varSymId = sym->GetVarEquivSym(func)->m_id;
        if(sym->IsInt32())
        {
            blockData->liveInt32Syms->Set(varSymId);
            if(lossy)
            {
                blockData->liveLossyInt32Syms->Set(varSymId);
            }
            else
            {
                blockData->liveLossyInt32Syms->Clear(varSymId);
            }
            return;
        }

        if (sym->IsFloat64())
        {
            blockData->liveFloat64Syms->Set(varSymId);
            return;
        }

        // SIMD_JS
        if (sym->IsSimd128F4())
        {
            blockData->liveSimd128F4Syms->Set(varSymId);
            return;
        }

        if (sym->IsSimd128I4())
        {
            blockData->liveSimd128I4Syms->Set(varSymId);
            return;
        }

    }

    blockData->liveVarSyms->Set(sym->m_id);
}