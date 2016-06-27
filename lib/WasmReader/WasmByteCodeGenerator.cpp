//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

#if DBG_DUMP
#define DebugPrintOp(op) if (PHASE_TRACE(Js::WasmReaderPhase, m_currentFunc->body)) { PrintOpName(op); }
#else
#define DebugPrintOp(op)
#endif

namespace Wasm
{
WasmBytecodeGenerator::WasmBytecodeGenerator(Js::ScriptContext * scriptContext, Js::Utf8SourceInfo * sourceInfo, byte* binaryBuffer, uint binaryBufferLength) :
    m_scriptContext(scriptContext),
    m_sourceInfo(sourceInfo),
    m_alloc(_u("WasmBytecodeGen"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_reader(scriptContext->GetThreadContext()->GetPageAllocator(), binaryBuffer, binaryBufferLength),
    m_f32RegSlots(nullptr),
    m_f64RegSlots(nullptr),
    m_i32RegSlots(nullptr),
    m_currentFunc(nullptr),
    m_evalStack(&m_alloc),
    m_blockInfos(&m_alloc)
{
    m_writer.Create();

    // TODO (michhol): try to make this more accurate?
    const long astSize = 0;
    m_writer.InitData(&m_alloc, astSize);

    // Initialize maps needed by binary reader
    Binary::WasmBinaryReader::Init(scriptContext);
}
typedef bool(*SectionProcessFunc)(WasmBytecodeGenerator*);
typedef void(*AfterSectionCallback)(WasmBytecodeGenerator*);

WasmModule *
WasmBytecodeGenerator::GenerateModule()
{
    // TODO: can this be in a better place?
    m_sourceInfo->EnsureInitialized(0);
    m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();

    m_module = Anew(&m_alloc, WasmModule);
    m_module->info = m_reader.m_moduleInfo;
    m_module->heapOffset = 0;
    m_module->importFuncOffset = m_module->heapOffset + 1;
    m_module->funcOffset = m_module->heapOffset + 1;

    m_reader.InitializeReader();
    m_reader.m_module = m_module;

    BVFixed* visitedSections = BVFixed::New(bSectLimit + 1, &m_alloc);

    const auto readerProcess = [](WasmBytecodeGenerator* gen) { return gen->m_reader.ProcessCurrentSection(); };
    // By default lest the reader process the section
#define WASM_SECTION(name, id, flag, precedent) readerProcess,
    SectionProcessFunc sectionProcess[bSectLimit + 1] = {
#include "WasmSections.h"
        nullptr
    };

    // Will callback regardless if the section is present or not
    AfterSectionCallback afterSectionCallback[bSectLimit + 1] = {};

    afterSectionCallback[bSectFunctionSignatures] = [](WasmBytecodeGenerator* gen) {
        gen->m_module->funcOffset = gen->m_module->importFuncOffset + gen->m_module->info->GetImportCount();
    };
    afterSectionCallback[bSectIndirectFunctionTable] = [](WasmBytecodeGenerator* gen) {
        gen->m_module->indirFuncTableOffset = gen->m_module->funcOffset + gen->m_module->info->GetFunctionCount();
    };

    sectionProcess[bSectFunctionBodies] = [](WasmBytecodeGenerator* gen) {
        gen->m_module->funcCount = gen->m_module->info->GetFunctionCount();
        gen->m_module->functions = AnewArrayZ(&gen->m_alloc, WasmFunction*, gen->m_module->funcCount);
        if (PHASE_ON1(Js::WasmLazyTrapPhase))
        {
            gen->m_module->lazyTraps = AnewArrayZ(&gen->m_alloc, WasmCompilationException*, gen->m_module->funcCount);
        }
        return gen->m_reader.ReadFunctionBodies([](uint32 index, void* g) {
            WasmBytecodeGenerator* gen = (WasmBytecodeGenerator*)g;
            if (index >= gen->m_module->funcCount) {
                return false;
            }
            WasmFunction* fn = gen->GenerateFunction();
            gen->m_module->functions[index] = fn;
            return true;
        }, gen);
    };

    for (SectionCode sectionCode = (SectionCode)(bSectInvalid + 1); sectionCode < bSectLimit ; sectionCode = (SectionCode)(sectionCode + 1))
    {
        SectionCode precedent = SectionInfo::All[sectionCode].precedent;
        if (m_reader.ReadNextSection((SectionCode)sectionCode))
        {
            if (precedent != bSectInvalid && !visitedSections->Test(precedent))
            {
                throw WasmCompilationException(_u("%s section missing before %s"),
                                               SectionInfo::All[precedent].name,
                                               SectionInfo::All[sectionCode].name);
            }
            visitedSections->Set(sectionCode);

            if (!sectionProcess[sectionCode](this))
            {
                throw WasmCompilationException(_u("Error while reading section %s"), SectionInfo::All[sectionCode].name);
            }
        }

        if (afterSectionCallback[sectionCode])
        {
            afterSectionCallback[sectionCode](this);
        }
    }

#if DBG_DUMP
    if (PHASE_TRACE1(Js::WasmReaderPhase))
    {
        m_reader.PrintOps();
    }
#endif
    // If we see a FunctionSignatures section we need to see a FunctionBodies section
    if (visitedSections->Test(bSectFunctionSignatures) && !visitedSections->Test(bSectFunctionBodies))
    {
        throw WasmCompilationException(_u("Missing required section: %s"), SectionInfo::All[bSectFunctionBodies].name);
    }
    // reserve space for as many function tables as there are signatures, though we won't fill them all
    m_module->memSize = m_module->funcOffset + m_module->info->GetFunctionCount() + m_module->info->GetSignatureCount() + m_module->info->GetImportCount();

    return m_module;
}

WasmFunction *
WasmBytecodeGenerator::GenerateFunction()
{
    WasmFunctionInfo* wasmInfo = m_reader.m_currentNode.func.info;
    TRACE_WASM_DECODER(_u("GenerateFunction %u \n"), wasmInfo->GetNumber());

    WasmRegisterSpace f32Space(ReservedRegisterCount);
    WasmRegisterSpace f64Space(ReservedRegisterCount);
    WasmRegisterSpace i32Space(ReservedRegisterCount);

    m_f32RegSlots = &f32Space;
    m_f64RegSlots = &f64Space;
    m_i32RegSlots = &i32Space;

    m_currentFunc = Anew(&m_alloc, WasmFunction);

    char16* functionName = nullptr;
    int nameLength = 0;
    for (uint32 iExport = 0; iExport < m_module->info->GetExportCount(); ++iExport)
    {
        Wasm::WasmExport* funcExport = m_module->info->GetFunctionExport(iExport);
        if (funcExport && funcExport->nameLength > 0)
        {
            const uint32 funcIndex = funcExport->funcIndex;
            if (funcIndex == wasmInfo->GetNumber())
            {
                nameLength = funcExport->nameLength + 16;
                functionName = RecyclerNewArrayZ(m_scriptContext->GetRecycler(), char16, nameLength);
                nameLength = swprintf_s(functionName, nameLength, _u("%s[%u]"), funcExport->name, wasmInfo->GetNumber());
                break;
            }
        }
    }

    if (!functionName)
    {
        functionName = RecyclerNewArrayZ(m_scriptContext->GetRecycler(), char16, 32);
        nameLength = swprintf_s(functionName, 32, _u("wasm-function[%u]"), wasmInfo->GetNumber());
    }

    m_currentFunc->body = Js::FunctionBody::NewFromRecycler(
        m_scriptContext,
        functionName,
        nameLength,
        0,
        0,
        m_sourceInfo,
        m_sourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
        wasmInfo->GetNumber(),
        nullptr,
        Js::FunctionInfo::Attributes::None
#ifdef PERF_COUNTERS
        , false /* is function from deferred deserialized proxy */
#endif
        );
    // TODO (michhol): numbering
    m_currentFunc->body->SetSourceInfo(0);
    m_currentFunc->body->AllocateAsmJsFunctionInfo();
    m_currentFunc->body->SetIsAsmJsFunction(true);
    m_currentFunc->body->SetIsAsmjsMode(true);
    m_currentFunc->body->SetIsWasmFunction(true);
    m_currentFunc->body->GetAsmJsFunctionInfo()->SetIsHeapBufferConst(true);
    m_funcInfo = wasmInfo;
    m_currentFunc->wasmInfo = m_funcInfo;
    m_nestedIfLevel = 0;
    m_maxArgOutDepth = 0;

    Assert(m_evalStack.Empty());
    // The stack should always be empty when starting a new function, make sure by clearing it in case we missed something
    m_evalStack.Clear();

    // TODO: fix these bools
    m_writer.Begin(m_currentFunc->body, &m_alloc, true, true, false);
    try
    {
        try
        {
            if (PHASE_OFF(Js::WasmBytecodePhase, m_currentFunc->body) && PHASE_ON1(Js::WasmLazyTrapPhase)) 
            {
                throw WasmCompilationException(_u("Compilation skipped"));
            }
            m_funcInfo->SetExitLabel(m_writer.DefineLabel());
            EnregisterLocals();

            WasmOp op = wnLIMIT;
            EmitInfo exprInfo;
            EnterEvalStackScope();
            while ((op = m_reader.ReadExpr()) != wnFUNC_END)
            {
                exprInfo = EmitExpr(op);
            }
            DebugPrintOp(op);
            // Functions are like blocks. Emit implicit return of last stmt/expr, unless it is a return or end of file (sexpr).
            Wasm::WasmTypes::WasmType returnType = m_funcInfo->GetSignature()->GetResultType();

            // If the last expression yielded a value, return it
            if (exprInfo.type != WasmTypes::Unreachable)
            {
                if (exprInfo.type != returnType && returnType != Wasm::WasmTypes::Void)
                {
                    throw WasmCompilationException(_u("Last expression return type mismatch return type"));
                }
                uint32 arity = 0;
                if (returnType != Wasm::WasmTypes::Void)
                {
                    arity = 1;
                }
                m_reader.m_currentNode.ret.arity = arity;
                EmitReturnExpr();
            }
            ExitEvalStackScope();
        }
        catch (...)
        {
            m_writer.Reset();
            throw;
        }
        m_writer.MarkAsmJsLabel(m_funcInfo->GetExitLabel());
        m_writer.EmptyAsm(Js::OpCodeAsmJs::Ret);

        m_writer.End();

#if DBG_DUMP
        if (PHASE_DUMP(Js::ByteCodePhase, m_currentFunc->body))
        {
            Js::AsmJsByteCodeDumper::DumpBasic(m_currentFunc->body);
        }
#endif

        // TODO: refactor out to separate procedure
        Js::AsmJsFunctionInfo * info = m_currentFunc->body->GetAsmJsFunctionInfo();
        if (m_funcInfo->GetParamCount() >= Js::Constants::InvalidArgSlot)
        {
            Js::Throw::OutOfMemory();
        }
        Js::ArgSlot paramCount = (Js::ArgSlot)m_funcInfo->GetParamCount();
        info->SetArgCount(paramCount);

        Js::ArgSlot argSizeLength = max(paramCount, 3ui16);
        info->SetArgSizeArrayLength(argSizeLength);
        uint* argSizeArray = RecyclerNewArrayLeafZ(m_scriptContext->GetRecycler(), uint, argSizeLength);
        info->SetArgsSizesArray(argSizeArray);

        if (m_module->memSize > 0)
        {
            info->SetUsesHeapBuffer(true);
        }
        if (paramCount > 0)
        {
            m_currentFunc->body->SetHasImplicitArgIns(true);
            m_currentFunc->body->SetInParamsCount(paramCount + 1);
            m_currentFunc->body->SetReportedInParamsCount(paramCount + 1);
            info->SetArgTypeArray(RecyclerNewArrayLeaf(m_scriptContext->GetRecycler(), Js::AsmJsVarType::Which, paramCount));
        }
        Js::ArgSlot paramSize = 0;
        for (Js::ArgSlot i = 0; i < paramCount; ++i)
        {
            WasmTypes::WasmType type = m_funcInfo->GetParam(i);
            info->SetArgType(GetAsmJsVarType(type), i);
            uint16 size = 0;
            switch (type)
            {
            case WasmTypes::F32:
            case WasmTypes::I32:
                CompileAssert(sizeof(float) == sizeof(int32));
#ifdef _M_X64
                // on x64, we always alloc (at least) 8 bytes per arguments
                size = sizeof(void*);
#elif _M_IX86
                size = sizeof(int32);
#else
                Assert(UNREACHED);
#endif
                break;
            case WasmTypes::F64:
            case WasmTypes::I64:
                CompileAssert(sizeof(double) == sizeof(int64));
                size = sizeof(int64);
                break;
            default:
                Assume(UNREACHED);
            }
            argSizeArray[i] = size;
            // REVIEW: reduce number of checked adds
            paramSize = UInt16Math::Add(paramSize, size);
        }
        info->SetArgByteSize(paramSize);

        info->SetIntVarCount(m_i32RegSlots->GetVarCount());
        info->SetFloatVarCount(m_f32RegSlots->GetVarCount());
        info->SetDoubleVarCount(m_f64RegSlots->GetVarCount());

        info->SetIntTmpCount(m_i32RegSlots->GetTmpCount());
        info->SetFloatTmpCount(m_f32RegSlots->GetTmpCount());
        info->SetDoubleTmpCount(m_f64RegSlots->GetTmpCount());

        info->SetIntConstCount(ReservedRegisterCount);
        info->SetFloatConstCount(ReservedRegisterCount);
        info->SetDoubleConstCount(ReservedRegisterCount);

        const uint32 nbConst = 
            ((info->GetDoubleConstCount() + 1) * WAsmJs::DOUBLE_SLOTS_SPACE)
            + (uint32)((info->GetFloatConstCount() + 1) * WAsmJs::FLOAT_SLOTS_SPACE + 0.5 /*ceil*/)
            + (uint32)((info->GetIntConstCount() + 1) * WAsmJs::INT_SLOTS_SPACE + 0.5/*ceil*/)
            + Js::AsmJsFunctionMemory::RequiredVarConstants;

        // This is constant, 29 for 32bits and 17 for 64bits systems
        Assert(nbConst == 29 || nbConst == 17);
        m_currentFunc->body->CheckAndSetConstantCount(nbConst);

        info->SetReturnType(GetAsmJsReturnType(m_funcInfo->GetResultType()));

        const int32 intByteOffset = ReservedRegisterCount * sizeof(Js::Var);
        CompileAssert(intByteOffset >= 0);
        info->SetIntByteOffset(intByteOffset);
        info->SetFloatByteOffset(UInt32Math::Add(info->GetIntByteOffset(), UInt32Math::Mul(m_i32RegSlots->GetRegisterCount(), sizeof(int32))));
        info->SetDoubleByteOffset(Math::AlignOverflowCheck<int>(UInt32Math::Add(info->GetFloatByteOffset(), UInt32Math::Mul(m_f32RegSlots->GetRegisterCount(), sizeof(float))), sizeof(double)));

        m_currentFunc->body->SetOutParamMaxDepth(m_maxArgOutDepth);
        m_currentFunc->body->SetVarCount(UInt32Math::Add(
            UInt32Math::Add(m_f32RegSlots->GetRegisterCount(), m_f64RegSlots->GetRegisterCount()),
            m_i32RegSlots->GetRegisterCount()
        ));
    }
    catch (WasmCompilationException& ex)
    {
        if (!PHASE_ON1(Js::WasmLazyTrapPhase))
        {
            throw WasmCompilationException(_u("function %s: %s"), functionName, ex.GetErrorMessage());
        }

        // Since we will continue compilation for other functions, clear the eval stack
        m_evalStack.Clear();
        Assert(m_module->lazyTraps != nullptr);
        WasmCompilationException* lazyTrap = Anew(&m_alloc, WasmCompilationException, _u("(delayed) function %s: %s"), functionName, ex.GetErrorMessage());
        m_module->lazyTraps[wasmInfo->GetNumber()] = lazyTrap;
    }
    return m_currentFunc;
}

void
WasmBytecodeGenerator::EnregisterLocals()
{
    m_locals = AnewArray(&m_alloc, WasmLocal, m_funcInfo->GetLocalCount());

    for (uint i = 0; i < m_funcInfo->GetLocalCount(); ++i)
    {
        WasmTypes::WasmType type = m_funcInfo->GetLocal(i);
        WasmRegisterSpace * regSpace = GetRegisterSpace(type);
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
                m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, m_locals[i].location, 0.0f);
                break;
            case WasmTypes::F64:
                m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, m_locals[i].location, 0.0);
                break;
            case WasmTypes::I32:
                m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, m_locals[i].location, 0);
                break;
            case WasmTypes::I64:
                throw WasmCompilationException(_u("I64 locals NYI"));
            default:
                Assume(UNREACHED);
            }
        }
    }
}

