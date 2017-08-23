//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
#include "WasmLimits.h"
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
    return (uint32)(type - 1) < (WasmTypes::Limit - 1);
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

const char16 * GetTypeName(WasmType type)
{
    switch (type) {
    case WasmTypes::WasmType::Void: return _u("void");
    case WasmTypes::WasmType::I32: return _u("i32");
    case WasmTypes::WasmType::I64: return _u("i64");
    case WasmTypes::WasmType::F32: return _u("f32");
    case WasmTypes::WasmType::F64: return _u("f64");
    case WasmTypes::WasmType::Any: return _u("any");
    default: Assert(UNREACHED); break;
    }
    return _u("unknown");
}

} // namespace WasmTypes

WasmTypes::WasmType LanguageTypes::ToWasmType(int8 binType)
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

bool FunctionIndexTypes::CanBeExported(FunctionIndexTypes::Type funcType)
{
    return funcType == FunctionIndexTypes::Function || funcType == FunctionIndexTypes::ImportThunk;
}

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule* module, const byte* source, size_t length) :
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
}

void WasmBinaryReader::ThrowDecodingError(const char16* msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    throw WasmCompilationException(msg, argptr);
}

SectionHeader WasmBinaryReader::ReadNextSection()
{
    while (true)
    {
        if (EndOfModule())
        {
            memset(&m_currentSection, 0, sizeof(SectionHeader));
            m_currentSection.code = bSectLimit;
            return m_currentSection;
        }

        m_currentSection = ReadSectionHeader();
        if (SectionInfo::All[m_currentSection.code].flag == fSectIgnore)
        {
            TRACE_WASM_SECTION(_u("Ignore this section"));
            m_pc = m_currentSection.end;
            // Read next section
            continue;
        }

        return m_currentSection;
    }
}

