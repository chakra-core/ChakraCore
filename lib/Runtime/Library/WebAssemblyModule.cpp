//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM

#include "../WasmReader/WasmReaderPch.h"

namespace Js
{
WebAssemblyModule::WebAssemblyModule(Js::ScriptContext* scriptContext, const byte* binaryBuffer, uint binaryBufferLength, DynamicType * type) :
    DynamicObject(type),
    m_memory(nullptr),
    m_alloc(_u("WebAssemblyModule"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_functionsInfo(nullptr),
    m_funcCount(0),
    m_importCount(0),
    m_indirectfuncs(nullptr),
    m_indirectFuncCount(0),
    m_exports(nullptr),
    m_exportCount(0),
    m_datasegCount(0),
    m_signatures(nullptr),
    m_signaturesCount(0),
    m_startFuncIndex(Js::Constants::UninitializedValue),
    isMemExported(false),
    m_binaryBuffer(binaryBuffer)
{
    //the first elm is the number of Vars in front of I32; makes for a nicer offset computation
    memset(globalCounts, 0, sizeof(uint) * Wasm::WasmTypes::Limit);
    globals = Anew(&m_alloc, WasmGlobalsList, &m_alloc);
    m_reader = Anew(&m_alloc, Wasm::WasmBinaryReader, &m_alloc, this, binaryBuffer, binaryBufferLength);
}

/* static */
bool
WebAssemblyModule::Is(Var value)
{
    return JavascriptOperators::GetTypeId(value) == TypeIds_WebAssemblyModule;
}

/* static */
WebAssemblyModule *
WebAssemblyModule::FromVar(Var value)
{
    Assert(WebAssemblyModule::Is(value));
    return static_cast<WebAssemblyModule*>(value);
}

/* static */
Var
WebAssemblyModule::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    ScriptContext* scriptContext = function->GetScriptContext();

    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

    Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
    bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
    Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr);

    if (!(callInfo.Flags & CallFlags_New) || (newTarget && JavascriptOperators::IsUndefinedObject(newTarget)))
    {
        JavascriptError::ThrowTypeError(scriptContext, JSERR_ClassConstructorCannotBeCalledWithoutNew, _u("WebAssembly.Module"));
    }

    if (args.Info.Count < 2)
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedBufferSource);
    }
    BYTE* buffer;
    uint byteLength;
    WebAssembly::ReadBufferSource(args[1], scriptContext, &buffer, &byteLength);

    return CreateModule(scriptContext, buffer, byteLength);
}

/* static */
WebAssemblyModule *
WebAssemblyModule::CreateModule(
    ScriptContext* scriptContext,
    const byte* buffer,
    const uint lengthBytes)
{
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmPhase);
    Unused(wasmPhase);

    WebAssemblyModule * webAssemblyModule = nullptr;
    Wasm::WasmReaderInfo * readerInfo = nullptr;
    Js::FunctionBody * currentBody = nullptr;
    try
    {
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        SRCINFO const * srcInfo = scriptContext->cache->noContextGlobalSourceInfo;
        Js::Utf8SourceInfo* utf8SourceInfo = Utf8SourceInfo::New(scriptContext, (LPCUTF8)buffer, lengthBytes / sizeof(char16), lengthBytes, srcInfo, false);

        // copy buffer so external changes to it don't cause issues when defer parsing
        byte* newBuffer = RecyclerNewArray(scriptContext->GetRecycler(), byte, lengthBytes);
        js_memcpy_s(newBuffer, lengthBytes, buffer, lengthBytes);

        Wasm::WasmModuleGenerator bytecodeGen(scriptContext, utf8SourceInfo, newBuffer, lengthBytes);

        webAssemblyModule = bytecodeGen.GenerateModule();

        for (uint i = 0; i < webAssemblyModule->GetWasmFunctionCount(); ++i)
        {
            currentBody = webAssemblyModule->GetWasmFunctionInfo(i)->GetBody();
            if (!PHASE_OFF(WasmDeferredPhase, currentBody))
            {
                continue;
            }
            readerInfo = currentBody->GetAsmJsFunctionInfo()->GetWasmReaderInfo();

            Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);
        }
    }
    catch (Wasm::WasmCompilationException& ex)
    {
        Wasm::WasmCompilationException newEx = ex;
        if (currentBody != nullptr)
        {
            char16* originalMessage = ex.ReleaseErrorMessage();
            intptr_t offset = readerInfo->m_module->GetReader()->GetCurrentOffset();
            intptr_t start = readerInfo->m_funcInfo->m_readerInfo.startOffset;
            uint32 size = readerInfo->m_funcInfo->m_readerInfo.size;

            newEx = Wasm::WasmCompilationException(
                _u("function %s at offset %d/%d: %s"),
                currentBody->GetDisplayName(),
                offset - start,
                size,
                originalMessage
            );
            currentBody->GetAsmJsFunctionInfo()->SetWasmReaderInfo(nullptr);
            SysFreeString(originalMessage);
        }
        JavascriptLibrary *library = scriptContext->GetLibrary();
        JavascriptError *pError = library->CreateWebAssemblyCompileError();
        JavascriptError::SetErrorMessage(pError, JSERR_WasmCompileError, newEx.ReleaseErrorMessage(), scriptContext);
        JavascriptExceptionOperators::Throw(pError, scriptContext);
    }

    return webAssemblyModule;
}

