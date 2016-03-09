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
typedef ProcessSectionResult(*SectionProcessFunc)(WasmBytecodeGenerator*, SectionCode);

WasmModule *
WasmBytecodeGenerator::GenerateModule()
{
    // TODO: can this be in a better place?
    m_sourceInfo->EnsureInitialized(0);
    m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();

    m_module = Anew(&m_alloc, WasmModule);
    m_module->functions = Anew(&m_alloc, WasmFunctionArray, &m_alloc, 0);
    m_module->info = m_reader->m_moduleInfo;
    m_module->heapOffset = 0;
    m_module->funcOffset = m_module->heapOffset + 1;

    m_reader->InitializeReader();

    BVFixed* visitedSections = BVFixed::New(bSectLimit + 1, &m_alloc);

    const auto readerProcess = [](WasmBytecodeGenerator* gen, SectionCode code) {return gen->m_reader->ProcessSection(code); };
    // By default lest the reader process the section
#define WASM_SECTION(name, id, flag, precedent) readerProcess,
    SectionProcessFunc sectionProcess[bSectLimit + 1] = {
#include "WasmSections.h"
        nullptr
    };

    sectionProcess[bSectFunctionSignatures] = [](WasmBytecodeGenerator* gen, SectionCode code) {
        Assert(code == bSectFunctionSignatures);
        if (gen->m_reader->ProcessSection(code) != psrEnd)
        {
            return psrInvalid;
        }
        gen->m_module->indirFuncTableOffset = gen->m_module->funcOffset + gen->m_module->info->GetFunctionCount();
        return psrEnd;
    };

    sectionProcess[bSectImportTable] = [](WasmBytecodeGenerator* gen, SectionCode code) {
        Assert(code == bSectImportTable);
        // todo::
        return psrInvalid;
    };

    sectionProcess[bSectFunctionBodies] = [](WasmBytecodeGenerator* gen, SectionCode code) {
        Assert(code == bSectFunctionBodies);
        bool isEntry = true;
        ProcessSectionResult lastRes;
        while ((lastRes = gen->m_reader->ProcessSection(code, isEntry)) == psrContinue)
        {
            isEntry = false;
            gen->m_module->functions->Add(gen->GenerateFunction());
        }
        return lastRes;
    };

    for (uint32 sectionCode = 0; sectionCode < bSectLimit ; sectionCode++)
    {
        SectionCode precedent = SectionInfo::All[sectionCode].precedent;
        if (m_reader->ReadNextSection((SectionCode)sectionCode))
        {
            if (precedent != bSectInvalid && !visitedSections->Test(precedent))
            {
                throw WasmCompilationException(L"%s section missing before %s",
                                               SectionInfo::All[precedent].name,
                                               SectionInfo::All[sectionCode].name);
            }
            visitedSections->Set(sectionCode);

            if (sectionProcess[sectionCode](this, (SectionCode)sectionCode) == psrInvalid)
            {
                throw WasmCompilationException(L"Error while reading section %s", SectionInfo::All[sectionCode].name);
            }
        }
    }

    // If we see a FunctionSignatures section we need to see a FunctionBodies section
    if (visitedSections->Test(bSectFunctionSignatures) && !visitedSections->Test(bSectFunctionBodies))
    {
        throw WasmCompilationException(L"Missing required section: %s", SectionInfo::All[bSectFunctionBodies].name);
    }
    // reserve space for as many function tables as there are signatures, though we won't fill them all
    m_module->memSize = m_module->funcOffset + m_module->info->GetFunctionCount() + m_module->info->GetSignatureCount();

    return m_module;
}

WasmFunction *
WasmBytecodeGenerator::InitializeImport()
{
    m_func = Anew(&m_alloc, WasmFunction);
    m_func->wasmInfo = m_reader->m_currentNode.func.info;
    return m_func;
}

