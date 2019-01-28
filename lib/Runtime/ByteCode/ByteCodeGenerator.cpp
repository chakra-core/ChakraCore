//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"
#include "FormalsUtil.h"
#include "Library/StackScriptFunction.h"

#if DBG
#include "pnodewalk.h"
#endif

void PreVisitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator);
void PostVisitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator);

bool IsCallOfConstants(ParseNode *pnode)
{
    return pnode->AsParseNodeCall()->callOfConstants && pnode->AsParseNodeCall()->argCount > ByteCodeGenerator::MinArgumentsForCallOptimization;
}

template <class PrefixFn, class PostfixFn>
void Visit(ParseNode *pnode, ByteCodeGenerator* byteCodeGenerator, PrefixFn prefix, PostfixFn postfix, ParseNode * pnodeParent = nullptr);

//the only point of this type (as opposed to using a lambda) is to provide a named type in code coverage
template <typename TContext> class ParseNodeVisitor
{
    TContext* m_context;
    void(*m_fn)(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, TContext* context);
public:

    ParseNodeVisitor(TContext* ctx, void(*prefixParam)(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, TContext* context)) :
        m_context(ctx), m_fn(prefixParam)
    {
    }

    void operator () (ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator)
    {
        if (m_fn)
        {
            m_fn(pnode, byteCodeGenerator, m_context);
        }
    }
};

template<class TContext>
void VisitIndirect(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, TContext* context,
    void (*prefix)(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, TContext* context),
    void (*postfix)(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, TContext* context))
{
    ParseNodeVisitor<TContext> prefixHelper(context, prefix);
    ParseNodeVisitor<TContext> postfixHelper(context, postfix);

    Visit(pnode, byteCodeGenerator, prefixHelper, postfixHelper, nullptr);
}

template <class PrefixFn, class PostfixFn>
void VisitList(ParseNode *pnode, ByteCodeGenerator* byteCodeGenerator, PrefixFn prefix, PostfixFn postfix)
{
    Assert(pnode != nullptr);
    Assert(pnode->nop == knopList);

    do
    {
        ParseNode * pnode1 = pnode->AsParseNodeBin()->pnode1;
        Visit(pnode1, byteCodeGenerator, prefix, postfix);
        pnode = pnode->AsParseNodeBin()->pnode2;
    }
    while (pnode->nop == knopList);
    Visit(pnode, byteCodeGenerator, prefix, postfix);
}

template <class PrefixFn, class PostfixFn>
void VisitWithStmt(ParseNode *pnode, Js::RegSlot loc, ByteCodeGenerator* byteCodeGenerator, PrefixFn prefix, PostfixFn postfix, ParseNode *pnodeParent)
{
    // Note the fact that we're visiting the body of a with statement. This allows us to optimize register assignment
    // in the normal case of calls not requiring that their "this" objects be found dynamically.
    Scope *scope = pnode->AsParseNodeWith()->scope;

    byteCodeGenerator->PushScope(scope);
    Visit(pnode->AsParseNodeWith()->pnodeBody, byteCodeGenerator, prefix, postfix, pnodeParent);

    scope->SetIsObject();
    scope->SetMustInstantiate(true);

    byteCodeGenerator->PopScope();
}

bool BlockHasOwnScope(ParseNodeBlock* pnodeBlock, ByteCodeGenerator *byteCodeGenerator)
{
    Assert(pnodeBlock->nop == knopBlock);
    return pnodeBlock->scope != nullptr &&
        (!(pnodeBlock->grfpn & fpnSyntheticNode) ||
            (pnodeBlock->blockType == PnodeBlockType::Global && byteCodeGenerator->IsEvalWithNoParentScopeInfo()));
}

void BeginVisitBlock(ParseNodeBlock *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    if (BlockHasOwnScope(pnode, byteCodeGenerator))
    {
        Scope *scope = pnode->scope;
        FuncInfo *func = scope->GetFunc();

        if (scope->IsInnerScope())
        {
            // Give this scope an index so its slots can be accessed via the index in the byte code,
            // not a register.
            scope->SetInnerScopeIndex(func->AcquireInnerScopeIndex());
        }

        byteCodeGenerator->PushBlock(pnode);
        byteCodeGenerator->PushScope(pnode->scope);
    }
}

void EndVisitBlock(ParseNodeBlock *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    if (BlockHasOwnScope(pnode, byteCodeGenerator))
    {
        Scope *scope = pnode->scope;
        FuncInfo *func = scope->GetFunc();

        if (!byteCodeGenerator->IsInDebugMode() &&
            scope->HasInnerScopeIndex())
        {
            // In debug mode, don't release the current index, as we're giving each scope a unique index, regardless
            // of nesting.
            Assert(scope->GetInnerScopeIndex() == func->CurrentInnerScopeIndex());
            func->ReleaseInnerScopeIndex();
        }

        Assert(byteCodeGenerator->GetCurrentScope() == scope);
        byteCodeGenerator->PopScope();
        byteCodeGenerator->PopBlock();
    }
}

void BeginVisitCatch(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Scope *scope = pnode->AsParseNodeCatch()->scope;
    FuncInfo *func = scope->GetFunc();

    if (func->GetCallsEval() || func->GetChildCallsEval() ||
        (byteCodeGenerator->GetFlags() & (fscrEval | fscrImplicitThis)))
    {
        scope->SetIsObject();
    }

    // Give this scope an index so its slots can be accessed via the index in the byte code,
    // not a register.
    scope->SetInnerScopeIndex(func->AcquireInnerScopeIndex());

    byteCodeGenerator->PushScope(pnode->AsParseNodeCatch()->scope);
}

void EndVisitCatch(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Scope *scope = pnode->AsParseNodeCatch()->scope;

    if (scope->HasInnerScopeIndex() && !byteCodeGenerator->IsInDebugMode())
    {
        // In debug mode, don't release the current index, as we're giving each scope a unique index,
        // regardless of nesting.
        FuncInfo *func = scope->GetFunc();

        Assert(scope->GetInnerScopeIndex() == func->CurrentInnerScopeIndex());
        func->ReleaseInnerScopeIndex();
    }

    byteCodeGenerator->PopScope();
}

bool CreateNativeArrays(ByteCodeGenerator *byteCodeGenerator, FuncInfo *funcInfo)
{
#if ENABLE_PROFILE_INFO
    Js::FunctionBody *functionBody = funcInfo ? funcInfo->GetParsedFunctionBody() : nullptr;

    return
        !PHASE_OFF_OPTFUNC(Js::NativeArrayPhase, functionBody) &&
        !byteCodeGenerator->IsInDebugMode() &&
        (
            functionBody
                ? Js::DynamicProfileInfo::IsEnabled(Js::NativeArrayPhase, functionBody)
                : Js::DynamicProfileInfo::IsEnabledForAtLeastOneFunction(
                    Js::NativeArrayPhase,
                    byteCodeGenerator->GetScriptContext())
        );
#else
    return false;
#endif
}

bool EmitAsConstantArray(ParseNode *pnodeArr, ByteCodeGenerator *byteCodeGenerator)
{
    Assert(pnodeArr && pnodeArr->nop == knopArray);

    // TODO: We shouldn't have to handle an empty funcinfo stack here, but there seem to be issues
    // with the stack involved nested deferral. Remove this null check when those are resolved.
    if (CreateNativeArrays(byteCodeGenerator, byteCodeGenerator->TopFuncInfo()))
    {
        return pnodeArr->AsParseNodeArrLit()->arrayOfNumbers;
    }

    return pnodeArr->AsParseNodeArrLit()->arrayOfTaggedInts && pnodeArr->AsParseNodeArrLit()->count > 1;
}

void PropagateFlags(ParseNode *pnodeChild, ParseNode *pnodeParent);

template<class PrefixFn, class PostfixFn>
void Visit(ParseNode *pnode, ByteCodeGenerator* byteCodeGenerator, PrefixFn prefix, PostfixFn postfix, ParseNode *pnodeParent)
{
    if (pnode == nullptr)
    {
        return;
    }

    ThreadContext::ProbeCurrentStackNoDispose(Js::Constants::MinStackByteCodeVisitor, byteCodeGenerator->GetScriptContext());

    prefix(pnode, byteCodeGenerator);
    switch (pnode->nop)
    {
    default:
    {
        uint flags = ParseNode::Grfnop(pnode->nop);
        if (flags&fnopUni)
        {
            Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
        }
        else if (flags&fnopBin)
        {
            Visit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, prefix, postfix);
            Visit(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, prefix, postfix);

            if (ByteCodeGenerator::IsSuper(pnode->AsParseNodeBin()->pnode1))
            {
                Visit(pnode->AsParseNodeSuperReference()->pnodeThis, byteCodeGenerator, prefix, postfix);
            }
        }

        break;
    }

    case knopParamPattern:
        Visit(pnode->AsParseNodeParamPattern()->pnode1, byteCodeGenerator, prefix, postfix);
        break;

    case knopArrayPattern:
        if (!byteCodeGenerator->InDestructuredPattern())
        {
            byteCodeGenerator->SetInDestructuredPattern(true);
            Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
            byteCodeGenerator->SetInDestructuredPattern(false);
        }
        else
        {
            Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
        }
        break;

    case knopCall:
        Visit(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeCall()->pnodeArgs, byteCodeGenerator, prefix, postfix);

        if (pnode->AsParseNodeCall()->isSuperCall)
        {
            Visit(pnode->AsParseNodeSuperCall()->pnodeThis, byteCodeGenerator, prefix, postfix);
            Visit(pnode->AsParseNodeSuperCall()->pnodeNewTarget, byteCodeGenerator, prefix, postfix);
        }
        break;

    case knopNew:
    {
        Visit(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator, prefix, postfix);
        if (!IsCallOfConstants(pnode))
        {
            Visit(pnode->AsParseNodeCall()->pnodeArgs, byteCodeGenerator, prefix, postfix);
        }
        break;
    }

    case knopQmark:
        Visit(pnode->AsParseNodeTri()->pnode1, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeTri()->pnode2, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeTri()->pnode3, byteCodeGenerator, prefix, postfix);
        break;
    case knopList:
        VisitList(pnode, byteCodeGenerator, prefix, postfix);
        break;
    // PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
    case knopVarDecl:
    case knopConstDecl:
    case knopLetDecl:
        if (pnode->AsParseNodeVar()->pnodeInit != nullptr)
            Visit(pnode->AsParseNodeVar()->pnodeInit, byteCodeGenerator, prefix, postfix);
        break;
    // PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
    case knopFncDecl:
    {
        // Inner function declarations are visited before anything else in the scope.
        // (See VisitFunctionsInScope.)
        break;
    }
    case knopClassDecl:
    {
        Visit(pnode->AsParseNodeClass()->pnodeDeclName, byteCodeGenerator, prefix, postfix);
        // Now visit the class name and methods.
        BeginVisitBlock(pnode->AsParseNodeClass()->pnodeBlock, byteCodeGenerator);
        // The extends clause is bound to the scope which contains the class name
        // (and the class name identifier is in a TDZ when the extends clause is evaluated).
        // See ES 2017 14.5.13 Runtime Semantics: ClassDefinitionEvaluation.
        Visit(pnode->AsParseNodeClass()->pnodeExtends, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeClass()->pnodeName, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeClass()->pnodeStaticMembers, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeClass()->pnodeConstructor, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeClass()->pnodeMembers, byteCodeGenerator, prefix, postfix);
        EndVisitBlock(pnode->AsParseNodeClass()->pnodeBlock, byteCodeGenerator);
        break;
    }
    case knopStrTemplate:
    {
        // Visit the string node lists only if we do not have a tagged template.
        // We never need to visit the raw strings as they are not used in non-tagged templates and
        // tagged templates will register them as part of the callsite constant object.
        if (!pnode->AsParseNodeStrTemplate()->isTaggedTemplate)
        {
            Visit(pnode->AsParseNodeStrTemplate()->pnodeStringLiterals, byteCodeGenerator, prefix, postfix);
        }
        Visit(pnode->AsParseNodeStrTemplate()->pnodeSubstitutionExpressions, byteCodeGenerator, prefix, postfix);
        break;
    }
    case knopExportDefault:
        Visit(pnode->AsParseNodeExportDefault()->pnodeExpr, byteCodeGenerator, prefix, postfix);
        break;
    // PTNODE(knopProg       , "program"    ,None    ,Fnc  ,fnopNone)
    case knopProg:
    {
        // We expect that the global statements have been generated (meaning that the pnodeFncs
        // field is a real pointer, not an enumeration).
        Assert(pnode->AsParseNodeFnc()->pnodeBody);

        uint i = 0;
        VisitNestedScopes(pnode->AsParseNodeFnc()->pnodeScopes, pnode, byteCodeGenerator, prefix, postfix, &i);
        // Visiting global code: track the last value statement.
        BeginVisitBlock(pnode->AsParseNodeFnc()->pnodeScopes, byteCodeGenerator);
        pnode->AsParseNodeProg()->pnodeLastValStmt = VisitBlock(pnode->AsParseNodeFnc()->pnodeBody, byteCodeGenerator, prefix, postfix);
        EndVisitBlock(pnode->AsParseNodeFnc()->pnodeScopes, byteCodeGenerator);

        break;
    }
    case knopFor:
        BeginVisitBlock(pnode->AsParseNodeFor()->pnodeBlock, byteCodeGenerator);
        Visit(pnode->AsParseNodeFor()->pnodeInit, byteCodeGenerator, prefix, postfix);
        byteCodeGenerator->EnterLoop();
        Visit(pnode->AsParseNodeFor()->pnodeCond, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeFor()->pnodeIncr, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeFor()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        byteCodeGenerator->ExitLoop();
        EndVisitBlock(pnode->AsParseNodeFor()->pnodeBlock, byteCodeGenerator);
        break;
    // PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
    case knopIf:
        Visit(pnode->AsParseNodeIf()->pnodeCond, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeIf()->pnodeTrue, byteCodeGenerator, prefix, postfix, pnode);
        if (pnode->AsParseNodeIf()->pnodeFalse != nullptr)
        {
            Visit(pnode->AsParseNodeIf()->pnodeFalse, byteCodeGenerator, prefix, postfix, pnode);
        }
        break;
    // PTNODE(knopWhile      , "while"        ,None    ,While,fnopBreak|fnopContinue)
    // PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
    case knopDoWhile:
    case knopWhile:
        byteCodeGenerator->EnterLoop();
        Visit(pnode->AsParseNodeWhile()->pnodeCond, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeWhile()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        byteCodeGenerator->ExitLoop();
        break;
    // PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
    case knopForIn:
    case knopForOf:
        BeginVisitBlock(pnode->AsParseNodeForInOrForOf()->pnodeBlock, byteCodeGenerator);
        Visit(pnode->AsParseNodeForInOrForOf()->pnodeLval, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeForInOrForOf()->pnodeObj, byteCodeGenerator, prefix, postfix);
        byteCodeGenerator->EnterLoop();
        Visit(pnode->AsParseNodeForInOrForOf()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        byteCodeGenerator->ExitLoop();
        EndVisitBlock(pnode->AsParseNodeForInOrForOf()->pnodeBlock, byteCodeGenerator);
        break;
    // PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
    case knopReturn:
        if (pnode->AsParseNodeReturn()->pnodeExpr != nullptr)
            Visit(pnode->AsParseNodeReturn()->pnodeExpr, byteCodeGenerator, prefix, postfix);
        break;
    // PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
    case knopBlock:
    {
        ParseNodeBlock * pnodeBlock = pnode->AsParseNodeBlock();
        if (pnodeBlock->pnodeStmt != nullptr)
        {
            BeginVisitBlock(pnodeBlock, byteCodeGenerator);
            pnodeBlock->pnodeLastValStmt = VisitBlock(pnodeBlock->pnodeStmt, byteCodeGenerator, prefix, postfix, pnode);
            EndVisitBlock(pnodeBlock, byteCodeGenerator);
        }
        else
        {
            pnodeBlock->pnodeLastValStmt = nullptr;
        }
        break;
    }
    // PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
    case knopWith:
        Visit(pnode->AsParseNodeWith()->pnodeObj, byteCodeGenerator, prefix, postfix);
        VisitWithStmt(pnode, pnode->AsParseNodeWith()->pnodeObj->location, byteCodeGenerator, prefix, postfix, pnode);
        break;
    // PTNODE(knopBreak      , "break"        ,None    ,Jump ,fnopNone)
    case knopBreak:
        // TODO: some representation of target
        break;
    // PTNODE(knopContinue   , "continue"    ,None    ,Jump ,fnopNone)
    case knopContinue:
        // TODO: some representation of target
        break;
    // PTNODE(knopSwitch     , "switch"    ,None    ,Switch,fnopBreak)
    case knopSwitch:
        Visit(pnode->AsParseNodeSwitch()->pnodeVal, byteCodeGenerator, prefix, postfix);
        BeginVisitBlock(pnode->AsParseNodeSwitch()->pnodeBlock, byteCodeGenerator);
        for (ParseNodeCase *pnodeT = pnode->AsParseNodeSwitch()->pnodeCases; nullptr != pnodeT; pnodeT = pnodeT->pnodeNext)
        {
            Visit(pnodeT, byteCodeGenerator, prefix, postfix, pnode);
        }
        Visit(pnode->AsParseNodeSwitch()->pnodeBlock, byteCodeGenerator, prefix, postfix);
        EndVisitBlock(pnode->AsParseNodeSwitch()->pnodeBlock, byteCodeGenerator);
        break;
    // PTNODE(knopCase       , "case"        ,None    ,Case ,fnopNone)
    case knopCase:
        Visit(pnode->AsParseNodeCase()->pnodeExpr, byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeCase()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        break;
    case knopTypeof:
        Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
        break;
    // PTNODE(knopTryCatchFinally,"try-catch-finally",None,TryCatchFinally,fnopCleanup)
    case knopTryFinally:
        Visit(pnode->AsParseNodeTryFinally()->pnodeTry, byteCodeGenerator, prefix, postfix, pnode);
        Visit(pnode->AsParseNodeTryFinally()->pnodeFinally, byteCodeGenerator, prefix, postfix, pnode);
        break;
    // PTNODE(knopTryCatch      , "try-catch" ,None    ,TryCatch  ,fnopCleanup)
    case knopTryCatch:
        Visit(pnode->AsParseNodeTryCatch()->pnodeTry, byteCodeGenerator, prefix, postfix, pnode);
        Visit(pnode->AsParseNodeTryCatch()->pnodeCatch, byteCodeGenerator, prefix, postfix, pnode);
        break;
    // PTNODE(knopTry        , "try"       ,None    ,Try  ,fnopCleanup)
    case knopTry:
        Visit(pnode->AsParseNodeTry()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        break;
    case knopCatch:
        BeginVisitCatch(pnode, byteCodeGenerator);
        Visit(pnode->AsParseNodeCatch()->GetParam(), byteCodeGenerator, prefix, postfix);
        Visit(pnode->AsParseNodeCatch()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        EndVisitCatch(pnode, byteCodeGenerator);
        break;
    case knopFinally:
        Visit(pnode->AsParseNodeFinally()->pnodeBody, byteCodeGenerator, prefix, postfix, pnode);
        break;
    // PTNODE(knopThrow      , "throw"     ,None    ,Uni  ,fnopNone)
    case knopThrow:
        Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
        break;
    case knopArray:
    {
        bool arrayLitOpt = EmitAsConstantArray(pnode, byteCodeGenerator);
        if (!arrayLitOpt)
        {
            Visit(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator, prefix, postfix);
        }
        break;
    }
    case knopComma:
    {
        ParseNode *pnode1 = pnode->AsParseNodeBin()->pnode1;
        if (pnode1->nop == knopComma)
        {
            // Spot-fix to avoid recursion on very large comma expressions.
            ArenaAllocator *alloc = byteCodeGenerator->GetAllocator();
            SList<ParseNode*> rhsStack(alloc);
            do
            {
                rhsStack.Push(pnode1->AsParseNodeBin()->pnode2);
                pnode1 = pnode1->AsParseNodeBin()->pnode1;
            }
            while (pnode1->nop == knopComma);

            Visit(pnode1, byteCodeGenerator, prefix, postfix);
            while (!rhsStack.Empty())
            {
                ParseNode *pnodeRhs = rhsStack.Pop();
                Visit(pnodeRhs, byteCodeGenerator, prefix, postfix);
            }
        }
        else
        {
            Visit(pnode1, byteCodeGenerator, prefix, postfix);
        }
        Visit(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator, prefix, postfix);
    }
        break;
    }
    if (pnodeParent)
    {
        PropagateFlags(pnode, pnodeParent);
    }
    postfix(pnode, byteCodeGenerator);
}

bool IsJump(ParseNode *pnode)
{
    switch (pnode->nop)
    {
    case knopBreak:
    case knopContinue:
    case knopThrow:
    case knopReturn:
        return true;

    case knopBlock:
    case knopDoWhile:
    case knopWhile:
    case knopWith:
    case knopIf:
    case knopForIn:
    case knopForOf:
    case knopFor:
    case knopSwitch:
    case knopCase:
    case knopTryFinally:
    case knopTryCatch:
    case knopTry:
    case knopCatch:
    case knopFinally:
        return (pnode->AsParseNodeStmt()->grfnop & fnopJump) != 0;

    default:
        return false;
    }
}

void PropagateFlags(ParseNode *pnodeChild, ParseNode *pnodeParent)
{
    if (IsJump(pnodeChild))
    {
        pnodeParent->AsParseNodeStmt()->grfnop |= fnopJump;
    }
}

void Bind(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator);
void BindReference(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator);
void AssignRegisters(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator);

// TODO[ianhall]: This should be in a shared AST Utility header or source file
bool IsExpressionStatement(ParseNode* stmt, const Js::ScriptContext *const scriptContext)
{
    if (stmt->nop == knopFncDecl)
    {
        // 'knopFncDecl' is used for both function declarations and function expressions. In a program, a function expression
        // produces the function object that is created for the function expression as its value for the program. A function
        // declaration does not produce a value for the program.
        return !stmt->AsParseNodeFnc()->IsDeclaration();
    }
    if ((stmt->nop >= 0) && (stmt->nop<knopLim))
    {
        return (ParseNode::Grfnop(stmt->nop) & fnopNotExprStmt) == 0;
    }
    return false;
}

bool MustProduceValue(ParseNode *pnode, const Js::ScriptContext *const scriptContext)
{
    // Determine whether the current statement is guaranteed to produce a value.

    if (IsExpressionStatement(pnode, scriptContext))
    {
        // These are trivially true.
        return true;
    }

    for (;;)
    {
        switch (pnode->nop)
        {
        case knopFor:
            // Check the common "for (;;)" case.
            if (pnode->AsParseNodeFor()->pnodeCond != nullptr ||
                pnode->AsParseNodeFor()->pnodeBody == nullptr)
            {
                return false;
            }
            // Loop body is always executed. Look at the loop body next.
            pnode = pnode->AsParseNodeFor()->pnodeBody;
            break;

        case knopIf:
            // True only if both "if" and "else" exist, and both produce values.
            if (pnode->AsParseNodeIf()->pnodeTrue == nullptr ||
                pnode->AsParseNodeIf()->pnodeFalse == nullptr)
            {
                return false;
            }
            if (!MustProduceValue(pnode->AsParseNodeIf()->pnodeFalse, scriptContext))
            {
                return false;
            }
            pnode = pnode->AsParseNodeIf()->pnodeTrue;
            break;

        case knopWhile:
            // Check the common "while (1)" case.
            if (pnode->AsParseNodeWhile()->pnodeBody == nullptr ||
                (pnode->AsParseNodeWhile()->pnodeCond &&
                (pnode->AsParseNodeWhile()->pnodeCond->nop != knopInt ||
                pnode->AsParseNodeWhile()->pnodeCond->AsParseNodeInt()->lw == 0)))
            {
                return false;
            }
            // Loop body is always executed. Look at the loop body next.
            pnode = pnode->AsParseNodeWhile()->pnodeBody;
            break;

        case knopDoWhile:
            if (pnode->AsParseNodeWhile()->pnodeBody == nullptr)
            {
                return false;
            }
            // Loop body is always executed. Look at the loop body next.
            pnode = pnode->AsParseNodeWhile()->pnodeBody;
            break;

        case knopBlock:
            return pnode->AsParseNodeBlock()->pnodeLastValStmt != nullptr;

        case knopWith:
            if (pnode->AsParseNodeWith()->pnodeBody == nullptr)
            {
                return false;
            }
            pnode = pnode->AsParseNodeWith()->pnodeBody;
            break;

        case knopSwitch:
            {
                // This is potentially the most inefficient case. We could consider adding a flag to the PnSwitch
                // struct and computing it when we visit the switch, but:
                // a. switch statements at global scope shouldn't be that common;
                // b. switch statements with many arms shouldn't be that common;
                // c. switches without default cases can be trivially skipped.
                if (pnode->AsParseNodeSwitch()->pnodeDefault == nullptr)
                {
                    // Can't guarantee that any code is executed.
                return false;
                }
                ParseNodeCase *pnodeCase;
                for (pnodeCase = pnode->AsParseNodeSwitch()->pnodeCases; pnodeCase; pnodeCase = pnodeCase->pnodeNext)
                {
                    if (pnodeCase->pnodeBody == nullptr)
                    {
                        if (pnodeCase->pnodeNext == nullptr)
                        {
                            // Last case has no code to execute.
                        return false;
                        }
                        // Fall through to the next case.
                    }
                    else
                    {
                        if (!MustProduceValue(pnodeCase->pnodeBody, scriptContext))
                        {
                        return false;
                        }
                    }
                }
            return true;
            }

        case knopTryCatch:
            // True only if both try and catch produce a value.
            if (pnode->AsParseNodeTryCatch()->pnodeTry->pnodeBody == nullptr ||
                pnode->AsParseNodeTryCatch()->pnodeCatch->pnodeBody == nullptr)
            {
                return false;
            }
            if (!MustProduceValue(pnode->AsParseNodeTryCatch()->pnodeCatch->pnodeBody, scriptContext))
            {
                return false;
            }
            pnode = pnode->AsParseNodeTryCatch()->pnodeTry->pnodeBody;
            break;

        case knopTryFinally:
            if (pnode->AsParseNodeTryFinally()->pnodeFinally->pnodeBody == nullptr)
            {
                // No finally body: look at the try body.
                if (pnode->AsParseNodeTryFinally()->pnodeTry->pnodeBody == nullptr)
                {
                    return false;
                }
                pnode = pnode->AsParseNodeTryFinally()->pnodeTry->pnodeBody;
                break;
            }
            // Skip the try body, since the finally body will always follow it.
            pnode = pnode->AsParseNodeTryFinally()->pnodeFinally->pnodeBody;
            break;

        default:
            return false;
        }
    }
}

ByteCodeGenerator::ByteCodeGenerator(Js::ScriptContext* scriptContext, Js::ScopeInfo* parentScopeInfo) :
    alloc(nullptr),
    scriptContext(scriptContext),
    flags(0),
    funcInfoStack(nullptr),
    pRootFunc(nullptr),
    pCurrentFunction(nullptr),
    globalScope(nullptr),
    currentScope(nullptr),
    parentScopeInfo(parentScopeInfo),
    dynamicScopeCount(0),
    isBinding(false),
    inDestructuredPattern(false)
{
    m_writer.Create();
}

void ByteCodeGenerator::FinalizeFuncInfos()
{
    if (this->funcInfosToFinalize == nullptr)
    {
        return;
    }

    FOREACH_SLIST_ENTRY(FuncInfo*, funcInfo, this->funcInfosToFinalize)
    {
        if (funcInfo->canDefer)
        {
            funcInfo->byteCodeFunction->SetAttributes((Js::FunctionInfo::Attributes)(funcInfo->byteCodeFunction->GetAttributes() | Js::FunctionInfo::Attributes::CanDefer));
        }
    }
    NEXT_SLIST_ENTRY;

    this->funcInfosToFinalize = nullptr;
}

void ByteCodeGenerator::AddFuncInfoToFinalizationSet(FuncInfo * funcInfo)
{
    if (this->funcInfosToFinalize == nullptr)
    {
        this->funcInfosToFinalize = Anew(alloc, SList<FuncInfo*>, alloc);
    }

    this->funcInfosToFinalize->Prepend(funcInfo);
}

/* static */
bool ByteCodeGenerator::IsFalse(ParseNode* node)
{
    return (node->nop == knopInt && node->AsParseNodeInt()->lw == 0) || node->nop == knopFalse;
}

/* static */
bool ByteCodeGenerator::IsThis(ParseNode* pnode)
{
    return pnode->nop == knopName && pnode->AsParseNodeName()->IsSpecialName() && pnode->AsParseNodeSpecialName()->isThis;
}

/* static */
bool ByteCodeGenerator::IsSuper(ParseNode* pnode)
{
    return pnode->nop == knopName && pnode->AsParseNodeName()->IsSpecialName() && pnode->AsParseNodeSpecialName()->isSuper;
}

bool ByteCodeGenerator::IsES6DestructuringEnabled() const
{
    return scriptContext->GetConfig()->IsES6DestructuringEnabled();
}

bool ByteCodeGenerator::IsES6ForLoopSemanticsEnabled() const
{
    return scriptContext->GetConfig()->IsES6ForLoopSemanticsEnabled();
}

// ByteCodeGenerator debug mode means we are generating debug mode user-code. Library code is always in non-debug mode.
bool ByteCodeGenerator::IsInDebugMode() const
{
    return m_utf8SourceInfo->IsInDebugMode();
}

// ByteCodeGenerator non-debug mode means we are not debugging, or we are generating library code which is always in non-debug mode.
bool ByteCodeGenerator::IsInNonDebugMode() const
{
    return scriptContext->IsScriptContextInNonDebugMode() || m_utf8SourceInfo->GetIsLibraryCode();
}

bool ByteCodeGenerator::ShouldTrackDebuggerMetadata() const
{
    return (IsInDebugMode())
#if DBG_DUMP
        || (Js::Configuration::Global.flags.Debug)
#endif
        ;
}

void ByteCodeGenerator::SetRootFuncInfo(FuncInfo* func)
{
    Assert(pRootFunc == nullptr || pRootFunc == func->byteCodeFunction || !IsInNonDebugMode());

    if ((this->flags & fscrImplicitThis) && !this->HasParentScopeInfo())
    {
        // Mark a top-level event handler, since it will need to construct the "this" pointer's
        // namespace hierarchy to access globals.
        Assert(!func->IsGlobalFunction());
        func->SetIsTopLevelEventHandler(true);
    }

    if (pRootFunc)
    {
        return;
    }

    this->pRootFunc = func->byteCodeFunction->GetParseableFunctionInfo();
    this->m_utf8SourceInfo->AddTopLevelFunctionInfo(func->byteCodeFunction->GetFunctionInfo(), scriptContext->GetRecycler());
}

Js::RegSlot ByteCodeGenerator::NextVarRegister()
{
    return funcInfoStack->Top()->NextVarRegister();
}

Js::RegSlot ByteCodeGenerator::NextConstRegister()
{
    return funcInfoStack->Top()->NextConstRegister();
}

FuncInfo * ByteCodeGenerator::TopFuncInfo() const
{
    return funcInfoStack->Empty() ? nullptr : funcInfoStack->Top();
}

void ByteCodeGenerator::EnterLoop()
{
    if (this->TopFuncInfo())
    {
        this->TopFuncInfo()->hasLoop = true;
    }
    loopDepth++;
}

void ByteCodeGenerator::SetHasTry(bool has)
{
    TopFuncInfo()->GetParsedFunctionBody()->SetHasTry(has);
}

void ByteCodeGenerator::SetHasFinally(bool has)
{
    TopFuncInfo()->GetParsedFunctionBody()->SetHasFinally(has);
}

// TODO: per-function register assignment for env and global symbols
void ByteCodeGenerator::AssignRegister(Symbol *sym)
    {
    AssertMsg(sym->GetDecl() == nullptr || sym->GetDecl()->nop != knopConstDecl || sym->GetDecl()->nop != knopLetDecl,
        "const and let should get only temporary register, assigned during emit stage");
    if (sym->GetLocation() == Js::Constants::NoRegister)
    {
        sym->SetLocation(NextVarRegister());
    }
}

void ByteCodeGenerator::AddTargetStmt(ParseNodeStmt *pnodeStmt)
{
    FuncInfo *top = funcInfoStack->Top();
    top->AddTargetStmt(pnodeStmt);
}

Js::RegSlot ByteCodeGenerator::AssignThisConstRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    return top->AssignThisConstRegister();
}

Js::RegSlot ByteCodeGenerator::AssignNullConstRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    return top->AssignNullConstRegister();
}

Js::RegSlot ByteCodeGenerator::AssignUndefinedConstRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    return top->AssignUndefinedConstRegister();
}

Js::RegSlot ByteCodeGenerator::AssignTrueConstRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    return top->AssignTrueConstRegister();
}

Js::RegSlot ByteCodeGenerator::AssignFalseConstRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    return top->AssignFalseConstRegister();
}