/* static */
bool
WebAssemblyModule::ValidateModule(
    ScriptContext* scriptContext,
    const byte* buffer,
    const uint lengthBytes)
{
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmPhase);
    Unused(wasmPhase);

    try
    {
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        SRCINFO const * srcInfo = scriptContext->cache->noContextGlobalSourceInfo;
        Js::Utf8SourceInfo* utf8SourceInfo = Utf8SourceInfo::New(scriptContext, (LPCUTF8)buffer, lengthBytes / sizeof(char16), lengthBytes, srcInfo, false);

        Wasm::WasmModuleGenerator bytecodeGen(scriptContext, utf8SourceInfo, (byte*)buffer, lengthBytes);

        WebAssemblyModule * webAssemblyModule = bytecodeGen.GenerateModule();

        for (uint i = 0; i < webAssemblyModule->GetWasmFunctionCount(); ++i)
        {
            Js::FunctionBody * body = webAssemblyModule->GetWasmFunctionInfo(i)->GetBody();
            Wasm::WasmReaderInfo * readerInfo = body->GetAsmJsFunctionInfo()->GetWasmReaderInfo();

            // TODO: avoid actually generating bytecode here
            Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);
        }
    }
    catch (Wasm::WasmCompilationException& ex)
    {
        char16* originalMessage = ex.ReleaseErrorMessage();
        SysFreeString(originalMessage);

        return false;
    }

    return true;
}

uint32
WebAssemblyModule::GetMaxFunctionIndex() const
{
    return UInt32Math::Add(GetImportCount(), GetWasmFunctionCount());
}

Wasm::WasmSignature*
WebAssemblyModule::GetFunctionSignature(uint32 funcIndex) const
{
    Wasm::FunctionIndexTypes::Type funcType = GetFunctionIndexType(funcIndex);
    if (funcType == Wasm::FunctionIndexTypes::Invalid)
    {
        throw Wasm::WasmCompilationException(_u("Function index out of range"));
    }

    funcIndex = NormalizeFunctionIndex(funcIndex);
    switch (funcType)
    {
    case Wasm::FunctionIndexTypes::Import:
        return GetSignature(GetFunctionImport(funcIndex)->sigId);
    case Wasm::FunctionIndexTypes::Function:
        return GetWasmFunctionInfo(funcIndex)->GetSignature();
    default:
        throw Wasm::WasmCompilationException(_u("Unknown function index type"));
    }
}

Wasm::FunctionIndexTypes::Type
WebAssemblyModule::GetFunctionIndexType(uint32 funcIndex) const
{
    if (funcIndex >= GetMaxFunctionIndex())
    {
        return Wasm::FunctionIndexTypes::Invalid;
    }
    if (funcIndex < GetImportCount())
    {
        return Wasm::FunctionIndexTypes::Import;
    }
    return Wasm::FunctionIndexTypes::Function;
}

uint32
WebAssemblyModule::NormalizeFunctionIndex(uint32 funcIndex) const
{
    switch (GetFunctionIndexType(funcIndex))
    {
    case Wasm::FunctionIndexTypes::Import:
        return funcIndex;
    case Wasm::FunctionIndexTypes::Function:
        Assert(funcIndex >= GetImportCount());
        return funcIndex - GetImportCount();
    default:
        throw Wasm::WasmCompilationException(_u("Failed function index normalization: function index out of range"));
    }
}

void
WebAssemblyModule::InitializeMemory(uint32 minPage, uint32 maxPage)
{
    if (m_memory != nullptr)
    {
        throw Wasm::WasmCompilationException(_u("Memory already allocated"));
    }

    if (maxPage < minPage)
    {
        throw Wasm::WasmCompilationException(_u("Memory: MaxPage (%d) must be greater than MinPage (%d)"), maxPage, minPage);
    }

    m_memory = WebAssemblyMemory::CreateMemoryObject(minPage, maxPage, GetScriptContext());
}

