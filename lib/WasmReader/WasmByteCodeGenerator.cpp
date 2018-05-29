//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
#include "Language/WebAssemblySource.h"
#include "ByteCode/WasmByteCodeWriter.h"
#include "EmptyWasmByteCodeWriter.h"
#include "ByteCode/ByteCodeDumper.h"
#include "AsmJsByteCodeDumper.h"

#if DBG_DUMP
#define DebugPrintOp(op) if (DO_WASM_TRACE_BYTECODE) { PrintOpBegin(op); }
#define DebugPrintOpEnd() if (DO_WASM_TRACE_BYTECODE) { PrintOpEnd(); }
#else
#define DebugPrintOp(op)
#define DebugPrintOpEnd()
#endif

namespace Wasm
{
#define WASM_SIGNATURE(id, nTypes, ...) const WasmTypes::WasmType WasmOpCodeSignatures::id[] = {__VA_ARGS__};
#include "WasmBinaryOpCodes.h"

template<typename WriteFn>
void WasmBytecodeGenerator::WriteTypeStack(WriteFn writefn) const
{
    writefn(_u("["));
    int i = 0;
    while (m_evalStack.Peek(i).type != WasmTypes::Limit)
    {
        ++i;
    }
    --i;
    bool isFirst = true;
    while (i >= 0)
    {
        EmitInfo info = m_evalStack.Peek(i--);
        if (!isFirst)
        {
            writefn(_u(", "));
        }
        isFirst = false;
        writefn(GetTypeName(info.type));
    }
    writefn(_u("]"));
}

uint32 WasmBytecodeGenerator::WriteTypeStackToString(_Out_writes_(maxlen) char16* out, uint32 maxlen) const
{
    AssertOrFailFast(out != nullptr);
    uint32 numwritten = 0;
    WriteTypeStack([&] (const char16* msg)
    {
        numwritten += _snwprintf_s(out + numwritten, maxlen - numwritten, _TRUNCATE, msg);
    });
    if (numwritten >= maxlen - 5)
    {
        // null out the last 5 characters so we can properly end it
        for (int i = 1; i <= 5; i++)
        {
            *(out + maxlen - i) = 0;
        }
        numwritten -= 5;
        numwritten += _snwprintf_s(out + numwritten, maxlen - numwritten, _TRUNCATE, _u("...]"));
    }
    return numwritten;
}

#if DBG_DUMP
void WasmBytecodeGenerator::PrintTypeStack() const
{
    WriteTypeStack([](const char16* msg) { Output::Print(msg); });
}

void WasmBytecodeGenerator::PrintOpBegin(WasmOp op)
{
    if (lastOpId == opId) Output::Print(_u("\r\n"));
    lastOpId = ++opId;
    const int depth = m_blockInfos.Count() - 1;
    if (depth > 0)
    {
        Output::SkipToColumn(depth);
    }
    const auto PrintSignature = [this](uint32 sigId)
    {
        if (sigId < m_module->GetSignatureCount())
        {
            Output::Print(_u(" "));
            WasmSignature* sig = m_module->GetSignature(sigId);
            sig->Dump(64);
        }
        else
        {
            Output::Print(_u(" invalid signature id %u"), sigId);
        }
    };

    switch (op)
    {
#define WASM_OPCODE(opname, ...) \
case wb##opname: \
    Output::Print(_u(#opname)); \
    break;
#include "WasmBinaryOpCodes.h"
    }
    switch (op)
    {
    case wbIf:
    case wbLoop:
    case wbBlock: 
        if (GetReader()->m_currentNode.block.IsSingleResult())
        {
            Output::Print(_u(" () -> %s"), GetTypeName(GetReader()->m_currentNode.block.GetSingleResult()));
        }
        else
        {
            PrintSignature(GetReader()->m_currentNode.block.GetSignatureId()); break;
        }
        break;
    case wbBr:
    case wbBrIf: Output::Print(_u(" depth: %u"), GetReader()->m_currentNode.br.depth); break;
    case wbBrTable: Output::Print(_u(" %u cases, default: %u"), GetReader()->m_currentNode.brTable.numTargets, GetReader()->m_currentNode.brTable.defaultTarget); break;
    case wbCallIndirect: PrintSignature(GetReader()->m_currentNode.call.num); break;
    case wbCall:
    {
        uint32 id = GetReader()->m_currentNode.call.num;
        if (id < m_module->GetWasmFunctionCount())
        {
            FunctionIndexTypes::Type funcType = GetReader()->m_currentNode.call.funcType;
            switch (funcType)
            {
            case Wasm::FunctionIndexTypes::Invalid: Output::Print(_u(" (invalid) ")); break;
            case Wasm::FunctionIndexTypes::ImportThunk: Output::Print(_u(" (thunk) ")); break;
            case Wasm::FunctionIndexTypes::Function: Output::Print(_u(" (func) ")); break;
            case Wasm::FunctionIndexTypes::Import: Output::Print(_u(" (import) ")); break;
            default:  Output::Print(_u(" (unknown)")); break;
            }
            auto func = this->m_module->GetWasmFunctionInfo(id);
            func->GetBody()->DumpFullFunctionName();
        }
        else
        {
            Output::Print(_u(" invalid id %u"), id);
        }
        break;
    }
    case wbSetLocal:
    case wbGetLocal:
    case wbTeeLocal:
    case wbGetGlobal:
    case wbSetGlobal: Output::Print(_u(" (%d)"), GetReader()->m_currentNode.var.num); break;
    case wbI32Const: Output::Print(_u(" (%d, 0x%x)"), GetReader()->m_currentNode.cnst.i32, GetReader()->m_currentNode.cnst.i32); break;
    case wbI64Const: Output::Print(_u(" (%lld, 0x%llx)"), GetReader()->m_currentNode.cnst.i64, GetReader()->m_currentNode.cnst.i64); break;
    case wbF32Const: Output::Print(_u(" (%.4f)"), GetReader()->m_currentNode.cnst.f32); break;
    case wbF64Const: Output::Print(_u(" (%.4f)"), GetReader()->m_currentNode.cnst.f64); break;
#define WASM_MEM_OPCODE(opname, ...) case wb##opname: // FallThrough
#include "WasmBinaryOpCodes.h"
    {
        const uint8 alignment = GetReader()->m_currentNode.mem.alignment;
        const uint32 offset = GetReader()->m_currentNode.mem.offset;
        switch (((!!alignment) << 1) | (!!offset))
        {
        case 0: // no alignment, no offset
            Output::Print(_u(" [i]")); break;
        case 1: // no alignment, offset
            Output::Print(_u(" [i + %u (0x%x)]"), offset, offset); break;
        case 2: // alignment, no offset
            Output::Print(_u(" [i & ~0x%x]"), (1 << alignment) - 1); break;
        case 3: // alignment, offset
            Output::Print(_u(" [i + %u (0x%x) & ~0x%x]"), offset, offset, (1 << alignment) - 1); break;
        }
        break;
    }
    }
    Output::SkipToColumn(40);
    PrintTypeStack();
}

void WasmBytecodeGenerator::PrintOpEnd()
{
    if (lastOpId == opId)
    {
        ++opId;
        Output::Print(_u(" -> "));
        PrintTypeStack();
        Output::Print(_u("\r\n"));
    }
}
#endif

/* static */
Js::AsmJsRetType WasmToAsmJs::GetAsmJsReturnType(WasmTypes::WasmType wasmType)
{
    switch (wasmType)
    {
    case WasmTypes::I32: return Js::AsmJsRetType::Signed;
    case WasmTypes::I64: return Js::AsmJsRetType::Int64;
    case WasmTypes::F32: return Js::AsmJsRetType::Float;
    case WasmTypes::F64: return Js::AsmJsRetType::Double;
    case WasmTypes::Void: return Js::AsmJsRetType::Void;
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Simd::EnsureSimdIsEnabled();
        return Js::AsmJsRetType::Float32x4;
#endif
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown return type %u"), wasmType);
    }
}

/* static */
Js::AsmJsVarType WasmToAsmJs::GetAsmJsVarType(WasmTypes::WasmType wasmType)
{
    Js::AsmJsVarType asmType = Js::AsmJsVarType::Int;
    switch (wasmType)
    {
    case WasmTypes::I32: return Js::AsmJsVarType::Int;
    case WasmTypes::I64: return Js::AsmJsVarType::Int64;
    case WasmTypes::F32: return Js::AsmJsVarType::Float;
    case WasmTypes::F64: return Js::AsmJsVarType::Double;
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Simd::EnsureSimdIsEnabled();
        return Js::AsmJsVarType::Float32x4;
#endif
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown var type %u"), wasmType);
    }
}

typedef bool(*SectionProcessFunc)(WasmModuleGenerator*);
typedef void(*AfterSectionCallback)(WasmModuleGenerator*);

WasmModuleGenerator::WasmModuleGenerator(Js::ScriptContext* scriptContext, Js::WebAssemblySource* src) :
    m_sourceInfo(src->GetSourceInfo()),
    m_scriptContext(scriptContext),
    m_recycler(scriptContext->GetRecycler())
{
    m_module = RecyclerNewFinalized(m_recycler, Js::WebAssemblyModule, scriptContext, src->GetBuffer(), src->GetBufferLength(), scriptContext->GetLibrary()->GetWebAssemblyModuleType());

    m_sourceInfo->EnsureInitialized(0);
    m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();
}

Js::WebAssemblyModule* WasmModuleGenerator::GenerateModule()
{
    Js::AutoProfilingPhase wasmPhase(m_scriptContext, Js::WasmReaderPhase);
    Unused(wasmPhase);

    m_module->GetReader()->InitializeReader();

    BVStatic<bSectLimit + 1> visitedSections;

    SectionCode nextExpectedSection = bSectCustom;
    while (true)
    {
        SectionHeader sectionHeader = GetReader()->ReadNextSection();
        SectionCode sectionCode = sectionHeader.code;
        if (sectionCode == bSectLimit)
        {
            TRACE_WASM(PHASE_TRACE1(Js::WasmSectionPhase), _u("Done reading module's sections"));
            break;
        }

        // Make sure dependency for this section has been seen
        SectionCode precedent = SectionInfo::All[sectionCode].precedent;
        if (precedent != bSectLimit && !visitedSections.Test(precedent))
        {
            throw WasmCompilationException(_u("%s section missing before %s"),
                SectionInfo::All[precedent].name,
                sectionHeader.name);
        }
        visitedSections.Set(sectionCode);

        // Custom section are allowed in any order
        if (sectionCode != bSectCustom)
        {
            if (sectionCode < nextExpectedSection)
            {
                throw WasmCompilationException(_u("Invalid Section %s"), sectionHeader.name);
            }
            nextExpectedSection = SectionCode(sectionCode + 1);
        }

        if (!GetReader()->ProcessCurrentSection())
        {
            throw WasmCompilationException(_u("Error while reading section %s"), sectionHeader.name);
        }
    }

    uint32 funcCount = m_module->GetWasmFunctionCount();
    SourceContextInfo * sourceContextInfo = m_sourceInfo->GetSrcInfo()->sourceContextInfo;
    m_sourceInfo->EnsureInitialized(funcCount);
    sourceContextInfo->nextLocalFunctionId += funcCount;
    sourceContextInfo->EnsureInitialized();

    for (uint32 i = 0; i < funcCount; ++i)
    {
        GenerateFunctionHeader(i);
    }

#if ENABLE_DEBUG_CONFIG_OPTIONS
    WasmFunctionInfo* firstThunk = nullptr, *lastThunk = nullptr;
    for (uint32 i = 0; i < funcCount; ++i)
    {
        WasmFunctionInfo* info = m_module->GetWasmFunctionInfo(i);
        Assert(info->GetBody());
        if (PHASE_TRACE(Js::WasmInOutPhase, info->GetBody()))
        {
            uint32 index = m_module->GetWasmFunctionCount();
            WasmFunctionInfo* newInfo = m_module->AddWasmFunctionInfo(info->GetSignature());
            if (!firstThunk)
            {
                firstThunk = newInfo;
            }
            lastThunk = newInfo;
            GenerateFunctionHeader(index);
            m_module->SwapWasmFunctionInfo(i, index);
            m_module->AttachCustomInOutTracingReader(newInfo, index);
        }
    }

    if (firstThunk)
    {
        int sourceId = (int)firstThunk->GetBody()->GetSourceContextId();
        char16 range[64];
        swprintf_s(range, 64, _u("%d.%d-%d.%d"),
                   sourceId, firstThunk->GetBody()->GetLocalFunctionId(),
                   sourceId, lastThunk->GetBody()->GetLocalFunctionId());
        char16 offFullJit[128];
        swprintf_s(offFullJit, 128, _u("-off:fulljit:%s"), range);
        char16 offSimpleJit[128];
        swprintf_s(offSimpleJit, 128, _u("-off:simplejit:%s"), range);
        char16 offLoopJit[128];
        swprintf_s(offLoopJit, 128, _u("-off:jitloopbody:%s"), range);
        char16* argv[] = { nullptr, offFullJit, offSimpleJit, offLoopJit };
        CmdLineArgsParser parser(nullptr);
        parser.Parse(ARRAYSIZE(argv), argv);
    }
#endif

    // If we see a FunctionSignatures section we need to see a FunctionBodies section
    if (visitedSections.Test(bSectFunction) && !visitedSections.Test(bSectFunctionBodies))
    {
        throw WasmCompilationException(_u("Missing required section: %s"), SectionInfo::All[bSectFunctionBodies].name);
    }

    return m_module;
}

WasmBinaryReader* WasmModuleGenerator::GetReader() const
{
    return m_module->GetReader();
}

void WasmModuleGenerator::GenerateFunctionHeader(uint32 index)
{
    WasmFunctionInfo* wasmInfo = m_module->GetWasmFunctionInfo(index);
    if (!wasmInfo)
    {
        throw WasmCompilationException(_u("Invalid function index %u"), index);
    }

    const char16* functionName = nullptr;
    int nameLength = 0;

    if (wasmInfo->GetNameLength() > 0)
    {
        functionName = wasmInfo->GetName();
        nameLength = wasmInfo->GetNameLength();
    }
    else
    {
        for (uint32 iExport = 0; iExport < m_module->GetExportCount(); ++iExport)
        {
            Wasm::WasmExport* wasmExport = m_module->GetExport(iExport);
            if (wasmExport  &&
                wasmExport->kind == ExternalKinds::Function &&
                wasmExport->nameLength > 0 &&
                m_module->GetFunctionIndexType(wasmExport->index) == FunctionIndexTypes::Function &&
                wasmExport->index == wasmInfo->GetNumber())
            {
                nameLength = wasmExport->nameLength + 16;
                char16 * autoName = RecyclerNewArrayLeafZ(m_recycler, char16, nameLength);
                nameLength = swprintf_s(autoName, nameLength, _u("%s[%u]"), wasmExport->name, wasmInfo->GetNumber());
                functionName = autoName;
                break;
            }
        }
    }

    if (!functionName)
    {
        char16* autoName = RecyclerNewArrayLeafZ(m_recycler, char16, 32);
        nameLength = swprintf_s(autoName, 32, _u("wasm-function[%u]"), wasmInfo->GetNumber());
        functionName = autoName;
    }

    Js::FunctionBody* body = Js::FunctionBody::NewFromRecycler(
        m_scriptContext,
        functionName,
        nameLength,
        0,
        0,
        m_sourceInfo,
        m_sourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
        wasmInfo->GetNumber(),
        Js::FunctionInfo::Attributes::ErrorOnNew,
        Js::FunctionBody::Flags_None
#ifdef PERF_COUNTERS
        , false /* is function from deferred deserialized proxy */
#endif
    );
    wasmInfo->SetBody(body);
    // TODO (michhol): numbering
    body->SetSourceInfo(0);
    body->AllocateAsmJsFunctionInfo();
    body->SetIsAsmJsFunction(true);
    body->SetIsAsmjsMode(true);
    body->SetIsWasmFunction(true);

    WasmReaderInfo* readerInfo = RecyclerNew(m_recycler, WasmReaderInfo);
    readerInfo->m_funcInfo = wasmInfo;
    readerInfo->m_module = m_module;

    Js::AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
    info->SetWasmReaderInfo(readerInfo);
    info->SetWebAssemblyModule(m_module);

    Js::ArgSlot paramCount = wasmInfo->GetParamCount();
    info->SetArgCount(paramCount);
    info->SetWasmSignature(wasmInfo->GetSignature());
    Js::ArgSlot argSizeLength = max(paramCount, 3ui16);
    info->SetArgSizeArrayLength(argSizeLength);
    uint32* argSizeArray = RecyclerNewArrayLeafZ(m_recycler, uint32, argSizeLength);
    info->SetArgsSizesArray(argSizeArray);

    if (paramCount > 0)
    {
        // +1 here because asm.js includes the this pointer
        body->SetInParamsCount(paramCount + 1);
        body->SetReportedInParamsCount(paramCount + 1);
        info->SetArgTypeArray(RecyclerNewArrayLeaf(m_recycler, Js::AsmJsVarType::Which, paramCount));
    }
    else
    {
        // overwrite default value in this case
        body->SetHasImplicitArgIns(false);
    }
    for (Js::ArgSlot i = 0; i < paramCount; ++i)
    {
        WasmTypes::WasmType type = wasmInfo->GetSignature()->GetParam(i);
        info->SetArgType(WasmToAsmJs::GetAsmJsVarType(type), i);
        argSizeArray[i] = wasmInfo->GetSignature()->GetParamSize(i);
    }
    info->SetArgByteSize(wasmInfo->GetSignature()->GetParamsSize());
    if (wasmInfo->GetSignature()->GetResultCount() <= 1)
    {
        WasmTypes::WasmType returnType = wasmInfo->GetSignature()->GetResultCount() == 1 ? wasmInfo->GetSignature()->GetResult(0) : WasmTypes::Void;
        info->SetReturnType(WasmToAsmJs::GetAsmJsReturnType(returnType));
    }
    else
    {
        throw WasmCompilationException(_u("Multi return values not supported"));
    }
}

WAsmJs::RegisterSpace* AllocateRegisterSpace(ArenaAllocator* alloc, WAsmJs::Types)
{
    return Anew(alloc, WAsmJs::RegisterSpace, 1);
}

void WasmBytecodeGenerator::GenerateFunctionBytecode(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo, bool validateOnly /*= false*/)
{
    WasmBytecodeGenerator generator(scriptContext, readerinfo, validateOnly);
    generator.GenerateFunction();
    if (!generator.GetReader()->IsCurrentFunctionCompleted())
    {
        throw WasmCompilationException(_u("Invalid function format"));
    }
}

void WasmBytecodeGenerator::ValidateFunction(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo)
{
    GenerateFunctionBytecode(scriptContext, readerinfo, true);
}

WasmBytecodeGenerator::WasmBytecodeGenerator(Js::ScriptContext* scriptContext, WasmReaderInfo* readerInfo, bool validateOnly) :
    m_scriptContext(scriptContext),
    m_alloc(_u("WasmBytecodeGen"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_evalStack(&m_alloc),
    mTypedRegisterAllocator(&m_alloc, AllocateRegisterSpace, Simd::IsEnabled() ? 0 : 1 << WAsmJs::SIMD),
    m_blockInfos(&m_alloc),
    currentProfileId(0),
    isUnreachable(false)
{
    m_emptyWriter = Anew(&m_alloc, Js::EmptyWasmByteCodeWriter);
    m_writer = m_originalWriter = validateOnly ? m_emptyWriter : Anew(&m_alloc, Js::WasmByteCodeWriter);
    m_writer->Create();
    m_funcInfo = readerInfo->m_funcInfo;
    m_module = readerInfo->m_module;
    // Init reader to current func offset
    GetReader()->SeekToFunctionBody(m_funcInfo);

    const uint32 estimated = GetReader()->EstimateCurrentFunctionBytecodeSize();
    m_writer->InitData(&m_alloc, estimated);
}

void WasmBytecodeGenerator::GenerateFunction()
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (DO_WASM_TRACE_BYTECODE)
    {
        Output::Print(_u("Generate WebAssembly Bytecode: "));
        GetFunctionBody()->DumpFullFunctionName();
        Output::Print(_u("\n"));
    }
#endif
    if (PHASE_OFF(Js::WasmBytecodePhase, GetFunctionBody()))
    {
        throw WasmCompilationException(_u("Compilation skipped"));
    }
    Js::AutoProfilingPhase functionProfiler(m_scriptContext, Js::WasmBytecodePhase);
    Unused(functionProfiler);

    m_maxArgOutDepth = 0;

    m_writer->Begin(GetFunctionBody(), &m_alloc);
    struct AutoCleanupGeneratorState
    {
        WasmBytecodeGenerator* gen;
        AutoCleanupGeneratorState(WasmBytecodeGenerator* gen) : gen(gen) {}
        ~AutoCleanupGeneratorState()
        {
            if (gen)
            {
                TRACE_WASM(PHASE_TRACE(Js::WasmBytecodePhase, gen->GetFunctionBody()), _u("\nHad Compilation error!"));
                gen->GetReader()->FunctionEnd();
                gen->m_originalWriter->Reset();
            }
        }
        void Complete() { gen = nullptr; }
    };
    AutoCleanupGeneratorState autoCleanupGeneratorState(this);
    Js::ByteCodeLabel exitLabel = m_writer->DefineLabel();
    m_funcInfo->SetExitLabel(exitLabel);
    EnregisterLocals();

    // The function's yield type is the return type
    Js::ByteCodeLabel blockLabel = m_writer->DefineLabel();
    WasmBlock funcBlockData;
    funcBlockData.SetSignatureId(m_funcInfo->GetSignature()->GetSignatureId());
    m_funcBlock = PushLabel(funcBlockData, blockLabel, true, false);
    EnterEvalStackScope(m_funcBlock);
    EmitBlockCommon(m_funcBlock);
    m_writer->MarkAsmJsLabel(blockLabel);
    PolymorphicEmitInfo yieldInfo = PopLabel(blockLabel);
    if (yieldInfo.Count() > 0 || m_funcInfo->GetResultCount() == 0)
    {
        EmitReturnExpr(&yieldInfo);
    }
    DebugPrintOpEnd();
    ExitEvalStackScope(m_funcBlock);
    SetUnreachableState(false);
    m_writer->MarkAsmJsLabel(exitLabel);
    m_writer->EmptyAsm(Js::OpCodeAsmJs::Ret);
    m_writer->SetCallSiteCount(this->currentProfileId);
    m_writer->End();
    GetReader()->FunctionEnd();
    autoCleanupGeneratorState.Complete();
    // Make sure we don't have any unforeseen exceptions as we finalize the body
    AutoDisableInterrupt autoDisableInterrupt(m_scriptContext->GetThreadContext(), true);

#if DBG_DUMP
    if ((
        PHASE_DUMP(Js::WasmBytecodePhase, GetFunctionBody()) ||
        PHASE_DUMP(Js::ByteCodePhase, GetFunctionBody()) 
        ) && !IsValidating())
    {
        Js::AsmJsByteCodeDumper::Dump(GetFunctionBody(), &mTypedRegisterAllocator, nullptr);
    }
    if (PHASE_DUMP(Js::WasmOpCodeDistributionPhase, GetFunctionBody()))
    {
        m_module->GetReader()->PrintOps();
    }
#endif

    Js::AsmJsFunctionInfo* info = GetFunctionBody()->GetAsmJsFunctionInfo();
    mTypedRegisterAllocator.CommitToFunctionBody(GetFunctionBody());
    mTypedRegisterAllocator.CommitToFunctionInfo(info, GetFunctionBody());

    GetFunctionBody()->CheckAndSetOutParamMaxDepth(m_maxArgOutDepth);
    autoDisableInterrupt.Completed();
}

void WasmBytecodeGenerator::EnregisterLocals()
{
    uint32 nLocals = m_funcInfo->GetLocalCount();
    m_locals = AnewArray(&m_alloc, WasmLocal, nLocals);

    m_funcInfo->GetBody()->SetFirstTmpReg(nLocals);
    for (uint32 i = 0; i < nLocals; ++i)
    {
        WasmTypes::WasmType type = m_funcInfo->GetLocal(i);
        WasmRegisterSpace* regSpace = GetRegisterSpace(type);
        m_locals[i] = WasmLocal(regSpace->AcquireRegister(), type);

        // Zero only the locals not corresponding to formal parameters.
        if (i >= m_funcInfo->GetParamCount()) {
            switch (type)
            {
            case WasmTypes::F32:
                m_writer->AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, m_locals[i].location, 0.0f);
                break;
            case WasmTypes::F64:
                m_writer->AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, m_locals[i].location, 0.0);
                break;
            case WasmTypes::I32:
                m_writer->AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, m_locals[i].location, 0);
                break;
            case WasmTypes::I64:
                m_writer->AsmLong1Const1(Js::OpCodeAsmJs::Ld_LongConst, m_locals[i].location, 0);
                break;
#ifdef ENABLE_WASM_SIMD
            case WasmTypes::M128:
            {
                Simd::EnsureSimdIsEnabled();
                m_writer->WasmSimdConst(Js::OpCodeAsmJs::Simd128_LdC, m_locals[i].location, 0, 0, 0, 0);
                break;
            }
#endif
            default:
                Assume(UNREACHED);
            }
        }
    }
}

template <size_t lanes>
EmitInfo WasmBytecodeGenerator::EmitSimdBuildExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    const WasmTypes::WasmType resultType = signature[0];
    const WasmTypes::WasmType type = signature[1];

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    EmitInfo args[lanes];
    for (uint i = 0; i < lanes; i++)
    {
        args[i] = PopEvalStack(type);
    }

