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

uint32 GetTypeByteSize(WasmType type)
{
    switch (type)
    {
    case Void: return sizeof(Js::Var);
    case I32: return sizeof(int32);
    case I64: return sizeof(int64);
    case F32: return sizeof(float);
    case F64: return sizeof(double);
    default:
        Js::Throw::InternalError();
    }
}

} // namespace WasmTypes

WasmTypes::WasmType 
LanguageTypes::ToWasmType(int8 binType)
{
    switch (binType)
    {
    case LanguageTypes::i32: return WasmTypes::I32;
    case LanguageTypes::i64: return WasmTypes::I64;
    case LanguageTypes::f32: return WasmTypes::F32;
    case LanguageTypes::f64: return WasmTypes::F64;
    default:
        throw WasmCompilationException(_u("Invalid binary type %d"), binType);
    }
}

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule * module, const byte* source, size_t length) :
    m_module(module),
    m_curFuncEnd(nullptr),
    m_alloc(alloc),
    m_readerState(READER_STATE_UNKNOWN)
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
    m_readerState = READER_STATE_UNKNOWN;
#if DBG_DUMP
    if (DO_WASM_TRACE_SECTION)
    {
        const byte* startModule = m_pc;

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
    m_readerState = READER_STATE_MODULE;

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
        ReadFunctionHeaders();
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
        m_readerState = READER_STATE_UNKNOWN;
        return false;
    }

    m_readerState = READER_STATE_UNKNOWN;

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

void
WasmBinaryReader::ReadFunctionHeaders()
{
    uint32 len;
    uint32 entries = LEB128(len);
    uint32 importCount = m_module->GetImportCount();
    if (m_module->GetWasmFunctionCount() < importCount ||
        entries != m_module->GetWasmFunctionCount() - importCount)
    {
        ThrowDecodingError(_u("Function signatures and function bodies count mismatch"));
    }

    for (uint32 i = 0; i < entries; ++i)
    {
        uint32 funcIndex = i + importCount;
        WasmFunctionInfo* funcInfo = m_module->GetWasmFunctionInfo(funcIndex);

        const uint32 funcSize = LEB128(len);
        funcInfo->m_readerInfo.index = funcIndex;
        funcInfo->m_readerInfo.size = funcSize;
        funcInfo->m_readerInfo.startOffset = (m_pc - m_start);
        CheckBytesLeft(funcSize);
        TRACE_WASM_DECODER(_u("Function body header: index = %u, size = %u"), funcIndex, funcSize);
        const byte* end = m_pc + funcSize;
        m_pc = end;
    }
}

