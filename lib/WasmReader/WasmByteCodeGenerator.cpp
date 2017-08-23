//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
#include "Language/WebAssemblySource.h"
#include "ByteCode/WasmByteCodeWriter.h"
#include "EmptyWasmByteCodeWriter.h"

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
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
case wb##opname: \
    Output::Print(_u(#opname)); \
    break;
#include "WasmBinaryOpCodes.h"
    }
    switch (op)
    {
    case wbIf:
    case wbLoop:
    case wbBlock: Output::Print(_u(" () -> %s"), GetTypeName(GetReader()->m_currentNode.block.sig)); break;
    case wbBr:
    case wbBrIf: Output::Print(_u(" depth: %u"), GetReader()->m_currentNode.br.depth); break;
    case wbBrTable: Output::Print(_u(" %u cases, default: %u"), GetReader()->m_currentNode.brTable.numTargets, GetReader()->m_currentNode.brTable.defaultTarget); break;
    case wbCall:
    case wbCallIndirect:
    {
        uint id = GetReader()->m_currentNode.call.num;
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
            Output::Print(_u(" invalid id"));
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
#define WASM_MEM_OPCODE(opname, opcode, sig, nyi) case wb##opname: // FallThrough
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
    default:
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
    default:
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
    m_module->GetReader()->InitializeReader();

    BVStatic<bSectLimit + 1> visitedSections;

    SectionCode nextExpectedSection = bSectCustom;
    while (true)
    {
        SectionHeader sectionHeader = GetReader()->ReadNextSection();
        SectionCode sectionCode = sectionHeader.code;
        if (sectionCode == bSectLimit)
        {
            TRACE_WASM_SECTION(_u("Done reading module's sections"));
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

#if DBG_DUMP
    if (PHASE_TRACE1(Js::WasmReaderPhase))
    {
        GetReader()->PrintOps();
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
        nullptr,
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
    info->SetReturnType(WasmToAsmJs::GetAsmJsReturnType(wasmInfo->GetResultType()));
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
    mTypedRegisterAllocator(&m_alloc, AllocateRegisterSpace, 1 << WAsmJs::SIMD),
    m_blockInfos(&m_alloc),
    isUnreachable(false)
{
    m_emptyWriter = Anew(&m_alloc, Js::EmptyWasmByteCodeWriter);
    m_writer = m_originalWriter = validateOnly ? m_emptyWriter : Anew(&m_alloc, Js::WasmByteCodeWriter);
    m_writer->Create();
    m_funcInfo = readerInfo->m_funcInfo;
    m_module = readerInfo->m_module;
    // Init reader to current func offset
    GetReader()->SeekToFunctionBody(m_funcInfo);

    // Use binary size to estimate bytecode size
    const uint32 astSize = readerInfo->m_funcInfo->m_readerInfo.size;
    m_writer->InitData(&m_alloc, astSize);
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
    try
    {
        Js::ByteCodeLabel exitLabel = m_writer->DefineLabel();
        m_funcInfo->SetExitLabel(exitLabel);
        EnregisterLocals();

        EnterEvalStackScope();
        // The function's yield type is the return type
        GetReader()->m_currentNode.block.sig = m_funcInfo->GetResultType();
        EmitInfo lastInfo = EmitBlock();
        if (lastInfo.type != WasmTypes::Void || m_funcInfo->GetResultType() == WasmTypes::Void)
        {
            EmitReturnExpr(&lastInfo);
        }
        DebugPrintOpEnd();
        ExitEvalStackScope();
        SetUnreachableState(false);
        m_writer->MarkAsmJsLabel(exitLabel);
        m_writer->EmptyAsm(Js::OpCodeAsmJs::Ret);
        m_writer->End();
        GetReader()->FunctionEnd();
    }
    catch (...)
    {
        TRACE_WASM_BYTECODE(_u("\nHad Compilation error!"));
        GetReader()->FunctionEnd();
        m_originalWriter->Reset();
        throw;
    }
    // Make sure we don't have any unforeseen exceptions as we finalize the body
    AutoDisableInterrupt autoDisableInterrupt(m_scriptContext->GetThreadContext(), true);

#if DBG_DUMP
    if (PHASE_DUMP(Js::ByteCodePhase, GetFunctionBody()) && !IsValidating())
    {
        Js::AsmJsByteCodeDumper::Dump(GetFunctionBody(), &mTypedRegisterAllocator, nullptr);
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
        if (regSpace == nullptr)
        {
            throw WasmCompilationException(_u("Unable to find local register space"));
        }
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
            default:
                Assume(UNREACHED);
            }
        }
    }
}

void WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    DebugPrintOp(op);
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
    case opcode: \
        if (nyi) throw WasmCompilationException(_u("Operator %s NYI"), _u(#opname)); break;
#include "WasmBinaryOpCodes.h"
    default:
        break;
    }

    EmitInfo info;
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
        info.type = WasmTypes::Any;
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
        info.type = WasmTypes::Any;
        break;
    case wbBrIf:
        info = EmitBrIf();
        break;
    case wbSelect:
        info = EmitSelect();
        break;
    case wbBrTable:
        EmitBrTable();
        info.type = WasmTypes::Any;
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
        info.type = WasmTypes::Any;
        break;
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, nyi, viewtype) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess(wb##opname, WasmOpCodeSignatures::sig, viewtype, false); \
        break;
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, nyi, viewtype) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess(wb##opname, WasmOpCodeSignatures::sig, viewtype, true); \
        break;
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjsop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 3);\
        info = EmitBinExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjsop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 2);\
        info = EmitUnaryExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_EMPTY__OPCODE(opname, opcode, asmjsop, nyi) \
    case wb##opname: \
        m_writer->EmptyAsm(Js::OpCodeAsmJs::##asmjsop);\
        break;
#include "WasmBinaryOpCodes.h"
    default:
        throw WasmCompilationException(_u("Unknown expression's op 0x%X"), op);
    }

    if (info.type != WasmTypes::Void)
    {
        PushEvalStack(info);
    }
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
    default:
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
    EnterEvalStackScope();
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
    if (blockInfo && blockInfo->HasYield())
    {
        EmitInfo info = PopEvalStack();
        YieldToBlock(*blockInfo, info);
        ReleaseLocation(&info);
    }
    ExitEvalStackScope();
    if (canResetUnreachable)
    {
        SetUnreachableState(false);
    }
}

EmitInfo WasmBytecodeGenerator::EmitBlock()
{
    Js::ByteCodeLabel blockLabel = m_writer->DefineLabel();

    BlockInfo blockInfo = PushLabel(blockLabel);
    EmitBlockCommon(&blockInfo);
    m_writer->MarkAsmJsLabel(blockLabel);
    EmitInfo yieldInfo = PopLabel(blockLabel);

    // block yields last value
    return yieldInfo;
}

EmitInfo WasmBytecodeGenerator::EmitLoop()
{
    Js::ByteCodeLabel loopTailLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel loopHeadLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel loopLandingPadLabel = m_writer->DefineLabel();

    uint32 loopId = m_writer->EnterLoop(loopHeadLabel);

    // Internally we create a block for loop to exit, but semantically, they don't exist so pop it
    BlockInfo implicitBlockInfo = PushLabel(loopTailLabel);
    m_blockInfos.Pop();

    // We don't want nested block to jump directly to the loop header
    // instead, jump to the landing pad and let it jump back to the loop header
    PushLabel(loopLandingPadLabel, false);
    EmitBlockCommon(&implicitBlockInfo);
    PopLabel(loopLandingPadLabel);

    // By default we don't loop, jump over the landing pad
    m_writer->AsmBr(loopTailLabel);
    m_writer->MarkAsmJsLabel(loopLandingPadLabel);
    m_writer->AsmBr(loopHeadLabel);

    // Put the implicit block back on the stack and yield the last expression to it
    m_blockInfos.Push(implicitBlockInfo);
    m_writer->MarkAsmJsLabel(loopTailLabel);
    // Pop the implicit block to resolve the yield correctly
    EmitInfo loopInfo = PopLabel(loopTailLabel);
    m_writer->ExitLoop(loopId);

    return loopInfo;
}

template<WasmOp wasmOp>
EmitInfo WasmBytecodeGenerator::EmitCall()
{
    uint32 funcNum = Js::Constants::UninitializedValue;
    uint32 signatureId = Js::Constants::UninitializedValue;
    WasmSignature* calleeSignature = nullptr;
    EmitInfo indirectIndexInfo;
    const bool isImportCall = GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Import;
    Assert(isImportCall || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Function || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::ImportThunk);
    switch (wasmOp)
    {
    case wbCall:
        funcNum = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetWasmFunctionInfo(funcNum)->GetSignature();
        break;
    case wbCallIndirect:
        indirectIndexInfo = PopEvalStack(WasmTypes::I32, _u("Indirect call index must be int type"));
        signatureId = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetSignature(signatureId);
        ReleaseLocation(&indirectIndexInfo);
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
        ReleaseLocation(&info);
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

    AdeleteArray(&m_alloc, nArgs, argsList);

    // emit call
    switch (wasmOp)
    {
    case wbCall:
    {
        uint32 offset = isImportCall ? m_module->GetImportFuncOffset() : m_module->GetFuncOffset();
        uint32 index = UInt32Math::Add(offset, funcNum);
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, index);
        break;
    }
    case wbCallIndirect:
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdSlotArr, 0, 1, m_module->GetTableEnvironmentOffset());
        m_writer->AsmSlot(Js::OpCodeAsmJs::LdArr_WasmFunc, 0, 0, indirectIndexInfo.location);
        m_writer->AsmReg1IntConst1(Js::OpCodeAsmJs::CheckSignature, 0, calleeSignature->GetSignatureId());
        break;
    default:
        Assume(UNREACHED);
    }

    // calculate number of RegSlots(Js::Var) the call consumes
    Js::ArgSlot args;
    Js::OpCodeAsmJs callOp = Js::OpCodeAsmJs::Nop;
    if (isImportCall)
    {
        args = calleeSignature->GetParamCount();
        ArgSlotMath::Inc(args, argOverflow);
        callOp = Js::OpCodeAsmJs::Call;
    }
    else
    {
        Assert(Math::IsAligned<Js::ArgSlot>(argSize, sizeof(Js::Var)));
        args = argSize / sizeof(Js::Var);
        callOp = Js::OpCodeAsmJs::I_Call;
    }

    m_writer->AsmCall(callOp, 0, 0, args, WasmToAsmJs::GetAsmJsReturnType(calleeSignature->GetResultType()));

    // emit result coercion
    EmitInfo retInfo;
    retInfo.type = calleeSignature->GetResultType();
    if (retInfo.type != WasmTypes::Void)
    {
        Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
        retInfo.location = GetRegisterSpace(retInfo.type)->AcquireTmpRegister();
        switch (retInfo.type)
        {
        case WasmTypes::F32:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::Ld_Flt;
            break;
        case WasmTypes::F64:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTD : Js::OpCodeAsmJs::Ld_Db;
            break;
        case WasmTypes::I32:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::Ld_Int;
            break;
        case WasmTypes::I64:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTL : Js::OpCodeAsmJs::Ld_Long;
            break;
        default:
            throw WasmCompilationException(_u("Unknown call return type %u"), retInfo.type);
        }
        m_writer->AsmReg2(convertOp, retInfo.location, 0);
    }

    // track stack requirements for out params

    // + 1 for return address
    uint32 maxDepthForLevel = args + 1;
    if (maxDepthForLevel > m_maxArgOutDepth)
    {
        m_maxArgOutDepth = maxDepthForLevel;
    }

    return retInfo;
}

EmitInfo WasmBytecodeGenerator::EmitIfElseExpr()
{
    Js::ByteCodeLabel falseLabel = m_writer->DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer->DefineLabel();

    EmitInfo checkExpr = PopEvalStack(WasmTypes::I32, _u("If expression must have type i32"));
    ReleaseLocation(&checkExpr);

    m_writer->AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    BlockInfo blockInfo = PushLabel(endLabel);
    bool endOnElse = false;
    EmitBlockCommon(&blockInfo, &endOnElse);
    EnsureYield(blockInfo);

    m_writer->AsmBr(endLabel);
    m_writer->MarkAsmJsLabel(falseLabel);

    EmitInfo retInfo;
    EmitInfo falseExpr;
    if (endOnElse)
    {
        if (blockInfo.HasYield())
        {
            // Indicate that we need this block to yield a value
            blockInfo.yieldInfo->didYield = false;
        }
        EmitBlockCommon(&blockInfo);
        EnsureYield(blockInfo);
    }
    else if (blockInfo.HasYield())
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
    EmitInfo scrutineeInfo = PopEvalStack(WasmTypes::I32, _u("br_table expression must be of type i32"));

    m_writer->AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeInfo.location, scrutineeInfo.location);
    EmitInfo yieldValue;
    BlockInfo defaultBlockInfo = GetBlockInfo(defaultEntry);
    if (defaultBlockInfo.HasYield())
    {
        // If the scrutinee is any then check the stack before popping
        if (scrutineeInfo.type == WasmTypes::Any && m_evalStack.Peek().type == WasmTypes::Limit)
        {
            yieldValue = scrutineeInfo;
        }
        else
        {
            yieldValue = PopEvalStack();
        }
    }

    // Compile cases
    for (uint32 i = 0; i < numTargets; i++)
    {
        uint32 target = targetTable[i];
        BlockInfo blockInfo = GetBlockInfo(target);
        if (!defaultBlockInfo.IsEquivalent(blockInfo))
        {
            WasmTypes::WasmType defaultType = defaultBlockInfo.yieldInfo ? defaultBlockInfo.yieldInfo->info.type : WasmTypes::Void;
            WasmTypes::WasmType type = blockInfo.yieldInfo ? blockInfo.yieldInfo->info.type : WasmTypes::Void;
            throw WasmCompilationException(_u("br_table target %u signature mismatch. Expected ()->%s, got ()->%s"), target, GetTypeName(defaultType), GetTypeName(type));
        }
        YieldToBlock(blockInfo, yieldValue);
        m_writer->AsmBrReg1Const1(Js::OpCodeAsmJs::Case_IntConst, blockInfo.label, scrutineeInfo.location, i);
    }

    YieldToBlock(defaultBlockInfo, yieldValue);
    m_writer->AsmBr(defaultBlockInfo.label, Js::OpCodeAsmJs::EndSwitch_Int);
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
    EmitInfo info = PopEvalStack();
    ReleaseLocation(&info);
    return EmitInfo();
}