    switch (lanes)
    {
        case 4:
            m_writer->AsmReg5(op, resultReg, args[3].location, args[2].location, args[1].location, args[0].location);
            break;
        case 8:
            m_writer->AsmReg9(op, resultReg, args[7].location, args[6].location, args[5].location, args[4].location, args[3].location, args[2].location, args[1].location, args[0].location);
            break;
        case 16:
            m_writer->AsmReg17(op, resultReg, args[15].location, args[14].location, args[13].location, args[12].location, args[11].location, args[10].location, args[9].location, args[8].location, args[7].location, args[6].location, args[5].location, args[4].location, args[3].location, args[2].location, args[1].location, args[0].location);
            break;
        default:
            Assert(UNREACHED);
    }

    for (uint i = 0; i < lanes; i++)
    {
        ReleaseLocation(&args[i]);
    }

    return EmitInfo(resultReg, resultType);
}

void WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    DebugPrintOp(op);
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, imp, wat) \
    case opcode: \
        if (!imp) throw WasmCompilationException(_u("Operator %s is Not Yet Implemented"), _u(wat)); break;
#include "WasmBinaryOpCodes.h"
    default:
        break;
    }

    PolymorphicEmitInfo info;
    switch (op)
    {
    case wbGetGlobal:
        info = EmitGetGlobal();
        break;
    case wbSetGlobal:
        info = EmitSetGlobal();
        break;
    case wbGetLocal:
        info = EmitGetLocal();
        break;
    case wbSetLocal:
        info = EmitSetLocal(false);
        break;
    case wbTeeLocal:
        info = EmitSetLocal(true);
        break;
    case wbReturn:
        EmitReturnExpr();
        info = EmitInfo(WasmTypes::Any);
        break;
    case wbF32Const:
        info = EmitConst(WasmTypes::F32, GetReader()->m_currentNode.cnst);
        break;
    case wbF64Const:
        info = EmitConst(WasmTypes::F64, GetReader()->m_currentNode.cnst);
        break;
    case wbI32Const:
        info = EmitConst(WasmTypes::I32, GetReader()->m_currentNode.cnst);
        break;
    case wbI64Const:
        info = EmitConst(WasmTypes::I64, GetReader()->m_currentNode.cnst);
        break;
#ifdef ENABLE_WASM_SIMD
    case wbM128Const:
        Simd::EnsureSimdIsEnabled();
        info = EmitConst(WasmTypes::M128, GetReader()->m_currentNode.cnst);
        break;
#endif
    case wbBlock:
        info = EmitBlock();
        break;
    case wbLoop:
        info = EmitLoop();
        break;
    case wbCall:
        info = EmitCall<wbCall>();
        break;
    case wbCallIndirect:
        info = EmitCall<wbCallIndirect>();
        break;
    case wbIf:
        info = EmitIfElseExpr();
        break;
    case wbElse:
        throw WasmCompilationException(_u("Unexpected else opcode"));
    case wbEnd:
        throw WasmCompilationException(_u("Unexpected end opcode"));
    case wbBr:
        EmitBr();
        info = EmitInfo(WasmTypes::Any);
        break;
    case wbBrIf:
        info = EmitBrIf();
        break;
    case wbSelect:
        info = EmitSelect();
        break;
    case wbBrTable:
        EmitBrTable();
        info = EmitInfo(WasmTypes::Any);
        break;
    case wbDrop:
        info = EmitDrop();
        break;
    case wbNop:
        return;
    case wbCurrentMemory:
    {
        SetUsesMemory(0);
        Js::RegSlot tempReg = GetRegisterSpace(WasmTypes::I32)->AcquireTmpRegister();
        info = EmitInfo(tempReg, WasmTypes::I32);
        m_writer->AsmReg1(Js::OpCodeAsmJs::CurrentMemory_Int, tempReg);
        break;
    }
    case wbGrowMemory:
    {
        info = EmitGrowMemory();
        break;
    }
    case wbUnreachable:
        m_writer->EmptyAsm(Js::OpCodeAsmJs::Unreachable_Void);
        SetUnreachableState(true);
        info = EmitInfo(WasmTypes::Any);
        break;
#ifdef ENABLE_WASM_SIMD
    case wbM128Bitselect:
        Simd::EnsureSimdIsEnabled();
        info = EmitM128BitSelect();
        break;
    case wbV8X16Shuffle:
        Simd::EnsureSimdIsEnabled();
        info = EmitV8X16Shuffle();
        break;
#define WASM_EXTRACTLANE_OPCODE(opname, opcode, sig, asmjsop, ...) \
    case wb##opname: \
        Simd::EnsureSimdIsEnabled();\
        info = EmitExtractLaneExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_REPLACELANE_OPCODE(opname, opcode, sig, asmjsop, ...) \
    case wb##opname: \
        Simd::EnsureSimdIsEnabled();\
        info = EmitReplaceLaneExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#endif
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, imp, viewtype, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess<false, false>(wb##opname, WasmOpCodeSignatures::sig, viewtype); \
        break;
#define WASM_ATOMICREAD_OPCODE(opname, opcode, sig, imp, viewtype, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess<false, true>(wb##opname, WasmOpCodeSignatures::sig, viewtype); \
        break;
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, imp, viewtype, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess<true, false>(wb##opname, WasmOpCodeSignatures::sig, viewtype); \
        break;
#define WASM_ATOMICSTORE_OPCODE(opname, opcode, sig, imp, viewtype, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess<true, true>(wb##opname, WasmOpCodeSignatures::sig, viewtype); \
        break;
#define WASM_SIMD_MEMREAD_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, ...) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitSimdMemAccess(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig, viewtype, dataWidth, false); \
        break;
#define WASM_SIMD_MEMSTORE_OPCODE(opname, opcode, sig, asmjsop, viewtype, dataWidth, ...) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitSimdMemAccess(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig, viewtype, dataWidth, true); \
        break;
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjsop, imp, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 3);\
        info = EmitBinExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjsop, imp, wat) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 2);\
        info = EmitUnaryExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_SIMD_BUILD_OPCODE(opname, opcode, sig, asmjop, lanes, ...) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 2);\
        info = EmitSimdBuildExpr<lanes>(Js::OpCodeAsmJs::##asmjop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_EMPTY__OPCODE(opname, opcode, asmjsop, imp, wat) \
    case wb##opname: \
        m_writer->EmptyAsm(Js::OpCodeAsmJs::##asmjsop);\
        break;
#include "WasmBinaryOpCodes.h"
    default:
        throw WasmCompilationException(_u("Unknown expression's op 0x%X"), op);
    }

    PushEvalStack(info);
    DebugPrintOpEnd();
}


