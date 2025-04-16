//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
struct InlineCacheUnit
{
    InlineCacheUnit() : loadCacheId((uint)-1), loadMethodCacheId((uint)-1), storeCacheId((uint)-1) {}

    union {
        struct {
            uint loadCacheId;
            uint loadMethodCacheId;
            uint storeCacheId;
        };
        struct {
            uint cacheId;
        };
    };
};

typedef JsUtil::BaseDictionary<ParseNode*, SList<Symbol*>*, ArenaAllocator, PowerOf2SizePolicy> CapturedSymMap;
typedef JsUtil::BaseDictionary<Js::ProfileId, Js::ProfileId, ArenaAllocator, PowerOf2SizePolicy> CallSiteToCallApplyCallSiteMap;

class FuncInfo
{
private:
    struct SlotKey
    {
        Scope* scope;
        Js::PropertyId slot;
    };

    template<class TSlotKey>
    class SlotKeyComparer
    {
    public:
        static bool Equals(TSlotKey key1, TSlotKey key2)
        {
            return (key1.scope == key2.scope && key1.slot == key2.slot);
        }

        static int GetHashCode(TSlotKey key)
        {
            return ::Math::PointerCastToIntegralTruncate<int>(key.scope) | key.slot & ArenaAllocator::ObjectAlignmentMask;
        }
    };

    uint inlineCacheCount;
    uint rootObjectLoadInlineCacheCount;
    uint rootObjectLoadMethodInlineCacheCount;
    uint rootObjectStoreInlineCacheCount;
    uint isInstInlineCacheCount;
    uint referencedPropertyIdCount;
    uint NewInlineCache()
    {
        AssertMsg(this->inlineCacheCount < (uint)-2, "Inline cache index wrapped around?");
        return inlineCacheCount++;
    }
    uint NewRootObjectLoadInlineCache()
    {
        AssertMsg(this->rootObjectLoadInlineCacheCount < (uint)-2, "Inline cache index wrapped around?");
        return rootObjectLoadInlineCacheCount++;
    }
    uint NewRootObjectLoadMethodInlineCache()
    {
        AssertMsg(this->rootObjectLoadMethodInlineCacheCount < (uint)-2, "Inline cache index wrapped around?");
        return rootObjectLoadMethodInlineCacheCount++;
    }
    uint NewRootObjectStoreInlineCache()
    {
        AssertMsg(this->rootObjectStoreInlineCacheCount < (uint)-2, "Inline cache index wrapped around?");
        return rootObjectStoreInlineCacheCount++;
    }
    uint NewReferencedPropertyId()
    {
        AssertMsg(this->referencedPropertyIdCount < (uint)-2, "Referenced Property Id index wrapped around?");
        return referencedPropertyIdCount++;
    }

    FuncInfo *currentChildFunction;
    Scope *currentChildScope;
    SymbolTable *capturedSyms;
    CapturedSymMap *capturedSymMap;
    uint        nextForInLoopLevel;
    uint        maxForInLoopLevel;
public:
    static const Js::RegSlot InitialConstRegsCount = 2; // constRegsCount is set to 2 because R0 is the return register, and R1 is the root object

    ArenaAllocator *alloc;
    // set in Bind/Assign pass
    Js::RegSlot varRegsCount; // number of registers used for non-constants
    Js::RegSlot constRegsCount; // number of registers used for constants
    Js::ArgSlot inArgsCount; // number of in args (including 'this')
    Js::RegSlot outArgsMaxDepth; // max depth of out args stack
    Js::RegSlot outArgsCurrentExpr; // max number of out args accumulated in the current nested expression
    uint        innerScopeCount;
    uint        currentInnerScopeIndex;
    Js::ByteCodeLabel currentOptionalChainSkipLabel;
#if DBG
    uint32 outArgsDepth; // number of calls nested in an expression
#endif
    const char16 *name; // name of the function
    Js::RegSlot nullConstantRegister; // location, if any, of enregistered null constant
    Js::RegSlot undefinedConstantRegister; // location, if any, of enregistered undefined constant
    Js::RegSlot trueConstantRegister; // location, if any, of enregistered true constant
    Js::RegSlot falseConstantRegister; // location, if any, of enregistered false constant
    Js::RegSlot thisConstantRegister; // location, if any, of enregistered 'this' constant

private:
    Js::RegSlot envRegister; // location, if any, of the closure environment
public:
    Js::RegSlot frameObjRegister; // location, if any, of the heap-allocated local frame
    Js::RegSlot frameSlotsRegister; // location, if any, of the heap-allocated local frame
    Js::RegSlot paramSlotsRegister; // location, if any, of the heap allocated local frame for param scope
    Js::RegSlot frameDisplayRegister; // location, if any, of the display of nested frames
    Js::RegSlot funcObjRegister;
    Js::RegSlot localClosureReg;
    Js::RegSlot awaitRegister;
    Js::RegSlot yieldRegister;
    Js::RegSlot firstTmpReg;
    Js::RegSlot curTmpReg;
    int argsPlaceHolderSlotCount;   // count of place holder slots for same name args and destructuring patterns