#if DBG_DUMP
void
WasmBytecodeGenerator::PrintOpName(WasmOp op) const
{
    switch (op)
    {
#define WASM_KEYWORD(token, name) \
    case wn##token: \
        Output::Print(_u(#token ## "\r\n")); \
        break;
#include "WasmKeywords.h"
    }
}
#endif

EmitInfo
WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    DebugPrintOp(op);

    EmitInfo info;

    switch (op)
    {
    case wnGETLOCAL:
        info = EmitGetLocal();
        break;
    case wnSETLOCAL:
        info = EmitSetLocal();
        break;
    case wnRETURN:
        info = EmitReturnExpr();
        break;
    case wnCONST_F32:
        info = EmitConst<WasmTypes::F32>();
        break;
    case wnCONST_F64:
        info = EmitConst<WasmTypes::F64>();
        break;
    case wnCONST_I32:
        info = EmitConst<WasmTypes::I32>();
        break;
    case wnBLOCK:
        info = EmitBlock();
        break;
    case wnLOOP:
        info = EmitLoop();
        break;
    case wnCALL_IMPORT:
        info = EmitCall<wnCALL_IMPORT>();
        break;
    case wnCALL:
        info = EmitCall<wnCALL>();
        break;
    case wnCALL_INDIRECT:
        info = EmitCall<wnCALL_INDIRECT>();
        break;
    case wnIF:
        info = EmitIfElseExpr();
        break;
    case wnELSE:
        throw WasmCompilationException(_u("Unexpected else opcode"));
    case wnEND:
        throw WasmCompilationException(_u("Unexpected end opcode"));
    case wnBR:
        info = EmitBr<wnBR>();
        break;
    case wnBR_IF:
        info = EmitBr<wnBR_IF>();
        break;
    case wnSELECT:
        info = EmitSelect();
        break;
    case wnBR_TABLE:
        info = EmitBrTable();
        break;
    case wnNOP:
        info = EmitInfo();
        break;
#define WASM_MEMREAD(token, name, type) \
    case wn##token: \
        info = EmitMemRead<wn##token, WasmTypes::##type>(); \
        break;
#define WASM_MEMSTORE(token, name, type) \
    case wn##token: \
        info = EmitMemStore<wn##token, WasmTypes::##type>(); \
        break;
#define WASM_KEYWORD_BIN_TYPED(token, name, op, resultType, lhsType, rhsType) \
    case wn##token: \
        info = EmitBinExpr<Js::OpCodeAsmJs::##op, WasmTypes::##resultType, WasmTypes::##lhsType, WasmTypes::##rhsType>(); \
        break;
#define WASM_KEYWORD_UNARY(token, name, op, resultType, inputType) \
    case wn##token: \
        info = EmitUnaryExpr<Js::OpCodeAsmJs::##op, WasmTypes::##resultType, WasmTypes::##inputType>(); \
        break;
#include "WasmKeywords.h"
    case wnNYI:
        switch (m_reader.GetLastBinOp())
        {
#define WASM_OPCODE(opname, opcode, token, sig) \
    case opcode: \
        throw WasmCompilationException(_u("Operator " #opname## " NYI"));
        break;
#include "WasmBinaryOpcodes.h"
        default:
            break;
        }
    default:
        throw WasmCompilationException(_u("Unknown expression's op 0x%X"), op);
    }
    PushEvalStack(info);
    return info;
}

EmitInfo
WasmBytecodeGenerator::EmitGetLocal()
{
    if (m_funcInfo->GetLocalCount() < m_reader.m_currentNode.var.num)
    {
        throw WasmCompilationException(_u("%u is not a valid local"), m_reader.m_currentNode.var.num);
    }

    WasmLocal local = m_locals[m_reader.m_currentNode.var.num];

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    m_writer.AsmReg2(op, tmpReg, local.location);

    return EmitInfo(tmpReg, local.type);
}

EmitInfo
WasmBytecodeGenerator::EmitSetLocal()
{
    uint localNum = m_reader.m_currentNode.var.num;
    if (localNum >= m_funcInfo->GetLocalCount())
    {
        throw WasmCompilationException(_u("%u is not a valid local"), localNum);
    }

    WasmLocal local = m_locals[localNum];

    EmitInfo info = PopEvalStack();
    if (info.type != local.type)
    {
        throw WasmCompilationException(_u("TypeError in setlocal for %u"), localNum);
    }

    m_writer.AsmReg2(GetLoadOp(local.type), local.location, info.location);

    return info;
}

template<WasmTypes::WasmType type>
EmitInfo
WasmBytecodeGenerator::EmitConst()
{
    WasmRegisterSpace * regSpace = GetRegisterSpace(type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    switch (type)
    {
    case WasmTypes::F32:
        m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, tmpReg, m_reader.m_currentNode.cnst.f32);
        break;
    case WasmTypes::F64:
        m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, tmpReg, m_reader.m_currentNode.cnst.f64);
        break;
    case WasmTypes::I32:
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, tmpReg, m_reader.m_currentNode.cnst.i32);
        break;
    default:
        throw WasmCompilationException(_u("Unknown type %u"), type);
    }

    return EmitInfo(tmpReg, type);
}

EmitInfo
WasmBytecodeGenerator::EmitBlockCommon()
{
    WasmOp op;
    EmitInfo blockInfo;
    EnterEvalStackScope();
    while ((op = m_reader.ReadFromBlock()) != wnEND && op != wnELSE)
    {
        blockInfo = EmitExpr(op);
    }
    DebugPrintOp(op);
    ExitEvalStackScope();
    return blockInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitBlock()
{
    Js::ByteCodeLabel blockLabel = m_writer.DefineLabel();

    PushLabel(blockLabel);
    EmitInfo blockInfo = EmitBlockCommon();
    YieldToBlock(0, blockInfo);
    m_writer.MarkAsmJsLabel(blockLabel);
    blockInfo = PopLabel(blockLabel);

    // block yields last value
    return blockInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitLoop()
{
    Js::ByteCodeLabel loopTailLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel loopHeadLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel loopLandingPadLabel = m_writer.DefineLabel();

    uint loopId = m_writer.EnterLoop(loopHeadLabel);
    PushLabel(loopTailLabel);
    // We don't want nested block to jump directly to the loop header
    // instead, jump to the landing pad and let it jump back to the loop header
    PushLabel(loopLandingPadLabel, false);
    EmitInfo loopInfo = EmitBlockCommon();
    YieldToBlock(1, loopInfo);

    // By default we don't loop, jump over the landing pad
    m_writer.AsmBr(loopTailLabel);
    m_writer.MarkAsmJsLabel(loopLandingPadLabel);
    m_writer.AsmBr(loopHeadLabel);
    m_writer.MarkAsmJsLabel(loopTailLabel);
    PopLabel(loopLandingPadLabel);
    loopInfo = PopLabel(loopTailLabel);
    m_writer.ExitLoop(loopId);

    return loopInfo;
}

template<WasmOp wasmOp>
EmitInfo
WasmBytecodeGenerator::EmitCall()
{
    uint funcNum = Js::Constants::UninitializedValue;
    uint signatureId = Js::Constants::UninitializedValue;
    WasmSignature * calleeSignature = nullptr;
    EmitInfo indirectIndexInfo;
    switch (wasmOp)
    {
    case wnCALL:
        funcNum = m_reader.m_currentNode.call.num;
        if (funcNum >= m_module->info->GetFunctionCount())
        {
            throw WasmCompilationException(_u("Call is to unknown function"));
        }
        calleeSignature = m_module->info->GetFunSig(funcNum)->GetSignature();
        break;
    case wnCALL_IMPORT:
    {
        funcNum = m_reader.m_currentNode.call.num;
        if (funcNum >= m_module->info->GetImportCount())
        {
            throw WasmCompilationException(L"Call is to unknown function");
        }
        uint sigId = m_module->info->GetFunctionImport(funcNum)->sigId;
        calleeSignature = m_module->info->GetSignature(sigId);
        break;
    }
    case wnCALL_INDIRECT:
        signatureId = m_reader.m_currentNode.call.num;
        calleeSignature = m_module->info->GetSignature(signatureId);
        break;
    default:
        Assume(UNREACHED);
    }

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (wasmOp == wnCALL_IMPORT)
    {
        argSize = (Js::ArgSlot)(calleeSignature->GetParamCount() * sizeof(Js::Var));
        startCallOp = Js::OpCodeAsmJs::StartCall;
    }
    else
    {
        startCallOp = Js::OpCodeAsmJs::I_StartCall;
        argSize = (Js::ArgSlot)calleeSignature->GetParamsSize();
    }
    // Add return value
    argSize += sizeof(Js::Var);

    if (argSize >= UINT16_MAX)
    {
        throw WasmCompilationException(_u("Argument size too big"));
    }

    m_writer.AsmStartCall(startCallOp, argSize);

    if (calleeSignature->GetParamCount() != m_reader.m_currentNode.call.arity)
    {
        throw WasmCompilationException(_u("Mismatch between call signature and arity"));
    }

    int32 argsBytesLeft = argSize;
    for (int i = calleeSignature->GetParamCount() - 1; i >= 0; --i)
    {
        EmitInfo info = PopEvalStack();
        if (calleeSignature->GetParam(i) != info.type)
        {
            throw WasmCompilationException(_u("Call argument does not match formal type"));
        }

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        switch (info.type)
        {
        case WasmTypes::F32:
            if (wasmOp == wnCALL_IMPORT) 
            {
                throw WasmCompilationException(_u("External calls with float argument NYI"));
            }
            argOp = Js::OpCodeAsmJs::I_ArgOut_Flt;
            break;
        case WasmTypes::F64:
            if (wasmOp == wnCALL_IMPORT)
            {
                argOp = Js::OpCodeAsmJs::ArgOut_Db;
            }
            else
            {
                argOp = Js::OpCodeAsmJs::I_ArgOut_Db;
            }
            break;
        case WasmTypes::I32:
            argOp = wasmOp == wnCALL_IMPORT ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
            break;
        default:
            throw WasmCompilationException(_u("Unknown argument type %u"), info.type);
        }
        argsBytesLeft -= wasmOp == wnCALL_IMPORT ? sizeof(Js::Var) : calleeSignature->GetParamSize(i);
        if (argsBytesLeft < 0 || (argsBytesLeft % sizeof(Js::Var)) != 0) 
        {
            throw WasmCompilationException(_u("Error while emitting call arguments"));
        }
        Js::RegSlot argLoc = argsBytesLeft / sizeof(Js::Var);

        m_writer.AsmReg2(argOp, argLoc, info.location);
        ReleaseLocation(&info);
    }

    // emit call
    switch (wasmOp)
    {
    case wnCALL:
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->funcOffset);
        break;
    case wnCALL_IMPORT:
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->importFuncOffset);
        break;
    case wnCALL_INDIRECT:
        indirectIndexInfo = PopEvalStack();
        if (indirectIndexInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(_u("Indirect call index must be int type"));
        }
        // todo:: Add bounds check. Asm.js doesn't need it because there has to be an & operator
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlotArr, 0, 1, calleeSignature->GetSignatureId() + m_module->indirFuncTableOffset);
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdArr_Func, 0, 0, indirectIndexInfo.location);
        ReleaseLocation(&indirectIndexInfo);
        break;
    default:
        Assume(UNREACHED);
    }

    // calculate number of RegSlots the arguments consume
    Js::ArgSlot args;
    Js::OpCodeAsmJs callOp = Js::OpCodeAsmJs::Nop;
    if (wasmOp == wnCALL_IMPORT)
    {
        args = (Js::ArgSlot)(calleeSignature->GetParamCount() + 1);
        callOp = Js::OpCodeAsmJs::Call;
    }
    else
    {
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var))));
        callOp = Js::OpCodeAsmJs::I_Call;
    }

    m_writer.AsmCall(callOp, 0, 0, args, GetAsmJsReturnType(calleeSignature->GetResultType()));

    // emit result coercion
    EmitInfo retInfo;
    retInfo.type = calleeSignature->GetResultType();
    if (retInfo.type != WasmTypes::Void)
    {
        Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
        switch (retInfo.type)
        {
        case WasmTypes::F32:
            retInfo.location = m_f32RegSlots->AcquireTmpRegister();
            convertOp = wasmOp == wnCALL_IMPORT ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::I_Conv_VTF;
            break;
        case WasmTypes::F64:
            retInfo.location = m_f64RegSlots->AcquireTmpRegister();
            convertOp = wasmOp == wnCALL_IMPORT ? Js::OpCodeAsmJs::Conv_VTD : Js::OpCodeAsmJs::I_Conv_VTD;
            break;
        case WasmTypes::I32:
            retInfo.location = m_i32RegSlots->AcquireTmpRegister();
            convertOp = wasmOp == wnCALL_IMPORT ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::I_Conv_VTI;
            break;
        case WasmTypes::I64:
            throw WasmCompilationException(_u("I64 return type NYI"));
        default:
            throw WasmCompilationException(_u("Unknown call return type %u"), retInfo.type);
        }
        m_writer.AsmReg2(convertOp, retInfo.location, 0);
    }

    // track stack requirements for out params

    // + 1 for return address
    uint maxDepthForLevel = args + 1;
    if (maxDepthForLevel > m_maxArgOutDepth)
    {
        m_maxArgOutDepth = maxDepthForLevel;
    }

    return retInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitIfElseExpr()
{
    ++m_nestedIfLevel;

    if (m_nestedIfLevel == 0)
    {
        // overflow
        Js::Throw::OutOfMemory();
    }

    EmitInfo checkExpr = PopEvalStack();

    if (checkExpr.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("If expression must have type i32"));
    }

    // TODO: save this so I can break
    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    m_i32RegSlots->ReleaseLocation(&checkExpr);

    EmitInfo trueExpr = EmitBlock();

    m_writer.AsmBr(endLabel);

    m_writer.MarkAsmJsLabel(falseLabel);

    WasmOp op = m_reader.GetLastOp(); // wnEND or wnELSE
    EmitInfo retInfo;
    EmitInfo falseExpr;
    if (op == wnELSE)
    {
        falseExpr = EmitBlock(); 
        // Read END
        op = m_reader.GetLastOp();
    }

    if (!WasmTypes::IsLocalType(trueExpr.type) || falseExpr.type != trueExpr.type)
    {
        // if types are void or mismatched if/else doesn't yield a value
        ReleaseLocation(&trueExpr);
        ReleaseLocation(&falseExpr);
    }
    else
    {
        // EmitExpr always yields temp register, so these must be different
        Assert(falseExpr.location != trueExpr.location);
        // we will use result info true expr as our result info
        m_writer.AsmReg2(GetLoadOp(falseExpr.type), trueExpr.location, falseExpr.location);
        ReleaseLocation(&falseExpr);
        retInfo = trueExpr;
    }

    if (op != wnEND)
    {
        throw WasmCompilationException(_u("Missing END opcode"));
    }

    m_writer.MarkAsmJsLabel(endLabel);

    Assert(m_nestedIfLevel > 0);
    --m_nestedIfLevel;

    return retInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitBrTable()
{
    const uint arity = m_reader.m_currentNode.brTable.arity;
    const uint numTargets = m_reader.m_currentNode.brTable.numTargets;
    const UINT* targetTable = m_reader.m_currentNode.brTable.targetTable;
    const UINT defaultEntry = m_reader.m_currentNode.brTable.defaultTarget;

    // Compile scrutinee
    EmitInfo scrutineeInfo = PopEvalStack();
    if (scrutineeInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"br_table expression must be of type I32");
    }

    m_writer.AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeInfo.location, scrutineeInfo.location);
    EmitInfo yieldInfo;
    if (arity == 1) 
    {
        yieldInfo = PopEvalStack();
    }
    // Compile cases
    for (uint i = 0; i < numTargets; i++)
    {
        uint target = targetTable[i];
        YieldToBlock(target, yieldInfo);
        Js::ByteCodeLabel targetLabel = GetLabel(target);
        m_writer.AsmBrReg1Const1(Js::OpCodeAsmJs::Case_IntConst, targetLabel, scrutineeInfo.location, i);
    }

    YieldToBlock(defaultEntry, yieldInfo);
    m_writer.AsmBr(GetLabel(defaultEntry), Js::OpCodeAsmJs::EndSwitch_Int);
    ReleaseLocation(&scrutineeInfo);
    ReleaseLocation(&yieldInfo);

    return EmitInfo(WasmTypes::Unreachable);
}