void ByteCodeGenerator::SetNeedEnvRegister()
{
    FuncInfo *top = funcInfoStack->Top();
    top->SetNeedEnvRegister();
}

void ByteCodeGenerator::AssignFrameObjRegister()
{
    FuncInfo* top = funcInfoStack->Top();
    if (top->frameObjRegister == Js::Constants::NoRegister)
    {
        top->frameObjRegister = top->NextVarRegister();
    }
}

void ByteCodeGenerator::AssignFrameDisplayRegister()
{
    FuncInfo* top = funcInfoStack->Top();
    if (top->frameDisplayRegister == Js::Constants::NoRegister)
    {
        top->frameDisplayRegister = top->NextVarRegister();
    }
}

void ByteCodeGenerator::AssignFrameSlotsRegister()
{
    FuncInfo* top = funcInfoStack->Top();
    if (top->frameSlotsRegister == Js::Constants::NoRegister)
    {
        top->frameSlotsRegister = NextVarRegister();
    }
}

void ByteCodeGenerator::AssignParamSlotsRegister()
{
    FuncInfo* top = funcInfoStack->Top();
    Assert(top->paramSlotsRegister == Js::Constants::NoRegister);
    top->paramSlotsRegister = NextVarRegister();
}

void ByteCodeGenerator::SetNumberOfInArgs(Js::ArgSlot argCount)
{
    FuncInfo *top = funcInfoStack->Top();
    top->inArgsCount = argCount;
}

Js::RegSlot ByteCodeGenerator::EnregisterConstant(unsigned int constant)
{
    Js::RegSlot loc = Js::Constants::NoRegister;
    FuncInfo *top = funcInfoStack->Top();
    if (!top->constantToRegister.TryGetValue(constant, &loc))
    {
        loc = NextConstRegister();
        top->constantToRegister.Add(constant, loc);
    }
    return loc;
}

Js::RegSlot ByteCodeGenerator::EnregisterBigIntConstant(ParseNode* pnode)
{
    Js::RegSlot loc = Js::Constants::NoRegister;
    FuncInfo *top = funcInfoStack->Top();
    if (!top->bigintToRegister.TryGetValue(pnode, &loc))
    {
        loc = NextConstRegister();
        top->bigintToRegister.Add(pnode, loc);
    }
    return loc;
}

Js::RegSlot ByteCodeGenerator::EnregisterStringConstant(IdentPtr pid)
{
    Js::RegSlot loc = Js::Constants::NoRegister;
    FuncInfo *top = funcInfoStack->Top();
    if (!top->stringToRegister.TryGetValue(pid, &loc))
    {
        loc = NextConstRegister();
        top->stringToRegister.Add(pid, loc);
    }
    return loc;
}

Js::RegSlot ByteCodeGenerator::EnregisterDoubleConstant(double d)
{
    Js::RegSlot loc = Js::Constants::NoRegister;
    FuncInfo *top = funcInfoStack->Top();
    if (!top->TryGetDoubleLoc(d, &loc))
    {
        loc = NextConstRegister();
        top->AddDoubleConstant(d, loc);
    }
    return loc;
}

Js::RegSlot ByteCodeGenerator::EnregisterStringTemplateCallsiteConstant(ParseNode* pnode)
{
    Assert(pnode->nop == knopStrTemplate);
    Assert(pnode->AsParseNodeStrTemplate()->isTaggedTemplate);

    Js::RegSlot loc = Js::Constants::NoRegister;
    FuncInfo* top = funcInfoStack->Top();

    if (!top->stringTemplateCallsiteRegisterMap.TryGetValue(pnode, &loc))
    {
        loc = NextConstRegister();

        top->stringTemplateCallsiteRegisterMap.Add(pnode, loc);
    }

    return loc;
}

//
// Restore all outer func scope info when reparsing a deferred func.
//
void ByteCodeGenerator::RestoreScopeInfo(Js::ScopeInfo *scopeInfo, FuncInfo * func)
{
    if (scopeInfo)
    {
        PROBE_STACK_NO_DISPOSE(scriptContext, Js::Constants::MinStackByteCodeVisitor);

        Js::ParseableFunctionInfo * pfi = scopeInfo->GetFunctionInfo()->GetParseableFunctionInfo();
        bool newFunc = (func == nullptr || func->byteCodeFunction != pfi);

        if (newFunc)
        {
            func = Anew(alloc, FuncInfo, pfi->GetDisplayName(), alloc, this, nullptr, nullptr, nullptr, pfi);
        }

        // Recursively restore enclosing scope info so outermost scopes/funcs are pushed first.
        this->RestoreScopeInfo(scopeInfo->GetParentScopeInfo(), func);
        this->RestoreOneScope(scopeInfo, func);

        if (newFunc)
        {
            PushFuncInfo(_u("RestoreScopeInfo"), func);
            if (!pfi->DoStackNestedFunc())
            {
                func->hasEscapedUseNestedFunc = true;
            }
        }
    }
    else
    {
        Assert(this->TopFuncInfo() == nullptr);
        // funcBody is glo
        Assert(currentScope == nullptr);
        currentScope = Anew(alloc, Scope, alloc, ScopeType_Global);
        globalScope = currentScope;

        if (func == nullptr || !func->byteCodeFunction->GetIsGlobalFunc())
        {
            func = Anew(alloc, FuncInfo, Js::Constants::GlobalFunction,
                alloc, this, nullptr, nullptr/*currentScope*/, nullptr, nullptr/*functionBody*/);
            PushFuncInfo(_u("RestoreScopeInfo"), func);
        }
        func->SetBodyScope(currentScope);
    }
}

void ByteCodeGenerator::RestoreOneScope(Js::ScopeInfo * scopeInfo, FuncInfo * func)
{
    TRACE_BYTECODE(_u("\nRestore ScopeInfo: %s #symbols: %d %s\n"),
        func->name, scopeInfo->GetSymbolCount(), scopeInfo->IsObject() ? _u("isObject") : _u(""));

    Scope * scope = scopeInfo->GetScope();

    scope->SetFunc(func);

    switch (scope->GetScopeType())
    {
        case ScopeType_Parameter:
            Assert(func->GetParamScope() == nullptr);
            func->SetParamScope(scope);
            break;

        case ScopeType_FuncExpr:
            Assert(func->GetFuncExprScope() == nullptr);
            func->SetFuncExprScope(scope);
            break;

        case ScopeType_FunctionBody:
        case ScopeType_GlobalEvalBlock:
            Assert(func->GetBodyScope() == nullptr || (func->GetBodyScope()->GetScopeType() == ScopeType_Global && scope->GetScopeType() == ScopeType_GlobalEvalBlock));
            func->SetBodyScope(scope);
            func->SetHasCachedScope(scopeInfo->IsCached());
            break;
    }
    
    Assert(!scopeInfo->IsCached() || scope == func->GetBodyScope());

    // scopeInfo->scope was created/saved during parsing.
    // We no longer need it by now.
    // Clear it to avoid GC false positive (arena memory later used by GC).
    scopeInfo->SetScope(nullptr);
    this->PushScope(scope);
}

FuncInfo * ByteCodeGenerator::StartBindGlobalStatements(ParseNodeProg *pnode)
{
    if (parentScopeInfo)
    {
        trackEnvDepth = true;
        RestoreScopeInfo(parentScopeInfo, nullptr);
        trackEnvDepth = false;
        // "currentScope" is the parentFunc scope. This ensures the deferred func declaration
        // symbol will bind to the func declaration symbol already available in parentFunc scope.
    }
    else
    {
        currentScope = pnode->scope;
        Assert(currentScope);
        globalScope = currentScope;
    }

    Js::FunctionBody * byteCodeFunction;

    if (!IsInNonDebugMode() && this->pCurrentFunction != nullptr && this->pCurrentFunction->GetIsGlobalFunc() && !this->pCurrentFunction->IsFakeGlobalFunc(flags))
    {
        // we will re-use the global FunctionBody which was created before deferred parse.
        byteCodeFunction = this->pCurrentFunction;
        byteCodeFunction->RemoveDeferParseAttribute();
        byteCodeFunction->ResetByteCodeGenVisitState();
    }
    else if ((this->flags & fscrDeferredFnc))
    {
        byteCodeFunction = this->EnsureFakeGlobalFuncForUndefer(pnode);
    }
    else
    {
        byteCodeFunction = this->MakeGlobalFunctionBody(pnode);

        // Mark this global function to required for register script event
        byteCodeFunction->SetIsTopLevel(true);

        if (pnode->GetStrictMode() != 0)
        {
            byteCodeFunction->SetIsStrictMode();
        }
    }
    if (byteCodeFunction->IsReparsed())
    {
        byteCodeFunction->RestoreState(pnode);
    }
    else
    {
        byteCodeFunction->SaveState(pnode);
    }

    FuncInfo *funcInfo = Anew(alloc, FuncInfo, Js::Constants::GlobalFunction,
        alloc, this, nullptr, globalScope, pnode, byteCodeFunction);

    int32 currentAstSize = pnode->astSize;
    if (currentAstSize > this->maxAstSize)
    {
        this->maxAstSize = currentAstSize;
    }
    PushFuncInfo(_u("StartBindGlobalStatements"), funcInfo);

    return funcInfo;
}

void ByteCodeGenerator::AssignPropertyId(IdentPtr pid)
{
    if (pid->GetPropertyId() == Js::Constants::NoProperty)
    {
        Js::PropertyId id = TopFuncInfo()->byteCodeFunction->GetOrAddPropertyIdTracked(SymbolName(pid->Psz(), pid->Cch()));
        pid->SetPropertyId(id);
    }
}

void ByteCodeGenerator::AssignPropertyId(Symbol *sym, Js::ParseableFunctionInfo* functionInfo)
{
    sym->SetPosition(functionInfo->GetOrAddPropertyIdTracked(sym->GetName()));
}

template <class PrefixFn, class PostfixFn>
ParseNode* VisitBlock(ParseNode *pnode, ByteCodeGenerator* byteCodeGenerator, PrefixFn prefix, PostfixFn postfix, ParseNode *pnodeParent = nullptr)
{
    ParseNode *pnodeLastVal = nullptr;
    if (pnode != nullptr)
    {
        bool fTrackVal = byteCodeGenerator->IsBinding() &&
            (byteCodeGenerator->GetFlags() & fscrReturnExpression) &&
            byteCodeGenerator->TopFuncInfo()->IsGlobalFunction();
        while (pnode->nop == knopList)
        {
            Visit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, prefix, postfix, pnodeParent);
            if (fTrackVal)
            {
                // If we're tracking values, find the last statement (if any) in the block that is
                // guaranteed to produce a value.
                if (MustProduceValue(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator->GetScriptContext()))
                {
                    pnodeLastVal = pnode->AsParseNodeBin()->pnode1;
                }
                if (IsJump(pnode->AsParseNodeBin()->pnode1))
                {
                    // This is a jump out of the current block. The remaining instructions (if any)
                    // will not be executed, so stop tracking them.
                    fTrackVal = false;
                }
            }
            pnode = pnode->AsParseNodeBin()->pnode2;
        }
        Visit(pnode, byteCodeGenerator, prefix, postfix, pnodeParent);
        if (fTrackVal)
        {
            if (MustProduceValue(pnode, byteCodeGenerator->GetScriptContext()))
            {
                pnodeLastVal = pnode;
            }
        }
    }
    return pnodeLastVal;
}

// Attributes that should be consistent between defer parse and full parse.
static const Js::FunctionInfo::Attributes StableFunctionInfoAttributesMask = (Js::FunctionInfo::Attributes)
(
    Js::FunctionInfo::Attributes::ErrorOnNew |
    Js::FunctionInfo::Attributes::Async |
    Js::FunctionInfo::Attributes::Lambda |
    Js::FunctionInfo::Attributes::SuperReference |
    Js::FunctionInfo::Attributes::ClassConstructor |
    Js::FunctionInfo::Attributes::BaseConstructorKind |
    Js::FunctionInfo::Attributes::ClassMethod |
    Js::FunctionInfo::Attributes::Method |
    Js::FunctionInfo::Attributes::Generator |
    Js::FunctionInfo::Attributes::Module |
    Js::FunctionInfo::Attributes::ComputedName |
    Js::FunctionInfo::Attributes::HomeObj
);

static Js::FunctionInfo::Attributes GetFunctionInfoAttributes(ParseNodeFnc * pnodeFnc)
{
    Js::FunctionInfo::Attributes attributes = Js::FunctionInfo::Attributes::None;
    if (pnodeFnc->IsAsync())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ErrorOnNew | Js::FunctionInfo::Attributes::Async);
    }
    if (pnodeFnc->IsLambda())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ErrorOnNew | Js::FunctionInfo::Attributes::Lambda);
    }
    if (pnodeFnc->HasSuperReference())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::SuperReference);
    }
    if (pnodeFnc->IsClassMember())
    {
        if (pnodeFnc->IsClassConstructor())
        {
            attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ClassConstructor);

            if (pnodeFnc->IsBaseClassConstructor())
            {
                attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::BaseConstructorKind);
            }
        }
        else
        {
            attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ErrorOnNew | Js::FunctionInfo::Attributes::ClassMethod);
        }
    }
    if (pnodeFnc->IsMethod())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::Method);

        // #sec-runtime-semantics-classdefinitionevaluation calls #sec-makeconstructor. #sec-makeconstructor
        // creates a prototype. Thus a method that is a class constructor has a prototype and should not
        // throw an error when new is called on the method.
        if (!pnodeFnc->IsClassConstructor())
        {
            attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ErrorOnNew);
        }
    }
    if (pnodeFnc->IsGenerator())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::Generator);
    }
    if (pnodeFnc->IsAccessor())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ErrorOnNew);
    }
    if (pnodeFnc->IsModule())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::Module);
    }
    if (pnodeFnc->CanBeDeferred())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::CanDefer);
    }
    if (pnodeFnc->HasComputedName() && pnodeFnc->pnodeName == nullptr)
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::ComputedName);
    }
    if (pnodeFnc->HasHomeObj())
    {
        attributes = (Js::FunctionInfo::Attributes)(attributes | Js::FunctionInfo::Attributes::HomeObj);
    }
    return attributes;
}