void
WasmBinaryReader::SeekToFunctionBody(FunctionBodyReaderInfo readerInfo)
{
    if (readerInfo.startOffset >= (m_end - m_start))
    {
        ThrowDecodingError(_u("Function byte offset out of bounds"));
    }
    if (m_readerState != READER_STATE_UNKNOWN)
    {
        ThrowDecodingError(_u("Wasm reader in an invalid state to read function code"));
    }
    m_readerState = READER_STATE_FUNCTION;

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

void WasmBinaryReader::FunctionEnd()
{
    m_readerState = READER_STATE_UNKNOWN;
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
    case wbCurrentMemory:
    case wbGrowMemory:
        // Reserved value currently unused
        ReadConst<uint8>();
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
    // Reserved value currently unused
    ReadConst<uint8>();

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
    int8 blockType = ReadConst<int8>();
    m_funcState.count++;
    m_currentNode.block.sig = blockType == LanguageTypes::emptyBlock ? WasmTypes::Void : LanguageTypes::ToWasmType(blockType);
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

        int8 form = ReadConst<int8>();
        if (form != LanguageTypes::func)
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

    for (uint32 iFunc = 0; iFunc < nFunctions; iFunc++)
    {
        uint32 sigIndex = LEB128(len);
        if (sigIndex >= m_module->GetSignatureCount())
        {
            ThrowDecodingError(_u("Function signature is out of bound"));
        }

        WasmSignature* sig = m_module->GetSignature(sigIndex);
        m_module->AddWasmFunctionInfo(sig);
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
        const char16* exportName = ReadInlineName(length, nameLength);

        ExternalKinds::ExternalKind kind = (ExternalKinds::ExternalKind)ReadConst<int8>();
        uint32 index = LEB128(length);
        switch (kind)
        {
        case ExternalKinds::Function:
        {
            FunctionIndexTypes::Type type = m_module->GetFunctionIndexType(index);
            if (type != FunctionIndexTypes::ImportThunk && type != FunctionIndexTypes::Function)
            {
                ThrowDecodingError(_u("Invalid Export %u => func[%u]"), iExport, index);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);

#if DBG_DUMP
            if (type == FunctionIndexTypes::ImportThunk)
            {
                WasmImport* import = m_module->GetFunctionImport(index);
                TRACE_WASM_DECODER(_u("Export #%u: Import(%s.%s)(%u) => %s"), iExport, import->modName, import->fnName, index, exportName);
            }
            else
            {
                TRACE_WASM_DECODER(_u("Export #%u: Function(%u) => %s"), iExport, index, exportName);
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
            m_module->SetMemoryExported();
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
        int8 elementType = ReadConst<int8>();
        if (elementType != LanguageTypes::anyfunc)
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
        m_module->SetTableSize(initialLength);
        m_module->CalculateEquivalentSignatures();
        TRACE_WASM_DECODER(_u("Indirect table: %u entries"), initialLength);
    }
}

void
WasmBinaryReader::ReadElementSection()
{
    uint32 length = 0;
    uint32 count = LEB128(length);
    if (count != 0)
    {
        m_module->AllocateElementSegs(count);
    }
    TRACE_WASM_DECODER(_u("Indirect table element: %u entries"), count);

    for (uint32 i = 0; i < count; ++i)
    {
        uint32 index = LEB128(length); // Table id
        if (index != 0)
        {
            ThrowDecodingError(_u("Invalid table index %d"), index); //MVP limitation
        }

        WasmNode initExpr = ReadInitExpr(); //Offset Init
        uint32 numElem = LEB128(length);

        WasmElementSegment *eSeg = Anew(m_alloc, WasmElementSegment, m_alloc, index, initExpr, numElem);

        for (uint32 iElem = 0; iElem < numElem; ++iElem)
        {
            uint32 elem = LEB128(length);
            FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(elem);
            if (funcType == FunctionIndexTypes::Invalid)
            {
                ThrowDecodingError(_u("Invalid function index %d"), elem);
            }
            if (funcType != FunctionIndexTypes::ImportThunk && funcType != FunctionIndexTypes::Function)
            {
                Assert(UNREACHED);
                ThrowDecodingError(_u("Unknown function type to insert in the table"));
            }
            eSeg->AddElement(elem, *m_module);
        }
        m_module->SetTableValues(eSeg, i);
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
        case  wbI64Const:
            global->SetReferenceType(WasmGlobal::Const);
            global->cnst = globalNode.cnst;
            break;
        case  wbGetGlobal:
            global->SetReferenceType(WasmGlobal::LocalReference);
            global->var = globalNode.var;
            break;
        default:
            Assert(UNREACHED);
        }

        m_module->globals->Add(global);
    }
}

const char16* WasmBinaryReader::ReadInlineName(uint32& length, uint32& nameLength)
{
    nameLength = LEB128(length);
    CheckBytesLeft(nameLength);
    LPCUTF8 rawName = m_pc;

    m_pc += nameLength;
    length += nameLength;

    return CvtUtf8Str(rawName, nameLength);
}

const char16* WasmBinaryReader::CvtUtf8Str(LPCUTF8 name, uint32 nameLen)
{
    utf8::DecodeOptions decodeOptions = utf8::doDefault;
    charcount_t utf16Len = utf8::ByteIndexIntoCharacterIndex(name, nameLen, decodeOptions);
    char16* contents = AnewArray(m_alloc, char16, utf16Len + 1);
    if (contents == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    utf8::DecodeIntoAndNullTerminate(contents, name, utf16Len, decodeOptions);
    return contents;
}

void
WasmBinaryReader::ReadImportEntries()
{
    uint32 len = 0;
    uint32 entries = LEB128(len);

    for (uint32 i = 0; i < entries; ++i)
    {
        uint32 modNameLen = 0, fnNameLen = 0;
        const char16* modName = ReadInlineName(len, modNameLen);
        const char16* fnName = ReadInlineName(len, fnNameLen);

        ExternalKinds::ExternalKind kind = (ExternalKinds::ExternalKind)ReadConst<int8>();
        TRACE_WASM_DECODER(_u("Import #%u: \"%s\".\"%s\", kind: %d"), i, modName, fnName, kind);
        switch (kind)
        {
        case ExternalKinds::Function:
        {
            uint32 sigId = LEB128(len);
            m_module->AddFunctionImport(sigId, modName, modNameLen, fnName, fnNameLen);
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
    if (m_readerState != READER_STATE_MODULE)
    {
        ThrowDecodingError(_u("Wasm reader in an invalid state to read init_expr"));
    }

    m_funcState.count = 0;
    m_funcState.size = m_currentSection.end - m_pc;
    ReadExpr();
    WasmNode node = m_currentNode;
    switch (node.op)
    {
    case wbI32Const:
    case wbF32Const:
    case wbI64Const:
    case wbF64Const:
        break;
    case wbGetGlobal:
    {
        uint32 globalIndex = node.var.num;
        if (globalIndex >= (uint32)m_module->globals->Count())
        {
            ThrowDecodingError(_u("Global %u out of bounds"), globalIndex);
        }
        WasmGlobal* global = m_module->globals->Item(globalIndex);
        if (global->GetMutability())
        {
            ThrowDecodingError(_u("initializer expression cannot reference a mutable global"));
        }
        break;
    }
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
    return LanguageTypes::ToWasmType(ReadConst<int8>());
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
