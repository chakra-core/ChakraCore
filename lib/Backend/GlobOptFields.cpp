//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

bool
GlobOpt::DoFieldCopyProp() const
{
    BasicBlock *block = this->currentBlock;
    Loop *loop = block->loop;
    if (this->isRecursiveCallOnLandingPad)
    {
        // The landing pad at this point only contains load hosted by PRE.
        // These need to be copy-prop'd into the loop.
        // We want to look at the implicit-call info of the loop, not it's parent.

        Assert(block->IsLandingPad());
        loop = block->next->loop;
        Assert(loop);
    }

    return DoFieldCopyProp(loop);
}

bool
GlobOpt::DoFunctionFieldCopyProp() const
{
    return DoFieldCopyProp(nullptr);
}

bool
GlobOpt::DoFieldCopyProp(Loop * loop) const
{
    if (PHASE_OFF(Js::CopyPropPhase, this->func))
    {
        // Can't do field copy prop without copy prop
        return false;
    }

    if (PHASE_FORCE(Js::FieldCopyPropPhase, this->func))
    {
        // Force always turns on field copy prop
        return true;
    }

    if (PHASE_OFF(Js::FieldCopyPropPhase, this->func))
    {
        return false;
    }

    return this->DoFieldOpts(loop);
}

bool
GlobOpt::DoObjTypeSpec() const
{
    return this->DoObjTypeSpec(this->currentBlock->loop);
}

bool
GlobOpt::DoObjTypeSpec(Loop *loop) const
{
    if (!this->func->DoFastPaths())
    {
        return false;
    }
    if (PHASE_FORCE(Js::ObjTypeSpecPhase, this->func))
    {
        return true;
    }
    if (PHASE_OFF(Js::ObjTypeSpecPhase, this->func))
    {
        return false;
    }
    if (this->func->IsLoopBody() && this->func->HasProfileInfo() && this->func->GetReadOnlyProfileInfo()->IsObjTypeSpecDisabledInJitLoopBody())
    {
        return false;
    }
    if (this->ImplicitCallFlagsAllowOpts(this->func))
    {
        Assert(loop == nullptr || loop->CanDoFieldCopyProp());
        return true;
    }
    return loop != nullptr && loop->CanDoFieldCopyProp();
}

bool
GlobOpt::DoFieldOpts(Loop * loop) const
{
    if (this->ImplicitCallFlagsAllowOpts(this->func))
    {
        Assert(loop == nullptr || loop->CanDoFieldCopyProp());
        return true;
    }
    return loop != nullptr && loop->CanDoFieldCopyProp();
}

bool GlobOpt::DoFieldPRE() const
{
    Loop *loop = this->currentBlock->loop;

    return DoFieldPRE(loop);
}

bool
GlobOpt::DoFieldPRE(Loop *loop) const
{
    if (PHASE_OFF(Js::FieldPREPhase, this->func))
    {
        return false;
    }

    if (PHASE_FORCE(Js::FieldPREPhase, func))
    {
        // Force always turns on field PRE
        return true;
    }

    if (this->func->HasProfileInfo() && this->func->GetReadOnlyProfileInfo()->IsFieldPREDisabled())
    {
        return false;
    }

    return DoFieldOpts(loop);
}

bool GlobOpt::HasMemOp(Loop *loop)
{
#pragma prefast(suppress: 6285, "logical-or of constants is by design")
    return (
        loop &&
        loop->doMemOp &&
        (
            !PHASE_OFF(Js::MemSetPhase, this->func) ||
            !PHASE_OFF(Js::MemCopyPhase, this->func)
        ) &&
        loop->memOpInfo &&
        loop->memOpInfo->candidates &&
        !loop->memOpInfo->candidates->Empty()
    );
}

void
GlobOpt::KillLiveFields(StackSym * stackSym, BVSparse<JitArenaAllocator> * bv)
{
    if (stackSym->IsTypeSpec())
    {
        stackSym = stackSym->GetVarEquivSym(this->func);
    }
    Assert(stackSym);

    // If the sym has no objectSymInfo, it must not represent an object and, hence, has no type sym or
    // property syms to kill.
    if (!stackSym->HasObjectInfo() || stackSym->IsSingleDef())
    {
        return;
    }

    // Note that the m_writeGuardSym is killed here as well, because it is part of the
    // m_propertySymList of the object.
    ObjectSymInfo * objectSymInfo = stackSym->GetObjectInfo();
    PropertySym * propertySym = objectSymInfo->m_propertySymList;
    while (propertySym != nullptr)
    {
        Assert(propertySym->m_stackSym == stackSym);
        bv->Clear(propertySym->m_id);
        if (this->IsLoopPrePass())
        {
            for (Loop * loop = this->rootLoopPrePass; loop != nullptr; loop = loop->parent)
            {
                loop->fieldKilled->Set(propertySym->m_id);
            }
        }
        else if (bv->IsEmpty())
        {
            // shortcut
            break;
        }
        propertySym = propertySym->m_nextInStackSymList;
    }

    this->KillObjectType(stackSym, bv);
}

void
GlobOpt::KillLiveFields(PropertySym * propertySym, BVSparse<JitArenaAllocator> * bv)
{
    KillLiveFields(propertySym->m_propertyEquivSet, bv);
}

void GlobOpt::KillLiveFields(BVSparse<JitArenaAllocator> *const fieldsToKill, BVSparse<JitArenaAllocator> *const bv) const
{
    Assert(bv);

    if (fieldsToKill)
    {
        bv->Minus(fieldsToKill);

        if (this->IsLoopPrePass())
        {
            for (Loop * loop = this->rootLoopPrePass; loop != nullptr; loop = loop->parent)
            {
                loop->fieldKilled->Or(fieldsToKill);
            }
        }
    }
}

void
GlobOpt::KillLiveElems(IR::IndirOpnd * indirOpnd, IR::Opnd * valueOpnd, BVSparse<JitArenaAllocator> * bv, bool inGlobOpt, Func *func)
{
    IR::RegOpnd *indexOpnd = indirOpnd->GetIndexOpnd();

    // obj.x = 10;
    // obj["x"] = ...;   // This needs to kill obj.x...  We need to kill all fields...
    //
    // Also, 'arguments[i] =' needs to kill all slots even if 'i' is an int.
    //
    // NOTE: we only need to kill slots here, not all fields. It may be good to separate these one day.
    //
    // Regarding the check for type specialization:
    // - Type specialization does not always update the value to a definite type.
    // - The loop prepass is conservative on values even when type specialization occurs.
    // - We check the type specialization status for the sym as well. For the purpose of doing kills, we can assume that
    //   if type specialization happened, that fields don't need to be killed. Note that they may be killed in the next
    //   pass based on the value.
    bool isSafeToTransfer = true;
    if (func->GetThisOrParentInlinerHasArguments() || this->IsNonNumericRegOpnd(indexOpnd, inGlobOpt, &isSafeToTransfer))
    {
        this->KillAllFields(bv); // This also kills all property type values, as the same bit-vector tracks those stack syms
        SetAnyPropertyMayBeWrittenTo();
    }
    else if (inGlobOpt)
    {
        Value * indexValue = indexOpnd ? this->currentBlock->globOptData.FindValue(indexOpnd->GetSym()) : nullptr;
        ValueInfo * indexValueInfo = indexValue ? indexValue->GetValueInfo() : nullptr;
        int indexLowerBound = 0;

        if (!isSafeToTransfer || indirOpnd->GetOffset() < 0 || (indexOpnd && (!indexValueInfo || !indexValueInfo->TryGetIntConstantLowerBound(&indexLowerBound, false) || indexLowerBound < 0)))
        {
            // Write/delete to a non-integer numeric index can't alias a name on the RHS of a dot, but it change object layout
            this->KillAllObjectTypes(bv);
        }
        else if ((!valueOpnd || valueOpnd->IsVar()) && this->objectTypeSyms != nullptr)
        {
            // If we wind up converting a native array, block final-type opt at this point, because we could evolve
            // to a type with the wrong type ID. Do this by noting that we may have evolved any type and so must
            // check it before evolving it further.
            IR::RegOpnd *baseOpnd = indirOpnd->GetBaseOpnd();
            Value * baseValue = baseOpnd ? this->currentBlock->globOptData.FindValue(baseOpnd->m_sym) : nullptr;
            ValueInfo * baseValueInfo = baseValue ? baseValue->GetValueInfo() : nullptr;
            if (!baseValueInfo || !baseValueInfo->IsNotNativeArray())
            {
                if (this->currentBlock->globOptData.maybeWrittenTypeSyms == nullptr)
                {
                    this->currentBlock->globOptData.maybeWrittenTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
                }
                this->currentBlock->globOptData.maybeWrittenTypeSyms->Or(this->objectTypeSyms);
            }
        }
    }
}

void
GlobOpt::KillAllFields(BVSparse<JitArenaAllocator> * bv)
{
    bv->ClearAll();
    if (this->IsLoopPrePass())
    {
        for (Loop * loop = this->rootLoopPrePass; loop != nullptr; loop = loop->parent)
        {
            loop->allFieldsKilled = true;
        }
    }
}

void
GlobOpt::SetAnyPropertyMayBeWrittenTo()
{
    this->func->anyPropertyMayBeWrittenTo = true;
}

void
GlobOpt::AddToPropertiesWrittenTo(Js::PropertyId propertyId)
{
    this->func->EnsurePropertiesWrittenTo();
    this->func->propertiesWrittenTo->Item(propertyId);
}

