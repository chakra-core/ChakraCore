//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#ifdef ASMJS_PLAT
#include "ByteCode/Symbol.h"
#include "ByteCode/FuncInfo.h"
#include "ByteCode/ByteCodeApi.h"
#include "ByteCode/ByteCodeWriter.h"
#include "ByteCode/ByteCodeGenerator.h"
#include "ByteCode/AsmJsByteCodeWriter.h"
#include "Language/AsmJsByteCodeGenerator.h"

#if DBG_DUMP
#include "ByteCode/ByteCodeDumper.h"
#include "ByteCode/AsmJsByteCodeDumper.h"
#endif

namespace Js
{

    bool AsmJsModuleCompiler::CompileAllFunctions()
    {
        const int size = mFunctionArray.Count();

        for (int i = 0; i < size; i++)
        {
            AsmJsFunc* func = mFunctionArray.Item(i);

            if (!CompileFunction(func, i))
            {
                // an error occurred in the function, revert state on all asm.js functions
                for (int j = 0; j <= i; j++)
                {
                    RevertFunction(j);
                }
                return false;
            }
            func->Finish();
        }
        return true;
    }


    void AsmJsModuleCompiler::RevertFunction(int funcIndex)
    {
        AsmJsFunc* func = mFunctionArray.Item(funcIndex);
        FunctionBody * funcBody = func->GetFuncBody();
        funcBody->ResetByteCodeGenState();
        funcBody->AddDeferParseAttribute();
        funcBody->SetFunctionParsed(false);
        funcBody->ResetEntryPoint();
        funcBody->SetEntryPoint(funcBody->GetDefaultEntryPointInfo(), GetScriptContext()->DeferredParsingThunk);
        funcBody->SetIsAsmjsMode(false);
        funcBody->SetIsAsmJsFunction(false);
        func->GetFncNode()->funcInfo->byteCodeFunction = func->GetFuncBody();
    }

    void AsmJsModuleCompiler::RevertAllFunctions()
    {
        for (int i = 0; i < mFunctionArray.Count(); i++)
        {
            RevertFunction(i);
        }
    }


    bool AsmJsModuleCompiler::CommitFunctions()
    {
        const int size = mFunctionArray.Count();
        // if changeHeap is defined, it must be first function, so we should skip it
        for (int i = 0; i < size; i++)
        {
            AsmJsFunc* func = mFunctionArray.Item(i);
            FunctionBody* functionBody = func->GetFuncBody();
            AsmJsFunctionInfo* asmInfo = functionBody->AllocateAsmJsFunctionInfo();

            if (!asmInfo->Init(func))
            {
                return false;
            }
            asmInfo->SetUsesHeapBuffer(mUsesHeapBuffer);

            functionBody->CheckAndSetOutParamMaxDepth(func->GetMaxArgOutDepth());
            // should be set in EmitOneFunction
            Assert(functionBody->GetIsAsmjsMode());
            Assert(functionBody->GetIsAsmJsFunction());
            ((EntryPointInfo*)functionBody->GetDefaultEntryPointInfo())->SetIsAsmJSFunction(true);

#if DBG_DUMP && defined(ASMJS_PLAT)
            if (PHASE_DUMP(ByteCodePhase, functionBody))
            {
                AsmJsByteCodeDumper::Dump(functionBody, nullptr, func);
            }
#endif
#if _M_IX86
            if (PHASE_ON1(AsmJsJITTemplatePhase) && !Configuration::Global.flags.NoNative)
            {
                AsmJsCodeGenerator* generator = GetScriptContext()->GetAsmJsCodeGenerator();
                AccumulateCompileTime();
                if (!generator)
                {
                    generator = GetScriptContext()->InitAsmJsCodeGenerator();
                }
                Assert(generator);
                generator->CodeGen(functionBody);
                AccumulateCompileTime(AsmJsCompilation::TemplateJIT);
            }
#endif
        }

        return true;
    }

    bool AsmJsModuleCompiler::CommitModule()
    {
        FuncInfo* funcInfo = GetModuleFunctionNode()->funcInfo;
        FunctionBody* functionBody = funcInfo->GetParsedFunctionBody();
        AsmJsModuleInfo* asmInfo = functionBody->AllocateAsmJsModuleInfo();

        if (funcInfo->byteCodeFunction->GetIsNamedFunctionExpression())
        {
            Assert(GetModuleFunctionNode()->pnodeName);
            Assert(GetModuleFunctionNode()->pnodeName->nop == knopVarDecl);
            ParseNodeVar * nameNode = GetModuleFunctionNode()->pnodeName;
            if (nameNode->sym->IsInSlot(GetByteCodeGenerator(), funcInfo))
            {
                GetByteCodeGenerator()->AssignPropertyId(nameNode->name());
                // if module is a named function expression, we may need to restore this for debugger
                AsmJsClosureFunction* closure = Anew(&mAllocator, AsmJsClosureFunction, nameNode->pid, AsmJsSymbol::ClosureFunction, &mAllocator);
                DefineIdentifier(nameNode->pid, closure);
            }
        }

        int argCount = 0;
        if (mBufferArgName)
        {
            argCount = 3;
        }
        else if (mForeignArgName)
        {
            argCount = 2;
        }
        else if (mStdLibArgName)
        {
            argCount = 1;
        }

        const int functionCount = mFunctionArray.Count();
        const int functionTableCount = mFunctionTableArray.Count();
        const int importFunctionCount = mImportFunctions.GetTotalVarCount();
        asmInfo->SetFunctionCount(functionCount);
        asmInfo->SetFunctionTableCount(functionTableCount);
        asmInfo->SetFunctionImportCount(importFunctionCount);
        asmInfo->SetVarCount(mVarCount);
        asmInfo->SetVarImportCount(mVarImportCount);
        asmInfo->SetArgInCount(argCount);
        asmInfo->SetModuleMemory(mModuleMemory);
        asmInfo->SetAsmMathBuiltinUsed(mAsmMathBuiltinUsedBV);
        asmInfo->SetAsmArrayBuiltinUsed(mAsmArrayBuiltinUsedBV);
        asmInfo->SetMaxHeapAccess(mMaxHeapAccess);
        int varCount = 3; // 3 possible arguments

        functionBody->SetInParamsCount(4); // Always set 4 inParams so the memory space is the same (globalEnv,stdlib,foreign,buffer)
        functionBody->SetReportedInParamsCount(4);
        functionBody->CheckAndSetConstantCount(2); // Return register + Root
        functionBody->CreateConstantTable();
        functionBody->CheckAndSetVarCount(varCount);
        functionBody->SetIsAsmjsMode(true);
        functionBody->NewObjectLiteral(); // allocate one object literal for the export object

        AsmJSByteCodeGenerator::EmitEmptyByteCode(funcInfo, GetByteCodeGenerator(), GetModuleFunctionNode());

        // Create export module proxy
        asmInfo->SetExportFunctionIndex(mExportFuncIndex);
        asmInfo->SetExportsCount(mExports.Count());
        auto exportIter = mExports.GetIterator();

        for (int exportIndex = 0; exportIter.IsValid(); ++exportIndex)
        {
            const AsmJsModuleExport& exMod = exportIter.CurrentValue();
            auto ex = asmInfo->GetExport(exportIndex);
            *ex.id = exMod.id;
            *ex.location = exMod.location;
            exportIter.MoveNext();
        }

        int iVar = 0, iVarImp = 0, iFunc = 0, iFuncImp = 0;
        const int moduleEnvCount = mModuleEnvironment.Count();
        asmInfo->InitializeSlotMap(moduleEnvCount);
        auto slotMap = asmInfo->GetAsmJsSlotMap();
        for (int i = 0; i < moduleEnvCount; i++)
        {
            AsmJsSymbol* sym = mModuleEnvironment.GetValueAt(i);
            if (sym)
            {
                AsmJsSlot * slot = RecyclerNewLeaf(GetScriptContext()->GetRecycler(), AsmJsSlot);
                slot->symType = sym->GetSymbolType();
                slotMap->AddNew(sym->GetName()->GetPropertyId(), slot);
                switch (sym->GetSymbolType())
                {
                case AsmJsSymbol::Variable:
                {
                    AsmJsVar* var = AsmJsVar::FromSymbol(sym);
                    auto& modVar = asmInfo->GetVar(iVar++);
                    modVar.location = var->GetLocation();
                    modVar.type = var->GetVarType().which();
                    if (var->GetVarType().isInt())
                    {
                        modVar.initialiser.intInit = var->GetIntInitialiser();
                    }
                    else if (var->GetVarType().isFloat())
                    {
                        modVar.initialiser.floatInit = var->GetFloatInitialiser();
                    }
                    else if (var->GetVarType().isDouble())
                    {
                        modVar.initialiser.doubleInit = var->GetDoubleInitialiser();
                    }
                    else
                    {
                        Assert(UNREACHED);
                    }

                    modVar.isMutable = var->isMutable();

                    slot->location = modVar.location;
                    slot->varType = var->GetVarType().which();
                    slot->isConstVar = !modVar.isMutable;
                    break;
                }
                case AsmJsSymbol::ConstantImport:
                {
                    AsmJsConstantImport* var = AsmJsConstantImport::FromSymbol(sym);
                    auto& modVar = asmInfo->GetVarImport(iVarImp++);
                    modVar.location = var->GetLocation();
                    modVar.field = var->GetField()->GetPropertyId();
                    modVar.type = var->GetVarType().which();

                    slot->location = modVar.location;
                    slot->varType = modVar.type;
                    break;
                }
                case AsmJsSymbol::ImportFunction:
                {
                    AsmJsImportFunction* func = AsmJsImportFunction::FromSymbol(sym);
                    auto& modVar = asmInfo->GetFunctionImport(iFuncImp++);
                    modVar.location = func->GetFunctionIndex();
                    modVar.field = func->GetField()->GetPropertyId();

                    slot->location = modVar.location;
                    break;
                }
                case AsmJsSymbol::FuncPtrTable:
                {
                    AsmJsFunctionTable* funcTable = AsmJsFunctionTable::FromSymbol(sym);
                    const uint size = funcTable->GetSize();
                    const RegSlot index = funcTable->GetFunctionIndex();
                    asmInfo->SetFunctionTableSize(index, size);
                    auto& modTable = asmInfo->GetFunctionTable(index);
                    for (uint j = 0; j < size; j++)
                    {
                        modTable.moduleFunctionIndex[j] = funcTable->GetModuleFunctionIndex(j);
                    }
                    slot->funcTableSize = size;
                    slot->location = index;

                    break;
                }
                case AsmJsSymbol::ModuleFunction:
                {
                    AsmJsFunc* func = AsmJsFunc::FromSymbol(sym);
                    auto& modVar = asmInfo->GetFunction(iFunc++);
                    modVar.location = func->GetFunctionIndex();
                    slot->location = modVar.location;
                    break;
                }
                case AsmJsSymbol::ArrayView:
                {
                    AsmJsArrayView * var = AsmJsArrayView::FromSymbol(sym);
                    slot->viewType = var->GetViewType();
                    break;
                }
                case AsmJsSymbol::ModuleArgument:
                {
                    AsmJsModuleArg * arg = AsmJsModuleArg::FromSymbol(sym);
                    slot->argType = arg->GetArgType();
                    break;
                }
                // used only for module validation
                case AsmJsSymbol::MathConstant:
                {
                    AsmJsMathConst * constVar = AsmJsMathConst::FromSymbol(sym);
                    slot->mathConstVal = *constVar->GetVal();
                    break;
                }
                case AsmJsSymbol::MathBuiltinFunction:
                {
                    AsmJsMathFunction * mathFunc = AsmJsMathFunction::FromSymbol(sym);
                    slot->builtinMathFunc = mathFunc->GetMathBuiltInFunction();
                    break;
                }
                case AsmJsSymbol::TypedArrayBuiltinFunction:
                {
                    AsmJsTypedArrayFunction * mathFunc = AsmJsTypedArrayFunction::FromSymbol(sym);
                    slot->builtinArrayFunc = mathFunc->GetArrayBuiltInFunction();
                    break;
                }
                case AsmJsSymbol::ClosureFunction:
                    // we don't need to store any additional info in this case
                    break;
                default:
                    Assume(UNREACHED);
                }
            }
        }
        return true;
    }

