//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

namespace Js
{
    //
    // Persist one symbol info into ScopeInfo.
    //
    void ScopeInfo::SaveSymbolInfo(Symbol* sym, MapSymbolData* mapSymbolData)
    {
        // We don't need to create slot for or save "arguments"
        bool needScopeSlot = !sym->GetIsArguments() && sym->GetHasNonLocalReference();
        Js::PropertyId scopeSlot = Constants::NoSlot;

        if (sym->GetIsModuleExportStorage())
        {
            // Export symbols aren't in slots but we need to persist the fact that they are export storage
            scopeSlot = sym->GetScope()->GetScopeSlotCount() + mapSymbolData->nonScopeSymbolCount++;
        }
        else if (needScopeSlot)
        {
            // Any symbol may have non-local ref from deferred child. Allocate slot for it.
            scopeSlot = sym->EnsureScopeSlot(mapSymbolData->func);
        }

        if (needScopeSlot || sym->GetIsModuleExportStorage())
        {
            Js::PropertyId propertyId = sym->EnsurePosition(mapSymbolData->func);
            this->SetSymbolId(scopeSlot, propertyId);
            this->SetSymbolType(scopeSlot, sym->GetSymbolType());
            this->SetHasFuncAssignment(scopeSlot, sym->GetHasFuncAssignment());
            this->SetIsBlockVariable(scopeSlot, sym->GetIsBlockVar());
            this->SetIsFuncExpr(scopeSlot, sym->GetIsFuncExpr());
            this->SetIsModuleExportStorage(scopeSlot, sym->GetIsModuleExportStorage());
            this->SetIsModuleImport(scopeSlot, sym->GetIsModuleImport());
        }

        TRACE_BYTECODE(_u("%12s %d\n"), sym->GetName().GetBuffer(), sym->GetScopeSlot());
    }

    inline void AddSlotCount(int& count, int addCount)
    {
        if (addCount != 0 && Int32Math::Add(count, addCount, &count))
        {
            ::Math::DefaultOverflowPolicy();
        }
    }

    //
    // Create scope info for a single scope.
    //
    ScopeInfo* ScopeInfo::SaveOneScopeInfo(/*ByteCodeGenerator* byteCodeGenerator, ParseableFunctionInfo* parent,*/ Scope* scope, ScriptContext *scriptContext)
    {
        Assert(scope->GetScopeInfo() == nullptr);
        Assert(scope->GetScopeType() != ScopeType_Global);

        int count = scope->Count();

        // Add argsPlaceHolder which includes same name args and destructuring patterns on parameters
        AddSlotCount(count, scope->GetFunc()->argsPlaceHolderSlotCount);
        AddSlotCount(count, scope->GetFunc()->thisScopeSlot != Js::Constants::NoRegister ? 1 : 0);
        AddSlotCount(count, scope->GetFunc()->superScopeSlot != Js::Constants::NoRegister ? 1 : 0);
        AddSlotCount(count, scope->GetFunc()->newTargetScopeSlot != Js::Constants::NoRegister ? 1 : 0);

        ScopeInfo* scopeInfo = RecyclerNewPlusZ(scriptContext->GetRecycler(),
            count * sizeof(SymbolInfo),
            ScopeInfo, scope->GetFunc()->byteCodeFunction->GetFunctionInfo(),/*parent ? parent->GetFunctionInfo() : nullptr,*/ count);
        scopeInfo->SetScopeType(scope->GetScopeType());
        scopeInfo->isDynamic = scope->GetIsDynamic();
        scopeInfo->isObject = scope->GetIsObject();
        scopeInfo->mustInstantiate = scope->GetMustInstantiate();
        scopeInfo->isCached = (scope->GetFunc()->GetBodyScope() == scope) && scope->GetFunc()->GetHasCachedScope();
        scopeInfo->hasLocalInClosure = scope->GetHasOwnLocalInClosure();
        

        TRACE_BYTECODE(_u("\nSave ScopeInfo: %s #symbols: %d %s\n"),
            scope->GetFunc()->name, count,
            scopeInfo->isObject ? _u("isObject") : _u(""));

        MapSymbolData mapSymbolData = { scope->GetFunc(), 0 };
        scope->ForEachSymbol([&mapSymbolData, scopeInfo, scope](Symbol * sym)
        {
            Assert(scope == sym->GetScope());
            scopeInfo->SaveSymbolInfo(sym, &mapSymbolData);
        });

        scope->SetScopeInfo(scopeInfo);

        return scopeInfo;
    }

    //
    // Ensure the pids referenced by this scope are tracked.
    //
    void ScopeInfo::EnsurePidTracking(ScriptContext* scriptContext)
    {
        for (int i = 0; i < symbolCount; i++)
        {
            auto propertyName = scriptContext->GetPropertyName(symbols[i].propertyId);
            scriptContext->TrackPid(propertyName);
        }
    }

