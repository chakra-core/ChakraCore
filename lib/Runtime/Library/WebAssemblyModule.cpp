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
    m_hasMemory(false),
    m_hasTable(false),
    m_memImport(nullptr),
    m_tableImport(nullptr),
    m_importedFunctionCount(0),
    m_memoryInitSize(0),
    m_memoryMaxSize(0),
    m_tableInitSize(0),
    m_tableMaxSize(0),
    m_alloc(_u("WebAssemblyModule"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_indirectfuncs(nullptr),
    m_exports(nullptr),
    m_exportCount(0),
    m_datasegCount(0),
    m_elementsegCount(0),
    m_elementsegs(nullptr),
    m_signatures(nullptr),
    m_signaturesCount(0),
    m_startFuncIndex(Js::Constants::UninitializedValue),
    m_binaryBuffer(binaryBuffer),
    m_customSections(nullptr)
{
    //the first elm is the number of Vars in front of I32; makes for a nicer offset computation
    memset(m_globalCounts, 0, sizeof(uint) * Wasm::WasmTypes::Limit);
    m_functionsInfo = RecyclerNew(scriptContext->GetRecycler(), WasmFunctionInfosList, scriptContext->GetRecycler());
    m_imports = Anew(&m_alloc, WasmImportsList, &m_alloc);
    m_globals = Anew(&m_alloc, WasmGlobalsList, &m_alloc);
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

Var
WebAssemblyModule::EntryExports(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !WebAssemblyModule::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedModule);
    }
    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);

    Var exportArray = JavascriptOperators::NewJavascriptArrayNoArg(scriptContext);

    for (uint i = 0; i < module->GetExportCount(); ++i)
    {
        Wasm::WasmExport wasmExport = module->m_exports[i];
        Js::JavascriptString * kind = GetExternalKindString(scriptContext, wasmExport.kind);
        Js::JavascriptString * name = JavascriptString::NewCopySz(wasmExport.name, scriptContext);
        Var pair = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
        JavascriptOperators::OP_SetProperty(pair, PropertyIds::kind, kind, scriptContext);
        JavascriptOperators::OP_SetProperty(pair, PropertyIds::name, name, scriptContext);
        JavascriptArray::Push(scriptContext, exportArray, pair);
    }
    return exportArray;
}

Var
WebAssemblyModule::EntryImports(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !WebAssemblyModule::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedModule);
    }

    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);

    Var importArray = JavascriptOperators::NewJavascriptArrayNoArg(scriptContext);
    for (uint32 i = 0; i < module->GetImportCount(); ++i)
    {
        Wasm::WasmImport * import = module->GetImport(i);
        Js::JavascriptString * kind = GetExternalKindString(scriptContext, import->kind);
        Js::JavascriptString * moduleName = JavascriptString::NewCopySz(import->modName, scriptContext);
        Js::JavascriptString * name = JavascriptString::NewCopySz(import->importName, scriptContext);

        Var pair = JavascriptOperators::NewJavascriptObjectNoArg(scriptContext);
        JavascriptOperators::OP_SetProperty(pair, PropertyIds::kind, kind, scriptContext);
        JavascriptOperators::OP_SetProperty(pair, PropertyIds::module, moduleName, scriptContext);
        JavascriptOperators::OP_SetProperty(pair, PropertyIds::name, name, scriptContext);
        JavascriptArray::Push(scriptContext, importArray, pair);
    }

    return importArray;
}

