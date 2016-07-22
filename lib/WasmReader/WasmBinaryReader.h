//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    namespace WasmTypes
    {
        // based on binary format encoding values
        enum LocalType {
            bAstStmt = 0,  // void
            bAstI32 = 1,
            bAstI64 = 2,
            bAstF32 = 3,
            bAstF64 = 4,
            bAstLimit
        };

    } // namespace WasmTypes

    // binary opcodes
    enum WasmBinOp
    {
#define WASM_OPCODE(opname, opcode, token) wb##opname = opcode,
#include "WasmBinaryOpcodes.h"
        wbFuncEnd,
        wbLimit
    };

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
        WasmBinOp GetLastBinOp() const { return m_lastOp; }
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

        Wasm::WasmTypes::WasmType GetWasmType(WasmTypes::LocalType type);
        WasmOp GetWasmToken(WasmBinOp op);
        WasmBinOp ASTNode();

        void CallNode();
        void CallIndirectNode();
        void CallImportNode();
        void BrNode();
        void BrTableNode();
        WasmOp MemNode(WasmBinOp op);
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
        template <WasmTypes::LocalType type> void ConstNode();
        template <typename T> T ReadConst();
        char16* ReadInlineName(uint32& length, uint32& nameLength);
        char16* CvtUtf8Str(LPUTF8 name, uint32 nameLen);
        UINT LEB128(UINT &length, bool sgn = false);
        INT SLEB128(UINT &length);

        void CheckBytesLeft(UINT bytesNeeded);
        bool EndOfFunc();
        bool EndOfModule();
        DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
        Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

        ArenaAllocator* m_alloc;
        uint m_funcNumber;
        byte* m_start, *m_end, *m_pc, *m_curFuncEnd;
        SectionHeader m_currentSection;
        WasmBinOp m_lastOp;
        ReaderState m_funcState;   // func AST level

    private:
        WasmModule* m_module;
        // types
        static const Wasm::WasmTypes::WasmType binaryToWasmTypes[];
        // opcodes
        static Wasm::WasmOp binWasmOpToWasmOp[];
#if DBG_DUMP
        typedef JsUtil::BaseHashSet<WasmBinOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
        OpSet* m_ops;
#endif
    }; // WasmBinaryReader
} // namespace Wasm
#endif // ENABLE_WASM
