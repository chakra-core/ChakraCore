//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{
WasmBytecodeGenerator::WasmBytecodeGenerator(Js::ScriptContext * scriptContext, Js::Utf8SourceInfo * sourceInfo, BaseWasmReader * reader) :
    m_scriptContext(scriptContext),
    m_sourceInfo(sourceInfo),
    m_alloc(L"WasmBytecodeGen", scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_reader(reader)
{
    m_writer.Create();

    m_f32RegSlots = Anew(&m_alloc, WasmRegisterSpace, ReservedRegisterCount);
    m_f64RegSlots = Anew(&m_alloc, WasmRegisterSpace, ReservedRegisterCount);
    m_i32RegSlots = Anew(&m_alloc, WasmRegisterSpace, ReservedRegisterCount);

    // TODO (michhol): try to make this more accurate?
    const long astSize = 0;
    m_writer.InitData(&m_alloc, astSize);

    m_labels = Anew(&m_alloc, SListCounted<Js::ByteCodeLabel>, &m_alloc);

    // Initialize maps needed by binary reader
    Binary::WasmBinaryReader::Init(scriptContext);
}

WasmScript *
WasmBytecodeGenerator::GenerateWasmScript()
{
    // TODO: can this be in a better place?
     m_sourceInfo->EnsureInitialized(0);
     m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();

    // TODO: should scripts support multiple modules?

    m_wasmScript = Anew(&m_alloc, WasmScript);
    m_wasmScript->globalBody = Js::FunctionBody::NewFromRecycler(
        m_scriptContext,
        L"[Global WebAssembly Code]",
        19,
        0,
        0,
        m_sourceInfo,
        m_sourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
        0,
        nullptr,
        Js::FunctionInfo::Attributes::None
#ifdef PERF_COUNTERS
        , false /* is function from deferred deserialized proxy */
#endif
        );

    // TODO (michhol): numbering
    m_wasmScript->globalBody->SetSourceInfo(0);
    WasmOp op;
    while ((op = m_reader->ReadFromScript()) != wnLIMIT)
    {
        switch (op)
        {
        case wnMODULE:
            m_wasmScript->module = GenerateModule();
            break;
        case wnINVOKE:
            GenerateInvoke();
            break;
        default:
            Assert(UNREACHED);
        }
    }
    return m_wasmScript;
}

WasmModule *
WasmBytecodeGenerator::GenerateModule()
{
    m_module = Anew(&m_alloc, WasmModule);
    m_module->functions = Anew(&m_alloc, WasmFunctionArray, &m_alloc, 0);
    m_module->exports = Anew(&m_alloc, WasmExportDictionary, &m_alloc);

    m_module->heapOffset = 0;
    m_module->funcOffset = m_module->heapOffset + 1;

    WasmOp op;
    while ((op = m_reader->ReadFromModule()) != wnLIMIT)
    {
        switch (op)
        {
        case wnFUNC:
            if (m_reader->IsBinaryReader())
            {
                // export is part of function declaration for binary format
                if (m_reader->m_currentNode.func.info->Exported())
                {
                    if (m_module->exports->AddNew(m_reader->m_currentNode.func.info->GetName(), m_reader->m_currentNode.func.info->GetNumber()) == -1)
                    {
                        throw WasmCompilationException(L"Unable to export function");
                    }
                }
            }
            m_module->functions->Add(GenerateFunction());
            break;
        case wnIMPORT:
            m_module->functions->Add(InitializeImport());
            break;
        case wnEXPORT:
            AddExport();
            break;
        // TODO: implement below ops
        case wnGLOBAL:
        case wnTABLE:
        case wnMEMORY:
        case wnDATA:
        default:
            Assert(UNREACHED);
        }
    }
    m_module->memSize = m_module->funcOffset + m_module->functions->Count();

    return m_module;
}

WasmFunction *
WasmBytecodeGenerator::InitializeImport()
{
    m_func = Anew(&m_alloc, WasmFunction);
    m_func->wasmInfo = m_reader->m_currentNode.func.info;
    m_func->imported = true;
    return m_func;
}

WasmFunction *
WasmBytecodeGenerator::GenerateFunction()
{
    m_func = Anew(&m_alloc, WasmFunction);
    m_func->imported = false;
    m_func->body = Js::FunctionBody::NewFromRecycler(
        m_scriptContext,
        L"func",
        5,
        0,
        0,
        m_sourceInfo,
        m_sourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
        0,
        nullptr,
        Js::FunctionInfo::Attributes::None
#ifdef PERF_COUNTERS
        , false /* is function from deferred deserialized proxy */
#endif
        );
    // TODO (michhol): numbering
    m_func->body->SetSourceInfo(0);
    m_func->body->AllocateAsmJsFunctionInfo();
    m_func->body->SetIsAsmJsFunction(true);
    m_func->body->SetIsAsmjsMode(true);
    m_func->body->SetIsWasmFunction(true);
    m_funcInfo = m_reader->m_currentNode.func.info;
    m_func->wasmInfo = m_funcInfo;
    m_nestedIfLevel = 0;
    m_nestedCallDepth = 0;
    m_maxArgOutDepth = 0;
    m_argOutDepth = 0;

    // TODO: fix these bools
    m_writer.Begin(m_func->body, &m_alloc, false, true, false);

    EnregisterLocals();

    WasmOp op;
    EmitInfo exprInfo;
    while ((op = m_reader->ReadExpr()) != wnLIMIT)
    {
        exprInfo = EmitExpr(op);
        // REVIEW: should this be Assert or throw if false, or does this even hold?
        // Assert(exprInfo.type == WasmTypes::Void);
    }

    // Functions are like blocks. Emit implicit return of last stmt/expr, unless it is a return or end of file (sexpr).
    if (op != wnLIMIT && op != wnRETURN)
    {
        EmitReturnExpr(&exprInfo);
    }

    m_writer.End();

#if DBG_DUMP
    if (PHASE_DUMP(Js::ByteCodePhase, m_func->body))
    {
        Js::AsmJsByteCodeDumper::DumpBasic(m_func->body);
    }
#endif

    Js::AsmJsFunctionInfo * info = m_func->body->GetAsmJsFunctionInfo();
    if (m_funcInfo->GetParamCount() >= Js::Constants::InvalidArgSlot)
    {
        Js::Throw::OutOfMemory();
    }
    Js::ArgSlot paramCount = (Js::ArgSlot)m_funcInfo->GetParamCount();
    info->SetArgCount(paramCount);

    info->SetArgSizeArrayLength(max(paramCount, 3ui16));
    uint* argSizeArray = RecyclerNewArrayLeafZ(m_scriptContext->GetRecycler(), uint, paramCount);
    info->SetArgsSizesArray(argSizeArray);

    if (paramCount > 0)
    {
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
    info->SetFloatVarCount(ReservedRegisterCount);
    info->SetDoubleVarCount(ReservedRegisterCount);

    info->SetReturnType(GetAsmJsReturnType(m_funcInfo->GetResultType()));

    // REVIEW: overflow checks?
    info->SetIntByteOffset(ReservedRegisterCount * sizeof(Js::Var));
    info->SetFloatByteOffset(info->GetIntByteOffset() + m_i32RegSlots->GetRegisterCount() * sizeof(int32));
    info->SetDoubleByteOffset(Math::Align<int>(info->GetFloatByteOffset() + m_f32RegSlots->GetRegisterCount() * sizeof(float), sizeof(double)));

    m_func->body->SetOutParamDepth(m_maxArgOutDepth);
    m_func->body->SetVarCount(m_f32RegSlots->GetRegisterCount() + m_f64RegSlots->GetRegisterCount() + m_i32RegSlots->GetRegisterCount());
    return m_func;
}

void
WasmBytecodeGenerator::EnregisterLocals()
{
    m_locals = AnewArray(&m_alloc, WasmLocal, m_funcInfo->GetLocalCount());

    for (uint i = 0; i < m_funcInfo->GetLocalCount(); ++i)
    {
        WasmTypes::WasmType type = m_funcInfo->GetLocal(i);
        WasmRegisterSpace * regSpace = GetRegisterSpace(type);

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
                AssertMsg(UNREACHED, "Unimplemented");
                break;
            default:
                Assume(UNREACHED);
            }
        }
    }
}

