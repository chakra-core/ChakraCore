//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

#if DBG_DUMP
#define DebugPrintOp(op) if (PHASE_TRACE(Js::WasmReaderPhase, GetFunctionBody())) { PrintOpName(op); }
#else
#define DebugPrintOp(op)
#endif

namespace Wasm
{
#define WASM_SIGNATURE(id, nTypes, ...) const WasmTypes::WasmType WasmOpCodeSignatures::id[] = {__VA_ARGS__};
#include "WasmBinaryOpcodes.h"

#if DBG_DUMP
void
WasmBytecodeGenerator::PrintOpName(WasmOp op) const
{
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
case wb##opname: \
    Output::Print(_u("%s\r\n"), _u(#opname)); \
    break;
#include "WasmBinaryOpCodes.h"
    }
}
#endif

/* static */
Js::AsmJsRetType
WasmToAsmJs::GetAsmJsReturnType(WasmTypes::WasmType wasmType)
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
WasmToAsmJs::GetAsmJsVarType(WasmTypes::WasmType wasmType)
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

typedef bool(*SectionProcessFunc)(WasmModuleGenerator*);
typedef void(*AfterSectionCallback)(WasmModuleGenerator*);

WasmModuleGenerator::WasmModuleGenerator(Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* sourceInfo, byte* binaryBuffer, uint binaryBufferLength) :
    m_sourceInfo(sourceInfo),
    m_scriptContext(scriptContext),
    m_recycler(scriptContext->GetRecycler())
{
    m_module = RecyclerNewFinalizedLeaf(m_recycler, WasmModule, scriptContext, binaryBuffer, binaryBufferLength);

    m_sourceInfo->EnsureInitialized(0);
    m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();
}

WasmModule*
WasmModuleGenerator::GenerateModule()
{
    m_module->SetHeapOffset(0);
    m_module->SetImportFuncOffset(m_module->GetHeapOffset() + 1);
    m_module->SetFuncOffset(m_module->GetHeapOffset() + 1);

    BVStatic<bSectLimit + 1> visitedSections;

    // Will callback regardless if the section is present or not
    AfterSectionCallback afterSectionCallback[bSectLimit + 1] = {};

    afterSectionCallback[bSectFunctionSignatures] = [](WasmModuleGenerator* gen) {
        gen->m_module->SetFuncOffset(gen->m_module->GetImportFuncOffset() + gen->m_module->GetImportCount());
    };
    afterSectionCallback[bSectIndirectFunctionTable] = [](WasmModuleGenerator* gen) {
        gen->m_module->SetIndirFuncTableOffset(gen->m_module->GetFuncOffset() + gen->m_module->GetFunctionCount());
    };
    afterSectionCallback[bSectFunctionBodies] = [](WasmModuleGenerator* gen) {
        uint32 funcCount = gen->m_module->GetFunctionCount();
        for (uint32 i = 0; i < funcCount; ++i)
        {
            gen->GenerateFunctionHeader(i);
        }
    };

    for (SectionCode sectionCode = (SectionCode)(bSectInvalid + 1); sectionCode < bSectLimit ; sectionCode = (SectionCode)(sectionCode + 1))
    {
        SectionCode precedent = SectionInfo::All[sectionCode].precedent;
        if (GetReader()->ReadNextSection((SectionCode)sectionCode))
        {
            if (precedent != bSectInvalid && !visitedSections.Test(precedent))
            {
                throw WasmCompilationException(_u("%s section missing before %s"),
                                               SectionInfo::All[precedent].name,
                                               SectionInfo::All[sectionCode].name);
            }
            visitedSections.Set(sectionCode);

            if (!GetReader()->ProcessCurrentSection())
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
        GetReader()->PrintOps();
    }
#endif
    // If we see a FunctionSignatures section we need to see a FunctionBodies section
    if (visitedSections.Test(bSectFunctionSignatures) && !visitedSections.Test(bSectFunctionBodies))
    {
        throw WasmCompilationException(_u("Missing required section: %s"), SectionInfo::All[bSectFunctionBodies].name);
    }

    return m_module;
}

void
WasmModuleGenerator::GenerateFunctionHeader(uint32 index)
{
    WasmFunctionInfo* wasmInfo = m_module->GetFunctionInfo(index);
    if (!wasmInfo)
    {
        throw WasmCompilationException(_u("Invalid function index %u"), index);
    }

    char16* functionName = nullptr;
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
            Wasm::WasmExport* funcExport = m_module->GetFunctionExport(iExport);
            if (funcExport && funcExport->nameLength > 0)
            {
                const uint32 funcIndex = funcExport->funcIndex - m_module->GetImportCount();
                if (funcIndex == wasmInfo->GetNumber())
                {
                    nameLength = funcExport->nameLength + 16;
                    functionName = RecyclerNewArrayLeafZ(m_recycler, char16, nameLength);
                    nameLength = swprintf_s(functionName, nameLength, _u("%s[%u]"), funcExport->name, wasmInfo->GetNumber());
                    break;
                }
            }
        }
    }

    if (!functionName)
    {
        functionName = RecyclerNewArrayLeafZ(m_recycler, char16, 32);
        nameLength = swprintf_s(functionName, 32, _u("wasm-function[%u]"), wasmInfo->GetNumber());
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
        Js::FunctionInfo::Attributes::None
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
    body->GetAsmJsFunctionInfo()->SetIsHeapBufferConst(true);

    WasmReaderInfo* readerInfo = RecyclerNew(m_recycler, WasmReaderInfo);
    readerInfo->m_funcInfo = wasmInfo;
    readerInfo->m_module = m_module;

    Js::AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
    info->SetWasmReaderInfo(readerInfo);

    if (wasmInfo->GetParamCount() >= Js::Constants::InvalidArgSlot)
    {
        Js::Throw::OutOfMemory();
    }
    Js::ArgSlot paramCount = (Js::ArgSlot)wasmInfo->GetParamCount();
    info->SetArgCount(paramCount);

    Js::ArgSlot argSizeLength = max(paramCount, 3ui16);
    info->SetArgSizeArrayLength(argSizeLength);
    uint* argSizeArray = RecyclerNewArrayLeafZ(m_recycler, uint, argSizeLength);
    info->SetArgsSizesArray(argSizeArray);

    if (m_module->GetMemory()->minSize > 0)
    {
        info->SetUsesHeapBuffer(true);
    }
    if (paramCount > 0)
    {
        body->SetHasImplicitArgIns(true);
        body->SetInParamsCount(paramCount + 1);
        body->SetReportedInParamsCount(paramCount + 1);
        info->SetArgTypeArray(RecyclerNewArrayLeaf(m_recycler, Js::AsmJsVarType::Which, paramCount));
    }
    Js::ArgSlot paramSize = 0;
    for (Js::ArgSlot i = 0; i < paramCount; ++i)
    {
        WasmTypes::WasmType type = wasmInfo->GetParam(i);
        info->SetArgType(WasmToAsmJs::GetAsmJsVarType(type), i);
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
    info->SetReturnType(WasmToAsmJs::GetAsmJsReturnType(wasmInfo->GetResultType()));
}

void
WasmBytecodeGenerator::GenerateFunctionBytecode(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo)
{
    WasmBytecodeGenerator generator(scriptContext, readerinfo);
    generator.GenerateFunction();
    if (!generator.GetReader()->IsCurrentFunctionCompleted())
    {
        throw WasmCompilationException(_u("Invalid function format"));
    }
}

WasmBytecodeGenerator::WasmBytecodeGenerator(Js::ScriptContext* scriptContext, WasmReaderInfo* readerInfo) :
    m_scriptContext(scriptContext),
    m_alloc(_u("WasmBytecodeGen"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_evalStack(&m_alloc),
    m_i32RegSlots(ReservedRegisterCount),
    m_f32RegSlots(ReservedRegisterCount),
    m_f64RegSlots(ReservedRegisterCount),
    m_blockInfos(&m_alloc)
{
#if DBG_DUMP
    m_i32RegSlots.mType = WAsmJs::RegisterSpace::INT32;
    m_f32RegSlots.mType = WAsmJs::RegisterSpace::FLOAT32;
    m_f64RegSlots.mType = WAsmJs::RegisterSpace::FLOAT64;
#endif

    m_writer.Create();
    m_funcInfo = readerInfo->m_funcInfo;
    m_module = readerInfo->m_module;
    // Init reader to current func offset
    GetReader()->SeekToFunctionBody(m_funcInfo->m_readerInfo);

    // Use binary size to estimate bytecode size
    const long astSize = readerInfo->m_funcInfo->m_readerInfo.size;
    m_writer.InitData(&m_alloc, astSize);
}

void
WasmBytecodeGenerator::GenerateFunction()
{
    TRACE_WASM_DECODER(_u("GenerateFunction %u \n"), m_funcInfo->GetNumber());
    if (PHASE_OFF(Js::WasmBytecodePhase, GetFunctionBody()))
    {
        throw WasmCompilationException(_u("Compilation skipped"));
    }
    Js::AutoProfilingPhase functionProfiler(m_scriptContext, Js::WasmFunctionBodyPhase);
    Unused(functionProfiler);

    m_maxArgOutDepth = 0;

    // TODO: fix these bools
    m_writer.Begin(GetFunctionBody(), &m_alloc, true, true, false);
    try
    {
        m_funcInfo->SetExitLabel(m_writer.DefineLabel());
        EnregisterLocals();

        WasmOp op = wbLimit;
        EnterEvalStackScope();
        while ((op = GetReader()->ReadExpr()) != wbFuncEnd)
        {
            EmitExpr(op);
        }
        DebugPrintOp(op);

        EmitReturnExpr();
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
    if (PHASE_DUMP(Js::ByteCodePhase, GetFunctionBody()))
    {
        Js::AsmJsByteCodeDumper::DumpBasic(GetFunctionBody());
    }
#endif

    Js::AsmJsFunctionInfo * info = GetFunctionBody()->GetAsmJsFunctionInfo();
    info->SetIntVarCount(m_i32RegSlots.GetVarCount());
    info->SetFloatVarCount(m_f32RegSlots.GetVarCount());
    info->SetDoubleVarCount(m_f64RegSlots.GetVarCount());

    info->SetIntTmpCount(m_i32RegSlots.GetTmpCount());
    info->SetFloatTmpCount(m_f32RegSlots.GetTmpCount());
    info->SetDoubleTmpCount(m_f64RegSlots.GetTmpCount());

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
    GetFunctionBody()->CheckAndSetConstantCount(nbConst);

    info->SetReturnType(WasmToAsmJs::GetAsmJsReturnType(m_funcInfo->GetResultType()));

    uint32 byteOffset = ReservedRegisterCount * sizeof(Js::Var);
    info->SetIntByteOffset(byteOffset);
    byteOffset = UInt32Math::Add(byteOffset, UInt32Math::Mul(m_i32RegSlots.GetRegisterCount(), sizeof(int32)));
    info->SetFloatByteOffset(byteOffset);
    byteOffset = UInt32Math::Add(info->GetFloatByteOffset(), UInt32Math::Mul(m_f32RegSlots.GetRegisterCount(), sizeof(float)));
    // The offsets are stored as int, since we do uint32 math, make sure we haven't overflowed INT_MAX
    if (byteOffset >= INT_MAX)
    {
        Js::Throw::OutOfMemory();
    }
    byteOffset = Math::AlignOverflowCheck<int>(byteOffset, sizeof(double));
    info->SetDoubleByteOffset(byteOffset);

    GetFunctionBody()->SetOutParamMaxDepth(m_maxArgOutDepth);
    GetFunctionBody()->SetVarCount(UInt32Math::Add(
        UInt32Math::Add(m_f32RegSlots.GetRegisterCount(), m_f64RegSlots.GetRegisterCount()),
        m_i32RegSlots.GetRegisterCount()
    ));
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

void
WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    DebugPrintOp(op);
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
    case opcode: \
        if (nyi) throw WasmCompilationException(_u("Operator %s NYI"), _u(#opname)); break;
#include "WasmBinaryOpcodes.h"
    default:
        break;
    }

    EmitInfo info;

    switch (op)
    {
    case wbGetLocal:
        info = EmitGetLocal();
        break;
    case wbSetLocal:
        EmitSetLocal(false);
        return;
    case wbTeeLocal:
        info = EmitSetLocal(true);
        break;
    case wbReturn:
        info = EmitReturnExpr();
        break;
    case wbF32Const:
        info = EmitConst<WasmTypes::F32>();
        break;
    case wbF64Const:
        info = EmitConst<WasmTypes::F64>();
        break;
    case wbI32Const:
        info = EmitConst<WasmTypes::I32>();
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
        info = EmitBr<wbBr>();
        break;
    case wbBrIf:
        info = EmitBr<wbBrIf>();
        break;
    case wbSelect:
        info = EmitSelect();
        break;
    case wbBrTable:
        info = EmitBrTable();
        break;
    case wbDrop:
        EmitDrop();
        return;
    case wbNop:
        info = EmitInfo();
        break;
    case wbCurrentMemory:
        {
        Js::RegSlot tempReg = m_i32RegSlots.AcquireTmpRegister();
        m_writer.AsmReg1(Js::OpCodeAsmJs::CurrentMemory_Int, tempReg);
        info = EmitInfo(tempReg, WasmTypes::I32);
        }
        break;
    case wbUnreachable:
        m_writer.EmptyAsm(Js::OpCodeAsmJs::Unreachable_Void);
        info = EmitInfo(WasmTypes::Unreachable);
        break;
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess<wb##opname, WasmOpCodeSignatures::sig>(false); \
        break;
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        EmitMemAccess<wb##opname, WasmOpCodeSignatures::sig>(true); \
        return;
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 3);\
        info = EmitBinExpr<Js::OpCodeAsmJs::##asmjop, WasmOpCodeSignatures::sig>(); \
        break;
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 2);\
        info = EmitUnaryExpr<Js::OpCodeAsmJs::##asmjop, WasmOpCodeSignatures::sig>(); \
        break;
#include "WasmBinaryOpCodes.h"
    default:
        throw WasmCompilationException(_u("Unknown expression's op 0x%X"), op);
    }
    PushEvalStack(info);
}

EmitInfo
WasmBytecodeGenerator::EmitGetLocal()
{
    if (m_funcInfo->GetLocalCount() < GetReader()->m_currentNode.var.num)
    {
        throw WasmCompilationException(_u("%u is not a valid local"), GetReader()->m_currentNode.var.num);
    }

    WasmLocal local = m_locals[GetReader()->m_currentNode.var.num];

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    m_writer.AsmReg2(op, tmpReg, local.location);

    return EmitInfo(tmpReg, local.type);
}

EmitInfo
WasmBytecodeGenerator::EmitSetLocal(bool tee)
{
    uint localNum = GetReader()->m_currentNode.var.num;
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

    if (tee)
    {
        return info;
    }
    else
    {
        ReleaseLocation(&info);
        return EmitInfo();
    }
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
        m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, tmpReg, GetReader()->m_currentNode.cnst.f32);
        break;
    case WasmTypes::F64:
        m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, tmpReg, GetReader()->m_currentNode.cnst.f64);
        break;
    case WasmTypes::I32:
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, tmpReg, GetReader()->m_currentNode.cnst.i32);
        break;
    default:
        throw WasmCompilationException(_u("Unknown type %u"), type);
    }

    return EmitInfo(tmpReg, type);
}

EmitInfo
WasmBytecodeGenerator::EmitBlockCommon(bool* endOnElse /*= nullptr*/)
{
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

    return ExitEvalStackScope();
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
    // We don't want nested block to jump directly to the loop header
    // instead, jump to the landing pad and let it jump back to the loop header
    PushLabel(loopLandingPadLabel, false);
    EmitInfo loopInfo = EmitBlockCommon();

    // By default we don't loop, jump over the landing pad
    m_writer.AsmBr(loopTailLabel);
    m_writer.MarkAsmJsLabel(loopLandingPadLabel);
    m_writer.AsmBr(loopHeadLabel);
    m_writer.MarkAsmJsLabel(loopTailLabel);
    PopLabel(loopLandingPadLabel);
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
    bool isImportCall = GetReader()->m_currentNode.call.isImport;
    switch (wasmOp)
    {
    case wbCall:
        funcNum = GetReader()->m_currentNode.call.num;
        if (isImportCall)
        {
            if (funcNum >= m_module->GetImportCount())
            {
                throw WasmCompilationException(_u("Call is to unknown import"));
            }
            uint sigId = m_module->GetFunctionImport(funcNum)->sigId;
            calleeSignature = m_module->GetSignature(sigId);
        }
        else
        {
            if (funcNum >= m_module->GetFunctionCount())
            {
                throw WasmCompilationException(_u("Call is to unknown function"));
            }
            calleeSignature = m_module->GetFunctionInfo(funcNum)->GetSignature();
        }
        break;
    case wbCallIndirect:
        indirectIndexInfo = PopEvalStack();
        signatureId = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetSignature(signatureId);
        ReleaseLocation(&indirectIndexInfo);
        break;
    default:
        Assume(UNREACHED);
    }

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (isImportCall)
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

    uint32 nArgs = calleeSignature->GetParamCount();
    //copy args into a list so they could be generated in the right order (FIFO)
    JsUtil::List<EmitInfo, ArenaAllocator> argsList(&m_alloc);
    for (uint32 i = 0; i < nArgs; i++)
    {
        argsList.Add(PopEvalStack());
    }
    Assert((uint32)argsList.Count() == nArgs);

    // Size of the this pointer (aka undefined)
    int32 argsBytesLeft = sizeof(Js::Var);
    for (uint32 i = 0; i < nArgs; ++i)
    {
        EmitInfo info = argsList.Item(nArgs - i - 1);
        if (calleeSignature->GetParam(i) != info.type)
        {
            throw WasmCompilationException(_u("Call argument does not match formal type"));
        }

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        switch (info.type)
        {
        case WasmTypes::F32:
            if (isImportCall)
            {
                throw WasmCompilationException(_u("External calls with float argument NYI"));
            }
            argOp = Js::OpCodeAsmJs::I_ArgOut_Flt;
            break;
        case WasmTypes::F64:
            if (isImportCall)
            {
                argOp = Js::OpCodeAsmJs::ArgOut_Db;
            }
            else
            {
                argOp = Js::OpCodeAsmJs::I_ArgOut_Db;
            }
            break;
        case WasmTypes::I32:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
            break;
        default:
            throw WasmCompilationException(_u("Unknown argument type %u"), info.type);
        }
        //argSize

        if (argsBytesLeft < 0 || (argsBytesLeft % sizeof(Js::Var)) != 0)
        {
            throw WasmCompilationException(_u("Error while emitting call arguments"));
        }
        Js::RegSlot argLoc = argsBytesLeft / sizeof(Js::Var);
        argsBytesLeft += isImportCall ? sizeof(Js::Var) : calleeSignature->GetParamSize(i);

        m_writer.AsmReg2(argOp, argLoc, info.location);
    }

    //registers need to be released from higher ordinals to lower
    for (uint32 i = 0; i < nArgs; i++)
    {
        ReleaseLocation(&(argsList.Item(i)));
    }

    // emit call
    switch (wasmOp)
    {
    case wbCall:
        if (isImportCall)
        {
            m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->GetImportFuncOffset());
        }
        else
        {
            m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->GetFuncOffset());
        }
        break;
    case wbCallIndirect:
        if (indirectIndexInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(_u("Indirect call index must be int type"));
        }
        // todo:: Add bounds check. Asm.js doesn't need it because there has to be an & operator
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlotArr, 0, 1, calleeSignature->GetSignatureId() + m_module->GetIndirFuncTableOffset());
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdArr_Func, 0, 0, indirectIndexInfo.location);
        break;
    default:
        Assume(UNREACHED);
    }

    // calculate number of RegSlots the arguments consume
    Js::ArgSlot args;
    Js::OpCodeAsmJs callOp = Js::OpCodeAsmJs::Nop;
    if (isImportCall)
    {
        args = (Js::ArgSlot)(calleeSignature->GetParamCount() + 1);
        callOp = Js::OpCodeAsmJs::Call;
    }
    else
    {
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var))));
        callOp = Js::OpCodeAsmJs::I_Call;
    }

    m_writer.AsmCall(callOp, 0, 0, args, WasmToAsmJs::GetAsmJsReturnType(calleeSignature->GetResultType()));

    // emit result coercion
    EmitInfo retInfo;
    retInfo.type = calleeSignature->GetResultType();
    if (retInfo.type != WasmTypes::Void)
    {
        Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
        switch (retInfo.type)
        {
        case WasmTypes::F32:
            retInfo.location = m_f32RegSlots.AcquireTmpRegister();
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::I_Conv_VTF;
            break;
        case WasmTypes::F64:
            retInfo.location = m_f64RegSlots.AcquireTmpRegister();
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTD : Js::OpCodeAsmJs::I_Conv_VTD;
            break;
        case WasmTypes::I32:
            retInfo.location = m_i32RegSlots.AcquireTmpRegister();
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::I_Conv_VTI;
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
    EmitInfo checkExpr = PopEvalStack();

    if (checkExpr.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("If expression must have type i32"));
    }

    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    m_i32RegSlots.ReleaseLocation(&checkExpr);

    PushLabel(endLabel);

    bool endOnElse = false;
    EmitInfo trueExpr = EmitBlockCommon(&endOnElse);
    YieldToBlock(0, trueExpr);

    m_writer.AsmBr(endLabel);

    m_writer.MarkAsmJsLabel(falseLabel);

    EmitInfo retInfo;
    EmitInfo falseExpr;
    if (endOnElse)
    {
        falseExpr = EmitBlockCommon();
        YieldToBlock(0, falseExpr);
    }
    m_writer.MarkAsmJsLabel(endLabel);

    return PopLabel(endLabel);
}