template<Js::OpCodeAsmJs op, WasmTypes::WasmType resultType, WasmTypes::WasmType lhsType, WasmTypes::WasmType rhsType>
EmitInfo
WasmBytecodeGenerator::EmitBinExpr()
{
    EmitInfo rhs = PopEvalStack();
    EmitInfo lhs = PopEvalStack();

    if (lhsType != lhs.type)
    {
        throw WasmCompilationException(_u("Invalid type for LHS"));
    }
    if (rhsType != rhs.type)
    {
        throw WasmCompilationException(_u("Invalid type for RHS"));
    }

    GetRegisterSpace(rhsType)->ReleaseLocation(&rhs);
    GetRegisterSpace(lhsType)->ReleaseLocation(&lhs);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer.AsmReg3(op, resultReg, lhs.location, rhs.location);

    return EmitInfo(resultReg, resultType);
}

template<Js::OpCodeAsmJs op, WasmTypes::WasmType resultType, WasmTypes::WasmType inputType>
EmitInfo
WasmBytecodeGenerator::EmitUnaryExpr()
{
    EmitInfo info = PopEvalStack();

    if (inputType != info.type)
    {
        throw WasmCompilationException(_u("Invalid input type"));
    }

    GetRegisterSpace(inputType)->ReleaseLocation(&info);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer.AsmReg2(op, resultReg, info.location);

    return EmitInfo(resultReg, resultType);
}

