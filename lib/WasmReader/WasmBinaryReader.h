//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    // Language Types binary encoding with varint7
    namespace LanguageTypes
    { 
        const int8 i32 = -0x1;
        const int8 i64 = -0x2;
        const int8 f32 = -0x3;
        const int8 f64 = -0x4;
        const int8 m128 = -0x5;
        const int8 anyfunc = -0x10;
        const int8 func = -0x20;
        const int8 emptyBlock = -0x40;
        WasmTypes::WasmType ToWasmType(int8);
    }

    struct SectionHeader
    {
        SectionCode code;
        const byte* start;
        const byte* end;
        uint32 nameLength;
        const char16* name;
    };

    struct BinaryLocation
    {
        intptr_t offset;
        intptr_t size;
    };

    static const uint32 binaryVersion = 0x1;

    class WasmBinaryReader : public WasmReaderBase
    {
    public:
        WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule* module, const byte* source, size_t length);

        void InitializeReader();
        SectionHeader ReadNextSection();
        // Fully read the section in the reader. Return true if the section fully read
        bool ProcessCurrentSection();
        virtual void SeekToFunctionBody(class WasmFunctionInfo* funcInfo) override;
        virtual bool IsCurrentFunctionCompleted() const override;

        WasmOp ReadPrefixedOpCode(WasmOp prefix, bool isSupported, const char16* notSupportedMsg);
        WasmOp ReadOpCode();
        virtual WasmOp ReadExpr() override;
        virtual void FunctionEnd() override;
        virtual uint32 EstimateCurrentFunctionBytecodeSize() const override;
#if DBG_DUMP
        void PrintOps();
#endif
        BinaryLocation GetCurrentLocation() const { return {m_pc - m_start, m_end - m_start}; }
    private:
        struct ReaderState
        {
#if ENABLE_DEBUG_CONFIG_OPTIONS
            Js::FunctionBody* body = nullptr;
#endif
            uint32 count; // current entry
            uint32 size;  // binary size of the function
        };

        void BlockNode();
        void CallNode();
        void CallIndirectNode();
        void BrNode();
        void BrTableNode();
        void MemNode();
        void LaneNode();
        void ShuffleNode();
        void VarNode();

        // Module readers
        void ValidateModuleHeader();
        SectionHeader ReadSectionHeader();
        void ReadMemorySection(bool isImportSection);
        void ReadSignatureTypeSection();
        void ReadFunctionSignatures();
        void ReadFunctionHeaders();
        void ReadExportSection();
        void ReadTableSection(bool isImportSection);
        void ReadDataSection();
        void ReadImportSection();
        void ReadStartFunction();
        void ReadNameSection();
        void ReadElementSection();
        void ReadGlobalSection();
        void ReadCustomSection();

        // Primitive reader
        template <WasmTypes::WasmType type> void ConstNode();
        template <typename T> T ReadConst();
        ExternalKinds ReadExternalKind() { return (ExternalKinds)ReadConst<uint8>(); }
        bool ReadMutableValue();
        const char16* ReadInlineName(uint32& length, uint32& nameLength);
        template<typename LEBType = uint32, uint32 bits = sizeof(LEBType) * 8>
        LEBType LEB128(uint32 &length);
        template<typename LEBType = int32, uint32 bits = sizeof(LEBType) * 8>
        LEBType SLEB128(uint32 &length)
        {
            CompileAssert(LEBType(-1) < LEBType(0));
            return LEB128<LEBType, bits>(length);
        }
        WasmNode ReadInitExpr(bool isOffset = false);
        template<typename SectionLimitType>
        SectionLimitType ReadSectionLimitsBase(uint32 maxInitial, uint32 maxMaximum, const char16* errorMsg);

        void CheckBytesLeft(uint32 bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...) const;
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        const byte* m_start, *m_end, *m_pc, *m_curFuncEnd;
        SectionHeader m_currentSection;
        ReaderState m_funcState;   // func AST level

    private:
        enum
        {
            READER_STATE_UNKNOWN,
            READER_STATE_FUNCTION,
            READER_STATE_MODULE
        } m_readerState;
        Js::WebAssemblyModule* m_module;
#if ENABLE_DEBUG_CONFIG_OPTIONS
        Js::FunctionBody* GetFunctionBody() const;
#endif
#if DBG_DUMP
        typedef JsUtil::BaseHashSet<WasmOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
        OpSet* m_ops;
#endif
    }; // WasmBinaryReader
} // namespace Wasm
#endif // ENABLE_WASM