    uint canDefer : 1;
    uint callsEval : 1;
    uint childCallsEval : 1;
    uint hasArguments : 1;
    uint hasHeapArguments : 1;
    uint isTopLevelEventHandler : 1;
    uint hasLocalInClosure : 1;
    uint hasClosureReference : 1;
    uint hasCachedScope : 1;
    uint funcExprNameReference : 1;
    uint applyEnclosesArgs : 1;
    uint escapes : 1;
    uint hasLoop : 1;
    uint hasEscapedUseNestedFunc : 1;
    uint needEnvRegister : 1;
    uint isBodyAndParamScopeMerged : 1;
#if DBG
    // FunctionBody was reused on recompile of a redeferred enclosing function.
    uint isReused:1;
#endif

    typedef JsUtil::BaseDictionary<uint, Js::RegSlot, ArenaAllocator, PrimeSizePolicy> ConstantRegisterMap;
    ConstantRegisterMap constantToRegister; // maps uint constant to register
    typedef JsUtil::BaseDictionary<IdentPtr, Js::RegSlot, ArenaAllocator> PidRegisterMap;
    PidRegisterMap stringToRegister; // maps string constant to register
    typedef JsUtil::BaseDictionary<double,Js::RegSlot, ArenaAllocator, PrimeSizePolicy> DoubleRegisterMap;
    DoubleRegisterMap doubleConstantToRegister; // maps double constant to register
    typedef JsUtil::BaseDictionary<ParseNodePtr, Js::RegSlot, ArenaAllocator> BigIntRegisterMap;
    BigIntRegisterMap bigintToRegister; // maps bigint constant to register

    typedef JsUtil::BaseDictionary<ParseNodePtr, Js::RegSlot, ArenaAllocator, PowerOf2SizePolicy> StringTemplateCallsiteRegisterMap;
    StringTemplateCallsiteRegisterMap stringTemplateCallsiteRegisterMap; // maps string template callsite constant to register

    Scope *paramScope; // top level scope for parameter default values
    Scope *bodyScope; // top level scope of the function body
    Scope *funcExprScope;
    ParseNodeFnc *root; // top-level AST for function
    Js::ParseableFunctionInfo* byteCodeFunction; // reference to generated bytecode function (could be defer parsed or actually parsed)
    SList<ParseNodeStmt*> targetStatements; // statements that are targets of jumps (break or continue)
    Js::ByteCodeLabel singleExit;
    typedef SList<InlineCacheUnit> InlineCacheList;
    typedef JsUtil::BaseDictionary<Js::PropertyId, InlineCacheList*, ArenaAllocator, PowerOf2SizePolicy> InlineCacheIdMap;
    typedef JsUtil::BaseDictionary<Js::RegSlot, InlineCacheIdMap*, ArenaAllocator, PowerOf2SizePolicy> InlineCacheMap;
    typedef JsUtil::BaseDictionary<Js::PropertyId, uint, ArenaAllocator, PowerOf2SizePolicy> RootObjectInlineCacheIdMap;
    typedef JsUtil::BaseDictionary<Js::PropertyId, uint, ArenaAllocator, PowerOf2SizePolicy> ReferencedPropertyIdMap;
    RootObjectInlineCacheIdMap * rootObjectLoadInlineCacheMap;
    RootObjectInlineCacheIdMap * rootObjectLoadMethodInlineCacheMap;
    RootObjectInlineCacheIdMap * rootObjectStoreInlineCacheMap;
    InlineCacheMap * inlineCacheMap;
    ReferencedPropertyIdMap * referencedPropertyIdToMapIndex;
    CallSiteToCallApplyCallSiteMap * callSiteToCallApplyCallSiteMap;
    SListBase<uint> valueOfStoreCacheIds;
    SListBase<uint> toStringStoreCacheIds;
    typedef JsUtil::BaseDictionary<SlotKey, Js::ProfileId, ArenaAllocator, PowerOf2SizePolicy, SlotKeyComparer> SlotProfileIdMap;
    SlotProfileIdMap slotProfileIdMap;
    Symbol *argumentsSymbol;
    Symbol *thisSymbol;
    Symbol *newTargetSymbol;
    Symbol* importMetaSymbol;
    Symbol *superSymbol;
    Symbol *superConstructorSymbol;
    JsUtil::List<Js::RegSlot, ArenaAllocator> nonUserNonTempRegistersToInitialize;

