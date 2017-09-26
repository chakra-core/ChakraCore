//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum ScopeType: int
{
    ScopeType_Unknown,
    ScopeType_Global,
    ScopeType_GlobalEvalBlock,
    ScopeType_FunctionBody,
    ScopeType_FuncExpr,
    ScopeType_Block,
    ScopeType_Catch,
    ScopeType_CatchParamPattern,
    ScopeType_With,
    ScopeType_Parameter
};

class Scope
{
private:
    Scope *enclosingScope;
    Js::ScopeInfo *scopeInfo;
    Js::RegSlot location;
    FuncInfo *func;
    Symbol *m_symList;
    int m_count;
    ArenaAllocator *alloc;
    uint scopeSlotCount; // count of slots in the local scope
    uint innerScopeIndex;
    ScopeType const scopeType;
    BYTE isDynamic : 1;
    BYTE isObject : 1;
    BYTE canMerge : 1;
    BYTE capturesAll : 1;
    BYTE mustInstantiate : 1;
    BYTE hasCrossScopeFuncAssignment : 1;
    BYTE hasDuplicateFormals : 1;
    BYTE canMergeWithBodyScope : 1;
    BYTE hasLocalInClosure : 1;
    BYTE isBlockInLoop : 1;
public:
#if DBG
    BYTE isRestored : 1;
#endif
    Scope(ArenaAllocator *alloc, ScopeType scopeType, int capacity = 0) :
        alloc(alloc),
        func(nullptr),
        enclosingScope(nullptr),
        scopeInfo(nullptr),
        isDynamic(false),
        isObject(false),
        canMerge(true),
        capturesAll(false),
        mustInstantiate(false),
        hasCrossScopeFuncAssignment(false),
        hasDuplicateFormals(false),
        canMergeWithBodyScope(true),
        hasLocalInClosure(false),
        isBlockInLoop(false),
        location(Js::Constants::NoRegister),
        m_symList(nullptr),
        m_count(0),
        scopeSlotCount(0),
        innerScopeIndex((uint)-1),
        scopeType(scopeType)
#if DBG
        , isRestored(false)
#endif
    {
    }

    Symbol *FindLocalSymbol(SymbolName const& key)
    {
        Symbol *sym;
        for (sym = m_symList; sym; sym = sym->GetNext())
        {
            if (sym->GetName() == key)
            {
                break;
            }
        }
        return sym;
    }

    template<class Fn>
    void ForEachSymbol(Fn fn)
    {
        for (Symbol *sym = m_symList; sym;)
        {
            Symbol *next = sym->GetNext();
            fn(sym);
            sym = next;
        }
    }

    template<class Fn>
    void ForEachSymbolUntil(Fn fn)
    {
        for (Symbol *sym = m_symList; sym;)
        {
            Symbol *next = sym->GetNext();
            if (fn(sym))
            {
                return;
            }
            sym = next;
        }
    }

    // For JScript, this should not return NULL because
    // there is always an enclosing global scope.
    Symbol *FindSymbol(SymbolName const& name, SymbolType symbolType, bool fCreate = true)
    {
        Symbol *sym = FindLocalSymbol(name);
        if (sym == nullptr)
        {
            if (enclosingScope != nullptr)
            {
                sym = enclosingScope->FindSymbol(name, symbolType);
            }
            else if (fCreate)
            {
                sym = Anew(alloc, Symbol, name, nullptr, symbolType);
                AddNewSymbol(sym);
            }
        }
        return sym;
    }

    void AddSymbol(Symbol *sym)
    {
        if (enclosingScope == nullptr)
        {
            sym->SetIsGlobal(true);
        }
        sym->SetScope(this);
        for (Symbol *symInList = m_symList; symInList; symInList = symInList->GetNext())
        {
            if (symInList->GetName() == sym->GetName())
            {
                return;
            }
        }
        sym->SetNext(m_symList);
        m_symList = sym;
        m_count++;
    }

