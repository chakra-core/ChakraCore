//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmModule::WasmModule(Js::ScriptContext* scriptContext, byte* binaryBuffer, uint binaryBufferLength) :
    m_memory(),
    m_alloc(_u("WasmModule"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_functionsInfo(nullptr),
    m_funcCount(0),
    m_importCount(0),
    m_equivalentSignatureMap(nullptr),
    m_indirectfuncs(nullptr),
    m_indirectFuncCount(0),
    m_exports(nullptr),
    m_exportCount(0),
    m_datasegCount(0),
    m_signatures(nullptr),
    m_signaturesCount(0),
    m_startFuncIndex(Js::Constants::UninitializedValue),
    globalCounts {0, 0, 0, 0, 0}, //the first elm is the number of Vars in front of I32; makes for a nicer offset computation
    globals (&m_alloc)
{
    m_reader = Anew(&m_alloc, WasmBinaryReader, &m_alloc, this, binaryBuffer, binaryBufferLength);
    m_reader->InitializeReader();
}

uint32
WasmModule::GetMaxFunctionIndex() const
{
    return UInt32Math::Add(GetImportCount(), GetWasmFunctionCount());
}

Wasm::WasmSignature*
WasmModule::GetFunctionSignature(uint32 funcIndex) const
{
    FunctionIndexTypes::Type funcType = GetFunctionIndexType(funcIndex);
    if (funcType == FunctionIndexTypes::Invalid)
    {
        throw WasmCompilationException(_u("Function index out of range"));
    }

    funcIndex = NormalizeFunctionIndex(funcIndex);
    switch (funcType)
    {
    case FunctionIndexTypes::Import:
        return GetSignature(GetFunctionImport(funcIndex)->sigId);
    case FunctionIndexTypes::Function:
        return GetWasmFunctionInfo(funcIndex)->GetSignature();
    default:
        throw WasmCompilationException(_u("Unknown function index type"));
    }
}

FunctionIndexTypes::Type
WasmModule::GetFunctionIndexType(uint32 funcIndex) const
{
    if (funcIndex >= GetMaxFunctionIndex())
    {
        return FunctionIndexTypes::Invalid;
    }
    if (funcIndex < GetImportCount())
    {
        return FunctionIndexTypes::Import;
    }
    return FunctionIndexTypes::Function;
}

uint32
WasmModule::NormalizeFunctionIndex(uint32 funcIndex) const
{
    switch (GetFunctionIndexType(funcIndex))
    {
    case FunctionIndexTypes::Import:
        return funcIndex;
    case FunctionIndexTypes::Function:
        Assert(funcIndex >= GetImportCount());
        return funcIndex - GetImportCount();
    default:
        throw WasmCompilationException(_u("Failed function index normalization: function index out of range"));
    }
}

void
WasmModule::InitializeMemory(uint32 minPage, uint32 maxPage)
{
    if (m_memory.minSize != 0)
    {
        throw WasmCompilationException(_u("Memory already allocated"));
    }

    if (maxPage < minPage)
    {
        throw WasmCompilationException(_u("Memory: MaxPage (%d) must be greater than MinPage (%d)"), maxPage, minPage);
    }

    CompileAssert(Memory::PAGE_SIZE < INT_MAX);
    m_memory.minSize = (uint64)minPage * Memory::PAGE_SIZE;
    m_memory.maxSize = (uint64)maxPage * Memory::PAGE_SIZE;
}

const WasmModule::Memory *
WasmModule::GetMemory() const
{
    return &m_memory;
}

void
WasmModule::SetSignature(uint32 index, WasmSignature * signature)
{
    Assert(index < GetSignatureCount());
    signature->SetSignatureId(index);
    m_signatures[index] = signature;
}

WasmSignature *
WasmModule::GetSignature(uint32 index) const
{
    if (index >= GetSignatureCount())
    {
        throw WasmCompilationException(_u("Invalid signature index %u"), index);
    }

    return m_signatures[index];
}

uint32
WasmModule::GetSignatureCount() const
{
    return m_signaturesCount;
}

void
WasmModule::CalculateEquivalentSignatures()
{
    Assert(m_equivalentSignatureMap == nullptr);
    uint32 sigCount = GetSignatureCount();
    m_equivalentSignatureMap = AnewArray(&m_alloc, uint32, sigCount);
    memset(m_equivalentSignatureMap, -1, sigCount * sizeof(uint32));

    const auto IsEquivalentSignatureSet = [this](uint32 sigId)
    {
        return m_equivalentSignatureMap[sigId] != (uint32)-1;
    };

    for (uint32 iSig = 0; iSig < sigCount; iSig++)
    {
        if (!IsEquivalentSignatureSet(iSig))
        {
            m_equivalentSignatureMap[iSig] = iSig;
            WasmSignature* sig = GetSignature(iSig);
            // todo:: Find a better way than O(n^2) algo here
            for (uint32 iSig2 = iSig + 1; iSig2 < sigCount; iSig2++)
            {
                if (!IsEquivalentSignatureSet(iSig2) && sig->IsEquivalent(GetSignature(iSig2)))
                {
                    m_equivalentSignatureMap[iSig2] = iSig;
                }
            }
        }
    }
}

uint32
WasmModule::GetEquivalentSignatureId(uint32 sigId) const
{
    if (m_equivalentSignatureMap && sigId < GetSignatureCount())
    {
        return m_equivalentSignatureMap[sigId];
    }
    Assert(UNREACHED);
    return sigId;
}

void
WasmModule::AllocateTable(uint32 entries)
{
    m_indirectFuncCount = entries;
    m_indirectfuncs = AnewArray(&m_alloc, uint32, entries);
    for (uint32 i = 0; i < entries ; i++)
    {
        // Initialize with invalid index
        m_indirectfuncs[i] = Js::Constants::UninitializedValue;
    }
}

void
WasmModule::SetTableValue(uint32 funcIndex, uint32 tableIndex)
{
    if (tableIndex < GetTableSize())
    {
        m_indirectfuncs[tableIndex] = funcIndex;
    }
}

uint32
WasmModule::GetTableValue(uint32 tableIndex) const
{
    if (tableIndex >= GetTableSize())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs[tableIndex];
}

uint32
WasmModule::GetTableSize() const
{
    return m_indirectFuncCount;
}

uint32
WasmModule::GetWasmFunctionCount() const
{
    return m_funcCount;
}

void WasmModule::AllocateWasmFunctions(uint32 count)
{
    m_funcCount = count;
    if (count > 0)
    {
        m_functionsInfo = AnewArray(&m_alloc, WasmFunctionInfo*, count);
    }
}

bool
WasmModule::SetWasmFunctionInfo(WasmFunctionInfo* funcInfo, uint32 index)
{
    if (index < m_funcCount)
    {
        m_functionsInfo[index] = funcInfo;
        Assert(funcInfo->GetNumber() == index);
        return true;
    }
    return false;
}

WasmFunctionInfo*
WasmModule::GetWasmFunctionInfo(uint index) const
{
    if (index >= m_funcCount)
    {
        throw WasmCompilationException(_u("Invalid function index %u"), index);
    }

    return m_functionsInfo[index];
}

void WasmModule::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(&m_alloc, WasmExport, entries);
    m_exportCount = entries;
}

void WasmModule::SetExport(uint32 iExport, uint32 funcIndex, char16* exportName, uint32 nameLength, ExternalKinds::ExternalKind kind)
{
    m_exports[iExport].funcIndex = funcIndex;
    m_exports[iExport].nameLength = nameLength;
    m_exports[iExport].name = exportName;
    m_exports[iExport].kind = kind;
}

Wasm::WasmExport* WasmModule::GetFunctionExport(uint32 iExport) const
{
    if (iExport >= m_exportCount)
    {
        return nullptr;
    }
    return &m_exports[iExport];
}

void
WasmModule::AllocateFunctionImports(uint32 entries)
{
    m_imports = AnewArrayZ(&m_alloc, WasmImport, entries);
    m_importCount = entries;
}

void
WasmModule::SetFunctionImport(uint32 i, uint32 sigId, char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen, ExternalKinds::ExternalKind kind)
{
    m_imports[i].sigId = sigId;
    m_imports[i].modNameLen = modNameLen;
    m_imports[i].modName = modName;
    m_imports[i].fnNameLen = fnNameLen;
    m_imports[i].fnName = fnName;
}

void
WasmModule::AddGlobalImport(char16* modName, uint32 modNameLen, char16* fnName, uint32 fnNameLen, ExternalKinds::ExternalKind kind, WasmGlobal* importedGlobal)
{
    WasmImport* wi = Anew(&m_alloc, WasmImport);
    wi->sigId = 0;
    wi->fnName = fnName;
    wi->fnNameLen = fnNameLen;
    wi->modName = modName;
    wi->modNameLen = modNameLen;

    importedGlobal->importVar = wi;
    importedGlobal->SetReferenceType(WasmGlobal::ImportedReference);
    globals.Add(importedGlobal);
}

Wasm::WasmImport*
WasmModule::GetFunctionImport(uint32 i) const
{
    if (i >= m_importCount)
    {
        throw WasmCompilationException(_u("Import function index out of range"));
    }
    return &m_imports[i];
}

void
WasmModule::AllocateDataSegs(uint32 count)
{
    Assert(count != 0);
    m_datasegCount = count;
    m_datasegs = AnewArray(&m_alloc, WasmDataSegment*, count);
}

bool
WasmModule::AddDataSeg(WasmDataSegment* seg, uint32 index)
{
    if (index >= m_datasegCount)
    {
        return false;
    }
    m_datasegs[index] = seg;
    return true;
}

WasmDataSegment*
WasmModule::GetDataSeg(uint32 index) const
{
    if (index >= m_datasegCount)
    {
        return nullptr;
    }
    return m_datasegs[index];
}

void
WasmModule::SetStartFunction(uint32 i)
{
    if (i >= m_funcCount) {
        TRACE_WASM_DECODER(_u("Invalid start function index"));
        return;
    }
    m_startFuncIndex = i;
}

uint32
WasmModule::GetStartFunction() const
{
    return m_startFuncIndex;
}

void WasmModule::SetSignatureCount(uint32 count)
{
    Assert(m_signaturesCount == 0 && m_signatures == nullptr);
    m_signaturesCount = count;
    m_signatures = AnewArray(&m_alloc, WasmSignature*, count);
}

uint32 WasmModule::GetModuleEnvironmentSize() const
{
    static const uint DOUBLE_SIZE_IN_INTS = sizeof(double) / sizeof(int);
    // 1 for the heap
    uint32 size = 1;
    size = UInt32Math::Add(size, GetWasmFunctionCount());
    // reserve space for as many function tables as there are signatures, though we won't fill them all
    size = UInt32Math::Add(size, GetSignatureCount());
    size = UInt32Math::Add(size, GetImportCount());
    size = UInt32Math::Add(size, WAsmJs::ConvertToJsVarOffset<byte>(GetGlobalsByteSize()));
    return size;
}

void WasmModule::Finalize(bool isShutdown)
{
    m_alloc.Clear();
}

void WasmModule::Dispose(bool isShutdown)
{
    Assert(m_alloc.Size() == 0);
}

void WasmModule::Mark(Recycler * recycler)
{
}

uint WasmModule::GetOffsetForGlobal(WasmGlobal* global)
{
    WasmTypes::WasmType type = global->GetType();
    if (type >= WasmTypes::Limit)
    {
        throw WasmCompilationException(_u("Invalid Global type"));
    }

    uint32 offset = WAsmJs::ConvertFromJsVarOffset<byte>(GetGlobalOffset());

    for (uint i = 1; i < (uint)type; i++)
    {
        offset = AddGlobalByteSizeToOffset((WasmTypes::WasmType)i, offset);
    }

    uint32 typeSize = WasmTypes::GetTypeByteSize(type);
    uint32 sizeUsed = WAsmJs::ConvertOffset<byte>(global->GetOffset(), typeSize);
    offset = UInt32Math::Add(offset, sizeUsed);
    return WAsmJs::ConvertOffset(offset, sizeof(byte), typeSize);
}

uint WasmModule::AddGlobalByteSizeToOffset(WasmTypes::WasmType type, uint32 offset) const
{
    uint32 typeSize = WasmTypes::GetTypeByteSize(type);
    offset = Math::AlignOverflowCheck(offset, typeSize);
    uint32 sizeUsed = WAsmJs::ConvertOffset<byte>(globalCounts[type], typeSize);
    return UInt32Math::Add(offset, sizeUsed);
}

uint WasmModule::GetGlobalsByteSize() const
{
    uint32 size = 0;
    for (WasmTypes::WasmType type = (WasmTypes::WasmType)(WasmTypes::Void + 1); type < WasmTypes::Limit; type = (WasmTypes::WasmType)(type + 1))
    {
        size = AddGlobalByteSizeToOffset(type, size);
    }
    return size;
}

} // namespace Wasm

#endif // ENABLE_WASM