FuncInfo * ByteCodeGenerator::StartBindFunction(const char16 *name, uint nameLength, uint shortNameOffset, bool* pfuncExprWithName, ParseNodeFnc *pnodeFnc, Js::ParseableFunctionInfo * reuseNestedFunc)
{
    bool funcExprWithName;
    Js::ParseableFunctionInfo* parseableFunctionInfo = nullptr;

    Js::AutoRestoreFunctionInfo autoRestoreFunctionInfo(reuseNestedFunc, reuseNestedFunc ? reuseNestedFunc->GetOriginalEntryPoint_Unchecked() : nullptr);

    if (this->pCurrentFunction &&
        this->pCurrentFunction->IsFunctionParsed())
    {
        Assert(this->pCurrentFunction->StartInDocument() == pnodeFnc->ichMin);
        Assert(this->pCurrentFunction->LengthInChars() == pnodeFnc->LengthInCodepoints());

        // This is the root function for the current AST subtree, and it already has a FunctionBody
        // (created by a deferred parse) which we're now filling in.
        Js::FunctionBody * parsedFunctionBody = this->pCurrentFunction;
        parsedFunctionBody->RemoveDeferParseAttribute();

        Assert(!parsedFunctionBody->IsDeferredParseFunction() || parsedFunctionBody->IsReparsed());

        pnodeFnc->SetDeclaration(parsedFunctionBody->GetIsDeclaration());
        if (!pnodeFnc->CanBeDeferred())
        {
            parsedFunctionBody->SetAttributes(
                (Js::FunctionInfo::Attributes)(parsedFunctionBody->GetAttributes() & ~Js::FunctionInfo::Attributes::CanDefer));
        }
        funcExprWithName =
            !(parsedFunctionBody->GetIsDeclaration() || pnodeFnc->IsMethod()) &&
            pnodeFnc->pnodeName != nullptr &&
            pnodeFnc->pnodeName->nop == knopVarDecl;
        *pfuncExprWithName = funcExprWithName;

        Assert(parsedFunctionBody->GetLocalFunctionId() == pnodeFnc->functionId || !IsInNonDebugMode());

        // Some state may be tracked on the function body during the visit pass. Since the previous visit pass may have failed,
        // we need to reset the state on the function body.
        parsedFunctionBody->ResetByteCodeGenVisitState();

        if (parsedFunctionBody->GetScopeInfo())
        {
            // Propagate flags from the (real) parent function.
            Js::ParseableFunctionInfo *parent = parsedFunctionBody->GetScopeInfo()->GetParseableFunctionInfo();
            if (parent)
            {
                if (parent->GetHasOrParentHasArguments())
                {
                    parsedFunctionBody->SetHasOrParentHasArguments(true);
                }
            }
        }

        parseableFunctionInfo = parsedFunctionBody;
    }
    else
    {
        funcExprWithName = *pfuncExprWithName;
        Js::LocalFunctionId functionId = pnodeFnc->functionId;

        // Create a function body if:
        //  1. The parse node is not defer parsed
        //  2. Or creating function proxies is disallowed
        bool createFunctionBody = (pnodeFnc->pnodeBody != nullptr);
        if (!CONFIG_FLAG(CreateFunctionProxy)) createFunctionBody = true;

        const Js::FunctionInfo::Attributes attributes = GetFunctionInfoAttributes(pnodeFnc);

        if (createFunctionBody)
        {
            if (reuseNestedFunc)
            {
                if (!reuseNestedFunc->IsFunctionBody())
                {
                    reuseNestedFunc->GetUtf8SourceInfo()->StopTrackingDeferredFunction(reuseNestedFunc->GetLocalFunctionId());
                    Js::FunctionBody * parsedFunctionBody =
                        Js::FunctionBody::NewFromParseableFunctionInfo(reuseNestedFunc->GetParseableFunctionInfo());
                    autoRestoreFunctionInfo.funcBody = parsedFunctionBody;
                    parseableFunctionInfo = parsedFunctionBody;
                }
                else
                {
                    parseableFunctionInfo = reuseNestedFunc->GetFunctionBody();
                }
                Assert((parseableFunctionInfo->GetAttributes() & StableFunctionInfoAttributesMask) == (attributes & StableFunctionInfoAttributesMask));
            }
            else
            {
                parseableFunctionInfo = Js::FunctionBody::NewFromRecycler(scriptContext, name, nameLength, shortNameOffset, pnodeFnc->nestedCount, m_utf8SourceInfo,
                    m_utf8SourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId, functionId
                    , attributes
                    , pnodeFnc->IsClassConstructor() ?
                        Js::FunctionBody::FunctionBodyFlags::Flags_None :
                        Js::FunctionBody::FunctionBodyFlags::Flags_HasNoExplicitReturnValue
#ifdef PERF_COUNTERS
                    , false /* is function from deferred deserialized proxy */
#endif
                );
            }
        }
        else
        {
            if (reuseNestedFunc)
            {
                Assert(!reuseNestedFunc->IsFunctionBody() || reuseNestedFunc->GetFunctionBody()->GetByteCode() != nullptr);
                Assert(pnodeFnc->pnodeBody == nullptr);
                parseableFunctionInfo = reuseNestedFunc;
                Assert((parseableFunctionInfo->GetAttributes() & StableFunctionInfoAttributesMask) == (attributes & StableFunctionInfoAttributesMask));
            }
            else
            {
                parseableFunctionInfo = Js::ParseableFunctionInfo::New(scriptContext, pnodeFnc->nestedCount, functionId, m_utf8SourceInfo, name, nameLength, shortNameOffset, attributes,
                                        pnodeFnc->IsClassConstructor() ?
                                            Js::FunctionBody::FunctionBodyFlags::Flags_None :
                                            Js::FunctionBody::FunctionBodyFlags::Flags_HasNoExplicitReturnValue);
            }
        }

        // In either case register the function reference
        scriptContext->GetLibrary()->RegisterDynamicFunctionReference(parseableFunctionInfo);

#if DBG
        parseableFunctionInfo->deferredParseNextFunctionId = pnodeFnc->deferredParseNextFunctionId;
#endif
        parseableFunctionInfo->SetIsDeclaration(pnodeFnc->IsDeclaration() != 0);
        parseableFunctionInfo->SetIsMethod(pnodeFnc->IsMethod() != 0);
        parseableFunctionInfo->SetIsAccessor(pnodeFnc->IsAccessor() != 0);
        if (pnodeFnc->IsAccessor())
        {
            scriptContext->optimizationOverrides.SetSideEffects(Js::SideEffects_Accessor);
        }
    }

    Scope *funcExprScope = nullptr;
    if (funcExprWithName)
    {
        funcExprScope = pnodeFnc->scope;
        Assert(funcExprScope);
        PushScope(funcExprScope);
        Symbol *sym = AddSymbolToScope(funcExprScope, name, nameLength, pnodeFnc->pnodeName, STFunction);

        sym->SetIsFuncExpr(true);

        sym->SetPosition(parseableFunctionInfo->GetOrAddPropertyIdTracked(sym->GetName()));

        pnodeFnc->SetFuncSymbol(sym);

        if (funcExprScope->GetIsObject())
        {
            funcExprScope->SetMustInstantiate(true);
        }
    }

    Scope *paramScope = pnodeFnc->pnodeScopes ? pnodeFnc->pnodeScopes->scope : nullptr;
    Scope *bodyScope = pnodeFnc->pnodeBodyScope ? pnodeFnc->pnodeBodyScope->scope : nullptr;
    Assert(paramScope != nullptr || !pnodeFnc->pnodeScopes);
    if (paramScope == nullptr)
    {
        paramScope = Anew(alloc, Scope, alloc, ScopeType_Parameter, true);
        if (pnodeFnc->pnodeScopes)
        {
            pnodeFnc->pnodeScopes->scope = paramScope;
        }
    }
    if (bodyScope == nullptr)
    {
        bodyScope = Anew(alloc, Scope, alloc, ScopeType_FunctionBody, true);
        if (pnodeFnc->pnodeBodyScope)
        {
            pnodeFnc->pnodeBodyScope->scope = bodyScope;
        }
    }

    AssertMsg(pnodeFnc->nop == knopFncDecl, "Non-function declaration trying to create function body");

    parseableFunctionInfo->SetIsGlobalFunc(false);
    if (pnodeFnc->GetStrictMode() != 0)
    {
        parseableFunctionInfo->SetIsStrictMode();
    }

    FuncInfo *funcInfo = Anew(alloc, FuncInfo, name, alloc, this, paramScope, bodyScope, pnodeFnc, parseableFunctionInfo);

#if DBG
    funcInfo->isReused = (reuseNestedFunc != nullptr);
#endif

    if (pnodeFnc->GetArgumentsObjectEscapes())
    {
        // If the parser detected that the arguments object escapes, then the function scope escapes
        // and cannot be cached.
        this->FuncEscapes(bodyScope);
        funcInfo->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("ArgumentsObjectEscapes")));
    }

    if (parseableFunctionInfo->IsFunctionBody())
    {
        Js::FunctionBody * parsedFunctionBody = parseableFunctionInfo->GetFunctionBody();
        if (parsedFunctionBody->IsReparsed())
        {
            parsedFunctionBody->RestoreState(pnodeFnc);
        }
        else
        {
            parsedFunctionBody->SaveState(pnodeFnc);
        }
    }

    funcInfo->SetChildCallsEval(!!pnodeFnc->ChildCallsEval());

    if (pnodeFnc->CallsEval())
    {
        funcInfo->SetCallsEval(true);

        bodyScope->SetIsDynamic(true);
        bodyScope->SetIsObject();
        bodyScope->SetCapturesAll(true);
        bodyScope->SetMustInstantiate(true);

        // Do not mark param scope as dynamic as it does not leak declarations
        paramScope->SetIsObject();
        paramScope->SetCapturesAll(true);
        paramScope->SetMustInstantiate(true);
    }

    PushFuncInfo(_u("StartBindFunction"), funcInfo);

    if (funcExprScope)
    {
        funcExprScope->SetFunc(funcInfo);
        funcInfo->funcExprScope = funcExprScope;
    }

    int32 currentAstSize = pnodeFnc->astSize;
    if (currentAstSize > this->maxAstSize)
    {
        this->maxAstSize = currentAstSize;
    }

    autoRestoreFunctionInfo.Clear();

    if (!pnodeFnc->IsBodyAndParamScopeMerged())
    {
        funcInfo->ResetBodyAndParamScopeMerged();
    }

    return funcInfo;
}

void ByteCodeGenerator::EndBindFunction(bool funcExprWithName)
{
    bool isGlobalScope = currentScope->GetScopeType() == ScopeType_Global;

    Assert(currentScope->GetScopeType() == ScopeType_FunctionBody || isGlobalScope);
    PopScope(); // function body

    if (isGlobalScope)
    {
        Assert(currentScope == nullptr);
    }
    else
    {
        Assert(currentScope->GetScopeType() == ScopeType_Parameter);
        PopScope(); // parameter scope
    }

    if (funcExprWithName)
    {
        Assert(currentScope->GetScopeType() == ScopeType_FuncExpr);
        PopScope();
    }

    funcInfoStack->Pop();
}

void ByteCodeGenerator::StartBindCatch(ParseNode *pnode)
{
    Scope *scope = pnode->AsParseNodeCatch()->scope;
    Assert(scope);
    Assert(currentScope);
    scope->SetFunc(currentScope->GetFunc());
    PushScope(scope);
}

void ByteCodeGenerator::EndBindCatch()
{
    PopScope();
}

void ByteCodeGenerator::PushScope(Scope *innerScope)
{
    Assert(innerScope != nullptr);

    innerScope->SetEnclosingScope(currentScope);

    currentScope = innerScope;

    if (currentScope->GetIsDynamic())
    {
        this->dynamicScopeCount++;
    }

    if (this->trackEnvDepth && currentScope->GetMustInstantiate())
    {
        this->envDepth++;
        if (this->envDepth == 0)
        {
            Js::Throw::OutOfMemory();
        }
    }
}

void ByteCodeGenerator::PopScope()
{
    Assert(currentScope != nullptr);
    if (this->trackEnvDepth && currentScope->GetMustInstantiate())
    {
        this->envDepth--;
        Assert(this->envDepth != (uint16)-1);
    }
    if (currentScope->GetIsDynamic())
    {
        this->dynamicScopeCount--;
    }
    currentScope = currentScope->GetEnclosingScope();
}

void ByteCodeGenerator::PushBlock(ParseNodeBlock *pnode)
{
    pnode->SetEnclosingBlock(currentBlock);
    currentBlock = pnode;
}

void ByteCodeGenerator::PopBlock()
{
    currentBlock = currentBlock->GetEnclosingBlock();
}

void ByteCodeGenerator::PushFuncInfo(char16 const * location, FuncInfo* funcInfo)
{
    // We might have multiple global scope for deferparse.
    // Assert(!funcInfo->IsGlobalFunction() || this->TopFuncInfo() == nullptr || this->TopFuncInfo()->IsGlobalFunction());
    if (PHASE_TRACE1(Js::ByteCodePhase))
    {
        Output::Print(_u("%s: PushFuncInfo: %s"), location, funcInfo->name);
        if (this->TopFuncInfo())
        {
            Output::Print(_u(" Top: %s"), this->TopFuncInfo()->name);
        }
        Output::Print(_u("\n"));
        Output::Flush();
    }
    funcInfoStack->Push(funcInfo);
}

void ByteCodeGenerator::PopFuncInfo(char16 const * location)
{
    FuncInfo * funcInfo = funcInfoStack->Pop();
    // Assert(!funcInfo->IsGlobalFunction() || this->TopFuncInfo() == nullptr || this->TopFuncInfo()->IsGlobalFunction());
    if (PHASE_TRACE1(Js::ByteCodePhase))
    {
        Output::Print(_u("%s: PopFuncInfo: %s"), location, funcInfo->name);
        if (this->TopFuncInfo())
        {
            Output::Print(_u(" Top: %s"), this->TopFuncInfo()->name);
        }
        Output::Print(_u("\n"));
        Output::Flush();
    }
}

Symbol * ByteCodeGenerator::FindSymbol(Symbol **symRef, IdentPtr pid, bool forReference)
{
    const char16 *key = nullptr;

    Symbol *sym = nullptr;
    Assert(symRef);
    if (*symRef)
    {
        sym = *symRef;
    }
    else
    {
        this->AssignPropertyId(pid);
        return nullptr;
    }
    key = reinterpret_cast<const char16*>(sym->GetPid()->Psz());

    Scope *symScope = sym->GetScope();
    Assert(symScope);

#if DBG_DUMP
    if (this->Trace())
    {
        if (sym != nullptr)
        {
            Output::Print(_u("resolved %s to symbol of type %s: \n"), key, sym->GetSymbolTypeName());
        }
        else
        {
            Output::Print(_u("did not resolve %s\n"), key);
        }
    }
#endif

    if (!sym->GetIsGlobal() && !sym->GetIsModuleExportStorage())
    {
        FuncInfo *top = funcInfoStack->Top();

        bool nonLocalRef = symScope->GetFunc() != top;
        Scope *scope = nullptr;
        if (forReference)
        {
            Js::PropertyId i;
            scope = FindScopeForSym(symScope, nullptr, &i, top);
            // If we have a reference to a local within a with, we want to generate a closure represented by an object.
            if (scope != symScope && scope->GetIsDynamic())
            {
                nonLocalRef = true;
                sym->SetHasNonLocalReference();
                symScope->SetIsObject();
            }
        }

        // This may not be a non-local reference, but the symbol may still be accessed non-locally. ('with', e.g.)
        // In that case, make sure we still process the symbol and its scope for closure capture.
        if (nonLocalRef || sym->GetHasNonLocalReference())
        {
            // Symbol referenced through a closure. Mark it as such and give it a property ID.
            this->ProcessCapturedSym(sym);
            sym->SetPosition(top->byteCodeFunction->GetOrAddPropertyIdTracked(sym->GetName()));
            // If this is var is local to a function (meaning that it belongs to the function's scope
            // *or* to scope that need not be instantiated, like a function expression scope, which we'll
            // merge with the function scope, then indicate that fact.
            this->ProcessScopeWithCapturedSym(symScope);
            if (symScope->GetFunc()->GetHasArguments() && sym->GetIsFormal())
            {
                // A formal is referenced non-locally. We need to allocate it on the heap, so
                // do the same for the whole arguments object.

                // Formal is referenced. So count of formals to function > 0.
                // So no need to check for inParams here.

                symScope->GetFunc()->SetHasHeapArguments(true);
            }
            if (symScope->GetFunc() != top)
            {
                top->SetHasClosureReference(true);
            }
        }
        else if (!nonLocalRef && sym->GetHasNonLocalReference() && !sym->GetIsCommittedToSlot() && !sym->HasVisitedCapturingFunc())
        {
            sym->SetHasNonCommittedReference(true);
        }

        if (sym->GetIsFuncExpr())
        {
            symScope->GetFunc()->SetFuncExprNameReference(true);
        }
    }

    return sym;
}

Symbol * ByteCodeGenerator::AddSymbolToScope(Scope *scope, const char16 *key, int keyLength, ParseNode *varDecl, SymbolType symbolType)
{
    Symbol *sym = nullptr;

    switch (varDecl->nop)
    {
    case knopConstDecl:
    case knopLetDecl:
    case knopVarDecl:
        sym = varDecl->AsParseNodeVar()->sym;
        break;
    case knopName:
        AnalysisAssert(varDecl->AsParseNodeName()->GetSymRef());
        sym = *varDecl->AsParseNodeName()->GetSymRef();
        break;
    default:
        AnalysisAssert(0);
        sym = nullptr;
        break;
    }

    if (sym->GetScope() != scope && sym->GetScope()->GetScopeType() != ScopeType_Parameter)
    {
        // This can happen when we have a function declared at global eval scope, and it has
        // references in deferred function bodies inside the eval. The BCG creates a new global scope
        // on such compiles, so we essentially have to migrate the symbol to the new scope.
        // We check fscrEvalCode, not fscrEval, because the same thing can happen in indirect eval,
        // when fscrEval is not set.
        Assert(scope->GetScopeType() == ScopeType_Global || scope->GetScopeType() == ScopeType_GlobalEvalBlock);
        scope->AddNewSymbol(sym);
    }

    Assert(sym && sym->GetScope() && (sym->GetScope() == scope || sym->GetScope()->GetScopeType() == ScopeType_Parameter));

    if (sym->NeedsScopeObject())
    {
        scope->SetIsObject();
    }

    return sym;
}

Symbol * ByteCodeGenerator::AddSymbolToFunctionScope(const char16 *key, int keyLength, ParseNode *varDecl, SymbolType symbolType)
{
    Scope* scope = currentScope->GetFunc()->GetBodyScope();
    return this->AddSymbolToScope(scope, key, keyLength, varDecl, symbolType);
}

FuncInfo *ByteCodeGenerator::FindEnclosingNonLambda()
{
    for (Scope *scope = GetCurrentScope(); scope; scope = scope->GetEnclosingScope())
    {
        if (!scope->GetFunc()->IsLambda())
        {
            return scope->GetFunc();
        }
    }
    Assert(0);
    return nullptr;
}

FuncInfo* GetParentFuncInfo(FuncInfo* child)
{
    for (Scope* scope = child->GetBodyScope(); scope; scope = scope->GetEnclosingScope())
    {
        if (scope->GetFunc() != child)
        {
            return scope->GetFunc();
        }
    }
    Assert(0);
    return nullptr;
}

bool ByteCodeGenerator::CanStackNestedFunc(FuncInfo * funcInfo, bool trace)
{
#if ENABLE_DEBUG_CONFIG_OPTIONS
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
#endif
    Assert(!funcInfo->IsGlobalFunction());
    bool const doStackNestedFunc = !funcInfo->HasMaybeEscapedNestedFunc() && !IsInDebugMode()
        && !funcInfo->byteCodeFunction->IsCoroutine()
        && !funcInfo->byteCodeFunction->IsModule();
    if (!doStackNestedFunc)
    {
        return false;
    }

    bool callsEval = funcInfo->GetCallsEval() || funcInfo->GetChildCallsEval();
    if (callsEval)
    {
        if (trace)
        {
            PHASE_PRINT_TESTTRACE(Js::StackFuncPhase, funcInfo->byteCodeFunction,
                _u("HasMaybeEscapedNestedFunc (Eval): %s (function %s)\n"),
                funcInfo->byteCodeFunction->GetDisplayName(),
                funcInfo->byteCodeFunction->GetDebugNumberSet(debugStringBuffer));
        }
        return false;
    }

    if (funcInfo->GetBodyScope()->GetIsObject() || funcInfo->GetParamScope()->GetIsObject() || (funcInfo->GetFuncExprScope() && funcInfo->GetFuncExprScope()->GetIsObject()))
    {
        if (trace)
        {
            PHASE_PRINT_TESTTRACE(Js::StackFuncPhase, funcInfo->byteCodeFunction,
                _u("HasMaybeEscapedNestedFunc (ObjectScope): %s (function %s)\n"),
                funcInfo->byteCodeFunction->GetDisplayName(),
                funcInfo->byteCodeFunction->GetDebugNumberSet(debugStringBuffer));
        }
        return false;
    }

    if (!funcInfo->IsBodyAndParamScopeMerged())
    {
        if (trace)
        {
            PHASE_PRINT_TESTTRACE(Js::StackFuncPhase, funcInfo->byteCodeFunction,
                _u("CanStackNestedFunc: %s (Split Scope)\n"),
                funcInfo->byteCodeFunction->GetDisplayName());
        }
        return false;
    }

    if (trace && funcInfo->byteCodeFunction->GetNestedCount())
    {
        // Only print functions that actually have nested functions, although we will still mark
        // functions that don't have nested child functions as DoStackNestedFunc.
        PHASE_PRINT_TESTTRACE(Js::StackFuncPhase, funcInfo->byteCodeFunction,
            _u("DoStackNestedFunc: %s (function %s)\n"),
            funcInfo->byteCodeFunction->GetDisplayName(),
            funcInfo->byteCodeFunction->GetDebugNumberSet(debugStringBuffer));
    }

    return !PHASE_OFF(Js::StackFuncPhase, funcInfo->byteCodeFunction);
}

bool ByteCodeGenerator::NeedObjectAsFunctionScope(FuncInfo * funcInfo, ParseNodeFnc * pnodeFnc) const
{
    return funcInfo->GetCallsEval()
        || funcInfo->GetChildCallsEval()
        || NeedScopeObjectForArguments(funcInfo, pnodeFnc)
        || (this->flags & (fscrEval | fscrImplicitThis));
}

Scope * ByteCodeGenerator::FindScopeForSym(Scope *symScope, Scope *scope, Js::PropertyId *envIndex, FuncInfo *funcInfo) const
{
    for (scope = scope ? scope->GetEnclosingScope() : currentScope; scope; scope = scope->GetEnclosingScope())
    {
        if (scope->GetFunc() != funcInfo
            && scope->GetMustInstantiate()
            && scope != this->globalScope)
        {
            (*envIndex)++;
        }
        if (scope == symScope || scope->GetIsDynamic())
        {
            break;
        }
    }

    Assert(scope);
    return scope;
}

/* static */
Js::OpCode ByteCodeGenerator::GetStFldOpCode(FuncInfo* funcInfo, bool isRoot, bool isLetDecl, bool isConstDecl, bool isClassMemberInit, bool forceStrictModeForClassComputedPropertyName)
{
    return GetStFldOpCode(funcInfo->GetIsStrictMode() || forceStrictModeForClassComputedPropertyName, isRoot, isLetDecl, isConstDecl, isClassMemberInit);
}

/* static */
Js::OpCode ByteCodeGenerator::GetScopedStFldOpCode(FuncInfo* funcInfo, bool isConsoleScopeLetConst)
{
    return GetScopedStFldOpCode(funcInfo->GetIsStrictMode(), isConsoleScopeLetConst);
}

/* static */
Js::OpCode ByteCodeGenerator::GetStElemIOpCode(FuncInfo* funcInfo)
{
    return GetStElemIOpCode(funcInfo->GetIsStrictMode());
}

bool ByteCodeGenerator::DoJitLoopBodies(FuncInfo *funcInfo) const
{
    // Never JIT loop bodies in a function with a try.
    // Otherwise, always JIT loop bodies under /forcejitloopbody.
    // Otherwise, JIT loop bodies unless we're in eval/"new Function" or feature is disabled.

    Assert(funcInfo->byteCodeFunction->IsFunctionParsed());
    Js::FunctionBody* functionBody = funcInfo->byteCodeFunction->GetFunctionBody();

    return functionBody->ForceJITLoopBody() || funcInfo->byteCodeFunction->IsJitLoopBodyPhaseEnabled();
}

void ByteCodeGenerator::Generate(__in ParseNodeProg *pnodeProg, uint32 grfscr, __in ByteCodeGenerator* byteCodeGenerator,
    __inout Js::ParseableFunctionInfo ** ppRootFunc, __in uint sourceIndex,
    __in bool forceNoNative, __in Parser* parser, Js::ScriptFunction **functionRef)
{
#if DBG
    struct WalkerPolicyTest : public WalkerPolicyBase<bool, ParseNodeWalker<WalkerPolicyTest>*>
    {
        inline bool ContinueWalk(ResultType) { return ThreadContext::IsCurrentStackAvailable(Js::Constants::MinStackByteCodeVisitor); }
        virtual ResultType WalkChild(ParseNode *pnode, ParseNodeWalker<WalkerPolicyTest>* walker) { return ContinueWalk(true) && walker->Walk(pnode, walker); }
    };
    ParseNodeWalker<WalkerPolicyTest> walker;
    // Just walk the ast to see if our walker encounters any problems
    walker.Walk(pnodeProg, &walker);
#endif
    Js::ScriptContext * scriptContext = byteCodeGenerator->scriptContext;

#ifdef PROFILE_EXEC
    scriptContext->ProfileBegin(Js::ByteCodePhase);
#endif
    JS_ETW_INTERNAL(EventWriteJSCRIPT_BYTECODEGEN_START(scriptContext, 0));

    ThreadContext * threadContext = scriptContext->GetThreadContext();
    Js::Utf8SourceInfo * utf8SourceInfo = scriptContext->GetSource(sourceIndex);
    byteCodeGenerator->m_utf8SourceInfo = utf8SourceInfo;

    // For dynamic code, just provide a small number since that source info should have very few functions
    // For static code, the nextLocalFunctionId is a good guess of the initial size of the array to minimize reallocs
    SourceContextInfo * sourceContextInfo = utf8SourceInfo->GetSrcInfo()->sourceContextInfo;
    utf8SourceInfo->EnsureInitialized((grfscr & fscrDynamicCode) ? 4 : (sourceContextInfo->nextLocalFunctionId - pnodeProg->functionId));
    sourceContextInfo->EnsureInitialized();

    ArenaAllocator localAlloc(_u("ByteCode"), threadContext->GetPageAllocator(), Js::Throw::OutOfMemory);

    // Make sure FuncInfo's get finalized when byte code gen is done.
    struct AutoFinalizeFuncInfos {
        AutoFinalizeFuncInfos(ByteCodeGenerator * byteCodeGenerator) : byteCodeGenerator(byteCodeGenerator) {}
        ~AutoFinalizeFuncInfos() {
            if (byteCodeGenerator)
            {
                byteCodeGenerator->FinalizeFuncInfos();
            }
        }
        ByteCodeGenerator * byteCodeGenerator;
    } autoFinalizeFuncInfos(byteCodeGenerator);

    byteCodeGenerator->parser = parser;
    byteCodeGenerator->SetCurrentSourceIndex(sourceIndex);
    byteCodeGenerator->Begin(&localAlloc, grfscr, *ppRootFunc);
    byteCodeGenerator->functionRef = functionRef;
    Visit(pnodeProg, byteCodeGenerator, Bind, AssignRegisters);

    byteCodeGenerator->forceNoNative = forceNoNative;
    byteCodeGenerator->EmitProgram(pnodeProg);

    if (byteCodeGenerator->flags & fscrEval)
    {
        // The eval caller's frame always escapes if eval refers to the caller's arguments.
        byteCodeGenerator->GetRootFunc()->GetFunctionBody()->SetFuncEscapes(
            byteCodeGenerator->funcEscapes || pnodeProg->m_UsesArgumentsAtGlobal);
    }

#ifdef IR_VIEWER
    if (grfscr & fscrIrDumpEnable)
    {
        byteCodeGenerator->GetRootFunc()->GetFunctionBody()->SetIRDumpEnabled(true);
    }
#endif /* IR_VIEWER */

    byteCodeGenerator->CheckDeferParseHasMaybeEscapedNestedFunc();

#ifdef PROFILE_EXEC
    scriptContext->ProfileEnd(Js::ByteCodePhase);
#endif
    JS_ETW_INTERNAL(EventWriteJSCRIPT_BYTECODEGEN_STOP(scriptContext, 0));

#if ENABLE_NATIVE_CODEGEN && defined(ENABLE_PREJIT)
    if (!byteCodeGenerator->forceNoNative && !scriptContext->GetConfig()->IsNoNative()
        && Js::Configuration::Global.flags.Prejit
        && (grfscr & fscrNoPreJit) == 0)
    {
        GenerateAllFunctions(scriptContext->GetNativeCodeGenerator(), byteCodeGenerator->GetRootFunc()->GetFunctionBody());
    }
#endif

    if (ppRootFunc)
    {
        *ppRootFunc = byteCodeGenerator->GetRootFunc();
    }

#ifdef PERF_COUNTERS
    PHASE_PRINT_TESTTRACE1(Js::DeferParsePhase, _u("TestTrace: deferparse - # of func: %d # deferparsed: %d\n"),
        PerfCounter::CodeCounterSet::GetTotalFunctionCounter().GetValue(), PerfCounter::CodeCounterSet::GetDeferredFunctionCounter().GetValue());
#endif
}