void
GlobOpt::ProcessFieldKills(IR::Instr *instr, BVSparse<JitArenaAllocator> *bv, bool inGlobOpt)
{
    if (bv->IsEmpty() && (!this->IsLoopPrePass() || this->rootLoopPrePass->allFieldsKilled))
    {
        return;
    }

    if (instr->m_opcode == Js::OpCode::FromVar || instr->m_opcode == Js::OpCode::Conv_Prim)
    {
        return;
    }

    IR::Opnd * dstOpnd = instr->GetDst();
    if (dstOpnd)
    {
        if (dstOpnd->IsRegOpnd())
        {
            Sym * sym = dstOpnd->AsRegOpnd()->m_sym;
            if (sym->IsStackSym())
            {
                KillLiveFields(sym->AsStackSym(), bv);
            }
        }
        else if (dstOpnd->IsSymOpnd())
        {
            Sym * sym = dstOpnd->AsSymOpnd()->m_sym;
            if (sym->IsStackSym())
            {
                KillLiveFields(sym->AsStackSym(), bv);
            }
            else
            {
                Assert(sym->IsPropertySym());
                if (instr->m_opcode == Js::OpCode::InitLetFld || instr->m_opcode == Js::OpCode::InitConstFld || instr->m_opcode == Js::OpCode::InitFld)
                {
                    // These can grow the aux slot of the activation object.
                    // We need to kill the slot array sym as well.
                    PropertySym * slotArraySym = PropertySym::Find(sym->AsPropertySym()->m_stackSym->m_id,
                        (Js::DynamicObject::GetOffsetOfAuxSlots())/sizeof(Js::Var) /*, PropertyKindSlotArray */, instr->m_func);
                    if (slotArraySym)
                    {
                        bv->Clear(slotArraySym->m_id);
                    }
                }
            }
        }
    }

    if (bv->IsEmpty() && (!this->IsLoopPrePass() || this->rootLoopPrePass->allFieldsKilled))
    {
        return;
    }

    Sym *sym;
    IR::JnHelperMethod fnHelper;
    switch(instr->m_opcode)
    {
    case Js::OpCode::StElemC:
    case Js::OpCode::StElemI_A:
    case Js::OpCode::StElemI_A_Strict:
        Assert(dstOpnd != nullptr);
        KillLiveFields(this->lengthEquivBv, bv);
        KillLiveElems(dstOpnd->AsIndirOpnd(), instr->GetSrc1(), bv, inGlobOpt, instr->m_func);
        if (inGlobOpt)
        {
            KillObjectHeaderInlinedTypeSyms(this->currentBlock, false);
        }
        break;

    case Js::OpCode::InitComputedProperty:
    case Js::OpCode::InitGetElemI:
    case Js::OpCode::InitSetElemI:
        KillLiveElems(dstOpnd->AsIndirOpnd(), instr->GetSrc1(), bv, inGlobOpt, instr->m_func);
        if (inGlobOpt)
        {
            KillObjectHeaderInlinedTypeSyms(this->currentBlock, false);
        }
        break;

    case Js::OpCode::DeleteElemI_A:
    case Js::OpCode::DeleteElemIStrict_A:
        Assert(dstOpnd != nullptr);
        KillLiveElems(instr->GetSrc1()->AsIndirOpnd(), nullptr, bv, inGlobOpt, instr->m_func);
        break;

    case Js::OpCode::DeleteFld:
    case Js::OpCode::DeleteRootFld:
    case Js::OpCode::DeleteFldStrict:
    case Js::OpCode::DeleteRootFldStrict:
    case Js::OpCode::ScopedDeleteFld:
    case Js::OpCode::ScopedDeleteFldStrict:
        sym = instr->GetSrc1()->AsSymOpnd()->m_sym;
        KillLiveFields(sym->AsPropertySym(), bv);
        if (inGlobOpt)
        {
            AddToPropertiesWrittenTo(sym->AsPropertySym()->m_propertyId);
            this->KillAllObjectTypes(bv);
        }
        break;

    case Js::OpCode::InitSetFld:
    case Js::OpCode::InitGetFld:
    case Js::OpCode::InitClassMemberGet:
    case Js::OpCode::InitClassMemberSet:
        sym = instr->GetDst()->AsSymOpnd()->m_sym;
        KillLiveFields(sym->AsPropertySym(), bv);
        if (inGlobOpt)
        {
            AddToPropertiesWrittenTo(sym->AsPropertySym()->m_propertyId);
            this->KillAllObjectTypes(bv);
        }
        break;

    case Js::OpCode::ConsoleScopedStFld:
    case Js::OpCode::ConsoleScopedStFldStrict:
    case Js::OpCode::ScopedStFld:
    case Js::OpCode::ScopedStFldStrict:
        // This is already taken care of for FastFld opcodes

        if (inGlobOpt)
        {
            KillObjectHeaderInlinedTypeSyms(this->currentBlock, false);
            if (this->objectTypeSyms)
            {
                if (this->currentBlock->globOptData.maybeWrittenTypeSyms == nullptr)
                {
                    this->currentBlock->globOptData.maybeWrittenTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
                }
                this->currentBlock->globOptData.maybeWrittenTypeSyms->Or(this->objectTypeSyms);
            }
        }

        // fall through

    case Js::OpCode::InitFld:
    case Js::OpCode::InitConstFld:
    case Js::OpCode::InitLetFld:
    case Js::OpCode::InitRootFld:
    case Js::OpCode::InitRootConstFld:
    case Js::OpCode::InitRootLetFld:
#if !FLOATVAR
    case Js::OpCode::StSlotBoxTemp:
#endif
    case Js::OpCode::StFld:
    case Js::OpCode::StRootFld:
    case Js::OpCode::StFldStrict:
    case Js::OpCode::StRootFldStrict:
    case Js::OpCode::StSlot:
    case Js::OpCode::StSlotChkUndecl:
    case Js::OpCode::StSuperFld:
        Assert(dstOpnd != nullptr);
        sym = dstOpnd->AsSymOpnd()->m_sym;
        if (inGlobOpt)
        {
            AddToPropertiesWrittenTo(sym->AsPropertySym()->m_propertyId);
        }
        if ((inGlobOpt && (sym->AsPropertySym()->m_propertyId == Js::PropertyIds::valueOf || sym->AsPropertySym()->m_propertyId == Js::PropertyIds::toString)) ||
            instr->CallsAccessor())
        {
            // If overriding valueof/tostring, we might have expected a previous LdFld to bailout on implicitCalls but didn't.
            // CSE's for example would have expected a bailout. Clear all fields to prevent optimizing across.
            this->KillAllFields(bv);
        }
        else
        {
            KillLiveFields(sym->AsPropertySym(), bv);
        }
        break;

    case Js::OpCode::InlineArrayPush:
    case Js::OpCode::InlineArrayPop:
        if(instr->m_func->GetThisOrParentInlinerHasArguments())
        {
            this->KillAllFields(bv);
            this->SetAnyPropertyMayBeWrittenTo();
        }
        else
        {
            KillLiveFields(this->lengthEquivBv, bv);
            if (inGlobOpt)
            {
                // Deleting an item, or pushing a property to a non-array, may change object layout
                KillAllObjectTypes(bv);
            }
        }
        break;

    case Js::OpCode::InlineeStart:
    case Js::OpCode::InlineeEnd:
        Assert(!instr->UsesAllFields());

        // Kill all live 'arguments' and 'caller' fields, as 'inlineeFunction.arguments' and 'inlineeFunction.caller' 
        // cannot be copy-propped across different instances of the same inlined function.
        KillLiveFields(argumentsEquivBv, bv);
        KillLiveFields(callerEquivBv, bv);
        break;

    case Js::OpCode::CallDirect:
        fnHelper = instr->GetSrc1()->AsHelperCallOpnd()->m_fnHelper;

        switch (fnHelper)
        {
            case IR::JnHelperMethod::HelperArray_Shift:
            case IR::JnHelperMethod::HelperArray_Splice:
            case IR::JnHelperMethod::HelperArray_Unshift:
                // Kill length field for built-ins that can update it.
                if (nullptr != this->lengthEquivBv)
                {
                    // If has arguments, all fields are killed in fall through
                    if (!instr->m_func->GetThisOrParentInlinerHasArguments())
                    {
                        KillLiveFields(this->lengthEquivBv, bv);
                    }
                }
                // fall through

            case IR::JnHelperMethod::HelperArray_Reverse:
                if (instr->m_func->GetThisOrParentInlinerHasArguments())
                {
                    this->KillAllFields(bv);
                    this->SetAnyPropertyMayBeWrittenTo();
                }
                else if (inGlobOpt)
                {
                    // Deleting an item may change object layout
                    KillAllObjectTypes(bv);
                }
                break;

            case IR::JnHelperMethod::HelperArray_Slice:
            case IR::JnHelperMethod::HelperArray_Concat:
                if (inGlobOpt && this->objectTypeSyms)
                {
                    if (this->currentBlock->globOptData.maybeWrittenTypeSyms == nullptr)
                    {
                        this->currentBlock->globOptData.maybeWrittenTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
                    }
                    this->currentBlock->globOptData.maybeWrittenTypeSyms->Or(this->objectTypeSyms);
                }
                break;

            case IR::JnHelperMethod::HelperRegExp_Exec:
            case IR::JnHelperMethod::HelperRegExp_ExecResultNotUsed:
            case IR::JnHelperMethod::HelperRegExp_ExecResultUsed:
            case IR::JnHelperMethod::HelperRegExp_ExecResultUsedAndMayBeTemp:
            case IR::JnHelperMethod::HelperRegExp_MatchResultNotUsed:
            case IR::JnHelperMethod::HelperRegExp_MatchResultUsed:
            case IR::JnHelperMethod::HelperRegExp_MatchResultUsedAndMayBeTemp:
            case IR::JnHelperMethod::HelperRegExp_ReplaceStringResultUsed:
            case IR::JnHelperMethod::HelperRegExp_ReplaceStringResultNotUsed:
            case IR::JnHelperMethod::HelperRegExp_SplitResultNotUsed:
            case IR::JnHelperMethod::HelperRegExp_SplitResultUsed:
            case IR::JnHelperMethod::HelperRegExp_SplitResultUsedAndMayBeTemp:
            case IR::JnHelperMethod::HelperRegExp_SymbolSearch:
            case IR::JnHelperMethod::HelperString_Match:
            case IR::JnHelperMethod::HelperString_Search:
            case IR::JnHelperMethod::HelperString_Split:
            case IR::JnHelperMethod::HelperString_Replace:
                // Consider: We may not need to kill all fields here.
                // We need to kill all the built-in properties that can be written, though, and there are a lot of those.
                this->KillAllFields(bv);
                break;
        }
        break;

    case Js::OpCode::LdHeapArguments:
    case Js::OpCode::LdLetHeapArguments:
    case Js::OpCode::LdHeapArgsCached:
    case Js::OpCode::LdLetHeapArgsCached:
        if (inGlobOpt) {
            this->KillLiveFields(this->slotSyms, bv);
        }
        break;

    case Js::OpCode::InitClass:
    case Js::OpCode::InitProto:
    case Js::OpCode::NewScObjectNoCtor:
    case Js::OpCode::NewScObjectNoCtorFull:
        if (inGlobOpt)
        {
            // Opcodes that make an object into a prototype may break object-header-inlining and final type opt.
            // Kill all known object layouts.
            KillAllObjectTypes(bv);
        }
        break;

    default:
        if (instr->UsesAllFields())
        {
            // This also kills all property type values, as the same bit-vector tracks those stack syms.
            this->KillAllFields(bv);
        }
        break;
    }
}