    FuncInfo(
        const char16 *name,
        ArenaAllocator *alloc,
        ByteCodeGenerator *byteCodeGenerator,
        Scope *paramScope,
        Scope *bodyScope,
        ParseNodeFnc *pnode,
        Js::ParseableFunctionInfo* byteCodeFunction);
    uint NewIsInstInlineCache() { return isInstInlineCacheCount++; }
    uint GetInlineCacheCount() const { return this->inlineCacheCount; }
    uint GetRootObjectLoadInlineCacheCount() const { return this->rootObjectLoadInlineCacheCount; }
    uint GetRootObjectLoadMethodInlineCacheCount() const { return this->rootObjectLoadMethodInlineCacheCount; }
    uint GetRootObjectStoreInlineCacheCount() const { return this->rootObjectStoreInlineCacheCount; }
    uint GetIsInstInlineCacheCount() const { return this->isInstInlineCacheCount; }
    uint GetReferencedPropertyIdCount() const { return this->referencedPropertyIdCount; }
    void SetFirstTmpReg(Js::RegSlot tmpReg)
    {
        Assert(this->firstTmpReg == Js::Constants::NoRegister);
        Assert(this->curTmpReg == Js::Constants::NoRegister);
        this->firstTmpReg = tmpReg;
        this->curTmpReg = tmpReg;
    }

    bool IsTmpReg(Js::RegSlot tmpReg)
    {
        Assert(this->firstTmpReg != Js::Constants::NoRegister);
        return !RegIsConst(tmpReg) && tmpReg >= firstTmpReg;
    }

    bool RegIsConst(Js::RegSlot reg)
    {
        // varRegsCount includes the tmp regs, so if reg number is larger than that,
        // then it must be in the negative range for const.
        return reg >= varRegsCount;
    }

    Js::RegSlot NextVarRegister()
    {
        AssertMsg(this->firstTmpReg == Js::Constants::NoRegister, "Shouldn't assign var register after we start allocating temp reg");
        Js::RegSlot reg = varRegsCount;
        UInt32Math::Inc(varRegsCount);
        return REGSLOT_TO_VARREG(reg);
    }

    Js::RegSlot NextConstRegister()
    {
        AssertMsg(this->firstTmpReg == Js::Constants::NoRegister, "Shouldn't assign var register after we start allocating temp reg");
        Js::RegSlot reg = constRegsCount;
        UInt32Math::Inc(constRegsCount);
        return REGSLOT_TO_CONSTREG(reg);
    }

    Js::RegSlot RegCount() const
    {
        return constRegsCount + varRegsCount;
    }

    uint InnerScopeCount() const { return innerScopeCount; }
    uint CurrentInnerScopeIndex() const { return currentInnerScopeIndex; }
    uint AcquireInnerScopeIndex();
    void ReleaseInnerScopeIndex();

    bool GetApplyEnclosesArgs() const { return applyEnclosesArgs; }
    void SetApplyEnclosesArgs(bool b) { applyEnclosesArgs=b; }

    bool IsGlobalFunction() const;

    // Fake global ->
    //    1) new Function code's global code
    //    2) global code generated from the reparsing deferred parse function

    bool IsFakeGlobalFunction(uint32 flags) const 
    {
        return IsGlobalFunction() && !(flags & fscrGlobalCode);
    }