EmitInfo WasmBytecodeGenerator::EmitBinExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType lhsType = signature[1];
    WasmTypes::WasmType rhsType = signature[2];

    EmitInfo rhs = PopEvalStack(lhsType);
    EmitInfo lhs = PopEvalStack(rhsType);

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

EmitInfo WasmBytecodeGenerator::EmitMemAccess(WasmOp wasmOp, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType, bool isStore)
{
    WasmTypes::WasmType type = signature[0];
    SetUsesMemory(0);

    const uint32 mask = Js::ArrayBufferView::ViewMask[viewType];
    const uint32 alignment = GetReader()->m_currentNode.mem.alignment;
    const uint32 offset = GetReader()->m_currentNode.mem.offset;

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

    if (isStore) // Stores
    {
        m_writer->WasmMemAccess(Js::OpCodeAsmJs::StArrWasm, rhsInfo.location, exprInfo.location, offset, viewType);
        ReleaseLocation(&rhsInfo);
        ReleaseLocation(&exprInfo);

        return EmitInfo();
    }

    ReleaseLocation(&exprInfo);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();   
    m_writer->WasmMemAccess(Js::OpCodeAsmJs::LdArrWasm, resultReg, exprInfo.location, offset, viewType);

    EmitInfo yieldInfo;
    if (!isStore)
    {
        // Yield only on load
        yieldInfo = EmitInfo(resultReg, type);
    }
    return yieldInfo;
}