EmitInfo
WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    switch (op)
    {
    case wnGETLOCAL:
        return EmitGetLocal();
    case wnSETLOCAL:
        return EmitSetLocal();
    case wnRETURN:
        return EmitReturnExpr();
    case wnCONST_F32:
        return EmitConst<WasmTypes::F32>();
    case wnCONST_F64:
        return EmitConst<WasmTypes::F64>();
    case wnCONST_I32:
        return EmitConst<WasmTypes::I32>();
    case wnBLOCK:
        return EmitBlock();
    case wnLOOP:
        return EmitLoop();
    case wnCALL:
        return EmitCall();
    case wnIF:
        return EmitIfExpr();
    case wnIF_ELSE:
        return EmitIfElseExpr();
    case wnBREAK:
        return EmitBreak();
    case wnSWITCH:
        return EmitSwitch();
    case wnNOP:
        return EmitInfo();
#define WASM_KEYWORD_BIN_TYPED(token, name, op, resultType, lhsType, rhsType) \
    case wn##token: \
        return EmitBinExpr<Js::OpCodeAsmJs::##op, WasmTypes::##resultType, WasmTypes::##lhsType, WasmTypes::##rhsType>();

#include "WasmKeywords.h"

    default:
        Assert(UNREACHED);
    }
    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitGetLocal()
{
    if (m_funcInfo->GetLocalCount() < m_reader->m_currentNode.var.num)
    {
        throw WasmCompilationException(L"%u is not a valid local", m_reader->m_currentNode.var.num);
    }

    WasmLocal local = m_locals[m_reader->m_currentNode.var.num];

Js::OpCodeAsmJs op = Js::OpCodeAsmJs::Nop;
switch (local.type)
{
case WasmTypes::F32:
    op = Js::OpCodeAsmJs::Ld_Flt;
    break;
case WasmTypes::F64:
    op = Js::OpCodeAsmJs::Ld_Db;
    break;
case WasmTypes::I32:
    op = Js::OpCodeAsmJs::Ld_Int;
    break;
default:
    Assume(UNREACHED);
}

WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);

Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