void
GlobOpt::ProcessFieldKills(IR::Instr * instr)
{
    if (!this->DoFieldCopyProp() && !this->DoFieldRefOpts() && !DoCSE())
    {
        Assert(this->currentBlock->globOptData.liveFields->IsEmpty());
        return;
    }

    ProcessFieldKills(instr, this->currentBlock->globOptData.liveFields, true);
}

Value *
GlobOpt::CreateFieldSrcValue(PropertySym * sym, PropertySym * originalSym, IR::Opnd ** ppOpnd, IR::Instr * instr)
{
#if DBG
    // If the opcode going to kill all field values immediate anyway, we shouldn't be giving it a value
    Assert(!instr->UsesAllFields());

    AssertCanCopyPropOrCSEFieldLoad(instr);

    Assert(instr->GetSrc1() == *ppOpnd);
#endif

    // Only give a value to fields if we are doing field copy prop.
    // Consider: We should always copy prop local slots, but the only use right now is LdSlot from jit loop body.
    // This should have one onus load, and thus no need for copy prop of field itself.  We may want to support
    // copy prop LdSlot if there are other uses of local slots
    if (!this->DoFieldCopyProp())
    {
        return nullptr;
    }

    BOOL wasLive = this->currentBlock->globOptData.liveFields->TestAndSet(sym->m_id);

    if (sym != originalSym)
    {
        this->currentBlock->globOptData.liveFields->TestAndSet(originalSym->m_id);
    }

    if (!wasLive)
    {
        // We don't clear the value when we kill the field.
        // Clear it to make sure we don't use the old value.
        this->currentBlock->globOptData.ClearSymValue(sym);
        this->currentBlock->globOptData.ClearSymValue(originalSym);
    }

    Assert((*ppOpnd)->AsSymOpnd()->m_sym == sym || this->IsLoopPrePass());
    
    // We don't use the sym store to do copy prop on hoisted fields, but create a value
    // in case it can be copy prop out of the loop.
    return this->NewGenericValue(ValueType::Uninitialized, *ppOpnd);
}

bool
GlobOpt::NeedBailOnImplicitCallWithFieldOpts(Loop *loop, bool hasLiveFields) const
{
    if (!(((this->DoFieldRefOpts(loop) ||
            this->DoFieldCopyProp(loop)) &&
           hasLiveFields)))
    {
        return false;
    }

    return true;
}

IR::Instr *
GlobOpt::EnsureDisableImplicitCallRegion(Loop * loop)
{
    Assert(loop->bailOutInfo != nullptr);
    IR::Instr * endDisableImplicitCall = loop->endDisableImplicitCall;
    if (endDisableImplicitCall)
    {
        return endDisableImplicitCall;
    }

    IR::Instr * bailOutTarget = EnsureBailTarget(loop);

    Func * bailOutFunc = loop->GetFunc();
    Assert(loop->bailOutInfo->bailOutFunc == bailOutFunc);

    IR::MemRefOpnd * disableImplicitCallAddress = IR::MemRefOpnd::New(this->func->GetThreadContextInfo()->GetDisableImplicitFlagsAddr(), TyInt8, bailOutFunc);
    IR::IntConstOpnd * disableImplicitCallAndExceptionValue = IR::IntConstOpnd::New(DisableImplicitCallAndExceptionFlag, TyInt8, bailOutFunc, true);
    IR::IntConstOpnd * enableImplicitCallAndExceptionValue = IR::IntConstOpnd::New(DisableImplicitNoFlag, TyInt8, bailOutFunc, true);

    IR::Opnd * implicitCallFlags = Lowerer::GetImplicitCallFlagsOpnd(bailOutFunc);
    IR::IntConstOpnd * noImplicitCall = IR::IntConstOpnd::New(Js::ImplicitCall_None, TyInt8, bailOutFunc, true);

    // Consider: if we are already doing implicit call in the outer loop, we don't need to clear the implicit call bit again
    IR::Instr * clearImplicitCall = IR::Instr::New(Js::OpCode::Ld_A, implicitCallFlags, noImplicitCall, bailOutFunc);
    bailOutTarget->InsertBefore(clearImplicitCall);

    IR::Instr * disableImplicitCall = IR::Instr::New(Js::OpCode::Ld_A, disableImplicitCallAddress, disableImplicitCallAndExceptionValue, bailOutFunc);
    bailOutTarget->InsertBefore(disableImplicitCall);

    endDisableImplicitCall = IR::Instr::New(Js::OpCode::Ld_A, disableImplicitCallAddress, enableImplicitCallAndExceptionValue, bailOutFunc);
    bailOutTarget->InsertBefore(endDisableImplicitCall);

    IR::BailOutInstr * bailOutInstr = IR::BailOutInstr::New(Js::OpCode::BailOnNotEqual, IR::BailOutOnImplicitCalls, loop->bailOutInfo, loop->bailOutInfo->bailOutFunc);
    bailOutInstr->SetSrc1(implicitCallFlags);
    bailOutInstr->SetSrc2(noImplicitCall);
    bailOutTarget->InsertBefore(bailOutInstr);

    loop->endDisableImplicitCall = endDisableImplicitCall;
    return endDisableImplicitCall;
}

#if DBG
bool
GlobOpt::IsPropertySymId(SymID symId) const
{
    return this->func->m_symTable->Find(symId)->IsPropertySym();
}

void
GlobOpt::AssertCanCopyPropOrCSEFieldLoad(IR::Instr * instr)
{
    // Consider: Hoisting LdRootFld may have complication with exception if the field doesn't exist.
    // We need to have another opcode for the hoisted version to avoid the exception and bailout.

    Assert(instr->m_opcode == Js::OpCode::LdSlot || instr->m_opcode == Js::OpCode::LdSlotArr
        || instr->m_opcode == Js::OpCode::LdFld || instr->m_opcode == Js::OpCode::LdFldForCallApplyTarget
        || instr->m_opcode == Js::OpCode::LdLen_A
        || instr->m_opcode == Js::OpCode::LdRootFld  || instr->m_opcode == Js::OpCode::LdSuperFld
        || instr->m_opcode == Js::OpCode::LdFldForTypeOf || instr->m_opcode == Js::OpCode::LdRootFldForTypeOf
        || instr->m_opcode == Js::OpCode::LdMethodFld || instr->m_opcode == Js::OpCode::LdMethodFldPolyInlineMiss
        || instr->m_opcode == Js::OpCode::LdRootMethodFld
        || instr->m_opcode == Js::OpCode::LdMethodFromFlags
        || instr->m_opcode == Js::OpCode::ScopedLdMethodFld
        || instr->m_opcode == Js::OpCode::CheckFixedFld
        || instr->m_opcode == Js::OpCode::CheckPropertyGuardAndLoadType
        || instr->m_opcode == Js::OpCode::ScopedLdFld
        || instr->m_opcode == Js::OpCode::ScopedLdFldForTypeOf);

    Assert(instr->m_opcode == Js::OpCode::CheckFixedFld || instr->GetDst()->GetType() == TyVar || instr->m_func->GetJITFunctionBody()->IsAsmJsMode());
    Assert(instr->GetSrc1()->GetType() == TyVar || instr->m_func->GetJITFunctionBody()->IsAsmJsMode());
    Assert(instr->GetSrc1()->AsSymOpnd()->m_sym->IsPropertySym());
    Assert(instr->GetSrc2() == nullptr);
}
#endif

StackSym *
GlobOpt::EnsureObjectTypeSym(StackSym * objectSym)
{
    Assert(!objectSym->IsTypeSpec());

    objectSym->EnsureObjectInfo(this->func);

    if (objectSym->HasObjectTypeSym())
    {
        Assert(this->objectTypeSyms);
        return objectSym->GetObjectTypeSym();
    }

    if (this->objectTypeSyms == nullptr)
    {
        this->objectTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
    }

    StackSym * typeSym = StackSym::New(TyVar, this->func);

    objectSym->GetObjectInfo()->m_typeSym = typeSym;

    this->objectTypeSyms->Set(typeSym->m_id);

    return typeSym;
}

PropertySym *
GlobOpt::EnsurePropertyWriteGuardSym(PropertySym * propertySym)
{
    // Make sure that the PropertySym has a proto cache sym which is chained into the propertySym list.
    if (!propertySym->m_writeGuardSym)
    {
        propertySym->m_writeGuardSym = PropertySym::New(propertySym->m_stackSym, propertySym->m_propertyId, (uint32)-1, (uint)-1, PropertyKindWriteGuard, this->func);
    }

    return propertySym->m_writeGuardSym;
}

void
GlobOpt::PreparePropertySymForTypeCheckSeq(PropertySym *propertySym)
{
    Assert(!propertySym->m_stackSym->IsTypeSpec());
    EnsureObjectTypeSym(propertySym->m_stackSym);
    EnsurePropertyWriteGuardSym(propertySym);
}