    //
    // Save scope info for an individual scope and link it to its enclosing scope.
    //
    ScopeInfo * ScopeInfo::SaveScopeInfo(Scope * scope/*ByteCodeGenerator* byteCodeGenerator, FuncInfo* parentFunc, FuncInfo* func*/, ScriptContext * scriptContext)
    {
        // Advance past scopes that will be excluded from the closure environment. (But note that we always want the body scope.)
        while (scope && (!scope->GetMustInstantiate() && scope != scope->GetFunc()->GetBodyScope()))
        {
            scope = scope->GetEnclosingScope();
        }

        // If we've exhausted the scope chain, we're done.
        if (scope == nullptr || scope->GetScopeType() == ScopeType_Global)
        {
            return nullptr;
        }

        // If we've already collected info for this scope, we're done.
        ScopeInfo * scopeInfo = scope->GetScopeInfo();
        if (scopeInfo != nullptr)
        {
            return scopeInfo;
        }

        // Do the work for this scope.
        scopeInfo = ScopeInfo::SaveOneScopeInfo(scope, scriptContext);

        // Link to the parent (if any).
        scope = scope->GetEnclosingScope();
        if (scope)
        {
            scopeInfo->SetParentScopeInfo(ScopeInfo::SaveScopeInfo(scope, scriptContext));
        }

        return scopeInfo;
    }

    void ScopeInfo::SaveEnclosingScopeInfo(ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
    {
        // TODO: Not technically necessary, as we always do scope look up on eval if it is
        // not found in the scope chain, and block scopes are always objects in eval.
        // But if we save the global eval block scope for deferred child so that we can look up
        // let/const in that scope with slot index instead of doing a scope lookup.
        // We will have to implement encoding block scope info to enable, which will also
        // enable defer parsing function that are in block scopes.

        if (funcInfo->byteCodeFunction &&
            funcInfo->byteCodeFunction->GetScopeInfo() != nullptr)
        {
            // No need to regenerate scope info if we re-compile an enclosing function
            return;
        }

        Scope* currentScope = byteCodeGenerator->GetCurrentScope();
        Assert(currentScope->GetFunc() == funcInfo);

        while (currentScope->GetFunc() == funcInfo)
        {
            currentScope = currentScope->GetEnclosingScope();
        }

        ScopeInfo * scopeInfo = ScopeInfo::SaveScopeInfo(currentScope, byteCodeGenerator->GetScriptContext());
        if (scopeInfo != nullptr)
        {
            funcInfo->byteCodeFunction->SetScopeInfo(scopeInfo);
        }
    }

    //
    // Load persisted scope info.
    //
    void ScopeInfo::ExtractScopeInfo(Parser *parser, /*ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo,*/ Scope* scope)
    {
        ScriptContext* scriptContext;
        ArenaAllocator* alloc;

        // Load scope attributes and push onto scope stack.
        scope->SetMustInstantiate(this->mustInstantiate);
        scope->SetHasOwnLocalInClosure(this->hasLocalInClosure);
        scope->SetIsDynamic(this->isDynamic);
        if (this->isObject)
        {
            scope->SetIsObject();
        }

        Assert(parser);

        scriptContext = parser->GetScriptContext();
        alloc = parser->GetAllocator();

        // Load scope symbols
        // On first access to the scopeinfo, replace the ID's with PropertyRecord*'s to save the dictionary lookup
        // on later accesses. Replace them all before allocating Symbol's to prevent inconsistency on OOM.
        if (!this->areNamesCached && !PHASE_OFF1(Js::CacheScopeInfoNamesPhase))
        {
            for (int i = 0; i < symbolCount; i++)
            {
                PropertyId propertyId = GetSymbolId(i);
                if (propertyId != 0) // There may be empty slots, e.g. "arguments" may have no slot
                {
                    PropertyRecord const* name = scriptContext->GetPropertyName(propertyId);
                    this->SetPropertyName(i, name);
                }
            }
            this->areNamesCached = true;
        }

        for (int i = 0; i < symbolCount; i++)
        {
            PropertyRecord const* name = nullptr;
            if (this->areNamesCached)
            {
                name = this->GetPropertyName(i);
            }
            else
            {
                PropertyId propertyId = GetSymbolId(i);
                if (propertyId != 0) // There may be empty slots, e.g. "arguments" may have no slot
                {
                    name = scriptContext->GetPropertyName(propertyId);
                }
            }

            if (name != nullptr)
            {
                SymbolType symbolType = GetSymbolType(i);
                SymbolName symName(name->GetBuffer(), name->GetLength());
                Symbol *sym = Anew(alloc, Symbol, symName, nullptr, symbolType);

                sym->SetScopeSlot(static_cast<PropertyId>(i));
                sym->SetIsBlockVar(GetIsBlockVariable(i));
                sym->SetIsFuncExpr(GetIsFuncExpr(i));
                sym->SetIsModuleExportStorage(GetIsModuleExportStorage(i));
                sym->SetIsModuleImport(GetIsModuleImport(i));
                if (GetHasFuncAssignment(i))
                {
                    sym->RestoreHasFuncAssignment();
                }
                scope->AddNewSymbol(sym);
                sym->SetHasNonLocalReference();
                Assert(parser);
                parser->RestorePidRefForSym(sym);

                TRACE_BYTECODE(_u("%12s %d\n"), sym->GetName().GetBuffer(), sym->GetScopeSlot());
            }
        }
        this->scope = scope;
        DebugOnly(scope->isRestored = true);
    }

    ScopeInfo::AutoCapturesAllScope::AutoCapturesAllScope(Scope* scope, bool turnOn)
        : scope(scope)
    {
        oldCapturesAll = scope->GetCapturesAll();
        if (turnOn)
        {
            scope->SetCapturesAll(true);
        }
    }

    ScopeInfo::AutoCapturesAllScope::~AutoCapturesAllScope()
    {
        scope->SetCapturesAll(oldCapturesAll);
    }
} // namespace Js