    Scope *GetBodyScope() const 
    {
        return bodyScope;
    }

    void SetBodyScope(Scope * scope)
    {
        bodyScope = scope;
    }

    Scope *GetParamScope() const 
    {
        return paramScope;
    }

    void SetParamScope(Scope * scope)
    {
        paramScope = scope;
    }

    Scope *GetTopLevelScope() const 
    {
        // Top level scope will be the same for knopProg and knopFncDecl.
        return paramScope;
    }

    Scope* GetFuncExprScope() const 
    {
        return funcExprScope;
    }

    void SetFuncExprScope(Scope* funcExprScope) {
        this->funcExprScope = funcExprScope;
    }

    Symbol *GetArgumentsSymbol() const
    {
        return argumentsSymbol;
    }

    void SetArgumentsSymbol(Symbol *sym)
    {
        Assert(argumentsSymbol == nullptr || argumentsSymbol == sym);
        argumentsSymbol = sym;
    }

    void SetThisSymbol(Symbol *sym)
    {
        Assert(thisSymbol == nullptr || thisSymbol == sym);
        thisSymbol = sym;
    }

    Symbol* GetThisSymbol() const
    {
        return thisSymbol;
    }

    void SetNewTargetSymbol(Symbol *sym)
    {
        Assert(newTargetSymbol == nullptr || newTargetSymbol == sym);
        newTargetSymbol = sym;
    }

    Symbol* GetNewTargetSymbol() const
    {
        return newTargetSymbol;
    }

    void SetImportMetaSymbol(Symbol* sym)
    {
        Assert(importMetaSymbol == nullptr || importMetaSymbol == sym);
        importMetaSymbol = sym;
    }

    Symbol* GetImportMetaSymbol() const
    {
        return importMetaSymbol;
    }

    void SetSuperSymbol(Symbol *sym)
    {
        Assert(superSymbol == nullptr || superSymbol == sym);
        superSymbol = sym;
    }

    Symbol* GetSuperSymbol() const
    {
        return superSymbol;
    }

    void SetSuperConstructorSymbol(Symbol *sym)
    {
        Assert(superConstructorSymbol == nullptr || superConstructorSymbol == sym);
        superConstructorSymbol = sym;
    }

    Symbol* GetSuperConstructorSymbol() const
    {
        return superConstructorSymbol;
    }

    bool GetCallsEval() const {
        return callsEval;
    }

    void SetCallsEval(bool does) {
        callsEval = does;
    }

    bool GetHasArguments() const {
        return hasArguments;
    }

    void SetHasArguments(bool has) {
        hasArguments = has;
    }

    bool GetHasHeapArguments() const
    {
        return hasHeapArguments;
    }

    void SetHasHeapArguments(bool has, bool optArgInBackend = false)
    {
        hasHeapArguments = has;
        byteCodeFunction->SetDoBackendArgumentsOptimization(optArgInBackend);
    }

    bool GetIsTopLevelEventHandler() const {
        return isTopLevelEventHandler;
    }

    void SetIsTopLevelEventHandler(bool is) {
        isTopLevelEventHandler = is;
    }

    bool GetChildCallsEval() const {
        return childCallsEval;
    }

    void SetChildCallsEval(bool does) {
        childCallsEval = does;
    }

    bool GetHasLocalInClosure() const {
        return hasLocalInClosure;
    }

    void SetHasLocalInClosure(bool has) {
        hasLocalInClosure = has;
    }

    bool GetHasClosureReference() const {
        return hasClosureReference;
    }

    void SetHasCachedScope(bool has) {
        hasCachedScope = has;
    }

    bool GetHasCachedScope() const {
        return hasCachedScope;
    }

    void SetFuncExprNameReference(bool has) {
        funcExprNameReference = has;
    }

    bool GetFuncExprNameReference() const {
        return funcExprNameReference;
    }

    void SetHasClosureReference(bool has) {
        hasClosureReference = has;
    }

    bool GetIsStrictMode() const {
        return this->byteCodeFunction->GetIsStrictMode();
    }

    bool Escapes() const {
        return escapes;
    }

    void SetEscapes(bool does) {
        escapes = does;
    }

    bool HasMaybeEscapedNestedFunc() const {
        return hasEscapedUseNestedFunc;
    }