template<WasmOp wasmOp, WasmTypes::WasmType type>
EmitInfo
WasmBytecodeGenerator::EmitMemRead()
{
    const uint offset = m_reader.m_currentNode.mem.offset;
    m_currentFunc->body->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);

    EmitInfo exprInfo = PopEvalStack();

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("Index expression must be of type I32"));
    }
    if (offset != 0)
    {
        Js::RegSlot tempReg = m_i32RegSlots->AcquireTmpRegister();
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, tempReg, offset);

        m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Int, exprInfo.location, exprInfo.location, tempReg);
        m_i32RegSlots->ReleaseTmpRegister(tempReg);
    }
    m_i32RegSlots->ReleaseLocation(&exprInfo);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();

    m_writer.AsmTypedArr(Js::OpCodeAsmJs::LdArr, resultReg, exprInfo.location, GetViewType(wasmOp));

    return EmitInfo(resultReg, type);
}

template<WasmOp wasmOp, WasmTypes::WasmType type>
EmitInfo
WasmBytecodeGenerator::EmitMemStore()
{
    const uint offset = m_reader.m_currentNode.mem.offset;
    m_currentFunc->body->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);
    // TODO (michhol): combine with MemRead

    EmitInfo rhsInfo = PopEvalStack();
    EmitInfo exprInfo = PopEvalStack();

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("Index expression must be of type I32"));
    }
    if (offset != 0)
    {
        Js::RegSlot indexReg = m_i32RegSlots->AcquireTmpRegister();
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, indexReg, offset);

        m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Int, exprInfo.location, exprInfo.location, indexReg);
        m_i32RegSlots->ReleaseTmpRegister(indexReg);
    }
    if (rhsInfo.type != type)
    {
        throw WasmCompilationException(_u("Invalid type for store op"));
    }

    m_writer.AsmTypedArr(Js::OpCodeAsmJs::StArr, rhsInfo.location, exprInfo.location, GetViewType(wasmOp));
    ReleaseLocation(&rhsInfo);
    ReleaseLocation(&exprInfo);

    Js::RegSlot retLoc = GetRegisterSpace(type)->AcquireTmpRegister();
    if (retLoc != rhsInfo.location)
    {
        m_writer.AsmReg2(GetLoadOp(type), retLoc, rhsInfo.location);
    }

    return EmitInfo(retLoc, type);
}