Var WebAssemblyModule::EntryCustomSections(RecyclableObject* function, CallInfo callInfo, ...)
{
    PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

    ARGUMENTS(args, callInfo);
    AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");
    ScriptContext* scriptContext = function->GetScriptContext();

    Assert(!(callInfo.Flags & CallFlags_New));

    if (args.Info.Count < 2 || !WebAssemblyModule::Is(args[1]))
    {
        JavascriptError::ThrowTypeError(scriptContext, WASMERR_NeedModule);
    }
    if (args.Info.Count < 3)
    {
        JavascriptError::ThrowTypeErrorVar(scriptContext, JSERR_NeedString, _u("sectionName"));
    }

    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);
    JavascriptString * sectionName = JavascriptConversion::ToString(args[2], scriptContext);
    const char16* sectionNameBuf = sectionName->GetString();
    charcount_t sectionNameLength = sectionName->GetLength();

    Var customSections = JavascriptOperators::NewJavascriptArrayNoArg(scriptContext);
    for (uint32 i = 0; i < module->GetCustomSectionCount(); ++i)
    {
        Wasm::CustomSection customSection = module->GetCustomSection(i);
        if (sectionNameLength == customSection.nameLength &&
            // can't use string compare because null-terminator is a valid character for custom section names
            memcmp(sectionNameBuf, customSection.name, sectionNameLength * sizeof(char16)) == 0)
        {
            const uint32 byteLength = customSection.payloadSize;
            ArrayBuffer* arrayBuffer = scriptContext->GetLibrary()->CreateArrayBuffer(byteLength);
            if (byteLength > 0)
            {
                js_memcpy_s(arrayBuffer->GetBuffer(), byteLength, customSection.payload, byteLength);
            }
            JavascriptArray::Push(scriptContext, customSections, arrayBuffer);
        }
    }

    return customSections;
}

/* static */
WebAssemblyModule *
WebAssemblyModule::CreateModule(
    ScriptContext* scriptContext,
    const byte* buffer,
    const uint lengthBytes)
{
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmBytecodePhase);
    Unused(wasmPhase);

    WebAssemblyModule * webAssemblyModule = nullptr;
    Wasm::WasmReaderInfo * readerInfo = nullptr;
    Js::FunctionBody * currentBody = nullptr;
    try
    {
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        SourceContextInfo * sourceContextInfo = scriptContext->CreateSourceContextInfo(scriptContext->GetNextSourceContextId(), nullptr, 0, nullptr);
        SRCINFO si = {
            /* sourceContextInfo   */ sourceContextInfo,
            /* dlnHost             */ 0,
            /* ulColumnHost        */ 0,
            /* lnMinHost           */ 0,
            /* ichMinHost          */ 0,
            /* ichLimHost          */ 0,
            /* ulCharOffset        */ 0,
            /* mod                 */ 0,
            /* grfsi               */ 0
        };

        // copy buffer so external changes to it don't cause issues when defer parsing
        byte* newBuffer = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), byte, lengthBytes);
        js_memcpy_s(newBuffer, lengthBytes, buffer, lengthBytes);

        // Note: We don't have real "source info" for Wasm. Following are just placeholders.
        // Hack: Wasm handles debugging differently. Fake this as "LibraryCode" so that
        // normal script debugging code ignores this source info and its functions.
        const int32 cchLength = static_cast<int32>(lengthBytes / sizeof(char16));
        Js::Utf8SourceInfo* utf8SourceInfo = Utf8SourceInfo::NewWithNoCopy(
            scriptContext, (LPCUTF8)newBuffer, cchLength, lengthBytes, &si, /*isLibraryCode*/true);
        scriptContext->SaveSourceNoCopy(utf8SourceInfo, cchLength, /*isCesu8*/false);

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
        JavascriptError::ThrowWebAssemblyCompileErrorVar(scriptContext, WASMERR_WasmCompileError, newEx.ReleaseErrorMessage());
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
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmBytecodePhase);
    Unused(wasmPhase);

    try
    {
        Js::AutoDynamicCodeReference dynamicFunctionReference(scriptContext);
        SRCINFO const * srcInfo = scriptContext->cache->noContextGlobalSourceInfo;

        // review: unsure if we need copy here, but seems safer to do it
        byte* newBuffer = RecyclerNewArrayLeaf(scriptContext->GetRecycler(), byte, lengthBytes);
        js_memcpy_s(newBuffer, lengthBytes, buffer, lengthBytes);

        // Note: We don't have real "source info" for Wasm. Following are just placeholders.
        // Hack: Wasm handles debugging differently. Fake this as "LibraryCode" so that
        // normal script debugging code ignores this source info and its functions.
        const int32 cchLength = static_cast<int32>(lengthBytes / sizeof(char16));
        Js::Utf8SourceInfo* utf8SourceInfo = Utf8SourceInfo::NewWithNoCopy(
            scriptContext, (LPCUTF8)newBuffer, cchLength, lengthBytes, srcInfo, /*isLibraryCode*/true);
        scriptContext->SaveSourceNoCopy(utf8SourceInfo, cchLength, /*isCesu8*/false);

        Wasm::WasmModuleGenerator bytecodeGen(scriptContext, utf8SourceInfo, (byte*)newBuffer, lengthBytes);

        WebAssemblyModule * webAssemblyModule = bytecodeGen.GenerateModule();

        for (uint i = 0; i < webAssemblyModule->GetWasmFunctionCount(); ++i)
        {
            Js::FunctionBody * body = webAssemblyModule->GetWasmFunctionInfo(i)->GetBody();
            Wasm::WasmReaderInfo * readerInfo = body->GetAsmJsFunctionInfo()->GetWasmReaderInfo();

            // TODO: avoid actually generating bytecode here
            Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);

#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (PHASE_ON(WasmValidatePrejitPhase, body))
            {
                CONFIG_FLAG(MaxAsmJsInterpreterRunCount) = 0;
                AsmJsScriptFunction * funcObj = scriptContext->GetLibrary()->CreateAsmJsScriptFunction(body);
                FunctionEntryPointInfo * entypointInfo = (FunctionEntryPointInfo*)funcObj->GetEntryPointInfo();
                entypointInfo->SetIsAsmJSFunction(true);
                GenerateFunction(scriptContext->GetNativeCodeGenerator(), body, funcObj);
            }
#endif
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
    return GetWasmFunctionCount();
}