m_writer.AsmReg2(op, tmpReg, local.location);

return EmitInfo(tmpReg, local.type);
}

EmitInfo
WasmBytecodeGenerator::EmitSetLocal()
{
    if (m_funcInfo->GetLocalCount() < m_reader->m_currentNode.var.num)
    {
        throw WasmCompilationException(L"%u is not a valid local", m_reader->m_currentNode.var.num);
    }

    WasmLocal local = m_locals[m_reader->m_currentNode.var.num];

    Js::OpCodeAsmJs op = Js::OpCodeAsmJs::Nop;
    WasmRegisterSpace * regSpace = nullptr;
    switch (local.type)
    {
    case WasmTypes::F32:
        op = Js::OpCodeAsmJs::Ld_Flt;
        regSpace = m_f32RegSlots;
        break;
    case WasmTypes::F64:
        op = Js::OpCodeAsmJs::Ld_Db;
        regSpace = m_f64RegSlots;
        break;
    case WasmTypes::I32:
        op = Js::OpCodeAsmJs::Ld_Int;
        regSpace = m_i32RegSlots;
        break;
    default:
        Assume(UNREACHED);
    }

    EmitInfo info = EmitExpr(m_reader->ReadExpr());

    if (info.type != local.type)
    {
        throw WasmCompilationException(L"TypeError in setlocal for %u", m_reader->m_currentNode.var.num);
    }

    m_writer.AsmReg2(op, local.location, info.location);

    regSpace->ReleaseLocation(&info);

    // REVIEW: should this produce result of setlocal? currently produces void
    return EmitInfo();
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
        m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, tmpReg, m_reader->m_currentNode.cnst.f32);
        break;
    case WasmTypes::F64:
        m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, tmpReg, m_reader->m_currentNode.cnst.f64);
        break;
    case WasmTypes::I32:
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, tmpReg, m_reader->m_currentNode.cnst.i32);
        break;
    default:
        Assume(UNREACHED);
    }

    return EmitInfo(tmpReg, type);
}