EmitInfo WasmBytecodeGenerator::EmitGetGlobal()
{
    uint32 globalIndex = GetReader()->m_currentNode.var.num;
    WasmGlobal* global = m_module->GetGlobal(globalIndex);

    WasmTypes::WasmType type = global->GetType();

    Js::RegSlot slot = m_module->GetOffsetForGlobal(global);

    CompileAssert(WasmTypes::I32 == 1);
    CompileAssert(WasmTypes::I64 == 2);
    CompileAssert(WasmTypes::F32 == 3);
    CompileAssert(WasmTypes::F64 == 4);
    static const Js::OpCodeAsmJs globalOpcodes[] = {
        Js::OpCodeAsmJs::LdSlot_Int,
        Js::OpCodeAsmJs::LdSlot_Long,
        Js::OpCodeAsmJs::LdSlot_Flt,
        Js::OpCodeAsmJs::LdSlot_Db
    };

    WasmRegisterSpace* regSpace = GetRegisterSpace(type);
    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();
    EmitInfo info(tmpReg, type);

    m_writer->AsmSlot(globalOpcodes[type - 1], tmpReg, WasmBytecodeGenerator::ModuleEnvRegister, slot);

    return info;
}

EmitInfo WasmBytecodeGenerator::EmitSetGlobal()
{
    uint32 globalIndex = GetReader()->m_currentNode.var.num;
    WasmGlobal* global = m_module->GetGlobal(globalIndex);
    Js::RegSlot slot = m_module->GetOffsetForGlobal(global);

    WasmTypes::WasmType type = global->GetType();
    EmitInfo info = PopEvalStack(type);

    CompileAssert(WasmTypes::I32 == 1);
    CompileAssert(WasmTypes::I64 == 2);
    CompileAssert(WasmTypes::F32 == 3);
    CompileAssert(WasmTypes::F64 == 4);
    static const Js::OpCodeAsmJs globalOpcodes[] = {
        Js::OpCodeAsmJs::StSlot_Int,
        Js::OpCodeAsmJs::StSlot_Long,
        Js::OpCodeAsmJs::StSlot_Flt,
        Js::OpCodeAsmJs::StSlot_Db
    };

    m_writer->AsmSlot(globalOpcodes[type - 1], info.location, WasmBytecodeGenerator::ModuleEnvRegister, slot);
    ReleaseLocation(&info);

    return EmitInfo();
}