void WasmBytecodeGenerator::EmitReturnExpr(EmitInfo* explicitRetInfo)
{
    if (m_funcInfo->GetResultType() == WasmTypes::Void)
    {
        // TODO (michhol): consider moving off explicit 0 for return reg
        m_writer->AsmReg1(Js::OpCodeAsmJs::LdUndef, 0);
    }
    else
    {
        EmitInfo retExprInfo = explicitRetInfo ? *explicitRetInfo : PopEvalStack();
        if (retExprInfo.type != WasmTypes::Any && m_funcInfo->GetResultType() != retExprInfo.type)
        {
            throw WasmCompilationException(_u("Result type must match return type"));
        }

        Js::OpCodeAsmJs retOp = GetReturnOp(retExprInfo.type);
        m_writer->Conv(retOp, 0, retExprInfo.location);
        ReleaseLocation(&retExprInfo);
    }
    m_writer->AsmBr(m_funcInfo->GetExitLabel());

    SetUnreachableState(true);
}

EmitInfo WasmBytecodeGenerator::EmitSelect()
{
    EmitInfo conditionInfo = PopEvalStack(WasmTypes::I32, _u("select condition must have i32 type"));
    EmitInfo falseInfo = PopEvalStack();
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

    BlockInfo blockInfo = GetBlockInfo(depth);
    if (blockInfo.HasYield())
    {
        EmitInfo info = PopEvalStack();
        YieldToBlock(blockInfo, info);
        ReleaseLocation(&info);
    }
    m_writer->AsmBr(blockInfo.label);
    SetUnreachableState(true);
}

