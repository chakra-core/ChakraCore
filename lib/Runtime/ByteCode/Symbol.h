//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

enum SymbolType : byte
{
    STFunction,
    STVariable,
    STMemberName,
    STFormal,
    STUnknown
};

typedef JsUtil::CharacterBuffer<WCHAR> SymbolName;
class Symbol
{
private:
    const SymbolName name;
    IdentPtr pid;
    ParseNode *decl;
    Scope *scope;                   // scope defining this symbol
    Js::PropertyId position;        // argument position in function declaration
    Js::RegSlot location;           // register in which the symbol resides
    Js::PropertyId scopeSlot;
    Js::PropertyId moduleIndex;
    Symbol *next;

    SymbolType symbolType;
    BYTE defCount;
    BYTE needDeclaration : 1;
    BYTE isBlockVar : 1;
    BYTE isGlobal : 1;
    BYTE isEval : 1;
    BYTE hasNonLocalReference : 1;  // if true, then this symbol needs to be heap-allocated
    BYTE isFuncExpr : 1;              // if true, then this symbol is allocated on it's on activation object
    BYTE isCatch : 1;               // if true then this a catch identifier
    BYTE hasInit : 1;
    BYTE isUsed : 1;
    BYTE isGlobalCatch : 1;
    BYTE isCommittedToSlot : 1;
    BYTE hasNonCommittedReference : 1;
    BYTE hasRealBlockVarRef : 1;
    BYTE hasBlockFncVarRedecl : 1;
    BYTE hasVisitedCapturingFunc : 1;
    BYTE isTrackedForDebugger : 1; // Whether the sym is tracked for debugger scope. This is fine because a sym can only be added to (not more than) one scope.
    BYTE isModuleExportStorage : 1; // If true, this symbol should be stored in the global scope export storage array.
    BYTE isModuleImport : 1; // If true, this symbol is the local name of a module import statement
    BYTE isUsedInLdElem : 1;
    BYTE needsScopeObject : 1;

    // These are get and set a lot, don't put it in bit fields, we are exceeding the number of bits anyway
    bool hasFuncAssignment;
    bool hasMaybeEscapedUse;
    bool isNonSimpleParameter;

    AssignmentState assignmentState;

public:
    Symbol(SymbolName const& name, ParseNode *decl, SymbolType symbolType) :
        name(name),
        decl(decl),
        next(nullptr),
        location(Js::Constants::NoRegister),
        needDeclaration(false),
        isBlockVar(false),
        isGlobal(false),
        hasNonLocalReference(false),
        isFuncExpr(false),
        isCatch(false),
        hasInit(false),
        isUsed(false),
        defCount(0),
        position(Js::Constants::NoProperty),
        scopeSlot(Js::Constants::NoProperty),
        isGlobalCatch(false),
        isCommittedToSlot(false),
        hasNonCommittedReference(false),
        hasRealBlockVarRef(false),
        hasBlockFncVarRedecl(false),
        hasVisitedCapturingFunc(false),
        isTrackedForDebugger(false),
        isNonSimpleParameter(false),
        assignmentState(NotAssigned),
        isModuleExportStorage(false),
        isModuleImport(false),
        isUsedInLdElem(false),
        needsScopeObject(false),
        moduleIndex(Js::Constants::NoProperty)
    {
        SetSymbolType(symbolType);

        // Set it so we don't have to check it explicitly
        isEval = MatchName(_u("eval"), 4);

        if (PHASE_TESTTRACE1(Js::StackFuncPhase) && hasFuncAssignment)
        {
            Output::Print(_u("HasFuncDecl: %s\n"), this->GetName().GetBuffer());
            Output::Flush();
        }
    }

    bool MatchName(const char16 *key, int length)
    {
        return name == SymbolName(key, length);
    }

    void SetScope(Scope *scope)
    {
        this->scope = scope;
    }
    Scope * GetScope() const { return scope; }

