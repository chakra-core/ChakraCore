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
    };

    static const unsigned int experimentalVersion = 0xd;

    class WasmBinaryReader : public WasmReaderBase
    {
    public:
        WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule * module, const byte* source, size_t length);

        void InitializeReader();
        bool ReadNextSection(SectionCode nextSection);
        // Fully read the section in the reader. Return true if the section fully read
        bool ProcessCurrentSection();
        virtual void SeekToFunctionBody(FunctionBodyReaderInfo readerInfo) override;
        virtual bool IsCurrentFunctionCompleted() const override;
        virtual WasmOp ReadExpr() override;
#if DBG_DUMP
        void PrintOps();
#endif
        intptr_t GetCurrentOffset() const { return m_pc - m_start; }
    private:
        struct ReaderState
        {
            UINT32 count; // current entry
            UINT32 size;  // number of entries
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
        void ReadMemorySection();
        void ReadSignatures();
        void ReadFunctionsSignatures();
        void ReadFunctionHeaders();
        void ReadExportTable();
        void ReadTableSection();
        void ReadDataSegments();
        void ReadImportEntries();
        void ReadStartFunction();
        void ReadNamesSection();
        void ReadElementSection();
        void ReadGlobalsSection();

        // Primitive reader
        template <WasmTypes::WasmType type> void ConstNode();
        template <typename T> T ReadConst();
        const char16* ReadInlineName(uint32& length, uint32& nameLength);
        const char16* CvtUtf8Str(LPCUTF8 name, uint32 nameLen);
        template<typename MaxAllowedType = UINT>
        MaxAllowedType LEB128(UINT &length, bool sgn = false);
        template<typename MaxAllowedType = INT>
        MaxAllowedType SLEB128(UINT &length);
        WasmNode ReadInitExpr();

        void CheckBytesLeft(UINT bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        uint m_funcNumber;
        const byte* m_start, *m_end, *m_pc, *m_curFuncEnd;
        SectionHeader m_currentSection;
        ReaderState m_funcState;   // func AST level

    private:
        Js::WebAssemblyModule * m_module;
#if DBG_DUMP
        typedef JsUtil::BaseHashSet<WasmOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
        OpSet* m_ops;
#endif
    }; // WasmBinaryReader
} // namespace Wasm
#endif // ENABLE_WASM