    void SetHasMaybeEscapedNestedFunc(DebugOnly(char16 const * reason));

    bool IsDeferred() const;

    bool IsRestored()
    {
        // FuncInfo are from RestoredScopeInfo
        return root == nullptr;
    }

    Js::FunctionBody* GetParsedFunctionBody() const
    {
        AssertMsg(this->byteCodeFunction->IsFunctionParsed(), "Function must be parsed in order to call this method");
        Assert(!IsDeferred() || this->byteCodeFunction->GetFunctionBody()->GetByteCode() != nullptr);

        return this->byteCodeFunction->GetFunctionBody();
    }

    bool IsBodyAndParamScopeMerged() const {
        return isBodyAndParamScopeMerged;
    }

    void ResetBodyAndParamScopeMerged() {
        isBodyAndParamScopeMerged = false;
    }

    BOOL HasSuperReference() const;
    BOOL HasDirectSuper() const;
    BOOL IsClassMember() const;
    BOOL IsLambda() const;
    BOOL IsClassConstructor() const;
    BOOL IsBaseClassConstructor() const;
    BOOL IsDerivedClassConstructor() const;
    bool IsAsyncGenerator() const;

    void RemoveTargetStmt(ParseNodeStmt* pnodeStmt) {
        targetStatements.Remove(pnodeStmt);
    }

    void AddTargetStmt(ParseNodeStmt *pnodeStmt) {
        targetStatements.Prepend(pnodeStmt);
    }

    Js::RegSlot LookupDouble(double d) {
        return doubleConstantToRegister.Lookup(d,Js::Constants::NoRegister);
    }

    bool TryGetDoubleLoc(double d, Js::RegSlot *loc) {
        Js::RegSlot ret=LookupDouble(d);
        *loc=ret;
        return(ret!=Js::Constants::NoRegister);
    }

    void AddDoubleConstant(double d, Js::RegSlot location) {
        doubleConstantToRegister.Item(d,location);
    }

    bool NeedEnvRegister() const { return this->needEnvRegister; }
    void SetNeedEnvRegister() { this->needEnvRegister = true; };
    Js::RegSlot GetEnvRegister() const
    {
        Assert(this->envRegister != Js::Constants::NoRegister);
        return this->envRegister;
    }
    Js::RegSlot AssignEnvRegister(bool constReg)
    {
        Assert(needEnvRegister);
        Assert(this->envRegister == Js::Constants::NoRegister);
        Js::RegSlot reg = constReg? NextConstRegister() : NextVarRegister();
        this->envRegister = reg;
        return reg;
    }

    Js::RegSlot AssignThisConstRegister()
    {
        if (this->thisConstantRegister == Js::Constants::NoRegister)
        {
            this->thisConstantRegister = this->NextVarRegister();
        }
        return this->thisConstantRegister;
    }

    Js::RegSlot AssignNullConstRegister()
    {
        if (this->nullConstantRegister == Js::Constants::NoRegister)
        {
            this->nullConstantRegister = NextConstRegister();
        }
        return this->nullConstantRegister;
    }

    Js::RegSlot AssignUndefinedConstRegister()
    {
        if (this->undefinedConstantRegister == Js::Constants::NoRegister)
        {
            this->undefinedConstantRegister = NextConstRegister();
        }
        return this->undefinedConstantRegister;
    }

    Js::RegSlot AssignTrueConstRegister()
    {
        if (this->trueConstantRegister == Js::Constants::NoRegister)
        {
            this->trueConstantRegister = NextConstRegister();
        }
        return this->trueConstantRegister;
    }

    Js::RegSlot AssignFalseConstRegister()
    {
        if (this->falseConstantRegister == Js::Constants::NoRegister)
        {
            this->falseConstantRegister = NextConstRegister();
        }
        return this->falseConstantRegister;
    }

    Js::RegSlot AssignAwaitRegister()
    {
        AssertMsg(this->awaitRegister == Js::Constants::NoRegister, "await register should only be assigned once by FinalizeRegisters()");
        this->awaitRegister = NextVarRegister();
        return this->awaitRegister;
    }