void ByteCodeGenerator::CheckDeferParseHasMaybeEscapedNestedFunc()
{
    if (!this->parentScopeInfo)
    {
        return;
    }

    Assert(this->funcInfoStack && !this->funcInfoStack->Empty());

    // Box the stack nested function if we detected new may be escaped use function.
    SList<FuncInfo *>::Iterator i(this->funcInfoStack);
    bool succeed = i.Next();
    Assert(succeed);
    Assert(i.Data()->IsGlobalFunction()); // We always leave a glo on type when defer parsing.
    Assert(!i.Data()->IsRestored());
    succeed = i.Next();
    FuncInfo * top = i.Data();

    Assert(!top->IsGlobalFunction());
    Assert(top->IsRestored());
    Js::FunctionBody * rootFuncBody = this->GetRootFunc()->GetFunctionBody();
    if (!rootFuncBody->DoStackNestedFunc())
    {
        top->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("DeferredChild")));
    }
    else
    {
        // We have to wait until it is parsed before we populate the stack nested func parent.
        FuncInfo * parentFunc = top->GetParamScope() ? top->GetParamScope()->GetEnclosingFunc() : top->GetBodyScope()->GetEnclosingFunc();
        if (!parentFunc->IsGlobalFunction())
        {
            Assert(parentFunc->byteCodeFunction != rootFuncBody);
            Js::ParseableFunctionInfo * parentFunctionInfo = parentFunc->byteCodeFunction;
            if (parentFunctionInfo->DoStackNestedFunc())
            {
                rootFuncBody->SetStackNestedFuncParent(parentFunctionInfo->GetFunctionInfo());
            }
        }
    }

    do
    {
        FuncInfo * funcInfo = i.Data();
        Assert(funcInfo->IsRestored());
        Js::ParseableFunctionInfo * parseableFunctionInfo = funcInfo->byteCodeFunction;
        if (parseableFunctionInfo == nullptr)
        {
            Assert(funcInfo->GetBodyScope() && funcInfo->GetBodyScope()->GetScopeType() == ScopeType_Global);
            return;
        }
        bool didStackNestedFunc = parseableFunctionInfo->DoStackNestedFunc();
        if (!didStackNestedFunc)
        {
            return;
        }
        if (!parseableFunctionInfo->IsFunctionBody())
        {
            continue;
        }
        Js::FunctionBody * functionBody = funcInfo->GetParsedFunctionBody();
        if (funcInfo->HasMaybeEscapedNestedFunc())
        {
            // This should box the rest of the parent functions.
            if (PHASE_TESTTRACE(Js::StackFuncPhase, this->pCurrentFunction))
            {
                char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];

                Output::Print(_u("DeferParse: box and disable stack function: %s (function %s)\n"),
                    functionBody->GetDisplayName(), functionBody->GetDebugNumberSet(debugStringBuffer));
                Output::Flush();
            }

            // During the box workflow we reset all the parents of all nested functions and up. If a fault occurs when the stack function
            // is created this will cause further issues when trying to use the function object again. So failing faster seems to make more sense.
            try
            {
                Js::StackScriptFunction::Box(functionBody, functionRef);
            }
            catch (Js::OutOfMemoryException)
            {
                FailedToBox_OOM_unrecoverable_error((ULONG_PTR)functionBody);
            }

            return;
        }
    }
    while (i.Next());
}

void ByteCodeGenerator::Begin(
    __in ArenaAllocator *alloc,
    __in uint32 grfscr,
    __in Js::ParseableFunctionInfo* pRootFunc)
{
    this->alloc = alloc;
    this->flags = grfscr;
    this->pRootFunc = pRootFunc;
    this->pCurrentFunction = pRootFunc ? pRootFunc->GetFunctionBody() : nullptr;
    if (this->pCurrentFunction && this->pCurrentFunction->GetIsGlobalFunc() && IsInNonDebugMode())
    {
        // This is the deferred parse case (not due to debug mode), in which case the global function will not be marked to compiled again.
        this->pCurrentFunction = nullptr;
    }

    this->globalScope = nullptr;
    this->currentScope = nullptr;
    this->currentBlock = nullptr;
    this->isBinding = true;
    this->inPrologue = false;
    this->funcEscapes = false;
    this->maxAstSize = 0;
    this->loopDepth = 0;
    this->envDepth = 0;
    this->trackEnvDepth = false;
    this->funcInfosToFinalize = nullptr;

    this->funcInfoStack = Anew(alloc, SList<FuncInfo*>, alloc);
}

HRESULT GenerateByteCode(__in ParseNodeProg *pnode, __in uint32 grfscr, __in Js::ScriptContext* scriptContext, __inout Js::ParseableFunctionInfo ** ppRootFunc,
                         __in uint sourceIndex, __in bool forceNoNative, __in Parser* parser, __in CompileScriptException *pse, Js::ScopeInfo* parentScopeInfo,
                        Js::ScriptFunction ** functionRef)
{
    HRESULT hr = S_OK;
    ByteCodeGenerator byteCodeGenerator(scriptContext, parentScopeInfo);
    BEGIN_TRANSLATE_EXCEPTION_TO_HRESULT_NESTED
    {
        // Main code.
        ByteCodeGenerator::Generate(pnode, grfscr, &byteCodeGenerator, ppRootFunc, sourceIndex, forceNoNative, parser, functionRef);
    }
    END_TRANSLATE_EXCEPTION_TO_HRESULT(hr);

    if (FAILED(hr))
    {
        hr = pse->ProcessError(nullptr, hr, nullptr);
    }

    return hr;
}

void BindInstAndMember(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Assert(pnode->nop == knopDot);

    BindReference(pnode, byteCodeGenerator);

    ParseNodeName *right = pnode->AsParseNodeBin()->pnode2->AsParseNodeName();    
    byteCodeGenerator->AssignPropertyId(right->pid);
    right->sym = nullptr;
    right->ClearSymRef();
    right->grfpn |= fpnMemberReference;
}

void BindReference(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    // Do special reference-op binding so that we can, for instance, handle call from inside "with"
    // where the "this" instance must be found dynamically.

    bool isCallNode = false;
    bool funcEscapes = false;
    switch (pnode->nop)
    {
    case knopCall:
        isCallNode = true;
        pnode = pnode->AsParseNodeCall()->pnodeTarget;
        break;
    case knopDelete:
    case knopTypeof:
        pnode = pnode->AsParseNodeUni()->pnode1;
        break;
    case knopDot:
    case knopIndex:
        funcEscapes = true;
        // fall through
    case knopAsg:
        pnode = pnode->AsParseNodeBin()->pnode1;
        break;
    default:
        AssertMsg(0, "Unexpected opcode in BindReference");
        return;
    }

    if (pnode->nop == knopName)
    {
        ParseNodeName * pnodeName = pnode->AsParseNodeName();
        pnodeName->sym = byteCodeGenerator->FindSymbol(pnodeName->GetSymRef(), pnodeName->pid, isCallNode);

        if (funcEscapes &&
            pnodeName->sym &&
            pnodeName->sym->GetSymbolType() == STFunction &&
            (!pnodeName->sym->GetIsGlobal() || (byteCodeGenerator->GetFlags() & fscrEval)))
        {
            // Dot, index, and scope ops can cause a local function on the LHS to escape.
            // Make sure scopes are not cached in this case.
            byteCodeGenerator->FuncEscapes(pnodeName->sym->GetScope());
        }
    }
}

void MarkFormal(ByteCodeGenerator *byteCodeGenerator, Symbol *formal, bool assignLocation, bool needDeclaration)
{
    if (assignLocation)
    {
        formal->SetLocation(byteCodeGenerator->NextVarRegister());
    }
    if (needDeclaration)
    {
        formal->SetNeedDeclaration(true);
    }
}

void AddArgsToScope(ParseNodeFnc * pnodeFnc, ByteCodeGenerator *byteCodeGenerator, bool assignLocation)
{
    Assert(byteCodeGenerator->TopFuncInfo()->varRegsCount == 0);
    Js::ArgSlot pos = 1;
    bool isNonSimpleParameterList = pnodeFnc->HasNonSimpleParameterList();

    auto addArgToScope = [&](ParseNode *arg)
    {
        if (arg->IsVarLetOrConst())
        {
            ParseNodeVar * pnodeVarArg = arg->AsParseNodeVar();
            Symbol *formal = byteCodeGenerator->AddSymbolToScope(byteCodeGenerator->TopFuncInfo()->GetParamScope(),
                reinterpret_cast<const char16*>(pnodeVarArg->pid->Psz()),
                pnodeVarArg->pid->Cch(),
                pnodeVarArg,
                STFormal);
#if DBG_DUMP
            if (byteCodeGenerator->Trace())
            {
                Output::Print(_u("current context has declared arg %s of type %s at position %d\n"), arg->AsParseNodeVar()->pid->Psz(), formal->GetSymbolTypeName(), pos);
            }
#endif

            if (isNonSimpleParameterList)
            {
                formal->SetIsNonSimpleParameter(true);
            }

            pnodeVarArg->sym = formal;
            MarkFormal(byteCodeGenerator, formal, assignLocation || isNonSimpleParameterList, isNonSimpleParameterList);
        }
        else if (arg->nop == knopParamPattern)
        {
            arg->AsParseNodeParamPattern()->location = byteCodeGenerator->NextVarRegister();
        }
        else
        {
            Assert(false);
        }
        ArgSlotMath::Inc(pos);
    };

    // We process rest separately because the number of in args needs to exclude rest.
    MapFormalsWithoutRest(pnodeFnc, addArgToScope);
    byteCodeGenerator->SetNumberOfInArgs(pos);

    if (pnodeFnc->pnodeRest != nullptr)
    {
        // The rest parameter will always be in a register, regardless of whether it is in a scope slot.
        // We save the assignLocation value for the assert condition below.
        bool assignLocationSave = assignLocation;
        assignLocation = true;

        addArgToScope(pnodeFnc->pnodeRest);

        assignLocation = assignLocationSave;
    }

    MapFormalsFromPattern(pnodeFnc, addArgToScope);

    Assert(!assignLocation || byteCodeGenerator->TopFuncInfo()->varRegsCount + 1 == pos);
}

void AddVarsToScope(ParseNode *vars, ByteCodeGenerator *byteCodeGenerator)
{
    while (vars != nullptr)
    {
        Symbol *sym = byteCodeGenerator->AddSymbolToFunctionScope(reinterpret_cast<const char16*>(vars->AsParseNodeVar()->pid->Psz()), vars->AsParseNodeVar()->pid->Cch(), vars, STVariable);

#if DBG_DUMP
        if (sym->GetSymbolType() == STVariable && byteCodeGenerator->Trace())
        {
            Output::Print(_u("current context has declared var %s of type %s\n"),
                vars->AsParseNodeVar()->pid->Psz(), sym->GetSymbolTypeName());
        }
#endif

        if (sym->IsArguments() || sym->IsSpecialSymbol() || vars->AsParseNodeVar()->pnodeInit == nullptr)
        {
            // LHS's of var decls are usually bound to symbols later, during the Visit/Bind pass,
            // so that things like catch scopes can be taken into account.
            // The exception is "arguments", which always binds to the local scope.
            // We can also bind to the function scope symbol now if there's no init value
            // to assign.
            vars->AsParseNodeVar()->sym = sym;
            if (sym->IsArguments())
            {
                FuncInfo* funcInfo = byteCodeGenerator->TopFuncInfo();
                funcInfo->SetArgumentsSymbol(sym);
            }
            else if (sym->IsSpecialSymbol())
            {
                FuncInfo* funcInfo = byteCodeGenerator->TopFuncInfo();

                if (sym->IsThis())
                {
                    funcInfo->SetThisSymbol(sym);
                }
                else if (sym->IsNewTarget())
                {
                    funcInfo->SetNewTargetSymbol(sym);
                }
                else if (sym->IsSuper())
                {
                    funcInfo->SetSuperSymbol(sym);
                }
                else if (sym->IsSuperConstructor())
                {
                    funcInfo->SetSuperConstructorSymbol(sym);
                }
            }
        }
        else
        {
            vars->AsParseNodeVar()->sym = nullptr;
        }
        vars = vars->AsParseNodeVar()->pnodeNext;
    }
}

template <class Fn>
void VisitFncDecls(ParseNode *fns, Fn action)
{
    while (fns != nullptr)
    {
        switch (fns->nop)
        {
        case knopFncDecl:
            action(fns);
            fns = fns->AsParseNodeFnc()->pnodeNext;
            break;

        case knopBlock:
            fns = fns->AsParseNodeBlock()->pnodeNext;
            break;

        case knopCatch:
            fns = fns->AsParseNodeCatch()->pnodeNext;
            break;

        case knopWith:
            fns = fns->AsParseNodeWith()->pnodeNext;
            break;

        default:
            AssertMsg(false, "Unexpected opcode in tree of scopes");
            return;
        }
    }
}

FuncInfo* PreVisitFunction(ParseNodeFnc* pnodeFnc, ByteCodeGenerator* byteCodeGenerator, Js::ParseableFunctionInfo *reuseNestedFunc)
{
    // Do binding of function name(s), initialize function scope, propagate function-wide properties from
    // the parent (if any).
    FuncInfo* parentFunc = byteCodeGenerator->TopFuncInfo();

    // fIsRoot indicates that this is the root function to be returned to a ParseProcedureText/AddScriptLet/etc. call.
    // In such cases, the global function is just a wrapper around the root function's declaration.
    // We used to assert that this was the only top-level function body, but it's possible to trick
    // "new Function" into compiling more than one function (see WOOB 1121759).
    bool fIsRoot = (!(byteCodeGenerator->GetFlags() & fscrGlobalCode) &&
        parentFunc->IsGlobalFunction() &&
        parentFunc->root->GetTopLevelScope() == pnodeFnc);

    const char16 *funcName = Js::Constants::AnonymousFunction;
    uint funcNameLength = Js::Constants::AnonymousFunctionLength;
    uint functionNameOffset = 0;
    bool funcExprWithName = false;

    if (pnodeFnc->hint != nullptr)
    {
        funcName = reinterpret_cast<const char16*>(pnodeFnc->hint);
        funcNameLength = pnodeFnc->hintLength;
        functionNameOffset = pnodeFnc->hintOffset;
        Assert(funcNameLength != 0 || funcNameLength == (int)wcslen(funcName));
    }
    if (pnodeFnc->IsDeclaration() || pnodeFnc->IsMethod())
    {
        // Class members have the fully qualified name stored in 'hint', no need to replace it.
        if (pnodeFnc->pid && !pnodeFnc->IsClassMember())
        {
            funcName = reinterpret_cast<const char16*>(pnodeFnc->pid->Psz());
            funcNameLength = pnodeFnc->pid->Cch();
            functionNameOffset = 0;
        }
    }
    else if (pnodeFnc->pnodeName != nullptr)
    {
        Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
        funcName = reinterpret_cast<const char16*>(pnodeFnc->pnodeName->pid->Psz());
        funcNameLength = pnodeFnc->pnodeName->pid->Cch();
        functionNameOffset = 0;
        //
        // create the new scope for Function expression only in ES5 mode
        //
        funcExprWithName = true;
    }

    if (byteCodeGenerator->Trace())
    {
        Output::Print(_u("function start %s\n"), funcName);
    }

    Assert(pnodeFnc->funcInfo == nullptr);
    FuncInfo* funcInfo = pnodeFnc->funcInfo = byteCodeGenerator->StartBindFunction(funcName, funcNameLength, functionNameOffset, &funcExprWithName, pnodeFnc, reuseNestedFunc);
    funcInfo->byteCodeFunction->SetIsNamedFunctionExpression(funcExprWithName);
    funcInfo->byteCodeFunction->SetIsNameIdentifierRef(pnodeFnc->isNameIdentifierRef);

    if (fIsRoot)
    {
        byteCodeGenerator->SetRootFuncInfo(funcInfo);
    }

    if (pnodeFnc->pnodeBody == nullptr)
    {
        // This is a deferred byte code gen, so we're done.
        // Process the formal arguments, even if there's no AST for the body, to support Function.length.
        Js::ArgSlot pos = 1;
        // We skip the rest parameter here because it is not counted towards the in arg count.
        MapFormalsWithoutRest(pnodeFnc, [&](ParseNode *pnode) { ArgSlotMath::Inc(pos); });
        byteCodeGenerator->SetNumberOfInArgs(pos);
        return funcInfo;
    }

    if (pnodeFnc->HasReferenceableBuiltInArguments())
    {
        // The parser identified that there is a way to reference the built-in 'arguments' variable from this function. So, we
        // need to determine whether we need to create the variable or not. We need to create the variable iff:
        if (pnodeFnc->CallsEval())
        {
            // 1. eval is called.
            // 2. when the debugging is enabled, since user can seek arguments during breakpoint.
            funcInfo->SetHasArguments(true);
            funcInfo->SetHasHeapArguments(true);
            if (funcInfo->inArgsCount == 0)
            {
                // If no formals to function, no need to create the propertyid array
                byteCodeGenerator->AssignNullConstRegister();
            }
        }
        else if (pnodeFnc->UsesArguments())
        {
            // 3. the function directly references an 'arguments' identifier
            funcInfo->SetHasArguments(true);
            funcInfo->GetParsedFunctionBody()->SetUsesArgumentsObject(true);
            if (pnodeFnc->HasHeapArguments())
            {
                bool doStackArgsOpt = (!pnodeFnc->HasAnyWriteToFormals() || funcInfo->GetIsStrictMode());
#ifdef PERF_HINT
                if (PHASE_TRACE1(Js::PerfHintPhase) && !doStackArgsOpt)
                {
                    WritePerfHint(PerfHints::HeapArgumentsDueToWriteToFormals, funcInfo->GetParsedFunctionBody(), 0);
                }
#endif

                //With statements - need scope object to be present.
                if ((doStackArgsOpt && pnodeFnc->funcInfo->GetParamScope()->Count() > 1) && ((byteCodeGenerator->GetFlags() & fscrEval) ||
                    pnodeFnc->HasWithStmt() || byteCodeGenerator->IsInDebugMode() || PHASE_OFF1(Js::StackArgFormalsOptPhase) || PHASE_OFF1(Js::StackArgOptPhase)))
                {
                    doStackArgsOpt = false;
#ifdef PERF_HINT
                    if (PHASE_TRACE1(Js::PerfHintPhase))
                    {
                        if (pnodeFnc->HasWithStmt())
                        {
                            WritePerfHint(PerfHints::HasWithBlock, funcInfo->GetParsedFunctionBody(), 0);
                        }

                        if(byteCodeGenerator->GetFlags() & fscrEval)
                        {
                            WritePerfHint(PerfHints::SrcIsEval, funcInfo->GetParsedFunctionBody(), 0);
                        }
                    }
#endif
                }
                funcInfo->SetHasHeapArguments(true, !pnodeFnc->IsCoroutine() && doStackArgsOpt /*= Optimize arguments in backend*/);
                if (funcInfo->inArgsCount == 0)
                {
                    // If no formals to function, no need to create the propertyid array
                    byteCodeGenerator->AssignNullConstRegister();
                }
            }
        }
    }

    Js::FunctionBody* parentFunctionBody = parentFunc->GetParsedFunctionBody();
    if (funcInfo->GetHasArguments() ||
        parentFunctionBody->GetHasOrParentHasArguments())
    {
        // The JIT uses this info, for instance, to narrow kills of array operations
        funcInfo->GetParsedFunctionBody()->SetHasOrParentHasArguments(true);
    }
    PreVisitBlock(pnodeFnc->pnodeScopes, byteCodeGenerator);
    // If we have arguments, we are going to need locations if the function is in strict mode or we have a non-simple parameter list. This is because we will not create a scope object.
    bool assignLocationForFormals = !byteCodeGenerator->NeedScopeObjectForArguments(funcInfo, funcInfo->root);
    AddArgsToScope(pnodeFnc, byteCodeGenerator, assignLocationForFormals);

    return funcInfo;
}

void AssignFuncSymRegister(ParseNodeFnc * pnodeFnc, ByteCodeGenerator * byteCodeGenerator, FuncInfo * callee)
{
    // register to hold the allocated function (in enclosing sequence of global statements)
    // TODO: Make the parser identify uses of function decls as RHS's of expressions.
    // Currently they're all marked as used, so they all get permanent (non-temp) registers.
    if (pnodeFnc->pnodeName == nullptr)
    {
        return;
    }
    Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
    Symbol *sym = pnodeFnc->pnodeName->sym;
    if (sym)
    {
        if (!sym->GetIsGlobal() && !(callee->funcExprScope && callee->funcExprScope->GetIsObject()))
        {
            // If the func decl is used, we have to give the expression a register to protect against:
            // x.x = function f() {...};
            // x.y = function f() {...};
            // If we let the value reside in the local slot for f, then both assignments will get the
            // second definition.
            if (!pnodeFnc->IsDeclaration())
            {
                // A named function expression's name belongs to the enclosing scope.
                // In ES5 mode, it is visible only inside the inner function.
                // Allocate a register for the 'name' symbol from an appropriate register namespace.
                if (callee->GetFuncExprNameReference())
                {
                    // This is a function expression with a name, but probably doesn't have a use within
                    // the function. If that is the case then allocate a register for LdFuncExpr inside the function
                    // we just finished post-visiting.
                    if (sym->GetLocation() == Js::Constants::NoRegister)
                    {
                        sym->SetLocation(callee->NextVarRegister());
                    }
                }
            }
            else
            {
                // Function declaration
                byteCodeGenerator->AssignRegister(sym);
                pnodeFnc->location = sym->GetLocation();

                Assert(byteCodeGenerator->GetCurrentScope()->GetFunc() == sym->GetScope()->GetFunc());
                if (byteCodeGenerator->GetCurrentScope()->GetFunc() != sym->GetScope()->GetFunc())
                {
                    Assert(GetParentFuncInfo(byteCodeGenerator->GetCurrentScope()->GetFunc()) == sym->GetScope()->GetFunc());
                    sym->GetScope()->SetMustInstantiate(true);
                    byteCodeGenerator->ProcessCapturedSym(sym);
                    sym->GetScope()->GetFunc()->SetHasLocalInClosure(true);
                }

                Symbol * functionScopeVarSym = sym->GetFuncScopeVarSym();
                if (functionScopeVarSym &&
                    !functionScopeVarSym->GetIsGlobal() &&
                    !functionScopeVarSym->IsInSlot(byteCodeGenerator, sym->GetScope()->GetFunc()))
                {
                    byteCodeGenerator->AssignRegister(functionScopeVarSym);
                }
            }
        }
        else if (!pnodeFnc->IsDeclaration())
        {
            if (sym->GetLocation() == Js::Constants::NoRegister)
            {
                // Here, we are assigning a register for the LdFuncExpr instruction inside the function we just finished
                // post-visiting. The symbol is given a register from the register pool for the function we just finished
                // post-visiting, rather than from the parent function's register pool.
                sym->SetLocation(callee->NextVarRegister());
            }
        }
    }
}