EmitInfo WasmBytecodeGenerator::EmitBrIf()
{
    uint32 depth = GetReader()->m_currentNode.br.depth;

    EmitInfo conditionInfo = PopEvalStack(WasmTypes::I32, _u("br_if condition must have i32 type"));
    ReleaseLocation(&conditionInfo);

    EmitInfo info;
    BlockInfo blockInfo = GetBlockInfo(depth);
    if (blockInfo.HasYield())
    {
        info = PopEvalStack();
        YieldToBlock(blockInfo, info);
        if (info.type == WasmTypes::Any)
        {
            Assert(IsUnreachable());
            // Use the block's yield type to continue type check
            info = EmitInfo(blockInfo.yieldInfo->info.type);
        }
    }

    m_writer->AsmBrReg1(Js::OpCodeAsmJs::BrTrue_Int, blockInfo.label, conditionInfo.location);
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
    case WasmTypes::Any:
        // In unreachable mode load the any type like an int since we won't actually emit the load
        Assert(IsUnreachable());
        if (IsUnreachable())
        {
            return Js::OpCodeAsmJs::Ld_Int;
        }
    default:
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
    case WasmTypes::Any:
        // In unreachable mode load the any type like an int since we won't actually emit the load
        Assert(IsUnreachable());
        if (IsUnreachable())
        {
            return Js::OpCodeAsmJs::Return_Int;
        }
    default:
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

EmitInfo WasmBytecodeGenerator::EnsureYield(BlockInfo info)
{
    EmitInfo yieldEmitInfo;
    if (info.HasYield())
    {
        yieldEmitInfo = info.yieldInfo->info;
        if (!info.DidYield())
        {
            // Emit a load to the yield location to make sure we have a dest there
            // Most likely we can't reach this code so the value doesn't matter
            info.yieldInfo->didYield = true;
            EmitLoadConst(yieldEmitInfo, GetZeroCnst());
        }
    }
    return yieldEmitInfo;
}

EmitInfo WasmBytecodeGenerator::PopLabel(Js::ByteCodeLabel labelValidation)
{
    Assert(m_blockInfos.Count() > 0);
    BlockInfo info = m_blockInfos.Pop();
    UNREFERENCED_PARAMETER(labelValidation);
    Assert(info.label == labelValidation);
    return EnsureYield(info);
}

BlockInfo WasmBytecodeGenerator::PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo /*= true*/)
{
    BlockInfo info;
    info.label = label;
    if (addBlockYieldInfo)
    {
        WasmTypes::WasmType type = GetReader()->m_currentNode.block.sig;
        if (type != WasmTypes::Void)
        {
            info.yieldInfo = Anew(&m_alloc, BlockInfo::YieldInfo);
            info.yieldInfo->info = EmitInfo(GetRegisterSpace(type)->AcquireTmpRegister(), type);
            info.yieldInfo->didYield = false;
        }
    }
    m_blockInfos.Push(info);
    return info;
}

