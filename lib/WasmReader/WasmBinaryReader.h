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
        const int8 i32 = 0x80 - 0x1;
        const int8 i64 = 0x80 - 0x2;
        const int8 f32 = 0x80 - 0x3;
        const int8 f64 = 0x80 - 0x4;
        const int8 anyfunc = 0x80 - 0x10;
        const int8 func = 0x80 - 0x20;
        const int8 emptyBlock = 0x80 - 0x40;
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

    struct SectionLimits
    {
        uint32 initial;
        uint32 maximum;
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
        virtual WasmOp ReadExpr() override;
        virtual void FunctionEnd() override;
#if DBG_DUMP
        void PrintOps();
#endif
        BinaryLocation GetCurrentLocation() const { return {m_pc - m_start, m_end - m_start}; }
    private:
        struct ReaderState
        {
            uint32 count; // current entry
            size_t size;  // number of entries
        };

        void BlockNode();
        void CallNode();
        void CallIndirectNode();
        void BrNode();
        void BrTableNode();
        void MemNode();
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
        uint8 ReadVarUInt7();
        bool ReadMutableValue();
        const char16* ReadInlineName(uint32& length, uint32& nameLength);
        template<typename MaxAllowedType = uint32>
        MaxAllowedType LEB128(uint32 &length, bool sgn = false);
        template<typename MaxAllowedType = int32>
        MaxAllowedType SLEB128(uint32 &length);
        WasmNode ReadInitExpr(bool isOffset = false);
        SectionLimits ReadSectionLimits(uint32 maxInitial, uint32 maxMaximum, const char16* errorMsg);

        void CheckBytesLeft(uint32 bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        uint32 m_funcNumber;
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
#if DBG_DUMP
        typedef JsUtil::BaseHashSet<WasmOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
        OpSet* m_ops;
#endif
    }; // WasmBinaryReader
} // namespace Wasm
#endif // ENABLE_WASM
