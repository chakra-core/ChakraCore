//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

SExprParser::SExprParser(PageAllocator * alloc, LPCUTF8 source, size_t length) :
    m_alloc(L"SExprParser", alloc, Js::Throw::OutOfMemory),
    m_inExpr(false)
{
    m_scanner = Anew(&m_alloc, SExprScanner, &m_alloc);
    m_blockNesting = Anew(&m_alloc, JsUtil::Stack<SExpr::BlockType>, &m_alloc);
    m_context.offset = 0;
    m_context.source = source;
    m_context.length = length;

    m_scanner->Init(&m_context, &m_token);
}

bool
SExprParser::IsBinaryReader()
{
    return false;
}

WasmOp
SExprParser::ReadFromScript()
{
    SExprTokenType tok = m_scanner->Scan();

    if (tok == wtkEOF)
    {
        return wnLIMIT;
    }

    if (tok != wtkLPAREN)
    {
        ThrowSyntaxError();
    }
    tok = m_scanner->Scan();

    switch (tok)
    {
    case wtkMODULE:
        // reset per-module data
        m_funcNumber = 0;
        m_nameToFuncMap = Anew(&m_alloc, NameToIndexMap, &m_alloc);
        m_nameToLocalMap = Anew(&m_alloc, NameToIndexMap, &m_alloc);
        return wnMODULE;
    case wtkINVOKE:
        return wnINVOKE;
    case wtkASSERTEQ:
        return wnASSERTEQ;
    default:
        ThrowSyntaxError();
    }
}

WasmOp
SExprParser::ReadFromModule()
{
    SExprTokenType tok = m_scanner->Scan();

    if (IsEndOfExpr(tok))
    {
        return wnLIMIT;
    }

    tok = m_scanner->Scan();

    switch(tok)
    {
    case wtkFUNC:
        return ParseFunctionHeader();
    case wtkEXPORT:
        return ParseExport();
    // TODO: implement the following
    case wtkTABLE:
    case wtkMEMORY:
    case wtkDATA:
    case wtkGLOBAL:
    default:
        ThrowSyntaxError();
    }
}

WasmOp
SExprParser::ReadFromBlock()
{
    SExprTokenType tok = m_scanner->Scan();

    // in some cases we will have already scanned the LParen
    if (!m_inExpr)
    {
        // we could be in nested expression and might need to pop off some RParens
        while (tok == wtkRPAREN && m_blockNesting->Count() > 0 && m_blockNesting->Top() == SExpr::Expr)
        {
            tok = m_scanner->Scan();
            m_blockNesting->Pop();
        }

        if (tok == wtkRPAREN && m_blockNesting->Count() > 0 && m_blockNesting->Top() == SExpr::Block)
        {
            m_blockNesting->Pop();
            return wnLIMIT;
        }
        if (tok != wtkLPAREN || m_blockNesting->Count() == 0)
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
    }
    return ReadExprCore(tok);
}

WasmOp
SExprParser::ReadFromCall()
{
    SExprTokenType tok = m_scanner->Scan();

    // in some cases we will have already scanned the LParen
    if (!m_inExpr)
    {
        while (tok == wtkRPAREN && m_blockNesting->Count() > 0 && m_blockNesting->Top() == SExpr::Expr)
        {
            tok = m_scanner->Scan();
            m_blockNesting->Pop();
        }

        if (tok == wtkRPAREN && m_blockNesting->Count() > 0 && m_blockNesting->Top() == SExpr::Call)
        {
            m_blockNesting->Pop();
            return wnLIMIT;
        }
        if (tok != wtkLPAREN || m_blockNesting->Count() == 0)
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
    }
    return ReadExprCore(tok);
}

WasmOp
SExprParser::ReadExpr()
{
    SExprTokenType tok = m_scanner->Scan();

    // in some cases we will have already scanned the LParen
    if (!m_inExpr)
    {
        // we could be in nested expression and might need to pop off some RParens
        while (tok == wtkRPAREN && m_blockNesting->Count() > 0 && m_blockNesting->Top() == SExpr::Expr)
        {
            tok = m_scanner->Scan();
            m_blockNesting->Pop();
        }
        // if we have a RParen, and no more nested exprs, then we are done with function
        if (tok == wtkRPAREN && m_blockNesting->Count() == 0)
        {
            return wnLIMIT;
        }
        if (tok != wtkLPAREN)
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
    }

    return ReadExprCore(tok);
}

