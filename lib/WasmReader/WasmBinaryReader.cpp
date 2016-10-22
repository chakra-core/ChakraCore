//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
#if ENABLE_DEBUG_CONFIG_OPTIONS
#include "Codex\Utf8Helper.h"
#endif

namespace Wasm
{

namespace WasmTypes
{
bool IsLocalType(WasmTypes::WasmType type)
{
    // Check if type in range ]Void,Limit[
    return (uint)(type - 1) < (WasmTypes::Limit - 1);
}

} // namespace WasmTypes

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, WasmModule* module, byte* source, size_t length) :
    m_module(module),
    m_curFuncEnd(nullptr),
    m_alloc(alloc)
{
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
        return ReadFunctionHeaders();
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
        ReadTableSection();
        break;
    case bSectElement:
        ReadElementSection();
        break;
    case bSectNames:
        ReadNamesSection();
        break;
    case bSectGlobal:
        ReadGlobalsSection();
        break;
    default:
        Assert(UNREACHED);
        return false;
    }

    return m_pc == m_currentSection.end;
}

SectionHeader
WasmBinaryReader::ReadSectionHeader()
{
    SectionHeader header;
    header.start = m_pc;
    header.code = bSectInvalid;

    UINT len = 0;
    UINT32 sectionId = LEB128(len);

    UINT32 sectionSize = LEB128(len);
    header.end = m_pc + sectionSize;
    CheckBytesLeft(sectionSize);

    const char *sectionName = nullptr;
    UINT32 nameLength = 0;

    if (sectionId > 0)
    {
        SectionCode sectCode = (SectionCode)(sectionId - 1);

        if (sectCode >= bSectNames) // ">=" since "Name" isn't considered to be a known section
        {
            ThrowDecodingError(_u("Invalid known section opcode %d"), sectCode);
        }

        sectionName = SectionInfo::All[sectCode].id;
        header.code = sectCode;
        nameLength = static_cast<UINT32>(strlen(sectionName)); //sectionName (SectionInfo.id) is null-terminated
    }
    else
    {
        nameLength = LEB128(len);
        CheckBytesLeft(nameLength);
        sectionName = (char*)(m_pc);
        m_pc += nameLength; //skip section name for now
        header.code = bSectUser;
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    if (DO_WASM_TRACE_SECTION)
    {
        char16* wstr = nullptr;
        size_t unused;
        utf8::NarrowStringToWide<utf8::malloc_allocator>(sectionName, nameLength, &wstr, &unused);
        TRACE_WASM_SECTION(_u("Section Header: %s, length = %u (0x%x)"), wstr, sectionSize, sectionSize);
        free(wstr);
    }
#endif
    return header;
}

#if DBG_DUMP
void
WasmBinaryReader::PrintOps()
{
    WasmOp * ops = HeapNewArray(WasmOp, m_ops->Count());

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
            WasmOp tmp = ops[j];
            ops[j] = ops[j - 1];
            ops[j - 1] = tmp;

            --j;
        }
    }
    for (i = 0; i < m_ops->Count(); ++i)
    {
        switch (ops[i])
        {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
    case opcode: \
        Output::Print(_u("%s\r\n"), _u(#opname)); \
        break;
#include "WasmBinaryOpCodes.h"
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
    if (entries != m_module->GetWasmFunctionCount())
    {
        ThrowDecodingError(_u("Function signatures and function bodies count mismatch"));
    }

    for (uint32 i = 0; i < entries; ++i)
    {
        WasmFunctionInfo* funcInfo = m_module->GetWasmFunctionInfo(i);

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

    WasmFunctionInfo* funcInfo = m_module->GetWasmFunctionInfo(readerInfo.index);

    // locals
    for (uint32 j = 0; j < entryCount; j++)
    {
        uint32 count = LEB128(len);
        m_funcState.count += len;
        WasmTypes::WasmType type = ReadWasmType(len);
        if (!WasmTypes::IsLocalType(type))
        {
            ThrowDecodingError(_u("Invalid local type"));
        }
        m_funcState.count += len;
        funcInfo->AddLocal(type, count);
        switch (type)
        {
        case WasmTypes::I32: TRACE_WASM_DECODER(_u("Local: type = I32, count = %u"), count); break;
        case WasmTypes::I64: TRACE_WASM_DECODER(_u("Local: type = I64, count = %u"), count); break;
        case WasmTypes::F32: TRACE_WASM_DECODER(_u("Local: type = F32, count = %u"), count); break;
        case WasmTypes::F64: TRACE_WASM_DECODER(_u("Local: type = F64, count = %u"), count); break;
            break;
        default:
            break;
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
    WasmOp op = m_currentNode.op = (WasmOp)*m_pc++;
    ++m_funcState.count;

    if (EndOfFunc())
    {
        // end of AST
        if (op != wbEnd)
        {
            ThrowDecodingError(_u("missing function end opcode"));
        }
        return op;
    }

    switch (op)
    {
    case wbBlock:
    case wbLoop:
    case wbIf:
        BlockNode();
        break;
    case wbElse:
        // no node attributes
        break;
    case wbCall:
        CallNode();
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
        break;
    case wbI32Const:
        ConstNode<WasmTypes::I32>();
        break;
    case wbI64Const:
        ConstNode<WasmTypes::I64>();
        break;
    case wbF32Const:
        ConstNode<WasmTypes::F32>();
        break;
    case wbF64Const:
        ConstNode<WasmTypes::F64>();
        break;
    case wbSetLocal:
    case wbGetLocal:
    case wbTeeLocal:
    case wbGetGlobal:
    case wbSetGlobal:
        VarNode();
        break;
    case wbDrop:
        break;
    case wbEnd:
        break;
    case wbNop:
        break;
#define WASM_MEM_OPCODE(opname, opcode, sig, nyi) \
    case wb##opname: \
        MemNode(); \
    break;
#include "WasmBinaryOpCodes.h"
    default:
        break;
    }

#if DBG_DUMP
    m_ops->AddNew(op);
#endif
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

    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(funcNum);
    if (funcType == FunctionIndexTypes::Invalid)
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.funcType = funcType;
    m_currentNode.call.num = funcNum;
}

void
WasmBinaryReader::CallIndirectNode()
{
    UINT length = 0;

    UINT32 funcNum = LEB128(length);
    m_funcState.count += length;
    if (funcNum >= m_module->GetSignatureCount())
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.num = funcNum;
    m_currentNode.call.funcType = FunctionIndexTypes::Function;
}

void WasmBinaryReader::BlockNode()
{
    uint8 sig = ReadConst<uint8>();
    m_funcState.count++;
    if (sig >= WasmTypes::Limit)
    {
        ThrowDecodingError(_u("Invalid block signature type"));
    }
    m_currentNode.block.sig = (WasmTypes::WasmType)sig;
}

// control flow
void
WasmBinaryReader::BrNode()
{
    UINT len = 0;
    m_currentNode.br.depth = LEB128(len);
    m_funcState.count += len;
}

void
WasmBinaryReader::BrTableNode()
{
    UINT len = 0;
    m_currentNode.brTable.numTargets = LEB128(len);
    m_funcState.count += len;
    m_currentNode.brTable.targetTable = AnewArray(m_alloc, UINT32, m_currentNode.brTable.numTargets);

    for (UINT32 i = 0; i < m_currentNode.brTable.numTargets; i++)
    {
        m_currentNode.brTable.targetTable[i] = LEB128(len);
        m_funcState.count += len;
    }
    m_currentNode.brTable.defaultTarget = LEB128(len);
    m_funcState.count += len;
}

void
WasmBinaryReader::MemNode()
{
    uint len = 0;

    LEB128(len); // flags (unused)
    m_funcState.count += len;

    m_currentNode.mem.offset = LEB128(len);
    m_funcState.count += len;
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
template <WasmTypes::WasmType localType>
void WasmBinaryReader::ConstNode()
{
    UINT len = 0;
    switch (localType)
    {
    case WasmTypes::I32:
        m_currentNode.cnst.i32 = SLEB128(len);
        m_funcState.count += len;
        break;
    case WasmTypes::I64:
        m_currentNode.cnst.i64 = SLEB128<INT64>(len);
        m_funcState.count += len;
        break;
    case WasmTypes::F32:
        m_currentNode.cnst.f32 = ReadConst<float>();
        m_funcState.count += sizeof(float);
        break;
    case WasmTypes::F64:
        m_currentNode.cnst.f64 = ReadConst<double>();
        m_funcState.count += sizeof(double);
        break;
    }
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
    UINT32 count = LEB128(length);
    if (count > 1)
    {
        ThrowDecodingError(_u("Maximum of 1 memory allowed"));
    }

    if (count == 1)
    {
        uint32 flags = LEB128(length);
        uint32 minPage = LEB128(length);
        uint32 maxPage = minPage;
        if (flags & 0x1)
        {
            maxPage = LEB128(length);
        }
        m_module->InitializeMemory(minPage, maxPage);
    }
}

void
WasmBinaryReader::ReadSignatures()
{
    UINT len = 0;
    const uint32 count = LEB128(len);
    m_module->SetSignatureCount(count);
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
        UINT32 paramCount = LEB128(len);
        WasmTypes::WasmType type;
        sig->AllocateParams(paramCount);

        for (UINT32 j = 0; j < paramCount; j++)
        {
            type = ReadWasmType(len);
            sig->SetParam(type, j);
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
        m_module->SetSignature(i, sig);
    }
}

void
WasmBinaryReader::ReadFunctionsSignatures()
{
    UINT len = 0;
    uint32 nFunctions = LEB128(len);
    m_module->AllocateWasmFunctions(nFunctions);

    for (uint32 iFunc = 0; iFunc < nFunctions; iFunc++)
    {
        uint32 sigIndex = LEB128(len);
        if (sigIndex >= m_module->GetSignatureCount())
        {
            ThrowDecodingError(_u("Function signature is out of bound"));
        }

        WasmSignature* sig = m_module->GetSignature(sigIndex);
        WasmFunctionInfo* newFunction = Anew(m_alloc, WasmFunctionInfo, m_alloc, sig, iFunc);
        m_module->SetWasmFunctionInfo(newFunction, iFunc);
    }
}

void WasmBinaryReader::ReadExportTable()
{
    uint32 length;
    uint32 entries = LEB128(length);
    m_module->AllocateFunctionExports(entries);

    for (uint32 iExport = 0; iExport < entries; iExport++)
    {
        uint32 nameLength;
        char16* exportName = ReadInlineName(length, nameLength);

        ExternalKinds::ExternalKind kind = (ExternalKinds::ExternalKind)ReadConst<int8>();
        uint32 index = LEB128(length);
        switch (kind)
        {
        case ExternalKinds::Function:
        {
            FunctionIndexTypes::Type type = m_module->GetFunctionIndexType(index);
            if (type == FunctionIndexTypes::Invalid)
            {
                ThrowDecodingError(_u("Invalid Export %u => func[%u]"), iExport, index);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);

#if DBG_DUMP
            uint32 normIndex = m_module->NormalizeFunctionIndex(index);
            if (type == FunctionIndexTypes::Import)
            {
                WasmImport* import = m_module->GetFunctionImport(normIndex);
                TRACE_WASM_DECODER(_u("Export #%u: Import(%s.%s)(%u) => %s"), iExport, import->modName, import->fnName, normIndex, exportName);
            }
            else
            {
                TRACE_WASM_DECODER(_u("Export #%u: Function(%u) => %s"), iExport, normIndex, exportName);
            }
#endif
            break;
        }
        case ExternalKinds::Memory:
        {
            if (index != 0)
            {
                ThrowDecodingError(_u("Invalid memory index %s"), index);
            }
            m_module->SetMemoryIsExported();
            break;
        }
        case ExternalKinds::Global:
            m_module->SetExport(iExport, index, exportName, nameLength, kind);
            break;
        case ExternalKinds::Table:
            ThrowDecodingError(_u("Exported Kind Table, NYI"));
        default:
            ThrowDecodingError(_u("Exported Kind %d, NYI"), kind);
            break;
        }

    }
}

void WasmBinaryReader::ReadTableSection()
{
    uint32 length;
    uint32 entries = LEB128(length);
    if (entries > 1)
    {
        ThrowDecodingError(_u("Maximum of one table allowed"));
    }

    if (entries > 0)
    {
        uint8 elementType = ReadConst<uint8>();
        if (elementType != ElementTypes::anyfunc)
        {
            ThrowDecodingError(_u("Only anyfunc type is supported. Unknown type %d"), elementType);
        }
        uint32 flags = LEB128(length);
        uint32 initialLength = LEB128(length);
        if (flags & 0x1)
        {
            uint32 maximumLength = LEB128(length);

            // Allocate maximum length for now until resizing supported
            initialLength = maximumLength;
        }
        m_module->AllocateTable(initialLength);
        m_module->CalculateEquivalentSignatures();
        TRACE_WASM_DECODER(_u("Indirect table: %u entries"), initialLength);
    }
}

void
WasmBinaryReader::ReadElementSection()
{
    uint32 length = 0;
    uint32 count = LEB128(length);

    for (uint32 i = 0; i < count; ++i)
    {
        uint32 index = LEB128(length);
        if (index != 0)
        {
            ThrowDecodingError(_u("Invalid table index %d"), index);
        }

        WasmNode initExpr = ReadInitExpr();
        if (initExpr.op != wbI32Const)
        {
            ThrowDecodingError(_u("Only int32.const supported for element offset"));
        }

        uint32 offset = initExpr.cnst.i32;
        uint32 numElem = LEB128(length);
        uint32 end = UInt32Math::Add(offset, numElem);
        if (end > m_module->GetTableSize())
        {
            ThrowDecodingError(_u("Out of bounds element in Table[%d][%d], max index: %d"), index, end - 1 , m_module->GetTableSize() - 1);
        }

        for (uint32 iElem = offset; iElem < end; ++iElem)
        {
            uint32 elem = LEB128(length);
            FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(elem);
            if (funcType == FunctionIndexTypes::Invalid)
            {
                ThrowDecodingError(_u("Invalid function index %d"), elem);
            }
            if (funcType == FunctionIndexTypes::Import)
            {
                ThrowDecodingError(_u("Import functions in the table NYI"));
            }
            m_module->SetTableValue(elem, iElem);
        }
    }
}

void
WasmBinaryReader::ReadDataSegments()
{
    UINT len = 0;
    const uint32 entries = LEB128(len);
    if (entries > 0)
    {
        m_module->AllocateDataSegs(entries);
    }

    for (uint32 i = 0; i < entries; ++i)
    {
        UINT32 index = LEB128(len);
        if (index != 0)
        {
            ThrowDecodingError(_u("Memory index out of bounds %d > 0"), index);
        }
        TRACE_WASM_DECODER(_u("Data Segment #%u"), i);
        WasmNode initExpr = ReadInitExpr();
        if (initExpr.op != wbI32Const && initExpr.op != wbGetGlobal)
        {
            ThrowDecodingError(_u("Only i32.const supported for data segment offset"));
        }
        //UINT32 offset = initExpr.cnst.i32;
        UINT32 dataByteLen = LEB128(len);
        WasmDataSegment *dseg = Anew(m_alloc, WasmDataSegment, m_alloc, initExpr, dataByteLen, m_pc);
        CheckBytesLeft(dataByteLen);
        m_pc += dataByteLen;
        m_module->AddDataSeg(dseg, i);
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
        WasmFunctionInfo* funsig = m_module->GetWasmFunctionInfo(i);
        funsig->SetName(ReadInlineName(len, fnNameLen), fnNameLen);
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

void
WasmBinaryReader::ReadGlobalsSection()
{
    UINT len = 0;
    UINT numEntries = LEB128(len);

    for (UINT i = 0; i < numEntries; ++i)
    {
        WasmTypes::WasmType type = ReadWasmType(len);
        bool mutability = ReadConst<UINT8>() == 1;
        WasmGlobal* global = Anew(m_alloc, WasmGlobal, m_module->globalCounts[type]++, type, mutability);

        WasmNode globalNode = ReadInitExpr();
        switch (globalNode.op) {
        case  wbI32Const:
        case  wbF32Const:
        case  wbF64Const:
            global->SetReferenceType(WasmGlobal::Const);
            global->cnst = globalNode.cnst;
            break;
        case  wbGetGlobal:
            global->SetReferenceType(WasmGlobal::LocalReference);
            global->var = globalNode.var;
            break;
        case  wbI64Const:
            ThrowDecodingError(_u("i64 globals NYI"));
        default:
            Assert(UNREACHED);
        }

        m_module->globals.Add(global);
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

    uint importFunctionCount = 0; //TODO: we are probably much better of using lists

    if (entries > 0)
    {
        m_module->AllocateFunctionImports(entries);
    }
    for (uint32 i = 0; i < entries; ++i)
    {
        uint32 modNameLen = 0, fnNameLen = 0;
        char16* modName = ReadInlineName(len, modNameLen);
        char16* fnName = ReadInlineName(len, fnNameLen);

        ExternalKinds::ExternalKind kind = (ExternalKinds::ExternalKind)ReadConst<int8>();
        TRACE_WASM_DECODER(_u("Import #%u: \"%s\".\"%s\", kind: %d"), i, modName, fnName, kind);
        switch (kind)
        {
        case ExternalKinds::Function:
        {
            uint32 sigId = LEB128(len);
            if (sigId >= m_module->GetSignatureCount())
            {
                ThrowDecodingError(_u("Function signature %u is out of bound"), sigId);
            }
            m_module->SetFunctionImport(importFunctionCount, sigId, modName, modNameLen, fnName, fnNameLen, kind);
            importFunctionCount++;
            break;
        }
        case ExternalKinds::Global:
        {
            WasmTypes::WasmType type = ReadWasmType(len);
            bool mutability = ReadConst<UINT8>() == 1;
            WasmGlobal* importedGlobal = Anew(m_alloc, WasmGlobal, m_module->globalCounts[type]++, type, mutability);
            if (importedGlobal->GetType() == WasmTypes::I64)
            {
                ThrowDecodingError(_u("I64 Globals, NYI"));
            }
            m_module->AddGlobalImport(modName, modNameLen, fnName, fnNameLen, kind, importedGlobal);
            break;
        }
        case ExternalKinds::Table:
            ThrowDecodingError(_u("Imported Kind Table, NYI"));
        case ExternalKinds::Memory:
            ThrowDecodingError(_u("Imported Kind Memory, NYI"));
        default:
            ThrowDecodingError(_u("Imported Kind %d, NYI"), kind);
            break;
        }
    }
    m_module->SetImportCount(importFunctionCount);
}

void
WasmBinaryReader::ReadStartFunction()
{
    uint32 len = 0;
    uint32 id = LEB128(len);
    m_module->SetStartFunction(id);

    // TODO: Validate
    // 1. Valid function id
    // 2. Function should be void and nullary
}

template<typename MaxAllowedType>
MaxAllowedType
WasmBinaryReader::LEB128(UINT &length, bool sgn)
{
    MaxAllowedType result = 0;
    uint shamt = 0;
    byte b = 0;
    length = 1;
    uint maxReads = sizeof(MaxAllowedType) == 4 ? 5 : 10;
    CompileAssert(sizeof(MaxAllowedType) == 4 || sizeof(MaxAllowedType) == 8);

    // LEB128 needs at least one byte
    CheckBytesLeft(1);

    for (uint i = 0; i < maxReads; i++, length++)
    {
        b = *m_pc++;
        result = result | ((MaxAllowedType)(b & 0x7f) << shamt);
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

    if (sgn && (shamt < sizeof(MaxAllowedType) * 8) && (0x40 & b))
    {
        if (sizeof(MaxAllowedType) == 4)
        {
            result |= -(1 << shamt);
        }
        else if (sizeof(MaxAllowedType) == 8)
        {
            result |= -((int64)1 << shamt);
        }
    }

    if (!sgn)
    {
        if (sizeof(MaxAllowedType) == 4)
        {
            TRACE_WASM_LEB128(_u("Binary decoder: LEB128 length = %u, value = %u (0x%x)"), length, result, result);
        }
        else if (sizeof(MaxAllowedType) == 8)
        {
            TRACE_WASM_LEB128(_u("Binary decoder: LEB128 length = %u, value = %llu (0x%llx)"), length, result, result);
        }
    }

    return result;
}

// Signed LEB128
template<>
INT
WasmBinaryReader::SLEB128(UINT &length)
{
    INT result = LEB128<UINT>(length, true);

    TRACE_WASM_LEB128(_u("Binary decoder: SLEB128 length = %u, value = %d (0x%x)"), length, result, result);
    return result;
}

template<>
INT64
WasmBinaryReader::SLEB128(UINT &length)
{
    INT64 result = LEB128<UINT64>(length, true);

    TRACE_WASM_LEB128(_u("Binary decoder: SLEB128 length = %u, value = %lld (0x%llx)"), length, result, result);
    return result;
}

WasmNode
WasmBinaryReader::ReadInitExpr()
{
    m_funcState.count = 0;
    m_funcState.size = 123456; // some aribtrary big value
    ReadExpr();
    WasmNode node = m_currentNode;
    switch (node.op)
    {
    case wbI32Const:
    case wbF32Const:
    case wbI64Const:
    case wbF64Const:
    case wbGetGlobal:
        break;
    default:
        ThrowDecodingError(_u("Invalid initexpr opcode"));
    }

    if (ReadExpr() != wbEnd)
    {
        ThrowDecodingError(_u("Missing end opcode after init expr"));
    }
    return node;
}

template <typename T>
T WasmBinaryReader::ReadConst()
{
    CheckBytesLeft(sizeof(T));
    T value = *((T*)m_pc);
    m_pc += sizeof(T);

    return value;
}

WasmTypes::WasmType
WasmBinaryReader::ReadWasmType(uint32& length)
{
    length = 1;
    WasmTypes::WasmType type = (WasmTypes::WasmType)ReadConst<UINT8>();
    if (type >= WasmTypes::Limit)
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

} // namespace Wasm

#undef TRACE_WASM_DECODER

#endif // ENABLE_WASM
