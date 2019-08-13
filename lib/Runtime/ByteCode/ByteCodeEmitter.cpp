//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"
#include "FormalsUtil.h"
#include "Language/AsmJs.h"
#include "ConfigFlagsList.h"

void EmitReference(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);
void EmitAssignment(ParseNode *asgnNode, ParseNode *lhs, Js::RegSlot rhsLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);
void EmitLoad(ParseNode *rhs, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);
void EmitCall(ParseNodeCall* pnodeCall, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, BOOL fReturnValue, BOOL fEvaluateComponents, Js::RegSlot overrideThisLocation = Js::Constants::NoRegister, Js::RegSlot newTargetLocation = Js::Constants::NoRegister);
void EmitYield(Js::RegSlot inputLocation, Js::RegSlot resultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, Js::RegSlot yieldStarIterator = Js::Constants::NoRegister);

void EmitUseBeforeDeclaration(Symbol *sym, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);
void EmitUseBeforeDeclarationRuntimeError(ByteCodeGenerator *byteCodeGenerator, Js::RegSlot location);
void VisitClearTmpRegs(ParseNode * pnode, ByteCodeGenerator * byteCodeGenerator, FuncInfo * funcInfo);

bool CallTargetIsArray(ParseNode *pnode)
{
    return pnode->nop == knopName && pnode->AsParseNodeName()->PropertyIdFromNameNode() == Js::PropertyIds::Array;
}

#define STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode) \
if ((isTopLevel)) \
{ \
    byteCodeGenerator->StartStatement(pnode); \
}

#define ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode) \
if ((isTopLevel)) \
{ \
    byteCodeGenerator->EndStatement(pnode); \
}

BOOL MayHaveSideEffectOnNode(ParseNode *pnode, ParseNode *pnodeSE)
{
    // Try to determine whether pnodeSE may kill the named var represented by pnode.

    if (pnode->nop == knopComputedName)
    {
        pnode = pnode->AsParseNodeUni()->pnode1;
    }

    if (pnode->nop != knopName)
    {
        // Only investigating named vars here.
        return false;
    }

    uint fnop = ParseNode::Grfnop(pnodeSE->nop);
    if (fnop & fnopLeaf)
    {
        // pnodeSE is a leaf and can't kill anything.
        return false;
    }

    if (fnop & fnopAsg)
    {
        // pnodeSE is an assignment (=, ++, +=, etc.)
        // Trying to examine the LHS of pnodeSE caused small perf regressions,
        // maybe because of code layout or some other subtle effect.
        return true;
    }

    if (fnop & fnopUni)
    {
        // pnodeSE is a unary op, so recurse to the source (if present - e.g., [] may have no opnd).
        if (pnodeSE->nop == knopTempRef)
        {
            return false;
        }
        else
        {
            return pnodeSE->AsParseNodeUni()->pnode1 && MayHaveSideEffectOnNode(pnode, pnodeSE->AsParseNodeUni()->pnode1);
        }
    }
    else if (fnop & fnopBin)
    {
        // pnodeSE is a binary (or ternary) op, so recurse to the sources (if present).
        return MayHaveSideEffectOnNode(pnode, pnodeSE->AsParseNodeBin()->pnode1) ||
            (pnodeSE->AsParseNodeBin()->pnode2 && MayHaveSideEffectOnNode(pnode, pnodeSE->AsParseNodeBin()->pnode2));
    }
    else if (pnodeSE->nop == knopQmark)
    {
        ParseNodeTri * pnodeTriSE = pnodeSE->AsParseNodeTri();
        return MayHaveSideEffectOnNode(pnode, pnodeTriSE->pnode1) ||
            MayHaveSideEffectOnNode(pnode, pnodeTriSE->pnode2) ||
            MayHaveSideEffectOnNode(pnode, pnodeTriSE->pnode3);
    }
    else if (pnodeSE->nop == knopCall || pnodeSE->nop == knopNew)
    {
        return MayHaveSideEffectOnNode(pnode, pnodeSE->AsParseNodeCall()->pnodeTarget) ||
            (pnodeSE->AsParseNodeCall()->pnodeArgs && MayHaveSideEffectOnNode(pnode, pnodeSE->AsParseNodeCall()->pnodeArgs));
    }
    else if (pnodeSE->nop == knopList)
    {
        return true;
    }

    return false;
}

bool IsCallOfConstants(ParseNode *pnode);
bool BlockHasOwnScope(ParseNodeBlock * pnodeBlock, ByteCodeGenerator *byteCodeGenerator);
bool CreateNativeArrays(ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);

bool IsArguments(ParseNode *pnode)
{
    for (;;)
    {
        switch (pnode->nop)
        {
        case knopName:
            return pnode->AsParseNodeName()->sym && pnode->AsParseNodeName()->sym->IsArguments();

        case knopCall:
        case knopNew:
            if (IsArguments(pnode->AsParseNodeCall()->pnodeTarget))
            {
                return true;
            }

            if (pnode->AsParseNodeCall()->pnodeArgs)
            {
                ParseNode *pnodeArg = pnode->AsParseNodeCall()->pnodeArgs;
                while (pnodeArg->nop == knopList)
                {
                    if (IsArguments(pnodeArg->AsParseNodeBin()->pnode1))
                        return true;

                    pnodeArg = pnodeArg->AsParseNodeBin()->pnode2;
                }

                pnode = pnodeArg;
                break;
            }

            return false;

        case knopArray:
            if (pnode->AsParseNodeArrLit()->arrayOfNumbers || pnode->AsParseNodeArrLit()->count == 0)
            {
                return false;
            }

            pnode = pnode->AsParseNodeUni()->pnode1;
            break;

        case knopQmark:
            if (IsArguments(pnode->AsParseNodeTri()->pnode1) || IsArguments(pnode->AsParseNodeTri()->pnode2))
            {
                return true;
            }

            pnode = pnode->AsParseNodeTri()->pnode3;
            break;

            //
            // Cases where we don't check for "arguments" yet.
            // Assume that they might have it. Disable the optimization is such scenarios
            //
        case knopList:
        case knopObject:
        case knopVarDecl:
        case knopConstDecl:
        case knopLetDecl:
        case knopFncDecl:
        case knopClassDecl:
        case knopFor:
        case knopIf:
        case knopDoWhile:
        case knopWhile:
        case knopForIn:
        case knopForOf:
        case knopReturn:
        case knopBlock:
        case knopBreak:
        case knopContinue:
        case knopTypeof:
        case knopThrow:
        case knopWith:
        case knopFinally:
        case knopTry:
        case knopTryCatch:
        case knopTryFinally:
        case knopArrayPattern:
        case knopObjectPattern:
        case knopParamPattern:
            return true;

        default:
        {
            uint flags = ParseNode::Grfnop(pnode->nop);
            if (flags&fnopUni)
            {
                ParseNodeUni * pnodeUni = pnode->AsParseNodeUni();
                Assert(pnodeUni->pnode1);

                pnode = pnodeUni->pnode1;
                break;
            }
            else if (flags&fnopBin)
            {
                ParseNodeBin * pnodeBin = pnode->AsParseNodeBin();
                Assert(pnodeBin->pnode1 && pnodeBin->pnode2);

                if (IsArguments(pnodeBin->pnode1))
                {
                    return true;
                }

                pnode = pnodeBin->pnode2;
                break;
            }

            return false;
        }

        }
    }
}

bool ApplyEnclosesArgs(ParseNode* fncDecl, ByteCodeGenerator* byteCodeGenerator);
void Emit(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, BOOL fReturnValue, bool isConstructorCall = false, Js::RegSlot bindingNameLocation = Js::Constants::NoRegister, bool isTopLevel = false);
void EmitBinaryOpnds(ParseNode* pnode1, ParseNode* pnode2, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, Js::RegSlot computedPropertyLocation = Js::Constants::NoRegister);
bool IsExpressionStatement(ParseNode* stmt, const Js::ScriptContext *const scriptContext);
void EmitInvoke(Js::RegSlot location, Js::RegSlot callObjLocation, Js::PropertyId propertyId, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);
void EmitInvoke(Js::RegSlot location, Js::RegSlot callObjLocation, Js::PropertyId propertyId, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, Js::RegSlot arg1Location);

static const Js::OpCode nopToOp[knopLim] =
{
#define OP(x) Br##x##_A
#define PTNODE(nop,sn,pc,nk,grfnop,json) Js::OpCode::pc,
#include "ptlist.h"
};
static const Js::OpCode nopToCMOp[knopLim] =
{
#define OP(x) Cm##x##_A
#define PTNODE(nop,sn,pc,nk,grfnop,json) Js::OpCode::pc,
#include "ptlist.h"
};

Js::OpCode ByteCodeGenerator::ToChkUndeclOp(Js::OpCode op) const
{
    switch (op)
    {
    case Js::OpCode::StLocalSlot:
        return Js::OpCode::StLocalSlotChkUndecl;

    case Js::OpCode::StParamSlot:
        return Js::OpCode::StParamSlotChkUndecl;

    case Js::OpCode::StInnerSlot:
        return Js::OpCode::StInnerSlotChkUndecl;

    case Js::OpCode::StEnvSlot:
        return Js::OpCode::StEnvSlotChkUndecl;

    case Js::OpCode::StObjSlot:
        return Js::OpCode::StObjSlotChkUndecl;

    case Js::OpCode::StLocalObjSlot:
        return Js::OpCode::StLocalObjSlotChkUndecl;

    case Js::OpCode::StParamObjSlot:
        return Js::OpCode::StParamObjSlotChkUndecl;

    case Js::OpCode::StInnerObjSlot:
        return Js::OpCode::StInnerObjSlotChkUndecl;

    case Js::OpCode::StEnvObjSlot:
        return Js::OpCode::StEnvObjSlotChkUndecl;

    default:
        AssertMsg(false, "Unknown opcode for chk undecl mapping");
        return Js::OpCode::InvalidOpCode;
    }
}

// Tracks a register slot let/const property for the passed in debugger block/catch scope.
// debuggerScope         - The scope to add the variable to.
// symbol                - The symbol that represents the register property.
// funcInfo              - The function info used to store the property into the tracked debugger register slot list.
// flags                 - The flags to assign to the property.
// isFunctionDeclaration - Whether or not the register is a function declaration, which requires that its byte code offset be updated immediately.
void ByteCodeGenerator::TrackRegisterPropertyForDebugger(
    Js::DebuggerScope *debuggerScope,
    Symbol *symbol,
    FuncInfo *funcInfo,
    Js::DebuggerScopePropertyFlags flags /*= Js::DebuggerScopePropertyFlags_None*/,
    bool isFunctionDeclaration /*= false*/)
{
    Assert(debuggerScope);
    Assert(symbol);
    Assert(funcInfo);

    Js::RegSlot location = symbol->GetLocation();

    Js::DebuggerScope *correctDebuggerScope = debuggerScope;
    if (debuggerScope->scopeType != Js::DiagExtraScopesType::DiagBlockScopeDirect && debuggerScope->scopeType != Js::DiagExtraScopesType::DiagCatchScopeDirect)
    {
        // We have to get the appropriate scope and add property over there.
        // Make sure the scope is created whether we're in debug mode or not, because we
        // need the empty scopes present during reparsing for debug mode.
        correctDebuggerScope = debuggerScope->GetSiblingScope(location, Writer()->GetFunctionWrite());
    }

    if (this->ShouldTrackDebuggerMetadata() && !symbol->GetIsTrackedForDebugger())
    {
        // Only track the property if we're in debug mode since it's only needed by the debugger.
        Js::PropertyId propertyId = symbol->EnsurePosition(this);

        this->Writer()->AddPropertyToDebuggerScope(
            correctDebuggerScope,
            location,
            propertyId,
            /*shouldConsumeRegister*/ true,
            flags,
            isFunctionDeclaration);

        Js::FunctionBody *byteCodeFunction = funcInfo->GetParsedFunctionBody();
        byteCodeFunction->InsertSymbolToRegSlotList(location, propertyId, funcInfo->varRegsCount);

        symbol->SetIsTrackedForDebugger(true);
    }
}

void ByteCodeGenerator::TrackActivationObjectPropertyForDebugger(
    Js::DebuggerScope *debuggerScope,
    Symbol *symbol,
    Js::DebuggerScopePropertyFlags flags /*= Js::DebuggerScopePropertyFlags_None*/,
    bool isFunctionDeclaration /*= false*/)
{
    Assert(debuggerScope);
    Assert(symbol);

    // Only need to track activation object properties in debug mode.
    if (ShouldTrackDebuggerMetadata() && !symbol->GetIsTrackedForDebugger())
    {
        Js::RegSlot location = symbol->GetLocation();
        Js::PropertyId propertyId = symbol->EnsurePosition(this);

        this->Writer()->AddPropertyToDebuggerScope(
            debuggerScope,
            location,
            propertyId,
            /*shouldConsumeRegister*/ false,
            flags,
            isFunctionDeclaration);

        symbol->SetIsTrackedForDebugger(true);
    }
}

void ByteCodeGenerator::TrackSlotArrayPropertyForDebugger(
    Js::DebuggerScope *debuggerScope,
    Symbol* symbol,
    Js::PropertyId propertyId,
    Js::DebuggerScopePropertyFlags flags /*= Js::DebuggerScopePropertyFlags_None*/,
    bool isFunctionDeclaration /*= false*/)
{
    // Note: Slot array properties are tracked even in non-debug mode in order to support slot array serialization
    // of let/const variables between non-debug and debug mode (for example, when a slot array var escapes and is retrieved
    // after a debugger attach or for WWA apps).  They are also needed for heap enumeration.
    Assert(debuggerScope);
    Assert(symbol);

    if (!symbol->GetIsTrackedForDebugger())
    {
        Js::RegSlot location = symbol->GetScopeSlot();
        Assert(location != Js::Constants::NoRegister);
        Assert(propertyId != Js::Constants::NoProperty);

        this->Writer()->AddPropertyToDebuggerScope(
            debuggerScope,
            location,
            propertyId,
            /*shouldConsumeRegister*/ false,
            flags,
            isFunctionDeclaration);

        symbol->SetIsTrackedForDebugger(true);
    }
}

// Tracks a function declaration inside a block scope for the debugger metadata's current scope (let binding).
void ByteCodeGenerator::TrackFunctionDeclarationPropertyForDebugger(Symbol *functionDeclarationSymbol, FuncInfo *funcInfoParent)
{
    Assert(functionDeclarationSymbol);
    Assert(funcInfoParent);
    AssertMsg(functionDeclarationSymbol->GetIsBlockVar(), "We should only track inner function let bindings for the debugger.");

    // Note: we don't have to check symbol->GetIsTrackedForDebugger, as we are not doing actual work here,
    //       which is done in other Track* functions that we call.

    if (functionDeclarationSymbol->IsInSlot(this, funcInfoParent))
    {
        if (functionDeclarationSymbol->GetScope()->GetIsObject())
        {
            this->TrackActivationObjectPropertyForDebugger(
                this->Writer()->GetCurrentDebuggerScope(),
                functionDeclarationSymbol,
                Js::DebuggerScopePropertyFlags_None,
                true /*isFunctionDeclaration*/);
        }
        else
        {
            // Make sure the property has a slot. This will bump up the size of the slot array if necessary.
            // Note that slot array inner function bindings are tracked even in non-debug mode in order
            // to keep the lifetime of the closure binding that could escape around for heap enumeration.
            functionDeclarationSymbol->EnsureScopeSlot(this, funcInfoParent);
            functionDeclarationSymbol->EnsurePosition(this);
            this->TrackSlotArrayPropertyForDebugger(
                this->Writer()->GetCurrentDebuggerScope(),
                functionDeclarationSymbol,
                functionDeclarationSymbol->GetPosition(),
                Js::DebuggerScopePropertyFlags_None,
                true /*isFunctionDeclaration*/);
        }
    }
    else
    {
        this->TrackRegisterPropertyForDebugger(
            this->Writer()->GetCurrentDebuggerScope(),
            functionDeclarationSymbol,
            funcInfoParent,
            Js::DebuggerScopePropertyFlags_None,
            true /*isFunctionDeclaration*/);
    }
}

// Updates the byte code offset of the property with the passed in location and ID.
// Used to track let/const variables that are in the dead zone debugger side.
// location                 - The activation object, scope slot index, or register location for the property.
// propertyId               - The ID of the property to update.
// shouldConsumeRegister    - Whether or not the a register should be consumed (used for reg slot locations).
void ByteCodeGenerator::UpdateDebuggerPropertyInitializationOffset(Js::RegSlot location, Js::PropertyId propertyId, bool shouldConsumeRegister)
{
    Assert(this->Writer());
    Js::DebuggerScope* currentDebuggerScope = this->Writer()->GetCurrentDebuggerScope();
    Assert(currentDebuggerScope);
    if (currentDebuggerScope != nullptr)
    {
        this->Writer()->UpdateDebuggerPropertyInitializationOffset(
            currentDebuggerScope,
            location,
            propertyId,
            shouldConsumeRegister);
    }
}

void ByteCodeGenerator::LoadHeapArguments(FuncInfo *funcInfo)
{
    if (funcInfo->GetHasCachedScope())
    {
        this->LoadCachedHeapArguments(funcInfo);
    }
    else
    {
        this->LoadUncachedHeapArguments(funcInfo);
    }
}

void GetFormalArgsArray(ByteCodeGenerator *byteCodeGenerator, FuncInfo * funcInfo, Js::PropertyIdArray *propIds)
{
    Assert(funcInfo);
    Assert(propIds);
    Assert(byteCodeGenerator);

    bool hadDuplicates = false;
    Js::ArgSlot i = 0;

    auto processArg = [&](ParseNode *pnode)
    {
        if (pnode->IsVarLetOrConst())
        {
            Assert(i < propIds->count);
            Symbol *sym = pnode->AsParseNodeVar()->sym;
            Assert(sym);
            Js::PropertyId symPos = sym->EnsurePosition(byteCodeGenerator);

            //
            // Check if the function has any same name parameters
            // For the same name param, only the last one will be passed the correct propertyid
            // For remaining dup param names, pass Constants::NoProperty
            //
            for (Js::ArgSlot j = 0; j < i; j++)
            {
                if (propIds->elements[j] == symPos)
                {
                    // Found a dup parameter name
                    propIds->elements[j] = Js::Constants::NoProperty;
                    hadDuplicates = true;
                    break;
                }
            }
            propIds->elements[i] = symPos;
        }
        else
        {
            propIds->elements[i] = Js::Constants::NoProperty;
        }
        ++i;
    };
    MapFormals(funcInfo->root, processArg);

    propIds->hadDuplicates = hadDuplicates;
}

void ByteCodeGenerator::LoadUncachedHeapArguments(FuncInfo *funcInfo)
{
    Assert(funcInfo->GetHasHeapArguments());

    Scope *scope = funcInfo->GetBodyScope();
    Assert(scope);
    Symbol *argSym = funcInfo->GetArgumentsSymbol();
    Assert(argSym && argSym->IsArguments());
    Js::RegSlot argumentsLoc = argSym->GetLocation();


    Js::OpCode opcode = !funcInfo->root->HasNonSimpleParameterList() ? Js::OpCode::LdHeapArguments : Js::OpCode::LdLetHeapArguments;
    bool hasRest = funcInfo->root->pnodeRest != nullptr;
    uint count = funcInfo->inArgsCount + (hasRest ? 1 : 0) - 1;
    if (count == 0)
    {
        // If no formals to function (only "this"), then no need to create the scope object.
        // Leave both the arguments location and the propertyIds location as null.
        Assert(funcInfo->root->pnodeParams == nullptr && !hasRest);
    }
    else if (!NeedScopeObjectForArguments(funcInfo, funcInfo->root))
    {
        // We may not need a scope object for arguments, e.g. strict mode with no eval.
    }
    else if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
    {
        // Pass the frame object and ID array to the runtime, and put the resulting Arguments object
        // at the expected location.
        Js::PropertyIdArray *propIds = funcInfo->GetParsedFunctionBody()->AllocatePropertyIdArrayForFormals(UInt32Math::Mul(count, sizeof(Js::PropertyId)), count, 0);
        GetFormalArgsArray(this, funcInfo, propIds);
    }

    this->m_writer.Reg1(opcode, argumentsLoc);
    EmitLocalPropInit(argSym->GetLocation(), argSym, funcInfo);
}

void ByteCodeGenerator::LoadCachedHeapArguments(FuncInfo *funcInfo)
{
    Assert(funcInfo->GetHasHeapArguments());

    Scope *scope = funcInfo->GetBodyScope();
    Assert(scope);
    Symbol *argSym = funcInfo->GetArgumentsSymbol();
    Assert(argSym && argSym->IsArguments());
    Js::RegSlot argumentsLoc = argSym->GetLocation();

    Js::OpCode op = !funcInfo->root->HasNonSimpleParameterList() ? Js::OpCode::LdHeapArgsCached : Js::OpCode::LdLetHeapArgsCached;

    this->m_writer.Reg1(op, argumentsLoc);
    EmitLocalPropInit(argumentsLoc, argSym, funcInfo);
}

Js::JavascriptArray* ByteCodeGenerator::BuildArrayFromStringList(ParseNode* stringNodeList, uint arrayLength, Js::ScriptContext* scriptContext)
{
    Assert(stringNodeList);

    uint index = 0;
    Js::Var str;
    IdentPtr pid;
    Js::JavascriptArray* pArr = scriptContext->GetLibrary()->CreateArray(arrayLength);

    while (stringNodeList->nop == knopList)
    {
        Assert(stringNodeList->AsParseNodeBin()->pnode1->nop == knopStr);

        pid = stringNodeList->AsParseNodeBin()->pnode1->AsParseNodeStr()->pid;
        str = Js::JavascriptString::NewCopyBuffer(pid->Psz(), pid->Cch(), scriptContext);
        pArr->SetItemWithAttributes(index, str, PropertyEnumerable);

        stringNodeList = stringNodeList->AsParseNodeBin()->pnode2;
        index++;
    }

    Assert(stringNodeList->nop == knopStr);

    pid = stringNodeList->AsParseNodeStr()->pid;
    str = Js::JavascriptString::NewCopyBuffer(pid->Psz(), pid->Cch(), scriptContext);
    pArr->SetItemWithAttributes(index, str, PropertyEnumerable);

    return pArr;
}

// For now, this just assigns field ids for the current script.
// Later, we will combine this information with the global field id map.
// This temporary code will not work if a global member is accessed both with and without a LHS.
void ByteCodeGenerator::AssignPropertyIds(Js::ParseableFunctionInfo* functionInfo)
{
    globalScope->ForEachSymbol([this, functionInfo](Symbol * sym)
    {
        this->AssignPropertyId(sym, functionInfo);
    });
}

void ByteCodeGenerator::InitBlockScopedContent(ParseNodeBlock *pnodeBlock, Js::DebuggerScope* debuggerScope, FuncInfo *funcInfo)
{
    Assert(pnodeBlock->nop == knopBlock);

    auto genBlockInit = [this, debuggerScope, funcInfo](ParseNode *pnode)
    {
        // Only check if the scope is valid when let/const vars are in the scope.  If there are no let/const vars,
        // the debugger scope will not be created.
        AssertMsg(debuggerScope, "Missing a case of scope tracking in BeginEmitBlock.");

        FuncInfo *funcInfo = this->TopFuncInfo();
        Symbol *sym = pnode->AsParseNodeVar()->sym;
        Scope *scope = sym->GetScope();

        if (sym->GetIsGlobal())
        {
            Js::PropertyId propertyId = sym->EnsurePosition(this);
            if (this->flags & fscrEval)
            {
                AssertMsg(this->IsConsoleScopeEval(), "Let/Consts cannot be in global scope outside of console eval");
                Js::OpCode op = (sym->GetDecl()->nop == knopConstDecl) ? Js::OpCode::InitUndeclConsoleConstFld : Js::OpCode::InitUndeclConsoleLetFld;
                this->m_writer.ElementScopedU(op, funcInfo->FindOrAddReferencedPropertyId(propertyId));
            }
            else
            {
                Js::OpCode op = (sym->GetDecl()->nop == knopConstDecl) ?
                    Js::OpCode::InitUndeclRootConstFld : Js::OpCode::InitUndeclRootLetFld;
                this->m_writer.ElementRootU(op, funcInfo->FindOrAddReferencedPropertyId(propertyId));
            }
        }
        else if (sym->IsInSlot(this, funcInfo) || (scope->GetIsObject() && sym->NeedsSlotAlloc(this, funcInfo)))
        {
            if (scope->GetIsObject())
            {
                Js::RegSlot scopeLocation = scope->GetLocation();
                Js::PropertyId propertyId = sym->EnsurePosition(this);

                if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
                {
                    uint cacheId = funcInfo->FindOrAddInlineCacheId(scopeLocation, propertyId, false, true);

                    Js::OpCode op = (sym->GetDecl()->nop == knopConstDecl) ?
                        Js::OpCode::InitUndeclLocalConstFld : Js::OpCode::InitUndeclLocalLetFld;

                    this->m_writer.ElementP(op, ByteCodeGenerator::ReturnRegister, cacheId);
                }
                else
                {
                    uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->InnerScopeToRegSlot(scope), propertyId, false, true);

                    Js::OpCode op = (sym->GetDecl()->nop == knopConstDecl) ?
                        Js::OpCode::InitUndeclConstFld : Js::OpCode::InitUndeclLetFld;

                    this->m_writer.ElementPIndexed(op, ByteCodeGenerator::ReturnRegister, scope->GetInnerScopeIndex(), cacheId);
                }

                TrackActivationObjectPropertyForDebugger(debuggerScope, sym, pnode->nop == knopConstDecl ? Js::DebuggerScopePropertyFlags_Const : Js::DebuggerScopePropertyFlags_None);
            }
            else
            {
                Js::RegSlot tmpReg = funcInfo->AcquireTmpRegister();
                this->m_writer.Reg1(Js::OpCode::InitUndecl, tmpReg);
                this->EmitLocalPropInit(tmpReg, sym, funcInfo);
                funcInfo->ReleaseTmpRegister(tmpReg);

                // Slot array properties are tracked in non-debug mode as well because they need to stay
                // around for heap enumeration and escaping during attach/detach.
                TrackSlotArrayPropertyForDebugger(debuggerScope, sym, sym->EnsurePosition(this), pnode->nop == knopConstDecl ? Js::DebuggerScopePropertyFlags_Const : Js::DebuggerScopePropertyFlags_None);
            }
        }
        else if (!sym->GetIsModuleExportStorage())
        {
            if (sym->GetDecl()->AsParseNodeVar()->isSwitchStmtDecl)
            {
                // let/const declared in a switch is the only case of a variable that must be checked for
                // use-before-declaration dynamically within its own function.
                this->m_writer.Reg1(Js::OpCode::InitUndecl, sym->GetLocation());
            }
            // Syms that begin in register may be delay-captured. In debugger mode, such syms
            // will live only in slots, so tell the debugger to find them there.
            if (sym->NeedsSlotAlloc(this, funcInfo))
            {
                TrackSlotArrayPropertyForDebugger(debuggerScope, sym, sym->EnsurePosition(this), pnode->nop == knopConstDecl ? Js::DebuggerScopePropertyFlags_Const : Js::DebuggerScopePropertyFlags_None);
            }
            else
            {
                TrackRegisterPropertyForDebugger(debuggerScope, sym, funcInfo, pnode->nop == knopConstDecl ? Js::DebuggerScopePropertyFlags_Const : Js::DebuggerScopePropertyFlags_None);
            }
        }
    };

    IterateBlockScopedVariables(pnodeBlock, genBlockInit);
}

// Records the start of a debugger scope if the passed in node has any let/const variables (or is not a block node).
// If it has no let/const variables, nullptr will be returned as no scope will be created.
Js::DebuggerScope* ByteCodeGenerator::RecordStartScopeObject(ParseNode * pnode, Js::DiagExtraScopesType scopeType, Js::RegSlot scopeLocation /*= Js::Constants::NoRegister*/, int* index /*= nullptr*/)
{
    Assert(pnode);
    if (pnode->nop == knopBlock && !pnode->AsParseNodeBlock()->HasBlockScopedContent())
    {
        // In order to reduce allocations now that we track debugger scopes in non-debug mode,
        // don't add a block to the chain if it has no let/const variables at all.
        return nullptr;
    }

    return this->Writer()->RecordStartScopeObject(scopeType, scopeLocation, index);
}

// Records the end of the current scope, but only if the current block has block scoped content.
// Otherwise, a scope would not have been added (see ByteCodeGenerator::RecordStartScopeObject()).
void ByteCodeGenerator::RecordEndScopeObject(ParseNode *pnodeBlock)
{
    Assert(pnodeBlock);
    if (pnodeBlock->nop == knopBlock && !pnodeBlock->AsParseNodeBlock()->HasBlockScopedContent())
    {
        return;
    }

    this->Writer()->RecordEndScopeObject();
}

void BeginEmitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    Js::DebuggerScope* debuggerScope = nullptr;

    if (BlockHasOwnScope(pnodeBlock, byteCodeGenerator))
    {
        Scope *scope = pnodeBlock->scope;
        byteCodeGenerator->PushScope(scope);

        Js::RegSlot scopeLocation = scope->GetLocation();
        if (scope->GetMustInstantiate())
        {
            Assert(scopeLocation == Js::Constants::NoRegister);
            scopeLocation = funcInfo->FirstInnerScopeReg() + scope->GetInnerScopeIndex();

            if (scope->GetIsObject())
            {
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeInObject, scopeLocation);

                byteCodeGenerator->Writer()->Unsigned1(Js::OpCode::NewBlockScope, scope->GetInnerScopeIndex());
            }
            else
            {
                int scopeIndex = Js::DebuggerScope::InvalidScopeIndex;
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeInSlot, scopeLocation, &scopeIndex);

                // TODO: Handle heap enumeration
                int scopeSlotCount = scope->GetScopeSlotCount();
                byteCodeGenerator->Writer()->Num3(Js::OpCode::NewInnerScopeSlots, scope->GetInnerScopeIndex(), scopeSlotCount + Js::ScopeSlots::FirstSlotIndex, scopeIndex);
            }
        }
        else
        {
            // In the direct register access case, there is no block scope emitted but we can still track
            // the start and end offset of the block.  The location registers for let/const variables will still be
            // captured along with this range in InitBlockScopedContent().
            debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeDirect);
        }

        bool const isGlobalEvalBlockScope = scope->IsGlobalEvalBlockScope();
        Js::RegSlot frameDisplayLoc = Js::Constants::NoRegister;
        Js::RegSlot tmpInnerEnvReg = Js::Constants::NoRegister;
        ParseNodePtr pnodeScope;
        for (pnodeScope = pnodeBlock->pnodeScopes; pnodeScope;)
        {
            switch (pnodeScope->nop)
            {
            case knopFncDecl:
                if (pnodeScope->AsParseNodeFnc()->IsDeclaration())
                {
                    // The frameDisplayLoc register's lifetime has to be controlled by this function. We can't let
                    // it be released by DefineOneFunction, because further iterations of this loop can allocate
                    // temps, and we can't let frameDisplayLoc be re-purposed until this loop completes.
                    // So we'll supply a temp that we allocate and release here.
                    if (frameDisplayLoc == Js::Constants::NoRegister)
                    {
                        if (funcInfo->frameDisplayRegister != Js::Constants::NoRegister)
                        {
                            frameDisplayLoc = funcInfo->frameDisplayRegister;
                        }
                        else
                        {
                            frameDisplayLoc = funcInfo->GetEnvRegister();
                        }
                        tmpInnerEnvReg = funcInfo->AcquireTmpRegister();
                        frameDisplayLoc = byteCodeGenerator->PrependLocalScopes(frameDisplayLoc, tmpInnerEnvReg, funcInfo);
                    }
                    byteCodeGenerator->DefineOneFunction(pnodeScope->AsParseNodeFnc(), funcInfo, true, frameDisplayLoc);
                }

                // If this is the global eval block scope, the function is actually assigned to the global
                // so we don't need to keep the registers.
                if (isGlobalEvalBlockScope)
                {
                    funcInfo->ReleaseLoc(pnodeScope);
                    pnodeScope->location = Js::Constants::NoRegister;
                }
                pnodeScope = pnodeScope->AsParseNodeFnc()->pnodeNext;
                break;

            case knopBlock:
                pnodeScope = pnodeScope->AsParseNodeBlock()->pnodeNext;
                break;

            case knopCatch:
                pnodeScope = pnodeScope->AsParseNodeCatch()->pnodeNext;
                break;

            case knopWith:
                pnodeScope = pnodeScope->AsParseNodeWith()->pnodeNext;
                break;
            }
        }

        if (tmpInnerEnvReg != Js::Constants::NoRegister)
        {
            funcInfo->ReleaseTmpRegister(tmpInnerEnvReg);
        }
    }
    else
    {
        Scope *scope = pnodeBlock->scope;
        if (scope)
        {
            if (scope->GetMustInstantiate())
            {
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeInObject);
            }
            else
            {
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeDirect);
            }
        }
        else
        {
            debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeBlock, Js::DiagExtraScopesType::DiagBlockScopeInSlot);
        }
    }

    byteCodeGenerator->InitBlockScopedContent(pnodeBlock, debuggerScope, funcInfo);
}

void EndEmitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    if (BlockHasOwnScope(pnodeBlock, byteCodeGenerator))
    {
        Scope *scope = pnodeBlock->scope;
        Assert(scope);
        Assert(scope == byteCodeGenerator->GetCurrentScope());
        byteCodeGenerator->PopScope();
    }

    byteCodeGenerator->RecordEndScopeObject(pnodeBlock);
}

void CloneEmitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    if (BlockHasOwnScope(pnodeBlock, byteCodeGenerator))
    {
        // Only let variables have observable behavior when there are per iteration
        // bindings.  const variables do not since they are immutable.  Therefore,
        // (and the spec agrees), only create new scope clones if the loop variable
        // is a let declaration.
        bool isConst = false;
        pnodeBlock->scope->ForEachSymbolUntil([&isConst](Symbol * const sym) {
            // Exploit the fact that a for loop sxBlock can only have let and const
            // declarations, and can only have one or the other, regardless of how
            // many syms there might be.  Thus only check the first sym.
            isConst = sym->GetDecl()->nop == knopConstDecl;
            return true;
        });

        if (!isConst)
        {
            Scope *scope = pnodeBlock->scope;
            Assert(scope == byteCodeGenerator->GetCurrentScope());

            if (scope->GetMustInstantiate())
            {
                Js::OpCode op = scope->GetIsObject() ? Js::OpCode::CloneBlockScope : Js::OpCode::CloneInnerScopeSlots;

                byteCodeGenerator->Writer()->Unsigned1(op, scope->GetInnerScopeIndex());
            }
        }
    }
}

void EmitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, BOOL fReturnValue)
{
    Assert(pnodeBlock->nop == knopBlock);
    ParseNode *pnode = pnodeBlock->pnodeStmt;
    if (pnode == nullptr)
    {
        return;
    }

    BeginEmitBlock(pnodeBlock, byteCodeGenerator, funcInfo);

    ParseNode *pnodeLastValStmt = pnodeBlock->pnodeLastValStmt;

    while (pnode->nop == knopList)
    {
        ParseNode* stmt = pnode->AsParseNodeBin()->pnode1;
        if (stmt == pnodeLastValStmt)
        {
            // This is the last guaranteed return value, so any potential return values have to be
            // copied to the return register from this point forward.
            pnodeLastValStmt = nullptr;
        }
        byteCodeGenerator->EmitTopLevelStatement(stmt, funcInfo, fReturnValue && (pnodeLastValStmt == nullptr));
        pnode = pnode->AsParseNodeBin()->pnode2;
    }

    if (pnode == pnodeLastValStmt)
    {
        pnodeLastValStmt = nullptr;
    }
    byteCodeGenerator->EmitTopLevelStatement(pnode, funcInfo, fReturnValue && (pnodeLastValStmt == nullptr));

    EndEmitBlock(pnodeBlock, byteCodeGenerator, funcInfo);
}

void ClearTmpRegs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, FuncInfo* emitFunc)
{
    if (emitFunc->IsTmpReg(pnode->location))
    {
        pnode->location = Js::Constants::NoRegister;
    }
}

void ByteCodeGenerator::EmitTopLevelStatement(ParseNode *stmt, FuncInfo *funcInfo, BOOL fReturnValue)
{
    if (stmt->nop == knopFncDecl && stmt->AsParseNodeFnc()->IsDeclaration())
    {
        // Function declarations (not function-declaration RHS's) are already fully processed.
        // Skip them here so the temp registers don't get messed up.
        return;
    }

    if (stmt->nop == knopName || stmt->nop == knopDot)
    {
        // Generating span for top level names are mostly useful in debugging mode, because user can debug it even though no side-effect expected.
        // But the name can have runtime error, e.g., foo.bar; // where foo is not defined.
        // At this time we need to throw proper line number and offset. so recording on all modes will be useful.
        StartStatement(stmt);
        Writer()->Empty(Js::OpCode::Nop);
        EndStatement(stmt);
    }

    Emit(stmt, this, funcInfo, fReturnValue, false/*isConstructorCall*/, Js::Constants::NoRegister/*computedPropertyLocation*/, true/*isTopLevel*/);
    if (funcInfo->IsTmpReg(stmt->location))
    {
        if (!stmt->isUsed && !fReturnValue)
        {
            m_writer.Reg1(Js::OpCode::Unused, stmt->location);
        }
        funcInfo->ReleaseLoc(stmt);
    }
}

// ByteCodeGenerator::DefineFunctions
//
// Emit byte code for scope-wide function definitions before any calls in the scope, regardless of lexical
// order. Note that stores to the closure array are not emitted until we see the knopFncDecl in the tree
// to make sure that sources of the stores have been defined.
void ByteCodeGenerator::DefineFunctions(FuncInfo *funcInfoParent)
{
    // DefineCachedFunctions doesn't depend on whether the user vars are declared or not, so
    // we'll just overload this variable to mean that the functions getting called again and we don't need to do anything
    if (funcInfoParent->GetHasCachedScope())
    {
        this->DefineCachedFunctions(funcInfoParent);
    }
    else
    {
        this->DefineUncachedFunctions(funcInfoParent);
    }
}

// Iterate over all child functions in a function's parameter and body scopes.
template<typename Fn>
void MapContainerScopeFunctions(ParseNode* pnodeScope, Fn fn)
{
    auto mapFncDeclsInScopeList = [&](ParseNode *pnodeHead)
    {
        for (ParseNode *pnode = pnodeHead; pnode != nullptr;)
        {
            switch (pnode->nop)
            {
            case knopFncDecl:
                fn(pnode);
                pnode = pnode->AsParseNodeFnc()->pnodeNext;
                break;

            case knopBlock:
                pnode = pnode->AsParseNodeBlock()->pnodeNext;
                break;

            case knopCatch:
                pnode = pnode->AsParseNodeCatch()->pnodeNext;
                break;

            case knopWith:
                pnode = pnode->AsParseNodeWith()->pnodeNext;
                break;

            default:
                AssertMsg(false, "Unexpected opcode in tree of scopes");
                return;
            }
        }
    };
    pnodeScope->AsParseNodeFnc()->MapContainerScopes(mapFncDeclsInScopeList);
}

void ByteCodeGenerator::DefineCachedFunctions(FuncInfo *funcInfoParent)
{
    ParseNode *pnodeParent = funcInfoParent->root;
    uint slotCount = 0;

    auto countFncSlots = [&](ParseNode *pnodeFnc)
    {
        if (pnodeFnc->AsParseNodeFnc()->GetFuncSymbol() != nullptr && pnodeFnc->AsParseNodeFnc()->IsDeclaration())
        {
            slotCount++;
        }
    };
    MapContainerScopeFunctions(pnodeParent, countFncSlots);

    if (slotCount == 0)
    {
        return;
    }

    size_t extraBytesActual = AllocSizeMath::Mul(slotCount, sizeof(Js::FuncInfoEntry));
    // Reg2Aux takes int for byteCount so we need to convert to int. OOM if we can't because it would truncate data.
    if (extraBytesActual > INT_MAX)
    {
        Js::Throw::OutOfMemory();
    }
    int extraBytes = (int)extraBytesActual;

    Js::FuncInfoArray *info = AnewPlus(alloc, extraBytes, Js::FuncInfoArray, slotCount);

    // slotCount is guaranteed to be non-zero here.
    Js::AuxArray<uint32> * slotIdInCachedScopeToNestedIndexArray = funcInfoParent->GetParsedFunctionBody()->AllocateSlotIdInCachedScopeToNestedIndexArray(slotCount);

    slotCount = 0;

    auto fillEntries = [&](ParseNode *pnodeFnc)
    {
        Symbol *sym = pnodeFnc->AsParseNodeFnc()->GetFuncSymbol();
        if (sym != nullptr && (pnodeFnc->AsParseNodeFnc()->IsDeclaration()))
        {
            AssertMsg(!pnodeFnc->AsParseNodeFnc()->IsGenerator(), "Generator functions are not supported by InitCachedFuncs but since they always escape they should disable function caching");
            Js::FuncInfoEntry *entry = &info->elements[slotCount];
            entry->nestedIndex = pnodeFnc->AsParseNodeFnc()->nestedIndex;
            entry->scopeSlot = sym->GetScopeSlot();

            slotIdInCachedScopeToNestedIndexArray->elements[slotCount] = pnodeFnc->AsParseNodeFnc()->nestedIndex;
            slotCount++;
        }
    };
    MapContainerScopeFunctions(pnodeParent, fillEntries);

    m_writer.AuxNoReg(Js::OpCode::InitCachedFuncs,
        info,
        sizeof(Js::FuncInfoArray) + extraBytes,
        sizeof(Js::FuncInfoArray) + extraBytes);

    slotCount = 0;
    auto defineOrGetCachedFunc = [&](ParseNode *pnodeFnc)
    {
        Symbol *sym = pnodeFnc->AsParseNodeFnc()->GetFuncSymbol();
        if (pnodeFnc->AsParseNodeFnc()->IsDeclaration())
        {
            // Do we need to define the function here (i.e., is it not one of our cached locals)?
            // Only happens if the sym is null (e.g., function x.y(){}).
            if (sym == nullptr)
            {
                this->DefineOneFunction(pnodeFnc->AsParseNodeFnc(), funcInfoParent);
            }
            else if (!sym->IsInSlot(this, funcInfoParent) && sym->GetLocation() != Js::Constants::NoRegister)
            {
                // If it was defined by InitCachedFuncs, do we need to put it in a register rather than a slot?
                m_writer.Reg1Unsigned1(Js::OpCode::GetCachedFunc, sym->GetLocation(), slotCount);
            }
            // The "x = function() {...}" case is being generated on the fly, during emission,
            // so the caller expects to be able to release this register.
            funcInfoParent->ReleaseLoc(pnodeFnc);
            pnodeFnc->location = Js::Constants::NoRegister;
            slotCount++;
        }
    };
    MapContainerScopeFunctions(pnodeParent, defineOrGetCachedFunc);

    AdeletePlus(alloc, extraBytes, info);
}

void ByteCodeGenerator::DefineUncachedFunctions(FuncInfo *funcInfoParent)
{
    ParseNode *pnodeParent = funcInfoParent->root;
    auto defineCheck = [&](ParseNode *pnodeFnc)
    {
        Assert(pnodeFnc->nop == knopFncDecl);

        //
        // Don't define the function upfront in following cases
        // 1. x = function() {...};
        //    Don't define the function for all modes.
        //    Such a function can only be accessed via the LHS, so we define it at the assignment point
        //    rather than the scope entry to save a register (and possibly save the whole definition).
        //
        // 2. x = function f() {...};
        //    f is not visible in the enclosing scope.
        //    Such function expressions should be emitted only at the assignment point, as can be used only
        //    after the assignment. Might save register.
        //

        if (pnodeFnc->AsParseNodeFnc()->IsDeclaration())
        {
            this->DefineOneFunction(pnodeFnc->AsParseNodeFnc(), funcInfoParent);
            // The "x = function() {...}" case is being generated on the fly, during emission,
            // so the caller expects to be able to release this register.
            funcInfoParent->ReleaseLoc(pnodeFnc);
            pnodeFnc->location = Js::Constants::NoRegister;
        }
    };
    MapContainerScopeFunctions(pnodeParent, defineCheck);
}

void EmitAssignmentToFuncName(ParseNodeFnc *pnodeFnc, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfoParent)
{
    // Assign the location holding the func object reference to the given name.
    Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
    Symbol *sym = pnodeFnc->pnodeName->sym;

    if (sym != nullptr && !sym->GetIsFuncExpr())
    {
        if (sym->GetIsModuleExportStorage())
        {
            byteCodeGenerator->EmitPropStore(pnodeFnc->location, sym, nullptr, funcInfoParent);
        }
        else if (sym->GetIsGlobal())
        {
            Js::PropertyId propertyId = sym->GetPosition();
            byteCodeGenerator->EmitGlobalFncDeclInit(pnodeFnc->location, propertyId, funcInfoParent);
            if (byteCodeGenerator->GetFlags() & fscrEval && !funcInfoParent->GetIsStrictMode())
            {
                byteCodeGenerator->EmitPropStore(pnodeFnc->location, sym, nullptr, funcInfoParent);
            }
        }
        else
        {
            if (sym->NeedsSlotAlloc(byteCodeGenerator, funcInfoParent))
            {
                if (!sym->GetHasNonCommittedReference() ||
                    (funcInfoParent->GetParsedFunctionBody()->DoStackNestedFunc()))
                {
                    // No point in trying to optimize if there are no references before we have to commit to slot.
                    // And not safe to delay putting a stack function in the slot, since we may miss boxing.
                    sym->SetIsCommittedToSlot();
                }
            }

            if (sym->GetScope()->GetFunc() != byteCodeGenerator->TopFuncInfo())
            {
                byteCodeGenerator->EmitPropStore(pnodeFnc->location, sym, nullptr, funcInfoParent);
            }
            else
            {
                byteCodeGenerator->EmitLocalPropInit(pnodeFnc->location, sym, funcInfoParent);
            }

            Symbol * fncScopeSym = sym->GetFuncScopeVarSym();

            if (fncScopeSym)
            {
                if (fncScopeSym->GetIsGlobal() && byteCodeGenerator->GetFlags() & fscrEval)
                {
                    Js::PropertyId propertyId = fncScopeSym->GetPosition();
                    byteCodeGenerator->EmitGlobalFncDeclInit(pnodeFnc->location, propertyId, funcInfoParent);
                }
                else
                {
                    byteCodeGenerator->EmitPropStore(pnodeFnc->location, fncScopeSym, nullptr, funcInfoParent, false, false, /* isFncDeclVar */true);
                }
            }
        }
    }
}

Js::RegSlot ByteCodeGenerator::DefineOneFunction(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfoParent, bool generateAssignment, Js::RegSlot regEnv, Js::RegSlot frameDisplayTemp)
{
    Assert(pnodeFnc->nop == knopFncDecl);

    funcInfoParent->AcquireLoc(pnodeFnc);

    if (regEnv == Js::Constants::NoRegister)
    {
        // If the child needs a closure, find a heap-allocated frame to pass to it.
        if (frameDisplayTemp != Js::Constants::NoRegister)
        {
            // We allocated a temp to hold a local frame display value. Use that.
            // It's likely that the FD is on the stack, and we used the temp to load it back.
            regEnv = frameDisplayTemp;
        }
        else if (funcInfoParent->frameDisplayRegister != Js::Constants::NoRegister)
        {
            // This function has built a frame display, so pass it down.
            regEnv = funcInfoParent->frameDisplayRegister;
        }
        else
        {
            // This function has no captured locals but inherits a closure environment, so pass it down.
            regEnv = funcInfoParent->GetEnvRegister();
        }

        regEnv = this->PrependLocalScopes(regEnv, Js::Constants::NoRegister, funcInfoParent);
    }

    // AssertMsg(funcInfo->nonLocalSymbols == 0 || regEnv != funcInfoParent->nullConstantRegister,
    // "We need a closure for the nested function");

    Assert(pnodeFnc->nestedIndex != (uint)-1);

    // If we are in a parameter scope and it is not merged with body scope then we have to create the child function as an inner function
    if (regEnv == funcInfoParent->frameDisplayRegister || regEnv == funcInfoParent->GetEnvRegister())
    {
        m_writer.NewFunction(pnodeFnc->location, pnodeFnc->nestedIndex, pnodeFnc->IsCoroutine(), pnodeFnc->GetHomeObjLocation());
    }
    else
    {
        m_writer.NewInnerFunction(pnodeFnc->location, pnodeFnc->nestedIndex, regEnv, pnodeFnc->IsCoroutine(), pnodeFnc->GetHomeObjLocation());
    }

    if (funcInfoParent->IsGlobalFunction() && (this->flags & fscrEval))
    {
        // A function declared at global scope in eval is untrackable,
        // so make sure the caller's cached scope is invalidated.
        this->funcEscapes = true;
    }
    else
    {
        if (pnodeFnc->IsDeclaration())
        {
            Symbol * funcSymbol = pnodeFnc->GetFuncSymbol();
            if (funcSymbol)
            {
                // In the case where a let/const declaration is the same symbol name
                // as the function declaration (shadowing case), the let/const var and
                // the function declaration symbol are the same and share the same flags
                // (particularly, sym->GetIsBlockVar() for this code path).
                //
                // For example:
                // let a = 0;       // <-- sym->GetIsBlockVar() = true
                // function b(){}   // <-- sym2->GetIsBlockVar() = false
                //
                // let x = 0;       // <-- sym3->GetIsBlockVar() = true
                // function x(){}   // <-- sym3->GetIsBlockVar() = true
                //
                // In order to tell if the function is actually part
                // of a block scope, we compare against the function scope here.
                // Note that having a function with the same name as a let/const declaration
                // is a redeclaration error, but we're pushing the fix for this out since it's
                // a bit involved.
                Assert(funcInfoParent->GetBodyScope() != nullptr && funcSymbol->GetScope() != nullptr);
                bool isFunctionDeclarationInBlock = funcSymbol->GetIsBlockVar();

                // Track all vars/lets/consts register slot function declarations.
                if (ShouldTrackDebuggerMetadata()
                    // If this is a let binding function declaration at global level, we want to
                    // be sure to track the register location as well.
                    && !(funcInfoParent->IsGlobalFunction() && !isFunctionDeclarationInBlock))
                {
                    if (!funcSymbol->IsInSlot(this, funcInfoParent))
                    {
                        funcInfoParent->byteCodeFunction->GetFunctionBody()->InsertSymbolToRegSlotList(funcSymbol->GetName(), pnodeFnc->location, funcInfoParent->varRegsCount);
                    }
                }

                if (isFunctionDeclarationInBlock)
                {
                    // We only track inner let bindings for the debugger side.
                    this->TrackFunctionDeclarationPropertyForDebugger(funcSymbol, funcInfoParent);
                }
            }
        }
    }

    if (pnodeFnc->IsDefaultModuleExport())
    {
        this->EmitAssignmentToDefaultModuleExport(pnodeFnc, funcInfoParent);
    }

    if (pnodeFnc->pnodeName == nullptr || !generateAssignment)
    {
        return regEnv;
    }

    EmitAssignmentToFuncName(pnodeFnc, this, funcInfoParent);

    return regEnv;
}

void ByteCodeGenerator::DefineUserVars(FuncInfo *funcInfo)
{
    // Initialize scope-wide variables on entry to the scope. TODO: optimize by detecting uses that are always reached
    // by an existing initialization.

    BOOL fGlobal = funcInfo->IsGlobalFunction();
    ParseNode *pnode;
    Js::FunctionBody *byteCodeFunction = funcInfo->GetParsedFunctionBody();
    // Global declarations need a temp register to hold the init value, but the node shouldn't get a register.
    // Just assign one on the fly and re-use it for all initializations.
    Js::RegSlot tmpReg = fGlobal ? funcInfo->AcquireTmpRegister() : Js::Constants::NoRegister;

    for (pnode = funcInfo->root->pnodeVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
    {
        Symbol* sym = pnode->AsParseNodeVar()->sym;

        if (sym != nullptr && !(pnode->AsParseNodeVar()->isBlockScopeFncDeclVar && sym->GetIsBlockVar()))
        {
            if (sym->IsSpecialSymbol())
            {
                // Special symbols have already had their initial values stored into their registers.
                // In default-argument case we've stored those values into their slot locations, as well.
                // We must do that because a default parameter may access a special symbol through a scope slot.
                // In the non-default-argument case, though, we didn't yet store the values into the
                // slots so let's do that now.
                if (!funcInfo->root->HasNonSimpleParameterList())
                {
                    EmitPropStoreForSpecialSymbol(sym->GetLocation(), sym, sym->GetPid(), funcInfo, true);

                    if (ShouldTrackDebuggerMetadata() && !sym->IsInSlot(this, funcInfo))
                    {
                        byteCodeFunction->InsertSymbolToRegSlotList(sym->GetName(), sym->GetLocation(), funcInfo->varRegsCount);
                    }
                }

                continue;
            }

            if (sym->GetIsCatch() || (pnode->nop == knopVarDecl && sym->GetIsBlockVar()))
            {
                // The init node was bound to the catch object, because it's inside a catch and has the
                // same name as the catch object. But we want to define a user var at function scope,
                // so find the right symbol. (We'll still assign the RHS value to the catch object symbol.)
                // This also applies to a var declaration in the same scope as a let declaration.
#if DBG
                if (sym->IsArguments())
                {
                    // There is a block scoped var named arguments
                    Assert(!funcInfo->GetHasArguments());
                    continue;
                }
                else if (!sym->GetIsCatch())
                {
                    // Assert that catch cannot be at function scope and let and var at function scope is redeclaration error.
                    Assert(funcInfo->bodyScope != sym->GetScope());
                }
#endif
                sym = funcInfo->bodyScope->FindLocalSymbol(sym->GetName());
                Assert(sym && !sym->GetIsCatch() && !sym->GetIsBlockVar());
            }

            if (sym->GetSymbolType() == STVariable && !sym->GetIsModuleExportStorage())
            {
                if (fGlobal)
                {
                    Js::PropertyId propertyId = sym->EnsurePosition(this);

                    // We do need to initialize some globals to avoid JS errors on loading undefined variables.
                    // But we first need to make sure we're not trashing built-ins.

                    if (this->flags & fscrEval)
                    {
                        if (funcInfo->byteCodeFunction->GetIsStrictMode())
                        {
                            // Check/Init the property of the frame object
                            this->m_writer.ElementRootU(Js::OpCode::LdLocalElemUndef,
                                funcInfo->FindOrAddReferencedPropertyId(propertyId));
                        }
                        else
                        {
                            // The check and the init involve the first element in the scope chain.
                            this->m_writer.ElementScopedU(
                                Js::OpCode::LdElemUndefScoped, funcInfo->FindOrAddReferencedPropertyId(propertyId));
                        }
                    }
                    else
                    {
                        this->m_writer.ElementU(Js::OpCode::LdElemUndef, ByteCodeGenerator::RootObjectRegister,
                            funcInfo->FindOrAddReferencedPropertyId(propertyId));
                    }
                }
                else if (!sym->IsArguments())
                {
                    if (sym->NeedsSlotAlloc(this, funcInfo))
                    {
                        if (!sym->GetHasNonCommittedReference() ||
                            (sym->GetHasFuncAssignment() && funcInfo->GetParsedFunctionBody()->DoStackNestedFunc()))
                        {
                            // No point in trying to optimize if there are no references before we have to commit to slot.
                            // And not safe to delay putting a stack function in the slot, since we may miss boxing.
                            sym->SetIsCommittedToSlot();
                        }
                    }

                    // Undef-initialize the home location if it is a register (not closure-captured, or else capture
                    // is delayed) or a property of an object.
                    if ((!sym->GetHasInit() && !sym->IsInSlot(this, funcInfo)) ||
                        (funcInfo->bodyScope->GetIsObject() && !funcInfo->GetHasCachedScope()))
                    {
                        Js::RegSlot reg = sym->GetLocation();
                        if (reg == Js::Constants::NoRegister)
                        {
                            Assert(sym->IsInSlot(this, funcInfo));
                            reg = funcInfo->AcquireTmpRegister();
                        }
                        this->m_writer.Reg1(Js::OpCode::LdUndef, reg);
                        this->EmitLocalPropInit(reg, sym, funcInfo);

                        if (ShouldTrackDebuggerMetadata() && !sym->GetHasInit() && !sym->IsInSlot(this, funcInfo))
                        {
                            byteCodeFunction->InsertSymbolToRegSlotList(sym->GetName(), reg, funcInfo->varRegsCount);
                        }

                        funcInfo->ReleaseTmpRegister(reg);
                    }
                }
                else if (ShouldTrackDebuggerMetadata())
                {
                    if (!sym->GetHasInit() && !sym->IsInSlot(this, funcInfo))
                    {
                        Js::RegSlot reg = sym->GetLocation();
                        if (reg != Js::Constants::NoRegister)
                        {
                            byteCodeFunction->InsertSymbolToRegSlotList(sym->GetName(), reg, funcInfo->varRegsCount);
                        }
                    }
                }
                sym->SetHasInit(TRUE);
            }
        }

    }
    if (tmpReg != Js::Constants::NoRegister)
    {
        funcInfo->ReleaseTmpRegister(tmpReg);
    }

    for (int i = 0; i < funcInfo->nonUserNonTempRegistersToInitialize.Count(); ++i)
    {
        m_writer.Reg1(Js::OpCode::LdUndef, funcInfo->nonUserNonTempRegistersToInitialize.Item(i));
    }
}

void ByteCodeGenerator::InitBlockScopedNonTemps(ParseNode *pnode, FuncInfo *funcInfo)
{
    // Initialize all non-temp register variables on entry to the enclosing func - in particular,
    // those with lifetimes that begin after the start of user code and may not be initialized normally.
    // This protects us from, for instance, trying to restore garbage on bailout.
    // It was originally done in debugger mode only, but we do it always to avoid issues with boxing
    // garbage on exit from jitted loop bodies.
    while (pnode)
    {
        switch (pnode->nop)
        {
        case knopFncDecl:
        {
            // If this is a block-scoped function, initialize it.
            ParseNodeFnc * pnodeFnc = pnode->AsParseNodeFnc();
            ParseNodeVar *pnodeName = pnodeFnc->pnodeName;
            if (!pnodeFnc->IsMethod() && pnodeName != nullptr)
            {
                Symbol *sym = pnodeName->sym;
                Assert(sym);
                if (sym->GetLocation() != Js::Constants::NoRegister &&
                    sym->GetScope()->IsBlockScope(funcInfo) &&
                    sym->GetScope()->GetFunc() == funcInfo)
                {
                    this->m_writer.Reg1(Js::OpCode::LdUndef, sym->GetLocation());
                }
            }

            // No need to recurse to the nested scopes, as they belong to a nested function.
            pnode = pnodeFnc->pnodeNext;
            break;
        }

        case knopBlock:
        {
            ParseNodeBlock * pnodeBlock = pnode->AsParseNodeBlock();
            Scope *scope = pnodeBlock->scope;
            if (scope)
            {
                if (scope->IsBlockScope(funcInfo))
                {
                    Js::RegSlot scopeLoc = scope->GetLocation();
                    if (scopeLoc != Js::Constants::NoRegister && !funcInfo->IsTmpReg(scopeLoc))
                    {
                        this->m_writer.Reg1(Js::OpCode::LdUndef, scopeLoc);
                    }
                }
                auto fnInit = [this, funcInfo](ParseNode *pnode)
                {
                    Symbol *sym = pnode->AsParseNodeVar()->sym;
                    if (!sym->IsInSlot(this, funcInfo) && !sym->GetIsGlobal() && !sym->GetIsModuleImport())
                    {
                        this->m_writer.Reg1(Js::OpCode::InitUndecl, pnode->AsParseNodeVar()->sym->GetLocation());
                    }
                };
                IterateBlockScopedVariables(pnodeBlock, fnInit);
            }
            InitBlockScopedNonTemps(pnodeBlock->pnodeScopes, funcInfo);
            pnode = pnodeBlock->pnodeNext;
            break;
        }
        case knopCatch:
            InitBlockScopedNonTemps(pnode->AsParseNodeCatch()->pnodeScopes, funcInfo);
            pnode = pnode->AsParseNodeCatch()->pnodeNext;
            break;

        case knopWith:
        {
            Js::RegSlot withLoc = pnode->location;
            AssertMsg(withLoc != Js::Constants::NoRegister && !funcInfo->IsTmpReg(withLoc),
                "We should put with objects at known stack locations in debug mode");
            this->m_writer.Reg1(Js::OpCode::LdUndef, withLoc);
            InitBlockScopedNonTemps(pnode->AsParseNodeWith()->pnodeScopes, funcInfo);
            pnode = pnode->AsParseNodeWith()->pnodeNext;
            break;
        }

        default:
            Assert(false);
            return;
        }
    }
}

void ByteCodeGenerator::EmitScopeObjectInit(FuncInfo *funcInfo)
{
    Assert(!funcInfo->byteCodeFunction->GetFunctionBody()->DoStackNestedFunc());

    if (!funcInfo->GetHasCachedScope() /* || forcing scope/inner func caching */)
    {
        return;
    }

    Scope* currentScope = funcInfo->GetCurrentChildScope();
    uint slotCount = currentScope->GetScopeSlotCount();
    uint cachedFuncCount = 0;
    Js::PropertyId firstFuncSlot = Js::Constants::NoProperty;
    Js::PropertyId firstVarSlot = Js::Constants::NoProperty;

    uint extraAlloc = UInt32Math::Add(slotCount, Js::ActivationObjectEx::ExtraSlotCount());
    extraAlloc = UInt32Math::Mul(extraAlloc, sizeof(Js::PropertyId));

    // Create and fill the array of local property ID's.
    // They all have slots assigned to them already (if they need them): see StartEmitFunction.

    Js::PropertyIdArray *propIds = funcInfo->GetParsedFunctionBody()->AllocatePropertyIdArrayForFormals(extraAlloc, slotCount, Js::ActivationObjectEx::ExtraSlotCount());

    ParseNodeFnc *pnodeFnc = funcInfo->root;
    ParseNode *pnode;
    Symbol *sym;

    if (funcInfo->GetFuncExprNameReference() && pnodeFnc->GetFuncSymbol()->GetScope() == funcInfo->GetBodyScope())
    {
        Symbol::SaveToPropIdArray(pnodeFnc->GetFuncSymbol(), propIds, this);
    }

    if (funcInfo->GetHasArguments())
    {
        // Because the arguments object can access all instances of same-named formals ("function(x,x){...}"),
        // be sure we initialize any duplicate appearances of a formal parameter to "NoProperty".
        Js::PropertyId slot = 0;
        auto initArg = [&](ParseNode *pnode)
        {
            if (pnode->IsVarLetOrConst())
            {
                Symbol *sym = pnode->AsParseNodeVar()->sym;
                Assert(sym);
                if (sym->GetScopeSlot() == slot)
                {
                    // This is the last appearance of the formal, so record the ID.
                    Symbol::SaveToPropIdArray(sym, propIds, this);
                }
                else
                {
                    // This is an earlier duplicate appearance of the formal, so use NoProperty as a placeholder
                    // since this slot can't be accessed by name.
                    Assert(sym->GetScopeSlot() != Js::Constants::NoProperty && sym->GetScopeSlot() > slot);
                    propIds->elements[slot] = Js::Constants::NoProperty;
                }
            }
            else
            {
                // This is for patterns
                propIds->elements[slot] = Js::Constants::NoProperty;
            }
            slot++;
        };
        MapFormalsWithoutRest(pnodeFnc, initArg);

        // If the rest is in the slot - we need to keep that slot.
        if (pnodeFnc->pnodeRest != nullptr && pnodeFnc->pnodeRest->sym->IsInSlot(this, funcInfo))
        {
            Symbol::SaveToPropIdArray(pnodeFnc->pnodeRest->sym, propIds, this);
        }
    }
    else
    {
        MapFormals(pnodeFnc, [&](ParseNode *pnode)
        {
            if (pnode->IsVarLetOrConst())
            {
                Symbol::SaveToPropIdArray(pnode->AsParseNodeVar()->sym, propIds, this);
            }
        });
    }

    auto saveFunctionVarsToPropIdArray = [&](ParseNode *pnodeFunction)
    {
        if (pnodeFunction->AsParseNodeFnc()->IsDeclaration())
        {
            ParseNode *pnodeName = pnodeFunction->AsParseNodeFnc()->pnodeName;
            if (pnodeName != nullptr)
            {
                while (pnodeName->nop == knopList)
                {
                    if (pnodeName->AsParseNodeBin()->pnode1->nop == knopVarDecl)
                    {
                        sym = pnodeName->AsParseNodeBin()->pnode1->AsParseNodeVar()->sym;
                        if (sym)
                        {
                            Symbol::SaveToPropIdArray(sym, propIds, this, &firstFuncSlot);
                        }
                    }
                    pnodeName = pnodeName->AsParseNodeBin()->pnode2;
                }
                if (pnodeName->nop == knopVarDecl)
                {
                    sym = pnodeName->AsParseNodeVar()->sym;
                    if (sym)
                    {
                        Symbol::SaveToPropIdArray(sym, propIds, this, &firstFuncSlot);
                        cachedFuncCount++;
                    }
                }
            }
        }
    };
    MapContainerScopeFunctions(pnodeFnc, saveFunctionVarsToPropIdArray);

    if (currentScope->GetScopeType() != ScopeType_Parameter)
    {
        for (pnode = pnodeFnc->pnodeVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
        {
            sym = pnode->AsParseNodeVar()->sym;
            if (!(pnode->AsParseNodeVar()->isBlockScopeFncDeclVar && sym->GetIsBlockVar()))
            {
                if (sym->GetIsCatch() || (pnode->nop == knopVarDecl && sym->GetIsBlockVar()))
                {
                    sym = currentScope->FindLocalSymbol(sym->GetName());
                }
                Symbol::SaveToPropIdArray(sym, propIds, this, &firstVarSlot);
            }
        }

        ParseNodeBlock *pnodeBlock = pnodeFnc->pnodeScopes;
        for (pnode = pnodeBlock->pnodeLexVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
        {
            sym = pnode->AsParseNodeVar()->sym;
            Symbol::SaveToPropIdArray(sym, propIds, this, &firstVarSlot);
        }

        pnodeBlock = pnodeFnc->pnodeBodyScope;
        for (pnode = pnodeBlock->pnodeLexVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
        {
            sym = pnode->AsParseNodeVar()->sym;
            Symbol::SaveToPropIdArray(sym, propIds, this, &firstVarSlot);
        }
    }
    else
    {
        Assert(!funcInfo->IsBodyAndParamScopeMerged());
    }

    // Write the first func slot and first var slot into the auxiliary data
    Js::PropertyId *slots = propIds->elements + slotCount;
    slots[0] = cachedFuncCount;
    slots[1] = firstFuncSlot;
    slots[2] = firstVarSlot;
    slots[3] = funcInfo->GetParsedFunctionBody()->NewObjectLiteral();

    propIds->hasNonSimpleParams = funcInfo->root->HasNonSimpleParameterList();

    funcInfo->GetParsedFunctionBody()->SetHasCachedScopePropIds(true);
}

void ByteCodeGenerator::SetClosureRegisters(FuncInfo* funcInfo, Js::FunctionBody* byteCodeFunction)
{
    if (funcInfo->frameDisplayRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->MapAndSetLocalFrameDisplayRegister(funcInfo->frameDisplayRegister);
    }

    if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->MapAndSetLocalClosureRegister(funcInfo->frameObjRegister);
        byteCodeFunction->SetHasScopeObject(true);
    }
    else if (funcInfo->frameSlotsRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->MapAndSetLocalClosureRegister(funcInfo->frameSlotsRegister);
    }

    if (funcInfo->paramSlotsRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->MapAndSetParamClosureRegister(funcInfo->paramSlotsRegister);
    }
}

void ByteCodeGenerator::FinalizeRegisters(FuncInfo * funcInfo, Js::FunctionBody * byteCodeFunction)
{
    if (byteCodeFunction->IsCoroutine())
    {
        // EmitYield uses 'false' to create the IteratorResult object
        funcInfo->AssignFalseConstRegister();
    }

    if (funcInfo->NeedEnvRegister())
    {
        bool constReg = !funcInfo->GetIsTopLevelEventHandler() && funcInfo->IsGlobalFunction() && !(this->flags & fscrEval);
        funcInfo->AssignEnvRegister(constReg);
    }

    // Set the function body's constant count before emitting anything so that the byte code writer
    // can distinguish constants from variables.
    byteCodeFunction->CheckAndSetConstantCount(funcInfo->constRegsCount);

    this->SetClosureRegisters(funcInfo, byteCodeFunction);

    if (this->IsInDebugMode() || byteCodeFunction->IsCoroutine())
    {
        // Give permanent registers to the inner scopes in debug mode.
        // TODO: We create seperate debuggerscopes for each block which has own scope. These are stored in the var registers
        // allocated below. Ideally we should change this logic to not allocate separate registers for these and save the debug
        // info in corresponding symbols and use it from there. This will also affect the temp register allocation logic in
        // EmitOneFunction.
        uint innerScopeCount = funcInfo->InnerScopeCount();
        byteCodeFunction->SetInnerScopeCount(innerScopeCount);
        if (innerScopeCount)
        {
            funcInfo->SetFirstInnerScopeReg(funcInfo->NextVarRegister());
            for (uint i = 1; i < innerScopeCount; i++)
            {
                funcInfo->NextVarRegister();
            }
        }
    }

    // NOTE: The FB expects the yield reg to be the final non-temp.
    if (byteCodeFunction->IsCoroutine())
    {
        funcInfo->AssignYieldRegister();
    }

    Js::RegSlot firstTmpReg = funcInfo->varRegsCount;
    funcInfo->SetFirstTmpReg(firstTmpReg);
    byteCodeFunction->SetFirstTmpReg(funcInfo->RegCount());
}

void ByteCodeGenerator::InitScopeSlotArray(FuncInfo * funcInfo)
{
    // Record slots info for ScopeSlots/ScopeObject.
    uint scopeSlotCount = funcInfo->bodyScope->GetScopeSlotCount();
    bool isSplitScope = !funcInfo->IsBodyAndParamScopeMerged();
    Assert(funcInfo->paramScope == nullptr || funcInfo->paramScope->GetScopeSlotCount() == 0 || isSplitScope);
    uint scopeSlotCountForParamScope = funcInfo->paramScope != nullptr ? funcInfo->paramScope->GetScopeSlotCount() : 0;

    if (scopeSlotCount == 0 && scopeSlotCountForParamScope == 0)
    {
        return;
    }

    Js::FunctionBody *byteCodeFunction = funcInfo->GetParsedFunctionBody();
    if (scopeSlotCount > 0 || scopeSlotCountForParamScope > 0)
    {
        byteCodeFunction->SetScopeSlotArraySizes(scopeSlotCount, scopeSlotCountForParamScope);
    }

    // TODO: Need to add property ids for the case when scopeSlotCountForParamSCope is non-zero
    if (scopeSlotCount)
    {
        Js::PropertyId *propertyIdsForScopeSlotArray = RecyclerNewArrayLeafZ(scriptContext->GetRecycler(), Js::PropertyId, scopeSlotCount);
        byteCodeFunction->SetPropertyIdsForScopeSlotArray(propertyIdsForScopeSlotArray, scopeSlotCount, scopeSlotCountForParamScope);
        AssertMsg(!byteCodeFunction->IsReparsed() || byteCodeFunction->WasEverAsmJsMode() || byteCodeFunction->scopeSlotArraySize == scopeSlotCount,
            "The slot array size is different between debug and non-debug mode");
#if DEBUG
        for (UINT i = 0; i < scopeSlotCount; i++)
        {
            propertyIdsForScopeSlotArray[i] = Js::Constants::NoProperty;
        }
#endif
        auto setPropertyIdForScopeSlotArray =
            [scopeSlotCount, propertyIdsForScopeSlotArray]
            (Js::PropertyId slot, Js::PropertyId propId)
        {
            if (slot < 0 || (uint)slot >= scopeSlotCount)
            {
                Js::Throw::FatalInternalError();
            }
            propertyIdsForScopeSlotArray[slot] = propId;
        };

        auto setPropIdsForScopeSlotArray = [this, funcInfo, setPropertyIdForScopeSlotArray](Symbol *const sym)
        {
            if (sym->NeedsSlotAlloc(this, funcInfo))
            {
                // All properties should get correct propertyId here.
                Assert(sym->HasScopeSlot()); // We can't allocate scope slot now. Any symbol needing scope slot must have allocated it before this point.
                setPropertyIdForScopeSlotArray(sym->GetScopeSlot(), sym->EnsurePosition(funcInfo));
            }
        };

        funcInfo->GetBodyScope()->ForEachSymbol(setPropIdsForScopeSlotArray);

#if DEBUG
        for (UINT i = 0; i < scopeSlotCount; i++)
        {
            Assert(propertyIdsForScopeSlotArray[i] != Js::Constants::NoProperty
                || funcInfo->frameObjRegister != Js::Constants::NoRegister); // ScopeObject may have unassigned entries, e.g. for same-named parameters
        }
#endif
    }
}

// temporarily load all constants and special registers in a single block
void ByteCodeGenerator::LoadAllConstants(FuncInfo *funcInfo)
{
    Symbol *sym;

    Js::FunctionBody *byteCodeFunction = funcInfo->GetParsedFunctionBody();
    byteCodeFunction->CreateConstantTable();

    if (funcInfo->nullConstantRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->RecordNullObject(byteCodeFunction->MapRegSlot(funcInfo->nullConstantRegister));
    }

    if (funcInfo->undefinedConstantRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->RecordUndefinedObject(byteCodeFunction->MapRegSlot(funcInfo->undefinedConstantRegister));
    }

    if (funcInfo->trueConstantRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->RecordTrueObject(byteCodeFunction->MapRegSlot(funcInfo->trueConstantRegister));
    }

    if (funcInfo->falseConstantRegister != Js::Constants::NoRegister)
    {
        byteCodeFunction->RecordFalseObject(byteCodeFunction->MapRegSlot(funcInfo->falseConstantRegister));
    }

    if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
    {
        m_writer.RecordObjectRegister(funcInfo->frameObjRegister);
        if (!funcInfo->GetApplyEnclosesArgs())
        {
            this->EmitScopeObjectInit(funcInfo);
        }

#if DBG
        uint count = 0;
        funcInfo->GetBodyScope()->ForEachSymbol([&](Symbol *const sym)
        {
            if (sym->NeedsSlotAlloc(this, funcInfo))
            {
                // All properties should get correct propertyId here.
                count++;
            }
        });

        if (funcInfo->GetParamScope() != nullptr)
        {
            funcInfo->GetParamScope()->ForEachSymbol([&](Symbol *const sym)
            {
                if (sym->NeedsSlotAlloc(this, funcInfo))
                {
                    // All properties should get correct propertyId here.
                    count++;
                }
            });
        }

        // A reparse should result in the same size of the activation object.
        // Exclude functions which were created from the ByteCodeCache.
        AssertMsg(!byteCodeFunction->IsReparsed() || byteCodeFunction->HasGeneratedFromByteCodeCache() ||
            byteCodeFunction->scopeObjectSize == count || byteCodeFunction->WasEverAsmJsMode(),
            "The activation object size is different between debug and non-debug mode");
        byteCodeFunction->scopeObjectSize = count;
#endif
    }
    else if (funcInfo->frameSlotsRegister != Js::Constants::NoRegister)
    {
        int scopeSlotCount = funcInfo->bodyScope->GetScopeSlotCount();
        int paramSlotCount = funcInfo->paramScope->GetScopeSlotCount();
        if (scopeSlotCount == 0 && paramSlotCount == 0)
        {
            AssertMsg(funcInfo->frameDisplayRegister != Js::Constants::NoRegister, "Why do we need scope slots?");
            m_writer.Reg1(Js::OpCode::LdC_A_Null, funcInfo->frameSlotsRegister);
        }
    }

    if (funcInfo->funcExprScope && funcInfo->funcExprScope->GetIsObject())
    {
        byteCodeFunction->MapAndSetFuncExprScopeRegister(funcInfo->funcExprScope->GetLocation());
        byteCodeFunction->SetEnvDepth((uint16)-1);
    }

    bool thisLoadedFromParams = false;

    if (funcInfo->NeedEnvRegister())
    {
        byteCodeFunction->MapAndSetEnvRegister(funcInfo->GetEnvRegister());
        if (funcInfo->GetIsTopLevelEventHandler())
        {
            if (funcInfo->GetThisSymbol())
            {
            byteCodeFunction->MapAndSetThisRegisterForEventHandler(funcInfo->GetThisSymbol()->GetLocation());
            }
            // The environment is the namespace hierarchy starting with "this".
            Assert(!funcInfo->RegIsConst(funcInfo->GetEnvRegister()));
            thisLoadedFromParams = true;
            this->InvalidateCachedOuterScopes(funcInfo);
        }
        else if (funcInfo->IsGlobalFunction() && !(this->flags & fscrEval))
        {
            Assert(funcInfo->RegIsConst(funcInfo->GetEnvRegister()));

            if (funcInfo->GetIsStrictMode())
            {
                byteCodeFunction->RecordStrictNullDisplayConstant(byteCodeFunction->MapRegSlot(funcInfo->GetEnvRegister()));
            }
            else
            {
                byteCodeFunction->RecordNullDisplayConstant(byteCodeFunction->MapRegSlot(funcInfo->GetEnvRegister()));
            }
        }
        else
        {
            // environment may be required to load "this"
            Assert(!funcInfo->RegIsConst(funcInfo->GetEnvRegister()));
            this->InvalidateCachedOuterScopes(funcInfo);
        }
    }

    if (funcInfo->frameDisplayRegister != Js::Constants::NoRegister)
    {
        m_writer.RecordFrameDisplayRegister(funcInfo->frameDisplayRegister);
    }

    this->RecordAllIntConstants(funcInfo);
    this->RecordAllStrConstants(funcInfo);
    this->RecordAllStringTemplateCallsiteConstants(funcInfo);

    funcInfo->doubleConstantToRegister.Map([byteCodeFunction](double d, Js::RegSlot location)
    {
        byteCodeFunction->RecordFloatConstant(byteCodeFunction->MapRegSlot(location), d);
    });

    // WARNING !!!
    // DO NOT emit any bytecode before loading the heap arguments. This is because those opcodes may bail 
    // out (unlikely, since opcodes emitted in this function should not correspond to user code, but possible)
    // and the Jit assumes that there cannot be any bailouts before LdHeapArguments (or its equivalent)

    if (funcInfo->GetHasArguments())
    {
        sym = funcInfo->GetArgumentsSymbol();
        Assert(sym);
        Assert(funcInfo->GetHasHeapArguments());

        if (funcInfo->GetCallsEval() || (!funcInfo->GetApplyEnclosesArgs()))
        {
            this->LoadHeapArguments(funcInfo);
        }

    }
    else if (!funcInfo->IsGlobalFunction() && !IsInNonDebugMode())
    {
        uint count = funcInfo->inArgsCount + (funcInfo->root->pnodeRest != nullptr ? 1 : 0) - 1;
        if (count != 0)
        {
            Js::PropertyIdArray *propIds = RecyclerNewPlus(scriptContext->GetRecycler(), UInt32Math::Mul(count, sizeof(Js::PropertyId)), Js::PropertyIdArray, count, 0);

            GetFormalArgsArray(this, funcInfo, propIds);
            byteCodeFunction->SetPropertyIdsOfFormals(propIds);
        }
    }

    // Class constructors do not have a [[call]] slot but we don't implement a generic way to express this.
    // What we do is emit a check for the new flag here. If we don't have CallFlags_New set, the opcode will throw.
    // We need to do this before emitting 'this' since the base class constructor will try to construct a new object.
    if (funcInfo->IsClassConstructor())
    {
        m_writer.Empty(Js::OpCode::ChkNewCallFlag);
    }

    // new.target may be used to construct the 'this' register so make sure to load it first
    if (funcInfo->GetNewTargetSymbol())
    {
        this->LoadNewTargetObject(funcInfo);
    }

    if (funcInfo->GetThisSymbol())
    {
        this->LoadThisObject(funcInfo, thisLoadedFromParams);
    }
    else if (ShouldLoadConstThis(funcInfo))
    {
        this->EmitThis(funcInfo, funcInfo->thisConstantRegister, funcInfo->nullConstantRegister);
    }

    if (funcInfo->GetSuperSymbol())
    {
        this->LoadSuperObject(funcInfo);
    }

    if (funcInfo->GetSuperConstructorSymbol())
    {
        this->LoadSuperConstructorObject(funcInfo);
    }

    //
    // If the function is a function expression with a name,
    // load the function object at runtime to its activation object.
    //
    sym = funcInfo->root->GetFuncSymbol();
    bool funcExprWithName = !funcInfo->IsGlobalFunction() && sym && sym->GetIsFuncExpr();

    if (funcExprWithName)
    {
        if (funcInfo->GetFuncExprNameReference() ||
            (funcInfo->funcExprScope && funcInfo->funcExprScope->GetIsObject()))
        {
            //
            // x = function f(...) { ... }
            // A named function expression's name (Symbol:f) belongs to the enclosing scope.
            // Thus there are no uses of 'f' within the scope of the function (as references to 'f'
            // are looked up in the closure). So, we can't use f's register as it is from the enclosing
            // scope's register namespace. So use a tmp register.
            // In ES5 mode though 'f' is *not* a part of the enclosing scope. So we always assign 'f' a register
            // from it's register namespace, which LdFuncExpr can use.
            //
            Js::RegSlot ldFuncExprDst = sym->GetLocation();
            this->m_writer.Reg1(Js::OpCode::LdFuncExpr, ldFuncExprDst);

            if (sym->IsInSlot(this, funcInfo))
            {
                Js::RegSlot scopeLocation;
                AnalysisAssert(funcInfo->funcExprScope);

                if (funcInfo->funcExprScope->GetIsObject())
                {
                    scopeLocation = funcInfo->funcExprScope->GetLocation();
                    this->m_writer.Property(Js::OpCode::StFuncExpr, sym->GetLocation(), scopeLocation,
                        funcInfo->FindOrAddReferencedPropertyId(sym->GetPosition()));
                }
                else if (funcInfo->paramScope->GetIsObject() || (funcInfo->paramScope->GetCanMerge() && funcInfo->bodyScope->GetIsObject()))
                {
                    this->m_writer.ElementU(Js::OpCode::StLocalFuncExpr, sym->GetLocation(),
                        funcInfo->FindOrAddReferencedPropertyId(sym->GetPosition()));
                }
                else
                {
                    Assert(sym->HasScopeSlot());
                    this->m_writer.SlotI1(Js::OpCode::StLocalSlot, sym->GetLocation(),
                                          sym->GetScopeSlot() + Js::ScopeSlots::FirstSlotIndex);
                }
            }
            else if (ShouldTrackDebuggerMetadata())
            {
                funcInfo->byteCodeFunction->GetFunctionBody()->InsertSymbolToRegSlotList(sym->GetName(), sym->GetLocation(), funcInfo->varRegsCount);
            }
        }
    }
}

void ByteCodeGenerator::InvalidateCachedOuterScopes(FuncInfo *funcInfo)
{
    Assert(funcInfo->GetEnvRegister() != Js::Constants::NoRegister);

    // Walk the scope stack, from funcInfo outward, looking for scopes that have been cached.

    Scope *scope = funcInfo->GetBodyScope()->GetEnclosingScope();
    uint32 envIndex = 0;

    while (scope && scope->GetFunc() == funcInfo)
    {
        // Skip over FuncExpr Scope and parameter scope for current funcInfo to get to the first enclosing scope of the outer function.
        scope = scope->GetEnclosingScope();
    }

    for (; scope; scope = scope->GetEnclosingScope())
    {
        FuncInfo *func = scope->GetFunc();
        if (scope == func->GetBodyScope())
        {
            if (func->Escapes() && func->GetHasCachedScope())
            {
                AssertOrFailFast(scope->GetIsObject());
                this->m_writer.Unsigned1(Js::OpCode::InvalCachedScope, envIndex);
            }
        }
        if (scope->GetMustInstantiate())
        {
            envIndex++;
        }
    }
}

void ByteCodeGenerator::LoadThisObject(FuncInfo *funcInfo, bool thisLoadedFromParams)
{
    Symbol* thisSym = funcInfo->GetThisSymbol();
    Assert(thisSym);
    Assert(!funcInfo->IsLambda());

    if (this->scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled() && funcInfo->IsClassConstructor())
    {
        // Derived class constructors initialize 'this' to be Undecl
        //   - we'll check this value during a super call and during 'this' access
        //
        // Base class constructors initialize 'this' to a new object using new.target
        if (funcInfo->IsBaseClassConstructor())
        {
            Symbol* newTargetSym = funcInfo->GetNewTargetSymbol();
            Assert(newTargetSym);

            this->Writer()->Reg2(Js::OpCode::NewScObjectNoCtorFull, thisSym->GetLocation(), newTargetSym->GetLocation());
        }
        else
        {
            this->m_writer.Reg1(Js::OpCode::InitUndecl, thisSym->GetLocation());
        }
    }
    else if (!funcInfo->IsGlobalFunction())
    {
        //
        // thisLoadedFromParams would be true for the event Handler case,
        // "this" would have been loaded from parameters to put in the environment
        //
        if (!thisLoadedFromParams)
        {
            Js::RegSlot tmpReg = funcInfo->AcquireTmpRegister();
            m_writer.ArgIn0(tmpReg);
            EmitThis(funcInfo, thisSym->GetLocation(), tmpReg);
            funcInfo->ReleaseTmpRegister(tmpReg);
        }
        else
        {
            EmitThis(funcInfo, thisSym->GetLocation(), thisSym->GetLocation());
        }
    }
    else
    {
        Assert(funcInfo->IsGlobalFunction());
        Js::RegSlot root = funcInfo->nullConstantRegister;
        EmitThis(funcInfo, thisSym->GetLocation(), root);
    }
}

void ByteCodeGenerator::LoadNewTargetObject(FuncInfo *funcInfo)
{
    Symbol* newTargetSym = funcInfo->GetNewTargetSymbol();
    Assert(newTargetSym);

    if (funcInfo->IsClassConstructor())
    {
        Assert(!funcInfo->IsLambda());

        m_writer.ArgIn0(newTargetSym->GetLocation());
    }
    else if (funcInfo->IsGlobalFunction())
    {
        m_writer.Reg1(Js::OpCode::LdUndef, newTargetSym->GetLocation());
    }
    else
    {
        m_writer.Reg1(Js::OpCode::LdNewTarget, newTargetSym->GetLocation());
    }
}

void ByteCodeGenerator::LoadSuperConstructorObject(FuncInfo *funcInfo)
{
    Symbol* superConstructorSym = funcInfo->GetSuperConstructorSymbol();
    Assert(superConstructorSym);
    Assert(!funcInfo->IsLambda());

    if (funcInfo->IsDerivedClassConstructor())
    {
        m_writer.Reg1(Js::OpCode::LdFuncObj, superConstructorSym->GetLocation());
    }
    else
    {
        m_writer.Reg1(Js::OpCode::LdUndef, superConstructorSym->GetLocation());
    }
}

void ByteCodeGenerator::LoadSuperObject(FuncInfo *funcInfo)
{
    Symbol* superSym = funcInfo->GetSuperSymbol();
    Assert(superSym);
    Assert(!funcInfo->IsLambda());

    m_writer.Reg1(Js::OpCode::LdHomeObj, superSym->GetLocation());
}

void ByteCodeGenerator::EmitSuperCall(FuncInfo* funcInfo, ParseNodeSuperCall * pnodeSuperCall, BOOL fReturnValue)
{
    FuncInfo* nonLambdaFunc = funcInfo;
    bool isResultUsed = pnodeSuperCall->isUsed;

    if (funcInfo->IsLambda())
    {
        nonLambdaFunc = this->FindEnclosingNonLambda();
    }

    if (nonLambdaFunc->IsBaseClassConstructor())
    {
        // super() is not allowed in base class constructors. If we detect this, emit a ReferenceError and skip making the call.
        this->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_ClassSuperInBaseClass));
        return;
    }

    pnodeSuperCall->isUsed = true;

    // pnode->location refers to two things: the result of the inner function call (`temp` in the pseudocode below),
    // and the result of the super() expression itself
    funcInfo->AcquireLoc(pnodeSuperCall);

    // We need to emit 'this' directly so we can skip throwing a reference error if 'this' is currently undecl (we want to get undecl if 'this' is undecl)
    funcInfo->AcquireLoc(pnodeSuperCall->pnodeThis);
    EmitPropLoadThis(pnodeSuperCall->pnodeThis->location, pnodeSuperCall->pnodeThis, funcInfo, false);

    EmitLoad(pnodeSuperCall->pnodeNewTarget, this, funcInfo);

    Assert(pnodeSuperCall->isSuperCall);
    EmitLoad(pnodeSuperCall->pnodeTarget, this, funcInfo);

    //
    // if (super is class constructor) {
    //   _this = new.target;
    // } else {
    //   _this = NewScObjFull(new.target);
    // }
    //
    // temp = super.call(_this, new.target); // CallFlag_New | CallFlag_NewTarget | CallFlag_ExtraArg
    // if (temp is object) {
    //   _this = temp;
    // }
    //
    // if (UndeclBlockVar === this) {
    //   this = _this;
    // } else {
    //   throw ReferenceError;
    // }
    //
    Js::RegSlot thisForSuperCall = funcInfo->AcquireTmpRegister();
    Js::RegSlot valueForThis = funcInfo->AcquireTmpRegister();
    Js::RegSlot tmpUndeclReg = funcInfo->AcquireTmpRegister();

    Js::ByteCodeLabel useNewTargetForThisLabel = this->Writer()->DefineLabel();
    Js::ByteCodeLabel makeCallLabel = this->Writer()->DefineLabel();
    Js::ByteCodeLabel useSuperCallResultLabel = this->Writer()->DefineLabel();
    Js::ByteCodeLabel doneLabel = this->Writer()->DefineLabel();

    Js::RegSlot tmpReg = this->EmitLdObjProto(Js::OpCode::LdFuncObjProto, pnodeSuperCall->pnodeTarget->location, funcInfo);
    this->Writer()->BrReg1(Js::OpCode::BrOnClassConstructor, useNewTargetForThisLabel, tmpReg);
    this->Writer()->Reg2(Js::OpCode::NewScObjectNoCtorFull, thisForSuperCall, pnodeSuperCall->pnodeNewTarget->location);
    this->Writer()->Br(Js::OpCode::Br, makeCallLabel);
    this->Writer()->MarkLabel(useNewTargetForThisLabel);
    this->Writer()->Reg2(Js::OpCode::Ld_A, thisForSuperCall, pnodeSuperCall->pnodeNewTarget->location);
    this->Writer()->MarkLabel(makeCallLabel);
    EmitCall(pnodeSuperCall, this, funcInfo, fReturnValue, /*fEvaluateComponents*/ true, thisForSuperCall, pnodeSuperCall->pnodeNewTarget->location);

    // We have to use another temp for the this value before assigning to this register.
    // This is because IRBuilder does not expect us to use the value of a temp after potentially assigning to that same temp.
    // Ex:
    // _this = new.target;
    // temp = super.call(_this);
    // if (temp is object) {
    //   _this = temp; // creates a new sym for _this as it was previously used
    // }
    // this = _this; // tries to loads a value from the old sym (which is dead)

    this->Writer()->BrReg1(Js::OpCode::BrOnObject_A, useSuperCallResultLabel, pnodeSuperCall->location);
    this->Writer()->Reg2(Js::OpCode::Ld_A, valueForThis, thisForSuperCall);
    this->Writer()->Br(Js::OpCode::Br, doneLabel);
    this->Writer()->MarkLabel(useSuperCallResultLabel);
    this->Writer()->Reg2(Js::OpCode::Ld_A, valueForThis, pnodeSuperCall->location);
    this->Writer()->MarkLabel(doneLabel);

    // The call is done and we know what we will bind to 'this' so let's check to see if 'this' is already decl.
    Js::ByteCodeLabel skipLabel = this->Writer()->DefineLabel();
    this->Writer()->Reg1(Js::OpCode::InitUndecl, tmpUndeclReg);
    this->Writer()->BrReg2(Js::OpCode::BrSrEq_A, skipLabel, pnodeSuperCall->pnodeThis->location, tmpUndeclReg);
    this->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_ClassThisAlreadyAssigned));
    this->Writer()->MarkLabel(skipLabel);

    // If calling code cares about the return value, then move the selected `this` value into the result register.
    if (isResultUsed)
    {
        this->Writer()->Reg2(Js::OpCode::Ld_A, pnodeSuperCall->location, valueForThis);
    }

    Symbol* thisSym = pnodeSuperCall->pnodeThis->sym;
    this->Writer()->Reg2(Js::OpCode::StrictLdThis, pnodeSuperCall->pnodeThis->location, valueForThis);

    EmitPropStoreForSpecialSymbol(pnodeSuperCall->pnodeThis->location, thisSym, pnodeSuperCall->pnodeThis->pid, funcInfo, false);

    funcInfo->ReleaseTmpRegister(tmpUndeclReg);
    funcInfo->ReleaseTmpRegister(valueForThis);
    funcInfo->ReleaseTmpRegister(thisForSuperCall);

    funcInfo->ReleaseLoc(pnodeSuperCall->pnodeTarget);
    funcInfo->ReleaseLoc(pnodeSuperCall->pnodeNewTarget);
    funcInfo->ReleaseLoc(pnodeSuperCall->pnodeThis);
}

void ByteCodeGenerator::EmitClassConstructorEndCode(FuncInfo *funcInfo)
{
    Symbol* thisSym = funcInfo->GetThisSymbol();

    if (thisSym && thisSym->GetLocation() != Js::Constants::NoRegister)
    {
        EmitPropLoad(ByteCodeGenerator::ReturnRegister, thisSym, thisSym->GetPid(), funcInfo, true);
        this->m_writer.Reg1(Js::OpCode::ChkUndecl, ByteCodeGenerator::ReturnRegister);
    }
}

void ByteCodeGenerator::EmitThis(FuncInfo *funcInfo, Js::RegSlot lhsLocation, Js::RegSlot fromRegister)
{
    if (funcInfo->byteCodeFunction->GetIsStrictMode() && !funcInfo->IsGlobalFunction() && !funcInfo->IsLambda())
    {
        m_writer.Reg2(Js::OpCode::StrictLdThis, lhsLocation, fromRegister);
    }
    else
    {
        m_writer.Reg2Int1(Js::OpCode::LdThis, lhsLocation, fromRegister, this->GetModuleID());
    }
}

void ByteCodeGenerator::EmitLoadFormalIntoRegister(ParseNode *pnodeFormal, Js::RegSlot pos, FuncInfo *funcInfo)
{
    if (pnodeFormal->IsVarLetOrConst())
    {
        // Get the param from its argument position into its assigned register.
        // The position should match the location, otherwise, it has been shadowed by parameter with the same name
        Symbol *formal = pnodeFormal->AsParseNodeVar()->sym;
        if (formal->GetLocation() + 1 == pos)
        {
            // Transfer to the frame object, etc., if necessary.
            this->EmitLocalPropInit(formal->GetLocation(), formal, funcInfo);
        }
    }
}

void ByteCodeGenerator::HomeArguments(FuncInfo *funcInfo)
{
    if (ShouldTrackDebuggerMetadata())
    {
        // Add formals to the debugger propertyidcontainer for reg slots
        auto addFormalsToPropertyIdContainer = [this, funcInfo](ParseNode *pnodeFormal)
        {
            if (pnodeFormal->IsVarLetOrConst())
            {
                Symbol* formal = pnodeFormal->AsParseNodeVar()->sym;
                if (!formal->IsInSlot(this, funcInfo))
                {
                    Assert(!formal->GetHasInit());
                    funcInfo->GetParsedFunctionBody()->InsertSymbolToRegSlotList(formal->GetName(), formal->GetLocation(), funcInfo->varRegsCount);
                }
            }
        };

        MapFormals(funcInfo->root, addFormalsToPropertyIdContainer);
    }

    // Transfer formal parameters to their home locations on the local frame.
    if (funcInfo->GetHasArguments())
    {
        if (funcInfo->root->pnodeRest != nullptr)
        {
            // Since we don't have to iterate over arguments here, we'll trust the location to be correct.
            EmitLoadFormalIntoRegister(funcInfo->root->pnodeRest, funcInfo->root->pnodeRest->sym->GetLocation() + 1, funcInfo);
        }

        // The arguments object creation helper does this work for us.
        return;
    }

    Js::ArgSlot pos = 1;
    auto loadFormal = [&](ParseNode *pnodeFormal)
    {
        EmitLoadFormalIntoRegister(pnodeFormal, pos, funcInfo);
        pos++;
    };
    MapFormals(funcInfo->root, loadFormal);
}

void ByteCodeGenerator::DefineLabels(FuncInfo *funcInfo)
{
    funcInfo->singleExit = m_writer.DefineLabel();
    SList<ParseNodeStmt *>::Iterator iter(&funcInfo->targetStatements);
    while (iter.Next())
    {
        ParseNodeStmt * node = iter.Data();
        node->breakLabel = m_writer.DefineLabel();
        node->continueLabel = m_writer.DefineLabel();
        node->emitLabels = true;
    }
}

void ByteCodeGenerator::EmitGlobalBody(FuncInfo *funcInfo)
{
    // Emit global code (global scope or eval), fixing up the return register with the implicit
    // return value.
    ParseNode *pnode = funcInfo->root->pnodeBody;
    ParseNode *pnodeLastVal = funcInfo->root->AsParseNodeProg()->pnodeLastValStmt;
    if (pnodeLastVal == nullptr || pnodeLastVal->IsPatternDeclaration())
    {
        // We're not guaranteed to compute any values, so fix up the return register at the top
        // in case.
        this->m_writer.Reg1(Js::OpCode::LdUndef, ReturnRegister);
    }

    while (pnode->nop == knopList)
    {
        ParseNode *stmt = pnode->AsParseNodeBin()->pnode1;
        if (stmt == pnodeLastVal)
        {
            pnodeLastVal = nullptr;
        }
        if (pnodeLastVal == nullptr && (this->flags & fscrReturnExpression))
        {
            EmitTopLevelStatement(stmt, funcInfo, true);
        }
        else
        {
            // Haven't hit the post-dominating return value yet,
            // so don't bother with the return register.
            EmitTopLevelStatement(stmt, funcInfo, false);
        }
        pnode = pnode->AsParseNodeBin()->pnode2;
    }
    EmitTopLevelStatement(pnode, funcInfo, false);
}

void ByteCodeGenerator::EmitFunctionBody(FuncInfo *funcInfo)
{
    // Emit a function body. Only explicit returns and the implicit "undef" at the bottom
    // get copied to the return register.
    ParseNode *pnodeBody = funcInfo->root->pnodeBody;
    ParseNode *pnode = pnodeBody;
    while (pnode->nop == knopList)
    {
        ParseNode *stmt = pnode->AsParseNodeBin()->pnode1;
        if (stmt->CapturesSyms())
        {
            CapturedSymMap *map = funcInfo->EnsureCapturedSymMap();
            SList<Symbol*> *list = map->Item(stmt);
            FOREACH_SLIST_ENTRY(Symbol*, sym, list)
            {
                if (!sym->GetIsCommittedToSlot())
                {
                    Assert(sym->GetLocation() != Js::Constants::NoProperty);
                    sym->SetIsCommittedToSlot();
                    ParseNode *decl = sym->GetDecl();
                    Assert(decl);
                    if (PHASE_TRACE(Js::DelayCapturePhase, funcInfo->byteCodeFunction))
                    {
                        Output::Print(_u("--- DelayCapture: Committed symbol '%s' to slot.\n"),
                            sym->GetName().GetBuffer());
                        Output::Flush();
                    }
                    // REVIEW[ianhall]: HACK to work around this causing an error due to sym not yet being initialized
                    // what is this doing? Why are we assigning sym to itself?
                    bool old = sym->GetNeedDeclaration();
                    sym->SetNeedDeclaration(false);
                    this->EmitPropStore(sym->GetLocation(), sym, sym->GetPid(), funcInfo, decl->nop == knopLetDecl, decl->nop == knopConstDecl);
                    sym->SetNeedDeclaration(old);
                }
            }
            NEXT_SLIST_ENTRY;
        }
        EmitTopLevelStatement(stmt, funcInfo, false);
        pnode = pnode->AsParseNodeBin()->pnode2;
    }
    Assert(!pnode->CapturesSyms());
    EmitTopLevelStatement(pnode, funcInfo, false);
}

void ByteCodeGenerator::EmitProgram(ParseNodeProg *pnodeProg)
{
    // Indicate that the binding phase is over.
    this->isBinding = false;
    this->trackEnvDepth = true;
    AssignPropertyIds(pnodeProg->funcInfo->byteCodeFunction);

    int32 initSize = this->maxAstSize / AstBytecodeRatioEstimate;

    // Use the temp allocator in bytecode write temp buffer.
    m_writer.InitData(this->alloc, initSize);

#ifdef LOG_BYTECODE_AST_RATIO
    // log the max Ast size
    Output::Print(_u("Max Ast size: %d"), initSize);
#endif

    Assert(pnodeProg && pnodeProg->nop == knopProg);

    if (this->parentScopeInfo)
    {
        // Scope stack is already set up the way we want it, so don't visit the global scope.
        // Start emitting with the nested scope (i.e., the deferred function).
        this->EmitScopeList(pnodeProg->pnodeScopes);
    }
    else
    {
        this->EmitScopeList(pnodeProg);
    }
}

void EmitDestructuredObject(ParseNode *lhs, Js::RegSlot rhsLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);
void EmitDestructuredValueOrInitializer(ParseNodePtr lhsElementNode, Js::RegSlot rhsLocation, ParseNodePtr initializer, bool isNonPatternAssignmentTarget, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo);

void ByteCodeGenerator::PopulateFormalsScope(uint beginOffset, FuncInfo *funcInfo, ParseNodeFnc *pnodeFnc)
{
    Js::DebuggerScope *debuggerScope = nullptr;
    auto processArg = [&](ParseNode *pnodeArg) {
        if (pnodeArg->IsVarLetOrConst())
        {
            if (debuggerScope == nullptr)
            {
                debuggerScope = RecordStartScopeObject(pnodeFnc, funcInfo->paramScope && funcInfo->paramScope->GetIsObject() ? Js::DiagParamScopeInObject : Js::DiagParamScope);
                debuggerScope->SetBegin(beginOffset);
            }

            InsertPropertyToDebuggerScope(funcInfo, debuggerScope, pnodeArg->AsParseNodeVar()->sym);
        }
    };

    MapFormals(pnodeFnc, processArg);
    MapFormalsFromPattern(pnodeFnc, processArg);

    if (debuggerScope != nullptr)
    {
        if (!funcInfo->GetParsedFunctionBody()->IsParamAndBodyScopeMerged())
        {
            InsertPropertyToDebuggerScope(funcInfo, debuggerScope, funcInfo->GetArgumentsSymbol());
        }

        RecordEndScopeObject(pnodeFnc);
    }
}

void ByteCodeGenerator::InsertPropertyToDebuggerScope(FuncInfo* funcInfo, Js::DebuggerScope* debuggerScope, Symbol* sym)
{
    if (sym)
    {
        Js::FunctionBody* funcBody = funcInfo->GetParsedFunctionBody();
        Js::DebuggerScopePropertyFlags flag = Js::DebuggerScopePropertyFlags_None;
        Js::RegSlot location = sym->GetLocation();
        if (ShouldTrackDebuggerMetadata() && !funcInfo->IsBodyAndParamScopeMerged() && funcInfo->bodyScope->FindLocalSymbol(sym->GetName()) != nullptr)
        {
            flag |= Js::DebuggerScopePropertyFlags_HasDuplicateInBody;
            location = funcBody->MapRegSlot(location);
        }

        debuggerScope->AddProperty(location, sym->EnsurePosition(funcInfo), flag);
    }
}

void ByteCodeGenerator::EmitDefaultArgs(FuncInfo *funcInfo, ParseNodeFnc *pnodeFnc)
{
    uint beginOffset = m_writer.GetCurrentOffset();

    auto emitDefaultArg = [&](ParseNode *pnodeArg)
    {
        if (pnodeArg->nop == knopParamPattern)
        {
            this->StartStatement(pnodeArg);

            Assert(pnodeArg->AsParseNodeParamPattern()->location != Js::Constants::NoRegister);
            ParseNodePtr pnode1 = pnodeArg->AsParseNodeParamPattern()->pnode1;

            if (pnode1->IsPattern())
            {
                EmitAssignment(nullptr, pnode1, pnodeArg->AsParseNodeParamPattern()->location, this, funcInfo);
            }
            else
            {
                Assert(pnode1->nop == knopAsg);
                Assert(pnode1->AsParseNodeBin()->pnode1->IsPattern());
                EmitDestructuredValueOrInitializer(pnode1->AsParseNodeBin()->pnode1,
                    pnodeArg->AsParseNodeParamPattern()->location,
                    pnode1->AsParseNodeBin()->pnode2,
                    false /*isNonPatternAssignmentTarget*/,
                    this,
                    funcInfo);
            }
            this->EndStatement(pnodeArg);
            return;
        }
        else if (pnodeArg->IsVarLetOrConst())
        {
            Js::RegSlot location = pnodeArg->AsParseNodeVar()->sym->GetLocation();

            if (pnodeArg->AsParseNodeVar()->pnodeInit == nullptr)
            {
                // Since the formal hasn't been initialized in LdLetHeapArguments, we'll initialize it here.
                pnodeArg->AsParseNodeVar()->sym->SetNeedDeclaration(false);
                EmitPropStore(location, pnodeArg->AsParseNodeVar()->sym, pnodeArg->AsParseNodeVar()->pid, funcInfo, true);

                return;
            }

            // Load the default argument if we got undefined, skip RHS evaluation otherwise.
            Js::ByteCodeLabel noDefaultLabel = this->m_writer.DefineLabel();
            Js::ByteCodeLabel endLabel = this->m_writer.DefineLabel();
            this->StartStatement(pnodeArg);
            // Let us use strict not equal to differentiate between null and undefined
            m_writer.BrReg2(Js::OpCode::BrSrNeq_A, noDefaultLabel, location, funcInfo->undefinedConstantRegister);

            Emit(pnodeArg->AsParseNodeVar()->pnodeInit, this, funcInfo, false);
            pnodeArg->AsParseNodeVar()->sym->SetNeedDeclaration(false); // After emit to prevent foo(a = a)

            if (funcInfo->GetHasArguments() && pnodeArg->AsParseNodeVar()->sym->IsInSlot(this, funcInfo))
            {
                EmitPropStore(pnodeArg->AsParseNodeVar()->pnodeInit->location, pnodeArg->AsParseNodeVar()->sym, pnodeArg->AsParseNodeVar()->pid, funcInfo, true);

                m_writer.Br(endLabel);
            }
            else
            {
                EmitAssignment(nullptr, pnodeArg, pnodeArg->AsParseNodeVar()->pnodeInit->location, this, funcInfo);
            }

            funcInfo->ReleaseLoc(pnodeArg->AsParseNodeVar()->pnodeInit);

            m_writer.MarkLabel(noDefaultLabel);

            if (funcInfo->GetHasArguments() && pnodeArg->AsParseNodeVar()->sym->IsInSlot(this, funcInfo))
            {
                EmitPropStore(location, pnodeArg->AsParseNodeVar()->sym, pnodeArg->AsParseNodeVar()->pid, funcInfo, true);

                m_writer.MarkLabel(endLabel);
            }

            this->EndStatement(pnodeArg);
        }
    };

    // If the function is async, we wrap the default arguments in a try catch and reject a Promise in case of error.
    if (pnodeFnc->IsAsync())
    {
        uint cacheId;
        Js::ByteCodeLabel catchLabel = m_writer.DefineLabel();
        Js::ByteCodeLabel doneLabel = m_writer.DefineLabel();
        Js::RegSlot catchArgLocation = funcInfo->AcquireTmpRegister();
        Js::RegSlot promiseLocation = funcInfo->AcquireTmpRegister();
        Js::RegSlot rejectLocation = funcInfo->AcquireTmpRegister();

        // try
        m_writer.RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ true);
        m_writer.Br(Js::OpCode::TryCatch, catchLabel);

        // Rest cannot have a default argument, so we ignore it.
        MapFormalsWithoutRest(pnodeFnc, emitDefaultArg);

        m_writer.RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ false);
        m_writer.Empty(Js::OpCode::Leave);
        m_writer.Br(doneLabel);

        // catch
        m_writer.MarkLabel(catchLabel);
        m_writer.Reg1(Js::OpCode::Catch, catchArgLocation);

        m_writer.RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ true);
        m_writer.Empty(Js::OpCode::Nop);

        // return Promise.reject(error);
        cacheId = funcInfo->FindOrAddRootObjectInlineCacheId(Js::PropertyIds::Promise, false, false);
        m_writer.PatchableRootProperty(Js::OpCode::LdRootFld, promiseLocation, cacheId, false, false);

        EmitInvoke(rejectLocation, promiseLocation, Js::PropertyIds::reject, this, funcInfo, catchArgLocation);

        m_writer.Reg2(Js::OpCode::Ld_A, ByteCodeGenerator::ReturnRegister, rejectLocation);

        m_writer.RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ false);
        m_writer.Empty(Js::OpCode::Leave);
        m_writer.Br(funcInfo->singleExit);
        m_writer.Empty(Js::OpCode::Leave);

        m_writer.MarkLabel(doneLabel);

        this->SetHasTry(true);

        funcInfo->ReleaseTmpRegister(rejectLocation);
        funcInfo->ReleaseTmpRegister(promiseLocation);
        funcInfo->ReleaseTmpRegister(catchArgLocation);
    }
    else
    {
        // Rest cannot have a default argument, so we ignore it.
        MapFormalsWithoutRest(pnodeFnc, emitDefaultArg);
    }

    if (m_writer.GetCurrentOffset() > beginOffset)
    {
        PopulateFormalsScope(beginOffset, funcInfo, pnodeFnc);
    }
}

void ByteCodeGenerator::EmitOneFunction(ParseNodeFnc *pnodeFnc)
{
    Assert(pnodeFnc && (pnodeFnc->nop == knopProg || pnodeFnc->nop == knopFncDecl));
    FuncInfo *funcInfo = pnodeFnc->funcInfo;
    Assert(funcInfo != nullptr);

    if (funcInfo->IsFakeGlobalFunction(this->flags))
    {
        return;
    }

    Js::ParseableFunctionInfo* deferParseFunction = funcInfo->byteCodeFunction;
    deferParseFunction->SetGrfscr(deferParseFunction->GetGrfscr() | (this->flags & ~fscrDeferredFncExpression));
    deferParseFunction->SetSourceInfo(this->GetCurrentSourceIndex(),
        funcInfo->root,
        !!(this->flags & fscrEvalCode),
        ((this->flags & fscrDynamicCode) && !(this->flags & fscrEvalCode)));

    deferParseFunction->SetInParamsCount(funcInfo->inArgsCount);
    if (pnodeFnc->HasDefaultArguments())
    {
        deferParseFunction->SetReportedInParamsCount(pnodeFnc->firstDefaultArg + 1);
    }
    else
    {
        deferParseFunction->SetReportedInParamsCount(funcInfo->inArgsCount);
    }

    // Note: Don't check the actual attributes on the functionInfo here, since CanDefer has been cleared while
    // we're generating byte code.
    if (deferParseFunction->IsDeferred() || funcInfo->canDefer)
    {
        Js::ScopeInfo::SaveEnclosingScopeInfo(this, funcInfo);
    }

    if (funcInfo->root->pnodeBody == nullptr)
    {
        if (!PHASE_OFF1(Js::SkipNestedDeferredPhase) && (this->GetFlags() & fscrCreateParserState) == fscrCreateParserState && deferParseFunction->GetCompileCount() == 0)
        {
            deferParseFunction->BuildDeferredStubs(funcInfo->root);
        }
        Assert(!deferParseFunction->IsFunctionBody() || deferParseFunction->GetFunctionBody()->GetByteCode() != nullptr);
        return;
    }

    Js::FunctionBody* byteCodeFunction = funcInfo->GetParsedFunctionBody();

    try
    {
        if (!funcInfo->IsGlobalFunction())
        {
            // Note: Do not set the stack nested func flag if the function has been redeferred and recompiled.
            // In that case the flag already has the value we want.
            if (CanStackNestedFunc(funcInfo, true) && byteCodeFunction->GetCompileCount() == 0)
            {
#if DBG
                byteCodeFunction->SetCanDoStackNestedFunc();
#endif
                if (funcInfo->root->astSize <= ParseNodeFnc::MaxStackClosureAST)
                {
                    byteCodeFunction->SetStackNestedFunc(true);
                }
            }
        }

        if (byteCodeFunction->DoStackNestedFunc())
        {
            uint nestedCount = byteCodeFunction->GetNestedCount();
            for (uint i = 0; i < nestedCount; i++)
            {
                Js::FunctionProxy * nested = byteCodeFunction->GetNestedFunctionProxy(i);
                if (nested->IsFunctionBody())
                {
                    nested->GetFunctionBody()->SetStackNestedFuncParent(byteCodeFunction->GetFunctionInfo());
                }
            }
        }

        if (byteCodeFunction->GetByteCode() != nullptr)
        {
            // Previously compiled function nested within a re-deferred and re-compiled function.
            return;
        }

        // Bug : 301517
        // In the debug mode the hasOnlyThis optimization needs to be disabled, since user can break in this function
        // and do operation on 'this' and its property, which may not be defined yet.
        if (funcInfo->root->HasOnlyThisStmts() && !IsInDebugMode())
        {
            byteCodeFunction->SetHasOnlyThisStmts(true);
        }

        if (byteCodeFunction->IsInlineApplyDisabled() || this->scriptContext->GetConfig()->IsNoNative())
        {
            if ((pnodeFnc->nop == knopFncDecl) && (funcInfo->GetHasHeapArguments()) && (!funcInfo->GetCallsEval()) && ApplyEnclosesArgs(pnodeFnc, this))
            {
                bool applyEnclosesArgs = true;
                for (ParseNode* pnodeVar = funcInfo->root->pnodeVars; pnodeVar; pnodeVar = pnodeVar->AsParseNodeVar()->pnodeNext)
                {
                    Symbol* sym = pnodeVar->AsParseNodeVar()->sym;
                    if (sym->GetSymbolType() == STVariable && !sym->IsArguments())
                    {
                        applyEnclosesArgs = false;
                        break;
                    }
                }
                auto constAndLetCheck = [](ParseNodeBlock *pnodeBlock, bool *applyEnclosesArgs)
                {
                    if (*applyEnclosesArgs)
                    {
                        for (auto lexvar = pnodeBlock->pnodeLexVars; lexvar; lexvar = lexvar->AsParseNodeVar()->pnodeNext)
                        {
                            Symbol* sym = lexvar->AsParseNodeVar()->sym;
                            if (sym->GetSymbolType() == STVariable && !sym->IsArguments())
                            {
                                *applyEnclosesArgs = false;
                                break;
                            }
                        }
                    }
                };
                constAndLetCheck(funcInfo->root->pnodeScopes, &applyEnclosesArgs);
                constAndLetCheck(funcInfo->root->pnodeBodyScope, &applyEnclosesArgs);
                funcInfo->SetApplyEnclosesArgs(applyEnclosesArgs);
            }
        }

        InitScopeSlotArray(funcInfo);
        FinalizeRegisters(funcInfo, byteCodeFunction);
        DebugOnly(Js::RegSlot firstTmpReg = funcInfo->varRegsCount);

        // Reserve temp registers for the inner scopes. We prefer temps because the JIT will then renumber them
        // and see different lifetimes. (Note that debug mode requires permanent registers. See FinalizeRegisters.)
        // Need to revisit the condition when enabling JitES6Generators.
        uint innerScopeCount = funcInfo->InnerScopeCount();
        if (!this->IsInDebugMode() && !byteCodeFunction->IsCoroutine())
        {
            byteCodeFunction->SetInnerScopeCount(innerScopeCount);
            if (innerScopeCount)
            {
                funcInfo->SetFirstInnerScopeReg(funcInfo->AcquireTmpRegister());
                for (uint i = 1; i < innerScopeCount; i++)
                {
                    funcInfo->AcquireTmpRegister();
                }
            }
        }

        funcInfo->inlineCacheMap = Anew(alloc, FuncInfo::InlineCacheMap,
            alloc,
            funcInfo->RegCount() // Pass the actual register count. // TODO: Check if we can reduce this count
            );
        funcInfo->rootObjectLoadInlineCacheMap = Anew(alloc, FuncInfo::RootObjectInlineCacheIdMap,
            alloc,
            10);
        funcInfo->rootObjectLoadMethodInlineCacheMap = Anew(alloc, FuncInfo::RootObjectInlineCacheIdMap,
            alloc,
            10);
        funcInfo->rootObjectStoreInlineCacheMap = Anew(alloc, FuncInfo::RootObjectInlineCacheIdMap,
            alloc,
            10);
        funcInfo->referencedPropertyIdToMapIndex = Anew(alloc, FuncInfo::RootObjectInlineCacheIdMap,
            alloc,
            10);

        byteCodeFunction->AllocateLiteralRegexArray();
        m_callSiteId = 0;
        m_writer.Begin(byteCodeFunction, alloc, this->DoJitLoopBodies(funcInfo), funcInfo->hasLoop, this->IsInDebugMode());
        this->PushFuncInfo(_u("EmitOneFunction"), funcInfo);

        this->inPrologue = true;

        Scope* paramScope = funcInfo->GetParamScope();
        Scope* bodyScope = funcInfo->GetBodyScope();

        // For now, emit all constant loads at top of function (should instead put in closest dominator of uses).
        LoadAllConstants(funcInfo);
        HomeArguments(funcInfo);

        if (!funcInfo->IsBodyAndParamScopeMerged())
        {
            byteCodeFunction->SetParamAndBodyScopeNotMerged();

            // Pop the body scope before emitting the default args
            PopScope();
            Assert(this->GetCurrentScope() == paramScope);
        }

        if (funcInfo->root->pnodeRest != nullptr)
        {
            byteCodeFunction->SetHasRestParameter();
        }

        if (funcInfo->IsGlobalFunction())
        {
            EnsureNoRedeclarations(pnodeFnc->pnodeScopes, funcInfo);
        }

        ::BeginEmitBlock(pnodeFnc->pnodeScopes, this, funcInfo);

        DefineLabels(funcInfo);

        // We need to emit the storage for special symbols before we emit the default arguments in case the default
        // argument expressions reference those special names.
        if (pnodeFnc->HasNonSimpleParameterList())
        {
            // If the param and body scope are merged, the special symbol vars are located in the body scope so we
            // need to walk over the var list.
            if (funcInfo->IsBodyAndParamScopeMerged())
            {
                for (ParseNodePtr pnodeVar = pnodeFnc->pnodeVars; pnodeVar; pnodeVar = pnodeVar->AsParseNodeVar()->pnodeNext)
                {
#if DBG
                    bool reachedEndOfSpecialSymbols = false;
#endif
                    Symbol* sym = pnodeVar->AsParseNodeVar()->sym;

                    if (sym != nullptr && sym->IsSpecialSymbol())
                    {
                        EmitPropStoreForSpecialSymbol(sym->GetLocation(), sym, sym->GetPid(), funcInfo, true);
                        if (ShouldTrackDebuggerMetadata() && !sym->IsInSlot(this, funcInfo))
                        {
                            byteCodeFunction->InsertSymbolToRegSlotList(sym->GetName(), sym->GetLocation(), funcInfo->varRegsCount);
                        }
                    }
                    else
                    {
#if DBG
                        reachedEndOfSpecialSymbols = true;
#else
                        // All of the special symbols exist at the beginning of the var list (parser guarantees this and debug build asserts this)
                        // so we can quit walking at the first non-special one we see.
                        break;
#endif
                    }

#if DBG
                    if (reachedEndOfSpecialSymbols)
                    {
                        Assert(sym == nullptr || !sym->IsSpecialSymbol());
                    }
#endif
                }
            }
            else
            {
                paramScope->ForEachSymbol([&](Symbol* sym) {
                    if (sym && sym->IsSpecialSymbol())
                    {
                        EmitPropStoreForSpecialSymbol(sym->GetLocation(), sym, sym->GetPid(), funcInfo, true);
                    }
                });
            }
        }

        if (pnodeFnc->HasNonSimpleParameterList() || !funcInfo->IsBodyAndParamScopeMerged())
        {
            Assert(pnodeFnc->HasNonSimpleParameterList() || CONFIG_FLAG(ForceSplitScope));

            this->InitBlockScopedNonTemps(funcInfo->root->pnodeScopes, funcInfo);

            EmitDefaultArgs(funcInfo, pnodeFnc);

            if (!funcInfo->IsBodyAndParamScopeMerged())
            {
                Assert(this->GetCurrentScope() == paramScope);
                // Push the body scope
                PushScope(bodyScope);

                funcInfo->SetCurrentChildScope(bodyScope);

                // Mark the beginning of the body scope so that new scope slots can be created.
                this->Writer()->Empty(Js::OpCode::BeginBodyScope);
            }
        }

        // If the function has non simple parameter list, the params needs to be evaluated when the generator object is created
        // (that is when the function is called). This yield opcode is to mark the  begining of the function body.
        // TODO: Inserting a yield should have almost no impact on perf as it is a direct return from the function. But this needs
        // to be verified. Ideally if the function has simple parameter list then we can avoid inserting the opcode and the additional call.
        if (pnodeFnc->IsGenerator())
        {
            Js::RegSlot tempReg = funcInfo->AcquireTmpRegister();
            EmitYield(funcInfo->AssignUndefinedConstRegister(), tempReg, this, funcInfo);
            m_writer.Reg1(Js::OpCode::Unused, tempReg);
            funcInfo->ReleaseTmpRegister(tempReg);
        }

        DefineUserVars(funcInfo);

        // Emit all scope-wide function definitions before emitting function bodies
        // so that calls may reference functions they precede lexically.
        // Note, global eval scope is a fake local scope and is handled as if it were
        // a lexical block instead of a true global scope, so do not define the functions
        // here. They will be defined during BeginEmitBlock.
        if (!(funcInfo->IsGlobalFunction() && this->IsEvalWithNoParentScopeInfo()))
        {
            // This only handles function declarations, which param scope cannot have any.
            DefineFunctions(funcInfo);
        }

        if (pnodeFnc->HasNonSimpleParameterList() || !funcInfo->IsBodyAndParamScopeMerged())
        {
            Assert(pnodeFnc->HasNonSimpleParameterList() || CONFIG_FLAG(ForceSplitScope));
            this->InitBlockScopedNonTemps(funcInfo->root->pnodeBodyScope, funcInfo);
        }
        else
        {
            this->InitBlockScopedNonTemps(funcInfo->root->pnodeScopes, funcInfo);
        }

        if (!pnodeFnc->HasNonSimpleParameterList() && funcInfo->GetHasArguments() && !NeedScopeObjectForArguments(funcInfo, pnodeFnc))
        {
            // If we didn't create a scope object and didn't have default args, we still need to transfer the formals to their slots.
            MapFormalsWithoutRest(pnodeFnc, [&](ParseNode *pnodeArg) { EmitPropStore(pnodeArg->AsParseNodeVar()->sym->GetLocation(), pnodeArg->AsParseNodeVar()->sym, pnodeArg->AsParseNodeVar()->pid, funcInfo); });
        }

        // Rest needs to trigger use before declaration until all default args have been processed.
        if (pnodeFnc->pnodeRest != nullptr)
        {
            pnodeFnc->pnodeRest->sym->SetNeedDeclaration(false);
        }

        Js::RegSlot formalsUpperBound = Js::Constants::NoRegister; // Needed for tracking the last RegSlot in the param scope
        if (!funcInfo->IsBodyAndParamScopeMerged())
        {
            // Emit bytecode to copy the initial values from param names to their corresponding body bindings.
            // We have to do this after the rest param is marked as false for need declaration.
            Symbol* funcSym = funcInfo->root->GetFuncSymbol();
            paramScope->ForEachSymbol([&](Symbol* param) {
                Symbol* varSym = funcInfo->GetBodyScope()->FindLocalSymbol(param->GetName());
                if ((funcSym == nullptr || funcSym != param)    // Do not copy the symbol over to body as the function expression symbol
                                                                // is expected to stay inside the function expression scope
                    && (varSym && varSym->GetSymbolType() == STVariable && (varSym->IsInSlot(this, funcInfo) || varSym->GetLocation() != Js::Constants::NoRegister)))
                {
                    if (!varSym->GetNeedDeclaration())
                    {
                        if (param->IsInSlot(this, funcInfo))
                        {
                            // Simulating EmitPropLoad here. We can't directly call the method as we have to use the param scope specifically.
                            // Walking the scope chain is not possible at this time.
                            Js::RegSlot tempReg = funcInfo->AcquireTmpRegister();
                            Js::PropertyId slot = param->EnsureScopeSlot(this, funcInfo);
                            Js::ProfileId profileId = funcInfo->FindOrAddSlotProfileId(paramScope, slot);
                            Js::OpCode op = paramScope->GetIsObject() ? Js::OpCode::LdParamObjSlot : Js::OpCode::LdParamSlot;
                            slot = slot + (paramScope->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex);

                            this->m_writer.SlotI1(op, tempReg, slot, profileId);

                            this->EmitPropStore(tempReg, varSym, varSym->GetPid(), funcInfo);
                            funcInfo->ReleaseTmpRegister(tempReg);
                        }
                        else if (param->GetLocation() != Js::Constants::NoRegister)
                        {
                            this->EmitPropStore(param->GetLocation(), varSym, varSym->GetPid(), funcInfo);
                        }
                        else
                        {
                            Assert(param->IsArguments() && !funcInfo->GetHasArguments());
                        }
                    }
                    else
                    {
                        // There is a let redeclaration of arguments symbol. Any other var will cause a
                        // re-declaration error.
                        Assert(param->IsArguments());
                    }
                }

                if (ShouldTrackDebuggerMetadata() && param->GetLocation() != Js::Constants::NoRegister)
                {
                    if (formalsUpperBound == Js::Constants::NoRegister || formalsUpperBound < param->GetLocation())
                    {
                        formalsUpperBound = param->GetLocation();
                    }
                }
            });
        }
        if (ShouldTrackDebuggerMetadata() && byteCodeFunction->GetPropertyIdOnRegSlotsContainer())
        {
            byteCodeFunction->GetPropertyIdOnRegSlotsContainer()->formalsUpperBound = formalsUpperBound;
        }

        if (pnodeFnc->pnodeBodyScope != nullptr)
        {
            ::BeginEmitBlock(pnodeFnc->pnodeBodyScope, this, funcInfo);
        }

        this->inPrologue = false;

        if (funcInfo->IsGlobalFunction())
        {
            EmitGlobalBody(funcInfo);
        }
        else
        {
            EmitFunctionBody(funcInfo);
        }

        if (pnodeFnc->pnodeBodyScope != nullptr)
        {
            ::EndEmitBlock(pnodeFnc->pnodeBodyScope, this, funcInfo);
        }
        ::EndEmitBlock(pnodeFnc->pnodeScopes, this, funcInfo);

        if (!this->IsInDebugMode())
        {
            // Release the temp registers that we reserved for inner scopes above.
            if (innerScopeCount)
            {
                Js::RegSlot tmpReg = funcInfo->FirstInnerScopeReg() + innerScopeCount - 1;
                for (uint i = 0; i < innerScopeCount; i++)
                {
                    funcInfo->ReleaseTmpRegister(tmpReg);
                    tmpReg--;
                }
            }
        }

        Assert(funcInfo->firstTmpReg == firstTmpReg);
        Assert(funcInfo->curTmpReg == firstTmpReg);
        Assert(byteCodeFunction->GetFirstTmpReg() == firstTmpReg + byteCodeFunction->GetConstantCount());

        byteCodeFunction->CheckAndSetVarCount(funcInfo->varRegsCount);
        byteCodeFunction->CheckAndSetOutParamMaxDepth(funcInfo->outArgsMaxDepth);
        byteCodeFunction->SetForInLoopDepth(funcInfo->GetMaxForInLoopLevel());

        // Do a uint32 add just to verify that we haven't overflowed the reg slot type.
        UInt32Math::Add(funcInfo->varRegsCount, funcInfo->constRegsCount);

#if DBG_DUMP
        if (PHASE_STATS1(Js::ByteCodePhase))
        {
            Output::Print(_u(" BCode: %-10d, Aux: %-10d, AuxC: %-10d Total: %-10d,  %s\n"),
                m_writer.ByteCodeDataSize(),
                m_writer.AuxiliaryDataSize(),
                m_writer.AuxiliaryContextDataSize(),
                m_writer.ByteCodeDataSize() + m_writer.AuxiliaryDataSize() + m_writer.AuxiliaryContextDataSize(),
                funcInfo->name);

            this->scriptContext->byteCodeDataSize += m_writer.ByteCodeDataSize();
            this->scriptContext->byteCodeAuxiliaryDataSize += m_writer.AuxiliaryDataSize();
            this->scriptContext->byteCodeAuxiliaryContextDataSize += m_writer.AuxiliaryContextDataSize();
        }
#endif

        this->MapCacheIdsToPropertyIds(funcInfo);
        this->MapReferencedPropertyIds(funcInfo);

        Assert(this->TopFuncInfo() == funcInfo);
        PopFuncInfo(_u("EmitOneFunction"));
        m_writer.SetCallSiteCount(m_callSiteId);
#ifdef LOG_BYTECODE_AST_RATIO
        m_writer.End(funcInfo->root->astSize, this->maxAstSize);
#else
        m_writer.End();
#endif
    }
    catch (...)
    {
        // Failed to generate byte-code for this function body (likely OOM or stack overflow). Notify the function body so that
        // it can revert intermediate state changes that may have taken place during byte code generation before the failure.
        byteCodeFunction->ResetByteCodeGenState();
        m_writer.Reset();
        throw;
    }

#ifdef PERF_HINT
    if (PHASE_TRACE1(Js::PerfHintPhase) && !byteCodeFunction->GetIsGlobalFunc())
    {
        if (byteCodeFunction->GetHasTry())
        {
            WritePerfHint(PerfHints::HasTryBlock_Verbose, byteCodeFunction);
        }

        if (funcInfo->GetCallsEval())
        {
            WritePerfHint(PerfHints::CallsEval_Verbose, byteCodeFunction);
        }
        else if (funcInfo->GetChildCallsEval())
        {
            WritePerfHint(PerfHints::ChildCallsEval, byteCodeFunction);
        }
    }
#endif

    if (!byteCodeFunction->GetSourceContextInfo()->IsDynamic() && byteCodeFunction->GetIsTopLevel() && !(this->flags & fscrEvalCode))
    {
        // Add the top level of nested functions to the tracking dictionary. Wait until this point so that all nested functions have gone
        // through the Emit API so source info, etc., is initialized, and these are not orphaned functions left behind by an unfinished pass.
        byteCodeFunction->ForEachNestedFunc([&](Js::FunctionProxy * nestedFunc, uint32 i)
        {
            if (nestedFunc && nestedFunc->IsDeferredParseFunction() && nestedFunc->GetParseableFunctionInfo()->GetIsDeclaration())
            {
                byteCodeFunction->GetUtf8SourceInfo()->TrackDeferredFunction(nestedFunc->GetLocalFunctionId(), nestedFunc->GetParseableFunctionInfo());
            }
            return true;
        });
    }

    byteCodeFunction->SetInitialDefaultEntryPoint();
    byteCodeFunction->SetCompileCount(UInt32Math::Add(byteCodeFunction->GetCompileCount(), 1));

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (byteCodeFunction->IsInDebugMode() != scriptContext->IsScriptContextInDebugMode()) // debug mode mismatch
    {
        if (m_utf8SourceInfo->GetIsLibraryCode())
        {
            Assert(!byteCodeFunction->IsInDebugMode()); // Library script byteCode is never in debug mode
        }
        else
        {
            Js::Throw::FatalInternalError();
        }
    }
#endif

#if DBG_DUMP
    if (PHASE_DUMP(Js::ByteCodePhase, funcInfo->byteCodeFunction) && Js::Configuration::Global.flags.Verbose)
    {
        pnodeFnc->Dump();
    }
    if (this->Trace() || PHASE_DUMP(Js::ByteCodePhase, funcInfo->byteCodeFunction))
    {
        Js::ByteCodeDumper::Dump(byteCodeFunction);
    }
    if (PHASE_DUMP(Js::DebuggerScopePhase, funcInfo->byteCodeFunction))
    {
        byteCodeFunction->DumpScopes();
    }
#endif
#if ENABLE_NATIVE_CODEGEN
    if ((!PHASE_OFF(Js::BackEndPhase, funcInfo->byteCodeFunction))
        && !this->forceNoNative
        && !this->scriptContext->GetConfig()->IsNoNative())
    {
        GenerateFunction(this->scriptContext->GetNativeCodeGenerator(), byteCodeFunction);
    }
#endif
}

void ByteCodeGenerator::MapCacheIdsToPropertyIds(FuncInfo *funcInfo)
{
    Js::FunctionBody *functionBody = funcInfo->GetParsedFunctionBody();
    uint rootObjectLoadInlineCacheStart = funcInfo->GetInlineCacheCount();
    uint rootObjectLoadMethodInlineCacheStart = rootObjectLoadInlineCacheStart + funcInfo->GetRootObjectLoadInlineCacheCount();
    uint rootObjectStoreInlineCacheStart = rootObjectLoadMethodInlineCacheStart + funcInfo->GetRootObjectLoadMethodInlineCacheCount();
    uint totalFieldAccessInlineCacheCount = rootObjectStoreInlineCacheStart + funcInfo->GetRootObjectStoreInlineCacheCount();

    functionBody->CreateCacheIdToPropertyIdMap(rootObjectLoadInlineCacheStart, rootObjectLoadMethodInlineCacheStart,
        rootObjectStoreInlineCacheStart, totalFieldAccessInlineCacheCount, funcInfo->GetIsInstInlineCacheCount());

    if (totalFieldAccessInlineCacheCount == 0)
    {
        return;
    }

    funcInfo->inlineCacheMap->Map([functionBody](Js::RegSlot regSlot, FuncInfo::InlineCacheIdMap *inlineCacheIdMap)
    {
        inlineCacheIdMap->Map([functionBody](Js::PropertyId propertyId, FuncInfo::InlineCacheList* inlineCacheList)
        {
            if (inlineCacheList)
            {
                inlineCacheList->Iterate([functionBody, propertyId](InlineCacheUnit cacheUnit)
                {
                    CompileAssert(offsetof(InlineCacheUnit, cacheId) == offsetof(InlineCacheUnit, loadCacheId));
                    if (cacheUnit.loadCacheId != -1)
                    {
                        functionBody->SetPropertyIdForCacheId(cacheUnit.loadCacheId, propertyId);
                    }
                    if (cacheUnit.loadMethodCacheId != -1)
                    {
                        functionBody->SetPropertyIdForCacheId(cacheUnit.loadMethodCacheId, propertyId);
                    }
                    if (cacheUnit.storeCacheId != -1)
                    {
                        functionBody->SetPropertyIdForCacheId(cacheUnit.storeCacheId, propertyId);
                    }
                });
            }
        });
    });

    funcInfo->rootObjectLoadInlineCacheMap->Map([functionBody, rootObjectLoadInlineCacheStart](Js::PropertyId propertyId, uint cacheId)
    {
        functionBody->SetPropertyIdForCacheId(cacheId + rootObjectLoadInlineCacheStart, propertyId);
    });
    funcInfo->rootObjectLoadMethodInlineCacheMap->Map([functionBody, rootObjectLoadMethodInlineCacheStart](Js::PropertyId propertyId, uint cacheId)
    {
        functionBody->SetPropertyIdForCacheId(cacheId + rootObjectLoadMethodInlineCacheStart, propertyId);
    });
    funcInfo->rootObjectStoreInlineCacheMap->Map([functionBody, rootObjectStoreInlineCacheStart](Js::PropertyId propertyId, uint cacheId)
    {
        functionBody->SetPropertyIdForCacheId(cacheId + rootObjectStoreInlineCacheStart, propertyId);
    });

    SListBase<uint>::Iterator valueOfIter(&funcInfo->valueOfStoreCacheIds);
    while (valueOfIter.Next())
    {
        functionBody->SetPropertyIdForCacheId(valueOfIter.Data(), Js::PropertyIds::valueOf);
    }

    SListBase<uint>::Iterator toStringIter(&funcInfo->toStringStoreCacheIds);
    while (toStringIter.Next())
    {
        functionBody->SetPropertyIdForCacheId(toStringIter.Data(), Js::PropertyIds::toString);
    }

#if DBG
    functionBody->VerifyCacheIdToPropertyIdMap();
#endif
}

void ByteCodeGenerator::MapReferencedPropertyIds(FuncInfo * funcInfo)
{
    Js::FunctionBody *functionBody = funcInfo->GetParsedFunctionBody();
    uint referencedPropertyIdCount = funcInfo->GetReferencedPropertyIdCount();
    functionBody->CreateReferencedPropertyIdMap(referencedPropertyIdCount);

    funcInfo->referencedPropertyIdToMapIndex->Map([functionBody](Js::PropertyId propertyId, uint mapIndex)
    {
        functionBody->SetReferencedPropertyIdWithMapIndex(mapIndex, propertyId);
    });

#if DBG
    functionBody->VerifyReferencedPropertyIdMap();
#endif
}

void ByteCodeGenerator::EmitScopeList(ParseNode *pnode, ParseNode *breakOnBodyScopeNode)
{
    while (pnode)
    {
        if (breakOnBodyScopeNode != nullptr && breakOnBodyScopeNode == pnode)
        {
            break;
        }

        switch (pnode->nop)
        {
        case knopFncDecl:
#ifdef ASMJS_PLAT
            if (pnode->AsParseNodeFnc()->GetAsmjsMode())
            {
                Js::ExclusiveContext context(this, GetScriptContext());
                if (Js::AsmJSCompiler::Compile(&context, pnode->AsParseNodeFnc(), pnode->AsParseNodeFnc()->pnodeParams))
                {
                    pnode = pnode->AsParseNodeFnc()->pnodeNext;
                    break;
                }
                else if (CONFIG_FLAG(AsmJsStopOnError))
                {
                    exit(JSERR_AsmJsCompileError);
                }
                else
                {
                    // If deferral is not allowed, throw and reparse everything with asm.js disabled.
                    throw Js::AsmJsParseException();
                }
            }
#endif
            // FALLTHROUGH
        case knopProg:
            if (pnode->AsParseNodeFnc()->funcInfo)
            {
                FuncInfo* funcInfo = pnode->AsParseNodeFnc()->funcInfo;
                Scope* paramScope = funcInfo->GetParamScope();

                if (!funcInfo->IsBodyAndParamScopeMerged())
                {
                    funcInfo->SetCurrentChildScope(paramScope);
                }
                else
                {
                    funcInfo->SetCurrentChildScope(funcInfo->GetBodyScope());
                }
                this->StartEmitFunction(pnode->AsParseNodeFnc());

                PushFuncInfo(_u("StartEmitFunction"), funcInfo);

                if (!funcInfo->IsBodyAndParamScopeMerged())
                {
                    this->EmitScopeList(pnode->AsParseNodeFnc()->pnodeBodyScope->pnodeScopes);
                }
                else
                {
                    this->EmitScopeList(pnode->AsParseNodeFnc()->pnodeScopes);
                }

                this->EmitOneFunction(pnode->AsParseNodeFnc());
                this->EndEmitFunction(pnode->AsParseNodeFnc());

                Assert(pnode->AsParseNodeFnc()->pnodeBody == nullptr || funcInfo->isReused || funcInfo->GetCurrentChildScope() == funcInfo->GetBodyScope());
                funcInfo->SetCurrentChildScope(nullptr);
            }
            pnode = pnode->AsParseNodeFnc()->pnodeNext;
            break;

        case knopBlock:
        {
            ParseNodeBlock * pnodeBlock = pnode->AsParseNodeBlock();
            this->StartEmitBlock(pnodeBlock);
            this->EmitScopeList(pnodeBlock->pnodeScopes);
            this->EndEmitBlock(pnodeBlock);
            pnode = pnodeBlock->pnodeNext;
            break;
        }
        case knopCatch:
        {
            ParseNodeCatch * pnodeCatch = pnode->AsParseNodeCatch();
            this->StartEmitCatch(pnodeCatch);
            this->EmitScopeList(pnodeCatch->pnodeScopes);
            this->EndEmitCatch(pnodeCatch);
            pnode = pnodeCatch->pnodeNext;
            break;
        }

        case knopWith:
            this->StartEmitWith(pnode);
            this->EmitScopeList(pnode->AsParseNodeWith()->pnodeScopes);
            this->EndEmitWith(pnode);
            pnode = pnode->AsParseNodeWith()->pnodeNext;
            break;

        default:
            AssertMsg(false, "Unexpected opcode in tree of scopes");
            break;
        }
    }
}

void ByteCodeGenerator::EnsureFncDeclScopeSlot(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfo)
{
    if (pnodeFnc->pnodeName)
    {
        Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
        Symbol *sym = pnodeFnc->pnodeName->sym;
        // If this function is shadowing the arguments symbol in body then skip it.
        // We will allocate scope slot for the arguments symbol during EmitLocalPropInit.
        if (sym && !sym->IsArguments())
        {
            sym->EnsureScopeSlot(this, funcInfo);
        }
    }
}

// Similar to EnsureFncScopeSlot visitor function, but verifies that a slot is needed before assigning it.
void ByteCodeGenerator::CheckFncDeclScopeSlot(ParseNodeFnc *pnodeFnc, FuncInfo *funcInfo)
{
    if (pnodeFnc->pnodeName)
    {
        Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
        Symbol *sym = pnodeFnc->pnodeName->sym;
        if (sym && sym->NeedsSlotAlloc(this, funcInfo))
        {
            sym->EnsureScopeSlot(this, funcInfo);
        }
    }
}

void ByteCodeGenerator::StartEmitFunction(ParseNodeFnc *pnodeFnc)
{
    Assert(pnodeFnc->nop == knopFncDecl || pnodeFnc->nop == knopProg);

    FuncInfo *funcInfo = pnodeFnc->funcInfo;
    Scope * const bodyScope = funcInfo->GetBodyScope();
    Scope * const paramScope = funcInfo->GetParamScope();

    if (funcInfo->byteCodeFunction->IsFunctionParsed() && funcInfo->root->pnodeBody != nullptr)
    {
        if (funcInfo->GetParsedFunctionBody()->GetByteCode() == nullptr && !(flags & (fscrEval | fscrImplicitThis)))
        {
            // Only set the environment depth if it's truly known (i.e., not in eval or event handler).
            funcInfo->GetParsedFunctionBody()->SetEnvDepth(this->envDepth);
        }

        if (funcInfo->GetCallsEval())
        {
            funcInfo->byteCodeFunction->SetDontInline(true);
        }

        Scope * const funcExprScope = funcInfo->funcExprScope;
        if (funcExprScope)
        {
            if (funcInfo->GetCallsEval())
            {
                Assert(funcExprScope->GetIsObject());
            }

            if (funcExprScope->GetIsObject())
            {
                funcExprScope->SetCapturesAll(true);
                funcExprScope->SetMustInstantiate(true);
                PushScope(funcExprScope);
            }
            else
            {
                Symbol *sym = funcInfo->root->GetFuncSymbol();
                if (funcInfo->IsBodyAndParamScopeMerged())
                {
                    funcInfo->bodyScope->AddSymbol(sym);
                }
                else
                {
                    funcInfo->paramScope->AddSymbol(sym);
                }
                sym->EnsureScopeSlot(this, funcInfo);
                if (sym->GetHasNonLocalReference())
                {
                    sym->GetScope()->SetHasOwnLocalInClosure(true);
                }
            }
        }

        if (pnodeFnc->nop != knopProg)
        {
            if (!bodyScope->GetIsObject() && NeedObjectAsFunctionScope(funcInfo, pnodeFnc))
            {
                Assert(bodyScope->GetIsObject());
            }

            if (bodyScope->GetIsObject())
            {
                bodyScope->SetLocation(funcInfo->frameObjRegister);
            }
            else
            {
                bodyScope->SetLocation(funcInfo->frameSlotsRegister);
            }

            if (!funcInfo->IsBodyAndParamScopeMerged())
            {
                if (paramScope->GetIsObject())
                {
                    paramScope->SetLocation(funcInfo->frameObjRegister);
                }
                else
                {
                    paramScope->SetLocation(funcInfo->frameSlotsRegister);
                }
            }

            if (bodyScope->GetIsObject())
            {
                // Win8 908700: Disable under F12 debugger because there are too many cached scopes holding onto locals.
                funcInfo->SetHasCachedScope(
                    !PHASE_OFF(Js::CachedScopePhase, funcInfo->byteCodeFunction) &&
                    !funcInfo->Escapes() &&
                    funcInfo->frameObjRegister != Js::Constants::NoRegister &&
                    !ApplyEnclosesArgs(pnodeFnc, this) &&
                    funcInfo->IsBodyAndParamScopeMerged() && // There is eval in the param scope
                    !pnodeFnc->HasDefaultArguments() &&
                    !pnodeFnc->HasDestructuredParams() &&
                    (PHASE_FORCE(Js::CachedScopePhase, funcInfo->byteCodeFunction) || !IsInDebugMode())
#if ENABLE_TTD
                    && !funcInfo->GetParsedFunctionBody()->GetScriptContext()->GetThreadContext()->IsRuntimeInTTDMode()
#endif
                    && !funcInfo->byteCodeFunction->IsCoroutine()
                );

                if (funcInfo->GetHasCachedScope())
                {
                    Assert(funcInfo->funcObjRegister == Js::Constants::NoRegister);
                    Symbol *funcSym = funcInfo->root->GetFuncSymbol();
                    if (funcSym && funcSym->GetIsFuncExpr())
                    {
                        if (funcSym->GetLocation() == Js::Constants::NoRegister)
                        {
                            funcInfo->funcObjRegister = funcInfo->NextVarRegister();
                        }
                        else
                        {
                            funcInfo->funcObjRegister = funcSym->GetLocation();
                        }
                    }
                    else
                    {
                        funcInfo->funcObjRegister = funcInfo->NextVarRegister();
                    }
                    Assert(funcInfo->funcObjRegister != Js::Constants::NoRegister);
                }

                ParseNode *pnode;
                Symbol *sym;

                if (funcInfo->GetHasArguments())
                {
                    // Process function's formal parameters
                    MapFormals(pnodeFnc, [&](ParseNode *pnode)
                    {
                        if (pnode->IsVarLetOrConst())
                        {
                            pnode->AsParseNodeVar()->sym->EnsureScopeSlot(this, funcInfo);
                        }
                    });

                    MapFormalsFromPattern(pnodeFnc, [&](ParseNode *pnode) { pnode->AsParseNodeVar()->sym->EnsureScopeSlot(this, funcInfo); });

                    // Only allocate scope slot for "arguments" when really necessary. "hasDeferredChild"
                    // doesn't require scope slot for "arguments" because inner functions can't access
                    // outer function's arguments directly.
                    sym = funcInfo->GetArgumentsSymbol();
                    Assert(sym);
                    if (sym->NeedsSlotAlloc(this, funcInfo))
                    {
                        sym->EnsureScopeSlot(this, funcInfo);
                    }
                }

                sym = funcInfo->root->GetFuncSymbol();

                if (sym && sym->NeedsSlotAlloc(this, funcInfo))
                {
                    if (funcInfo->funcExprScope && funcInfo->funcExprScope->GetIsObject())
                    {
                        sym->SetScopeSlot(0);
                    }
                    else if (funcInfo->GetFuncExprNameReference())
                    {
                        sym->EnsureScopeSlot(this, funcInfo);
                    }
                }

                if (!funcInfo->GetHasArguments())
                {
                    Symbol *formal;
                    Js::ArgSlot pos = 1;
                    auto moveArgToReg = [&](ParseNode *pnode)
                    {
                        if (pnode->IsVarLetOrConst())
                        {
                            formal = pnode->AsParseNodeVar()->sym;
                            // Get the param from its argument position into its assigned register.
                            // The position should match the location; otherwise, it has been shadowed by parameter with the same name.
                            if (formal->GetLocation() + 1 == pos)
                            {
                                pnode->AsParseNodeVar()->sym->EnsureScopeSlot(this, funcInfo);
                            }
                        }
                        pos++;
                    };
                    MapFormals(pnodeFnc, moveArgToReg);
                    MapFormalsFromPattern(pnodeFnc, [&](ParseNode *pnode) { pnode->AsParseNodeVar()->sym->EnsureScopeSlot(this, funcInfo); });
                }

                for (pnode = pnodeFnc->pnodeVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
                {
                    sym = pnode->AsParseNodeVar()->sym;
                    if (!(pnode->AsParseNodeVar()->isBlockScopeFncDeclVar && sym->GetIsBlockVar()))
                    {
                        if (sym->GetIsCatch() || (pnode->nop == knopVarDecl && sym->GetIsBlockVar()))
                        {
                            sym = funcInfo->bodyScope->FindLocalSymbol(sym->GetName());
                        }
                        if (sym->GetSymbolType() == STVariable && !sym->IsArguments())
                        {
                            sym->EnsureScopeSlot(this, funcInfo);
                        }
                    }
                }
                auto ensureFncDeclScopeSlots = [&](ParseNode *pnodeScope)
                {
                    for (pnode = pnodeScope; pnode;)
                    {
                        switch (pnode->nop)
                        {
                        case knopFncDecl:
                            if (pnode->AsParseNodeFnc()->IsDeclaration())
                            {
                                EnsureFncDeclScopeSlot(pnode->AsParseNodeFnc(), funcInfo);
                            }
                            pnode = pnode->AsParseNodeFnc()->pnodeNext;
                            break;
                        case knopBlock:
                            pnode = pnode->AsParseNodeBlock()->pnodeNext;
                            break;
                        case knopCatch:
                            pnode = pnode->AsParseNodeCatch()->pnodeNext;
                            break;
                        case knopWith:
                            pnode = pnode->AsParseNodeWith()->pnodeNext;
                            break;
                        }
                    }
                };
                pnodeFnc->MapContainerScopes(ensureFncDeclScopeSlots);

                if (pnodeFnc->pnodeBody)
                {
                    Assert(pnodeFnc->pnodeScopes->nop == knopBlock);
                    this->EnsureLetConstScopeSlots(pnodeFnc->pnodeBodyScope, funcInfo);
                }
            }
            else
            {
                ParseNode *pnode;
                Symbol *sym;

                pnodeFnc->MapContainerScopes([&](ParseNode *pnodeScope) { this->EnsureFncScopeSlots(pnodeScope, funcInfo); });

                for (pnode = pnodeFnc->pnodeVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
                {
                    sym = pnode->AsParseNodeVar()->sym;
                    if (!(pnode->AsParseNodeVar()->isBlockScopeFncDeclVar && sym->GetIsBlockVar()))
                    {
                        if (sym->GetIsCatch() || (pnode->nop == knopVarDecl && sym->GetIsBlockVar()))
                        {
                            sym = funcInfo->bodyScope->FindLocalSymbol(sym->GetName());
                        }
                        if (sym->GetSymbolType() == STVariable && sym->NeedsSlotAlloc(this, funcInfo) && !sym->IsArguments())
                        {
                            sym->EnsureScopeSlot(this, funcInfo);
                        }
                    }
                }

                auto ensureScopeSlot = [&](ParseNode *pnode)
                {
                    if (pnode->IsVarLetOrConst())
                    {
                        sym = pnode->AsParseNodeVar()->sym;
                        if (sym->GetSymbolType() == STFormal && sym->NeedsSlotAlloc(this, funcInfo))
                        {
                            sym->EnsureScopeSlot(this, funcInfo);
                        }
                    }
                };
                // Process function's formal parameters
                MapFormals(pnodeFnc, ensureScopeSlot);
                MapFormalsFromPattern(pnodeFnc, ensureScopeSlot);

                if (funcInfo->GetHasArguments())
                {
                    sym = funcInfo->GetArgumentsSymbol();
                    Assert(sym);

                    // There is no eval so the arguments may be captured in a lambda.
                    // But we cannot relay on slots getting allocated while the lambda is emitted as the function body may be reparsed.
                    sym->EnsureScopeSlot(this, funcInfo);
                }

                if (pnodeFnc->pnodeBody)
                {
                    this->EnsureLetConstScopeSlots(pnodeFnc->pnodeScopes, funcInfo);
                    this->EnsureLetConstScopeSlots(pnodeFnc->pnodeBodyScope, funcInfo);
                }
            }

            // When we have split scope and body scope does not have any scope slots allocated, we don't have to mark the body scope as mustinstantiate.
            if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
            {
                bodyScope->SetMustInstantiate(true);
            }
            else if (pnodeFnc->IsBodyAndParamScopeMerged() || bodyScope->GetScopeSlotCount() != 0)
            {
                bodyScope->SetMustInstantiate(funcInfo->frameSlotsRegister != Js::Constants::NoRegister);
            }

            if (!pnodeFnc->IsBodyAndParamScopeMerged())
            {
                if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
                {
                    paramScope->SetMustInstantiate(true);
                }
                else
                {
                    // In the case of function expression being captured in the param scope the hasownlocalinclosure will be false for param scope,
                    // as function expression symbol stays in the function expression scope. We don't have to set mustinstantiate for param scope in that case.
                    paramScope->SetMustInstantiate(paramScope->GetHasOwnLocalInClosure());
                }
            }
        }
        else
        {
            bool newScopeForEval = (funcInfo->byteCodeFunction->GetIsStrictMode() && (this->GetFlags() & fscrEval));

            if (newScopeForEval)
            {
                Assert(bodyScope->GetIsObject());
            }
        }
    }

    if (!funcInfo->IsBodyAndParamScopeMerged())
    {
        ParseNodeBlock * paramBlock = pnodeFnc->pnodeScopes;
        Assert(paramBlock->blockType == Parameter);

        PushScope(paramScope);

        // While emitting the functions we have to stop when we see the body scope block.
        // Otherwise functions defined in the body scope will not be able to get the right references.
        this->EmitScopeList(paramBlock->pnodeScopes, pnodeFnc->pnodeBodyScope);
        Assert(this->GetCurrentScope() == paramScope);
    }

    PushScope(bodyScope);
}

void ByteCodeGenerator::EmitModuleExportAccess(Symbol* sym, Js::OpCode opcode, Js::RegSlot location, FuncInfo* funcInfo)
{
    if (EnsureSymbolModuleSlots(sym, funcInfo))
    {
        this->Writer()->SlotI2(opcode, location, sym->GetModuleIndex(), sym->GetScopeSlot());
    }
    else
    {
        this->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(ERRInvalidExportName));

        if (opcode == Js::OpCode::LdModuleSlot)
        {
            this->Writer()->Reg1(Js::OpCode::LdUndef, location);
        }
    }
}

bool ByteCodeGenerator::EnsureSymbolModuleSlots(Symbol* sym, FuncInfo* funcInfo)
{
    Assert(sym->GetIsModuleExportStorage());

    if (sym->GetModuleIndex() != Js::Constants::NoProperty && sym->GetScopeSlot() != Js::Constants::NoProperty)
    {
        return true;
    }

    Js::JavascriptLibrary* library = this->GetScriptContext()->GetLibrary();
    library->EnsureModuleRecordList();
    uint moduleIndex = this->GetModuleID();
    uint moduleSlotIndex;
    Js::SourceTextModuleRecord* moduleRecord = library->GetModuleRecord(moduleIndex);

    if (sym->GetIsModuleImport())
    {
        Js::PropertyId localImportNameId = sym->EnsurePosition(funcInfo);
        Js::ModuleNameRecord* moduleNameRecord = nullptr;
        if (!moduleRecord->ResolveImport(localImportNameId, &moduleNameRecord))
        {
            return false;
        }

        AnalysisAssert(moduleNameRecord != nullptr);
        Assert(moduleNameRecord->module->IsSourceTextModuleRecord());
        Js::SourceTextModuleRecord* resolvedModuleRecord =
            (Js::SourceTextModuleRecord*)PointerValue(moduleNameRecord->module);

        moduleIndex = resolvedModuleRecord->GetModuleId();
        moduleSlotIndex = resolvedModuleRecord->GetLocalExportSlotIndexByLocalName(moduleNameRecord->bindingName);
    }
    else
    {
        Js::PropertyId exportNameId = sym->EnsurePosition(funcInfo);
        moduleSlotIndex = moduleRecord->GetLocalExportSlotIndexByLocalName(exportNameId);
    }

    sym->SetModuleIndex(moduleIndex);
    sym->SetScopeSlot(moduleSlotIndex);

    return true;
}

void ByteCodeGenerator::EmitAssignmentToDefaultModuleExport(ParseNode* pnode, FuncInfo* funcInfo)
{
    // We are assigning pnode to the default export of the current module.
    uint moduleIndex = this->GetModuleID();

    Js::JavascriptLibrary* library = this->GetScriptContext()->GetLibrary();
    library->EnsureModuleRecordList();
    Js::SourceTextModuleRecord* moduleRecord = library->GetModuleRecord(moduleIndex);
    uint moduleSlotIndex = moduleRecord->GetLocalExportSlotIndexByExportName(Js::PropertyIds::default_);

    this->Writer()->SlotI2(Js::OpCode::StModuleSlot, pnode->location, moduleIndex, moduleSlotIndex);
}

void ByteCodeGenerator::EnsureLetConstScopeSlots(ParseNodeBlock *pnodeBlock, FuncInfo *funcInfo)
{
    bool callsEval = pnodeBlock->GetCallsEval() || pnodeBlock->GetChildCallsEval();
    auto ensureLetConstSlots = ([this, funcInfo, callsEval](ParseNode *pnode)
    {
        Symbol *sym = pnode->AsParseNodeVar()->sym;
        if (callsEval || sym->NeedsSlotAlloc(this, funcInfo))
        {
            sym->EnsureScopeSlot(this, funcInfo);
            this->ProcessCapturedSym(sym);
        }
    });
    IterateBlockScopedVariables(pnodeBlock, ensureLetConstSlots);
}

void ByteCodeGenerator::EnsureFncScopeSlots(ParseNode *pnode, FuncInfo *funcInfo)
{
    while (pnode)
    {
        switch (pnode->nop)
        {
        case knopFncDecl:
            if (pnode->AsParseNodeFnc()->IsDeclaration())
            {
                this->CheckFncDeclScopeSlot(pnode->AsParseNodeFnc(), funcInfo);
            }
            pnode = pnode->AsParseNodeFnc()->pnodeNext;
            break;
        case knopBlock:
            pnode = pnode->AsParseNodeBlock()->pnodeNext;
            break;
        case knopCatch:
            pnode = pnode->AsParseNodeCatch()->pnodeNext;
            break;
        case knopWith:
            pnode = pnode->AsParseNodeWith()->pnodeNext;
            break;
        }
    }
}

void ByteCodeGenerator::EndEmitFunction(ParseNodeFnc *pnodeFnc)
{
    Assert(pnodeFnc->nop == knopFncDecl || pnodeFnc->nop == knopProg);
    Assert(pnodeFnc->nop == knopFncDecl && currentScope->GetEnclosingScope() != nullptr || pnodeFnc->nop == knopProg);

    PopScope(); // function body

    FuncInfo *funcInfo = pnodeFnc->funcInfo;

    Scope* paramScope = funcInfo->paramScope;
    if (!funcInfo->IsBodyAndParamScopeMerged())
    {
        Assert(this->GetCurrentScope() == paramScope);
        PopScope(); // Pop the param scope
    }

    if (funcInfo->byteCodeFunction->IsFunctionParsed() && funcInfo->root->pnodeBody != nullptr)
    {
        // StartEmitFunction omits the matching PushScope for already-parsed functions.
        // TODO: Refactor Start and EndEmitFunction for clarity.
        Scope *scope = funcInfo->funcExprScope;
        if (scope && scope->GetMustInstantiate())
        {
            Assert(currentScope == scope);
            PopScope();
        }
    }

    Assert(funcInfo == this->TopFuncInfo());
    PopFuncInfo(_u("EndEmitFunction"));
}

void ByteCodeGenerator::StartEmitCatch(ParseNodeCatch *pnodeCatch)
{
    Assert(pnodeCatch->nop == knopCatch);

    Scope *scope = pnodeCatch->scope;
    FuncInfo *funcInfo = scope->GetFunc();

    // Catch scope is a dynamic object if it can be passed to a scoped lookup helper (i.e., eval is present or we're in an event handler).
    if (funcInfo->GetCallsEval() || funcInfo->GetChildCallsEval() || (this->flags & (fscrEval | fscrImplicitThis)))
    {
        scope->SetIsObject();
    }

    ParseNode * pnodeParam = pnodeCatch->GetParam();
    if (pnodeParam->nop == knopParamPattern)
    {
        scope->SetCapturesAll(funcInfo->GetCallsEval() || funcInfo->GetChildCallsEval());
        scope->SetMustInstantiate(scope->Count() > 0 && (scope->GetMustInstantiate() || scope->GetCapturesAll() || funcInfo->IsGlobalFunction()));

        Parser::MapBindIdentifier(pnodeParam->AsParseNodeParamPattern()->pnode1, [&](ParseNodePtr item)
        {
            Symbol *sym = item->AsParseNodeVar()->sym;
            if (funcInfo->IsGlobalFunction())
            {
                sym->SetIsGlobalCatch(true);
            }

            if (sym->NeedsScopeObject())
            {
                scope->SetIsObject();
            }

            Assert(sym->GetScopeSlot() == Js::Constants::NoProperty);
            if (sym->NeedsSlotAlloc(this, funcInfo))
            {
                sym->EnsureScopeSlot(this, funcInfo);
            }
        });

        // In the case of pattern we will always going to push the scope.
        PushScope(scope);
    }
    else
    {
        Symbol *sym = pnodeParam->AsParseNodeName()->sym;

        // Catch object is stored in the catch scope if there may be an ambiguous lookup or a var declaration that hides it.
        scope->SetCapturesAll(funcInfo->GetCallsEval() || funcInfo->GetChildCallsEval() || sym->GetHasNonLocalReference());
        scope->SetMustInstantiate(scope->GetCapturesAll() || funcInfo->IsGlobalFunction());

        if (funcInfo->IsGlobalFunction())
        {
            sym->SetIsGlobalCatch(true);
        }

        if (sym->NeedsScopeObject())
        {
            scope->SetIsObject();
        }

        if (scope->GetMustInstantiate())
        {
            if (sym->IsInSlot(this, funcInfo))
            {
                // Since there is only one symbol we are pushing to slot.
                // Also in order to make IsInSlot to return true - forcing the sym-has-non-local-reference.
                this->ProcessCapturedSym(sym);
                sym->EnsureScopeSlot(this, funcInfo);
            }
        }

        PushScope(scope);
    }
}

void ByteCodeGenerator::EndEmitCatch(ParseNodeCatch *pnodeCatch)
{
    Assert(pnodeCatch->nop == knopCatch);
    Assert(currentScope == pnodeCatch->scope);
    PopScope();
}

void ByteCodeGenerator::StartEmitBlock(ParseNodeBlock *pnodeBlock)
{
    if (!BlockHasOwnScope(pnodeBlock, this))
    {
        return;
    }

    Assert(pnodeBlock->nop == knopBlock);

    PushBlock(pnodeBlock);

    Scope *scope = pnodeBlock->scope;
    if (pnodeBlock->GetCallsEval() || pnodeBlock->GetChildCallsEval() || (this->flags & (fscrEval | fscrImplicitThis)))
    {
        Assert(scope->GetIsObject());
    }

    // TODO: Consider nested deferred parsing.
    if (scope->GetMustInstantiate())
    {
        FuncInfo *funcInfo = scope->GetFunc();
        this->EnsureFncScopeSlots(pnodeBlock->pnodeScopes, funcInfo);
        this->EnsureLetConstScopeSlots(pnodeBlock, funcInfo);
        PushScope(scope);
    }
}

void ByteCodeGenerator::EndEmitBlock(ParseNodeBlock *pnodeBlock)
{
    if (!BlockHasOwnScope(pnodeBlock, this))
    {
        return;
    }

    Assert(pnodeBlock->nop == knopBlock);

    Scope *scope = pnodeBlock->scope;
    if (scope && scope->GetMustInstantiate())
    {
        Assert(currentScope == pnodeBlock->scope);
        PopScope();
    }

    PopBlock();
}

void ByteCodeGenerator::StartEmitWith(ParseNode *pnodeWith)
{
    Assert(pnodeWith->nop == knopWith);

    Scope *scope = pnodeWith->AsParseNodeWith()->scope;

    AssertOrFailFast(scope->GetIsObject());

    PushScope(scope);
}

void ByteCodeGenerator::EndEmitWith(ParseNode *pnodeWith)
{
    Assert(pnodeWith->nop == knopWith);
    Assert(currentScope == pnodeWith->AsParseNodeWith()->scope);

    PopScope();
}

Js::RegSlot ByteCodeGenerator::PrependLocalScopes(Js::RegSlot evalEnv, Js::RegSlot tempLoc, FuncInfo *funcInfo)
{
    Scope *currScope = this->currentScope;
    Scope *funcScope = funcInfo->GetCurrentChildScope() ? funcInfo->GetCurrentChildScope() : funcInfo->GetBodyScope();

    if (currScope == funcScope)
    {
        return evalEnv;
    }

    bool acquireTempLoc = tempLoc == Js::Constants::NoRegister;
    if (acquireTempLoc)
    {
        tempLoc = funcInfo->AcquireTmpRegister();
    }

    // The with/catch objects must be prepended to the environment we pass to eval() or to a func declared inside with,
    // but the list must first be reversed so that innermost scopes appear first in the list.
    while (currScope != funcScope)
    {
        Scope *innerScope;
        for (innerScope = currScope; innerScope->GetEnclosingScope() != funcScope; innerScope = innerScope->GetEnclosingScope())
            ;
        if (innerScope->GetMustInstantiate())
        {
            if (!innerScope->HasInnerScopeIndex())
            {
                if (evalEnv == funcInfo->GetEnvRegister() || evalEnv == funcInfo->frameDisplayRegister)
                {
                    this->m_writer.Reg2(Js::OpCode::LdInnerFrameDisplayNoParent, tempLoc, innerScope->GetLocation());
                }
                else
                {
                    this->m_writer.Reg3(Js::OpCode::LdInnerFrameDisplay, tempLoc, innerScope->GetLocation(), evalEnv);
                }
            }
            else
            {
                if (evalEnv == funcInfo->GetEnvRegister() || evalEnv == funcInfo->frameDisplayRegister)
                {
                    this->m_writer.Reg1Unsigned1(Js::OpCode::LdIndexedFrameDisplayNoParent, tempLoc, innerScope->GetInnerScopeIndex());
                }
                else
                {
                    this->m_writer.Reg2Int1(Js::OpCode::LdIndexedFrameDisplay, tempLoc, evalEnv, innerScope->GetInnerScopeIndex());
                }
            }
            evalEnv = tempLoc;
        }
        funcScope = innerScope;
    }

    if (acquireTempLoc)
    {
        funcInfo->ReleaseTmpRegister(tempLoc);
    }
    return evalEnv;
}

void ByteCodeGenerator::EmitLoadInstance(Symbol *sym, IdentPtr pid, Js::RegSlot *pThisLocation, Js::RegSlot *pInstLocation, FuncInfo *funcInfo)
{
    Js::ByteCodeLabel doneLabel = 0;
    bool fLabelDefined = false;
    Js::RegSlot scopeLocation = Js::Constants::NoRegister;
    Js::RegSlot thisLocation = *pThisLocation;
    Js::RegSlot instLocation = *pInstLocation;
    Js::PropertyId envIndex = -1;
    Scope *scope = nullptr;
    Scope *symScope = sym ? sym->GetScope() : this->globalScope;
    Assert(symScope);

    if (sym != nullptr && sym->GetIsModuleExportStorage())
    {
        *pInstLocation = Js::Constants::NoRegister;
        return;
    }

    for (;;)
    {
        scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        if (scope == this->globalScope)
        {
            break;
        }

        if (scope != symScope)
        {
            // We're not sure where the function is (eval/with/etc).
            // So we're going to need registers to hold the instance where we (dynamically) find
            // the function, and possibly to hold the "this" pointer we will pass to it.
            // Assign them here so that they can't overlap with the scopeLocation assigned below.
            // Otherwise we wind up with temp lifetime confusion in the IRBuilder. (Win8 281689)
            if (instLocation == Js::Constants::NoRegister)
            {
                instLocation = funcInfo->AcquireTmpRegister();
                // The "this" pointer will not be the same as the instance, so give it its own register.
                thisLocation = funcInfo->AcquireTmpRegister();
            }
        }

        if (envIndex == -1)
        {
            Assert(funcInfo == scope->GetFunc());
            scopeLocation = scope->GetLocation();
        }

        if (scope == symScope)
        {
            break;
        }

        // Found a scope to which the property may have been added.
        Assert(scope && scope->GetIsDynamic());

        if (!fLabelDefined)
        {
            fLabelDefined = true;
            doneLabel = this->m_writer.DefineLabel();
        }

        Js::ByteCodeLabel nextLabel = this->m_writer.DefineLabel();
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();

        bool unwrapWithObj = scope->GetScopeType() == ScopeType_With && scriptContext->GetConfig()->IsES6UnscopablesEnabled();
        if (envIndex != -1)
        {
            this->m_writer.BrEnvProperty(
                Js::OpCode::BrOnNoEnvProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId),
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::RegSlot tmpReg = funcInfo->AcquireTmpRegister();

            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(Js::OpCode::LdEnvObj, tmpReg,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::OpCode op = unwrapWithObj ? Js::OpCode::UnwrapWithObj : Js::OpCode::Ld_A;

            this->m_writer.Reg2(op, instLocation, tmpReg);
            if (thisLocation != Js::Constants::NoRegister)
            {
                this->m_writer.Reg2(op, thisLocation, tmpReg);
            }

            funcInfo->ReleaseTmpRegister(tmpReg);
        }
        else if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
        {
            this->m_writer.BrLocalProperty(Js::OpCode::BrOnNoLocalProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Assert(!unwrapWithObj);
            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.Reg1(Js::OpCode::LdLocalObj, instLocation);
            if (thisLocation != Js::Constants::NoRegister)
            {
                this->m_writer.Reg1(Js::OpCode::LdLocalObj, thisLocation);
            }
        }
        else
        {
            this->m_writer.BrProperty(Js::OpCode::BrOnNoProperty, nextLabel, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Js::OpCode op = unwrapWithObj ? Js::OpCode::UnwrapWithObj : Js::OpCode::Ld_A;
            this->m_writer.Reg2(op, instLocation, scopeLocation);
            if (thisLocation != Js::Constants::NoRegister)
            {
                this->m_writer.Reg2(op, thisLocation, scopeLocation);
            }
        }

        this->m_writer.Br(doneLabel);
        this->m_writer.MarkLabel(nextLabel);
    }

    if (sym == nullptr || sym->GetIsGlobal())
    {
        if (this->flags & (fscrEval | fscrImplicitThis))
        {
            // Load of a symbol with unknown scope from within eval.
            // Get it from the closure environment.
            if (instLocation == Js::Constants::NoRegister)
            {
                instLocation = funcInfo->AcquireTmpRegister();
            }

            // TODO: It should be possible to avoid this double call to ScopedLdInst by having it return both
            // results at once. The reason for the uncertainty here is that we don't know whether the callee
            // belongs to a "with" object. If it does, we have to pass the "with" object as "this"; in all other
            // cases, we pass "undefined". For now, there are apparently no significant performance issues.
            Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();

            if (thisLocation == Js::Constants::NoRegister)
            {
                thisLocation = funcInfo->AcquireTmpRegister();
            }
            this->m_writer.ScopedProperty2(Js::OpCode::ScopedLdInst, instLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId), thisLocation);
        }
        else
        {
            if (instLocation == Js::Constants::NoRegister)
            {
                instLocation = ByteCodeGenerator::RootObjectRegister;
            }
            else
            {
                this->m_writer.Reg2(Js::OpCode::Ld_A, instLocation, ByteCodeGenerator::RootObjectRegister);
            }

            if (thisLocation == Js::Constants::NoRegister)
            {
                thisLocation = funcInfo->undefinedConstantRegister;
            }
            else
            {
                this->m_writer.Reg2(Js::OpCode::Ld_A, thisLocation, funcInfo->undefinedConstantRegister);
            }
        }
    }
    else if (instLocation != Js::Constants::NoRegister)
    {
        if (envIndex != -1)
        {
            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(Js::OpCode::LdEnvObj, instLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));
        }
        else if (scope->HasInnerScopeIndex())
        {
            this->m_writer.Reg1Unsigned1(Js::OpCode::LdInnerScope, instLocation, scope->GetInnerScopeIndex());
        }
        else if (symScope == funcInfo->GetParamScope())
        {
            Assert(funcInfo->frameObjRegister != Js::Constants::NoRegister && !funcInfo->IsBodyAndParamScopeMerged());
            this->m_writer.Reg1(Js::OpCode::LdParamObj, instLocation);
        }
        else if (symScope != funcInfo->GetBodyScope())
        {
            this->m_writer.Reg2(Js::OpCode::Ld_A, instLocation, scopeLocation);
        }
        else
        {
            Assert(funcInfo->frameObjRegister != Js::Constants::NoRegister);
            this->m_writer.Reg1(Js::OpCode::LdLocalObj, instLocation);
        }

        if (thisLocation != Js::Constants::NoRegister)
        {
            this->m_writer.Reg2(Js::OpCode::Ld_A, thisLocation, funcInfo->undefinedConstantRegister);
        }
        else
        {
            thisLocation = funcInfo->undefinedConstantRegister;
        }
    }

    *pThisLocation = thisLocation;
    *pInstLocation = instLocation;

    if (fLabelDefined)
    {
        this->m_writer.MarkLabel(doneLabel);
    }
}

void ByteCodeGenerator::EmitGlobalFncDeclInit(Js::RegSlot rhsLocation, Js::PropertyId propertyId, FuncInfo * funcInfo)
{
    // Note: declared variables and assignments in the global function go to the root object directly.
    if (this->flags & fscrEval)
    {
        // Func decl's always get their init values before any use, so we don't pre-initialize the property to undef.
        // That means that we have to use ScopedInitFld so that we initialize the property on the right instance
        // even if the instance doesn't have the property yet (i.e., collapse the init-to-undef and the store
        // into one operation). See WOOB 1121763 and 1120973.
        this->m_writer.ScopedProperty(Js::OpCode::ScopedInitFunc, rhsLocation,
            funcInfo->FindOrAddReferencedPropertyId(propertyId));
    }
    else
    {
        this->EmitPatchableRootProperty(Js::OpCode::InitRootFld, rhsLocation, propertyId, false, true, funcInfo);
    }
}

void
ByteCodeGenerator::EmitPatchableRootProperty(Js::OpCode opcode,
    Js::RegSlot regSlot, Js::PropertyId propertyId, bool isLoadMethod, bool isStore, FuncInfo * funcInfo)
{
    uint cacheId = funcInfo->FindOrAddRootObjectInlineCacheId(propertyId, isLoadMethod, isStore);
    this->m_writer.PatchableRootProperty(opcode, regSlot, cacheId, isLoadMethod, isStore);
}

void ByteCodeGenerator::EmitLocalPropInit(Js::RegSlot rhsLocation, Symbol *sym, FuncInfo *funcInfo)
{
    Scope *scope = sym->GetScope();

    // Check consistency of sym->IsInSlot.
    Assert(sym->NeedsSlotAlloc(this, funcInfo) || sym->GetScopeSlot() == Js::Constants::NoProperty);

    // Arrived at the scope in which the property was defined.
    if (sym->NeedsSlotAlloc(this, funcInfo))
    {
        // The property is in memory rather than register. We'll have to load it from the slots.
        if (scope->GetIsObject())
        {
            Assert(!this->TopFuncInfo()->GetParsedFunctionBody()->DoStackNestedFunc());
            Js::PropertyId propertyId = sym->EnsurePosition(this);
            Js::RegSlot objReg;
            if (scope->HasInnerScopeIndex())
            {
                objReg = funcInfo->InnerScopeToRegSlot(scope);
            }
            else
            {
                objReg = scope->GetLocation();
            }
            uint cacheId = funcInfo->FindOrAddInlineCacheId(objReg, propertyId, false, true);
            Js::OpCode op = this->GetInitFldOp(scope, objReg, funcInfo, sym->GetIsNonSimpleParameter());
            if (objReg != Js::Constants::NoRegister && objReg == funcInfo->frameObjRegister)
            {
                this->m_writer.ElementP(op, rhsLocation, cacheId);
            }
            else if (scope->HasInnerScopeIndex())
            {
                this->m_writer.ElementPIndexed(op, rhsLocation, scope->GetInnerScopeIndex(), cacheId);
            }
            else
            {
                this->m_writer.PatchableProperty(op, rhsLocation, scope->GetLocation(), cacheId);
            }
        }
        else
        {
            // Make sure the property has a slot. This will bump up the size of the slot array if necessary.
            Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
            Js::RegSlot slotReg = scope->GetCanMerge() ? funcInfo->frameSlotsRegister : scope->GetLocation();
            // Now store the property to its slot.
            Js::OpCode op = this->GetStSlotOp(scope, -1, slotReg, false, funcInfo);

            if (slotReg != Js::Constants::NoRegister && slotReg == funcInfo->frameSlotsRegister)
            {
                this->m_writer.SlotI1(op, rhsLocation, slot + Js::ScopeSlots::FirstSlotIndex);
            }
            else
            {
                this->m_writer.SlotI2(op, rhsLocation, scope->GetInnerScopeIndex(), slot + Js::ScopeSlots::FirstSlotIndex);
            }
        }
    }
    if (sym->GetLocation() != Js::Constants::NoRegister && rhsLocation != sym->GetLocation())
    {
        this->m_writer.Reg2(Js::OpCode::Ld_A, sym->GetLocation(), rhsLocation);
    }
}

Js::OpCode
ByteCodeGenerator::GetStSlotOp(Scope *scope, int envIndex, Js::RegSlot scopeLocation, bool chkBlockVar, FuncInfo *funcInfo)
{
    Js::OpCode op;

    if (envIndex != -1)
    {
        if (scope->GetIsObject())
        {
            op = Js::OpCode::StEnvObjSlot;
        }
        else
        {
            op = Js::OpCode::StEnvSlot;
        }
    }
    else if (scopeLocation != Js::Constants::NoRegister &&
        scopeLocation == funcInfo->frameSlotsRegister)
    {
        if (scope->GetScopeType() == ScopeType_Parameter && scope != scope->GetFunc()->GetCurrentChildScope())
        {
            // Symbol is from the param scope of a split scope function and we are emitting the body.
            // We should use the param scope's bytecode now.
            Assert(!funcInfo->IsBodyAndParamScopeMerged());
            op = Js::OpCode::StParamSlot;
        }
        else
        {
            op = Js::OpCode::StLocalSlot;
        }
    }
    else if (scopeLocation != Js::Constants::NoRegister &&
        scopeLocation == funcInfo->frameObjRegister)
    {
        if (scope->GetScopeType() == ScopeType_Parameter && scope != scope->GetFunc()->GetCurrentChildScope())
        {
            // Symbol is from the param scope of a split scope function and we are emitting the body.
            // We should use the param scope's bytecode now.
            Assert(!funcInfo->IsBodyAndParamScopeMerged());
            op = Js::OpCode::StParamObjSlot;
        }
        else
        {
            op = Js::OpCode::StLocalObjSlot;
        }
    }
    else
    {
        Assert(scope->HasInnerScopeIndex());
        if (scope->GetIsObject())
        {
            op = Js::OpCode::StInnerObjSlot;
        }
        else
        {
            op = Js::OpCode::StInnerSlot;
        }
    }

    if (chkBlockVar)
    {
        op = this->ToChkUndeclOp(op);
    }

    return op;
}

Js::OpCode
ByteCodeGenerator::GetInitFldOp(Scope *scope, Js::RegSlot scopeLocation, FuncInfo *funcInfo, bool letDecl)
{
    Js::OpCode op;

    if (scopeLocation != Js::Constants::NoRegister &&
        scopeLocation == funcInfo->frameObjRegister)
    {
        op = letDecl ? Js::OpCode::InitLocalLetFld : Js::OpCode::InitLocalFld;
    }
    else if (scope->HasInnerScopeIndex())
    {
        op = letDecl ? Js::OpCode::InitInnerLetFld : Js::OpCode::InitInnerFld;
    }
    else
    {
        op = letDecl ? Js::OpCode::InitLetFld : Js::OpCode::InitFld;
    }

    return op;
}

void ByteCodeGenerator::EmitPropStore(Js::RegSlot rhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool isLetDecl, bool isConstDecl, bool isFncDeclVar, bool skipUseBeforeDeclarationCheck)
{
    Js::ByteCodeLabel doneLabel = 0;
    bool fLabelDefined = false;
    Js::PropertyId envIndex = -1;
    Scope *symScope = sym == nullptr || sym->GetIsGlobal() ? this->globalScope : sym->GetScope();
    Assert(symScope);
    // isFncDeclVar denotes that the symbol being stored to here is the var
    // binding of a function declaration and we know we want to store directly
    // to it, skipping over any dynamic scopes that may lie in between.
    Scope *scope = nullptr;
    Js::RegSlot scopeLocation = Js::Constants::NoRegister;
    bool scopeAcquired = false;
    Js::OpCode op;

    if (sym && sym->GetIsModuleExportStorage())
    {
        if (!isConstDecl && sym->GetDecl() && sym->GetDecl()->nop == knopConstDecl)
        {
            this->m_writer.W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(ERRAssignmentToConst));
        }

        EmitModuleExportAccess(sym, Js::OpCode::StModuleSlot, rhsLocation, funcInfo);
        return;
    }

    if (isFncDeclVar)
    {
        // async functions allow for the fncDeclVar to be in the body or parameter scope
        // of the parent function, so we need to calculate envIndex in lieu of the while
        // loop below.
        do
        {
            scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        } while (scope != symScope);
        Assert(scope == symScope);
        scopeLocation = scope->GetLocation();
    }

    while (!isFncDeclVar)
    {
        scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        if (scope == this->globalScope)
        {
            break;
        }
        if (envIndex == -1)
        {
            Assert(funcInfo == scope->GetFunc());
            scopeLocation = scope->GetLocation();
        }

        if (scope == symScope)
        {
            break;
        }

        // Found a scope to which the property may have been added.
        Assert(scope && scope->GetIsDynamic());

        if (!fLabelDefined)
        {
            fLabelDefined = true;
            doneLabel = this->m_writer.DefineLabel();
        }
        Js::ByteCodeLabel nextLabel = this->m_writer.DefineLabel();
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();

        Js::RegSlot unwrappedScopeLocation = scopeLocation;
        bool unwrapWithObj = scope->GetScopeType() == ScopeType_With && scriptContext->GetConfig()->IsES6UnscopablesEnabled();
        if (envIndex != -1)
        {
            this->m_writer.BrEnvProperty(
                Js::OpCode::BrOnNoEnvProperty,
                nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId),
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::RegSlot instLocation = funcInfo->AcquireTmpRegister();

            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(
                Js::OpCode::LdEnvObj,
                instLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            if (unwrapWithObj)
            {
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, instLocation, instLocation);
            }

            this->m_writer.PatchableProperty(
                Js::OpCode::StFld,
                rhsLocation,
                instLocation,
                funcInfo->FindOrAddInlineCacheId(instLocation, propertyId, false, true));

            funcInfo->ReleaseTmpRegister(instLocation);
        }
        else if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
        {
            this->m_writer.BrLocalProperty(Js::OpCode::BrOnNoLocalProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Assert(!unwrapWithObj);
            this->m_writer.ElementP(Js::OpCode::StLocalFld, rhsLocation,
                funcInfo->FindOrAddInlineCacheId(scopeLocation, propertyId, false, true));
        }
        else
        {
            this->m_writer.BrProperty(Js::OpCode::BrOnNoProperty, nextLabel, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            if (unwrapWithObj)
            {
                unwrappedScopeLocation = funcInfo->AcquireTmpRegister();
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, unwrappedScopeLocation, scopeLocation);
                scopeLocation = unwrappedScopeLocation;
            }

            uint cacheId = funcInfo->FindOrAddInlineCacheId(scopeLocation, propertyId, false, true);
            this->m_writer.PatchableProperty(Js::OpCode::StFld, rhsLocation, scopeLocation, cacheId);

            if (unwrapWithObj)
            {
                funcInfo->ReleaseTmpRegister(unwrappedScopeLocation);
            }
        }

        this->m_writer.Br(doneLabel);
        this->m_writer.MarkLabel(nextLabel);
    }

    // Arrived at the scope in which the property was defined.
    if (!skipUseBeforeDeclarationCheck && sym && sym->GetNeedDeclaration() && scope->GetFunc() == funcInfo)
    {
        EmitUseBeforeDeclarationRuntimeError(this, Js::Constants::NoRegister);
        // Intentionally continue on to do normal EmitPropStore behavior so
        // that the bytecode ends up well-formed for the backend.  This is
        // in contrast to EmitPropLoad and EmitPropTypeof where they both
        // tell EmitUseBeforeDeclarationRuntimeError to emit a LdUndef in place
        // of their load and then they skip emitting their own bytecode.
        // Potayto potahto.
    }

    if (sym == nullptr || sym->GetIsGlobal())
    {
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();
        bool isConsoleScopeLetConst = this->IsConsoleScopeEval() && (isLetDecl || isConstDecl);
        if (this->flags & fscrEval)
        {
            if (funcInfo->byteCodeFunction->GetIsStrictMode() && funcInfo->IsGlobalFunction())
            {
                uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->frameDisplayRegister, propertyId, false, true);
                this->m_writer.ElementP(GetScopedStFldOpCode(funcInfo, isConsoleScopeLetConst), rhsLocation, cacheId);
            }
            else
            {
                uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->GetEnvRegister(), propertyId, false, true);
                // In "eval", store to a symbol with unknown scope goes through the closure environment.
                this->m_writer.ElementP(GetScopedStFldOpCode(funcInfo, isConsoleScopeLetConst), rhsLocation, cacheId);
            }
        }
        else if (this->flags & fscrImplicitThis)
        {
            uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->GetEnvRegister(), propertyId, false, true);

            // In HTML event handler, store to a symbol with unknown scope goes through the closure environment.
            this->m_writer.ElementP(GetScopedStFldOpCode(funcInfo, isConsoleScopeLetConst), rhsLocation, cacheId);
        }
        else
        {
            this->EmitPatchableRootProperty(GetStFldOpCode(funcInfo, true, isLetDecl, isConstDecl, false), rhsLocation, propertyId, false, true, funcInfo);
        }
    }
    else if (sym->GetIsFuncExpr())
    {
        // Store to function expr variable.

        // strict mode: we need to throw type error
        if (funcInfo->byteCodeFunction->GetIsStrictMode())
        {
            // Note that in this case the sym's location belongs to the parent function, so we can't use it.
            // It doesn't matter which register we use, as long as it's valid for this function.
            this->m_writer.W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_CantAssignToReadOnly));
        }
    }
    else if (sym->IsInSlot(this, funcInfo) || envIndex != -1)
    {
        if (!isConstDecl && sym->GetIsConst())
        {
            // This is a case where const reassignment can't be proven statically (e.g., eval, with) so
            // we have to catch it at runtime.
            this->m_writer.W1(
                Js::OpCode::RuntimeTypeError, SCODE_CODE(ERRAssignmentToConst));
        }
        // Make sure the property has a slot. This will bump up the size of the slot array if necessary.
        Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
        bool chkBlockVar = !isLetDecl && !isConstDecl && NeedCheckBlockVar(sym, scope, funcInfo);

        // The property is in memory rather than register. We'll have to load it from the slots.
        op = this->GetStSlotOp(scope, envIndex, scopeLocation, chkBlockVar, funcInfo);

        if (envIndex != -1)
        {
            this->m_writer.SlotI2(op, rhsLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var),
                slot + (sym->GetScope()->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex));
        }
        else if (scopeLocation != Js::Constants::NoRegister &&
            (scopeLocation == funcInfo->frameSlotsRegister || scopeLocation == funcInfo->frameObjRegister))
        {
            this->m_writer.SlotI1(op, rhsLocation,
                slot + (sym->GetScope()->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex));
        }
        else
        {
            Assert(scope->HasInnerScopeIndex());
            this->m_writer.SlotI2(op, rhsLocation, scope->GetInnerScopeIndex(),
                slot + (sym->GetScope()->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex));
        }

        if (this->ShouldTrackDebuggerMetadata() && (isLetDecl || isConstDecl))
        {
            Js::PropertyId location = scope->GetIsObject() ? sym->GetLocation() : slot;
            this->UpdateDebuggerPropertyInitializationOffset(location, sym->GetPosition(), false);
        }
    }
    else if (isConstDecl)
    {
        this->m_writer.Reg2(Js::OpCode::InitConst, sym->GetLocation(), rhsLocation);

        if (this->ShouldTrackDebuggerMetadata())
        {
            this->UpdateDebuggerPropertyInitializationOffset(sym->GetLocation(), sym->GetPosition());
        }
    }
    else
    {
        if (!isConstDecl && sym->GetDecl() && sym->GetDecl()->nop == knopConstDecl)
        {
            // This is a case where const reassignment can't be proven statically (e.g., eval, with) so
            // we have to catch it at runtime.
            this->m_writer.W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(ERRAssignmentToConst));
        }
        if (rhsLocation != sym->GetLocation())
        {
            this->m_writer.Reg2(Js::OpCode::Ld_A, sym->GetLocation(), rhsLocation);

            if (this->ShouldTrackDebuggerMetadata() && isLetDecl)
            {
                this->UpdateDebuggerPropertyInitializationOffset(sym->GetLocation(), sym->GetPosition());
            }
        }
    }
    if (fLabelDefined)
    {
        this->m_writer.MarkLabel(doneLabel);
    }

    if (scopeAcquired)
    {
        funcInfo->ReleaseTmpRegister(scopeLocation);
    }
}

Js::OpCode
ByteCodeGenerator::GetLdSlotOp(Scope *scope, int envIndex, Js::RegSlot scopeLocation, FuncInfo *funcInfo)
{
    Js::OpCode op;

    if (envIndex != -1)
    {
        if (scope->GetIsObject())
        {
            op = Js::OpCode::LdEnvObjSlot;
        }
        else
        {
            op = Js::OpCode::LdEnvSlot;
        }
    }
    else if (scopeLocation != Js::Constants::NoRegister &&
        scopeLocation == funcInfo->frameSlotsRegister)
    {
        if (scope->GetScopeType() == ScopeType_Parameter && scope != scope->GetFunc()->GetCurrentChildScope())
        {
            // Symbol is from the param scope of a split scope function and we are emitting the body.
            // We should use the param scope's bytecode now.
            Assert(!funcInfo->IsBodyAndParamScopeMerged());
            op = Js::OpCode::LdParamSlot;
        }
        else
        {
            op = Js::OpCode::LdLocalSlot;
        }
    }
    else if (scopeLocation != Js::Constants::NoRegister &&
        scopeLocation == funcInfo->frameObjRegister)
    {
        if (scope->GetScopeType() == ScopeType_Parameter && scope != scope->GetFunc()->GetCurrentChildScope())
        {
            // Symbol is from the param scope of a split scope function and we are emitting the body.
            // We should use the param scope's bytecode now.
            Assert(!funcInfo->IsBodyAndParamScopeMerged());
            op = Js::OpCode::LdParamObjSlot;
        }
        else
        {
            op = Js::OpCode::LdLocalObjSlot;
        }
    }
    else if (scope->HasInnerScopeIndex())
    {
        if (scope->GetIsObject())
        {
            op = Js::OpCode::LdInnerObjSlot;
        }
        else
        {
            op = Js::OpCode::LdInnerSlot;
        }
    }
    else
    {
        AssertOrFailFast(scope->GetIsObject());
        op = Js::OpCode::LdObjSlot;
    }

    return op;
}

bool ByteCodeGenerator::ShouldLoadConstThis(FuncInfo* funcInfo)
{
#if DBG
    // We should load a const 'this' binding if the following holds
    // - The function has a 'this' name node
    // - We are in a global or global lambda function
    // - The function has no 'this' symbol (an indirect eval would have this symbol)
    if (funcInfo->thisConstantRegister != Js::Constants::NoRegister)
    {
        Assert((funcInfo->IsLambda() || funcInfo->IsGlobalFunction())
            && !funcInfo->GetThisSymbol()
            && !(this->flags & fscrEval));
    }
#endif

    return funcInfo->thisConstantRegister != Js::Constants::NoRegister;
}

void ByteCodeGenerator::EmitPropLoadThis(Js::RegSlot lhsLocation, ParseNodeSpecialName *pnodeSpecialName, FuncInfo *funcInfo, bool chkUndecl)
{
    Symbol* sym = pnodeSpecialName->sym;

    if (!sym && this->ShouldLoadConstThis(funcInfo))
    {
        this->Writer()->Reg2(Js::OpCode::Ld_A, lhsLocation, funcInfo->thisConstantRegister);
    }
    else
    {
        this->EmitPropLoad(lhsLocation, pnodeSpecialName->sym, pnodeSpecialName->pid, funcInfo, true);

        if ((!sym || sym->GetNeedDeclaration()) && chkUndecl)
        {
            this->Writer()->Reg1(Js::OpCode::ChkUndecl, lhsLocation);
        }
    }
}

void ByteCodeGenerator::EmitPropStoreForSpecialSymbol(Js::RegSlot rhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool init)
{
    if (!funcInfo->IsGlobalFunction() || (this->flags & fscrEval))
    {
        if (init)
        {
            EmitLocalPropInit(rhsLocation, sym, funcInfo);
        }
        else
        {
            EmitPropStore(rhsLocation, sym, pid, funcInfo, false, false, false, true);
        }
    }
}

void ByteCodeGenerator::EmitPropLoad(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo, bool skipUseBeforeDeclarationCheck)
{
    // If sym belongs to a parent frame, get it from the closure environment.
    // If it belongs to this func, but there's a non-local reference, get it from the heap-allocated frame.
    // (TODO: optimize this by getting the sym from its normal location if there are no non-local defs.)
    // Otherwise, just copy the value to the lhsLocation.

    Js::ByteCodeLabel doneLabel = 0;
    bool fLabelDefined = false;
    Js::RegSlot scopeLocation = Js::Constants::NoRegister;
    Js::PropertyId envIndex = -1;
    Scope *scope = nullptr;
    Scope *symScope = sym ? sym->GetScope() : this->globalScope;
    Assert(symScope);

    if (sym && sym->GetIsModuleExportStorage())
    {
        EmitModuleExportAccess(sym, Js::OpCode::LdModuleSlot, lhsLocation, funcInfo);
        return;
    }

    for (;;)
    {
        scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        if (scope == this->globalScope)
        {
            break;
        }

        scopeLocation = scope->GetLocation();

        if (scope == symScope)
        {
            break;
        }

        // Found a scope to which the property may have been added.
        Assert(scope && scope->GetIsDynamic());

        if (!fLabelDefined)
        {
            fLabelDefined = true;
            doneLabel = this->m_writer.DefineLabel();
        }

        Js::ByteCodeLabel nextLabel = this->m_writer.DefineLabel();
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();

        Js::RegSlot unwrappedScopeLocation = Js::Constants::NoRegister;
        bool unwrapWithObj = scope->GetScopeType() == ScopeType_With && scriptContext->GetConfig()->IsES6UnscopablesEnabled();
        if (envIndex != -1)
        {
            this->m_writer.BrEnvProperty(
                Js::OpCode::BrOnNoEnvProperty,
                nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId),
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::RegSlot instLocation = funcInfo->AcquireTmpRegister();

            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(
                Js::OpCode::LdEnvObj,
                instLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            if (unwrapWithObj)
            {
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, instLocation, instLocation);
            }

            this->m_writer.PatchableProperty(
                Js::OpCode::LdFld,
                lhsLocation,
                instLocation,
                funcInfo->FindOrAddInlineCacheId(instLocation, propertyId, false, false));

            funcInfo->ReleaseTmpRegister(instLocation);
        }
        else if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
        {
            this->m_writer.BrLocalProperty(Js::OpCode::BrOnNoLocalProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Assert(!unwrapWithObj);
            this->m_writer.ElementP(Js::OpCode::LdLocalFld, lhsLocation,
                funcInfo->FindOrAddInlineCacheId(scopeLocation, propertyId, false, false));
        }
        else
        {
            this->m_writer.BrProperty(Js::OpCode::BrOnNoProperty, nextLabel, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            if (unwrapWithObj)
            {
                unwrappedScopeLocation = funcInfo->AcquireTmpRegister();
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, unwrappedScopeLocation, scopeLocation);
                scopeLocation = unwrappedScopeLocation;
            }

            uint cacheId = funcInfo->FindOrAddInlineCacheId(scopeLocation, propertyId, false, false);
            this->m_writer.PatchableProperty(Js::OpCode::LdFld, lhsLocation, scopeLocation, cacheId);

            if (unwrapWithObj)
            {
                funcInfo->ReleaseTmpRegister(unwrappedScopeLocation);
            }
        }

        this->m_writer.Br(doneLabel);
        this->m_writer.MarkLabel(nextLabel);
    }

    // Arrived at the scope in which the property was defined.
    if (sym && sym->GetNeedDeclaration() && scope->GetFunc() == funcInfo && !skipUseBeforeDeclarationCheck)
    {
        // Ensure this symbol has a slot if it needs one.
        if (sym->IsInSlot(this, funcInfo))
        {
            Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
            funcInfo->FindOrAddSlotProfileId(scope, slot);
        }

        if (skipUseBeforeDeclarationCheck)
        {
            if (lhsLocation != Js::Constants::NoRegister)
            {
                this->m_writer.Reg1(Js::OpCode::InitUndecl, lhsLocation);
            }
        }
        else
        {
            EmitUseBeforeDeclarationRuntimeError(this, lhsLocation);
        }
    }
    else if (sym == nullptr || sym->GetIsGlobal())
    {
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();
        if (this->flags & fscrEval)
        {
            if (funcInfo->byteCodeFunction->GetIsStrictMode() && funcInfo->IsGlobalFunction())
            {
                uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->frameDisplayRegister, propertyId, false, false);
                this->m_writer.ElementP(Js::OpCode::ScopedLdFld, lhsLocation, cacheId);
            }
            else
            {
                uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->GetEnvRegister(), propertyId, false, false);

                // Load of a symbol with unknown scope from within eval
                // Get it from the closure environment.
                this->m_writer.ElementP(Js::OpCode::ScopedLdFld, lhsLocation, cacheId);
            }
        }
        else if (this->flags & fscrImplicitThis)
        {
            uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->GetEnvRegister(), propertyId, false, false);

            // Load of a symbol with unknown scope from within event handler.
            // Get it from the closure environment.
            this->m_writer.ElementP(Js::OpCode::ScopedLdFld, lhsLocation, cacheId);
        }
        else
        {
            // Special case non-writable built-ins
            // TODO: support non-writable global property in general by detecting what attribute the property have current?
            // But can't be done if we are byte code serialized, because the attribute might be different for use fields
            // next time we run. May want to catch that in the JIT.
            Js::OpCode opcode = Js::OpCode::LdRootFld;

            // These properties are non-writable
            switch (propertyId)
            {
            case Js::PropertyIds::NaN:
                opcode = Js::OpCode::LdNaN;
                break;
            case Js::PropertyIds::Infinity:
                opcode = Js::OpCode::LdInfinity;
                break;
            case Js::PropertyIds::undefined:
                opcode = Js::OpCode::LdUndef;
                break;
            case Js::PropertyIds::__chakraLibrary:
                if (CONFIG_FLAG(LdChakraLib)) {
                    opcode = Js::OpCode::LdChakraLib;
                }
                break;
            }

            if (opcode == Js::OpCode::LdRootFld)
            {
                this->EmitPatchableRootProperty(Js::OpCode::LdRootFld, lhsLocation, propertyId, false, false, funcInfo);
            }
            else
            {
                this->Writer()->Reg1(opcode, lhsLocation);
            }
        }
    }
    else if (sym->IsInSlot(this, funcInfo) || envIndex != -1)
    {
        // Make sure the property has a slot. This will bump up the size of the slot array if necessary.
        Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
        Js::ProfileId profileId = funcInfo->FindOrAddSlotProfileId(scope, slot);
        bool chkBlockVar = NeedCheckBlockVar(sym, scope, funcInfo);
        Js::OpCode op;

        // Now get the property from its slot.
        op = this->GetLdSlotOp(scope, envIndex, scopeLocation, funcInfo);
        slot = slot + (sym->GetScope()->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex);

        if (envIndex != -1)
        {
            this->m_writer.SlotI2(op, lhsLocation, envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var), slot, profileId);
        }
        else if (scopeLocation != Js::Constants::NoRegister &&
            (scopeLocation == funcInfo->frameSlotsRegister || scopeLocation == funcInfo->frameObjRegister))
        {
            this->m_writer.SlotI1(op, lhsLocation, slot, profileId);
        }
        else if (scope->HasInnerScopeIndex())
        {
            this->m_writer.SlotI2(op, lhsLocation, scope->GetInnerScopeIndex(), slot, profileId);
        }
        else
        {
            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.Slot(op, lhsLocation, scopeLocation, slot, profileId);
        }

        if (chkBlockVar)
        {
            this->m_writer.Reg1(Js::OpCode::ChkUndecl, lhsLocation);
        }
    }
    else
    {
        if (lhsLocation != sym->GetLocation())
        {
            this->m_writer.Reg2(Js::OpCode::Ld_A, lhsLocation, sym->GetLocation());
        }
        if (sym->GetIsBlockVar() && ((sym->GetDecl()->nop == knopLetDecl || sym->GetDecl()->nop == knopConstDecl) && sym->GetDecl()->AsParseNodeVar()->isSwitchStmtDecl))
        {
            this->m_writer.Reg1(Js::OpCode::ChkUndecl, lhsLocation);
        }
    }

    if (fLabelDefined)
    {
        this->m_writer.MarkLabel(doneLabel);
    }
}

bool ByteCodeGenerator::NeedCheckBlockVar(Symbol* sym, Scope* scope, FuncInfo* funcInfo) const
{
    bool tdz = sym->GetIsBlockVar()
        && (scope->GetFunc() != funcInfo || ((sym->GetDecl()->nop == knopLetDecl || sym->GetDecl()->nop == knopConstDecl) && sym->GetDecl()->AsParseNodeVar()->isSwitchStmtDecl));

    return tdz || sym->GetIsNonSimpleParameter();
}

void ByteCodeGenerator::EmitPropDelete(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo)
{
    // If sym belongs to a parent frame, delete it from the closure environment.
    // If it belongs to this func, but there's a non-local reference, get it from the heap-allocated frame.
    // (TODO: optimize this by getting the sym from its normal location if there are no non-local defs.)
    // Otherwise, just return false.

    Js::ByteCodeLabel doneLabel = 0;
    bool fLabelDefined = false;
    Js::RegSlot scopeLocation = Js::Constants::NoRegister;
    Js::PropertyId envIndex = -1;
    Scope *scope = nullptr;
    Scope *symScope = sym ? sym->GetScope() : this->globalScope;
    Assert(symScope);

    for (;;)
    {
        scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        if (scope == this->globalScope)
        {
            scopeLocation = ByteCodeGenerator::RootObjectRegister;
        }
        else if (envIndex == -1)
        {
            Assert(funcInfo == scope->GetFunc());
            scopeLocation = scope->GetLocation();
        }

        if (scope == symScope)
        {
            break;
        }

        // Found a scope to which the property may have been added.
        Assert(scope && scope->GetIsDynamic());

        if (!fLabelDefined)
        {
            fLabelDefined = true;
            doneLabel = this->m_writer.DefineLabel();
        }

        Js::ByteCodeLabel nextLabel = this->m_writer.DefineLabel();
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();
        bool unwrapWithObj = scope->GetScopeType() == ScopeType_With && scriptContext->GetConfig()->IsES6UnscopablesEnabled();
        if (envIndex != -1)
        {
            this->m_writer.BrEnvProperty(
                Js::OpCode::BrOnNoEnvProperty,
                nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId),
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::RegSlot instLocation = funcInfo->AcquireTmpRegister();

            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(
                Js::OpCode::LdEnvObj,
                instLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            if (unwrapWithObj)
            {
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, instLocation, instLocation);
            }

            this->m_writer.Property(Js::OpCode::DeleteFld, lhsLocation, instLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            funcInfo->ReleaseTmpRegister(instLocation);
        }
        else if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
        {
            this->m_writer.BrLocalProperty(Js::OpCode::BrOnNoLocalProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Assert(!unwrapWithObj);
            this->m_writer.ElementU(Js::OpCode::DeleteLocalFld, lhsLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));
        }
        else
        {
            this->m_writer.BrProperty(Js::OpCode::BrOnNoProperty, nextLabel, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Js::RegSlot unwrappedScopeLocation = Js::Constants::NoRegister;
            if (unwrapWithObj)
            {
                unwrappedScopeLocation = funcInfo->AcquireTmpRegister();
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, unwrappedScopeLocation, scopeLocation);
                scopeLocation = unwrappedScopeLocation;
            }

            this->m_writer.Property(Js::OpCode::DeleteFld, lhsLocation, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            if (unwrapWithObj)
            {
                funcInfo->ReleaseTmpRegister(unwrappedScopeLocation);
            }
        }

        this->m_writer.Br(doneLabel);
        this->m_writer.MarkLabel(nextLabel);
    }

    // Arrived at the scope in which the property was defined.
    if (sym == nullptr || sym->GetIsGlobal())
    {
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();
        if (this->flags & (fscrEval | fscrImplicitThis))
        {
            this->m_writer.ScopedProperty(Js::OpCode::ScopedDeleteFld, lhsLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));
        }
        else
        {
            this->m_writer.Property(Js::OpCode::DeleteRootFld, lhsLocation, ByteCodeGenerator::RootObjectRegister,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));
        }
    }
    else
    {
        // The delete will look like a non-local reference, so make sure a slot is reserved.
        sym->EnsureScopeSlot(this, funcInfo);
        this->m_writer.Reg1(Js::OpCode::LdFalse, lhsLocation);
    }

    if (fLabelDefined)
    {
        this->m_writer.MarkLabel(doneLabel);
    }
}

void ByteCodeGenerator::EmitTypeOfFld(FuncInfo * funcInfo, Js::PropertyId propertyId, Js::RegSlot value, Js::RegSlot instance, Js::OpCode ldFldOp)
{

    uint cacheId;
    Js::RegSlot tmpReg = funcInfo->AcquireTmpRegister();
    switch (ldFldOp)
    {
    case Js::OpCode::LdRootFldForTypeOf:
        cacheId = funcInfo->FindOrAddRootObjectInlineCacheId(propertyId, false, false);
        this->Writer()->PatchableRootProperty(ldFldOp, tmpReg, cacheId, false, false);
        break;

    case Js::OpCode::LdLocalFld:
    case Js::OpCode::ScopedLdFldForTypeOf:
        cacheId = funcInfo->FindOrAddInlineCacheId(instance, propertyId, false, false);
        this->Writer()->ElementP(ldFldOp, tmpReg, cacheId);
        break;

    default:
        cacheId = funcInfo->FindOrAddInlineCacheId(instance, propertyId, false, false);
        this->Writer()->PatchableProperty(ldFldOp, tmpReg, instance, cacheId);
        break;
    }

    this->Writer()->Reg2(Js::OpCode::Typeof, value, tmpReg);
    funcInfo->ReleaseTmpRegister(tmpReg);
}

void ByteCodeGenerator::EmitPropTypeof(Js::RegSlot lhsLocation, Symbol *sym, IdentPtr pid, FuncInfo *funcInfo)
{
    // If sym belongs to a parent frame, delete it from the closure environment.
    // If it belongs to this func, but there's a non-local reference, get it from the heap-allocated frame.
    // (TODO: optimize this by getting the sym from its normal location if there are no non-local defs.)
    // Otherwise, just return false

    Js::ByteCodeLabel doneLabel = 0;
    bool fLabelDefined = false;
    Js::RegSlot scopeLocation = Js::Constants::NoRegister;
    Js::PropertyId envIndex = -1;
    Scope *scope = nullptr;
    Scope *symScope = sym ? sym->GetScope() : this->globalScope;
    Assert(symScope);

    if (sym && sym->GetIsModuleExportStorage())
    {
        Js::RegSlot tmpLocation = funcInfo->AcquireTmpRegister();
        EmitModuleExportAccess(sym, Js::OpCode::LdModuleSlot, tmpLocation, funcInfo);
        this->m_writer.Reg2(Js::OpCode::Typeof, lhsLocation, tmpLocation);
        funcInfo->ReleaseTmpRegister(tmpLocation);
        return;
    }

    for (;;)
    {
        scope = this->FindScopeForSym(symScope, scope, &envIndex, funcInfo);
        if (scope == this->globalScope)
        {
            scopeLocation = ByteCodeGenerator::RootObjectRegister;
        }
        else if (envIndex == -1)
        {
            Assert(funcInfo == scope->GetFunc());
            scopeLocation = scope->GetLocation();
        }

        if (scope == symScope)
        {
            break;
        }

        // Found a scope to which the property may have been added.
        Assert(scope && scope->GetIsDynamic());

        if (!fLabelDefined)
        {
            fLabelDefined = true;
            doneLabel = this->m_writer.DefineLabel();
        }

        Js::ByteCodeLabel nextLabel = this->m_writer.DefineLabel();
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();

        bool unwrapWithObj = scope->GetScopeType() == ScopeType_With && scriptContext->GetConfig()->IsES6UnscopablesEnabled();
        if (envIndex != -1)
        {
            this->m_writer.BrEnvProperty(Js::OpCode::BrOnNoEnvProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId),
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            Js::RegSlot instLocation = funcInfo->AcquireTmpRegister();

            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.SlotI1(Js::OpCode::LdEnvObj,
                instLocation,
                envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var));

            if (unwrapWithObj)
            {
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, instLocation, instLocation);
            }

            this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, instLocation, Js::OpCode::LdFldForTypeOf);

            funcInfo->ReleaseTmpRegister(instLocation);
        }
        else if (scopeLocation != Js::Constants::NoRegister && scopeLocation == funcInfo->frameObjRegister)
        {
            this->m_writer.BrLocalProperty(Js::OpCode::BrOnNoLocalProperty, nextLabel,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Assert(!unwrapWithObj);
            this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, scopeLocation, Js::OpCode::LdLocalFld);
        }
        else
        {
            this->m_writer.BrProperty(Js::OpCode::BrOnNoProperty, nextLabel, scopeLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));

            Js::RegSlot unwrappedScopeLocation = Js::Constants::NoRegister;
            if (unwrapWithObj)
            {
                unwrappedScopeLocation = funcInfo->AcquireTmpRegister();
                this->m_writer.Reg2(Js::OpCode::UnwrapWithObj, unwrappedScopeLocation, scopeLocation);
                scopeLocation = unwrappedScopeLocation;
            }

            this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, scopeLocation, Js::OpCode::LdFldForTypeOf);

            if (unwrapWithObj)
            {
                funcInfo->ReleaseTmpRegister(unwrappedScopeLocation);
            }
        }

        this->m_writer.Br(doneLabel);
        this->m_writer.MarkLabel(nextLabel);
    }

    // Arrived at the scope in which the property was defined.
    if (sym && sym->GetNeedDeclaration() && scope->GetFunc() == funcInfo)
    {
        // Ensure this symbol has a slot if it needs one.
        if (sym->IsInSlot(this, funcInfo))
        {
            Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
            funcInfo->FindOrAddSlotProfileId(scope, slot);
        }

        EmitUseBeforeDeclarationRuntimeError(this, lhsLocation);
    }
    else if (sym == nullptr || sym->GetIsGlobal())
    {
        Js::PropertyId propertyId = sym ? sym->EnsurePosition(this) : pid->GetPropertyId();
        if (this->flags & fscrEval)
        {
            if (funcInfo->byteCodeFunction->GetIsStrictMode() && funcInfo->IsGlobalFunction())
            {
                this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, funcInfo->frameDisplayRegister, Js::OpCode::ScopedLdFldForTypeOf);
            }
            else
            {
                this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, funcInfo->GetEnvRegister(), Js::OpCode::ScopedLdFldForTypeOf);
            }
        }
        else if (this->flags & fscrImplicitThis)
        {
            this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, funcInfo->GetEnvRegister(), Js::OpCode::ScopedLdFldForTypeOf);
        }
        else
        {
            this->EmitTypeOfFld(funcInfo, propertyId, lhsLocation, ByteCodeGenerator::RootObjectRegister, Js::OpCode::LdRootFldForTypeOf);
        }
    }
    else if (sym->IsInSlot(this, funcInfo) || envIndex != -1)
    {
        // Make sure the property has a slot. This will bump up the size of the slot array if necessary.
        Js::PropertyId slot = sym->EnsureScopeSlot(this, funcInfo);
        Js::ProfileId profileId = funcInfo->FindOrAddSlotProfileId(scope, slot);
        Js::RegSlot tmpLocation = funcInfo->AcquireTmpRegister();
        bool chkBlockVar = NeedCheckBlockVar(sym, scope, funcInfo);
        Js::OpCode op;

        op = this->GetLdSlotOp(scope, envIndex, scopeLocation, funcInfo);
        slot = slot + (sym->GetScope()->GetIsObject() ? 0 : Js::ScopeSlots::FirstSlotIndex);

        if (envIndex != -1)
        {
            this->m_writer.SlotI2(op, tmpLocation, envIndex + Js::FrameDisplay::GetOffsetOfScopes() / sizeof(Js::Var), slot, profileId);
        }
        else if (scopeLocation != Js::Constants::NoRegister &&
            (scopeLocation == funcInfo->frameSlotsRegister || scopeLocation == funcInfo->frameObjRegister))
        {
            this->m_writer.SlotI1(op, tmpLocation, slot, profileId);
        }
        else if (scope->HasInnerScopeIndex())
        {
            this->m_writer.SlotI2(op, tmpLocation, scope->GetInnerScopeIndex(), slot, profileId);
        }
        else
        {
            AssertOrFailFast(scope->GetIsObject());
            this->m_writer.Slot(op, tmpLocation, scopeLocation, slot, profileId);
        }

        if (chkBlockVar)
        {
            this->m_writer.Reg1(Js::OpCode::ChkUndecl, tmpLocation);
        }

        this->m_writer.Reg2(Js::OpCode::Typeof, lhsLocation, tmpLocation);
        funcInfo->ReleaseTmpRegister(tmpLocation);
    }
    else
    {
        this->m_writer.Reg2(Js::OpCode::Typeof, lhsLocation, sym->GetLocation());
    }

    if (fLabelDefined)
    {
        this->m_writer.MarkLabel(doneLabel);
    }
}

void ByteCodeGenerator::EnsureNoRedeclarations(ParseNodeBlock *pnodeBlock, FuncInfo *funcInfo)
{
    // Emit dynamic runtime checks for variable re-declarations. Only necessary for global functions (script or eval).
    // In eval only var declarations can cause redeclaration, and only in non-strict mode, because let/const variables
    // remain local to the eval code.

    Assert(pnodeBlock->nop == knopBlock);
    Assert(pnodeBlock->blockType == PnodeBlockType::Global || pnodeBlock->scope->GetScopeType() == ScopeType_GlobalEvalBlock);

    if (!(this->flags & fscrEvalCode))
    {
        IterateBlockScopedVariables(pnodeBlock, [this](ParseNode *pnode)
        {
            FuncInfo *funcInfo = this->TopFuncInfo();
            Symbol *sym = pnode->AsParseNodeVar()->sym;

            Assert(sym->GetIsGlobal());

            Js::PropertyId propertyId = sym->EnsurePosition(this);

            this->m_writer.ElementRootU(Js::OpCode::EnsureNoRootFld, funcInfo->FindOrAddReferencedPropertyId(propertyId));
        });
    }

    auto emitRedeclCheck = [this](Symbol * sym, FuncInfo * funcInfo)
    {
        Js::PropertyId propertyId = sym->EnsurePosition(this);

        if (this->flags & fscrEval)
        {
            if (!funcInfo->byteCodeFunction->GetIsStrictMode())
            {
                this->m_writer.ScopedProperty(Js::OpCode::ScopedEnsureNoRedeclFld, ByteCodeGenerator::RootObjectRegister,
                    funcInfo->FindOrAddReferencedPropertyId(propertyId));
            }
        }
        else
        {
            this->m_writer.ElementRootU(Js::OpCode::EnsureNoRootRedeclFld, funcInfo->FindOrAddReferencedPropertyId(propertyId));
        }
    };

    // scan for function declarations
    // these behave like "var" declarations
    for (ParseNodePtr pnode = pnodeBlock->pnodeScopes; pnode;)
    {
        switch (pnode->nop) {

        case knopFncDecl:
            if (pnode->AsParseNodeFnc()->IsDeclaration())
            {
                emitRedeclCheck(pnode->AsParseNodeFnc()->pnodeName->sym, funcInfo);
            }

            pnode = pnode->AsParseNodeFnc()->pnodeNext;
            break;

        case knopBlock:
            pnode = pnode->AsParseNodeBlock()->pnodeNext;
            break;

        case knopCatch:
            pnode = pnode->AsParseNodeCatch()->pnodeNext;
            break;

        case knopWith:
            pnode = pnode->AsParseNodeWith()->pnodeNext;
            break;

        default:
            Assert(UNREACHED);
        }
    }

    // scan for var declarations
    for (ParseNode *pnode = funcInfo->root->pnodeVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
    {
        Symbol* sym = pnode->AsParseNodeVar()->sym;

        if (sym == nullptr || pnode->AsParseNodeVar()->isBlockScopeFncDeclVar || sym->IsSpecialSymbol())
            continue;

        if (sym->GetIsCatch() || (pnode->nop == knopVarDecl && sym->GetIsBlockVar()))
        {
            // The init node was bound to the catch object, because it's inside a catch and has the
            // same name as the catch object. But we want to define a user var at function scope,
            // so find the right symbol. (We'll still assign the RHS value to the catch object symbol.)
            // This also applies to a var declaration in the same scope as a let declaration.

            // Assert that catch cannot be at function scope and let and var at function scope is redeclaration error.
            Assert(sym->GetIsCatch() || funcInfo->bodyScope != sym->GetScope());
            sym = funcInfo->bodyScope->FindLocalSymbol(sym->GetName());
            Assert(sym && !sym->GetIsCatch() && !sym->GetIsBlockVar());
        }

        Assert(sym->GetIsGlobal());

        if (sym->GetSymbolType() == STVariable)
        {
            emitRedeclCheck(sym, funcInfo);
        }
    }
}

void ByteCodeGenerator::RecordAllIntConstants(FuncInfo * funcInfo)
{
    Js::FunctionBody *byteCodeFunction = this->TopFuncInfo()->GetParsedFunctionBody();
    funcInfo->constantToRegister.Map([byteCodeFunction](unsigned int val, Js::RegSlot location)
    {
        byteCodeFunction->RecordIntConstant(byteCodeFunction->MapRegSlot(location), val);
    });
}

void ByteCodeGenerator::RecordAllStrConstants(FuncInfo * funcInfo)
{
    Js::FunctionBody *byteCodeFunction = this->TopFuncInfo()->GetParsedFunctionBody();
    funcInfo->stringToRegister.Map([byteCodeFunction](IdentPtr pid, Js::RegSlot location)
    {
        byteCodeFunction->RecordStrConstant(byteCodeFunction->MapRegSlot(location), pid->Psz(), pid->Cch(), pid->IsUsedInLdElem());
    });
}

void ByteCodeGenerator::RecordAllStringTemplateCallsiteConstants(FuncInfo* funcInfo)
{
    Js::FunctionBody *byteCodeFunction = this->TopFuncInfo()->GetParsedFunctionBody();
    funcInfo->stringTemplateCallsiteRegisterMap.Map([byteCodeFunction](ParseNodePtr pnode, Js::RegSlot location)
    {
        Js::ScriptContext* scriptContext = byteCodeFunction->GetScriptContext();
        Js::JavascriptLibrary* library = scriptContext->GetLibrary();
        Js::RecyclableObject* callsiteObject = library->TryGetStringTemplateCallsiteObject(pnode);

        if (callsiteObject == nullptr)
        {
            Js::RecyclableObject* rawArray = ByteCodeGenerator::BuildArrayFromStringList(pnode->AsParseNodeStrTemplate()->pnodeStringRawLiterals, pnode->AsParseNodeStrTemplate()->countStringLiterals, scriptContext);
            rawArray->Freeze();

            callsiteObject = ByteCodeGenerator::BuildArrayFromStringList(pnode->AsParseNodeStrTemplate()->pnodeStringLiterals, pnode->AsParseNodeStrTemplate()->countStringLiterals, scriptContext);
            callsiteObject->SetPropertyWithAttributes(Js::PropertyIds::raw, rawArray, PropertyNone, nullptr);
            callsiteObject->Freeze();

            library->AddStringTemplateCallsiteObject(callsiteObject);
        }

        byteCodeFunction->RecordConstant(byteCodeFunction->MapRegSlot(location), callsiteObject);
    });
}

bool IsApplyArgs(ParseNodeCall* callNode)
{
    ParseNode* target = callNode->pnodeTarget;
    ParseNode* args = callNode->pnodeArgs;
    if ((target != nullptr) && (target->nop == knopDot))
    {
        ParseNode* lhsNode = target->AsParseNodeBin()->pnode1;
        if ((lhsNode != nullptr) && ((lhsNode->nop == knopDot) || (lhsNode->nop == knopName)) && !IsArguments(lhsNode))
        {
            ParseNode* nameNode = target->AsParseNodeBin()->pnode2;
            if (nameNode != nullptr)
            {
                bool nameIsApply = nameNode->AsParseNodeName()->PropertyIdFromNameNode() == Js::PropertyIds::apply;
                if (nameIsApply && args != nullptr && args->nop == knopList)
                {
                    ParseNode* arg1 = args->AsParseNodeBin()->pnode1;
                    ParseNode* arg2 = args->AsParseNodeBin()->pnode2;
                    if ((arg1 != nullptr) && ByteCodeGenerator::IsThis(arg1) && (arg2 != nullptr) && (arg2->nop == knopName) && (arg2->AsParseNodeName()->sym != nullptr))
                    {
                        return arg2->AsParseNodeName()->sym->IsArguments();
                    }
                }
            }
        }
    }
    return false;
}

void PostCheckApplyEnclosesArgs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, ApplyCheck* applyCheck)
{
    if ((pnode == nullptr) || (!applyCheck->matches))
    {
        return;
    }

    if (pnode->nop == knopCall)
    {
        if ((!pnode->isUsed) && IsApplyArgs(pnode->AsParseNodeCall()))
        {
            if (!applyCheck->insideApplyCall)
            {
                applyCheck->matches = false;
            }
            applyCheck->insideApplyCall = false;
        }
    }
}

void CheckApplyEnclosesArgs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, ApplyCheck* applyCheck)
{
    if ((pnode == nullptr) || (!applyCheck->matches))
    {
        return;
    }

    switch (pnode->nop)
    {
    case knopName:
    {
        Symbol* sym = pnode->AsParseNodeName()->sym;
        if (sym != nullptr)
        {
            if (sym->IsArguments())
            {
                if (!applyCheck->insideApplyCall)
                {
                    applyCheck->matches = false;
                }
            }
        }
        break;
    }

    case knopCall:
        if ((!pnode->isUsed) && IsApplyArgs(pnode->AsParseNodeCall()))
        {
            // no nested apply calls
            if (applyCheck->insideApplyCall)
            {
                applyCheck->matches = false;
            }
            else
            {
                applyCheck->insideApplyCall = true;
                applyCheck->sawApply = true;
                pnode->AsParseNodeCall()->isApplyCall = true;
            }
        }
        break;
    }
}

unsigned int CountArguments(ParseNode *pnode, BOOL *pSideEffect = nullptr)
{
    // If the caller passed us a pSideEffect, it wants to know whether there are potential
    // side-effects in the argument list. We need to know this so that the call target
    // operands can be preserved if necessary.
    // For now, treat any non-leaf op as a potential side-effect. This causes no detectable slowdowns,
    // but we can be more precise if we need to be.
    if (pSideEffect)
    {
        *pSideEffect = FALSE;
    }

    unsigned int argCount = 1;
    if (pnode != nullptr)
    {
        while (pnode->nop == knopList)
        {
            argCount++;
            if (pSideEffect && !(ParseNode::Grfnop(pnode->AsParseNodeBin()->pnode1->nop) & fnopLeaf))
            {
                *pSideEffect = TRUE;
            }
            pnode = pnode->AsParseNodeBin()->pnode2;
        }
        argCount++;
        if (pSideEffect && !(ParseNode::Grfnop(pnode->nop) & fnopLeaf))
        {
            *pSideEffect = TRUE;
        }
    }

    AssertOrFailFastMsg(argCount < Js::Constants::UShortMaxValue, "Number of allowed arguments are already capped at parser level");

    return argCount;
}

void SaveOpndValue(ParseNode *pnode, FuncInfo *funcInfo)
{
    // Save a local name to a register other than its home location.
    // This guards against side-effects in cases like x.foo(x = bar()).
    Symbol *sym = nullptr;
    if (pnode->nop == knopName)
    {
        sym = pnode->AsParseNodeName()->sym;
    }
    else if (pnode->nop == knopComputedName)
    {
        ParseNode *pnode1 = pnode->AsParseNodeUni()->pnode1;
        if (pnode1->nop == knopName)
        {
            sym = pnode1->AsParseNodeName()->sym;
        }
    }

    if (sym == nullptr)
    {
        return;
    }

    // If the target is a local being kept in its home location,
    // protect the target's value in the event the home location is overwritten.
    if (pnode->location != Js::Constants::NoRegister &&
        sym->GetScope()->GetFunc() == funcInfo &&
        pnode->location == sym->GetLocation())
    {
        pnode->location = funcInfo->AcquireTmpRegister();
    }
}

void ByteCodeGenerator::StartStatement(ParseNode* node)
{
    Assert(TopFuncInfo() != nullptr);
    m_writer.StartStatement(node, TopFuncInfo()->curTmpReg - TopFuncInfo()->firstTmpReg);
}

void ByteCodeGenerator::EndStatement(ParseNode* node)
{
    m_writer.EndStatement(node);
}

void ByteCodeGenerator::StartSubexpression(ParseNode* node)
{
    Assert(TopFuncInfo() != nullptr);
    m_writer.StartSubexpression(node);
}

void ByteCodeGenerator::EndSubexpression(ParseNode* node)
{
    m_writer.EndSubexpression(node);
}

void EmitReference(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    // Generate code for the LHS of an assignment.
    switch (pnode->nop)
    {
    case knopDot:
        Emit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
        break;

    case knopIndex:
        Emit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
        Emit(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo, false);
        break;

    case knopName:
        break;

    case knopArrayPattern:
    case knopObjectPattern:
        break;

    case knopCall:
    case knopNew:
        // Emit the operands of a call that will be used as a LHS.
        // These have to be emitted before the RHS, but they have to persist until
        // the end of the expression.
        // Emit the call target operands first.
        switch (pnode->AsParseNodeCall()->pnodeTarget->nop)
        {
        case knopDot:
        case knopIndex:
            funcInfo->AcquireLoc(pnode->AsParseNodeCall()->pnodeTarget);
            EmitReference(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, funcInfo);
            break;

        case knopName:
        {
            Symbol *sym = pnode->AsParseNodeCall()->pnodeTarget->AsParseNodeName()->sym;
            if (!sym || sym->GetLocation() == Js::Constants::NoRegister)
            {
                funcInfo->AcquireLoc(pnode->AsParseNodeCall()->pnodeTarget);
            }
            if (sym && (sym->IsInSlot(byteCodeGenerator, funcInfo) || sym->GetScope()->GetFunc() != funcInfo))
            {
                // Can't get the value from the assigned register, so load it here.
                EmitLoad(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, funcInfo);
            }
            else
            {
                // EmitLoad will check for needsDeclaration and emit the Use Before Declaration error
                // bytecode op as necessary, but EmitReference does not check this (by design). So we
                // must manually check here.
                EmitUseBeforeDeclaration(pnode->AsParseNodeCall()->pnodeTarget->AsParseNodeName()->sym, byteCodeGenerator, funcInfo);
                EmitReference(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, funcInfo);
            }
            break;
        }
        default:
            EmitLoad(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, funcInfo);
            break;
        }

        // Now the arg list. We evaluate everything now and emit the ArgOut's later.
        if (pnode->AsParseNodeCall()->pnodeArgs)
        {
            ParseNode *pnodeArg = pnode->AsParseNodeCall()->pnodeArgs;
            while (pnodeArg->nop == knopList)
            {
                Emit(pnodeArg->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
                pnodeArg = pnodeArg->AsParseNodeBin()->pnode2;
            }
            Emit(pnodeArg, byteCodeGenerator, funcInfo, false);
        }

        if (pnode->AsParseNodeCall()->isSuperCall)
        {
            Emit(pnode->AsParseNodeSuperCall()->pnodeThis, byteCodeGenerator, funcInfo, false);
            Emit(pnode->AsParseNodeSuperCall()->pnodeNewTarget, byteCodeGenerator, funcInfo, false);
        }
        break;

    default:
        Emit(pnode, byteCodeGenerator, funcInfo, false);
        break;
    }
}

void EmitGetIterator(Js::RegSlot iteratorLocation, Js::RegSlot iterableLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);
void EmitIteratorNext(Js::RegSlot itemLocation, Js::RegSlot iteratorLocation, Js::RegSlot nextInputLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);
void EmitIteratorClose(Js::RegSlot iteratorLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);
void EmitIteratorComplete(Js::RegSlot doneLocation, Js::RegSlot iteratorResultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);
void EmitIteratorValue(Js::RegSlot valueLocation, Js::RegSlot iteratorResultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo);

void EmitDestructuredElement(ParseNode *elem, Js::RegSlot sourceLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo *funcInfo)
{
    switch (elem->nop)
    {
    case knopVarDecl:
    case knopLetDecl:
    case knopConstDecl:
        // We manually need to set NeedDeclaration since the node won't be visited.
        elem->AsParseNodeVar()->sym->SetNeedDeclaration(false);
        break;

    default:
        EmitReference(elem, byteCodeGenerator, funcInfo);
    }

    EmitAssignment(nullptr, elem, sourceLocation, byteCodeGenerator, funcInfo);
    funcInfo->ReleaseReference(elem);
}

void EmitDestructuredRestArray(ParseNode *elem,
    Js::RegSlot iteratorLocation,
    Js::RegSlot shouldCallReturnFunctionLocation,
    Js::RegSlot shouldCallReturnFunctionLocationFinally,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    Js::RegSlot restArrayLocation = funcInfo->AcquireTmpRegister();
    bool isAssignmentTarget = !(elem->AsParseNodeUni()->pnode1->IsPattern() || elem->AsParseNodeUni()->pnode1->IsVarLetOrConst());

    if (isAssignmentTarget)
    {
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocation);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocationFinally);
        EmitReference(elem->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);
    }

    byteCodeGenerator->Writer()->Reg1Unsigned1(
        Js::OpCode::NewScArray,
        restArrayLocation,
        ByteCodeGenerator::DefaultArraySize);

    // BytecodeGen can't convey to IRBuilder that some of the temporaries used here are live. When we
    // have a rest parameter, a counter is used in a loop for the array index, but there is no way to
    // convey this is live on the back edge.
    // As a workaround, we have a persistent var reg that is used for the loop counter
    Js::RegSlot counterLocation = elem->location;
    // TODO[ianhall]: Is calling EnregisterConstant() during Emit phase allowed?
    Js::RegSlot zeroConstantReg = byteCodeGenerator->EnregisterConstant(0);
    byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, counterLocation, zeroConstantReg);

    // loopTop:
    Js::ByteCodeLabel loopTop = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->MarkLabel(loopTop);

    Js::RegSlot itemLocation = funcInfo->AcquireTmpRegister();

    EmitIteratorNext(itemLocation, iteratorLocation, Js::Constants::NoRegister, byteCodeGenerator, funcInfo);

    Js::RegSlot doneLocation = funcInfo->AcquireTmpRegister();
    EmitIteratorComplete(doneLocation, itemLocation, byteCodeGenerator, funcInfo);

    Js::ByteCodeLabel iteratorDone = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, iteratorDone, doneLocation);

    Js::RegSlot valueLocation = funcInfo->AcquireTmpRegister();
    EmitIteratorValue(valueLocation, itemLocation, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocationFinally);

    byteCodeGenerator->Writer()->Element(
        ByteCodeGenerator::GetStElemIOpCode(funcInfo),
        valueLocation, restArrayLocation, counterLocation);
    funcInfo->ReleaseTmpRegister(valueLocation);
    funcInfo->ReleaseTmpRegister(doneLocation);
    funcInfo->ReleaseTmpRegister(itemLocation);

    byteCodeGenerator->Writer()->Reg2(Js::OpCode::Incr_A, counterLocation, counterLocation);

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

    byteCodeGenerator->Writer()->Br(loopTop);

    // iteratorDone:
    byteCodeGenerator->Writer()->MarkLabel(iteratorDone);

    ParseNode *restElem = elem->AsParseNodeUni()->pnode1;
    if (isAssignmentTarget)
    {
        EmitAssignment(nullptr, restElem, restArrayLocation, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseReference(restElem);
    }
    else
    {
        EmitDestructuredElement(restElem, restArrayLocation, byteCodeGenerator, funcInfo);
    }

    funcInfo->ReleaseTmpRegister(restArrayLocation);
}

void EmitDestructuredArray(
    ParseNode *lhs,
    Js::RegSlot rhsLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo);

void EmitIteratorCloseIfNotDone(Js::RegSlot iteratorLocation, Js::RegSlot doneLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    Js::ByteCodeLabel skipCloseLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, skipCloseLabel, doneLocation);

    EmitIteratorClose(iteratorLocation, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->MarkLabel(skipCloseLabel);
}

/*
  EmitDestructuredArray(lhsArray, rhs):
    iterator = rhs[@@iterator]

    if lhsArray empty
      return

    for each element in lhsArray except rest
      value = iterator.next()
      if element is a nested destructured array
        EmitDestructuredArray(element, value)
      else
        if value is undefined and there is an initializer
          evaluate initializer
          evaluate element reference
          element = initializer
        else
          element = value

    if lhsArray has a rest element
      rest = []
      while iterator is not done
        value = iterator.next()
        rest.append(value)
*/
void EmitDestructuredArrayCore(
    ParseNode *list,
    Js::RegSlot iteratorLocation,
    Js::RegSlot shouldCallReturnFunctionLocation,
    Js::RegSlot shouldCallReturnFunctionLocationFinally,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo
    )
{
    Assert(list != nullptr);

    ParseNode *elem = nullptr;
    while (list != nullptr)
    {
        ParseNode *init = nullptr;

        if (list->nop == knopList)
        {
            elem = list->AsParseNodeBin()->pnode1;
        }
        else
        {
            elem = list;
        }

        if (elem->nop == knopEllipsis)
        {
            break;
        }

        switch (elem->nop)
        {
        case knopAsg:
            // An assignment node will always have an initializer
            init = elem->AsParseNodeBin()->pnode2;
            elem = elem->AsParseNodeBin()->pnode1;
            break;

        case knopVarDecl:
        case knopLetDecl:
        case knopConstDecl:
            init = elem->AsParseNodeVar()->pnodeInit;
            break;

        default:
            break;
        }

        byteCodeGenerator->StartStatement(elem);

        bool isAssignmentTarget = !(elem->IsPattern() || elem->IsVarLetOrConst());

        if (isAssignmentTarget)
        {
            byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocation);
            byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocationFinally);
            EmitReference(elem, byteCodeGenerator, funcInfo);
        }

        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

        Js::RegSlot itemLocation = funcInfo->AcquireTmpRegister();
        EmitIteratorNext(itemLocation, iteratorLocation, Js::Constants::NoRegister, byteCodeGenerator, funcInfo);

        Js::RegSlot doneLocation = funcInfo->AcquireTmpRegister();
        EmitIteratorComplete(doneLocation, itemLocation, byteCodeGenerator, funcInfo);

        if (elem->nop == knopEmpty)
        {
            if (list->nop == knopList)
            {
                list = list->AsParseNodeBin()->pnode2;
                funcInfo->ReleaseTmpRegister(doneLocation);
                funcInfo->ReleaseTmpRegister(itemLocation);
                continue;
            }
            else
            {
                Assert(list->nop == knopEmpty);
                EmitIteratorCloseIfNotDone(iteratorLocation, doneLocation, byteCodeGenerator, funcInfo);
                funcInfo->ReleaseTmpRegister(doneLocation);
                funcInfo->ReleaseTmpRegister(itemLocation);
                break;
            }
        }

        // If the iterator hasn't completed, skip assigning undefined.
        Js::ByteCodeLabel iteratorAlreadyDone = byteCodeGenerator->Writer()->DefineLabel();
        byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, iteratorAlreadyDone, doneLocation);

        // We're not done with the iterator, so assign the .next() value.
        Js::RegSlot valueLocation = funcInfo->AcquireTmpRegister();
        EmitIteratorValue(valueLocation, itemLocation, byteCodeGenerator, funcInfo);
        Js::ByteCodeLabel beforeDefaultAssign = byteCodeGenerator->Writer()->DefineLabel();

        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocation);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocationFinally);
        byteCodeGenerator->Writer()->Br(beforeDefaultAssign);

        // iteratorAlreadyDone:
        byteCodeGenerator->Writer()->MarkLabel(iteratorAlreadyDone);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, valueLocation, funcInfo->undefinedConstantRegister);

        // beforeDefaultAssign:
        byteCodeGenerator->Writer()->MarkLabel(beforeDefaultAssign);

        if (elem->IsPattern())
        {
            // If we get an undefined value and have an initializer, use it in place of undefined.
            if (init != nullptr)
            {
                /*
                the IR builder uses two symbols for a temp register in the if else path
                R9 <- R3
                if (...)
                R9 <- R2
                R10 = R9.<property>  // error -> IR creates a new lifetime for the if path, and the direct path dest is not referenced
                hence we have to create a new temp

                TEMP REG USED TO FIX THIS PRODUCES THIS
                R9 <- R3
                if (BrEq_A R9, R3)
                R10 <- R2               :
                else
                R10 <- R9               : skipdefault
                ...  = R10[@@iterator]  : loadIter
                */

                // Temp Register
                Js::RegSlot valueLocationTmp = funcInfo->AcquireTmpRegister();
                byteCodeGenerator->StartStatement(init);

                Js::ByteCodeLabel skipDefault = byteCodeGenerator->Writer()->DefineLabel();
                Js::ByteCodeLabel loadIter = byteCodeGenerator->Writer()->DefineLabel();

                // check value is undefined
                byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrSrNeq_A, skipDefault, valueLocation, funcInfo->undefinedConstantRegister);

                // Evaluate the default expression and assign it.
                Emit(init, byteCodeGenerator, funcInfo, false);
                byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, valueLocationTmp, init->location);
                funcInfo->ReleaseLoc(init);

                // jmp to loadIter
                byteCodeGenerator->Writer()->Br(loadIter);

                // skipDefault:
                byteCodeGenerator->Writer()->MarkLabel(skipDefault);
                byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, valueLocationTmp, valueLocation);

                // loadIter:
                // @@iterator
                byteCodeGenerator->Writer()->MarkLabel(loadIter);
                byteCodeGenerator->EndStatement(init);

                if (elem->nop == knopObjectPattern)
                {
                    EmitDestructuredObject(elem, valueLocationTmp, byteCodeGenerator, funcInfo);
                }
                else
                {
                    // Recursively emit a destructured array using the current .next() as the RHS.
                    EmitDestructuredArray(elem, valueLocationTmp, byteCodeGenerator, funcInfo);
                }

                funcInfo->ReleaseTmpRegister(valueLocationTmp);
            }
            else
            {
                if (elem->nop == knopObjectPattern)
                {
                    EmitDestructuredObject(elem, valueLocation, byteCodeGenerator, funcInfo);
                }
                else
                {
                    // Recursively emit a destructured array using the current .next() as the RHS.
                    EmitDestructuredArray(elem, valueLocation, byteCodeGenerator, funcInfo);
                }
            }
        }
        else
        {
            EmitDestructuredValueOrInitializer(elem, valueLocation, init, isAssignmentTarget, byteCodeGenerator, funcInfo);
        }

        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

        if (list->nop != knopList)
        {
            EmitIteratorCloseIfNotDone(iteratorLocation, doneLocation, byteCodeGenerator, funcInfo);
        }

        funcInfo->ReleaseTmpRegister(valueLocation);
        funcInfo->ReleaseTmpRegister(doneLocation);
        funcInfo->ReleaseTmpRegister(itemLocation);

        if (isAssignmentTarget)
        {
            funcInfo->ReleaseReference(elem);
        }

        byteCodeGenerator->EndStatement(elem);

        if (list->nop == knopList)
        {
            list = list->AsParseNodeBin()->pnode2;
        }
        else
        {
            break;
        }
    }

    // If we saw a rest element, emit the rest array.
    if (elem != nullptr && elem->nop == knopEllipsis)
    {
        EmitDestructuredRestArray(elem,
            iteratorLocation,
            shouldCallReturnFunctionLocation,
            shouldCallReturnFunctionLocationFinally,
            byteCodeGenerator,
            funcInfo);
    }
}

// Generating
// try {
//    CallIteratorClose
// } catch (e) {
//    do nothing
// }

void EmitTryCatchAroundClose(
    Js::RegSlot iteratorLocation,
    Js::ByteCodeLabel endLabel,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    Js::ByteCodeLabel catchLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->Br(Js::OpCode::TryCatch, catchLabel);

    //
    // There is no need to add TryScopeRecord here as we are going to call 'return' function and there is not yield expression here.

    EmitIteratorClose(iteratorLocation, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
    byteCodeGenerator->Writer()->Br(endLabel);

    byteCodeGenerator->Writer()->MarkLabel(catchLabel);
    Js::RegSlot catchParamLocation = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::Catch, catchParamLocation);
    funcInfo->ReleaseTmpRegister(catchParamLocation);

    byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
}

struct ByteCodeGenerator::TryScopeRecord : public JsUtil::DoublyLinkedListElement<TryScopeRecord>
{
    Js::OpCode op;
    Js::ByteCodeLabel label;
    Js::RegSlot reg1;
    Js::RegSlot reg2;

    TryScopeRecord(Js::OpCode op, Js::ByteCodeLabel label) : op(op), label(label), reg1(Js::Constants::NoRegister), reg2(Js::Constants::NoRegister) { }
    TryScopeRecord(Js::OpCode op, Js::ByteCodeLabel label, Js::RegSlot r1, Js::RegSlot r2) : op(op), label(label), reg1(r1), reg2(r2) { }
};

// Generating
// catch(e) {
//      if (shouldCallReturn)
//          CallReturnWhichWrappedByTryCatch
//      throw e;
// }
void EmitTopLevelCatch(Js::ByteCodeLabel catchLabel,
    Js::RegSlot iteratorLocation,
    Js::RegSlot shouldCallReturnLocation,
    Js::RegSlot shouldCallReturnLocationFinally,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    Js::ByteCodeLabel afterCatchBlockLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
    byteCodeGenerator->Writer()->Br(afterCatchBlockLabel);
    byteCodeGenerator->Writer()->MarkLabel(catchLabel);

    Js::RegSlot catchParamLocation = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::Catch, catchParamLocation);

    ByteCodeGenerator::TryScopeRecord tryRecForCatch(Js::OpCode::ResumeCatch, catchLabel);
    if (funcInfo->byteCodeFunction->IsCoroutine())
    {
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForCatch);
    }

    Js::ByteCodeLabel skipCallCloseLabel = byteCodeGenerator->Writer()->DefineLabel();

    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, skipCallCloseLabel, shouldCallReturnLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnLocationFinally);
    EmitTryCatchAroundClose(iteratorLocation, skipCallCloseLabel, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->MarkLabel(skipCallCloseLabel);

    // Rethrow the exception.
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::Throw, catchParamLocation);

    funcInfo->ReleaseTmpRegister(catchParamLocation);

    if (funcInfo->byteCodeFunction->IsCoroutine())
    {
        byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
    }

    byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
    byteCodeGenerator->Writer()->MarkLabel(afterCatchBlockLabel);
}

// Generating
// finally {
//      if (shouldCallReturn)
//          CallReturn
// }

void EmitTopLevelFinally(Js::ByteCodeLabel finallyLabel,
    Js::RegSlot iteratorLocation,
    Js::RegSlot shouldCallReturnLocation,
    Js::RegSlot yieldExceptionLocation,
    Js::RegSlot yieldOffsetLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    bool isCoroutine = funcInfo->byteCodeFunction->IsCoroutine();

    Js::ByteCodeLabel afterFinallyBlockLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);

    byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(false);
    byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

    byteCodeGenerator->Writer()->Br(afterFinallyBlockLabel);
    byteCodeGenerator->Writer()->MarkLabel(finallyLabel);
    byteCodeGenerator->Writer()->Empty(Js::OpCode::Finally);

    ByteCodeGenerator::TryScopeRecord tryRecForFinally(Js::OpCode::ResumeFinally, finallyLabel, yieldExceptionLocation, yieldOffsetLocation);
    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForFinally);
    }

    Js::ByteCodeLabel skipCallCloseLabel = byteCodeGenerator->Writer()->DefineLabel();

    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, skipCallCloseLabel, shouldCallReturnLocation);
    EmitIteratorClose(iteratorLocation, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->MarkLabel(skipCallCloseLabel);

    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
        funcInfo->ReleaseTmpRegister(yieldOffsetLocation);
        funcInfo->ReleaseTmpRegister(yieldExceptionLocation);
    }

    byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(false);
    byteCodeGenerator->Writer()->Empty(Js::OpCode::LeaveNull);
    byteCodeGenerator->Writer()->MarkLabel(afterFinallyBlockLabel);
}

void EmitCatchAndFinallyBlocks(Js::ByteCodeLabel catchLabel,
    Js::ByteCodeLabel finallyLabel,
    Js::RegSlot iteratorLocation,
    Js::RegSlot shouldCallReturnFunctionLocation,
    Js::RegSlot shouldCallReturnFunctionLocationFinally,
    Js::RegSlot yieldExceptionLocation,
    Js::RegSlot yieldOffsetLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo
    )
{
    bool isCoroutine = funcInfo->byteCodeFunction->IsCoroutine();
    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
    }

    EmitTopLevelCatch(catchLabel,
        iteratorLocation,
        shouldCallReturnFunctionLocation,
        shouldCallReturnFunctionLocationFinally,
        byteCodeGenerator,
        funcInfo);

    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
    }

    EmitTopLevelFinally(finallyLabel,
        iteratorLocation,
        shouldCallReturnFunctionLocationFinally,
        yieldExceptionLocation,
        yieldOffsetLocation,
        byteCodeGenerator,
        funcInfo);

    funcInfo->ReleaseTmpRegister(shouldCallReturnFunctionLocationFinally);
    funcInfo->ReleaseTmpRegister(shouldCallReturnFunctionLocation);
}

// Emit a wrapper try..finaly block around the destructuring elements
void EmitDestructuredArray(
    ParseNode *lhs,
    Js::RegSlot rhsLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    byteCodeGenerator->StartStatement(lhs);
    Js::RegSlot iteratorLocation = funcInfo->AcquireTmpRegister();

    EmitGetIterator(iteratorLocation, rhsLocation, byteCodeGenerator, funcInfo);

    Assert(lhs->nop == knopArrayPattern);
    ParseNode *list = lhs->AsParseNodeArrLit()->pnode1;

    if (list == nullptr)
    { // Handline this case ([] = obj);
        EmitIteratorClose(iteratorLocation, byteCodeGenerator, funcInfo);

        // No elements to bind or assign.
        funcInfo->ReleaseTmpRegister(iteratorLocation);
        byteCodeGenerator->EndStatement(lhs);
        return;
    }

    // This variable facilitates on when to call the return function (which is Iterator close). When we are emitting bytecode for destructuring element
    // this variable will be set to true.
    Js::RegSlot shouldCallReturnFunctionLocation = funcInfo->AcquireTmpRegister();
    Js::RegSlot shouldCallReturnFunctionLocationFinally = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

    byteCodeGenerator->SetHasFinally(true);
    byteCodeGenerator->SetHasTry(true);
    byteCodeGenerator->TopFuncInfo()->byteCodeFunction->SetDontInline(true);

    Js::RegSlot regException = Js::Constants::NoRegister;
    Js::RegSlot regOffset = Js::Constants::NoRegister;
    bool isCoroutine = funcInfo->byteCodeFunction->IsCoroutine();

    if (isCoroutine)
    {
        regException = funcInfo->AcquireTmpRegister();
        regOffset = funcInfo->AcquireTmpRegister();
    }

    // Insert try node here
    Js::ByteCodeLabel finallyLabel = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel catchLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

    ByteCodeGenerator::TryScopeRecord tryRecForTryFinally(Js::OpCode::TryFinallyWithYield, finallyLabel);

    if (isCoroutine)
    {
        byteCodeGenerator->Writer()->BrReg2(Js::OpCode::TryFinallyWithYield, finallyLabel, regException, regOffset);
        tryRecForTryFinally.reg1 = regException;
        tryRecForTryFinally.reg2 = regOffset;
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTryFinally);
    }
    else
    {
        byteCodeGenerator->Writer()->Br(Js::OpCode::TryFinally, finallyLabel);
    }

    byteCodeGenerator->Writer()->Br(Js::OpCode::TryCatch, catchLabel);

    ByteCodeGenerator::TryScopeRecord tryRecForTry(Js::OpCode::TryCatch, catchLabel);
    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTry);
    }

    EmitDestructuredArrayCore(list,
        iteratorLocation,
        shouldCallReturnFunctionLocation,
        shouldCallReturnFunctionLocationFinally,
        byteCodeGenerator,
        funcInfo);

    EmitCatchAndFinallyBlocks(catchLabel,
        finallyLabel,
        iteratorLocation,
        shouldCallReturnFunctionLocation,
        shouldCallReturnFunctionLocationFinally,
        regException,
        regOffset,
        byteCodeGenerator,
        funcInfo);

    funcInfo->ReleaseTmpRegister(iteratorLocation);

    byteCodeGenerator->EndStatement(lhs);
}

void EmitNameInvoke(Js::RegSlot lhsLocation,
    Js::RegSlot objectLocation,
    ParseNodePtr nameNode,
    ByteCodeGenerator* byteCodeGenerator,
    FuncInfo* funcInfo)
{
    Assert(nameNode != nullptr);
    if (nameNode->nop == knopComputedName)
    {
        ParseNodePtr pnode1 = nameNode->AsParseNodeUni()->pnode1;
        Emit(pnode1, byteCodeGenerator, funcInfo, false/*isConstructorCall*/);

        byteCodeGenerator->Writer()->Element(Js::OpCode::LdElemI_A, lhsLocation, objectLocation, pnode1->location);
        funcInfo->ReleaseLoc(pnode1);
    }
    else
    {
        Assert(nameNode->nop == knopStr);
        Js::PropertyId propertyId = nameNode->AsParseNodeStr()->pid->GetPropertyId();

        uint cacheId = funcInfo->FindOrAddInlineCacheId(objectLocation, propertyId, false/*isLoadMethod*/, false/*isStore*/);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, lhsLocation, objectLocation, cacheId);
    }
}

void EmitDestructuredValueOrInitializer(ParseNodePtr lhsElementNode,
    Js::RegSlot rhsLocation,
    ParseNodePtr initializer,
    bool isNonPatternAssignmentTarget,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    // If we have initializer we need to see if the destructured value is undefined or not - if it is undefined we need to assign initializer

    Js::ByteCodeLabel useDefault = -1;
    Js::ByteCodeLabel end = -1;
    Js::RegSlot rhsLocationTmp = rhsLocation;

    if (initializer != nullptr)
    {
        rhsLocationTmp = funcInfo->AcquireTmpRegister();

        useDefault = byteCodeGenerator->Writer()->DefineLabel();
        end = byteCodeGenerator->Writer()->DefineLabel();

        byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrSrEq_A, useDefault, rhsLocation, funcInfo->undefinedConstantRegister);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, rhsLocationTmp, rhsLocation);

        byteCodeGenerator->Writer()->Br(end);
        byteCodeGenerator->Writer()->MarkLabel(useDefault);

        Emit(initializer, byteCodeGenerator, funcInfo, false/*isConstructorCall*/);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, rhsLocationTmp, initializer->location);
        funcInfo->ReleaseLoc(initializer);

        byteCodeGenerator->Writer()->MarkLabel(end);
    }

    if (lhsElementNode->nop == knopArrayPattern)
    {
        EmitDestructuredArray(lhsElementNode, rhsLocationTmp, byteCodeGenerator, funcInfo);
    }
    else if (lhsElementNode->nop == knopObjectPattern)
    {
        EmitDestructuredObject(lhsElementNode, rhsLocationTmp, byteCodeGenerator, funcInfo);
    }
    else if (isNonPatternAssignmentTarget)
    {
        EmitAssignment(nullptr, lhsElementNode, rhsLocationTmp, byteCodeGenerator, funcInfo);
    }
    else
    {
        EmitDestructuredElement(lhsElementNode, rhsLocationTmp, byteCodeGenerator, funcInfo);
    }

    if (initializer != nullptr)
    {
        funcInfo->ReleaseTmpRegister(rhsLocationTmp);
    }
}

void EmitDestructuredObjectMember(ParseNodePtr memberNode,
    Js::RegSlot rhsLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    Assert(memberNode->nop == knopObjectPatternMember);

    Js::RegSlot nameLocation = funcInfo->AcquireTmpRegister();
    EmitNameInvoke(nameLocation, rhsLocation, memberNode->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo);

    // Imagine we are transforming
    // {x:x1} = {} to x1 = {}.x  (here x1 is the second node of the member but that is our lhsnode)

    ParseNodePtr lhsElementNode = memberNode->AsParseNodeBin()->pnode2;
    ParseNodePtr init = nullptr;
    if (lhsElementNode->IsVarLetOrConst())
    {
        init = lhsElementNode->AsParseNodeVar()->pnodeInit;
    }
    else if (lhsElementNode->nop == knopAsg)
    {
        init = lhsElementNode->AsParseNodeBin()->pnode2;
        lhsElementNode = lhsElementNode->AsParseNodeBin()->pnode1;
    }

    EmitDestructuredValueOrInitializer(lhsElementNode, nameLocation, init, false /*isNonPatternAssignmentTarget*/, byteCodeGenerator, funcInfo);

    funcInfo->ReleaseTmpRegister(nameLocation);
}

void EmitDestructuredObject(ParseNode *lhs,
    Js::RegSlot rhsLocationOrig,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    Assert(lhs->nop == knopObjectPattern);
    ParseNodePtr pnode1 = lhs->AsParseNodeUni()->pnode1;

    byteCodeGenerator->StartStatement(lhs);

    Js::ByteCodeLabel skipThrow = byteCodeGenerator->Writer()->DefineLabel();
    Js::RegSlot rhsLocation = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, rhsLocation, rhsLocationOrig);
    byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrNeq_A, skipThrow, rhsLocation, funcInfo->undefinedConstantRegister);
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_ObjectCoercible));
    byteCodeGenerator->Writer()->MarkLabel(skipThrow);

    if (pnode1 != nullptr)
    {
        Assert(pnode1->nop == knopList || pnode1->nop == knopObjectPatternMember);

        ParseNodePtr current = pnode1;
        while (current->nop == knopList)
        {
            ParseNodePtr memberNode = current->AsParseNodeBin()->pnode1;
            EmitDestructuredObjectMember(memberNode, rhsLocation, byteCodeGenerator, funcInfo);
            current = current->AsParseNodeBin()->pnode2;
        }
        EmitDestructuredObjectMember(current, rhsLocation, byteCodeGenerator, funcInfo);
    }

    funcInfo->ReleaseTmpRegister(rhsLocation);
    byteCodeGenerator->EndStatement(lhs);
}

void EmitAssignment(
    ParseNode *asgnNode,
    ParseNode *lhs,
    Js::RegSlot rhsLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    switch (lhs->nop)
    {
        // assignment to a local or global variable
    case knopVarDecl:
    case knopLetDecl:
    case knopConstDecl:
    {
        Symbol *sym = lhs->AsParseNodeVar()->sym;
        Assert(sym != nullptr);
        byteCodeGenerator->EmitPropStore(rhsLocation, sym, nullptr, funcInfo, lhs->nop == knopLetDecl, lhs->nop == knopConstDecl);
        break;
    }

    case knopName:
    {
        // Special names like 'this' or 'new.target' cannot be assigned to
        ParseNodeName * pnodeNameLhs = lhs->AsParseNodeName();
        if (pnodeNameLhs->IsSpecialName())
        {
            byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_CantAssignTo));
        }
        else
        {
            byteCodeGenerator->EmitPropStore(rhsLocation, pnodeNameLhs->sym, pnodeNameLhs->pid, funcInfo);
        }
        break;
    }

    // x.y =
    case knopDot:
    {
        // PutValue(x, "y", rhs)
        Js::PropertyId propertyId = lhs->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();

        if (ByteCodeGenerator::IsSuper(lhs->AsParseNodeBin()->pnode1))
        {
            Emit(lhs->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
            Js::RegSlot tmpReg = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, lhs->AsParseNodeBin()->pnode1->location, funcInfo);
            funcInfo->ReleaseLoc(lhs->AsParseNodeSuperReference()->pnodeThis);
            uint cacheId = funcInfo->FindOrAddInlineCacheId(tmpReg, propertyId, false, true);
            byteCodeGenerator->Writer()->PatchablePropertyWithThisPtr(Js::OpCode::StSuperFld, rhsLocation, tmpReg, lhs->AsParseNodeSuperReference()->pnodeThis->location, cacheId);
        }
        else
        {
            uint cacheId = funcInfo->FindOrAddInlineCacheId(lhs->AsParseNodeBin()->pnode1->location, propertyId, false, true);
            byteCodeGenerator->Writer()->PatchableProperty(
                ByteCodeGenerator::GetStFldOpCode(funcInfo, false, false, false, false), rhsLocation, lhs->AsParseNodeBin()->pnode1->location, cacheId);
        }

        break;
    }

    case knopIndex:
    {
        Js::RegSlot targetLocation = lhs->AsParseNodeBin()->pnode1->location;

        if (ByteCodeGenerator::IsSuper(lhs->AsParseNodeBin()->pnode1))
        {
            // We need to emit the 'this' node for the super reference even if we aren't planning to use the 'this' value.
            // This is because we might be in a derived class constructor where we haven't yet called super() to bind the 'this' value.
            // See ecma262 abstract operation 'MakeSuperPropertyReference'
            Emit(lhs->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(lhs->AsParseNodeSuperReference()->pnodeThis);
            targetLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, targetLocation, funcInfo);
        }

        byteCodeGenerator->Writer()->Element(
            ByteCodeGenerator::GetStElemIOpCode(funcInfo),
            rhsLocation, targetLocation, lhs->AsParseNodeBin()->pnode2->location);

        break;
    }

    case knopObjectPattern:
    {
        Assert(byteCodeGenerator->IsES6DestructuringEnabled());
        // Copy the rhs value to be the result of the assignment if needed.
        if (asgnNode != nullptr)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, asgnNode->location, rhsLocation);
        }
        return EmitDestructuredObject(lhs, rhsLocation, byteCodeGenerator, funcInfo);
    }

    case knopArrayPattern:
    {
        Assert(byteCodeGenerator->IsES6DestructuringEnabled());
        // Copy the rhs value to be the result of the assignment if needed.
        if (asgnNode != nullptr)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, asgnNode->location, rhsLocation);
        }
        return EmitDestructuredArray(lhs, rhsLocation, byteCodeGenerator, funcInfo);
    }

    case knopArray:
    case knopObject:
        // Assignment to array/object can get through to byte code gen when the parser fails to convert destructuring
        // assignment to pattern (because of structural mismatch between LHS & RHS?). Revisit when we nail
        // down early vs. runtime errors for destructuring.
        byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_CantAssignTo));
        break;

    default:
        byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_CantAssignTo));
        break;
    }

    if (asgnNode != nullptr)
    {
        // We leave it up to the caller to pass this node only if the assignment expression is used.
        if (asgnNode->location != rhsLocation)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, asgnNode->location, rhsLocation);
        }
    }
}

void EmitLoad(
    ParseNode *lhs,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    // Emit the instructions to load the value into the LHS location. Do not assign/free any temps
    // in the process.
    // We usually get here as part of an op-equiv expression: x.y += z;
    // In such a case, x has to be emitted first, then the value of x.y loaded (by this function), then z emitted.
    switch (lhs->nop)
    {

        // load of a local or global variable
    case knopName:
    {
        funcInfo->AcquireLoc(lhs);
        byteCodeGenerator->EmitPropLoad(lhs->location, lhs->AsParseNodeName()->sym, lhs->AsParseNodeName()->pid, funcInfo);
        break;
    }

    // = x.y
    case knopDot:
    {
        // get field id for "y"
        Js::PropertyId propertyId = lhs->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();
        funcInfo->AcquireLoc(lhs);
        EmitReference(lhs, byteCodeGenerator, funcInfo);
        uint cacheId = funcInfo->FindOrAddInlineCacheId(lhs->AsParseNodeBin()->pnode1->location, propertyId, false, false);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, lhs->location, lhs->AsParseNodeBin()->pnode1->location, cacheId);
        break;
    }

    case knopIndex:
        funcInfo->AcquireLoc(lhs);
        EmitReference(lhs, byteCodeGenerator, funcInfo);
        byteCodeGenerator->Writer()->Element(
            Js::OpCode::LdElemI_A, lhs->location, lhs->AsParseNodeBin()->pnode1->location, lhs->AsParseNodeBin()->pnode2->location);
        break;

        // f(x) +=
    case knopCall:
    {
        ParseNodeCall * pnodeCallLhs = lhs->AsParseNodeCall();
        if (pnodeCallLhs->pnodeTarget->nop == knopImport)
        {
            ParseNodePtr args = pnodeCallLhs->pnodeArgs;
            Assert(CountArguments(args) == 2); // import() takes one argument
            Emit(args, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(args);
            funcInfo->AcquireLoc(pnodeCallLhs);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::ImportCall, pnodeCallLhs->location, args->location);
        }
        else
        {
            funcInfo->AcquireLoc(pnodeCallLhs);
            EmitReference(pnodeCallLhs, byteCodeGenerator, funcInfo);
            EmitCall(pnodeCallLhs, byteCodeGenerator, funcInfo, /*fReturnValue=*/ false, /*fEvaluateComponents=*/ false);
        }
        break;
    }
    default:
        funcInfo->AcquireLoc(lhs);
        Emit(lhs, byteCodeGenerator, funcInfo, false);
        break;
    }
}

void EmitList(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    if (pnode != nullptr)
    {
        while (pnode->nop == knopList)
        {
            byteCodeGenerator->EmitTopLevelStatement(pnode->AsParseNodeBin()->pnode1, funcInfo, false);
            pnode = pnode->AsParseNodeBin()->pnode2;
        }
        byteCodeGenerator->EmitTopLevelStatement(pnode, funcInfo, false);
    }
}

void EmitOneArg(
    ParseNode *pnode,
    BOOL fAssignRegs,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::ArgSlot &argIndex,
    Js::ArgSlot &spreadIndex,
    Js::RegSlot argTempLocation,
    bool emitProfiledArgout,
    Js::AuxArray<uint32> *spreadIndices = nullptr
)
{
    bool noArgOuts = argTempLocation != Js::Constants::NoRegister;

    // If this is a put, the arguments have already been evaluated (see EmitReference).
    // We just need to emit the ArgOut instructions.
    if (fAssignRegs)
    {
        Emit(pnode, byteCodeGenerator, funcInfo, false);
    }

    if (pnode->nop == knopEllipsis)
    {
        Assert(spreadIndices != nullptr);
        spreadIndices->elements[spreadIndex++] = argIndex + 1; // account for 'this'
        Js::RegSlot regVal = funcInfo->AcquireTmpRegister();
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::LdCustomSpreadIteratorList, regVal, pnode->location);
        if (noArgOuts)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, argTempLocation, regVal);
        }
        else
        {
            byteCodeGenerator->Writer()->ArgOut<true>(argIndex + 1, regVal, callSiteId, emitProfiledArgout);
        }
        funcInfo->ReleaseTmpRegister(regVal);
    }
    else
    {
        if (noArgOuts)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, argTempLocation, pnode->location);
        }
        else
        {
            byteCodeGenerator->Writer()->ArgOut<true>(argIndex + 1, pnode->location, callSiteId, emitProfiledArgout);
        }
    }
    argIndex++;

    if (fAssignRegs)
    {
        funcInfo->ReleaseLoc(pnode);
    }
}

size_t EmitArgsWithArgOutsAtEnd(
    ParseNode *pnode,
    BOOL fAssignRegs,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::RegSlot thisLocation,
    Js::ArgSlot argsCountForStartCall,
    bool emitProfiledArgouts,
    Js::AuxArray<uint32> *spreadIndices = nullptr
)
{
    AssertOrFailFast(pnode != nullptr);

    Js::ArgSlot argIndex = 0;
    Js::ArgSlot spreadIndex = 0;

    Js::RegSlot argTempLocation = funcInfo->AcquireTmpRegister();
    Js::RegSlot firstArgTempLocation = argTempLocation;

    while (pnode->nop == knopList)
    {
        EmitOneArg(pnode->AsParseNodeBin()->pnode1, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, argIndex, spreadIndex, argTempLocation, false /*emitProfiledArgout*/, spreadIndices);
        pnode = pnode->AsParseNodeBin()->pnode2;
        argTempLocation = funcInfo->AcquireTmpRegister();
    }

    EmitOneArg(pnode, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, argIndex, spreadIndex, argTempLocation, false /*emitProfiledArgout*/, spreadIndices);

    byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, argsCountForStartCall);

    // Emit all argOuts now

    if (thisLocation != Js::Constants::NoRegister)
    {
        // Emit the "this" object.
        byteCodeGenerator->Writer()->ArgOut<true>(0, thisLocation, callSiteId, false /*emitProfiledArgouts*/);
    }

    for (Js::ArgSlot index = 0; index < argIndex; index++)
    {
        byteCodeGenerator->Writer()->ArgOut<true>(index + 1, firstArgTempLocation + index, callSiteId, emitProfiledArgouts);
    }

    // Now release all those temps register
    for (Js::ArgSlot index = argIndex; index > 0; index--)
    {
        funcInfo->ReleaseTmpRegister(argTempLocation--);
    }

    return argIndex;
}

size_t EmitArgs(
    ParseNode *pnode,
    BOOL fAssignRegs,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    bool emitProfiledArgouts,
    Js::AuxArray<uint32> *spreadIndices = nullptr
    )
{
    Js::ArgSlot argIndex = 0;
    Js::ArgSlot spreadIndex = 0;

    if (pnode != nullptr)
    {
        while (pnode->nop == knopList)
        {
            EmitOneArg(pnode->AsParseNodeBin()->pnode1, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, argIndex, spreadIndex, Js::Constants::NoRegister, emitProfiledArgouts, spreadIndices);
            pnode = pnode->AsParseNodeBin()->pnode2;
        }

        EmitOneArg(pnode, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, argIndex, spreadIndex, Js::Constants::NoRegister, emitProfiledArgouts, spreadIndices);
    }

    return argIndex;
}



void EmitArgListStart(
    Js::RegSlot thisLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId)
{
    if (thisLocation != Js::Constants::NoRegister)
    {
        // Emit the "this" object.
        byteCodeGenerator->Writer()->ArgOut<true>(0, thisLocation, callSiteId, false /*emitProfiledArgout*/);
    }
}

Js::ArgSlot EmitArgListEnd(
    ParseNode *pnode,
    Js::RegSlot thisLocation,
    Js::RegSlot evalLocation,
    Js::RegSlot newTargetLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    size_t argIndex,
    Js::ProfileId callSiteId)
{
    BOOL fEvalInModule = false;
    BOOL fIsEval = (evalLocation != Js::Constants::NoRegister);
    BOOL fHasNewTarget = (newTargetLocation != Js::Constants::NoRegister);

    static const size_t maxExtraArgSlot = 4;  // max(extraEvalArg, extraArg), where extraEvalArg==2 (moduleRoot,env), extraArg==4 (this, eval, evalInModule, newTarget)
    AssertOrFailFastMsg(argIndex < Js::Constants::UShortMaxValue - maxExtraArgSlot, "Number of allowed arguments are already capped at parser level");

    Js::ArgSlot argSlotIndex = (Js::ArgSlot) argIndex;
    Js::ArgSlot evalIndex;

    if (fIsEval && argSlotIndex > 0)
    {
        Assert(!fHasNewTarget);

        // Pass the frame display as an extra argument to "eval".
        // Do this only if eval is called with some args
        Js::RegSlot evalEnv;
        if (funcInfo->IsGlobalFunction() && !(funcInfo->GetIsStrictMode() && byteCodeGenerator->GetFlags() & fscrEval))
        {
            // Use current environment as the environment for the function being called when:
            // - this is the root global function (not an eval's global function)
            // - this is an eval's global function that is not in strict mode (see else block)
            evalEnv = funcInfo->GetEnvRegister();
        }
        else
        {
            // Use the frame display as the environment for the function being called when:
            // - this is not a global function and thus it will have its own scope
            // - this is an eval's global function that is in strict mode, since in strict mode the eval's global function
            //   has its own scope
            evalEnv = funcInfo->frameDisplayRegister;
        }

        evalEnv = byteCodeGenerator->PrependLocalScopes(evalEnv, evalLocation, funcInfo);

        // Passing the FrameDisplay as an extra argument
        evalIndex = argSlotIndex + 1;

        if (evalEnv == funcInfo->GetEnvRegister() || evalEnv == funcInfo->frameDisplayRegister)
        {
            byteCodeGenerator->Writer()->ArgOutEnv(evalIndex);
        }
        else
        {
            byteCodeGenerator->Writer()->ArgOut<false>(evalIndex, evalEnv, callSiteId, false /*emitProfiledArgout*/);
        }
    }

    if (fHasNewTarget)
    {
        Assert(!fIsEval);

        byteCodeGenerator->Writer()->ArgOut<true>(argSlotIndex + 1, newTargetLocation, callSiteId, false /*emitProfiledArgout*/);
    }

    Js::ArgSlot argIntCount = argSlotIndex + 1 + (Js::ArgSlot)fIsEval + (Js::ArgSlot)fEvalInModule + (Js::ArgSlot)fHasNewTarget;

    // eval and no args passed, return 1 as argument count
    if (fIsEval && pnode == nullptr)
    {
        return 1;
    }

    return argIntCount;
}

Js::ArgSlot EmitArgList(
    ParseNode *pnode,
    Js::RegSlot thisLocation,
    Js::RegSlot newTargetLocation,
    BOOL fIsEval,
    BOOL fAssignRegs,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::ArgSlot argsCountForStartCall,
    bool emitArgOutsAtEnd,
    bool emitProfiledArgouts,
    uint16 spreadArgCount = 0,
    Js::AuxArray<uint32> **spreadIndices = nullptr)
{
    // This function emits the arguments for a call.
    // ArgOut's with uses immediately following defs.
    if (!emitArgOutsAtEnd)
    {
        byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, argsCountForStartCall);
        EmitArgListStart(thisLocation, byteCodeGenerator, funcInfo, callSiteId);
    }

    Js::RegSlot evalLocation = Js::Constants::NoRegister;

    //
    // If Emitting arguments for eval and assigning registers, get a tmpLocation for eval.
    // This would be used while generating frameDisplay in EmitArgListEnd.
    //
    if (fIsEval)
    {
        evalLocation = funcInfo->AcquireTmpRegister();
    }

    if (spreadArgCount > 0)
    {
        const size_t extraAlloc = UInt32Math::Mul(spreadArgCount, sizeof(uint32));
        Assert(spreadIndices != nullptr);
        *spreadIndices = AnewPlus(byteCodeGenerator->GetAllocator(), extraAlloc, Js::AuxArray<uint32>, spreadArgCount);
    }

    size_t argIndex = 0;
    if (emitArgOutsAtEnd)
    {
        argIndex = EmitArgsWithArgOutsAtEnd(pnode, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, thisLocation, argsCountForStartCall, emitProfiledArgouts, spreadIndices == nullptr ? nullptr : *spreadIndices);
    }
    else
    {
        argIndex = EmitArgs(pnode, fAssignRegs, byteCodeGenerator, funcInfo, callSiteId, emitProfiledArgouts, spreadIndices == nullptr ? nullptr : *spreadIndices);
    }

    Js::ArgSlot argumentsCount = EmitArgListEnd(pnode, thisLocation, evalLocation, newTargetLocation, byteCodeGenerator, funcInfo, argIndex, callSiteId);

    if (fIsEval)
    {
        funcInfo->ReleaseTmpRegister(evalLocation);
    }

    return argumentsCount;
}

void EmitConstantArgsToVarArray(ByteCodeGenerator *byteCodeGenerator, __out_ecount(argCount) Js::Var *vars, ParseNode *args, uint argCount)
{
    uint index = 0;
    while (args->nop == knopList && index < argCount)
    {
        if (args->AsParseNodeBin()->pnode1->nop == knopInt)
        {
            int value = args->AsParseNodeBin()->pnode1->AsParseNodeInt()->lw;
            vars[index++] = Js::TaggedInt::ToVarUnchecked(value);
        }
        else if (args->AsParseNodeBin()->pnode1->nop == knopFlt)
        {
            Js::Var number = Js::JavascriptNumber::New(args->AsParseNodeBin()->pnode1->AsParseNodeFloat()->dbl, byteCodeGenerator->GetScriptContext());
#if ! FLOATVAR
            byteCodeGenerator->GetScriptContext()->BindReference(number);
#endif
            vars[index++] = number;
        }
        else
        {
            AnalysisAssert(false);
        }
        args = args->AsParseNodeBin()->pnode2;
    }

    if (index == argCount)
    {
        Assert(false);
        Js::Throw::InternalError();
        return;
    }

    if (args->nop == knopInt)
    {
        int value = args->AsParseNodeInt()->lw;
        vars[index++] = Js::TaggedInt::ToVarUnchecked(value);
    }
    else if (args->nop == knopFlt)
    {
        Js::Var number = Js::JavascriptNumber::New(args->AsParseNodeFloat()->dbl, byteCodeGenerator->GetScriptContext());
#if ! FLOATVAR
        byteCodeGenerator->GetScriptContext()->BindReference(number);
#endif
        vars[index++] = number;
    }
    else
    {
        AnalysisAssert(false);
    }
}

void EmitConstantArgsToIntArray(ByteCodeGenerator *byteCodeGenerator, __out_ecount(argCount) int32 *vars, ParseNode *args, uint argCount)
{
    uint index = 0;
    while (args->nop == knopList && index < argCount)
    {
        Assert(args->AsParseNodeBin()->pnode1->nop == knopInt);
        vars[index++] = args->AsParseNodeBin()->pnode1->AsParseNodeInt()->lw;
        args = args->AsParseNodeBin()->pnode2;
    }

    if (index >= argCount)
    {
        Js::Throw::InternalError();
        return;
    }

    Assert(args->nop == knopInt);
    vars[index++] = args->AsParseNodeInt()->lw;

    Assert(index == argCount);
}

void EmitConstantArgsToFltArray(ByteCodeGenerator *byteCodeGenerator, __out_ecount(argCount) double *vars, ParseNode *args, uint argCount)
{
    uint index = 0;
    while (args->nop == knopList && index < argCount)
    {
        OpCode nop = args->AsParseNodeBin()->pnode1->nop;
        if (nop == knopInt)
        {
            vars[index++] = (double)args->AsParseNodeBin()->pnode1->AsParseNodeInt()->lw;
        }
        else
        {
            Assert(nop == knopFlt);
            vars[index++] = args->AsParseNodeBin()->pnode1->AsParseNodeFloat()->dbl;
        }
        args = args->AsParseNodeBin()->pnode2;
    }

    if (index >= argCount)
    {
        Js::Throw::InternalError();
        return;
    }

    if (args->nop == knopInt)
    {
        vars[index++] = (double)args->AsParseNodeInt()->lw;
    }
    else
    {
        Assert(args->nop == knopFlt);
        vars[index++] = args->AsParseNodeFloat()->dbl;
    }

    Assert(index == argCount);
}

//
// Called when we have new Ctr(constant, constant...)
//
Js::ArgSlot EmitNewObjectOfConstants(
    ParseNode *pnode,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    unsigned int argCount)
{
    EmitArgListStart(Js::Constants::NoRegister, byteCodeGenerator, funcInfo, Js::Constants::NoProfileId);

    // Create the vars array
    Js::VarArrayVarCount *vars = AnewPlus(byteCodeGenerator->GetAllocator(), UInt32Math::Mul((argCount - 1), sizeof(Js::Var)), Js::VarArrayVarCount, Js::TaggedInt::ToVarUnchecked(argCount - 1));

    // Emit all constants to the vars array
    EmitConstantArgsToVarArray(byteCodeGenerator, vars->elements, pnode->AsParseNodeCall()->pnodeArgs, argCount - 1);

    // Finish the arg list
    Js::ArgSlot actualArgCount = EmitArgListEnd(
        pnode->AsParseNodeCall()->pnodeArgs,
        Js::Constants::NoRegister,
        Js::Constants::NoRegister,
        Js::Constants::NoRegister,
        byteCodeGenerator,
        funcInfo,
        argCount - 1,
        Js::Constants::NoProfileId);

    // Make sure the cacheId to regSlot map in the ByteCodeWriter is left in a consistent state after writing NewScObject_A
    byteCodeGenerator->Writer()->RemoveEntryForRegSlotFromCacheIdMap(pnode->AsParseNodeCall()->pnodeTarget->location);

    // Generate the opcode with vars
    byteCodeGenerator->Writer()->AuxiliaryContext(
        Js::OpCode::NewScObject_A,
        funcInfo->AcquireLoc(pnode),
        vars,
        UInt32Math::MulAdd<sizeof(Js::Var), sizeof(Js::VarArray)>((argCount-1)),
        pnode->AsParseNodeCall()->pnodeTarget->location);


        AdeletePlus(byteCodeGenerator->GetAllocator(), UInt32Math::Mul((argCount-1), sizeof(Js::VarArrayVarCount)), vars);

    return actualArgCount;
}

void EmitMethodFld(bool isRoot, bool isScoped, Js::RegSlot location, Js::RegSlot callObjLocation, Js::PropertyId propertyId, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, bool registerCacheIdForCall = true)
{
    Js::OpCode opcode;
    if (!isRoot)
    {
        if (callObjLocation == funcInfo->frameObjRegister)
        {
            opcode = Js::OpCode::LdLocalMethodFld;
        }
        else
        {
            opcode = Js::OpCode::LdMethodFld;
        }
    }
    else if (isScoped)
    {
        opcode = Js::OpCode::ScopedLdMethodFld;
    }
    else
    {
        opcode = Js::OpCode::LdRootMethodFld;
    }

    if (isScoped || !isRoot)
    {
        Assert(isScoped || !isRoot || callObjLocation == ByteCodeGenerator::RootObjectRegister);
        uint cacheId = funcInfo->FindOrAddInlineCacheId(callObjLocation, propertyId, true, false);
        if (callObjLocation == funcInfo->frameObjRegister)
        {
            byteCodeGenerator->Writer()->ElementP(opcode, location, cacheId, false /*isCtor*/, registerCacheIdForCall);
        }
        else
        {
            byteCodeGenerator->Writer()->PatchableProperty(opcode, location, callObjLocation, cacheId, false /*isCtor*/, registerCacheIdForCall);
        }
    }
    else
    {
        uint cacheId = funcInfo->FindOrAddRootObjectInlineCacheId(propertyId, true, false);
        byteCodeGenerator->Writer()->PatchableRootProperty(opcode, location, cacheId, true, false, registerCacheIdForCall);
    }
}

void EmitMethodFld(ParseNode *pnode, Js::RegSlot callObjLocation, Js::PropertyId propertyId, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, bool registerCacheIdForCall = true)
{
    // Load a call target of the form x.y(). (Call target may be a plain knopName if we're getting it from
    // the global object, etc.)
    bool isRoot = pnode->nop == knopName && (pnode->AsParseNodeName()->sym == nullptr || pnode->AsParseNodeName()->sym->GetIsGlobal());
    bool isScoped = (byteCodeGenerator->GetFlags() & fscrEval) != 0 ||
        (isRoot && callObjLocation != ByteCodeGenerator::RootObjectRegister);

    EmitMethodFld(isRoot, isScoped, pnode->location, callObjLocation, propertyId, byteCodeGenerator, funcInfo, registerCacheIdForCall);
}

// lhs.apply(this, arguments);
void EmitApplyCall(ParseNodeCall* pnodeCall, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, BOOL fReturnValue)
{
    ParseNode* applyNode = pnodeCall->pnodeTarget;
    ParseNode* thisNode = pnodeCall->pnodeArgs->AsParseNodeBin()->pnode1;
    Assert(applyNode->nop == knopDot);

    ParseNode* funcNode = applyNode->AsParseNodeBin()->pnode1;
    Js::ByteCodeLabel slowPath = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel afterSlowPath = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel argsAlreadyCreated = byteCodeGenerator->Writer()->DefineLabel();

    Assert(applyNode->nop == knopDot);

    Emit(funcNode, byteCodeGenerator, funcInfo, false);

    funcInfo->AcquireLoc(applyNode);
    Js::PropertyId propertyId = applyNode->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();

    // As we won't be emitting a call instruction for apply, no need to register the cacheId for apply
    // load to be associated with the call. This is also required, as in the absence of a corresponding
    // call for apply, we won't remove the entry for "apply" cacheId from
    // ByteCodeWriter::callRegToLdFldCacheIndexMap, which is contrary to our assumption that we would
    // have removed an entry from a map upon seeing its corresponding call.
    EmitMethodFld(applyNode, funcNode->location, propertyId, byteCodeGenerator, funcInfo, false /*registerCacheIdForCall*/);

    Symbol *argSym = funcInfo->GetArgumentsSymbol();
    Assert(argSym && argSym->IsArguments());
    Js::RegSlot argumentsLoc = argSym->GetLocation();

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdArgumentsFromFrame, argumentsLoc);
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrNotNull_A, argsAlreadyCreated, argumentsLoc);

    // If apply is overridden, bail to slow path.
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFncNeqApply, slowPath, applyNode->location);

    // Note: acquire and release a temp register for this stack arg pointer instead of trying to stash it
    // in funcInfo->stackArgReg. Otherwise, we'll needlessly load and store it in jitted loop bodies and
    // may crash if we try to unbox it on the store.
    Js::RegSlot stackArgReg = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdStackArgPtr, stackArgReg);

    Js::RegSlot argCountLocation = funcInfo->AcquireTmpRegister();

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdArgCnt, argCountLocation);
    byteCodeGenerator->Writer()->Reg5(Js::OpCode::ApplyArgs, funcNode->location, funcNode->location, thisNode->location, stackArgReg, argCountLocation);

    funcInfo->ReleaseTmpRegister(argCountLocation);
    funcInfo->ReleaseTmpRegister(stackArgReg);
    funcInfo->ReleaseLoc(applyNode);
    funcInfo->ReleaseLoc(funcNode);

    // Clear these nodes as they are going to be used to re-generate the slow path.
    VisitClearTmpRegs(applyNode, byteCodeGenerator, funcInfo);
    VisitClearTmpRegs(funcNode, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->Br(afterSlowPath);

    // slow path
    byteCodeGenerator->Writer()->MarkLabel(slowPath);
    if (funcInfo->frameObjRegister != Js::Constants::NoRegister)
    {
        byteCodeGenerator->EmitScopeObjectInit(funcInfo);
    }
    byteCodeGenerator->LoadHeapArguments(funcInfo);

    byteCodeGenerator->Writer()->MarkLabel(argsAlreadyCreated);
    EmitCall(pnodeCall, byteCodeGenerator, funcInfo, fReturnValue, /*fEvaluateComponents*/true);
    byteCodeGenerator->Writer()->MarkLabel(afterSlowPath);
}

void EmitMethodElem(ParseNode *pnode, Js::RegSlot callObjLocation, Js::RegSlot indexLocation, ByteCodeGenerator *byteCodeGenerator)
{
    // Load a call target of the form x[y]().
    byteCodeGenerator->Writer()->Element(Js::OpCode::LdMethodElem, pnode->location, callObjLocation, indexLocation);
}

void EmitCallTargetNoEvalComponents(
    ParseNode *pnodeTarget,
    BOOL fSideEffectArgs,
    Js::RegSlot *thisLocation,
    bool *releaseThisLocation,
    Js::RegSlot *callObjLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    // We first get a reference to the call target, then evaluate the arguments, then
    // evaluate the call target.

    // - emit reference to target
    //    - copy instance to scratch reg if necessary.
    //    - assign this
    //    - assign instance for dynamic/global name
    // - emit args
    // - do call (CallFld/Elem/I)

    *releaseThisLocation = true;

    switch (pnodeTarget->nop)
    {
    case knopDot:
        *thisLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;
        *callObjLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;
        break;

    case knopIndex:
        *thisLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;
        *callObjLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;
        break;

    case knopName:
        // If the call target is a name, do some extra work to get its instance and the "this" pointer.
        byteCodeGenerator->EmitLoadInstance(pnodeTarget->AsParseNodeName()->sym, pnodeTarget->AsParseNodeName()->pid, thisLocation, callObjLocation, funcInfo);
        if (*thisLocation == Js::Constants::NoRegister)
        {
            *thisLocation = funcInfo->undefinedConstantRegister;
        }

        break;

    default:
        *thisLocation = funcInfo->undefinedConstantRegister;
        break;
    }
}

void EmitCallTarget(
    ParseNode *pnodeTarget,
    BOOL fSideEffectArgs,
    Js::RegSlot *thisLocation,
    bool *releaseThisLocation,
    Js::RegSlot *callObjLocation,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo)
{
    // - emit target
    //    - assign this
    // - emit args
    // - do call

    // The call target is fully evaluated before the argument list. Note that we're not handling
    // put-call cases here currently, as such cases only apply to host objects
    // and are very unlikely to behave differently depending on the order of evaluation.

    *releaseThisLocation = true;

    switch (pnodeTarget->nop)
    {
    case knopDot:
    {
        ParseNodeBin * pnodeBinTarget = pnodeTarget->AsParseNodeBin();
        funcInfo->AcquireLoc(pnodeBinTarget);
        // Assign the call target operand(s), putting them into expression temps if necessary to protect
        // them from side-effects.
        if (fSideEffectArgs)
        {
            // Though we're done with target evaluation after this point, still protect opnd1 from
            // arg side-effects as it's the "this" pointer.
            SaveOpndValue(pnodeBinTarget->pnode1, funcInfo);
        }

        Assert(pnodeBinTarget->pnode2->nop == knopName);
        if ((pnodeBinTarget->pnode2->AsParseNodeName()->PropertyIdFromNameNode() == Js::PropertyIds::apply) || (pnodeTarget->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode() == Js::PropertyIds::call))
        {
            pnodeBinTarget->pnode1->SetIsCallApplyTargetLoad();
        }

        Emit(pnodeBinTarget->pnode1, byteCodeGenerator, funcInfo, false);
        Js::PropertyId propertyId = pnodeBinTarget->pnode2->AsParseNodeName()->PropertyIdFromNameNode();
        Js::RegSlot protoLocation = pnodeBinTarget->pnode1->location;

        if (ByteCodeGenerator::IsSuper(pnodeBinTarget->pnode1))
        {
            Emit(pnodeBinTarget->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
            protoLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, protoLocation, funcInfo);
            funcInfo->ReleaseLoc(pnodeBinTarget->AsParseNodeSuperReference()->pnodeThis);
            funcInfo->ReleaseLoc(pnodeBinTarget->pnode1);

            // Function calls on the 'super' object should maintain current 'this' pointer
            *thisLocation = pnodeBinTarget->AsParseNodeSuperReference()->pnodeThis->location;
            *releaseThisLocation = false;
        }
        else
        {
            *thisLocation = pnodeBinTarget->pnode1->location;
        }

        EmitMethodFld(pnodeBinTarget, protoLocation, propertyId, byteCodeGenerator, funcInfo);
        break;
    }

    case knopIndex:
    {
        funcInfo->AcquireLoc(pnodeTarget);
        // Assign the call target operand(s), putting them into expression temps if necessary to protect
        // them from side-effects.
        if (fSideEffectArgs || !(ParseNode::Grfnop(pnodeTarget->AsParseNodeBin()->pnode2->nop) & fnopLeaf))
        {
            // Though we're done with target evaluation after this point, still protect opnd1 from
            // arg or opnd2 side-effects as it's the "this" pointer.
            SaveOpndValue(pnodeTarget->AsParseNodeBin()->pnode1, funcInfo);
        }
        Emit(pnodeTarget->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
        Emit(pnodeTarget->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo, false);

        Js::RegSlot indexLocation = pnodeTarget->AsParseNodeBin()->pnode2->location;
        Js::RegSlot protoLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;

        if (ByteCodeGenerator::IsSuper(pnodeTarget->AsParseNodeBin()->pnode1))
        {
            Emit(pnodeTarget->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
            protoLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, protoLocation, funcInfo);
            funcInfo->ReleaseLoc(pnodeTarget->AsParseNodeSuperReference()->pnodeThis);

            // Function calls on the 'super' object should maintain current 'this' pointer
            *thisLocation = pnodeTarget->AsParseNodeSuperReference()->pnodeThis->location;
            *releaseThisLocation = false;
        }
        else
        {
            *thisLocation = pnodeTarget->AsParseNodeBin()->pnode1->location;
        }

        EmitMethodElem(pnodeTarget, protoLocation, indexLocation, byteCodeGenerator);

        funcInfo->ReleaseLoc(pnodeTarget->AsParseNodeBin()->pnode2); // don't release indexLocation until after we use it.

        if (ByteCodeGenerator::IsSuper(pnodeTarget->AsParseNodeBin()->pnode1))
        {
            funcInfo->ReleaseLoc(pnodeTarget->AsParseNodeBin()->pnode1);
        }
        break;
    }

    case knopName:
    {
        ParseNodeName * pnodeNameTarget = pnodeTarget->AsParseNodeName();
        if (!pnodeNameTarget->IsSpecialName())
        {
            funcInfo->AcquireLoc(pnodeNameTarget);
            // Assign the call target operand(s), putting them into expression temps if necessary to protect
            // them from side-effects.
            if (fSideEffectArgs)
            {
                SaveOpndValue(pnodeNameTarget, funcInfo);
            }
            byteCodeGenerator->EmitLoadInstance(pnodeNameTarget->sym, pnodeNameTarget->pid, thisLocation, callObjLocation, funcInfo);
            if (*callObjLocation != Js::Constants::NoRegister)
            {
                // Load the call target as a property of the instance.
                Js::PropertyId propertyId = pnodeNameTarget->PropertyIdFromNameNode();
                EmitMethodFld(pnodeNameTarget, *callObjLocation, propertyId, byteCodeGenerator, funcInfo);
                break;
            }
        }

        // FALL THROUGH to evaluate call target.
    }

    default:
        // Assign the call target operand(s), putting them into expression temps if necessary to protect
        // them from side-effects.
        Emit(pnodeTarget, byteCodeGenerator, funcInfo, false);
        *thisLocation = funcInfo->undefinedConstantRegister;
        break;
    }

    // "This" pointer should have been assigned by the above.
    Assert(*thisLocation != Js::Constants::NoRegister);
}

void EmitCallI(
    ParseNodeCall *pnodeCall,
    BOOL fEvaluateComponents,
    BOOL fIsEval,
    BOOL fHasNewTarget,
    uint32 actualArgCount,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::AuxArray<uint32> *spreadIndices = nullptr)
{
    // Emit a call where the target is in a register, because it's either a local name or an expression we've
    // already evaluated.

    ParseNode *pnodeTarget = pnodeCall->pnodeTarget;
    Js::OpCode op;
    Js::CallFlags callFlags = Js::CallFlags::CallFlags_None;
    uint spreadExtraAlloc = 0;
    bool isSuperCall = pnodeCall->isSuperCall;

    Js::ArgSlot actualArgSlotCount = (Js::ArgSlot) actualArgCount;

    // check for integer overflow
    if ((size_t)actualArgSlotCount != actualArgCount)
    {
        Js::Throw::OutOfMemory();
    }

    
    if (fEvaluateComponents && !isSuperCall)
    {
        // Release the call target operands we assigned above. If we didn't assign them here,
        // we'll need them later, so we can't re-use them for the result of the call.
        funcInfo->ReleaseLoc(pnodeTarget);
    }
    // Grab a register for the call result.
    if (pnodeCall->isUsed)
    {
        funcInfo->AcquireLoc(pnodeCall);
    }

    if (fIsEval)
    {
        op = Js::OpCode::CallIExtendedFlags;
        callFlags = Js::CallFlags::CallFlags_ExtraArg;
    }
    else
    {
        if (isSuperCall)
        {
            callFlags = Js::CallFlags_New;
        }
        if (fHasNewTarget)
        {
            callFlags = (Js::CallFlags) (callFlags | Js::CallFlags::CallFlags_ExtraArg | Js::CallFlags::CallFlags_NewTarget);
        }

        if (pnodeCall->spreadArgCount > 0)
        {
            op = (isSuperCall || fHasNewTarget) ? Js::OpCode::CallIExtendedFlags : Js::OpCode::CallIExtended;
        }
        else
        {
            op = (isSuperCall || fHasNewTarget) ? Js::OpCode::CallIFlags : Js::OpCode::CallI;
        }
    }

    if (op == Js::OpCode::CallI || op == Js::OpCode::CallIFlags)
    {
        if (isSuperCall)
        {
            Js::RegSlot tmpReg = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdFuncObjProto, pnodeTarget->location, funcInfo);
            byteCodeGenerator->Writer()->CallI(op, pnodeCall->location, tmpReg, actualArgSlotCount, callSiteId, callFlags);
        }
        else
        {
            byteCodeGenerator->Writer()->CallI(op, pnodeCall->location, pnodeTarget->location, actualArgSlotCount, callSiteId, callFlags);
        }
    }
    else
    {
        uint spreadIndicesSize = 0;
        Js::CallIExtendedOptions options = Js::CallIExtended_None;

        if (pnodeCall->spreadArgCount > 0)
        {
            Assert(spreadIndices != nullptr);
            spreadExtraAlloc = UInt32Math::Mul(spreadIndices->count, sizeof(uint32));
            spreadIndicesSize = UInt32Math::Add(sizeof(*spreadIndices), spreadExtraAlloc);
            options = Js::CallIExtended_SpreadArgs;
        }

        if (isSuperCall)
        {
            Js::RegSlot tmpReg = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdFuncObjProto, pnodeTarget->location, funcInfo);
            byteCodeGenerator->Writer()->CallIExtended(op, pnodeCall->location, tmpReg, actualArgSlotCount, options, spreadIndices, spreadIndicesSize, callSiteId, callFlags);
        }
        else
        {
            byteCodeGenerator->Writer()->CallIExtended(op, pnodeCall->location, pnodeTarget->location, actualArgSlotCount, options, spreadIndices, spreadIndicesSize, callSiteId, callFlags);
        }
    }

    if (pnodeCall->spreadArgCount > 0)
    {
        Assert(spreadExtraAlloc != 0);
        AdeletePlus(byteCodeGenerator->GetAllocator(), spreadExtraAlloc, spreadIndices);
    }
}

void EmitCallInstrNoEvalComponents(
    ParseNodeCall *pnodeCall,
    BOOL fIsEval,
    Js::RegSlot thisLocation,
    Js::RegSlot callObjLocation,
    uint32 actualArgCount,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::AuxArray<uint32> *spreadIndices = nullptr)
{
    // Emit the call instruction. The call target is a reference at this point, and we evaluate
    // it as part of doing the actual call.
    // Note that we don't handle the (fEvaluateComponents == TRUE) case in this function.
    // (This function is only called on the !fEvaluateComponents branch in EmitCall.)

    ParseNode *pnodeTarget = pnodeCall->pnodeTarget;

    switch (pnodeTarget->nop)
    {
    case knopDot:
    {
        Assert(pnodeTarget->AsParseNodeBin()->pnode2->nop == knopName);
        Js::PropertyId propertyId = pnodeTarget->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();

        EmitMethodFld(pnodeTarget, callObjLocation, propertyId, byteCodeGenerator, funcInfo);
        EmitCallI(pnodeCall, /*fEvaluateComponents*/ FALSE, fIsEval, /*fHasNewTarget*/ FALSE, actualArgCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
    }
    break;

    case knopIndex:
    {
        EmitMethodElem(pnodeTarget, pnodeTarget->AsParseNodeBin()->pnode1->location, pnodeTarget->AsParseNodeBin()->pnode2->location, byteCodeGenerator);
        EmitCallI(pnodeCall, /*fEvaluateComponents*/ FALSE, fIsEval, /*fHasNewTarget*/ FALSE, actualArgCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
    }
    break;

    case knopName:
    {
        if (callObjLocation != Js::Constants::NoRegister)
        {
            // We still have to get the property from its instance, so emit CallFld.
            if (thisLocation != callObjLocation)
            {
                funcInfo->ReleaseTmpRegister(thisLocation);
            }
            funcInfo->ReleaseTmpRegister(callObjLocation);

            Js::PropertyId propertyId = pnodeTarget->AsParseNodeName()->PropertyIdFromNameNode();
            EmitMethodFld(pnodeTarget, callObjLocation, propertyId, byteCodeGenerator, funcInfo);
            EmitCallI(pnodeCall, /*fEvaluateComponents*/ FALSE, fIsEval, /*fHasNewTarget*/ FALSE, actualArgCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
            break;
        }
    }
    // FALL THROUGH

    default:
        EmitCallI(pnodeCall, /*fEvaluateComponents*/ FALSE, fIsEval, /*fHasNewTarget*/ FALSE, actualArgCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
        break;
    }
}

void EmitCallInstr(
    ParseNodeCall *pnodeCall,
    BOOL fIsEval,
    BOOL fHasNewTarget,
    Js::RegSlot thisLocation,
    Js::RegSlot callObjLocation,
    uint32 actualArgCount,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    Js::ProfileId callSiteId,
    Js::AuxArray<uint32> *spreadIndices = nullptr)
{
    // Emit a call instruction. The call target has been fully evaluated already, so we always
    // emit a CallI through the register that holds the target value.
    // Note that we don't handle !fEvaluateComponents cases at this point.
    // (This function is only called on the fEvaluateComponents branch in EmitCall.)

    if (thisLocation != Js::Constants::NoRegister)
    {
        funcInfo->ReleaseTmpRegister(thisLocation);
    }

    if (callObjLocation != Js::Constants::NoRegister &&
        callObjLocation != thisLocation)
    {
        funcInfo->ReleaseTmpRegister(callObjLocation);
    }

    EmitCallI(pnodeCall, /*fEvaluateComponents*/ TRUE, fIsEval, fHasNewTarget, actualArgCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
}

void EmitNew(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    Js::ArgSlot argCount = pnode->AsParseNodeCall()->argCount;
    argCount++; // include "this"

    BOOL fSideEffectArgs = FALSE;
    unsigned int tmpCount = CountArguments(pnode->AsParseNodeCall()->pnodeArgs, &fSideEffectArgs);
    AssertOrFailFastMsg(argCount == tmpCount, "argCount cannot overflow as max args capped at parser level");

    byteCodeGenerator->StartStatement(pnode);

    // Start call, allocate out param space
    funcInfo->StartRecordingOutArgs(argCount);

    // Assign the call target operand(s), putting them into expression temps if necessary to protect
    // them from side-effects.
    if (fSideEffectArgs)
    {
        SaveOpndValue(pnode->AsParseNodeCall()->pnodeTarget, funcInfo);
    }

    Emit(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, funcInfo, false, true);

    if (pnode->AsParseNodeCall()->pnodeArgs == nullptr)
    {
        funcInfo->ReleaseLoc(pnode->AsParseNodeCall()->pnodeTarget);
        Js::OpCode op = (CreateNativeArrays(byteCodeGenerator, funcInfo)
            && CallTargetIsArray(pnode->AsParseNodeCall()->pnodeTarget))
            ? Js::OpCode::NewScObjArray : Js::OpCode::NewScObject;
        Assert(argCount == 1);

        Js::ProfileId callSiteId = byteCodeGenerator->GetNextCallSiteId(op);
        byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, argCount);
        byteCodeGenerator->Writer()->CallI(op, funcInfo->AcquireLoc(pnode),
            pnode->AsParseNodeCall()->pnodeTarget->location, argCount, callSiteId);
    }
    else
    {
        uint32 actualArgCount = 0;

        if (IsCallOfConstants(pnode))
        {
            byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, argCount);
            funcInfo->ReleaseLoc(pnode->AsParseNodeCall()->pnodeTarget);
            actualArgCount = EmitNewObjectOfConstants(pnode, byteCodeGenerator, funcInfo, argCount);
        }
        else
        {
            Js::OpCode op;
            if ((CreateNativeArrays(byteCodeGenerator, funcInfo) && CallTargetIsArray(pnode->AsParseNodeCall()->pnodeTarget)))
            {
                op = pnode->AsParseNodeCall()->spreadArgCount > 0 ? Js::OpCode::NewScObjArraySpread : Js::OpCode::NewScObjArray;
            }
            else
            {
                op = pnode->AsParseNodeCall()->spreadArgCount > 0 ? Js::OpCode::NewScObjectSpread : Js::OpCode::NewScObject;
            }

            Js::ProfileId callSiteId = byteCodeGenerator->GetNextCallSiteId(op);

            // Only emit profiled argouts if we're going to profile this call.
            bool emitProfiledArgouts = callSiteId != byteCodeGenerator->GetCurrentCallSiteId();

            Js::AuxArray<uint32> *spreadIndices = nullptr;

            actualArgCount = EmitArgList(pnode->AsParseNodeCall()->pnodeArgs, Js::Constants::NoRegister, Js::Constants::NoRegister,
                false, true, byteCodeGenerator, funcInfo, callSiteId, argCount, pnode->AsParseNodeCall()->hasDestructuring, emitProfiledArgouts, pnode->AsParseNodeCall()->spreadArgCount, &spreadIndices);

            funcInfo->ReleaseLoc(pnode->AsParseNodeCall()->pnodeTarget);

            if (pnode->AsParseNodeCall()->spreadArgCount > 0)
            {
                Assert(spreadIndices != nullptr);
                uint spreadExtraAlloc = UInt32Math::Mul(spreadIndices->count, sizeof(uint32));
                uint spreadIndicesSize = UInt32Math::Add(sizeof(*spreadIndices), spreadExtraAlloc);
                byteCodeGenerator->Writer()->CallIExtended(op, funcInfo->AcquireLoc(pnode), pnode->AsParseNodeCall()->pnodeTarget->location,
                    (uint16)actualArgCount, Js::CallIExtended_SpreadArgs,
                    spreadIndices, spreadIndicesSize, callSiteId);
            }
            else
            {
                byteCodeGenerator->Writer()->CallI(op, funcInfo->AcquireLoc(pnode), pnode->AsParseNodeCall()->pnodeTarget->location,
                    (uint16)actualArgCount, callSiteId);
            }
        }

        Assert(argCount == actualArgCount);
    }

    // End call, pop param space
    funcInfo->EndRecordingOutArgs(argCount);
    return;
}

void EmitCall(
    ParseNodeCall * pnodeCall,
    ByteCodeGenerator* byteCodeGenerator,
    FuncInfo* funcInfo,
    BOOL fReturnValue,
    BOOL fEvaluateComponents,
    Js::RegSlot overrideThisLocation,
    Js::RegSlot newTargetLocation)
{
    // If the call returns a float, we'll note this in the byte code.
    Js::RegSlot thisLocation = Js::Constants::NoRegister;
    Js::RegSlot callObjLocation = Js::Constants::NoRegister;
    BOOL fHasNewTarget = newTargetLocation != Js::Constants::NoRegister;
    BOOL fSideEffectArgs = FALSE;
    BOOL fIsSuperCall = pnodeCall->isSuperCall;
    ParseNode *pnodeTarget = pnodeCall->pnodeTarget;
    ParseNode *pnodeArgs = pnodeCall->pnodeArgs;
    uint16 spreadArgCount = pnodeCall->spreadArgCount;

    if (CreateNativeArrays(byteCodeGenerator, funcInfo) && CallTargetIsArray(pnodeTarget)) {
        // some minifiers (potentially incorrectly) assume that "v = new Array()" and "v = Array()" are equivalent,
        // and replace the former with the latter to save 4 characters. What that means for us is that it, at least
        // initially, uses the "Call" path. We want to guess that it _is_ just "new Array()" and change over to the
        // "new" path, since then our native array handling can kick in.
        /*EmitNew(pnode, byteCodeGenerator, funcInfo);
        return;*/
    }

    unsigned int argCount = CountArguments(pnodeArgs, &fSideEffectArgs);

    BOOL fIsEval = pnodeCall->isEvalCall;
    Js::ArgSlot argSlotCount = (Js::ArgSlot)argCount;

    if (fIsEval)
    {
        Assert(!fHasNewTarget);

        //
        // "eval" takes the closure environment as an extra argument
        // Pass the closure env only if some argument is passed
        // For just eval(), don't pass the closure environment
        //
        if (argCount > 1)
        {
            argCount++;
        }
    }
    else if (fHasNewTarget)
    {
        // When we need to pass new.target explicitly, it is passed as an extra argument.
        // This is similar to how eval passes an extra argument for the frame display and is
        // used to support cases where we need to pass both 'this' and new.target as part of
        // a function call.
        // OpCode::LdNewTarget knows how to look at the call flags and fetch this argument.
        argCount++;
    }

    // argCount indicates the total arguments count including the extra arguments.
    // argSlotCount indicates the actual arguments count. So argCount should always never be les sthan argSlotCount.
    if (argCount < (unsigned int)argSlotCount)
    {
        Js::Throw::OutOfMemory();
    }

    if (fReturnValue)
    {
        pnodeCall->isUsed = true;
    }

    //
    // Set up the call.
    //

    bool releaseThisLocation = true;

    // We already emit the call target for super calls in EmitSuperCall
    if (!fIsSuperCall)
    {
        if (!fEvaluateComponents)
        {
            EmitCallTargetNoEvalComponents(pnodeTarget, fSideEffectArgs, &thisLocation, &releaseThisLocation, &callObjLocation, byteCodeGenerator, funcInfo);
        }
        else
        {
            EmitCallTarget(pnodeTarget, fSideEffectArgs, &thisLocation, &releaseThisLocation, &callObjLocation, byteCodeGenerator, funcInfo);
        }
    }

    // If we are strictly overriding the this location, ignore what the call target set this location to.
    if (overrideThisLocation != Js::Constants::NoRegister)
    {
        thisLocation = overrideThisLocation;
        releaseThisLocation = false;
    }

    // Evaluate the arguments (nothing mode-specific here).
    // Start call, allocate out param space
    // We have to use the arguments count including the extra args to Start Call as we use it to allocated space for all the args
    funcInfo->StartRecordingOutArgs(argCount);

    Js::ProfileId callSiteId = byteCodeGenerator->GetNextCallSiteId(Js::OpCode::CallI);

    // Only emit profiled argouts if we're going to allocate callSiteInfo (on the DynamicProfileInfo) for this call.
    bool emitProfiledArgouts = callSiteId != byteCodeGenerator->GetCurrentCallSiteId();
    Js::AuxArray<uint32> *spreadIndices;
    EmitArgList(pnodeArgs, thisLocation, newTargetLocation, fIsEval, fEvaluateComponents, byteCodeGenerator, funcInfo, callSiteId, (Js::ArgSlot)argCount, pnodeCall->hasDestructuring, emitProfiledArgouts, spreadArgCount, &spreadIndices);

    if (!fEvaluateComponents)
    {
        EmitCallInstrNoEvalComponents(pnodeCall, fIsEval, thisLocation, callObjLocation, argSlotCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
    }
    else
    {
        EmitCallInstr(pnodeCall, fIsEval, fHasNewTarget, releaseThisLocation ? thisLocation : Js::Constants::NoRegister, callObjLocation, argSlotCount, byteCodeGenerator, funcInfo, callSiteId, spreadIndices);
    }

    // End call, pop param space
    funcInfo->EndRecordingOutArgs((Js::ArgSlot)argCount);
}

void EmitInvoke(
    Js::RegSlot location,
    Js::RegSlot callObjLocation,
    Js::PropertyId propertyId,
    ByteCodeGenerator* byteCodeGenerator,
    FuncInfo* funcInfo)
{
    EmitMethodFld(false, false, location, callObjLocation, propertyId, byteCodeGenerator, funcInfo);

    funcInfo->StartRecordingOutArgs(1);

    Js::ProfileId callSiteId = byteCodeGenerator->GetNextCallSiteId(Js::OpCode::CallI);

    byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, 1);
    EmitArgListStart(callObjLocation, byteCodeGenerator, funcInfo, callSiteId);

    byteCodeGenerator->Writer()->CallI(Js::OpCode::CallI, location, location, 1, callSiteId);
}

void EmitInvoke(
    Js::RegSlot location,
    Js::RegSlot callObjLocation,
    Js::PropertyId propertyId,
    ByteCodeGenerator* byteCodeGenerator,
    FuncInfo* funcInfo,
    Js::RegSlot arg1Location)
{
    EmitMethodFld(false, false, location, callObjLocation, propertyId, byteCodeGenerator, funcInfo);

    funcInfo->StartRecordingOutArgs(2);

    Js::ProfileId callSiteId = byteCodeGenerator->GetNextCallSiteId(Js::OpCode::CallI);

    byteCodeGenerator->Writer()->StartCall(Js::OpCode::StartCall, 2);
    EmitArgListStart(callObjLocation, byteCodeGenerator, funcInfo, callSiteId);
    byteCodeGenerator->Writer()->ArgOut<true>(1, arg1Location, callSiteId, false /*emitProfiledArgout*/);

    byteCodeGenerator->Writer()->CallI(Js::OpCode::CallI, location, location, 2, callSiteId);
}

void EmitMemberNode(ParseNode *memberNode, Js::RegSlot objectLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, ParseNode* parentNode, bool useStore, bool* isObjectEmpty = nullptr)
{
    ParseNode *nameNode = memberNode->AsParseNodeBin()->pnode1;
    ParseNode *exprNode = memberNode->AsParseNodeBin()->pnode2;

    bool isFncDecl = exprNode->nop == knopFncDecl;
    bool isClassMember = isFncDecl && exprNode->AsParseNodeFnc()->IsClassMember();

    if (isFncDecl)
    {
        Assert(exprNode->AsParseNodeFnc()->HasHomeObj());
        exprNode->AsParseNodeFnc()->SetHomeObjLocation(objectLocation);
    }

    Js::RegSlot computedNamePropertyKey = Js::Constants::NoRegister;

    // Moved SetComputedNameVar before LdFld of prototype because loading the prototype undefers the function TypeHandler
    // which makes this bytecode too late to influence the function.name.
    if (nameNode->nop == knopComputedName)
    {
        // Computed property name
        // Transparently pass the name expr
        // The Emit will replace this with a temp register if necessary to preserve the value.
        nameNode->location = nameNode->AsParseNodeUni()->pnode1->location;
        computedNamePropertyKey = funcInfo->AcquireTmpRegister();

        EmitBinaryOpnds(nameNode, exprNode, byteCodeGenerator, funcInfo, computedNamePropertyKey);

        if (isFncDecl && !exprNode->AsParseNodeFnc()->IsClassConstructor() && exprNode->AsParseNodeFnc()->pnodeName == nullptr)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::SetComputedNameVar, exprNode->location, computedNamePropertyKey);
        }
    }

    // Classes allocates a RegSlot as part of Instance Methods EmitClassInitializers,
    // but if we don't have any members then we don't need to load the prototype.
    Assert(isClassMember == (isObjectEmpty != nullptr));
    if (isClassMember && *isObjectEmpty)
    {
        *isObjectEmpty = false;
        int cacheId = funcInfo->FindOrAddInlineCacheId(parentNode->location, Js::PropertyIds::prototype, false, false);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, objectLocation, parentNode->location, cacheId);
    }

    if (nameNode->nop == knopComputedName)
    {
        AssertOrFailFast(memberNode->nop == knopGetMember || memberNode->nop == knopSetMember || memberNode->nop == knopMember);

        Js::OpCode setOp = memberNode->nop == knopGetMember ?
            (isClassMember ? Js::OpCode::InitClassMemberGetComputedName : Js::OpCode::InitGetElemI) :
            memberNode->nop == knopSetMember ?
            (isClassMember ? Js::OpCode::InitClassMemberSetComputedName : Js::OpCode::InitSetElemI) :
            (isClassMember ? Js::OpCode::InitClassMemberComputedName : Js::OpCode::InitComputedProperty);

        byteCodeGenerator->Writer()->Element(setOp, exprNode->location, objectLocation, computedNamePropertyKey, true);

        funcInfo->ReleaseLoc(exprNode);
        funcInfo->ReleaseLoc(nameNode);
        funcInfo->ReleaseTmpRegister(computedNamePropertyKey);

        return;
    }

    Js::OpCode stFldOpCode = (Js::OpCode)0;
    if (useStore)
    {
        stFldOpCode = ByteCodeGenerator::GetStFldOpCode(funcInfo, false, false, false, isClassMember);
    }

    Emit(exprNode, byteCodeGenerator, funcInfo, false);
    Js::PropertyId propertyId = nameNode->AsParseNodeStr()->pid->GetPropertyId();

    if (Js::PropertyIds::name == propertyId
        && exprNode->nop == knopFncDecl
        && exprNode->AsParseNodeFnc()->IsStaticMember()
        && parentNode != nullptr && parentNode->nop == knopClassDecl
        && parentNode->AsParseNodeClass()->pnodeConstructor != nullptr)
    {
        Js::ParseableFunctionInfo* nameFunc = parentNode->AsParseNodeClass()->pnodeConstructor->funcInfo->byteCodeFunction->GetParseableFunctionInfo();
        nameFunc->SetIsStaticNameFunction(true);
    }

    if (memberNode->nop == knopMember || memberNode->nop == knopMemberShort)
    {
        // The internal prototype should be set only if the production is of the form PropertyDefinition : PropertyName : AssignmentExpression
        if (propertyId == Js::PropertyIds::__proto__ && memberNode->nop != knopMemberShort && (exprNode->nop != knopFncDecl || !exprNode->AsParseNodeFnc()->IsMethod()))
        {
            byteCodeGenerator->Writer()->Property(Js::OpCode::InitProto, exprNode->location, objectLocation,
                funcInfo->FindOrAddReferencedPropertyId(propertyId));
        }
        else
        {
            uint cacheId = funcInfo->FindOrAddInlineCacheId(objectLocation, propertyId, false, true);
            Js::OpCode patchablePropertyOpCode;

            if (useStore)
            {
                patchablePropertyOpCode = stFldOpCode;
            }
            else if (isClassMember)
            {
                patchablePropertyOpCode = Js::OpCode::InitClassMember;
            }
            else
            {
                patchablePropertyOpCode = Js::OpCode::InitFld;
            }

            byteCodeGenerator->Writer()->PatchableProperty(patchablePropertyOpCode, exprNode->location, objectLocation, cacheId);
        }
    }
    else
    {
        AssertOrFailFast(memberNode->nop == knopGetMember || memberNode->nop == knopSetMember);

        Js::OpCode setOp = memberNode->nop == knopGetMember ?
            (isClassMember ? Js::OpCode::InitClassMemberGet : Js::OpCode::InitGetFld) :
            (isClassMember ? Js::OpCode::InitClassMemberSet : Js::OpCode::InitSetFld);

        byteCodeGenerator->Writer()->Property(setOp, exprNode->location, objectLocation, funcInfo->FindOrAddReferencedPropertyId(propertyId));
    }

    funcInfo->ReleaseLoc(exprNode);

    if (propertyId == Js::PropertyIds::valueOf)
    {
        byteCodeGenerator->GetScriptContext()->optimizationOverrides.SetSideEffects(Js::SideEffects_ValueOf);
    }
    else if (propertyId == Js::PropertyIds::toString)
    {
        byteCodeGenerator->GetScriptContext()->optimizationOverrides.SetSideEffects(Js::SideEffects_ToString);
    }
}

void EmitClassInitializers(ParseNode *memberList, Js::RegSlot objectLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, ParseNode* parentNode, bool isObjectEmpty)
{
    if (memberList != nullptr)
    {
        while (memberList->nop == knopList)
        {
            ParseNode *memberNode = memberList->AsParseNodeBin()->pnode1;
            EmitMemberNode(memberNode, objectLocation, byteCodeGenerator, funcInfo, parentNode, /*useStore*/ false, &isObjectEmpty);
            memberList = memberList->AsParseNodeBin()->pnode2;
        }
        EmitMemberNode(memberList, objectLocation, byteCodeGenerator, funcInfo, parentNode, /*useStore*/ false, &isObjectEmpty);
    }
}

void EmitObjectInitializers(ParseNode *memberList, Js::RegSlot objectLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    ParseNode *pmemberList = memberList;
    unsigned int argCount = 0;
    uint32  value;
    Js::PropertyId propertyId;

    //
    // 1. Add all non-int property ids to a dictionary propertyIds with value true
    // 2. Get the count of propertyIds
    // 3. Create a propertyId array of size count
    // 4. Put the propIds in the auxiliary area
    // 5. Get the objectLiteralCacheId
    // 6. Generate propId inits with values
    //

    // Handle propertyId collision
    typedef JsUtil::BaseHashSet<Js::PropertyId, ArenaAllocator, PowerOf2SizePolicy> PropertyIdSet;
    PropertyIdSet* propertyIds = Anew(byteCodeGenerator->GetAllocator(), PropertyIdSet, byteCodeGenerator->GetAllocator(), 17);

    bool hasComputedName = false;
    if (memberList != nullptr)
    {
        while (memberList->nop == knopList)
        {
            if (memberList->AsParseNodeBin()->pnode1->AsParseNodeBin()->pnode1->nop == knopComputedName)
            {
                hasComputedName = true;
                break;
            }

            propertyId = memberList->AsParseNodeBin()->pnode1->AsParseNodeBin()->pnode1->AsParseNodeStr()->pid->GetPropertyId();
            if (!byteCodeGenerator->GetScriptContext()->IsNumericPropertyId(propertyId, &value))
            {
                propertyIds->Item(propertyId);
            }

            memberList = memberList->AsParseNodeBin()->pnode2;
        }

        if (memberList->AsParseNodeBin()->pnode1->nop != knopComputedName && !hasComputedName)
        {
            propertyId = memberList->AsParseNodeBin()->pnode1->AsParseNodeStr()->pid->GetPropertyId();
            if (!byteCodeGenerator->GetScriptContext()->IsNumericPropertyId(propertyId, &value))
            {
                propertyIds->Item(propertyId);
            }
        }
    }

    argCount = propertyIds->Count();

    memberList = pmemberList;
    if ((memberList == nullptr) || (argCount == 0))
    {
        // Empty literal or numeric property only object literal
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::NewScObjectSimple, objectLocation);
    }
    else
    {
        uint32 allocSize = UInt32Math::Mul(argCount, sizeof(Js::PropertyId));
        Js::PropertyIdArray *propIds = AnewPlus(byteCodeGenerator->GetAllocator(), allocSize, Js::PropertyIdArray, argCount, 0);

        if (propertyIds->ContainsKey(Js::PropertyIds::__proto__))
        {
            // Always record whether the initializer contains __proto__ no matter if current environment has it enabled
            // or not, in case the bytecode is later run with __proto__ enabled.
            propIds->has__proto__ = true;
        }

        unsigned int argIndex = 0;
        while (memberList->nop == knopList)
        {
            if (memberList->AsParseNodeBin()->pnode1->AsParseNodeBin()->pnode1->nop == knopComputedName)
            {
                break;
            }
            propertyId = memberList->AsParseNodeBin()->pnode1->AsParseNodeBin()->pnode1->AsParseNodeStr()->pid->GetPropertyId();
            if (!byteCodeGenerator->GetScriptContext()->IsNumericPropertyId(propertyId, &value) && propertyIds->Remove(propertyId))
            {
                propIds->elements[argIndex] = propertyId;
                argIndex++;
            }
            memberList = memberList->AsParseNodeBin()->pnode2;
        }

        if (memberList->AsParseNodeBin()->pnode1->nop != knopComputedName && !hasComputedName)
        {
            propertyId = memberList->AsParseNodeBin()->pnode1->AsParseNodeStr()->pid->GetPropertyId();
            if (!byteCodeGenerator->GetScriptContext()->IsNumericPropertyId(propertyId, &value) && propertyIds->Remove(propertyId))
            {
                propIds->elements[argIndex] = propertyId;
                argIndex++;
            }
        }

        uint32 literalObjectId = funcInfo->GetParsedFunctionBody()->NewObjectLiteral();

        // Generate the opcode with propIds and cacheId
        byteCodeGenerator->Writer()->Auxiliary(Js::OpCode::NewScObjectLiteral, objectLocation, propIds, UInt32Math::Add(sizeof(Js::PropertyIdArray), allocSize), literalObjectId);

        Adelete(byteCodeGenerator->GetAllocator(), propertyIds);

        AdeletePlus(byteCodeGenerator->GetAllocator(), allocSize, propIds);
    }

    memberList = pmemberList;

    bool useStore = false;
    // Generate the actual assignment to those properties
    if (memberList != nullptr)
    {
        while (memberList->nop == knopList)
        {
            ParseNode *memberNode = memberList->AsParseNodeBin()->pnode1;

            if (memberNode->AsParseNodeBin()->pnode1->nop == knopComputedName)
            {
                useStore = true;
            }

            byteCodeGenerator->StartSubexpression(memberNode);
            EmitMemberNode(memberNode, objectLocation, byteCodeGenerator, funcInfo, nullptr, useStore);
            byteCodeGenerator->EndSubexpression(memberNode);
            memberList = memberList->AsParseNodeBin()->pnode2;
        }

        byteCodeGenerator->StartSubexpression(memberList);
        EmitMemberNode(memberList, objectLocation, byteCodeGenerator, funcInfo, nullptr, useStore);
        byteCodeGenerator->EndSubexpression(memberList);
    }
}

void EmitStringTemplate(ParseNodeStrTemplate *pnodeStrTemplate, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    Assert(pnodeStrTemplate->pnodeStringLiterals);

    // For a tagged string template, we will create the callsite constant object as part of the FunctionBody constants table.
    // We only need to emit code for non-tagged string templates here.
    if (!pnodeStrTemplate->isTaggedTemplate)
    {
        // If we have no substitutions and this is not a tagged template, we can emit just the single cooked string.
        if (pnodeStrTemplate->pnodeSubstitutionExpressions == nullptr)
        {
            Assert(pnodeStrTemplate->pnodeStringLiterals->nop != knopList);

            funcInfo->AcquireLoc(pnodeStrTemplate);
            Emit(pnodeStrTemplate->pnodeStringLiterals, byteCodeGenerator, funcInfo, false);

            Assert(pnodeStrTemplate->location != pnodeStrTemplate->pnodeStringLiterals->location);

            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnodeStrTemplate->location, pnodeStrTemplate->pnodeStringLiterals->location);
            funcInfo->ReleaseLoc(pnodeStrTemplate->pnodeStringLiterals);
        }
        else
        {
            // If we have substitutions but no tag function, we can skip the callSite object construction (and also ignore raw strings).
            funcInfo->AcquireLoc(pnodeStrTemplate);

            // First string must be a list node since we have substitutions.
            AssertMsg(pnodeStrTemplate->pnodeStringLiterals->nop == knopList, "First string in the list must be a knopList node.");

            ParseNode* stringNodeList = pnodeStrTemplate->pnodeStringLiterals;

            // Emit the first string and load that into the pnode location.
            Emit(stringNodeList->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);

            Assert(pnodeStrTemplate->location != stringNodeList->AsParseNodeBin()->pnode1->location);

            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnodeStrTemplate->location, stringNodeList->AsParseNodeBin()->pnode1->location);
            funcInfo->ReleaseLoc(stringNodeList->AsParseNodeBin()->pnode1);

            ParseNode* expressionNodeList = pnodeStrTemplate->pnodeSubstitutionExpressions;
            ParseNode* stringNode;
            ParseNode* expressionNode;

            // Now append the substitution expressions and remaining string constants via normal add operator
            // We will always have one more string constant than substitution expression
            // `strcon1 ${expr1} strcon2 ${expr2} strcon3` = strcon1 + expr1 + strcon2 + expr2 + strcon3
            //
            // strcon1 --- step 1 (above)
            // expr1   \__ step 2
            // strcon2 /
            // expr2   \__ step 3
            // strcon3 /
            while (stringNodeList->nop == knopList)
            {
                // If the current head of the expression list is a list, fetch the node and walk the list.
                if (expressionNodeList->nop == knopList)
                {
                    expressionNode = expressionNodeList->AsParseNodeBin()->pnode1;
                    expressionNodeList = expressionNodeList->AsParseNodeBin()->pnode2;
                }
                else
                {
                    // This is the last element of the expression list.
                    expressionNode = expressionNodeList;
                }

                // Emit the expression and append it to the string we're building.
                Emit(expressionNode, byteCodeGenerator, funcInfo, false);

                Js::RegSlot toStringLocation = funcInfo->AcquireTmpRegister();
                byteCodeGenerator->Writer()->Reg2(Js::OpCode::Conv_Str, toStringLocation, expressionNode->location);
                byteCodeGenerator->Writer()->Reg3(Js::OpCode::Add_A, pnodeStrTemplate->location, pnodeStrTemplate->location, toStringLocation);
                funcInfo->ReleaseTmpRegister(toStringLocation);
                funcInfo->ReleaseLoc(expressionNode);

                // Move to the next string in the list - we already got ahead of the expressions in the first string literal above.
                stringNodeList = stringNodeList->AsParseNodeBin()->pnode2;

                // If the current head of the string literal list is also a list node, need to fetch the actual string literal node.
                if (stringNodeList->nop == knopList)
                {
                    stringNode = stringNodeList->AsParseNodeBin()->pnode1;
                }
                else
                {
                    // This is the last element of the string literal list.
                    stringNode = stringNodeList;
                }

                // Emit the string node following the previous expression and append it to the string.
                // This is either just some string in the list or it is the last string.
                Emit(stringNode, byteCodeGenerator, funcInfo, false);
                byteCodeGenerator->Writer()->Reg3(Js::OpCode::Add_A, pnodeStrTemplate->location, pnodeStrTemplate->location, stringNode->location);
                funcInfo->ReleaseLoc(stringNode);
            }
        }
    }
}

void SetNewArrayElements(ParseNode *pnode, Js::RegSlot arrayLocation, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    ParseNode *args = pnode->AsParseNodeUni()->pnode1;
    uint argCount = pnode->AsParseNodeArrLit()->count;
    uint spreadCount = pnode->AsParseNodeArrLit()->spreadCount;
    bool nativeArrays = CreateNativeArrays(byteCodeGenerator, funcInfo);

    bool arrayIntOpt = nativeArrays && pnode->AsParseNodeArrLit()->arrayOfInts;
    if (arrayIntOpt)
    {
        int extraAlloc = 0, auxSize = 0;
        if (Int32Math::Mul(argCount, sizeof(int32), &extraAlloc)
            || Int32Math::Add(sizeof(Js::AuxArray<int>), extraAlloc, &auxSize))
        {
            ::Math::DefaultOverflowPolicy();
        }
        Js::AuxArray<int> *ints = AnewPlus(byteCodeGenerator->GetAllocator(), extraAlloc, Js::AuxArray<int32>, argCount);
        EmitConstantArgsToIntArray(byteCodeGenerator, ints->elements, args, argCount);
        Assert(!pnode->AsParseNodeArrLit()->hasMissingValues);
        byteCodeGenerator->Writer()->Auxiliary(
            Js::OpCode::NewScIntArray,
            pnode->location,
            ints,
            auxSize,
            argCount);
        AdeletePlus(byteCodeGenerator->GetAllocator(), extraAlloc, ints);
        return;
    }

    bool arrayNumOpt = nativeArrays && pnode->AsParseNodeArrLit()->arrayOfNumbers;
    if (arrayNumOpt)
    {
        int extraAlloc = 0, auxSize = 0;
        if (Int32Math::Mul(argCount, sizeof(double), &extraAlloc)
            || Int32Math::Add(sizeof(Js::AuxArray<double>), extraAlloc, &auxSize))
        {
            ::Math::DefaultOverflowPolicy();
        }
        Js::AuxArray<double> *doubles = AnewPlus(byteCodeGenerator->GetAllocator(), extraAlloc, Js::AuxArray<double>, argCount);
        EmitConstantArgsToFltArray(byteCodeGenerator, doubles->elements, args, argCount);
        Assert(!pnode->AsParseNodeArrLit()->hasMissingValues);
        byteCodeGenerator->Writer()->Auxiliary(
            Js::OpCode::NewScFltArray,
            pnode->location,
            doubles,
            auxSize,
            argCount);
        AdeletePlus(byteCodeGenerator->GetAllocator(), extraAlloc, doubles);
        return;
    }

    bool arrayLitOpt = pnode->AsParseNodeArrLit()->arrayOfTaggedInts && pnode->AsParseNodeArrLit()->count > 1;
    Assert(!arrayLitOpt || !nativeArrays);

    Js::RegSlot spreadArrLoc = arrayLocation;
    Js::AuxArray<uint32> *spreadIndices = nullptr;
    const uint extraAlloc = UInt32Math::Mul(spreadCount, sizeof(uint32));
    if (pnode->AsParseNodeArrLit()->spreadCount > 0)
    {
        arrayLocation = funcInfo->AcquireTmpRegister();
        spreadIndices = AnewPlus(byteCodeGenerator->GetAllocator(), extraAlloc, Js::AuxArray<uint32>, spreadCount);
    }

    byteCodeGenerator->Writer()->Reg1Unsigned1(
        pnode->AsParseNodeArrLit()->hasMissingValues ? Js::OpCode::NewScArrayWithMissingValues : Js::OpCode::NewScArray,
        arrayLocation,
        argCount);

    if (args != nullptr)
    {
        Js::OpCode opcode;
        Js::RegSlot arrLoc;
        if (argCount == 1 && !byteCodeGenerator->Writer()->DoProfileNewScArrayOp(Js::OpCode::NewScArray))
        {
            opcode = Js::OpCode::StArrItemC_CI4;
            arrLoc = arrayLocation;
        }
        else if (arrayLitOpt)
        {
            opcode = Js::OpCode::StArrSegItem_A;
            arrLoc = funcInfo->AcquireTmpRegister();
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::LdArrHead, arrLoc, arrayLocation);
        }
        else if (Js::JavascriptArray::HasInlineHeadSegment(argCount))
        {
            // The head segment will be allocated inline as an interior pointer. To keep the array alive, the set operation
            // should be done relative to the array header to keep it alive (instead of the array segment).
            opcode = Js::OpCode::StArrInlineItem_CI4;
            arrLoc = arrayLocation;
        }
        else if (argCount <= Js::JavascriptArray::MaxInitialDenseLength)
        {
            opcode = Js::OpCode::StArrSegItem_CI4;
            arrLoc = funcInfo->AcquireTmpRegister();
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::LdArrHead, arrLoc, arrayLocation);
        }
        else
        {
            opcode = Js::OpCode::StArrItemI_CI4;
            arrLoc = arrayLocation;
        }

        if (arrayLitOpt)
        {
            uint32 allocSize = UInt32Math::Mul(argCount, sizeof(Js::Var));
            Js::VarArray *vars = AnewPlus(byteCodeGenerator->GetAllocator(), allocSize, Js::VarArray, argCount);

            EmitConstantArgsToVarArray(byteCodeGenerator, vars->elements, args, argCount);

            // Generate the opcode with vars
            byteCodeGenerator->Writer()->Auxiliary(Js::OpCode::StArrSegItem_A, arrLoc, vars, UInt32Math::Add(sizeof(Js::VarArray), allocSize), argCount);

            AdeletePlus(byteCodeGenerator->GetAllocator(), allocSize, vars);
        }
        else
        {
            uint i = 0;
            unsigned spreadIndex = 0;
            Js::RegSlot rhsLocation;
            while (args->nop == knopList)
            {
                if (args->AsParseNodeBin()->pnode1->nop != knopEmpty)
                {
                    Emit(args->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
                    rhsLocation = args->AsParseNodeBin()->pnode1->location;
                    Js::RegSlot regVal = rhsLocation;
                    if (args->AsParseNodeBin()->pnode1->nop == knopEllipsis)
                    {
                        AnalysisAssert(spreadIndices);
                        regVal = funcInfo->AcquireTmpRegister();
                        byteCodeGenerator->Writer()->Reg2(Js::OpCode::LdCustomSpreadIteratorList, regVal, rhsLocation);
                        spreadIndices->elements[spreadIndex++] = i;
                    }

                    byteCodeGenerator->Writer()->ElementUnsigned1(opcode, regVal, arrLoc, i);

                    if (args->AsParseNodeBin()->pnode1->nop == knopEllipsis)
                    {
                        funcInfo->ReleaseTmpRegister(regVal);
                    }

                    funcInfo->ReleaseLoc(args->AsParseNodeBin()->pnode1);
                }

                args = args->AsParseNodeBin()->pnode2;
                i++;
            }

            if (args->nop != knopEmpty)
            {
                Emit(args, byteCodeGenerator, funcInfo, false);
                rhsLocation = args->location;
                Js::RegSlot regVal = rhsLocation;
                if (args->nop == knopEllipsis)
                {
                    regVal = funcInfo->AcquireTmpRegister();
                    byteCodeGenerator->Writer()->Reg2(Js::OpCode::LdCustomSpreadIteratorList, regVal, rhsLocation);
                    AnalysisAssert(spreadIndices);
                    spreadIndices->elements[spreadIndex] = i;
                }

                byteCodeGenerator->Writer()->ElementUnsigned1(opcode, regVal, arrLoc, i);

                if (args->nop == knopEllipsis)
                {
                    funcInfo->ReleaseTmpRegister(regVal);
                }

                funcInfo->ReleaseLoc(args);
                i++;
            }
            Assert(i <= argCount);
        }

        if (arrLoc != arrayLocation)
        {
            funcInfo->ReleaseTmpRegister(arrLoc);
        }
    }

    if (pnode->AsParseNodeArrLit()->spreadCount > 0)
    {
        byteCodeGenerator->Writer()->Reg2Aux(Js::OpCode::SpreadArrayLiteral, spreadArrLoc, arrayLocation, spreadIndices, UInt32Math::Add(sizeof(Js::AuxArray<uint32>), extraAlloc), extraAlloc);
        AdeletePlus(byteCodeGenerator->GetAllocator(), extraAlloc, spreadIndices);
        funcInfo->ReleaseTmpRegister(arrayLocation);
    }
}

// FIX: TODO: mixed-mode expressions (arithmetic expressions mixed with boolean expressions); current solution
// will not short-circuit in some cases and is not complete (for example: var i=(x==y))
// This uses Aho and Ullman style double-branch generation (p. 494 ASU); we will need to peephole optimize or replace
// with special case for single-branch style.
void EmitBooleanExpression(
    _In_ ParseNode* expr,
    Js::ByteCodeLabel trueLabel,
    Js::ByteCodeLabel falseLabel,
    _In_ ByteCodeGenerator* byteCodeGenerator,
    _In_ FuncInfo* funcInfo,
    bool trueFallthrough,
    bool falseFallthrough)
{
    Assert(!trueFallthrough || !falseFallthrough);

    byteCodeGenerator->StartStatement(expr);
    switch (expr->nop)
    {

    case knopLogOr:
    {
        Js::ByteCodeLabel leftFalse = byteCodeGenerator->Writer()->DefineLabel();
        EmitBooleanExpression(expr->AsParseNodeBin()->pnode1, trueLabel, leftFalse, byteCodeGenerator, funcInfo, false, true);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        byteCodeGenerator->Writer()->MarkLabel(leftFalse);
        EmitBooleanExpression(expr->AsParseNodeBin()->pnode2, trueLabel, falseLabel, byteCodeGenerator, funcInfo, trueFallthrough, falseFallthrough);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        break;
    }

    case knopLogAnd:
    {
        Js::ByteCodeLabel leftTrue = byteCodeGenerator->Writer()->DefineLabel();
        EmitBooleanExpression(expr->AsParseNodeBin()->pnode1, leftTrue, falseLabel, byteCodeGenerator, funcInfo, true, false);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        byteCodeGenerator->Writer()->MarkLabel(leftTrue);
        EmitBooleanExpression(expr->AsParseNodeBin()->pnode2, trueLabel, falseLabel, byteCodeGenerator, funcInfo, trueFallthrough, falseFallthrough);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        break;
    }

    case knopLogNot:
        EmitBooleanExpression(expr->AsParseNodeUni()->pnode1, falseLabel, trueLabel, byteCodeGenerator, funcInfo, falseFallthrough, trueFallthrough);
        funcInfo->ReleaseLoc(expr->AsParseNodeUni()->pnode1);
        break;

    case knopEq:
    case knopEqv:
    case knopNEqv:
    case knopNe:
    case knopLt:
    case knopLe:
    case knopGe:
    case knopGt:
        EmitBinaryOpnds(expr->AsParseNodeBin()->pnode1, expr->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        byteCodeGenerator->Writer()->BrReg2(nopToOp[expr->nop], trueLabel, expr->AsParseNodeBin()->pnode1->location,
            expr->AsParseNodeBin()->pnode2->location);
        if (!falseFallthrough)
        {
            byteCodeGenerator->Writer()->Br(falseLabel);
        }
        break;
    case knopTrue:
        if (!trueFallthrough)
        {
            byteCodeGenerator->Writer()->Br(trueLabel);
        }
        break;
    case knopFalse:
        if (!falseFallthrough)
        {
            byteCodeGenerator->Writer()->Br(falseLabel);
        }
        break;
    default:
        // Note: we usually release the temp assigned to a node after we Emit it.
        // But in this case, EmitBooleanExpression is just a wrapper around a normal Emit call,
        // and the caller of EmitBooleanExpression expects to be able to release this register.

        Emit(expr, byteCodeGenerator, funcInfo, false);
        if (trueFallthrough)
        {
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, falseLabel, expr->location);
        }
        else
        {
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
            if (!falseFallthrough)
            {
                byteCodeGenerator->Writer()->Br(falseLabel);
            }
        }
        break;
    }

    byteCodeGenerator->EndStatement(expr);
}

void EmitGeneratingBooleanExpression(ParseNode *expr, Js::ByteCodeLabel trueLabel, bool truefallthrough, Js::ByteCodeLabel falseLabel, bool falsefallthrough, Js::RegSlot writeto,
    ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    switch (expr->nop)
    {

    case knopLogOr:
    {
        byteCodeGenerator->StartStatement(expr);
        Js::ByteCodeLabel leftFalse = byteCodeGenerator->Writer()->DefineLabel();
        EmitGeneratingBooleanExpression(expr->AsParseNodeBin()->pnode1, trueLabel, false, leftFalse, true, writeto, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        byteCodeGenerator->Writer()->MarkLabel(leftFalse);
        EmitGeneratingBooleanExpression(expr->AsParseNodeBin()->pnode2, trueLabel, truefallthrough, falseLabel, falsefallthrough, writeto, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        byteCodeGenerator->EndStatement(expr);
        break;
    }

    case knopLogAnd:
    {
        byteCodeGenerator->StartStatement(expr);
        Js::ByteCodeLabel leftTrue = byteCodeGenerator->Writer()->DefineLabel();
        EmitGeneratingBooleanExpression(expr->AsParseNodeBin()->pnode1, leftTrue, true, falseLabel, false, writeto, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        byteCodeGenerator->Writer()->MarkLabel(leftTrue);
        EmitGeneratingBooleanExpression(expr->AsParseNodeBin()->pnode2, trueLabel, truefallthrough, falseLabel, falsefallthrough, writeto, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        byteCodeGenerator->EndStatement(expr);
        break;
    }

    case knopLogNot:
    {
        byteCodeGenerator->StartStatement(expr);
        // this time we want a boolean expression, since Logical Not is nice and only returns true or false
        Js::ByteCodeLabel emitTrue = byteCodeGenerator->Writer()->DefineLabel();
        Js::ByteCodeLabel emitFalse = byteCodeGenerator->Writer()->DefineLabel();
        EmitBooleanExpression(expr->AsParseNodeUni()->pnode1, emitFalse, emitTrue, byteCodeGenerator, funcInfo, false, true);
        byteCodeGenerator->Writer()->MarkLabel(emitTrue);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, writeto);
        byteCodeGenerator->Writer()->Br(trueLabel);
        byteCodeGenerator->Writer()->MarkLabel(emitFalse);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, writeto);
        if (!falsefallthrough)
        {
            byteCodeGenerator->Writer()->Br(falseLabel);
        }
        funcInfo->ReleaseLoc(expr->AsParseNodeUni()->pnode1);
        byteCodeGenerator->EndStatement(expr);
        break;
    }
    case knopEq:
    case knopEqv:
    case knopNEqv:
    case knopNe:
    case knopLt:
    case knopLe:
    case knopGe:
    case knopGt:
        byteCodeGenerator->StartStatement(expr);
        EmitBinaryOpnds(expr->AsParseNodeBin()->pnode1, expr->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode2);
        funcInfo->ReleaseLoc(expr->AsParseNodeBin()->pnode1);
        funcInfo->AcquireLoc(expr);
        byteCodeGenerator->Writer()->Reg3(nopToCMOp[expr->nop], expr->location, expr->AsParseNodeBin()->pnode1->location,
            expr->AsParseNodeBin()->pnode2->location);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, writeto, expr->location);
        // The inliner likes small bytecode
        if (!(truefallthrough || falsefallthrough))
        {
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
            byteCodeGenerator->Writer()->Br(falseLabel);
        }
        else if (truefallthrough && !falsefallthrough) {
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, falseLabel, expr->location);
        }
        else if (falsefallthrough && !truefallthrough) {
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
        }
        byteCodeGenerator->EndStatement(expr);
        break;
    case knopTrue:
        byteCodeGenerator->StartStatement(expr);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, writeto);
        if (!truefallthrough)
        {
            byteCodeGenerator->Writer()->Br(trueLabel);
        }
        byteCodeGenerator->EndStatement(expr);
        break;
    case knopFalse:
        byteCodeGenerator->StartStatement(expr);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, writeto);
        if (!falsefallthrough)
        {
            byteCodeGenerator->Writer()->Br(falseLabel);
        }
        byteCodeGenerator->EndStatement(expr);
        break;
    default:
        // Note: we usually release the temp assigned to a node after we Emit it.
        // But in this case, EmitBooleanExpression is just a wrapper around a normal Emit call,
        // and the caller of EmitBooleanExpression expects to be able to release this register.

        // For diagnostics purposes, register the name and dot to the statement list.
        if (expr->nop == knopName || expr->nop == knopDot)
        {
            byteCodeGenerator->StartStatement(expr);
            Emit(expr, byteCodeGenerator, funcInfo, false);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, writeto, expr->location);
            // The inliner likes small bytecode
            if (!(truefallthrough || falsefallthrough))
            {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
                byteCodeGenerator->Writer()->Br(falseLabel);
            }
            else if (truefallthrough && !falsefallthrough) {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, falseLabel, expr->location);
            }
            else if (falsefallthrough && !truefallthrough) {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
            }
            byteCodeGenerator->EndStatement(expr);
        }
        else
        {
            Emit(expr, byteCodeGenerator, funcInfo, false);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, writeto, expr->location);
            // The inliner likes small bytecode
            if (!(truefallthrough || falsefallthrough))
            {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
                byteCodeGenerator->Writer()->Br(falseLabel);
            }
            else if (truefallthrough && !falsefallthrough) {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrFalse_A, falseLabel, expr->location);
            }
            else if (falsefallthrough && !truefallthrough) {
                byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, trueLabel, expr->location);
            }
        }
        break;
    }
}

// used by while and for loops
void EmitLoop(
    ParseNodeLoop *loopNode,
    ParseNode *cond,
    ParseNode *body,
    ParseNode *incr,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    BOOL fReturnValue,
    BOOL doWhile = FALSE,
    ParseNodeBlock *forLoopBlock = nullptr)
{
    // Need to increment loop count whether we are going to profile or not for HasLoop()

    Js::ByteCodeLabel loopEntrance = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel continuePastLoop = byteCodeGenerator->Writer()->DefineLabel();

    uint loopId = byteCodeGenerator->Writer()->EnterLoop(loopEntrance);
    loopNode->loopId = loopId;

    if (doWhile)
    {
        Emit(body, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(body);
        if (loopNode->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(loopNode->continueLabel);
        }
        if (!ByteCodeGenerator::IsFalse(cond) ||
            byteCodeGenerator->IsInDebugMode())
        {
            EmitBooleanExpression(cond, loopEntrance, continuePastLoop, byteCodeGenerator, funcInfo, false, false);
        }
        funcInfo->ReleaseLoc(cond);
    }
    else
    {
        if (cond)
        {
            if (!(cond->nop == knopInt &&
                cond->AsParseNodeInt()->lw != 0))
            {
                Js::ByteCodeLabel trueLabel = byteCodeGenerator->Writer()->DefineLabel();
                EmitBooleanExpression(cond, trueLabel, continuePastLoop, byteCodeGenerator, funcInfo, true, false);
                byteCodeGenerator->Writer()->MarkLabel(trueLabel);
            }
            funcInfo->ReleaseLoc(cond);
        }
        Emit(body, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(body);

        if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled() &&
            forLoopBlock != nullptr)
        {
            CloneEmitBlock(forLoopBlock, byteCodeGenerator, funcInfo);
        }

        if (loopNode->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(loopNode->continueLabel);
        }

        if (incr != nullptr)
        {
            Emit(incr, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(incr);
        }

        byteCodeGenerator->Writer()->Br(loopEntrance);
    }

    byteCodeGenerator->Writer()->MarkLabel(continuePastLoop);
    if (loopNode->emitLabels)
    {
        byteCodeGenerator->Writer()->MarkLabel(loopNode->breakLabel);
    }

    byteCodeGenerator->Writer()->ExitLoop(loopId);
}

void ByteCodeGenerator::EmitInvertedLoop(ParseNodeLoop* outerLoop, ParseNodeFor* invertedLoop, FuncInfo* funcInfo)
{
    Js::ByteCodeLabel invertedLoopLabel = this->m_writer.DefineLabel();
    Js::ByteCodeLabel afterInvertedLoop = this->m_writer.DefineLabel();

    // emit branch around original
    Emit(outerLoop->AsParseNodeFor()->pnodeInit, this, funcInfo, false);
    funcInfo->ReleaseLoc(outerLoop->AsParseNodeFor()->pnodeInit);
    this->m_writer.BrS(Js::OpCode::BrNotHasSideEffects, invertedLoopLabel, Js::SideEffects_Any);

    // emit original
    EmitLoop(outerLoop, outerLoop->AsParseNodeFor()->pnodeCond, outerLoop->AsParseNodeFor()->pnodeBody,
        outerLoop->AsParseNodeFor()->pnodeIncr, this, funcInfo, false);

    // clear temporary registers since inverted loop may share nodes with
    // emitted original loop
    VisitClearTmpRegs(outerLoop, this, funcInfo);

    // emit branch around inverted
    this->m_writer.Br(afterInvertedLoop);
    this->m_writer.MarkLabel(invertedLoopLabel);

    // Emit a zero trip test for the original outer-loop
    Js::ByteCodeLabel zeroTrip = this->m_writer.DefineLabel();
    ParseNode* testNode = this->GetParser()->CopyPnode(outerLoop->AsParseNodeFor()->pnodeCond);
    EmitBooleanExpression(testNode, zeroTrip, afterInvertedLoop, this, funcInfo, true, false);
    this->m_writer.MarkLabel(zeroTrip);
    funcInfo->ReleaseLoc(testNode);

    // emit inverted
    Emit(invertedLoop->pnodeInit, this, funcInfo, false);
    funcInfo->ReleaseLoc(invertedLoop->pnodeInit);
    EmitLoop(invertedLoop, invertedLoop->pnodeCond, invertedLoop->pnodeBody,
        invertedLoop->pnodeIncr, this, funcInfo, false);
    this->m_writer.MarkLabel(afterInvertedLoop);
}

void EmitGetIterator(Js::RegSlot iteratorLocation, Js::RegSlot iterableLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    // get iterator object from the iterable
    EmitInvoke(iteratorLocation, iterableLocation, Js::PropertyIds::_symbolIterator, byteCodeGenerator, funcInfo);

    // throw TypeError if the result is not an object
    Js::ByteCodeLabel skipThrow = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrOnObject_A, skipThrow, iteratorLocation);
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_NeedObject));
    byteCodeGenerator->Writer()->MarkLabel(skipThrow);
}

void EmitIteratorNext(Js::RegSlot itemLocation, Js::RegSlot iteratorLocation, Js::RegSlot nextInputLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    // invoke next() on the iterator
    if (nextInputLocation == Js::Constants::NoRegister)
    {
        EmitInvoke(itemLocation, iteratorLocation, Js::PropertyIds::next, byteCodeGenerator, funcInfo);
    }
    else
    {
        EmitInvoke(itemLocation, iteratorLocation, Js::PropertyIds::next, byteCodeGenerator, funcInfo, nextInputLocation);
    }

    // throw TypeError if the result is not an object
    Js::ByteCodeLabel skipThrow = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrOnObject_A, skipThrow, itemLocation);
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_NeedObject));
    byteCodeGenerator->Writer()->MarkLabel(skipThrow);
}

// Generating
// if (hasReturnFunction) {
//     value = Call Retrun;
//     if (value != Object)
//        throw TypeError;
// }

void EmitIteratorClose(Js::RegSlot iteratorLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    Js::RegSlot returnLocation = funcInfo->AcquireTmpRegister();

    Js::ByteCodeLabel skipThrow = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel noReturn = byteCodeGenerator->Writer()->DefineLabel();

    uint cacheId = funcInfo->FindOrAddInlineCacheId(iteratorLocation, Js::PropertyIds::return_, false, false);
    byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, returnLocation, iteratorLocation, cacheId);

    byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrEq_A, noReturn, returnLocation, funcInfo->undefinedConstantRegister);

    EmitInvoke(returnLocation, iteratorLocation, Js::PropertyIds::return_, byteCodeGenerator, funcInfo);

    // throw TypeError if the result is not an Object
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrOnObject_A, skipThrow, returnLocation);
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_NeedObject));
    byteCodeGenerator->Writer()->MarkLabel(skipThrow);
    byteCodeGenerator->Writer()->MarkLabel(noReturn);

    funcInfo->ReleaseTmpRegister(returnLocation);
}

void EmitIteratorComplete(Js::RegSlot doneLocation, Js::RegSlot iteratorResultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    // get the iterator result's "done" property
    uint cacheId = funcInfo->FindOrAddInlineCacheId(iteratorResultLocation, Js::PropertyIds::done, false, false);
    byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, doneLocation, iteratorResultLocation, cacheId);

    // Do not need to do ToBoolean explicitly with current uses of EmitIteratorComplete since BrTrue_A does this.
    // Add a ToBoolean controlled by template flag if needed for new uses later on.
}

void EmitIteratorValue(Js::RegSlot valueLocation, Js::RegSlot iteratorResultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    // get the iterator result's "value" property
    uint cacheId = funcInfo->FindOrAddInlineCacheId(iteratorResultLocation, Js::PropertyIds::value, false, false);
    byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, valueLocation, iteratorResultLocation, cacheId);
}

void EmitForInOfLoopBody(ParseNodeForInOrForOf *loopNode,
    Js::ByteCodeLabel loopEntrance,
    Js::ByteCodeLabel continuePastLoop,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    BOOL fReturnValue)
{
    if (loopNode->pnodeLval->nop != knopVarDecl &&
        loopNode->pnodeLval->nop != knopLetDecl &&
        loopNode->pnodeLval->nop != knopConstDecl)
    {
        EmitReference(loopNode->pnodeLval, byteCodeGenerator, funcInfo);
    }
    else
    {
        Symbol * sym = loopNode->pnodeLval->AsParseNodeVar()->sym;
        sym->SetNeedDeclaration(false);
    }

    if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
    {
        BeginEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
    }

    EmitAssignment(nullptr, loopNode->pnodeLval, loopNode->itemLocation, byteCodeGenerator, funcInfo);

    // The StartStatement is already done in the caller of this function.
    byteCodeGenerator->EndStatement(loopNode->pnodeLval);

    funcInfo->ReleaseReference(loopNode->pnodeLval);

    Emit(loopNode->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
    funcInfo->ReleaseLoc(loopNode->pnodeBody);

    if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
    {
        EndEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
    }

    funcInfo->ReleaseTmpRegister(loopNode->itemLocation);
    if (loopNode->emitLabels)
    {
        byteCodeGenerator->Writer()->MarkLabel(loopNode->continueLabel);
    }
    byteCodeGenerator->Writer()->Br(loopEntrance);
    byteCodeGenerator->Writer()->MarkLabel(continuePastLoop);
    if (loopNode->emitLabels)
    {
        byteCodeGenerator->Writer()->MarkLabel(loopNode->breakLabel);
    }
}

void EmitForIn(ParseNodeForInOrForOf *loopNode,
    Js::ByteCodeLabel loopEntrance,
    Js::ByteCodeLabel continuePastLoop,
    ByteCodeGenerator *byteCodeGenerator,
    FuncInfo *funcInfo,
    BOOL fReturnValue)
{
    Assert(loopNode->nop == knopForIn);
    Assert(loopNode->location == Js::Constants::NoRegister);

    // Grab registers for the enumerator and for the current enumerated item.
    // The enumerator register will be released after this call returns.
    loopNode->itemLocation = funcInfo->AcquireTmpRegister();

    uint forInLoopLevel = funcInfo->AcquireForInLoopLevel();

    // get enumerator from the collection
    byteCodeGenerator->Writer()->Reg1Unsigned1(Js::OpCode::InitForInEnumerator, loopNode->pnodeObj->location, forInLoopLevel);

    // The StartStatement is already done in the caller of the current function, which is EmitForInOrForOf
    byteCodeGenerator->EndStatement(loopNode);

    // Need to increment loop count whether we are going into profile or not for HasLoop()
    uint loopId = byteCodeGenerator->Writer()->EnterLoop(loopEntrance);
    loopNode->loopId = loopId;

    // The EndStatement will happen in the EmitForInOfLoopBody function
    byteCodeGenerator->StartStatement(loopNode->pnodeLval);

    // branch past loop when MoveAndGetNext returns nullptr
    byteCodeGenerator->Writer()->BrReg1Unsigned1(Js::OpCode::BrOnEmpty, continuePastLoop, loopNode->itemLocation, forInLoopLevel);

    EmitForInOfLoopBody(loopNode, loopEntrance, continuePastLoop, byteCodeGenerator, funcInfo, fReturnValue);

    byteCodeGenerator->Writer()->ExitLoop(loopId);

    funcInfo->ReleaseForInLoopLevel(forInLoopLevel);

    if (!byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
    {
        EndEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
    }
}

void EmitForInOrForOf(ParseNodeForInOrForOf *loopNode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, BOOL fReturnValue)
{
    bool isForIn = (loopNode->nop == knopForIn);
    Assert(isForIn || loopNode->nop == knopForOf);

    BeginEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);

    byteCodeGenerator->StartStatement(loopNode);
    if (!isForIn)
    {
        funcInfo->AcquireLoc(loopNode);
    }

    // Record the branch bytecode offset.
    // This is used for "ignore exception" and "set next stmt" scenarios. See ProbeContainer::GetNextUserStatementOffsetForAdvance:
    // If there is a branch recorded between current offset and next stmt offset, we'll use offset of the branch recorded,
    // otherwise use offset of next stmt.
    // The idea here is that when we bail out after ignore exception, we need to bail out to the beginning of the ForIn,
    // but currently ForIn stmt starts at the condition part, which is needed for correct handling of break point on ForIn
    // (break every time on the loop back edge) and correct display of current statement under debugger.
    // See WinBlue 231880 for details.
    byteCodeGenerator->Writer()->RecordStatementAdjustment(Js::FunctionBody::SAT_All);
    if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled() &&
        loopNode->pnodeBlock->HasBlockScopedContent())
    {
        byteCodeGenerator->Writer()->RecordForInOrOfCollectionScope();
    }
    Js::ByteCodeLabel loopEntrance = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel continuePastLoop = byteCodeGenerator->Writer()->DefineLabel();

    if (loopNode->pnodeLval->nop == knopVarDecl)
    {
        EmitReference(loopNode->pnodeLval, byteCodeGenerator, funcInfo);
    }

    Emit(loopNode->pnodeObj, byteCodeGenerator, funcInfo, false); // evaluate collection expression
    funcInfo->ReleaseLoc(loopNode->pnodeObj);

    if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
    {
        EndEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
        if (loopNode->pnodeBlock->scope != nullptr)
        {
            loopNode->pnodeBlock->scope->ForEachSymbol([](Symbol *sym) {
                sym->SetIsTrackedForDebugger(false);
            });
        }
    }

    if (isForIn)
    {
        EmitForIn(loopNode, loopEntrance, continuePastLoop, byteCodeGenerator, funcInfo, fReturnValue);

        if (!byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
        {
            EndEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
        }

        return;
    }

    Js::ByteCodeLabel skipThrow = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrNeq_A, skipThrow, loopNode->pnodeObj->location, funcInfo->undefinedConstantRegister);
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_ObjectCoercible));
    byteCodeGenerator->Writer()->MarkLabel(skipThrow);

    Js::RegSlot regException = Js::Constants::NoRegister;
    Js::RegSlot regOffset = Js::Constants::NoRegister;

    // These two temp variables store the information of return function to be called or not.
    // one variable is used for catch block and one is used for finally block. These variable will be set to true when we think that return function
    // to be called on abrupt loop break.
    // Why two variables? since these are temps and JIT does like not flow if single variable is used in multiple blocks.
    Js::RegSlot shouldCallReturnFunctionLocation = funcInfo->AcquireTmpRegister();
    Js::RegSlot shouldCallReturnFunctionLocationFinally = funcInfo->AcquireTmpRegister();

    bool isCoroutine = funcInfo->byteCodeFunction->IsCoroutine();

    if (isCoroutine)
    {
        regException = funcInfo->AcquireTmpRegister();
        regOffset = funcInfo->AcquireTmpRegister();
    }

    // Grab registers for the enumerator and for the current enumerated item.
    // The enumerator register will be released after this call returns.
    loopNode->itemLocation = funcInfo->AcquireTmpRegister();

    // We want call profile information on the @@iterator call, so instead of adding a GetForOfIterator bytecode op
    // to do all the following work in a helper do it explicitly in bytecode so that the @@iterator call is exposed
    // to the profiler and JIT.

    byteCodeGenerator->SetHasFinally(true);
    byteCodeGenerator->SetHasTry(true);
    byteCodeGenerator->TopFuncInfo()->byteCodeFunction->SetDontInline(true);

    // do a ToObject on the collection
    Js::RegSlot tmpObj = funcInfo->AcquireTmpRegister();
    byteCodeGenerator->Writer()->Reg2(Js::OpCode::Conv_Obj, tmpObj, loopNode->pnodeObj->location);

    EmitGetIterator(loopNode->location, tmpObj, byteCodeGenerator, funcInfo);
    funcInfo->ReleaseTmpRegister(tmpObj);

    // The whole loop is surrounded with try..catch..finally - in order to capture the abrupt completion.
    Js::ByteCodeLabel finallyLabel = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel catchLabel = byteCodeGenerator->Writer()->DefineLabel();
    byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

    ByteCodeGenerator::TryScopeRecord tryRecForTryFinally(Js::OpCode::TryFinallyWithYield, finallyLabel);

    if (isCoroutine)
    {
        byteCodeGenerator->Writer()->BrReg2(Js::OpCode::TryFinallyWithYield, finallyLabel, regException, regOffset);
        tryRecForTryFinally.reg1 = regException;
        tryRecForTryFinally.reg2 = regOffset;
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTryFinally);
    }
    else
    {
        byteCodeGenerator->Writer()->Br(Js::OpCode::TryFinally, finallyLabel);
    }

    byteCodeGenerator->Writer()->Br(Js::OpCode::TryCatch, catchLabel);

    ByteCodeGenerator::TryScopeRecord tryRecForTry(Js::OpCode::TryCatch, catchLabel);
    if (isCoroutine)
    {
        byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTry);
    }

    byteCodeGenerator->EndStatement(loopNode);

    // Need to increment loop count whether we are going into profile or not for HasLoop()
    uint loopId = byteCodeGenerator->Writer()->EnterLoop(loopEntrance);
    loopNode->loopId = loopId;

    byteCodeGenerator->StartStatement(loopNode->pnodeLval);

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, shouldCallReturnFunctionLocationFinally);

    EmitIteratorNext(loopNode->itemLocation, loopNode->location, Js::Constants::NoRegister, byteCodeGenerator, funcInfo);

    Js::RegSlot doneLocation = funcInfo->AcquireTmpRegister();
    EmitIteratorComplete(doneLocation, loopNode->itemLocation, byteCodeGenerator, funcInfo);

    // branch past loop if the result's done property is truthy
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, continuePastLoop, doneLocation);
    funcInfo->ReleaseTmpRegister(doneLocation);

    // otherwise put result's value property in itemLocation
    EmitIteratorValue(loopNode->itemLocation, loopNode->itemLocation, byteCodeGenerator, funcInfo);

    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocation);
    byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, shouldCallReturnFunctionLocationFinally);

    EmitForInOfLoopBody(loopNode, loopEntrance, continuePastLoop, byteCodeGenerator, funcInfo, fReturnValue);

    byteCodeGenerator->Writer()->ExitLoop(loopId);

    EmitCatchAndFinallyBlocks(catchLabel,
        finallyLabel,
        loopNode->location,
        shouldCallReturnFunctionLocation,
        shouldCallReturnFunctionLocationFinally,
        regException,
        regOffset,
        byteCodeGenerator,
        funcInfo);

    if (!byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
    {
        EndEmitBlock(loopNode->pnodeBlock, byteCodeGenerator, funcInfo);
    }
}

void EmitArrayLiteral(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    funcInfo->AcquireLoc(pnode);
    ParseNode *args = pnode->AsParseNodeUni()->pnode1;
    if (args == nullptr)
    {
        byteCodeGenerator->Writer()->Reg1Unsigned1(
            pnode->AsParseNodeArrLit()->hasMissingValues ? Js::OpCode::NewScArrayWithMissingValues : Js::OpCode::NewScArray,
            pnode->location,
            ByteCodeGenerator::DefaultArraySize);
    }
    else
    {
        SetNewArrayElements(pnode, pnode->location, byteCodeGenerator, funcInfo);
    }
}

void EmitJumpCleanup(ParseNodeStmt *pnode, ParseNode *pnodeTarget, ByteCodeGenerator *byteCodeGenerator, FuncInfo * funcInfo)
{
    for (; pnode != pnodeTarget; pnode = pnode->pnodeOuter)
    {
        switch (pnode->nop)
        {
        case knopTry:
        case knopCatch:
        case knopFinally:
            // We insert OpCode::Leave when there is a 'return' inside try/catch/finally.
            // This is for flow control and does not participate in identifying boundaries of try/catch blocks,
            // thus we shouldn't call RecordCrossFrameEntryExitRecord() here.
            byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
            break;

        case knopForOf:
#if ENABLE_PROFILE_INFO
            if (Js::DynamicProfileInfo::EnableImplicitCallFlags(funcInfo->GetParsedFunctionBody()))
            {
                byteCodeGenerator->Writer()->Unsigned1(Js::OpCode::ProfiledLoopEnd, pnode->AsParseNodeLoop()->loopId);
            }
#endif
            // The ForOf loop code is wrapped around try..catch..finally - Forcing couple Leave bytecode over here
            byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
            byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
            break;

#if ENABLE_PROFILE_INFO
        case knopWhile:
        case knopDoWhile:
        case knopFor:
        case knopForIn:
            if (Js::DynamicProfileInfo::EnableImplicitCallFlags(funcInfo->GetParsedFunctionBody()))
            {
                byteCodeGenerator->Writer()->Unsigned1(Js::OpCode::ProfiledLoopEnd, pnode->AsParseNodeLoop()->loopId);
            }
            break;
#endif

        }
    }
}

void EmitBinaryOpnds(ParseNode *pnode1, ParseNode *pnode2, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, Js::RegSlot computedPropertyLocation)
{
    // If opnd2 can overwrite opnd1, make sure the value of opnd1 is stashed away.
    if (MayHaveSideEffectOnNode(pnode1, pnode2))
    {
        SaveOpndValue(pnode1, funcInfo);
    }

    Emit(pnode1, byteCodeGenerator, funcInfo, false);

    if (pnode1->nop == knopComputedName && computedPropertyLocation != Js::Constants::NoRegister)
    {
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Conv_Prop, computedPropertyLocation, pnode1->location);
    }

    Emit(pnode2, byteCodeGenerator, funcInfo, false, false, computedPropertyLocation);
}

void EmitBinaryReference(ParseNode *pnode1, ParseNode *pnode2, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, BOOL fLoadLhs)
{
    // Make sure that the RHS of an assignment doesn't kill the opnd's of the expression on the LHS.
    switch (pnode1->nop)
    {
    case knopName:
        if (fLoadLhs && MayHaveSideEffectOnNode(pnode1, pnode2))
        {
            // Given x op y, y may kill x, so stash x.
            // Note that this only matters if we're loading x prior to the op.
            SaveOpndValue(pnode1, funcInfo);
        }
        break;
    case knopDot:
        if (fLoadLhs)
        {
            // We're loading the value of the LHS before the RHS, so make sure the LHS gets a register first.
            funcInfo->AcquireLoc(pnode1);
        }
        if (MayHaveSideEffectOnNode(pnode1->AsParseNodeBin()->pnode1, pnode2))
        {
            // Given x.y op z, z may kill x, so stash x away.
            SaveOpndValue(pnode1->AsParseNodeBin()->pnode1, funcInfo);
        }
        break;
    case knopIndex:
        if (fLoadLhs)
        {
            // We're loading the value of the LHS before the RHS, so make sure the LHS gets a register first.
            funcInfo->AcquireLoc(pnode1);
        }
        if (MayHaveSideEffectOnNode(pnode1->AsParseNodeBin()->pnode1, pnode2) ||
            MayHaveSideEffectOnNode(pnode1->AsParseNodeBin()->pnode1, pnode1->AsParseNodeBin()->pnode2))
        {
            // Given x[y] op z, y or z may kill x, so stash x away.
            SaveOpndValue(pnode1->AsParseNodeBin()->pnode1, funcInfo);
        }
        if (MayHaveSideEffectOnNode(pnode1->AsParseNodeBin()->pnode2, pnode2))
        {
            // Given x[y] op z, z may kill y, so stash y away.
            // But make sure that x gets a register before y.
            funcInfo->AcquireLoc(pnode1->AsParseNodeBin()->pnode1);
            SaveOpndValue(pnode1->AsParseNodeBin()->pnode2, funcInfo);
        }
        break;
    }

    if (fLoadLhs)
    {
        // Emit code to load the value of the LHS.
        EmitLoad(pnode1, byteCodeGenerator, funcInfo);
    }
    else
    {
        // Emit code to evaluate the LHS opnds, but don't load the LHS's value.
        EmitReference(pnode1, byteCodeGenerator, funcInfo);
    }

    // Evaluate the RHS.
    Emit(pnode2, byteCodeGenerator, funcInfo, false);
}

void EmitUseBeforeDeclarationRuntimeError(ByteCodeGenerator * byteCodeGenerator, Js::RegSlot location)
{
    byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_UseBeforeDeclaration));

    if (location != Js::Constants::NoRegister)
    {
        // Optionally load something into register in order to do not confuse IRBuilder. This value will never be used.
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, location);
    }
}

void EmitUseBeforeDeclaration(Symbol *sym, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    // Don't emit static use-before-declaration error in a closure or dynamic scope case. We detect such cases with dynamic checks,
    // if necessary.
    if (sym != nullptr &&
        !sym->GetIsModuleExportStorage() &&
        sym->GetNeedDeclaration() &&
        byteCodeGenerator->GetCurrentScope()->HasStaticPathToAncestor(sym->GetScope()) &&
        sym->GetScope()->GetFunc() == funcInfo)
    {
        EmitUseBeforeDeclarationRuntimeError(byteCodeGenerator, Js::Constants::NoRegister);
    }
}

void EmitBinary(Js::OpCode opcode, ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    byteCodeGenerator->StartStatement(pnode);
    EmitBinaryOpnds(pnode->AsParseNodeBin()->pnode1, pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
    funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode2);
    funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode1);
    funcInfo->AcquireLoc(pnode);
    byteCodeGenerator->Writer()->Reg3(opcode,
        pnode->location,
        pnode->AsParseNodeBin()->pnode1->location,
        pnode->AsParseNodeBin()->pnode2->location);
    byteCodeGenerator->EndStatement(pnode);
}

bool CollectConcat(ParseNode *pnodeAdd, DListCounted<ParseNode *, ArenaAllocator>& concatOpnds, ArenaAllocator *arenaAllocator)
{
    Assert(pnodeAdd->nop == knopAdd);
    Assert(pnodeAdd->CanFlattenConcatExpr());

    bool doConcatString = false;
    DList<ParseNode*, ArenaAllocator> pnodeStack(arenaAllocator);
    pnodeStack.Prepend(pnodeAdd->AsParseNodeBin()->pnode2);
    ParseNode * pnode = pnodeAdd->AsParseNodeBin()->pnode1;
    while (true)
    {
        if (!pnode->CanFlattenConcatExpr())
        {
            concatOpnds.Append(pnode);
        }
        else if (pnode->nop == knopStr)
        {
            concatOpnds.Append(pnode);

            // Detect if there are any string larger then the append size limit.
            // If there are, we can do concat; otherwise, still use add so we will not lose the AddLeftDead opportunities.
            doConcatString = doConcatString || !Js::CompoundString::ShouldAppendChars(pnode->AsParseNodeStr()->pid->Cch());
        }
        else
        {
            Assert(pnode->nop == knopAdd);
            pnodeStack.Prepend(pnode->AsParseNodeBin()->pnode2);
            pnode = pnode->AsParseNodeBin()->pnode1;
            continue;
        }

        if (pnodeStack.Empty())
        {
            break;
        }

        pnode = pnodeStack.Head();
        pnodeStack.RemoveHead();
    }

    return doConcatString;
}

void EmitConcat3(ParseNode *pnode, ParseNode *pnode1, ParseNode *pnode2, ParseNode *pnode3, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    byteCodeGenerator->StartStatement(pnode);
    if (MayHaveSideEffectOnNode(pnode1, pnode2) || MayHaveSideEffectOnNode(pnode1, pnode3))
    {
        SaveOpndValue(pnode1, funcInfo);
    }

    if (MayHaveSideEffectOnNode(pnode2, pnode3))
    {
        SaveOpndValue(pnode2, funcInfo);
    }

    Emit(pnode1, byteCodeGenerator, funcInfo, false);
    Emit(pnode2, byteCodeGenerator, funcInfo, false);
    Emit(pnode3, byteCodeGenerator, funcInfo, false);
    funcInfo->ReleaseLoc(pnode3);
    funcInfo->ReleaseLoc(pnode2);
    funcInfo->ReleaseLoc(pnode1);
    funcInfo->AcquireLoc(pnode);
    byteCodeGenerator->Writer()->Reg4(Js::OpCode::Concat3,
        pnode->location,
        pnode1->location,
        pnode2->location,
        pnode3->location);
    byteCodeGenerator->EndStatement(pnode);
}

void EmitNewConcatStrMulti(ParseNode *pnode, uint8 count, ParseNode *pnode1, ParseNode *pnode2, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    EmitBinaryOpnds(pnode1, pnode2, byteCodeGenerator, funcInfo);
    funcInfo->ReleaseLoc(pnode2);
    funcInfo->ReleaseLoc(pnode1);
    funcInfo->AcquireLoc(pnode);
    byteCodeGenerator->Writer()->Reg3B1(Js::OpCode::NewConcatStrMulti,
        pnode->location,
        pnode1->location,
        pnode2->location,
        count);
}

void EmitAdd(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
    Assert(pnode->nop == knopAdd);

    if (pnode->CanFlattenConcatExpr())
    {
        // We should only have a string concat if the feature is on.
        Assert(!PHASE_OFF1(Js::ByteCodeConcatExprOptPhase));
        DListCounted<ParseNode*, ArenaAllocator> concatOpnds(byteCodeGenerator->GetAllocator());
        bool doConcatString = CollectConcat(pnode, concatOpnds, byteCodeGenerator->GetAllocator());
        if (doConcatString)
        {
            uint concatCount = concatOpnds.Count();
            Assert(concatCount >= 2);

            // Don't do concatN if the number is too high
            // CONSIDER: although we could have done multiple ConcatNs
            if (concatCount > 2 && concatCount <= UINT8_MAX)
            {
#if DBG
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
                ParseNode * pnode1 = concatOpnds.Head();
                concatOpnds.RemoveHead();
                ParseNode * pnode2 = concatOpnds.Head();
                concatOpnds.RemoveHead();
                if (concatCount == 3)
                {
                    OUTPUT_TRACE_DEBUGONLY(Js::ByteCodeConcatExprOptPhase, _u("%s(%s) offset:#%d : Concat3\n"),
                        funcInfo->GetParsedFunctionBody()->GetDisplayName(),
                        funcInfo->GetParsedFunctionBody()->GetDebugNumberSet(debugStringBuffer),
                        byteCodeGenerator->Writer()->ByteCodeDataSize());
                    EmitConcat3(pnode, pnode1, pnode2, concatOpnds.Head(), byteCodeGenerator, funcInfo);
                    return;
                }

                OUTPUT_TRACE_DEBUGONLY(Js::ByteCodeConcatExprOptPhase, _u("%s(%s) offset:#%d: ConcatMulti %d\n"),
                    funcInfo->GetParsedFunctionBody()->GetDisplayName(),
                    funcInfo->GetParsedFunctionBody()->GetDebugNumberSet(debugStringBuffer),
                    byteCodeGenerator->Writer()->ByteCodeDataSize(), concatCount);
                byteCodeGenerator->StartStatement(pnode);
                funcInfo->AcquireLoc(pnode);

                // CONSIDER: this may cause the backend not able CSE repeating pattern within the concat.
                EmitNewConcatStrMulti(pnode, (uint8)concatCount, pnode1, pnode2, byteCodeGenerator, funcInfo);

                uint i = 2;
                do
                {
                    ParseNode * currNode = concatOpnds.Head();
                    concatOpnds.RemoveHead();
                    ParseNode * currNode2 = concatOpnds.Head();
                    concatOpnds.RemoveHead();

                    EmitBinaryOpnds(currNode, currNode2, byteCodeGenerator, funcInfo);
                    funcInfo->ReleaseLoc(currNode2);
                    funcInfo->ReleaseLoc(currNode);
                    byteCodeGenerator->Writer()->Reg3B1(
                        Js::OpCode::SetConcatStrMultiItem2, pnode->location, currNode->location, currNode2->location, (uint8)i);
                    i += 2;
                } while (concatOpnds.Count() > 1);

                if (!concatOpnds.Empty())
                {
                    ParseNode * currNode = concatOpnds.Head();
                    Emit(currNode, byteCodeGenerator, funcInfo, false);
                    funcInfo->ReleaseLoc(currNode);
                    byteCodeGenerator->Writer()->Reg2B1(
                        Js::OpCode::SetConcatStrMultiItem, pnode->location, currNode->location, (uint8)i);
                    i++;
                }

                Assert(concatCount == i);
                byteCodeGenerator->EndStatement(pnode);
                return;
            }
        }

        // Since we collected all the node already, let's just emit them instead of doing it recursively.
        byteCodeGenerator->StartStatement(pnode);
        ParseNode * currNode = concatOpnds.Head();
        concatOpnds.RemoveHead();
        ParseNode * currNode2 = concatOpnds.Head();
        concatOpnds.RemoveHead();

        EmitBinaryOpnds(currNode, currNode2, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(currNode2);
        funcInfo->ReleaseLoc(currNode);
        Js::RegSlot dstReg = funcInfo->AcquireLoc(pnode);
        byteCodeGenerator->Writer()->Reg3(
            Js::OpCode::Add_A, dstReg, currNode->location, currNode2->location);
        while (!concatOpnds.Empty())
        {
            currNode = concatOpnds.Head();
            concatOpnds.RemoveHead();
            Emit(currNode, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(currNode);
            byteCodeGenerator->Writer()->Reg3(
                Js::OpCode::Add_A, dstReg, dstReg, currNode->location);
        }
        byteCodeGenerator->EndStatement(pnode);
    }
    else
    {
        EmitBinary(Js::OpCode::Add_A, pnode, byteCodeGenerator, funcInfo);
    }
}

void ByteCodeGenerator::EmitLeaveOpCodesBeforeYield()
{
    for (TryScopeRecord* node = this->tryScopeRecordsList.Tail(); node != nullptr; node = node->Previous())
    {
        switch (node->op)
        {
        case Js::OpCode::TryFinallyWithYield:
            this->Writer()->Empty(Js::OpCode::LeaveNull);
            break;
        case Js::OpCode::TryCatch:
        case Js::OpCode::ResumeFinally:
        case Js::OpCode::ResumeCatch:
            this->Writer()->Empty(Js::OpCode::Leave);
            break;
        default:
            AssertMsg(false, "Unexpected OpCode before Yield in the Try-Catch-Finally cache for generator!");
            break;
        }
    }
}

void ByteCodeGenerator::EmitTryBlockHeadersAfterYield()
{
    for (TryScopeRecord* node = this->tryScopeRecordsList.Head(); node != nullptr; node = node->Next())
    {
        switch (node->op)
        {
        case Js::OpCode::TryCatch:
            this->Writer()->Br(node->op, node->label);
            break;
        case Js::OpCode::TryFinallyWithYield:
        case Js::OpCode::ResumeFinally:
            this->Writer()->BrReg2(node->op, node->label, node->reg1, node->reg2);
            break;
        case Js::OpCode::ResumeCatch:
            this->Writer()->Empty(node->op);
            break;
        default:
            AssertMsg(false, "Unexpected OpCode after yield in the Try-Catch-Finally cache for generator!");
            break;
        }
    }
}

void EmitYield(Js::RegSlot inputLocation, Js::RegSlot resultLocation, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, Js::RegSlot yieldStarIterator)
{
    // If the bytecode emitted by this function is part of 'yield*', inputLocation is the object
    // returned by the iterable's next/return/throw method. Otherwise, it is the yielded value.
    if (yieldStarIterator == Js::Constants::NoRegister)
    {
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::NewScObjectSimple, funcInfo->yieldRegister);

        uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->yieldRegister, Js::PropertyIds::value, false, true);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::StFld, inputLocation, funcInfo->yieldRegister, cacheId);

        cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->yieldRegister, Js::PropertyIds::done, false, true);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::StFld, funcInfo->falseConstantRegister, funcInfo->yieldRegister, cacheId);
    }
    else
    {
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, funcInfo->yieldRegister, inputLocation);
    }

    byteCodeGenerator->EmitLeaveOpCodesBeforeYield();
    byteCodeGenerator->Writer()->Reg2(Js::OpCode::Yield, funcInfo->yieldRegister, funcInfo->yieldRegister);
    byteCodeGenerator->EmitTryBlockHeadersAfterYield();

    if (yieldStarIterator == Js::Constants::NoRegister)
    {
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::ResumeYield, resultLocation, funcInfo->yieldRegister);
    }
    else
    {
        byteCodeGenerator->Writer()->Reg3(Js::OpCode::ResumeYieldStar, resultLocation, funcInfo->yieldRegister, yieldStarIterator);
    }
}

void EmitYieldStar(ParseNodeUni* yieldStarNode, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    funcInfo->AcquireLoc(yieldStarNode);

    Js::ByteCodeLabel loopEntrance = byteCodeGenerator->Writer()->DefineLabel();
    Js::ByteCodeLabel continuePastLoop = byteCodeGenerator->Writer()->DefineLabel();

    Js::RegSlot iteratorLocation = funcInfo->AcquireTmpRegister();

    // Evaluate operand
    Emit(yieldStarNode->pnode1, byteCodeGenerator, funcInfo, false);
    funcInfo->ReleaseLoc(yieldStarNode->pnode1);

    EmitGetIterator(iteratorLocation, yieldStarNode->pnode1->location, byteCodeGenerator, funcInfo);

    // Call the iterator's next()
    EmitIteratorNext(yieldStarNode->location, iteratorLocation, funcInfo->undefinedConstantRegister, byteCodeGenerator, funcInfo);

    uint loopId = byteCodeGenerator->Writer()->EnterLoop(loopEntrance);
    // since a yield* doesn't have a user defined body, we cannot return from this loop
    // which means we don't need to support EmitJumpCleanup() and there do not need to
    // remember the loopId like the loop statements do.

    Js::RegSlot doneLocation = funcInfo->AcquireTmpRegister();
    EmitIteratorComplete(doneLocation, yieldStarNode->location, byteCodeGenerator, funcInfo);

    // branch past the loop if the done property is truthy
    byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, continuePastLoop, doneLocation);
    funcInfo->ReleaseTmpRegister(doneLocation);

    EmitYield(yieldStarNode->location, yieldStarNode->location, byteCodeGenerator, funcInfo, iteratorLocation);

    funcInfo->ReleaseTmpRegister(iteratorLocation);

    byteCodeGenerator->Writer()->Br(loopEntrance);
    byteCodeGenerator->Writer()->MarkLabel(continuePastLoop);
    byteCodeGenerator->Writer()->ExitLoop(loopId);

    // Put the iterator result's value in yieldStarNode->location.
    // It will be used as the result value of the yield* operator expression.
    EmitIteratorValue(yieldStarNode->location, yieldStarNode->location, byteCodeGenerator, funcInfo);
}

void TrackIntConstantsOnGlobalUserObject(ByteCodeGenerator *byteCodeGenerator, bool isSymGlobalAndSingleAssignment, Js::PropertyId propertyId)
{
    if (isSymGlobalAndSingleAssignment)
    {
        byteCodeGenerator->GetScriptContext()->TrackIntConstPropertyOnGlobalUserObject(propertyId);
    }
}

void TrackIntConstantsOnGlobalObject(ByteCodeGenerator *byteCodeGenerator, bool isSymGlobalAndSingleAssignment, Js::PropertyId propertyId)
{
    if (isSymGlobalAndSingleAssignment)
    {
        byteCodeGenerator->GetScriptContext()->TrackIntConstPropertyOnGlobalObject(propertyId);
    }
}

void TrackIntConstantsOnGlobalObject(ByteCodeGenerator *byteCodeGenerator, Symbol *sym)
{
    if (sym && sym->GetIsGlobal() && sym->IsAssignedOnce())
    {
        Js::PropertyId propertyId = sym->EnsurePosition(byteCodeGenerator);
        byteCodeGenerator->GetScriptContext()->TrackIntConstPropertyOnGlobalObject(propertyId);
    }
}

void TrackMemberNodesInObjectForIntConstants(ByteCodeGenerator *byteCodeGenerator, ParseNodePtr objNode)
{
    Assert(objNode->nop == knopObject);

    ParseNodePtr memberList = objNode->AsParseNodeUni()->pnode1;

    while (memberList != nullptr)
    {
        ParseNodePtr memberNode = memberList->nop == knopList ? memberList->AsParseNodeBin()->pnode1 : memberList;
        ParseNodePtr memberNameNode = memberNode->AsParseNodeBin()->pnode1;
        ParseNodePtr memberValNode = memberNode->AsParseNodeBin()->pnode2;

        if (memberNameNode->nop != knopComputedName && memberValNode->nop == knopInt)
        {
            Js::PropertyId propertyId = memberNameNode->AsParseNodeStr()->pid->GetPropertyId();
            TrackIntConstantsOnGlobalUserObject(byteCodeGenerator, true, propertyId);
        }

        memberList = memberList->nop == knopList ? memberList->AsParseNodeBin()->pnode2 : nullptr;
    }
}

void TrackGlobalIntAssignmentsForknopDotProps(ParseNodePtr knopDotNode, ByteCodeGenerator * byteCodeGenerator)
{
    Assert(knopDotNode->nop == knopDot);

    ParseNodePtr objectNode = knopDotNode->AsParseNodeBin()->pnode1;
    ParseNodeName * propertyNode = knopDotNode->AsParseNodeBin()->pnode2->AsParseNodeName();
    bool isSymGlobalAndSingleAssignment = false;

    if (objectNode->nop == knopName)
    {
        if (ByteCodeGenerator::IsThis(objectNode))
        {
            // Assume 'this' always refer to GlobalObject
            // Cases like "this.a = "
            isSymGlobalAndSingleAssignment = propertyNode->pid->IsSingleAssignment();
            Js::PropertyId propertyId = propertyNode->PropertyIdFromNameNode();
            TrackIntConstantsOnGlobalObject(byteCodeGenerator, isSymGlobalAndSingleAssignment, propertyId);
        }
        else
        {
            Symbol * sym = objectNode->AsParseNodeName()->sym;
            isSymGlobalAndSingleAssignment = sym && sym->GetIsGlobal() && sym->IsAssignedOnce() && propertyNode->pid->IsSingleAssignment();
            Js::PropertyId propertyId = propertyNode->PropertyIdFromNameNode();
            TrackIntConstantsOnGlobalUserObject(byteCodeGenerator, isSymGlobalAndSingleAssignment, propertyId);
        }
    }
}

void TrackGlobalIntAssignments(ParseNodePtr pnode, ByteCodeGenerator * byteCodeGenerator)
{
    // Track the Global Int Constant properties' assignments here.
    uint nodeType = ParseNode::Grfnop(pnode->nop);
    if (nodeType & fnopAsg)
    {
        if (nodeType & fnopBin)
        {
            ParseNodePtr lhs = pnode->AsParseNodeBin()->pnode1;
            ParseNodePtr rhs = pnode->AsParseNodeBin()->pnode2;

            Assert(lhs && rhs);

            // Don't track other than integers and objects with member nodes.
            if (rhs->nop == knopObject)
            {
                TrackMemberNodesInObjectForIntConstants(byteCodeGenerator, rhs);
            }
            else if (rhs->nop != knopInt &&
                ((rhs->nop != knopLsh && rhs->nop != knopRsh) || (rhs->AsParseNodeBin()->pnode1->nop != knopInt || rhs->AsParseNodeBin()->pnode2->nop != knopInt)))
            {
                return;
            }

            if (lhs->nop == knopName)
            {
                // Handle "a = <Integer>" cases here
                Symbol * sym = lhs->AsParseNodeName()->sym;
                TrackIntConstantsOnGlobalObject(byteCodeGenerator, sym);
            }
            else if (lhs->nop == knopDot && lhs->AsParseNodeBin()->pnode2->nop == knopName)
            {
                // Cases like "obj.a = <Integer>"
                TrackGlobalIntAssignmentsForknopDotProps(lhs, byteCodeGenerator);
            }
        }
        else if (nodeType & fnopUni)
        {
            ParseNodePtr lhs = pnode->AsParseNodeUni()->pnode1;

            if (lhs->nop == knopName)
            {
                // Cases like "a++"
                Symbol * sym = lhs->AsParseNodeName()->sym;
                TrackIntConstantsOnGlobalObject(byteCodeGenerator, sym);
            }
            else if (lhs->nop == knopDot && lhs->AsParseNodeBin()->pnode2->nop == knopName)
            {
                // Cases like "obj.a++"
                TrackGlobalIntAssignmentsForknopDotProps(lhs, byteCodeGenerator);
            }
        }
    }
}

void Emit(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo, BOOL fReturnValue, bool isConstructorCall, Js::RegSlot bindingNameLocation, bool isTopLevel)
{
    if (pnode == nullptr)
    {
        return;
    }

    ThreadContext::ProbeCurrentStackNoDispose(Js::Constants::MinStackByteCodeVisitor, byteCodeGenerator->GetScriptContext());

    TrackGlobalIntAssignments(pnode, byteCodeGenerator);

    // printNop(pnode->nop);
    switch (pnode->nop)
    {
    case knopList:
        EmitList(pnode, byteCodeGenerator, funcInfo);
        break;
    case knopInt:
        // currently, these are loaded at the top
        break;
        // PTNODE(knopFlt        , "flt const"    ,None    ,Flt  ,fnopLeaf|fnopConst)
    case knopFlt:
        // currently, these are loaded at the top
        break;
        // PTNODE(knopStr        , "str const"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopStr:
        // TODO: protocol for combining string constants
        break;
        // PTNODE(knopRegExp     , "reg expr"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopRegExp:
        funcInfo->GetParsedFunctionBody()->SetLiteralRegex(pnode->AsParseNodeRegExp()->regexPatternIndex, pnode->AsParseNodeRegExp()->regexPattern);
        byteCodeGenerator->Writer()->Reg1Unsigned1(Js::OpCode::NewRegEx, funcInfo->AcquireLoc(pnode), pnode->AsParseNodeRegExp()->regexPatternIndex);
        break;
        // PTNODE(knopNull       , "null"        ,Null    ,None ,fnopLeaf)
    case knopNull:
        // enregistered
        break;
        // PTNODE(knopFalse      , "false"        ,False   ,None ,fnopLeaf)
    case knopFalse:
        // enregistered
        break;
        // PTNODE(knopTrue       , "true"        ,True    ,None ,fnopLeaf)
    case knopTrue:
        // enregistered
        break;
        // PTNODE(knopEmpty      , "empty"        ,Empty   ,None ,fnopLeaf)
    case knopEmpty:
        break;
        // Unary operators.
    // PTNODE(knopNot        , "~"            ,BitNot  ,Uni  ,fnopUni)
    case knopNot:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        byteCodeGenerator->Writer()->Reg2(
            Js::OpCode::Not_A, funcInfo->AcquireLoc(pnode), pnode->AsParseNodeUni()->pnode1->location);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
        // PTNODE(knopNeg        , "unary -"    ,Neg     ,Uni  ,fnopUni)
    case knopNeg:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        funcInfo->AcquireLoc(pnode);
        byteCodeGenerator->Writer()->Reg2(
            Js::OpCode::Neg_A, pnode->location, pnode->AsParseNodeUni()->pnode1->location);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
        // PTNODE(knopPos        , "unary +"    ,Pos     ,Uni  ,fnopUni)
    case knopPos:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        byteCodeGenerator->Writer()->Reg2(
            Js::OpCode::Conv_Num, funcInfo->AcquireLoc(pnode), pnode->AsParseNodeUni()->pnode1->location);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
        // PTNODE(knopLogNot     , "!"            ,LogNot  ,Uni  ,fnopUni)
    case knopLogNot:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Js::ByteCodeLabel doneLabel = byteCodeGenerator->Writer()->DefineLabel();
        // For boolean expressions that compute a result, we have to burn a register for the result
        // so that the back end can identify it cheaply as a single temp lifetime. Revisit this if we do
        // full-on renaming in the back end.
        funcInfo->AcquireLoc(pnode);
        if (pnode->AsParseNodeUni()->pnode1->nop == knopInt)
        {
            int32 value = pnode->AsParseNodeUni()->pnode1->AsParseNodeInt()->lw;
            Js::OpCode op = value ? Js::OpCode::LdFalse : Js::OpCode::LdTrue;
            byteCodeGenerator->Writer()->Reg1(op, pnode->location);
        }
        else
        {
            Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
            byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdFalse, pnode->location);
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrTrue_A, doneLabel, pnode->AsParseNodeUni()->pnode1->location);
            byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, pnode->location);
            byteCodeGenerator->Writer()->MarkLabel(doneLabel);
        }
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    }
    // PTNODE(knopEllipsis     , "..."       ,Spread  ,Uni         , fnopUni)
    case knopEllipsis:
    {
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        // Transparently pass the location of the array.
        pnode->location = pnode->AsParseNodeUni()->pnode1->location;
        break;
    }
    // PTNODE(knopIncPost    , "post++"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
    case knopIncPost:
    case knopDecPost:
        // FALL THROUGH to the faster pre-inc/dec case if the result of the expression is not needed.
        if (pnode->isUsed || fReturnValue)
        {
            byteCodeGenerator->StartStatement(pnode);
            const Js::OpCode op = (pnode->nop == knopDecPost) ? Js::OpCode::Sub_A : Js::OpCode::Add_A;
            ParseNode* pnode1 = pnode->AsParseNodeUni()->pnode1;

            // Grab a register for the expression result.
            funcInfo->AcquireLoc(pnode);

            // Load the initial value, convert it (this is the expression result), and increment it.
            EmitLoad(pnode1, byteCodeGenerator, funcInfo);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Conv_Num, pnode->location, pnode1->location);

            // Use temporary register if lhs cannot be assigned
            Js::RegSlot incDecResult = pnode1->location;
            if (funcInfo->RegIsConst(incDecResult) ||
                (pnode1->nop == knopName && pnode1->AsParseNodeName()->sym && pnode1->AsParseNodeName()->sym->GetIsFuncExpr()))
            {
                incDecResult = funcInfo->AcquireTmpRegister();
            }

            Js::RegSlot oneReg = funcInfo->constantToRegister.LookupWithKey(1, Js::Constants::NoRegister);
            Assert(oneReg != Js::Constants::NoRegister);
            byteCodeGenerator->Writer()->Reg3(op, incDecResult, pnode->location, oneReg);

            // Store the incremented value.
            EmitAssignment(nullptr, pnode1, incDecResult, byteCodeGenerator, funcInfo);

            // Release the incremented value and the l-value.
            if (incDecResult != pnode1->location)
            {
                funcInfo->ReleaseTmpRegister(incDecResult);
            }
            funcInfo->ReleaseLoad(pnode1);
            byteCodeGenerator->EndStatement(pnode);

            break;
        }
        else
        {
            pnode->nop = (pnode->nop == knopIncPost) ? knopIncPre : knopDecPre;
        }
        // FALL THROUGH to the fast pre-inc/dec case if the result of the expression is not needed.

    // PTNODE(knopIncPre     , "++ pre"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
    case knopIncPre:
    case knopDecPre:
    {
        byteCodeGenerator->StartStatement(pnode);
        const Js::OpCode op = (pnode->nop == knopDecPre) ? Js::OpCode::Decr_A : Js::OpCode::Incr_A;
        ParseNode* pnode1 = pnode->AsParseNodeUni()->pnode1;

        // Assign a register for the result only if the result is used or the LHS can't be assigned to
        // (i.e., is a constant).
        const bool need_result_location =
            pnode->isUsed
            || fReturnValue
            || funcInfo->RegIsConst(pnode1->location)
            || (pnode1->nop == knopName && pnode1->AsParseNodeName()->sym && pnode1->AsParseNodeName()->sym->GetIsFuncExpr());

        if (need_result_location)
        {
            const Js::RegSlot result_location = funcInfo->AcquireLoc(pnode);

            EmitLoad(pnode1, byteCodeGenerator, funcInfo);
            byteCodeGenerator->Writer()->Reg2(op, result_location, pnode1->location);

            // Store the incremented value and release the l-value.
            EmitAssignment(nullptr, pnode1, result_location, byteCodeGenerator, funcInfo);
        }
        else
        {
            EmitLoad(pnode1, byteCodeGenerator, funcInfo);
            byteCodeGenerator->Writer()->Reg2(op, pnode1->location, pnode1->location);

            // Store the incremented value and release the l-value.
            EmitAssignment(nullptr, pnode1, pnode1->location, byteCodeGenerator, funcInfo);
        }
        funcInfo->ReleaseLoad(pnode1);

        byteCodeGenerator->EndStatement(pnode);
        break;
    }
    // PTNODE(knopTypeof     , "typeof"    ,None    ,Uni  ,fnopUni)
    case knopTypeof:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        ParseNode* pnodeOpnd = pnode->AsParseNodeUni()->pnode1;
        switch (pnodeOpnd->nop)
        {
        case knopDot:
        {
            Emit(pnodeOpnd->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
            Js::PropertyId propertyId = pnodeOpnd->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();
            Assert(pnodeOpnd->AsParseNodeBin()->pnode2->nop == knopName);
            funcInfo->ReleaseLoc(pnodeOpnd->AsParseNodeBin()->pnode1);
            funcInfo->AcquireLoc(pnode);

            byteCodeGenerator->EmitTypeOfFld(funcInfo, propertyId, pnode->location, pnodeOpnd->AsParseNodeBin()->pnode1->location, Js::OpCode::LdFldForTypeOf);
            break;
        }

        case knopIndex:
        {
            EmitBinaryOpnds(pnodeOpnd->AsParseNodeBin()->pnode1, pnodeOpnd->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
            funcInfo->ReleaseLoc(pnodeOpnd->AsParseNodeBin()->pnode2);
            funcInfo->ReleaseLoc(pnodeOpnd->AsParseNodeBin()->pnode1);
            funcInfo->AcquireLoc(pnode);
            byteCodeGenerator->Writer()->Element(Js::OpCode::TypeofElem, pnode->location, pnodeOpnd->AsParseNodeBin()->pnode1->location, pnodeOpnd->AsParseNodeBin()->pnode2->location);
            break;
        }
        case knopName:
        {
            ParseNodeName * pnodeNameOpnd = pnodeOpnd->AsParseNodeName();
            if (pnodeNameOpnd->IsUserIdentifier())
            {
                funcInfo->AcquireLoc(pnode);
                byteCodeGenerator->EmitPropTypeof(pnode->location, pnodeNameOpnd->sym, pnodeNameOpnd->pid, funcInfo);
                break;
            }
            // Special names should fallthrough to default case
        }

        default:
            Emit(pnodeOpnd, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(pnodeOpnd);
            byteCodeGenerator->Writer()->Reg2(
                Js::OpCode::Typeof, funcInfo->AcquireLoc(pnode), pnodeOpnd->location);
            break;
        }
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    }
    // PTNODE(knopVoid       , "void"        ,Void    ,Uni  ,fnopUni)
    case knopVoid:
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, funcInfo->AcquireLoc(pnode));
        break;
        // PTNODE(knopArray      , "arr cnst"    ,None    ,Uni  ,fnopUni)
    case knopArray:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        EmitArrayLiteral(pnode, byteCodeGenerator, funcInfo);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
        // PTNODE(knopObject     , "obj cnst"    ,None    ,Uni  ,fnopUni)
    case knopObject:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        funcInfo->AcquireLoc(pnode);
        EmitObjectInitializers(pnode->AsParseNodeUni()->pnode1, pnode->location, byteCodeGenerator, funcInfo);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
        // PTNODE(knopComputedName, "[name]"      ,None    ,Uni  ,fnopUni)
    case knopComputedName:
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        if (pnode->location == Js::Constants::NoRegister)
        {
            // The name is some expression with no home location. We can just re-use the register.
            pnode->location = pnode->AsParseNodeUni()->pnode1->location;
        }
        else if (pnode->location != pnode->AsParseNodeUni()->pnode1->location)
        {
            // The name had to be protected from side-effects of the RHS.
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeUni()->pnode1->location);
        }
        break;
        // Binary and Ternary Operators
    case knopAdd:
        EmitAdd(pnode, byteCodeGenerator, funcInfo);
        break;
    case knopSub:
    case knopMul:
    case knopExpo:
    case knopDiv:
    case knopMod:
    case knopOr:
    case knopXor:
    case knopAnd:
    case knopLsh:
    case knopRsh:
    case knopRs2:
    case knopIn:
        EmitBinary(nopToOp[pnode->nop], pnode, byteCodeGenerator, funcInfo);
        break;
    case knopInstOf:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        EmitBinaryOpnds(pnode->AsParseNodeBin()->pnode1, pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode2);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode1);
        funcInfo->AcquireLoc(pnode);
        uint cacheId = funcInfo->NewIsInstInlineCache();
        byteCodeGenerator->Writer()->Reg3C(nopToOp[pnode->nop], pnode->location, pnode->AsParseNodeBin()->pnode1->location,
            pnode->AsParseNodeBin()->pnode2->location, cacheId);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
    }
    break;
    case knopEq:
    case knopEqv:
    case knopNEqv:
    case knopNe:
    case knopLt:
    case knopLe:
    case knopGe:
    case knopGt:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        EmitBinaryOpnds(pnode->AsParseNodeBin()->pnode1, pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode2);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode1);
        funcInfo->AcquireLoc(pnode);
        byteCodeGenerator->Writer()->Reg3(nopToCMOp[pnode->nop], pnode->location, pnode->AsParseNodeBin()->pnode1->location,
            pnode->AsParseNodeBin()->pnode2->location);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    case knopNew:
    {
        EmitNew(pnode, byteCodeGenerator, funcInfo);

        byteCodeGenerator->EndStatement(pnode);
        break;
    }
    case knopDelete:
    {
        ParseNode *pexpr = pnode->AsParseNodeUni()->pnode1;
        byteCodeGenerator->StartStatement(pnode);
        switch (pexpr->nop)
        {
        case knopName:
        {
            ParseNodeName * pnodeName = pexpr->AsParseNodeName();
            if (pnodeName->IsSpecialName())
            {
                funcInfo->AcquireLoc(pnode);
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdTrue, pnode->location);
            }
            else
            {
                funcInfo->AcquireLoc(pnode);
                byteCodeGenerator->EmitPropDelete(pnode->location, pnodeName->sym, pnodeName->pid, funcInfo);
            }
            break;
        }
        case knopDot:
        {
            if (ByteCodeGenerator::IsSuper(pexpr->AsParseNodeBin()->pnode1))
            {
                byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeReferenceError, SCODE_CODE(JSERR_DeletePropertyWithSuper));

                funcInfo->AcquireLoc(pnode);
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, pnode->location);
            }
            else
            {
                Emit(pexpr->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);

                funcInfo->ReleaseLoc(pexpr->AsParseNodeBin()->pnode1);
                Js::PropertyId propertyId = pexpr->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();
                funcInfo->AcquireLoc(pnode);
                byteCodeGenerator->Writer()->Property(Js::OpCode::DeleteFld, pnode->location, pexpr->AsParseNodeBin()->pnode1->location,
                    funcInfo->FindOrAddReferencedPropertyId(propertyId));
            }

            break;
        }
        case knopIndex:
        {
            EmitBinaryOpnds(pexpr->AsParseNodeBin()->pnode1, pexpr->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);
            funcInfo->ReleaseLoc(pexpr->AsParseNodeBin()->pnode2);
            funcInfo->ReleaseLoc(pexpr->AsParseNodeBin()->pnode1);
            funcInfo->AcquireLoc(pnode);
            byteCodeGenerator->Writer()->Element(Js::OpCode::DeleteElemI_A, pnode->location, pexpr->AsParseNodeBin()->pnode1->location, pexpr->AsParseNodeBin()->pnode2->location);
            break;
        }
        default:
        {
            Emit(pexpr, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(pexpr);
            byteCodeGenerator->Writer()->Reg2(
                Js::OpCode::Delete_A, funcInfo->AcquireLoc(pnode), pexpr->location);
            break;
        }
        }
        byteCodeGenerator->EndStatement(pnode);
        break;
    }
    case knopCall:
    {
        ParseNodeCall * pnodeCall = pnode->AsParseNodeCall();

        byteCodeGenerator->StartStatement(pnodeCall);

        if (pnodeCall->isSuperCall)
        {
            byteCodeGenerator->EmitSuperCall(funcInfo, pnodeCall->AsParseNodeSuperCall(), fReturnValue);
        }
        else if (pnodeCall->pnodeTarget->nop == knopImport)
        {
            ParseNodePtr args = pnodeCall->pnodeArgs;
            Assert(CountArguments(args) == 2); // import() takes one argument
            Emit(args, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(args);
            funcInfo->AcquireLoc(pnodeCall);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::ImportCall, pnodeCall->location, args->location);
        }
        else
        {
            if (pnodeCall->isApplyCall && funcInfo->GetApplyEnclosesArgs())
            {
                // TODO[ianhall]: Can we remove the ApplyCall bytecode gen time optimization?
                EmitApplyCall(pnodeCall, byteCodeGenerator, funcInfo, fReturnValue);
            }
            else
            {
                EmitCall(pnodeCall, byteCodeGenerator, funcInfo, fReturnValue, /*fEvaluateComponents*/ true);
            }
        }

        byteCodeGenerator->EndStatement(pnode);
        break;
    }
    case knopIndex:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        EmitBinaryOpnds(pnode->AsParseNodeBin()->pnode1, pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo);

        Js::RegSlot callObjLocation = pnode->AsParseNodeBin()->pnode1->location;
        Js::RegSlot protoLocation = callObjLocation;

        if (ByteCodeGenerator::IsSuper(pnode->AsParseNodeBin()->pnode1))
        {
            Emit(pnode->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
            protoLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, callObjLocation, funcInfo);
            funcInfo->ReleaseLoc(pnode->AsParseNodeSuperReference()->pnodeThis);
        }

        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode2);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode1);
        funcInfo->AcquireLoc(pnode);

        byteCodeGenerator->Writer()->Element(
            Js::OpCode::LdElemI_A, pnode->location, protoLocation, pnode->AsParseNodeBin()->pnode2->location);

        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    }
    // this is MemberExpression as rvalue
    case knopDot:
    {
        Emit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode1);
        funcInfo->AcquireLoc(pnode);
        Js::PropertyId propertyId = pnode->AsParseNodeBin()->pnode2->AsParseNodeName()->PropertyIdFromNameNode();

        Js::RegSlot callObjLocation = pnode->AsParseNodeBin()->pnode1->location;
        Js::RegSlot protoLocation = callObjLocation;

        if (propertyId == Js::PropertyIds::length)
        {
            uint cacheId = funcInfo->FindOrAddInlineCacheId(protoLocation, propertyId, false, false);
            byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdLen_A, pnode->location, protoLocation, cacheId);
        }
        else if (pnode->IsCallApplyTargetLoad())
        {
            if (ByteCodeGenerator::IsSuper(pnode->AsParseNodeBin()->pnode1))
            {
                Emit(pnode->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
                protoLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, callObjLocation, funcInfo);
                funcInfo->ReleaseLoc(pnode->AsParseNodeSuperReference()->pnodeThis);
            }
            uint cacheId = funcInfo->FindOrAddInlineCacheId(protoLocation, propertyId, false, false);
            byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFldForCallApplyTarget, pnode->location, protoLocation, cacheId);
        }
        else
        {
            if (ByteCodeGenerator::IsSuper(pnode->AsParseNodeBin()->pnode1))
            {
                Emit(pnode->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, funcInfo, false);
                protoLocation = byteCodeGenerator->EmitLdObjProto(Js::OpCode::LdHomeObjProto, callObjLocation, funcInfo);
                funcInfo->ReleaseLoc(pnode->AsParseNodeSuperReference()->pnodeThis);
                uint cacheId = funcInfo->FindOrAddInlineCacheId(protoLocation, propertyId, false, false);
                byteCodeGenerator->Writer()->PatchablePropertyWithThisPtr(Js::OpCode::LdSuperFld, pnode->location, protoLocation, pnode->AsParseNodeSuperReference()->pnodeThis->location, cacheId, isConstructorCall);
            }
            else
            {
                uint cacheId = funcInfo->FindOrAddInlineCacheId(callObjLocation, propertyId, false, false);
                byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, pnode->location, callObjLocation, cacheId, isConstructorCall);
            }
        }

        break;
    }

    // PTNODE(knopAsg        , "="            ,None    ,Bin  ,fnopBin|fnopAsg)
    case knopAsg:
    {
        ParseNode *lhs = pnode->AsParseNodeBin()->pnode1;
        ParseNode *rhs = pnode->AsParseNodeBin()->pnode2;
        byteCodeGenerator->StartStatement(pnode);
        if (pnode->isUsed || fReturnValue)
        {
            // If the assignment result is used, grab a register to hold it and pass it to EmitAssignment,
            // which will copy the assigned value there.
            funcInfo->AcquireLoc(pnode);
            EmitBinaryReference(lhs, rhs, byteCodeGenerator, funcInfo, false);
            EmitAssignment(pnode, lhs, rhs->location, byteCodeGenerator, funcInfo);
        }
        else
        {
            EmitBinaryReference(lhs, rhs, byteCodeGenerator, funcInfo, false);
            EmitAssignment(nullptr, lhs, rhs->location, byteCodeGenerator, funcInfo);
        }
        funcInfo->ReleaseLoc(rhs);
        if (!(byteCodeGenerator->IsES6DestructuringEnabled() && (lhs->IsPattern())))
        {
            funcInfo->ReleaseReference(lhs);
        }
        byteCodeGenerator->EndStatement(pnode);
        break;
    }

    case knopName:
        funcInfo->AcquireLoc(pnode);

        if (ByteCodeGenerator::IsThis(pnode))
        {
            byteCodeGenerator->EmitPropLoadThis(pnode->location, pnode->AsParseNodeSpecialName(), funcInfo, true);
        }
        else
        {
            byteCodeGenerator->EmitPropLoad(pnode->location, pnode->AsParseNodeName()->sym, pnode->AsParseNodeName()->pid, funcInfo);
        }
        break;

    case knopComma:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        // The parser marks binary opnd pnodes as used, but value of the first opnd of a comma is not used.
        // Easier to correct this here than to check every binary op in the parser.
        ParseNode *pnode1 = pnode->AsParseNodeBin()->pnode1;
        pnode1->isUsed = false;
        if (pnode1->nop == knopComma)
        {
            // Spot fix for giant comma expressions that send us into OOS if we use a simple recursive
            // algorithm. Instead of recursing on comma LHS's, iterate over them, pushing the RHS's onto
            // a stack. (This suggests a model for removing recursion from Emit altogether...)
            ArenaAllocator *alloc = byteCodeGenerator->GetAllocator();
            SList<ParseNode *> rhsStack(alloc);
            do
            {
                rhsStack.Push(pnode1->AsParseNodeBin()->pnode2);
                pnode1 = pnode1->AsParseNodeBin()->pnode1;
                pnode1->isUsed = false;
            } while (pnode1->nop == knopComma);

            Emit(pnode1, byteCodeGenerator, funcInfo, false);
            if (funcInfo->IsTmpReg(pnode1->location))
            {
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::Unused, pnode1->location);
            }

            while (!rhsStack.Empty())
            {
                ParseNode *pnodeRhs = rhsStack.Pop();
                pnodeRhs->isUsed = false;
                Emit(pnodeRhs, byteCodeGenerator, funcInfo, false);
                if (funcInfo->IsTmpReg(pnodeRhs->location))
                {
                    byteCodeGenerator->Writer()->Reg1(Js::OpCode::Unused, pnodeRhs->location);
                }
                funcInfo->ReleaseLoc(pnodeRhs);
            }
        }
        else
        {
            Emit(pnode1, byteCodeGenerator, funcInfo, false);
            if (funcInfo->IsTmpReg(pnode1->location))
            {
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::Unused, pnode1->location);
            }
        }
        funcInfo->ReleaseLoc(pnode1);

        pnode->AsParseNodeBin()->pnode2->isUsed = pnode->isUsed || fReturnValue;
        Emit(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo, false);
        funcInfo->ReleaseLoc(pnode->AsParseNodeBin()->pnode2);
        funcInfo->AcquireLoc(pnode);
        if (pnode->AsParseNodeBin()->pnode2->isUsed && pnode->location != pnode->AsParseNodeBin()->pnode2->location)
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeBin()->pnode2->location);
        }
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
    }
    break;

    // The binary logical ops && and || resolve to the value of the left-hand expression if its
    // boolean value short-circuits the operation, and to the value of the right-hand expression
    // otherwise. (In other words, the "truth" of the right-hand expression is never tested.)
    // PTNODE(knopLogOr      , "||"        ,None    ,Bin  ,fnopBin)
    case knopLogOr:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Js::ByteCodeLabel doneLabel = byteCodeGenerator->Writer()->DefineLabel();
        // We use a single dest here for the whole generating boolean expr, because we were poorly
        // optimizing the previous version where we had a dest for each level
        funcInfo->AcquireLoc(pnode);
        EmitGeneratingBooleanExpression(pnode, doneLabel, true, doneLabel, true, pnode->location, byteCodeGenerator, funcInfo);
        byteCodeGenerator->Writer()->MarkLabel(doneLabel);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    }
    // PTNODE(knopLogAnd     , "&&"        ,None    ,Bin  ,fnopBin)
    case knopLogAnd:
    {
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        Js::ByteCodeLabel doneLabel = byteCodeGenerator->Writer()->DefineLabel();
        // We use a single dest here for the whole generating boolean expr, because we were poorly
        // optimizing the previous version where we had a dest for each level
        funcInfo->AcquireLoc(pnode);
        EmitGeneratingBooleanExpression(pnode, doneLabel, true, doneLabel, true, pnode->location, byteCodeGenerator, funcInfo);
        byteCodeGenerator->Writer()->MarkLabel(doneLabel);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    }
    // PTNODE(knopQmark      , "?"            ,None    ,Tri  ,fnopBin)
    case knopQmark:
    {
        Js::ByteCodeLabel trueLabel = byteCodeGenerator->Writer()->DefineLabel();
        Js::ByteCodeLabel falseLabel = byteCodeGenerator->Writer()->DefineLabel();
        Js::ByteCodeLabel skipLabel = byteCodeGenerator->Writer()->DefineLabel();
        EmitBooleanExpression(pnode->AsParseNodeTri()->pnode1, trueLabel, falseLabel, byteCodeGenerator, funcInfo, true, false);
        byteCodeGenerator->Writer()->MarkLabel(trueLabel);
        funcInfo->ReleaseLoc(pnode->AsParseNodeTri()->pnode1);

        // For boolean expressions that compute a result, we have to burn a register for the result
        // so that the back end can identify it cheaply as a single temp lifetime. Revisit this if we do
        // full-on renaming in the back end.
        funcInfo->AcquireLoc(pnode);

        Emit(pnode->AsParseNodeTri()->pnode2, byteCodeGenerator, funcInfo, false);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeTri()->pnode2->location);
        funcInfo->ReleaseLoc(pnode->AsParseNodeTri()->pnode2);

        // Record the branch bytecode offset
        byteCodeGenerator->Writer()->RecordStatementAdjustment(Js::FunctionBody::SAT_FromCurrentToNext);

        byteCodeGenerator->Writer()->Br(skipLabel);

        byteCodeGenerator->Writer()->MarkLabel(falseLabel);
        Emit(pnode->AsParseNodeTri()->pnode3, byteCodeGenerator, funcInfo, false);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeTri()->pnode3->location);
        funcInfo->ReleaseLoc(pnode->AsParseNodeTri()->pnode3);

        byteCodeGenerator->Writer()->MarkLabel(skipLabel);

        break;
    }

    case knopAsgAdd:
    case knopAsgSub:
    case knopAsgMul:
    case knopAsgDiv:
    case knopAsgExpo:
    case knopAsgMod:
    case knopAsgAnd:
    case knopAsgXor:
    case knopAsgOr:
    case knopAsgLsh:
    case knopAsgRsh:
    case knopAsgRs2:
    {
        byteCodeGenerator->StartStatement(pnode);

        ParseNode *lhs = pnode->AsParseNodeBin()->pnode1;
        ParseNode *rhs = pnode->AsParseNodeBin()->pnode2;

        // Assign a register for the result only if the result is used or the LHS can't be assigned to
        // (i.e., is a constant).
        const bool need_result_location =
            pnode->isUsed
            || fReturnValue
            || funcInfo->RegIsConst(lhs->location)
            || (lhs->nop == knopName && lhs->AsParseNodeName()->sym && lhs->AsParseNodeName()->sym->GetIsFuncExpr());

        if (need_result_location)
        {
            const Js::RegSlot result_location = funcInfo->AcquireLoc(pnode);

            // Grab a register for the initial value and load it.
            EmitBinaryReference(lhs, rhs, byteCodeGenerator, funcInfo, true);
            funcInfo->ReleaseLoc(rhs);
            // Do the arithmetic, store the result, and release the l-value.
            byteCodeGenerator->Writer()->Reg3(nopToOp[pnode->nop], result_location, lhs->location, rhs->location);

            EmitAssignment(pnode, lhs, result_location, byteCodeGenerator, funcInfo);
        }
        else
        {
            // Grab a register for the initial value and load it. Might modify lhs->location.
            EmitBinaryReference(lhs, rhs, byteCodeGenerator, funcInfo, true);

            funcInfo->ReleaseLoc(rhs);
            // Do the arithmetic, store the result, and release the l-value.
            byteCodeGenerator->Writer()->Reg3(nopToOp[pnode->nop], lhs->location, lhs->location, rhs->location);
            EmitAssignment(nullptr, lhs, lhs->location, byteCodeGenerator, funcInfo);
        }
        funcInfo->ReleaseLoad(lhs);

        byteCodeGenerator->EndStatement(pnode);
        break;
    }
        // General nodes.
        // PTNODE(knopTempRef      , "temp ref"  ,None   ,Uni ,fnopUni)
    case knopTempRef:
        // TODO: check whether mov is necessary
        funcInfo->AcquireLoc(pnode);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeUni()->pnode1->location);
        break;
        // PTNODE(knopTemp      , "temp"        ,None   ,None ,fnopLeaf)
    case knopTemp:
        // Emit initialization code
        if (pnode->AsParseNodeVar()->pnodeInit != nullptr)
        {
            byteCodeGenerator->StartStatement(pnode);
            Emit(pnode->AsParseNodeVar()->pnodeInit, byteCodeGenerator, funcInfo, false);
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, pnode->location, pnode->AsParseNodeVar()->pnodeInit->location);
            funcInfo->ReleaseLoc(pnode->AsParseNodeVar()->pnodeInit);
            byteCodeGenerator->EndStatement(pnode);
        }
        break;
        // PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
    case knopVarDecl:
    case knopConstDecl:
    case knopLetDecl:
    {
        // Emit initialization code
        ParseNodePtr initNode = pnode->AsParseNodeVar()->pnodeInit;
        AssertMsg(pnode->nop != knopConstDecl || initNode != nullptr, "knopConstDecl expected to have an initializer");

        if (initNode != nullptr || pnode->nop == knopLetDecl)
        {
            Symbol *sym = pnode->AsParseNodeVar()->sym;
            Js::RegSlot rhsLocation;

            byteCodeGenerator->StartStatement(pnode);

            if (initNode != nullptr)
            {
                Emit(initNode, byteCodeGenerator, funcInfo, false);
                rhsLocation = initNode->location;

                if (initNode->nop == knopObject)
                {
                    TrackMemberNodesInObjectForIntConstants(byteCodeGenerator, initNode);
                }
                else if (initNode->nop == knopInt)
                {
                    TrackIntConstantsOnGlobalObject(byteCodeGenerator, sym);
                }
            }
            else
            {
                Assert(pnode->nop == knopLetDecl);
                rhsLocation = funcInfo->AcquireTmpRegister();
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, rhsLocation);
            }

            if (pnode->nop != knopVarDecl)
            {
                Assert(sym->GetDecl() == pnode || (sym->IsArguments() && !funcInfo->GetHasArguments()));
                sym->SetNeedDeclaration(false);
            }

            EmitAssignment(nullptr, pnode, rhsLocation, byteCodeGenerator, funcInfo);
            funcInfo->ReleaseTmpRegister(rhsLocation);

            byteCodeGenerator->EndStatement(pnode);
        }
        break;
    }
    // PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
    case knopFncDecl:
        // The "function declarations" were emitted in DefineFunctions()
        if (!pnode->AsParseNodeFnc()->IsDeclaration())
        {
            byteCodeGenerator->DefineOneFunction(pnode->AsParseNodeFnc(), funcInfo, false);
        }
        break;
        // PTNODE(knopClassDecl, "class"    ,None    ,None ,fnopLeaf)
    case knopClassDecl:
    {
        ParseNodeClass * pnodeClass = pnode->AsParseNodeClass();
        funcInfo->AcquireLoc(pnodeClass);

        Assert(pnodeClass->pnodeConstructor);
        pnodeClass->pnodeConstructor->location = pnodeClass->location;

        BeginEmitBlock(pnodeClass->pnodeBlock, byteCodeGenerator, funcInfo);

        // Extends
        if (pnodeClass->pnodeExtends)
        {
            // We can't do StartStatement/EndStatement for pnodeExtends here because the load locations may differ between
            // defer and nondefer parse modes.
            Emit(pnodeClass->pnodeExtends, byteCodeGenerator, funcInfo, false);
        }

        // Constructor
        Emit(pnodeClass->pnodeConstructor, byteCodeGenerator, funcInfo, false);

        if (bindingNameLocation != Js::Constants::NoRegister && !pnodeClass->pnodeConstructor->pnodeName)
        {
            Assert(pnodeClass->pnodeConstructor->HasComputedName());
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::SetComputedNameVar, pnodeClass->pnodeConstructor->location, bindingNameLocation);
        }

        if (pnodeClass->pnodeExtends)
        {
            byteCodeGenerator->StartStatement(pnodeClass->pnodeExtends);
            byteCodeGenerator->Writer()->InitClass(pnodeClass->location, pnodeClass->pnodeExtends->location);
            byteCodeGenerator->EndStatement(pnodeClass->pnodeExtends);
        }
        else
        {
            byteCodeGenerator->Writer()->InitClass(pnodeClass->location);
        }

        Js::RegSlot protoLoc = funcInfo->AcquireTmpRegister(); //register set if we have Instance Methods
        int cacheId = funcInfo->FindOrAddInlineCacheId(pnodeClass->location, Js::PropertyIds::prototype, false, false);
        byteCodeGenerator->Writer()->PatchableProperty(Js::OpCode::LdFld, protoLoc, pnodeClass->location, cacheId);

        // Static Methods
        EmitClassInitializers(pnodeClass->pnodeStaticMembers, pnodeClass->location, byteCodeGenerator, funcInfo, pnode, /*isObjectEmpty*/ false);

        // Instance Methods
        EmitClassInitializers(pnodeClass->pnodeMembers, protoLoc, byteCodeGenerator, funcInfo, pnode, /*isObjectEmpty*/ true);
        funcInfo->ReleaseTmpRegister(protoLoc);

        // Emit name binding.
        if (pnodeClass->pnodeName)
        {
            Symbol * sym = pnodeClass->pnodeName->sym;
            sym->SetNeedDeclaration(false);
            byteCodeGenerator->EmitPropStore(pnodeClass->location, sym, nullptr, funcInfo, false, true);
        }

        EndEmitBlock(pnodeClass->pnodeBlock, byteCodeGenerator, funcInfo);

        if (pnodeClass->pnodeExtends)
        {
            funcInfo->ReleaseLoc(pnodeClass->pnodeExtends);
        }

        if (pnodeClass->pnodeDeclName)
        {
            Symbol * sym = pnodeClass->pnodeDeclName->sym;
            sym->SetNeedDeclaration(false);
            byteCodeGenerator->EmitPropStore(pnodeClass->location, sym, nullptr, funcInfo, true, false);
        }

        if (pnodeClass->IsDefaultModuleExport())
        {
            byteCodeGenerator->EmitAssignmentToDefaultModuleExport(pnodeClass, funcInfo);
        }

        break;
    }
    case knopStrTemplate:
        STARTSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        EmitStringTemplate(pnode->AsParseNodeStrTemplate(), byteCodeGenerator, funcInfo);
        ENDSTATEMENET_IFTOPLEVEL(isTopLevel, pnode);
        break;
    case knopEndCode:
        byteCodeGenerator->Writer()->RecordStatementAdjustment(Js::FunctionBody::SAT_All);

        // load undefined for the fallthrough case:
        if (!funcInfo->IsGlobalFunction())
        {
            if (funcInfo->IsClassConstructor())
            {
                // For class constructors, we need to explicitly load 'this' into the return register.
                byteCodeGenerator->EmitClassConstructorEndCode(funcInfo);
            }
            else
            {
                // In the global function, implicit return values are copied to the return register, and if
                // necessary the return register is initialized at the top. Don't clobber the value here.
                byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, ByteCodeGenerator::ReturnRegister);
            }
        }

        // Label for non-fall-through return
        byteCodeGenerator->Writer()->MarkLabel(funcInfo->singleExit);

        if (funcInfo->GetHasCachedScope())
        {
            byteCodeGenerator->Writer()->Empty(Js::OpCode::CommitScope);
        }
        byteCodeGenerator->StartStatement(pnode);
        byteCodeGenerator->Writer()->Empty(Js::OpCode::Ret);
        byteCodeGenerator->EndStatement(pnode);
        break;
        // PTNODE(knopDebugger   , "debugger"    ,None    ,None ,fnopNone)
    case knopDebugger:
        byteCodeGenerator->StartStatement(pnode);
        byteCodeGenerator->Writer()->Empty(Js::OpCode::Break);
        byteCodeGenerator->EndStatement(pnode);
        break;
        // PTNODE(knopFor        , "for"        ,None    ,For  ,fnopBreak|fnopContinue)
    case knopFor:
    {
        ParseNodeFor * pnodeFor = pnode->AsParseNodeFor();
        if (pnodeFor->pnodeInverted != nullptr)
        {
            byteCodeGenerator->EmitInvertedLoop(pnodeFor, pnodeFor->pnodeInverted, funcInfo);
        }
        else
        {
            BeginEmitBlock(pnodeFor->pnodeBlock, byteCodeGenerator, funcInfo);
            Emit(pnodeFor->pnodeInit, byteCodeGenerator, funcInfo, false);
            funcInfo->ReleaseLoc(pnodeFor->pnodeInit);
            if (byteCodeGenerator->IsES6ForLoopSemanticsEnabled())
            {
                CloneEmitBlock(pnodeFor->pnodeBlock, byteCodeGenerator, funcInfo);
            }
            EmitLoop(pnodeFor,
                pnodeFor->pnodeCond,
                pnodeFor->pnodeBody,
                pnodeFor->pnodeIncr,
                byteCodeGenerator,
                funcInfo,
                fReturnValue,
                FALSE,
                pnodeFor->pnodeBlock);
            EndEmitBlock(pnodeFor->pnodeBlock, byteCodeGenerator, funcInfo);
        }
        break;
    }
        // PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
    case knopIf:
    {
        ParseNodeIf * pnodeIf = pnode->AsParseNodeIf();
        byteCodeGenerator->StartStatement(pnodeIf);

        Js::ByteCodeLabel trueLabel = byteCodeGenerator->Writer()->DefineLabel();
        Js::ByteCodeLabel falseLabel = byteCodeGenerator->Writer()->DefineLabel();
        EmitBooleanExpression(pnodeIf->pnodeCond, trueLabel, falseLabel, byteCodeGenerator, funcInfo, true, false);
        funcInfo->ReleaseLoc(pnodeIf->pnodeCond);

        byteCodeGenerator->EndStatement(pnodeIf);

        byteCodeGenerator->Writer()->MarkLabel(trueLabel);
        Emit(pnodeIf->pnodeTrue, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(pnodeIf->pnodeTrue);
        if (pnodeIf->pnodeFalse != nullptr)
        {
            // has else clause
            Js::ByteCodeLabel skipLabel = byteCodeGenerator->Writer()->DefineLabel();

            // Record the branch bytecode offset
            byteCodeGenerator->Writer()->RecordStatementAdjustment(Js::FunctionBody::SAT_FromCurrentToNext);

            // then clause skips else clause
            byteCodeGenerator->Writer()->Br(skipLabel);
            // generate code for else clause
            byteCodeGenerator->Writer()->MarkLabel(falseLabel);
            Emit(pnodeIf->pnodeFalse, byteCodeGenerator, funcInfo, fReturnValue);
            funcInfo->ReleaseLoc(pnodeIf->pnodeFalse);
            byteCodeGenerator->Writer()->MarkLabel(skipLabel);
        }
        else
        {
            byteCodeGenerator->Writer()->MarkLabel(falseLabel);
        }

        if (pnodeIf->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(pnodeIf->breakLabel);
        }
        break;
    }
    case knopWhile:
    {
        ParseNodeWhile * pnodeWhile = pnode->AsParseNodeWhile();
        EmitLoop(pnodeWhile,
            pnodeWhile->pnodeCond,
            pnodeWhile->pnodeBody,
            nullptr,
            byteCodeGenerator,
            funcInfo,
            fReturnValue);
        break;
    }
        // PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
    case knopDoWhile:
    {
        ParseNodeWhile * pnodeWhile = pnode->AsParseNodeWhile();
        EmitLoop(pnodeWhile,
            pnodeWhile->pnodeCond,
            pnodeWhile->pnodeBody,
            nullptr,
            byteCodeGenerator,
            funcInfo,
            fReturnValue,
            true);
        break;
    }
        // PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
    case knopForIn:
        EmitForInOrForOf(pnode->AsParseNodeForInOrForOf(), byteCodeGenerator, funcInfo, fReturnValue);
        break;
    case knopForOf:
        EmitForInOrForOf(pnode->AsParseNodeForInOrForOf(), byteCodeGenerator, funcInfo, fReturnValue);
        break;
        // PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
    case knopReturn:
        {
        ParseNodeReturn * pnodeReturn = pnode->AsParseNodeReturn();
        byteCodeGenerator->StartStatement(pnodeReturn);
        if (pnodeReturn->pnodeExpr != nullptr)
            {
            if (pnodeReturn->pnodeExpr->location == Js::Constants::NoRegister)
            {
                // No need to burn a register for the return value. If we need a temp, use R0 directly.
                pnodeReturn->pnodeExpr->location = ByteCodeGenerator::ReturnRegister;
            }
            Emit(pnodeReturn->pnodeExpr, byteCodeGenerator, funcInfo, fReturnValue);
            if (pnodeReturn->pnodeExpr->location != ByteCodeGenerator::ReturnRegister)
            {
                byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, ByteCodeGenerator::ReturnRegister, pnodeReturn->pnodeExpr->location);
            }
            funcInfo->GetParsedFunctionBody()->SetHasNoExplicitReturnValue(false);
        }
        else
        {
            byteCodeGenerator->Writer()->Reg1(Js::OpCode::LdUndef, ByteCodeGenerator::ReturnRegister);
        }
        if (funcInfo->IsClassConstructor())
        {
            // return expr; // becomes like below:
            //
            // if (IsObject(expr)) {
            //   return expr;
            // } else if (IsBaseClassConstructor) {
            //   return this;
            // } else if (!IsUndefined(expr)) {
            //   throw TypeError;
            // }

            Js::ByteCodeLabel returnExprLabel = byteCodeGenerator->Writer()->DefineLabel();
            byteCodeGenerator->Writer()->BrReg1(Js::OpCode::BrOnObject_A, returnExprLabel, ByteCodeGenerator::ReturnRegister);

            if (funcInfo->IsBaseClassConstructor())
            {
                byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, ByteCodeGenerator::ReturnRegister, funcInfo->GetThisSymbol()->GetLocation());
            }
            else
            {
                Js::ByteCodeLabel returnThisLabel = byteCodeGenerator->Writer()->DefineLabel();
                byteCodeGenerator->Writer()->BrReg2(Js::OpCode::BrSrEq_A, returnThisLabel, ByteCodeGenerator::ReturnRegister, funcInfo->undefinedConstantRegister);
                byteCodeGenerator->Writer()->W1(Js::OpCode::RuntimeTypeError, SCODE_CODE(JSERR_ClassDerivedConstructorInvalidReturnType));
                byteCodeGenerator->Writer()->MarkLabel(returnThisLabel);
                byteCodeGenerator->EmitClassConstructorEndCode(funcInfo);
            }

            byteCodeGenerator->Writer()->MarkLabel(returnExprLabel);
        }
        if (pnodeReturn->grfnop & fnopCleanup)
        {
            EmitJumpCleanup(pnodeReturn, nullptr, byteCodeGenerator, funcInfo);
        }

        byteCodeGenerator->Writer()->Br(funcInfo->singleExit);
        byteCodeGenerator->EndStatement(pnodeReturn);
        break;
    }
        // PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
    case knopBlock:
    {
        ParseNodeBlock * pnodeBlock = pnode->AsParseNodeBlock();

        if (pnodeBlock->pnodeStmt != nullptr)
        {
            EmitBlock(pnodeBlock, byteCodeGenerator, funcInfo, fReturnValue);
            if (pnodeBlock->emitLabels)
            {
                byteCodeGenerator->Writer()->MarkLabel(pnodeBlock->breakLabel);
            }
        }
        break;
    }
        // PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
    case knopWith:
    {
        ParseNodeWith * pnodeWith = pnode->AsParseNodeWith();
        Assert(pnodeWith->pnodeObj != nullptr);
        byteCodeGenerator->StartStatement(pnodeWith);
        // Copy the with object to a temp register (the location assigned to pnode) so that if the with object
        // is overwritten in the body, the lookups are not affected.
        funcInfo->AcquireLoc(pnodeWith);
        Emit(pnodeWith->pnodeObj, byteCodeGenerator, funcInfo, false);

        Js::RegSlot regVal = (byteCodeGenerator->GetScriptContext()->GetConfig()->IsES6UnscopablesEnabled()) ? funcInfo->AcquireTmpRegister() : pnodeWith->location;
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Conv_Obj, regVal, pnodeWith->pnodeObj->location);
        if (byteCodeGenerator->GetScriptContext()->GetConfig()->IsES6UnscopablesEnabled())
        {
            byteCodeGenerator->Writer()->Reg2(Js::OpCode::NewUnscopablesWrapperObject, pnodeWith->location, regVal);
        }
        byteCodeGenerator->EndStatement(pnodeWith);

#ifdef PERF_HINT
        if (PHASE_TRACE1(Js::PerfHintPhase))
        {
            WritePerfHint(PerfHints::HasWithBlock, funcInfo->byteCodeFunction->GetFunctionBody(), byteCodeGenerator->Writer()->GetCurrentOffset() - 1);
        }
#endif
        if (pnodeWith->pnodeBody != nullptr)
        {
            Scope *scope = pnodeWith->scope;
            scope->SetLocation(pnodeWith->location);
            byteCodeGenerator->PushScope(scope);

            Js::DebuggerScope *debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeWith, Js::DiagExtraScopesType::DiagWithScope, regVal);

            if (byteCodeGenerator->ShouldTrackDebuggerMetadata())
            {
                byteCodeGenerator->Writer()->AddPropertyToDebuggerScope(debuggerScope, regVal, Js::Constants::NoProperty, /*shouldConsumeRegister*/ true, Js::DebuggerScopePropertyFlags_WithObject);
            }

            Emit(pnodeWith->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
            funcInfo->ReleaseLoc(pnodeWith->pnodeBody);
            byteCodeGenerator->PopScope();

            byteCodeGenerator->RecordEndScopeObject(pnodeWith);
        }
        if (pnodeWith->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(pnodeWith->breakLabel);
        }
        if (byteCodeGenerator->GetScriptContext()->GetConfig()->IsES6UnscopablesEnabled())
        {
            funcInfo->ReleaseTmpRegister(regVal);
        }
        funcInfo->ReleaseLoc(pnodeWith->pnodeObj);
        break;
    }
    // PTNODE(knopBreak      , "break"        ,None    ,Jump ,fnopNone)
    case knopBreak:
    {
        ParseNodeJump * pnodeJump = pnode->AsParseNodeJump();
        Assert(pnodeJump->pnodeTarget->emitLabels);
        byteCodeGenerator->StartStatement(pnodeJump);
        if (pnodeJump->grfnop & fnopCleanup)
        {
            EmitJumpCleanup(pnodeJump, pnodeJump->pnodeTarget, byteCodeGenerator, funcInfo);
        }
        byteCodeGenerator->Writer()->Br(pnodeJump->pnodeTarget->breakLabel);
        if (pnodeJump->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(pnodeJump->breakLabel);
        }
        byteCodeGenerator->EndStatement(pnodeJump);
        break;
    }
    case knopContinue:
        {
        ParseNodeJump * pnodeJump = pnode->AsParseNodeJump();
        Assert(pnodeJump->pnodeTarget->emitLabels);
        byteCodeGenerator->StartStatement(pnodeJump);
        if (pnodeJump->grfnop & fnopCleanup)
        {
            EmitJumpCleanup(pnodeJump, pnodeJump->pnodeTarget, byteCodeGenerator, funcInfo);
        }
        byteCodeGenerator->Writer()->Br(pnodeJump->pnodeTarget->continueLabel);
        byteCodeGenerator->EndStatement(pnodeJump);
        break;
    }
        // PTNODE(knopContinue   , "continue"    ,None    ,Jump ,fnopNone)
    case knopSwitch:
    {
        ParseNodeSwitch * pnodeSwitch = pnode->AsParseNodeSwitch();
        BOOL fHasDefault = false;
        Assert(pnodeSwitch->pnodeVal != nullptr);
        byteCodeGenerator->StartStatement(pnodeSwitch);
        Emit(pnodeSwitch->pnodeVal, byteCodeGenerator, funcInfo, false);

        Js::RegSlot regVal = funcInfo->AcquireTmpRegister();

        byteCodeGenerator->Writer()->Reg2(Js::OpCode::BeginSwitch, regVal, pnodeSwitch->pnodeVal->location);

        BeginEmitBlock(pnodeSwitch->pnodeBlock, byteCodeGenerator, funcInfo);

        byteCodeGenerator->EndStatement(pnodeSwitch);

        // TODO: if all cases are compile-time constants, emit a switch statement in the byte
        // code so the BE can optimize it.

        ParseNodeCase *pnodeCase;
        for (pnodeCase = pnodeSwitch->pnodeCases; pnodeCase; pnodeCase = pnodeCase->pnodeNext)
        {
            // Jump to the first case body if this one doesn't match. Make sure any side-effects of the case
            // expression take place regardless.
            pnodeCase->labelCase = byteCodeGenerator->Writer()->DefineLabel();
            if (pnodeCase == pnodeSwitch->pnodeDefault)
            {
                fHasDefault = true;
                continue;
            }
            Emit(pnodeCase->pnodeExpr, byteCodeGenerator, funcInfo, false);
            byteCodeGenerator->Writer()->BrReg2(
                Js::OpCode::Case, pnodeCase->labelCase, regVal, pnodeCase->pnodeExpr->location);
            funcInfo->ReleaseLoc(pnodeCase->pnodeExpr);
        }

        // No explicit case value matches. Jump to the default arm (if any) or break out altogether.
        if (fHasDefault)
        {
            byteCodeGenerator->Writer()->Br(Js::OpCode::EndSwitch, pnodeSwitch->pnodeDefault->labelCase);
        }
        else
        {
            if (!pnodeSwitch->emitLabels)
            {
                pnodeSwitch->breakLabel = byteCodeGenerator->Writer()->DefineLabel();
            }
            byteCodeGenerator->Writer()->Br(Js::OpCode::EndSwitch, pnodeSwitch->breakLabel);
        }
        // Now emit the case arms to which we jump on matching a case value.
        for (pnodeCase = pnodeSwitch->pnodeCases; pnodeCase; pnodeCase = pnodeCase->pnodeNext)
        {
            byteCodeGenerator->Writer()->MarkLabel(pnodeCase->labelCase);
            Emit(pnodeCase->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
            funcInfo->ReleaseLoc(pnodeCase->pnodeBody);
        }

        EndEmitBlock(pnodeSwitch->pnodeBlock, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseTmpRegister(regVal);
        funcInfo->ReleaseLoc(pnodeSwitch->pnodeVal);

        if (!fHasDefault || pnodeSwitch->emitLabels)
        {
            byteCodeGenerator->Writer()->MarkLabel(pnodeSwitch->breakLabel);
        }
        break;
    }

    case knopTryCatch:
    {
        Js::ByteCodeLabel catchLabel = (Js::ByteCodeLabel) - 1;

        ParseNodeTryCatch * pnodeTryCatch = pnode->AsParseNodeTryCatch();
        ParseNodeTry *pnodeTry = pnodeTryCatch->pnodeTry;
        Assert(pnodeTry);
        ParseNodeCatch *pnodeCatch = pnodeTryCatch->pnodeCatch;
        Assert(pnodeCatch);

        catchLabel = byteCodeGenerator->Writer()->DefineLabel();

        // Note: try uses OpCode::Leave which causes a return to parent interpreter thunk,
        // same for catch block. Thus record cross interpreter frame entry/exit records for them.
        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ true);

        byteCodeGenerator->Writer()->Br(Js::OpCode::TryCatch, catchLabel);

        ByteCodeGenerator::TryScopeRecord tryRecForTry(Js::OpCode::TryCatch, catchLabel);
        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTry);
        }

        Emit(pnodeTry->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(pnodeTry->pnodeBody);

        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
        }

        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(/* isEnterBlock = */ false);

        byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
        byteCodeGenerator->Writer()->Br(pnodeTryCatch->breakLabel);
        byteCodeGenerator->Writer()->MarkLabel(catchLabel);
        
        ParseNode *pnodeObj = pnodeCatch->GetParam();
        Assert(pnodeObj);

        Js::RegSlot location;

        bool acquiredTempLocation = false;

        Js::DebuggerScope *debuggerScope = nullptr;
        Js::DebuggerScopePropertyFlags debuggerPropertyFlags = Js::DebuggerScopePropertyFlags_CatchObject;

        bool isPattern = pnodeObj->nop == knopParamPattern;

        if (isPattern)
        {
            location = pnodeObj->AsParseNodeParamPattern()->location;
        }
        else
        {
            location = pnodeObj->AsParseNodeName()->sym->GetLocation();
        }

        if (location == Js::Constants::NoRegister)
        {
            location = funcInfo->AcquireLoc(pnodeObj);
            acquiredTempLocation = true;
        }
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::Catch, location);

        Scope *scope = pnodeCatch->scope;
        byteCodeGenerator->PushScope(scope);

        if (scope->GetMustInstantiate())
        {
            Assert(scope->GetLocation() == Js::Constants::NoRegister);
            if (scope->GetIsObject())
            {
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeTryCatch, Js::DiagCatchScopeInObject, funcInfo->InnerScopeToRegSlot(scope));
                byteCodeGenerator->Writer()->Unsigned1(Js::OpCode::NewPseudoScope, scope->GetInnerScopeIndex());
            }
            else
            {

                int index = Js::DebuggerScope::InvalidScopeIndex;
                debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeTryCatch, Js::DiagCatchScopeInSlot, funcInfo->InnerScopeToRegSlot(scope), &index);
                byteCodeGenerator->Writer()->Num3(Js::OpCode::NewInnerScopeSlots, scope->GetInnerScopeIndex(), scope->GetScopeSlotCount() + Js::ScopeSlots::FirstSlotIndex, index);
            }
        }
        else
        {
            debuggerScope = byteCodeGenerator->RecordStartScopeObject(pnodeTryCatch, Js::DiagCatchScopeDirect, location);
        }

        auto ParamTrackAndInitialization = [&](Symbol *sym, bool initializeParam, Js::RegSlot location)
        {
            if (sym->IsInSlot(byteCodeGenerator, funcInfo))
            {
                Assert(scope->GetMustInstantiate());
                if (scope->GetIsObject())
                {
                    Js::OpCode op = (sym->GetDecl()->nop == knopLetDecl) ? Js::OpCode::InitUndeclLetFld :
                        byteCodeGenerator->GetInitFldOp(scope, scope->GetLocation(), funcInfo, false);

                    Js::PropertyId propertyId = sym->EnsurePosition(byteCodeGenerator);
                    uint cacheId = funcInfo->FindOrAddInlineCacheId(funcInfo->InnerScopeToRegSlot(scope), propertyId, false, true);
                    byteCodeGenerator->Writer()->ElementPIndexed(op, location, scope->GetInnerScopeIndex(), cacheId);

                    byteCodeGenerator->TrackActivationObjectPropertyForDebugger(debuggerScope, sym, debuggerPropertyFlags);
                }
                else
                {
                    byteCodeGenerator->TrackSlotArrayPropertyForDebugger(debuggerScope, sym, sym->EnsurePosition(byteCodeGenerator), debuggerPropertyFlags);
                    if (initializeParam)
                    {
                        byteCodeGenerator->EmitLocalPropInit(location, sym, funcInfo);
                    }
                    else
                    {
                        Js::RegSlot tmpReg = funcInfo->AcquireTmpRegister();
                        byteCodeGenerator->Writer()->Reg1(Js::OpCode::InitUndecl, tmpReg);
                        byteCodeGenerator->EmitLocalPropInit(tmpReg, sym, funcInfo);
                        funcInfo->ReleaseTmpRegister(tmpReg);
                    }
                }
            }
            else
            {
                byteCodeGenerator->TrackRegisterPropertyForDebugger(debuggerScope, sym, funcInfo, debuggerPropertyFlags);
                if (initializeParam)
                {
                    byteCodeGenerator->EmitLocalPropInit(location, sym, funcInfo);
                }
                else
                {
                    byteCodeGenerator->Writer()->Reg1(Js::OpCode::InitUndecl, location);
                }
            }
        };

        ByteCodeGenerator::TryScopeRecord tryRecForCatch(Js::OpCode::ResumeCatch, catchLabel);
        if (isPattern)
        {
            Parser::MapBindIdentifier(pnodeObj->AsParseNodeParamPattern()->pnode1, [&](ParseNodePtr item)
            {
                Js::RegSlot itemLocation = item->AsParseNodeVar()->sym->GetLocation();
                if (itemLocation == Js::Constants::NoRegister)
                {
                    // The var has no assigned register, meaning it's captured, so we have no reg to write to.
                    // Emit the designated return reg in the byte code to avoid asserting on bad register.
                    itemLocation = ByteCodeGenerator::ReturnRegister;
                }
                ParamTrackAndInitialization(item->AsParseNodeVar()->sym, false /*initializeParam*/, itemLocation);
            });
            byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

            // Now emitting bytecode for destructuring pattern
            byteCodeGenerator->StartStatement(pnodeCatch);
            ParseNodePtr pnode1 = pnodeObj->AsParseNodeParamPattern()->pnode1;
            Assert(pnode1->IsPattern());

            if (funcInfo->byteCodeFunction->IsCoroutine())
            {
                byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForCatch);
            }

            EmitAssignment(nullptr, pnode1, location, byteCodeGenerator, funcInfo);
            byteCodeGenerator->EndStatement(pnodeCatch);
        }
        else
        {
            ParamTrackAndInitialization(pnodeObj->AsParseNodeName()->sym, true /*initializeParam*/, location);
            if (scope->GetMustInstantiate())
            {
                pnodeObj->AsParseNodeName()->sym->SetIsGlobalCatch(true);
            }
            byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

            // Allow a debugger to stop on the 'catch (e)'
            byteCodeGenerator->StartStatement(pnodeCatch);
            byteCodeGenerator->Writer()->Empty(Js::OpCode::Nop);
            byteCodeGenerator->EndStatement(pnodeCatch);
            if (funcInfo->byteCodeFunction->IsCoroutine())
            {
                byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForCatch);
            }
        }

        Emit(pnodeCatch->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);

        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
        }

        byteCodeGenerator->PopScope();

        byteCodeGenerator->RecordEndScopeObject(pnodeTryCatch);

        funcInfo->ReleaseLoc(pnodeCatch->pnodeBody);

        if (acquiredTempLocation)
        {
            funcInfo->ReleaseLoc(pnodeObj);
        }

        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(false);

        byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
        byteCodeGenerator->Writer()->MarkLabel(pnodeTryCatch->breakLabel);
        break;
    }

    case knopTryFinally:
    {
        Js::ByteCodeLabel finallyLabel = (Js::ByteCodeLabel) - 1;

        ParseNodeTryFinally * pnodeTryFinally = pnode->AsParseNodeTryFinally();
        ParseNodeTry *pnodeTry = pnodeTryFinally->pnodeTry;
        Assert(pnodeTry);
        ParseNodeFinally *pnodeFinally = pnodeTryFinally->pnodeFinally;
        Assert(pnodeFinally);

        // If we yield from the finally block after an exception, we have to store the exception object for the future next call.
        // When we yield from the Try-Finally the offset to the end of the Try block is needed for the branch instruction.
        Js::RegSlot regException = Js::Constants::NoRegister;
        Js::RegSlot regOffset = Js::Constants::NoRegister;

        finallyLabel = byteCodeGenerator->Writer()->DefineLabel();
        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

        // [CONSIDER][aneeshd] Ideally the TryFinallyWithYield opcode needs to be used only if there is a yield expression.
        // For now, if the function is generator we are using the TryFinallyWithYield.
        ByteCodeGenerator::TryScopeRecord tryRecForTry(Js::OpCode::TryFinallyWithYield, finallyLabel);
        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            regException = funcInfo->AcquireTmpRegister();
            regOffset = funcInfo->AcquireTmpRegister();
            byteCodeGenerator->Writer()->BrReg2(Js::OpCode::TryFinallyWithYield, finallyLabel, regException, regOffset);
            tryRecForTry.reg1 = regException;
            tryRecForTry.reg2 = regOffset;
            byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForTry);
        }
        else
        {
            byteCodeGenerator->Writer()->Br(Js::OpCode::TryFinally, finallyLabel);
        }

        // Increasing the stack as we will be storing the additional values when we enter try..finally.
        funcInfo->StartRecordingOutArgs(1);

        Emit(pnodeTry->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(pnodeTry->pnodeBody);

        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
        }

        byteCodeGenerator->Writer()->Empty(Js::OpCode::Leave);
        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(false);

        // Note: although we don't use OpCode::Leave for finally block,
        // OpCode::LeaveNull causes a return to parent interpreter thunk.
        // This has to be on offset prior to offset of 1st statement of finally.
        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(true);

        byteCodeGenerator->Writer()->Br(pnodeTryFinally->breakLabel);
        byteCodeGenerator->Writer()->MarkLabel(finallyLabel);
        byteCodeGenerator->Writer()->Empty(Js::OpCode::Finally);

        ByteCodeGenerator::TryScopeRecord tryRecForFinally(Js::OpCode::ResumeFinally, finallyLabel, regException, regOffset);
        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.LinkToEnd(&tryRecForFinally);
        }

        Emit(pnodeFinally->pnodeBody, byteCodeGenerator, funcInfo, fReturnValue);
        funcInfo->ReleaseLoc(pnodeFinally->pnodeBody);

        if (funcInfo->byteCodeFunction->IsCoroutine())
        {
            byteCodeGenerator->tryScopeRecordsList.UnlinkFromEnd();
            funcInfo->ReleaseTmpRegister(regOffset);
            funcInfo->ReleaseTmpRegister(regException);
        }

        funcInfo->EndRecordingOutArgs(1);

        byteCodeGenerator->Writer()->RecordCrossFrameEntryExitRecord(false);

        byteCodeGenerator->Writer()->Empty(Js::OpCode::LeaveNull);

        byteCodeGenerator->Writer()->MarkLabel(pnodeTryFinally->breakLabel);
        break;
    }
    case knopThrow:
        byteCodeGenerator->StartStatement(pnode);
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        byteCodeGenerator->Writer()->Reg1(Js::OpCode::Throw, pnode->AsParseNodeUni()->pnode1->location);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        byteCodeGenerator->EndStatement(pnode);
        break;
    case knopYieldLeaf:
        byteCodeGenerator->StartStatement(pnode);
        funcInfo->AcquireLoc(pnode);
        EmitYield(funcInfo->undefinedConstantRegister, pnode->location, byteCodeGenerator, funcInfo);
        byteCodeGenerator->EndStatement(pnode);
        break;
    case knopAwait:
    case knopYield:
        byteCodeGenerator->StartStatement(pnode);
        funcInfo->AcquireLoc(pnode);
        Emit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, funcInfo, false);
        EmitYield(pnode->AsParseNodeUni()->pnode1->location, pnode->location, byteCodeGenerator, funcInfo);
        funcInfo->ReleaseLoc(pnode->AsParseNodeUni()->pnode1);
        byteCodeGenerator->EndStatement(pnode);
        break;
    case knopYieldStar:
        byteCodeGenerator->StartStatement(pnode);
        EmitYieldStar(pnode->AsParseNodeUni(), byteCodeGenerator, funcInfo);
        byteCodeGenerator->EndStatement(pnode);
        break;
    case knopExportDefault:
        Emit(pnode->AsParseNodeExportDefault()->pnodeExpr, byteCodeGenerator, funcInfo, false);
        byteCodeGenerator->EmitAssignmentToDefaultModuleExport(pnode->AsParseNodeExportDefault()->pnodeExpr, funcInfo);
        funcInfo->ReleaseLoc(pnode->AsParseNodeExportDefault()->pnodeExpr);
        pnode = pnode->AsParseNodeExportDefault()->pnodeExpr;
        break;
    default:
        AssertMsg(0, "emit unhandled pnode op");
        break;
    }

    if (fReturnValue && IsExpressionStatement(pnode, byteCodeGenerator->GetScriptContext()) && !pnode->IsPatternDeclaration())
    {
        // If this statement may produce the global function's return value, copy its result to the return register.
        // fReturnValue implies global function, which implies that "return" is a parse error.
        Assert(funcInfo->IsGlobalFunction());
        Assert(pnode->nop != knopReturn);
        byteCodeGenerator->Writer()->Reg2(Js::OpCode::Ld_A, ByteCodeGenerator::ReturnRegister, pnode->location);
    }
}