template<typename T>
Js::RegSlot
WasmBytecodeGenerator::GetConstReg(T constVal)
{
    Js::RegSlot location = m_funcInfo->GetConst(constVal);
    if (location == Js::Constants::NoRegister)
    {
        WasmRegisterSpace * regSpace = GetRegisterSpace(m_reader.m_currentNode.type);
        location = regSpace->AcquireConstRegister();
        m_funcInfo->AddConst(constVal, location);
    }
    return location;
}

EmitInfo
WasmBytecodeGenerator::EmitReturnExpr()
{
    if (m_funcInfo->GetResultType() == WasmTypes::Void)
    {
        if (m_reader.m_currentNode.ret.arity != 0)
        {
            throw WasmCompilationException(_u("Nonzero arity for return op in void function"));
        }
        // TODO (michhol): consider moving off explicit 0 for return reg
        m_writer.AsmReg1(Js::OpCodeAsmJs::LdUndef, 0);
    }
    else
    {
        if (m_reader.m_currentNode.ret.arity != 1)
        {
            throw WasmCompilationException(_u("Unexpected arity for return op"));
        }
        EmitInfo retExprInfo = PopEvalStack();

        if (m_funcInfo->GetResultType() != retExprInfo.type)
        {
            throw WasmCompilationException(_u("Result type must match return type"));
        }

        Js::OpCodeAsmJs retOp = Js::OpCodeAsmJs::Nop;
        switch (retExprInfo.type)
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
        default:
            throw WasmCompilationException(_u("Unknown return type %u"), retExprInfo.type);
        }

        m_writer.Conv(retOp, 0, retExprInfo.location);
        ReleaseLocation(&retExprInfo);
    }
    m_writer.AsmBr(m_funcInfo->GetExitLabel());

    return EmitInfo(WasmTypes::Unreachable);
}