EmitInfo
WasmBytecodeGenerator::EmitBlock()
{
    WasmOp op;
    Js::ByteCodeLabel blockLabel = m_writer.DefineLabel();
    m_labels->Push(blockLabel);
    if (m_reader->IsBinaryReader())
    {
        UINT blockCount = m_reader->m_currentNode.block.count;
        if (blockCount <= 0)
        {
            throw WasmCompilationException(L"Invalid block node count");
        }
        for (UINT i = 0; i < blockCount; i++)
        {
            op = m_reader->ReadFromBlock();
            EmitInfo info = EmitExpr(op);
        }
    }
    else
    {
        op = m_reader->ReadFromBlock();
        if (op == wnLIMIT)
        {
            throw WasmCompilationException(L"Block must have at least one expression");
        }
        do
        {
            EmitInfo info = EmitExpr(op);
            // REVIEW: should this be Assert or throw if false, or does this even hold?
            Assert(info.type == WasmTypes::Void);
            op = m_reader->ReadFromBlock();
        } while (op != wnLIMIT);
    }
    m_writer.MarkAsmJsLabel(blockLabel);
    m_labels->Pop();
    // REVIEW: can a block give a result?
    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitLoop()
{
    WasmOp op;
    Js::ByteCodeLabel loopHeaderLabel = m_writer.DefineLabel();
    m_labels->Push(loopHeaderLabel);
    m_writer.MarkAsmJsLabel(loopHeaderLabel);
    if (m_reader->IsBinaryReader())
    {
        UINT blockCount = m_reader->m_currentNode.block.count;
        if (blockCount <= 0)
        {
            throw WasmCompilationException(L"Invalid block node count");
        }
        for (UINT i = 0; i < blockCount; i++)
        {
            op = m_reader->ReadFromBlock();
            EmitInfo info = EmitExpr(op);
        }
    }
    m_labels->Pop();
    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitCall()
{
    ++m_nestedCallDepth;

    uint funcNum = m_reader->m_currentNode.var.num;
    if (funcNum >= m_module->functions->Count())
    {
        // TODO: implement forward calls
        AssertMsg(UNREACHED, "Forward calls currently unsupported");
    }

    WasmFunction * callee = m_module->functions->GetBuffer()[funcNum];

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (callee->imported)
    {
        // TODO: michhol set upper limit on param count to prevent overflow
        argSize = (Js::ArgSlot)(callee->wasmInfo->GetParamCount() * sizeof(Js::Var));
        startCallOp = Js::OpCodeAsmJs::StartCall;
    }
    else
    {
        argSize = callee->body->GetAsmJsFunctionInfo()->GetArgByteSize();
        startCallOp = Js::OpCodeAsmJs::I_StartCall;
    }

    m_writer.AsmStartCall(startCallOp, argSize + sizeof(void*));

    WasmOp op;
    uint i = 0;
    Js::RegSlot nextLoc = 1;

    uint maxDepthForLevel = m_argOutDepth;
    while (i < callee->wasmInfo->GetParamCount() && (op = m_reader->ReadFromCall()) != wnLIMIT)
    {
        // emit args
        EmitInfo info = EmitExpr(op);
        if (callee->wasmInfo->GetParam(i) != info.type)
        {
            throw WasmCompilationException(L"Call argument does not match formal type");
        }

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        Js::RegSlot argLoc = nextLoc;
        switch (info.type)
        {
        case WasmTypes::F32:
            // REVIEW: support FFI call with f32 params?
            Assert(!callee->imported);
            argOp = Js::OpCodeAsmJs::I_ArgOut_Flt;
            ++nextLoc;
            break;
        case WasmTypes::F64:
            if (callee->imported)
            {
                argOp = Js::OpCodeAsmJs::ArgOut_Db;
                ++nextLoc;
            }
            else
            {
                argOp = Js::OpCodeAsmJs::I_ArgOut_Db;
                // this indexes into physical stack, so on x86 we need double width
                nextLoc += sizeof(double) / sizeof(Js::Var);
            }
            break;
        case WasmTypes::I32:
            argOp = callee->imported ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
            ++nextLoc;
            break;
        default:
            Assume(UNREACHED);
        }

        m_writer.AsmReg2(argOp, argLoc, info.location);
        GetRegisterSpace(info.type)->ReleaseLocation(&info);
        // if there are nested calls, track whichever is the deepest
        if (maxDepthForLevel < m_argOutDepth)
        {
            maxDepthForLevel = m_argOutDepth;
        }

        ++i;
    }

    if (!m_reader->IsBinaryReader()) {
        // [b-gekua] REVIEW: SExpr must consume RPAREN for call whereas Binary
        // does not. This and other kinds of special casing for BinaryReader
        // may be eliminated with a refactoring of WasmBinaryReader to use the
        // same "protocol" for scoping constructs as SExprParser (i.e., signal
        // the end of scope with wnLIMIT).
        m_reader->ReadFromCall();
    }

    if (i != callee->wasmInfo->GetParamCount())
    {
        throw WasmCompilationException(L"Call has wrong number of arguments");
    }

    // emit call

    m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->funcOffset);

    // calculate number of RegSlots the arguments consume
    Js::ArgSlot args;
    if (callee->imported)
    {
        args = (Js::ArgSlot)(callee->wasmInfo->GetParamCount() + 1);
    }
    else
    {
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var)))) + 1;
    }
    Js::OpCodeAsmJs callOp = callee->imported ? Js::OpCodeAsmJs::Call : Js::OpCodeAsmJs::I_Call;
    m_writer.AsmCall(callOp, 0, 0, args, GetAsmJsReturnType(callee->wasmInfo->GetResultType()));

    // emit result coercion
    EmitInfo retInfo;
    retInfo.type = callee->wasmInfo->GetResultType();
    Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
    switch (retInfo.type)
    {
    case WasmTypes::F32:
        Assert(!callee->imported);
        retInfo.location = m_f32RegSlots->AcquireTmpRegister();
        convertOp = Js::OpCodeAsmJs::I_Conv_VTF;
        break;
    case WasmTypes::F64:
        retInfo.location = m_f64RegSlots->AcquireTmpRegister();
        convertOp = callee->imported ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::I_Conv_VTF;
        break;
    case WasmTypes::I32:
        retInfo.location = m_i32RegSlots->AcquireTmpRegister();
        convertOp = callee->imported ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::I_Conv_VTI;
        break;
    case WasmTypes::I64:
        Assert(UNREACHED);
        break;
    default:
        Assume(UNREACHED);
    }
    m_writer.AsmReg2(convertOp, retInfo.location, 0);


    // track stack requirements for out params

    // + 1 for return address
    maxDepthForLevel += args + 1;
    if (m_nestedCallDepth > 1)
    {
        m_argOutDepth = maxDepthForLevel;
    }
    else
    {
        m_argOutDepth = 0;
    }
    if (maxDepthForLevel > m_maxArgOutDepth)
    {
        m_maxArgOutDepth = maxDepthForLevel;
    }

    Assert(m_nestedCallDepth > 0);
    --m_nestedCallDepth;

    return retInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitIfExpr()
{
    ++m_nestedIfLevel;

    if (m_nestedIfLevel == 0)
    {
        // overflow
        Js::Throw::OutOfMemory();
    }

    EmitInfo checkExpr = EmitExpr(m_reader->ReadExpr());

    if (checkExpr.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"If expression must have type i32");
    }

    // TODO: save this so I can break
    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    GetRegisterSpace(checkExpr.type)->ReleaseLocation(&checkExpr);

    EmitInfo innerExpr = EmitExpr(m_reader->ReadExpr());

    if (innerExpr.type != WasmTypes::Void)
    {
        throw WasmCompilationException(L"Result of if must be void");
    }

    m_writer.MarkAsmJsLabel(falseLabel);

    Assert(m_nestedIfLevel > 0);
    --m_nestedIfLevel;

    return EmitInfo();
}

