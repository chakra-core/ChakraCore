//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
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

    struct PolymorphicEmitInfo
    {
    private:
        uint32 count = 0;
        union
        {
            EmitInfo singleInfo;
            EmitInfo* infos;
        };
    public:
        PolymorphicEmitInfo(): count(0), infos(nullptr) {}
        PolymorphicEmitInfo(EmitInfo info)
        {
            Init(info);
        }
        uint32 Count() const { return count; }
        void Init(EmitInfo info);
        void Init(uint32 count, ArenaAllocator* alloc);
        void SetInfo(EmitInfo info, uint32 index);
        EmitInfo GetInfo(uint32 index) const;
        bool IsUnreachable() const;
        bool IsEquivalent(PolymorphicEmitInfo other) const;
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
        PolymorphicEmitInfo paramInfo;
        PolymorphicEmitInfo yieldInfo;
        bool didYield = false;
        Js::ByteCodeLabel label;
        bool DidYield() const { return didYield; }
        bool HasYield() const
        {
            return yieldInfo.Count() > 0;
        }
        bool IsEquivalent(const BlockInfo* other) const
        {
            if (HasYield() != other->HasYield())
            {
                return false;
            }
            if (HasYield())
            {
                return yieldInfo.IsEquivalent(other->yieldInfo);
            }

            return true;
        }
    };

    typedef JsUtil::BaseDictionary<uint32, LPCUTF8, ArenaAllocator> WasmExportDictionary;

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

        template <size_t lanes> 
        EmitInfo EmitSimdBuildExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        void EmitExpr(WasmOp op);
        PolymorphicEmitInfo EmitBlock();
        void EmitBlockCommon(BlockInfo* blockInfo, bool* endOnElse = nullptr);
        PolymorphicEmitInfo EmitLoop();

        template<WasmOp wasmOp>
        PolymorphicEmitInfo EmitCall();
        PolymorphicEmitInfo EmitIfElseExpr();
        void EmitBrTable();
        EmitInfo EmitDrop();
        EmitInfo EmitGrowMemory();
        EmitInfo EmitGetLocal();
        EmitInfo EmitGetGlobal();
        EmitInfo EmitSetGlobal();
        EmitInfo EmitSetLocal(bool tee);
        void EmitReturnExpr(PolymorphicEmitInfo* explicitRetInfo = nullptr);
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
        PolymorphicEmitInfo EmitBrIf();

        template<bool isStore, bool isAtomic>
        EmitInfo EmitMemAccess(WasmOp wasmOp, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType);
        EmitInfo EmitSimdMemAccess(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType, uint8 dataWidth, bool isStore);
        EmitInfo EmitBinExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        EmitInfo EmitUnaryExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        EmitInfo EmitM128BitSelect();
        EmitInfo EmitV8X16Shuffle();
        EmitInfo EmitExtractLaneExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        EmitInfo EmitReplaceLaneExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature);
        void CheckLaneIndex(Js::OpCodeAsmJs op, const uint index);
        EmitInfo EmitLaneIndex(Js::OpCodeAsmJs op);

        EmitInfo EmitConst(WasmTypes::WasmType type, WasmConstLitNode cnst);
        void EmitLoadConst(EmitInfo dst, WasmConstLitNode cnst);
        WasmConstLitNode GetZeroCnst();
        void EnsureStackAvailable();

        void EnregisterLocals();
        void ReleaseLocation(EmitInfo* info);
        void ReleaseLocation(PolymorphicEmitInfo* info);

        PolymorphicEmitInfo PopLabel(Js::ByteCodeLabel labelValidation);
        BlockInfo* PushLabel(WasmBlock blockData, Js::ByteCodeLabel label, bool addBlockYieldInfo = true, bool checkInParams = true);
        void YieldToBlock(BlockInfo* blockInfo, PolymorphicEmitInfo expr);
        BlockInfo* GetBlockInfo(uint32 relativeDepth) const;

        Js::OpCodeAsmJs GetLoadOp(WasmTypes::WasmType type);
        Js::OpCodeAsmJs GetReturnOp(WasmTypes::WasmType type);
        WasmRegisterSpace* GetRegisterSpace(WasmTypes::WasmType type);

        EmitInfo PopValuePolymorphic() { return PopEvalStack(); }
        PolymorphicEmitInfo PopStackPolymorphic(PolymorphicEmitInfo expectedTypes, const char16* mismatchMessage = nullptr);
        PolymorphicEmitInfo PopStackPolymorphic(const BlockInfo* blockInfo, const char16* mismatchMessage = nullptr);
        EmitInfo PopEvalStack(WasmTypes::WasmType expectedType = WasmTypes::Any, const char16* mismatchMessage = nullptr);
        void PushEvalStack(PolymorphicEmitInfo);
        PolymorphicEmitInfo EnsureYield(BlockInfo*);
        void EnterEvalStackScope(const BlockInfo*);
        // The caller needs to release the location of the returned EmitInfo
        void ExitEvalStackScope(const BlockInfo*);
        void SetUnreachableState(bool isUnreachable);
        bool IsUnreachable() const { return this->isUnreachable; }
        void SetUsesMemory(uint32 memoryIndex);

        Js::FunctionBody* GetFunctionBody() const { return m_funcInfo->GetBody(); }
        WasmReaderBase* GetReader() const;

        Js::ProfileId GetNextProfileId();
        bool IsValidating() const { return m_originalWriter == m_emptyWriter; }

        ArenaAllocator m_alloc;

        bool isUnreachable;
        WasmLocal* m_locals;

        WasmFunctionInfo* m_funcInfo;
        BlockInfo* m_funcBlock;
        Js::WebAssemblyModule* m_module;

        uint32 m_maxArgOutDepth;

        Js::IWasmByteCodeWriter* m_writer;
        Js::IWasmByteCodeWriter* m_emptyWriter;
        Js::IWasmByteCodeWriter* m_originalWriter;
        Js::ScriptContext* m_scriptContext;

        WAsmJs::TypedRegisterAllocator mTypedRegisterAllocator;

        Js::ProfileId currentProfileId;
        JsUtil::Stack<BlockInfo*> m_blockInfos;
        JsUtil::Stack<EmitInfo> m_evalStack;
    };
}
