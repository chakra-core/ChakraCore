//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
#if ENABLE_DEBUG_CONFIG_OPTIONS
#include "Codex/Utf8Helper.h"
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

bool
FunctionIndexTypes::CanBeExported(FunctionIndexTypes::Type funcType)
{
    return funcType == FunctionIndexTypes::Function || funcType == FunctionIndexTypes::ImportThunk;
}

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule * module, const byte* source, size_t length) :
    m_module(module),
    m_curFuncEnd(nullptr),
    m_alloc(alloc),
    m_readerState(READER_STATE_UNKNOWN)
{
    m_start = m_pc = source;
    m_end = source + length;
    m_currentSection.code = bSectLimit;
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
        SectionCode prevSect = bSectLimit;
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
    while (true)
    {
        if (EndOfModule() || SectionInfo::All[nextSection].flag == fSectIgnore)
        {
            return false;
        }

        m_currentSection = ReadSectionHeader();
        if (SectionInfo::All[m_currentSection.code].flag == fSectIgnore)
        {
            TRACE_WASM_DECODER(_u("Ignore this section"));
            m_pc = m_currentSection.end;
            // Read next section
            continue;
        }

        // Process the custom sections now
        if (m_currentSection.code == bSectCustom)
        {
            if (!ProcessCurrentSection())
            {
                ThrowDecodingError(_u("Error while reading custom section %s"), m_currentSection.name);
            }
            // Read next section
            continue;
        }

        if (m_currentSection.code < nextSection)
        {
            ThrowDecodingError(_u("Invalid Section %s"), m_currentSection.code);
        }

        if (m_currentSection.code != nextSection)
        {
            TRACE_WASM_DECODER(_u("The current section is not the one we are looking for"));
            // We know about this section, but it's not the one we're looking for
            m_pc = m_currentSection.start;
            return false;
        }
        return true;
    }
}

