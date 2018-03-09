//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"

void ParseNode::Init(OpCode nop, charcount_t ichMin, charcount_t ichLim)
{
    this->nop = nop;
    this->grfpn = PNodeFlags::fpnNone;
    this->location = Js::Constants::NoRegister;
    this->emitLabels = false;
    this->isUsed = true;
    this->notEscapedUse = false;
    this->isInList = false;
    this->isCallApplyTargetLoad = false;
    this->isSpecialName = false;
    this->ichMin = ichMin;
    this->ichLim = ichLim;
}

ParseNodeUni * ParseNode::AsParseNodeUni()
{
    Assert(((this->Grfnop() & fnopUni) && this->nop != knopParamPattern) || this->nop == knopThrow);
    return reinterpret_cast<ParseNodeUni *>(this);
}

ParseNodeBin * ParseNode::AsParseNodeBin()
{
    Assert(((this->Grfnop() & fnopBin) && this->nop != knopQmark && this->nop != knopCall && this->nop != knopNew) || this->nop == knopList);
    return reinterpret_cast<ParseNodeBin *>(this);
}

ParseNodeTri * ParseNode::AsParseNodeTri()
{
    Assert(this->nop == knopQmark);
    return reinterpret_cast<ParseNodeTri *>(this);
}

ParseNodeInt * ParseNode::AsParseNodeInt()
{
    Assert(this->nop == knopInt);
    return reinterpret_cast<ParseNodeInt *>(this);
}

ParseNodeFloat * ParseNode::AsParseNodeFloat()
{
    Assert(this->nop == knopFlt);
    return reinterpret_cast<ParseNodeFloat *>(this);
}

ParseNodeVar * ParseNode::AsParseNodeVar()
{
    Assert(this->nop == knopVarDecl || this->nop == knopConstDecl || this->nop == knopLetDecl || this->nop == knopTemp);
    return reinterpret_cast<ParseNodeVar *>(this);
}

ParseNodePid * ParseNode::AsParseNodePid()
{
    Assert(this->nop == knopName || this->nop == knopStr || this->nop == knopRegExp || this->nop == knopSpecialName);
    return reinterpret_cast<ParseNodePid *>(this);
}

ParseNodeSpecialName * ParseNode::AsParseNodeSpecialName()
{
    Assert(this->nop == knopName && this->isSpecialName);
    return reinterpret_cast<ParseNodeSpecialName *>(this);
}

ParseNodeExportDefault * ParseNode::AsParseNodeExportDefault()
{
    Assert(this->nop == knopExportDefault);
    return reinterpret_cast<ParseNodeExportDefault *>(this);
}

ParseNodeStrTemplate * ParseNode::AsParseNodeStrTemplate()
{
    Assert(this->nop == knopStrTemplate);
    return reinterpret_cast<ParseNodeStrTemplate *>(this);
}

ParseNodeSuperReference * ParseNode::AsParseNodeSuperReference()
{
    Assert(this->nop == knopDot || this->nop == knopIndex);
    Assert(this->AsParseNodeBin()->pnode1 && this->AsParseNodeBin()->pnode1->isSpecialName && this->AsParseNodeBin()->pnode1->AsParseNodeSpecialName()->isSuper);
    return reinterpret_cast<ParseNodeSuperReference*>(this);
}

ParseNodeArrLit * ParseNode::AsParseNodeArrLit()
{
    Assert(this->nop == knopArray || this->nop == knopArrayPattern);
    return reinterpret_cast<ParseNodeArrLit*>(this);
}

ParseNodeCall * ParseNode::AsParseNodeCall()
{
    Assert(this->nop == knopCall || this->nop == knopNew);
    return reinterpret_cast<ParseNodeCall*>(this);
}

ParseNodeSuperCall * ParseNode::AsParseNodeSuperCall()
{
    Assert(this->nop == knopCall && this->AsParseNodeCall()->isSuperCall);
    return reinterpret_cast<ParseNodeSuperCall*>(this);
}

ParseNodeClass * ParseNode::AsParseNodeClass()
{
    Assert(this->nop == knopClassDecl);
    return reinterpret_cast<ParseNodeClass*>(this);
}

ParseNodeParamPattern * ParseNode::AsParseNodeParamPattern()
{
    Assert(this->nop == knopParamPattern);
    return reinterpret_cast<ParseNodeParamPattern*>(this);
}

ParseNodeStmt * ParseNode::AsParseNodeStmt()
{
    Assert(this->nop == knopBlock || this->nop == knopBreak || this->nop == knopContinue || this->nop == knopWith || this->nop == knopIf || this->nop == knopSwitch || this->nop == knopCase || this->nop == knopReturn
        || this->nop == knopTryFinally || this->nop == knopTryCatch || this->nop == knopTry || this->nop == knopCatch || this->nop == knopFinally
        || this->nop == knopWhile || this->nop == knopDoWhile || this->nop == knopFor || this->nop == knopForIn || this->nop == knopForOf);
    return reinterpret_cast<ParseNodeStmt*>(this);
}

ParseNodeBlock * ParseNode::AsParseNodeBlock()
{
    Assert(this->nop == knopBlock);
    return reinterpret_cast<ParseNodeBlock*>(this);
}

ParseNodeJump * ParseNode::AsParseNodeJump()
{
    Assert(this->nop == knopBreak || this->nop == knopContinue);
    return reinterpret_cast<ParseNodeJump*>(this);
}

ParseNodeWith * ParseNode::AsParseNodeWith()
{
    Assert(this->nop == knopWith);
    return reinterpret_cast<ParseNodeWith*>(this);
}

ParseNodeIf * ParseNode::AsParseNodeIf()
{
    Assert(this->nop == knopIf);
    return reinterpret_cast<ParseNodeIf*>(this);
}