// todo: combine with emit:if
EmitInfo
WasmBytecodeGenerator::EmitIfElseExpr()
{
    ++m_nestedIfLevel;

    if (m_nestedIfLevel == 0)
    {
        // overflow
        Js::Throw::OutOfMemory();
    }

    EmitInfo checkExpr = EmitExpr(m_reader->ReadExpr());

    if (checkExpr.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"If expression must have type i32");
    }

    // TODO: save this so I can break
    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    GetRegisterSpace(checkExpr.type)->ReleaseLocation(&checkExpr);

    EmitInfo trueExpr = EmitExpr(m_reader->ReadExpr());

    m_writer.AsmBr(endLabel);

    m_writer.MarkAsmJsLabel(falseLabel);

    EmitInfo falseExpr = EmitExpr(m_reader->ReadExpr());

    if (falseExpr.type != trueExpr.type)
    {
        throw WasmCompilationException(L"If/Else branches must have same result type");
    }

    // we will use result info true expr as our result info
    // review: how will this work with break? must we store this location?
    if (falseExpr.type != WasmTypes::Void && falseExpr.location != trueExpr.location)
    {
        Js::OpCodeAsmJs op = Js::OpCodeAsmJs::Nop;
        switch (falseExpr.type)
        {
        case WasmTypes::F32:
            op = Js::OpCodeAsmJs::Ld_Flt;
            break;
        case WasmTypes::F64:
            op = Js::OpCodeAsmJs::Ld_Db;
            break;
        case WasmTypes::I32:
            op = Js::OpCodeAsmJs::Ld_Int;
            break;
        case WasmTypes::I64:
            AssertMsg(UNREACHED, "NYI");
            break;
        default:
            Assume(UNREACHED);
        }
        m_writer.AsmReg2(op, trueExpr.location, falseExpr.location);
        GetRegisterSpace(falseExpr.type)->ReleaseLocation(&falseExpr);
    }

    m_writer.MarkAsmJsLabel(endLabel);

    Assert(m_nestedIfLevel > 0);
    --m_nestedIfLevel;

    return trueExpr;
}

