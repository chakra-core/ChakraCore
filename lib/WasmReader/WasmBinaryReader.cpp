//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"


#ifdef ENABLE_WASM

namespace Wasm
{
    namespace WasmTypes
    {
        bool IsLocalType(WasmTypes::WasmType type) 
        {
            // Check if type in range ]Void,Limit[
            return (uint)(type - 1) < (WasmTypes::Limit - 1);
        }
    }

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

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, byte* source, size_t length) :
    m_alloc(alloc),
    m_curFuncEnd(nullptr),
    m_lastOp(WasmBinOp::wbLimit)
{
    m_moduleInfo = Anew(m_alloc, ModuleInfo, m_alloc);

    m_start = m_pc = source;
    m_end = source + length;
    m_currentSection.code = bSectInvalid;
#if DBG_DUMP
    m_ops = Anew(m_alloc, OpSet, m_alloc);
#endif
}

void WasmBinaryReader::InitializeReader()
{
    ValidateModuleHeader();
#if DBG_DUMP
    if (DO_WASM_TRACE_SECTION)
    {
        byte* startModule = m_pc;

        bool doRead = true;
        SectionCode prevSect = bSectInvalid;
        while (doRead)
        {
            SectionHeader secHeader = ReadSectionHeader();
            if (secHeader.code <= prevSect)
            {
                TRACE_WASM_SECTION(_u("Unknown section order"));
            }
            prevSect = secHeader.code;
            // skip the section
            m_pc = secHeader.end;
            doRead = !EndOfModule();
        }
        m_pc = startModule;
    }
#endif
}

void
WasmBinaryReader::ThrowDecodingError(const char16* msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    throw WasmCompilationException(msg, argptr);
}

bool
WasmBinaryReader::ReadNextSection(SectionCode nextSection)
{
    if (EndOfModule() || SectionInfo::All[nextSection].flag == fSectIgnore)
    {
        return false;
    }

    SectionHeader secHeader = ReadSectionHeader();
    if (secHeader.code == bSectInvalid || SectionInfo::All[secHeader.code].flag == fSectIgnore)
    {
        TRACE_WASM_DECODER(_u("Ignore this section"));
        m_pc = secHeader.end;
        return ReadNextSection(nextSection);
    }
    if (secHeader.code < nextSection)
    {
        ThrowDecodingError(_u("Invalid Section %s"), secHeader.code);
    }

    if (secHeader.code != nextSection)
    {
        TRACE_WASM_DECODER(_u("The current section is not the one we are looking for"));
        // We know about this section, but it's not the one we're looking for
        m_pc = secHeader.start;
        return false;
    }
    m_currentSection = secHeader;
    return true;
}

bool
WasmBinaryReader::ProcessCurrentSection()
{
    Assert(m_currentSection.code != bSectInvalid);
    TRACE_WASM_SECTION(_u("Process section %s"), SectionInfo::All[m_currentSection.code].name);
    switch (m_currentSection.code)
    {
    case bSectMemory:
        ReadMemorySection();
        break;
    case bSectSignatures:
        ReadSignatures();
        break;
    case bSectImportTable:
        ReadImportEntries();
        break;
    case bSectFunctionSignatures:
        ReadFunctionsSignatures();
        break;
    case bSectFunctionBodies:
        // The function bodies cannot be read fully by the binary reader alone. Call ReadFunctionBodies() instead
        Assert(UNREACHED);
        break;
    case bSectExportTable:
        ReadExportTable();
        break;
    case bSectStartFunction:
        ReadStartFunction();
        break;
    case bSectDataSegments:
        ReadDataSegments();
        break;
    case bSectIndirectFunctionTable:
        ReadIndirectFunctionTable();
        break;
    case bSectNames:
        ReadNamesSection();
        break;
    default:
        Assert(false);
        return false;
    }

    return m_pc == m_currentSection.end;
}