ParseNodeSwitch * ParseNode::AsParseNodeSwitch()
{
    Assert(this->nop == knopSwitch);
    return reinterpret_cast<ParseNodeSwitch*>(this);
}

ParseNodeCase * ParseNode::AsParseNodeCase()
{
    Assert(this->nop == knopCase);
    return reinterpret_cast<ParseNodeCase*>(this);
}

ParseNodeReturn * ParseNode::AsParseNodeReturn()
{
    Assert(this->nop == knopReturn);
    return reinterpret_cast<ParseNodeReturn*>(this);
}

ParseNodeTryFinally * ParseNode::AsParseNodeTryFinally()
{
    Assert(this->nop == knopTryFinally);
    return reinterpret_cast<ParseNodeTryFinally*>(this);
}

ParseNodeTryCatch * ParseNode::AsParseNodeTryCatch()
{
    Assert(this->nop == knopTryCatch);
    return reinterpret_cast<ParseNodeTryCatch*>(this);
}

ParseNodeTry * ParseNode::AsParseNodeTry()
{
    Assert(this->nop == knopTry);
    return reinterpret_cast<ParseNodeTry*>(this);
}

ParseNodeCatch * ParseNode::AsParseNodeCatch()
{
    Assert(this->nop == knopCatch);
    return reinterpret_cast<ParseNodeCatch*>(this);
}

ParseNodeFinally * ParseNode::AsParseNodeFinally()
{
    Assert(this->nop == knopFinally);
    return reinterpret_cast<ParseNodeFinally*>(this);
}

ParseNodeLoop * ParseNode::AsParseNodeLoop()
{
    Assert(this->nop == knopWhile || this->nop == knopDoWhile || this->nop == knopFor || this->nop == knopForIn || this->nop == knopForOf);
    return reinterpret_cast<ParseNodeLoop*>(this);
}

ParseNodeWhile * ParseNode::AsParseNodeWhile()
{
    Assert(this->nop == knopWhile || this->nop == knopDoWhile);
    return reinterpret_cast<ParseNodeWhile*>(this);
}

ParseNodeFor * ParseNode::AsParseNodeFor()
{
    Assert(this->nop == knopFor);
    return reinterpret_cast<ParseNodeFor*>(this);
}

ParseNodeForInOrForOf * ParseNode::AsParseNodeForInOrForOf()
{
    Assert(this->nop == knopForIn || this->nop == knopForOf);
    return reinterpret_cast<ParseNodeForInOrForOf*>(this);
}

ParseNodeFnc * ParseNode::AsParseNodeFnc()
{
    Assert(this->nop == knopFncDecl || this->nop == knopProg || this->nop == knopModule);
    return reinterpret_cast<ParseNodeFnc*>(this);
}

ParseNodeProg * ParseNode::AsParseNodeProg()
{
    Assert(this->nop == knopProg);
    return reinterpret_cast<ParseNodeProg*>(this);
}

ParseNodeModule * ParseNode::AsParseNodeModule()
{
    // TODO: Currently there is not way to distingish a ParseNodeModule to ParseNodeProg node
    Assert(this->nop == knopProg);
    return reinterpret_cast<ParseNodeModule*>(this);
}

IdentPtr ParseNode::name()
{
    if (this->nop == knopName || this->nop == knopStr)
    {
        return this->AsParseNodePid()->pid;
    }
    else if (this->nop == knopVarDecl || this->nop == knopConstDecl)
    {
        return this->AsParseNodeVar()->pid;
    }
    return nullptr;
}

ParseNodePtr ParseNode::GetFormalNext()
{
    ParseNodePtr pnodeNext = nullptr;

    if (nop == knopParamPattern)
    {
        pnodeNext = this->AsParseNodeParamPattern()->pnodeNext;
    }
    else
    {
        Assert(IsVarLetOrConst());
        pnodeNext = this->AsParseNodeVar()->pnodeNext;
    }
    return pnodeNext;
}

void ParseNodeCall::Init(OpCode nop, ParseNodePtr pnodeTarget, ParseNodePtr pnodeArgs, charcount_t ichMin, charcount_t ichLim)
{
    __super::Init(nop, ichMin, ichLim);

    this->pnodeTarget = pnodeTarget;
    this->pnodeArgs = pnodeArgs;
    this->argCount = 0;
    this->spreadArgCount = 0;
    this->callOfConstants = false;
    this->isApplyCall = false;
    this->isEvalCall = false;
    this->isSuperCall = false;
    this->hasDestructuring = false;
}

void ParseNodeSuperCall::Init(OpCode nop, ParseNodePtr pnodeTarget, ParseNodePtr pnodeArgs, charcount_t ichMin, charcount_t ichLim)
{
    __super::Init(nop, pnodeTarget, pnodeArgs, ichMin, ichLim);

    this->isSuperCall = true;
    this->pnodeThis = nullptr;
    this->pnodeNewTarget = nullptr;
}

void ParseNodeBlock::Init(int blockId, PnodeBlockType blockType, charcount_t ichMin, charcount_t ichLim)
{
    __super::Init(knopBlock, ichMin, ichLim);

    this->pnodeScopes = nullptr;
    this->pnodeNext = nullptr;
    this->scope = nullptr;
    this->enclosingBlock = nullptr;
    this->pnodeLexVars = nullptr;
    this->pnodeStmt = nullptr;
    this->pnodeLastValStmt = nullptr;

    this->callsEval = false;
    this->childCallsEval = false;
    this->blockType = blockType;
    this->blockId = blockId;

    if (blockType != PnodeBlockType::Regular)
    {
        this->grfpn |= PNodeFlags::fpnSyntheticNode;
    }
}