EmitInfo
WasmBytecodeGenerator::EmitSelect()
{
    EmitInfo conditionInfo = PopEvalStack();
    if (conditionInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("select condition must have I32 type"));
    }

    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel doneLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, conditionInfo.location);
    ReleaseLocation(&conditionInfo);

    m_writer.AsmBr(doneLabel);
    m_writer.MarkAsmJsLabel(falseLabel);

    EmitInfo falseInfo = PopEvalStack();
    EmitInfo trueInfo = PopEvalStack();
    if (trueInfo.type != falseInfo.type)
    {
        throw WasmCompilationException(_u("select operands must both have same type"));
    }

    Js::OpCodeAsmJs op = GetLoadOp(trueInfo.type);

    m_writer.AsmReg2(op, trueInfo.location, falseInfo.location);
    ReleaseLocation(&falseInfo);

    m_writer.MarkAsmJsLabel(doneLabel);

    return trueInfo;
}

template<WasmOp wasmOp>
EmitInfo
WasmBytecodeGenerator::EmitBr()
{
    UINT depth = m_reader.m_currentNode.br.depth;
    bool hasSubExpr = m_reader.m_currentNode.br.hasSubExpr;

    EmitInfo conditionInfo;
    if (wasmOp == WasmOp::wnBR_IF)
    {
        conditionInfo = PopEvalStack();
        if (conditionInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(_u("br_if condition must have I32 type"));
        }
    }

    EmitInfo info;
    if (hasSubExpr)
    {
        // TODO: Handle value that Break is supposed to "throw".
        info = PopEvalStack();
    }
    YieldToBlock(depth, info);

    Js::ByteCodeLabel target = GetLabel(depth);
    if (wasmOp == WasmOp::wnBR)
    {
        m_writer.AsmBr(target);
    }
    else
    {
        Assert(wasmOp == WasmOp::wnBR_IF);
        m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrTrue_Int, target, conditionInfo.location);
        m_i32RegSlots->ReleaseLocation(&conditionInfo);
    }

    ReleaseLocation(&info);
    return EmitInfo(WasmTypes::Unreachable);
}