EmitInfo WasmBytecodeGenerator::EmitGetLocal()
{
    uint32 localIndex = GetReader()->m_currentNode.var.num;
    if (m_funcInfo->GetLocalCount() <= localIndex)
    {
        throw WasmCompilationException(_u("%u is not a valid local"), localIndex);
    }

    WasmLocal local = m_locals[localIndex];

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace* regSpace = GetRegisterSpace(local.type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    m_writer->AsmReg2(op, tmpReg, local.location);

    return EmitInfo(tmpReg, local.type);
}

EmitInfo WasmBytecodeGenerator::EmitSetLocal(bool tee)
{
    uint32 localNum = GetReader()->m_currentNode.var.num;
    if (localNum >= m_funcInfo->GetLocalCount())
    {
        throw WasmCompilationException(_u("%u is not a valid local"), localNum);
    }

    WasmLocal local = m_locals[localNum];

    EmitInfo info = PopEvalStack(local.type);

    m_writer->AsmReg2(GetLoadOp(local.type), local.location, info.location);

    if (tee)
    {
        if (info.type == WasmTypes::Any)
        {
            throw WasmCompilationException(_u("Can't tee_local unreachable values"));
        }
        return info;
    }
    else
    {
        ReleaseLocation(&info);
        return EmitInfo();
    }
}

EmitInfo WasmBytecodeGenerator::EmitConst(WasmTypes::WasmType type, WasmConstLitNode cnst)
{
    Js::RegSlot tmpReg = GetRegisterSpace(type)->AcquireTmpRegister();
    EmitInfo dst(tmpReg, type);
    EmitLoadConst(dst, cnst);
    return dst;
}

void WasmBytecodeGenerator::EmitLoadConst(EmitInfo dst, WasmConstLitNode cnst)
{
    switch (dst.type)
    {
    case WasmTypes::F32:
        m_writer->AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, dst.location, cnst.f32);
        break;
    case WasmTypes::F64:
        m_writer->AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, dst.location, cnst.f64);
        break;
    case WasmTypes::I32:
        m_writer->AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, dst.location, cnst.i32);
        break;
    case WasmTypes::I64:
        m_writer->AsmLong1Const1(Js::OpCodeAsmJs::Ld_LongConst, dst.location, cnst.i64);
        break;
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
    {
        Simd::EnsureSimdIsEnabled();
        m_writer->WasmSimdConst(Js::OpCodeAsmJs::Simd128_LdC, dst.location, cnst.v128[0], cnst.v128[1], cnst.v128[2], cnst.v128[3]);
        break;
    }
#endif
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown type %u"), dst.type);
    }
}

WasmConstLitNode WasmBytecodeGenerator::GetZeroCnst()
{
    WasmConstLitNode cnst = {0};
    return cnst;
}

void WasmBytecodeGenerator::EnsureStackAvailable()
{
    if (!ThreadContext::IsCurrentStackAvailable(Js::Constants::MinStackCompile))
    {
        throw WasmCompilationException(_u("Maximum supported nested blocks reached"));
    }
}

void WasmBytecodeGenerator::EmitBlockCommon(BlockInfo* blockInfo, bool* endOnElse /*= nullptr*/)
{
    EnsureStackAvailable();
    bool canResetUnreachable = !IsUnreachable();
    WasmOp op;
    EnterEvalStackScope(blockInfo);

    if(endOnElse) *endOnElse = false;
    do {
        op = GetReader()->ReadExpr();
        if (op == wbEnd)
        {
            break;
        }
        if (endOnElse && op == wbElse)
        {
            *endOnElse = true;
            break;
        }
        EmitExpr(op);
    } while (true);
    DebugPrintOp(op);
    if (blockInfo->HasYield())
    {
        PolymorphicEmitInfo info = PopStackPolymorphic(blockInfo);
        YieldToBlock(blockInfo, info);
        ReleaseLocation(&info);
    }
    ExitEvalStackScope(blockInfo);
    if (canResetUnreachable)
    {
        SetUnreachableState(false);
    }
}

PolymorphicEmitInfo WasmBytecodeGenerator::EmitBlock()
{
    Js::ByteCodeLabel blockLabel = m_writer->DefineLabel();

    BlockInfo* blockInfo = PushLabel(GetReader()->m_currentNode.block, blockLabel);
    EmitBlockCommon(blockInfo);
    m_writer->MarkAsmJsLabel(blockLabel);
    PolymorphicEmitInfo yieldInfo = PopLabel(blockLabel);

    // block yields last value
    return yieldInfo;
}

PolymorphicEmitInfo WasmBytecodeGenerator::EmitLoop()
{
    Js::ByteCodeLabel loopTailLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel loopHeadLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel loopLandingPadLabel = m_writer->DefineLabel();

    // Push possibly yielding loop label before capturing all the yielding registers
    BlockInfo* implicitBlockInfo = PushLabel(GetReader()->m_currentNode.block, loopTailLabel);

    // Save the first tmp (per type) of this loop to discern a yield outside the loop in jitloopbody scenario
    Js::RegSlot curRegs[WAsmJs::LIMIT];
    for (WAsmJs::Types type = WAsmJs::Types(0); type != WAsmJs::LIMIT; type = WAsmJs::Types(type + 1))
    {
        uint32 minYield = 0;
        if (!mTypedRegisterAllocator.IsTypeExcluded(type))
        {
            CompileAssert(sizeof(minYield) == sizeof(Js::RegSlot));
            minYield = static_cast<uint32>(mTypedRegisterAllocator.GetRegisterSpace(type)->PeekNextTmpRegister());
        }
        curRegs[type] = minYield;
    }
    uint32 loopId = m_writer->WasmLoopStart(loopHeadLabel, curRegs);

    // Internally we create a block for loop to exit, but semantically, they don't exist so pop it
    m_blockInfos.Pop();

    // We don't want nested block to jump directly to the loop header
    // instead, jump to the landing pad and let it jump back to the loop header
    PushLabel(GetReader()->m_currentNode.block, loopLandingPadLabel, false);
    EmitBlockCommon(implicitBlockInfo);
    PopLabel(loopLandingPadLabel);

    // By default we don't loop, jump over the landing pad
    m_writer->AsmBr(loopTailLabel);
    m_writer->MarkAsmJsLabel(loopLandingPadLabel);
    m_writer->AsmBr(loopHeadLabel);

    // Put the implicit block back on the stack and yield the last expression to it
    m_blockInfos.Push(implicitBlockInfo);
    m_writer->MarkAsmJsLabel(loopTailLabel);
    // Pop the implicit block to resolve the yield correctly
    PolymorphicEmitInfo loopInfo = PopLabel(loopTailLabel);
    m_writer->ExitLoop(loopId);

    return loopInfo;
}