Wasm::FunctionIndexTypes::Type
WebAssemblyModule::GetFunctionIndexType(uint32 funcIndex) const
{
    if (funcIndex >= GetMaxFunctionIndex())
    {
        return Wasm::FunctionIndexTypes::Invalid;
    }
    if (funcIndex < GetImportedFunctionCount())
    {
        return Wasm::FunctionIndexTypes::ImportThunk;
    }
    return Wasm::FunctionIndexTypes::Function;
}

void
WebAssemblyModule::InitializeMemory(uint32 minPage, uint32 maxPage)
{
    if (m_hasMemory)
    {
        throw Wasm::WasmCompilationException(_u("Memory already allocated"));
    }

    if (maxPage < minPage)
    {
        throw Wasm::WasmCompilationException(_u("Memory: MaxPage (%d) must be greater than MinPage (%d)"), maxPage, minPage);
    }
    m_hasMemory = true;
    m_memoryInitSize = minPage;
    m_memoryMaxSize = maxPage;
}

WebAssemblyMemory *
WebAssemblyModule::CreateMemory() const
{
    return WebAssemblyMemory::CreateMemoryObject(m_memoryInitSize, m_memoryMaxSize, GetScriptContext());
}

Wasm::WasmSignature *
WebAssemblyModule::GetSignatures() const
{
    return m_signatures;
}

Wasm::WasmSignature *
WebAssemblyModule::GetSignature(uint32 index) const
{
    if (index >= GetSignatureCount())
    {
        throw Wasm::WasmCompilationException(_u("Invalid signature index %u"), index);
    }

    return &m_signatures[index];
}

uint32
WebAssemblyModule::GetSignatureCount() const
{
    return m_signaturesCount;
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
WebAssemblyModule::InitializeTable(uint32 minEntries, uint32 maxEntries)
{
    if (m_hasTable)
    {
        throw Wasm::WasmCompilationException(_u("Table already allocated"));
    }

    if (maxEntries < minEntries)
    {
        throw Wasm::WasmCompilationException(_u("Table: max entries (%d) is less than min entries (%d)"), maxEntries, minEntries);
    }
    m_hasTable = true;
    m_tableInitSize = minEntries;
    m_tableMaxSize = maxEntries;
}

WebAssemblyTable *
WebAssemblyModule::CreateTable() const
{
    return WebAssemblyTable::Create(m_tableInitSize, m_tableMaxSize, GetScriptContext());
}

bool
WebAssemblyModule::IsValidTableImport(const WebAssemblyTable * table) const
{
    return m_tableImport && table->GetInitialLength() >= m_tableInitSize && table->GetMaximumLength() <= m_tableMaxSize;
}

uint32
WebAssemblyModule::GetWasmFunctionCount() const
{
    return (uint32)m_functionsInfo->Count();
}

Wasm::WasmFunctionInfo*
WebAssemblyModule::AddWasmFunctionInfo(Wasm::WasmSignature* sig)
{
    uint32 functionId = GetWasmFunctionCount();
    // must be recycler memory, since it holds reference to the function body
    Wasm::WasmFunctionInfo* funcInfo = RecyclerNew(GetRecycler(), Wasm::WasmFunctionInfo, &m_alloc, sig, functionId);
    m_functionsInfo->Add(funcInfo);
    return funcInfo;
}

Wasm::WasmFunctionInfo*
WebAssemblyModule::GetWasmFunctionInfo(uint index) const
{
    if (index >= GetWasmFunctionCount())
    {
        throw Wasm::WasmCompilationException(_u("Invalid function index %u"), index);
    }

    return m_functionsInfo->Item(index);
}

void
WebAssemblyModule::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(&m_alloc, Wasm::WasmExport, entries);
    m_exportCount = entries;
}