    void AsmJsModuleCompiler::ASTPrepass(ParseNodePtr pnode, AsmJsFunc * func)
    {
        ThreadContext::ProbeCurrentStackNoDispose(Js::Constants::MinStackByteCodeVisitor, GetByteCodeGenerator()->GetScriptContext());

        if (pnode == NULL)
        {
            return;
        }

        switch (pnode->nop) {
            // these first cases do the interesting work
        case knopBreak:
        case knopContinue:
            GetByteCodeGenerator()->AddTargetStmt(pnode->AsParseNodeJump()->pnodeTarget);
            break;

        case knopInt:
            func->AddConst<int>(pnode->AsParseNodeInt()->lw);
            break;
        case knopFlt:
        {
            const double d = pnode->AsParseNodeFloat()->dbl;
            if (ParserWrapper::IsMinInt(pnode))
            {
                func->AddConst<int>((int)d);
            }
            else if (ParserWrapper::IsUnsigned(pnode))
            {
                func->AddConst<int>((int)(uint32)d);
            }
            else
            {
                func->AddConst<double>(d);
            }
            break;
        }
        case knopName:
        {
            GetByteCodeGenerator()->AssignPropertyId(pnode->name());
            AsmJsSymbol * declSym = LookupIdentifier(pnode->name());
            if (declSym)
            {
                if (AsmJsMathConst::Is(declSym))
                {
                    AsmJsMathConst * definition = AsmJsMathConst::FromSymbol(declSym);
                    Assert(definition->GetType().isDouble());
                    func->AddConst<double>(*definition->GetVal());
                }
                else if (AsmJsVar::Is(declSym) && !declSym->isMutable())
                {
                    AsmJsVar * definition = AsmJsVar::FromSymbol(declSym);
                    switch (definition->GetVarType().which())
                    {
                    case AsmJsVarType::Double:
                        func->AddConst<double>(definition->GetDoubleInitialiser());
                        break;
                    case AsmJsVarType::Float:
                        func->AddConst<float>(definition->GetFloatInitialiser());
                        break;
                    case AsmJsVarType::Int:
                        func->AddConst<int>(definition->GetIntInitialiser());
                        break;
                    default:
                        Assume(UNREACHED);
                    }
                }
            }
            break;
        }
        case knopCall:
        {
            ASTPrepass(pnode->AsParseNodeCall()->pnodeTarget, func);
            bool evalArgs = true;
            if (pnode->AsParseNodeCall()->pnodeTarget->nop == knopName)
            {
                AsmJsFunctionDeclaration* funcDecl = this->LookupFunction(pnode->AsParseNodeCall()->pnodeTarget->name());
                if (AsmJsMathFunction::IsFround(funcDecl) && pnode->AsParseNodeCall()->argCount > 0)
                {
                    switch (pnode->AsParseNodeCall()->pnodeArgs->nop)
                    {
                    case knopFlt:
                        func->AddConst<float>((float)pnode->AsParseNodeCall()->pnodeArgs->AsParseNodeFloat()->dbl);
                        evalArgs = false;
                        break;

                    case knopInt:
                        func->AddConst<float>((float)pnode->AsParseNodeCall()->pnodeArgs->AsParseNodeInt()->lw);
                        evalArgs = false;
                        break;

                    case knopNeg:
                        if (pnode->AsParseNodeCall()->pnodeArgs->AsParseNodeUni()->pnode1->nop == knopInt && pnode->AsParseNodeCall()->pnodeArgs->AsParseNodeUni()->pnode1->AsParseNodeInt()->lw == 0)
                        {
                            func->AddConst<float>(-0.0f);
                            evalArgs = false;
                            break;
                        }
                    }
                }
            }
            if (evalArgs)
            {
                ASTPrepass(pnode->AsParseNodeCall()->pnodeArgs, func);
            }
            break;
        }
        case knopVarDecl:
            GetByteCodeGenerator()->AssignPropertyId(pnode->name());
            ASTPrepass(pnode->AsParseNodeVar()->pnodeInit, func);
            break;
            // all the rest of the cases simply walk the AST
        case knopQmark:
            ASTPrepass(pnode->AsParseNodeTri()->pnode1, func);
            ASTPrepass(pnode->AsParseNodeTri()->pnode2, func);
            ASTPrepass(pnode->AsParseNodeTri()->pnode3, func);
            break;
        case knopList:
            do
            {
                ParseNode * pnode1 = pnode->AsParseNodeBin()->pnode1;
                ASTPrepass(pnode1, func);
                pnode = pnode->AsParseNodeBin()->pnode2;
            } while (pnode->nop == knopList);
            ASTPrepass(pnode, func);
            break;
        case knopFor:
            ASTPrepass(pnode->AsParseNodeFor()->pnodeInit, func);
            ASTPrepass(pnode->AsParseNodeFor()->pnodeCond, func);
            ASTPrepass(pnode->AsParseNodeFor()->pnodeIncr, func);
            ASTPrepass(pnode->AsParseNodeFor()->pnodeBody, func);
            break;
        case knopIf:
            ASTPrepass(pnode->AsParseNodeIf()->pnodeCond, func);
            ASTPrepass(pnode->AsParseNodeIf()->pnodeTrue, func);
            ASTPrepass(pnode->AsParseNodeIf()->pnodeFalse, func);
            break;
        case knopDoWhile:
        case knopWhile:
            ASTPrepass(pnode->AsParseNodeWhile()->pnodeCond, func);
            ASTPrepass(pnode->AsParseNodeWhile()->pnodeBody, func);
            break;
        case knopReturn:
            ASTPrepass(pnode->AsParseNodeReturn()->pnodeExpr, func);
            break;
        case knopBlock:
            ASTPrepass(pnode->AsParseNodeBlock()->pnodeStmt, func);
            break;
        case knopSwitch:
            ASTPrepass(pnode->AsParseNodeSwitch()->pnodeVal, func);
            for (ParseNodeCase *pnodeT = pnode->AsParseNodeSwitch()->pnodeCases; NULL != pnodeT; pnodeT = pnodeT->pnodeNext)
            {
                ASTPrepass(pnodeT, func);
            }
            ASTPrepass(pnode->AsParseNodeSwitch()->pnodeBlock, func);
            break;
        case knopCase:
            ASTPrepass(pnode->AsParseNodeCase()->pnodeExpr, func);
            ASTPrepass(pnode->AsParseNodeCase()->pnodeBody, func);
            break;
        case knopComma:
        {
            ParseNode *pnode1 = pnode->AsParseNodeBin()->pnode1;
            if (pnode1->nop == knopComma)
            {
                // avoid recursion on very large comma expressions.
                ArenaAllocator *alloc = GetByteCodeGenerator()->GetAllocator();
                SList<ParseNode*> *rhsStack = Anew(alloc, SList<ParseNode*>, alloc);
                do {
                    rhsStack->Push(pnode1->AsParseNodeBin()->pnode2);
                    pnode1 = pnode1->AsParseNodeBin()->pnode1;
                } while (pnode1->nop == knopComma);
                ASTPrepass(pnode1, func);
                while (!rhsStack->Empty())
                {
                    ParseNode *pnodeRhs = rhsStack->Pop();
                    ASTPrepass(pnodeRhs, func);
                }
                Adelete(alloc, rhsStack);
            }
            else
            {
                ASTPrepass(pnode1, func);
            }
            ASTPrepass(pnode->AsParseNodeBin()->pnode2, func);
            break;
        }
        default:
        {
            uint flags = ParseNode::Grfnop(pnode->nop);
            if (flags&fnopUni)
            {
                ASTPrepass(pnode->AsParseNodeUni()->pnode1, func);
            }
            else if (flags&fnopBin)
            {
                ASTPrepass(pnode->AsParseNodeBin()->pnode1, func);
                ASTPrepass(pnode->AsParseNodeBin()->pnode2, func);
            }
            break;
        }
        }
    }