EmitInfo
WasmBytecodeGenerator::EmitBrTable()
{
    const uint numTargets = GetReader()->m_currentNode.brTable.numTargets;
    const UINT* targetTable = GetReader()->m_currentNode.brTable.targetTable;
    const UINT defaultEntry = GetReader()->m_currentNode.brTable.defaultTarget;

    // Compile scrutinee
    EmitInfo scrutineeInfo = PopEvalStack();
    if (scrutineeInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"br_table expression must be of type I32");
    }

    m_writer.AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeInfo.location, scrutineeInfo.location);
    EmitInfo yieldInfo;
    if (ShouldYieldToBlock(defaultEntry))
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

EmitInfo
WasmBytecodeGenerator::EmitDrop()
{
    EmitInfo info = PopEvalStack();
    ReleaseLocation(&info);
    return EmitInfo();
}

template<Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature>
EmitInfo
WasmBytecodeGenerator::EmitBinExpr()
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType lhsType = signature[1];
    WasmTypes::WasmType rhsType = signature[2];

    EmitInfo rhs = PopEvalStack();
    EmitInfo lhs = PopEvalStack();

    if (rhs.type == WasmTypes::Unreachable)
    {
        if (lhs.type != WasmTypes::Unreachable)
        {
            ReleaseLocation(&lhs);
        }
        return rhs;
    }
    if (lhs.type == WasmTypes::Unreachable)
    {
        ReleaseLocation(&rhs);
        return lhs;
    }

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