    void AddNewSymbol(Symbol *sym)
    {
        if (scopeType == ScopeType_Global)
        {
            sym->SetIsGlobal(true);
        }
        sym->SetScope(this);
        sym->SetNext(m_symList);
        m_symList = sym;
        m_count++;
    }

    bool HasStaticPathToAncestor(Scope const * target) const
    {
        return target == this || (!isDynamic && enclosingScope != nullptr && enclosingScope->HasStaticPathToAncestor(target));
    }

    void SetEnclosingScope(Scope *enclosingScope)
    {
        // Check for scope cycles
        Assert(enclosingScope != this);
        Assert(enclosingScope == nullptr || this != enclosingScope->GetEnclosingScope());
        this->enclosingScope = enclosingScope;
    }

    Scope *GetEnclosingScope() const
    {
        return enclosingScope;
    }

    void SetScopeInfo(Js::ScopeInfo * scopeInfo)
    {
        this->scopeInfo = scopeInfo;
    }

    Js::ScopeInfo * GetScopeInfo() const
    {
        return this->scopeInfo;
    }

    ScopeType GetScopeType() const
    {
        return this->scopeType;
    }

    bool IsInnerScope() const
    {
        return scopeType == ScopeType_Block
            || scopeType == ScopeType_Catch
            || scopeType == ScopeType_CatchParamPattern
            || scopeType == ScopeType_GlobalEvalBlock;
    }

    int Count() const
    {
        return m_count;
    }

    void SetFunc(FuncInfo *func)
    {
        this->func = func;
    }

    FuncInfo *GetFunc() const
    {
        return func;
    }

    FuncInfo *GetEnclosingFunc()
    {
        Scope *scope = this;
        while (scope && scope->func == nullptr)
        {
            scope = scope->GetEnclosingScope();
        }
        AnalysisAssert(scope);
        return scope->func;
    }

    void SetLocation(Js::RegSlot loc) { location = loc; }
    Js::RegSlot GetLocation() const { return location; }

    void SetIsDynamic(bool is) { isDynamic = is; }
    bool GetIsDynamic() const { return isDynamic; }

    bool IsEmpty() const;

    bool IsBlockScope(FuncInfo *funcInfo);

    void SetIsObject();
    bool GetIsObject() const { return isObject; }

    void SetCapturesAll(bool does) { capturesAll = does; }
    bool GetCapturesAll() const { return capturesAll; }

    void SetMustInstantiate(bool must) { mustInstantiate = must; }
    bool GetMustInstantiate() const { return mustInstantiate; }

    void SetCanMerge(bool can) { canMerge = can; }
    bool GetCanMerge() const { return canMerge && !mustInstantiate && !isObject; }

    void SetScopeSlotCount(uint i) { scopeSlotCount = i; }
    uint GetScopeSlotCount() const { return scopeSlotCount; }

    void SetHasDuplicateFormals() { hasDuplicateFormals = true; }
    bool GetHasDuplicateFormals() { return hasDuplicateFormals; }

    void SetHasOwnLocalInClosure(bool has) { hasLocalInClosure = has; }
    bool GetHasOwnLocalInClosure() const { return hasLocalInClosure; }

    void SetIsBlockInLoop(bool is = true) { isBlockInLoop = is; }
    bool IsBlockInLoop() const { return isBlockInLoop; }

    bool HasInnerScopeIndex() const { return innerScopeIndex != (uint)-1; }
    uint GetInnerScopeIndex() const { return innerScopeIndex; }
    void SetInnerScopeIndex(uint index) { innerScopeIndex = index; }

    int AddScopeSlot();

    void SetHasCrossScopeFuncAssignment() { hasCrossScopeFuncAssignment = true; }
    bool HasCrossScopeFuncAssignment() const { return hasCrossScopeFuncAssignment; }

    void ForceAllSymbolNonLocalReference(ByteCodeGenerator *byteCodeGenerator);

    bool IsGlobalEvalBlockScope() const;

    static void MergeParamAndBodyScopes(ParseNode *pnodeScope);
    static void RemoveParamScope(ParseNode *pnodeScope);
};