bool FuncAllowsDirectSuper(FuncInfo *funcInfo, ByteCodeGenerator *byteCodeGenerator)
{
    if (!funcInfo->IsBaseClassConstructor() && funcInfo->IsClassConstructor())
    {
        return true;
    }

    if (funcInfo->IsGlobalFunction() && ((byteCodeGenerator->GetFlags() & fscrEval) != 0))
    {
        Js::JavascriptFunction *caller = nullptr;
        if (Js::JavascriptStackWalker::GetCaller(&caller, byteCodeGenerator->GetScriptContext()) && caller->GetFunctionInfo()->GetAllowDirectSuper())
        {
            return true;
        }
    }

    return false;
}

FuncInfo* PostVisitFunction(ParseNodeFnc* pnodeFnc, ByteCodeGenerator* byteCodeGenerator)
{
    // Assign function-wide registers such as local frame object, closure environment, etc., based on
    // observed attributes. Propagate attributes to the parent function (if any).
    FuncInfo *top = byteCodeGenerator->TopFuncInfo();
    Symbol *sym = pnodeFnc->GetFuncSymbol();
    bool funcExprWithName = !top->IsGlobalFunction() && sym && sym->GetIsFuncExpr();

    if (top->IsLambda())
    {
        FuncInfo *enclosingNonLambda = byteCodeGenerator->FindEnclosingNonLambda();

        if (enclosingNonLambda->IsGlobalFunction())
        {
            top->byteCodeFunction->SetEnclosedByGlobalFunc();
        }

        if (FuncAllowsDirectSuper(enclosingNonLambda, byteCodeGenerator))
        {
            top->byteCodeFunction->GetFunctionInfo()->SetAllowDirectSuper();
        }
    }
    else if (FuncAllowsDirectSuper(top, byteCodeGenerator))
    {
        top->byteCodeFunction->GetFunctionInfo()->SetAllowDirectSuper();
    }

    // If this is a named function expression and has deferred child, mark has non-local reference.
    if (funcExprWithName)
    {
        // If we are reparsing this function due to being in debug mode - we should restore the state of this from the earlier parse
        if (top->byteCodeFunction->IsFunctionParsed() && top->GetParsedFunctionBody()->HasFuncExprNameReference())
        {
            top->SetFuncExprNameReference(true);
        }
        if (sym->GetHasNonLocalReference())
        {
            // Before doing this, though, make sure there's no local symbol that hides the function name
            // from the nested functions. If a lookup starting at the current local scope finds some symbol
            // other than the func expr, then it's hidden. (See Win8 393618.)
            byteCodeGenerator->ProcessCapturedSym(sym);

            top->SetFuncExprNameReference(true);
            if (pnodeFnc->pnodeBody)
            {
                top->GetParsedFunctionBody()->SetFuncExprNameReference(true);
            }
            byteCodeGenerator->ProcessScopeWithCapturedSym(sym->GetScope());
        }
    }

    if (pnodeFnc->nop != knopProg
        && !top->bodyScope->GetIsObject()
        && byteCodeGenerator->NeedObjectAsFunctionScope(top, pnodeFnc))
    {
        // Even if it wasn't determined during visiting this function that we need a scope object, we still have a few conditions that may require one.
        top->bodyScope->SetIsObject();
        if (!top->IsBodyAndParamScopeMerged())
        {
            // If we have the function inside an eval then access to outer variables should go through scope object.
            // So we set the body scope as object and we need to set the param scope also as object in case of split scope.
            top->paramScope->SetIsObject();
        }
    }

    if (pnodeFnc->nop == knopProg
        && top->byteCodeFunction->GetIsStrictMode()
        && (byteCodeGenerator->GetFlags() & fscrEval))
    {
        // At global scope inside a strict mode eval, vars will not leak out and require a scope object (along with its parent.)
        top->bodyScope->SetIsObject();
    }

    if (pnodeFnc->pnodeBody)
    {
        if (!top->IsGlobalFunction())
        {
            auto fnProcess =
                [byteCodeGenerator, top](Symbol *const sym)
                {
                    if (sym->GetHasNonLocalReference() && !sym->GetIsModuleExportStorage())
                    {
                        byteCodeGenerator->ProcessCapturedSym(sym);
                    }
                };

            Scope *bodyScope = top->bodyScope;
            Scope *paramScope = top->paramScope;
            if (paramScope != nullptr)
            {
                if (paramScope->GetHasOwnLocalInClosure())
                {
                    paramScope->ForEachSymbol(fnProcess);
                    top->SetHasLocalInClosure(true);
                }
            }

            if (bodyScope->GetHasOwnLocalInClosure())
            {
                bodyScope->ForEachSymbol(fnProcess);
                top->SetHasLocalInClosure(true);
            }

            PostVisitBlock(pnodeFnc->pnodeBodyScope, byteCodeGenerator);
            PostVisitBlock(pnodeFnc->pnodeScopes, byteCodeGenerator);
        }

        // This function refers to the closure environment if:
        // 1. it has a child function (we'll pass the environment to the constructor when the child is created -
        //      even if it's not needed, it's as cheap as loading "null" from the library);
        // 2. it calls eval (and will use the environment to construct the scope chain to pass to eval);
        // 3. it refers to a local defined in a parent function;
        // 4. some parent calls eval;
        // 5. we're in an event handler;
        // 6. the function was declared inside a "with";
        // 7. we're in an eval expression.
        if (pnodeFnc->nestedCount != 0 ||
            top->GetCallsEval() ||
            top->GetHasClosureReference() ||
            byteCodeGenerator->InDynamicScope() ||
            (byteCodeGenerator->GetFlags() & (fscrImplicitThis | fscrEval)))
        {
            byteCodeGenerator->SetNeedEnvRegister();
        }

        // This function needs to construct a local frame on the heap if it is not the global function (even in eval) and:
        // 1. it calls eval, which may refer to or declare any locals in this frame;
        // 2. a child calls eval (which may refer to locals through a closure);
        // 3. it uses non-strict mode "arguments", so the arguments have to be put in a closure;
        // 4. it defines a local that is used by a child function (read from a closure).
        // 5. it is a main function that's wrapped in a function expression scope but has locals used through
        //    a closure (used in forReference function call cases in a with for example).
        if (!top->IsGlobalFunction())
        {
            if (top->GetCallsEval() ||
                top->GetChildCallsEval() ||
                (top->GetHasArguments() && byteCodeGenerator->NeedScopeObjectForArguments(top, pnodeFnc)) ||
                top->GetHasLocalInClosure() ||
                (top->funcExprScope && top->funcExprScope->GetMustInstantiate()) ||
                // When we have split scope normally either eval will be present or the GetHasLocalInClosure will be true as one of the formal is
                // captured. But when we force split scope or split scope happens due to some other reasons we have to make sure we allocate frame
                // slot register here.
                (!top->IsBodyAndParamScopeMerged()))
            {
                if (!top->GetCallsEval() && top->GetHasLocalInClosure())
                {
                    byteCodeGenerator->AssignFrameSlotsRegister();
                }

                if (!top->IsBodyAndParamScopeMerged())
                {
                    byteCodeGenerator->AssignParamSlotsRegister();
                }

                if (byteCodeGenerator->NeedObjectAsFunctionScope(top, top->root)
                    || top->bodyScope->GetIsObject()
                    || top->paramScope->GetIsObject())
                {
                    byteCodeGenerator->AssignFrameObjRegister();
                }

                // The function also needs to construct a frame display if:
                // 1. it calls eval;
                // 2. it has a child function.
                // 3. When has arguments and in debug mode. So that frame display be there along with frame object register.
                if (top->GetCallsEval() ||
                    pnodeFnc->nestedCount != 0
                    || (top->GetHasArguments()
                        && (pnodeFnc->pnodeParams != nullptr)
                        && byteCodeGenerator->IsInDebugMode()))
                {
                    byteCodeGenerator->SetNeedEnvRegister(); // This to ensure that Env should be there when the FrameDisplay register is there.
                    byteCodeGenerator->AssignFrameDisplayRegister();
                }
            }

            if (top->GetHasArguments())
            {
                Symbol *argSym = top->GetArgumentsSymbol();
                Assert(argSym);
                if (argSym)
                {
                    Assert(top->bodyScope->GetScopeSlotCount() == 0);
                    Assert(top->argsPlaceHolderSlotCount == 0);
                    byteCodeGenerator->AssignRegister(argSym);
                    uint i = 0;
                    auto setArgScopeSlot = [&](ParseNode *pnodeArg)
                    {
                        if (pnodeArg->IsVarLetOrConst())
                        {
                            Symbol* sym = pnodeArg->AsParseNodeVar()->sym;
                            if (sym->GetScopeSlot() != Js::Constants::NoProperty)
                            {
                                top->argsPlaceHolderSlotCount++; // Same name args appeared before
                            }
                            sym->SetScopeSlot(i);
                        }
                        else if (pnodeArg->nop == knopParamPattern)
                        {
                            top->argsPlaceHolderSlotCount++;
                        }
                        i++;
                    };

                    // We need to include the rest as well -as it will get slot assigned.
                    if (byteCodeGenerator->NeedScopeObjectForArguments(top, pnodeFnc))
                    {
                        MapFormals(pnodeFnc, setArgScopeSlot);
                        if (argSym->NeedsSlotAlloc(byteCodeGenerator, top))
                        {
                            Assert(argSym->GetScopeSlot() == Js::Constants::NoProperty);
                            argSym->SetScopeSlot(i++);
                        }
                        MapFormalsFromPattern(pnodeFnc, setArgScopeSlot);
                    }

                    top->paramScope->SetScopeSlotCount(i);

                    Assert(top->GetHasHeapArguments());
                    if (byteCodeGenerator->NeedScopeObjectForArguments(top, pnodeFnc)
                        && !pnodeFnc->HasNonSimpleParameterList())
                    {
                        top->byteCodeFunction->SetHasImplicitArgIns(false);
                    }
                }
            }
        }
        else
        {
            Assert(top->IsGlobalFunction() || pnodeFnc->IsModule());
            // eval is called in strict mode
            bool newScopeForEval = (top->byteCodeFunction->GetIsStrictMode() && (byteCodeGenerator->GetFlags() & fscrEval));

            if (newScopeForEval)
            {
                byteCodeGenerator->SetNeedEnvRegister();
                byteCodeGenerator->AssignFrameObjRegister();
                byteCodeGenerator->AssignFrameDisplayRegister();
            }
        }

        if (top->GetNewTargetSymbol())
        {
            byteCodeGenerator->AssignRegister(top->GetNewTargetSymbol());
        }
        if (top->GetThisSymbol())
        {
            byteCodeGenerator->AssignRegister(top->GetThisSymbol());

            // Indirect eval has a 'this' binding and needs to load from null
            if (top->IsGlobalFunction())
            {
                byteCodeGenerator->AssignNullConstRegister();
            }
        }
        if (top->GetSuperSymbol())
        {
            byteCodeGenerator->AssignRegister(top->GetSuperSymbol());
        }
        if (top->GetSuperConstructorSymbol())
        {
            byteCodeGenerator->AssignRegister(top->GetSuperConstructorSymbol());
        }

        Assert(!funcExprWithName || sym);
        if (funcExprWithName)
        {
            Assert(top->funcExprScope);
            // If the func expr may be accessed via eval, force the func expr scope into an object.
            if (top->GetCallsEval() || top->GetChildCallsEval())
            {
                top->funcExprScope->SetIsObject();
            }
            if (top->funcExprScope->GetIsObject())
            {
                top->funcExprScope->SetLocation(byteCodeGenerator->NextVarRegister());
            }
        }
    }

    byteCodeGenerator->EndBindFunction(funcExprWithName);

    // If the "child" is the global function, we're done.
    if (top->IsGlobalFunction())
    {
        return top;
    }

    if (top->IsBodyAndParamScopeMerged())
    {
        Scope::MergeParamAndBodyScopes(pnodeFnc);
        Scope::RemoveParamScope(pnodeFnc);
    }

    FuncInfo* const parentFunc = byteCodeGenerator->TopFuncInfo();

    Js::FunctionBody * parentFunctionBody = parentFunc->byteCodeFunction->GetFunctionBody();
    Assert(parentFunctionBody != nullptr);
    bool setHasNonLocalReference = parentFunctionBody->HasAllNonLocalReferenced();

    // This is required for class constructors as will be able to determine the actual home object register only after emitting InitClass
    if (pnodeFnc->HasHomeObj() && pnodeFnc->GetHomeObjLocation() == Js::Constants::NoRegister)
    {
        pnodeFnc->SetHomeObjLocation(parentFunc->AssignUndefinedConstRegister());
    }

    // If we have any deferred child, we need to instantiate the fake global block scope if it is not empty
    if (parentFunc->IsGlobalFunction())
    {
        if (byteCodeGenerator->IsEvalWithNoParentScopeInfo())
        {
            Scope * globalEvalBlockScope = parentFunc->GetGlobalEvalBlockScope();
            if (globalEvalBlockScope->GetHasOwnLocalInClosure())
            {
                globalEvalBlockScope->SetMustInstantiate(true);
            }
        }
    }
    else
    {
        if (setHasNonLocalReference)
        {
            // All locals are already marked as non-locals-referenced. Mark the parent as well.
            if (parentFunctionBody->HasSetIsObject())
            {
                // Updated the current function, as per the previous stored info.
                parentFunc->GetBodyScope()->SetIsObject();
                parentFunc->GetParamScope()->SetIsObject();
            }
        }
        // Propagate HasMaybeEscapedNestedFunc
        if (!byteCodeGenerator->CanStackNestedFunc(top, false) ||
            byteCodeGenerator->NeedObjectAsFunctionScope(top, pnodeFnc))
        {
            parentFunc->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("Child")));
        }
    }

    if (top->GetCallsEval() || top->GetChildCallsEval())
    {
        parentFunc->SetChildCallsEval(true);
        ParseNodeBlock *currentBlock = byteCodeGenerator->GetCurrentBlock();
        if (currentBlock)
        {
            Assert(currentBlock->nop == knopBlock);
            currentBlock->SetChildCallsEval(true);
        }
        parentFunc->SetHasHeapArguments(true);
        setHasNonLocalReference = true;
        parentFunctionBody->SetAllNonLocalReferenced(true);

        Scope * const funcExprScope = top->funcExprScope;
        if (funcExprScope)
        {
            // If we have the body scope as an object, the outer function expression scope also needs to be an object to propagate the name.
            funcExprScope->SetIsObject();
        }

        if (parentFunc->inArgsCount == 1)
        {
            // If no formals to function, no need to create the propertyid array
            byteCodeGenerator->AssignNullConstRegister();
        }
    }

    if (setHasNonLocalReference && !parentFunctionBody->HasDoneAllNonLocalReferenced())
    {
        parentFunc->GetBodyScope()->ForceAllSymbolNonLocalReference(byteCodeGenerator);
        if (!parentFunc->IsGlobalFunction())
        {
            parentFunc->GetParamScope()->ForceAllSymbolNonLocalReference(byteCodeGenerator);
        }
        parentFunctionBody->SetHasDoneAllNonLocalReferenced(true);
    }

    if (pnodeFnc->IsGenerator())
    {
        top->AssignUndefinedConstRegister();
    }

    if ((top->root->IsConstructor() && (top->GetCallsEval() || top->GetChildCallsEval())) || top->IsClassConstructor())
    {
        if (!top->IsBaseClassConstructor())
        {
            // Derived class constructors need to check undefined against explicit return statements.
            top->AssignUndefinedConstRegister();
        }
    }

    AssignFuncSymRegister(pnodeFnc, byteCodeGenerator, top);

    if (pnodeFnc->pnodeBody && pnodeFnc->HasReferenceableBuiltInArguments() && pnodeFnc->UsesArguments() &&
        pnodeFnc->HasHeapArguments())
    {
        bool doStackArgsOpt = top->byteCodeFunction->GetDoBackendArgumentsOptimization();

        bool hasAnyParamInClosure = top->GetHasLocalInClosure() && top->GetParamScope()->GetHasOwnLocalInClosure();

        if ((doStackArgsOpt && top->inArgsCount > 1))
        {
            if (doStackArgsOpt && hasAnyParamInClosure)
            {
                top->SetHasHeapArguments(true, false /*= Optimize arguments in backend*/);
#ifdef PERF_HINT
                if (PHASE_TRACE1(Js::PerfHintPhase))
                {
                    WritePerfHint(PerfHints::HeapArgumentsDueToNonLocalRef, top->GetParsedFunctionBody(), 0);
                }
#endif
            }
            else if (!top->GetHasLocalInClosure() && !(byteCodeGenerator->GetFlags() & fscrEval) && !top->byteCodeFunction->IsEval())
            {
                //Scope object creation instr will be a MOV NULL instruction in the Lowerer - if we still decide to do StackArgs after Globopt phase.
                //Note that if we're in eval, scoped ldfld/stfld will traverse the whole frame display, including this slot, so it can't be null.
                top->byteCodeFunction->SetDoScopeObjectCreation(false);
            }
        }
    }
    return top;
}

void ByteCodeGenerator::ProcessCapturedSym(Symbol *sym)
{
    // The symbol's home function will tell us which child function we're currently processing.
    // This is the one that captures the symbol, from the declaring function's perspective.
    // So based on that information, note either that, (a.) the symbol is committed to the heap from its
    // inception, (b.) the symbol must be committed when the capturing function is instantiated.

    FuncInfo *funcHome = sym->GetScope()->GetFunc();
    FuncInfo *funcChild = funcHome->GetCurrentChildFunction();

    Assert(sym->NeedsSlotAlloc(this, funcHome) || sym->GetIsGlobal() || sym->GetIsModuleImport() || sym->GetIsModuleExportStorage());

    // If this is not a local property, or not all its references can be tracked, or
    // it's not scoped to the function, or we're in debug mode, disable the delayed capture optimization.
    if (funcHome->IsGlobalFunction() ||
        funcHome->GetCallsEval() ||
        funcHome->GetChildCallsEval() ||
        funcChild == nullptr ||
        sym->GetScope() != funcHome->GetBodyScope() ||
        this->IsInDebugMode() ||
        PHASE_OFF(Js::DelayCapturePhase, funcHome->byteCodeFunction))
    {
        sym->SetIsCommittedToSlot();
    }

    if (sym->GetIsCommittedToSlot())
    {
        return;
    }

    AnalysisAssert(funcChild);
    ParseNode *pnodeChild = funcChild->root;

    Assert(pnodeChild && pnodeChild->nop == knopFncDecl);

    if (pnodeChild->AsParseNodeFnc()->IsDeclaration())
    {
        // The capturing function is a declaration but may still be limited to an inner scope.
        Scope *scopeChild = funcHome->GetCurrentChildScope();
        if (scopeChild == sym->GetScope() || scopeChild->GetScopeType() == ScopeType_FunctionBody)
        {
            // The symbol is captured on entry to the scope in which it's declared.
            // (Check the scope type separately so that we get the special parameter list and
            // named function expression cases as well.)
            sym->SetIsCommittedToSlot();
            return;
        }
    }

    // There is a chance we can limit the region in which the symbol lives on the heap.
    // Note which function captures the symbol.
    funcChild->AddCapturedSym(sym);
}

void ByteCodeGenerator::ProcessScopeWithCapturedSym(Scope *scope)
{
    Assert(scope->GetHasOwnLocalInClosure());

    // (Note: if any catch var is closure-captured, we won't merge the catch scope with the function scope.
    // So don't mark the function scope "has local in closure".)
    FuncInfo *func = scope->GetFunc();
    bool notCatch = scope->GetScopeType() != ScopeType_Catch && scope->GetScopeType() != ScopeType_CatchParamPattern;
    if (scope == func->GetBodyScope() || scope == func->GetParamScope() || (scope->GetCanMerge() && notCatch))
    {
        func->SetHasLocalInClosure(true);
    }
    else
    {
        if (scope->HasCrossScopeFuncAssignment())
        {
            func->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("InstantiateScopeWithCrossScopeAssignment")));
        }
        scope->SetMustInstantiate(true);
    }
}

void MarkInit(ParseNode* pnode)
{
    if (pnode->nop == knopList)
    {
        do
        {
            MarkInit(pnode->AsParseNodeBin()->pnode1);
            pnode = pnode->AsParseNodeBin()->pnode2;
        }
        while (pnode->nop == knopList);
        MarkInit(pnode);
    }
    else
    {
        Symbol *sym = nullptr;
        ParseNode *pnodeInit = nullptr;
        if (pnode->nop == knopVarDecl)
        {
            sym = pnode->AsParseNodeVar()->sym;
            pnodeInit = pnode->AsParseNodeVar()->pnodeInit;
        }
        else if (pnode->nop == knopAsg && pnode->AsParseNodeBin()->pnode1->nop == knopName)
        {
            sym = pnode->AsParseNodeBin()->pnode1->AsParseNodeName()->sym;
            pnodeInit = pnode->AsParseNodeBin()->pnode2;
        }

        if (sym && !sym->GetIsUsed() && pnodeInit)
        {
            sym->SetHasInit(true);
            if (sym->HasVisitedCapturingFunc())
            {
                sym->SetHasNonCommittedReference(false);
            }
        }
    }
}

void AddFunctionsToScope(ParseNodePtr scope, ByteCodeGenerator * byteCodeGenerator)
{
    VisitFncDecls(scope, [byteCodeGenerator](ParseNode *fn)
    {
        ParseNode *pnodeName = fn->AsParseNodeFnc()->pnodeName;
        if (pnodeName && pnodeName->nop == knopVarDecl && fn->AsParseNodeFnc()->IsDeclaration())
        {
            const char16 *fnName = pnodeName->AsParseNodeVar()->pid->Psz();
            if (byteCodeGenerator->Trace())
            {
                Output::Print(_u("current context has declared function %s\n"), fnName);
            }
            // In ES6, functions are scoped to the block, which will be the current scope.
            // Pre-ES6, function declarations are scoped to the function body, so get that scope.
            Symbol *sym;
            if (!byteCodeGenerator->GetCurrentScope()->IsGlobalEvalBlockScope())
            {
                sym = byteCodeGenerator->AddSymbolToScope(byteCodeGenerator->GetCurrentScope(), fnName, pnodeName->AsParseNodeVar()->pid->Cch(), pnodeName, STFunction);
            }
            else
            {
                sym = byteCodeGenerator->AddSymbolToFunctionScope(fnName, pnodeName->AsParseNodeVar()->pid->Cch(), pnodeName, STFunction);
            }

            pnodeName->AsParseNodeVar()->sym = sym;
            if (sym->GetScope() != sym->GetScope()->GetFunc()->GetBodyScope() &&
                sym->GetScope() != sym->GetScope()->GetFunc()->GetParamScope())
            {
                sym->SetIsBlockVar(true);
            }
        }
    });
}

