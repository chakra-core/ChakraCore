//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Wasm
{
    struct EmitInfo
    {
        EmitInfo(Js::RegSlot location_, const WasmTypes::WasmType& type_) :
            location(location_), type(type_)
        {
        }
        EmitInfo(const WasmTypes::WasmType& type_) :
            location(Js::Constants::NoRegister), type(type_)
        {
        }
        EmitInfo() :
            location(Js::Constants::NoRegister), type(WasmTypes::Void)
        {
        }

        Js::RegSlot location;
        WasmTypes::WasmType type;
    };

    struct WasmLocal
    {
        WasmLocal() :
            location(Js::Constants::NoRegister), type(WasmTypes::Limit)
        {
        }
        WasmLocal(Js::RegSlot loc, WasmTypes::WasmType tp) :
            location(loc), type(tp)
        {
        }
        Js::RegSlot location;
        WasmTypes::WasmType type;
    };

    class WasmToAsmJs
    {
    public:
        static Js::AsmJsRetType GetAsmJsReturnType(WasmTypes::WasmType wasmType);
        static Js::AsmJsVarType GetAsmJsVarType(WasmTypes::WasmType wasmType);
    };

    class WasmCompilationException
    {
        void FormatError(const char16* _msg, va_list arglist);
        char16* errorMsg;
    public:
        ~WasmCompilationException();
        WasmCompilationException(const char16* _msg, ...);
        WasmCompilationException(const char16* _msg, va_list arglist);

        void SetErrorMessage(char16* _errorMsg)
        {
            errorMsg = _errorMsg;
        }

        const char16* GetErrorMessage() const
        {
            return errorMsg;
        }

        char16* ReleaseErrorMessage()
        {
            Assert(errorMsg);
            char16* msg = errorMsg;
            errorMsg = nullptr;
            return msg;
        };
    };

    struct BlockYieldInfo
    {
        Js::RegSlot yieldLocs[WasmTypes::Limit];
        WasmTypes::WasmType type = WasmTypes::Limit;
    };

    struct BlockInfo
    {
        BlockYieldInfo* yieldInfo = nullptr;
        Js::ByteCodeLabel label;
    };

    typedef JsUtil::BaseDictionary<uint, LPCUTF8, ArenaAllocator> WasmExportDictionary;

    struct WasmReaderInfo
    {
        Binary::WasmBinaryReader* m_reader;
        WasmFunctionInfo* m_funcInfo;
        WasmModule* m_module;
    };

    class WasmModuleGenerator
    {
    public:
        WasmModuleGenerator(Js::ScriptContext * scriptContext, Js::Utf8SourceInfo * sourceInfo, byte* binaryBuffer, uint binaryBufferLength);
        WasmModule * GenerateModule();
        WasmFunction * GenerateFunctionHeader(uint32 index);
    private:
        ArenaAllocator* m_alloc;
        Js::Utf8SourceInfo * m_sourceInfo;
        Js::ScriptContext * m_scriptContext;
        Binary::WasmBinaryReader* m_reader;
        WasmModule * m_module;
    };

    class WasmBytecodeGenerator
    {
    public:
        static const Js::RegSlot ModuleSlotRegister = 0;
        static const Js::RegSlot ReturnRegister = 0;

        static const Js::RegSlot FunctionRegister = 0;
        static const Js::RegSlot CallReturnRegister = 0;
        static const Js::RegSlot ModuleEnvRegister = 1;
        static const Js::RegSlot ArrayBufferRegister = 2;
        static const Js::RegSlot ArraySizeRegister = 3;
        static const Js::RegSlot ScriptContextBufferRegister = 4;
        static const Js::RegSlot ReservedRegisterCount = 5;

        WasmBytecodeGenerator(Js::ScriptContext* scriptContext, Js::FunctionBody* body, WasmReaderInfo* readerinfo);
        static void GenerateFunctionBytecode(Js::ScriptContext* scriptContext, Js::FunctionBody* body, WasmReaderInfo* readerinfo);

    private:
        void GenerateFunction();

        EmitInfo EmitExpr(WasmOp op);
        EmitInfo EmitBlock();
        EmitInfo EmitBlockCommon();
        EmitInfo EmitLoop();

        template<WasmOp wasmOp>
        EmitInfo EmitCall();
        EmitInfo EmitIfElseExpr();
        EmitInfo EmitBrTable();
        EmitInfo EmitGetLocal();
        EmitInfo EmitSetLocal();
        EmitInfo EmitReturnExpr();
        EmitInfo EmitSelect();
#if DBG_DUMP
        void PrintOpName(WasmOp op) const;
#endif
        template<WasmOp wasmOp>
        EmitInfo EmitBr();

        template<WasmOp wasmOp, WasmTypes::WasmType type>
        EmitInfo EmitMemRead();

        template<WasmOp wasmOp, WasmTypes::WasmType type>
        EmitInfo EmitMemStore();

        template<Js::OpCodeAsmJs op, WasmTypes::WasmType resultType, WasmTypes::WasmType lhsType, WasmTypes::WasmType rhsType>
        EmitInfo EmitBinExpr();

        template<Js::OpCodeAsmJs op, WasmTypes::WasmType resultType, WasmTypes::WasmType inputType>
        EmitInfo EmitUnaryExpr();

        template<WasmTypes::WasmType type>
        EmitInfo EmitConst();

        void EnregisterLocals();
        void ReleaseLocation(EmitInfo * info);

        EmitInfo PopLabel(Js::ByteCodeLabel labelValidation);
        void PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo = true);
        void YieldToBlock(uint relativeDepth, EmitInfo expr);
        BlockInfo GetBlockInfo(uint relativeDepth);
        Js::ByteCodeLabel GetLabel(uint relativeDepth);

        static Js::ArrayBufferView::ViewType GetViewType(WasmOp op);
        static Js::OpCodeAsmJs GetLoadOp(WasmTypes::WasmType type);
        WasmRegisterSpace * GetRegisterSpace(WasmTypes::WasmType type);

        EmitInfo PopEvalStack();
        void PushEvalStack(EmitInfo);
        void EnterEvalStackScope();
        void ExitEvalStackScope();

        ArenaAllocator m_alloc;

        WasmLocal * m_locals;

        WasmFunctionInfo * m_funcInfo;
        Js::FunctionBody * m_body;
        WasmModule * m_module;

        uint m_maxArgOutDepth;

        Binary::WasmBinaryReader* m_reader;

        Js::AsmJsByteCodeWriter m_writer;
        Js::ScriptContext * m_scriptContext;

        WasmRegisterSpace m_i32RegSlots;
        WasmRegisterSpace m_f32RegSlots;
        WasmRegisterSpace m_f64RegSlots;

        JsUtil::Stack<BlockInfo> m_blockInfos;
        JsUtil::Stack<EmitInfo> m_evalStack;
    };
}