    void AsmJsModuleCompiler::BindArguments(ParseNode* argList)
    {
        for (ParseNode* pnode = argList; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
        {
            GetByteCodeGenerator()->AssignPropertyId(pnode->name());
        }
    }

    bool AsmJsModuleCompiler::CompileFunction(AsmJsFunc * func, int funcIndex)
    {
        ParseNodeFnc * fncNode = func->GetFncNode();
        ParseNodePtr pnodeBody = nullptr;

        Assert(fncNode->nop == knopFncDecl && fncNode->funcInfo && fncNode->funcInfo->IsDeferred() && fncNode->pnodeBody == NULL);

        Js::ParseableFunctionInfo* deferParseFunction = fncNode->funcInfo->byteCodeFunction;
        Utf8SourceInfo * utf8SourceInfo = deferParseFunction->GetUtf8SourceInfo();
        ULONG grfscr = utf8SourceInfo->GetParseFlags();
        grfscr = grfscr & (~fscrGlobalCode);
        func->SetOrigParseFlags(grfscr);
        deferParseFunction->SetGrfscr(grfscr | (grfscr & ~fscrDeferredFncExpression));
        deferParseFunction->SetSourceInfo(GetByteCodeGenerator()->GetCurrentSourceIndex(),
            fncNode,
            !!(grfscr & fscrEvalCode),
            ((grfscr & fscrDynamicCode) && !(grfscr & fscrEvalCode)));

        deferParseFunction->SetInParamsCount(fncNode->funcInfo->inArgsCount);
        deferParseFunction->SetReportedInParamsCount(fncNode->funcInfo->inArgsCount);

        if (fncNode->pnodeBody == NULL)
        {
            if (!PHASE_OFF1(Js::SkipNestedDeferredPhase) && (grfscr & fscrCreateParserState) == fscrCreateParserState && deferParseFunction->GetCompileCount() == 0)
            {
                deferParseFunction->BuildDeferredStubs(fncNode);
            }
        }
        deferParseFunction->SetIsAsmjsMode(true);
        Parser ps(GetScriptContext(), FALSE, this->GetAllocator()->GetPageAllocator());
        FunctionBody * funcBody;
        ParseNodeProg * parseTree;

        CompileScriptException se;
        funcBody = deferParseFunction->ParseAsmJs(&ps, &se, &parseTree);
        fncNode->funcInfo->byteCodeFunction = funcBody;

        TRACE_BYTECODE(_u("\nDeferred parse %s\n"), funcBody->GetDisplayName());
        if (parseTree && parseTree->nop == knopProg)
        {
            auto body = parseTree->pnodeBody;
            if (body && body->nop == knopList)
            {
                auto fncDecl = body->AsParseNodeBin()->pnode1;
                if (fncDecl && fncDecl->nop == knopFncDecl)
                {
                    pnodeBody = fncDecl->AsParseNodeFnc()->pnodeBody;
                    func->SetFuncBody(funcBody);
                }
            }
        }
        GetByteCodeGenerator()->PushFuncInfo(_u("Start asm.js AST prepass"), fncNode->funcInfo);
        BindArguments(fncNode->pnodeParams);
        ASTPrepass(pnodeBody, func);
        GetByteCodeGenerator()->PopFuncInfo(_u("End asm.js AST prepass"));

        fncNode->pnodeBody = pnodeBody;

        if (!pnodeBody)
        {
            // body should never be null if parsing succeeded
            Assert(UNREACHED);
            return Fail(fncNode, _u("Function should always have parse nodes"));
        }

        // Check if this function requires a bigger Ast
        UpdateMaxAstSize(fncNode->astSize);

        if (!SetupFunctionArguments(func, pnodeBody))
        {
            // failure message will be printed by SetupFunctionArguments
            fncNode->pnodeBody = NULL;
            return false;
        }

        if (!SetupLocalVariables(func))
        {
            // failure message will be printed by SetupLocalVariables
            fncNode->pnodeBody = NULL;
            return false;
        }

        // now that we have setup the function, we can generate bytecode for it
        AsmJSByteCodeGenerator gen(func, this);
        bool wasEmit = gen.EmitOneFunction();
        fncNode->pnodeBody = NULL;
        return wasEmit;
    }


    bool AsmJsModuleCompiler::SetupFunctionArguments(AsmJsFunc * func, ParseNodePtr pnode)
    {
        // Check arguments
        ArgSlot numArguments = 0;
        ParseNode * fncNode = func->GetFncNode();
        ParseNode* argNode = ParserWrapper::FunctionArgsList(fncNode, numArguments);

        if (!func->EnsureArgCount(numArguments))
        {
            return Fail(argNode, _u("Cannot have variable number of arguments"));
        }

        ArgSlot index = 0;
        while (argNode)
        {
            if (pnode->nop != knopList)
            {
                return Fail(pnode, _u("Missing assignment statement for argument"));
            }


            if (!ParserWrapper::IsDefinition(argNode))
            {
                return Fail(argNode, _u("duplicate argument name not allowed"));
            }

            PropertyName argName = argNode->name();
            if (!AsmJSCompiler::CheckIdentifier(*this, argNode, argName))
            {
                return false;
            }

            // creates the variable
            AsmJsVarBase* var = func->DefineVar(argName, true);
            if (!var)
            {
                return Fail(argNode, _u("Failed to define var"));
            }

            ParseNode* argDefinition = ParserWrapper::GetBinaryLeft(pnode);
            if (argDefinition->nop != knopAsg)
            {
                return Fail(argDefinition, _u("Expecting an assignment"));
            }

            ParseNode* lhs = ParserWrapper::GetBinaryLeft(argDefinition);
            ParseNode* rhs = ParserWrapper::GetBinaryRight(argDefinition);

#define NodeDefineThisArgument(n,var) (n->nop == knopName && ParserWrapper::VariableName(n)->GetPropertyId() == var->GetName()->GetPropertyId())

            if (!NodeDefineThisArgument(lhs, var))
            {
                return Fail(lhs, _u("Defining wrong argument"));
            }

            if (rhs->nop == knopPos)
            {
                // unary + => double
                var->SetVarType(AsmJsVarType::Double);
                var->SetLocation(func->AcquireRegister<double>());
                // validate stmt
                ParseNode* argSym = ParserWrapper::GetUnaryNode(rhs);

                if (!NodeDefineThisArgument(argSym, var))
                {
                    return Fail(lhs, _u("Defining wrong argument"));
                }
            }
            else if (rhs->nop == knopOr)
            {
                var->SetVarType(AsmJsVarType::Int);
                var->SetLocation(func->AcquireRegister<int>());

                ParseNode* argSym = ParserWrapper::GetBinaryLeft(rhs);
                ParseNode* intSym = ParserWrapper::GetBinaryRight(rhs);
                // validate stmt
                if (!NodeDefineThisArgument(argSym, var))
                {
                    return Fail(lhs, _u("Defining wrong argument"));
                }
                if (intSym->nop != knopInt || intSym->AsParseNodeInt()->lw != 0)
                {
                    return Fail(lhs, _u("Or value must be 0 when defining arguments"));
                }
            }
            else if (rhs->nop == knopCall)
            {
                ParseNodeCall* callNode = rhs->AsParseNodeCall();
                if (callNode->pnodeTarget->nop != knopName)
                {
                    return Fail(rhs, _u("call should be for fround"));
                }
                AsmJsFunctionDeclaration* funcDecl = this->LookupFunction(callNode->pnodeTarget->name());

                if (!funcDecl)
                    return Fail(rhs, _u("Cannot resolve function for argument definition, or wrong function"));

                if (AsmJsMathFunction::Is(funcDecl))
                {
                    if (!AsmJsMathFunction::IsFround(funcDecl))
                    {
                        return Fail(rhs, _u("call should be for fround"));
                    }
                    var->SetVarType(AsmJsVarType::Float);
                    var->SetLocation(func->AcquireRegister<float>());
                }
                else
                {
                    return Fail(rhs, _u("Wrong function used for argument definition"));
                }

                if (callNode->argCount == 0 || !NodeDefineThisArgument(callNode->pnodeArgs, var))
                {
                    return Fail(lhs, _u("Defining wrong argument"));
                }
            }
            else
            {
                return Fail(rhs, _u("arguments are not casted as valid Asm.js type"));
            }

            if (PHASE_TRACE1(ByteCodePhase))
            {
                Output::Print(_u("    Argument [%s] Valid"), argName->Psz());
            }

            if (!func->EnsureArgType(var, index++))
            {
                return Fail(rhs, _u("Unexpected argument type"));
            }

            argNode = ParserWrapper::NextVar(argNode);
            pnode = ParserWrapper::GetBinaryRight(pnode);
        }

        func->SetBodyNode(pnode);
        return true;
    }

    bool AsmJsModuleCompiler::SetupLocalVariables(AsmJsFunc * func)
    {
        ParseNodePtr pnode = func->GetBodyNode();
        MathBuiltin mathBuiltin;
        // define all variables
        BVSparse<ArenaAllocator> initializerBV(&mAllocator);
        while (pnode->nop == knopList)
        {
            ParseNode * varNode = ParserWrapper::GetBinaryLeft(pnode);
            while (varNode && varNode->nop != knopEndCode)
            {
                ParseNode * decl;
                if (varNode->nop == knopList)
                {
                    decl = ParserWrapper::GetBinaryLeft(varNode);
                    varNode = ParserWrapper::GetBinaryRight(varNode);
                }
                else
                {
                    decl = varNode;
                    varNode = nullptr;
                }
                // if we have hit a non-declaration, we are done processing the function header
                if (decl->nop != knopVarDecl)
                {
                    return true;
                }
                ParseNode* pnodeInit = decl->AsParseNodeVar()->pnodeInit;
                AsmJsSymbol * declSym = nullptr;

                bool isFroundInit = false;
                if (!pnodeInit)
                {
                    return Fail(decl, _u("The righthand side of a var declaration missing an initialization (empty)"));
                }

                if (pnodeInit->nop == knopName)
                {
                    declSym = LookupIdentifier(pnodeInit->name(), func);
                    if ((!AsmJsVar::Is(declSym) && !AsmJsMathConst::Is(declSym)) || declSym->isMutable())
                    {
                        return Fail(decl, _u("Var declaration with non-constant"));
                    }
                }
                else if (pnodeInit->nop == knopCall)
                {
                    if (pnodeInit->AsParseNodeCall()->pnodeTarget->nop != knopName)
                    {
                        return Fail(decl, _u("Var declaration with something else than a literal value|fround call"));
                    }
                    AsmJsFunctionDeclaration* funcDecl = this->LookupFunction(pnodeInit->AsParseNodeCall()->pnodeTarget->name());

                    if (!funcDecl)
                        return Fail(pnodeInit, _u("Cannot resolve function name"));

                    if (AsmJsMathFunction::Is(funcDecl))
                    {
                        if (!AsmJsMathFunction::IsFround(funcDecl) || !ParserWrapper::IsFroundNumericLiteral(pnodeInit->AsParseNodeCall()->pnodeArgs))
                        {
                            return Fail(decl, _u("Var declaration with something else than a literal value|fround call"));
                        }
                        isFroundInit = true;
                    }
                    else
                    {
                        return Fail(varNode, _u("Unknown function call on var declaration"));
                    }
                }
                else if (pnodeInit->nop != knopInt && pnodeInit->nop != knopFlt)
                {
                    return Fail(decl, _u("Var declaration with something else than a literal value|fround call"));
                }
                if (!AsmJSCompiler::CheckIdentifier(*this, decl, decl->name()))
                {
                    // CheckIdentifier will print failure message
                    return false;
                }

                AsmJsVar* var = (AsmJsVar*)func->DefineVar(decl->name(), false);
                if (!var)
                {
                    return Fail(decl, _u("Failed to define var"));
                }
                // If we are declaring a var that we previously used in an initializer, that value will be undefined
                // so we need to throw an error.
                if (initializerBV.Test(var->GetName()->GetPropertyId()))
                {
                    return Fail(decl, _u("Cannot declare a var after using it in an initializer"));
                }
                RegSlot loc = Constants::NoRegister;
                if (pnodeInit->nop == knopInt)
                {
                    var->SetVarType(AsmJsVarType::Int);
                    var->SetLocation(func->AcquireRegister<int>());
                    var->SetConstInitialiser(pnodeInit->AsParseNodeInt()->lw);
                    loc = func->GetConstRegister<int>(pnodeInit->AsParseNodeInt()->lw);
                }
                else if (ParserWrapper::IsMinInt(pnodeInit))
                {
                    var->SetVarType(AsmJsVarType::Int);
                    var->SetLocation(func->AcquireRegister<int>());
                    var->SetConstInitialiser(INT_MIN);
                    loc = func->GetConstRegister<int>(INT_MIN);
                }
                else if (ParserWrapper::IsUnsigned(pnodeInit))
                {
                    var->SetVarType(AsmJsVarType::Int);
                    var->SetLocation(func->AcquireRegister<int>());
                    var->SetConstInitialiser((int)((uint32)pnodeInit->AsParseNodeFloat()->dbl));
                    loc = func->GetConstRegister<int>((uint32)pnodeInit->AsParseNodeFloat()->dbl);
                }
                else if (pnodeInit->nop == knopFlt)
                {
                    if (pnodeInit->AsParseNodeFloat()->maybeInt)
                    {
                        return Fail(decl, _u("Var declaration with integer literal outside range [-2^31, 2^32)"));
                    }
                    var->SetVarType(AsmJsVarType::Double);
                    var->SetLocation(func->AcquireRegister<double>());
                    loc = func->GetConstRegister<double>(pnodeInit->AsParseNodeFloat()->dbl);
                    var->SetConstInitialiser(pnodeInit->AsParseNodeFloat()->dbl);
                }
                else if (pnodeInit->nop == knopName)
                {
                    if (AsmJsVar::Is(declSym))
                    {
                        AsmJsVar * definition = AsmJsVar::FromSymbol(declSym);
                        initializerBV.Set(definition->GetName()->GetPropertyId());
                        switch (definition->GetVarType().which())
                        {
                        case AsmJsVarType::Double:
                            var->SetVarType(AsmJsVarType::Double);
                            var->SetLocation(func->AcquireRegister<double>());
                            var->SetConstInitialiser(definition->GetDoubleInitialiser());
                            break;

                        case AsmJsVarType::Float:
                            var->SetVarType(AsmJsVarType::Float);
                            var->SetLocation(func->AcquireRegister<float>());
                            var->SetConstInitialiser(definition->GetFloatInitialiser());
                            break;

                        case AsmJsVarType::Int:
                            var->SetVarType(AsmJsVarType::Int);
                            var->SetLocation(func->AcquireRegister<int>());
                            var->SetConstInitialiser(definition->GetIntInitialiser());
                            break;

                        default:
                            Assume(UNREACHED);
                        }
                    }
                    else
                    {
                        Assert(declSym->GetType() == AsmJsType::Double);

                        AsmJsMathConst * definition = AsmJsMathConst::FromSymbol(declSym);

                        var->SetVarType(AsmJsVarType::Double);
                        var->SetLocation(func->AcquireRegister<double>());
                        var->SetConstInitialiser(*definition->GetVal());
                    }
                }
                else if (pnodeInit->nop == knopCall)
                {
                    if (isFroundInit)
                    {
                        var->SetVarType(AsmJsVarType::Float);
                        var->SetLocation(func->AcquireRegister<float>());
                        if (pnodeInit->AsParseNodeCall()->pnodeArgs->nop == knopInt)
                        {
                            int iVal = pnodeInit->AsParseNodeCall()->pnodeArgs->AsParseNodeInt()->lw;
                            var->SetConstInitialiser((float)iVal);
                            loc = func->GetConstRegister<float>((float)iVal);
                        }
                        else if (ParserWrapper::IsNegativeZero(pnodeInit->AsParseNodeCall()->pnodeArgs))
                        {
                            var->SetConstInitialiser(-0.0f);
                            loc = func->GetConstRegister<float>(-0.0f);
                        }
                        else
                        {
                            // note: fround((-)NumericLiteral) is explicitly allowed for any range, so we do not need to check for maybeInt
                            Assert(pnodeInit->AsParseNodeCall()->pnodeArgs->nop == knopFlt);
                            float fVal = (float)pnodeInit->AsParseNodeCall()->pnodeArgs->AsParseNodeFloat()->dbl;
                            var->SetConstInitialiser((float)fVal);
                            loc = func->GetConstRegister<float>(fVal);
                        }
                    }
                    else
                    {
                        Assert(UNREACHED);
                    }
                }

                if (loc == Constants::NoRegister && pnodeInit->nop != knopName)
                {
                    return Fail(decl, _u("Cannot find Register constant for var"));
                }
            }

            if (ParserWrapper::GetBinaryRight(pnode)->nop == knopEndCode)
            {
                break;
            }
            pnode = ParserWrapper::GetBinaryRight(pnode);
        }
        return true;
    }

    AsmJsFunc* AsmJsModuleCompiler::CreateNewFunctionEntry(ParseNodeFnc* pnodeFnc)
    {
        PropertyName name = ParserWrapper::FunctionName(pnodeFnc);
        if (!name)
        {
            return nullptr;
        }

        GetByteCodeGenerator()->AssignPropertyId(name);
        AsmJsFunc* func = Anew(&mAllocator, AsmJsFunc, name, pnodeFnc, &mAllocator, mCx->scriptContext);
        if (func)
        {
            if (DefineIdentifier(name, func))
            {
                uint index = (uint)mFunctionArray.Count();
                if (pnodeFnc->nestedIndex != index)
                {
                    return nullptr;
                }
                func->SetFunctionIndex((RegSlot)index);
                mFunctionArray.Add(func);
                Assert(index + 1 == (uint)mFunctionArray.Count());
                return func;
            }
            // Error adding function
            mAllocator.Free(func, sizeof(AsmJsFunc));
        }
        // Error allocating a new function
        return nullptr;
    }

    bool AsmJsModuleCompiler::CheckByteLengthCall(ParseNode * callNode, ParseNode * bufferDecl)
    {
        if (callNode->nop != knopCall || callNode->AsParseNodeCall()->pnodeTarget->nop != knopName)
        {
            return false;
        }
        AsmJsTypedArrayFunction* arrayFunc = LookupIdentifier<AsmJsTypedArrayFunction>(callNode->AsParseNodeCall()->pnodeTarget->name());
        if (!arrayFunc)
        {
            return false;
        }

        return callNode->AsParseNodeCall()->argCount == 1 &&
            !callNode->AsParseNodeCall()->isApplyCall &&
            !callNode->AsParseNodeCall()->isEvalCall &&
            callNode->AsParseNodeCall()->spreadArgCount == 0 &&
            arrayFunc->GetArrayBuiltInFunction() == AsmJSTypedArrayBuiltin_byteLength &&
            callNode->AsParseNodeCall()->pnodeArgs->nop == knopName &&
            callNode->AsParseNodeCall()->pnodeArgs->name()->GetPropertyId() == bufferDecl->name()->GetPropertyId();
    }

    bool AsmJsModuleCompiler::Fail(ParseNode* usepn, const wchar *error)
    {
        AsmJSCompiler::OutputError(GetScriptContext(), error);
        return false;
    }

    bool AsmJsModuleCompiler::FailName(ParseNode *usepn, const wchar *fmt, PropertyName name)
    {
        AsmJSCompiler::OutputError(GetScriptContext(), fmt, name->Psz());
        return false;
    }

    bool AsmJsModuleCompiler::LookupStandardLibraryMathName(PropertyName name, MathBuiltin *mathBuiltin) const
    {
        return mStandardLibraryMathNames.TryGetValue(name->GetPropertyId(), mathBuiltin);
    }

    bool AsmJsModuleCompiler::LookupStandardLibraryArrayName(PropertyName name, TypedArrayBuiltin *builtin) const
    {
        return mStandardLibraryArrayNames.TryGetValue(name->GetPropertyId(), builtin);
    }

    void AsmJsModuleCompiler::InitBufferArgName(PropertyName n)
    {
#if DBG
        Assert(!mBufferArgNameInit);
        mBufferArgNameInit = true;
#endif
        mBufferArgName = n;
    }

    void AsmJsModuleCompiler::InitForeignArgName(PropertyName n)
    {
#if DBG
        Assert(!mForeignArgNameInit);
        mForeignArgNameInit = true;
#endif
        mForeignArgName = n;
    }

    void AsmJsModuleCompiler::InitStdLibArgName(PropertyName n)
    {
#if DBG
        Assert(!mStdLibArgNameInit);
        mStdLibArgNameInit = true;
#endif
        mStdLibArgName = n;
    }

    Js::PropertyName AsmJsModuleCompiler::GetStdLibArgName() const
    {
#if DBG
        Assert(mBufferArgNameInit);
#endif
        return mStdLibArgName;
    }

    Js::PropertyName AsmJsModuleCompiler::GetForeignArgName() const
    {
#if DBG
        Assert(mForeignArgNameInit);
#endif
        return mForeignArgName;
    }

    Js::PropertyName AsmJsModuleCompiler::GetBufferArgName() const
    {
#if DBG
        Assert(mStdLibArgNameInit);
#endif
        return mBufferArgName;
    }

    bool AsmJsModuleCompiler::Init()
    {
        if (mInitialised)
        {
            return false;
        }
        mInitialised = true;

        struct MathFunc
        {
            MathFunc(PropertyId id_ = 0, AsmJsMathFunction* val_ = nullptr) :
                id(id_), val(val_)
            {
            }
            PropertyId id;
            AsmJsMathFunction* val;
        };
        MathFunc mathFunctions[AsmJSMathBuiltinFunction_COUNT];
        // we could move the mathBuiltinFuncname to MathFunc struct
        mathFunctions[AsmJSMathBuiltin_sin] = MathFunc(PropertyIds::sin, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_sin, OpCodeAsmJs::Sin_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_cos] = MathFunc(PropertyIds::cos, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_cos, OpCodeAsmJs::Cos_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_tan] = MathFunc(PropertyIds::tan, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_tan, OpCodeAsmJs::Tan_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_asin] = MathFunc(PropertyIds::asin, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_asin, OpCodeAsmJs::Asin_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_acos] = MathFunc(PropertyIds::acos, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_acos, OpCodeAsmJs::Acos_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_atan] = MathFunc(PropertyIds::atan, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_atan, OpCodeAsmJs::Atan_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_ceil] = MathFunc(PropertyIds::ceil, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_ceil, OpCodeAsmJs::Ceil_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_floor] = MathFunc(PropertyIds::floor, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_floor, OpCodeAsmJs::Floor_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_exp] = MathFunc(PropertyIds::exp, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_exp, OpCodeAsmJs::Exp_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_log] = MathFunc(PropertyIds::log, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_log, OpCodeAsmJs::Log_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_pow] = MathFunc(PropertyIds::pow, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_pow, OpCodeAsmJs::Pow_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_sqrt] = MathFunc(PropertyIds::sqrt, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_sqrt, OpCodeAsmJs::Sqrt_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_abs] = MathFunc(PropertyIds::abs, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_abs, OpCodeAsmJs::Abs_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_atan2] = MathFunc(PropertyIds::atan2, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_atan2, OpCodeAsmJs::Atan2_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_imul] = MathFunc(PropertyIds::imul, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_imul, OpCodeAsmJs::Imul_Int, AsmJsRetType::Signed, AsmJsType::Intish, AsmJsType::Intish));
        mathFunctions[AsmJSMathBuiltin_fround] = MathFunc(PropertyIds::fround, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_fround, OpCodeAsmJs::Fround_Flt, AsmJsRetType::Float, AsmJsType::Floatish));
        mathFunctions[AsmJSMathBuiltin_min] = MathFunc(PropertyIds::min, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_min, OpCodeAsmJs::Min_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_max] = MathFunc(PropertyIds::max, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_max, OpCodeAsmJs::Max_Db, AsmJsRetType::Double, AsmJsType::MaybeDouble, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_clz32] = MathFunc(PropertyIds::clz32, Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_clz32, OpCodeAsmJs::Clz32_Int, AsmJsRetType::Fixnum, AsmJsType::Intish));

        mathFunctions[AsmJSMathBuiltin_abs].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_abs, OpCodeAsmJs::Abs_Int, AsmJsRetType::Unsigned, AsmJsType::Signed));
        mathFunctions[AsmJSMathBuiltin_min].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_min, OpCodeAsmJs::Min_Int, AsmJsRetType::Signed, AsmJsType::Signed, AsmJsType::Signed));
        mathFunctions[AsmJSMathBuiltin_max].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 2, AsmJSMathBuiltin_max, OpCodeAsmJs::Max_Int, AsmJsRetType::Signed, AsmJsType::Signed, AsmJsType::Signed));

        //Float Overloads
        mathFunctions[AsmJSMathBuiltin_fround].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_fround, OpCodeAsmJs::Fround_Db, AsmJsRetType::Float, AsmJsType::MaybeDouble));
        mathFunctions[AsmJSMathBuiltin_fround].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_fround, OpCodeAsmJs::Fround_Int, AsmJsRetType::Float, AsmJsType::Int));// should we split this into signed and unsigned?
        mathFunctions[AsmJSMathBuiltin_abs].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_abs, OpCodeAsmJs::Abs_Flt, AsmJsRetType::Floatish, AsmJsType::MaybeFloat));
        mathFunctions[AsmJSMathBuiltin_ceil].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_ceil, OpCodeAsmJs::Ceil_Flt, AsmJsRetType::Floatish, AsmJsType::MaybeFloat));
        mathFunctions[AsmJSMathBuiltin_floor].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_floor, OpCodeAsmJs::Floor_Flt, AsmJsRetType::Floatish, AsmJsType::MaybeFloat));
        mathFunctions[AsmJSMathBuiltin_sqrt].val->SetOverload(Anew(&mAllocator, AsmJsMathFunction, nullptr, &mAllocator, 1, AsmJSMathBuiltin_sqrt, OpCodeAsmJs::Sqrt_Flt, AsmJsRetType::Floatish, AsmJsType::MaybeFloat));

        for (int i = 0; i < AsmJSMathBuiltinFunction_COUNT; i++)
        {
            if (!AddStandardLibraryMathName((PropertyId)mathFunctions[i].id, mathFunctions[i].val, mathFunctions[i].val->GetMathBuiltInFunction()))
            {
                return false;
            }
        }

        struct ConstMath
        {
            ConstMath(PropertyId id_, const double* val_, AsmJSMathBuiltinFunction mathLibConstName_) :
                id(id_), val(val_), mathLibConstName(mathLibConstName_) { }
            PropertyId id;
            AsmJSMathBuiltinFunction mathLibConstName;
            const double* val;
        };
        ConstMath constMath[] = {
            ConstMath(PropertyIds::E       , &Math::E                           , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_e),
            ConstMath(PropertyIds::LN10     , &Math::LN10                        , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_ln10),
            ConstMath(PropertyIds::LN2      , &Math::LN2                         , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_ln2),
            ConstMath(PropertyIds::LOG2E    , &Math::LOG2E                       , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_log2e),
            ConstMath(PropertyIds::LOG10E   , &Math::LOG10E                      , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_log10e),
            ConstMath(PropertyIds::PI       , &Math::PI                          , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_pi),
            ConstMath(PropertyIds::SQRT1_2  , &Math::SQRT1_2                     , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_sqrt1_2),
            ConstMath(PropertyIds::SQRT2    , &Math::SQRT2                       , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_sqrt2),
            ConstMath(PropertyIds::Infinity , &NumberConstants::POSITIVE_INFINITY, AsmJSMathBuiltinFunction::AsmJSMathBuiltin_infinity),
            ConstMath(PropertyIds::NaN      , &NumberConstants::NaN              , AsmJSMathBuiltinFunction::AsmJSMathBuiltin_nan),
        };
        const int size = sizeof(constMath) / sizeof(ConstMath);
        for (int i = 0; i < size; i++)
        {
            if (!AddStandardLibraryMathName(constMath[i].id, constMath[i].val, constMath[i].mathLibConstName))
            {
                return false;
            }
        }


        struct ArrayFunc
        {
            ArrayFunc(PropertyId id_ = 0, AsmJsTypedArrayFunction* val_ = nullptr) :
                id(id_), val(val_)
            {
            }
            PropertyId id;
            AsmJsTypedArrayFunction* val;
        };

        ArrayFunc arrayFunctions[AsmJSMathBuiltinFunction_COUNT];
        arrayFunctions[AsmJSTypedArrayBuiltin_Int8Array] = ArrayFunc(PropertyIds::Int8Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Int8Array, ArrayBufferView::TYPE_INT8));
        arrayFunctions[AsmJSTypedArrayBuiltin_Uint8Array] = ArrayFunc(PropertyIds::Uint8Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Uint8Array, ArrayBufferView::TYPE_UINT8));
        arrayFunctions[AsmJSTypedArrayBuiltin_Int16Array] = ArrayFunc(PropertyIds::Int16Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Int16Array, ArrayBufferView::TYPE_INT16));
        arrayFunctions[AsmJSTypedArrayBuiltin_Uint16Array] = ArrayFunc(PropertyIds::Uint16Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Uint16Array, ArrayBufferView::TYPE_UINT16));
        arrayFunctions[AsmJSTypedArrayBuiltin_Int32Array] = ArrayFunc(PropertyIds::Int32Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Int32Array, ArrayBufferView::TYPE_INT32));
        arrayFunctions[AsmJSTypedArrayBuiltin_Uint32Array] = ArrayFunc(PropertyIds::Uint32Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Uint32Array, ArrayBufferView::TYPE_UINT32));
        arrayFunctions[AsmJSTypedArrayBuiltin_Float32Array] = ArrayFunc(PropertyIds::Float32Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Float32Array, ArrayBufferView::TYPE_FLOAT32));
        arrayFunctions[AsmJSTypedArrayBuiltin_Float64Array] = ArrayFunc(PropertyIds::Float64Array, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_Float64Array, ArrayBufferView::TYPE_FLOAT64));
        arrayFunctions[AsmJSTypedArrayBuiltin_byteLength] = ArrayFunc(PropertyIds::byteLength, Anew(&mAllocator, AsmJsTypedArrayFunction, nullptr, &mAllocator, AsmJSTypedArrayBuiltin_byteLength, ArrayBufferView::TYPE_COUNT));

        for (int i = 0; i < AsmJSTypedArrayBuiltin_COUNT; i++)
        {
            if (!AddStandardLibraryArrayName((PropertyId)arrayFunctions[i].id, arrayFunctions[i].val, arrayFunctions[i].val->GetArrayBuiltInFunction()))
            {
                return false;
            }
        }
        return true;
    }

    AsmJsModuleCompiler::AsmJsModuleCompiler(ExclusiveContext *cx, AsmJSParser &parser) :
        mCx(cx)
        , mCurrentParserNode(parser)
        , mAllocator(_u("Asmjs"), cx->scriptContext->GetThreadContext()->GetPageAllocator(), Throw::OutOfMemory)
        , mModuleFunctionName(nullptr)
        , mStandardLibraryMathNames(&mAllocator)
        , mStandardLibraryArrayNames(&mAllocator)
        , mFunctionArray(&mAllocator)
        , mModuleEnvironment(&mAllocator)
        , mFunctionTableArray(&mAllocator)
        , mInitialised(false)
        , mIntVarSpace()
        , mDoubleVarSpace()
        , mExports(&mAllocator)
        , mExportFuncIndex(Js::Constants::NoRegister)
        , mVarImportCount(0)
        , mVarCount(0)
        , mFuncPtrTableCount(0)
        , mCompileTime()
        , mCompileTimeLastTick(GetTick())
        , mMaxAstSize(0)
        , mArrayViews(&mAllocator)
        , mUsesHeapBuffer(false)
        , mMaxHeapAccess(0)