template <class PrefixFn, class PostfixFn>
void VisitNestedScopes(ParseNode* pnodeScopeList, ParseNode* pnodeParent, ByteCodeGenerator* byteCodeGenerator,
    PrefixFn prefix, PostfixFn postfix, uint *pIndex, bool breakOnBodyScope = false)
{
    // Visit all scopes nested in this scope before visiting this function's statements. This way we have all the
    // attributes of all the inner functions before we assign registers within this function.
    // All the attributes we need to propagate downward should already be recorded by the parser.
    // - call to "eval()"
    // - nested in "with"
    FuncInfo * parentFuncInfo = pnodeParent->AsParseNodeFnc()->funcInfo;
    Js::ParseableFunctionInfo* parentFunc = parentFuncInfo->byteCodeFunction;
    ParseNode* pnodeScope;
    uint i = 0;

    // Cache to restore it back once we come out of current function.
    Js::FunctionBody * pLastReuseFunc = byteCodeGenerator->pCurrentFunction;

    for (pnodeScope = pnodeScopeList; pnodeScope;)
    {
        if (breakOnBodyScope && pnodeScope == pnodeParent->AsParseNodeFnc()->pnodeBodyScope)
        {
            break;
        }

        switch (pnodeScope->nop)
        {
        case knopFncDecl:
        {
            ParseNodeFnc * pnodeFnc = pnodeScope->AsParseNodeFnc();
            if (pLastReuseFunc)
            {
                if (!byteCodeGenerator->IsInNonDebugMode())
                {
                    // Here we are trying to match the inner sub-tree as well with already created inner function.

                    if ((pLastReuseFunc->GetIsGlobalFunc() && parentFunc->GetIsGlobalFunc())
                        || (!pLastReuseFunc->GetIsGlobalFunc() && !parentFunc->GetIsGlobalFunc()))
                    {
                        Assert(pLastReuseFunc->StartInDocument() == pnodeParent->ichMin);
                        Assert(pLastReuseFunc->LengthInChars() == pnodeParent->LengthInCodepoints());
                        Assert(pLastReuseFunc->GetNestedCount() == parentFunc->GetNestedCount());

                        // If the current function is not parsed yet, its function body is not generated yet.
                        // Reset pCurrentFunction to null so that it will not be able re-use anything.
                        Js::FunctionProxy* proxy = pLastReuseFunc->GetNestedFunctionProxy((*pIndex));
                        if (proxy && proxy->IsFunctionBody())
                        {
                            byteCodeGenerator->pCurrentFunction = proxy->GetFunctionBody();
                        }
                        else
                        {
                            byteCodeGenerator->pCurrentFunction = nullptr;
                        }
                    }
                }
                else if (!parentFunc->GetIsGlobalFunc())
                {
                    // In the deferred parsing mode, we will be reusing the only one function (which is asked when on ::Begin) all inner function will be created.
                    byteCodeGenerator->pCurrentFunction = nullptr;
                }
            }

            Js::ParseableFunctionInfo::NestedArray * parentNestedArray = parentFunc->GetNestedArray();
            Js::ParseableFunctionInfo* reuseNestedFunc = nullptr;
            if (parentNestedArray)
            {
                Assert(*pIndex < parentNestedArray->nestedCount);
                Js::FunctionInfo * info = parentNestedArray->functionInfoArray[*pIndex];
                if (info && info->HasParseableInfo())
                {
                    reuseNestedFunc = info->GetParseableFunctionInfo();

                    // If parentFunc was redeferred, try to set pCurrentFunction to this FunctionBody,
                    // and cleanup to reparse (as previous cleanup stops at redeferred parentFunc).
                    if (!byteCodeGenerator->IsInNonDebugMode()
                        && !byteCodeGenerator->pCurrentFunction
                        && reuseNestedFunc->IsFunctionBody())
                    {
                        byteCodeGenerator->pCurrentFunction = reuseNestedFunc->GetFunctionBody();
                    }
                }
            }
            PreVisitFunction(pnodeFnc, byteCodeGenerator, reuseNestedFunc);
            FuncInfo *funcInfo = pnodeFnc->funcInfo;

            parentFuncInfo->OnStartVisitFunction(pnodeFnc);

            if (pnodeFnc->pnodeBody)
            {
                if (!byteCodeGenerator->IsInNonDebugMode() && pLastReuseFunc != nullptr && byteCodeGenerator->pCurrentFunction == nullptr)
                {
                    // Patch current non-parsed function's FunctionBodyImpl with the new generated function body.
                    // So that the function object (pointing to the old function body) can able to get to the new one.

                    Js::FunctionProxy* proxy = pLastReuseFunc->GetNestedFunctionProxy((*pIndex));
                    if (proxy && !proxy->IsFunctionBody())
                    {
                        proxy->UpdateFunctionBodyImpl(funcInfo->byteCodeFunction->GetFunctionBody());
                    }
                }

                Scope *paramScope = funcInfo->GetParamScope();
                Scope *bodyScope = funcInfo->GetBodyScope();

                BeginVisitBlock(pnodeFnc->pnodeScopes, byteCodeGenerator);
                i = 0;
                ParseNodePtr containerScope = pnodeFnc->pnodeScopes;

                // Push the param scope
                byteCodeGenerator->PushScope(paramScope);

                if (pnodeFnc->HasNonSimpleParameterList() && !funcInfo->IsBodyAndParamScopeMerged())
                {
                    // Set param scope as the current child scope.
                    funcInfo->SetCurrentChildScope(paramScope);
                    Assert(containerScope->nop == knopBlock && containerScope->AsParseNodeBlock()->blockType == Parameter);
                    VisitNestedScopes(containerScope->AsParseNodeBlock()->pnodeScopes, pnodeFnc, byteCodeGenerator, prefix, postfix, &i, true);
                    MapFormals(pnodeFnc, [&](ParseNode *argNode) { Visit(argNode, byteCodeGenerator, prefix, postfix); });
                }

                // Push the body scope
                byteCodeGenerator->PushScope(bodyScope);
                funcInfo->SetCurrentChildScope(bodyScope);

                PreVisitBlock(pnodeFnc->pnodeBodyScope, byteCodeGenerator);
                AddVarsToScope(pnodeFnc->pnodeVars, byteCodeGenerator);

                if (!pnodeFnc->HasNonSimpleParameterList() || funcInfo->IsBodyAndParamScopeMerged())
                {
                    VisitNestedScopes(containerScope, pnodeFnc, byteCodeGenerator, prefix, postfix, &i);
                    MapFormals(pnodeFnc, [&](ParseNode *argNode) { Visit(argNode, byteCodeGenerator, prefix, postfix); });
                }

                if (pnodeFnc->HasNonSimpleParameterList())
                {
                    byteCodeGenerator->AssignUndefinedConstRegister();

                    if (!funcInfo->IsBodyAndParamScopeMerged())
                    {
                        Assert(pnodeFnc->pnodeBodyScope->scope);
                        VisitNestedScopes(pnodeFnc->pnodeBodyScope->pnodeScopes, pnodeFnc, byteCodeGenerator, prefix, postfix, &i);
                    }
                }

                BeginVisitBlock(pnodeFnc->pnodeBodyScope, byteCodeGenerator);

                ParseNode* pnode = pnodeFnc->pnodeBody;
                while (pnode->nop == knopList)
                {
                    // Check to see whether initializations of locals to "undef" can be skipped.
                    // The logic to do this is cheap - omit the init if we see an init with a value
                    // on the RHS at the top statement level (i.e., not inside a block, try, loop, etc.)
                    // before we see a use. The motivation is to help identify single-def locals in the BE.
                    // Note that this can't be done for globals.
                    byteCodeGenerator->SetCurrentTopStatement(pnode->AsParseNodeBin()->pnode1);
                    Visit(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator, prefix, postfix);
                    if (!funcInfo->GetCallsEval() && !funcInfo->GetChildCallsEval() &&
                        // So that it will not be marked as init thus it will be added to the diagnostics symbols container.
                        !(byteCodeGenerator->ShouldTrackDebuggerMetadata()))
                    {
                        MarkInit(pnode->AsParseNodeBin()->pnode1);
                    }
                    pnode = pnode->AsParseNodeBin()->pnode2;
                }
                byteCodeGenerator->SetCurrentTopStatement(pnode);
                Visit(pnode, byteCodeGenerator, prefix, postfix);

                EndVisitBlock(pnodeFnc->pnodeBodyScope, byteCodeGenerator);
                EndVisitBlock(pnodeFnc->pnodeScopes, byteCodeGenerator);
            }

            if (!pnodeFnc->pnodeBody)
            {
                // For defer prase scenario push the scopes here
                byteCodeGenerator->PushScope(funcInfo->GetParamScope());
                byteCodeGenerator->PushScope(funcInfo->GetBodyScope());
            }

            if (!parentFuncInfo->IsFakeGlobalFunction(byteCodeGenerator->GetFlags()))
            {
                pnodeFnc->nestedIndex = *pIndex;
                parentFunc->SetNestedFunc(funcInfo->byteCodeFunction->GetFunctionInfo(), (*pIndex)++, byteCodeGenerator->GetFlags());
            }

            Assert(parentFunc);

            parentFuncInfo->OnEndVisitFunction(pnodeFnc);

            PostVisitFunction(pnodeFnc, byteCodeGenerator);

            pnodeScope = pnodeFnc->pnodeNext;

            byteCodeGenerator->pCurrentFunction = pLastReuseFunc;
            break;
        }

        case knopBlock:
        {
            ParseNodeBlock * pnodeBlockScope = pnodeScope->AsParseNodeBlock();
            PreVisitBlock(pnodeBlockScope, byteCodeGenerator);
            bool isMergedScope;
            parentFuncInfo->OnStartVisitScope(pnodeBlockScope->scope, &isMergedScope);
            VisitNestedScopes(pnodeBlockScope->pnodeScopes, pnodeParent, byteCodeGenerator, prefix, postfix, pIndex);
            parentFuncInfo->OnEndVisitScope(pnodeBlockScope->scope, isMergedScope);
            PostVisitBlock(pnodeBlockScope, byteCodeGenerator);

            pnodeScope = pnodeScope->AsParseNodeBlock()->pnodeNext;
            break;
        }

        case knopCatch:
        {
            ParseNodeCatch * pnodeCatchScope = pnodeScope->AsParseNodeCatch();
            PreVisitCatch(pnodeCatchScope, byteCodeGenerator);

            if (pnodeCatchScope->HasParam() && !pnodeCatchScope->HasPatternParam())
            {
                Visit(pnodeCatchScope->GetParam(), byteCodeGenerator, prefix, postfix);
            }

            bool isMergedScope;
            parentFuncInfo->OnStartVisitScope(pnodeCatchScope->scope, &isMergedScope);
            VisitNestedScopes(pnodeCatchScope->pnodeScopes, pnodeParent, byteCodeGenerator, prefix, postfix, pIndex);

            parentFuncInfo->OnEndVisitScope(pnodeCatchScope->scope, isMergedScope);
            PostVisitCatch(pnodeCatchScope, byteCodeGenerator);

            pnodeScope = pnodeCatchScope->pnodeNext;
            break;
        }

        case knopWith:
        {
            PreVisitWith(pnodeScope, byteCodeGenerator);
            bool isMergedScope;
            parentFuncInfo->OnStartVisitScope(pnodeScope->AsParseNodeWith()->scope, &isMergedScope);
            VisitNestedScopes(pnodeScope->AsParseNodeWith()->pnodeScopes, pnodeParent, byteCodeGenerator, prefix, postfix, pIndex);
            parentFuncInfo->OnEndVisitScope(pnodeScope->AsParseNodeWith()->scope, isMergedScope);
            PostVisitWith(pnodeScope, byteCodeGenerator);
            pnodeScope = pnodeScope->AsParseNodeWith()->pnodeNext;
            break;
        }

        default:
            AssertMsg(false, "Unexpected opcode in tree of scopes");
            return;
        }
    }
}

void PreVisitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator)
{
    if (!pnodeBlock->scope &&
        !pnodeBlock->HasBlockScopedContent() &&
        !pnodeBlock->GetCallsEval())
    {
        // Do nothing here if the block doesn't declare anything or call eval (which may declare something).
        return;
    }

    bool isGlobalEvalBlockScope = false;
    FuncInfo *func = byteCodeGenerator->TopFuncInfo();
    if (func->IsGlobalFunction() &&
        func->root->pnodeScopes == pnodeBlock &&
        byteCodeGenerator->IsEvalWithNoParentScopeInfo())
    {
        isGlobalEvalBlockScope = true;
    }
    Assert(!pnodeBlock->scope ||
           isGlobalEvalBlockScope == (pnodeBlock->scope->GetScopeType() == ScopeType_GlobalEvalBlock));

    ArenaAllocator *alloc = byteCodeGenerator->GetAllocator();
    Scope *scope;

    if ((pnodeBlock->blockType == PnodeBlockType::Global && !byteCodeGenerator->IsEvalWithNoParentScopeInfo()) || pnodeBlock->blockType == PnodeBlockType::Function)
    {
        scope = byteCodeGenerator->GetCurrentScope();

        if (pnodeBlock->blockType == PnodeBlockType::Function)
        {
            AnalysisAssert(pnodeBlock->scope);
            if (pnodeBlock->scope->GetScopeType() == ScopeType_Parameter
                && scope->GetScopeType() == ScopeType_FunctionBody)
            {
                scope = scope->GetEnclosingScope();
            }
        }

        pnodeBlock->scope = scope;
    }
    else if (!(pnodeBlock->grfpn & fpnSyntheticNode) || isGlobalEvalBlockScope)
    {
        scope = pnodeBlock->scope;
        if (!scope)
        {
            scope = Anew(alloc, Scope, alloc,
                         isGlobalEvalBlockScope? ScopeType_GlobalEvalBlock : ScopeType_Block, true);
            pnodeBlock->scope = scope;
        }
        scope->SetFunc(byteCodeGenerator->TopFuncInfo());
        // For now, prevent block scope from being merged with enclosing function scope.
        // Consider optimizing this.
        scope->SetCanMerge(false);

        if (isGlobalEvalBlockScope)
        {
            scope->SetIsObject();
        }

        byteCodeGenerator->PushScope(scope);
        byteCodeGenerator->PushBlock(pnodeBlock);
    }
    else
    {
        return;
    }

    Assert(scope && scope == pnodeBlock->scope);

    bool isGlobalScope = (scope->GetEnclosingScope() == nullptr);
    Assert(!isGlobalScope || (pnodeBlock->grfpn & fpnSyntheticNode));

    // If it is the global eval block scope, we don't what function decl to be assigned in the block scope.
    // They should already declared in the global function's scope.
    if (!isGlobalEvalBlockScope && !isGlobalScope)
    {
        AddFunctionsToScope(pnodeBlock->pnodeScopes, byteCodeGenerator);
    }

    // We can skip this check by not creating the GlobalEvalBlock above and in Parser::Parse for console eval but that seems to break couple of places
    // as we heavily depend on BlockHasOwnScope function. Once we clean up the creation of GlobalEvalBlock for evals we can clean this as well.
    if (byteCodeGenerator->IsConsoleScopeEval() && isGlobalEvalBlockScope && !isGlobalScope)
    {
        AssertMsg(scope->GetEnclosingScope()->GetScopeType() == ScopeType_Global, "Additional scope between Global and GlobalEvalBlock?");
        scope = scope->GetEnclosingScope();
        isGlobalScope = true;
    }

    auto addSymbolToScope = [scope, byteCodeGenerator, isGlobalScope](ParseNode *pnode)
        {
            Symbol *sym = byteCodeGenerator->AddSymbolToScope(scope, reinterpret_cast<const char16*>(pnode->AsParseNodeVar()->pid->Psz()), pnode->AsParseNodeVar()->pid->Cch(), pnode, STVariable);
#if DBG_DUMP
        if (sym->GetSymbolType() == STVariable && byteCodeGenerator->Trace())
        {
            Output::Print(_u("current context has declared %s %s of type %s\n"),
                sym->GetDecl()->nop == knopLetDecl ? _u("let") : _u("const"),
                pnode->AsParseNodeVar()->pid->Psz(),
                sym->GetSymbolTypeName());
        }
#endif
            sym->SetIsGlobal(isGlobalScope);
            sym->SetIsBlockVar(true);
            sym->SetIsConst(pnode->nop == knopConstDecl);
            sym->SetNeedDeclaration(true);
            pnode->AsParseNodeVar()->sym = sym;
        };

    byteCodeGenerator->IterateBlockScopedVariables(pnodeBlock, addSymbolToScope);
}

void PostVisitBlock(ParseNodeBlock *pnodeBlock, ByteCodeGenerator *byteCodeGenerator)
{
    if (!BlockHasOwnScope(pnodeBlock, byteCodeGenerator))
    {
        return;
    }

    Scope *scope = pnodeBlock->scope;

    if (pnodeBlock->GetCallsEval() || pnodeBlock->GetChildCallsEval() || (byteCodeGenerator->GetFlags() & (fscrEval | fscrImplicitThis)))
    {
        bool scopeIsEmpty = scope->IsEmpty();
        scope->SetIsObject();
        scope->SetCapturesAll(true);
        scope->SetMustInstantiate(!scopeIsEmpty);
    }

    if (scope->GetHasOwnLocalInClosure())
    {
        byteCodeGenerator->ProcessScopeWithCapturedSym(scope);
    }

    byteCodeGenerator->PopScope();
    byteCodeGenerator->PopBlock();

    ParseNodeBlock *currentBlock = byteCodeGenerator->GetCurrentBlock();
    if (currentBlock && (pnodeBlock->GetCallsEval() || pnodeBlock->GetChildCallsEval()))
    {
        currentBlock->SetChildCallsEval(true);
    }
}

void PreVisitCatch(ParseNodeCatch *pnodeCatch, ByteCodeGenerator *byteCodeGenerator)
{
    // Push the catch scope and add the catch expression to it.
    byteCodeGenerator->StartBindCatch(pnodeCatch);
    
    if (pnodeCatch->HasPatternParam())
    {
        ParseNodeParamPattern * pnodeParamPattern = pnodeCatch->GetParam()->AsParseNodeParamPattern();
        Parser::MapBindIdentifier(pnodeParamPattern->pnode1, [&](ParseNodePtr item)
        {
            Symbol *sym = item->AsParseNodeVar()->sym;
#if DBG_DUMP
            if (byteCodeGenerator->Trace())
            {
                Output::Print(_u("current context has declared catch var %s of type %s\n"),
                    item->AsParseNodeVar()->pid->Psz(), sym->GetSymbolTypeName());
            }
#endif
            sym->SetIsCatch(true);
            sym->SetIsBlockVar(true);
        });
    }
    else if (pnodeCatch->HasParam())
    {
        ParseNodeName * pnodeName = pnodeCatch->GetParam()->AsParseNodeName();
        Symbol *sym = *pnodeName->GetSymRef();
        Assert(sym->GetScope() == pnodeCatch->scope);
#if DBG_DUMP
        if (byteCodeGenerator->Trace())
        {
            Output::Print(_u("current context has declared catch var %s of type %s\n"),
                pnodeName->pid->Psz(), sym->GetSymbolTypeName());
        }
#endif
        sym->SetIsCatch(true);
        pnodeName->sym = sym;
    }
    
    // This call will actually add the nested function symbols to the enclosing function scope (which is what we want).
    AddFunctionsToScope(pnodeCatch->pnodeScopes, byteCodeGenerator);
}

void PostVisitCatch(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Scope *scope = pnode->AsParseNodeCatch()->scope;
    if (scope->GetHasOwnLocalInClosure())
    {
        byteCodeGenerator->ProcessScopeWithCapturedSym(scope);
    }
    byteCodeGenerator->EndBindCatch();
}

void PreVisitWith(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    ArenaAllocator *alloc = byteCodeGenerator->GetAllocator();
    Scope *scope = Anew(alloc, Scope, alloc, ScopeType_With);
    scope->SetFunc(byteCodeGenerator->TopFuncInfo());
    scope->SetIsDynamic(true);
    pnode->AsParseNodeWith()->scope = scope;

    byteCodeGenerator->PushScope(scope);
}

void PostVisitWith(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    byteCodeGenerator->PopScope();
}

bool IsMathLibraryId(Js::PropertyId propertyId)
{
    return (propertyId >= Js::PropertyIds::abs) && (propertyId <= Js::PropertyIds::fround);
}

bool IsLibraryFunction(ParseNode* expr, Js::ScriptContext* scriptContext)
{
    if (expr && expr->nop == knopDot)
    {
        ParseNode* lhs = expr->AsParseNodeBin()->pnode1;
        ParseNode* rhs = expr->AsParseNodeBin()->pnode2;
        if ((lhs != nullptr) && (rhs != nullptr) && (lhs->nop == knopName) && (rhs->nop == knopName))
        {
            Symbol* lsym = lhs->AsParseNodeName()->sym;
            if ((lsym == nullptr || lsym->GetIsGlobal()) && lhs->AsParseNodeName()->PropertyIdFromNameNode() == Js::PropertyIds::Math)
            {
                return IsMathLibraryId(rhs->AsParseNodeName()->PropertyIdFromNameNode());
            }
        }
    }
    return false;
}

struct SymCheck
{
    static const int kMaxInvertedSyms = 8;
    Symbol* syms[kMaxInvertedSyms];
    Symbol* permittedSym;
    int symCount;
    bool result;
    bool cond;