WebAssemblyMemory *
WebAssemblyModule::GetMemory() const
{
    return m_memory;
}

void
WebAssemblyModule::SetSignature(uint32 index, Wasm::WasmSignature * signature)
{
    Assert(index < GetSignatureCount());
    signature->SetSignatureId(index);
    m_signatures[index] = signature;
}

Wasm::WasmSignature *
WebAssemblyModule::GetSignature(uint32 index) const
{
    if (index >= GetSignatureCount())
    {
        throw Wasm::WasmCompilationException(_u("Invalid signature index %u"), index);
    }

    return m_signatures[index];
}

uint32
WebAssemblyModule::GetSignatureCount() const
{
    return m_signaturesCount;
}

void
WebAssemblyModule::CalculateEquivalentSignatures()
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
            Wasm::WasmSignature* sig = GetSignature(iSig);
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
WebAssemblyModule::GetEquivalentSignatureId(uint32 sigId) const
{
    if (m_equivalentSignatureMap && sigId < GetSignatureCount())
    {
        return m_equivalentSignatureMap[sigId];
    }
    Assert(UNREACHED);
    return sigId;
}

void
WebAssemblyModule::AllocateTable(uint32 entries)
{
    m_indirectFuncCount = entries;
    m_indirectfuncs = AnewArray(&m_alloc, uint32, entries);
    for (uint32 i = 0; i < entries; i++)
    {
        // Initialize with invalid index
        m_indirectfuncs[i] = Js::Constants::UninitializedValue;
    }
}

void
WebAssemblyModule::SetTableValue(uint32 funcIndex, uint32 tableIndex)
{
    if (tableIndex < GetTableSize())
    {
        m_indirectfuncs[tableIndex] = funcIndex;
    }
}

uint32
WebAssemblyModule::GetTableValue(uint32 tableIndex) const
{
    if (tableIndex >= GetTableSize())
    {
        return Js::Constants::InvalidSourceIndex;
    }
    return m_indirectfuncs[tableIndex];
}

uint32
WebAssemblyModule::GetTableSize() const
{
    return m_indirectFuncCount;
}

uint32
WebAssemblyModule::GetWasmFunctionCount() const
{
    return m_funcCount;
}

void WebAssemblyModule::AllocateWasmFunctions(uint32 count)
{
    m_funcCount = count;
    if (count > 0)
    {
        m_functionsInfo = RecyclerNewArrayZ(GetRecycler(), Wasm::WasmFunctionInfo*, count);
    }
}

bool
WebAssemblyModule::SetWasmFunctionInfo(Wasm::WasmSignature* sig, uint32 index)
{
    if (index < m_funcCount)
    {
        // must be recycler memory, since it holds reference to the function body
        m_functionsInfo[index] = RecyclerNew(GetRecycler(), Wasm::WasmFunctionInfo, &m_alloc, sig, index);
        return true;
    }
    return false;
}

Wasm::WasmFunctionInfo*
WebAssemblyModule::GetWasmFunctionInfo(uint index) const
{
    if (index >= m_funcCount)
    {
        throw Wasm::WasmCompilationException(_u("Invalid function index %u"), index);
    }

    return m_functionsInfo[index];
}

void WebAssemblyModule::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(&m_alloc, Wasm::WasmExport, entries);
    m_exportCount = entries;
}

void WebAssemblyModule::SetExport(uint32 iExport, uint32 funcIndex, const char16* exportName, uint32 nameLength, Wasm::ExternalKinds::ExternalKind kind)
{
    m_exports[iExport].funcIndex = funcIndex;
    m_exports[iExport].nameLength = nameLength;
    m_exports[iExport].name = exportName;
    m_exports[iExport].kind = kind;
}

Wasm::WasmExport* WebAssemblyModule::GetFunctionExport(uint32 iExport) const
{
    if (iExport >= m_exportCount)
    {
        return nullptr;
    }
    return &m_exports[iExport];
}

void
WebAssemblyModule::AllocateFunctionImports(uint32 entries)
{
    m_imports = AnewArrayZ(&m_alloc, Wasm::WasmImport, entries);
    m_importCount = entries;
}

void
WebAssemblyModule::SetFunctionImport(uint32 i, uint32 sigId, const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen, Wasm::ExternalKinds::ExternalKind kind)
{
    m_imports[i].sigId = sigId;
    m_imports[i].modNameLen = modNameLen;
    m_imports[i].modName = modName;
    m_imports[i].fnNameLen = fnNameLen;
    m_imports[i].fnName = fnName;
}