void WasmBytecodeGenerator::YieldToBlock(BlockInfo blockInfo, EmitInfo expr)
{
    if (blockInfo.HasYield() && expr.type != WasmTypes::Any)
    {
        EmitInfo yieldInfo = blockInfo.yieldInfo->info;

        if (yieldInfo.type != expr.type)
        {
            throw WasmCompilationException(_u("Invalid yield type"));
        }

        if (!IsUnreachable())
        {
            blockInfo.yieldInfo->didYield = true;
            m_writer->AsmReg2(GetLoadOp(expr.type), yieldInfo.location, expr.location);
        }
    }
}

Wasm::BlockInfo WasmBytecodeGenerator::GetBlockInfo(uint32 relativeDepth) const
{
    if (relativeDepth >= (uint32)m_blockInfos.Count())
    {
        throw WasmCompilationException(_u("Invalid branch target"));
    }
    return m_blockInfos.Peek(relativeDepth);
}

WasmRegisterSpace* WasmBytecodeGenerator::GetRegisterSpace(WasmTypes::WasmType type)
{
    switch (type)
    {
    case WasmTypes::I32: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT32);
    case WasmTypes::I64: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT64);
    case WasmTypes::F32: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT32);
    case WasmTypes::F64: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT64);
    default:
        return nullptr;
    }
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

void WasmBytecodeGenerator::PushEvalStack(EmitInfo info)
{
    Assert(!m_evalStack.Empty());
    m_evalStack.Push(info);
}

void WasmBytecodeGenerator::EnterEvalStackScope()
{
    m_evalStack.Push(EmitInfo(WasmTypes::Limit));
}

void WasmBytecodeGenerator::ExitEvalStackScope()
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

} // namespace Wasm

#endif // ENABLE_WASM