bool
GlobOpt::IsPropertySymPreparedForTypeCheckSeq(PropertySym *propertySym)
{
    Assert(!propertySym->m_stackSym->IsTypeSpec());

    // The following doesn't need to be true. We may copy prop a constant into an object sym, which has
    // previously been prepared for type check sequence optimization.
    // Assert(!propertySym->m_stackSym->m_isIntConst || !propertySym->HasObjectTypeSym());

    // The following doesn't need to be true. We may copy prop the object sym into a field load or store
    // that doesn't have object type spec info and hence the operand wasn't prepared and doesn't have a write
    // guard. The object sym, however, may have other field operations which are object type specialized and
    // thus the type sym for it has been created.
    // Assert(propertySym->HasObjectTypeSym() == propertySym->HasWriteGuardSym());

    return propertySym->HasObjectTypeSym();
}

bool
GlobOpt::PreparePropertySymOpndForTypeCheckSeq(IR::PropertySymOpnd * propertySymOpnd, IR::Instr* instr, Loop * loop)
{
    if (!DoFieldRefOpts(loop) || !OpCodeAttr::FastFldInstr(instr->m_opcode) || instr->CallsAccessor())
    {
        return false;
    }

    if (!propertySymOpnd->HasObjTypeSpecFldInfo())
    {
        return false;
    }

    ObjTypeSpecFldInfo* info = propertySymOpnd->GetObjTypeSpecInfo();

    if (info->UsesAccessor() || info->IsRootObjectNonConfigurableFieldLoad())
    {
        return false;
    }

    if (info->IsPoly() && !info->GetEquivalentTypeSet())
    {
        return false;
    }

    PropertySym * propertySym = propertySymOpnd->m_sym->AsPropertySym();

    PreparePropertySymForTypeCheckSeq(propertySym);
    propertySymOpnd->SetTypeCheckSeqCandidate(true);
    propertySymOpnd->SetIsBeingStored(propertySymOpnd == instr->GetDst());

    return true;
}

bool
GlobOpt::CheckIfPropOpEmitsTypeCheck(IR::Instr *instr, IR::PropertySymOpnd *opnd)
{
    if (!DoFieldRefOpts() || !OpCodeAttr::FastFldInstr(instr->m_opcode))
    {
        return false;
    }

    if (!opnd->IsTypeCheckSeqCandidate())
    {
        return false;
    }

    return CheckIfInstrInTypeCheckSeqEmitsTypeCheck(instr, opnd);
}

IR::PropertySymOpnd *
GlobOpt::CreateOpndForTypeCheckOnly(IR::PropertySymOpnd* opnd, Func* func)
{
    // Used only for CheckObjType instruction today. Future users should make a call
    // whether the new operand is jit optimized in their scenario or not.

    Assert(!opnd->IsRootObjectNonConfigurableFieldLoad());
    IR::PropertySymOpnd *newOpnd = opnd->CopyCommon(func);

    newOpnd->SetObjTypeSpecFldInfo(opnd->GetObjTypeSpecInfo());
    newOpnd->SetUsesAuxSlot(opnd->UsesAuxSlot());
    newOpnd->SetSlotIndex(opnd->GetSlotIndex());

    newOpnd->objTypeSpecFlags = opnd->objTypeSpecFlags;
    // If we're turning the instruction owning this operand into a CheckObjType, we will do a type check here
    // only for the sake of downstream instructions, so the flags pertaining to this property access are
    // irrelevant, because we don't do a property access here.
    newOpnd->SetTypeCheckOnly(true);
    newOpnd->usesFixedValue = false;

    newOpnd->finalType = opnd->finalType;
    newOpnd->guardedPropOps = opnd->guardedPropOps != nullptr ? opnd->guardedPropOps->CopyNew() : nullptr;
    newOpnd->writeGuards = opnd->writeGuards != nullptr ? opnd->writeGuards->CopyNew() : nullptr;

    newOpnd->SetIsJITOptimizedReg(true);

    return newOpnd;
}

bool
GlobOpt::FinishOptPropOp(IR::Instr *instr, IR::PropertySymOpnd *opnd, BasicBlock* block, bool updateExistingValue, bool* emitsTypeCheckOut, bool* changesTypeValueOut)
{
    if (!DoFieldRefOpts() || !OpCodeAttr::FastFldInstr(instr->m_opcode))
    {
        return false;
    }

    bool isTypeCheckSeqCandidate = opnd->IsTypeCheckSeqCandidate();
    bool isObjTypeSpecialized = false;
    bool isObjTypeChecked = false;

    if (isTypeCheckSeqCandidate)
    {
        isObjTypeSpecialized = ProcessPropOpInTypeCheckSeq<true>(instr, opnd, block, updateExistingValue, emitsTypeCheckOut, changesTypeValueOut, &isObjTypeChecked);
    }

    if (opnd == instr->GetDst() && this->objectTypeSyms)
    {
        if (block == nullptr)
        {
            block = this->currentBlock;
        }

        // This is a property store that may change the layout of the object that it stores to. This means that
        // it may change any aliased object. Do two things to address this:
        // - Add all object types in this function to the set that may have had a property added. This will prevent
        //   final type optimization across this instruction. (Only needed here for non-specialized stores.)
        // - Kill all type symbols that currently hold object-header-inlined types. Any of them may have their layout
        //   changed by the addition of a property.

        SymID opndId = opnd->HasObjectTypeSym() ? opnd->GetObjectTypeSym()->m_id : -1;

        if (!isObjTypeChecked)
        {
            if (block->globOptData.maybeWrittenTypeSyms == nullptr)
            {
                block->globOptData.maybeWrittenTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
            }
            if (isObjTypeSpecialized)
            {
                // The current object will be protected by a type check, unless no further accesses to it are
                // protected by this access.
                Assert(this->objectTypeSyms->Test(opndId));
                this->objectTypeSyms->Clear(opndId);
            }
            block->globOptData.maybeWrittenTypeSyms->Or(this->objectTypeSyms);
            if (isObjTypeSpecialized)
            {
                this->objectTypeSyms->Set(opndId);
            }
        }

        if (!isObjTypeSpecialized || opnd->ChangesObjectLayout())
        {
            this->KillObjectHeaderInlinedTypeSyms(block, isObjTypeSpecialized, opndId);
        }
        else if (!isObjTypeChecked && this->HasLiveObjectHeaderInlinedTypeSym(block, true, opndId))
        {
            opnd->SetTypeCheckRequired(true);
        }
    }

    return isObjTypeSpecialized;
}

void
GlobOpt::KillObjectHeaderInlinedTypeSyms(BasicBlock *block, bool isObjTypeSpecialized, SymID opndId)
{
    this->MapObjectHeaderInlinedTypeSymsUntil(block, isObjTypeSpecialized, opndId, [&](SymID symId)->bool  { this->currentBlock->globOptData.liveFields->Clear(symId); return false; });
}

bool
GlobOpt::HasLiveObjectHeaderInlinedTypeSym(BasicBlock *block, bool isObjTypeSpecialized, SymID opndId)
{
    return this->MapObjectHeaderInlinedTypeSymsUntil(block, true, opndId, [&](SymID symId)->bool { return this->currentBlock->globOptData.liveFields->Test(symId); });
}