WasmFunction *
WasmBytecodeGenerator::GenerateFunction()
{
    m_func = Anew(&m_alloc, WasmFunction);
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

    m_funcInfo->SetExitLabel(m_writer.DefineLabel());
    EnregisterLocals();

    WasmOp op;
    EmitInfo exprInfo;
    while ((op = m_reader->ReadExpr()) != wnLIMIT)
    {
        exprInfo = EmitExpr(op);
        ReleaseLocation(&exprInfo);
    }

    // Functions are like blocks. Emit implicit return of last stmt/expr, unless it is a return or end of file (sexpr).
    if (op != wnLIMIT && op != wnRETURN)
    {
        EmitReturnExpr(&exprInfo);
    }
    m_writer.MarkAsmJsLabel(m_funcInfo->GetExitLabel());
    m_writer.EmptyAsm(Js::OpCodeAsmJs::Ret);

    m_writer.End();

#if DBG_DUMP
    if (PHASE_DUMP(Js::ByteCodePhase, m_func->body))
    {
        Js::AsmJsByteCodeDumper::DumpBasic(m_func->body);
    }
#endif

    // TODO: refactor out to separate procedure
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

    if (m_module->memSize > 0)
    {
        info->SetUsesHeapBuffer(true);
    }
    if (paramCount > 0)
    {
        m_func->body->SetHasImplicitArgIns(true);
        m_func->body->SetInParamsCount(paramCount + 1);
        m_func->body->SetReportedInParamsCount(paramCount + 1);
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

void
WasmBytecodeGenerator::PrintOpName(WasmOp op)
{
#define MakeWide(x) L##x
    switch (op)
    {
#define WASM_KEYWORD(token, name) \
    case wn##token: \
        Output::Print(MakeWide(#token ## "\r\n")); \
        break;
#include "WasmKeywords.h"
    }
#undef MakeWide
}

EmitInfo
WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
#if DBG_DUMP
    if (PHASE_TRACE(Js::WasmReaderPhase, m_func->body))
    {
        PrintOpName(op);
    }
#endif

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
        return EmitCall<wnCALL>();
    case wnCALL_INDIRECT:
        return EmitCall<wnCALL_INDIRECT>();
    case wnIF:
        return EmitIfExpr();
    case wnIF_ELSE:
        return EmitIfElseExpr();
    case wnBR:
        return EmitBr<wnBR>();
    case wnBR_IF:
        return EmitBr<wnBR_IF>();
    case wnSELECT:
        return EmitSelect();
    case wnBR_TABLE:
        return EmitBrTable();
    case wnNOP:
        return EmitInfo();
#define WASM_MEMREAD(token, name, type) \
    case wn##token: \
        return EmitMemRead<wn##token, WasmTypes::##type>();
#define WASM_MEMSTORE(token, name, type) \
    case wn##token: \
        return EmitMemStore<wn##token, WasmTypes::##type>();
#define WASM_KEYWORD_BIN_TYPED(token, name, op, resultType, lhsType, rhsType) \
    case wn##token: \
        return EmitBinExpr<Js::OpCodeAsmJs::##op, WasmTypes::##resultType, WasmTypes::##lhsType, WasmTypes::##rhsType>();

#define WASM_KEYWORD_UNARY(token, name, op, resultType, inputType) \
    case wn##token: \
        return EmitUnaryExpr<Js::OpCodeAsmJs::##op, WasmTypes::##resultType, WasmTypes::##inputType>();

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

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    m_writer.AsmReg2(op, tmpReg, local.location);

    return EmitInfo(tmpReg, local.type);
}

EmitInfo
WasmBytecodeGenerator::EmitSetLocal()
{
    uint localNum = m_reader->m_currentNode.var.num;
    if (localNum >= m_funcInfo->GetLocalCount())
    {
        throw WasmCompilationException(L"%u is not a valid local", localNum);
    }

    WasmLocal local = m_locals[localNum];

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);
    EmitInfo info = EmitExpr(m_reader->ReadExpr());

    if (info.type != local.type)
    {
        throw WasmCompilationException(L"TypeError in setlocal for %u", localNum);
    }

    m_writer.AsmReg2(op, local.location, info.location);

    regSpace->ReleaseLocation(&info);

    Js::RegSlot tmp = regSpace->AcquireTmpRegister();
    m_writer.AsmReg2(op, tmp, local.location);

    return EmitInfo(tmp, local.type);
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
            ReleaseLocation(&info);
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
            ReleaseLocation(&info);
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

    Js::ByteCodeLabel loopTailLabel = m_writer.DefineLabel();
    m_labels->Push(loopTailLabel);

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
            ReleaseLocation(&info);
        }
    }
    m_writer.MarkAsmJsLabel(loopTailLabel);
    m_labels->Pop();
    m_labels->Pop();
    return EmitInfo();
}