    bool AddSymbol(Symbol* sym)
    {
        if (symCount < kMaxInvertedSyms)
        {
            syms[symCount++] = sym;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool MatchSymbol(Symbol* sym)
    {
        if (sym != permittedSym)
        {
            for (int i = 0; i < symCount; i++)
            {
                if (sym == syms[i])
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Init()
    {
        symCount = 0;
        result = true;
    }
};

void CheckInvertableExpr(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, SymCheck* symCheck)
{
    if (symCheck->result)
    {
        switch (pnode->nop)
        {
        case knopName:
            if (symCheck->MatchSymbol(pnode->AsParseNodeName()->sym))
            {
                symCheck->result = false;
            }
            break;
        case knopCall:
        {
            ParseNode* callTarget = pnode->AsParseNodeCall()->pnodeTarget;
            if (callTarget != nullptr)
            {
                if (callTarget->nop == knopName)
                {
                    Symbol* sym = callTarget->AsParseNodeName()->sym;
                    if (sym && sym->SingleDef())
                    {
                        ParseNode* decl = sym->GetDecl();
                        if (decl == nullptr ||
                            decl->nop != knopVarDecl ||
                            !IsLibraryFunction(decl->AsParseNodeVar()->pnodeInit, byteCodeGenerator->GetScriptContext()))
                        {
                            symCheck->result = false;
                        }
                        }
                    else
                    {
                        symCheck->result = false;
                    }
                    }
                else if (callTarget->nop == knopDot)
                {
                    if (!IsLibraryFunction(callTarget, byteCodeGenerator->GetScriptContext()))
                    {
                        symCheck->result = false;
                }
                    }
                    }
            else
            {
                symCheck->result = false;
                }
            break;
                       }
        case knopDot:
            if (!IsLibraryFunction(pnode, byteCodeGenerator->GetScriptContext()))
            {
                symCheck->result = false;
            }
            break;
        case knopTrue:
        case knopFalse:
        case knopAdd:
        case knopSub:
        case knopDiv:
        case knopMul:
        case knopExpo:
        case knopMod:
        case knopNeg:
        case knopInt:
        case knopFlt:
        case knopLt:
        case knopGt:
        case knopLe:
        case knopGe:
        case knopEq:
        case knopNe:
            break;
        default:
            symCheck->result = false;
            break;
        }
    }
}

bool InvertableExpr(SymCheck* symCheck, ParseNode* expr, ByteCodeGenerator* byteCodeGenerator)
{
    symCheck->result = true;
    symCheck->cond = false;
    symCheck->permittedSym = nullptr;
    VisitIndirect<SymCheck>(expr, byteCodeGenerator, symCheck, &CheckInvertableExpr, nullptr);
    return symCheck->result;
}

bool InvertableExprPlus(SymCheck* symCheck, ParseNode* expr, ByteCodeGenerator* byteCodeGenerator, Symbol* permittedSym)
{
    symCheck->result = true;
    symCheck->cond = true;
    symCheck->permittedSym = permittedSym;
    VisitIndirect<SymCheck>(expr, byteCodeGenerator, symCheck, &CheckInvertableExpr, nullptr);
    return symCheck->result;
}

void CheckLocalVarDef(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Assert(pnode->nop == knopAsg);
    if (pnode->AsParseNodeBin()->pnode1 != nullptr)
    {
        ParseNode *lhs = pnode->AsParseNodeBin()->pnode1;
        if (lhs->nop == knopName)
        {
            Symbol *sym = lhs->AsParseNodeName()->sym;
            if (sym != nullptr)
            {
                sym->RecordDef();
                if (sym->IsUsedInLdElem())
                {
                    Ident::TrySetIsUsedInLdElem(pnode->AsParseNodeBin()->pnode2);
                }
            }
        }
    }
}

ParseNode* ConstructInvertedStatement(ParseNode* stmt, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo,
    ParseNodeBin** outerStmtRef)
{
    if (stmt == nullptr)
    {
            return nullptr;
    }

    ParseNode * cStmt;
    if ((stmt->nop == knopAsg) || (stmt->nop == knopVarDecl))
    {
        ParseNode * rhs = nullptr;
        ParseNode * lhs = nullptr;

        if (stmt->nop == knopAsg)
        {
            rhs = stmt->AsParseNodeBin()->pnode2;
            lhs = stmt->AsParseNodeBin()->pnode1;
        }
        else if (stmt->nop == knopVarDecl)
        {
            rhs = stmt->AsParseNodeVar()->pnodeInit;
        }
        ArenaAllocator * alloc = byteCodeGenerator->GetAllocator();
        ParseNodeVar * loopInvar = Parser::StaticCreateTempNode(rhs, alloc);
        loopInvar->location = funcInfo->NextVarRegister();

        // Can't use a temp register here because the inversion happens at the parse tree level without generating
        // any bytecode yet. All local non-temp registers need to be initialized for jitted loop bodies, and since this is
        // not a user variable, track this register separately to have it be initialized at the top of the function.
        funcInfo->nonUserNonTempRegistersToInitialize.Add(loopInvar->location);

            // add temp node to list of initializers for new outer loop
        if ((*outerStmtRef)->pnode1 == nullptr)
        {
            (*outerStmtRef)->pnode1 = loopInvar;
        }
        else
        {
            ParseNodeBin * listNode = Parser::StaticCreateBinNode(knopList, nullptr, nullptr, alloc);
            (*outerStmtRef)->pnode2 = listNode;
            listNode->pnode1 = loopInvar;
            *outerStmtRef = listNode;
        }

        ParseNodeUni * tempName = Parser::StaticCreateTempRef(loopInvar, alloc);

        if (lhs != nullptr)
        {
            cStmt = Parser::StaticCreateBinNode(knopAsg, lhs, tempName, alloc);
        }
        else
        {
            // Use AddVarDeclNode to add the var to the function.
            // Do not use CreateVarDeclNode which is meant to be used while parsing. It assumes that
            // parser's internal data structures (m_ppnodeVar in particular) is at the "current" location.
            cStmt = byteCodeGenerator->GetParser()->AddVarDeclNode(stmt->AsParseNodeVar()->pid, funcInfo->root);
            cStmt->AsParseNodeVar()->pnodeInit = tempName;
            cStmt->AsParseNodeVar()->sym = stmt->AsParseNodeVar()->sym;
        }
    }
    else
    {
        cStmt = byteCodeGenerator->GetParser()->CopyPnode(stmt);
    }

    return cStmt;
}

ParseNodeFor* ConstructInvertedLoop(ParseNode* innerLoop, ParseNode* outerLoop, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    ArenaAllocator* alloc = byteCodeGenerator->GetAllocator();
    ParseNodeFor * outerLoopC = Parser::StaticCreateNodeT<knopFor>(alloc);
    outerLoopC->pnodeInit = innerLoop->AsParseNodeFor()->pnodeInit;
    outerLoopC->pnodeCond = innerLoop->AsParseNodeFor()->pnodeCond;
    outerLoopC->pnodeIncr = innerLoop->AsParseNodeFor()->pnodeIncr;
    outerLoopC->pnodeBlock = innerLoop->AsParseNodeFor()->pnodeBlock;
    outerLoopC->pnodeInverted = nullptr;

    ParseNodeFor * innerLoopC = Parser::StaticCreateNodeT<knopFor>(alloc);
    innerLoopC->pnodeInit = outerLoop->AsParseNodeFor()->pnodeInit;
    innerLoopC->pnodeCond = outerLoop->AsParseNodeFor()->pnodeCond;
    innerLoopC->pnodeIncr = outerLoop->AsParseNodeFor()->pnodeIncr;
    innerLoopC->pnodeBlock = outerLoop->AsParseNodeFor()->pnodeBlock;
    innerLoopC->pnodeInverted = nullptr;

    ParseNodeBlock * innerBod = Parser::StaticCreateBlockNode(alloc);
    innerLoopC->pnodeBody = innerBod;
    innerBod->scope = innerLoop->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->scope;

    ParseNodeBlock * outerBod = Parser::StaticCreateBlockNode(alloc);
    outerLoopC->pnodeBody = outerBod;
    outerBod->scope = outerLoop->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->scope;

    ParseNodeBin * listNode = Parser::StaticCreateBinNode(knopList, nullptr, nullptr, alloc);
    outerBod->pnodeStmt = listNode;

    ParseNode* innerBodOriginal = innerLoop->AsParseNodeFor()->pnodeBody;
    ParseNode* origStmt = innerBodOriginal->AsParseNodeBlock()->pnodeStmt;
    if (origStmt->nop == knopList)
    {
        ParseNode* invertedStmt = nullptr;
        while (origStmt->nop == knopList)
        {
            ParseNode* invertedItem = ConstructInvertedStatement(origStmt->AsParseNodeBin()->pnode1, byteCodeGenerator, funcInfo, &listNode);
            ParseNode * newInvertedStmt = Parser::StaticCreateBinNode(knopList, invertedItem, nullptr, alloc, invertedItem->ichMin, invertedItem->ichLim);
            if (invertedStmt != nullptr)
            {
                invertedStmt = invertedStmt->AsParseNodeBin()->pnode2 = newInvertedStmt;
            }
            else
            {
                invertedStmt = innerBod->pnodeStmt = newInvertedStmt;
            }
            origStmt = origStmt->AsParseNodeBin()->pnode2;
        }
        Assert(invertedStmt != nullptr);
        invertedStmt->AsParseNodeBin()->pnode2 = ConstructInvertedStatement(origStmt, byteCodeGenerator, funcInfo, &listNode);
    }
    else
    {
        innerBod->pnodeStmt = ConstructInvertedStatement(origStmt, byteCodeGenerator, funcInfo, &listNode);
    }

    if (listNode->pnode1 == nullptr)
    {
        listNode->pnode1 = Parser::StaticCreateTempNode(nullptr, alloc);
    }

    listNode->pnode2 = innerLoopC;
    return outerLoopC;
}

bool InvertableStmt(ParseNode* stmt, Symbol* outerVar, ParseNode* innerLoop, ParseNode* outerLoop, ByteCodeGenerator* byteCodeGenerator, SymCheck* symCheck)
{
    if (stmt != nullptr)
    {
        ParseNode* lhs = nullptr;
        ParseNode* rhs = nullptr;
        if (stmt->nop == knopAsg)
        {
            lhs = stmt->AsParseNodeBin()->pnode1;
            rhs = stmt->AsParseNodeBin()->pnode2;
        }
        else if (stmt->nop == knopVarDecl)
        {
            rhs = stmt->AsParseNodeVar()->pnodeInit;
        }

        if (lhs != nullptr)
        {
            if (lhs->nop == knopDot)
            {
                return false;
            }

            if (lhs->nop == knopName)
            {
                if ((lhs->AsParseNodeName()->sym != nullptr) && (lhs->AsParseNodeName()->sym->GetIsGlobal()))
                {
                    return false;
                }
            }
            else if (lhs->nop == knopIndex)
            {
                ParseNode* indexed = lhs->AsParseNodeBin()->pnode1;
                ParseNode* index = lhs->AsParseNodeBin()->pnode2;

                if ((index == nullptr) || (indexed == nullptr))
                {
                    return false;
                }

                if ((indexed->nop != knopName) || (indexed->AsParseNodeName()->sym == nullptr))
                {
                    return false;
                }

                if (!InvertableExprPlus(symCheck, index, byteCodeGenerator, outerVar))
                {
                    return false;
                }
            }
        }

        if (rhs != nullptr)
        {
            if (!InvertableExpr(symCheck, rhs, byteCodeGenerator))
            {
                return false;
            }
        }
        else
        {
            if (!InvertableExpr(symCheck, stmt, byteCodeGenerator))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

bool GatherInversionSyms(ParseNode* stmt, Symbol* outerVar, ParseNode* innerLoop, ByteCodeGenerator* byteCodeGenerator, SymCheck* symCheck)
{
    if (stmt != nullptr)
    {
        ParseNode* lhs = nullptr;
        Symbol* auxSym = nullptr;

        if (stmt->nop == knopAsg)
        {
            lhs = stmt->AsParseNodeBin()->pnode1;
        }
        else if (stmt->nop == knopVarDecl)
        {
            auxSym = stmt->AsParseNodeVar()->sym;
        }

        if (lhs != nullptr)
        {
            if (lhs->nop == knopDot)
            {
                return false;
            }

            if (lhs->nop == knopName)
            {
                ParseNodeName * pnodeNameLhs = lhs->AsParseNodeName();
                if ((pnodeNameLhs->sym == nullptr) || (pnodeNameLhs->sym->GetIsGlobal()))
                {
                    return false;
                }
                else
                {
                    auxSym = pnodeNameLhs->sym;
                }
            }
        }

        if (auxSym != nullptr)
        {
            return symCheck->AddSymbol(auxSym);
        }
    }

    return true;
}

bool InvertableBlock(ParseNode* block, Symbol* outerVar, ParseNode* innerLoop, ParseNode* outerLoop, ByteCodeGenerator* byteCodeGenerator,
    SymCheck* symCheck)
{
    if (block == nullptr)
    {
            return false;
        }

    if (!symCheck->AddSymbol(outerVar))
    {
            return false;
        }

        if ((innerLoop->AsParseNodeFor()->pnodeBody->nop == knopBlock && innerLoop->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->HasBlockScopedContent())
            || (outerLoop->AsParseNodeFor()->pnodeBody->nop == knopBlock && outerLoop->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->HasBlockScopedContent()))
        {
            // we can not invert loops if there are block scoped declarations inside
            return false;
        }

    if ((block != nullptr) && (block->nop == knopBlock))
    {
        ParseNode* stmt = block->AsParseNodeBlock()->pnodeStmt;
        while ((stmt != nullptr) && (stmt->nop == knopList))
        {
            if (!GatherInversionSyms(stmt->AsParseNodeBin()->pnode1, outerVar, innerLoop, byteCodeGenerator, symCheck))
            {
                    return false;
                }
            stmt = stmt->AsParseNodeBin()->pnode2;
            }

        if (!GatherInversionSyms(stmt, outerVar, innerLoop, byteCodeGenerator, symCheck))
        {
                return false;
            }

        stmt = block->AsParseNodeBlock()->pnodeStmt;
        while ((stmt != nullptr) && (stmt->nop == knopList))
        {
            if (!InvertableStmt(stmt->AsParseNodeBin()->pnode1, outerVar, innerLoop, outerLoop, byteCodeGenerator, symCheck))
            {
                    return false;
                }
            stmt = stmt->AsParseNodeBin()->pnode2;
            }

        if (!InvertableStmt(stmt, outerVar, innerLoop, outerLoop, byteCodeGenerator, symCheck))
        {
                return false;
            }

        return (InvertableExprPlus(symCheck, innerLoop->AsParseNodeFor()->pnodeCond, byteCodeGenerator, nullptr) &&
            InvertableExprPlus(symCheck, outerLoop->AsParseNodeFor()->pnodeCond, byteCodeGenerator, outerVar));
        }
    else
    {
            return false;
        }
}

// Start of invert loop optimization.
// For now, find simple cases (only for loops around single assignment).
// Returns new AST for inverted loop; also returns in out param
// side effects level, if any that guards the new AST (old AST will be
// used if guard fails).
// Should only be called with loopNode representing top-level statement.
ParseNodeFor* InvertLoop(ParseNode* outerLoop, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo)
{
    if (byteCodeGenerator->GetScriptContext()->optimizationOverrides.GetSideEffects() != Js::SideEffects_None)
    {
        return nullptr;
    }

    SymCheck symCheck;
    symCheck.Init();

    if (outerLoop->nop == knopFor)
    {
        ParseNode* innerLoop = outerLoop->AsParseNodeFor()->pnodeBody;
        if ((innerLoop == nullptr) || (innerLoop->nop != knopBlock))
        {
            return nullptr;
        }
        else
        {
            innerLoop = innerLoop->AsParseNodeBlock()->pnodeStmt;
        }

        if ((innerLoop != nullptr) && (innerLoop->nop == knopFor))
        {
            if ((outerLoop->AsParseNodeFor()->pnodeInit != nullptr) &&
                (outerLoop->AsParseNodeFor()->pnodeInit->nop == knopVarDecl) &&
                (outerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pnodeInit != nullptr) &&
                (outerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pnodeInit->nop == knopInt) &&
                (outerLoop->AsParseNodeFor()->pnodeIncr != nullptr) &&
                ((outerLoop->AsParseNodeFor()->pnodeIncr->nop == knopIncPre) || (outerLoop->AsParseNodeFor()->pnodeIncr->nop == knopIncPost)) &&
                (outerLoop->AsParseNodeFor()->pnodeIncr->AsParseNodeUni()->pnode1->nop == knopName) &&
                (outerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pid == outerLoop->AsParseNodeFor()->pnodeIncr->AsParseNodeUni()->pnode1->AsParseNodeName()->pid) &&
                (innerLoop->AsParseNodeFor()->pnodeIncr != nullptr) &&
                ((innerLoop->AsParseNodeFor()->pnodeIncr->nop == knopIncPre) || (innerLoop->AsParseNodeFor()->pnodeIncr->nop == knopIncPost)) &&
                (innerLoop->AsParseNodeFor()->pnodeInit != nullptr) &&
                (innerLoop->AsParseNodeFor()->pnodeInit->nop == knopVarDecl) &&
                (innerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pnodeInit != nullptr) &&
                (innerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pnodeInit->nop == knopInt) &&
                (innerLoop->AsParseNodeFor()->pnodeIncr->AsParseNodeUni()->pnode1->nop == knopName) &&
                (innerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->pid == innerLoop->AsParseNodeFor()->pnodeIncr->AsParseNodeUni()->pnode1->AsParseNodeName()->pid))
            {
                Symbol* outerVar = outerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->sym;
                Symbol* innerVar = innerLoop->AsParseNodeFor()->pnodeInit->AsParseNodeVar()->sym;
                if ((outerVar != nullptr) && (innerVar != nullptr))
                {
                    ParseNode* block = innerLoop->AsParseNodeFor()->pnodeBody;
                    if (InvertableBlock(block, outerVar, innerLoop, outerLoop, byteCodeGenerator, &symCheck))
                    {
                        return ConstructInvertedLoop(innerLoop, outerLoop, byteCodeGenerator, funcInfo);
                    }
                }
            }
        }
    }

    return nullptr;
}

void SetAdditionalBindInfoForVariables(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    Symbol *sym = pnode->AsParseNodeVar()->sym;
    if (sym == nullptr)
    {
        return;
    }

    FuncInfo* func = byteCodeGenerator->TopFuncInfo();
    if (!sym->GetIsGlobal() && !sym->IsArguments() &&
        (sym->GetScope() == func->GetBodyScope() || sym->GetScope() == func->GetParamScope() || sym->GetScope()->GetCanMerge()))
    {
        if (func->GetChildCallsEval())
        {
            func->SetHasLocalInClosure(true);
        }
        else
        {
            sym->RecordDef();
        }
    }
    if (sym->IsUsedInLdElem())
    {
        Ident::TrySetIsUsedInLdElem(pnode->AsParseNodeVar()->pnodeInit);
    }

    // If this decl does an assignment inside a loop body, then there's a chance
    // that a jitted loop body will expect us to begin with a valid value in this var.
    // So mark the sym as used so that we guarantee the var will at least get "undefined".
    if (byteCodeGenerator->IsInLoop() &&
        pnode->AsParseNodeVar()->pnodeInit)
    {
        sym->SetIsUsed(true);
    }
}

// bind references to definitions (prefix pass)
void Bind(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    if (pnode == nullptr)
{
        return;
    }

    switch (pnode->nop)
    {
    case knopBreak:
    case knopContinue:
        byteCodeGenerator->AddTargetStmt(pnode->AsParseNodeJump()->pnodeTarget);
        break;
    case knopProg:
        {
            FuncInfo* globFuncInfo = byteCodeGenerator->StartBindGlobalStatements(pnode->AsParseNodeProg());
            pnode->AsParseNodeFnc()->funcInfo = globFuncInfo;
            AddFunctionsToScope(pnode->AsParseNodeFnc()->GetTopLevelScope(), byteCodeGenerator);
            AddVarsToScope(pnode->AsParseNodeFnc()->pnodeVars, byteCodeGenerator);
            // There are no args to add, but "eval" gets a this pointer.
            byteCodeGenerator->SetNumberOfInArgs(!!(byteCodeGenerator->GetFlags() & fscrEvalCode));
            if (!globFuncInfo->IsFakeGlobalFunction(byteCodeGenerator->GetFlags()))
            {
                // Global code: the root function is the global function.
                byteCodeGenerator->SetRootFuncInfo(globFuncInfo);
            }
            else if (globFuncInfo->byteCodeFunction)
            {
            // If the current global code wasn't marked to be treated as global code (e.g. from deferred parsing),
            // we don't need to send a register script event for it.
                globFuncInfo->byteCodeFunction->SetIsTopLevel(false);
            }
            if (pnode->AsParseNodeFnc()->CallsEval())
            {
                globFuncInfo->SetCallsEval(true);
            }
            break;
        }
    case knopFncDecl:
        if (pnode->AsParseNodeFnc()->IsCoroutine())
        {
            // Always assume generator functions escape since tracking them requires tracking
            // the resulting generators in addition to the function.
            byteCodeGenerator->FuncEscapes(byteCodeGenerator->TopFuncInfo()->GetBodyScope());
        }
        if (!pnode->AsParseNodeFnc()->IsDeclaration())
        {
            FuncInfo *funcInfo = byteCodeGenerator->TopFuncInfo();
            if (!funcInfo->IsGlobalFunction() || (byteCodeGenerator->GetFlags() & fscrEval))
            {
                // In the case of a nested function expression, assumes that it escapes.
                // We could try to analyze what it touches to be more precise.
                byteCodeGenerator->FuncEscapes(funcInfo->GetBodyScope());
            }
            byteCodeGenerator->ProcessCapturedSyms(pnode);
        }
        else if (byteCodeGenerator->IsInLoop())
        {
            Symbol *funcSym = pnode->AsParseNodeFnc()->GetFuncSymbol();
            if (funcSym)
            {
                Symbol *funcVarSym = funcSym->GetFuncScopeVarSym();
                if (funcVarSym)
                {
                    // We're going to write to the funcVarSym when we do the function instantiation,
                    // so treat the funcVarSym as used. That way, we know it will get undef-initialized at the
                    // top of the function, so a jitted loop body won't have any issue with boxing if
                    // the function instantiation isn't executed.
                    Assert(funcVarSym != funcSym);
                    funcVarSym->SetIsUsed(true);
                }
            }
        }
        break;
    case knopName:
    {
        ParseNodeName * pnodeName = pnode->AsParseNodeName();
        if (pnodeName->sym == nullptr)
        {
            if (pnodeName->grfpn & fpnMemberReference)
            {
                // This is a member name. No binding.
                break;
            }

            Symbol *sym = byteCodeGenerator->FindSymbol(pnodeName->GetSymRef(), pnodeName->pid);
            if (sym)
            {
                // This is a named load, not just a reference, so if it's a nested function note that all
                // the nested scopes escape.
                Assert(!sym->GetDecl() || (pnodeName->GetSymRef() && *pnodeName->GetSymRef()));
                Assert(!sym->GetDecl() || ((*pnodeName->GetSymRef())->GetDecl() == sym->GetDecl()) ||
                       ((*pnodeName->GetSymRef())->GetFuncScopeVarSym() == sym));

                pnodeName->sym = sym;
                if (sym->GetSymbolType() == STFunction &&
                    (!sym->GetIsGlobal() || (byteCodeGenerator->GetFlags() & fscrEval)))
                {
                    byteCodeGenerator->FuncEscapes(sym->GetScope());
                }
            }
        }

        if (pnodeName->sym)
        {
            pnodeName->sym->SetIsUsed(true);
        }

        break;
    }
    case knopMember:
    case knopMemberShort:
    case knopObjectPatternMember:
    case knopGetMember:
    case knopSetMember:

        {
            // lhs is knopStr, rhs is expr
            ParseNode *id = pnode->AsParseNodeBin()->pnode1;
            if (id->nop == knopStr)
            {
                byteCodeGenerator->AssignPropertyId(id->AsParseNodeStr()->pid);
                id->grfpn |= fpnMemberReference;
            }
            break;
        }

        // TODO: convert index over string to Get/Put Value
    case knopIndex:
        BindReference(pnode, byteCodeGenerator);
        break;
    case knopDot:
        BindInstAndMember(pnode, byteCodeGenerator);
        break;
    case knopTryFinally:
        byteCodeGenerator->SetHasFinally(true);
    case knopTryCatch:
        byteCodeGenerator->SetHasTry(true);
        byteCodeGenerator->TopFuncInfo()->byteCodeFunction->SetDontInline(true);
        byteCodeGenerator->AddTargetStmt(pnode->AsParseNodeStmt());
        break;
    case knopAsg:
        BindReference(pnode, byteCodeGenerator);
        CheckLocalVarDef(pnode, byteCodeGenerator);
        break;
    case knopVarDecl:
        // "arguments" symbol or decl w/o RHS may have been bound already; otherwise, do the binding here.
        if (pnode->AsParseNodeVar()->sym == nullptr)
        {
            pnode->AsParseNodeVar()->sym = byteCodeGenerator->FindSymbol(pnode->AsParseNodeVar()->symRef, pnode->AsParseNodeVar()->pid);
        }
        SetAdditionalBindInfoForVariables(pnode, byteCodeGenerator);
        break;
    case knopConstDecl:
    case knopLetDecl:
        // "arguments" symbol or decl w/o RHS may have been bound already; otherwise, do the binding here.
        if (!pnode->AsParseNodeVar()->sym)
        {
            AssertMsg(pnode->AsParseNodeVar()->symRef && *pnode->AsParseNodeVar()->symRef, "'const' and 'let' should be binded when we bind block");
            pnode->AsParseNodeVar()->sym = *pnode->AsParseNodeVar()->symRef;
        }
        SetAdditionalBindInfoForVariables(pnode, byteCodeGenerator);
        break;
    case knopCall:
    case knopTypeof:
    case knopDelete:
        BindReference(pnode, byteCodeGenerator);
        break;

    case knopRegExp:
        pnode->AsParseNodeRegExp()->regexPatternIndex = byteCodeGenerator->TopFuncInfo()->GetParsedFunctionBody()->NewLiteralRegex();
        break;

    case knopComma:
        pnode->AsParseNodeBin()->pnode1->SetNotEscapedUse();
        break;

    case knopBlock:
    {
        for (ParseNode *pnodeScope = pnode->AsParseNodeBlock()->pnodeScopes; pnodeScope; /* no increment */)
        {
            switch (pnodeScope->nop)
            {
            case knopFncDecl:
                if (pnodeScope->AsParseNodeFnc()->IsDeclaration())
                {
                    byteCodeGenerator->ProcessCapturedSyms(pnodeScope);
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
        break;
    }

    }
}

void ByteCodeGenerator::ProcessCapturedSyms(ParseNode *pnode)
{
    SymbolTable *capturedSyms = pnode->AsParseNodeFnc()->funcInfo->GetCapturedSyms();
    if (capturedSyms)
    {
        FuncInfo *funcInfo = this->TopFuncInfo();
        CapturedSymMap *capturedSymMap = funcInfo->EnsureCapturedSymMap();
        ParseNode *pnodeStmt = this->GetCurrentTopStatement();

        SList<Symbol*> *capturedSymList;
        if (!pnodeStmt->CapturesSyms())
        {
            capturedSymList = Anew(this->alloc, SList<Symbol*>, this->alloc);
            capturedSymMap->Add(pnodeStmt, capturedSymList);
            pnodeStmt->SetCapturesSyms();
        }
        else
        {
            capturedSymList = capturedSymMap->Item(pnodeStmt);
        }

        capturedSyms->Map([&](Symbol *sym)
        {
            if (!sym->GetIsCommittedToSlot() && !sym->HasVisitedCapturingFunc())
            {
                capturedSymList->Prepend(sym);
                sym->SetHasVisitedCapturingFunc();
            }
        });
    }
}

void ByteCodeGenerator::FuncEscapes(Scope *scope)
{
    while (scope)
    {
        Assert(scope->GetFunc());
        scope->GetFunc()->SetEscapes(true);
        scope = scope->GetEnclosingScope();
    }

    if (this->flags & fscrEval)
    {
        // If a function declared inside eval escapes, we'll need
        // to invalidate the caller's cached scope.
        this->funcEscapes = true;
    }
}

bool ByteCodeGenerator::HasInterleavingDynamicScope(Symbol * sym) const
{
    Js::PropertyId unused;
    return this->InDynamicScope() &&
        sym->GetScope() != this->FindScopeForSym(sym->GetScope(), nullptr, &unused, this->TopFuncInfo());
}

void CheckMaybeEscapedUse(ParseNode * pnode, ByteCodeGenerator * byteCodeGenerator, bool isCall = false)
{
    if (pnode == nullptr)
    {
        return;
    }

    FuncInfo * topFunc = byteCodeGenerator->TopFuncInfo();
    if (topFunc->IsGlobalFunction())
    {
        return;
    }

    switch (pnode->nop)
    {
    case knopAsg:
        if (pnode->AsParseNodeBin()->pnode1->nop != knopName)
        {
            break;
        }
        // use of an assignment (e.g. (y = function() {}) + "1"), just make y an escaped use.
        pnode = pnode->AsParseNodeBin()->pnode1;
        isCall = false;
        // fall-through
    case knopName:
        if (!isCall)
        {
            // Mark the name has having escaped use
            if (pnode->AsParseNodeName()->sym)
            {
                pnode->AsParseNodeName()->sym->SetHasMaybeEscapedUse(byteCodeGenerator);
            }
        }
        break;

    case knopFncDecl:
        // A function declaration has an unknown use (not assignment nor call),
        // mark the function as having child escaped
        topFunc->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("UnknownUse")));
        break;
    }
}

void CheckFuncAssignment(Symbol * sym, ParseNode * pnode2, ByteCodeGenerator * byteCodeGenerator)
{
    if (pnode2 == nullptr)
    {
        return;
    }

    switch (pnode2->nop)
    {
    default:
        CheckMaybeEscapedUse(pnode2, byteCodeGenerator);
        break;
    case knopFncDecl:
        {
            FuncInfo * topFunc = byteCodeGenerator->TopFuncInfo();
            if (topFunc->IsGlobalFunction())
            {
                return;
            }
            // Use not as an assignment or assignment to an outer function's sym, or assigned to a formal
        // or assigned to multiple names.

            if (sym == nullptr
                || sym->GetScope()->GetFunc() != topFunc)
            {
                topFunc->SetHasMaybeEscapedNestedFunc(DebugOnly(
                sym == nullptr ? _u("UnknownAssignment") :
                (sym->GetScope()->GetFunc() != topFunc) ? _u("CrossFuncAssignment") :
                    _u("SomethingIsWrong!"))
                    );
            }
            else
            {
                // TODO-STACK-NESTED-FUNC: Since we only support single def functions, we can still put the
            // nested function on the stack and reuse even if the function goes out of the block scope.
                // However, we cannot allocate frame display or slots on the stack if the function is
            // declared in a loop, because there might be multiple functions referencing different
            // iterations of the scope.
                // For now, just disable everything.

                Scope * funcParentScope = pnode2->AsParseNodeFnc()->funcInfo->GetBodyScope()->GetEnclosingScope();
                while (sym->GetScope() != funcParentScope)
                {
                    if (funcParentScope->GetMustInstantiate())
                    {
                        topFunc->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("CrossScopeAssignment")));
                        break;
                    }
                    funcParentScope->SetHasCrossScopeFuncAssignment();
                    funcParentScope = funcParentScope->GetEnclosingScope();
                }

            // Need to always detect interleaving dynamic scope ('with') for assignments
            // as those may end up escaping into the 'with' scope.
                // TODO: the with scope is marked as MustInstantiate late during byte code emit
                // We could detect this using the loop above as well, by marking the with
            // scope as must instantiate early, this is just less risky of a fix for RTM.

                if (byteCodeGenerator->HasInterleavingDynamicScope(sym))
                {
                     byteCodeGenerator->TopFuncInfo()->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("InterleavingDynamicScope")));
                }

                sym->SetHasFuncAssignment(byteCodeGenerator);
            }
        }
        break;
    };
}

// Assign permanent (non-temp) registers for the function.
// These include constants (null, 3.7, this) and locals that use registers as their home locations.
// Assign the location fields of parse nodes whose values are constants/locals with permanent/known registers.
// Re-usable expression temps are assigned during the final Emit pass.
void AssignRegisters(ParseNode *pnode, ByteCodeGenerator *byteCodeGenerator)
{
    if (pnode == nullptr)
    {
        return;
    }

    Symbol *sym;
    OpCode nop = pnode->nop;
    switch (nop)
    {
    default:
        {
            uint flags = ParseNode::Grfnop(nop);
            if (flags & fnopUni)
            {
                CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
            }
            else if (flags & fnopBin)
            {
                CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator);
                CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator);
            }
        break;
    }

    case knopParamPattern:
        byteCodeGenerator->AssignUndefinedConstRegister();
        CheckMaybeEscapedUse(pnode->AsParseNodeParamPattern()->pnode1, byteCodeGenerator);
        break;

    case knopObjectPattern:
    case knopArrayPattern:
        byteCodeGenerator->AssignUndefinedConstRegister();
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;

    case knopDot:
        CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator);
        break;
    case knopMember:
    case knopMemberShort:
    case knopGetMember:
    case knopSetMember:
        CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator);
        break;

    case knopAsg:
        {
            Symbol * symName = pnode->AsParseNodeBin()->pnode1->nop == knopName ? pnode->AsParseNodeBin()->pnode1->AsParseNodeName()->sym : nullptr;
            CheckFuncAssignment(symName, pnode->AsParseNodeBin()->pnode2, byteCodeGenerator);

            if (pnode->IsInList())
            {
                // Assignment in array literal
                CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode1, byteCodeGenerator);
            }

            if (byteCodeGenerator->IsES6DestructuringEnabled() && (pnode->AsParseNodeBin()->pnode1->nop == knopArrayPattern || pnode->AsParseNodeBin()->pnode1->nop == knopObjectPattern))
            {
                // Destructured arrays may have default values and need undefined.
                byteCodeGenerator->AssignUndefinedConstRegister();

                // Any rest parameter in a destructured array will need a 0 constant.
                byteCodeGenerator->EnregisterConstant(0);
            }

        break;
    }

    case knopEllipsis:
        if (byteCodeGenerator->InDestructuredPattern())
        {
            // Get a register for the rest array counter.
            pnode->location = byteCodeGenerator->NextVarRegister();

            // Any rest parameter in a destructured array will need a 0 constant.
            byteCodeGenerator->EnregisterConstant(0);
        }
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;

    case knopQmark:
        CheckMaybeEscapedUse(pnode->AsParseNodeTri()->pnode1, byteCodeGenerator);
        CheckMaybeEscapedUse(pnode->AsParseNodeTri()->pnode2, byteCodeGenerator);
        CheckMaybeEscapedUse(pnode->AsParseNodeTri()->pnode3, byteCodeGenerator);
        break;
    case knopWith:
        pnode->location = byteCodeGenerator->NextVarRegister();
        CheckMaybeEscapedUse(pnode->AsParseNodeWith()->pnodeObj, byteCodeGenerator);
        break;
    case knopComma:
        if (!pnode->IsNotEscapedUse())
        {
            // Only the last expr in comma expr escape. Mark it if it is escapable.
            CheckMaybeEscapedUse(pnode->AsParseNodeBin()->pnode2, byteCodeGenerator);
        }
        break;
    case knopFncDecl:
        if (!byteCodeGenerator->TopFuncInfo()->IsGlobalFunction())
        {
            if (pnode->AsParseNodeFnc()->IsCoroutine())
            {
                // Assume generators always escape; otherwise need to analyze if
                // the return value of calls to generator function, the generator
                // objects, escape.
                FuncInfo* funcInfo = byteCodeGenerator->TopFuncInfo();
                funcInfo->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("Generator")));
            }

