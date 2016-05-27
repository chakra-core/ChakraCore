//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

bool Scope::IsGlobalEvalBlockScope() const
{
    return this->scopeType == ScopeType_GlobalEvalBlock;
}

bool Scope::IsBlockScope(FuncInfo *funcInfo)
{
    return this != funcInfo->GetBodyScope() && this != funcInfo->GetParamScope();
}

void Scope::SetHasLocalInClosure(bool has)
{
    SetHasOwnLocalInClosure(has);

    // (Note: if any catch var is closure-captured, we won't merge the catch scope with the function scope.
    // So don't mark the function scope "has local in closure".)
    bool notCatch = this->scopeType != ScopeType_Catch && this->scopeType != ScopeType_CatchParamPattern;
    if (has && (this == func->GetBodyScope() || this == func->GetParamScope()) || (GetCanMerge() && notCatch))
    {
        func->SetHasLocalInClosure(true);
    }
    else
    {
        if (hasCrossScopeFuncAssignment)
        {
            func->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("InstantiateScopeWithCrossScopeAssignment")));
        }
        SetMustInstantiate(true);
    }
}

int Scope::AddScopeSlot()
{
    int slot = scopeSlotCount++;
    if (scopeSlotCount == Js::ScopeSlots::MaxEncodedSlotCount)
    {
        this->GetEnclosingFunc()->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("TooManySlots")));
    }
    return slot;
}

void Scope::ForceAllSymbolNonLocalReference(ByteCodeGenerator *byteCodeGenerator)
{
    this->ForEachSymbol([this, byteCodeGenerator](Symbol *const sym)
    {
        if (!sym->GetIsArguments())
        {
            sym->SetHasNonLocalReference(true, byteCodeGenerator);
            this->GetFunc()->SetHasLocalInClosure(true);
        }
    });
}

bool Scope::IsEmpty() const
{
    if (GetFunc()->bodyScope == this || (GetFunc()->IsGlobalFunction() && this->IsGlobalEvalBlockScope()))
    {
        return Count() == 0 && !GetFunc()->isThisLexicallyCaptured;
    }
    else
    {
        return Count() == 0;
    }
}

void Scope::SetIsObject()
{
    if (this->isObject)
    {
        return;
    }

    this->isObject = true;

    // We might set the scope to be object after we have process the symbol
    // (e.g. "With" scope referencing a symbol in an outer scope).
    // If we have func assignment, we need to mark the function to not do stack nested function
    // as these are now assigned to a scope object.
    FuncInfo * funcInfo = this->GetFunc();
    if (funcInfo && !funcInfo->HasMaybeEscapedNestedFunc())
    {
        this->ForEachSymbolUntil([funcInfo](Symbol * const sym)
        {
            if (sym->GetHasFuncAssignment())
            {
                funcInfo->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("DelayedObjectScopeAssignment")));
                return true;
            }
            return false;
        });
    }
}

void Scope::MergeParamAndBodyScopes(ParseNode *pnodeScope, ByteCodeGenerator *byteCodeGenerator)
{
    Assert(pnodeScope->sxFnc.funcInfo);
    Scope *paramScope = pnodeScope->sxFnc.pnodeScopes->sxBlock.scope;
    Scope *bodyScope = pnodeScope->sxFnc.pnodeBodyScope->sxBlock.scope;

    Assert(paramScope->m_symList == nullptr || paramScope->symbolTable == nullptr);
    Assert(bodyScope->m_symList == nullptr || bodyScope->symbolTable == nullptr);

    if (paramScope->Count() == 0)
    {
        // Once the scopes are merged, there's no reason to instantiate the param scope.
        paramScope->SetMustInstantiate(false);

        // Scopes are already merged or we don't have an arguments object. Go ahead and
        // remove the param scope from the scope chain.
        bodyScope->SetEnclosingScope(paramScope->GetEnclosingScope());
        return;
    }

    bodyScope->scopeSlotCount = paramScope->scopeSlotCount;
    paramScope->ForEachSymbol([&](Symbol * sym)
    {
        bodyScope->AddNewSymbol(sym);
    });

    if (paramScope->GetIsObject())
    {
        bodyScope->SetIsObject();
    }
    if (paramScope->GetMustInstantiate())
    {
        bodyScope->SetMustInstantiate(true);
    }

    // Once the scopes are merged, there's no reason to instantiate the param scope.
    paramScope->SetMustInstantiate(false);

    paramScope->m_count = 0;
    paramScope->scopeSlotCount = 0;
    paramScope->m_symList = nullptr;
    paramScope->symbolTable = nullptr;

    // Remove the parameter scope from the scope chain.
    bodyScope->SetEnclosingScope(paramScope->GetEnclosingScope());
}