template<WasmOp wasmOp>
PolymorphicEmitInfo WasmBytecodeGenerator::EmitCall()
{
    uint32 funcNum = Js::Constants::UninitializedValue;
    uint32 signatureId = Js::Constants::UninitializedValue;
    WasmSignature* calleeSignature = nullptr;
    Js::ProfileId profileId = Js::Constants::NoProfileId;
    EmitInfo indirectIndexInfo;
    const bool isImportCall = GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Import;
    Assert(isImportCall || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Function || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::ImportThunk);
    switch (wasmOp)
    {
    case wbCall:
    {
        funcNum = GetReader()->m_currentNode.call.num;
        WasmFunctionInfo* calleeInfo = m_module->GetWasmFunctionInfo(funcNum);
        calleeSignature = calleeInfo->GetSignature();
        // currently only handle inlining internal function calls
        // in future we can expand to all calls by adding checks in inliner and falling back to call in case ScriptFunction doesn't match
        if (GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Function && !PHASE_TRACE1(Js::WasmInOutPhase))
        {
            profileId = GetNextProfileId();
        }
        break;
    }
    case wbCallIndirect:
        indirectIndexInfo = PopEvalStack(WasmTypes::I32, _u("Indirect call index must be int type"));
        signatureId = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetSignature(signatureId);
        break;
    default:
        Assume(UNREACHED);
    }

    const auto argOverflow = []
    {
        throw WasmCompilationException(_u("Argument size too big"));
    };

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (isImportCall)
    {
        argSize = ArgSlotMath::Mul(calleeSignature->GetParamCount(), sizeof(Js::Var), argOverflow);
        startCallOp = Js::OpCodeAsmJs::StartCall;
    }
    else
    {
        startCallOp = Js::OpCodeAsmJs::I_StartCall;
        argSize = calleeSignature->GetParamsSize();
    }
    // Add return value
    argSize = ArgSlotMath::Add(argSize, sizeof(Js::Var), argOverflow);
    if (!Math::IsAligned<Js::ArgSlot>(argSize, sizeof(Js::Var)))
    {
        AssertMsg(UNREACHED, "Wasm argument size should always be Var aligned");
        throw WasmCompilationException(_u("Internal Error"));
    }

    m_writer->AsmStartCall(startCallOp, argSize);

    Js::ArgSlot nArgs = calleeSignature->GetParamCount();

    // copy args into a list so they could be generated in the right order (FIFO)
    EmitInfo* argsList = AnewArray(&m_alloc, EmitInfo, nArgs);
    for (int i = int(nArgs) - 1; i >= 0; --i)
    {
        EmitInfo info = PopEvalStack(calleeSignature->GetParam((Js::ArgSlot)i), _u("Call argument does not match formal type"));
        // We can release the location of the arguments now, because we won't create new temps between start/call
        argsList[i] = info;
    }

    // Skip the this pointer (aka undefined)
    uint32 argLoc = 1;
    for (Js::ArgSlot i = 0; i < nArgs; ++i)
    {
        EmitInfo info = argsList[i];

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        switch (info.type)
        {
        case WasmTypes::F32:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Flt : Js::OpCodeAsmJs::I_ArgOut_Flt;
            break;
        case WasmTypes::F64:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Db : Js::OpCodeAsmJs::I_ArgOut_Db;
            break;
        case WasmTypes::I32:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
            break;
        case WasmTypes::I64:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Long : Js::OpCodeAsmJs::I_ArgOut_Long;
            break;
#ifdef ENABLE_WASM_SIMD
        case WasmTypes::M128:
            Simd::EnsureSimdIsEnabled();
            argOp = isImportCall ? Js::OpCodeAsmJs::Simd128_ArgOut_F4 : Js::OpCodeAsmJs::Simd128_I_ArgOut_F4;
            break;
#endif
        case WasmTypes::Any:
            // In unreachable mode allow any type as argument since we won't actually emit the call
            Assert(IsUnreachable());
            if (IsUnreachable())
            {
                argOp = Js::OpCodeAsmJs::ArgOut_Int;
                break;
            }
            // Fall through
        default:
            WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
            throw WasmCompilationException(_u("Unknown argument type %u"), info.type);
        }

        m_writer->AsmReg2(argOp, argLoc, info.location);

        // Calculated next argument Js::Var location
        if (isImportCall)
        {
            ++argLoc;
        }
        else
        {
            const Js::ArgSlot currentArgSize = calleeSignature->GetParamSize(i);
            Assert(Math::IsAligned<Js::ArgSlot>(currentArgSize, sizeof(Js::Var)));
            argLoc += currentArgSize / sizeof(Js::Var);
        }
    }

    Js::RegSlot funcReg = GetRegisterSpace(WasmTypes::Ptr)->AcquireTmpRegister();

    // emit call
    switch (wasmOp)
    {
    case wbCall:
    {
        uint32 offset = isImportCall ? m_module->GetImportFuncOffset() : m_module->GetFuncOffset();
        uint32 index = UInt32Math::Add(offset, funcNum);
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdSlot, funcReg, Js::AsmJsFunctionMemory::ModuleEnvRegister, index);
        break;
    }
    case wbCallIndirect:
    {
        Js::RegSlot slotReg = GetRegisterSpace(WasmTypes::Ptr)->AcquireTmpRegister();
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdSlotArr, slotReg, Js::AsmJsFunctionMemory::ModuleEnvRegister, m_module->GetTableEnvironmentOffset());
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdArr_WasmFunc, funcReg, slotReg, indirectIndexInfo.location);
        GetRegisterSpace(WasmTypes::Ptr)->ReleaseTmpRegister(slotReg);
        m_writer->AsmReg1IntConst1(Js::OpCodeAsmJs::CheckSignature, funcReg, calleeSignature->GetSignatureId());
        break;
    }
    default:
        Assume(UNREACHED);
    }

    // calculate number of RegSlots(Js::Var) the call consumes
    PolymorphicEmitInfo retInfo;
    retInfo.Init(calleeSignature->GetResultCount(), &m_alloc);
    for (uint32 i = 0; i < calleeSignature->GetResultCount(); ++i)
    {
        retInfo.SetInfo(EmitInfo(calleeSignature->GetResult(i)), i);
    }
    Js::ArgSlot args;
    if (isImportCall)
    {
        args = calleeSignature->GetParamCount();
        ArgSlotMath::Inc(args, argOverflow);

        Js::RegSlot varRetReg = GetRegisterSpace(WasmTypes::Ptr)->AcquireTmpRegister();
        AssertOrFailFastMsg(calleeSignature->GetResultCount() <= 1, "Multiple results from function imports not supported");
        WasmTypes::WasmType singleResType = calleeSignature->GetResultCount() > 0 ? calleeSignature->GetResult(0) : WasmTypes::Void;
        m_writer->AsmCall(Js::OpCodeAsmJs::Call, varRetReg, funcReg, args, WasmToAsmJs::GetAsmJsReturnType(singleResType), profileId);

        GetRegisterSpace(WasmTypes::Ptr)->ReleaseTmpRegister(varRetReg);
        GetRegisterSpace(WasmTypes::Ptr)->ReleaseTmpRegister(funcReg);

        ReleaseLocation(&indirectIndexInfo);
        //registers need to be released from higher ordinals to lower
        for (uint32 i = nArgs; i > 0; --i)
        {
            ReleaseLocation(&(argsList[i - 1]));
        }

        // emit result coercion
        if (calleeSignature->GetResultCount() > 0)
        {
            Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
            switch (singleResType)
            {
            case WasmTypes::F32:
                convertOp = Js::OpCodeAsmJs::Conv_VTF;
                break;
            case WasmTypes::F64:
                convertOp = Js::OpCodeAsmJs::Conv_VTD;
                break;
            case WasmTypes::I32:
                convertOp = Js::OpCodeAsmJs::Conv_VTI;
                break;
            case WasmTypes::I64:
                convertOp = Js::OpCodeAsmJs::Conv_VTL;
                break;
#ifdef ENABLE_WASM_SIMD
            case WasmTypes::M128:
                throw WasmCompilationException(_u("Return type: m128 not supported in import calls"));
#endif
            default:
                WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
                throw WasmCompilationException(_u("Unknown call return type %u"), singleResType);
            }
            Js::RegSlot location = GetRegisterSpace(singleResType)->AcquireTmpRegister();
            retInfo.SetInfo(EmitInfo(location, singleResType), 0);
            m_writer->AsmReg2(convertOp, location, varRetReg);
        }
    }
    else
    {
        GetRegisterSpace(WasmTypes::Ptr)->ReleaseTmpRegister(funcReg);
        ReleaseLocation(&indirectIndexInfo);
        //registers need to be released from higher ordinals to lower
        for (uint32 i = nArgs; i > 0; --i)
        {
            ReleaseLocation(&(argsList[i - 1]));
        }

        for (uint32 i = 0; i < calleeSignature->GetResultCount(); ++i)
        {
            EmitInfo info(calleeSignature->GetResult(i));
            info.location = GetRegisterSpace(info.type)->AcquireTmpRegister();
            retInfo.SetInfo(info, i);
        }

        EmitInfo singleResultInfo = retInfo.Count() > 0 ? retInfo.GetInfo(0) : EmitInfo(WasmTypes::Void);
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var))));
        // todo:: add bytecode to call and set aside multi results
        m_writer->AsmCall(Js::OpCodeAsmJs::I_Call, singleResultInfo.location, funcReg, args, WasmToAsmJs::GetAsmJsReturnType(singleResultInfo.type), profileId);
    }
    AdeleteArray(&m_alloc, nArgs, argsList);

    // WebAssemblyArrayBuffer is not detachable, no need to check for detached state here

    // track stack requirements for out params

    // + 1 for return address
    uint32 maxDepthForLevel = args + 1;
    if (maxDepthForLevel > m_maxArgOutDepth)
    {
        m_maxArgOutDepth = maxDepthForLevel;
    }

    return retInfo;
}

PolymorphicEmitInfo WasmBytecodeGenerator::EmitIfElseExpr()
{
    Js::ByteCodeLabel falseLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer->DefineLabel();

    EmitInfo checkExpr = PopEvalStack(WasmTypes::I32, _u("If expression must have type i32"));
    ReleaseLocation(&checkExpr);

    m_writer->AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    BlockInfo* blockInfo = PushLabel(GetReader()->m_currentNode.block, endLabel);
    bool endOnElse = false;
    EmitBlockCommon(blockInfo, &endOnElse);
    EnsureYield(blockInfo);

    m_writer->AsmBr(endLabel);
    m_writer->MarkAsmJsLabel(falseLabel);

    EmitInfo retInfo;
    EmitInfo falseExpr;
    if (endOnElse)
    {
        if (blockInfo->HasYield())
        {
            // Indicate that we need this block to yield a value
            blockInfo->didYield = false;
        }
        EmitBlockCommon(blockInfo);
        EnsureYield(blockInfo);
    }
    else if (blockInfo->HasYield())
    {
        throw WasmCompilationException(_u("Expected an else block when 'if' returns a value"));
    }
    m_writer->MarkAsmJsLabel(endLabel);

    return PopLabel(endLabel);
}

