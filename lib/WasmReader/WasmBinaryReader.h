//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    enum FuncDeclFlag
    {
        bFuncDeclName = 0x01,
        bFuncDeclImport = 0x02,
        bFuncDeclLocals = 0x04,
        bFuncDeclExport = 0x08
    };

    struct SectionHeader
    {
        SectionCode code;
        byte* start;
        byte* end;
    };

    static const unsigned int experimentalVersion = 0xb;

    class WasmBinaryReader
    {
    public:
        WasmBinaryReader(ArenaAllocator* alloc, WasmModule* module, byte* source, size_t length);

        void InitializeReader();
        bool ReadNextSection(SectionCode nextSection);
        // Fully read the section in the reader. Return true if the section fully read
        bool ProcessCurrentSection();
        void SeekToFunctionBody(FunctionBodyReaderInfo readerInfo);
        bool IsCurrentFunctionCompleted() const;
        WasmOp ReadExpr();
        WasmOp GetLastOp();
#if DBG_DUMP
        void PrintOps();
#endif

        WasmNode    m_currentNode;
    private:
        struct ReaderState
        {
            UINT32 count; // current entry
            UINT32 size;  // number of entries
        };

        WasmOp ASTNode();

        void CallNode();
        void CallIndirectNode();
        void CallImportNode();
        void BrNode();
        void BrTableNode();
        WasmOp MemNode(WasmOp op);
        void VarNode();

        // Module readers
        void ValidateModuleHeader();
        SectionHeader ReadSectionHeader();
        void ReadMemorySection();
        void ReadSignatures();
        void ReadFunctionsSignatures();
        bool ReadFunctionHeaders();
        void ReadExportTable();
        void ReadIndirectFunctionTable();
        void ReadDataSegments();
        void ReadImportEntries();
        void ReadStartFunction();
        void ReadNamesSection();

        // Primitive reader
        template <WasmTypes::WasmType type> void ConstNode();
        template <typename T> T ReadConst();
        char16* ReadInlineName(uint32& length, uint32& nameLength);
        char16* CvtUtf8Str(LPUTF8 name, uint32 nameLen);
        template<typename MaxAllowedType = UINT>
        MaxAllowedType LEB128(UINT &length, bool sgn = false);
        template<typename MaxAllowedType = INT>
        MaxAllowedType SLEB128(UINT &length);

        void CheckBytesLeft(UINT bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        uint m_funcNumber;
        byte* m_start, *m_end, *m_pc, *m_curFuncEnd;
        SectionHeader m_currentSection;
        WasmOp m_lastOp;
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
