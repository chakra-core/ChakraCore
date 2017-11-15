//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM

#include "../WasmReader/WasmReaderPch.h"
#include "Language/WebAssemblySource.h"

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
    m_binaryBufferLength(binaryBufferLength),
    m_customSections(nullptr)
{
    m_alloc = HeapNew(ArenaAllocator, _u("WebAssemblyModule"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory);
    //the first elm is the number of Vars in front of I32; makes for a nicer offset computation
    memset(m_globalCounts, 0, sizeof(uint) * Wasm::WasmTypes::Limit);
    m_functionsInfo = RecyclerNew(scriptContext->GetRecycler(), WasmFunctionInfosList, scriptContext->GetRecycler());
    m_imports = Anew(m_alloc, WasmImportsList, m_alloc);
    m_globals = Anew(m_alloc, WasmGlobalsList, m_alloc);
    m_reader = Anew(m_alloc, Wasm::WasmBinaryReader, m_alloc, this, binaryBuffer, binaryBufferLength);
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
    
    WebAssemblySource src(args[1], true, scriptContext);
    return CreateModule(scriptContext, &src);
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
    Var sectionNameVar = args.Info.Count > 2 ? args[2] : scriptContext->GetLibrary()->GetUndefined();

    WebAssemblyModule * module = WebAssemblyModule::FromVar(args[1]);
    JavascriptString * sectionName = JavascriptConversion::ToString(sectionNameVar, scriptContext);
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
    WebAssemblySource* src)
{
    Assert(src);
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmBytecodePhase);
    Unused(wasmPhase);

    WebAssemblyModule * webAssemblyModule = nullptr;
    Wasm::WasmReaderInfo * readerInfo = nullptr;
    Js::FunctionBody * currentBody = nullptr;
    try
    {
        Wasm::WasmModuleGenerator bytecodeGen(scriptContext, src);

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
        // The reason that we use GetTempErrorMessageRef here, instead of just doing simply
        // ReleaseErrorMessage and grabbing it away from the WasmCompilationException, is a
        // slight niggle in the lifetime of the message buffer. The normal error throw..Var
        // routines don't actually do anything to clean up their varargs - likely good, due
        // to the variety of situations that we want to use them in. However, this fails to
        // clean up strings if they're not cleaned up by some destructor. By handling these
        // error strings not by grabbing them away from the WasmCompilationException, I can
        // have the WasmCompilationException destructor clean up the strings when we throw,
        // by which point SetErrorMessage will already have copied out what it needs.
        BSTR originalMessage = ex.GetTempErrorMessageRef();
        if (currentBody != nullptr)
        {
            Wasm::BinaryLocation location = readerInfo->m_module->GetReader()->GetCurrentLocation();

            originalMessage = ex.ReleaseErrorMessage();
            ex = Wasm::WasmCompilationException(
                _u("function %s at offset %u/%u (0x%x/0x%x): %s"),
                currentBody->GetDisplayName(),
                location.offset, location.size,
                location.offset, location.size,
                originalMessage
            );
            currentBody->GetAsmJsFunctionInfo()->SetWasmReaderInfo(nullptr);
            SysFreeString(originalMessage);
            originalMessage = ex.GetTempErrorMessageRef();
        }
        JavascriptError::ThrowWebAssemblyCompileErrorVar(scriptContext, WASMERR_WasmCompileError, originalMessage);
    }

    return webAssemblyModule;
}

