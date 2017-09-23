//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

#if DBG_DUMP
static const char16 * const SymbolTypeNames[] = { _u("Function"), _u("Variable"), _u("MemberName"), _u("Formal"), _u("Unknown") };
#endif

bool Symbol::GetIsArguments() const
{
    return decl != nullptr && (decl->grfpn & PNodeFlags::fpnArguments);
}

Js::PropertyId Symbol::EnsurePosition(ByteCodeGenerator* byteCodeGenerator)
{
    // Guarantee that a symbol's name has a property ID mapping.
    if (this->position == Js::Constants::NoProperty)
    {
        this->position = this->EnsurePositionNoCheck(byteCodeGenerator->TopFuncInfo());
    }
    return this->position;
}

Js::PropertyId Symbol::EnsurePosition(FuncInfo *funcInfo)
{
    // Guarantee that a symbol's name has a property ID mapping.
    if (this->position == Js::Constants::NoProperty)
    {
        this->position = this->EnsurePositionNoCheck(funcInfo);
    }
    return this->position;
}

Js::PropertyId Symbol::EnsurePositionNoCheck(FuncInfo *funcInfo)
{
    return funcInfo->byteCodeFunction->GetOrAddPropertyIdTracked(this->GetName());
}

void Symbol::SaveToPropIdArray(Symbol *sym, Js::PropertyIdArray *propIds, ByteCodeGenerator *byteCodeGenerator, Js::PropertyId *pFirstSlot /* = null */)
{
    if (sym)
    {
        Js::PropertyId slot = sym->scopeSlot;
        if (slot != Js::Constants::NoProperty)
        {
            Assert((uint32)slot < propIds->count);
            propIds->elements[slot] = sym->EnsurePosition(byteCodeGenerator);
            if (pFirstSlot && !sym->GetIsArguments())
            {
                if (*pFirstSlot == Js::Constants::NoProperty ||
                    *pFirstSlot > slot)
                {
                    *pFirstSlot = slot;
                }
            }
        }
    }
}

bool Symbol::NeedsSlotAlloc(FuncInfo *funcInfo)
{
    return IsInSlot(funcInfo, true);
}

bool Symbol::IsInSlot(FuncInfo *funcInfo, bool ensureSlotAlloc)
{
    if (this->GetIsGlobal() || this->GetIsModuleExportStorage())
    {
        return false;
    }
    if (funcInfo->GetHasHeapArguments() && this->GetIsFormal() && ByteCodeGenerator::NeedScopeObjectForArguments(funcInfo, funcInfo->root))
    {
        return true;
    }
    if (this->GetIsGlobalCatch())
    {
        return true;
    }
    if (this->scope->GetCapturesAll())
    {
        return true;
    }

    return this->GetHasNonLocalReference() && (ensureSlotAlloc || this->GetIsCommittedToSlot());
}

bool Symbol::GetIsCommittedToSlot() const
{
    if (!PHASE_ON1(Js::DelayCapturePhase))
    {
        return true;
    }
    return isCommittedToSlot || this->scope->GetFunc()->GetCallsEval() || this->scope->GetFunc()->GetChildCallsEval();
}

Js::PropertyId Symbol::EnsureScopeSlot(FuncInfo *funcInfo)
{
    if (this->NeedsSlotAlloc(funcInfo) && this->scopeSlot == Js::Constants::NoProperty)
    {
        this->scopeSlot = this->scope->AddScopeSlot();
    }
    return this->scopeSlot;
}

void Symbol::SetHasNonLocalReference()
{
    this->hasNonLocalReference = true;
    this->scope->SetHasOwnLocalInClosure(true);
}

void Symbol::SetHasMaybeEscapedUse(ByteCodeGenerator * byteCodeGenerator)
{
    Assert(!this->GetIsMember());
    if (!hasMaybeEscapedUse)
    {
        SetHasMaybeEscapedUseInternal(byteCodeGenerator);
    }
}