template<class Fn>
bool
GlobOpt::MapObjectHeaderInlinedTypeSymsUntil(BasicBlock *block, bool isObjTypeSpecialized, SymID opndId, Fn fn)
{
    if (this->objectTypeSyms == nullptr)
    {
        return false;
    }

    FOREACH_BITSET_IN_SPARSEBV(symId, this->objectTypeSyms)
    {
        if (symId == opndId && isObjTypeSpecialized)
        {
            // The current object will be protected by a type check, unless no further accesses to it are
            // protected by this access.
            continue;
        }
        Value *value = block->globOptData.FindObjectTypeValue(symId);
        if (value)
        {
            JsTypeValueInfo *valueInfo = value->GetValueInfo()->AsJsType();
            Assert(valueInfo);
            if (valueInfo->GetJsType() != nullptr)
            {
                JITTypeHolder type(valueInfo->GetJsType());
                if (Js::DynamicType::Is(type->GetTypeId()))
                {
                    if (type->GetTypeHandler()->IsObjectHeaderInlinedTypeHandler())
                    {
                        if (fn(symId))
                        {
                            return true;
                        }
                    }
                }
            }
            else if (valueInfo->GetJsTypeSet())
            {
                Js::EquivalentTypeSet *typeSet = valueInfo->GetJsTypeSet();
                for (uint16 i = 0; i < typeSet->GetCount(); i++)
                {
                    JITTypeHolder type = typeSet->GetType(i);
                    if (type != nullptr && Js::DynamicType::Is(type->GetTypeId()))
                    {
                        if (type->GetTypeHandler()->IsObjectHeaderInlinedTypeHandler())
                        {
                            if (fn(symId))
                            {
                                return true;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
    NEXT_BITSET_IN_SPARSEBV;

    return false;
}

bool
GlobOpt::AreTypeSetsIdentical(Js::EquivalentTypeSet * leftTypeSet, Js::EquivalentTypeSet * rightTypeSet)
{
    return Js::EquivalentTypeSet::AreIdentical(leftTypeSet, rightTypeSet);
}

bool
GlobOpt::IsSubsetOf(Js::EquivalentTypeSet * leftTypeSet, Js::EquivalentTypeSet * rightTypeSet)
{
    return Js::EquivalentTypeSet::IsSubsetOf(leftTypeSet, rightTypeSet);
}

bool
GlobOpt::CompareCurrentTypesWithExpectedTypes(JsTypeValueInfo *valueInfo, IR::PropertySymOpnd * propertySymOpnd)
{
    bool isTypeDead = propertySymOpnd->IsTypeDead();

    if (valueInfo == nullptr || (valueInfo->GetJsType() == nullptr && valueInfo->GetJsTypeSet() == nullptr))
    {
        // No upstream types. Do a type check.
        return !isTypeDead;
    }

    if (!propertySymOpnd->HasEquivalentTypeSet() || propertySymOpnd->NeedsMonoCheck())
    {
        JITTypeHolder opndType = propertySymOpnd->GetType();

        if (valueInfo->GetJsType() != nullptr)
        {
            if (valueInfo->GetJsType() == propertySymOpnd->GetType())
            {
                return true;
            }
            if (propertySymOpnd->HasInitialType() && valueInfo->GetJsType() == propertySymOpnd->GetInitialType())
            {
                return !isTypeDead;
            }
            return false;
        }
        else
        {
            Assert(valueInfo->GetJsTypeSet());
            Js::EquivalentTypeSet *valueTypeSet = valueInfo->GetJsTypeSet();

            if (valueTypeSet->Contains(opndType))
            {
                return !isTypeDead;
            }
            if (propertySymOpnd->HasInitialType() && valueTypeSet->Contains(propertySymOpnd->GetInitialType()))
            {
                return !isTypeDead;
            }
            return false;
        }
    }
    else
    {
        Js::EquivalentTypeSet * opndTypeSet = propertySymOpnd->GetEquivalentTypeSet();

        if (valueInfo->GetJsType() != nullptr)
        {
            uint16 checkedTypeSetIndex;
            if (opndTypeSet->Contains(valueInfo->GetJsType(), &checkedTypeSetIndex))
            {
                return true;
            }
            return false;
        }
        else
        {
            if (IsSubsetOf(valueInfo->GetJsTypeSet(), opndTypeSet))
            {
                return true;
            }
            if (propertySymOpnd->IsMono() ?
                    valueInfo->GetJsTypeSet()->Contains(propertySymOpnd->GetFirstEquivalentType()) :
                    IsSubsetOf(opndTypeSet, valueInfo->GetJsTypeSet()))
            {
                return true;
            }
            return false;
        }
    }
}

bool
GlobOpt::ProcessPropOpInTypeCheckSeq(IR::Instr* instr, IR::PropertySymOpnd *opnd)
{
    return ProcessPropOpInTypeCheckSeq<true>(instr, opnd, this->currentBlock, false);
}

bool GlobOpt::CheckIfInstrInTypeCheckSeqEmitsTypeCheck(IR::Instr* instr, IR::PropertySymOpnd *opnd)
{
    bool emitsTypeCheck;
    ProcessPropOpInTypeCheckSeq<false>(instr, opnd, this->currentBlock, false, &emitsTypeCheck);
    return emitsTypeCheck;
}

template<bool makeChanges>
bool
GlobOpt::ProcessPropOpInTypeCheckSeq(IR::Instr* instr, IR::PropertySymOpnd *opnd, BasicBlock* block, bool updateExistingValue, bool* emitsTypeCheckOut, bool* changesTypeValueOut, bool *isTypeCheckedOut)
{
    // We no longer mark types as dead in the backward pass, so we should never see an instr with a dead type here
    // during the forward pass. For the time being we've retained the logic below to deal with dead types in case
    // we ever wanted to revert back to more aggressive type killing that we had before.
    Assert(!opnd->IsTypeDead());

    Assert(opnd->IsTypeCheckSeqCandidate());
    Assert(opnd->HasObjectTypeSym());

    bool isStore = opnd == instr->GetDst();
    bool isTypeDead = opnd->IsTypeDead();
    bool consumeType = makeChanges && !IsLoopPrePass();
    bool produceType = makeChanges && !isTypeDead;
    bool isSpecialized = false;
    bool emitsTypeCheck = false;
    bool addsProperty = false;

    if (block == nullptr)
    {
        block = this->currentBlock;
    }

    StackSym * typeSym = opnd->GetObjectTypeSym();

#if DBG
    uint16 typeCheckSeqFlagsBefore;
    Value* valueBefore = nullptr;
    JsTypeValueInfo* valueInfoBefore = nullptr;
    if (!makeChanges)
    {
        typeCheckSeqFlagsBefore = opnd->GetTypeCheckSeqFlags();
        valueBefore = block->globOptData.FindObjectTypeValue(typeSym);
        if (valueBefore != nullptr)
        {
            Assert(valueBefore->GetValueInfo() != nullptr && valueBefore->GetValueInfo()->IsJsType());
            valueInfoBefore = valueBefore->GetValueInfo()->AsJsType();
        }
    }
#endif

    Value *value = block->globOptData.FindObjectTypeValue(typeSym);
    JsTypeValueInfo* valueInfo = value != nullptr ? value->GetValueInfo()->AsJsType() : nullptr;

    if (consumeType && valueInfo != nullptr)
    {
        opnd->SetTypeAvailable(true);
    }

    bool doEquivTypeCheck = opnd->HasEquivalentTypeSet() && !opnd->NeedsMonoCheck();
    if (!doEquivTypeCheck)
    {
        AssertOrFailFast(!opnd->NeedsDepolymorphication());

        // We need a monomorphic type check here (e.g., final type opt, fixed field check on non-proto property).
        JITTypeHolder opndType = opnd->GetType();

        if (valueInfo == nullptr || (valueInfo->GetJsType() == nullptr && valueInfo->GetJsTypeSet() == nullptr))
        {
            // This is the initial type check.
            opnd->SetTypeAvailable(false);
            isSpecialized = !isTypeDead;
            emitsTypeCheck = isSpecialized;
            addsProperty = isStore && isSpecialized && opnd->HasInitialType();
            if (produceType)
            {
                SetObjectTypeFromTypeSym(typeSym, opndType, nullptr, block, updateExistingValue);
            }
        }
        else if (valueInfo->GetJsType() != nullptr)
        {
            // We have a monomorphic type check upstream. Check against initial/final type.
            const JITTypeHolder valueType(valueInfo->GetJsType());
            if (valueType == opndType)
            {
                // The type on this instruction matches the live value in the value table, so there is no need to
                // refresh the value table.
                isSpecialized = true;
                if (isTypeCheckedOut)
                {
                    *isTypeCheckedOut = true;
                }
                if (consumeType)
                {
                    opnd->SetTypeChecked(true);
                }
            }
            else if (opnd->HasInitialType() && valueType == opnd->GetInitialType())
            {
                // Checked type matches the initial type at this store.
                bool objectMayHaveAcquiredAdditionalProperties =
                    block->globOptData.maybeWrittenTypeSyms &&
                    block->globOptData.maybeWrittenTypeSyms->Test(typeSym->m_id);
                if (consumeType)
                {
                    opnd->SetTypeChecked(!objectMayHaveAcquiredAdditionalProperties);
                    opnd->SetInitialTypeChecked(!objectMayHaveAcquiredAdditionalProperties);
                }
                if (produceType)
                {
                    SetObjectTypeFromTypeSym(typeSym, opndType, nullptr, block, updateExistingValue);
                }
                isSpecialized = !isTypeDead || !objectMayHaveAcquiredAdditionalProperties;
                emitsTypeCheck = isSpecialized && objectMayHaveAcquiredAdditionalProperties;
                addsProperty = isSpecialized;
                if (isTypeCheckedOut)
                {
                    *isTypeCheckedOut = !objectMayHaveAcquiredAdditionalProperties;
                }
            }
            else
            {
                // This must be a type mismatch situation, because the value is available, but doesn't match either
                // the current type or the initial type. We will not optimize this instruction and we do not produce
                // a new type value here.
                isSpecialized = false;

                if (consumeType)
                {
                    opnd->SetTypeMismatch(true);
                }
            }
        }
        else
        {
            // We have an equivalent type check upstream, but we require a particular type at this point. We
            // can't treat it as "checked", but we may benefit from checking for the required type.
            Assert(valueInfo->GetJsTypeSet());
            Js::EquivalentTypeSet *valueTypeSet = valueInfo->GetJsTypeSet();
            if (valueTypeSet->Contains(opndType))
            {
                // Required type is in the type set we've checked. Check for the required type here, and
                // note in the value info that we've narrowed down to this type. (But leave the type set in the
                // value info so it can be merged with the same type set on other paths.)
                isSpecialized = !isTypeDead;
                emitsTypeCheck = isSpecialized;
                if (produceType)
                {
                    SetSingleTypeOnObjectTypeValue(value, opndType);
                }
            }
            else if (opnd->HasInitialType() && valueTypeSet->Contains(opnd->GetInitialType()))
            {
                // Required initial type is in the type set we've checked. Check for the initial type here, and
                // note in the value info that we've narrowed down to this type. (But leave the type set in the
                // value info so it can be merged with the same type set on other paths.)
                isSpecialized = !isTypeDead;
                emitsTypeCheck = isSpecialized;
                addsProperty = isSpecialized;
                if (produceType)
                {
                    SetSingleTypeOnObjectTypeValue(value, opndType);
                }
            }
            else
            {
                // This must be a type mismatch situation, because the value is available, but doesn't match either
                // the current type or the initial type. We will not optimize this instruction and we do not produce
                // a new type value here.
                isSpecialized = false;

                if (consumeType)
                {
                    opnd->SetTypeMismatch(true);
                }
            }
        }
    }
    else
    {
        Assert(!opnd->NeedsMonoCheck());

        Js::EquivalentTypeSet * opndTypeSet = opnd->GetEquivalentTypeSet();
        uint16 checkedTypeSetIndex = (uint16)-1;

        if (opnd->NeedsDepolymorphication())
        {
            // The opnd's type set (opndTypeSet) is non-equivalent. Test all the types coming from the valueInfo.
            // If all of them are contained in opndTypeSet, and all of them have the same slot index in opnd's
            // objtypespecfldinfo, then we can use that slot index and treat the set as equivalent.
            // (Also test whether all types do/don't use aux slots.)

            uint16 slotIndex = Js::Constants::NoSlot;
            bool auxSlot = false;

            // Do this work only if there is an upstream type value. We don't attempt to do a type check based on
            // a non-equivalent set.
            if (valueInfo != nullptr)
            {
                if (valueInfo->GetJsType() != nullptr)
                {
                    opnd->TryDepolymorphication(valueInfo->GetJsType(), Js::Constants::NoSlot, false, &slotIndex, &auxSlot, &checkedTypeSetIndex);
                }
                else if (valueInfo->GetJsTypeSet() != nullptr)
                {
                    Js::EquivalentTypeSet *typeSet = valueInfo->GetJsTypeSet();
                    for (uint16 i = 0; i < typeSet->GetCount(); i++)
                    {
                        opnd->TryDepolymorphication(typeSet->GetType(i), slotIndex, auxSlot, &slotIndex, &auxSlot);
                        if (slotIndex == Js::Constants::NoSlot)
                        {
                            // Indicates failure/mismatch. We're done.
                            break;
                        }
                    }
                }
            }

            if (slotIndex == Js::Constants::NoSlot)
            {
                // Indicates failure/mismatch
                isSpecialized = false;
                if (consumeType)
                {
                    opnd->SetTypeMismatch(true);
                }
            }
            else
            {
                // Indicates we can optimize, as all upstream types are equivalent here.

                opnd->SetSlotIndex(slotIndex);
                opnd->SetUsesAuxSlot(auxSlot);

                opnd->GetObjTypeSpecInfo()->SetSlotIndex(slotIndex);
                opnd->GetObjTypeSpecInfo()->SetUsesAuxSlot(auxSlot);

                isSpecialized = true;
                if (isTypeCheckedOut)
                {
                    *isTypeCheckedOut = true;
                }
                if (consumeType)
                {
                    opnd->SetTypeChecked(true);
                }
                if (checkedTypeSetIndex != (uint16)-1)
                {
                    opnd->SetCheckedTypeSetIndex(checkedTypeSetIndex);
                }
            }
        }
        else if (valueInfo == nullptr || (valueInfo->GetJsType() == nullptr && valueInfo->GetJsTypeSet() == nullptr))
        {
            // If we don't have a value for the type we will have to emit a type check and we produce a new type value here.
            if (produceType)
            {
                if (opnd->IsMono())
                {
                    SetObjectTypeFromTypeSym(typeSym, opnd->GetFirstEquivalentType(), nullptr, block, updateExistingValue);
                }
                else
                {
                    SetObjectTypeFromTypeSym(typeSym, nullptr, opndTypeSet, block, updateExistingValue);
                }
            }
            isSpecialized = !isTypeDead;
            emitsTypeCheck = isSpecialized;
        }
        else if (valueInfo->GetJsType() != nullptr ?
                 opndTypeSet->Contains(valueInfo->GetJsType(), &checkedTypeSetIndex) :
                 IsSubsetOf(valueInfo->GetJsTypeSet(), opndTypeSet))
        {
            // All the types in the value info are contained in the set required by this access,
            // meaning that they're equivalent to the opnd's type set.
            // We won't have a type check, and we don't need to touch the type value.
            isSpecialized = true;
            if (isTypeCheckedOut)
            {
                *isTypeCheckedOut = true;
            }
            if (consumeType)
            {
                opnd->SetTypeChecked(true);
            }
            if (checkedTypeSetIndex != (uint16)-1)
            {
                opnd->SetCheckedTypeSetIndex(checkedTypeSetIndex);
            }
        }
        else if (valueInfo->GetJsTypeSet() &&
                 (opnd->IsMono() ? 
                      valueInfo->GetJsTypeSet()->Contains(opnd->GetFirstEquivalentType()) : 
                      IsSubsetOf(opndTypeSet, valueInfo->GetJsTypeSet())
                 )
            )
        {
            // We have an equivalent type check upstream, but we require a tighter type check at this point.
            // We can't treat the operand as "checked", but check for equivalence with the tighter set and update the
            // value info.
            if (produceType)
            {
                if (opnd->IsMono())
                {
                    SetObjectTypeFromTypeSym(typeSym, opnd->GetFirstEquivalentType(), nullptr, block, updateExistingValue);
                }
                else
                {
                    SetObjectTypeFromTypeSym(typeSym, nullptr, opndTypeSet, block, updateExistingValue);
                }
            }
            isSpecialized = !isTypeDead;
            emitsTypeCheck = isSpecialized;
        }
        else
        {
            // This must be a type mismatch situation, because the value is available, but doesn't match either
            // the current type or the initial type. We will not optimize this instruction and we do not produce
            // a new type value here.
            isSpecialized = false;

            if (consumeType)
            {
                opnd->SetTypeMismatch(true);
            }
        }
    }

    Assert(isSpecialized || (!emitsTypeCheck && !addsProperty));

    if (consumeType && opnd->MayNeedWriteGuardProtection())
    {
        Assert(!isStore);
        PropertySym *propertySym = opnd->m_sym->AsPropertySym();
        Assert(propertySym->m_writeGuardSym);
        opnd->SetWriteGuardChecked(!!block->globOptData.liveFields->Test(propertySym->m_writeGuardSym->m_id));
    }

    // Even specialized property adds must kill all types for other property adds. That's because any other object sym
    // may, in fact, be an alias of the instance whose type is being modified here. (see Windows Blue Bug 541876)
    if (makeChanges && addsProperty)
    {
        Assert(isStore && isSpecialized);
        Assert(this->objectTypeSyms != nullptr);
        Assert(this->objectTypeSyms->Test(typeSym->m_id));

        if (block->globOptData.maybeWrittenTypeSyms == nullptr)
        {
            block->globOptData.maybeWrittenTypeSyms = JitAnew(this->alloc, BVSparse<JitArenaAllocator>, this->alloc);
        }

        this->objectTypeSyms->Clear(typeSym->m_id);
        block->globOptData.maybeWrittenTypeSyms->Or(this->objectTypeSyms);
        this->objectTypeSyms->Set(typeSym->m_id);
    }

    if (produceType && emitsTypeCheck && opnd->IsMono())
    {
        // Consider (ObjTypeSpec): Represent maybeWrittenTypeSyms as a flag on value info of the type sym.
        if (block->globOptData.maybeWrittenTypeSyms != nullptr)
        {
            // We're doing a type check here, so objtypespec of property adds is safe for this type
            // from this point forward.
            block->globOptData.maybeWrittenTypeSyms->Clear(typeSym->m_id);
        }
    }

    // Consider (ObjTypeSpec): Enable setting write guards live on instructions hoisted out of loops. Note that produceType
    // is false if the type values on loop back edges don't match (see earlier comments).
    // This means that hoisted instructions won't set write guards live if the type changes in the loop, even if
    // the corresponding properties have not been written inside the loop. This may result in some unnecessary type
    // checks and bailouts inside the loop. To enable this, we would need to verify the write guards are still live
    // on the back edge (much like we're doing for types above).

    // Consider (ObjTypeSpec): Support polymorphic write guards as well. We can't currently distinguish between mono and
    // poly write guards, and a type check can only protect operations matching with respect to polymorphism (see
    // BackwardPass::TrackObjTypeSpecProperties for details), so for now we only target monomorphic operations.
    if (produceType && emitsTypeCheck && opnd->IsMono())
    {
        // If the type check we'll emit here protects some property operations that require a write guard (i.e.
        // they must do an extra type check and property guard check, if they have been written to in this
        // function), let's mark the write guards as live here, so we can accurately track if their properties
        // have been written to. Make sure we only set those that we'll actually guard, i.e. those that match
        // with respect to polymorphism.
        if (opnd->GetWriteGuards() != nullptr)
        {
            block->globOptData.liveFields->Or(opnd->GetWriteGuards());
        }
    }

    if (makeChanges && isTypeDead)
    {
        this->KillObjectType(opnd->GetObjectSym(), block->globOptData.liveFields);
    }

#if DBG
    if (!makeChanges)
    {
        uint16 typeCheckSeqFlagsAfter = opnd->GetTypeCheckSeqFlags();
        Assert(typeCheckSeqFlagsBefore == typeCheckSeqFlagsAfter);

        Value* valueAfter = block->globOptData.FindObjectTypeValue(typeSym);
        Assert(valueBefore == valueAfter);
        if (valueAfter != nullptr)
        {
            Assert(valueBefore != nullptr);
            Assert(valueAfter->GetValueInfo() != nullptr && valueAfter->GetValueInfo()->IsJsType());
            JsTypeValueInfo* valueInfoAfter = valueAfter->GetValueInfo()->AsJsType();
            Assert(valueInfoBefore == valueInfoAfter);
            Assert(valueInfoBefore->GetJsType() == valueInfoAfter->GetJsType());
            Assert(valueInfoBefore->GetJsTypeSet() == valueInfoAfter->GetJsTypeSet());
        }
    }
#endif

    if (emitsTypeCheckOut != nullptr)
    {
        *emitsTypeCheckOut = emitsTypeCheck;
    }

    if (changesTypeValueOut != nullptr)
    {
        *changesTypeValueOut = isSpecialized && (emitsTypeCheck || addsProperty);
    }

    return isSpecialized;
}

void
GlobOpt::OptNewScObject(IR::Instr** instrPtr, Value* srcVal)
{
    IR::Instr *&instr = *instrPtr;

    if (!instr->IsNewScObjectInstr() || IsLoopPrePass() || !this->DoFieldRefOpts() || PHASE_OFF(Js::ObjTypeSpecNewObjPhase, this->func))
    {
        return;
    }

    bool isCtorInlined = instr->m_opcode == Js::OpCode::NewScObjectNoCtor;
    const JITTimeConstructorCache * ctorCache = instr->IsProfiledInstr() ?
        instr->m_func->GetConstructorCache(static_cast<Js::ProfileId>(instr->AsProfiledInstr()->u.profileId)) : nullptr;

    // TODO: OOP JIT, enable assert
    //Assert(ctorCache == nullptr || srcVal->GetValueInfo()->IsVarConstant() && Js::JavascriptFunction::Is(srcVal->GetValueInfo()->AsVarConstant()->VarValue()));
    Assert(ctorCache == nullptr || !ctorCache->IsTypeFinal() || ctorCache->CtorHasNoExplicitReturnValue());

    if (ctorCache != nullptr && !ctorCache->SkipNewScObject() && (isCtorInlined || ctorCache->IsTypeFinal()))
    {
        GenerateBailAtOperation(instrPtr, IR::BailOutFailedCtorGuardCheck);
    }
}

void
GlobOpt::ValueNumberObjectType(IR::Opnd *dstOpnd, IR::Instr *instr)
{
    if (!dstOpnd->IsRegOpnd())
    {
        return;
    }

    if (dstOpnd->AsRegOpnd()->m_sym->IsTypeSpec())
    {
        return;
    }

    if (instr->IsNewScObjectInstr())
    {
        // If we have a NewScObj* for which we have a valid constructor cache we know what type the created object will have.
        // Let's produce the type value accordingly so we don't insert a type check and bailout in the constructor and
        // potentially further downstream.
        Assert(!PHASE_OFF(Js::ObjTypeSpecNewObjPhase, this->func) || !instr->HasBailOutInfo());

        if (instr->HasBailOutInfo())
        {
            Assert(instr->IsProfiledInstr());
            Assert(instr->GetBailOutKind() == IR::BailOutFailedCtorGuardCheck);

            bool isCtorInlined = instr->m_opcode == Js::OpCode::NewScObjectNoCtor;
            JITTimeConstructorCache * ctorCache = instr->m_func->GetConstructorCache(static_cast<Js::ProfileId>(instr->AsProfiledInstr()->u.profileId));
            Assert(ctorCache != nullptr && (isCtorInlined || ctorCache->IsTypeFinal()));

            StackSym* objSym = dstOpnd->AsRegOpnd()->m_sym;
            StackSym* dstTypeSym = EnsureObjectTypeSym(objSym);
            Assert(this->currentBlock->globOptData.FindValue(dstTypeSym) == nullptr);

            SetObjectTypeFromTypeSym(dstTypeSym, ctorCache->GetType(), nullptr);
        }
    }
    else
    {
        // If the dst opnd is a reg that has a type sym associated with it, then we are either killing
        // the type's existing value or (in the case of a reg copy) assigning it the value of
        // the src's type sym (if any). If the dst doesn't have a type sym, but the src does, let's
        // give dst a new type sym and transfer the value.
        Value *newValue = nullptr;
        IR::Opnd * srcOpnd = instr->GetSrc1();

        if (instr->m_opcode == Js::OpCode::Ld_A && srcOpnd->IsRegOpnd() &&
            !srcOpnd->AsRegOpnd()->m_sym->IsTypeSpec() && srcOpnd->AsRegOpnd()->m_sym->HasObjectTypeSym())
        {
            StackSym *srcTypeSym = srcOpnd->AsRegOpnd()->m_sym->GetObjectTypeSym();
            newValue = this->currentBlock->globOptData.FindValue(srcTypeSym);
        }

        if (newValue == nullptr)
        {
            if (dstOpnd->AsRegOpnd()->m_sym->HasObjectTypeSym())
            {
                StackSym * typeSym = dstOpnd->AsRegOpnd()->m_sym->GetObjectTypeSym();
                this->currentBlock->globOptData.ClearSymValue(typeSym);
            }
        }
        else
        {
            Assert(newValue->GetValueInfo()->IsJsType());
            StackSym * typeSym;
            if (!dstOpnd->AsRegOpnd()->m_sym->HasObjectTypeSym())
            {
                typeSym = nullptr;
            }
            typeSym = EnsureObjectTypeSym(dstOpnd->AsRegOpnd()->m_sym);
            this->currentBlock->globOptData.SetValue(newValue, typeSym);
        }
    }
}

IR::Instr *
GlobOpt::SetTypeCheckBailOut(IR::Opnd *opnd, IR::Instr *instr, BailOutInfo *bailOutInfo)
{
    if (this->IsLoopPrePass() || !opnd->IsSymOpnd())
    {
        return instr;
    }

    if (!opnd->AsSymOpnd()->IsPropertySymOpnd())
    {
        return instr;
    }

    IR::PropertySymOpnd * propertySymOpnd = opnd->AsPropertySymOpnd();

    AssertMsg(propertySymOpnd->TypeCheckSeqBitsSetOnlyIfCandidate(), "Property sym operand optimized despite not being a candidate?");
    AssertMsg(bailOutInfo == nullptr || !instr->HasBailOutInfo(), "Why are we adding new bailout info to an instruction that already has it?");

    auto HandleBailout = [&](IR::BailOutKind bailOutKind)->void {
        // At this point, we have a cached type that is live downstream or the type check is required
        // for a fixed field load. If we can't do away with the type check, then we're going to need bailout,
        // so lets add bailout info if we don't already have it.
        if (!instr->HasBailOutInfo())
        {
            if (bailOutInfo)
            {
                instr = instr->ConvertToBailOutInstr(bailOutInfo, bailOutKind);
            }
            else
            {
                GenerateBailAtOperation(&instr, bailOutKind);
                BailOutInfo *bailOutInfo = instr->GetBailOutInfo();

                // Consider (ObjTypeSpec): If we're checking a fixed field here the bailout could be due to polymorphism or
                // due to a fixed field turning non-fixed. Consider distinguishing between the two.
                bailOutInfo->polymorphicCacheIndex = propertySymOpnd->m_inlineCacheIndex;
            }
        }
        else if (instr->GetBailOutKind() == IR::BailOutMarkTempObject)
        {
            Assert(!bailOutInfo);
            Assert(instr->GetBailOutInfo()->polymorphicCacheIndex == -1);
            instr->SetBailOutKind(bailOutKind | IR::BailOutMarkTempObject);
            instr->GetBailOutInfo()->polymorphicCacheIndex = propertySymOpnd->m_inlineCacheIndex;
        }
        else
        {
            Assert(bailOutKind == instr->GetBailOutKind());
        }
    };

    bool isTypeCheckProtected;
    IR::BailOutKind bailOutKind;
    if (GlobOpt::NeedsTypeCheckBailOut(instr, propertySymOpnd, opnd == instr->GetDst(), &isTypeCheckProtected, &bailOutKind))
    {
        HandleBailout(bailOutKind);
    }
    else
    {
        if (instr->m_opcode == Js::OpCode::LdMethodFromFlags)
        {
            // If LdMethodFromFlags is hoisted to the top of the loop, we should share the same bailout Info.
            // We don't need to do anything for LdMethodFromFlags that cannot be field hoisted.
            HandleBailout(IR::BailOutFailedInlineTypeCheck);
        }
        else if (instr->HasBailOutInfo())
        {
            // If we already have a bailout info, but don't actually need it, let's remove it. This can happen if
            // a CheckFixedFld added by the inliner (with bailout info) determined that the object's type has
            // been checked upstream and no bailout is necessary here.
            if (instr->m_opcode == Js::OpCode::CheckFixedFld)
            {
                AssertMsg(!PHASE_OFF(Js::FixedMethodsPhase, instr->m_func) ||
                    !PHASE_OFF(Js::UseFixedDataPropsPhase, instr->m_func), "CheckFixedFld with fixed method/data phase disabled?");
                Assert(isTypeCheckProtected);
                AssertMsg(instr->GetBailOutKind() == IR::BailOutFailedFixedFieldTypeCheck || instr->GetBailOutKind() == IR::BailOutFailedEquivalentFixedFieldTypeCheck,
                    "Only BailOutFailed[Equivalent]FixedFieldTypeCheck can be safely removed.  Why does CheckFixedFld carry a different bailout kind?.");
                instr->ClearBailOutInfo();
            }
            else if (propertySymOpnd->MayNeedTypeCheckProtection() && propertySymOpnd->IsTypeCheckProtected())
            {
                // Both the type and (if necessary) the proto object have been checked.
                // We're doing a direct slot access. No possibility of bailout here (not even implicit call).
                Assert(instr->GetBailOutKind() == IR::BailOutMarkTempObject);
                instr->ClearBailOutInfo();
            }
        }
    }

    return instr;
}

void
GlobOpt::SetSingleTypeOnObjectTypeValue(Value* value, const JITTypeHolder type)
{
    UpdateObjectTypeValue(value, type, true, nullptr, false);
}

void
GlobOpt::SetTypeSetOnObjectTypeValue(Value* value, Js::EquivalentTypeSet* typeSet)
{
    UpdateObjectTypeValue(value, nullptr, false, typeSet, true);
}

void
GlobOpt::UpdateObjectTypeValue(Value* value, const JITTypeHolder type, bool setType, Js::EquivalentTypeSet* typeSet, bool setTypeSet)
{
    Assert(value->GetValueInfo() != nullptr && value->GetValueInfo()->IsJsType());
    JsTypeValueInfo* valueInfo = value->GetValueInfo()->AsJsType();

    if (valueInfo->GetIsShared())
    {
        valueInfo = valueInfo->Copy(this->alloc);
        value->SetValueInfo(valueInfo);
    }

    if (setType)
    {
        valueInfo->SetJsType(type);
    }
    if (setTypeSet)
    {
        valueInfo->SetJsTypeSet(typeSet);
    }
}

void
GlobOpt::SetObjectTypeFromTypeSym(StackSym *typeSym, Value* value, BasicBlock* block)
{
    Assert(typeSym != nullptr);
    Assert(value != nullptr);
    Assert(value->GetValueInfo() != nullptr && value->GetValueInfo()->IsJsType());

    SymID typeSymId = typeSym->m_id;

    if (block == nullptr)
    {
        block = this->currentBlock;
    }

    block->globOptData.SetValue(value, typeSym);
    block->globOptData.liveFields->Set(typeSymId);
}

void
GlobOpt::SetObjectTypeFromTypeSym(StackSym *typeSym, const JITTypeHolder type, Js::EquivalentTypeSet * typeSet, BasicBlock* block, bool updateExistingValue)
{
    if (block == nullptr)
    {
        block = this->currentBlock;
    }

    SetObjectTypeFromTypeSym(typeSym, type, typeSet, &block->globOptData, updateExistingValue);
}

void
GlobOpt::SetObjectTypeFromTypeSym(StackSym *typeSym, const JITTypeHolder type, Js::EquivalentTypeSet * typeSet, GlobOptBlockData *blockData, bool updateExistingValue)
{
    Assert(typeSym != nullptr);

    SymID typeSymId = typeSym->m_id;

    if (blockData == nullptr)
    {
        blockData = &this->currentBlock->globOptData;
    }

    if (updateExistingValue)
    {
        Value* value = blockData->FindValueFromMapDirect(typeSymId);

        // If we're trying to update an existing value, the value better exist. We only do this when updating a generic
        // value created during loop pre-pass for field hoisting, so we expect the value info to still be blank.
        Assert(value != nullptr && value->GetValueInfo() != nullptr && value->GetValueInfo()->IsJsType());
        JsTypeValueInfo* valueInfo = value->GetValueInfo()->AsJsType();
        Assert(valueInfo->GetJsType() == nullptr && valueInfo->GetJsTypeSet() == nullptr);
        UpdateObjectTypeValue(value, type, true, typeSet, true);
    }
    else
    {
        JsTypeValueInfo* valueInfo = JsTypeValueInfo::New(this->alloc, type, typeSet);
        this->SetSymStoreDirect(valueInfo, typeSym);
        Value* value = NewValue(valueInfo);
        blockData->SetValue(value, typeSym);
    }

    blockData->liveFields->Set(typeSymId);
}

void
GlobOpt::KillObjectType(StackSym* objectSym, BVSparse<JitArenaAllocator>* liveFields)
{
    if (objectSym->IsTypeSpec())
    {
        objectSym = objectSym->GetVarEquivSym(this->func);
    }

    Assert(objectSym);

    // We may be conservatively attempting to kill type syms from object syms that don't actually
    // participate in object type specialization and hence don't actually have type syms (yet).
    if (!objectSym->HasObjectTypeSym())
    {
        return;
    }

    if (liveFields == nullptr)
    {
        liveFields = this->currentBlock->globOptData.liveFields;
    }

    liveFields->Clear(objectSym->GetObjectTypeSym()->m_id);
}

void
GlobOpt::KillAllObjectTypes(BVSparse<JitArenaAllocator>* liveFields)
{
    if (this->objectTypeSyms && liveFields)
    {
        liveFields->Minus(this->objectTypeSyms);
    }
}

void
GlobOpt::EndFieldLifetime(IR::SymOpnd *symOpnd)
{
    this->currentBlock->globOptData.liveFields->Clear(symOpnd->m_sym->m_id);
}

PropertySym *
GlobOpt::CopyPropPropertySymObj(IR::SymOpnd *symOpnd, IR::Instr *instr)
{
    Assert(symOpnd->m_sym->IsPropertySym());

    PropertySym *propertySym = symOpnd->m_sym->AsPropertySym();

    StackSym *objSym = propertySym->m_stackSym;

    Value * val = this->currentBlock->globOptData.FindValue(objSym);

    if (val && !PHASE_OFF(Js::ObjPtrCopyPropPhase, this->func))
    {
        StackSym *copySym = this->currentBlock->globOptData.GetCopyPropSym(objSym, val);
        if (copySym != nullptr)
        {
            PropertySym *newProp = PropertySym::FindOrCreate(
                copySym->m_id, propertySym->m_propertyId, propertySym->GetPropertyIdIndex(), propertySym->GetInlineCacheIndex(), propertySym->m_fieldKind, this->func);

            if (!this->IsLoopPrePass() || SafeToCopyPropInPrepass(objSym, copySym, val))
            {
#if DBG_DUMP
                if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::GlobOptPhase, this->func->GetSourceContextId(), this->func->GetLocalFunctionId()))
                {
                    Output::Print(_u("TRACE: "));
                    symOpnd->Dump();
                    Output::Print(_u(" : "));
                    Output::Print(_u("Copy prop obj ptr s%d, new property: "), copySym->m_id);
                    newProp->Dump();
                    Output::Print(_u("\n"));
                }
#endif

                // Copy prop
                this->CaptureByteCodeSymUses(instr);

                // If the old sym was part of an object type spec type check sequence,
                // let's make sure the new one is prepped for it as well.
                if (IsPropertySymPreparedForTypeCheckSeq(propertySym))
                {
                    PreparePropertySymForTypeCheckSeq(newProp);
                }
                symOpnd->m_sym = newProp;
                symOpnd->SetIsJITOptimizedReg(true);

                if (symOpnd->IsPropertySymOpnd())
                {
                    IR::PropertySymOpnd *propertySymOpnd = symOpnd->AsPropertySymOpnd();

                    if (propertySymOpnd->IsTypeCheckSeqCandidate())
                    {
                        // If the new pointer sym's expected type(s) don't match those in the inline-cache-based data for this access,
                        // we probably have a mismatch and can't safely objtypespec. If the saved objtypespecfldinfo isn't right for
                        // the new type, then we'll do an incorrect property access.
                        StackSym * newTypeSym = copySym->GetObjectTypeSym();
                        Value * newValue = currentBlock->globOptData.FindObjectTypeValueNoLivenessCheck(newTypeSym);
                        JsTypeValueInfo * newValueInfo = newValue ? newValue->GetValueInfo()->AsJsType() : nullptr;
                        bool shouldOptimize = CompareCurrentTypesWithExpectedTypes(newValueInfo, propertySymOpnd);
                        if (!shouldOptimize)
                        {
                            propertySymOpnd->SetTypeCheckSeqCandidate(false);
                        }
                    }

                    // This is no longer strictly necessary, since we don't set the type dead bits in the initial
                    // backward pass, but let's keep it around for now in case we choose to revert to the old model.
                    propertySymOpnd->SetTypeDeadIfTypeCheckSeqCandidate(false);
                }

                if (this->IsLoopPrePass())
                {
                    this->prePassCopyPropSym->Set(copySym->m_id);
                }
            }
            propertySym = newProp;

            if(instr->GetDst() && symOpnd->IsEqual(instr->GetDst()))
            {
                // Make sure any stack sym uses in the new destination property sym are unspecialized
                instr = ToVarUses(instr, symOpnd, true, nullptr);
            }
        }
    }

    return propertySym;
}

void
GlobOpt::UpdateObjPtrValueType(IR::Opnd * opnd, IR::Instr * instr)
{
    if (!opnd->IsSymOpnd() || !opnd->AsSymOpnd()->IsPropertySymOpnd())
    {
        return;
    }

    if (!instr->HasTypeCheckBailOut())
    {
        // No type check bailout, we didn't check that type of the object pointer.
        return;
    }

    // Only check that fixed field should have type check bailout in loop prepass.
    Assert(instr->m_opcode == Js::OpCode::CheckFixedFld || !this->IsLoopPrePass());

    if (instr->m_opcode != Js::OpCode::CheckFixedFld)
    {
        // DeadStore pass may remove type check bailout, except CheckFixedFld which always needs
        // type check bailout. So we can only change the type for CheckFixedFld.
        // Consider: See if we can expand that in the future.
        return;
    }

    IR::PropertySymOpnd * propertySymOpnd = opnd->AsPropertySymOpnd();
    StackSym * objectSym = propertySymOpnd->GetObjectSym();
    Value * objVal = this->currentBlock->globOptData.FindValue(objectSym);
    if (!objVal)
    {
        return;
    }

    ValueType objValueType = objVal->GetValueInfo()->Type();
    if (objValueType.IsDefinite())
    {
        return;
    }

    ValueInfo *objValueInfo = objVal->GetValueInfo();

    // It is possible for a valueInfo to be not definite and still have a byteCodeConstant as symStore, this is because we conservatively copy valueInfo in prePass
    if (objValueInfo->GetSymStore() && objValueInfo->GetSymStore()->IsStackSym() && objValueInfo->GetSymStore()->AsStackSym()->IsFromByteCodeConstantTable())
    {
        return;
    }

    // Verify that the types we're checking for here have been locked so that the type ID's can't be changed
    // without changing the type.
    if (!propertySymOpnd->HasObjectTypeSym())
    {
        return;
    }

    StackSym * typeSym = propertySymOpnd->GetObjectTypeSym();
    Assert(typeSym);
    Value * typeValue = currentBlock->globOptData.FindObjectTypeValue(typeSym);
    if (!typeValue)
    {
        return;
    }
    JsTypeValueInfo * typeValueInfo = typeValue->GetValueInfo()->AsJsType();
    JITTypeHolder type = typeValueInfo->GetJsType();
    if (type != nullptr)
    {
        if (Js::DynamicType::Is(type->GetTypeId()) &&
            !type->GetTypeHandler()->IsLocked())
        {
            return;
        }
    }
    else
    {
        Js::EquivalentTypeSet * typeSet = typeValueInfo->GetJsTypeSet();
        Assert(typeSet);
        for (uint16 i = 0; i < typeSet->GetCount(); i++)
        {
            type = typeSet->GetType(i);
            if (Js::DynamicType::Is(type->GetTypeId()) &&
                !type->GetTypeHandler()->IsLocked())
            {
                return;
            }
        }
    }

    AnalysisAssert(type != nullptr);
    Js::TypeId typeId = type->GetTypeId();

    // Passing false for useVirtual as we would never have a virtual typed array hitting this code path
    ValueType newValueType = ValueType::FromTypeId(typeId, false);

    if (newValueType == ValueType::Uninitialized)
    {
        switch (typeId)
        {
        default:
            // Can't mark as definite object because it may actually be object-with-array.
            // Consider: a value type that subsumes object, array, and object-with-array.
            break;
        case Js::TypeIds_NativeIntArray:
        case Js::TypeIds_NativeFloatArray:
            // Do not mark these values as definite to protect against array conversion
            break;
        case Js::TypeIds_Array:
            // Because array can change type id, we can only make it definite if we are doing array check hoist
            // so that implicit call will be installed between the array checks.
            if (!DoArrayCheckHoist() ||
                (currentBlock->loop
                ? !this->ImplicitCallFlagsAllowOpts(currentBlock->loop)
                : !this->ImplicitCallFlagsAllowOpts(this->func)))
            {
                break;
            }
            if (objValueType.IsLikelyArrayOrObjectWithArray())
            {
                // If we have likely no missing values before, keep the likely, because, we haven't proven that
                // the array really has no missing values
                if (!objValueType.HasNoMissingValues())
                {
                    newValueType = ValueType::GetObject(ObjectType::Array).SetArrayTypeId(typeId);
                }
            }
            else
            {
                newValueType = ValueType::GetObject(ObjectType::Array).SetArrayTypeId(typeId);
            }
            break;
        }
    }
    if (newValueType != ValueType::Uninitialized)
    {
        ChangeValueType(currentBlock, objVal, newValueType, false, true);
    }
}