#if DBG
        , mStdLibArgNameInit(false)
        , mForeignArgNameInit(false)
        , mBufferArgNameInit(false)
#endif
    {
        InitModuleNode(parser->AsParseNodeFnc());
    }

    bool AsmJsModuleCompiler::AddStandardLibraryMathName(PropertyId id, const double* cstAddr, AsmJSMathBuiltinFunction mathLibFunctionName)
    {
        // make sure this name is unique
        if (mStandardLibraryMathNames.ContainsKey(id))
        {
            return false;
        }

        MathBuiltin mathBuiltin(mathLibFunctionName, cstAddr);
        int addResult = mStandardLibraryMathNames.AddNew(id, mathBuiltin);
        if (addResult == -1)
        {
            // Error adding the function
            return false;
        }
        return true;
    }


    bool AsmJsModuleCompiler::AddStandardLibraryMathName(PropertyId id, AsmJsMathFunction* func, AsmJSMathBuiltinFunction mathLibFunctionName)
    {
        // make sure this name is unique
        if (mStandardLibraryMathNames.ContainsKey(id))
        {
            return false;
        }

        MathBuiltin mathBuiltin(mathLibFunctionName, func);
        int addResult = mStandardLibraryMathNames.AddNew(id, mathBuiltin);
        if (addResult == -1)
        {
            // Error adding the function
            return false;
        }
        return true;
    }

    bool AsmJsModuleCompiler::AddStandardLibraryArrayName(PropertyId id, AsmJsTypedArrayFunction* func, AsmJSTypedArrayBuiltinFunction arrayLibFunctionName)
    {
        // make sure this name is unique
        if (mStandardLibraryArrayNames.ContainsKey(id))
        {
            return false;
        }

        TypedArrayBuiltin arrayBuiltin(arrayLibFunctionName, func);
        int addResult = mStandardLibraryArrayNames.AddNew(id, arrayBuiltin);
        if (addResult == -1)
        {
            // Error adding the function
            return false;
        }
        return true;
    }

    Parser * AsmJsModuleCompiler::GetParser() const
    {
        return mCx->byteCodeGenerator->GetParser();
    }

    ByteCodeGenerator* AsmJsModuleCompiler::GetByteCodeGenerator() const
    {
        return mCx->byteCodeGenerator;
    }

    ScriptContext * AsmJsModuleCompiler::GetScriptContext() const
    {
        return mCx->scriptContext;
    }

    AsmJsSymbol* AsmJsModuleCompiler::LookupIdentifier(PropertyName name, AsmJsFunc* func /*= nullptr */, AsmJsLookupSource::Source* lookupSource /*= nullptr*/)
    {
        AsmJsSymbol* lookupResult = nullptr;
        if (name)
        {
            if (func)
            {
                lookupResult = func->LookupIdentifier(name, lookupSource);
                if (lookupResult)
                {
                    return lookupResult;
                }
            }

            lookupResult = mModuleEnvironment.LookupWithKey(name->GetPropertyId(), nullptr);
            if (lookupSource)
            {
                *lookupSource = AsmJsLookupSource::AsmJsModule;
            }
        }
        return lookupResult;
    }

    bool AsmJsModuleCompiler::DefineIdentifier(PropertyName name, AsmJsSymbol* symbol)
    {
        Assert(symbol);
        if (symbol)
        {
            // make sure this identifier is unique
            if (!LookupIdentifier(name))
            {
                int addResult = mModuleEnvironment.AddNew(name->GetPropertyId(), symbol);
                return addResult != -1;
            }
        }
        return false;
    }

    bool AsmJsModuleCompiler::AddNumericVar(PropertyName name, ParseNode* pnode, bool isFloat, bool isMutable /*= true*/)
    {
        Assert(ParserWrapper::IsNumericLiteral(pnode) || (isFloat && ParserWrapper::IsFroundNumericLiteral(pnode)));
        AsmJsVar* var = Anew(&mAllocator, AsmJsVar, name, isMutable);
        if (!var)
        {
            return false;
        }
        if (!DefineIdentifier(name, var))
        {
            return false;
        }

        ++mVarCount;

        if (isFloat)
        {
            var->SetVarType(AsmJsVarType::Float);
            var->SetLocation(mFloatVarSpace.AcquireRegister());
            if (pnode->nop == knopInt)
            {
                var->SetConstInitialiser((float)pnode->AsParseNodeInt()->lw);
            }
            else if (ParserWrapper::IsNegativeZero(pnode))
            {
                var->SetConstInitialiser(-0.0f);
            }
            else
            {
                var->SetConstInitialiser((float)pnode->AsParseNodeFloat()->dbl);
            }
        }
        else if (pnode->nop == knopInt)
        {
            var->SetVarType(AsmJsVarType::Int);
            var->SetLocation(mIntVarSpace.AcquireRegister());
            var->SetConstInitialiser(pnode->AsParseNodeInt()->lw);
        }
        else
        {
            if (ParserWrapper::IsMinInt(pnode))
            {
                var->SetVarType(AsmJsVarType::Int);
                var->SetLocation(mIntVarSpace.AcquireRegister());
                var->SetConstInitialiser(INT_MIN);
            }
            else if (ParserWrapper::IsUnsigned(pnode))
            {
                var->SetVarType(AsmJsVarType::Int);
                var->SetLocation(mIntVarSpace.AcquireRegister());
                var->SetConstInitialiser((int)((uint32)pnode->AsParseNodeFloat()->dbl));
            }
            else if (pnode->AsParseNodeFloat()->maybeInt)
            {
                // this means there was an int literal not in range [-2^31,3^32)
                return false;
            }
            else
            {
                var->SetVarType(AsmJsVarType::Double);
                var->SetLocation(mDoubleVarSpace.AcquireRegister());
                var->SetConstInitialiser(pnode->AsParseNodeFloat()->dbl);
            }
        }
        return true;
    }

    bool AsmJsModuleCompiler::AddGlobalVarImport(PropertyName name, PropertyName field, AsmJSCoercion coercion)
    {
        AsmJsConstantImport* var = Anew(&mAllocator, AsmJsConstantImport, name, field);
        if (!var)
        {
            return false;
        }
        if (!DefineIdentifier(name, var))
        {
            return false;
        }
        ++mVarImportCount;

        switch (coercion)
        {
        case Js::AsmJS_ToInt32:
            var->SetVarType(AsmJsVarType::Int);
            var->SetLocation(mIntVarSpace.AcquireRegister());
            break;
        case Js::AsmJS_ToNumber:
            var->SetVarType(AsmJsVarType::Double);
            var->SetLocation(mDoubleVarSpace.AcquireRegister());
            break;
        case Js::AsmJS_FRound:
            var->SetVarType(AsmJsVarType::Float);
            var->SetLocation(mFloatVarSpace.AcquireRegister());
            break;
        default:
            break;
        }

        return true;
    }

    bool AsmJsModuleCompiler::AddModuleFunctionImport(PropertyName name, PropertyName field)
    {
        AsmJsImportFunction* var = Anew(&mAllocator, AsmJsImportFunction, name, field, &mAllocator);
        if (!var)
        {
            return false;
        }
        if (!DefineIdentifier(name, var))
        {
            return false;
        }
        var->SetFunctionIndex(mImportFunctions.AcquireRegister());

        return true;
    }

    bool AsmJsModuleCompiler::AddNumericConst(PropertyName name, const double* cst)
    {
        AsmJsMathConst* var = Anew(&mAllocator, AsmJsMathConst, name, cst);
        if (!var)
        {
            return false;
        }
        if (!DefineIdentifier(name, var))
        {
            return false;
        }

        return true;
    }

    bool AsmJsModuleCompiler::AddArrayView(PropertyName name, ArrayBufferView::ViewType type)
    {
        AsmJsArrayView* view = Anew(&mAllocator, AsmJsArrayView, name, type);
        if (!view)
        {
            return false;
        }
        if (!DefineIdentifier(name, view))
        {
            return false;
        }
        mArrayViews.Enqueue(view);

        return true;
    }

    bool AsmJsModuleCompiler::AddFunctionTable(PropertyName name, const int size)
    {
        GetByteCodeGenerator()->AssignPropertyId(name);
        AsmJsFunctionTable* funcTable = Anew(&mAllocator, AsmJsFunctionTable, name, &mAllocator);
        if (!funcTable)
        {
            return false;
        }
        if (!DefineIdentifier(name, funcTable))
        {
            return false;
        }
        funcTable->SetSize(size);
        int pos = mFunctionTableArray.Add(funcTable);
        funcTable->SetFunctionIndex(pos);

        return true;
    }

    bool AsmJsModuleCompiler::AddExport(PropertyName name, RegSlot location)
    {
        AsmJsModuleExport * foundExport;
        if (mExports.TryGetReference(name->GetPropertyId(), &foundExport))
        {
            AsmJSCompiler::OutputMessage(GetScriptContext(), DEIT_GENERAL, _u("Warning: redefining export"));
            foundExport->location = location;
            return true;
        }
        else
        {
            AsmJsModuleExport ex;
            ex.id = name->GetPropertyId();
            ex.location = location;
            return mExports.Add(ex.id, ex) >= 0;
            // return is < 0 if count overflowed 31bits
        }
    }

    bool AsmJsModuleCompiler::SetExportFunc(AsmJsFunc* func)
    {
        Assert(mExports.Count() == 0 && func);
        mExportFuncIndex = func->GetFunctionIndex();
        return mExports.Count() == 0 && (uint32)mExportFuncIndex < (uint32)mFunctionArray.Count();
    }

    AsmJsFunctionDeclaration* AsmJsModuleCompiler::LookupFunction(PropertyName name)
    {
        return LookupIdentifier<AsmJsFunctionDeclaration>(name);
    }

    bool AsmJsModuleCompiler::AreAllFuncTableDefined()
    {
        const int size = mFunctionTableArray.Count();
        for (int i = 0; i < size; i++)
        {
            AsmJsFunctionTable* funcTable = mFunctionTableArray.Item(i);
            if (!funcTable->IsDefined())
            {
                AsmJSCompiler::OutputError(GetScriptContext(), _u("Function table %s was used in a function but does not appear in the module"), funcTable->GetName()->Psz());
                return false;
            }
        }
        return true;
    }

    void AsmJsModuleCompiler::UpdateMaxHeapAccess(uint index)
    {
        if (mMaxHeapAccess < index)
        {
            mMaxHeapAccess = index;
        }
    }

    void AsmJsModuleCompiler::InitMemoryOffsets()
    {
        mModuleMemory.mArrayBufferOffset = AsmJsModuleMemory::MemoryTableBeginOffset;
        mModuleMemory.mStdLibOffset = mModuleMemory.mArrayBufferOffset + 1;
        mModuleMemory.mDoubleOffset = mModuleMemory.mStdLibOffset + 1;
        mModuleMemory.mFuncOffset = mModuleMemory.mDoubleOffset + (mDoubleVarSpace.GetTotalVarCount() * WAsmJs::DOUBLE_SLOTS_SPACE);
        mModuleMemory.mFFIOffset = mModuleMemory.mFuncOffset + mFunctionArray.Count();
        mModuleMemory.mFuncPtrOffset = mModuleMemory.mFFIOffset + mImportFunctions.GetTotalVarCount();
        mModuleMemory.mFloatOffset = mModuleMemory.mFuncPtrOffset + GetFuncPtrTableCount();
        mModuleMemory.mIntOffset = mModuleMemory.mFloatOffset + (int32)(mFloatVarSpace.GetTotalVarCount() * WAsmJs::FLOAT_SLOTS_SPACE + 0.5);
        mModuleMemory.mMemorySize = mModuleMemory.mIntOffset + (int32)(mIntVarSpace.GetTotalVarCount() * WAsmJs::INT_SLOTS_SPACE + 0.5);
    }

    void AsmJsModuleCompiler::AccumulateCompileTime()
    {
        Js::TickDelta td;
        AsmJsCompileTime curTime = GetTick();
        td = curTime - mCompileTimeLastTick;
        mCompileTime = mCompileTime + td;
        mCompileTimeLastTick = curTime;
    }

    void AsmJsModuleCompiler::AccumulateCompileTime(AsmJsCompilation::Phases phase)
    {
        Js::TickDelta td;
        AsmJsCompileTime curTime = GetTick();
        td = curTime - mCompileTimeLastTick;
        mCompileTime = mCompileTime + td;
        mCompileTimeLastTick = curTime;
        mPhaseCompileTime[phase] = mPhaseCompileTime[phase] + td;
    }

    Js::AsmJsCompileTime AsmJsModuleCompiler::GetTick()
    {
        return Js::Tick::Now();
    }

    uint64 AsmJsModuleCompiler::GetCompileTime() const
    {
        return mCompileTime.ToMicroseconds();
    }

    static const char16* AsmPhaseNames[AsmJsCompilation::Phases_COUNT] = {
    _u("Module"),
    _u("ByteCode"),
    _u("TemplateJIT"),
    };

    void AsmJsModuleCompiler::PrintCompileTrace() const
    {
        // for testtrace, don't print time so that it can be used for baselines
        if (PHASE_TESTTRACE1(AsmjsPhase))
        {
            AsmJSCompiler::OutputMessage(GetScriptContext(), DEIT_ASMJS_SUCCEEDED, _u("Successfully compiled asm.js code"));
        }
        else
        {
            uint64 us = GetCompileTime();
            uint64 ms = us / 1000;
            us = us % 1000;
            AsmJSCompiler::OutputMessage(GetScriptContext(), DEIT_ASMJS_SUCCEEDED, _u("Successfully compiled asm.js code (total compilation time %llu.%llums)"), ms, us);
        }

        if (PHASE_TRACE1(AsmjsPhase))
        {
            for (int i = 0; i < AsmJsCompilation::Phases_COUNT; i++)
            {
                uint64 us = mPhaseCompileTime[i].ToMicroseconds();
                uint64 ms = us / 1000;
                us = us % 1000;
                Output::Print(_u("%20s : %llu.%llums\n"), AsmPhaseNames[i], ms, us);
            }
            Output::Flush();
        }
    }

    BVStatic<ASMMATH_BUILTIN_SIZE> AsmJsModuleCompiler::GetAsmMathBuiltinUsedBV()
    {
        return mAsmMathBuiltinUsedBV;
    }

    BVStatic<ASMARRAY_BUILTIN_SIZE> AsmJsModuleCompiler::GetAsmArrayBuiltinUsedBV()
    {
        return mAsmArrayBuiltinUsedBV;
    }

    void AsmJsModuleInfo::SetFunctionCount(int val)
    {
        Assert(mFunctions == nullptr);
        mFunctionCount = val;
        mFunctions = RecyclerNewArray(mRecycler, ModuleFunction, val);
    }

    void AsmJsModuleInfo::SetFunctionTableCount(int val)
    {
        Assert(mFunctionTables == nullptr);
        mFunctionTableCount = val;
        mFunctionTables = RecyclerNewArray(mRecycler, ModuleFunctionTable, val);
    }

    void AsmJsModuleInfo::SetFunctionImportCount(int val)
    {
        Assert(mFunctionImports == nullptr);
        mFunctionImportCount = val;
        mFunctionImports = RecyclerNewArray(mRecycler, ModuleFunctionImport, val);
    }

    void AsmJsModuleInfo::SetVarCount(int val)
    {
        Assert(mVars == nullptr);
        mVarCount = val;
        mVars = RecyclerNewArray(mRecycler, ModuleVar, val);
    }

    void AsmJsModuleInfo::SetVarImportCount(int val)
    {
        Assert(mVarImports == nullptr);
        mVarImportCount = val;
        mVarImports = RecyclerNewArray(mRecycler, ModuleVarImport, val);
    }

    void AsmJsModuleInfo::SetExportsCount(int count)
    {
        if (count)
        {
            mExports = RecyclerNewPlus(mRecycler, count * sizeof(PropertyId), PropertyIdArray, count, 0);
            mExportsFunctionLocation = RecyclerNewArray(mRecycler, RegSlot, count);
        }
        mExportsCount = count;
    }

    void AsmJsModuleInfo::InitializeSlotMap(int val)
    {
        Assert(mSlotMap == nullptr);
        mSlotsCount = val;
        mSlotMap = RecyclerNew(mRecycler, AsmJsSlotMap, mRecycler);
    }

    void AsmJsModuleInfo::SetFunctionTableSize(int index, uint size)
    {
        Assert(mFunctionTables != nullptr);
        Assert(index < mFunctionTableCount);
        ModuleFunctionTable& table = mFunctionTables[index];
        table.size = size;
        table.moduleFunctionIndex = RecyclerNewArray(mRecycler, RegSlot, size);
    }

    void AsmJsModuleInfo::EnsureHeapAttached(ScriptFunction * func)
    {
#ifdef ENABLE_WASM
        if (VarIs<WasmScriptFunction>(func))
        {
            WasmScriptFunction* wasmFunc = VarTo<WasmScriptFunction>(func);
            WebAssemblyMemory * wasmMem = wasmFunc->GetWebAssemblyMemory();
            if (wasmMem && wasmMem->GetBuffer() && wasmMem->GetBuffer()->IsDetached())
            {
                Throw::OutOfMemory();
            }
        }
        else
#endif
        {
            AsmJsScriptFunction* asmFunc = VarTo<AsmJsScriptFunction>(func);
            ArrayBuffer* moduleArrayBuffer = asmFunc->GetAsmJsArrayBuffer();
            if (moduleArrayBuffer && moduleArrayBuffer->IsDetached())
            {
                Throw::OutOfMemory();
            }
        }

    }

    void * AsmJsModuleInfo::ConvertFrameForJavascript(void* env, AsmJsScriptFunction* func)
    {
        FunctionBody * body = func->GetFunctionBody();
        AsmJsFunctionInfo * asmFuncInfo = body->GetAsmJsFunctionInfo();
        FunctionBody * moduleBody = asmFuncInfo->GetModuleFunctionBody();
        AsmJsModuleInfo * asmModuleInfo = moduleBody->GetAsmJsModuleInfo();
        Assert(asmModuleInfo);

        ScriptContext * scriptContext = func->GetScriptContext();
        // AsmJsModuleEnvironment is all laid out here
        Var * asmJsEnvironment = static_cast<Var*>(env);
        Var * asmBufferPtr = asmJsEnvironment + asmModuleInfo->GetModuleMemory().mArrayBufferOffset;
        ArrayBuffer * asmBuffer = *asmBufferPtr ? VarTo<ArrayBuffer>(*asmBufferPtr) : nullptr;

        Var stdLibObj = *(asmJsEnvironment + asmModuleInfo->GetModuleMemory().mStdLibOffset);
        Var asmMathObject = stdLibObj ? JavascriptOperators::OP_GetProperty(stdLibObj, PropertyIds::Math, scriptContext) : nullptr;

        Var * asmFFIs = asmJsEnvironment + asmModuleInfo->GetModuleMemory().mFFIOffset;
        Var * asmFuncs = asmJsEnvironment + asmModuleInfo->GetModuleMemory().mFuncOffset;
        Var ** asmFuncPtrs = reinterpret_cast<Var**>(asmJsEnvironment + asmModuleInfo->GetModuleMemory().mFuncPtrOffset);

        double * asmDoubleVars = reinterpret_cast<double*>(asmJsEnvironment + asmModuleInfo->GetModuleMemory().mDoubleOffset);
        int * asmIntVars = reinterpret_cast<int*>(asmJsEnvironment + asmModuleInfo->GetModuleMemory().mIntOffset);
        float * asmFloatVars = reinterpret_cast<float*>(asmJsEnvironment + asmModuleInfo->GetModuleMemory().mFloatOffset);

#if DEBUG
        Field(Var) * slotArray = RecyclerNewArrayZ(scriptContext->GetRecycler(), Field(Var), moduleBody->scopeSlotArraySize + ScopeSlots::FirstSlotIndex);
#else
        Field(Var) * slotArray = RecyclerNewArray(scriptContext->GetRecycler(), Field(Var), moduleBody->scopeSlotArraySize + ScopeSlots::FirstSlotIndex);
#endif
        ScopeSlots scopeSlots(slotArray);
        scopeSlots.SetCount(moduleBody->scopeSlotArraySize);
        scopeSlots.SetScopeMetadata(moduleBody->GetFunctionInfo());

        auto asmSlotMap = asmModuleInfo->GetAsmJsSlotMap();
        Assert((uint)asmModuleInfo->GetSlotsCount() >= moduleBody->scopeSlotArraySize);

        Js::ActivationObject* activeScopeObject = nullptr;
        if (moduleBody->GetObjectRegister() != 0)
        {
            activeScopeObject = static_cast<ActivationObject*>(scriptContext->GetLibrary()->CreateActivationObject());
        }

        PropertyId* propertyIdArray = moduleBody->GetPropertyIdsForScopeSlotArray();
        uint slotsCount = moduleBody->scopeSlotArraySize;
        for (uint i = 0; i < slotsCount; ++i)
        {
            AsmJsSlot * asmSlot = nullptr;
            bool found = asmSlotMap->TryGetValue(propertyIdArray[i], &asmSlot);
            // we should have everything we need in the map
            Assert(found);
            Var value = nullptr;
            switch (asmSlot->symType)
            {
            case AsmJsSymbol::ConstantImport:
            case AsmJsSymbol::Variable:
            {
                switch (asmSlot->varType)
                {
                case AsmJsVarType::Double:
                    value = JavascriptNumber::NewWithCheck(asmDoubleVars[asmSlot->location], scriptContext);
                    break;
                case AsmJsVarType::Float:
                    value = JavascriptNumber::NewWithCheck(asmFloatVars[asmSlot->location], scriptContext);
                    break;
                case AsmJsVarType::Int:
                    value = JavascriptNumber::ToVar(asmIntVars[asmSlot->location], scriptContext);
                    break;
                default:
                    Assume(UNREACHED);
                }
                break;
            }
            case AsmJsSymbol::ModuleArgument:
            {
                switch (asmSlot->argType)
                {
                case AsmJsModuleArg::ArgType::StdLib:
                    value = stdLibObj;
                    break;
                case AsmJsModuleArg::ArgType::Import:
                    // we can't reference this inside functions (and don't hold onto it), but must set to something, so set it to be undefined
                    value = scriptContext->GetLibrary()->GetUndefined();
                    break;
                case AsmJsModuleArg::ArgType::Heap:
                    value = asmBuffer;
                    break;
                default:
                    Assume(UNREACHED);
                }
                break;
            }
            case AsmJsSymbol::ImportFunction:
                value = asmFFIs[asmSlot->location];
                break;
            case AsmJsSymbol::FuncPtrTable:
                value = JavascriptArray::OP_NewScArrayWithElements(asmSlot->funcTableSize, asmFuncPtrs[asmSlot->location], scriptContext);
                break;
            case AsmJsSymbol::ModuleFunction:
                value = asmFuncs[asmSlot->location];
                break;
            case AsmJsSymbol::MathConstant:
                value = JavascriptNumber::NewWithCheck(asmSlot->mathConstVal, scriptContext);
                break;
            case AsmJsSymbol::ClosureFunction:
                // we can't reference this inside functions but must set to something, so set it to be undefined
                value = scriptContext->GetLibrary()->GetUndefined();
                break;
            case AsmJsSymbol::ArrayView:
            {
                AnalysisAssert(asmBuffer);
#ifdef _M_X64
                const bool isOptimizedBuffer = true;
#elif _M_IX86
                const bool isOptimizedBuffer = false;
#else
                Assert(UNREACHED);
                const bool isOptimizedBuffer = false;
#endif
                Assert(isOptimizedBuffer == asmBuffer->IsValidVirtualBufferLength(asmBuffer->GetByteLength()));
                switch (asmSlot->viewType)
                {
                case ArrayBufferView::TYPE_FLOAT32:
                    value = TypedArray<float, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 2, scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_FLOAT64:
                    value = TypedArray<double, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 3, scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_INT8:
                    value = TypedArray<int8, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength(), scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_INT16:
                    value = TypedArray<int16, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 1, scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_INT32:
                    value = TypedArray<int32, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 2, scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_UINT8:
                    value = TypedArray<uint8, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength(), scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_UINT16:
                    value = TypedArray<uint16, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 1, scriptContext->GetLibrary());
                    break;
                case ArrayBufferView::TYPE_UINT32:
                    value = TypedArray<uint32, false, isOptimizedBuffer>::Create(asmBuffer, 0, asmBuffer->GetByteLength() >> 2, scriptContext->GetLibrary());
                    break;
                default:
                    Assume(UNREACHED);
                }
                break;
            }
            case AsmJsSymbol::MathBuiltinFunction:
            {
                switch (asmSlot->builtinMathFunc)
                {
#define ASMJS_MATH_FUNC_NAMES(name, propertyName, funcInfo) \
            case AsmJSMathBuiltin_##name: \
                value = JavascriptOperators::OP_GetProperty(asmMathObject, PropertyIds::##propertyName, scriptContext); \
                break;
#include "AsmJsBuiltInNames.h"
                default:
                    Assume(UNREACHED);
                }
                break;
            }
            case AsmJsSymbol::TypedArrayBuiltinFunction:
                switch (asmSlot->builtinArrayFunc)
                {
#define ASMJS_ARRAY_NAMES(name, propertyName) \
            case AsmJSTypedArrayBuiltin_##name: \
                value = JavascriptOperators::OP_GetProperty(stdLibObj, PropertyIds::##propertyName, scriptContext); \
                break;
#include "AsmJsBuiltInNames.h"
                default:
                    Assume(UNREACHED);
                }
                break;

            default:
                Assume(UNREACHED);
            }
            if (activeScopeObject != nullptr)
            {
                activeScopeObject->SetPropertyWithAttributes(
                    propertyIdArray[i],
                    value,
                    asmSlot->isConstVar ? PropertyConstDefaults : PropertyDynamicTypeDefaults,
                    nullptr);
            }
            else
            {
                // ensure we aren't multiply writing to a slot
                Assert(scopeSlots.Get(i) == nullptr);
                scopeSlots.Set(i, value);
            }
        }

        if (activeScopeObject != nullptr)
        {
            return (void*)activeScopeObject;
        }
        else
        {
            return (void*)slotArray;
        }
    }
};
#endif
