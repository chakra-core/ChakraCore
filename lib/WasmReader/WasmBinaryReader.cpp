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
        Output::Print(L"\n"); \
        Output::Flush(); \
    } \

namespace Wasm
{
namespace Binary
{

bool WasmBinaryReader::isInit = false;
bool WasmBinaryReader::seenModuleHeader = false;
WasmTypes::Signature WasmBinaryReader::opSignatureTable[WasmTypes::OpSignatureId::bSigLimit]; // table of opcode signatures
WasmTypes::OpSignatureId WasmBinaryReader::opSignature[WasmBinOp::wbLimit];                   // opcode -> opcode signature ID
const Wasm::WasmTypes::WasmType WasmBinaryReader::binaryToWasmTypes[] = { Wasm::WasmTypes::WasmType::Void, Wasm::WasmTypes::WasmType::I32, Wasm::WasmTypes::WasmType::I64, Wasm::WasmTypes::WasmType::F32, Wasm::WasmTypes::WasmType::F64 };
Wasm::WasmOp WasmBinaryReader::binWasmOpToWasmOp[WasmBinOp::wbLimit + 1];

namespace WasmTypes
{
Signature::Signature() : args(nullptr), retType(LocalType::bAstLimit), argCount(0){}

Signature::Signature(ArenaAllocator *alloc, uint count, ...)
{
    va_list arguments;
    va_start(arguments, count);

    Assert(count > 0);
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
    m_moduleInfo = Anew(&m_alloc, ModuleInfo, &m_alloc);

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
WasmBinaryReader::ReadFromModule()
{
    TRACE_WASM_DECODER(L"Decoding Module");

    SectionCode sectionId;
    UINT length = 0;

    ModuleHeader();

    while (!EndOfModule())
    {
        if (m_moduleState.secId > bSectSignatures && m_moduleState.count < m_moduleState.size)
        {
            // still reading from a valid section
            sectionId = m_moduleState.secId;
        }
        else
        {
            // TODO: Should check that previous section, if any, was of the correct
            // length by comparing against m_moduleState.byteLen.

            // next section
            sectionId = m_moduleState.secId = SectionHeader();
            if (sectionId == bSectMemory || sectionId == bSectEnd)
            {
                m_moduleState.size = 1;
            }
            else
            {
                m_moduleState.size = LEB128(length);
                if (sectionId == bSectFunSigs)
                {
                    m_moduleInfo->SetFunctionCount(m_moduleState.size);
                }
            }
            // mark as visited
            m_visitedSections->Set(sectionId);
        }

        // TODO: Skip unknown sections.
        Assert(sectionId >= bSectMemory && sectionId < bSectLimit);

        switch (sectionId)
        {
        case bSectMemory:
        {
            ReadMemorySection();
            m_moduleState.count += m_moduleState.size;
            break; // This section is not used by bytecode generator for now, stay in decoder
        }
        case bSectSignatures:
            // signatures table
            for (UINT32 i = 0; i < m_moduleState.size; i++)
            {
                TRACE_WASM_DECODER(L"Signature #%u", i);
                Signature();
            }
            Assert(m_moduleState.count == 0);
            m_moduleState.count += m_moduleState.size;
            break; // This section is not used by bytecode generator, stay in decoder

        case bSectGlobals:
            ThrowDecodingError(L"Nonstandard global section not supported!");

        case bSectDataSegments:
            // TODO: Populate Data entry info
            ReadConst<UINT32>();  // dest addr in module memory
            ReadConst<UINT32>();  // source offset (?)
            ReadConst<UINT32>();  // size
            ReadConst<UINT8>();  // init
            m_moduleState.count++;
            break; // This section is not used by bytecode generator, stay in decoder

        case bSectFunSigs:
            if (!m_visitedSections->Test(bSectSignatures))
            {
                ThrowDecodingError(L"Signatures section missing before function table");
            }
            FunctionHeader();
            m_moduleState.count++;
            break;

        case bSectFunBodies:
            if (!m_visitedSections->Test(bSectFunSigs))
            {
                ThrowDecodingError(L"Function signatures section missing before function bodies");
            }
            if (m_moduleInfo->GetFunctionCount() != m_moduleState.size)
            {
                ThrowDecodingError(L"Function signatures and function bodies count mismatch");
            }
            FunctionBodyHeader();
            m_moduleState.count++;
            return wnFUNC;

        case bSectIndirectFunctionTable:
            if (!m_visitedSections->Test(bSectFunSigs))
            {
                ThrowDecodingError(L"Function signatures section missing before indirect function table");
            }

            for (UINT32 i = 0; i < m_moduleState.size; i++)
            {
                m_moduleInfo->AddIndirectFunctionIndex(ReadConst<UINT16>());
            }
            m_moduleState.count += m_moduleState.size;
            break;

        case bSectEnd:
            // One module per file. We will have to skip over func names and data segments.
            m_pc = m_end; // skip to end, we are done.
            return wnLIMIT;
        }
    }
     return wnLIMIT;
}

SectionCode
WasmBinaryReader::SectionHeader()
{
    UINT len;
    UINT32 sectionSize;
    UINT32 idSize;

    sectionSize = LEB128(len);
    idSize = LEB128(len);
    const char *sectionName = (char*)(m_pc);
    m_pc += idSize;
    m_moduleState.count = 0;
    m_moduleState.byteLen = sectionSize;

    auto cmp = [&sectionName, &idSize](const char* n) { return !memcmp(n, sectionName, idSize); };
    if (cmp("memory")) return bSectMemory;
    if (cmp("signatures")) return bSectSignatures;
    if (cmp("import_table")) return bSectImportTable;
    if (cmp("function_signatures")) return bSectFunSigs;
    if (cmp("function_bodies")) return bSectFunBodies;
    if (cmp("export_table")) return bSectExportTable;
    if (cmp("start_function")) return bSectStartFunction;
    if (cmp("data_segments")) return bSectDataSegments;
    if (cmp("function_table")) return bSectIndirectFunctionTable;
    if (cmp("end")) return bSectEnd;

    return bSectInvalid;
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
    // [b-gekua] REVIEW: It would be best to figure out how to unify
    // SExprParser and WasmBinaryReader's interface for those Nodes
    // that are repeatedly called (scoping construct) such as Blocks and Calls.
    // SExprParser uses an interface such that ReadFromX() will be
    // repeatedly called until we reach the end of the scope (at which
    // point ReadFromX() should return a wnLIMIT to signal this). This
    // Would eliminate a lot of the special casing in WasmBytecodeGenerator's
    // EmitX() functions. The gotcha is that this may involve adding
    // state to WasmBinaryReader to indicate how far in the scope we are.
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
    case wbCall:
        CallNode();
        break;
    case wbBr:
    case wbBrIf:
        BrNode();
        break;
    case wbBrTable:
        TableSwitchNode();
        break;
    case wbReturn:
        // Watch out for optional implicit block
        // (non-void return expression)
        if (!EndOfFunc())
            m_currentNode.opt.exists = true;
        break;
    case wbI8Const:
        m_currentNode.cnst.i32 = ReadConst<INT8>();
        m_funcState.count += sizeof(INT8);
        break;
    case wbI32Const:
        ConstNode<WasmTypes::bAstI32>();
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
    case wbIfElse:
    case wbIf:
        // no node attributes
        break;
    case wbNop:
        break;
#define WASM_MEM_OPCODE(opname, opcode, token, sig) \
    case wb##opname: \
    m_currentNode.op = MemNode(op); \
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

void
WasmBinaryReader::ModuleHeader()
{
    if (!seenModuleHeader)
    {
        if (ReadConst<UINT32>() != 0x6d736100)
        {
            ThrowDecodingError(L"Malformed WASM module header!");
        }

        if (ReadConst<UINT32>() != 10)
        {
            ThrowDecodingError(L"Invalid WASM version!");
        }
        seenModuleHeader = true;
    }
}

void
WasmBinaryReader::CallNode()
{
    UINT length = 0;
    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    if (funcNum >= m_moduleInfo->GetSignatureCount())
    {
        ThrowDecodingError(L"Function signature is out of bound");
    }
    m_currentNode.var.num = funcNum;
}

// control flow
void
WasmBinaryReader::BlockNode()
{
    UINT len = 0;
    m_currentNode.block.count = LEB128(len);
    m_funcState.count += len;
}

void
WasmBinaryReader::BrNode()
{
    UINT len = 0;
    m_currentNode.br.depth = LEB128(len);
    m_funcState.count += len;
    // TODO: binary encoding doesn't yet support br yielding value
    m_currentNode.br.hasSubExpr = false;
}

void
WasmBinaryReader::TableSwitchNode()
{
    UINT len = 0;
    m_currentNode.tableswitch.numCases = LEB128(len);
    m_funcState.count += len;
    m_currentNode.tableswitch.numEntries = LEB128(len);
    m_funcState.count += len;
    m_currentNode.tableswitch.jumpTable = AnewArray(&m_alloc, UINT16, m_currentNode.tableswitch.numEntries);
    // TODO tableswitch to BR_TABLE
    for (UINT32 i = 0; i < m_currentNode.tableswitch.numEntries; i++)
    {
        m_currentNode.tableswitch.jumpTable[i] = ReadConst<UINT16>();
        m_funcState.count += sizeof(UINT16);
    }
}

WasmOp
WasmBinaryReader::MemNode(WasmBinOp op)
{
    // Read memory access byte
    m_currentNode.mem.alignment = ReadConst<UINT8>();
    m_funcState.count++;
    // If offset bit is set, read memory_offset
    if (m_currentNode.mem.alignment & 0x08)
    {
        UINT length = 0;
        m_currentNode.mem.offset = LEB128(length);
        m_funcState.count += length;
    }
    else {
        m_currentNode.mem.offset = 0;
    }
    return GetWasmToken(op);
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
    UINT len = 0;
    switch (localType)
    {
    case WasmTypes::bAstI32:
        m_currentNode.cnst.i32 = SLEB128(len);
        m_funcState.count += len;
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
WasmBinaryReader::ReadMemorySection()
{
    UINT length = 0;
    UINT32 minPage = LEB128(length);
    UINT32 maxPage = LEB128(length);
    bool exported = ReadConst<UINT8>() != FALSE;
    m_moduleInfo->InitializeMemory(minPage, maxPage, exported);
}

void
WasmBinaryReader::Signature()
{
    UINT len = 0;
    WasmSignature * sig = Anew(&m_alloc, WasmSignature, &m_alloc);

    // TODO: use param count to create fixed size array
    UINT32 paramCount = LEB128(len);

    sig->SetResultType(GetWasmType((WasmTypes::LocalType)ReadConst<UINT8>()));

    for (UINT32 i = 0; i < paramCount; i++)
    {
        sig->AddParam(GetWasmType((WasmTypes::LocalType)ReadConst<UINT8>()));
    }
    

    m_moduleInfo->AddSignature(sig);
}

void
WasmBinaryReader::FunctionHeader()
{
    // TODO: Split this up into FunSig and Names sections
    UINT len = 0;
    UINT8 flags;
    UINT32 sigId;
    UINT32 nameOffset, nameLen;
    const char* funcName = nullptr;
    LPCUTF8 utf8FuncName = nullptr;
    WasmSignature * sig;

    Assert(m_moduleState.secId == bSectFunSigs && m_moduleState.count < m_moduleState.size);
    m_funcInfo = Anew(&m_alloc, WasmFunctionInfo, &m_alloc);
    m_moduleInfo->AddFunSig(m_funcInfo);

    flags = ReadConst<UINT8>();
    sigId = LEB128(len);

    if (sigId >= m_moduleInfo->GetSignatureCount())
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

    if ((m_funcInfo->Exported() || m_funcInfo->Imported()) && m_funcInfo->GetName() == nullptr)
    {
        ThrowDecodingError(L"Imports and exports must be named!");
    }
    // params
    sig = m_moduleInfo->GetSignature(sigId);
    m_funcInfo->SetSignature(sig);

    TRACE_WASM_DECODER(L"Function header: flags = %x, sig = %u, size = %u", flags, sigId, m_funcState.size);
}

void
WasmBinaryReader::FunctionBodyHeader()
{
    UINT32 i32Count = 0, i64Count = 0, f32Count = 0, f64Count = 0;
    UINT len = 0;

    m_currentNode.func.info = m_moduleInfo->GetFunSig(m_moduleState.count);

    // Reset func state
    m_funcState.count = 0;
    m_funcState.size = LEB128(len); // function body size in bytes including AST
    CheckBytesLeft(m_funcState.size);

    UINT32 entryCount = LEB128(len);
    m_funcState.size += len;

    // locals
    for (UINT32 i = 0; i < entryCount; i++)
    {
        UINT32 count = LEB128(len);
        m_funcState.size += len;
        UINT8 type = ReadConst<UINT8>();
        m_funcState.size++;

        switch (type)
        {
        case Wasm::WasmTypes::WasmType::I32:
            i32Count += count;
            break;
        case Wasm::WasmTypes::WasmType::I64:
            i64Count += count;
            break;
        case Wasm::WasmTypes::WasmType::F32:
            f32Count += count;
            break;
        case Wasm::WasmTypes::WasmType::F64:
            f64Count += count;
            break;
        default:
            ThrowDecodingError(L"Unexpected local type");
        }
    }

    for (UINT32 i = 0; i < i32Count; i++)
    {
        m_funcInfo->AddLocal(Wasm::WasmTypes::I32);
    }
    for (UINT32 i = 0; i < i64Count; i++)
    {
        m_funcInfo->AddLocal(Wasm::WasmTypes::I64);
    }
    for (UINT32 i = 0; i < f32Count; i++)
    {
        m_funcInfo->AddLocal(Wasm::WasmTypes::F32);
    }
    for (UINT32 i = 0; i < f64Count; i++)
    {
        m_funcInfo->AddLocal(Wasm::WasmTypes::F64);
    }

    TRACE_WASM_DECODER(L"Function body header: i32 = %u, i64 = %u, f32 = %u, f64 = %u", i32Count, i64Count, f32Count, f64Count);
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
    UINT len = 0;
    UINT32 offset = LEB128(len);
    if (offset > (UINT)(m_end - m_start))
    {
        ThrowDecodingError(L"Offset is out of range");
    }
    return offset;
}

UINT
WasmBinaryReader::LEB128(UINT &length, bool sgn)
{
    UINT result = 0;
    UINT shamt = 0;
    byte b;

    // LEB128 needs at least one byte
    CheckBytesLeft(1);

    for (UINT i = 0; i < 5; i++, length++)
    {
        // 5 bytes at most
        b = *m_pc++;
        result = result | ((b & 0x7f) << shamt);
        if (sgn)
        {
            shamt += 7;
            if ((b & 0x80) == 0)
                break;
        }
        else
        {
            if ((b & 0x80) == 0)
                break;
            shamt += 7;
        }
    }

    if (b & 0x80)
    {
        ThrowDecodingError(L"Invalid LEB128 format");
    }

    if (sgn && (shamt + 7 < sizeof(INT) * 8) && (0x40 & b))
    {
        result |= -(1 << shamt);
    }

    if (!sgn)
    {
        TRACE_WASM_DECODER(L"Binary decoder: LEB128 value = %u, length = %u", result, length);
    }

    return result;
}

// Signed LEB128
INT
WasmBinaryReader::SLEB128(UINT &length)
{
    INT result = LEB128(length, true);

    TRACE_WASM_DECODER(L"Binary decoder: LEB128 value = %d, length = %u", result, length);
    return result;
}

template <typename T>
T WasmBinaryReader::ReadConst()
{
    CheckBytesLeft(sizeof(T));
    T value = *((T*)m_pc);
    m_pc += sizeof(T);

    return value;
}

void
WasmBinaryReader::CheckBytesLeft(UINT bytesNeeded)
{
    UINT bytesLeft = (UINT)(m_end - m_pc);
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
