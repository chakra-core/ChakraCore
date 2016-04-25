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

        virtual void InitializeReader() override;
        virtual bool ReadNextSection(SectionCode nextSection) override;
        virtual bool ProcessCurrentSection() override;
        // todo:: Remove and use ReadNextSection and ProcessSection instead
        virtual WasmOp ReadFromModule();
        virtual WasmOp ReadFromBlock() override;
        virtual WasmOp ReadFromCall() override;
        virtual WasmOp ReadExpr() override;
        virtual void Unread() override { };
        virtual bool IsBinaryReader() override;
        static void __declspec(noreturn) ThrowSyntaxError();

    protected:
        SExprParseContext   m_context;
        SExprToken          m_token;

    private:
        enum FuncHeaderType : byte
        {
            funcType,
            funcImport,
            funcFull
        };

        WasmOp ReadExprCore(SExprTokenType tok);
        template <FuncHeaderType type>
        WasmOp ParseFunctionHeader();
        WasmOp ParseExport();
        WasmOp ParseMemory();
        WasmOp ParseTable();
        void ParseParam(WasmSignature * signature);
        void ParseResult(WasmSignature * signature);
        void ParseLocal();
        WasmOp ParseReturnExpr();
        void ParseMemOpExpr(WasmOp op);
        WasmOp ParseIfExpr();
        WasmOp ParseBrExpr(WasmOp op);
        WasmOp ParseBlock();
        WasmOp ParseCall();
        WasmOp ParseCallIndirect();
        WasmOp ParseConstLitExpr(SExprTokenType tok);
        void ParseGeneralExpr(WasmOp opcode);

        void ParseVarNode(WasmOp opcode);
        void ParseVar();
        void ParseFuncVar();

        bool IsEndOfExpr(SExprTokenType tok) const;
        WasmTypes::WasmType GetWasmType(SExprTokenType tok) const;

        virtual bool ReadFunctionBodies(FunctionBodyCallback callback, void* callbackdata) override;

        ArenaAllocator      m_alloc;
        SExprScanner *      m_scanner;

        uint m_funcNumber;
        typedef JsUtil::BaseDictionary<LPCUTF8, uint, ArenaAllocator> NameToIndexMap;
        NameToIndexMap * m_nameToFuncMap;

        NameToIndexMap * m_nameToLocalMap;

        JsUtil::Stack<SExpr::BlockType> * m_blockNesting;

        bool m_inExpr;
    };

} // namespace Wasm