WasmOp
SExprParser::ReadExprCore(SExprTokenType tok)
{
    m_inExpr = false;
    WasmOp op = WasmOp::wnLIMIT;
    switch (tok)
    {
    case wtkNOP:
        return wnNOP;
    case wtkBLOCK:
        return ParseBlock();
    case wtkCALL:
        return ParseCall();
    case wtkLOOP:
        return wnLOOP;
    case wtkLABEL:
        return wnLABEL;
    case wtkRETURN:
        return ParseReturnExpr();
    case wtkIF:
    case wtkIF_ELSE:
        return ParseIfExpr();
    case wtkGETLOCAL:
        op = wnGETLOCAL;
        goto ParseVarCommon;
    case wtkSETLOCAL:
        op = wnSETLOCAL;
        goto ParseVarCommon;
    case wtkGETGLOBAL:
        op = wnGetGlobal;
        goto ParseVarCommon;
    case wtkSETGLOBAL:
        op = wnSetGlobal;
        goto ParseVarCommon;
    case wtkCONST_I32:
    case wtkCONST_F32:
    case wtkCONST_F64:
        return ParseConstLitExpr(tok);
        break;

#define WASM_KEYWORD_BIN(token, name) \
        case wtk##token: \
            ParseGeneralExpr(wn##token); \
            return wn##token;
#define WASM_KEYWORD_UNARY(token, name) WASM_KEYWORD_BIN(token, name)

#include "WasmKeywords.h"

ParseVarCommon:
        ParseVarNode(op);
        return op;

    // TODO: implement enumerated ops
    case wtkGET_NEAR_S:
    case wtkGET_NEAR_U:
    case wtkGET_NEAR_UNALIGNED_S:
    case wtkGET_NEAR_UNALIGNED_U:
    case wtkSET_NEAR_S:
    case wtkSET_NEAR_U:
    case wtkSET_NEAR_UNALIGNED_S:
    case wtkSET_NEAR_UNALIGNED_U:
    case wtkBREAK:
    case wtkSWITCH:
    case wtkDISPATCH:
    case wtkDESTRUCT:
    default:
        ThrowSyntaxError();
    }
}

WasmOp
SExprParser::ParseFunctionHeader()
{
    m_currentNode.op = wnFUNC;

    SExprTokenType tok = m_scanner->Scan();

    if (tok == wtkSTRINGLIT)
    {
        if (!m_nameToFuncMap->AddNew(m_token.u.m_sz, m_funcNumber))
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
    }

    m_funcInfo = Anew(&m_alloc, WasmFunctionInfo, &m_alloc);
    m_currentNode.func.info = m_funcInfo;

    m_funcNumber++;

    if (IsEndOfExpr(tok))
    {
        return m_currentNode.op;
    }

    tok = m_scanner->Scan();

    // TODO: support <type>? for indirect calls

    while (tok == wtkPARAM)
    {
        ParseParam();

        tok = m_scanner->Scan();

        if (IsEndOfExpr(tok))
        {
            return m_currentNode.op;
        }

        tok = m_scanner->Scan();
    }

    if (tok == wtkRESULT)
    {
        ParseResult();

        tok = m_scanner->Scan();

        if (IsEndOfExpr(tok))
        {
            return m_currentNode.op;
        }

        tok = m_scanner->Scan();
    }

    while (tok == wtkLOCAL)
    {
        ParseLocal();

        tok = m_scanner->Scan();

        if (IsEndOfExpr(tok))
        {
            return m_currentNode.op;
        }

        tok = m_scanner->Scan();
    }
    // we have already scanned LParen and keyword, so we will undo keyword scan and notify that we are in an expr
    m_inExpr = true;
    m_scanner->UndoScan();

    return m_currentNode.op;
}

WasmOp
SExprParser::ParseExport()
{
    m_currentNode.op = wnEXPORT;

    m_scanner->ScanToken(wtkSTRINGLIT);

    m_currentNode.var.exportName = m_token.u.m_sz;

    ParseFuncVar();

    m_scanner->ScanToken(wtkRPAREN);

    return m_currentNode.op;
}

void
SExprParser::ParseParam()
{
    SExprTokenType tok = m_scanner->Scan();
    if (tok == wtkID)
    {
        if (!m_nameToLocalMap->AddNew(m_token.u.m_sz, m_funcInfo->GetLocalCount()))
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
        m_funcInfo->AddParam(GetWasmType(tok));
        m_scanner->ScanToken(wtkRPAREN);
    }
    else
    {
        while (tok != wtkRPAREN)
        {
            m_funcInfo->AddParam(GetWasmType(tok));
            tok = m_scanner->Scan();
        }
    }
}

void
SExprParser::ParseResult()
{
    SExprTokenType tok = m_scanner->Scan();
    m_funcInfo->SetResultType(GetWasmType(tok));
    m_scanner->ScanToken(wtkRPAREN);
}

void
SExprParser::ParseLocal()
{
    SExprTokenType tok = m_scanner->Scan();
    if (tok == wtkID)
    {
        if (!m_nameToLocalMap->AddNew(m_token.u.m_sz, m_funcInfo->GetLocalCount()))
        {
            ThrowSyntaxError();
        }
        tok = m_scanner->Scan();
        m_funcInfo->AddLocal(GetWasmType(tok));
        m_scanner->ScanToken(wtkRPAREN);
    }
    else
    {
        while (tok != wtkRPAREN)
        {
            m_funcInfo->AddLocal(GetWasmType(tok));
            tok = m_scanner->Scan();
        }
    }
}


WasmOp
SExprParser::ParseReturnExpr()
{
    m_currentNode.op = wnRETURN;

    // check for return expression
    SExprTokenType tok = m_scanner->Scan();
    if (tok == wtkRPAREN)
    {
        m_currentNode.opt.exists = false;
    }
    else if (tok == wtkLPAREN)
    {
        m_currentNode.opt.exists = true;
        m_inExpr = true;
        m_blockNesting->Push(SExpr::Expr);
    }
    else
    {
        ThrowSyntaxError();
    }

    return m_currentNode.op;
}


WasmOp
SExprParser::ParseIfExpr()
{
    m_currentNode.op = wnIF;

    m_blockNesting->Push(SExpr::Expr);

    return m_currentNode.op;
}

WasmOp SExprParser::ParseBlock()
{
    m_blockNesting->Push(SExpr::Block);
    return wnBLOCK;
}

WasmOp SExprParser::ParseCall()
{
    m_blockNesting->Push(SExpr::Call);

    ParseFuncVar();

    return wnCALL;
}

WasmOp
SExprParser::ParseConstLitExpr(SExprTokenType tok)
{

    switch (tok)
    {
    case wtkCONST_I32:
        m_scanner->ScanToken(wtkINTLIT);
        m_currentNode.op = wnCONST_I32;
        m_currentNode.cnst.i32 = (int32)m_token.u.lng;

        break;
    case wtkCONST_F32:
        m_currentNode.op = wnCONST_F32;
        m_scanner->ScanToken(wtkFLOATLIT);

        m_currentNode.cnst.f32 = (float)m_token.u.dbl;
        break;
    case wtkCONST_F64:
        m_currentNode.op = wnCONST_F64;
        m_scanner->ScanToken(wtkFLOATLIT);

        m_currentNode.cnst.f64 = m_token.u.dbl;
        break;
    default:
        ThrowSyntaxError();
    }
    m_scanner->ScanToken(wtkRPAREN);
    return m_currentNode.op;
}

void
SExprParser::ParseGeneralExpr(WasmOp opcode)
{
    m_currentNode.op = opcode;
    m_blockNesting->Push(SExpr::Expr);
}

WasmNode *
SExprParser::ParseInvoke()
{
    SExprTokenType tok = m_scanner->Scan();
    if (tok != wtkSTRINGLIT)
    {
        ThrowSyntaxError();
    }

    WasmNode * invokeNode = Anew(&m_alloc, WasmNode);
    invokeNode->op = wnINVOKE;
    invokeNode->invk.name = m_token.u.m_sz;

    return invokeNode;
}

WasmNode *
SExprParser::ParseAssertEq()
{
    m_scanner->ScanToken(wtkLPAREN);
    m_scanner->ScanToken(wtkINVOKE);

    WasmNode * assertNode = Anew(&m_alloc, WasmNode);
    assertNode->op = wnASSERTEQ;

    m_scanner->ScanToken(wtkRPAREN);

    return assertNode;
}

void
SExprParser::ParseVarNode(WasmOp opcode)
{
    ParseVar();

    m_currentNode.op = opcode;

    if (opcode == wnSETLOCAL || opcode == wnSetGlobal)
    {
        m_blockNesting->Push(SExpr::Expr);
    }
    else
    {
        m_scanner->ScanToken(wtkRPAREN);
    }

}
void SExprParser::ParseFuncVar()
{
    SExprTokenType tok = m_scanner->Scan();

    switch (tok)
    {
    case wtkID:
        m_currentNode.var.num = m_nameToFuncMap->Lookup(m_token.u.m_sz, UINT_MAX);
        if (m_currentNode.var.num == UINT_MAX)
        {
            ThrowSyntaxError();
        }
        break;
    case wtkINTLIT:
        if (m_token.u.lng < 0 || m_token.u.lng >= UINT_MAX)
        {
            ThrowSyntaxError();
        }
        m_currentNode.var.num = (uint)m_token.u.lng;
        break;
    default:
        ThrowSyntaxError();
    }
}

void SExprParser::ParseVar()
{
    SExprTokenType tok = m_scanner->Scan();

    switch (tok)
    {
    case wtkID:
        m_currentNode.var.num = m_nameToLocalMap->Lookup(m_token.u.m_sz, UINT_MAX);
        if (m_currentNode.var.num == UINT_MAX)
        {
            ThrowSyntaxError();
        }
        break;
    case wtkINTLIT:
        if (m_token.u.lng < 0 || m_token.u.lng >= UINT_MAX)
        {
            ThrowSyntaxError();
        }
        m_currentNode.var.num = (uint)m_token.u.lng;
        break;
    default:
        ThrowSyntaxError();
    }
}

bool
SExprParser::IsEndOfExpr(SExprTokenType tok) const
{
    if (tok == wtkRPAREN)
    {
        return true;
    }
    if (tok == wtkLPAREN)
    {
        return false;
    }
    ThrowSyntaxError();
}

WasmTypes::WasmType
SExprParser::GetWasmType(SExprTokenType tok) const
{
    switch (tok)
    {
#define WASM_LOCALTYPE(token, keyword) \
    case wtk##token: \
        return WasmTypes::##token;
#include "WasmKeywords.h"
    default:
        ThrowSyntaxError();
    }
}

void
SExprParser::ThrowSyntaxError()
{
    Js::Throw::InternalError();
}

} // namespace Wasm

#endif // ENABLE_WASM