/* static */
Js::AsmJsRetType
WasmBytecodeGenerator::GetAsmJsReturnType(WasmTypes::WasmType wasmType)
{
    switch (wasmType)
    {
    case WasmTypes::F32:
        return Js::AsmJsRetType::Float;
    case WasmTypes::F64:
        return Js::AsmJsRetType::Double;
    case WasmTypes::I32:
        return Js::AsmJsRetType::Signed;
    case WasmTypes::Void:
        return Js::AsmJsRetType::Void;
    case WasmTypes::I64:
        throw WasmCompilationException(_u("I64 support NYI"));
    default:
        throw WasmCompilationException(_u("Unknown return type %u"), wasmType);
    }
}

/* static */
Js::AsmJsVarType
WasmBytecodeGenerator::GetAsmJsVarType(WasmTypes::WasmType wasmType)
{
    Js::AsmJsVarType asmType = Js::AsmJsVarType::Int;
    switch (wasmType)
    {
    case WasmTypes::F32:
        return Js::AsmJsVarType::Float;
    case WasmTypes::F64:
        return Js::AsmJsVarType::Double;
    case WasmTypes::I32:
        return Js::AsmJsVarType::Int;
    case WasmTypes::I64:
        throw WasmCompilationException(_u("I64 support NYI"));
    default:
        throw WasmCompilationException(_u("Unknown var type %u"), wasmType);
    }
}

/* static */
Js::OpCodeAsmJs
WasmBytecodeGenerator::GetLoadOp(WasmTypes::WasmType wasmType)
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
        throw WasmCompilationException(_u("I64 support NYI"));
    default:
        throw WasmCompilationException(_u("Unknown load operator %u"), wasmType);
    }
}

/* static */
Js::ArrayBufferView::ViewType
WasmBytecodeGenerator::GetViewType(WasmOp op)
{
    switch (op)
    {
    case wnLOAD8S_I32:
    case wnSTORE8_I32:
        return Js::ArrayBufferView::TYPE_INT8;
        break;
    case wnLOAD8U_I32:
        return Js::ArrayBufferView::TYPE_UINT8;
        break;
    case wnLOAD16S_I32:
    case wnSTORE16_I32:
        return Js::ArrayBufferView::TYPE_INT16;
        break;
    case wnLOAD16U_I32:
        return Js::ArrayBufferView::TYPE_UINT16;
        break;
    case wnLOAD_F32:
    case wnSTORE_F32:
        return Js::ArrayBufferView::TYPE_FLOAT32;
        break;
    case wnLOAD_F64:
    case wnSTORE_F64:
        return Js::ArrayBufferView::TYPE_FLOAT64;
        break;
    case wnLOAD_I32:
    case wnSTORE_I32:
        return Js::ArrayBufferView::TYPE_INT32;
        break;
    default:
        throw WasmCompilationException(_u("Could not match typed array name"));
    }
}