void WasmBytecodeGenerator::EmitBrTable()
{
    const uint32 numTargets = GetReader()->m_currentNode.brTable.numTargets;
    const uint32* targetTable = GetReader()->m_currentNode.brTable.targetTable;
    const uint32 defaultEntry = GetReader()->m_currentNode.brTable.defaultTarget;

    // Compile scrutinee
    EmitInfo scrutineeInfo = PopStackPolymorphic(EmitInfo(WasmTypes::I32), _u("br_table expression must be of type i32")).GetInfo(0);

    m_writer->AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeInfo.location, scrutineeInfo.location);
    PolymorphicEmitInfo yieldValue;
    BlockInfo* defaultBlockInfo = GetBlockInfo(defaultEntry);
    if (defaultBlockInfo->HasYield())
    {
        yieldValue = PopStackPolymorphic(defaultBlockInfo);
    }

    // Compile cases
    for (uint32 i = 0; i < numTargets; i++)
    {
        uint32 target = targetTable[i];
        BlockInfo* blockInfo = GetBlockInfo(target);
        if (!defaultBlockInfo->IsEquivalent(blockInfo))
        {
            throw WasmCompilationException(_u("br_table target %u signature mismatch"));
        }
        YieldToBlock(blockInfo, yieldValue);
        m_writer->AsmBrReg1Const1(Js::OpCodeAsmJs::Case_IntConst, blockInfo->label, scrutineeInfo.location, i);
    }

    YieldToBlock(defaultBlockInfo, yieldValue);
    m_writer->AsmBr(defaultBlockInfo->label, Js::OpCodeAsmJs::EndSwitch_Int);
    ReleaseLocation(&scrutineeInfo);
    ReleaseLocation(&yieldValue);

    SetUnreachableState(true);
}

EmitInfo WasmBytecodeGenerator::EmitGrowMemory()
{
    SetUsesMemory(0);

    EmitInfo info = PopEvalStack(WasmTypes::I32, _u("Invalid type for GrowMemory"));

    m_writer->AsmReg2(Js::OpCodeAsmJs::GrowMemory, info.location, info.location);
    return info;
}

EmitInfo WasmBytecodeGenerator::EmitDrop()
{
    EmitInfo info = PopValuePolymorphic();
    ReleaseLocation(&info);
    return EmitInfo();
}

EmitInfo WasmBytecodeGenerator::EmitBinExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType lhsType = signature[1];
    WasmTypes::WasmType rhsType = signature[2];

    EmitInfo rhs = PopEvalStack(rhsType);
    EmitInfo lhs = PopEvalStack(lhsType);

    ReleaseLocation(&rhs);
    ReleaseLocation(&lhs);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer->AsmReg3(op, resultReg, lhs.location, rhs.location);

    return EmitInfo(resultReg, resultType);
}

EmitInfo WasmBytecodeGenerator::EmitUnaryExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType inputType = signature[1];

    EmitInfo info = PopEvalStack(inputType);

    ReleaseLocation(&info);
    if (resultType == WasmTypes::Void)
    {
        m_writer->AsmReg2(op, 0, info.location);
        return EmitInfo();
    }

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer->AsmReg2(op, resultReg, info.location);

    return EmitInfo(resultReg, resultType);
}

#ifdef ENABLE_WASM_SIMD
void WasmBytecodeGenerator::CheckLaneIndex(Js::OpCodeAsmJs op, const uint index)
{
    uint numLanes;
    switch (op) 
    {
    case Js::OpCodeAsmJs::Simd128_ExtractLane_I2:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_I2:
    case Js::OpCodeAsmJs::Simd128_ExtractLane_D2:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_D2:
        numLanes = 2;
        break;
    case Js::OpCodeAsmJs::Simd128_ExtractLane_I4:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_I4:
    case Js::OpCodeAsmJs::Simd128_ExtractLane_F4:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_F4:
        numLanes = 4;
        break;
    case Js::OpCodeAsmJs::Simd128_ExtractLane_I8:
    case Js::OpCodeAsmJs::Simd128_ExtractLane_U8:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_I8:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_U8:
        numLanes = 8;
        break;
    case Js::OpCodeAsmJs::Simd128_ExtractLane_I16:
    case Js::OpCodeAsmJs::Simd128_ExtractLane_U16:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_I16:
    case Js::OpCodeAsmJs::Simd128_ReplaceLane_U16:
        numLanes = 16;
        break;
    default:
        Assert(UNREACHED);
        numLanes = 0;
    }

    if (index >= numLanes)
    {
        throw WasmCompilationException(_u("index is out of range"));
    }
}

EmitInfo WasmBytecodeGenerator::EmitLaneIndex(Js::OpCodeAsmJs op)
{
    const uint index = GetReader()->m_currentNode.lane.index;
    CheckLaneIndex(op, index);
    WasmConstLitNode dummy;
    dummy.i32 = index;
    return EmitConst(WasmTypes::I32, dummy);
}

EmitInfo WasmBytecodeGenerator::EmitReplaceLaneExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature) {

    const WasmTypes::WasmType resultType = signature[0];
    const WasmTypes::WasmType valueType = signature[1];
    EmitInfo valueArg = PopEvalStack(valueType, _u("lane argument type mismatch"));

    EmitInfo simdArg = PopEvalStack(WasmTypes::M128, _u("simd argument type mismatch"));
    Assert(resultType == WasmTypes::M128);

    EmitInfo indexInfo = EmitLaneIndex(op);
    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();
    EmitInfo result(resultReg, resultType);

    m_writer->AsmReg4(op, resultReg, simdArg.location, indexInfo.location, valueArg.location);
    ReleaseLocation(&indexInfo);
    return result;
}

EmitInfo WasmBytecodeGenerator::EmitM128BitSelect()
{
    EmitInfo mask = PopEvalStack(WasmTypes::M128);
    EmitInfo arg2Info = PopEvalStack(WasmTypes::M128);
    EmitInfo arg1Info = PopEvalStack(WasmTypes::M128);
    Js::RegSlot resultReg = GetRegisterSpace(WasmTypes::M128)->AcquireTmpRegister();
    EmitInfo resultInfo(resultReg, WasmTypes::M128);
    m_writer->AsmReg4(Js::OpCodeAsmJs::Simd128_BitSelect_I4, resultReg, arg1Info.location, arg2Info.location, mask.location);
    return resultInfo;
}

EmitInfo WasmBytecodeGenerator::EmitV8X16Shuffle()
{
    EmitInfo arg2Info = PopEvalStack(WasmTypes::M128);
    EmitInfo arg1Info = PopEvalStack(WasmTypes::M128);

    Js::RegSlot resultReg = GetRegisterSpace(WasmTypes::M128)->AcquireTmpRegister();
    EmitInfo resultInfo(resultReg, WasmTypes::M128);

    uint8* indices = GetReader()->m_currentNode.shuffle.indices;
    for (uint i = 0; i < Simd::MAX_LANES; i++)
    {
        if (indices[i] >= Simd::MAX_LANES * 2)
        {
            throw WasmCompilationException(_u("%u-th shuffle lane index is larger than %u"), i, (Simd::MAX_LANES * 2 -1));
        }
    }

    m_writer->AsmShuffle(Js::OpCodeAsmJs::Simd128_Shuffle_V8X16, resultReg, arg1Info.location, arg2Info.location, indices);
    return resultInfo;
}

EmitInfo WasmBytecodeGenerator::EmitExtractLaneExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType simdArgType = signature[1];

    EmitInfo simdArgInfo = PopEvalStack(simdArgType, _u("Argument should be of type M128"));

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();
    EmitInfo resultInfo(resultReg, resultType);

    //put index into a register to reuse the existing infra in Interpreter and Compiler
    EmitInfo indexInfo = EmitLaneIndex(op);

    m_writer->AsmReg3(op, resultReg, simdArgInfo.location, indexInfo.location);
    ReleaseLocation(&indexInfo);
    ReleaseLocation(&simdArgInfo);
    return resultInfo;
}

EmitInfo WasmBytecodeGenerator::EmitSimdMemAccess(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType, uint8 dataWidth, bool isStore)
{

    WasmTypes::WasmType type = signature[0];
    SetUsesMemory(0);

    const uint32 mask = Js::ArrayBufferView::ViewMask[viewType];
    const uint alignment = GetReader()->m_currentNode.mem.alignment;
    const uint offset = GetReader()->m_currentNode.mem.offset;

    if ((mask << 1) & (1 << alignment))
    {
        throw WasmCompilationException(_u("alignment must not be larger than natural"));
    }

    EmitInfo rhsInfo;
    if (isStore)
    {
        rhsInfo = PopEvalStack(type, _u("Invalid type for store op"));
    }
    EmitInfo exprInfo = PopEvalStack(WasmTypes::I32, _u("Index expression must be of type i32"));

    if (isStore)
    {
        m_writer->AsmSimdTypedArr(op, rhsInfo.location, exprInfo.location, dataWidth, viewType, offset);

        ReleaseLocation(&rhsInfo);
        ReleaseLocation(&exprInfo);

        return EmitInfo();
    }

    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();
    m_writer->AsmSimdTypedArr(op, resultReg, exprInfo.location, dataWidth, viewType, offset);

    EmitInfo yieldInfo = EmitInfo(resultReg, type);
    ReleaseLocation(&exprInfo);

    return yieldInfo;
}
#endif