SectionHeader
WasmBinaryReader::ReadSectionHeader()
{
    UINT len = 0;
    UINT32 sectionSize;
    UINT32 idSize;

    SectionHeader header;
    header.start = m_pc;

    idSize = LEB128(len);
    const char *sectionName = (char*)(m_pc);
    m_pc += idSize;

    sectionSize = LEB128(len);
    header.end = m_pc + sectionSize;
    CheckBytesLeft(sectionSize);

    for (int i = 0; i < bSectLimit ; i++)
    {
        if (!memcmp(SectionInfo::All[i].id, sectionName, idSize))
        {
            header.code = (SectionCode)i;
            break;
        }
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    Assert(idSize < 64);
    char16 buf[64];
    size_t convertedChars = 0;
    mbstowcs_s(&convertedChars, buf, idSize + 1, sectionName, _TRUNCATE);
    buf[idSize] = 0;
    TRACE_WASM_SECTION(_u("Section Header: %s, length = %u (0x%x)"), buf, sectionSize, sectionSize);
#endif
    return header;
}

#if DBG_DUMP
void
WasmBinaryReader::PrintOps()
{
    WasmBinOp * ops = HeapNewArray(WasmBinOp, m_ops->Count());

    auto iter = m_ops->GetIterator();
    int i = 0;
    while (iter.IsValid())
    {
        ops[i] = iter.CurrentKey();
        iter.MoveNext();
        ++i;
    }
    for (i = 0; i < m_ops->Count(); ++i)
    {
        int j = i;
        while (j > 0 && ops[j-1] > ops[j])
        {
            WasmBinOp tmp = ops[j];
            ops[j] = ops[j - 1];
            ops[j - 1] = tmp;

            --j;
        }
    }
    for (i = 0; i < m_ops->Count(); ++i)
    {
        switch (ops[i])
        {
#define WASM_OPCODE(opname, opcode, token, sig) \
    case opcode: \
        Output::Print(_u(#token ## "\r\n")); \
        break;
#include "WasmBinaryOpcodes.h"
        }
    }
    HeapDeleteArray(m_ops->Count(), ops);
}

#endif

bool
WasmBinaryReader::ReadFunctionHeaders()
{
    uint32 len;
    uint32 entries = LEB128(len);
    if (entries != m_moduleInfo->GetFunctionCount())
    {
        ThrowDecodingError(_u("Function signatures and function bodies count mismatch"));
    }

    for (uint32 i = 0; i < entries; ++i)
    {
        WasmFunctionInfo* funcInfo = m_moduleInfo->GetFunSig(i);

        const uint32 funcSize = LEB128(len);
        funcInfo->m_readerInfo.index = i;
        funcInfo->m_readerInfo.size = funcSize;
        funcInfo->m_readerInfo.startOffset = (m_pc - m_start);
        CheckBytesLeft(funcSize);
        TRACE_WASM_DECODER(_u("Function body header: index = %u, size = %u"), i, funcSize);
        byte* end = m_pc + funcSize;
        m_pc = end;
    }
    return m_pc == m_currentSection.end;
}

void
WasmBinaryReader::SeekToFunctionBody(FunctionBodyReaderInfo readerInfo)
{
    if (readerInfo.startOffset >= (m_end - m_start))
    {
        ThrowDecodingError(_u("Function byte offset out of bounds"));
    }
    // Seek to the function start and skip function header (count)
    m_pc = m_start + readerInfo.startOffset;
    m_funcState.size = readerInfo.size;
    m_funcState.count = 0;
    CheckBytesLeft(readerInfo.size);
    m_curFuncEnd = m_pc + m_funcState.size;

    uint32 len = 0;
    uint32 entryCount = LEB128(len);
    m_funcState.count += len;

    WasmFunctionInfo* funcInfo = m_moduleInfo->GetFunSig(readerInfo.index);
    if (!funcInfo) 
    {
        ThrowDecodingError(_u("Invalid function index %u"), readerInfo.index);
    }
    // locals
    for (uint32 j = 0; j < entryCount; j++)
    {
        uint32 count = LEB128(len);
        m_funcState.count += len;
        Wasm::WasmTypes::WasmType type = ReadWasmType(len);
        if (!Wasm::WasmTypes::IsLocalType(type))
        {
            ThrowDecodingError(_u("Invalid local type"));
        }
        m_funcState.count += len;
        funcInfo->AddLocal(type, count);
        switch (type) 
        {
#define WASM_LOCALTYPE(token, name) case Wasm::WasmTypes::token: TRACE_WASM_DECODER(_u("Local: type = " #name## ", count = %u"), type, count); break;
#include "WasmKeywords.h"
        }
    }
}

bool WasmBinaryReader::IsCurrentFunctionCompleted() const
{
    return m_pc == m_curFuncEnd;
}

WasmOp
WasmBinaryReader::ReadExpr()
{
    return GetWasmToken(ASTNode());
}

Wasm::WasmOp WasmBinaryReader::GetLastOp()
{
    return GetWasmToken(m_lastOp);
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
        return wbFuncEnd;
    }

    WasmBinOp op = (WasmBinOp)*m_pc++;
    ++m_funcState.count;
    switch (op)
    {
    case wbBlock:
    case wbLoop:
        break;
    case wbCall:
        CallNode();
        break;
    case wbCallImport:
        CallImportNode();
        break;
    case wbCallIndirect:
        CallIndirectNode();
        break;
    case wbBr:
    case wbBrIf:
        BrNode();
        break;
    case wbBrTable:
        BrTableNode();
        break;
    case wbReturn:
        m_currentNode.ret.arity = ReadConst<UINT8>();
        ++m_funcState.count;
        break;
    case wbI32Const:
        ConstNode<WasmTypes::bAstI32>();
        break;
    case wbI64Const:
        ConstNode<WasmTypes::bAstI64>();
        break;
    case wbF32Const:
        ConstNode<WasmTypes::bAstF32>();
        break;
    case wbF64Const:
        ConstNode<WasmTypes::bAstF64>();
        break;
    case wbSetLocal:
    case wbGetLocal:
        VarNode();
        break;
    case wbIf:
    case wbElse:
        // no node attributes
        break;
    case wbEnd:
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
        ThrowDecodingError(_u("Unknown opcode 0x%X"), op);
    }

#if DBG_DUMP
    m_ops->AddNew(op);
#endif
    m_lastOp = op;
    return op;
}

void
WasmBinaryReader::ValidateModuleHeader()
{
    uint32 magicNumber = ReadConst<UINT32>();
    uint32 version = ReadConst<UINT32>();
    TRACE_WASM_DECODER(_u("Module Header: Magic 0x%x, Version %u"), magicNumber, version);
    if (magicNumber != 0x6d736100)
    {
        ThrowDecodingError(_u("Malformed WASM module header!"));
    }

    if (version != experimentalVersion)
    {
        ThrowDecodingError(_u("Invalid WASM version!"));
    }
}

void
WasmBinaryReader::CallNode()
{
    UINT length = 0;
    m_currentNode.call.arity = LEB128(length);
    m_funcState.count += length;

    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    if (funcNum >= m_moduleInfo->GetFunctionCount())
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.num = funcNum;
}

void
WasmBinaryReader::CallImportNode()
{
    UINT length = 0;
    m_currentNode.call.arity = LEB128(length);
    m_funcState.count += length;

    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    if (funcNum >= m_moduleInfo->GetImportCount())
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.num = funcNum;
}

void
WasmBinaryReader::CallIndirectNode()
{
    UINT length = 0;
    m_currentNode.call.arity = LEB128(length);
    m_funcState.count += length;

    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    if (funcNum >= m_moduleInfo->GetSignatureCount())
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.num = funcNum;
}

// control flow
void
WasmBinaryReader::BrNode()
{
    UINT len = 0;
    m_currentNode.br.arity = ReadConst<uint8>();
    m_funcState.count++;

    if (m_currentNode.br.arity > 1)
    {
        ThrowDecodingError(_u("NYI: br yielding more than 1 value"));
    }

    m_currentNode.br.depth = LEB128(len);
    m_funcState.count += len;

    m_currentNode.br.hasSubExpr = m_currentNode.br.arity == 1;
}

void
WasmBinaryReader::BrTableNode()
{
    UINT len = 0;
    m_currentNode.brTable.arity = LEB128(len);
    m_funcState.count += len;

    if (m_currentNode.brTable.arity > 1)
    {
        ThrowDecodingError(_u("NYI: br_table yielding more than 1 value"));
    }

    m_currentNode.brTable.numTargets = LEB128(len);
    m_funcState.count += len;
    m_currentNode.brTable.targetTable = AnewArray(m_alloc, UINT32, m_currentNode.brTable.numTargets);

    for (UINT32 i = 0; i < m_currentNode.brTable.numTargets; i++)
    {
        m_currentNode.brTable.targetTable[i] = ReadConst<uint32>();
        m_funcState.count += sizeof(uint32);
    }
    m_currentNode.brTable.defaultTarget = ReadConst<uint32>();
    m_funcState.count += sizeof(uint32);
}

WasmOp
WasmBinaryReader::MemNode(WasmBinOp op)
{
    uint len = 0;

    LEB128(len); // flags (unused)
    m_funcState.count += len;

    m_currentNode.mem.offset = LEB128(len);
    m_funcState.count += len;

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
    case WasmTypes::bAstI64:
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

Wasm::WasmTypes::WasmType
WasmBinaryReader::GetWasmType(WasmTypes::LocalType type)
{
    const uint32 binaryToWasmTypesLength = sizeof(WasmBinaryReader::binaryToWasmTypes) / sizeof(Wasm::WasmTypes::WasmType);
    if ((uint32)type >= binaryToWasmTypesLength)
    {
        ThrowDecodingError(_u("Invalid local type %u"), type);
    }
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
    return m_funcState.count >= m_funcState.size;
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
WasmBinaryReader::ReadSignatures()
{
    UINT len = 0;
    const uint32 count = LEB128(len);
    m_moduleInfo->SetSignatureCount(count);
    // signatures table
    for (UINT32 i = 0; i < count; i++)
    {
        TRACE_WASM_DECODER(_u("Signature #%u"), i);
        WasmSignature * sig = Anew(m_alloc, WasmSignature, m_alloc);

        char form = ReadConst<UINT8>();
        if (form != 0x40)
        {
            ThrowDecodingError(_u("Unexpected type form 0x%X"), form);
        }
        // TODO: use param count to create fixed size array
        UINT32 paramCount = LEB128(len);
        Wasm::WasmTypes::WasmType type;

        for (UINT32 j = 0; j < paramCount; j++)
        {
            type = ReadWasmType(len);
            sig->AddParam(type);
        }

        UINT32 resultCount = LEB128(len);
        if (resultCount != 0 && resultCount != 1)
        {
            ThrowDecodingError(_u("Unexpected result count %u"), resultCount);
        }
        if (resultCount == 1)
        {
            type = ReadWasmType(len);
            sig->SetResultType(type);
        }
        m_moduleInfo->SetSignature(i, sig);
    }
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
            ThrowDecodingError(_u("Function signature is out of bound"));
        }

        WasmFunctionInfo* newFunction = Anew(m_alloc, WasmFunctionInfo, m_alloc);
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
            ThrowDecodingError(_u("Invalid Export %u => func[%u]"), iExport, funcIndex);
        }
        uint32 nameLength;
        char16* exportName = ReadInlineName(length, nameLength);
        TRACE_WASM_DECODER(_u("Export #%u: Function(%u) => %s"), iExport, funcIndex, exportName);
        m_moduleInfo->SetFunctionExport(iExport, funcIndex, exportName, nameLength);
    }
}

void WasmBinaryReader::ReadIndirectFunctionTable()
{
    uint32 length;
    uint32 entries = LEB128(length);
    TRACE_WASM_DECODER(_u("Indirect table: %u entries = ["), entries);
    m_moduleInfo->AllocateIndirectFunctions(entries);
    for (uint32 i = 0; i < entries; i++)
    {
        uint32 functionIndex = LEB128(length);
        if (functionIndex >= m_moduleInfo->GetFunctionCount())
        {
            ThrowDecodingError(_u("Indirect function index %u is out of bound (max %u)"), functionIndex, m_moduleInfo->GetFunctionCount());
        }
        TRACE_WASM_DECODER(_u("%u, "), functionIndex);
        m_moduleInfo->SetIndirectFunction(functionIndex, i);
    }
    TRACE_WASM_DECODER(_u("]"), entries);
}

void
WasmBinaryReader::ReadDataSegments()
{
    UINT len = 0;
    const uint32 entries = LEB128(len);
    if (entries > 0)
    {
        m_moduleInfo->AllocateDataSegs(entries);
    }

    for (uint32 i = 0; i < entries; ++i)
    {
        TRACE_WASM_DECODER(L"Data Segment #%u", i);
        UINT32 offset = LEB128(len);
        UINT32 dataByteLen = LEB128(len);
        WasmDataSegment *dseg = Anew(m_alloc, WasmDataSegment, m_alloc, offset, dataByteLen, m_pc);
        CheckBytesLeft(dataByteLen);
        m_pc += dataByteLen;
        m_moduleInfo->AddDataSeg(dseg, i);
    }
}

void
WasmBinaryReader::ReadNamesSection()
{
    UINT len = 0;
    UINT numEntries = LEB128(len);

    for (UINT i = 0; i < numEntries; ++i)
    {
        UINT fnNameLen = 0;
        WasmFunctionInfo* funsig = m_moduleInfo->GetFunSig(i);
        funsig->SetName(ReadInlineName(len, fnNameLen));
        UINT numLocals = LEB128(len);
        if (numLocals != funsig->GetLocalCount())
        {
            ThrowDecodingError(_u("num locals mismatch in names section"));
        }
        for (UINT j = 0; j < numLocals; ++j)
        {
            UINT localNameLen = 0;
            ReadInlineName(len, localNameLen);
        }
    }
}

char16* WasmBinaryReader::ReadInlineName(uint32& length, uint32& nameLength)
{
    nameLength = LEB128(length);
    CheckBytesLeft(nameLength);
    LPUTF8 rawName = m_pc;

    m_pc += nameLength;
    length += nameLength;

    return CvtUtf8Str(rawName, nameLength);
}

char16* WasmBinaryReader::CvtUtf8Str(LPUTF8 name, uint32 nameLen)
{
    utf8::DecodeOptions decodeOptions = utf8::doDefault;
    charcount_t utf16Len = utf8::ByteIndexIntoCharacterIndex(name, nameLen, decodeOptions);
    char16* contents = AnewArray(m_alloc, char16, utf16Len + 1);
    if (contents == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    utf8::DecodeIntoAndNullTerminate((char16*)contents, name, utf16Len, decodeOptions);
    return contents;
}

void
WasmBinaryReader::ReadImportEntries()
{
    uint32 len = 0;
    uint32 entries = LEB128(len);
    if (entries > 0)
    {
        m_moduleInfo->AllocateFunctionImports(entries);
    }
    for (uint32 i = 0; i < entries; ++i)
    {
        uint32 sigId = LEB128(len);
        uint32 modNameLen = 0, fnNameLen = 0;

        if (sigId >= m_moduleInfo->GetSignatureCount())
        {
            ThrowDecodingError(_u("Function signature %u is out of bound"), sigId);
        }

        char16* modName = ReadInlineName(len, modNameLen);
        char16* fnName = ReadInlineName(len, fnNameLen);
        TRACE_WASM_DECODER(_u("Import #%u: \"%s\".\"%s\""), i, modName, fnName);
        m_moduleInfo->SetFunctionImport(i, sigId, modName, modNameLen, fnName, fnNameLen);
    }
}

void
WasmBinaryReader::ReadStartFunction()
{
    uint32 len = 0;
    uint32 id = LEB128(len);
    m_moduleInfo->SetStartFunction(id);

    // TODO: Validate
    // 1. Valid function id
    // 2. Function should be void and nullary
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

    if (b & 0x80 || m_pc > m_end)
    {
        ThrowDecodingError(_u("Invalid LEB128 format"));
    }

    if (sgn && (shamt < sizeof(INT) * 8) && (0x40 & b))
    {
        result |= -(1 << shamt);
    }

    if (!sgn)
    {
        TRACE_WASM_LEB128(_u("Binary decoder: LEB128 value = %u, length = %u"), result, length);
    }

    return result;
}

// Signed LEB128
INT
WasmBinaryReader::SLEB128(UINT &length)
{
    INT result = LEB128(length, true);

    TRACE_WASM_LEB128(_u("Binary decoder: SLEB128 value = %d, length = %u"), result, length);
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

Wasm::WasmTypes::WasmType
WasmBinaryReader::ReadWasmType(uint32& length)
{
    length = 1;
    Wasm::WasmTypes::WasmType type = GetWasmType((WasmTypes::LocalType)ReadConst<UINT8>());
    if (type >= Wasm::WasmTypes::Limit)
    {
        ThrowDecodingError(_u("Invalid type"));
    }
    return type;
}

void
WasmBinaryReader::CheckBytesLeft(UINT bytesNeeded)
{
    UINT bytesLeft = (UINT)(m_end - m_pc);
    if (bytesNeeded > bytesLeft)
    {
        ThrowDecodingError(_u("Out of file: Needed: %d, Left: %d"), bytesNeeded, bytesLeft);
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
    binWasmOpToWasmOp[WasmBinOp::wbFuncEnd] = Wasm::WasmOp::wnFUNC_END;
    binWasmOpToWasmOp[WasmBinOp::wbLimit] = Wasm::WasmOp::wnLIMIT;
    }

    isInit = true;
}

} // namespace Binary
} // namespace Wasm


#undef TRACE_WASM_DECODER

#endif // ENABLE_WASM