    Js::RegSlot AssignYieldRegister()
    {
        AssertMsg(this->yieldRegister == Js::Constants::NoRegister, "yield register should only be assigned once by FinalizeRegisters()");
        this->yieldRegister = NextVarRegister();
        return this->yieldRegister;
    }

    Js::RegSlot GetLocalScopeSlotsReg()
    {
        return this->localClosureReg;
    }

    Js::RegSlot GetLocalFrameDisplayReg()
    {
        return this->localClosureReg + 1;
    }

    Js::RegSlot InnerScopeToRegSlot(Scope *scope) const;
    Js::RegSlot FirstInnerScopeReg() const;
    void SetFirstInnerScopeReg(Js::RegSlot reg);

    void StartRecordingOutArgs(unsigned int argCount)
    {
#if DBG
        outArgsDepth++;
#endif
        // We should have checked for argCount overflow already
        Assert(argCount == (Js::ArgSlot)argCount);

        // Add one for the space to save the m_outParams pointer in InterpreterStackFrame::PushOut
        unsigned int outArgsCount = argCount + 1;
        outArgsCurrentExpr += (Js::ArgSlot)outArgsCount;

        // Check for overflow
        if ((Js::ArgSlot)outArgsCount != outArgsCount || outArgsCurrentExpr < outArgsCount)
        {
            Js::Throw::OutOfMemory();
        }
        outArgsMaxDepth = max(outArgsMaxDepth, outArgsCurrentExpr);
    }

    void EndRecordingOutArgs(Js::ArgSlot argCount)
    {
        AssertMsg(outArgsDepth > 0, "mismatched Start and End");
        Assert(outArgsCurrentExpr >= argCount);
#if DBG
        outArgsDepth--;
#endif
        // Add one to pop the space to save the m_outParams pointer
        outArgsCurrentExpr -= (argCount + 1);

        Assert(outArgsDepth != 0 || outArgsCurrentExpr == 0);
    }

    uint GetMaxForInLoopLevel() const { return this->maxForInLoopLevel;  }
    uint AcquireForInLoopLevel()
    {
        uint forInLoopLevel = this->nextForInLoopLevel++;
        this->maxForInLoopLevel = max(this->maxForInLoopLevel, this->nextForInLoopLevel);
        return forInLoopLevel;
    }

    void ReleaseForInLoopLevel(uint forInLoopLevel)
    {
        Assert(this->nextForInLoopLevel == forInLoopLevel + 1);
        this->nextForInLoopLevel = forInLoopLevel;
    }

    Js::RegSlot AcquireLoc(ParseNode *pnode);
    Js::RegSlot AcquireTmpRegister();
    void ReleaseLoc(ParseNode *pnode);
    void ReleaseReference(ParseNode *pnode);
    void ReleaseLoad(ParseNode *pnode);
    void ReleaseTmpRegister(Js::RegSlot tmpReg);

    uint FindOrAddReferencedPropertyId(Js::PropertyId propertyId);

    uint FindOrAddRootObjectInlineCacheId(Js::PropertyId propertyId, bool isLoadMethod, bool isStore);