EmitInfo
WasmBytecodeGenerator::EmitSwitch()
{
    Js::ByteCodeLabel defaultLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel breakLabel = m_writer.DefineLabel();
    uint numEntries = m_reader->m_currentNode.tableswitch.numEntries;
    uint numCases = m_reader->m_currentNode.tableswitch.numCases;
    UINT16* jumpTable = m_reader->m_currentNode.tableswitch.jumpTable;
    Js::ByteCodeLabel* labels = AnewArray(&m_alloc, Js::ByteCodeLabel, numEntries);

    Assert((numCases == numEntries) || (numCases + 1 == numEntries));

    m_labels->Push(breakLabel);
    // Compile scrutinee
    WasmOp op = m_reader->ReadFromBlock();
    EmitInfo scrutineeInfo = EmitExpr(op);

    WasmRegisterSpace* regSpace = GetRegisterSpace(WasmTypes::I32);
    Js::RegSlot scrutineeVal = regSpace->AcquireTmpRegister();
    m_writer.AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeVal, scrutineeInfo.location);
    // Compile cases
    for (uint i = 0; i < numEntries; i++)
    {
        Js::RegSlot caseLoc = regSpace->AcquireTmpRegister();
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, caseLoc, jumpTable[i]);
        Js::ByteCodeLabel label;
        if (i == numEntries - 1) {
            label = defaultLabel;
        }
        else
        {
            label = m_writer.DefineLabel();
        }
        labels[i] = label;

        m_writer.AsmBrReg2(Js::OpCodeAsmJs::Case_Int, label, scrutineeVal, caseLoc);
        regSpace->ReleaseTmpRegister(caseLoc);
    }
    regSpace->ReleaseTmpRegister(scrutineeVal);
    m_writer.AsmBr(defaultLabel, Js::OpCodeAsmJs::EndSwitch_Int);

    for (uint i = 0; i < numEntries; i++)
    {
        m_writer.MarkAsmJsLabel(labels[i]);
        WasmOp op = m_reader->ReadFromBlock();
        EmitInfo info = EmitExpr(op);
    }

    m_labels->Pop();
    m_writer.MarkAsmJsLabel(breakLabel);
    return EmitInfo();
}