template<WasmOp wasmOp>
EmitInfo
WasmBytecodeGenerator::EmitCall()
{
    ++m_nestedCallDepth;

    uint funcNum = Js::Constants::UninitializedValue;
    uint signatureId = Js::Constants::UninitializedValue;
    bool imported;
    WasmSignature * calleeSignature;
    if (wasmOp == wnCALL)
    {
        funcNum = m_reader->m_currentNode.var.num;
        if (funcNum >= m_module->info->GetFunctionCount())
        {
            throw WasmCompilationException(L"Call is to unknown function");
        }
        calleeSignature = m_module->info->GetFunSig(funcNum)->GetSignature();
        imported = m_module->info->GetFunSig(funcNum)->Imported();
    }
    else
    {
        signatureId = m_reader->m_currentNode.var.num;
        calleeSignature = m_module->info->GetSignature(signatureId);
        imported = false;
    }

    EmitInfo indirectIndexInfo;
    if (wasmOp == wnCALL_INDIRECT)
    {
        WasmOp indexOp = m_reader->ReadFromCall();
        indirectIndexInfo = EmitExpr(indexOp);
        if (indirectIndexInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(L"Indirect call index must be int type");
        }
    }

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (imported)
    {
        // TODO: michhol set upper limit on param count to prevent overflow
        argSize = (Js::ArgSlot)(calleeSignature->GetParamCount() * sizeof(Js::Var));
        startCallOp = Js::OpCodeAsmJs::StartCall;
    }
    else
    {
        if (calleeSignature->GetParamSize() >= UINT16_MAX)
        {
            throw WasmCompilationException(L"Argument size too big");
        }
        argSize = (Js::ArgSlot)calleeSignature->GetParamSize();

        startCallOp = Js::OpCodeAsmJs::I_StartCall;
    }

    m_writer.AsmStartCall(startCallOp, argSize + sizeof(void*));

    WasmOp op;
    uint i = 0;
    Js::RegSlot nextLoc = 1;

    uint maxDepthForLevel = m_argOutDepth;
    while (i < calleeSignature->GetParamCount() && (op = m_reader->ReadFromCall()) != wnLIMIT)
    {
        // emit args
        EmitInfo info = EmitExpr(op);
        if (calleeSignature->GetParam(i) != info.type)
        {
            throw WasmCompilationException(L"Call argument does not match formal type");
        }

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        Js::RegSlot argLoc = nextLoc;
        switch (info.type)
        {
        case WasmTypes::F32:
            // REVIEW: support FFI call with f32 params?
            Assert(!imported);
            argOp = Js::OpCodeAsmJs::I_ArgOut_Flt;
            ++nextLoc;
            break;
        case WasmTypes::F64:
            if (imported)
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
            argOp = imported ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
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

    if (i != calleeSignature->GetParamCount())
    {
        throw WasmCompilationException(L"Call has wrong number of arguments");
    }

    // emit call
    if (wasmOp == wnCALL)
    {
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, funcNum + m_module->funcOffset);
    }
    else
    {
        Assert(wasmOp == wnCALL_INDIRECT);
        Js::RegSlot tmp = m_i32RegSlots->AcquireTmpRegister();
        // todo:: Add bounds check. Asm.js doesn't need it because there has to be an & operator
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlotArr, tmp, 1, calleeSignature->GetSignatureId() + m_module->indirFuncTableOffset);
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdArr_Func, 0, tmp, indirectIndexInfo.location);
        m_i32RegSlots->ReleaseTmpRegister(tmp);
        GetRegisterSpace(indirectIndexInfo.type)->ReleaseLocation(&indirectIndexInfo);
    }

    // calculate number of RegSlots the arguments consume
    Js::ArgSlot args;
    if (imported)
    {
        args = (Js::ArgSlot)(calleeSignature->GetParamCount() + 1);
    }
    else
    {
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var)))) + 1;
    }
    Js::OpCodeAsmJs callOp = imported ? Js::OpCodeAsmJs::Call : Js::OpCodeAsmJs::I_Call;
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
            Assert(!imported);
            retInfo.location = m_f32RegSlots->AcquireTmpRegister();
            convertOp = Js::OpCodeAsmJs::I_Conv_VTF;
            break;
        case WasmTypes::F64:
            retInfo.location = m_f64RegSlots->AcquireTmpRegister();
            convertOp = imported ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::I_Conv_VTF;
            break;
        case WasmTypes::I32:
            retInfo.location = m_i32RegSlots->AcquireTmpRegister();
            convertOp = imported ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::I_Conv_VTI;
            break;
        case WasmTypes::I64:
            Assert(UNREACHED);
            break;
        default:
            Assume(UNREACHED);
        }
        m_writer.AsmReg2(convertOp, retInfo.location, 0);
    }

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

    EmitInfo retInfo;
    if (falseExpr.type == WasmTypes::Void)
    {
        if (trueExpr.type != WasmTypes::Void)
        {
            GetRegisterSpace(trueExpr.type)->ReleaseLocation(&trueExpr);
        }
    }
    else if (trueExpr.type == WasmTypes::Void)
    {
        GetRegisterSpace(falseExpr.type)->ReleaseLocation(&falseExpr);
    }
    else if (falseExpr.type != trueExpr.type)
    {
        throw WasmCompilationException(L"If/Else branches must have same result type");
    }
    else
    {
        // we will use result info true expr as our result info
        // review: how will this work with break? must we store this location?
        if (falseExpr.location != trueExpr.location)
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
        retInfo = trueExpr;
    }

    m_writer.MarkAsmJsLabel(endLabel);

    Assert(m_nestedIfLevel > 0);
    --m_nestedIfLevel;

    return retInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitBrTable()
{
    const uint numTargets = m_reader->m_currentNode.brTable.numTargets;
    const UINT* targetTable = m_reader->m_currentNode.brTable.targetTable;
    const UINT defaultEntry = m_reader->m_currentNode.brTable.defaultTarget;

    // Compile scrutinee
    WasmOp op = m_reader->ReadFromBlock();
    EmitInfo scrutineeInfo = EmitExpr(op);

    Js::ByteCodeLabel fallthroughLabel = m_writer.DefineLabel();
    
    WasmRegisterSpace* regSpace = GetRegisterSpace(WasmTypes::I32);
    Js::RegSlot scrutineeVal = regSpace->AcquireTmpRegister();
    m_writer.AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeVal, scrutineeInfo.location);
    // Compile cases
    for (uint i = 0; i < numTargets; i++)
    {
        uint target = targetTable[i];
        Js::RegSlot caseLoc = regSpace->AcquireTmpRegister();
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, caseLoc, target);
        Js::ByteCodeLabel targetLabel;
        if (target == 0)
        {
            targetLabel = fallthroughLabel;
        }
        else
        {
            targetLabel = GetLabel(target);
        }
        m_writer.AsmBrReg2(Js::OpCodeAsmJs::Case_Int, targetLabel, scrutineeVal, caseLoc);
        regSpace->ReleaseTmpRegister(caseLoc);
    }
    regSpace->ReleaseTmpRegister(scrutineeVal);

    m_writer.AsmBr(defaultEntry, Js::OpCodeAsmJs::EndSwitch_Int);

    m_writer.MarkAsmJsLabel(fallthroughLabel);

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
    if (rhsType != rhs.type)
    {
        throw WasmCompilationException(L"Invalid type for RHS");
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
    EmitInfo info = EmitExpr(m_reader->ReadExpr());

    if (inputType != info.type)
    {
        throw WasmCompilationException(L"Invalid input type");
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
    Js::RegSlot indexReg = GetRegisterSpace(WasmTypes::I32)->AcquireTmpRegister();
    // TODO: emit less bytecode with expr + 0
    m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, indexReg, m_reader->m_currentNode.mem.offset);

    EmitInfo exprInfo = EmitExpr(m_reader->ReadExpr());

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"Index expression must be of type I32");
    }

    m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Int, indexReg, exprInfo.location, indexReg);
    GetRegisterSpace(WasmTypes::I32)->ReleaseLocation(&exprInfo);

    GetRegisterSpace(WasmTypes::I32)->ReleaseTmpRegister(indexReg);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();

    m_writer.AsmTypedArr(Js::OpCodeAsmJs::LdArr, resultReg, indexReg, GetViewType(wasmOp));

    return EmitInfo(resultReg, type);
}