template<bool isStore, bool isAtomic>
EmitInfo WasmBytecodeGenerator::EmitMemAccess(WasmOp wasmOp, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType)
{
    Assert(!isAtomic || Wasm::Threads::IsEnabled());
    WasmTypes::WasmType type = signature[0];
    SetUsesMemory(0);

    const uint32 naturalAlignment = Js::ArrayBufferView::NaturalAlignment[viewType];
    const uint32 alignment = GetReader()->m_currentNode.mem.alignment;
    const uint32 offset = GetReader()->m_currentNode.mem.offset;

    if (alignment > naturalAlignment)
    {
        throw WasmCompilationException(_u("alignment must not be larger than natural"));
    }
    if (isAtomic && alignment != naturalAlignment)
    {
        throw WasmCompilationException(_u("invalid alignment for atomic RW. Expected %u, got %u"), naturalAlignment, alignment);
    }

    // Stores
    if (isStore)
    {
        EmitInfo rhsInfo = PopEvalStack(type, _u("Invalid type for store op"));
        EmitInfo exprInfo = PopEvalStack(WasmTypes::I32, _u("Index expression must be of type i32"));
        Js::OpCodeAsmJs op = isAtomic ? Js::OpCodeAsmJs::StArrAtomic : Js::OpCodeAsmJs::StArrWasm;
        m_writer->WasmMemAccess(op, rhsInfo.location, exprInfo.location, offset, viewType);
        ReleaseLocation(&rhsInfo);
        ReleaseLocation(&exprInfo);

        return EmitInfo();
    }

    // Loads
    EmitInfo exprInfo = PopEvalStack(WasmTypes::I32, _u("Index expression must be of type i32"));
    ReleaseLocation(&exprInfo);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();
    Js::OpCodeAsmJs op = isAtomic ? Js::OpCodeAsmJs::LdArrAtomic : Js::OpCodeAsmJs::LdArrWasm;
    m_writer->WasmMemAccess(op, resultReg, exprInfo.location, offset, viewType);

    return EmitInfo(resultReg, type);
}

void WasmBytecodeGenerator::EmitReturnExpr(PolymorphicEmitInfo* explicitRetInfo)
{
    PolymorphicEmitInfo retExprInfo = explicitRetInfo ? *explicitRetInfo : PopStackPolymorphic(m_funcBlock);
    for (uint32 i = 0; i < retExprInfo.Count(); ++i)
    {
        EmitInfo info = retExprInfo.GetInfo(i);
        Js::OpCodeAsmJs retOp = GetReturnOp(info.type);
        m_writer->Conv(retOp, 0, info.location);
        ReleaseLocation(&info);
    }
    m_writer->AsmBr(m_funcInfo->GetExitLabel());

    SetUnreachableState(true);
}

EmitInfo WasmBytecodeGenerator::EmitSelect()
{
    EmitInfo conditionInfo = PopEvalStack(WasmTypes::I32, _u("select condition must have i32 type"));
    EmitInfo falseInfo = PopValuePolymorphic();
    EmitInfo trueInfo = PopEvalStack(falseInfo.type, _u("select operands must both have same type"));
    ReleaseLocation(&conditionInfo);
    ReleaseLocation(&falseInfo);
    ReleaseLocation(&trueInfo);

    if (IsUnreachable())
    {
        if (trueInfo.type == WasmTypes::Any)
        {
            // Report the type of falseInfo for type checking
            return EmitInfo(falseInfo.type);
        }
        // Otherwise report the type of trueInfo for type checking
        return EmitInfo(trueInfo.type);
    }
    WasmTypes::WasmType selectType = trueInfo.type;
    EmitInfo resultInfo = EmitInfo(GetRegisterSpace(selectType)->AcquireTmpRegister(), selectType);

    Js::ByteCodeLabel falseLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel doneLabel = m_writer->DefineLabel();
    Js::OpCodeAsmJs loadOp = GetLoadOp(resultInfo.type);


    // var result;
    // if (!condition) goto:condFalse
    // result = trueRes;
    // goto:done;
    //:condFalse
    // result = falseRes;
    //:done
    m_writer->AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, conditionInfo.location);
    m_writer->AsmReg2(loadOp, resultInfo.location, trueInfo.location);
    m_writer->AsmBr(doneLabel);
    m_writer->MarkAsmJsLabel(falseLabel);
    m_writer->AsmReg2(loadOp, resultInfo.location, falseInfo.location);
    m_writer->MarkAsmJsLabel(doneLabel);

    return resultInfo;
}

void WasmBytecodeGenerator::EmitBr()
{
    uint32 depth = GetReader()->m_currentNode.br.depth;

    BlockInfo* blockInfo = GetBlockInfo(depth);
    if (blockInfo->HasYield())
    {
        PolymorphicEmitInfo info = PopStackPolymorphic(blockInfo);
        YieldToBlock(blockInfo, info);
        ReleaseLocation(&info);
    }
    m_writer->AsmBr(blockInfo->label);
    SetUnreachableState(true);
}

PolymorphicEmitInfo WasmBytecodeGenerator::EmitBrIf()
{
    uint32 depth = GetReader()->m_currentNode.br.depth;

    EmitInfo conditionInfo = PopEvalStack(WasmTypes::I32, _u("br_if condition must have i32 type"));
    ReleaseLocation(&conditionInfo);

    PolymorphicEmitInfo info;
    BlockInfo* blockInfo = GetBlockInfo(depth);

    if (blockInfo->HasYield())
    {
        info = PopStackPolymorphic(blockInfo);
        YieldToBlock(blockInfo, info);
        if (info.IsUnreachable())
        {
            Assert(IsUnreachable());
            Assert(info.Count() == blockInfo->yieldInfo.Count());
            // Use the block's yield type to continue type check
            for (uint32 i = 0; i < info.Count(); ++i)
            {
                info.SetInfo(EmitInfo(blockInfo->yieldInfo.GetInfo(i).type), i);
            }
        }
    }

    m_writer->AsmBrReg1(Js::OpCodeAsmJs::BrTrue_Int, blockInfo->label, conditionInfo.location);

    return info;
}

Js::OpCodeAsmJs WasmBytecodeGenerator::GetLoadOp(WasmTypes::WasmType wasmType)
{
    switch (wasmType)
    {
    case WasmTypes::F32:
        return Js::OpCodeAsmJs::Ld_Flt;
    case WasmTypes::F64:
        return Js::OpCodeAsmJs::Ld_Db;
    case WasmTypes::I32:
        return Js::OpCodeAsmJs::Ld_Int;
    case WasmTypes::I64:
        return Js::OpCodeAsmJs::Ld_Long;
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Simd::EnsureSimdIsEnabled();
        return Js::OpCodeAsmJs::Simd128_Ld_F4;
#endif
    case WasmTypes::Any:
        // In unreachable mode load the any type like an int since we won't actually emit the load
        Assert(IsUnreachable());
        if (IsUnreachable())
        {
            return Js::OpCodeAsmJs::Ld_Int;
        }
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown load operator %u"), wasmType);
    }
}

Js::OpCodeAsmJs WasmBytecodeGenerator::GetReturnOp(WasmTypes::WasmType type)
{
    Js::OpCodeAsmJs retOp = Js::OpCodeAsmJs::Nop;
    switch (type)
    {
    case WasmTypes::F32:
        retOp = Js::OpCodeAsmJs::Return_Flt;
        break;
    case WasmTypes::F64:
        retOp = Js::OpCodeAsmJs::Return_Db;
        break;
    case WasmTypes::I32:
        retOp = Js::OpCodeAsmJs::Return_Int;
        break;
    case WasmTypes::I64:
        retOp = Js::OpCodeAsmJs::Return_Long;
        break;
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Simd::EnsureSimdIsEnabled();
        retOp = Js::OpCodeAsmJs::Simd128_Return_F4;
        break;
#endif
    case WasmTypes::Any:
        // In unreachable mode load the any type like an int since we won't actually emit the load
        Assert(IsUnreachable());
        if (IsUnreachable())
        {
            return Js::OpCodeAsmJs::Return_Int;
        }
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown return type %u"), type);
    }
    return retOp;
}

void WasmBytecodeGenerator::ReleaseLocation(EmitInfo* info)
{
    if (WasmTypes::IsLocalType(info->type))
    {
        GetRegisterSpace(info->type)->ReleaseLocation(info);
    }
}

void WasmBytecodeGenerator::ReleaseLocation(PolymorphicEmitInfo* info)
{
    for (uint32 i = 0; i < info->Count(); ++i)
    {
        ReleaseLocation(&info->GetInfo(i));
    }
}

PolymorphicEmitInfo WasmBytecodeGenerator::EnsureYield(BlockInfo* blockInfo)
{
    PolymorphicEmitInfo yieldEmitInfo;
    if (blockInfo->HasYield())
    {
        yieldEmitInfo = blockInfo->yieldInfo;
        if (!blockInfo->DidYield())
        {
            // Emit a load to the yield location to make sure we have a dest there
            // Most likely we can't reach this code so the value doesn't matter
            blockInfo->didYield = true;
            for (uint32 i = 0; i < yieldEmitInfo.Count(); ++i)
            {
                EmitLoadConst(yieldEmitInfo.GetInfo(i), GetZeroCnst());
            }
        }
    }
    return yieldEmitInfo;
}

PolymorphicEmitInfo WasmBytecodeGenerator::PopLabel(Js::ByteCodeLabel labelValidation)
{
    Assert(m_blockInfos.Count() > 0);
    BlockInfo* blockInfo = m_blockInfos.Pop();
    UNREFERENCED_PARAMETER(labelValidation);
    Assert(blockInfo->label == labelValidation);
    return EnsureYield(blockInfo);
}

BlockInfo* WasmBytecodeGenerator::PushLabel(WasmBlock blockData, Js::ByteCodeLabel label, bool addBlockYieldInfo, bool checkInParams)
{
    BlockInfo* blockInfo = Anew(&m_alloc, BlockInfo);
    blockInfo->label = label;
    if (addBlockYieldInfo)
    {
        if (blockData.IsSingleResult())
        {
            if (blockData.GetSingleResult() != WasmTypes::Void)
            {
                blockInfo->yieldInfo.Init(EmitInfo(GetRegisterSpace(blockData.GetSingleResult())->AcquireTmpRegister(), blockData.GetSingleResult()));
            }
        }
        else
        {
            uint32 sigId = blockData.GetSignatureId();
            WasmSignature* signature = m_module->GetSignature(sigId);

            Js::ArgSlot paramCount = signature->GetParamCount();
            checkInParams = checkInParams && paramCount > 0;
            PolymorphicEmitInfo inParams;
            if (checkInParams)
            {
                inParams.Init(paramCount, &m_alloc);
                // Pop the params in reverse order
                for (int i = paramCount - 1; i >= 0; --i)
                {
                    Js::ArgSlot iArg = (Js::ArgSlot)i;
                    EmitInfo param = PopEvalStack(signature->GetParam(iArg));
                    ReleaseLocation(&param);
                    inParams.SetInfo(param, iArg);
                }
            }

            uint32 resultCount = signature->GetResultCount();
            if (resultCount > 0)
            {
                blockInfo->yieldInfo.Init(resultCount, &m_alloc);
                for (uint32 i = 0; i < resultCount; ++i)
                {
                    WasmTypes::WasmType type = signature->GetResult(i);
                    blockInfo->yieldInfo.SetInfo(EmitInfo(GetRegisterSpace(type)->AcquireTmpRegister(), type), i);
                }
            }

            if (checkInParams)
            {
                blockInfo->paramInfo.Init(paramCount, &m_alloc);
                // Acquire tmp registers in order
                for (uint32 i = 0; i < paramCount; ++i)
                {
                    EmitInfo info = inParams.GetInfo(i);
                    EmitInfo newInfo = info;
                    newInfo.location = GetRegisterSpace(info.type)->AcquireTmpRegister();
                    blockInfo->paramInfo.SetInfo(newInfo, i);
                }
                // Todo:: Instead of moving inparams to new location,
                // Treat inparams as local and bypass ReleaseLocation until we exit the scope

                // Move in params to new location in reverse order
                for (int i = paramCount - 1; i >= 0; --i)
                {
                    Js::ArgSlot iArg = (Js::ArgSlot)i;
                    EmitInfo info = inParams.GetInfo(iArg);
                    EmitInfo newInfo = blockInfo->paramInfo.GetInfo(iArg);
                    m_writer->AsmReg2(GetLoadOp(newInfo.type), newInfo.location, info.location);
                }
            }
        }
    }
    m_blockInfos.Push(blockInfo);
    return blockInfo;
}

