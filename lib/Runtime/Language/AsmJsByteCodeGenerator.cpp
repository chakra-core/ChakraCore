//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#ifdef ASMJS_PLAT
#include "ByteCode/Symbol.h"
#include "ByteCode/FuncInfo.h"
#ifdef DBG_DUMP
#include "ByteCode/ByteCodeDumper.h"
#include "ByteCode/AsmJsByteCodeDumper.h"
#endif
#include "ByteCode/ByteCodeWriter.h"
#include "ByteCode/ByteCodeGenerator.h"
#include "ByteCode/AsmJsByteCodeWriter.h"
#include "Language/AsmJsByteCodeGenerator.h"


namespace Js
{
    enum EBinaryMathOpCodes: int
    {
        BMO_ADD,
        BMO_SUB,
        BMO_MUL,
        BMO_DIV,
        BMO_REM,

        BMO_MAX,
    };

    enum EBinaryMathOpCodesTypes: int
    {
        BMOT_Int,
        BMOT_UInt,
        BMOT_Float,
        BMOT_Double,
        BMOT_MAX
    };

    const OpCodeAsmJs BinaryMathOpCodes[BMO_MAX][BMOT_MAX] = {
        /*BMO_ADD*/{ OpCodeAsmJs::Add_Int, OpCodeAsmJs::Add_Int, OpCodeAsmJs::Add_Flt, OpCodeAsmJs::Add_Db },
        /*BMO_SUB*/{ OpCodeAsmJs::Sub_Int, OpCodeAsmJs::Sub_Int, OpCodeAsmJs::Sub_Flt, OpCodeAsmJs::Sub_Db },
        /*BMO_MUL*/{ OpCodeAsmJs::Mul_Int, OpCodeAsmJs::Mul_Int, OpCodeAsmJs::Mul_Flt, OpCodeAsmJs::Mul_Db },
        /*BMO_DIV*/{ OpCodeAsmJs::Div_Int, OpCodeAsmJs::Div_UInt,OpCodeAsmJs::Div_Flt, OpCodeAsmJs::Div_Db },
        /*BMO_REM*/{ OpCodeAsmJs::Rem_Int, OpCodeAsmJs::Rem_UInt,OpCodeAsmJs::Nop,     OpCodeAsmJs::Rem_Db }
    };

    enum EBinaryComparatorOpCodes: int
    {
        /*<, <=, >, >=, ==, !=*/
        BCO_LT,
        BCO_LE,
        BCO_GT,
        BCO_GE,
        BCO_EQ,
        BCO_NE,

        BCO_MAX,
    };

    enum EBinaryComparatorOpCodesTypes
    {
        BCOT_Int,
        BCOT_UInt,
        BCOT_Float,
        BCOT_Double,
        BCOT_MAX
    };

