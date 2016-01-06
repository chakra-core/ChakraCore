//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    struct SExprParseContext
    {
        LPCUTF8 source;
        size_t offset;
        size_t length;
    };

    namespace SExpr
    {
        enum BlockType
        {
            Module,
            Function,
            Expr,
            Block,
            Call
        };
    }

    class SExprParser : public BaseWasmReader
    {
    public:
        SExprParser(PageAllocator * alloc, LPCUTF8 source, size_t length);

        virtual WasmOp ReadFromScript() override;
        virtual WasmOp ReadFromModule() override;
        virtual WasmOp ReadFromBlock() override;
        virtual WasmOp ReadFromCall() override;
        virtual WasmOp ReadExpr() override;
        virtual bool IsBinaryReader() override;
        static void __declspec(noreturn) ThrowSyntaxError();

    protected:
        SExprParseContext   m_context;
        SExprToken          m_token;

    private:
        WasmOp ReadExprCore(SExprTokenType tok);
        WasmOp ParseFunctionHeader();
        WasmOp ParseExport();
        void ParseParam();
        void ParseResult();
        void ParseLocal();
        WasmOp ParseReturnExpr();
        WasmOp ParseIfExpr();
        WasmOp ParseBlock();
        WasmOp ParseCall();
        WasmOp ParseConstLitExpr(SExprTokenType tok);
        void ParseGeneralExpr(WasmOp opcode);
        WasmNode * ParseInvoke();
        WasmNode * ParseAssertEq();

        void ParseVarNode(WasmOp opcode);
        void ParseVar();
        void ParseFuncVar();

        bool IsEndOfExpr(SExprTokenType tok) const;
        WasmTypes::WasmType GetWasmType(SExprTokenType tok) const;

        ArenaAllocator      m_alloc;
        SExprScanner *      m_scanner;

        uint m_funcNumber;
        typedef JsUtil::BaseDictionary<LPCUTF8, uint, ArenaAllocator> NameToIndexMap;
        NameToIndexMap * m_nameToFuncMap;

        NameToIndexMap * m_nameToLocalMap;

        SList<SExpr::BlockType> * m_blockNesting;

        WasmFunctionInfo *  m_funcInfo;

        bool m_inExpr;
    };

} // namespace Wasm