void
WebAssemblyModule::AddGlobalImport(const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen, Wasm::ExternalKinds::ExternalKind kind, Wasm::WasmGlobal* importedGlobal)
{
    Wasm::WasmImport* wi = Anew(&m_alloc, Wasm::WasmImport);
    wi->sigId = 0;
    wi->fnName = fnName;
    wi->fnNameLen = fnNameLen;
    wi->modName = modName;
    wi->modNameLen = modNameLen;

    importedGlobal->importVar = wi;
    importedGlobal->SetReferenceType(Wasm::WasmGlobal::ImportedReference);
    globals->Add(importedGlobal);
}

Wasm::WasmImport*
WebAssemblyModule::GetFunctionImport(uint32 i) const
{
    if (i >= m_importCount)
    {
        throw Wasm::WasmCompilationException(_u("Import function index out of range"));
    }
    return &m_imports[i];
}

void
WebAssemblyModule::AllocateDataSegs(uint32 count)
{
    Assert(count != 0);
    m_datasegCount = count;
    m_datasegs = AnewArray(&m_alloc, Wasm::WasmDataSegment*, count);
}

bool
WebAssemblyModule::AddDataSeg(Wasm::WasmDataSegment* seg, uint32 index)
{
    if (index >= m_datasegCount)
    {
        return false;
    }
    m_datasegs[index] = seg;
    return true;
}

Wasm::WasmDataSegment*
WebAssemblyModule::GetDataSeg(uint32 index) const
{
    if (index >= m_datasegCount)
    {
        return nullptr;
    }
    return m_datasegs[index];
}

void
WebAssemblyModule::SetStartFunction(uint32 i)
{
    if (i >= m_funcCount) {
        TRACE_WASM_DECODER(_u("Invalid start function index"));
        return;
    }
    m_startFuncIndex = i;
}

uint32
WebAssemblyModule::GetStartFunction() const
{
    return m_startFuncIndex;
}

void WebAssemblyModule::SetSignatureCount(uint32 count)
{
    Assert(m_signaturesCount == 0 && m_signatures == nullptr);
    m_signaturesCount = count;
    m_signatures = AnewArray(&m_alloc, Wasm::WasmSignature*, count);
}

uint32 WebAssemblyModule::GetModuleEnvironmentSize() const
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

void WebAssemblyModule::Finalize(bool isShutdown)
{
    m_alloc.Clear();
}

void WebAssemblyModule::Dispose(bool isShutdown)
{
    Assert(m_alloc.Size() == 0);
}

void WebAssemblyModule::Mark(Recycler * recycler)
{
}
uint WebAssemblyModule::GetOffsetForGlobal(Wasm::WasmGlobal* global)
{
    Wasm::WasmTypes::WasmType type = global->GetType();
    if (type >= Wasm::WasmTypes::Limit)
    {
        throw Wasm::WasmCompilationException(_u("Invalid Global type"));
    }

    uint32 offset = WAsmJs::ConvertFromJsVarOffset<byte>(GetGlobalOffset());

    for (uint i = 1; i < (uint)type; i++)
    {
        offset = AddGlobalByteSizeToOffset((Wasm::WasmTypes::WasmType)i, offset);
    }

    uint32 typeSize = Wasm::WasmTypes::GetTypeByteSize(type);
    uint32 sizeUsed = WAsmJs::ConvertOffset<byte>(global->GetOffset(), typeSize);
    offset = UInt32Math::Add(offset, sizeUsed);
    return WAsmJs::ConvertOffset(offset, sizeof(byte), typeSize);
}

uint WebAssemblyModule::AddGlobalByteSizeToOffset(Wasm::WasmTypes::WasmType type, uint32 offset) const
{
    uint32 typeSize = Wasm::WasmTypes::GetTypeByteSize(type);
    offset = ::Math::AlignOverflowCheck(offset, typeSize);
    uint32 sizeUsed = WAsmJs::ConvertOffset<byte>(globalCounts[type], typeSize);
    return UInt32Math::Add(offset, sizeUsed);
}

uint WebAssemblyModule::GetGlobalsByteSize() const
{
    uint32 size = 0;
    for (Wasm::WasmTypes::WasmType type = (Wasm::WasmTypes::WasmType)(Wasm::WasmTypes::Void + 1); type < Wasm::WasmTypes::Limit; type = (Wasm::WasmTypes::WasmType)(type + 1))
    {
        size = AddGlobalByteSizeToOffset(type, size);
    }
    return size;
}

} // namespace Js

#endif