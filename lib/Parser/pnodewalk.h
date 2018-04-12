//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

template <class ResultType, class Context>
struct WalkerPolicyBase
{
    typedef ResultType ResultType;
    typedef Context Context;

    inline bool ContinueWalk(ResultType) { return true; }
    inline ResultType DefaultResult() { return ResultType(); }
    inline ResultType WalkNode(ParseNode *pnode, Context context) { return DefaultResult(); }
    inline ResultType WalkListNode(ParseNode *pnode, Context context) { return DefaultResult(); }
    virtual ResultType WalkChild(ParseNode *pnode, Context context) { return DefaultResult(); }
    inline ResultType WalkFirstChild(ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline ResultType WalkSecondChild(ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline ResultType WalkNthChild(ParseNode *pparentnode, ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline void WalkReference(ParseNode **ppnode, Context context) { }
};

template <class Context>
struct WalkerPolicyBase<bool, Context>
{
    typedef bool ResultType;
    typedef Context Context;

    inline bool ContinueWalk(ResultType) { return true; }
    inline bool DefaultResult() { return true; }
    inline ResultType WalkNode(ParseNode *pnode, Context context) { return DefaultResult(); }
    inline ResultType WalkListNode(ParseNode *pnode, Context context) { return DefaultResult(); }
    virtual ResultType WalkChild(ParseNode *pnode, Context context) { return DefaultResult(); }
    inline ResultType WalkFirstChild(ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline ResultType WalkSecondChild(ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline ResultType WalkNthChild(ParseNode *pparentnode, ParseNode *pnode, Context context) { return WalkChild(pnode, context); }
    inline void WalkReference(ParseNode **ppnode, Context context) { }
};

template <typename WalkerPolicy>
class ParseNodeWalker : public WalkerPolicy
{
    using WalkerPolicy::ContinueWalk;
    using WalkerPolicy::DefaultResult;
    using WalkerPolicy::WalkNode;
    using WalkerPolicy::WalkListNode;
    using WalkerPolicy::WalkFirstChild;
    using WalkerPolicy::WalkSecondChild;
    using WalkerPolicy::WalkNthChild;
    using WalkerPolicy::WalkReference;
public:
    typedef typename WalkerPolicy::Context Context;

protected:
    typedef typename WalkerPolicy::ResultType ResultType;

private:
    ResultType WalkList(ParseNode *pnodeparent, ParseNode *&pnode, Context context)
    {
        ResultType result = DefaultResult();
        bool first = true;
        if (pnode)
        {
            result = WalkListNode(pnode, context);
            if (!ContinueWalk(result)) return result;

            ParseNodePtr current = pnode;
            ParseNodePtr *ppnode = &pnode;
            // Skip list nodes and nested VarDeclList nodes
            while ((current->nop == knopList && (current->grfpn & PNodeFlags::fpnDclList) == 0) ||
                   (current->nop == pnode->nop && (current->grfpn & pnode->grfpn & PNodeFlags::fpnDclList)))
            {
                WalkReference(&current->AsParseNodeBin()->pnode1, context);
                result = first ? WalkFirstChild(current->AsParseNodeBin()->pnode1, context) : WalkNthChild(pnodeparent, current->AsParseNodeBin()->pnode1, context);
                first = false;
                if (!ContinueWalk(result)) return result;
                ppnode = &current->AsParseNodeBin()->pnode2;
                current = *ppnode;
            }
            WalkReference(ppnode, context);
            result = first ? WalkFirstChild(*ppnode, context) : WalkNthChild(pnodeparent, *ppnode, context);
        }
        // Reset the reference back.
        WalkReference(nullptr, context);
        return result;
    }

    ResultType WalkLeaf(ParseNode *pnode, Context context)
    {
        return WalkNode(pnode, context);
    }

    ResultType WalkPreUnary(ParseNodeUni *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->pnode1) result = WalkFirstChild(pnode->pnode1, context);
        return result;
    }

    ResultType WalkPostUnary(ParseNodeUni *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnode1, context);
        if (ContinueWalk(result)) result = WalkNode(pnode, context);
        return result;
    }

    ResultType WalkBinary(ParseNodeBin *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnode1, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnode2, context);
        }
        return result;
    }

    ResultType WalkTernary(ParseNodeTri *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnode1, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->pnode2, context);
                if (ContinueWalk(result)) result = WalkNthChild(pnode, pnode->pnode3, context);
            }
        }
        return result;
    }

    ResultType WalkCall(ParseNodeCall *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeTarget, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkList(pnode, pnode->pnodeArgs, context);
        }
        return result;
    }

    ResultType WalkStringTemplate(ParseNodeStrTemplate *pnode, Context context)
    {
        ResultType result;

        if (!pnode->isTaggedTemplate)
        {
            if (pnode->pnodeSubstitutionExpressions == nullptr)
            {
                // If we don't have any substitution expressions, then we should only have one string literal and not a list
                result = WalkNode(pnode->pnodeStringLiterals, context);
            }
            else
            {
                result = WalkList(pnode, pnode->pnodeSubstitutionExpressions, context);
                if (ContinueWalk(result))
                {
                    result = WalkList(pnode, pnode->pnodeStringLiterals, context);
                }
            }
        }
        else
        {
            result = WalkList(pnode, pnode->pnodeStringLiterals, context);
        }

        return result;
    }

    ResultType WalkVar(ParseNodeVar *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->pnodeInit) result = WalkFirstChild(pnode->pnodeInit, context);
        return result;
    }

    ResultType WalkFnc(ParseNodeFnc *pnode, Context context)
    {
        ResultType result;
        // For ordering, arguments are considered prior to the function and the body after.
        ParseNodePtr argNode = pnode->pnodeParams;
        while (argNode)
        {
            result = argNode == pnode->pnodeParams ? WalkFirstChild(argNode, context) : WalkNthChild(pnode, argNode, context);
            if (!ContinueWalk(result)) return result;
            if (argNode->nop == knopParamPattern)
            {
                argNode = argNode->AsParseNodeParamPattern()->pnodeNext;
            }
            else
            {
                argNode = argNode->AsParseNodeVar()->pnodeNext;
            }
        }

        if (pnode->pnodeRest != nullptr)
        {
            result = WalkSecondChild(pnode->pnodeRest, context);
            if (!ContinueWalk(result))  return result;
        }

        result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkNthChild(pnode, pnode->pnodeBody, context);
        return result;
    }

    ResultType WalkProg(ParseNodeProg *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkList(pnode, pnode->pnodeBody, context);
        return result;
    }

    ResultType WalkFor(ParseNodeFor *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeInit, context);
        if (ContinueWalk(result))
        {
            result = WalkNthChild(pnode, pnode->pnodeCond, context);
            if (ContinueWalk(result))
            {
                result = WalkNthChild(pnode, pnode->pnodeIncr, context);
                if (ContinueWalk(result))
                {
                    result = WalkNode(pnode, context);
                    if (ContinueWalk(result))
                    {
                        result = WalkSecondChild(pnode->pnodeBody, context);
                    }
                }
            }
        }
        return result;
    }

    ResultType WalkIf(ParseNodeIf *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeCond, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->pnodeTrue, context);
                if (ContinueWalk(result) && pnode->pnodeFalse)
                    result = WalkNthChild(pnode, pnode->pnodeFalse, context);
            }
        }
        return result;
    }

    ResultType WalkWhile(ParseNodeWhile *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeCond, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkDoWhile(ParseNodeWhile *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeBody, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->pnodeCond, context);
            }
        }
        return result;
    }

    ResultType WalkForInOrForOf(ParseNodeForInOrForOf *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeLval, context);
        if (ContinueWalk(result))
        {
            result = WalkNthChild(pnode, pnode->pnodeObj, context);
            if (ContinueWalk(result))
            {
                result = WalkNode(pnode, context);
                if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeBody, context);
            }
        }
        return result;
    }

    ResultType WalkReturn(ParseNodeReturn *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->pnodeExpr) result = WalkFirstChild(pnode->pnodeExpr, context);
        return result;
    }

    ResultType WalkBlock(ParseNodeBlock *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->pnodeStmt)
            result = WalkList(pnode, pnode->pnodeStmt, context);
        return result;
    }

    ResultType WalkWith(ParseNodeWith *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeObj, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->pnodeBody, context);
            }
        }
        return result;
    }

    ResultType WalkSwitch(ParseNodeSwitch *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeVal, context);
        if (ContinueWalk(result))
        {
            for (ParseNodeCase** caseNode = &(pnode->pnodeCases); *caseNode != nullptr; caseNode = &((*caseNode)->pnodeNext))
            {
                result = *caseNode == pnode->pnodeCases ? WalkFirstChild(*caseNode, context) : WalkNthChild(pnode, *caseNode, context);
                if (!ContinueWalk(result)) return result;
            }
            result = WalkNode(pnode, context);
        }
        return result;
    }

    ResultType WalkCase(ParseNodeCase *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeExpr, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkTryFinally(ParseNodeTryFinally *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeTry, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeFinally, context);
        }
        return result;
    }

    ResultType WalkFinally(ParseNodeFinally *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkFirstChild(pnode->pnodeBody, context);
        return result;
    }

    ResultType WalkCatch(ParseNodeCatch *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->GetParam(), context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkTryCatch(ParseNodeTryCatch *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->pnodeTry, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->pnodeCatch, context);
        }
        return result;
    }

    ResultType WalkTry(ParseNodeTry *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkFirstChild(pnode->pnodeBody, context);
        return result;
    }

    ResultType WalkClass(ParseNodeClass *pnode, Context context)
    {
        // First walk the class node itself
        ResultType result = WalkNode(pnode, context);
        if (!ContinueWalk(result)) return result;
        // Walk extends expr
        result = WalkFirstChild(pnode->pnodeExtends, context);
        if (!ContinueWalk(result)) return result;
        // Walk the constructor
        result = WalkNthChild(pnode, pnode->pnodeConstructor, context);
        if (!ContinueWalk(result)) return result;
        // Walk all non-static members
        result = WalkList(pnode, pnode->pnodeMembers, context);
        if (!ContinueWalk(result)) return result;
        // Walk all static members
        result = WalkList(pnode, pnode->pnodeStaticMembers, context);
        return result;
    }

 public:
    ResultType Walk(ParseNode *pnode, Context context)
    {
        if (!pnode) return DefaultResult();

        switch (pnode->nop) {
        // Handle all special cases first.

        // Post-fix unary operators.
        //PTNODE(knopIncPost    , "++ post"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
        //PTNODE(knopDecPost    , "-- post"    ,Dec     ,Uni  ,fnopUni|fnopAsg)
        case knopIncPost:
        case knopDecPost:
            return WalkPostUnary(pnode->AsParseNodeUni(), context);

        // Call and call like
        //PTNODE(knopCall       , "()"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopNew        , "new"        ,None    ,Bin  ,fnopBin)
        case knopCall:
        case knopNew:
            return WalkCall(pnode->AsParseNodeCall(), context);

        // Ternary operator
        //PTNODE(knopQmark      , "?"            ,None    ,Tri  ,fnopBin)
        case knopQmark:
            return WalkTernary(pnode->AsParseNodeTri(), context);

        // General nodes.
        //PTNODE(knopList       , "<list>"    ,None    ,Bin  ,fnopNone)
        case knopList:
            return WalkList(NULL, pnode, context);

        //PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
        case knopVarDecl:
        case knopConstDecl:
        case knopLetDecl:
        case knopTemp:
            return WalkVar(pnode->AsParseNodeVar(), context);

        //PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
        case knopFncDecl:
            return WalkFnc(pnode->AsParseNodeFnc(), context);

        //PTNODE(knopProg       , "program"    ,None    ,Fnc  ,fnopNone)
        case knopProg:
            return WalkProg(pnode->AsParseNodeProg(), context);

        //PTNODE(knopFor        , "for"        ,None    ,For  ,fnopBreak|fnopContinue)
        case knopFor:
            return WalkFor(pnode->AsParseNodeFor(), context);

        //PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
        case knopIf:
            return WalkIf(pnode->AsParseNodeIf(), context);

        //PTNODE(knopWhile      , "while"        ,None    ,While,fnopBreak|fnopContinue)
        case knopWhile:
            return WalkWhile(pnode->AsParseNodeWhile(), context);

         //PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
        case knopDoWhile:
            return WalkDoWhile(pnode->AsParseNodeWhile(), context);

        //PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
        case knopForIn:
            return WalkForInOrForOf(pnode->AsParseNodeForInOrForOf(), context);

        case knopForOf:
            return WalkForInOrForOf(pnode->AsParseNodeForInOrForOf(), context);

        //PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
        case knopReturn:
            return WalkReturn(pnode->AsParseNodeReturn(), context);

        //PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
        case knopBlock:
            return WalkBlock(pnode->AsParseNodeBlock(), context);

        //PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
        case knopWith:
            return WalkWith(pnode->AsParseNodeWith(), context);

        //PTNODE(knopSwitch     , "switch"    ,None    ,Switch,fnopBreak)
        case knopSwitch:
            return WalkSwitch(pnode->AsParseNodeSwitch(), context);

        //PTNODE(knopCase       , "case"        ,None    ,Case ,fnopNone)
        case knopCase:
            return WalkCase(pnode->AsParseNodeCase(), context);

        //PTNODE(knopTryFinally,"try-finally",None,TryFinally,fnopCleanup)
        case knopTryFinally:
            return WalkTryFinally(pnode->AsParseNodeTryFinally(), context);

       case knopFinally:
           return WalkFinally(pnode->AsParseNodeFinally(), context);

        //PTNODE(knopCatch      , "catch"     ,None    ,Catch,fnopNone)
        case knopCatch:
            return WalkCatch(pnode->AsParseNodeCatch(), context);

        //PTNODE(knopTryCatch      , "try-catch" ,None    ,TryCatch  ,fnopCleanup)
        case knopTryCatch:
            return WalkTryCatch(pnode->AsParseNodeTryCatch(), context);

        //PTNODE(knopTry        , "try"       ,None    ,Try  ,fnopCleanup)
        case knopTry:
            return WalkTry(pnode->AsParseNodeTry(), context);

        //PTNODE(knopThrow      , "throw"     ,None    ,Uni  ,fnopNone)
        case knopThrow:
            return WalkPostUnary(pnode->AsParseNodeUni(), context);

        case knopStrTemplate:
            return WalkStringTemplate(pnode->AsParseNodeStrTemplate(), context);

        //PTNODE(knopClassDecl  , "classDecl" ,None    ,Class       ,fnopLeaf)
        case knopClassDecl:
            return WalkClass(pnode->AsParseNodeClass(), context);

        case knopExportDefault:
            return Walk(pnode->AsParseNodeExportDefault()->pnodeExpr, context);

        default:
        {
            uint fnop = ParseNode::Grfnop(pnode->nop);

            if (fnop & fnopLeaf || fnop & fnopNone)
            {
                return WalkLeaf(pnode, context);
            }
            else if (fnop & fnopBin)
            {
                return WalkBinary(pnode->AsParseNodeBin(), context);
            }
            else if (fnop & fnopUni)
            {
                // Prefix unary operators.
                return WalkPreUnary(pnode->AsParseNodeUni(), context);
            }

            // Some node types are both fnopNotExprStmt and something else. Try the above cases first and fall back to this one.
            if (fnop & fnopNotExprStmt)
            {
                return WalkLeaf(pnode, context);
            }

            Assert(false);
            __assume(false);
        }
        }
    }
};
