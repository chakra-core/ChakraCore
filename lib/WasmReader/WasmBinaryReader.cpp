//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"


#ifdef ENABLE_WASM
#define TRACE_WASM_DECODER(...) \
    if (PHASE_TRACE1(Js::WasmReaderPhase)) \
    {\
        Output::Print(__VA_ARGS__); \
        Output::Print(L"\n"); \
        Output::Flush(); \
    } \

#define TRACE_WASM_DECODER_PHASE(phase, ...) \
    if (PHASE_TRACE1(Js::phase##Phase)) \
    {\
        Output::Print(__VA_ARGS__); \
        Output::Print(L"\n"); \
        Output::Flush(); \
    } \

namespace Wasm
{

#define WASM_SECTION(name, id, flag, precedent, subsequent) flag,
SectionFlag BaseWasmReader::sectionFlags[bSectLimit] = {
#include "WasmSections.h"
};

#define WASM_SECTION(name, id, flag, precedent, subsequent) bSect ## precedent,
const SectionCode BaseWasmReader::sectionPrecedences[bSectLimit] = {
#include "WasmSections.h"
};

#define WASM_SECTION(name, id, flag, precedent, subsequent) bSect ## subsequent,
const SectionCode BaseWasmReader::sectionSubsequents[bSectLimit] = {
#include "WasmSections.h"
};

#define WASM_SECTION(name, id, flag, precedent, subsequent) L#name,
wchar_t* BaseWasmReader::sectionNames[bSectLimit] = {
#include "WasmSections.h"
};

#define WASM_SECTION(name, id, flag, precedent, subsequent) id,
char* BaseWasmReader::sectionIds[bSectLimit] = {
#include "WasmSections.h"
};

namespace Binary
{

bool WasmBinaryReader::isInit = false;
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

WasmBinaryReader::WasmBinaryReader(PageAllocator * alloc, byte* source, size_t length) :
    m_alloc(L"WasmBinaryDecoder", alloc, Js::Throw::OutOfMemory)
{
    m_moduleInfo = Anew(&m_alloc, ModuleInfo, &m_alloc);

    m_start = m_pc = source;
    m_end = source + length;
    ResetModuleData();
}

void WasmBinaryReader::InitializeReader()
{
    ModuleHeader();
#if DBG
    if (PHASE_TRACE1(Js::WasmReaderPhase))
    {
        byte* startModule = m_pc;

        bool doRead = true;
        SectionCode prevSect = bSectInvalid;
        while (doRead)
        {
            SectionHeader secHeader = ReadSectionHeader();
            if (secHeader.code <= prevSect)
            {
                TRACE_WASM_DECODER(L"Unknown section order");
            }
            prevSect = secHeader.code;
            // skip the section
            m_pc = secHeader.end;
            doRead = !EndOfModule() && secHeader.code != bSectEnd;
        }
        m_pc = startModule;
    }
#endif
}

void
WasmBinaryReader::ThrowDecodingError(const wchar_t* msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    Output::Print(L"Binary decoding failed: ");
    Output::VPrint(msg, argptr);
    Output::Flush();
    Js::Throw::InternalError();
}

bool
WasmBinaryReader::IsBinaryReader()
{
    return true;
}

bool
WasmBinaryReader::ReadNextSection(SectionCode nextSection)
{
    if (EndOfModule() || sectionFlags[nextSection] == fSectIgnore)
    {
        return false;
    }

    SectionHeader secHeader = ReadSectionHeader();
    if (secHeader.code == bSectInvalid || sectionFlags[secHeader.code] == fSectIgnore)
    {
        TRACE_WASM_DECODER(L"Ignore this section");
        m_pc = secHeader.end;
        return ReadNextSection(nextSection);
    }
    if (secHeader.code < nextSection)
    {
        ThrowDecodingError(L"Invalid Section %s", secHeader.code);
    }

    if (secHeader.code != nextSection)
    {
        TRACE_WASM_DECODER(L"The current section is not the one we are looking for");
        // We know about this section, but it's not the one we're looking for
        m_pc = secHeader.start;
        return false;
    }
    m_moduleState.secHeader = secHeader;
    m_moduleState.count = 0;
    m_moduleState.size = 0;
    return true;
}

ProcessSectionResult
WasmBinaryReader::ProcessSection(SectionCode sectionId, bool isEntry /*= true*/)
{
    UINT length = 0;
    switch (sectionId)
    {
    case bSectMemory:
    {
        ReadMemorySection();
        m_moduleState.count += m_moduleState.size;
        break; // This section is not used by bytecode generator for now, stay in decoder
    }
    case bSectSignatures:
    {
        const uint32 count = LEB128(length);
        // signatures table
        for (UINT32 i = 0; i < count; i++)
        {
            TRACE_WASM_DECODER(L"Signature #%u", i);
            Signature();
        }
        break; // This section is not used by bytecode generator, stay in decoder
    }
    case bSectDataSegments:
        // TODO: Populate Data entry info
        ReadConst<UINT32>();  // dest addr in module memory
        ReadConst<UINT32>();  // source offset (?)
        ReadConst<UINT32>();  // size
        ReadConst<UINT8>();  // init
        m_moduleState.count++;
        break; // This section is not used by bytecode generator, stay in decoder

    case bSectFunctionSignatures:
        ReadFunctionsSignatures();
        break;
    case bSectExportTable:
        ReadExportTable();
        break;
    case bSectFunctionBodies:
        if (isEntry)
        {
            uint32 entries = LEB128(length);
            if (entries != m_moduleInfo->GetFunctionCount())
            {
                ThrowDecodingError(L"Function signatures and function bodies count mismatch");
            }
            m_moduleState.size = entries;
            m_moduleState.count = 0;
        }
        FunctionBodyHeader();
        ++m_moduleState.count;
        break;
    case bSectIndirectFunctionTable:
        for (UINT32 i = 0; i < m_moduleState.size; i++)
        {
            m_moduleInfo->AddIndirectFunctionIndex(ReadConst<UINT16>());
        }
        m_moduleState.count += m_moduleState.size;
        break;

    case bSectEnd:
        // One module per file. We will have to skip over func names and data segments.
        m_pc = m_end; // skip to end, we are done.
    default:
        Assert(false);
        return psrInvalid;
    }

    if (m_moduleState.count < m_moduleState.size)
    {
        if (EndOfModule() || m_pc >= m_moduleState.secHeader.end)
        {
            return psrInvalid;
        }
        return psrContinue;
    }
    if (m_pc != m_moduleState.secHeader.end)
    {
        return psrInvalid;
    }
    return psrEnd;
}

SectionHeader
WasmBinaryReader::ReadSectionHeader()
{
    UINT len = 0;
    UINT32 sectionSize;
    UINT32 idSize;

    SectionHeader header;
    header.start = m_pc;
    sectionSize = LEB128(len);
    header.end = m_pc + sectionSize;
    CheckBytesLeft(sectionSize);

    idSize = LEB128(len);
    if (sectionSize < idSize + len)
    {
        ThrowDecodingError(L"Invalid section size");
    }
    const char *sectionName = (char*)(m_pc);
    m_pc += idSize;

    for (int i = 0; i < bSectLimit ; i++)
    {
        if (!memcmp(sectionIds[i], sectionName, idSize))
        {
            header.code = (SectionCode)i;
            break;
        }
    }

#if DBG
    Assert(idSize < 64);
    wchar_t buf[64];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, buf, idSize + 1, sectionName, _TRUNCATE);
    buf[idSize] = 0;
    TRACE_WASM_DECODER(L"Section Header: %s, length = %u (0x%x)", buf, sectionSize, sectionSize);
#endif
    return header;
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
    uint32 magicNumber = ReadConst<UINT32>();
    uint32 version = ReadConst<UINT32>();
    TRACE_WASM_DECODER(L"Module Header: Magic 0x%x, Version %u", magicNumber, version);
    if (magicNumber != 0x6d736100)
    {
        ThrowDecodingError(L"Malformed WASM module header!");
    }

    if (version != 10)
    {
        ThrowDecodingError(L"Invalid WASM version!");
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
    m_moduleState.count = 0;
    m_moduleState.size = 0;
    m_moduleState.secHeader.code = bSectInvalid;
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
    return (m_pc >= m_end);
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
WasmBinaryReader::ReadFunctionsSignatures()
{
    UINT len = 0;
    uint32 nFunctions = LEB128(len);
    m_moduleInfo->AllocateFunctions(nFunctions);

    for (uint32 iFunc = 0; iFunc < nFunctions; iFunc++)
    {
        uint32 sigIndex = LEB128(len);
        if (sigIndex >= m_moduleInfo->GetSignatureCount())
        {
            ThrowDecodingError(L"Function signature is out of bound");
        }
        WasmFunctionInfo* newFunction = Anew(&m_alloc, WasmFunctionInfo, &m_alloc);
        WasmSignature* sig = m_moduleInfo->GetSignature(sigIndex);
        newFunction->SetSignature(sig);
        m_moduleInfo->SetFunSig(newFunction, iFunc);
    }
}

void WasmBinaryReader::ReadExportTable()
{
    uint32 length;
    uint32 entries = LEB128(length);
    m_moduleInfo->AllocateFunctionExports(entries);

    for (uint32 iExport = 0; iExport < entries; iExport++)
    {
        uint32 funcIndex = LEB128(length);
        if (funcIndex >= m_moduleInfo->GetFunctionCount())
        {
            ThrowDecodingError(L"Invalid Export %u => func[%u]", iExport, funcIndex);
        }
        uint32 nameLength;
        wchar_t* exportName = ReadInlineName(length, nameLength);
        TRACE_WASM_DECODER(L"Export: Function(%u) => %s", funcIndex, exportName);
        m_moduleInfo->SetFunctionExport(iExport, funcIndex, exportName, nameLength);
    }
}

void
WasmBinaryReader::FunctionBodyHeader()
{
    UINT32 i32Count = 0, i64Count = 0, f32Count = 0, f64Count = 0;
    UINT len = 0;

    m_funcInfo = m_moduleInfo->GetFunSig(m_moduleState.count);
    m_currentNode.func.info = m_funcInfo;

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
        case Wasm::WasmTypes::I32:
            i32Count += count;
            break;
        case Wasm::WasmTypes::I64:
            i64Count += count;
            break;
        case Wasm::WasmTypes::F32:
            f32Count += count;
            break;
        case Wasm::WasmTypes::F64:
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

wchar_t* WasmBinaryReader::ReadInlineName(uint32& length, uint32& nameLength)
{
    nameLength = LEB128(length);
    CheckBytesLeft(nameLength);
    LPCUTF8 rawName = (LPCUTF8)(m_pc);
    
    m_pc += nameLength;
    length += nameLength;

    utf8::DecodeOptions decodeOptions = utf8::doDefault;
    charcount_t utf16Len = utf8::ByteIndexIntoCharacterIndex(rawName, nameLength, decodeOptions);
    wchar_t* contents = AnewArray(&m_alloc, wchar_t, utf16Len);
    if (contents == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    utf8::DecodeIntoAndNullTerminate((wchar_t*)contents, rawName, utf16Len, decodeOptions);

    return contents;
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
    length = 1;

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
        TRACE_WASM_DECODER_PHASE(WasmLEB128, L"Binary decoder: LEB128 value = %u, length = %u", result, length);
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
