//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    struct SectionHeader
    {
        SectionCode code;
        byte* start;
        byte* end;
    };

    static const unsigned int experimentalVersion = 0xc;

    class WasmBinaryReader : public WasmReaderBase
    {
    public:
        WasmBinaryReader(ArenaAllocator* alloc, WasmModule* module, byte* source, size_t length);

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
        bool ReadFunctionHeaders();
        void ReadExportTable();
        void ReadTableSection();
        void ReadDataSegments();
        void ReadImportEntries();
        void ReadStartFunction();
        void ReadNamesSection();
        void ReadElementSection();

        // Primitive reader
        template <WasmTypes::WasmType type> void ConstNode();
        template <typename T> T ReadConst();
        char16* ReadInlineName(uint32& length, uint32& nameLength);
        char16* CvtUtf8Str(LPUTF8 name, uint32 nameLen);
        UINT LEB128(UINT &length, bool sgn = false);
        INT SLEB128(UINT &length);
        WasmNode ReadInitExpr();

        void CheckBytesLeft(UINT bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        uint m_funcNumber;
        byte* m_start, *m_end, *m_pc, *m_curFuncEnd;
        SectionHeader m_currentSection;
        ReaderState m_funcState;   // func AST level

    private:
        WasmModule* m_module;
#if DBG_DUMP
        typedef JsUtil::BaseHashSet<WasmOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
        OpSet* m_ops;
#endif
    }; // WasmBinaryReader
} // namespace Wasm
#endif // ENABLE_WASM