    const OpCodeAsmJs BinaryComparatorOpCodes[BCO_MAX][BCOT_MAX] = {
                    //  int            unsigned int     double
        /*BCO_LT*/{ OpCodeAsmJs::CmLt_Int, OpCodeAsmJs::CmLt_UInt, OpCodeAsmJs::CmLt_Flt, OpCodeAsmJs::CmLt_Db },
        /*BCO_LE*/{ OpCodeAsmJs::CmLe_Int, OpCodeAsmJs::CmLe_UInt, OpCodeAsmJs::CmLe_Flt, OpCodeAsmJs::CmLe_Db },
        /*BCO_GT*/{ OpCodeAsmJs::CmGt_Int, OpCodeAsmJs::CmGt_UInt, OpCodeAsmJs::CmGt_Flt, OpCodeAsmJs::CmGt_Db },
        /*BCO_GE*/{ OpCodeAsmJs::CmGe_Int, OpCodeAsmJs::CmGe_UInt, OpCodeAsmJs::CmGe_Flt, OpCodeAsmJs::CmGe_Db },
        /*BCO_EQ*/{ OpCodeAsmJs::CmEq_Int, OpCodeAsmJs::CmEq_Int, OpCodeAsmJs::CmEq_Flt, OpCodeAsmJs::CmEq_Db },
        /*BCO_NE*/{ OpCodeAsmJs::CmNe_Int, OpCodeAsmJs::CmNe_Int, OpCodeAsmJs::CmNe_Flt, OpCodeAsmJs::CmNe_Db },
    };


#define CheckNodeLocation(info,type) if(!mFunction->IsValidLocation<type>(&info)){\
    throw AsmJsCompilationException( _u("Invalid Node location[%d] "), info.location ); }


    AsmJSByteCodeGenerator::AsmJSByteCodeGenerator( AsmJsFunc* func, AsmJsModuleCompiler* compiler ) :
        mFunction( func )
        , mAllocator(_u("AsmjsByteCode"), compiler->GetScriptContext()->GetThreadContext()->GetPageAllocator(), Throw::OutOfMemory)
        , mInfo( mFunction->GetFuncInfo() )
        , mCompiler( compiler )
        , mByteCodeGenerator(mCompiler->GetByteCodeGenerator())
    {
        mWriter.Create();

        const int32 astSize = func->GetFncNode()->astSize/AstBytecodeRatioEstimate;
        // Use the temp allocator in bytecode write temp buffer.
        mWriter.InitData(&mAllocator, astSize);

#ifdef LOG_BYTECODE_AST_RATIO
        // log the max Ast size
        Output::Print(_u("Max Ast size: %d"), astSize);
#endif
    }

    bool AsmJSByteCodeGenerator::BlockHasOwnScope( ParseNode* pnodeBlock )
    {
        Assert( pnodeBlock->nop == knopBlock );
        return pnodeBlock->AsParseNodeBlock()->scope != nullptr && ( !( pnodeBlock->grfpn & fpnSyntheticNode ) );
    }

    template<typename T> byte* AsmJSByteCodeGenerator::SetConstsToTable(byte* byteTable, T zeroValue)
    {
        T* typedTable = (T*)byteTable;
        // Return Register
        *typedTable = zeroValue;
        typedTable++;

        JsUtil::BaseDictionary<T, RegSlot, ArenaAllocator, PowerOf2SizePolicy, AsmJsComparer> intMap = mFunction->GetRegisterSpace<T>().GetConstMap();

        for (auto it = intMap.GetIterator(); it.IsValid(); it.MoveNext())
        {
            *typedTable = it.Current().Key();
            typedTable++;
        }
        return (byte*)typedTable;
    }

    // copy all constants from reg spaces to function body.
    void AsmJSByteCodeGenerator::LoadAllConstants()
    {
        FunctionBody *funcBody = mFunction->GetFuncBody();
        funcBody->CreateConstantTable();
        auto table = funcBody->GetConstTable();
        byte* tableEnd = (byte*)(table + funcBody->GetConstantCount());

        WAsmJs::TypedConstSourcesInfo constSourcesInfo = mFunction->GetTypedRegisterAllocator().GetConstSourceInfos();
        for (int i = 0; i < WAsmJs::LIMIT; ++i)
        {
            WAsmJs::Types type = (WAsmJs::Types)i;
            uint32 srcByteOffset = constSourcesInfo.srcByteOffsets[i];
            byte* byteTable = ((byte*)table) + srcByteOffset;
            if (srcByteOffset != Js::Constants::InvalidOffset)
            {
                switch (type)
                {
                case WAsmJs::INT32: byteTable = SetConstsToTable<int>(byteTable, 0); break;
                case WAsmJs::FLOAT32: byteTable = SetConstsToTable<float>(byteTable, 0); break;
                case WAsmJs::FLOAT64: byteTable = SetConstsToTable<double>(byteTable, 0); break;
#if TARGET_64
                case WAsmJs::INT64: SetConstsToTable<int64>(byteTable, 0); break;
#endif
                case WAsmJs::SIMD:
                {
                    AsmJsSIMDValue zeroValue;
                    zeroValue.f64[0] = 0; zeroValue.f64[1] = 0;
                    byteTable = SetConstsToTable<AsmJsSIMDValue>(byteTable, zeroValue);
                    break;
                }
                default:
                    Assert(false);
                    break;
                }
                if (byteTable > tableEnd)
                {
                    Assert(UNREACHED);
                    Js::Throw::FatalInternalError();
                }
            }
        }
    }

    void AsmJSByteCodeGenerator::FinalizeRegisters( FunctionBody* byteCodeFunction )
    {
        mFunction->CommitToFunctionBody(byteCodeFunction);

        // add 3 for each of I0, F0, and D0
        RegSlot regCount = mInfo->RegCount() + 3 + AsmJsFunctionMemory::RequiredVarConstants;
        byteCodeFunction->SetFirstTmpReg(regCount);
    }

    bool AsmJSByteCodeGenerator::EmitOneFunction()
    {
        Assert(mFunction->GetFncNode());
        Assert(mFunction->GetBodyNode());
        AsmJsFunctionCompilation autoCleanup( this );
        try
        {
            ParseNode* pnode = mFunction->GetFncNode();
            Assert( pnode && pnode->nop == knopFncDecl );
            Assert( mInfo != nullptr );

            ByteCodeGenerator* byteCodeGen = GetOldByteCodeGenerator();
            MaybeTodo( mInfo->IsFakeGlobalFunction( byteCodeGen->GetFlags() ) );

            // Support default arguments ?
            MaybeTodo( pnode->AsParseNodeFnc()->HasDefaultArguments() );

            FunctionBody* functionBody = mFunction->GetFuncBody();
            functionBody->SetStackNestedFunc( false );

            FinalizeRegisters(functionBody);

            ArenaAllocator* alloc = byteCodeGen->GetAllocator();
            mInfo->inlineCacheMap = Anew( alloc, FuncInfo::InlineCacheMap,
                                          alloc,
                                          mInfo->RegCount()   // Pass the actual register count. TODO: Check if we can reduce this count
                                          );
            mInfo->rootObjectLoadInlineCacheMap = Anew( alloc, FuncInfo::RootObjectInlineCacheIdMap,
                                                        alloc,
                                                        10 );
            mInfo->rootObjectStoreInlineCacheMap = Anew( alloc, FuncInfo::RootObjectInlineCacheIdMap,
                                                         alloc,
                                                         10 );
            mInfo->referencedPropertyIdToMapIndex = Anew( alloc, FuncInfo::RootObjectInlineCacheIdMap,
                                                          alloc,
                                                          10 );
            functionBody->AllocateLiteralRegexArray();


            mWriter.Begin(functionBody, alloc, true /* byteCodeGen->DoJitLoopBodies( funcInfo )*/, mInfo->hasLoop, false /* inDebugMode*/);

            // for now, emit all constant loads at top of function (should instead put in
            // closest dominator of uses)
            LoadAllConstants();
            DefineLabels( );
            EmitAsmJsFunctionBody();

            // Set that the function is asmjsFunction in functionBody here so that Initialize ExecutionMode call later will check for that and not profile in asmjsMode
            functionBody->SetIsAsmJsFunction(true);
            functionBody->SetIsAsmjsMode(true);

            // Do a uint32 add just to verify that we haven't overflowed the reg slot type.
            UInt32Math::Add( mFunction->GetRegisterSpace<int>().GetTotalVarCount(), mFunction->GetRegisterSpace<int>().GetConstCount());
            UInt32Math::Add( mFunction->GetRegisterSpace<double>().GetTotalVarCount(), mFunction->GetRegisterSpace<double>().GetConstCount());
            UInt32Math::Add( mFunction->GetRegisterSpace<float>().GetTotalVarCount(), mFunction->GetRegisterSpace<float>().GetConstCount());

            byteCodeGen->MapCacheIdsToPropertyIds( mInfo );
            byteCodeGen->MapReferencedPropertyIds( mInfo );

            mWriter.SetCallSiteCount(mFunction->GetProfileIdCount());
            mWriter.End();
            autoCleanup.FinishCompilation();

            functionBody->SetInitialDefaultEntryPoint();
        }
        catch( AsmJsCompilationException& e )
        {
            PrintAsmJsCompilationError( e.msg() );
            return false;
        }
        return true;
    }


    void AsmJSByteCodeGenerator::PrintAsmJsCompilationError(__out_ecount(256)  char16* msg)
    {
        uint offset = mWriter.GetCurrentOffset();
        ULONG line = 0;
        LONG col = 0;
        if (!mFunction->GetFuncBody()->GetLineCharOffset(offset, &line, &col))
        {
            line = 0;
            col = 0;
        }

        LPCOLESTR NoneName = _u("None");
        LPCOLESTR moduleName = NoneName;
        if (mCompiler->GetModuleFunctionName())
        {
            moduleName = mCompiler->GetModuleFunctionName()->Psz();
        }

        char16 filename[_MAX_FNAME];
        char16 ext[_MAX_EXT];
        bool hasURL = mFunction->GetFuncBody()->GetSourceContextInfo()->url != nullptr;
        Assert(hasURL || mFunction->GetFuncBody()->GetSourceContextInfo()->IsDynamic());
        if (hasURL)
        {
            _wsplitpath_s(mFunction->GetFuncBody()->GetSourceContextInfo()->url, NULL, 0, NULL, 0, filename, _MAX_FNAME, ext, _MAX_EXT);
        }
        AsmJSCompiler::OutputError(
            mCompiler->GetScriptContext(),
            _u("\n%s%s(%d, %d)\n\tAsm.js Compilation Error function : %s::%s\n\t%s\n"),
            hasURL ? filename : _u("[Dynamic code]"),
            hasURL ? ext : _u(""),
            line + 1,
            col + 1,
            moduleName,
            mFunction->GetName()->Psz(),
            msg);
    }

    void AsmJSByteCodeGenerator::DefineLabels()
    {
        mInfo->singleExit=mWriter.DefineLabel();
        SList<ParseNodeStmt *>::Iterator iter(&mInfo->targetStatements);
        while (iter.Next())
        {
            ParseNodeStmt * node = iter.Data();
            node->breakLabel=mWriter.DefineLabel();
            node->continueLabel=mWriter.DefineLabel();
            node->emitLabels=true;
        }
    }

    void AsmJSByteCodeGenerator::EmitAsmJsFunctionBody()
    {
        ParseNode *pnodeBody = mFunction->GetBodyNode();
        ParseNode *varStmts = pnodeBody;

        // Emit local var declarations: Load of constants to variables.
        while (varStmts->nop == knopList)
        {
            ParseNode * pnode = ParserWrapper::GetBinaryLeft(varStmts);
            while (pnode && pnode->nop != knopEndCode)
            {
                ParseNode * decl;
                if (pnode->nop == knopList)
                {
                    decl = ParserWrapper::GetBinaryLeft(pnode);
                    pnode = ParserWrapper::GetBinaryRight(pnode);
                }
                else
                {
                    decl = pnode;
                    pnode = nullptr;
                }

                if (decl->nop != knopVarDecl)
                {
                    goto varDeclEnd;
                }

                Assert(decl->nop == knopVarDecl);

                // since we are parsing the same way we created variables the same time, it is safe to assume these are AsmJsVar*
                AsmJsVar* var = (AsmJsVar*)mFunction->FindVar(ParserWrapper::VariableName(decl));
                AnalysisAssert(var);
                if (var->GetType().isInt())
                {
                    mWriter.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, var->GetLocation(), var->GetIntInitialiser());
                }
                else
                {
                    AsmJsVar * initSource = nullptr;
                    if (decl->AsParseNodeVar()->pnodeInit->nop == knopName)
                    {
                        AsmJsSymbol * initSym = mCompiler->LookupIdentifier(decl->AsParseNodeVar()->pnodeInit->name(), mFunction);
                        if (AsmJsVar::Is(initSym))
                        {
                            // in this case we are initializing with value of a constant var
                            initSource = AsmJsVar::FromSymbol(initSym);
                        }
                        else
                        {
                            Assert(initSym->GetType() == AsmJsType::Double);
                            AsmJsMathConst* initConst = AsmJsMathConst::FromSymbol(initSym);
                            mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Db, var->GetLocation(), mFunction->GetConstRegister<double>(*initConst->GetVal()));
                        }
                    }
                    else
                    {
                        initSource = var;
                    }
                    if (initSource)
                    {
                        if (var->GetType().isDouble())
                        {
                            mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Db, var->GetLocation(), mFunction->GetConstRegister<double>(initSource->GetDoubleInitialiser()));
                        }
                        else if (var->GetType().isFloat())
                        {
                            mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Flt, var->GetLocation(), mFunction->GetConstRegister<float>(initSource->GetFloatInitialiser()));
                        }
                        else
                        {
                            throw AsmJsCompilationException(_u("Unexpected variable type"));
                        }
                    }
                }
            }
                varStmts = ParserWrapper::GetBinaryRight(varStmts);
        }
    varDeclEnd:

        // Emit a function body. Only explicit returns and the implicit "undef" at the bottom
        // get copied to the return register.

        ParseNode *stmt = nullptr;
        while (varStmts->nop == knopList)
        {
            stmt = ParserWrapper::GetBinaryLeft(varStmts);
            EmitTopLevelStatement( stmt );
            varStmts = ParserWrapper::GetBinaryRight(varStmts);
        }
        Assert(!varStmts->CapturesSyms());

        // if last statement isn't return, type must be void
        if (!stmt || stmt->nop != knopReturn)
        {
            if (!mFunction->CheckAndSetReturnType(AsmJsRetType::Void))
            {
                throw AsmJsCompilationException(_u("Expected function return type to be void got %s instead"), mFunction->GetReturnType().toType().toChars());
            }
        }
        EmitTopLevelStatement(varStmts);
    }

    void AsmJSByteCodeGenerator::EmitTopLevelStatement( ParseNode *stmt )
    {
        if( stmt->nop == knopFncDecl && stmt->AsParseNodeFnc()->IsDeclaration() )
        {
            throw AsmJsCompilationException( _u("Cannot declare functions inside asm.js functions") );
        }
        const EmitExpressionInfo& info = Emit( stmt );
        // free tmp register here
        mFunction->ReleaseLocationGeneric( &info );
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::Emit( ParseNode *pnode )
    {
        if (!ThreadContext::IsCurrentStackAvailable(Js::Constants::MinStackCompile))
        {
            throw AsmJsCompilationException(_u("Out of stack space"));
        }

        if( !pnode )
        {
            return EmitExpressionInfo( AsmJsType::Void );
        }
        switch( pnode->nop )
        {
        case knopReturn:
            return EmitReturn( pnode );
        case knopList:{
            while( pnode && pnode->nop == knopList )
            {
                const EmitExpressionInfo& info = Emit( ParserWrapper::GetBinaryLeft( pnode ) );
                mFunction->ReleaseLocationGeneric( &info );
                pnode = ParserWrapper::GetBinaryRight( pnode );
            }
            return Emit( pnode );
        }
        case knopComma:{
            const EmitExpressionInfo& info = Emit( ParserWrapper::GetBinaryLeft( pnode ) );
            mFunction->ReleaseLocationGeneric( &info );
            return Emit( ParserWrapper::GetBinaryRight( pnode ) );
        }
        case knopBlock:
        {
            ParseNodeBlock * pnodeBlock = pnode->AsParseNodeBlock();
            EmitExpressionInfo info = Emit(pnodeBlock->pnodeStmt);
            if (pnodeBlock->emitLabels)
            {
                mWriter.MarkAsmJsLabel(pnodeBlock->breakLabel);
            }
            return info;
        }
        case knopCall:
            return EmitCall( pnode );
        case knopPos:
            return EmitUnaryPos( pnode );
        case knopNeg:
            return EmitUnaryNeg( pnode );
        case knopNot:
            return EmitUnaryNot( pnode );
        case knopLogNot:
            return EmitUnaryLogNot( pnode );
        case knopEq:
            return EmitBinaryComparator( pnode, BCO_EQ );
        case knopNe:
            return EmitBinaryComparator( pnode, BCO_NE );
        case knopLt:
            return EmitBinaryComparator( pnode, BCO_LT );
        case knopLe:
            return EmitBinaryComparator( pnode, BCO_LE );
        case knopGe:
            return EmitBinaryComparator( pnode, BCO_GE );
        case knopGt:
            return EmitBinaryComparator( pnode, BCO_GT );
        case knopOr:
            return EmitBinaryInt( pnode, OpCodeAsmJs::Or_Int );
        case knopXor:
            return EmitBinaryInt( pnode, OpCodeAsmJs::Xor_Int );
        case knopAnd:
            return EmitBinaryInt( pnode, OpCodeAsmJs::And_Int );
        case knopLsh:
            return EmitBinaryInt( pnode, OpCodeAsmJs::Shl_Int );
        case knopRsh:
            return EmitBinaryInt( pnode, OpCodeAsmJs::Shr_Int );
        case knopRs2:
            return EmitBinaryInt( pnode, OpCodeAsmJs::Shr_UInt );
        case knopMod:
            return EmitBinaryMultiType( pnode, BMO_REM );
        case knopDiv:
            return EmitBinaryMultiType( pnode, BMO_DIV );
        case knopMul:
            return EmitBinaryMultiType( pnode, BMO_MUL );
        case knopSub:
            return EmitBinaryMultiType( pnode, BMO_SUB );
        case knopAdd:
            return EmitBinaryMultiType( pnode, BMO_ADD );
        case knopName:
        case knopStr:
            return EmitIdentifier( pnode );
        case knopIndex:
            return EmitLdArrayBuffer( pnode );
        case knopEndCode:
            StartStatement(pnode);
            mWriter.MarkAsmJsLabel( mFunction->GetFuncInfo()->singleExit );
            mWriter.EmptyAsm( OpCodeAsmJs::Ret );
            EndStatement(pnode);
            break;
        case knopAsg:
            return EmitAssignment( pnode );
        case knopFlt:
            if (ParserWrapper::IsMinInt(pnode))
            {
                return EmitExpressionInfo(mFunction->GetConstRegister<int>(INT32_MIN), AsmJsType::Signed);
            }
            else if (ParserWrapper::IsUnsigned(pnode))
            {
                return EmitExpressionInfo(mFunction->GetConstRegister<int>((uint32)pnode->AsParseNodeFloat()->dbl), AsmJsType::Unsigned);
            }
            else if (pnode->AsParseNodeFloat()->maybeInt)
            {
                throw AsmJsCompilationException(_u("Int literal must be in the range [-2^31, 2^32)"));
            }
            else
            {
                return EmitExpressionInfo(mFunction->GetConstRegister<double>(pnode->AsParseNodeFloat()->dbl), AsmJsType::DoubleLit);
            }
        case knopInt:
            if (pnode->AsParseNodeInt()->lw < 0)
            {
                return EmitExpressionInfo(mFunction->GetConstRegister<int>(pnode->AsParseNodeInt()->lw), AsmJsType::Signed);
            }
            else
            {
                return EmitExpressionInfo(mFunction->GetConstRegister<int>(pnode->AsParseNodeInt()->lw), AsmJsType::Fixnum);
            }
        case knopIf:
            return EmitIf( pnode->AsParseNodeIf() );
        case knopQmark:
            return EmitQMark( pnode );
        case knopSwitch:
            return EmitSwitch( pnode->AsParseNodeSwitch() );
        case knopFor:
            MaybeTodo( pnode->AsParseNodeFor()->pnodeInverted != NULL );
            {
                ParseNodeFor * pnodeFor = pnode->AsParseNodeFor();
                const EmitExpressionInfo& initInfo = Emit(pnodeFor->pnodeInit );
                mFunction->ReleaseLocationGeneric( &initInfo );
                return EmitLoop( pnodeFor,
                          pnodeFor->pnodeCond,
                          pnodeFor->pnodeBody,
                          pnodeFor->pnodeIncr);
            }
            break;
        case knopWhile:
        {
            ParseNodeWhile * pnodeWhile = pnode->AsParseNodeWhile();
            return EmitLoop( pnodeWhile,
                      pnodeWhile->pnodeCond,
                      pnodeWhile->pnodeBody,
                      nullptr);
        }
        case knopDoWhile:
        {
            ParseNodeWhile * pnodeWhile = pnode->AsParseNodeWhile();
            return EmitLoop( pnodeWhile,
                      pnodeWhile->pnodeCond,
                      pnodeWhile->pnodeBody,
                      NULL,
                      true );
        }
        case knopBreak:
        {
            ParseNodeJump * pnodeJump = pnode->AsParseNodeJump();
            Assert(pnodeJump->pnodeTarget->emitLabels);
            StartStatement(pnodeJump);
            mWriter.AsmBr(pnodeJump->pnodeTarget->breakLabel);
            if (pnodeJump->emitLabels)
            {
                mWriter.MarkAsmJsLabel(pnodeJump->breakLabel);
            }
            EndStatement(pnodeJump);
            break;
        }
        case knopContinue:
            Assert( pnode->AsParseNodeJump()->pnodeTarget->emitLabels );
            StartStatement(pnode);
            mWriter.AsmBr( pnode->AsParseNodeJump()->pnodeTarget->continueLabel );
            EndStatement(pnode);
            break;
        case knopVarDecl:
            throw AsmJsCompilationException( _u("Variable declaration must happen at the top of the function") );
            break;
        default:
            throw AsmJsCompilationException( _u("Unhandled parse opcode for asm.js") );
            break;
        }

        return EmitExpressionInfo(AsmJsType::Void);
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitBinaryMultiType( ParseNode * pnode, EBinaryMathOpCodes op )
    {
        ParseNode* lhs = ParserWrapper::GetBinaryLeft(pnode);
        ParseNode* rhs = ParserWrapper::GetBinaryRight(pnode);

        EmitExpressionInfo lhsEmit = Emit( lhs );
        EmitExpressionInfo rhsEmit = Emit( rhs );
        AsmJsType& lType = lhsEmit.type;
        AsmJsType& rType = rhsEmit.type;

        // don't need coercion inside an a+b+c type expression
        if (op == BMO_ADD || op == BMO_SUB)
        {
            if (lType.GetWhich() == AsmJsType::Intish && (lhs->nop == knopAdd || lhs->nop == knopSub))
            {
                lType = AsmJsType::Int;
            }
            if (rType.GetWhich() == AsmJsType::Intish && (rhs->nop == knopAdd || rhs->nop == knopSub))
            {
                rType = AsmJsType::Int;
            }
        }

        EmitExpressionInfo emitInfo( AsmJsType::Double );
        StartStatement(pnode);
        if( lType.isInt() && rType.isInt() )
        {
            CheckNodeLocation( lhsEmit, int );
            CheckNodeLocation( rhsEmit, int );
            // because fixnum can be either signed or unsigned, use both lhs and rhs to infer sign
            auto opType = (lType.isSigned() && rType.isSigned()) ? BMOT_Int : BMOT_UInt;
            if (op == BMO_REM || op == BMO_DIV)
            {
                // div and rem must have explicit sign
                if (!(lType.isSigned() && rType.isSigned()) && !(lType.isUnsigned() && rType.isUnsigned()))
                {
                    throw AsmJsCompilationException(_u("arguments to / or %% must both be double?, float?, signed, or unsigned; %s and %s given"), lType.toChars(), rType.toChars());
                }
            }

            // try to reuse tmp register
            RegSlot intReg = GetAndReleaseBinaryLocations<int>( &lhsEmit, &rhsEmit );
            mWriter.AsmReg3(BinaryMathOpCodes[op][opType], intReg, lhsEmit.location, rhsEmit.location );
            emitInfo.location = intReg;
            emitInfo.type = AsmJsType::Intish;
        }
        else if (lType.isMaybeDouble() && rType.isMaybeDouble())
        {
            CheckNodeLocation( lhsEmit, double );
            CheckNodeLocation( rhsEmit, double );

            RegSlot dbReg = GetAndReleaseBinaryLocations<double>( &lhsEmit, &rhsEmit );
            mWriter.AsmReg3( BinaryMathOpCodes[op][BMOT_Double], dbReg, lhsEmit.location, rhsEmit.location );
            emitInfo.location = dbReg;
        }
        else if (lType.isMaybeFloat() && rType.isMaybeFloat())
        {
            if (BinaryMathOpCodes[op][BMOT_Float] == OpCodeAsmJs::Nop)
            {
                throw AsmJsCompilationException(_u("invalid Binary float operation"));
            }

            CheckNodeLocation(lhsEmit, float);
            CheckNodeLocation(rhsEmit, float);

            RegSlot floatReg = GetAndReleaseBinaryLocations<float>(&lhsEmit, &rhsEmit);
            mWriter.AsmReg3(BinaryMathOpCodes[op][BMOT_Float], floatReg, lhsEmit.location, rhsEmit.location);
            emitInfo.location = floatReg;
            emitInfo.type = AsmJsType::Floatish;
        }
        else
        {
            throw AsmJsCompilationException( _u("Unsupported math operation") );
        }
        EndStatement(pnode);
        return emitInfo;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitBinaryInt( ParseNode * pnode, OpCodeAsmJs op )
    {
        ParseNode* lhs = ParserWrapper::GetBinaryLeft( pnode );
        ParseNode* rhs = ParserWrapper::GetBinaryRight( pnode );
        const bool isRhs0 = rhs->nop == knopInt && rhs->AsParseNodeInt()->lw == 0;
        const bool isOr0Operation = op == OpCodeAsmJs::Or_Int && isRhs0;
        if( isOr0Operation && lhs->nop == knopCall )
        {
            EmitExpressionInfo info = EmitCall(lhs, AsmJsRetType::Signed);
            if (!info.type.isIntish())
            {
                throw AsmJsCompilationException(_u("Invalid type for [| & ^ >> << >>>] left and right operand must be of type intish"));
            }
            info.type = AsmJsType::Signed;
            return info;
        }
        const EmitExpressionInfo& lhsEmit = Emit( lhs );
        const EmitExpressionInfo& rhsEmit = Emit( rhs );
        const AsmJsType& lType = lhsEmit.type;
        const AsmJsType& rType = rhsEmit.type;
        if( !lType.isIntish() || !rType.isIntish() )
        {
            throw AsmJsCompilationException( _u("Invalid type for [| & ^ >> << >>>] left and right operand must be of type intish") );
        }
        CheckNodeLocation( lhsEmit, int );
        CheckNodeLocation( rhsEmit, int );
        StartStatement(pnode);
        EmitExpressionInfo emitInfo( AsmJsType::Signed );
        if( op == OpCodeAsmJs::Shr_UInt )
        {
            emitInfo.type = AsmJsType::Unsigned;
        }
        // ignore this specific operation, useful for non asm.js
        if( !isRhs0 || op == OpCodeAsmJs::And_Int )
        {
            RegSlot dstReg = GetAndReleaseBinaryLocations<int>( &lhsEmit, &rhsEmit );
            mWriter.AsmReg3( op, dstReg, lhsEmit.location, rhsEmit.location );
            emitInfo.location = dstReg;
        }
        else
        {
            mFunction->ReleaseLocation<int>( &rhsEmit );
            emitInfo.location = lhsEmit.location;
        }
        EndStatement(pnode);
        return emitInfo;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitReturn( ParseNode * pnode )
    {
        ParseNode* expr = pnode->AsParseNodeReturn()->pnodeExpr;
        // return is always the beginning of a statement
        AsmJsRetType retType;
        EmitExpressionInfo emitInfo( Constants::NoRegister, AsmJsType::Void );
        if( !expr )
        {
            if( !mFunction->CheckAndSetReturnType( AsmJsRetType::Void ) )
            {
                throw AsmJsCompilationException( _u("Different return type for the function") );
            }
            retType = AsmJsRetType::Void;
        }
        else
        {
            EmitExpressionInfo info = Emit(expr);
            StartStatement(pnode);
            if (info.type.isSubType(AsmJsType::Double))
            {
                CheckNodeLocation(info, double);
                // get return value from tmp register
                mWriter.Conv(OpCodeAsmJs::Return_Db, 0, info.location);
                mFunction->ReleaseLocation<double>(&info);
                emitInfo.type = AsmJsType::Double;
                retType = AsmJsRetType::Double;
            }
            else if (info.type.isSubType(AsmJsType::Signed))
            {
                CheckNodeLocation(info, int);
                // get return value from tmp register
                mWriter.Conv(OpCodeAsmJs::Return_Int, 0, info.location);
                mFunction->ReleaseLocation<int>(&info);
                emitInfo.type = AsmJsType::Signed;
                retType = AsmJsRetType::Signed;
            }
            else if (info.type.isSubType(AsmJsType::Float))
            {
                CheckNodeLocation(info, float);
                // get return value from tmp register
                mWriter.Conv(OpCodeAsmJs::Return_Flt, 0, info.location);
                mFunction->ReleaseLocation<float>(&info);
                emitInfo.type = AsmJsType::Float;
                retType = AsmJsRetType::Float;
            }
            else
            {
                throw AsmJsCompilationException(_u("Expression for return must be subtype of Signed, Double, or Float"));
            }
            EndStatement(pnode);
        }
        // check if we saw another return already with a different type
        if (!mFunction->CheckAndSetReturnType(retType))
        {
            throw AsmJsCompilationException(_u("Different return type for the function %s"), mFunction->GetName()->Psz());
        }
        mWriter.AsmBr( mFunction->GetFuncInfo()->singleExit );
        return emitInfo;
    }

    RegSlot AsmJsFunc::AcquireTmpRegisterGeneric(AsmJsRetType retType)
    {
        switch (retType.which())
        {
        case AsmJsRetType::Signed:
            return AcquireTmpRegister<int>();
        case AsmJsRetType::Double:
            return AcquireTmpRegister<double>();
        case AsmJsRetType::Float:
            return AcquireTmpRegister<float>();
        case AsmJsRetType::Void:
            return Js::Constants::NoRegister;
        default:
            Assert(UNREACHED);
            return Js::Constants::NoRegister;
        }
    }

    bool AsmJsFunc::IsVarLocationGeneric(const EmitExpressionInfo* pnode)
    {
        if (pnode->type.isInt())
        {
            return IsVarLocation<int>(pnode);
        }
        else if (pnode->type.isDouble())
        {
            return IsVarLocation<double>(pnode);
        }
        else if (pnode->type.isFloat())
        {
            return IsVarLocation<float>(pnode);
        }
        else
        {
            // Vars must have concrete type, so any "-ish" or "maybe" type
            // cannot be in a var location
            return false;
        }
    }

    RegSlot AsmJSByteCodeGenerator::EmitIndirectCallIndex(ParseNode* identifierNode, ParseNode* indexNode)
    {
        // check for table size annotation
        if (indexNode->nop != knopAnd)
        {
            throw AsmJsCompilationException(_u("Function table call must be of format identifier[expr & NumericLiteral](...)"));
        }

        ParseNode* tableSizeNode = ParserWrapper::GetBinaryRight(indexNode);
        if (tableSizeNode->nop != knopInt)
        {
            throw AsmJsCompilationException(_u("Function table call must be of format identifier[expr & NumericLiteral](...)"));
        }
        if (tableSizeNode->AsParseNodeInt()->lw < 0)
        {
            throw AsmJsCompilationException(_u("Function table size must be positive"));
        }
        const uint tableSize = tableSizeNode->AsParseNodeInt()->lw + 1;
        if (!::Math::IsPow2(tableSize))
        {
            throw AsmJsCompilationException(_u("Function table size must be a power of 2"));
        }

        // Check for function table identifier
        if (!ParserWrapper::IsNameDeclaration(identifierNode))
        {
            throw AsmJsCompilationException(_u("Function call must be of format identifier(...) or identifier[expr & size](...)"));
        }
        PropertyName funcName = identifierNode->name();
        AsmJsFunctionDeclaration* sym = mCompiler->LookupFunction(funcName);
        if (!sym)
        {
            throw AsmJsCompilationException(_u("Unable to find function table %s"), funcName->Psz());
        }
        else
        {
            if (!AsmJsFunctionTable::Is(sym))
            {
                throw AsmJsCompilationException(_u("Identifier %s is not a function table"), funcName->Psz());
            }
            AsmJsFunctionTable* funcTable = AsmJsFunctionTable::FromSymbol(sym);
            if (funcTable->GetSize() != tableSize)
            {
                throw AsmJsCompilationException(_u("Trying to load from Function table %s of size [%d] with size [%d]"), funcName->Psz(), funcTable->GetSize(), tableSize);
            }
        }

        const EmitExpressionInfo& indexInfo = Emit(indexNode);
        if (!indexInfo.type.isInt())
        {
            throw AsmJsCompilationException(_u("Array Buffer View index must be type int"));
        }
        CheckNodeLocation(indexInfo, int);
        return indexInfo.location;
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitCall(ParseNode * pnode, AsmJsRetType expectedType /*= AsmJsType::Void*/)
    {
        Assert( pnode->nop == knopCall );

        ParseNode* identifierNode = pnode->AsParseNodeCall()->pnodeTarget;
        RegSlot funcTableIndexRegister = Constants::NoRegister;

        // Function table
        if( pnode->AsParseNodeCall()->pnodeTarget->nop == knopIndex )
        {
            identifierNode = ParserWrapper::GetBinaryLeft( pnode->AsParseNodeCall()->pnodeTarget );
            ParseNode* indexNode = ParserWrapper::GetBinaryRight( pnode->AsParseNodeCall()->pnodeTarget );

            funcTableIndexRegister = EmitIndirectCallIndex(identifierNode, indexNode);
        }

        if( !ParserWrapper::IsNameDeclaration( identifierNode ) )
        {
            throw AsmJsCompilationException( _u("Function call must be of format identifier(...) or identifier[expr & size](...)") );
        }
        PropertyName funcName = identifierNode->name();
        AsmJsFunctionDeclaration* sym = mCompiler->LookupFunction(funcName);
        if( !sym )
        {
            throw AsmJsCompilationException( _u("Undefined function %s"), funcName->Psz() );
        }

        const bool isFFI = AsmJsImportFunction::Is(sym);
        const bool isMathBuiltin = AsmJsMathFunction::Is(sym);
        if(isMathBuiltin)
        {
            return EmitMathBuiltin(pnode, AsmJsMathFunction::FromSymbol(sym));
        }

        // math builtins have different requirements for call-site coercion
        if (!sym->CheckAndSetReturnType(expectedType))
        {
            throw AsmJsCompilationException(_u("Different return type found for function %s"), funcName->Psz());
        }

        const ArgSlot argCount = pnode->AsParseNodeCall()->argCount;

        EmitExpressionInfo * argArray = nullptr;
        AsmJsType* types = nullptr;

        // first, evaluate function arguments
        if (argCount > 0)
        {
            ParseNode* argNode = pnode->AsParseNodeCall()->pnodeArgs;
            argArray = AnewArray(&mAllocator, EmitExpressionInfo, argCount);
            types = AnewArray(&mAllocator, AsmJsType, argCount);
            for (ArgSlot i = 0; i < argCount; i++)
            {
                ParseNode* arg = argNode;
                if (argNode->nop == knopList)
                {
                    arg = ParserWrapper::GetBinaryLeft(argNode);
                    argNode = ParserWrapper::GetBinaryRight(argNode);
                }

                // Emit argument
                EmitExpressionInfo argInfo = Emit(arg);

                types[i] = argInfo.type;
                argArray[i].type = argInfo.type;

                if (!mFunction->IsVarLocationGeneric(&argInfo))
                {
                    argArray[i].location = argInfo.location;
                }
                else
                {
                    // If argument is a var, another argument might change its value, so move it to a temp register
                    mFunction->ReleaseLocationGeneric(&argInfo);
                    if (argInfo.type.isInt())
                    {
                        argArray[i].location = mFunction->AcquireTmpRegister<int>();
                        mWriter.AsmReg2(OpCodeAsmJs::Ld_Int, argArray[i].location, argInfo.location);
                    }
                    else if (argInfo.type.isFloat())
                    {
                        argArray[i].location = mFunction->AcquireTmpRegister<float>();
                        mWriter.AsmReg2(OpCodeAsmJs::Ld_Flt, argArray[i].location, argInfo.location);
                    }
                    else if (argInfo.type.isDouble())
                    {
                        argArray[i].location = mFunction->AcquireTmpRegister<double>();
                        mWriter.AsmReg2(OpCodeAsmJs::Ld_Db, argArray[i].location, argInfo.location);
                    }
                    else
                    {
                        throw AsmJsCompilationException(_u("Function %s doesn't support argument of type %s"), funcName->Psz(), argInfo.type.toChars());
                    }
                }
            }
        }

        // Check if this function supports the type of these arguments
        AsmJsRetType retType;
        const bool supported = sym->SupportsArgCall(argCount, types, retType);
        if (!supported)
        {
            throw AsmJsCompilationException(_u("Function %s doesn't support arguments"), funcName->Psz());
        }
        if (types)
        {
            AdeleteArray(&mAllocator, argCount, types);
            types = nullptr;
        }

        // need to validate return type again because function might support arguments,
        // but return a different type, i.e.: abs(int) -> int, but expecting double
        // don't validate the return type for foreign import functions
        if (!isFFI && retType != expectedType)
        {
            throw AsmJsCompilationException(_u("Function %s returns different type"), funcName->Psz());
        }

        const ArgSlot argByteSize = ArgSlotMath::Add(sym->GetArgByteSize(argCount), sizeof(Var));
        // +1 is for function object
        ArgSlot runtimeArg = ArgSlotMath::Add(argCount, 1);
        if (!isFFI) // for non import functions runtimeArg is calculated from argByteSize
        {
            runtimeArg = (ArgSlot)(::ceil((double)(argByteSize / sizeof(Var)))) + 1;
        }

        StartStatement(pnode);

        mWriter.AsmStartCall(isFFI ? OpCodeAsmJs::StartCall : OpCodeAsmJs::I_StartCall, argByteSize);

        if( argCount > 0 )
        {
            ParseNode* argNode = pnode->AsParseNodeCall()->pnodeArgs;
            uint16 regSlotLocation = 1;

            for(ArgSlot i = 0; i < argCount; i++)
            {
                // Get i arg node
                ParseNode* arg = argNode;
                if( argNode->nop == knopList )
                {
                    arg = ParserWrapper::GetBinaryLeft( argNode );
                    argNode = ParserWrapper::GetBinaryRight( argNode );
                }
                EmitExpressionInfo argInfo = argArray[i];
                // OutParams i
                if( argInfo.type.isDouble() )
                {
                    CheckNodeLocation( argInfo, double );
                    if (isFFI)
                    {
                        mWriter.AsmReg2(OpCodeAsmJs::ArgOut_Db, regSlotLocation, argInfo.location);
                        regSlotLocation++; // in case of external calls this is boxed and converted to a Var
                    }
                    else
                    {
                        mWriter.AsmReg2(OpCodeAsmJs::I_ArgOut_Db, regSlotLocation, argInfo.location);
                        regSlotLocation += sizeof(double) / sizeof(Var);// in case of internal calls we will pass this arg as double
                    }
                }
                else if (argInfo.type.isFloat())
                {
                    CheckNodeLocation(argInfo, float);
                    if (isFFI)
                    {
                        throw AsmJsCompilationException(_u("FFI function %s doesn't support float arguments"), funcName->Psz());
                    }
                    mWriter.AsmReg2(OpCodeAsmJs::I_ArgOut_Flt, regSlotLocation, argInfo.location);
                    regSlotLocation++;
                }
                else if (argInfo.type.isInt())
                {
                    CheckNodeLocation( argInfo, int );
                    mWriter.AsmReg2(isFFI ? OpCodeAsmJs::ArgOut_Int : OpCodeAsmJs::I_ArgOut_Int, regSlotLocation, argInfo.location);
                    regSlotLocation++;
                }
                else
                {
                    throw AsmJsCompilationException(_u("Function %s doesn't support argument of type %s"), funcName->Psz(), argInfo.type.toChars());
                }
            }

            for (ArgSlot i = argCount; i > 0; --i)
            {
                mFunction->ReleaseLocationGeneric(&argArray[i - 1]);
            }
            AdeleteArray(&mAllocator, argCount, argArray);
            argArray = nullptr;
        }


        // Make sure we have enough memory allocated for OutParameters
        // +1 is for return address
        mFunction->UpdateMaxArgOutDepth(ArgSlotMath::Add(runtimeArg, 1));

        // Load function from env
        ProfileId profileId = Js::Constants::NoProfileId;
        RegSlot funcReg = Js::Constants::NoRegister;
        switch( sym->GetSymbolType() )
        {
        case AsmJsSymbol::ModuleFunction:
            funcReg = mFunction->AcquireTmpRegister<intptr_t>();
            LoadModuleFunction(funcReg, sym->GetFunctionIndex());
            profileId = mFunction->GetNextProfileId();
            break;
        case AsmJsSymbol::ImportFunction:
            funcReg = mFunction->AcquireTmpRegister<intptr_t>();
            LoadModuleFFI(funcReg, sym->GetFunctionIndex());
            break;
        case AsmJsSymbol::FuncPtrTable:
            // Make sure the user is not trying to call the function table directly
            if (funcTableIndexRegister == Constants::NoRegister)
            {
                throw AsmJsCompilationException(_u("Direct call to function table '%s' is not allowed"), funcName->Psz());
            }
            mFunction->ReleaseTmpRegister<int>(funcTableIndexRegister);
            funcReg = mFunction->AcquireTmpRegister<intptr_t>();
            LoadModuleFunctionTable(funcReg, sym->GetFunctionIndex(), funcTableIndexRegister);
            break;
        default:
            throw AsmJsCompilationException(_u("Invalid function type"));
        }

        // use expected type because return type could be invalid if the function is a FFI
        EmitExpressionInfo info(expectedType.toType());
        mFunction->ReleaseTmpRegister<intptr_t>(funcReg);
        if (isFFI)
        {
            RegSlot retReg = mFunction->AcquireTmpRegister<intptr_t>();
            mWriter.AsmCall(OpCodeAsmJs::Call, retReg, funcReg, runtimeArg, expectedType, profileId);

            mFunction->ReleaseTmpRegister<intptr_t>(retReg);
            info.location = mFunction->AcquireTmpRegisterGeneric(expectedType);

            switch (expectedType.which())
            {
            case AsmJsRetType::Void:
                break;
            case AsmJsRetType::Signed:
                mWriter.AsmReg2(OpCodeAsmJs::Conv_VTI, info.location, retReg);
                break;
            case AsmJsRetType::Double:
                mWriter.AsmReg2(OpCodeAsmJs::Conv_VTD, info.location, retReg);
                break;
            default:
                Assert(UNREACHED);
            }
        }
        else
        {
            info.location = mFunction->AcquireTmpRegisterGeneric(expectedType);
            mWriter.AsmCall(OpCodeAsmJs::I_Call, info.location, funcReg, runtimeArg, expectedType, profileId);
        }

        // after foreign function call, we need to make sure that the heap hasn't been detached
        if (isFFI && mCompiler->UsesHeapBuffer())
        {
            mWriter.EmptyAsm(OpCodeAsmJs::CheckHeap);
            mCompiler->SetUsesHeapBuffer(true);
        }

        EndStatement(pnode);

        return info;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitMathBuiltin(ParseNode* pnode, AsmJsMathFunction* mathFunction)
    {
        if (mathFunction->GetMathBuiltInFunction() == AsmJSMathBuiltinFunction::AsmJSMathBuiltin_max || mathFunction->GetMathBuiltInFunction() == AsmJSMathBuiltinFunction::AsmJSMathBuiltin_min)
        {
            return EmitMinMax(pnode, mathFunction);
        }

        const ArgSlot argCount = pnode->AsParseNodeCall()->argCount;
        ParseNode* argNode = pnode->AsParseNodeCall()->pnodeArgs;
        const bool isFRound = AsmJsMathFunction::IsFround(mathFunction);

        // for fround, if we have a fround(NumericLiteral), we want to just emit Ld_Flt NumericLiteral
        if (argCount == 1 && isFRound && ParserWrapper::IsFroundNumericLiteral(argNode))
        {
            StartStatement(pnode);
            RegSlot dst = mFunction->AcquireTmpRegister<float>();
            EmitExpressionInfo emitInfo(dst, AsmJsType::Float);
            float constValue = -0.0f;
            if (argNode->nop == knopFlt)
            {
                constValue = (float)argNode->AsParseNodeFloat()->dbl;
            }
            else if (argNode->nop == knopInt)
            {
                constValue = (float)argNode->AsParseNodeInt()->lw;
            }
            else
            {
                Assert(ParserWrapper::IsNegativeZero(argNode));
            }
            mWriter.AsmReg2(OpCodeAsmJs::Ld_Flt, dst, mFunction->GetConstRegister<float>(constValue));
            EndStatement(pnode);
            return emitInfo;
        }

        AutoArrayPtr<AsmJsType> types(nullptr, 0);
        AutoArrayPtr<EmitExpressionInfo> argsInfo(nullptr, 0);
        if( argCount > 0 )
        {
            types.Set(HeapNewArray(AsmJsType, argCount), argCount);
            argsInfo.Set(HeapNewArray(EmitExpressionInfo, argCount), argCount);

            for(ArgSlot i = 0; i < argCount; i++)
            {
                // Get i arg node
                ParseNode* arg = argNode;
                // Special case for fround(abs()) call
                if (argNode->nop == knopCall && isFRound)
                {
                    // Emit argument
                    const EmitExpressionInfo& argInfo = EmitCall(arg, AsmJsRetType::Float);
                    types[i] = argInfo.type;
                    argsInfo[i].type = argInfo.type;
                    argsInfo[i].location = argInfo.location;
                }
                else
                {
                    if (argNode->nop == knopList)
                    {
                        arg = ParserWrapper::GetBinaryLeft(argNode);
                        argNode = ParserWrapper::GetBinaryRight(argNode);
                    }
                    // Emit argument
                    const EmitExpressionInfo& argInfo = Emit(arg);
                    types[i] = argInfo.type;
                    argsInfo[i].type = argInfo.type;
                    argsInfo[i].location = argInfo.location;
                }
            }
        }
        StartStatement(pnode);
        // Check if this function supports the type of these arguments
        AsmJsRetType retType;
        OpCodeAsmJs op;
        const bool supported = mathFunction->SupportsMathCall( argCount, types, op, retType );
        if( !supported )
        {
            throw AsmJsCompilationException( _u("Math builtin function doesn't support arguments") );
        }

        // Release all used location before acquiring a new tmp register
        for (int i = argCount - 1; i >= 0 ; i--)
        {
            mFunction->ReleaseLocationGeneric( &argsInfo[i] );
        }

        const int argByteSize = mathFunction->GetArgByteSize(argCount) + sizeof(Var);
        // + 1 is for function object
        int runtimeArg = (int)(::ceil((double)(argByteSize / sizeof(Var)))) + 1;

        // Make sure we have enough memory allocated for OutParameters
        // + 1 for return address
        mFunction->UpdateMaxArgOutDepth(runtimeArg + 1);

        const bool isInt = retType.toType().isInt();
        const bool isFloatish = retType.toType().isFloatish();
        Assert(isInt || isFloatish || retType.toType().isDouble());

        RegSlot dst;
        if( isInt )
        {
            dst = mFunction->AcquireTmpRegister<int>();
        }
        else if (isFloatish)
        {
            dst = mFunction->AcquireTmpRegister<float>();
        }
        else
        {
            dst = mFunction->AcquireTmpRegister<double>();
        }

        EmitExpressionInfo emitInfo(dst, retType.toType());

        switch( argCount )
        {
        case 1:
            mWriter.AsmReg2( op, dst, argsInfo[0].location );
            break;
        case 2:
            mWriter.AsmReg3( op, dst, argsInfo[0].location, argsInfo[1].location );
            break;
        default:
            Assume(UNREACHED);
        }
#if DBG
        for (int i = 0; i < argCount; i++)
        {
            if (argsInfo[i].type.isSubType(AsmJsType::Floatish))
            {
                CheckNodeLocation(argsInfo[i], float);
            }
            else if (argsInfo[i].type.isSubType(AsmJsType::MaybeDouble))
            {
                CheckNodeLocation(argsInfo[i], double);
            }
            else if (argsInfo[i].type.isSubType(AsmJsType::Intish))
            {
                CheckNodeLocation(argsInfo[i], int);
            }
        }
#endif
        EndStatement(pnode);
        return emitInfo;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitMinMax(ParseNode* pnode, AsmJsMathFunction* mathFunction)
    {
        Assert(mathFunction->GetArgCount() == 2);
        uint16 argCount = pnode->AsParseNodeCall()->argCount;
        ParseNode* argNode = pnode->AsParseNodeCall()->pnodeArgs;

        if (argCount < 2)
        {
            throw AsmJsCompilationException(_u("Math builtin function doesn't support arguments"));
        }

        AutoArrayPtr<AsmJsType> types(nullptr, 0);
        AutoArrayPtr<EmitExpressionInfo> argsInfo(nullptr, 0);
        types.Set(HeapNewArray(AsmJsType, mathFunction->GetArgCount()), mathFunction->GetArgCount());
        argsInfo.Set(HeapNewArray(EmitExpressionInfo, mathFunction->GetArgCount()), mathFunction->GetArgCount());

        ParseNode * arg = ParserWrapper::GetBinaryLeft(argNode);
        argNode = ParserWrapper::GetBinaryRight(argNode);
        // Emit first arg as arg0
        argsInfo[0] = Emit(arg);
        types[0] = argsInfo[0].type;

        EmitExpressionInfo dstInfo;
        for (int i = 1; i < argCount; i++)
        {
            if (argNode->nop == knopList)
            {
                arg = ParserWrapper::GetBinaryLeft(argNode);
                argNode = ParserWrapper::GetBinaryRight(argNode);
            }
            else
            {
                arg = argNode;
            }
            // arg1 will always be the next arg in the argList
            argsInfo[1] = Emit(arg);
            types[1] = argsInfo[1].type;

            // Check if this function supports the type of these arguments
            AsmJsRetType retType;
            OpCodeAsmJs op;
            const bool supported = mathFunction->SupportsMathCall(mathFunction->GetArgCount(), types, op, retType);
            if (!supported)
            {
                throw AsmJsCompilationException(_u("Math builtin function doesn't support arguments"));
            }

            const int argByteSize = mathFunction->GetArgByteSize(argCount) + sizeof(Var);
            // +1 is for function object
            int runtimeArg = (int)(::ceil((double)(argByteSize / sizeof(Var)))) + 1;
            // +1 is for return address

            // Make sure we have enough memory allocated for OutParameters
            mFunction->UpdateMaxArgOutDepth(runtimeArg + 1);
            mFunction->ReleaseLocationGeneric(&argsInfo[1]);
            mFunction->ReleaseLocationGeneric(&argsInfo[0]);

            dstInfo.type = retType.toType();
            if (retType.toType().isSigned())
            {
                dstInfo.location = mFunction->AcquireTmpRegister<int>();
            }
            else
            {
                Assert(retType.toType().isDouble());
                dstInfo.location = mFunction->AcquireTmpRegister<double>();
            }

            mWriter.AsmReg3(op, dstInfo.location, argsInfo[0].location, argsInfo[1].location);
            // for max/min calls with more than 2 arguments, we use the result of previous call for arg0
            argsInfo[0] = dstInfo;
#if DBG
            for (uint j = 0; j < mathFunction->GetArgCount(); j++)
            {
                if (argsInfo[j].type.isSubType(AsmJsType::MaybeDouble))
                {
                    CheckNodeLocation(argsInfo[j], double);
                }
                else if (argsInfo[j].type.isSubType(AsmJsType::Intish))
                {
                    CheckNodeLocation(argsInfo[j], int);
                }
                else
                {
                    Assert(UNREACHED);
                }
            }
#endif
        }
        return dstInfo;
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitIdentifier( ParseNode * pnode )
    {
        Assert( ParserWrapper::IsNameDeclaration( pnode ) );
        PropertyName name = pnode->name();
        AsmJsLookupSource::Source source;
        AsmJsSymbol* sym = mCompiler->LookupIdentifier( name, mFunction, &source );
        if( !sym )
        {
            throw AsmJsCompilationException( _u("Undefined identifier %s"), name->Psz() );
        }

        switch( sym->GetSymbolType() )
        {
        case AsmJsSymbol::Variable:
        {
            AsmJsVar * var = AsmJsVar::FromSymbol(sym);
            if (!var->isMutable())
            {
                // currently const is only allowed for variables at module scope
                Assert(source == AsmJsLookupSource::AsmJsModule);

                EmitExpressionInfo emitInfo(var->GetType());
                if (var->GetVarType().isInt())
                {
                    emitInfo.location = mFunction->AcquireTmpRegister<int>();
                    mWriter.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, emitInfo.location, var->GetIntInitialiser());
                }
                else if (var->GetVarType().isFloat())
                {
                    emitInfo.location = mFunction->AcquireTmpRegister<float>();
                    mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Flt, emitInfo.location, mFunction->GetConstRegister<float>(var->GetFloatInitialiser()));
                }
                else
                {
                    Assert(var->GetVarType().isDouble());
                    emitInfo.location = mFunction->AcquireTmpRegister<double>();
                    mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Db, emitInfo.location, mFunction->GetConstRegister<double>(var->GetDoubleInitialiser()));
                }
                return emitInfo;
            }
            // else fall through
        }
        case AsmJsSymbol::Argument:
        case AsmJsSymbol::ConstantImport:
        {
            AsmJsVarBase* var = AsmJsVarBase::FromSymbol(sym);
            if( source == AsmJsLookupSource::AsmJsFunction )
            {
                return EmitExpressionInfo( var->GetLocation(), var->GetType() );
            }
            else
            {
                Assert( source == AsmJsLookupSource::AsmJsModule );
                EmitExpressionInfo emitInfo(var->GetType());
                if (var->GetVarType().isInt())
                {
                    emitInfo.location = mFunction->AcquireTmpRegister<int>();
                    LoadModuleInt(emitInfo.location, var->GetLocation());
                }
                else if (var->GetVarType().isFloat())
                {
                    emitInfo.location = mFunction->AcquireTmpRegister<float>();
                    LoadModuleFloat(emitInfo.location, var->GetLocation());
                }
                else if (var->GetVarType().isDouble())
                {
                    emitInfo.location = mFunction->AcquireTmpRegister<double>();
                    LoadModuleDouble(emitInfo.location, var->GetLocation());
                }
                else
                {
                    Assert(UNREACHED);
                }
                return emitInfo;
            }
            break;
        }
        case AsmJsSymbol::MathConstant:
        {
            AsmJsMathConst* mathConst = AsmJsMathConst::FromSymbol(sym);
            Assert(mathConst->GetType().isDouble());
            RegSlot loc = mFunction->AcquireTmpRegister<double>();
            mWriter.AsmReg2( OpCodeAsmJs::Ld_Db, loc, mFunction->GetConstRegister<double>(*mathConst->GetVal()) );
            return EmitExpressionInfo(loc, AsmJsType::Double);
        }

        case AsmJsSymbol::ImportFunction:
        case AsmJsSymbol::FuncPtrTable:
        case AsmJsSymbol::ModuleFunction:
        case AsmJsSymbol::ArrayView:
        case AsmJsSymbol::MathBuiltinFunction:
        default:
            throw AsmJsCompilationException( _u("Cannot use identifier %s in this context"), name->Psz() );
        }
    }

    static const OpCodeAsmJs typedArrayOp[2][2] =
    {
        { OpCodeAsmJs::LdArrConst, OpCodeAsmJs::LdArr },//LoadTypedArray
        { OpCodeAsmJs::StArrConst, OpCodeAsmJs::StArr },//StoreTypedArray
    };

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitTypedArrayIndex(ParseNode* indexNode, OpCodeAsmJs &op, uint32 &indexSlot, ArrayBufferView::ViewType viewType, TypedArrayEmitType emitType)
    {
        mCompiler->SetUsesHeapBuffer(true);
        bool isConst = false;
        uint32 slot = 0;
        if(indexNode->nop == knopName)
        {
            AsmJsSymbol * declSym = mCompiler->LookupIdentifier(indexNode->name(), mFunction);
            if (AsmJsVar::Is(declSym) && !declSym->isMutable())
            {
                AsmJsVar * definition = AsmJsVar::FromSymbol(declSym);
                if(definition->GetVarType().isInt())
                {
                    slot = (uint32)definition->GetIntInitialiser();
                    isConst = true;
                }
            }
        }
        if (indexNode->nop == knopInt || indexNode->nop == knopFlt || isConst)
        {
            // Emit a different opcode for numerical literal
            if (!isConst)
            {
                if (indexNode->nop == knopInt)
                {
                    slot = (uint32)indexNode->AsParseNodeInt()->lw;
                }
                else if (ParserWrapper::IsMinInt(indexNode))
                {
                    // this is going to be an error, but we can do this to allow it to get same error message as invalid int
                    slot = (uint32)INT32_MIN;
                }
                else if (ParserWrapper::IsUnsigned(indexNode))
                {
                    slot = (uint32)indexNode->AsParseNodeFloat()->dbl;
                }
                else
                {
                    EmitExpressionInfo indexInfo = Emit(indexNode);
                    throw AsmJsCompilationException(_u("Array Index must be intish; %s given"), indexInfo.type.toChars());
                }
            }
            // do the right shift now
            switch( viewType )
            {
            case Js::ArrayBufferView::TYPE_INT16:
            case Js::ArrayBufferView::TYPE_UINT16:
                if (slot & 0x80000000)
                {
                    throw AsmJsCompilationException(_u("Numeric literal for heap16 must be within 0 <= n < 2^31; %d given"), slot);
                }
                slot <<= 1;
                break;
            case Js::ArrayBufferView::TYPE_INT32:
            case Js::ArrayBufferView::TYPE_UINT32:
            case Js::ArrayBufferView::TYPE_FLOAT32:
                if (slot & 0xC0000000)
                {
                    throw AsmJsCompilationException(_u("Numeric literal for heap32 must be within 0 <= n < 2^30; %d given"), slot);
                }
                slot <<= 2;
                break;
            case Js::ArrayBufferView::TYPE_FLOAT64:
                if (slot & 0xE0000000)
                {
                    throw AsmJsCompilationException(_u("Numeric literal for heap64 must be within 0 <= n < 2^29; %d given"), slot);
                }
                slot <<= 3;
                break;
            default:
                break;
            }
            mCompiler->UpdateMaxHeapAccess(slot);
            op = typedArrayOp[emitType][0];
        }
        else
        {
            EmitExpressionInfo indexInfo;
            if (indexNode->nop != knopRsh && viewType != Js::ArrayBufferView::TYPE_INT8 && viewType != Js::ArrayBufferView::TYPE_UINT8)
            {
                throw AsmJsCompilationException( _u("index expression isn't shifted; must be an Int8/Uint8 access") );
            }
            int val = 0;
            uint32 mask = (uint32)~0;
            ParseNode* index;
            if (indexNode->nop == knopRsh)
            {
                ParseNode* rhsNode = ParserWrapper::GetBinaryRight(indexNode);
                if (!rhsNode || rhsNode->nop != knopInt)
                {
                    throw AsmJsCompilationException(_u("shift amount must be constant"));
                }
                switch (viewType)
                {
#define ARRAYBUFFER_VIEW(name, align, RegType, MemType, irSuffix) \
                case Js::ArrayBufferView::TYPE_##name:\
                    val = align;\
                    mask = ARRAYBUFFER_VIEW_MASK(align);\
                    break;
                #include "AsmJsArrayBufferViews.h"
                default:
                    Assume(UNREACHED);
                }
                if (rhsNode->AsParseNodeInt()->lw != val)
                {
                    throw AsmJsCompilationException(_u("shift amount must be %d"), val);
                }
                index = ParserWrapper::GetBinaryLeft(indexNode);
            }
            else
            {
                index = indexNode;
            }

            isConst = false;
            if (index->nop == knopName)
            {
                AsmJsSymbol * declSym = mCompiler->LookupIdentifier(index->name(), mFunction);
                if (AsmJsVar::Is(declSym) && !declSym->isMutable())
                {
                    AsmJsVar * definition = AsmJsVar::FromSymbol(declSym);
                    if (definition->GetVarType().isInt())
                    {
                        slot = (uint32)definition->GetIntInitialiser();
                        slot &= mask;
                        op = typedArrayOp[emitType][0];
                        isConst = true;
                        mCompiler->UpdateMaxHeapAccess(slot);
                    }
                }
            }
            if( ParserWrapper::IsUInt( index) )
            {
                slot = ParserWrapper::GetUInt(index);
                slot &= mask;
                op = typedArrayOp[emitType][0];

                mCompiler->UpdateMaxHeapAccess(slot);
            }
            else if (!isConst)
            {
                indexInfo = Emit( index );
                if( !indexInfo.type.isIntish() )
                {
                    throw AsmJsCompilationException( _u("Left operand of >> must be intish; %s given"), indexInfo.type.toChars() );
                }
                indexSlot = indexInfo.location;
                op = typedArrayOp[emitType][1];
                return indexInfo;
            }
        }
        indexSlot = slot;
        return EmitExpressionInfo();
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitLdArrayBuffer( ParseNode * pnode )
    {
        ParseNode* arrayNameNode = ParserWrapper::GetBinaryLeft( pnode );
        ParseNode* indexNode = ParserWrapper::GetBinaryRight( pnode );
        if( !ParserWrapper::IsNameDeclaration( arrayNameNode ) )
        {
            throw AsmJsCompilationException( _u("Invalid symbol ") );
        }

        PropertyName name = arrayNameNode->name();
        AsmJsSymbol* sym = mCompiler->LookupIdentifier(name, mFunction);
        if(!AsmJsArrayView::Is(sym))
        {
            throw AsmJsCompilationException( _u("Invalid identifier %s"), name->Psz() );
        }
        AsmJsArrayView* arrayView = AsmJsArrayView::FromSymbol(sym);
        ArrayBufferView::ViewType viewType = arrayView->GetViewType();

        OpCodeAsmJs op;
        uint32 indexSlot = 0;
        EmitExpressionInfo indexInfo = EmitTypedArrayIndex(indexNode, op, indexSlot, viewType, LoadTypedArray);
        mFunction->ReleaseLocationGeneric(&indexInfo);

        EmitExpressionInfo info( arrayView->GetType() );
        if( info.type.isIntish() )
        {
            info.location = mFunction->AcquireTmpRegister<int>();
        }
        else if (info.type.isMaybeFloat())
        {
            info.location = mFunction->AcquireTmpRegister<float>();
        }
        else
        {
            Assert(info.type.isMaybeDouble());
            info.location = mFunction->AcquireTmpRegister<double>();
        }
        mWriter.AsmTypedArr( op, info.location, indexSlot, viewType );

        return info;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitAssignment( ParseNode * pnode )
    {
        StartStatement(pnode);
        ParseNode* lhs = ParserWrapper::GetBinaryLeft( pnode );
        ParseNode* rhs = ParserWrapper::GetBinaryRight(pnode);
        EmitExpressionInfo rhsEmit;
        if( ParserWrapper::IsNameDeclaration( lhs ) )
        {
            rhsEmit = Emit(rhs);
            const AsmJsType& rType = rhsEmit.type;

            PropertyName name = lhs->name();
            AsmJsLookupSource::Source source;
            AsmJsSymbol* sym = mCompiler->LookupIdentifier( name, mFunction, &source );
            if(!AsmJsVarBase::Is(sym))
            {
                throw AsmJsCompilationException( _u("Identifier %s is not a variable"), name->Psz() );
            }

            if( !sym->isMutable() )
            {
                throw AsmJsCompilationException( _u("Cannot assign to identifier %s"), name->Psz() );
            }

            AsmJsVarBase* var = AsmJsVarBase::FromSymbol(sym);
            if( !var->GetType().isSuperType( rType ) )
            {
                throw AsmJsCompilationException( _u("Cannot assign type %s to identifier %s"), rType.toChars(), name->Psz() );
            }

            switch( source )
            {
            case Js::AsmJsLookupSource::AsmJsModule:
                if( var->GetVarType().isInt() )
                {
                    CheckNodeLocation( rhsEmit, int );
                    SetModuleInt( var->GetLocation(), rhsEmit.location );
                }
                else if (var->GetVarType().isFloat())
                {
                    CheckNodeLocation(rhsEmit, float);
                    SetModuleFloat(var->GetLocation(), rhsEmit.location);
                }
                else if (var->GetVarType().isDouble())
                {
                    CheckNodeLocation( rhsEmit, double );
                    SetModuleDouble( var->GetLocation(), rhsEmit.location );
                }
                else
                {
                    Assert(UNREACHED);
                }
                break;
            case Js::AsmJsLookupSource::AsmJsFunction:
                if( var->GetVarType().isInt() )
                {
                    CheckNodeLocation( rhsEmit, int );
                    mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Int, var->GetLocation(), rhsEmit.location );
                }
                else if (var->GetVarType().isFloat())
                {
                    CheckNodeLocation(rhsEmit, float);
                    mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Flt, var->GetLocation(), rhsEmit.location);
                }
                else if (var->GetVarType().isDouble())
                {
                    CheckNodeLocation( rhsEmit, double );
                    mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Db, var->GetLocation(), rhsEmit.location );
                }
                else
                {
                    Assert(UNREACHED);
                }
                break;
            default:
                break;
            }

        }
        else if( lhs->nop == knopIndex )
        {
            ParseNode* arrayNameNode = ParserWrapper::GetBinaryLeft( lhs );
            ParseNode* indexNode = ParserWrapper::GetBinaryRight( lhs );
            if( !ParserWrapper::IsNameDeclaration( arrayNameNode ) )
            {
                throw AsmJsCompilationException( _u("Invalid symbol ") );
            }

            PropertyName name = arrayNameNode->name();
            AsmJsSymbol* sym = mCompiler->LookupIdentifier(name, mFunction);
            if (!AsmJsArrayView::Is(sym))
            {
                throw AsmJsCompilationException( _u("Invalid identifier %s"), name->Psz() );
            }
            // must emit index expr first in case it has side effects
            AsmJsArrayView* arrayView = AsmJsArrayView::FromSymbol(sym);
            ArrayBufferView::ViewType viewType = arrayView->GetViewType();

            OpCodeAsmJs op;
            uint32 indexSlot = 0;
            EmitExpressionInfo indexInfo = EmitTypedArrayIndex(indexNode, op, indexSlot, viewType, StoreTypedArray);
            rhsEmit = Emit(rhs);

            if (viewType == ArrayBufferView::TYPE_FLOAT32)
            {
                if (!rhsEmit.type.isFloatish() && !rhsEmit.type.isMaybeDouble())
                {
                    throw AsmJsCompilationException(_u("Cannot assign value to TYPE_FLOAT32 ArrayBuffer"));
                }
                // do the conversion to float only for double
                if (rhsEmit.type.isMaybeDouble())
                {
                    CheckNodeLocation(rhsEmit, double);
                    RegSlot dst = mFunction->AcquireTmpRegister<float>();
                    mWriter.AsmReg2(OpCodeAsmJs::Fround_Db, dst, rhsEmit.location);
                    mFunction->ReleaseLocation<double>(&rhsEmit);
                    rhsEmit.location = dst;
                    rhsEmit.type = AsmJsType::Float;
                }
            }
            else if (viewType == ArrayBufferView::TYPE_FLOAT64)
            {
                if (!rhsEmit.type.isMaybeFloat() && !rhsEmit.type.isMaybeDouble())
                {
                    throw AsmJsCompilationException(_u("Cannot assign value to TYPE_FLOAT64 ArrayBuffer"));
                }
                // do the conversion to double only for float
                if (rhsEmit.type.isMaybeFloat())
                {
                    CheckNodeLocation(rhsEmit, float);
                    RegSlot dst = mFunction->AcquireTmpRegister<double>();
                    mWriter.AsmReg2(OpCodeAsmJs::Conv_FTD, dst, rhsEmit.location);
                    mFunction->ReleaseLocation<float>(&rhsEmit);
                    rhsEmit.location = dst;
                    rhsEmit.type = AsmJsType::Double;
                }
            }
            else if (!rhsEmit.type.isSubType(arrayView->GetType()))
            {
                throw AsmJsCompilationException( _u("Cannot assign value ArrayBuffer") );
            }

            // to keep tmp registers in order, I need to release rhsEmit.local before indexInfo.location
            mWriter.AsmTypedArr(op, rhsEmit.location, indexSlot, viewType);
            RegSlot rhsReg = rhsEmit.location;
            mFunction->ReleaseLocationGeneric(&rhsEmit);
            mFunction->ReleaseLocationGeneric(&indexInfo);
            RegSlot newRhsReg;
            if (rhsEmit.type.isMaybeDouble())
            {
                newRhsReg = mFunction->AcquireTmpRegister<double>();
                mWriter.AsmReg2(OpCodeAsmJs::Ld_Db, newRhsReg, rhsReg);
            }
            else if (rhsEmit.type.isFloatish())
            {
                newRhsReg = mFunction->AcquireTmpRegister<float>();
                mWriter.AsmReg2(OpCodeAsmJs::Ld_Flt, newRhsReg, rhsReg);
            }
            else
            {
                newRhsReg = mFunction->AcquireTmpRegister<int>();
                mWriter.AsmReg2(OpCodeAsmJs::Ld_Int, newRhsReg, rhsReg);
            }
            rhsEmit.location = newRhsReg;


        }
        else
        {
            throw AsmJsCompilationException( _u("Can only assign to an identifier or an ArrayBufferView") );
        }
        EndStatement(pnode);
        return rhsEmit;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitBinaryComparator( ParseNode * pnode, EBinaryComparatorOpCodes op )
    {
        ParseNode* lhs = ParserWrapper::GetBinaryLeft( pnode );
        ParseNode* rhs = ParserWrapper::GetBinaryRight( pnode );
        const EmitExpressionInfo& lhsEmit = Emit( lhs );
        EmitExpressionInfo rhsEmit = Emit( rhs );
        const AsmJsType& lType = lhsEmit.type;
        const AsmJsType& rType = rhsEmit.type;
        StartStatement(pnode);
        EmitExpressionInfo emitInfo(AsmJsType::Int);
        OpCodeAsmJs compOp;

        if (lType.isUnsigned() && rType.isUnsigned())
        {
            CheckNodeLocation(lhsEmit, int);
            CheckNodeLocation(rhsEmit, int);
            emitInfo.location = GetAndReleaseBinaryLocations<int>(&lhsEmit, &rhsEmit);
            compOp = BinaryComparatorOpCodes[op][BCOT_UInt];
        }
        else if( lType.isSigned() && rType.isSigned() )
        {
            CheckNodeLocation( lhsEmit, int );
            CheckNodeLocation( rhsEmit, int );
            emitInfo.location = GetAndReleaseBinaryLocations<int>( &lhsEmit, &rhsEmit );
            compOp = BinaryComparatorOpCodes[op][BCOT_Int];
        }
        else if( lType.isDouble() && rType.isDouble() )
        {
            CheckNodeLocation( lhsEmit, double );
            CheckNodeLocation( rhsEmit, double );
            emitInfo.location = mFunction->AcquireTmpRegister<int>();
            mFunction->ReleaseLocation<double>( &rhsEmit );
            mFunction->ReleaseLocation<double>( &lhsEmit );
            compOp = BinaryComparatorOpCodes[op][BCOT_Double];
        }
        else if (lType.isFloat() && rType.isFloat())
        {
            CheckNodeLocation(lhsEmit, float);
            CheckNodeLocation(rhsEmit, float);
            emitInfo.location = mFunction->AcquireTmpRegister<int>();
            mFunction->ReleaseLocation<float>(&rhsEmit);
            mFunction->ReleaseLocation<float>(&lhsEmit);
            compOp = BinaryComparatorOpCodes[op][BCOT_Float];
        }
        else
        {
            throw AsmJsCompilationException( _u("Type not supported for comparison") );
        }
        mWriter.AsmReg3( compOp, emitInfo.location, lhsEmit.location, rhsEmit.location );
        EndStatement(pnode);
        return emitInfo;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitUnaryPos( ParseNode * pnode )
    {
        ParseNode* rhs = ParserWrapper::GetUnaryNode( pnode );
        EmitExpressionInfo rhsEmit ;
        if (rhs->nop == knopCall)
        {
            rhsEmit = EmitCall(rhs, AsmJsRetType::Double);
        }
        else
        {
            rhsEmit = Emit(rhs);
        }
        const AsmJsType& rType = rhsEmit.type;
        StartStatement(pnode);
        EmitExpressionInfo emitInfo( AsmJsType::Double );
        RegSlot dst;
        if( rType.isUnsigned() )
        {
            CheckNodeLocation( rhsEmit, int );
            dst = mFunction->AcquireTmpRegister<double>();
            mWriter.AsmReg2( OpCodeAsmJs::Conv_UTD, dst, rhsEmit.location );
            mFunction->ReleaseLocation<int>( &rhsEmit );
        }
        else if( rType.isSigned() )
        {
            CheckNodeLocation( rhsEmit, int );
            dst = mFunction->AcquireTmpRegister<double>();
            mWriter.AsmReg2( OpCodeAsmJs::Conv_ITD, dst, rhsEmit.location );
            mFunction->ReleaseLocation<int>( &rhsEmit );
        }
        else if (rType.isMaybeDouble())
        {
            CheckNodeLocation( rhsEmit, double );
            dst = rhsEmit.location;
        }
        else if (rType.isMaybeFloat())
        {
            CheckNodeLocation(rhsEmit, float);
            dst = mFunction->AcquireTmpRegister<double>();
            mWriter.AsmReg2(OpCodeAsmJs::Conv_FTD, dst, rhsEmit.location);
            mFunction->ReleaseLocation<float>(&rhsEmit);
        }
        else
        {
            throw AsmJsCompilationException( _u("Type not supported for unary +") );
        }
        emitInfo.location = dst;
        EndStatement(pnode);
        return emitInfo;
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitUnaryNeg( ParseNode * pnode )
    {
        ParseNode* rhs = ParserWrapper::GetUnaryNode( pnode );
        const EmitExpressionInfo& rhsEmit = Emit( rhs );
        const AsmJsType& rType = rhsEmit.type;
        StartStatement(pnode);
        EmitExpressionInfo emitInfo;
        if( rType.isInt() )
        {
            CheckNodeLocation( rhsEmit, int );
            RegSlot dst = GetAndReleaseUnaryLocations<int>( &rhsEmit );
            emitInfo.type = AsmJsType::Intish;
            mWriter.AsmReg2( OpCodeAsmJs::Neg_Int, dst, rhsEmit.location );
            emitInfo.location = dst;
        }
        else if (rType.isMaybeDouble())
        {
            CheckNodeLocation( rhsEmit, double );
            RegSlot dst = GetAndReleaseUnaryLocations<double>( &rhsEmit );
            emitInfo.type = AsmJsType::Double;
            mWriter.AsmReg2( OpCodeAsmJs::Neg_Db, dst, rhsEmit.location );
            emitInfo.location = dst;
        }
        else if (rType.isMaybeFloat())
        {
            CheckNodeLocation(rhsEmit, float);
            RegSlot dst = GetAndReleaseUnaryLocations<float>(&rhsEmit);
            emitInfo.type = AsmJsType::Floatish;
            mWriter.AsmReg2(OpCodeAsmJs::Neg_Flt, dst, rhsEmit.location);
            emitInfo.location = dst;
        }
        else
        {
            throw AsmJsCompilationException( _u("Type not supported for unary -") );
        }
        EndStatement(pnode);
        return emitInfo;
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitUnaryNot( ParseNode * pnode )
    {
        ParseNode* rhs = ParserWrapper::GetUnaryNode( pnode );
        int count = 1;
        while( rhs->nop == knopNot )
        {
            ++count;
            rhs = ParserWrapper::GetUnaryNode( rhs );
        }
        EmitExpressionInfo rhsEmit = Emit( rhs );
        AsmJsType rType = rhsEmit.type;
        StartStatement(pnode);
        if( count >= 2 && rType.isMaybeDouble() )
        {
            CheckNodeLocation( rhsEmit, double );
            count -= 2;
            RegSlot dst = mFunction->AcquireTmpRegister<int>();
            mWriter.AsmReg2( OpCodeAsmJs::Conv_DTI, dst, rhsEmit.location );
            mFunction->ReleaseLocation<double>( &rhsEmit );

            // allow the converted value to be negated (useful for   ~(~~(+x)) )
            rType = AsmJsType::Signed;
            rhsEmit.location = dst;
        }
        if (count >= 2 && rType.isMaybeFloat())
        {
            CheckNodeLocation(rhsEmit, float);
            count -= 2;
            RegSlot dst = mFunction->AcquireTmpRegister<int>();
            mWriter.AsmReg2(OpCodeAsmJs::Conv_FTI, dst, rhsEmit.location);
            mFunction->ReleaseLocation<float>(&rhsEmit);

            // allow the converted value to be negated (useful for   ~(~~(fround(x))) )
            rType = AsmJsType::Signed;
            rhsEmit.location = dst;
        }
        if( rType.isIntish() )
        {
            if( count & 1 )
            {
                CheckNodeLocation( rhsEmit, int );
                RegSlot dst = GetAndReleaseUnaryLocations<int>( &rhsEmit );
                // do the conversion only if we have an odd number of the operator
                mWriter.AsmReg2( OpCodeAsmJs::Not_Int, dst, rhsEmit.location );
                rhsEmit.location = dst;
            }
            rhsEmit.type = AsmJsType::Signed;
        }
        else
        {
            throw AsmJsCompilationException( _u("Type not supported for unary ~") );
        }
        EndStatement(pnode);
        return rhsEmit;
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitUnaryLogNot( ParseNode * pnode )
    {
        ParseNode* rhs = ParserWrapper::GetUnaryNode( pnode );
        int count = 1;
        while( rhs->nop == knopLogNot )
        {
            ++count;
            rhs = ParserWrapper::GetUnaryNode( rhs );
        }

        const EmitExpressionInfo& rhsEmit = Emit( rhs );
        const AsmJsType& rType = rhsEmit.type;
        StartStatement(pnode);
        EmitExpressionInfo emitInfo( AsmJsType::Int );
        if( rType.isInt() )
        {
            CheckNodeLocation( rhsEmit, int );
            RegSlot dst = GetAndReleaseUnaryLocations<int>( &rhsEmit );
            if( count & 1 )
            {
                // do the conversion only if we have an odd number of the operator
                mWriter.AsmReg2( OpCodeAsmJs::LogNot_Int, dst, rhsEmit.location );
            }
            else
            {
                // otherwise, make sure the result is 0|1
                mWriter.AsmReg2( OpCodeAsmJs::Conv_ITB, dst, rhsEmit.location );
            }
            emitInfo.location = dst;
        }
        else
        {
            throw AsmJsCompilationException( _u("Type not supported for unary !") );
        }
        EndStatement(pnode);
        return emitInfo;
    }


    EmitExpressionInfo AsmJSByteCodeGenerator::EmitBooleanExpression( ParseNode* expr, Js::ByteCodeLabel trueLabel, Js::ByteCodeLabel falseLabel )
    {
        switch( expr->nop )
        {
        case knopLogNot:{
            const EmitExpressionInfo& info = EmitBooleanExpression( expr->AsParseNodeUni()->pnode1, falseLabel, trueLabel );
            return info;
            break;
        }
//         case knopEq:
//         case knopNe:
//         case knopLt:
//         case knopLe:
//         case knopGe:
//         case knopGt:
//             byteCodeGenerator->StartStatement( expr );
//             EmitBinaryOpnds( expr->AsParseNodeBin()->pnode1, expr->AsParseNodeBin()->pnode2, byteCodeGenerator, funcInfo );
//             funcInfo->ReleaseLoc( expr->AsParseNodeBin()->pnode2 );
//             funcInfo->ReleaseLoc( expr->AsParseNodeBin()->pnode1 );
//             mWriter.BrReg2( nopToOp[expr->nop], trueLabel, expr->AsParseNodeBin()->pnode1->location,
//                                                  expr->AsParseNodeBin()->pnode2->location );
//             mWriter.AsmBr( falseLabel );
//             byteCodeGenerator->EndStatement( expr );
//             break;
//         case knopName:
//             byteCodeGenerator->StartStatement( expr );
//             Emit( expr, byteCodeGenerator, funcInfo, false );
//             mWriter.BrReg1( Js::OpCode::BrTrue_A, trueLabel, expr->location );
//             mWriter.AsmBr( falseLabel );
//             byteCodeGenerator->EndStatement( expr );
//             break;
        default:{
            const EmitExpressionInfo& info = Emit( expr );
            if( !info.type.isInt() )
            {
                throw AsmJsCompilationException( _u("Comparison expressions must be type signed") );
            }
            mWriter.AsmBrReg1( Js::OpCodeAsmJs::BrTrue_Int, trueLabel, info.location );
            mWriter.AsmBr( falseLabel );
            return info;
            break;
            }
        }
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitIf( ParseNodeIf * pnode )
    {
        Js::ByteCodeLabel trueLabel = mWriter.DefineLabel();
        Js::ByteCodeLabel falseLabel = mWriter.DefineLabel();
        const EmitExpressionInfo& boolInfo = EmitBooleanExpression( pnode->pnodeCond, trueLabel, falseLabel );
        mFunction->ReleaseLocation<int>( &boolInfo );


        mWriter.MarkAsmJsLabel( trueLabel );

        const EmitExpressionInfo& trueInfo = Emit( pnode->pnodeTrue );
        mFunction->ReleaseLocationGeneric( &trueInfo );

        if( pnode->pnodeFalse != nullptr )
        {
            // has else clause
            Js::ByteCodeLabel skipLabel = mWriter.DefineLabel();

            // Record the branch bytecode offset
            mWriter.RecordStatementAdjustment( Js::FunctionBody::SAT_FromCurrentToNext );

            // then clause skips else clause
            mWriter.AsmBr( skipLabel );
            // generate code for else clause
            mWriter.MarkAsmJsLabel( falseLabel );

            const EmitExpressionInfo& falseInfo = Emit( pnode->pnodeFalse );
            mFunction->ReleaseLocationGeneric( &falseInfo );

            mWriter.MarkAsmJsLabel( skipLabel );

        }
        else
        {
            mWriter.MarkAsmJsLabel( falseLabel );
        }
        if( pnode->emitLabels )
        {
            mWriter.MarkAsmJsLabel( pnode->breakLabel );
        }
        return EmitExpressionInfo( AsmJsType::Void );
    }

    Js::EmitExpressionInfo AsmJSByteCodeGenerator::EmitLoop( ParseNodeLoop *loopNode, ParseNode *cond, ParseNode *body, ParseNode *incr, BOOL doWhile /*= false */ )
    {
        // Need to increment loop count whether we are going to profile or not for HasLoop()
        StartStatement(loopNode);
        Js::ByteCodeLabel loopEntrance = mWriter.DefineLabel();
        Js::ByteCodeLabel continuePastLoop = mWriter.DefineLabel();

        uint loopId = mWriter.EnterLoop( loopEntrance );
        loopNode->loopId = loopId;
        EndStatement(loopNode);
        if( doWhile )
        {
            const EmitExpressionInfo& bodyInfo = Emit( body );
            mFunction->ReleaseLocationGeneric( &bodyInfo );

            if( loopNode->emitLabels )
            {
                mWriter.MarkAsmJsLabel( loopNode->continueLabel );
            }
            if( !ByteCodeGenerator::IsFalse( cond ) )
            {
                const EmitExpressionInfo& condInfo = EmitBooleanExpression( cond, loopEntrance, continuePastLoop );
                mFunction->ReleaseLocationGeneric( &condInfo );
            }
        }
        else
        {
            if( cond )
            {
                Js::ByteCodeLabel trueLabel = mWriter.DefineLabel();
                const EmitExpressionInfo& condInfo = EmitBooleanExpression( cond, trueLabel, continuePastLoop );
                mFunction->ReleaseLocationGeneric( &condInfo );
                mWriter.MarkAsmJsLabel( trueLabel );
            }
            const EmitExpressionInfo& bodyInfo = Emit( body );
            mFunction->ReleaseLocationGeneric( &bodyInfo );

            if( loopNode->emitLabels )
            {
                mWriter.MarkAsmJsLabel( loopNode->continueLabel );
            }
            if( incr != NULL )
            {
                const EmitExpressionInfo& incrInfo = Emit( incr );
                mFunction->ReleaseLocationGeneric( &incrInfo );
            }
            mWriter.AsmBr( loopEntrance );
        }
        mWriter.MarkAsmJsLabel( continuePastLoop );
        if( loopNode->emitLabels )
        {
            mWriter.MarkAsmJsLabel( loopNode->breakLabel );
        }

        mWriter.ExitLoop( loopId );



        return EmitExpressionInfo( AsmJsType::Void );
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitQMark( ParseNode * pnode )
    {
        StartStatement(pnode->AsParseNodeTri()->pnode1);
        Js::ByteCodeLabel trueLabel = mWriter.DefineLabel();
        Js::ByteCodeLabel falseLabel = mWriter.DefineLabel();
        Js::ByteCodeLabel skipLabel = mWriter.DefineLabel();
        EndStatement(pnode->AsParseNodeTri()->pnode1);
        const EmitExpressionInfo& boolInfo = EmitBooleanExpression( pnode->AsParseNodeTri()->pnode1, trueLabel, falseLabel );
        mFunction->ReleaseLocationGeneric( &boolInfo );

        RegSlot intReg = mFunction->AcquireTmpRegister<int>();
        RegSlot doubleReg = mFunction->AcquireTmpRegister<double>();
        RegSlot floatReg = mFunction->AcquireTmpRegister<float>();
        EmitExpressionInfo emitInfo( AsmJsType::Void );


        mWriter.MarkAsmJsLabel( trueLabel );
        const EmitExpressionInfo& trueInfo = Emit( pnode->AsParseNodeTri()->pnode2 );
        StartStatement(pnode->AsParseNodeTri()->pnode2);
        if( trueInfo.type.isInt() )
        {
            mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Int, intReg, trueInfo.location );
            mFunction->ReleaseLocation<int>( &trueInfo );
            mFunction->ReleaseTmpRegister<double>(doubleReg);
            mFunction->ReleaseTmpRegister<float>(floatReg);
            emitInfo.location = intReg;
            emitInfo.type = AsmJsType::Int;
        }
        else if( trueInfo.type.isDouble() )
        {
            mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Db, doubleReg, trueInfo.location );
            mFunction->ReleaseLocation<double>( &trueInfo );
            mFunction->ReleaseTmpRegister<int>( intReg );
            mFunction->ReleaseTmpRegister<float>(floatReg);
            emitInfo.location = doubleReg;
            emitInfo.type = AsmJsType::Double;
        }
        else if (trueInfo.type.isFloat())
        {
            mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Flt, floatReg, trueInfo.location);
            mFunction->ReleaseLocation<float>(&trueInfo);
            mFunction->ReleaseTmpRegister<int>(intReg);
            mFunction->ReleaseTmpRegister<double>(doubleReg);
            emitInfo.location = floatReg;
            emitInfo.type = AsmJsType::Float;
        }
        else
        {
            throw AsmJsCompilationException(_u("Conditional expressions must be of type int, double, or float"));
        }
        mWriter.AsmBr( skipLabel );
        EndStatement(pnode->AsParseNodeTri()->pnode2);
        mWriter.MarkAsmJsLabel( falseLabel );
        const EmitExpressionInfo& falseInfo = Emit( pnode->AsParseNodeTri()->pnode3 );
        StartStatement(pnode->AsParseNodeTri()->pnode3);
        if( falseInfo.type.isInt() )
        {
            if( !trueInfo.type.isInt() )
            {
                throw AsmJsCompilationException( _u("Conditional expressions results must be the same type") );
            }
            mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Int, intReg, falseInfo.location );
            mFunction->ReleaseLocation<int>( &falseInfo );
        }
        else if( falseInfo.type.isDouble() )
        {
            if( !trueInfo.type.isDouble() )
            {
                throw AsmJsCompilationException( _u("Conditional expressions results must be the same type") );
            }
            mWriter.AsmReg2( Js::OpCodeAsmJs::Ld_Db, doubleReg, falseInfo.location );
            mFunction->ReleaseLocation<double>( &falseInfo );
        }
        else if(falseInfo.type.isFloat())
        {
            if (!trueInfo.type.isFloat())
            {
                throw AsmJsCompilationException(_u("Conditional expressions results must be the same type"));
            }
            mWriter.AsmReg2(Js::OpCodeAsmJs::Ld_Flt, floatReg, falseInfo.location);
            mFunction->ReleaseLocation<float>(&falseInfo);
        }
        else
        {
            throw AsmJsCompilationException(_u("Conditional expressions must be of type int, double, or float"));
        }
        mWriter.MarkAsmJsLabel( skipLabel );
        EndStatement(pnode->AsParseNodeTri()->pnode3);
        return emitInfo;
    }

    EmitExpressionInfo AsmJSByteCodeGenerator::EmitSwitch( ParseNodeSwitch * pnode )
    {
        BOOL fHasDefault = false;
        Assert( pnode->pnodeVal != NULL );
        const EmitExpressionInfo& valInfo = Emit( pnode->pnodeVal );

        if( !valInfo.type.isSigned() )
        {
            throw AsmJsCompilationException( _u("Switch value must be type Signed, FixNum") );
        }

        RegSlot regVal = GetAndReleaseUnaryLocations<int>( &valInfo );
        StartStatement(pnode);
        mWriter.AsmReg2(OpCodeAsmJs::BeginSwitch_Int, regVal, valInfo.location);
        EndStatement(pnode);

        // TODO: if all cases are compile-time constants, emit a switch statement in the byte
        // code so the BE can optimize it.

        ParseNodeCase *pnodeCase;
        for( pnodeCase = pnode->pnodeCases; pnodeCase; pnodeCase = pnodeCase->pnodeNext )
        {
            // Jump to the first case body if this one doesn't match. Make sure any side-effects of the case
            // expression take place regardless.
            pnodeCase->labelCase = mWriter.DefineLabel();
            if( pnodeCase == pnode->pnodeDefault )
            {
                fHasDefault = true;
                continue;
            }
            ParseNode* caseExpr = pnodeCase->pnodeExpr;
            if ((caseExpr->nop != knopInt || (caseExpr->AsParseNodeInt()->lw >> 31) > 1) && !ParserWrapper::IsMinInt(caseExpr))
            {
                throw AsmJsCompilationException( _u("Switch case value must be int in the range [-2^31, 2^31)") );
            }

            const EmitExpressionInfo& caseExprInfo = Emit( pnodeCase->pnodeExpr );
            mWriter.AsmBrReg2( OpCodeAsmJs::Case_Int, pnodeCase->labelCase, regVal, caseExprInfo.location );
            // do not need to release location because int constants cannot be released
        }

        // No explicit case value matches. Jump to the default arm (if any) or break out altogether.
        if( fHasDefault )
        {
            mWriter.AsmBr( pnode->pnodeDefault->labelCase, OpCodeAsmJs::EndSwitch_Int );
        }
        else
        {
            if( !pnode->emitLabels )
            {
                pnode->breakLabel = mWriter.DefineLabel();
            }
            mWriter.AsmBr( pnode->breakLabel, OpCodeAsmJs::EndSwitch_Int );
        }
        // Now emit the case arms to which we jump on matching a case value.
        for( pnodeCase = pnode->pnodeCases; pnodeCase; pnodeCase = pnodeCase->pnodeNext )
        {
            mWriter.MarkAsmJsLabel( pnodeCase->labelCase );
            const EmitExpressionInfo& caseBodyInfo = Emit( pnodeCase->pnodeBody );
            mFunction->ReleaseLocationGeneric( &caseBodyInfo );
        }

        mFunction->ReleaseTmpRegister<int>( regVal );

        if( !fHasDefault || pnode->emitLabels )
        {
            mWriter.MarkAsmJsLabel( pnode->breakLabel );
        }

        return EmitExpressionInfo( AsmJsType::Void );
    }

    void AsmJSByteCodeGenerator::EmitEmptyByteCode(FuncInfo * funcInfo, ByteCodeGenerator * byteCodeGen, ParseNode * functionNode)
    {
        funcInfo->byteCodeFunction->SetGrfscr(byteCodeGen->GetFlags());
        funcInfo->byteCodeFunction->SetSourceInfo(byteCodeGen->GetCurrentSourceIndex(),
            funcInfo->root,
            !!(byteCodeGen->GetFlags() & fscrEvalCode),
            ((byteCodeGen->GetFlags() & fscrDynamicCode) && !(byteCodeGen->GetFlags() & fscrEvalCode)));

        FunctionBody * functionBody = funcInfo->byteCodeFunction->GetFunctionBody();

        class AutoCleanup
        {
        private:
            FunctionBody * mFunctionBody;
            ByteCodeGenerator * mByteCodeGen;
        public:
            AutoCleanup(FunctionBody * functionBody, ByteCodeGenerator * byteCodeGen) : mFunctionBody(functionBody), mByteCodeGen(byteCodeGen)
            {
            }

            void Done()
            {
                mFunctionBody = nullptr;
            }
            ~AutoCleanup()
            {
                if (mFunctionBody)
                {
                    mFunctionBody->ResetByteCodeGenState();
                    mByteCodeGen->Writer()->Reset();
                }
            }
        } autoCleanup(functionBody, byteCodeGen);

        byteCodeGen->Writer()->Begin(functionBody, byteCodeGen->GetAllocator(), false, false, false);
        byteCodeGen->Writer()->StartStatement(functionNode, 0);
        byteCodeGen->Writer()->Empty(OpCode::Nop);
        byteCodeGen->Writer()->EndStatement(functionNode);
        byteCodeGen->Writer()->End();

        functionBody->CheckAndSetConstantCount(FuncInfo::InitialConstRegsCount);

        autoCleanup.Done();
    }

    void AsmJSByteCodeGenerator::StartStatement(ParseNode* pnode)
    {
        mWriter.StartStatement(pnode, 0);
        //         Output::Print( _u("%*s+%d\n"),tab, " ", pnode->ichMin );
        //         ++tab;
    }

    void AsmJSByteCodeGenerator::EndStatement(ParseNode* pnode)
    {
        mWriter.EndStatement(pnode);
        //         Output::Print( _u("%*s-%d\n"),tab, " ", pnode->ichMin );
        //         --tab;
    }

   // int tab = 0;
    void AsmJSByteCodeGenerator::LoadModuleInt( RegSlot dst, RegSlot index )
    {
        mWriter.AsmSlot(OpCodeAsmJs::LdSlot_Int, dst, AsmJsFunctionMemory::ModuleEnvRegister, index + (int32)(mCompiler->GetIntOffset() / WAsmJs::INT_SLOTS_SPACE + 0.5));
    }
    void AsmJSByteCodeGenerator::LoadModuleFloat(RegSlot dst, RegSlot index)
    {
        mWriter.AsmSlot(OpCodeAsmJs::LdSlot_Flt, dst, AsmJsFunctionMemory::ModuleEnvRegister, index + (int32)(mCompiler->GetFloatOffset() / WAsmJs::FLOAT_SLOTS_SPACE + 0.5));
    }
    void AsmJSByteCodeGenerator::LoadModuleDouble( RegSlot dst, RegSlot index )
    {
        mWriter.AsmSlot(OpCodeAsmJs::LdSlot_Db, dst, AsmJsFunctionMemory::ModuleEnvRegister, index + mCompiler->GetDoubleOffset() / WAsmJs::DOUBLE_SLOTS_SPACE);
    }

    void AsmJSByteCodeGenerator::LoadModuleFFI( RegSlot dst, RegSlot index )
    {
        mWriter.AsmSlot(OpCodeAsmJs::LdSlot, dst, AsmJsFunctionMemory::ModuleEnvRegister, index + mCompiler->GetFFIOffset());
    }

    void AsmJSByteCodeGenerator::LoadModuleFunction( RegSlot dst, RegSlot index )
    {
        mWriter.AsmSlot(OpCodeAsmJs::LdSlot, dst, AsmJsFunctionMemory::ModuleEnvRegister, index + mCompiler->GetFuncOffset());
    }

    void AsmJSByteCodeGenerator::LoadModuleFunctionTable( RegSlot dst, RegSlot FuncTableIndex, RegSlot FuncIndexLocation )
    {
        RegSlot slotReg = mFunction->AcquireTmpRegister<intptr_t>();
        mWriter.AsmSlot( OpCodeAsmJs::LdSlotArr, slotReg, AsmJsFunctionMemory::ModuleEnvRegister, FuncTableIndex+mCompiler->GetFuncPtrOffset() );
        mWriter.AsmSlot( OpCodeAsmJs::LdArr_Func, dst, slotReg, FuncIndexLocation );

        mFunction->ReleaseTmpRegister<intptr_t>(slotReg);
    }

    void AsmJSByteCodeGenerator::SetModuleInt( Js::RegSlot dst, RegSlot src )
    {
        mWriter.AsmSlot(OpCodeAsmJs::StSlot_Int, src, AsmJsFunctionMemory::ModuleEnvRegister, dst + (int32)(mCompiler->GetIntOffset() / WAsmJs::INT_SLOTS_SPACE + 0.5));
    }

    void AsmJSByteCodeGenerator::SetModuleFloat(Js::RegSlot dst, RegSlot src)
    {
        mWriter.AsmSlot(OpCodeAsmJs::StSlot_Flt, src, AsmJsFunctionMemory::ModuleEnvRegister, dst + (int32)(mCompiler->GetFloatOffset() / WAsmJs::FLOAT_SLOTS_SPACE + 0.5));
    }

    void AsmJSByteCodeGenerator::SetModuleDouble( Js::RegSlot dst, RegSlot src )
    {
        mWriter.AsmSlot(OpCodeAsmJs::StSlot_Db, src, AsmJsFunctionMemory::ModuleEnvRegister, dst + mCompiler->GetDoubleOffset() / WAsmJs::DOUBLE_SLOTS_SPACE);
    }

    void AsmJsFunctionCompilation::CleanUp()
    {
        if( mGenerator && mGenerator->mInfo )
        {
            FunctionBody* body = mGenerator->mFunction->GetFuncBody();
            if( body )
            {
                body->ResetByteCodeGenState();
            }
            mGenerator->mWriter.Reset();
        }
    }

}
#endif