bool WasmBinaryReader::ProcessCurrentSection()
{
    Assert(m_currentSection.code != bSectLimit);
    TRACE_WASM_SECTION(_u("Process section %s"), SectionInfo::All[m_currentSection.code].name);
    m_readerState = READER_STATE_MODULE;

    switch (m_currentSection.code)
    {
    case bSectMemory:
        ReadMemorySection(false);
        break;
    case bSectType:
        ReadSignatureTypeSection();
        break;
    case bSectImport:
        ReadImportSection();
        break;
    case bSectFunction:
        ReadFunctionSignatures();
        break;
    case bSectFunctionBodies:
        ReadFunctionHeaders();
        break;
    case bSectExport:
        ReadExportSection();
        break;
    case bSectStartFunction:
        ReadStartFunction();
        break;
    case bSectData:
        ReadDataSection();
        break;
    case bSectTable:
        ReadTableSection(false);
        break;
    case bSectElement:
        ReadElementSection();
        break;
    case bSectName:
        ReadNameSection();
        break;
    case bSectGlobal:
        ReadGlobalSection();
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

SectionHeader WasmBinaryReader::ReadSectionHeader()
{
    SectionHeader header;
    header.start = m_pc;
    header.code = bSectLimit;

    uint32 len = 0;
    CompileAssert(sizeof(SectionCode) == sizeof(uint8));
    SectionCode sectionId = (SectionCode)ReadVarUInt7();

    if (sectionId > bsectLastKnownSection)
    {
        ThrowDecodingError(_u("Invalid known section opcode %u"), sectionId);
    }

    uint32 sectionSize = LEB128(len);
    header.end = m_pc + sectionSize;
    CheckBytesLeft(sectionSize);

    header.code = sectionId;
    if (sectionId == bSectCustom)
    {
        header.name = ReadInlineName(len, header.nameLength);
    }
    else
    {
        header.name = SectionInfo::All[sectionId].name;
        header.nameLength = SectionInfo::All[sectionId].nameLength;
    }

    TRACE_WASM_SECTION(_u("Section Header: %s, length = %u (0x%x)"), header.name, sectionSize, sectionSize);
    return header;
}

#if DBG_DUMP
void WasmBinaryReader::PrintOps()
{
    int count = m_ops->Count();
    if (count == 0)
    {
        return;
    }
    WasmOp* ops = HeapNewArray(WasmOp, count);

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

void WasmBinaryReader::ReadFunctionHeaders()
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
        if (funcSize > Limits::GetMaxFunctionSize())
        {
            ThrowDecodingError(_u("Function body too big"));
        }
        funcInfo->m_readerInfo.size = funcSize;
        funcInfo->m_readerInfo.startOffset = (m_pc - m_start);
        CheckBytesLeft(funcSize);
        TRACE_WASM_DECODER(_u("Function body header: index = %u, size = %u"), funcIndex, funcSize);
        const byte* end = m_pc + funcSize;
        m_pc = end;
    }
}

void WasmBinaryReader::SeekToFunctionBody(class WasmFunctionInfo* funcInfo)
{
    FunctionBodyReaderInfo readerInfo = funcInfo->m_readerInfo;
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

    uint32 length = 0;
    uint32 numLocalsEntries = LEB128(length);
    m_funcState.count += length;

    // locals
    for (uint32 j = 0; j < numLocalsEntries; j++)
    {
        uint32 numLocals = LEB128(length);
        m_funcState.count += length;
        WasmTypes::WasmType type = ReadWasmType(length);
        if (!WasmTypes::IsLocalType(type))
        {
            ThrowDecodingError(_u("Invalid local type"));
        }
        m_funcState.count += length;

        uint32 totalLocals = 0;
        if (UInt32Math::Add(funcInfo->GetLocalCount(), numLocals, &totalLocals) || totalLocals > Limits::GetMaxFunctionLocals())
        {
            ThrowDecodingError(_u("Too many locals"));
        }
        funcInfo->AddLocal(type, numLocals);
        TRACE_WASM_DECODER(_u("Local: type = %s, count = %u"), WasmTypes::GetTypeName(type), numLocals);
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

WasmOp WasmBinaryReader::ReadExpr()
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
    {
        // Reserved value currently unused
        uint8 reserved = ReadConst<uint8>();
        if (reserved != 0)
        {
            ThrowDecodingError(op == wbCurrentMemory
                ? _u("current_memory reserved value must be 0")
                : _u("grow_memory reserved value must be 0")
            );
        }
        break;
    }
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

void WasmBinaryReader::ValidateModuleHeader()
{
    uint32 bytesLeft = (uint32)(m_end - m_pc);
    if (bytesLeft > Limits::GetMaxModuleSize())
    {
        ThrowDecodingError(_u("Module too big"));
    }

    uint32 magicNumber = ReadConst<uint32>();
    uint32 version = ReadConst<uint32>();
    TRACE_WASM_DECODER(_u("Module Header: Magic 0x%x, Version %u"), magicNumber, version);
    if (magicNumber != 0x6d736100)
    {
        ThrowDecodingError(_u("Malformed WASM module header!"));
    }

    if (CONFIG_FLAG(WasmCheckVersion))
    {
        if (version != binaryVersion)
        {
            ThrowDecodingError(_u("Invalid WASM version!"));
        }
    }
}

void WasmBinaryReader::CallNode()
{
    uint32 length = 0;

    uint32 funcNum = LEB128(length);
    m_funcState.count += length;
    FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(funcNum);
    if (funcType == FunctionIndexTypes::Invalid)
    {
        ThrowDecodingError(_u("Function is out of bound"));
    }
    m_currentNode.call.funcType = funcType;
    m_currentNode.call.num = funcNum;
}

void WasmBinaryReader::CallIndirectNode()
{
    uint32 length = 0;

    uint32 funcNum = LEB128(length);
    // Reserved value currently unused
    uint8 reserved = ReadConst<uint8>();
    if (reserved != 0)
    {
        ThrowDecodingError(_u("call_indirect reserved value must be 0"));
    }
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
void WasmBinaryReader::BrNode()
{
    uint32 len = 0;
    m_currentNode.br.depth = LEB128(len);
    m_funcState.count += len;
}

void WasmBinaryReader::BrTableNode()
{
    uint32 len = 0;
    m_currentNode.brTable.numTargets = LEB128(len);
    if (m_currentNode.brTable.numTargets > Limits::GetMaxBrTableElems())
    {
        ThrowDecodingError(_u("br_table too big"));
    }
    m_funcState.count += len;
    m_currentNode.brTable.targetTable = AnewArray(m_alloc, uint32, m_currentNode.brTable.numTargets);

    for (uint32 i = 0; i < m_currentNode.brTable.numTargets; i++)
    {
        m_currentNode.brTable.targetTable[i] = LEB128(len);
        m_funcState.count += len;
    }
    m_currentNode.brTable.defaultTarget = LEB128(len);
    m_funcState.count += len;
}

void WasmBinaryReader::MemNode()
{
    uint32 len = 0;

    // flags
    const uint32 flags = LEB128(len);
    m_currentNode.mem.alignment = (uint8)flags;
    m_funcState.count += len;

    m_currentNode.mem.offset = LEB128(len);
    m_funcState.count += len;
}

// Locals/Globals
void WasmBinaryReader::VarNode()
{
    uint32 length;
    m_currentNode.var.num = LEB128(length);
    m_funcState.count += length;
}

// Const
template <WasmTypes::WasmType localType>
void WasmBinaryReader::ConstNode()
{
    uint32 len = 0;
    switch (localType)
    {
    case WasmTypes::I32:
        m_currentNode.cnst.i32 = SLEB128(len);
        m_funcState.count += len;
        break;
    case WasmTypes::I64:
        m_currentNode.cnst.i64 = SLEB128<int64>(len);
        m_funcState.count += len;
        break;
    case WasmTypes::F32:
    {
        m_currentNode.cnst.i32 = ReadConst<int32>();
        CompileAssert(sizeof(int32) == sizeof(float));
        m_funcState.count += sizeof(float);
        break;
    }
    case WasmTypes::F64:
        m_currentNode.cnst.i64 = ReadConst<int64>();
        CompileAssert(sizeof(int64) == sizeof(double));
        m_funcState.count += sizeof(double);
        break;
    }
}

bool WasmBinaryReader::EndOfFunc()
{
    return m_funcState.count >= m_funcState.size;
}

bool WasmBinaryReader::EndOfModule()
{
    return (m_pc >= m_end);
}

void WasmBinaryReader::ReadMemorySection(bool isImportSection)
{
    uint32 length = 0;
    uint32 count;
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
        SectionLimits limits = ReadSectionLimits(Limits::GetMaxMemoryInitialPages(), Limits::GetMaxMemoryMaximumPages(), _u("memory size too big"));
        m_module->InitializeMemory(limits.initial, limits.maximum);
    }
}

void WasmBinaryReader::ReadSignatureTypeSection()
{
    uint32 len = 0;
    const uint32 numTypes = LEB128(len);
    if (numTypes > Limits::GetMaxTypes())
    {
        ThrowDecodingError(_u("Too many signatures"));
    }

    m_module->SetSignatureCount(numTypes);
    // signatures table
    for (uint32 i = 0; i < numTypes; i++)
    {
        WasmSignature* sig = m_module->GetSignature(i);
        sig->SetSignatureId(i);
        int8 form = ReadConst<int8>();
        if (form != LanguageTypes::func)
        {
            ThrowDecodingError(_u("Unexpected type form 0x%X"), form);
        }

        uint32 paramCount32 = LEB128(len);
        if (paramCount32 > Limits::GetMaxFunctionParams() || paramCount32 > UINT16_MAX)
        {
            ThrowDecodingError(_u("Too many arguments in signature"));
        }

        Js::ArgSlot paramCount = (Js::ArgSlot)paramCount32;
        sig->AllocateParams(paramCount, m_module->GetRecycler());
        for (Js::ArgSlot j = 0; j < paramCount; j++)
        {
            WasmTypes::WasmType type = ReadWasmType(len);
            sig->SetParam(type, j);
        }

        uint32 resultCount = LEB128(len);
        if (resultCount > 1)
        {
            ThrowDecodingError(_u("Too many returns in signature: %u. Maximum allowed: 1"), resultCount);
        }
        if (resultCount == 1)
        {
            WasmTypes::WasmType type = ReadWasmType(len);
            sig->SetResultType(type);
        }
        sig->FinalizeSignature();
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (DO_WASM_TRACE_DECODER)
        {
            Output::Print(_u("Signature #%u: "), i);
            sig->Dump();
            Output::Print(_u("\n"));
        }
#endif
    }
}

void WasmBinaryReader::ReadFunctionSignatures()
{
    uint32 len = 0;
    uint32 nFunctions = LEB128(len);

    uint32 totalFunctions = 0;
    if (UInt32Math::Add(nFunctions, m_module->GetWasmFunctionCount(), &totalFunctions) || totalFunctions > Limits::GetMaxFunctions())
    {
        ThrowDecodingError(_u("Too many functions"));
    }

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

void WasmBinaryReader::ReadExportSection()
{
    uint32 length;
    uint32 numExports = LEB128(length);
    if (numExports > Limits::GetMaxExports())
    {
        ThrowDecodingError(_u("Too many exports"));
    }

    m_module->AllocateFunctionExports(numExports);

    ArenaAllocator tmpAlloc(_u("ExportDupCheck"), m_module->GetScriptContext()->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory);
    typedef SList<const char16*> NameList;
    JsUtil::BaseDictionary<uint32, NameList*, ArenaAllocator> exportsNameDict(&tmpAlloc);

    for (uint32 iExport = 0; iExport < numExports; iExport++)
    {
        uint32 nameLength;
        const char16* exportName = ReadInlineName(length, nameLength);

        // Check if the name is already used
        NameList* list = nullptr;
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
        SectionLimits limits = ReadSectionLimits(Limits::GetMaxTableSize(), Limits::GetMaxTableSize(), _u("table too big"));
        m_module->InitializeTable(limits.initial, limits.maximum);
        TRACE_WASM_DECODER(_u("Indirect table: %u to %u entries"), limits.initial, limits.maximum);
    }
}

void WasmBinaryReader::ReadElementSection()
{
    uint32 length = 0;
    uint32 numSegments = LEB128(length);
    if (numSegments > Limits::GetMaxElementSegments())
    {
        ThrowDecodingError(_u("Too many element segments"));
    }

    if (numSegments > 0)
    {
        m_module->AllocateElementSegs(numSegments);
    }
    TRACE_WASM_DECODER(_u("Indirect table element: %u entries"), numSegments);

    for (uint32 i = 0; i < numSegments; ++i)
    {
        uint32 index = LEB128(length); // Table id
        if (index != 0 || !(m_module->HasTable() || m_module->HasTableImport()))
        {
            ThrowDecodingError(_u("Unknown table index %d"), index); //MVP limitation
        }

        WasmNode initExpr = ReadInitExpr(true);
        uint32 numElem = LEB128(length);

        if (numElem > Limits::GetMaxTableSize())
        {
            ThrowDecodingError(_u("Too many table element"));
        }

        WasmElementSegment* eSeg = Anew(m_alloc, WasmElementSegment, m_alloc, index, initExpr, numElem);

        for (uint32 iElem = 0; iElem < numElem; ++iElem)
        {
            uint32 elem = LEB128(length);
            FunctionIndexTypes::Type funcType = m_module->GetFunctionIndexType(elem);
            if (!FunctionIndexTypes::CanBeExported(funcType))
            {
                ThrowDecodingError(_u("Invalid function to insert in the table %u"), elem);
            }
            eSeg->AddElement(elem);
        }
        m_module->SetElementSeg(eSeg, i);
    }
}

void WasmBinaryReader::ReadDataSection()
{
    uint32 len = 0;
    const uint32 numSegments = LEB128(len);
    if (numSegments > Limits::GetMaxDataSegments())
    {
        ThrowDecodingError(_u("Too many data segments"));
    }

    if (numSegments > 0)
    {
        m_module->AllocateDataSegs(numSegments);
    }

    for (uint32 i = 0; i < numSegments; ++i)
    {
        uint32 index = LEB128(len);
        if (index != 0 || !(m_module->HasMemory() || m_module->HasMemoryImport()))
        {
            ThrowDecodingError(_u("Unknown memory index %u"), index);
        }
        TRACE_WASM_DECODER(_u("Data Segment #%u"), i);
        WasmNode initExpr = ReadInitExpr(true);
        uint32 dataByteLen = LEB128(len);

        WasmDataSegment* dseg = Anew(m_alloc, WasmDataSegment, m_alloc, initExpr, dataByteLen, m_pc);
        CheckBytesLeft(dataByteLen);
        m_pc += dataByteLen;
        m_module->SetDataSeg(dseg, i);
    }
}

void WasmBinaryReader::ReadNameSection()
{
    uint32 len = 0;
    uint32 numFuncNames = LEB128(len);

    if (numFuncNames > Limits::GetMaxFunctions())
    {
        ThrowDecodingError(_u("Too many function names"));
    }

    for (uint32 i = 0; i < numFuncNames; ++i)
    {
        uint32 fnNameLen = 0;
        WasmFunctionInfo* funsig = m_module->GetWasmFunctionInfo(i);
        const char16* name = ReadInlineName(len, fnNameLen);
        funsig->SetName(name, fnNameLen);
        uint32 numLocals = LEB128(len);
        if (numLocals != funsig->GetLocalCount())
        {
            ThrowDecodingError(_u("num locals mismatch in names section"));
        }
        for (uint32 j = 0; j < numLocals; ++j)
        {
            uint32 localNameLen = 0;
            ReadInlineName(len, localNameLen);
        }
    }
}

void WasmBinaryReader::ReadGlobalSection()
{
    uint32 len = 0;
    uint32 numGlobals = LEB128(len);

    uint32 totalGlobals = 0;
    if (UInt32Math::Add(numGlobals, m_module->GetGlobalCount(), &totalGlobals) || totalGlobals > Limits::GetMaxGlobals())
    {
        ThrowDecodingError(_u("Too many globals"));
    }

    for (uint32 i = 0; i < numGlobals; ++i)
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

void WasmBinaryReader::ReadCustomSection()
{
    CustomSection customSection;
    customSection.name = m_currentSection.name;
    customSection.nameLength = m_currentSection.nameLength;
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

const char16* WasmBinaryReader::ReadInlineName(uint32& length, uint32& nameLength)
{
    uint32 rawNameLength = LEB128(length);
    if (rawNameLength > Limits::GetMaxStringSize())
    {
        ThrowDecodingError(_u("Name too long"));
    }

    CheckBytesLeft(rawNameLength);
    LPCUTF8 rawName = m_pc;

    m_pc += rawNameLength;
    length += rawNameLength;

    utf8::DecodeOptions decodeOptions = utf8::doThrowOnInvalidWCHARs;
    try
    {
        nameLength = (uint32)utf8::ByteIndexIntoCharacterIndex(rawName, rawNameLength, decodeOptions);
        char16* contents = AnewArray(m_alloc, char16, nameLength + 1);
        size_t decodedLength = utf8::DecodeUnitsIntoAndNullTerminate(contents, rawName, rawName + rawNameLength, decodeOptions);
        if (decodedLength != nameLength)
        {
            AssertMsg(UNREACHED, "We calculated the length before decoding, what happened ?");
            ThrowDecodingError(_u("Error while decoding utf8 string"));
        }
        return contents;
    }
    catch (utf8::InvalidWideCharException)
    {
        ThrowDecodingError(_u("Invalid UTF-8 encoding"));
    }
}

void WasmBinaryReader::ReadImportSection()
{
    uint32 len = 0;
    uint32 numImports = LEB128(len);

    if (numImports > Limits::GetMaxImports())
    {
        ThrowDecodingError(_u("Too many imports"));
    }

    for (uint32 i = 0; i < numImports; ++i)
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
            if (m_module->GetWasmFunctionCount() > Limits::GetMaxFunctions())
            {
                ThrowDecodingError(_u("Too many functions"));
            }
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
            if (m_module->GetGlobalCount() > Limits::GetMaxGlobals())
            {
                ThrowDecodingError(_u("Too many globals"));
            }
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

void WasmBinaryReader::ReadStartFunction()
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
MaxAllowedType WasmBinaryReader::LEB128(uint32 &length, bool sgn)
{
    MaxAllowedType result = 0;
    uint32 shamt = 0;
    byte b = 0;
    length = 1;
    uint32 maxReads = sizeof(MaxAllowedType) == 4 ? 5 : 10;
    CompileAssert(sizeof(MaxAllowedType) == 4 || sizeof(MaxAllowedType) == 8);

    for (uint32 i = 0; i < maxReads; i++, length++)
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
int32 WasmBinaryReader::SLEB128(uint32 &length)
{
    int32 result = LEB128<uint32>(length, true);

    TRACE_WASM_LEB128(_u("Binary decoder: SLEB128 length = %u, value = %d (0x%x)"), length, result, result);
    return result;
}

template<>
int64 WasmBinaryReader::SLEB128(uint32 &length)
{
    int64 result = LEB128<uint64>(length, true);

    TRACE_WASM_LEB128(_u("Binary decoder: SLEB128 length = %u, value = %lld (0x%llx)"), length, result, result);
    return result;
}

WasmNode WasmBinaryReader::ReadInitExpr(bool isOffset)
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

SectionLimits WasmBinaryReader::ReadSectionLimits(uint32 maxInitial, uint32 maxMaximum, const char16* errorMsg)
{
    SectionLimits limits;
    uint32 length = 0;
    uint32 flags = LEB128(length);
    limits.initial = LEB128(length);
    limits.maximum = maxMaximum;
    if (flags & 0x1)
    {
        limits.maximum = LEB128(length);
        if (limits.maximum > maxMaximum)
        {
            ThrowDecodingError(_u("Maximum %s"), errorMsg);
        }
    }
    if (limits.initial > maxInitial)
    {
        ThrowDecodingError(_u("Minimum %s"), errorMsg);
    }
    return limits;
}

// Do not use float version of ReadConst. Instead use integer version and reinterpret bits.
template<> double WasmBinaryReader::ReadConst<double>() { Assert(false); return 0; }
template<> float WasmBinaryReader::ReadConst<float>() { Assert(false); return 0;  }
template <typename T>
T WasmBinaryReader::ReadConst()
{
    CheckBytesLeft(sizeof(T));
    T value = *((T*)m_pc);
    m_pc += sizeof(T);

    return value;
}

uint8 WasmBinaryReader::ReadVarUInt7()
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

WasmTypes::WasmType WasmBinaryReader::ReadWasmType(uint32& length)
{
    length = 1;
    return LanguageTypes::ToWasmType(ReadConst<int8>());
}

void WasmBinaryReader::CheckBytesLeft(uint32 bytesNeeded)
{
    uint32 bytesLeft = (uint32)(m_end - m_pc);
    if (bytesNeeded > bytesLeft)
    {
        ThrowDecodingError(_u("Out of file: Needed: %d, Left: %d"), bytesNeeded, bytesLeft);
    }
}

} // namespace Wasm

#endif // ENABLE_WASM