template<Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature>
EmitInfo
WasmBytecodeGenerator::EmitUnaryExpr()
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType inputType = signature[1];

    EmitInfo info = PopEvalStack();

    if (info.type == WasmTypes::Unreachable)
    {
        return info;
    }

    if (inputType != info.type)
    {
        throw WasmCompilationException(_u("Invalid input type"));
    }

    GetRegisterSpace(inputType)->ReleaseLocation(&info);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer.AsmReg2(op, resultReg, info.location);

    return EmitInfo(resultReg, resultType);
}

template<WasmOp wasmOp, const WasmTypes::WasmType* signature>
EmitInfo
WasmBytecodeGenerator::EmitMemAccess(bool isStore)
{
    WasmTypes::WasmType type = signature[0];
    const uint offset = GetReader()->m_currentNode.mem.offset;
    GetFunctionBody()->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);

    EmitInfo rhsInfo;
    if (isStore)
    {
        rhsInfo = PopEvalStack();
    }
    EmitInfo exprInfo = PopEvalStack();

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("Index expression must be of type I32"));
    }
    if (offset != 0)
    {
        Js::RegSlot tempReg = m_i32RegSlots.AcquireTmpRegister();
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, tempReg, offset);

        m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Int, exprInfo.location, exprInfo.location, tempReg);
        m_i32RegSlots.ReleaseTmpRegister(tempReg);
    }

    if (isStore) // Stores
    {
        if (rhsInfo.type != type)
        {
            throw WasmCompilationException(_u("Invalid type for store op"));
        }
        m_writer.AsmTypedArr(Js::OpCodeAsmJs::StArrWasm, rhsInfo.location, exprInfo.location, GetViewType(wasmOp));
        ReleaseLocation(&rhsInfo);
        ReleaseLocation(&exprInfo);

        return EmitInfo();
    }

    ReleaseLocation(&exprInfo);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();
    m_writer.AsmTypedArr(Js::OpCodeAsmJs::LdArrWasm, resultReg, exprInfo.location, GetViewType(wasmOp));
    return EmitInfo(resultReg, type);
}