void
WebAssemblyModule::SetExport(uint32 iExport, uint32 funcIndex, const char16* exportName, uint32 nameLength, Wasm::ExternalKinds::ExternalKind kind)
{
    m_exports[iExport].index = funcIndex;
    m_exports[iExport].nameLength = nameLength;
    m_exports[iExport].name = exportName;
    m_exports[iExport].kind = kind;
}

Wasm::WasmExport*
WebAssemblyModule::GetExport(uint32 iExport) const
{
    if (iExport >= m_exportCount)
    {
        return nullptr;
    }
    return &m_exports[iExport];
}

uint32
WebAssemblyModule::GetImportCount() const
{
    return (uint32)m_imports->Count();
}

void
WebAssemblyModule::AddFunctionImport(uint32 sigId, const char16* modName, uint32 modNameLen, const char16* fnName, uint32 fnNameLen)
{
    if (sigId >= GetSignatureCount())
    {
        throw Wasm::WasmCompilationException(_u("Function signature %u is out of bound"), sigId);
    }

    // Store the information about the import
    Wasm::WasmImport* importInfo = Anew(&m_alloc, Wasm::WasmImport);
    importInfo->kind = Wasm::ExternalKinds::Function;
    importInfo->modNameLen = modNameLen;
    importInfo->modName = modName;
    importInfo->importNameLen = fnNameLen;
    importInfo->importName = fnName;
    m_imports->Add(importInfo);

    Wasm::WasmSignature* signature = GetSignature(sigId);
    Wasm::WasmFunctionInfo* funcInfo = AddWasmFunctionInfo(signature);
    // Create the custom reader to generate the import thunk
    Wasm::WasmCustomReader* customReader = Anew(&m_alloc, Wasm::WasmCustomReader, &m_alloc);
    for (uint32 iParam = 0; iParam < signature->GetParamCount(); iParam++)
    {
        Wasm::WasmNode node;
        node.op = Wasm::wbGetLocal;
        node.var.num = iParam;
        customReader->AddNode(node);
    }
    Wasm::WasmNode callNode;
    callNode.op = Wasm::wbCall;
    callNode.call.num = m_importedFunctionCount++;
    callNode.call.funcType = Wasm::FunctionIndexTypes::Import;
    customReader->AddNode(callNode);
    funcInfo->SetCustomReader(customReader);
#if DBG_DUMP
    funcInfo->importedFunctionReference = importInfo;
#endif

    // 32 to account for hardcoded part of the name + max uint in decimal representation
    uint32 bufferLength = 32;
    if (!UInt32Math::Add(modNameLen, bufferLength, &bufferLength) &&
        !UInt32Math::Add(fnNameLen, bufferLength, &bufferLength))
    {
        char16 * autoName = RecyclerNewArrayLeafZ(GetScriptContext()->GetRecycler(), char16, bufferLength);
        uint32 nameLength = swprintf_s(autoName, bufferLength, _u("%s.%s.Thunk[%u]"), modName, fnName, funcInfo->GetNumber());
        if (nameLength != (uint32)-1)
        {
            funcInfo->SetName(autoName, nameLength);
        }
        else
        {
            AssertMsg(UNREACHED, "Failed to generate import' thunk name");
        }
    }
}

Wasm::WasmImport*
WebAssemblyModule::GetImport(uint32 i) const
{
    if (i >= GetImportCount())
    {
        throw Wasm::WasmCompilationException(_u("Import index out of range"));
    }
    return m_imports->Item(i);
}