template<Js::OpCodeAsmJs op, WasmTypes::WasmType resultType, WasmTypes::WasmType lhsType, WasmTypes::WasmType rhsType>
EmitInfo
WasmBytecodeGenerator::EmitBinExpr()
{
    EmitInfo lhs = EmitExpr(m_reader->ReadExpr());
    EmitInfo rhs = EmitExpr(m_reader->ReadExpr());

    if (lhsType != lhs.type)
    {
        throw WasmCompilationException(L"Invalid type for LHS");
    }
    if (rhsType != lhs.type)
    {
        throw WasmCompilationException(L"Invalid type for RHS");
    }

    WasmRegisterSpace * regSpace = GetRegisterSpace(resultType);

    Js::RegSlot resultReg = regSpace->AcquireRegisterAndReleaseLocations(&lhs, &rhs);

    m_writer.AsmReg3(op, resultReg, lhs.location, rhs.location);

    return EmitInfo(resultReg, resultType);
}

template<typename T>
Js::RegSlot
WasmBytecodeGenerator::GetConstReg(T constVal)
{
    Js::RegSlot location = m_funcInfo->GetConst(constVal);
    if (location == Js::Constants::NoRegister)
    {
        WasmRegisterSpace * regSpace = GetRegisterSpace(m_reader->m_currentNode.type);
        location = regSpace->AcquireConstRegister();
        m_funcInfo->AddConst(constVal, location);
    }
    return location;
}

