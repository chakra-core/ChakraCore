//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class WebAssemblySource;
    struct IWasmByteCodeWriter;
}

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
        BSTR errorMsg;
        // We need to explicitly delete these; simply not including them makes compilers do
        // generation of simple copy-construct and copy-assign functions, which incorrectly
        // copy around the errorMsg pointer, making it harder to work with the lifetime. If
        // we just use the PREVENT_COPY macro (which defines the functions as private) then
        // we get linker errors, since MSVC doesn't check the accessibility of the function
        // references when templating, and tries to link to the undefined functions. Explit
        // deletion of the functions is therefore required here.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    private:
        WasmCompilationException(const WasmCompilationException&) = delete;
        WasmCompilationException& operator=(const WasmCompilationException& other) = delete;
#else //if defined(_MSC_VER) && _MSC_VER < 1900
        // For older versions of VS, we need to provide copy construct/assign operators due
        // to the lack of the ability to throw-by-move.
    public:
        WasmCompilationException(const WasmCompilationException& other)
        {
            errorMsg = SysAllocString(other.errorMsg);
        }
        WasmCompilationException& operator=(const WasmCompilationException& other)
        {
            if(this != &other)
            {
                SysFreeString(errorMsg);
                errorMsg = SysAllocString(other.errorMsg);
            }
            return *this;
        }
#endif
    public:
        WasmCompilationException(const char16* _msg, ...);
        WasmCompilationException(const char16* _msg, va_list arglist);
        WasmCompilationException(WasmCompilationException&& other)
        {
            errorMsg = other.errorMsg;
            other.errorMsg = nullptr;
        }

        ~WasmCompilationException()
        {
            SysFreeString(errorMsg);
        }

        BSTR ReleaseErrorMessage()
        {
            Assert(errorMsg);
            BSTR msg = errorMsg;
            errorMsg = nullptr;
            return msg;
        }

        WasmCompilationException& operator=(WasmCompilationException&& other)
        {
            if (this != &other)
            {
                SysFreeString(errorMsg);
                errorMsg = other.errorMsg;
                other.errorMsg = nullptr;
            }
            return *this;
        }

        BSTR GetTempErrorMessageRef()
        {
            // This is basically a work-around for some odd lifetime scoping with throw
            return errorMsg;
        }
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
        bool IsEquivalent(const BlockInfo& other) const
        {
            if (HasYield() != other.HasYield())
            {
                return false;
            }
            if (HasYield() && yieldInfo->info.type != other.yieldInfo->info.type)
            {
                return false;
            }
            return true;
        }
    };

    typedef JsUtil::BaseDictionary<uint32, LPCUTF8, ArenaAllocator> WasmExportDictionary;

    struct WasmReaderInfo
    {
        Field(WasmFunctionInfo*) m_funcInfo;
        Field(Js::WebAssemblyModule*) m_module;
        Field(Js::Var) m_bufferSrc;
    };

    class WasmModuleGenerator
    {
    public:
        WasmModuleGenerator(Js::ScriptContext* scriptContext, Js::WebAssemblySource* src);
        Js::WebAssemblyModule* GenerateModule();
        void GenerateFunctionHeader(uint32 index);
    private:
        WasmBinaryReader* GetReader() const;

        Memory::Recycler* m_recycler;
        Js::Utf8SourceInfo* m_sourceInfo;
        Js::ScriptContext* m_scriptContext;
        Js::WebAssemblyModule* m_module;
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

        WasmBytecodeGenerator(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo, bool validateOnly);
        static void GenerateFunctionBytecode(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo, bool validateOnly = false);
        static void ValidateFunction(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo);

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
        EmitInfo EmitGrowMemory();
        EmitInfo EmitGetLocal();
        EmitInfo EmitGetGlobal();
        EmitInfo EmitSetGlobal();
        EmitInfo EmitSetLocal(bool tee);
        void EmitReturnExpr(EmitInfo* explicitRetInfo = nullptr);
        EmitInfo EmitSelect();
        template<typename WriteFn>
        void WriteTypeStack(WriteFn fn) const;
        uint32 WriteTypeStackToString(_Out_writes_(maxlen) char16* out, uint32 maxlen) const;
#if DBG_DUMP
        uint32 opId = 0;
        uint32 lastOpId = 1;
        void PrintOpBegin(WasmOp op);
        void PrintTypeStack() const;
        void PrintOpEnd();
#endif
        void EmitBr();
        EmitInfo EmitBrIf();

        EmitInfo EmitMemAccess(WasmOp wasmOp, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType, bool isStore);
        EmitInfo EmitBinExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        EmitInfo EmitUnaryExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);

        EmitInfo EmitConst(WasmTypes::WasmType type, WasmConstLitNode cnst);
        void EmitLoadConst(EmitInfo dst, WasmConstLitNode cnst);
        WasmConstLitNode GetZeroCnst();

        void EnsureStackAvailable();
        void EnregisterLocals();
        void ReleaseLocation(EmitInfo* info);

        EmitInfo PopLabel(Js::ByteCodeLabel labelValidation);
        BlockInfo PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo = true);
        void YieldToBlock(BlockInfo blockInfo, EmitInfo expr);
        BlockInfo GetBlockInfo(uint32 relativeDepth) const;

        Js::OpCodeAsmJs GetLoadOp(WasmTypes::WasmType type);
        Js::OpCodeAsmJs GetReturnOp(WasmTypes::WasmType type);
        WasmRegisterSpace* GetRegisterSpace(WasmTypes::WasmType type);

        EmitInfo PopEvalStack(WasmTypes::WasmType expectedType = WasmTypes::Any, const char16* mismatchMessage = nullptr);
        void PushEvalStack(EmitInfo);
        EmitInfo EnsureYield(BlockInfo);
        void EnterEvalStackScope();
        // The caller needs to release the location of the returned EmitInfo
        void ExitEvalStackScope();
        void SetUnreachableState(bool isUnreachable);
        bool IsUnreachable() const { return this->isUnreachable; }
        void SetUsesMemory(uint32 memoryIndex);

        Js::FunctionBody* GetFunctionBody() const { return m_funcInfo->GetBody(); }
        WasmReaderBase* GetReader() const;

        bool IsValidating() const { return m_originalWriter == m_emptyWriter; }

        ArenaAllocator m_alloc;

        bool isUnreachable;
        WasmLocal* m_locals;

        WasmFunctionInfo* m_funcInfo;
        Js::WebAssemblyModule* m_module;

        uint32 m_maxArgOutDepth;

        Js::IWasmByteCodeWriter* m_writer;
        Js::IWasmByteCodeWriter* m_emptyWriter;
        Js::IWasmByteCodeWriter* m_originalWriter;
        Js::ScriptContext* m_scriptContext;

        WAsmJs::TypedRegisterAllocator mTypedRegisterAllocator;

        JsUtil::Stack<BlockInfo> m_blockInfos;
        JsUtil::Stack<EmitInfo> m_evalStack;
    };
}