void
WebAssemblyModule::AddGlobalImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen)
{
    Wasm::WasmImport* wi = Anew(&m_alloc, Wasm::WasmImport);
    wi->kind = Wasm::ExternalKinds::Global;
    wi->importName = importName;
    wi->importNameLen = importNameLen;
    wi->modName = modName;
    wi->modNameLen = modNameLen;
    m_imports->Add(wi);
}

void
WebAssemblyModule::AddMemoryImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen)
{
    Wasm::WasmImport* wi = Anew(&m_alloc, Wasm::WasmImport);
    wi->kind = Wasm::ExternalKinds::Memory;
    wi->importName = importName;
    wi->importNameLen = importNameLen;
    wi->modName = modName;
    wi->modNameLen = modNameLen;
    m_imports->Add(wi);
    m_memImport = wi;
}

void
WebAssemblyModule::AddTableImport(const char16* modName, uint32 modNameLen, const char16* importName, uint32 importNameLen)
{
    Wasm::WasmImport* wi = Anew(&m_alloc, Wasm::WasmImport);
    wi->kind = Wasm::ExternalKinds::Table;
    wi->importName = importName;
    wi->importNameLen = importNameLen;
    wi->modName = modName;
    wi->modNameLen = modNameLen;
    m_imports->Add(wi);
    m_tableImport = wi;
}

uint
WebAssemblyModule::GetOffsetFromInit(const Wasm::WasmNode& initExpr, const WebAssemblyEnvironment* env) const
{
    try
    {
        ValidateInitExportForOffset(initExpr);
    }
    catch (Wasm::WasmCompilationException &e)
    {
        // Should have been checked at compile time
        Assert(UNREACHED);
        throw e;
    }

    uint offset = 0;
    if (initExpr.op == Wasm::wbI32Const)
    {
        offset = initExpr.cnst.i32;
    }
    else if (initExpr.op == Wasm::wbGetGlobal)
    {
        Wasm::WasmGlobal* global = GetGlobal(initExpr.var.num);
        Assert(global->GetType() == Wasm::WasmTypes::I32);
        offset = env->GetGlobalValue(global).i32;
    }
    return offset;
}

void
WebAssemblyModule::ValidateInitExportForOffset(const Wasm::WasmNode& initExpr) const
{
    if (initExpr.op == Wasm::wbGetGlobal)
    {
        Wasm::WasmGlobal* global = GetGlobal(initExpr.var.num);
        if (global->GetType() != Wasm::WasmTypes::I32)
        {
            throw Wasm::WasmCompilationException(_u("global %u must be i32 for init_expr"), initExpr.var.num);
        }
    }
    else if (initExpr.op != Wasm::wbI32Const)
    {
        throw Wasm::WasmCompilationException(_u("Invalid init_expr for offset"));
    }
}

void
WebAssemblyModule::AddGlobal(Wasm::GlobalReferenceTypes::Type refType, Wasm::WasmTypes::WasmType type, bool isMutable, Wasm::WasmNode init)
{
    Wasm::WasmGlobal* global = Anew(&m_alloc, Wasm::WasmGlobal, refType, m_globalCounts[type]++, type, isMutable, init);
    m_globals->Add(global);
}

uint32
WebAssemblyModule::GetGlobalCount() const
{
    return (uint32)m_globals->Count();
}

Wasm::WasmGlobal*
WebAssemblyModule::GetGlobal(uint32 index) const
{
    if (index >= GetGlobalCount())
    {
        throw Wasm::WasmCompilationException(_u("Global index out of bounds %u"), index);
    }
    return m_globals->Item(index);
}

void
WebAssemblyModule::AllocateDataSegs(uint32 count)
{
    Assert(count != 0);
    m_datasegCount = count;
    m_datasegs = AnewArray(&m_alloc, Wasm::WasmDataSegment*, count);
}

