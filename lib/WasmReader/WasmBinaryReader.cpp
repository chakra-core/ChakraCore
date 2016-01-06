//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"


#ifdef ENABLE_WASM

#define TRACE_WASM_DECODER(...) \
    if (m_trace) \
    {\
        Output::Print(__VA_ARGS__); \
        Output::Flush(); \
    } \

namespace Wasm
{
namespace Binary
{

bool WasmBinaryReader::isInit = false;
WasmTypes::Signature WasmBinaryReader::opSignatureTable[WasmTypes::OpSignatureId::bSigLimit]; // table of opcode signatures
WasmTypes::OpSignatureId WasmBinaryReader::opSignature[WasmBinOp::wbLimit];                   // opcode -> opcode signature ID
const Wasm::WasmTypes::WasmType WasmBinaryReader::binaryToWasmTypes[] = { Wasm::WasmTypes::WasmType::Void, Wasm::WasmTypes::WasmType::I32, Wasm::WasmTypes::WasmType::I64, Wasm::WasmTypes::WasmType::F32, Wasm::WasmTypes::WasmType::F64 };
Wasm::WasmOp WasmBinaryReader::binWasmOpToWasmOp[WasmBinOp::wbLimit];

namespace WasmTypes
{
Signature::Signature() : args(nullptr), retType(LocalType::bAstLimit), argCount(0){}

Signature::Signature(ArenaAllocator *alloc, uint count, ...)
{
    va_list arguments;
    va_start(arguments, count);

    assert(count > 0);
    argCount = count - 1;
    retType = va_arg(arguments, LocalType);
    args = AnewArray(alloc, LocalType, argCount);
    for (uint i = 0; i < argCount; i++)
    {
        args[i] = va_arg(arguments, LocalType);
    }
    va_end(arguments);
}
} // namespace WasmTypes

WasmBinaryReader::WasmBinaryReader(PageAllocator * alloc, byte* source, size_t length, bool trace) :
    m_alloc(L"WasmBinaryDecoder", alloc, Js::Throw::OutOfMemory)
{
    m_start = m_pc = source;
    m_end = source + length;
    m_trace = trace;
    ResetModuleData();
}

bool
WasmBinaryReader::IsBinaryReader()
{
    return true;
}

WasmOp
WasmBinaryReader::ReadFromScript()
{
    if (m_pc < m_end)
    {
        return wnMODULE;
    }
    return wnLIMIT;
}

WasmOp
WasmBinaryReader::ReadFromModule()
{
    TRACE_WASM_DECODER(L"Decoding Module");
    SectionCode sectionId;
    UINT length = 0;
    while (1)
    {
        if (EndOfModule())
        {
            return wnLIMIT;
        }

        if (m_moduleState.secId > bSectSignatures && m_moduleState.count < m_moduleState.size)
        {
            // still reading from a valid section
            sectionId = m_moduleState.secId;
        }
        else
        {
            // next section
            sectionId = m_moduleState.secId = (SectionCode)ReadConst<UINT8>();
            m_moduleState.count = 0;
            if (sectionId == bSectMemory)
            {
                m_moduleState.size = 1;
            }
            else
            {
                m_moduleState.size = LEB128(length);
            }
            // mark as visited
            m_visitedSections->Set(sectionId);
        }

        Assert(sectionId >= bSectMemory && sectionId <= bSectEnd);

        switch (sectionId)
        {
        case bSectMemory:
            // TODO: Populate Memory entry info
            ReadConst<UINT8>(); // min mem zie
            ReadConst<UINT8>(); // max mem size
            ReadConst<UINT8>(); // exported ?
            m_moduleState.count += m_moduleState.size;
            break; // This section is not used by bytecode generator for now, stay in decoder

        case bSectSignatures:
            // signatures table
            for (UINT i = 0; i < m_moduleState.size; i++)
            {
                TRACE_WASM_DECODER(L"Signature #%u", i);
                Signature();
            }
            Assert(m_moduleState.count == 0);
            m_moduleState.count += m_moduleState.size;
            break; // This section is not used by bytecode generator, stay in decoder

        case bSectGlobals:
            // TODO: Populate Global entry info
            ReadConst<UINT32>();  // index to string in module
            ReadConst<UINT8>();   // memory type
            ReadConst<UINT8>();   // exported
            m_moduleState.count++;
            return wnGLOBAL;

        case bSectDataSegments:
            // TODO: Populate Data entry info
            ReadConst<UINT32>();  // dest addr in module memory
            ReadConst<UINT32>();  // source offset (?)
            ReadConst<UINT32>();  // size
            ReadConst<UINT8>();  // init
            m_moduleState.count++;
            break; // This section is not used by bytecode generator, stay in decoder

        case bSectFunctions:
            if (!m_visitedSections->Test(bSectSignatures))
            {
                ThrowDecodingError(L"Signatures section missing before function table");
            }
            FunctionHeader();
            m_moduleState.count++;

            return wnFUNC;

        case bSectFunctionTable:
            // TODO: Change to read one entry at a time, and return in bytecode gen in between.
            if (!m_visitedSections->Test(bSectFunctions))
            {
                ThrowDecodingError(L"Function declarations section missing before function table");
            }

            // TODO: Populate entry info
            ReadConst<UINT16>();
            m_moduleState.count++;
            return wnTABLE;

        case bSectEnd:
            // ResetModuleData();

            // One module per file for now. possibly another module in same file ? We will have to skip over global/func names.
            m_pc = m_end; // skip to end, we are done.
            return wnLIMIT;
        }
    }
     return wnLIMIT;
}


WasmOp
WasmBinaryReader::ReadFromBlock()
{
    return GetWasmToken(ASTNode());
}

WasmOp
WasmBinaryReader::ReadFromCall()
{
    return GetWasmToken(ASTNode());
}
WasmOp
WasmBinaryReader::ReadExpr()
{
    return GetWasmToken(ASTNode());
}

/*
Entry point for decoding a node
*/
WasmBinOp
WasmBinaryReader::ASTNode()
{
    if (EndOfFunc())
    {
        // end of AST
        return wbLimit;
    }

    WasmBinOp op = (WasmBinOp)*m_pc++;
    m_funcState.count++;
    switch (op)
    {
    case wbBlock:
    case wbLoop:
        BlockNode();
        break;
    case wbBr:
    case wbBrIf:
        BrNode();
        break;
    case wbReturn:
        break;
    case wbI32Const:
        this->ConstNode<WasmTypes::bAstI32>();
        break;
    case wbF32Const:
        ConstNode<WasmTypes::bAstF32>();
        break;
    case wbF64Const:
        ConstNode<WasmTypes::bAstF64>();
        break;
    case wbSetLocal:
    case wbGetLocal:
    case wbSetGlobal:
    case wbGetGlobal:
        VarNode();
        break;
    case wbIf:
        // no node attributes
        break;

#define WASM_SIMPLE_OPCODE(opname, opcode, token, sig) \
    case wb##opname: \
    m_currentNode.op = GetWasmToken(op); \
    break;
#include "WasmBinaryOpcodes.h"

    default:
        Assert(UNREACHED);
    }

    return op;
}

// control flow
void
WasmBinaryReader::BlockNode()
{
    m_currentNode.block.count = ReadConst<UINT8>();
    m_funcState.count++;
}

void
WasmBinaryReader::BrNode()
{
    m_currentNode.br.depth = ReadConst<UINT8>();
    m_funcState.count++;
}

// Locals/Globals
void
WasmBinaryReader::VarNode()
{
    UINT length;
    m_currentNode.var.num = LEB128(length);
    m_funcState.count += length;

}

// Const
template <WasmTypes::LocalType localType>
void WasmBinaryReader::ConstNode()
{
    switch (localType)
    {
    case WasmTypes::bAstI32:
        m_currentNode.cnst.i32 = ReadConst<INT32>();
        m_funcState.count += sizeof(INT32);
        break;
    case WasmTypes::bAstF32:
        m_currentNode.cnst.f32 = ReadConst<float>();
        m_funcState.count += sizeof(float);
        break;
    case WasmTypes::bAstF64:
        m_currentNode.cnst.f64 = ReadConst<double>();
        m_funcState.count += sizeof(double);
        break;
    }
}

void
WasmBinaryReader::ResetModuleData()
{

    m_visitedSections = BVFixed::New<ArenaAllocator>(bSectLimit + 1, &m_alloc);
    m_funcSignatureTable = Anew(&m_alloc, FuncSignatureTable, &m_alloc, 0);
    m_moduleState.count = 0;
    m_moduleState.size = 0;
    m_moduleState.secId = bSectInvalid;
}

Wasm::WasmTypes::WasmType
WasmBinaryReader::GetWasmType(WasmTypes::LocalType type)
{
    return binaryToWasmTypes[type];
}

WasmOp
WasmBinaryReader::GetWasmToken(WasmBinOp op)
{
    Assert(op <= WasmBinOp::wbLimit);
    return binWasmOpToWasmOp[op];
}

bool
WasmBinaryReader::EndOfFunc()
{
    return m_funcState.count == m_funcState.size;
}

bool
WasmBinaryReader::EndOfModule()
{
    return (m_pc == m_end);
}
// readers
void
WasmBinaryReader::Signature()
{
    WasmTypes::Signature sig;
    sig.args = nullptr;
    sig.argCount = (UINT)ReadConst<UINT8>();
    sig.retType = (WasmTypes::LocalType) ReadConst<UINT8>();
    if (sig.argCount)
    {
        sig.args = AnewArray(&m_alloc, WasmTypes::LocalType, sig.argCount);
    }

    for (UINT i = 0; i < sig.argCount; i++)
    {
        sig.args[i] = (WasmTypes::LocalType)ReadConst<UINT8>();
    }
    m_funcSignatureTable->Add(sig);
}

void
WasmBinaryReader::FunctionHeader()
{
    UINT8 flags;
    UINT16 sigId;
    UINT32 nameOffset, nameLen;
    UINT16 i32Count = 0, i64Count = 0, f32Count = 0, f64Count = 0;
    const char* funcName = nullptr;
    LPCUTF8 utf8FuncName = nullptr;
    WasmTypes::Signature sig;

    Assert(m_moduleState.secId == bSectFunctions && m_moduleState.count < m_moduleState.size);
    m_funcInfo = Anew(&m_alloc, WasmFunctionInfo, &m_alloc);
    m_currentNode.func.info = m_funcInfo;

    m_funcInfo->SetNumber(m_moduleState.count);
    flags = ReadConst<UINT8>();
    sigId = ReadConst<UINT16>();
    if (sigId >= m_funcSignatureTable->Count())
    {
        ThrowDecodingError(L"Function signature is out of bound");
    }
    if (flags & bFuncDeclName)
    {
        nameOffset = Offset();
        // read function name
        funcName = Name(nameOffset, nameLen);
        AssertMsg(nameLen > 0, "Invalid function name length");
        utf8FuncName = AnewArray(&m_alloc, CUTF8, nameLen);
        strcpy_s((char*)utf8FuncName, nameLen, funcName);
        m_funcInfo->SetName(utf8FuncName);
    }

    if (flags & bFuncDeclImport)
    {
        // imported function, no more info to decode.
        m_funcInfo->SetImported(true);
        return;
    }

    m_funcInfo->SetExported(flags & bFuncDeclExport ? true : false);

    // params
    sig = m_funcSignatureTable->GetBuffer()[sigId];
    for (UINT i = 0; i < sig.argCount; i++)
    {
        Assert(sig.args[i] >= WasmTypes::bAstStmt && sig.args[i] < WasmTypes::bAstLimit);
        // map encoded type id to WasmType
        m_funcInfo->AddParam(GetWasmType(sig.args[i]));
    }

    m_funcInfo->SetResultType(GetWasmType(sig.retType));

    // locals
    if (flags & bFuncDeclLocals)
    {
        // has locals
        i32Count = ReadConst<UINT16>();
        i64Count = ReadConst<UINT16>();
        f32Count = ReadConst<UINT16>();
        f64Count = ReadConst<UINT16>();

        for (UINT i = 0; i < i32Count; i ++)
        {
            m_funcInfo->AddLocal(Wasm::WasmTypes::I32);
        }
        for (UINT i = 0; i < i64Count; i++)
        {
            m_funcInfo->AddLocal(Wasm::WasmTypes::I64);
        }
        for (UINT i = 0; i < f32Count; i++)
        {
            m_funcInfo->AddLocal(Wasm::WasmTypes::F32);
        }
        for (UINT i = 0; i < f64Count; i++)
        {
            m_funcInfo->AddLocal(Wasm::WasmTypes::F64);
        }
    }

    // Reset func state
    m_funcState.count = 0;
    m_funcState.size = ReadConst<UINT16>(); // AST Size in bytes
    CheckBytesLeft(m_funcState.size);

    TRACE_WASM_DECODER(L"Function header: flags = %x, sig = %u, i32 = %u, i64 = %u, f32 = %u, f64 = %u, size = %u", flags, sigId, i32Count, i64Count, f32Count, f64Count);
}

const char *
WasmBinaryReader::Name(UINT offset, UINT &length)
{
    BYTE* str = m_start + offset;
    length = 0;
    if (offset == 0)
    {
        return "<?>";
    }
    // validate string and get length
    do
    {
        if (str >= m_end)
        {
            ThrowDecodingError(L"Offset is out of range");
        }
        length++;
    } while (*str++);

    return (const char*)(m_start + offset);

}
UINT
WasmBinaryReader::Offset()
{
    UINT32 offset = ReadConst<UINT32>();
    if (offset > (UINT)(m_end - m_start))
    {
        ThrowDecodingError(L"Offset is out of range");
    }
    return offset;
}

UINT
WasmBinaryReader::LEB128(UINT &length)
{
    // LEB128 needs at least one byte
    CheckBytesLeft(1);

    UINT result = 0, shamt = 0;;
    byte b;
    length = 1;
    for (UINT i = 0; i < 5; i++, length++)
    {
        // 5 bytes at most
        b = *m_pc++;
        result = result | ((b & 0x7f) << shamt);
        if ((b & 0x80) == 0)
            break;
        shamt += 7;
    }

    if (b & 0x80)
    {
        ThrowDecodingError(L"Invalid LEB128 format");
    }

    TRACE_WASM_DECODER(L"Binary decoder: LEB128 value = %u, length = %u", result, length);
    return result;
}

template <typename T>
T WasmBinaryReader::ReadConst()
{
    CheckBytesLeft(sizeof(T));
    T value = (T)(*m_pc);
    m_pc += sizeof(T);

    return value;
}

void
WasmBinaryReader::CheckBytesLeft(UINT bytesNeeded)
{
    UINT bytesLeft = m_end - m_pc;
    if ( bytesNeeded > bytesLeft)
    {
        Output::Print(L"Out of file: Needed: %d, Left: %d", bytesNeeded, bytesLeft);
        ThrowDecodingError(L"Out of file.");
    }
}

void
WasmBinaryReader::ThrowDecodingError(const wchar_t* msg)
{
    Output::Print(L"Binary decoding failed: %s", msg);
    Output::Flush();
    Js::Throw::InternalError();
}

void
WasmBinaryReader::Init(Js::ScriptContext * scriptContext)
{
    if (isInit)
    {
        return;
    }
    ArenaAllocator *alloc =  scriptContext->GetThreadContext()->GetThreadAlloc();
    // initialize Op Signature table
    {

#define WASM_SIGNATURE(id, count, ...) \
    AssertMsg(count >= 0 && count <= 3, "Invalid count for op signature"); \
    AssertMsg(WasmTypes::bSig##id >= 0 && WasmTypes::bSig##id < WasmTypes::bSigLimit, "Invalid signature ID for op"); \
    opSignatureTable[WasmTypes::bSig##id] = WasmTypes::Signature(alloc, count, __VA_ARGS__);

#include "WasmBinaryOpcodes.h"
    }

    // initialize opcode to op signature map
    {
#define WASM_OPCODE(opname, opcode, token, sig) \
    opSignature[wb##opname] = WasmTypes::bSig##sig;

#include "WasmBinaryOpcodes.h"
    }

    // initialize binary opcodes to SExpr tokens  map
    {
#define WASM_OPCODE(opname, opcode, token, sig) \
    binWasmOpToWasmOp[WasmBinOp::wb##opname] = Wasm::WasmOp::wn##token;
#include "WasmBinaryOpcodes.h"
    binWasmOpToWasmOp[WasmBinOp::wbLimit] = Wasm::WasmOp::wnLIMIT;
    }

    isInit = true;
}

} // namespace Binary
} // namespace Wasm


#undef TRACE_WASM_DECODER

#endif // ENABLE_WASM