/* static */
bool
WebAssemblyModule::ValidateModule(
    ScriptContext* scriptContext,
    WebAssemblySource* src)
{
    Assert(src);
    AutoProfilingPhase wasmPhase(scriptContext, Js::WasmBytecodePhase);
    Unused(wasmPhase);

    try
    {
        Wasm::WasmModuleGenerator bytecodeGen(scriptContext, src);

        WebAssemblyModule * webAssemblyModule = bytecodeGen.GenerateModule();

        for (uint i = 0; i < webAssemblyModule->GetWasmFunctionCount(); ++i)
        {
            Js::FunctionBody * body = webAssemblyModule->GetWasmFunctionInfo(i)->GetBody();
            Wasm::WasmReaderInfo * readerInfo = body->GetAsmJsFunctionInfo()->GetWasmReaderInfo();

            Wasm::WasmBytecodeGenerator::ValidateFunction(scriptContext, readerInfo);
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
        throw Wasm::WasmCompilationException(_u("Memory: MaxPage (%u) must be greater than MinPage (%u)"), maxPage, minPage);
    }
    auto minPageTooBig = [minPage] {
        throw Wasm::WasmCompilationException(_u("Memory: Unable to allocate minimum pages (%u)"), minPage);
    };
    uint32 minBytes = UInt32Math::Mul<WebAssembly::PageSize>(minPage, minPageTooBig);
    if (minBytes > ArrayBuffer::MaxArrayBufferLength)
    {
        minPageTooBig();
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
    Wasm::WasmFunctionInfo* funcInfo = RecyclerNew(GetRecycler(), Wasm::WasmFunctionInfo, m_alloc, sig, functionId);
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
WebAssemblyModule::SwapWasmFunctionInfo(uint i1, uint i2)
{
    Wasm::WasmFunctionInfo* f1 = GetWasmFunctionInfo(i1);
    Wasm::WasmFunctionInfo* f2 = GetWasmFunctionInfo(i2);
    m_functionsInfo->SetItem(i1, f2);
    m_functionsInfo->SetItem(i2, f1);
}

#if ENABLE_DEBUG_CONFIG_OPTIONS
void
WebAssemblyModule::AttachCustomInOutTracingReader(Wasm::WasmFunctionInfo* func, uint callIndex)
{
    Wasm::WasmFunctionInfo* calledFunc = GetWasmFunctionInfo(callIndex);
    Wasm::WasmSignature* signature = calledFunc->GetSignature();
    if (!calledFunc->GetSignature()->IsEquivalent(signature))
    {
        throw Wasm::WasmCompilationException(_u("InOut tracing reader signature mismatch"));
    }
    // Create the custom reader to generate the import thunk
    Wasm::WasmCustomReader* customReader = Anew(m_alloc, Wasm::WasmCustomReader, m_alloc);
    // Print the function name we are calling
    {
        Wasm::WasmNode nameNode;
        nameNode.op = Wasm::wbI32Const;
        nameNode.cnst.i32 = callIndex;
        customReader->AddNode(nameNode);
        nameNode.op = Wasm::wbPrintFuncName;
        customReader->AddNode(nameNode);
    }

    for (Js::ArgSlot iParam = 0; iParam < signature->GetParamCount(); iParam++)
    {
        Wasm::WasmNode node;
        node.op = Wasm::wbGetLocal;
        node.var.num = iParam;
        customReader->AddNode(node);

        Wasm::Local param = signature->GetParam(iParam);
        switch (param)
        {
        case Wasm::Local::I32: node.op = Wasm::wbPrintI32; break;
        case Wasm::Local::I64: node.op = Wasm::wbPrintI64; break;
        case Wasm::Local::F32: node.op = Wasm::wbPrintF32; break;
        case Wasm::Local::F64: node.op = Wasm::wbPrintF64; break;
        default:
            throw Wasm::WasmCompilationException(_u("Unknown param type"));
        }
        customReader->AddNode(node);

        if (iParam < signature->GetParamCount() - 1)
        {
            node.op = Wasm::wbPrintArgSeparator;
            customReader->AddNode(node);
        }
    }

    Wasm::WasmNode beginNode;
    beginNode.op = Wasm::wbPrintBeginCall;
    customReader->AddNode(beginNode);

    Wasm::WasmNode callNode;
    callNode.op = Wasm::wbCall;
    callNode.call.num = callIndex;
    callNode.call.funcType = Wasm::FunctionIndexTypes::Function;
    customReader->AddNode(callNode);

    Wasm::WasmTypes::WasmType returnType = signature->GetResultType();
    Wasm::WasmNode endNode;
    endNode.op = Wasm::wbI32Const;
    endNode.cnst.i32 = returnType;
    customReader->AddNode(endNode);
    endNode.op = Wasm::wbPrintEndCall;
    customReader->AddNode(endNode);

    if (returnType != Wasm::WasmTypes::Void)
    {
        Wasm::WasmNode node;
        switch (returnType)
        {
        case Wasm::WasmTypes::I32: node.op = Wasm::wbPrintI32; break;
        case Wasm::WasmTypes::I64: node.op = Wasm::wbPrintI64; break;
        case Wasm::WasmTypes::F32: node.op = Wasm::wbPrintF32; break;
        case Wasm::WasmTypes::F64: node.op = Wasm::wbPrintF64; break;
        default:
            throw Wasm::WasmCompilationException(_u("Unknown return type"));
        }
        customReader->AddNode(node);
    }
    endNode.op = Wasm::wbPrintNewLine;
    customReader->AddNode(endNode);

    func->SetCustomReader(customReader);
}
#endif

void
WebAssemblyModule::AllocateFunctionExports(uint32 entries)
{
    m_exports = AnewArrayZ(m_alloc, Wasm::WasmExport, entries);
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
    Wasm::WasmImport* importInfo = Anew(m_alloc, Wasm::WasmImport);
    importInfo->kind = Wasm::ExternalKinds::Function;
    importInfo->modNameLen = modNameLen;
    importInfo->modName = modName;
    importInfo->importNameLen = fnNameLen;
    importInfo->importName = fnName;
    m_imports->Add(importInfo);

    Wasm::WasmSignature* signature = GetSignature(sigId);
    Wasm::WasmFunctionInfo* funcInfo = AddWasmFunctionInfo(signature);
    // Create the custom reader to generate the import thunk
    Wasm::WasmCustomReader* customReader = Anew(m_alloc, Wasm::WasmCustomReader, m_alloc);
    for (uint32 iParam = 0; iParam < (uint32)signature->GetParamCount(); iParam++)
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
    Wasm::WasmImport* wi = Anew(m_alloc, Wasm::WasmImport);
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
    Wasm::WasmImport* wi = Anew(m_alloc, Wasm::WasmImport);
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
    Wasm::WasmImport* wi = Anew(m_alloc, Wasm::WasmImport);
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
    catch (Wasm::WasmCompilationException&)
    {
        // Should have been checked at compile time
        Assert(UNREACHED);
        throw;
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
    Wasm::WasmGlobal* global = Anew(m_alloc, Wasm::WasmGlobal, refType, m_globalCounts[type]++, type, isMutable, init);
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
    m_datasegs = AnewArray(m_alloc, Wasm::WasmDataSegment*, count);
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
    m_elementsegs = AnewArrayZ(m_alloc, Wasm::WasmElementSegment*, count);
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
    if (m_alloc)
    {
        HeapDelete(m_alloc);
        m_alloc = nullptr;
    }
}

void
WebAssemblyModule::Dispose(bool isShutdown)
{
    Assert(!m_alloc);
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
        m_customSections = Anew(m_alloc, CustomSectionsList, m_alloc);
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