    void SetDecl(ParseNode *pnodeDecl) { decl = pnodeDecl; }
    ParseNode* GetDecl() const { return decl; }

    void SetScopeSlot(Js::PropertyId slot)
    {
        this->scopeSlot = slot;
    }

    Symbol *GetNext() const
    {
        return next;
    }

    void SetNext(Symbol *sym)
    {
        next = sym;
    }

    void SetIsGlobal(bool b)
    {
        isGlobal = b;
    }

    void SetHasNonLocalReference();

    bool GetHasNonLocalReference() const
    {
        return hasNonLocalReference;
    }

    void SetIsFuncExpr(bool b)
    {
        isFuncExpr = b;
    }

    void SetIsBlockVar(bool is)
    {
        isBlockVar = is;
    }

    bool GetIsBlockVar() const
    {
        return isBlockVar;
    }

    void SetIsModuleExportStorage(bool is)
    {
        isModuleExportStorage = is;
    }

    bool GetIsModuleExportStorage() const
    {
        return isModuleExportStorage;
    }

    void SetIsModuleImport(bool is)
    {
        isModuleImport = is;
    }

    bool GetIsModuleImport() const
    {
        return isModuleImport;
    }

    void SetIsUsedInLdElem(bool is)
    {
        isUsedInLdElem = is;
    }

    bool IsUsedInLdElem() const
    {
        return isUsedInLdElem;
    }

    void SetNeedsScopeObject(bool does = true)
    {
        needsScopeObject = does;
    }

    bool NeedsScopeObject() const
    {
        return needsScopeObject;
    }

    void SetModuleIndex(Js::PropertyId index)
    {
        moduleIndex = index;
    }

    Js::PropertyId GetModuleIndex()
    {
        return moduleIndex;
    }

    void SetIsGlobalCatch(bool is)
    {
        isGlobalCatch = is;
    }

    bool GetIsGlobalCatch() const
    {
        return isGlobalCatch;
    }

    void SetIsCommittedToSlot()
    {
        this->isCommittedToSlot = true;
    }

    bool GetIsCommittedToSlot() const;

    void SetHasVisitedCapturingFunc()
    {
        this->hasVisitedCapturingFunc = true;
    }

    bool HasVisitedCapturingFunc() const
    {
        return hasVisitedCapturingFunc;
    }

    void SetHasNonCommittedReference(bool has)
    {
        this->hasNonCommittedReference = has;
    }

    bool GetHasNonCommittedReference() const
    {
        return hasNonCommittedReference;
    }

    void SetIsTrackedForDebugger(bool is)
    {
        isTrackedForDebugger = is;
    }

    bool GetIsTrackedForDebugger() const
    {
        return isTrackedForDebugger;
    }

    void SetNeedDeclaration(bool need)
    {
        needDeclaration = need;
    }

    bool GetNeedDeclaration() const
    {
        return needDeclaration;
    }

    bool GetIsFuncExpr() const
    {
        return isFuncExpr;
    }

    bool GetIsGlobal() const
    {
        return isGlobal;
    }

    bool GetIsMember() const
    {
        return symbolType == STMemberName;
    }

    bool GetIsFormal() const
    {
        return symbolType == STFormal;
    }

    bool GetIsEval() const
    {
        return isEval;
    }

    bool GetIsCatch() const
    {
        return isCatch;
    }

    void SetIsCatch(bool b)
    {
        isCatch = b;
    }

    bool GetHasInit() const
    {
        return hasInit;
    }

    void RecordDef()
    {
        defCount++;
    }

    bool SingleDef() const
    {
        return defCount == 1;
    }

    void SetHasInit(bool has)
    {
        hasInit = has;
    }

    bool GetIsUsed() const
    {
        return isUsed;
    }

    void SetIsUsed(bool is)
    {
        isUsed = is;
    }

    bool HasRealBlockVarRef() const
    {
        return hasRealBlockVarRef;
    }

    void SetHasRealBlockVarRef(bool has = true)
    {
        hasRealBlockVarRef = has;
    }