template<WasmOp wasmOp, WasmTypes::WasmType type>
EmitInfo
WasmBytecodeGenerator::EmitMemStore()
{
    // TODO (michhol): combine with MemRead
    Js::RegSlot indexReg = m_i32RegSlots->AcquireTmpRegister();
    // TODO: emit less bytecode with expr + 0
    m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, indexReg, m_reader->m_currentNode.mem.offset);

    EmitInfo exprInfo = EmitExpr(m_reader->ReadExpr());

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"Index expression must be of type I32");
    }

    m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Int, indexReg, exprInfo.location, indexReg);
    m_i32RegSlots->ReleaseLocation(&exprInfo);

    EmitInfo rhsInfo = EmitExpr(m_reader->ReadExpr());
    if (rhsInfo.type != type)
    {
        throw WasmCompilationException(L"Invalid type for store op");
    }

    m_writer.AsmTypedArr(Js::OpCodeAsmJs::StArr, rhsInfo.location, indexReg, GetViewType(wasmOp));
    ReleaseLocation(&rhsInfo);
    m_i32RegSlots->ReleaseTmpRegister(indexReg);

    Js::RegSlot retLoc = GetRegisterSpace(rhsInfo.type)->AcquireTmpRegister();
    m_writer.AsmReg2(GetLoadOp(rhsInfo.type), retLoc, rhsInfo.location);

    return EmitInfo(retLoc, rhsInfo.type);
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
    m_writer.AsmBr(m_funcInfo->GetExitLabel());

    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitSelect()
{
    EmitInfo conditionInfo = EmitExpr(m_reader->ReadExpr());
    if (conditionInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(L"select condition must have I32 type");
    }

    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel doneLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, conditionInfo.location);
    m_i32RegSlots->ReleaseLocation(&conditionInfo);

    EmitInfo trueInfo = EmitExpr(m_reader->ReadExpr());
    m_writer.AsmBr(doneLabel);
    m_writer.MarkAsmJsLabel(falseLabel);

    EmitInfo falseInfo = EmitExpr(m_reader->ReadExpr());
    if (trueInfo.type != falseInfo.type)
    {
        throw WasmCompilationException(L"select operands must both have same type");
    }

    Js::OpCodeAsmJs op = GetLoadOp(trueInfo.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(trueInfo.type);

    m_writer.AsmReg2(op, trueInfo.location, falseInfo.location);
    regSpace->ReleaseLocation(&falseInfo);

    m_writer.MarkAsmJsLabel(doneLabel);

    return trueInfo;
}