void WasmBytecodeGenerator::YieldToBlock(BlockInfo* blockInfo, PolymorphicEmitInfo polyExpr)
{
    if (blockInfo->HasYield() && !polyExpr.IsUnreachable())
    {
        PolymorphicEmitInfo polyYieldInfo = blockInfo->yieldInfo;

        if (!polyYieldInfo.IsEquivalent(polyExpr))
        {
            throw WasmCompilationException(_u("Invalid yield type"));
        }

        if (!IsUnreachable())
        {
            blockInfo->didYield = true;
            for (uint32 i = 0; i < polyExpr.Count(); ++i)
            {
                EmitInfo expr = polyExpr.GetInfo(i);
                EmitInfo yieldInfo = polyYieldInfo.GetInfo(i);
                m_writer->AsmReg2(GetReturnOp(expr.type), yieldInfo.location, expr.location);
            }
        }
    }
}

BlockInfo* WasmBytecodeGenerator::GetBlockInfo(uint32 relativeDepth) const
{
    if (relativeDepth >= (uint32)m_blockInfos.Count())
    {
        throw WasmCompilationException(_u("Invalid branch target"));
    }
    return m_blockInfos.Peek(relativeDepth);
}

Js::ProfileId
WasmBytecodeGenerator::GetNextProfileId()
{
    Js::ProfileId nextProfileId = this->currentProfileId;
    UInt16Math::Inc(this->currentProfileId);
    return nextProfileId;
}

WasmRegisterSpace* WasmBytecodeGenerator::GetRegisterSpace(WasmTypes::WasmType type)
{
    switch (type)
    {
#if TARGET_32
    case WasmTypes::Ptr:
#endif
    case WasmTypes::I32: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT32);
#if TARGET_64
    case WasmTypes::Ptr:
#endif
    case WasmTypes::I64:    return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT64);
    case WasmTypes::F32:    return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT32);
    case WasmTypes::F64:    return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT64);
#ifdef ENABLE_WASM_SIMD
    case WasmTypes::M128:
        Simd::EnsureSimdIsEnabled();
        return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::SIMD);
#endif
    default:
        WasmTypes::CompileAssertCasesNoFailFast<WasmTypes::I32, WasmTypes::I64, WasmTypes::F32, WasmTypes::F64, WASM_M128_CHECK_TYPE>();
        throw WasmCompilationException(_u("Unknown type %u"), type);
    }
}

PolymorphicEmitInfo WasmBytecodeGenerator::PopStackPolymorphic(PolymorphicEmitInfo expectedTypes, const char16* mismatchMessage /*= nullptr*/)
{
    PolymorphicEmitInfo info;
    uint32 count = expectedTypes.Count();
    info.Init(count, &m_alloc);

    for (uint32 i = 0; i < count; ++i)
    {
        // Check the stack before popping, it is valid to yield nothing if we are Unreachable
        if (IsUnreachable() && m_evalStack.Peek().type == WasmTypes::Limit)
        {
            info.SetInfo(EmitInfo(WasmTypes::Any), i);
        }
        else
        {
            info.SetInfo(PopEvalStack(expectedTypes.GetInfo(i).type, mismatchMessage), i);
        }
    }
    return info;
}


PolymorphicEmitInfo WasmBytecodeGenerator::PopStackPolymorphic(const BlockInfo* blockInfo, const char16* mismatchMessage)
{
    return PopStackPolymorphic(blockInfo->yieldInfo, mismatchMessage);
}

EmitInfo WasmBytecodeGenerator::PopEvalStack(WasmTypes::WasmType expectedType, const char16* mismatchMessage)
{
    // The scope marker should at least be there
    Assert(!m_evalStack.Empty());
    EmitInfo info = m_evalStack.Pop();
    if (info.type == WasmTypes::Limit)
    {
        throw WasmCompilationException(_u("Reached end of stack"));
    }
    if (expectedType != WasmTypes::Any &&
        info.type != WasmTypes::Any &&
        info.type != expectedType)
    {
        if (!mismatchMessage)
        {
            throw WasmCompilationException(_u("Type mismatch. Expected %s, got %s"), GetTypeName(expectedType), GetTypeName(info.type));
        }
        throw WasmCompilationException(mismatchMessage);
    }
    Assert(info.type != WasmTypes::Any || IsUnreachable());
    return info;
}

void WasmBytecodeGenerator::PushEvalStack(PolymorphicEmitInfo info)
{
    Assert(!m_evalStack.Empty());
    for (uint32 i = 0; i < info.Count(); ++i)
    {
        m_evalStack.Push(info.GetInfo(i));
    }
}

void WasmBytecodeGenerator::EnterEvalStackScope(const BlockInfo* blockInfo)
{
    m_evalStack.Push(EmitInfo(WasmTypes::Limit));
    // Push the in-params of the block upon entering the scope
    for (uint32 i = 0; i < blockInfo->paramInfo.Count(); ++i)
    {
        m_evalStack.Push(blockInfo->paramInfo.GetInfo(i));
    }
}

void WasmBytecodeGenerator::ExitEvalStackScope(const BlockInfo* blockInfo)
{
    Assert(!m_evalStack.Empty());
    EmitInfo info = m_evalStack.Pop();
    // It is possible to have unconsumed Any type left on the stack, simply remove them
    while (info.type == WasmTypes::Any)
    {
        Assert(!m_evalStack.Empty());
        info = m_evalStack.Pop();
    }
    if (info.type != WasmTypes::Limit)
    {
        // Put info back on stack so we can write it to string
        m_evalStack.Push(info);
        char16 buf[512] = { 0 };
        WriteTypeStackToString(buf, 512);
        throw WasmCompilationException(_u("Expected stack to be empty, but has %s"), buf);
    }
}

void WasmBytecodeGenerator::SetUnreachableState(bool isUnreachable)
{
    m_writer = isUnreachable ? m_emptyWriter : m_originalWriter;
    if (isUnreachable)
    {
        // Replace the current stack with the any type
        Assert(!m_evalStack.Empty());
        uint32 popped = 0;
        while (m_evalStack.Top().type != WasmTypes::Limit)
        {
            EmitInfo info = m_evalStack.Pop();
            ReleaseLocation(&info);
            ++popped;
        }
        while (popped-- > 0)
        {
            m_evalStack.Push(EmitInfo(WasmTypes::Any));
        }
    }
    this->isUnreachable = isUnreachable;
}

void WasmBytecodeGenerator::SetUsesMemory(uint32 memoryIndex)
{
    // Only support one memory at this time
    Assert(memoryIndex == 0);
    if (!m_module->HasMemory() && !m_module->HasMemoryImport())
    {
        throw WasmCompilationException(_u("unknown memory"));
    }
    GetFunctionBody()->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);
}

Wasm::WasmReaderBase* WasmBytecodeGenerator::GetReader() const
{
    if (m_funcInfo->GetCustomReader())
    {
        return m_funcInfo->GetCustomReader();
    }
    return m_module->GetReader();
}

void WasmCompilationException::FormatError(const char16* _msg, va_list arglist)
{
    char16 buf[2048];

    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, _msg, arglist);
    errorMsg = SysAllocString(buf);
}

WasmCompilationException::WasmCompilationException(const char16* _msg, ...) : errorMsg(nullptr)
{
    va_list arglist;
    va_start(arglist, _msg);
    FormatError(_msg, arglist);
}

WasmCompilationException::WasmCompilationException(const char16* _msg, va_list arglist) : errorMsg(nullptr)
{
    FormatError(_msg, arglist);
}

void PolymorphicEmitInfo::Init(EmitInfo info)
{
    if (info.type != WasmTypes::Void)
    {
        count = 1;
        singleInfo = info;
    }
    else
    {
        Assert(info.location == Js::Constants::NoRegister);
        count = 0;
        infos = nullptr;
    }
}

void PolymorphicEmitInfo::Init(uint32 count, ArenaAllocator* alloc)
{
    infos = nullptr;
    if (count > 1)
    {
        infos = AnewArray(alloc, EmitInfo, count);
    }
    this->count = count;
}

void PolymorphicEmitInfo::SetInfo(EmitInfo info, uint32 index)
{
    AssertOrFailFast(index < count);
    if (count == 1)
    {
        singleInfo = info;
    }
    else
    {
        infos[index] = info;
    }
}

Wasm::EmitInfo PolymorphicEmitInfo::GetInfo(uint32 index) const
{
    AssertOrFailFast(index < count);
    if (count == 1)
    {
        return singleInfo;
    }
    return infos[index];
}

bool PolymorphicEmitInfo::IsUnreachable() const
{
    if (count == 0)
    {
        return false;
    }
    if (count == 1)
    {
        return singleInfo.type == WasmTypes::Any;
    }
    for (uint32 i = 0; i < count; ++i)
    {
        if (infos[i].type == WasmTypes::Any)
        {
            return true;
        }
    }
    return false;
}

bool PolymorphicEmitInfo::IsEquivalent(PolymorphicEmitInfo other) const
{
    if (Count() != other.Count())
    {
        return false;
    }
    for (uint32 i = 0; i < Count(); ++i)
    {
        if (GetInfo(i).type != other.GetInfo(i).type)
        {
            return false;
        }
    }
    return true;
}

} // namespace Wasm

#endif // ENABLE_WASM