void
WasmBytecodeGenerator::ReleaseLocation(EmitInfo * info)
{
    if (WasmTypes::IsLocalType(info->type))
    {
        GetRegisterSpace(info->type)->ReleaseLocation(info);
    }
}

EmitInfo
WasmBytecodeGenerator::PopLabel(Js::ByteCodeLabel labelValidation)
{
    Assert(m_blockInfos.Count() > 0);
    BlockInfo info = m_blockInfos.Pop();
    UNREFERENCED_PARAMETER(labelValidation);
    Assert(info.label == labelValidation);

    EmitInfo yieldEmitInfo;
    if (info.yieldInfo)
    {
        BlockYieldInfo* yieldInfo = info.yieldInfo;
        for (int type = WasmTypes::Void + 1; type < WasmTypes::Limit; ++type)
        {
            if (
                yieldInfo->yieldLocs[type] != Js::Constants::NoRegister &&
                type != yieldInfo->type // Do not release the location used
            )
            {
                GetRegisterSpace((WasmTypes::WasmType)type)->ReleaseTmpRegister(yieldInfo->yieldLocs[type]);
            }
        }
        if (WasmTypes::IsLocalType(yieldInfo->type))
        {
            Assert(yieldInfo->type < WasmTypes::Limit);
            yieldEmitInfo.location = yieldInfo->yieldLocs[yieldInfo->type];
            yieldEmitInfo.type = yieldInfo->type;
        }
        else 
        {
            yieldEmitInfo.type = yieldInfo->type == WasmTypes::Limit ? WasmTypes::Unreachable : WasmTypes::Void;
            yieldEmitInfo.location = Js::Constants::NoRegister;
        }
    }
    return yieldEmitInfo;
}

void
WasmBytecodeGenerator::PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo /*= true*/)
{
    BlockInfo info;
    info.label = label;
    if (addBlockYieldInfo)
    {
        info.yieldInfo = Anew(&m_alloc, BlockYieldInfo);
        for (int type = WasmTypes::Void + 1; type < WasmTypes::Limit; ++type)
        {
            try 
            {
                info.yieldInfo->yieldLocs[type] = GetRegisterSpace((WasmTypes::WasmType)type)->AcquireTmpRegister();
            } 
            catch (WasmCompilationException) 
            {
                // This might be a NYI type
                info.yieldInfo->yieldLocs[type] = Js::Constants::NoRegister;
            }
        }
    }
    m_blockInfos.Push(info);
}

void
WasmBytecodeGenerator::YieldToBlock(uint relativeDepth, EmitInfo expr)
{
    BlockInfo info = GetBlockInfo(relativeDepth);
    BlockYieldInfo* yieldInfo = info.yieldInfo;
    if (
        !yieldInfo || // Cannot yield value to this block
        expr.type == WasmTypes::Unreachable // Do not modify this blocks type for unreachable
    )
    {
        return;
    }
    
    // If the type differs on any path, do not yield.
    // This will introduce some dead code on the first paths that tried to Yield a valid type
    if (
        yieldInfo->type == WasmTypes::Void ||
        expr.type == WasmTypes::Void ||
        (yieldInfo->type != WasmTypes::Limit && yieldInfo->type != expr.type)
    )
    {
        yieldInfo->type = WasmTypes::Void;
        return;
    }

    yieldInfo->type = expr.type;
    m_writer.AsmReg2(GetLoadOp(expr.type), yieldInfo->yieldLocs[expr.type], expr.location);
}

Wasm::BlockInfo
WasmBytecodeGenerator::GetBlockInfo(uint relativeDepth)
{
    if (relativeDepth >= (uint)m_blockInfos.Count())
    {
        throw WasmCompilationException(_u("Invalid branch target"));
    }
    return m_blockInfos.Peek(relativeDepth);
}

Js::ByteCodeLabel
WasmBytecodeGenerator::GetLabel(uint relativeDepth)
{
    return GetBlockInfo(relativeDepth).label;
}

WasmRegisterSpace *
WasmBytecodeGenerator::GetRegisterSpace(WasmTypes::WasmType type) const
{
    switch (type)
    {
    case WasmTypes::F32:
        return m_f32RegSlots;
    case WasmTypes::F64:
        return m_f64RegSlots;
    case WasmTypes::I32:
        return m_i32RegSlots;
    case WasmTypes::I64:
        throw WasmCompilationException(_u("I64 support NYI"));
    default:
        return nullptr;
    }
}

EmitInfo
WasmBytecodeGenerator::PopEvalStack()
{
    // The scope marker should at least be there
    Assert(!m_evalStack.Empty());
    EmitInfo info = m_evalStack.Pop();
    if (info.type == WasmTypes::Limit)
    {
        throw WasmCompilationException(_u("Missing operand"));
    }
    return info;
}

void
WasmBytecodeGenerator::PushEvalStack(EmitInfo info)
{
    Assert(!m_evalStack.Empty());
    m_evalStack.Push(info);
}

void
WasmBytecodeGenerator::EnterEvalStackScope()
{
    m_evalStack.Push(EmitInfo(WasmTypes::Limit));
}

void
WasmBytecodeGenerator::ExitEvalStackScope()
{
    Assert(!m_evalStack.Empty());
    EmitInfo info;
    while(info = m_evalStack.Pop(), info.type != WasmTypes::Limit)
    {
        ReleaseLocation(&info);
    }
}

void
WasmCompilationException::FormatError(const char16* _msg, va_list arglist)
{
    char16 buf[2048];

    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, _msg, arglist);
    errorMsg = SysAllocString(buf);
}

WasmCompilationException::~WasmCompilationException()
{
    if (errorMsg)
    {
        SysFreeString(errorMsg);
    }
}

WasmCompilationException::WasmCompilationException(const char16* _msg, ...)
{
    va_list arglist;
    va_start(arglist, _msg);
    FormatError(_msg, arglist);
}

WasmCompilationException::WasmCompilationException(const char16* _msg, va_list arglist)
{
    FormatError(_msg, arglist);
}

} // namespace Wasm

#endif // ENABLE_WASM