template<WasmOp wasmOp>
EmitInfo
WasmBytecodeGenerator::EmitBr()
{
    UINT depth = m_reader->m_currentNode.br.depth + 1;
    Assert(depth <= m_labels->Count());

    EmitInfo conditionInfo;
    if (wasmOp == WasmOp::wnBR_IF)
    {
        conditionInfo = EmitExpr(m_reader->ReadFromBlock());
        if (conditionInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(L"br_if condition must have I32 type");
        }
    }

    if (m_reader->m_currentNode.br.hasSubExpr)
    {
        // TODO: Handle value that Break is supposed to "throw".
        EmitInfo info = EmitExpr(m_reader->ReadFromBlock());
    }

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
    default:
        Assert(UNREACHED);
        return Js::OpCodeAsmJs::Nop;
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
        Assert(UNREACHED);
        return Js::ArrayBufferView::ViewType::TYPE_INVALID;
    }
}


void
WasmBytecodeGenerator::ReleaseLocation(EmitInfo * info)
{
    if (info->type != WasmTypes::Void)
    {
        Assert(info->type < WasmTypes::Limit);
        GetRegisterSpace(info->type)->ReleaseLocation(info);
    }
}

Js::ByteCodeLabel
WasmBytecodeGenerator::GetLabel(uint depth)
{
    Assert(depth != 0);
    SListCounted<Js::ByteCodeLabel>::Iterator itr(m_labels);
    for (UINT i = 0; i < depth; ++i)
    {
        itr.Next();
    }
    return itr.Data();
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
    va_list arglist;
    va_start(arglist, _msg);
    Output::VPrint(_msg, arglist);
    Output::Print(L"\r\n");
    Output::Flush();
}

} // namespace Wasm

#endif // ENABLE_WASM