            if (pnode->IsInList() && !pnode->IsNotEscapedUse())
            {
                byteCodeGenerator->TopFuncInfo()->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("InList")));
            }

            ParseNodePtr pnodeName = pnode->AsParseNodeFnc()->pnodeName;
            if (pnodeName != nullptr)
            {
                // REVIEW: does this apply now that compat mode is gone?
                // There is a weird case in compat mode where we may not have a sym assigned to a fnc decl's
                // name node if it is a named function declare inside 'with' that also assigned to something else
                // as well. Instead, We generate two knopFncDecl node one for parent function and one for the assignment.
                // Only the top one gets a sym, not the inner one.  The assignment in the 'with' will be using the inner
                // one.  Also we will detect that the assignment to a variable is an escape inside a 'with'.
                // Since we need the sym in the fnc decl's name, we just detect the escape here as "WithScopeFuncName".

                if (pnodeName->nop == knopVarDecl && pnodeName->AsParseNodeVar()->sym != nullptr)
                {
                    // Unlike in CheckFuncAssignment, we don't check for interleaving
                    // dynamic scope ('with') here, because we also generate direct assignment for
                    // function decl's names

                    pnodeName->AsParseNodeVar()->sym->SetHasFuncAssignment(byteCodeGenerator);

                    // Function declaration in block scope and non-strict mode has a
                    // corresponding var sym that we assign to as well.  Need to
                    // mark that symbol as has func assignment as well.
                    Symbol * functionScopeVarSym = pnodeName->AsParseNodeVar()->sym->GetFuncScopeVarSym();
                    if (functionScopeVarSym)
                    {
                        functionScopeVarSym->SetHasFuncAssignment(byteCodeGenerator);
                    }
                }
                else
                {
                    // The function has multiple names, or assign to o.x or o::x
                    byteCodeGenerator->TopFuncInfo()->SetHasMaybeEscapedNestedFunc(DebugOnly(
                        pnodeName->nop == knopList ? _u("MultipleFuncName") :
                        pnodeName->nop == knopDot ? _u("PropFuncName") :
                        pnodeName->nop == knopVarDecl && pnodeName->AsParseNodeVar()->sym == nullptr ? _u("WithScopeFuncName") :
                        _u("WeirdFuncName")
                    ));
                }
            }
        }

        break;
    case knopNew:
        CheckMaybeEscapedUse(pnode->AsParseNodeCall()->pnodeTarget, byteCodeGenerator);
        CheckMaybeEscapedUse(pnode->AsParseNodeCall()->pnodeArgs, byteCodeGenerator);
        break;
    case knopThrow:
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;

    // REVIEW: Technically, switch expr or case expr doesn't really escape as strict equal
    // doesn't cause the function to escape.
    case knopSwitch:
        CheckMaybeEscapedUse(pnode->AsParseNodeSwitch()->pnodeVal, byteCodeGenerator);
        break;
    case knopCase:
        CheckMaybeEscapedUse(pnode->AsParseNodeCase()->pnodeExpr, byteCodeGenerator);
        break;

    // REVIEW: Technically, the object for GetForInEnumerator doesn't escape, except when cached,
    // which we can make work.
    case knopForIn:
        CheckMaybeEscapedUse(pnode->AsParseNodeForInOrForOf()->pnodeObj, byteCodeGenerator);
        break;

    case knopForOf:
        byteCodeGenerator->AssignNullConstRegister();
        byteCodeGenerator->AssignUndefinedConstRegister();
        CheckMaybeEscapedUse(pnode->AsParseNodeForInOrForOf()->pnodeObj, byteCodeGenerator);
        break;

    case knopTrue:
        pnode->location = byteCodeGenerator->AssignTrueConstRegister();
        break;

    case knopFalse:
        pnode->location = byteCodeGenerator->AssignFalseConstRegister();
        break;

    case knopDecPost:
    case knopIncPost:
    case knopDecPre:
    case knopIncPre:
        byteCodeGenerator->EnregisterConstant(1);
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;
    case knopObject:
        byteCodeGenerator->AssignNullConstRegister();
        break;
    case knopClassDecl:
        {
            FuncInfo * topFunc = byteCodeGenerator->TopFuncInfo();
            topFunc->SetHasMaybeEscapedNestedFunc(DebugOnly(_u("Class")));

            // We may need undefined for the 'this', e.g. calling a class expression
            byteCodeGenerator->AssignUndefinedConstRegister();

        break;
        }
    case knopNull:
        pnode->location = byteCodeGenerator->AssignNullConstRegister();
        break;
    case knopCall:
    {
        if (pnode->AsParseNodeCall()->pnodeTarget->nop != knopIndex &&
            pnode->AsParseNodeCall()->pnodeTarget->nop != knopDot)
        {
            byteCodeGenerator->AssignUndefinedConstRegister();
        }

        FuncInfo *funcInfo = byteCodeGenerator->TopFuncInfo();

        if (pnode->AsParseNodeCall()->isEvalCall)
        {
            if (!funcInfo->GetParsedFunctionBody()->IsReparsed())
            {
                Assert(funcInfo->IsGlobalFunction() || funcInfo->GetCallsEval());
                funcInfo->SetCallsEval(true);
                funcInfo->GetParsedFunctionBody()->SetCallsEval(true);
            }
            else
            {
                // On reparsing, load the state from function Body, instead of using the state on the parse node,
                // as they might be different.
                pnode->AsParseNodeCall()->isEvalCall = funcInfo->GetParsedFunctionBody()->GetCallsEval();
            }
        }
        // Don't need to check pnode->AsParseNodeCall()->pnodeTarget even if it is a knopFncDecl,
        // e.g. (function(){})();
        // It is only used as a call, so don't count as an escape.
        // Although not assigned to a slot, we will still able to box it by boxing
        // all the stack function on the interpreter frame or the stack function link list
        // on a jitted frame
        break;
    }

    case knopInt:
        pnode->location = byteCodeGenerator->EnregisterConstant(pnode->AsParseNodeInt()->lw);
        break;
    case knopFlt:
    {
        pnode->location = byteCodeGenerator->EnregisterDoubleConstant(pnode->AsParseNodeFloat()->dbl);
        break;
    }
    case knopBigInt:
        pnode->location = byteCodeGenerator->EnregisterBigIntConstant(pnode);
        break;
    case knopStr:
        pnode->location = byteCodeGenerator->EnregisterStringConstant(pnode->AsParseNodeStr()->pid);
        break;
    case knopVarDecl:
    case knopConstDecl:
    case knopLetDecl:
        {
            sym = pnode->AsParseNodeVar()->sym;
            Assert(sym != nullptr);

            Assert(sym->GetScope()->GetEnclosingFunc() == byteCodeGenerator->TopFuncInfo());

            if (pnode->AsParseNodeVar()->isBlockScopeFncDeclVar && sym->GetIsBlockVar())
            {
                break;
            }

            if (!sym->GetIsGlobal())
            {
                FuncInfo *funcInfo = byteCodeGenerator->TopFuncInfo();

                // Check the function assignment for the sym that we have, even if we remap it to function level sym below
                // as we are going assign to the original sym
                CheckFuncAssignment(sym, pnode->AsParseNodeVar()->pnodeInit, byteCodeGenerator);

                // If this is a destructured param case then it is a let binding and we don't have to look for duplicate symbol in the body
                if ((sym->GetIsCatch() && pnode->AsParseNodeVar()->sym->GetScope()->GetScopeType() != ScopeType_CatchParamPattern) || (pnode->nop == knopVarDecl && sym->GetIsBlockVar() && !pnode->AsParseNodeVar()->isBlockScopeFncDeclVar))
                {
                    // The LHS of the var decl really binds to the local symbol, not the catch or let symbol.
                    // But the assignment will go to the catch or let symbol. Just assign a register to the local
                    // so that it can get initialized to undefined.
#if DBG
                    if (!sym->GetIsCatch())
                    {
                        // Catch cannot be at function scope and let and var at function scope is redeclaration error.
                        Assert(funcInfo->bodyScope != sym->GetScope());
                    }
#endif
                    auto symName = sym->GetName();
                    sym = funcInfo->bodyScope->FindLocalSymbol(symName);

                    if (sym == nullptr)
                    {
                        sym = funcInfo->paramScope->FindLocalSymbol(symName);
                    }
                    Assert((sym && !sym->GetIsCatch() && !sym->GetIsBlockVar()));
                }
                // Don't give the declared var a register if it's in a closure, because the closure slot
                // is its true "home". (Need to check IsGlobal again as the sym may have changed above.)
                if (!sym->GetIsGlobal() && !sym->IsInSlot(byteCodeGenerator, funcInfo))
                {
                    if (PHASE_TRACE(Js::DelayCapturePhase, funcInfo->byteCodeFunction))
                    {
                        if (sym->NeedsSlotAlloc(byteCodeGenerator, byteCodeGenerator->TopFuncInfo()))
                        {
                            Output::Print(_u("--- DelayCapture: Delayed capturing symbol '%s' during initialization.\n"),
                                sym->GetName().GetBuffer());
                            Output::Flush();
                        }
                    }
                    byteCodeGenerator->AssignRegister(sym);
                }
            }
            else
            {
                Assert(byteCodeGenerator->TopFuncInfo()->IsGlobalFunction());
            }

            break;
        }

    case knopFor:
        if ((pnode->AsParseNodeFor()->pnodeBody != nullptr) && (pnode->AsParseNodeFor()->pnodeBody->nop == knopBlock) &&
            (pnode->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->pnodeStmt != nullptr) &&
            (pnode->AsParseNodeFor()->pnodeBody->AsParseNodeBlock()->pnodeStmt->nop == knopFor) &&
            (!byteCodeGenerator->IsInDebugMode()))
        {
                FuncInfo *funcInfo = byteCodeGenerator->TopFuncInfo();
            pnode->AsParseNodeFor()->pnodeInverted = InvertLoop(pnode, byteCodeGenerator, funcInfo);
        }
        else
        {
            pnode->AsParseNodeFor()->pnodeInverted = nullptr;
        }

        break;

    case knopName:
        sym = pnode->AsParseNodeName()->sym;
        if (sym == nullptr)
        {
            Assert(pnode->AsParseNodeName()->pid->GetPropertyId() != Js::Constants::NoProperty);

            // Referring to 'this' with no var decl needs to load 'this' root value via LdThis from null
            if (ByteCodeGenerator::IsThis(pnode) && !byteCodeGenerator->TopFuncInfo()->GetThisSymbol() && !(byteCodeGenerator->GetFlags() & fscrEval))
            {
                byteCodeGenerator->AssignNullConstRegister();
                byteCodeGenerator->AssignThisConstRegister();
            }
        }
        else
        {
            // Note: don't give a register to a local if it's in a closure, because then the closure
            // is its true home.
            if (!sym->GetIsGlobal() &&
                !sym->GetIsMember() &&
                byteCodeGenerator->TopFuncInfo() == sym->GetScope()->GetEnclosingFunc() &&
                !sym->IsInSlot(byteCodeGenerator, byteCodeGenerator->TopFuncInfo()) &&
                !sym->HasVisitedCapturingFunc())
            {
                if (PHASE_TRACE(Js::DelayCapturePhase, byteCodeGenerator->TopFuncInfo()->byteCodeFunction))
                {
                    if (sym->NeedsSlotAlloc(byteCodeGenerator, byteCodeGenerator->TopFuncInfo()))
                    {
                        Output::Print(_u("--- DelayCapture: Delayed capturing symbol '%s'.\n"),
                            sym->GetName().GetBuffer());
                        Output::Flush();
                    }
                }

                // Local symbol being accessed in its own frame. Even if "with" or event
                // handler semantics make the binding ambiguous, it has a home location,
                // so assign it.
                byteCodeGenerator->AssignRegister(sym);

                // If we're in something like a "with" we'll need a scratch register to hold
                // the multiple possible values of the property.
                if (!byteCodeGenerator->HasInterleavingDynamicScope(sym))
                {
                    // We're not in a dynamic scope, or our home scope is nested within the dynamic scope, so we
                    // don't have to do dynamic binding. Just use the home location for this reference.
                    pnode->location = sym->GetLocation();
                }
            }
        }
        if (pnode->IsInList() && !pnode->IsNotEscapedUse())
        {
            // A node that is in a list is assumed to be escape, unless marked otherwise.
            // This includes array literal list/object literal list
            CheckMaybeEscapedUse(pnode, byteCodeGenerator);
        }
        break;

    case knopProg:
        if (!byteCodeGenerator->HasParentScopeInfo())
        {
            // If we're compiling a nested deferred function, don't pop the scope stack,
            // because we just want to leave it as-is for the emit pass.
            PostVisitFunction(pnode->AsParseNodeFnc(), byteCodeGenerator);
        }
        break;
    case knopReturn:
        {
            ParseNode *pnodeExpr = pnode->AsParseNodeReturn()->pnodeExpr;
            CheckMaybeEscapedUse(pnodeExpr, byteCodeGenerator);
            break;
        }

    case knopStrTemplate:
        {
            ParseNode* pnodeExprs = pnode->AsParseNodeStrTemplate()->pnodeSubstitutionExpressions;
            if (pnodeExprs != nullptr)
            {
                while (pnodeExprs->nop == knopList)
                {
                    Assert(pnodeExprs->AsParseNodeBin()->pnode1 != nullptr);
                    Assert(pnodeExprs->AsParseNodeBin()->pnode2 != nullptr);

                    CheckMaybeEscapedUse(pnodeExprs->AsParseNodeBin()->pnode1, byteCodeGenerator);
                    pnodeExprs = pnodeExprs->AsParseNodeBin()->pnode2;
                }

                // Also check the final element in the list
                CheckMaybeEscapedUse(pnodeExprs, byteCodeGenerator);
            }

            if (pnode->AsParseNodeStrTemplate()->isTaggedTemplate)
            {
                pnode->location = byteCodeGenerator->EnregisterStringTemplateCallsiteConstant(pnode);
            }
            break;
        }
    case knopExportDefault:
        {
            ParseNode* expr = pnode->AsParseNodeExportDefault()->pnodeExpr;

            if (expr != nullptr)
            {
                CheckMaybeEscapedUse(expr, byteCodeGenerator);
            }

            break;
        }
    case knopYieldLeaf:
        byteCodeGenerator->AssignUndefinedConstRegister();
        break;
    case knopYield:
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;
    case knopYieldStar:
        byteCodeGenerator->AssignNullConstRegister();
        byteCodeGenerator->AssignUndefinedConstRegister();
        CheckMaybeEscapedUse(pnode->AsParseNodeUni()->pnode1, byteCodeGenerator);
        break;
    }
}

// TODO[ianhall]: ApplyEnclosesArgs should be in ByteCodeEmitter.cpp but that becomes complicated because it depends on VisitIndirect
void PostCheckApplyEnclosesArgs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, ApplyCheck* applyCheck);
void CheckApplyEnclosesArgs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, ApplyCheck* applyCheck);
bool ApplyEnclosesArgs(ParseNode* fncDecl, ByteCodeGenerator* byteCodeGenerator)
{
    if (byteCodeGenerator->IsInDebugMode())
    {
        // Inspection of the arguments object will be messed up if we do ApplyArgs.
        return false;
    }

    if (!fncDecl->HasVarArguments()
        && fncDecl->AsParseNodeFnc()->pnodeParams == nullptr
        && fncDecl->AsParseNodeFnc()->pnodeRest == nullptr
        && fncDecl->AsParseNodeFnc()->nestedCount == 0)
    {
        ApplyCheck applyCheck;
        applyCheck.matches = true;
        applyCheck.sawApply = false;
        applyCheck.insideApplyCall = false;
        VisitIndirect<ApplyCheck>(fncDecl->AsParseNodeFnc()->pnodeBody, byteCodeGenerator, &applyCheck, &CheckApplyEnclosesArgs, &PostCheckApplyEnclosesArgs);
        return applyCheck.matches&&applyCheck.sawApply;
    }

    return false;
}

// TODO[ianhall]: VisitClearTmpRegs should be in ByteCodeEmitter.cpp but that becomes complicated because it depends on VisitIndirect
void ClearTmpRegs(ParseNode* pnode, ByteCodeGenerator* byteCodeGenerator, FuncInfo* emitFunc);
void VisitClearTmpRegs(ParseNode * pnode, ByteCodeGenerator * byteCodeGenerator, FuncInfo * funcInfo)
{
    VisitIndirect<FuncInfo>(pnode, byteCodeGenerator, funcInfo, &ClearTmpRegs, nullptr);
}

Js::FunctionBody * ByteCodeGenerator::MakeGlobalFunctionBody(ParseNode *pnode)
{
    Js::FunctionBody * func;

    func =
        Js::FunctionBody::NewFromRecycler(
            scriptContext,
            Js::Constants::GlobalFunction,
            Js::Constants::GlobalFunctionLength,
            0,
            pnode->AsParseNodeFnc()->nestedCount,
            m_utf8SourceInfo,
            m_utf8SourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
            pnode->AsParseNodeFnc()->functionId,
            Js::FunctionInfo::Attributes::None,
            Js::FunctionBody::FunctionBodyFlags::Flags_HasNoExplicitReturnValue
#ifdef PERF_COUNTERS
            , false /* is function from deferred deserialized proxy */
#endif
            );

    func->SetIsGlobalFunc(true);

    scriptContext->GetLibrary()->RegisterDynamicFunctionReference(func);

    return func;
}

bool ByteCodeGenerator::NeedScopeObjectForArguments(FuncInfo *funcInfo, ParseNodeFnc *pnodeFnc) const
{
    // We can avoid creating a scope object with arguments present if:
    bool dontNeedScopeObject =
        // We have arguments, and
        funcInfo->GetHasHeapArguments()
        // Either we are in strict mode, or have strict mode formal semantics from a non-simple parameter list, and
        && (funcInfo->GetIsStrictMode()
            || pnodeFnc->HasNonSimpleParameterList())
        // We're not in eval or event handler, which will force the scope(s) to be objects
        && !(this->flags & (fscrEval | fscrImplicitThis))
        // Neither of the scopes are objects
        && !funcInfo->paramScope->GetIsObject()
        && !funcInfo->bodyScope->GetIsObject();

    return funcInfo->GetHasHeapArguments()
        // Regardless of the conditions above, we won't need a scope object if there aren't any formals.
        && (pnodeFnc->pnodeParams != nullptr || pnodeFnc->pnodeRest != nullptr)
        && !dontNeedScopeObject;
}

Js::FunctionBody *ByteCodeGenerator::EnsureFakeGlobalFuncForUndefer(ParseNode *pnode)
{
    Js::FunctionBody *func = scriptContext->GetLibrary()->GetFakeGlobalFuncForUndefer();
    if (!func)
    {
        func = this->MakeGlobalFunctionBody(pnode);
        scriptContext->GetLibrary()->SetFakeGlobalFuncForUndefer(func);
    }
    if (pnode->AsParseNodeFnc()->GetStrictMode() != 0)
    {
        func->SetIsStrictMode();
    }

    return func;
}
