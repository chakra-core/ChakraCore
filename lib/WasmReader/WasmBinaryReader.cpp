//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
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

WasmBinaryReader::WasmBinaryReader(ArenaAllocator* alloc, Js::WebAssemblyModule* module, const byte* source, size_t length) :
    m_alloc(alloc),
    m_start(source),
    m_end(source + length),
    m_pc(source),
    m_curFuncEnd(nullptr),
    m_currentSection(),
    m_readerState(READER_STATE_UNKNOWN),
    m_module(module)
#if DBG_DUMP
    , m_ops(nullptr)
#endif
{
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

void WasmBinaryReader::ThrowDecodingError(const char16* msg, ...) const
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
            memset((void*)&m_currentSection, 0, sizeof(SectionHeader));
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
    SectionCode sectionId = (SectionCode)LEB128<uint8, 7>(len);

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

#if ENABLE_DEBUG_CONFIG_OPTIONS
Js::FunctionBody* WasmBinaryReader::GetFunctionBody() const
{
    if (m_readerState == READER_STATE_FUNCTION)
    {
        return m_funcState.body;
    }
    return nullptr;
}
#endif

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
        __analysis_assume(i < count);
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

    uint32 moduleId = m_module->GetWasmFunctionInfo(0)->GetBody()->GetSourceContextId();
    Output::Print(_u("Module #%u's current opcode distribution\n"), moduleId);
    for (i = 0; i < count; ++i)
    {
        switch (ops[i])
        {
#define WASM_OPCODE(opname, opcode, sig, imp, wat) \
    case opcode: \
        Output::Print(_u("%s: %s\r\n"), _u(#opname), _u(wat)); \
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
        funcInfo->SetReaderInfo(FunctionBodyReaderInfo(funcSize, (m_pc - m_start)));
        CheckBytesLeft(funcSize);
        TRACE_WASM_DECODER(_u("Function body header: index = %u, size = %u"), funcIndex, funcSize);
        const byte* end = m_pc + funcSize;
        m_pc = end;
    }
}

void WasmBinaryReader::SeekToFunctionBody(class WasmFunctionInfo* funcInfo)
{
    FunctionBodyReaderInfo readerInfo = funcInfo->GetReaderInfo();
    if (m_end < m_start || readerInfo.startOffset >= (size_t)(m_end - m_start))
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
#if ENABLE_DEBUG_CONFIG_OPTIONS
    m_funcState.body = funcInfo->GetBody();
    if (DO_WASM_TRACE_DECODER)
    {
        Output::Print(_u("Decoding "));
        m_funcState.body->DumpFullFunctionName();
        if (sizeof(intptr_t) == 8)
        {
            Output::Print(_u(": start = 0x%llx, end = 0x%llx, size = 0x%x\n"), (intptr_t)m_pc, (intptr_t)m_curFuncEnd, m_funcState.size);
        }
        else
        {
            Output::Print(_u(": start = 0x%x, end = 0x%x, size = 0x%x\n"), (intptr_t)m_pc, (intptr_t)m_curFuncEnd, m_funcState.size);
        }
    }
#endif

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
#if ENABLE_DEBUG_CONFIG_OPTIONS
    m_funcState.body = nullptr;
#endif
}

bool WasmBinaryReader::IsCurrentFunctionCompleted() const
{
    return m_pc == m_curFuncEnd;
}


uint32 WasmBinaryReader::EstimateCurrentFunctionBytecodeSize() const
{
    if (m_readerState != READER_STATE_FUNCTION)
    {
        ThrowDecodingError(_u("Wasm reader in an invalid state to get function information"));
    }
    // Use binary size to estimate bytecode size
    return m_funcState.size;
}

WasmOp WasmBinaryReader::ReadPrefixedOpCode(WasmOp prefix, bool isSupported, const char16* notSupportedMsg)
{
    CompileAssert(sizeof(WasmOp) >= 2);
    if (!isSupported)
    {
        ThrowDecodingError(notSupportedMsg);
    }
    CheckBytesLeft(1);
    ++m_funcState.count;
    return (WasmOp)((prefix << 8) | (*m_pc++));
}

WasmOp WasmBinaryReader::ReadOpCode()
{
    CheckBytesLeft(1);
    WasmOp op = (WasmOp)*m_pc++;
    ++m_funcState.count;

    switch (op)
    {
#define WASM_PREFIX(name, value, imp, errorMsg) \
    case prefix##name: \
        return ReadPrefixedOpCode(op, imp, _u(errorMsg));
#include "WasmBinaryOpCodes.h"
    }

    return op;
}

WasmOp WasmBinaryReader::ReadExpr()
{
    WasmOp op = m_currentNode.op = ReadOpCode();

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
#ifdef ENABLE_WASM_SIMD
    case wbV128Const:
        ConstNode<WasmTypes::V128>();
        break;
#endif
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
    case wbMemorySize:
    case wbMemoryGrow:
    {
        // Reserved value currently unused
        uint8 reserved = ReadConst<uint8>();
        if (reserved != 0)
        {
            ThrowDecodingError(op == wbMemorySize
                ? _u("memory.size reserved value must be 0")
                : _u("memory.grow reserved value must be 0")
            );
        }
        break;
    }
#ifdef ENABLE_WASM_SIMD
    case wbV8X16Shuffle:
        ShuffleNode();
        break;
#define WASM_LANE_OPCODE(opname, ...) \
    case wb##opname: \
        LaneNode(); \
        break;
#endif
#define WASM_MEM_OPCODE(opname, ...) \
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
    uint32 len = 0;
    int64 blocktype = SLEB128<int64, 33>(len);
    m_funcState.count += len;

    // Negative values means it's a value type or Void
    if (blocktype < 0)
    {
        // Only support 1 byte for negative values
        if (len != 1)
        {
            ThrowDecodingError(_u("Invalid blocktype %lld"), blocktype);
        }
        int8 blockSig = (int8)blocktype;
        // Check if it's an empty block
        if (blockSig == LanguageTypes::emptyBlock)
        {
            m_currentNode.block.SetSingleResult(WasmTypes::Void);
        }
        else
        {
            // Otherwise, it should be a value type
            WasmTypes::WasmType type = LanguageTypes::ToWasmType(blockSig);
            m_currentNode.block.SetSingleResult(type);
        }
    }
    else
    {
        // Positive values means it a function signature
        // Function signature is only supported with the Multi-Value feature
        if (!CONFIG_FLAG(WasmMultiValue))
        {
            ThrowDecodingError(_u("Block signature not supported"));
        }

        // Make sure the blocktype is a u32 after checking for negative values
        if (blocktype > UINT32_MAX)
        {
            ThrowDecodingError(_u("Invalid blocktype %lld"), blocktype);
        }
        uint32 blockId = (uint32)blocktype;
        if (!m_module->IsSignatureIndexValid(blockId))
        {
            ThrowDecodingError(_u("Block signature %u is invalid"), blockId);
        }
        m_currentNode.block.SetSignatureId(blockId);
    }
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

void WasmBinaryReader::ShuffleNode()
{
    CheckBytesLeft(Simd::MAX_LANES);
    for (uint32 i = 0; i < Simd::MAX_LANES; i++)
    {
        m_currentNode.shuffle.indices[i] = ReadConst<uint8>();
    }
    m_funcState.count += Simd::MAX_LANES;
}

void WasmBinaryReader::LaneNode()
{
    m_currentNode.lane.index = ReadConst<uint8>();
    m_funcState.count++;
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
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::V128:
        Simd::EnsureSimdIsEnabled();
        for (uint i = 0; i < Simd::VEC_WIDTH; i++) 
        {
            m_currentNode.cnst.v128[i] = ReadConst<uint>();
        }
        m_funcState.count += Simd::VEC_WIDTH;
        break;
#endif
    default:
        WasmTypes::CompileAssertCases<Wasm::WasmTypes::I32, Wasm::WasmTypes::I64, Wasm::WasmTypes::F32, Wasm::WasmTypes::F64, WASM_V128_CHECK_TYPE>();
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
        MemorySectionLimits limits = ReadSectionLimitsBase<MemorySectionLimits>(Limits::GetMaxMemoryInitialPages(), Limits::GetMaxMemoryMaximumPages(), _u("memory size too big"));
        if (Wasm::Threads::IsEnabled() && limits.IsShared() && !limits.HasMaximum())
        {
            ThrowDecodingError(_u("Shared memory must have a maximum size"));
        }
        m_module->InitializeMemory(&limits);
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
        int8 form = SLEB128<int8, 7>(len);
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
        if (resultCount > Limits::GetMaxFunctionReturns())
        {
            ThrowDecodingError(_u("Too many returns in signature: %u. Maximum allowed: %u"), resultCount, Limits::GetMaxFunctionReturns());
        }
        sig->AllocateResults(resultCount, m_module->GetRecycler());
        for (uint32 iResult = 0; iResult < resultCount; ++iResult)
        {
            WasmTypes::WasmType type = ReadWasmType(len);
            sig->SetResult(type, iResult);
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

        ExternalKinds kind = ReadExternalKind();
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
        int8 elementType = SLEB128<int8, 7>(length);
        if (elementType != LanguageTypes::anyfunc)
        {
            ThrowDecodingError(_u("Only anyfunc type is supported. Unknown type %d"), elementType);
        }
        TableSectionLimits limits = ReadSectionLimitsBase<TableSectionLimits>(Limits::GetMaxTableSize(), Limits::GetMaxTableSize(), _u("table too big"));
        m_module->InitializeTable(&limits);
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
        
        if (!WasmTypes::IsLocalType(type))
        {
            ThrowDecodingError(_u("Invalid global type %u"), type);
        }
        switch (type)
        {
        case WasmTypes::I32: break; // Handled
        case WasmTypes::I64: break; // Handled
        case WasmTypes::F32: break; // Handled
        case WasmTypes::F64: break; // Handled
#ifdef ENABLE_WASM_SIMD
        case WasmTypes::V128: ThrowDecodingError(_u("v128 globals not supported"));
#endif
        default:
            WasmTypes::CompileAssertCases<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_V128_CHECK_TYPE>();
        }
        
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

        ExternalKinds kind = ReadExternalKind();
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
    if (sig->GetParamCount() > 0 || sig->GetResultCount())
    {
        ThrowDecodingError(_u("Start function must be void and nullary"));
    }
    m_module->SetStartFunction(id);
}

template<typename LEBType, uint32 bits>
LEBType WasmBinaryReader::LEB128(uint32 &length)
{
    CompileAssert((sizeof(LEBType) * 8) >= bits);
    constexpr bool sign = LEBType(-1) < LEBType(0);
    LEBType result = 0;
    uint32 shift = 0;
    byte b = 0x80;
    length = 0;
    constexpr uint32 maxBytes = (uint32)((bits + 6) / 7);
    CompileAssert(maxBytes > 0);

    uint32 iByte = 0;
    for (; iByte < maxBytes && (b & 0x80) == 0x80; ++iByte)
    {
        CheckBytesLeft(1);
        b = *m_pc++;
        result = result | ((LEBType)(b & 0x7f) << shift);
        shift += 7;
    }

    if ((b & 0x80) == 0x80)
    {
        ThrowDecodingError(_u("Invalid LEB128 format"));
    }

    const bool isLastByte = iByte == maxBytes;
    constexpr bool hasExtraBits = (maxBytes * 7) != bits;
    if (hasExtraBits && isLastByte)
    {
        // A signed-LEB128 must sign-extend the final byte, excluding its most-significant bit
        // For unsigned values, the extra bits must be all zero.
        // For signed values, the extra bits *plus* the most significant bit must either be 0, or all ones.

        // e.g. for a 32-bit LEB128:
        //   bitsInLastByte = 4  (== 32 - (5-1) * 7)
        //     0bYYYXXXX where Y are extra bits and X are bits included in LEB128
        //   if signed: check if 0bYYYXXXX is 0b0000XXX or 0b1111XXX
        //   is unsigned: check if 0bYYYXXXX is 0b000XXXX

        constexpr int bitsInLastByte = bits - (maxBytes - 1) * 7;
        constexpr int lastBitToCheck = bitsInLastByte - (sign ? 1 : 0);
        constexpr byte signExtendedExtraBits = 0x7f & (0xFF << lastBitToCheck);
        const byte checkedBits = b & (0xFF << lastBitToCheck);
        bool validExtraBits =
            checkedBits == 0 ||
            (sign && checkedBits == signExtendedExtraBits);
        if (!validExtraBits)
        {
            ThrowDecodingError(_u("Invalid LEB128 format"));
        }
    }

    length = iByte;
    if (sign)
    {
        // Perform sign extension
        const int signExtendShift = (sizeof(LEBType) * 8) - shift;
        if (signExtendShift > 0)
        {
            result = static_cast<LEBType>(result << signExtendShift) >> signExtendShift;
        }
    }
    return result;
}

WasmNode WasmBinaryReader::ReadInitExpr(bool isOffset)
{
    if (m_readerState != READER_STATE_MODULE)
    {
        ThrowDecodingError(_u("Wasm reader in an invalid state to read init_expr"));
    }

    m_funcState.count = 0;
    m_funcState.size = (uint32)(m_currentSection.end - m_pc);
#if TARGET_64
    Assert(m_pc + m_funcState.size == m_currentSection.end);
#endif
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

template<typename SectionLimitType>
SectionLimitType WasmBinaryReader::ReadSectionLimitsBase(uint32 maxInitial, uint32 maxMaximum, const char16* errorMsg)
{
    SectionLimitType limits;
    uint32 length = 0;
    limits.flags = (SectionLimits::Flags)LEB128(length);
    limits.initial = LEB128(length);
    limits.maximum = maxMaximum;
    if (limits.HasMaximum())
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
    return LanguageTypes::ToWasmType(SLEB128<int8, 7>(length));
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