    uint FindOrAddInlineCacheId(Js::RegSlot instanceSlot, Js::PropertyId propertySlot, bool isLoadMethod, bool isStore)
    {
        Assert(instanceSlot != Js::Constants::NoRegister);
        Assert(propertySlot != Js::Constants::NoProperty);
        Assert(!isLoadMethod || !isStore);

        InlineCacheIdMap *properties;
        uint cacheId;

        if (isStore)
        {
            // ... = foo.toString;
            // foo.toString = ...;

            // We need a new cache here to ensure SetProperty() is called, which will set the side-effect bit
            // on the scriptContext.
            switch (propertySlot)
            {
            case Js::PropertyIds::valueOf:
                cacheId = this->NewInlineCache();
                valueOfStoreCacheIds.Prepend(alloc, cacheId);
                return cacheId;

            case Js::PropertyIds::toString:
                cacheId = this->NewInlineCache();
                toStringStoreCacheIds.Prepend(alloc, cacheId);
                return cacheId;
            };
        }

        if (!inlineCacheMap->TryGetValue(instanceSlot, &properties))
        {
            properties = Anew(alloc, InlineCacheIdMap, alloc, 17);
            inlineCacheMap->Add(instanceSlot, properties);
        }

        InlineCacheList* cacheList;
        if (!properties->TryGetValue(propertySlot, &cacheList))
        {
            cacheList = Anew(alloc, InlineCacheList, alloc);
            properties->Add(propertySlot, cacheList);
        }

        // If we share inline caches we should never have more than one entry in the list.
        Assert(!Js::FunctionBody::ShouldShareInlineCaches() || cacheList->Count() <= 1);

        InlineCacheUnit cacheIdUnit;

        if (Js::FunctionBody::ShouldShareInlineCaches() && !cacheList->Empty())
        {
            cacheIdUnit = cacheList->Head();
            if (isLoadMethod)
            {
                if (cacheIdUnit.loadMethodCacheId == (uint)-1)
                {
                    cacheIdUnit.loadMethodCacheId = this->NewInlineCache();
                }
                cacheId = cacheIdUnit.loadMethodCacheId;
            }
            else if (isStore)
            {
                if (cacheIdUnit.storeCacheId == (uint)-1)
                {
                    cacheIdUnit.storeCacheId = this->NewInlineCache();
                }
                cacheId = cacheIdUnit.storeCacheId;
            }
            else
            {
                if (cacheIdUnit.loadCacheId == (uint)-1)
                {
                    cacheIdUnit.loadCacheId = this->NewInlineCache();
                }
                cacheId = cacheIdUnit.loadCacheId;
            }
            cacheList->Head() = cacheIdUnit;
        }
        else
        {
            cacheId = this->NewInlineCache();
            if (Js::FunctionBody::ShouldShareInlineCaches())
            {
                if (isLoadMethod)
                {
                    cacheIdUnit.loadCacheId = (uint)-1;
                    cacheIdUnit.loadMethodCacheId = cacheId;
                    cacheIdUnit.storeCacheId = (uint)-1;
                }
                else if (isStore)
                {
                    cacheIdUnit.loadCacheId = (uint)-1;
                    cacheIdUnit.loadMethodCacheId = (uint)-1;
                    cacheIdUnit.storeCacheId = cacheId;
                }
                else
                {
                    cacheIdUnit.loadCacheId = cacheId;
                    cacheIdUnit.loadMethodCacheId = (uint)-1;
                    cacheIdUnit.storeCacheId = (uint)-1;
                }
            }
            else
            {
                cacheIdUnit.cacheId = cacheId;
            }
            cacheList->Prepend(cacheIdUnit);
        }

        return cacheId;
    }

    Js::ProfileId FindOrAddSlotProfileId(Scope* scope, Js::PropertyId propertyId)
    {
        SlotKey key;

        key.scope = scope;
        key.slot = propertyId;
        Js::ProfileId profileId = Js::Constants::NoProfileId;

        if (!this->slotProfileIdMap.TryGetValue(key, &profileId))
        {
            Assert(this->byteCodeFunction->IsFunctionParsed());
            if (this->byteCodeFunction->GetFunctionBody()->AllocProfiledSlotId(&profileId))
            {
                this->slotProfileIdMap.Add(key, profileId);
            }
        }

        return profileId;
    }

    Scope * GetGlobalBlockScope() const;
    Scope * GetGlobalEvalBlockScope() const;

    FuncInfo *GetCurrentChildFunction() const
    {
        return this->currentChildFunction;
    }

    void SetCurrentChildFunction(FuncInfo *funcInfo)
    {
        this->currentChildFunction = funcInfo;
    }

    Scope *GetCurrentChildScope() const
    {
        return this->currentChildScope;
    }

    void SetCurrentChildScope(Scope *scope)
    {
        this->currentChildScope = scope;
    }

    SymbolTable *GetCapturedSyms() const { return capturedSyms; }

    void OnStartVisitFunction(ParseNodeFnc *pnodeFnc);
    void OnEndVisitFunction(ParseNodeFnc *pnodeFnc);
    void OnStartVisitScope(Scope *scope, bool *pisMergedScope);
    void OnEndVisitScope(Scope *scope, bool isMergedScope = false);
    void AddCapturedSym(Symbol *sym);
    CapturedSymMap *EnsureCapturedSymMap();
    CallSiteToCallApplyCallSiteMap * EnsureCallSiteToCallApplyCallSiteMap();

#if DBG_DUMP
    void Dump();
#endif
};