    bool HasBlockFncVarRedecl() const
    {
        return hasBlockFncVarRedecl;
    }

    void SetHasBlockFncVarRedecl(bool has = true)
    {
        hasBlockFncVarRedecl = has;
    }

    AssignmentState GetAssignmentState() const
    {
        return assignmentState;
    }

    void PromoteAssignmentState()
    {
        if (assignmentState == NotAssigned)
        {
            assignmentState = AssignedOnce;
        }
        else if (assignmentState == AssignedOnce)
        {
            assignmentState = AssignedMultipleTimes;
        }
    }

    bool IsAssignedOnce()
    {
        return assignmentState == AssignedOnce;
    }

    // For stack nested function escape analysis
    bool GetHasMaybeEscapedUse() const { return hasMaybeEscapedUse; }
    void SetHasMaybeEscapedUse(ByteCodeGenerator * byteCodeGenerator);
    bool GetHasFuncAssignment() const { return hasFuncAssignment; }
    void SetHasFuncAssignment(ByteCodeGenerator * byteCodeGenerator);
    void RestoreHasFuncAssignment();

    bool GetIsNonSimpleParameter() const
    {
        return isNonSimpleParameter;
    }

    void SetIsNonSimpleParameter(bool is)
    {
        isNonSimpleParameter = is;
    }

    bool GetIsArguments() const;

    void SetPosition(Js::PropertyId pos)
    {
        position = pos;
    }

    Js::PropertyId GetPosition()
    {
        return position;
    }

    Js::PropertyId EnsurePosition(ByteCodeGenerator* byteCodeGenerator);
    Js::PropertyId EnsurePosition(FuncInfo *funcInfo);
    Js::PropertyId EnsurePositionNoCheck(FuncInfo *funcInfo);

    void SetLocation(Js::RegSlot location)
    {
        this->location = location;
    }

    Js::RegSlot GetLocation()
    {
        return location;
    }

    Js::PropertyId GetScopeSlot() const { return scopeSlot; }
    bool HasScopeSlot() const { return scopeSlot != Js::Constants::NoProperty; }

    SymbolType GetSymbolType()
    {
        return symbolType;
    }

    void SetSymbolType(SymbolType symbolType)
    {
        this->symbolType = symbolType;
        this->hasMaybeEscapedUse = GetIsFormal();
        this->hasFuncAssignment = (symbolType == STFunction);
    }

#if DBG_DUMP
    const char16 *GetSymbolTypeName();
#endif

    const JsUtil::CharacterBuffer<WCHAR>& GetName() const
    {
        return this->name;
    }

    Js::PropertyId EnsureScopeSlot(FuncInfo *funcInfo);
    bool IsInSlot(FuncInfo *funcInfo, bool ensureSlotAlloc = false);
    bool NeedsSlotAlloc(FuncInfo *funcInfo);

    static void SaveToPropIdArray(Symbol *sym, Js::PropertyIdArray *propIds, ByteCodeGenerator *byteCodeGenerator, Js::PropertyId *pFirstSlot = nullptr);

    Symbol * GetFuncScopeVarSym() const;

    void SetPid(IdentPtr pid)
    {
        this->pid = pid;
    }
    IdentPtr GetPid() const
    {
        return pid;
    }

private:
    void SetHasMaybeEscapedUseInternal(ByteCodeGenerator * byteCodeGenerator);
    void SetHasFuncAssignmentInternal(ByteCodeGenerator * byteCodeGenerator);
};

// specialize toKey to use the name in the symbol as the key
template <>
inline SymbolName JsUtil::ValueToKey<SymbolName, Symbol *>::ToKey(Symbol * const& sym)
{
    return sym->GetName();
}

typedef JsUtil::BaseHashSet<Symbol *, ArenaAllocator, PrimeSizePolicy, SymbolName, DefaultComparer, JsUtil::HashedEntry> SymbolTable;
