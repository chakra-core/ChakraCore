//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Wasm
{
    struct EmitInfo : WAsmJs::EmitInfoBase
    {
        EmitInfo(Js::RegSlot location_, const WasmTypes::WasmType& type_) :
            WAsmJs::EmitInfoBase(location_), type(type_)
        {
        }
        EmitInfo(const WasmTypes::WasmType& type_) : type(type_) {}
        EmitInfo() : type(WasmTypes::Void) {}

        WasmTypes::WasmType type;
    };
    typedef WAsmJs::RegisterSpace WasmRegisterSpace;

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
        WasmCompilationException(const char16* _msg, ...);
        WasmCompilationException(const char16* _msg, va_list arglist);

        char16* ReleaseErrorMessage()
        {
            Assert(errorMsg);
            char16* msg = errorMsg;
            errorMsg = nullptr;
            return msg;
        };
    };

    struct BlockInfo
    {
        struct YieldInfo
        {
            EmitInfo info;
            bool didYield = false;
        } *yieldInfo = nullptr;
        Js::ByteCodeLabel label;
        bool DidYield() const { return HasYield() && yieldInfo->didYield; }
        bool HasYield() const { return yieldInfo != nullptr; }
    };

    typedef JsUtil::BaseDictionary<uint, LPCUTF8, ArenaAllocator> WasmExportDictionary;

    struct WasmReaderInfo
    {
        WasmFunctionInfo* m_funcInfo;
        WasmModule* m_module;
    };

    class WasmModuleGenerator
    {
    public:
        WasmModuleGenerator(Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* sourceInfo, byte* binaryBuffer, uint binaryBufferLength);
        WasmModule* GenerateModule();
        void GenerateFunctionHeader(uint32 index);
    private:
        WasmBinaryReader* GetReader() const { return m_module->GetReader(); }

        Memory::Recycler* m_recycler;
        Js::Utf8SourceInfo* m_sourceInfo;
        Js::ScriptContext* m_scriptContext;
        WasmModule* m_module;
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

        WasmBytecodeGenerator(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo);
        static void GenerateFunctionBytecode(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo);

    private:
        void GenerateFunction();

        void EmitExpr(WasmOp op);
        EmitInfo EmitBlock();
        void EmitBlockCommon(BlockInfo* blockInfo, bool* endOnElse = nullptr);
        EmitInfo EmitLoop();

        template<WasmOp wasmOp>
        EmitInfo EmitCall();
        EmitInfo EmitIfElseExpr();
        void EmitBrTable();
        EmitInfo EmitDrop();
        EmitInfo EmitGetLocal();
        EmitInfo EmitSetLocal(bool tee);
        void EmitReturnExpr(EmitInfo* explicitRetInfo = nullptr);
        EmitInfo EmitSelect();
#if DBG_DUMP
        void PrintOpName(WasmOp op) const;
#endif
        void EmitBr();
        EmitInfo EmitBrIf();

        template<WasmOp wasmOp, const WasmTypes::WasmType* signature>
        EmitInfo EmitMemAccess(bool isStore);

        template<Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature>
        EmitInfo EmitBinExpr();

        template<Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature>
        EmitInfo EmitUnaryExpr();

        template<WasmTypes::WasmType type>
        EmitInfo EmitConst();
        void EmitLoadConst(EmitInfo dst, WasmConstLitNode cnst);

        void EnregisterLocals();
        void ReleaseLocation(EmitInfo* info);

        EmitInfo PopLabel(Js::ByteCodeLabel labelValidation);
        BlockInfo PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo = true);
        void YieldToBlock(BlockInfo blockInfo, EmitInfo expr);
        void YieldToBlock(uint relativeDepth, EmitInfo expr);
        bool ShouldYieldToBlock(uint relativeDepth) const;
        BlockInfo GetBlockInfo(uint relativeDepth) const;
        Js::ByteCodeLabel GetLabel(uint relativeDepth);

        static bool IsBlockOpCode(WasmOp op);
        static Js::ArrayBufferView::ViewType GetViewType(WasmOp op);
        static Js::OpCodeAsmJs GetLoadOp(WasmTypes::WasmType type);
        static Js::OpCodeAsmJs GetReturnOp(WasmTypes::WasmType type);
        WasmRegisterSpace* GetRegisterSpace(WasmTypes::WasmType type);

        EmitInfo PopEvalStack();
        void PushEvalStack(EmitInfo);
        void EnterEvalStackScope();
        // The caller needs to release the location of the returned EmitInfo
        void ExitEvalStackScope();
        void SetUnreachableState(bool isUnreachable);
        bool IsUnreachable() const { return this->isUnreachable; }

        Js::FunctionBody* GetFunctionBody() const { return m_funcInfo->GetBody(); }
        WasmBinaryReader* GetReader() const { return m_module->GetReader(); }

        ArenaAllocator m_alloc;

        bool isUnreachable;
        WasmLocal* m_locals;

        WasmFunctionInfo* m_funcInfo;
        WasmModule* m_module;

        uint m_maxArgOutDepth;

        Js::AsmJsByteCodeWriter m_writer;
        Js::ScriptContext* m_scriptContext;

        WasmRegisterSpace m_i32RegSlots;
        WasmRegisterSpace m_f32RegSlots;
        WasmRegisterSpace m_f64RegSlots;

        JsUtil::Stack<BlockInfo> m_blockInfos;
        JsUtil::Stack<EmitInfo> m_evalStack;
    };
}
