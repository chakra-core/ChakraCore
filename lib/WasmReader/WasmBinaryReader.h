//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
#ifdef ENABLE_WASM
namespace Wasm
{
    namespace Binary
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

            // memory and global types in binary format
            enum MemType {
                bMemI8 = 0,
                bMemU8 = 1,
                bMemI16 = 2,
                bMemU16 = 3,
                bMemI32 = 4,
                bMemU32 = 5,
                bMemI64 = 6,
                bMemU64 = 7,
                bMemF32 = 8,
                bMemF64 = 9,
                bMemLimit
            };

            // for functions and opcodes
            class Signature
            {
            public:
                LocalType *args;
                LocalType retType;
                uint32 argCount;
                Signature();
                Signature(ArenaAllocator *alloc, uint count, ...);
            };

            enum OpSignatureId
            {
#define WASM_SIGNATURE(id, ...) bSig##id,
#include "WasmBinaryOpcodes.h"
                bSigLimit
            };

        } // namespace WasmTypes

        // binary opcodes
        enum WasmBinOp
        {
#define WASM_OPCODE(opname, opcode, token, sig) wb##opname = opcode,
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

        static const unsigned int experimentalVersion = 0xa;

        class WasmBinaryReader : public BaseWasmReader
        {
        public:
            WasmBinaryReader(PageAllocator * alloc, byte* source, size_t length);
            static void Init(Js::ScriptContext *scriptContext);

            virtual void InitializeReader() override;
            virtual bool IsBinaryReader() override;
            virtual bool ReadNextSection(SectionCode nextSection) override;
            virtual bool ProcessCurrentSection() override;
            virtual bool ReadFunctionBodies(FunctionBodyCallback callback, void* callbackdata) override;
            virtual WasmOp ReadFromBlock() override;
            virtual WasmOp ReadFromCall() override;
            virtual WasmOp ReadExpr() override;
#if DBG_DUMP
            void PrintOps();
#endif

        private:
            struct ReaderState
            {
                UINT32 count; // current entry
                UINT32 size;  // number of entries
            };

            Wasm::WasmTypes::WasmType GetWasmType(WasmTypes::LocalType type);
            WasmOp GetWasmToken(WasmBinOp op);
            WasmBinOp ASTNode();

            void ModuleHeader();
            void CallNode();
            void CallIndirectNode();
            void CallImportNode();
            void BlockNode();
            void BrNode();
            void BrTableNode();
            WasmOp MemNode(WasmBinOp op);
            void VarNode();
            template <WasmTypes::LocalType type> void ConstNode();
            // readers
            void ReadMemorySection();
            void ReadSignatures();
            void ReadFunctionsSignatures();
            void ReadExportTable();
            void ReadIndirectFunctionTable();
            void ReadDataSegments();
            void ReadImportEntries();
            void ReadStartFunction();

            void ReadNamesSection();

            char16* ReadInlineName(uint32& length, uint32& nameLength);

            const char* Name(UINT32 offset, UINT &length);
            UINT32 Offset();
            UINT LEB128(UINT &length, bool sgn = false);
            INT SLEB128(UINT &length);
            template <typename T> T ReadConst();
            SectionHeader ReadSectionHeader();

            void CheckBytesLeft(UINT bytesNeeded);
            bool EndOfFunc();
            bool EndOfModule();
            DECLSPEC_NORETURN void ThrowDecodingError(const char16* msg, ...);
            Wasm::WasmTypes::WasmType ReadWasmType(uint32& length);

            ArenaAllocator      m_alloc;
            uint m_funcNumber;
            byte *m_start, *m_end, *m_pc;
            SectionHeader m_currentSection;
            ReaderState m_funcState;   // func AST level

        private:
            static bool isInit;
            static WasmTypes::Signature opSignatureTable[WasmTypes::OpSignatureId::bSigLimit]; // table of opcode signatures
            static WasmTypes::OpSignatureId opSignature[WasmBinOp::wbLimit];                   // opcode -> opcode signature ID
            // maps from binary format to sexpr codes
            // types
            static const Wasm::WasmTypes::WasmType binaryToWasmTypes[];
            // opcodes
            static Wasm::WasmOp binWasmOpToWasmOp[];
#if DBG_DUMP
            typedef JsUtil::BaseHashSet<WasmBinOp, ArenaAllocator, PowerOf2SizePolicy> OpSet;
            OpSet * m_ops;
#endif
        }; // WasmBinaryReader

    }
} // namespace Wasm
#endif // ENABLE_WASM