void
WebAssemblyModule::SetDataSeg(Wasm::WasmDataSegment* seg, uint32 index)
{
    Assert(index < m_datasegCount);
    m_datasegs[index] = seg;
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
WebAssemblyModule::AllocateElementSegs(uint32 count)
{
    Assert(count != 0);
    m_elementsegCount = count;
    m_elementsegs = AnewArrayZ(&m_alloc, Wasm::WasmElementSegment*, count);
}

void
WebAssemblyModule::SetElementSeg(Wasm::WasmElementSegment* seg, uint32 index)
{
    Assert(index < m_elementsegCount);
    m_elementsegs[index] = seg;
}

Wasm::WasmElementSegment*
WebAssemblyModule::GetElementSeg(uint32 index) const
{
    if (index >= m_elementsegCount)
    {
        throw Wasm::WasmCompilationException(_u("Invalid index for Element segment"));
    }
    return m_elementsegs[index];
}

void
WebAssemblyModule::SetStartFunction(uint32 i)
{
    if (i >= GetWasmFunctionCount())
    {
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

void
WebAssemblyModule::SetSignatureCount(uint32 count)
{
    Assert(m_signaturesCount == 0 && m_signatures == nullptr);
    m_signaturesCount = count;
    m_signatures = RecyclerNewArrayZ(GetRecycler(), Wasm::WasmSignature, count);
}

uint32
WebAssemblyModule::GetModuleEnvironmentSize() const
{
    static const uint DOUBLE_SIZE_IN_INTS = sizeof(double) / sizeof(int);
    // 1 each for memory, table, and signatures
    uint32 size = 3;
    size = UInt32Math::Add(size, GetWasmFunctionCount());
    size = UInt32Math::Add(size, GetImportedFunctionCount());
    size = UInt32Math::Add(size, WAsmJs::ConvertToJsVarOffset<byte>(GetGlobalsByteSize()));
    return size;
}

void
WebAssemblyModule::Finalize(bool isShutdown)
{
    m_alloc.Clear();
}

void
WebAssemblyModule::Dispose(bool isShutdown)
{
    Assert(m_alloc.Size() == 0);
}

void
WebAssemblyModule::Mark(Recycler * recycler)
{
}

uint
WebAssemblyModule::GetOffsetForGlobal(Wasm::WasmGlobal* global) const
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

uint
WebAssemblyModule::AddGlobalByteSizeToOffset(Wasm::WasmTypes::WasmType type, uint32 offset) const
{
    uint32 typeSize = Wasm::WasmTypes::GetTypeByteSize(type);
    offset = ::Math::AlignOverflowCheck(offset, typeSize);
    uint32 sizeUsed = WAsmJs::ConvertOffset<byte>(m_globalCounts[type], typeSize);
    return UInt32Math::Add(offset, sizeUsed);
}


JavascriptString *
WebAssemblyModule::GetExternalKindString(ScriptContext * scriptContext, Wasm::ExternalKinds::ExternalKind kind)
{
    switch (kind)
    {
    case Wasm::ExternalKinds::Function:
        return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("function"));
    case Wasm::ExternalKinds::Table:
        return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("table"));
    case Wasm::ExternalKinds::Memory:
        return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("memory"));
    case Wasm::ExternalKinds::Global:
        return scriptContext->GetLibrary()->CreateStringFromCppLiteral(_u("global"));
    default:
        Assume(UNREACHED);
    }
    return nullptr;
}

uint
WebAssemblyModule::GetGlobalsByteSize() const
{
    uint32 size = 0;
    for (Wasm::WasmTypes::WasmType type = (Wasm::WasmTypes::WasmType)(Wasm::WasmTypes::Void + 1); type < Wasm::WasmTypes::Limit; type = (Wasm::WasmTypes::WasmType)(type + 1))
    {
        size = AddGlobalByteSizeToOffset(type, size);
    }
    return size;
}

void
WebAssemblyModule::AddCustomSection(Wasm::CustomSection customSection)
{
    if (!m_customSections)
    {
        m_customSections = Anew(&m_alloc, CustomSectionsList, &m_alloc);
    }
    m_customSections->Add(customSection);
}

uint32
WebAssemblyModule::GetCustomSectionCount() const
{
    return m_customSections ? (uint32)m_customSections->Count() : 0;
}

Wasm::CustomSection
WebAssemblyModule::GetCustomSection(uint32 index) const
{
    if (index >= GetCustomSectionCount())
    {
        throw Wasm::WasmCompilationException(_u("Custom section index out of bounds %u"), index);
    }
    return m_customSections->Item(index);
}

} // namespace Js

#endif