void Symbol::SetHasMaybeEscapedUseInternal(ByteCodeGenerator * byteCodeGenerator)
{
    Assert(!hasMaybeEscapedUse);
    Assert(!this->GetIsFormal());
    hasMaybeEscapedUse = true;
    if (PHASE_TESTTRACE(Js::StackFuncPhase, byteCodeGenerator->TopFuncInfo()->byteCodeFunction))
    {
        Output::Print(_u("HasMaybeEscapedUse: %s\n"), this->GetName().GetBuffer());
        Output::Flush();
    }
    if (this->GetHasFuncAssignment())
    {
        this->GetScope()->GetFunc()->SetHasMaybeEscapedNestedFunc(
            DebugOnly(this->symbolType == STFunction ? _u("MaybeEscapedUseFuncDecl") : _u("MaybeEscapedUse")));
    }
}

void Symbol::SetHasFuncAssignment(ByteCodeGenerator * byteCodeGenerator)
{
    Assert(!this->GetIsMember());
    if (!hasFuncAssignment)
    {
        SetHasFuncAssignmentInternal(byteCodeGenerator);
    }
}

void Symbol::SetHasFuncAssignmentInternal(ByteCodeGenerator * byteCodeGenerator)
{
    Assert(!hasFuncAssignment);
    hasFuncAssignment = true;
    FuncInfo * top = byteCodeGenerator->TopFuncInfo();
    if (PHASE_TESTTRACE(Js::StackFuncPhase, top->byteCodeFunction))
    {
        Output::Print(_u("HasFuncAssignment: %s\n"), this->GetName().GetBuffer());
        Output::Flush();
    }

    if (this->GetHasMaybeEscapedUse() || this->GetScope()->GetIsObject())
    {
        byteCodeGenerator->TopFuncInfo()->SetHasMaybeEscapedNestedFunc(DebugOnly(
            this->GetIsFormal() ? _u("FormalAssignment") :
            this->GetScope()->GetIsObject() ? _u("ObjectScopeAssignment") :
            _u("MaybeEscapedUse")));
    }
}

void Symbol::RestoreHasFuncAssignment()
{
    Assert(hasFuncAssignment == (this->symbolType == STFunction));
    Assert(this->GetIsFormal() || !this->GetHasMaybeEscapedUse());
    hasFuncAssignment = true;
    if (PHASE_TESTTRACE1(Js::StackFuncPhase))
    {
        Output::Print(_u("RestoreHasFuncAssignment: %s\n"), this->GetName().GetBuffer());
        Output::Flush();
    }
}

Symbol * Symbol::GetFuncScopeVarSym() const
{
    if (!this->GetIsBlockVar())
    {
        return nullptr;
    }
    FuncInfo * parentFuncInfo = this->GetScope()->GetFunc();
    if (parentFuncInfo->GetIsStrictMode())
    {
        return nullptr;
    }
    Symbol *fncScopeSym = parentFuncInfo->GetBodyScope()->FindLocalSymbol(this->GetName());
    if (fncScopeSym == nullptr && parentFuncInfo->GetParamScope() != nullptr)
    {
        // We couldn't find the sym in the body scope, try finding it in the parameter scope.
        Scope* paramScope = parentFuncInfo->GetParamScope();
        fncScopeSym = paramScope->FindLocalSymbol(this->GetName());
    }

    // Parser should have added a fake var decl node for block scoped functions in non-strict mode
    // IsBlockVar() indicates a user let declared variable at function scope which
    // shadows the function's var binding, thus only emit the var binding init if
    // we do not have a block var symbol.
    if (!fncScopeSym || fncScopeSym->GetIsBlockVar())
    {
        return nullptr;
    }
    return fncScopeSym;
}

#if DBG_DUMP
const char16 * Symbol::GetSymbolTypeName()
{
    return SymbolTypeNames[symbolType];
}
#endif