EmitInfo
WasmBytecodeGenerator::EmitReturnExpr(EmitInfo *lastStmtExprInfo)
{
    bool hasNonVoidExpr = (lastStmtExprInfo && lastStmtExprInfo->type != WasmTypes::Void)|| m_reader->m_currentNode.opt.exists;
    bool explicitReturn = (lastStmtExprInfo == nullptr);
    if (hasNonVoidExpr)
    {
        if (m_funcInfo->GetResultType() == WasmTypes::Void)
        {
            throw WasmCompilationException(L"Void result type cannot return expression");
        }

        EmitInfo retExprInfo;
        if (explicitReturn)
        {
            Assert(m_reader->m_currentNode.opt.exists); // explicit expression must exist
            retExprInfo = EmitExpr(m_reader->ReadExpr());
        }
        else
        {
            Assert(lastStmtExprInfo && lastStmtExprInfo->type != WasmTypes::Void);
            retExprInfo = *lastStmtExprInfo;
        }

        if (m_funcInfo->GetResultType() != retExprInfo.type)
        {
            throw WasmCompilationException(L"Result type must match return type");
        }

        Js::OpCodeAsmJs retOp = Js::OpCodeAsmJs::Nop;
        switch (retExprInfo.type)
        {
        case WasmTypes::F32:
            retOp = Js::OpCodeAsmJs::Return_Flt;
            m_func->body->GetAsmJsFunctionInfo()->SetReturnType(Js::AsmJsRetType::Float);
            break;
        case WasmTypes::F64:
            retOp = Js::OpCodeAsmJs::Return_Db;
            m_func->body->GetAsmJsFunctionInfo()->SetReturnType(Js::AsmJsRetType::Double);
            break;
        case WasmTypes::I32:
            retOp = Js::OpCodeAsmJs::Return_Int;
            m_func->body->GetAsmJsFunctionInfo()->SetReturnType(Js::AsmJsRetType::Signed);
            break;
        default:
            Assume(UNREACHED);
        }

        m_writer.Conv(retOp, 0, retExprInfo.location);
        GetRegisterSpace(retExprInfo.type)->ReleaseLocation(&retExprInfo);
    }
    else
    {
        // void expression
        if (m_funcInfo->GetResultType() != WasmTypes::Void)
        {
            throw WasmCompilationException(L"Non-void result type must have return expression");
        }

        // TODO (michhol): consider moving off explicit 0 for return reg
        m_writer.AsmReg1(Js::OpCodeAsmJs::LdUndef, 0);
    }

    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitBreak()
{
    uint8 depth = m_reader->m_currentNode.br.depth;
    if (depth >= m_labels->Count())
        Assert(UNREACHED);

    // TODO: Handle value that Break is supposed to "throw".
    WasmOp op = m_reader->ReadFromBlock();
    EmitInfo info = EmitExpr(op);

    SListCounted<Js::ByteCodeLabel>::Iterator itr(m_labels);
    itr.Next();
    for (int i = 0; i < depth; i++) {
        itr.Next();
    }
    Js::ByteCodeLabel target = itr.Data();
    m_writer.AsmBr(target);
    return EmitInfo();
}

/* static */
Js::AsmJsRetType
WasmBytecodeGenerator::GetAsmJsReturnType(WasmTypes::WasmType wasmType)
{
    Js::AsmJsRetType asmType = Js::AsmJsRetType::Void;
    switch (wasmType)
    {
    case WasmTypes::F32:
        asmType = Js::AsmJsRetType::Float;
        break;
    case WasmTypes::F64:
        asmType = Js::AsmJsRetType::Double;
        break;
    case WasmTypes::I32:
        asmType = Js::AsmJsRetType::Signed;
        break;
    case WasmTypes::Void:
        asmType = Js::AsmJsRetType::Void;
        break;
    default:
        Assert(UNREACHED);
    }
    return asmType;
}

/* static */
Js::AsmJsVarType
WasmBytecodeGenerator::GetAsmJsVarType(WasmTypes::WasmType wasmType)
{
    Js::AsmJsVarType asmType = Js::AsmJsVarType::Int;
    switch (wasmType)
    {
    case WasmTypes::F32:
        asmType = Js::AsmJsVarType::Float;
        break;
    case WasmTypes::F64:
        asmType = Js::AsmJsVarType::Double;
        break;
    case WasmTypes::I32:
        asmType = Js::AsmJsVarType::Int;
        break;
    default:
        Assert(UNREACHED);
    }
    return asmType;
}

void
WasmBytecodeGenerator::GenerateInvoke()
{
    /*
    // TODO: this only works for single invoke

    // TODO: fix these bools
    m_writer.Begin(m_wasmScript->globalBody, &m_alloc, false, true, false);

    // TODO: loop through all arguments, not just 1, get arg count from parser
    EmitInfo argInfo = EmitExpr(invokeNode->invk.expr);

    const Js::ArgSlot argByteSize = sizeof(int32) + sizeof(Js::Var);
    m_writer.AsmStartCall(Js::OpCodeAsmJs::I_StartCall, argByteSize);
    m_writer.AsmReg2(Js::OpCodeAsmJs::ArgOut_Int, 1, argInfo.location);
    //m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, index + mCompiler->GetFuncOffset());
    m_writer.AsmCall(Js::OpCodeAsmJs::I_Call, 0, 0, (Js::ArgSlot)(::ceil((double)(argByteSize / sizeof(Js::Var)))) + 1, Js::AsmJsRetType::Signed);

    m_writer.End();
    */
}

void WasmBytecodeGenerator::AddExport()
{
    if (m_reader->m_currentNode.var.num > m_module->functions->Count())
    {
        throw WasmCompilationException(L"Invalid index for export");
    }
    if (m_module->exports->AddNew(m_reader->m_currentNode.var.exportName, m_reader->m_currentNode.var.num) == -1)
    {
        throw WasmCompilationException(L"Unable to export function");
    }
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
    default:
        return nullptr;
    }
}

WasmCompilationException::WasmCompilationException(const wchar_t* _msg, ...)
{
    Assert(UNREACHED); // TODO (michhol)
    va_list arglist;
    va_start(arglist, _msg);
    //vswprintf_s(msg_, _msg, arglist);
}

} // namespace Wasm

#endif // ENABLE_WASM