bool
WasmBinaryReader::ProcessCurrentSection()
{
    Assert(m_currentSection.code != bSectLimit);
    TRACE_WASM_SECTION(_u("Process section %s"), SectionInfo::All[m_currentSection.code].name);
    m_readerState = READER_STATE_MODULE;

    switch (m_currentSection.code)
    {
    case bSectMemory:
        ReadMemorySection(false);
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
        ReadTableSection(false);
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
    case bSectCustom:
        ReadCustomSection();
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
    header.code = bSectLimit;

    UINT len = 0;
    CompileAssert(sizeof(SectionCode) == sizeof(uint8));
    SectionCode sectionId = (SectionCode)ReadVarUInt7();

    if (sectionId > bsectLastKnownSection)
    {
        ThrowDecodingError(_u("Invalid known section opcode %u"), sectionId);
    }

    UINT32 sectionSize = LEB128(len);
    header.end = m_pc + sectionSize;
    CheckBytesLeft(sectionSize);

    header.code = sectionId;
    const char *sectionName = SectionInfo::All[sectionId].id;
    UINT32 nameLength = SectionInfo::All[sectionId].nameLength;
    if (sectionId == bSectCustom)
    {
        nameLength = LEB128(len);
        CheckBytesLeft(nameLength);
        sectionName = (const char*)(m_pc);
        m_pc += nameLength;
    }

    header.nameLength = nameLength;
    header.name = sectionName;

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
    int count = m_ops->Count();
    if (count == 0)
    {
        return;
    }
    WasmOp * ops = HeapNewArray(WasmOp, count);

    auto iter = m_ops->GetIterator();
    int i = 0;
    while (iter.IsValid())
    {
        ops[i] = iter.CurrentKey();
        iter.MoveNext();
        ++i;
    }
    for (i = 0; i < count; ++i)
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
    for (i = 0; i < count; ++i)
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
    HeapDeleteArray(count, ops);
}

#endif

void
WasmBinaryReader::ReadFunctionHeaders()
{
    uint32 len;
    uint32 entries = LEB128(len);
    uint32 importCount = m_module->GetImportedFunctionCount();
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

    if (CONFIG_FLAG(WasmCheckVersion))
    {
        // Accept version 0xd to avoid problem in our test infrastructure
        // We should eventually remove support for 0xd.
        // The Assert is here as a reminder in case we change the binary version and we haven't removed 0xd support yet
        CompileAssert(binaryVersion == 0x1);

        if (version != binaryVersion && version != 0xd)
        {
            ThrowDecodingError(_u("Invalid WASM version!"));
        }
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
    if (!m_module->HasTable() && !m_module->HasTableImport())
    {
        ThrowDecodingError(_u("Found call_indirect operator, but no table"));
    }

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

    // flags
    const uint32 flags = LEB128(len);
    m_currentNode.mem.alignment = (uint8)flags;
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
WasmBinaryReader::ReadMemorySection(bool isImportSection)
{
    UINT length = 0;
    UINT32 count;
    if (isImportSection)
    {
        count = 1;
    }
    else
    {
        count = LEB128(length);
    }
    if (count > 1)
    {
        ThrowDecodingError(_u("Maximum of 1 memory allowed"));
    }

    if (count == 1)
    {
        uint32 flags = LEB128(length);
        uint32 minPage = LEB128(length);
        if (minPage > 65536)
        {
            throw Wasm::WasmCompilationException(_u("Memory size must be at most 65536 pages (4GiB)"));
        }
        uint32 maxPage = UINT32_MAX;
        if (flags & 0x1)
        {
            maxPage = LEB128(length);
            if (maxPage > 65536)
            {
                throw Wasm::WasmCompilationException(_u("Memory size must be at most 65536 pages (4GiB)"));
            }
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

        WasmSignature * sig = m_module->GetSignature(i);
        sig->SetSignatureId(i);
        int8 form = ReadConst<int8>();
        if (form != LanguageTypes::func)
        {
            ThrowDecodingError(_u("Unexpected type form 0x%X"), form);
        }
        UINT32 paramCount = LEB128(len);
        WasmTypes::WasmType type;
        sig->AllocateParams(paramCount, m_module->GetRecycler());

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
        sig->FinalizeSignature();
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

    ArenaAllocator tmpAlloc(_u("ExportDupCheck"), m_module->GetScriptContext()->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory);
    typedef SList<const char16*> NameList;
    JsUtil::BaseDictionary<uint32, NameList*, ArenaAllocator> exportsNameDict(&tmpAlloc);

    for (uint32 iExport = 0; iExport < entries; iExport++)
    {
        uint32 nameLength;
        const char16* exportName = ReadInlineName(length, nameLength);

        // Check if the name is already used
        NameList* list;
        if (exportsNameDict.TryGetValue(nameLength, &list))
        {
            const char16** found = list->Find([exportName, nameLength](const char16* existing) { 
                return wcsncmp(exportName, existing, nameLength) == 0;
            });
            if (found)
            {
                ThrowDecodingError(_u("Duplicate export name: %s"), exportName);
            }
        }
        else
        {
            list = Anew(&tmpAlloc, NameList, &tmpAlloc);
            exportsNameDict.Add(nameLength, list);
        }
        list->Push(exportName);

        ExternalKinds::ExternalKind kind = (ExternalKinds::ExternalKind)ReadConst<int8>();
        uint32 index = LEB128(length);
        switch (kind)
        {
        case ExternalKinds::Function:
        {
            FunctionIndexTypes::Type type = m_module->GetFunctionIndexType(index);
            if (!FunctionIndexTypes::CanBeExported(type))
            {
                ThrowDecodingError(_u("Invalid Export %u => func[%u]"), iExport, index);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);

#if DBG_DUMP
            if (type == FunctionIndexTypes::ImportThunk)
            {
                WasmImport* import = m_module->GetWasmFunctionInfo(index)->importedFunctionReference;
                TRACE_WASM_DECODER(_u("Export #%u: Import(%s.%s)(%u) => %s"), iExport, import->modName, import->importName, index, exportName);
            }
            else
            {
                TRACE_WASM_DECODER(_u("Export #%u: Function(%u) => %s"), iExport, index, exportName);
            }
#endif
            break;
        }
        case ExternalKinds::Memory:
            if (index != 0)
            {
                ThrowDecodingError(_u("Unknown memory index %u for export %s"), index, exportName);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);
            break;
        case ExternalKinds::Table:
            if (index != 0)
            {
                ThrowDecodingError(_u("Unknown table index %u for export %s"), index, exportName);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);
            break;
        case ExternalKinds::Global:
            if (index >= m_module->GetGlobalCount())
            {
                ThrowDecodingError(_u("Unknown global %u for export %s"), index, exportName);
            }
            if (m_module->GetGlobal(index)->IsMutable())
            {
                ThrowDecodingError(_u("Mutable globals cannot be exported"), index, exportName);
            }
            m_module->SetExport(iExport, index, exportName, nameLength, kind);
            break;
        default:
            ThrowDecodingError(_u("Exported Kind %d, NYI"), kind);
            break;
        }

    }
}

void WasmBinaryReader::ReadTableSection(bool isImportSection)
{
    uint32 length;
    uint32 entries;
    if (isImportSection)
    {
        entries = 1;
    }
    else
    {
        entries = LEB128(length);
    }
    if (entries > 1)
    {
        ThrowDecodingError(_u("Maximum of one table allowed"));
    }

    if (entries == 1)
    {
        int8 elementType = ReadConst<int8>();
        if (elementType != LanguageTypes::anyfunc)
        {
            ThrowDecodingError(_u("Only anyfunc type is supported. Unknown type %d"), elementType);
        }
        uint32 flags = LEB128(length);
        uint32 initialLength = LEB128(length);
        uint32 maximumLength = UINT32_MAX;
        if (flags & 0x1)
        {
            maximumLength = LEB128(length);
        }
        m_module->InitializeTable(initialLength, maximumLength);
        TRACE_WASM_DECODER(_u("Indirect table: %u to %u entries"), initialLength, maximumLength);
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
        if (index != 0 || !(m_module->HasTable() || m_module->HasTableImport()))
        {
            ThrowDecodingError(_u("Unknown table index %d"), index); //MVP limitation
        }

        WasmNode initExpr = ReadInitExpr(true);
        uint32 numElem = LEB128(length);

        WasmElementSegment *eSeg = Anew(m_alloc, WasmElementSegment, m_alloc, index, initExpr, numElem);

        for (uint32 iElem = 0; iElem < numElem; ++iElem)
        {
            uint32 elem = LEB128(length);
            FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(elem);
            if (!FunctionIndexTypes::CanBeExported(funcType))
            {
                ThrowDecodingError(_u("Invalid function to insert in the table %u"), elem);
            }
            eSeg->AddElement(elem, *m_module);
        }
        m_module->SetElementSeg(eSeg, i);
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
        if (index != 0 || !(m_module->HasMemory() || m_module->HasMemoryImport()))
        {
            ThrowDecodingError(_u("Unknown memory index %u"), index);
        }
        TRACE_WASM_DECODER(_u("Data Segment #%u"), i);
        WasmNode initExpr = ReadInitExpr(true);

        //UINT32 offset = initExpr.cnst.i32;
        UINT32 dataByteLen = LEB128(len);
        WasmDataSegment *dseg = Anew(m_alloc, WasmDataSegment, m_alloc, initExpr, dataByteLen, m_pc);
        CheckBytesLeft(dataByteLen);
        m_pc += dataByteLen;
        m_module->SetDataSeg(dseg, i);
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
        bool isMutable = ReadMutableValue();
        WasmNode globalNode = ReadInitExpr();
        GlobalReferenceTypes::Type refType = GlobalReferenceTypes::Const;
        WasmTypes::WasmType initType;

        switch (globalNode.op) {
        case  wbI32Const: initType = WasmTypes::I32; break;
        case  wbF32Const: initType = WasmTypes::F32; break;
        case  wbF64Const: initType = WasmTypes::F64; break;
        case  wbI64Const: initType = WasmTypes::I64; break;
        case  wbGetGlobal:
            initType = m_module->GetGlobal(globalNode.var.num)->GetType();
            refType = GlobalReferenceTypes::LocalReference;
            break;
        default:
            Assert(UNREACHED);
            ThrowDecodingError(_u("Unknown global init_expr"));
        }
        if (type != initType)
        {
            ThrowDecodingError(_u("Type mismatch for global initialization"));
        }
        m_module->AddGlobal(refType, type, isMutable, globalNode);
    }
}

void
WasmBinaryReader::ReadCustomSection()
{
    CustomSection customSection;
    customSection.name = CvtUtf8Str((LPCUTF8)m_currentSection.name, m_currentSection.nameLength, &customSection.nameLength);
    customSection.payload = m_pc;

    size_t size = m_currentSection.end - m_pc;
    if (m_currentSection.end < m_pc || !Math::FitsInDWord(size))
    {
        ThrowDecodingError(_u("Invalid custom section size"));
    }
    customSection.payloadSize = (uint32)size;
    m_module->AddCustomSection(customSection);
    m_pc = m_currentSection.end;
}

const char16*
WasmBinaryReader::ReadInlineName(uint32& length, uint32& nameLength)
{
    nameLength = LEB128(length);
    CheckBytesLeft(nameLength);
    LPCUTF8 rawName = m_pc;

    m_pc += nameLength;
    length += nameLength;

    return CvtUtf8Str(rawName, nameLength);
}

const char16*
WasmBinaryReader::CvtUtf8Str(LPCUTF8 name, uint32 nameLen, charcount_t* dstLength)
{
    utf8::DecodeOptions decodeOptions = utf8::doDefault;
    charcount_t utf16Len = utf8::ByteIndexIntoCharacterIndex(name, nameLen, decodeOptions);
    char16* contents = AnewArray(m_alloc, char16, utf16Len + 1);
    if (contents == nullptr)
    {
        Js::Throw::OutOfMemory();
    }
    utf8::DecodeUnitsIntoAndNullTerminate(contents, name, name + nameLen, decodeOptions);
    if (dstLength)
    {
        *dstLength = utf16Len;
    }
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
            bool isMutable = ReadMutableValue();
            if (isMutable)
            {
                ThrowDecodingError(_u("Mutable globals cannot be imported"));
            }
            m_module->AddGlobal(GlobalReferenceTypes::ImportedReference, type, isMutable, {});
            m_module->AddGlobalImport(modName, modNameLen, fnName, fnNameLen);
            break;
        }
        case ExternalKinds::Table:
            ReadTableSection(true);
            m_module->AddTableImport(modName, modNameLen, fnName, fnNameLen);
            break;
        case ExternalKinds::Memory:
            ReadMemorySection(true);
            m_module->AddMemoryImport(modName, modNameLen, fnName, fnNameLen);

            break;
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
    Wasm::FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(id);
    if (!FunctionIndexTypes::CanBeExported(funcType))
    {
        ThrowDecodingError(_u("Invalid function index for start function %u"), id);
    }
    WasmSignature* sig = m_module->GetWasmFunctionInfo(id)->GetSignature();
    if (sig->GetParamCount() > 0 || sig->GetResultType() != WasmTypes::Void)
    {
        ThrowDecodingError(_u("Start function must be void and nullary"));
    }
    m_module->SetStartFunction(id);
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

    for (uint i = 0; i < maxReads; i++, length++)
    {
        CheckBytesLeft(1);
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
WasmBinaryReader::ReadInitExpr(bool isOffset)
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
        WasmGlobal* global = m_module->GetGlobal(globalIndex);
        if (global->GetReferenceType() != GlobalReferenceTypes::ImportedReference)
        {
            ThrowDecodingError(_u("initializer expression can only use imported globals"));
        }
        if (global->IsMutable())
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
    if (isOffset)
    {
        m_module->ValidateInitExportForOffset(node);
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

uint8
WasmBinaryReader::ReadVarUInt7()
{
    return ReadConst<uint8>() & 0x7F;
}

bool WasmBinaryReader::ReadMutableValue()
{
    uint8 mutableValue = ReadConst<UINT8>();
    switch (mutableValue)
    {
    case 0: return false;
    case 1: return true;
    default:
        ThrowDecodingError(_u("invalid mutability"));
    }
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
