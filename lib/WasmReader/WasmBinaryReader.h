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
            wbLimit
        };


        enum SectionCode
        {
            bSectInvalid        = -1,
            bSectMemory         = 0x00,
            bSectSignatures     = 0x01,
            bSectFunctions      = 0x02,
            bSectGlobals        = 0x03,
            bSectDataSegments   = 0x04,
            bSectFunctionTable  = 0x05,
            bSectEnd            = 0x06, // marks end of module
            bSectLimit
        };

        enum FuncDeclFlag
        {
            bFuncDeclName = 0x01,
            bFuncDeclImport = 0x02,
            bFuncDeclLocals = 0x04,
            bFuncDeclExport = 0x08
        };

        class WasmBinaryReader : public BaseWasmReader
        {
        public:
            WasmBinaryReader(PageAllocator * alloc, byte* source, size_t length, bool trace = false);
            static void Init(Js::ScriptContext *scriptContext);

            virtual bool IsBinaryReader() override;
            virtual WasmOp ReadFromScript() override;
            virtual WasmOp ReadFromModule() override;
            virtual WasmOp ReadFromBlock() override;
            virtual WasmOp ReadFromCall() override;
            virtual WasmOp ReadExpr() override;

        private:
            struct ReaderState
            {
                SectionCode secId;
                UINT count;         // current entry
                UINT size;          // number of entries

            };

            void ResetModuleData();
            Wasm::WasmTypes::WasmType GetWasmType(WasmTypes::LocalType type);
            WasmOp GetWasmToken(WasmBinOp op);
            WasmBinOp ASTNode();

            void CallNode();
            void BlockNode();
            void BrNode();
            void TableSwitchNode();
            void VarNode();
            template <WasmTypes::LocalType type> void ConstNode();
            // readers
            void Signature();
            void FunctionHeader();
            const char* Name(UINT32 offset, UINT &length);
            UINT32 Offset();
            UINT LEB128(UINT &length);
            template <typename T> T ReadConst();




            void CheckBytesLeft(UINT bytesNeeded);
            bool EndOfFunc();
            bool EndOfModule();
            void ThrowDecodingError(const wchar_t* msg);
            void Trace(const wchar_t* msg);

            typedef JsUtil::GrowingArray<WasmTypes::Signature, ArenaAllocator> FuncSignatureTable;
            FuncSignatureTable * m_funcSignatureTable;
            ArenaAllocator      m_alloc;
            uint m_funcNumber;
            byte *m_start, *m_end, *m_pc;
            bool m_trace;
            BVFixed * m_visitedSections;
            ReaderState m_moduleState; // module-level
            ReaderState m_funcState;   // func AST level
            WasmFunctionInfo *  m_funcInfo;


        private:

            static bool isInit;
            static WasmTypes::Signature opSignatureTable[WasmTypes::OpSignatureId::bSigLimit]; // table of opcode signatures
            static WasmTypes::OpSignatureId opSignature[WasmBinOp::wbLimit];                   // opcode -> opcode signature ID
            // maps from binary format to sexpr codes
            // types
            static const Wasm::WasmTypes::WasmType binaryToWasmTypes[];
            // opcodes
            static Wasm::WasmOp binWasmOpToWasmOp[];


        }; // WasmBinaryReader

    }
} // namespace Wasm
#endif // ENABLE_WASM