EmitInfo
WasmBytecodeGenerator::EmitReturnExpr()
{
    if (m_funcInfo->GetResultType() == WasmTypes::Void)
    {
        // TODO (michhol): consider moving off explicit 0 for return reg
        m_writer.AsmReg1(Js::OpCodeAsmJs::LdUndef, 0);
    }
    else
    {
        EmitInfo retExprInfo = PopEvalStack();
        if (retExprInfo.type == WasmTypes::Unreachable)
        {
            return retExprInfo;
        }

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
    UINT depth = GetReader()->m_currentNode.br.depth;

    EmitInfo conditionInfo;
    if (wasmOp == WasmOp::wbBrIf)
    {
        conditionInfo = PopEvalStack();
        if (conditionInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(_u("br_if condition must have I32 type"));
        }
    }

    EmitInfo info;
    if (ShouldYieldToBlock(depth))
    {
        // TODO: Handle value that Break is supposed to "throw".
        info = PopEvalStack();
    }
    YieldToBlock(depth, info);

    Js::ByteCodeLabel target = GetLabel(depth);
    if (wasmOp == WasmOp::wbBr)
    {
        m_writer.AsmBr(target);
    }
    else
    {
        Assert(wasmOp == WasmOp::wbBrIf);
        m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrTrue_Int, target, conditionInfo.location);
        m_i32RegSlots.ReleaseLocation(&conditionInfo);
    }

    ReleaseLocation(&info);
    return EmitInfo(WasmTypes::Unreachable);
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
    case wbI32LoadMem8S:
    case wbI32StoreMem8:
        return Js::ArrayBufferView::TYPE_INT8;
        break;
    case wbI32LoadMem8U:
        return Js::ArrayBufferView::TYPE_UINT8;
        break;
    case wbI32LoadMem16S:
    case wbI32StoreMem16:
        return Js::ArrayBufferView::TYPE_INT16;
        break;
    case wbI32LoadMem16U:
        return Js::ArrayBufferView::TYPE_UINT16;
        break;
    case wbF32LoadMem:
    case wbF32StoreMem:
        return Js::ArrayBufferView::TYPE_FLOAT32;
        break;
    case wbF64LoadMem:
    case wbF64StoreMem:
        return Js::ArrayBufferView::TYPE_FLOAT64;
        break;
    case wbI32LoadMem:
    case wbI32StoreMem:
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
        yieldEmitInfo = *info.yieldInfo;
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
        WasmTypes::WasmType type = GetReader()->m_currentNode.block.sig;
        if (type != WasmTypes::Void)
        {
            info.yieldInfo = Anew(&m_alloc, EmitInfo, GetRegisterSpace(type)->AcquireTmpRegister(), type);
        }
    }
    m_blockInfos.Push(info);
}

