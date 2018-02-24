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

    ResultType WalkPreUnary(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->AsParseNodeUni()->pnode1) result = WalkFirstChild(pnode->AsParseNodeUni()->pnode1, context);
        return result;
    }

    ResultType WalkPostUnary(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeUni()->pnode1, context);
        if (ContinueWalk(result)) result = WalkNode(pnode, context);
        return result;
    }

    ResultType WalkBinary(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeBin()->pnode1, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeBin()->pnode2, context);
        }
        return result;
    }

    ResultType WalkTernary(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeTri()->pnode1, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->AsParseNodeTri()->pnode2, context);
                if (ContinueWalk(result)) result = WalkNthChild(pnode, pnode->AsParseNodeTri()->pnode3, context);
            }
        }
        return result;
    }

    ResultType WalkCall(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeCall()->pnodeTarget, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkList(pnode, pnode->AsParseNodeCall()->pnodeArgs, context);
        }
        return result;
    }

    ResultType WalkStringTemplate(ParseNode *pnode, Context context)
    {
        ResultType result;

        if (!pnode->AsParseNodeStrTemplate()->isTaggedTemplate)
        {
            if (pnode->AsParseNodeStrTemplate()->pnodeSubstitutionExpressions == nullptr)
            {
                // If we don't have any substitution expressions, then we should only have one string literal and not a list
                result = WalkNode(pnode->AsParseNodeStrTemplate()->pnodeStringLiterals, context);
            }
            else
            {
                result = WalkList(pnode, pnode->AsParseNodeStrTemplate()->pnodeSubstitutionExpressions, context);
                if (ContinueWalk(result))
                {
                    result = WalkList(pnode, pnode->AsParseNodeStrTemplate()->pnodeStringLiterals, context);
                }
            }
        }
        else
        {
            // Tagged template nodes are call nodes
            result = WalkCall(pnode, context);
        }

        return result;
    }

    ResultType WalkVar(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->AsParseNodeVar()->pnodeInit) result = WalkFirstChild(pnode->AsParseNodeVar()->pnodeInit, context);
        return result;
    }

    ResultType WalkFnc(ParseNode *pnode, Context context)
    {
        ResultType result;
        // For ordering, arguments are considered prior to the function and the body after.
        for (ParseNode** argNode = &(pnode->AsParseNodeFnc()->pnodeParams); *argNode != nullptr; argNode = &((*argNode)->AsParseNodeVar()->pnodeNext))
        {
            result = *argNode == pnode->AsParseNodeFnc()->pnodeParams ? WalkFirstChild(*argNode, context) : WalkNthChild(pnode, *argNode, context);
            if (!ContinueWalk(result)) return result;
        }

        if (pnode->AsParseNodeFnc()->pnodeRest != nullptr)
        {
            result = WalkSecondChild(pnode->AsParseNodeFnc()->pnodeRest, context);
            if (!ContinueWalk(result))  return result;
        }

        result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkNthChild(pnode, pnode->AsParseNodeFnc()->pnodeBody, context);
        return result;
    }

    ResultType WalkProg(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkList(pnode, pnode->AsParseNodeFnc()->pnodeBody, context);
        return result;
    }

    ResultType WalkFor(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeFor()->pnodeInit, context);
        if (ContinueWalk(result))
        {
            result = WalkNthChild(pnode, pnode->AsParseNodeFor()->pnodeCond, context);
            if (ContinueWalk(result))
            {
                result = WalkNthChild(pnode, pnode->AsParseNodeFor()->pnodeIncr, context);
                if (ContinueWalk(result))
                {
                    result = WalkNode(pnode, context);
                    if (ContinueWalk(result))
                    {
                        result = WalkSecondChild(pnode->AsParseNodeFor()->pnodeBody, context);
                    }
                }
            }
        }
        return result;
    }

    ResultType WalkIf(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeIf()->pnodeCond, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->AsParseNodeIf()->pnodeTrue, context);
                if (ContinueWalk(result) && pnode->AsParseNodeIf()->pnodeFalse)
                    result = WalkNthChild(pnode, pnode->AsParseNodeIf()->pnodeFalse, context);
            }
        }
        return result;
    }

    ResultType WalkWhile(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeWhile()->pnodeCond, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeWhile()->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkDoWhile(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeWhile()->pnodeBody, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->AsParseNodeWhile()->pnodeCond, context);
            }
        }
        return result;
    }

    ResultType WalkForInOrForOf(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeForInOrForOf()->pnodeLval, context);
        if (ContinueWalk(result))
        {
            result = WalkNthChild(pnode, pnode->AsParseNodeForInOrForOf()->pnodeObj, context);
            if (ContinueWalk(result))
            {
                result = WalkNode(pnode, context);
                if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeForInOrForOf()->pnodeBody, context);
            }
        }
        return result;
    }

    ResultType WalkReturn(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->AsParseNodeReturn()->pnodeExpr) result = WalkFirstChild(pnode->AsParseNodeReturn()->pnodeExpr, context);
        return result;
    }

    ResultType WalkBlock(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result) && pnode->AsParseNodeBlock()->pnodeStmt)
            result = WalkList(pnode, pnode->AsParseNodeBlock()->pnodeStmt, context);
        return result;
    }

    ResultType WalkWith(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeWith()->pnodeObj, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result))
            {
                result = WalkSecondChild(pnode->AsParseNodeWith()->pnodeBody, context);
            }
        }
        return result;
    }

    ResultType WalkSwitch(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeSwitch()->pnodeVal, context);
        if (ContinueWalk(result))
        {
            for (ParseNode** caseNode = &(pnode->AsParseNodeSwitch()->pnodeCases); *caseNode != nullptr; caseNode = &((*caseNode)->AsParseNodeCase()->pnodeNext))
            {
                result = *caseNode == pnode->AsParseNodeSwitch()->pnodeCases ? WalkFirstChild(*caseNode, context) : WalkNthChild(pnode, *caseNode, context);
                if (!ContinueWalk(result)) return result;
            }
            result = WalkNode(pnode, context);
        }
        return result;
    }

    ResultType WalkCase(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeCase()->pnodeExpr, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeCase()->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkTryFinally(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeTryFinally()->pnodeTry, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeTryFinally()->pnodeFinally, context);
        }
        return result;
    }

    ResultType WalkFinally(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkFirstChild(pnode->AsParseNodeFinally()->pnodeBody, context);
        return result;
    }

    ResultType WalkCatch(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeCatch()->pnodeParam, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeCatch()->pnodeBody, context);
        }
        return result;
    }

    ResultType WalkTryCatch(ParseNode *pnode, Context context)
    {
        ResultType result = WalkFirstChild(pnode->AsParseNodeTryCatch()->pnodeTry, context);
        if (ContinueWalk(result))
        {
            result = WalkNode(pnode, context);
            if (ContinueWalk(result)) result = WalkSecondChild(pnode->AsParseNodeTryCatch()->pnodeCatch, context);
        }
        return result;
    }

    ResultType WalkTry(ParseNode *pnode, Context context)
    {
        ResultType result = WalkNode(pnode, context);
        if (ContinueWalk(result)) result = WalkFirstChild(pnode->AsParseNodeTry()->pnodeBody, context);
        return result;
    }

    ResultType WalkClass(ParseNode *pnode, Context context)
    {
        // First walk the class node itself
        ResultType result = WalkNode(pnode, context);
        if (!ContinueWalk(result)) return result;
        // Walk extends expr
        result = WalkFirstChild(pnode->AsParseNodeClass()->pnodeExtends, context);
        if (!ContinueWalk(result)) return result;
        // Walk the constructor
        result = WalkNthChild(pnode, pnode->AsParseNodeClass()->pnodeConstructor, context);
        if (!ContinueWalk(result)) return result;
        // Walk all non-static members
        result = WalkList(pnode, pnode->AsParseNodeClass()->pnodeMembers, context);
        if (!ContinueWalk(result)) return result;
        // Walk all static members
        result = WalkList(pnode, pnode->AsParseNodeClass()->pnodeStaticMembers, context);
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
            return WalkPostUnary(pnode, context);

        // Call and call like
        //PTNODE(knopCall       , "()"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopNew        , "new"        ,None    ,Bin  ,fnopBin)
        case knopCall:
        case knopNew:
            return WalkCall(pnode, context);

        // Ternary operator
        //PTNODE(knopQmark      , "?"            ,None    ,Tri  ,fnopBin)
        case knopQmark:
            return WalkTernary(pnode, context);

        // General nodes.
        //PTNODE(knopList       , "<list>"    ,None    ,Bin  ,fnopNone)
        case knopList:
            return WalkList(NULL, pnode, context);

        //PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
        case knopVarDecl:
        case knopConstDecl:
        case knopLetDecl:
        case knopTemp:
            return WalkVar(pnode, context);

        //PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
        case knopFncDecl:
            return WalkFnc(pnode, context);

        //PTNODE(knopProg       , "program"    ,None    ,Fnc  ,fnopNone)
        case knopProg:
            return WalkProg(pnode, context);

        //PTNODE(knopFor        , "for"        ,None    ,For  ,fnopBreak|fnopContinue)
        case knopFor:
            return WalkFor(pnode, context);

        //PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
        case knopIf:
            return WalkIf(pnode, context);

        //PTNODE(knopWhile      , "while"        ,None    ,While,fnopBreak|fnopContinue)
        case knopWhile:
            return WalkWhile(pnode, context);

         //PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
        case knopDoWhile:
            return WalkDoWhile(pnode, context);

        //PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
        case knopForIn:
            return WalkForInOrForOf(pnode, context);

        case knopForOf:
            return WalkForInOrForOf(pnode, context);

        //PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
        case knopReturn:
            return WalkReturn(pnode, context);

        //PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
        case knopBlock:
            return WalkBlock(pnode, context);

        //PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
        case knopWith:
            return WalkWith(pnode, context);

        //PTNODE(knopSwitch     , "switch"    ,None    ,Switch,fnopBreak)
        case knopSwitch:
            return WalkSwitch(pnode, context);

        //PTNODE(knopCase       , "case"        ,None    ,Case ,fnopNone)
        case knopCase:
            return WalkCase(pnode, context);

        //PTNODE(knopTryFinally,"try-finally",None,TryFinally,fnopCleanup)
        case knopTryFinally:
            return WalkTryFinally(pnode, context);

       case knopFinally:
           return WalkFinally(pnode, context);

        //PTNODE(knopCatch      , "catch"     ,None    ,Catch,fnopNone)
        case knopCatch:
            return WalkCatch(pnode, context);

        //PTNODE(knopTryCatch      , "try-catch" ,None    ,TryCatch  ,fnopCleanup)
        case knopTryCatch:
            return WalkTryCatch(pnode, context);

        //PTNODE(knopTry        , "try"       ,None    ,Try  ,fnopCleanup)
        case knopTry:
            return WalkTry(pnode, context);

        //PTNODE(knopThrow      , "throw"     ,None    ,Uni  ,fnopNone)
        case knopThrow:
            return WalkPostUnary(pnode, context);

        case knopStrTemplate:
            return WalkStringTemplate(pnode, context);

        //PTNODE(knopClassDecl  , "classDecl" ,None    ,Class       ,fnopLeaf)
        case knopClassDecl:
            return WalkClass(pnode, context);

        case knopExportDefault:
            return Walk(pnode->AsParseNodeExportDefault()->pnodeExpr, context);

        default:
        {
            uint fnop = ParseNode::Grfnop(pnode->nop);

            if (fnop & fnopLeaf || fnop && fnopNone)
            {
                return WalkLeaf(pnode, context);
            }
            else if (fnop & fnopBin)
            {
                return WalkBinary(pnode, context);
            }
            else if (fnop & fnopUni)
            {
                // Prefix unary operators.
                return WalkPreUnary(pnode, context);
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