void
WasmBytecodeGenerator::YieldToBlock(uint relativeDepth, EmitInfo expr)
{
    BlockInfo info = GetBlockInfo(relativeDepth);
    if (info.HasYield())
    {
        EmitInfo* yieldInfo = info.yieldInfo;

        // Do not yield unrechable expressions
        if (expr.type == WasmTypes::Unreachable)
        {
            return;
        }

        if (yieldInfo->type != expr.type)
        {
            throw WasmCompilationException(_u("Invalid yield type"));
        }

        m_writer.AsmReg2(GetLoadOp(expr.type), yieldInfo->location, expr.location);
    }
}

bool WasmBytecodeGenerator::ShouldYieldToBlock(uint relativeDepth) const
{
    return GetBlockInfo(relativeDepth).HasYield();
}

Wasm::BlockInfo
WasmBytecodeGenerator::GetBlockInfo(uint relativeDepth) const
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
WasmBytecodeGenerator::GetRegisterSpace(WasmTypes::WasmType type)
{
    switch (type)
    {
    case WasmTypes::F32:
        return &m_f32RegSlots;
    case WasmTypes::F64:
        return &m_f64RegSlots;
    case WasmTypes::I32:
        return &m_i32RegSlots;
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

EmitInfo
WasmBytecodeGenerator::ExitEvalStackScope()
{
    Assert(!m_evalStack.Empty());
    EmitInfo info, prevInfo;
    while(info = m_evalStack.Pop(), info.type != WasmTypes::Limit)
    {
        prevInfo = info;
        ReleaseLocation(&info);
    }
    return prevInfo;
}

void
WasmCompilationException::FormatError(const char16* _msg, va_list arglist)
{
    char16 buf[2048];

    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, _msg, arglist);
    errorMsg = SysAllocString(buf);
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
