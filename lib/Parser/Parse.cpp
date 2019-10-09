//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "ParserPch.h"
#include "FormalsUtil.h"
#include "../Runtime/Language/SourceDynamicProfileManager.h"

#include "ByteCode/ByteCodeSerializer.h"

#if DBG_DUMP
void PrintPnodeWIndent(ParseNode *pnode, int indentAmt);
#endif

const char* const nopNames[knopLim] = {
#define PTNODE(nop,sn,pc,nk,grfnop,json) sn,
#include "ptlist.h"
};
void printNop(int nop) {
    Output::Print(_u("%S\n"), nopNames[nop]);
}

const uint ParseNode::mpnopgrfnop[knopLim] =
{
#define PTNODE(nop,sn,pc,nk,grfnop,json) grfnop,
#include "ptlist.h"
};

bool Parser::IsES6DestructuringEnabled() const
{
    return m_scriptContext->GetConfig()->IsES6DestructuringEnabled();
}

struct BlockInfoStack
{
    StmtNest pstmt;
    ParseNodeBlock *pnodeBlock;
    ParseNodePtr *m_ppnodeLex;              // lexical variable list tail
    BlockInfoStack *pBlockInfoOuter;        // containing block's BlockInfoStack
    BlockInfoStack *pBlockInfoFunction;     // nearest function's BlockInfoStack (if pnodeBlock is a function, this points to itself)
};

#if DEBUG
Parser::Parser(Js::ScriptContext* scriptContext, BOOL strictMode, PageAllocator *alloc, bool isBackground, size_t size)
#else
Parser::Parser(Js::ScriptContext* scriptContext, BOOL strictMode, PageAllocator *alloc, bool isBackground)
#endif
    : m_nodeAllocator(_u("Parser"), alloc ? alloc : scriptContext->GetThreadContext()->GetPageAllocator(), Parser::OutOfMemory),
    m_cactIdentToNodeLookup(0),
    m_grfscr(fscrNil),
    m_length(0),
    m_originalLength(0),
    m_nextFunctionId(nullptr),
    m_sourceContextInfo(nullptr),
#if ENABLE_BACKGROUND_PARSING
    m_isInBackground(isBackground),
    m_hasParallelJob(false),
    m_doingFastScan(false),
#endif
    m_nextBlockId(0),
    m_tempGuestArenaReleased(false),
    m_tempGuestArena(scriptContext->GetTemporaryGuestAllocator(_u("ParserRegex")), scriptContext->GetRecycler()),
    // use the GuestArena directly for keeping the RegexPattern* alive during byte code generation
    m_registeredRegexPatterns(m_tempGuestArena->GetAllocator()),

    m_scriptContext(scriptContext),
    m_token(), // should initialize to 0/nullptrs
    m_scan(this, &m_token, scriptContext),

    m_currentNodeNonLambdaFunc(nullptr),
    m_currentNodeNonLambdaDeferredFunc(nullptr),
    m_currentNodeFunc(nullptr),
    m_currentNodeDeferredFunc(nullptr),
    m_currentNodeProg(nullptr),
    m_currDeferredStub(nullptr),
    m_currDeferredStubCount(0),
    m_pCurrentAstSize(nullptr),
    m_ppnodeScope(nullptr),
    m_ppnodeExprScope(nullptr),
    m_ppnodeVar(nullptr),
    m_inDeferredNestedFunc(false),
    m_reparsingLambdaParams(false),
    m_disallowImportExportStmt(false),
    m_isInParsingArgList(false),
    m_hasDestructuringPattern(false),
    m_hasDeferredShorthandInitError(false),
    m_deferCommaError(false),
    m_pnestedCount(nullptr),

    wellKnownPropertyPids(), // should initialize to nullptrs
    m_sourceLim(0),
    m_functionBody(nullptr),
    m_parseType(ParseType_Upfront),

    m_arrayDepth(0),
    m_funcInArrayDepth(0),
    m_funcInArray(0),
    m_scopeCountNoAst(0),

    m_funcParenExprDepth(0),
    m_deferEllipsisError(false),
    m_deferEllipsisErrorLoc(), // calls default initializer
    m_deferCommaErrorLoc(),
    m_tryCatchOrFinallyDepth(0),

    m_pstmtCur(nullptr),
    m_currentBlockInfo(nullptr),
    m_currentScope(nullptr),

    currBackgroundParseItem(nullptr),
    backgroundParseItems(nullptr),
    fastScannedRegExpNodes(nullptr),

    m_currentDynamicBlock(nullptr),

    m_UsesArgumentsAtGlobal(false),

    m_fUseStrictMode(strictMode),
    m_InAsmMode(false),
    m_deferAsmJs(true),
    m_fExpectExternalSource(FALSE),
    m_deferringAST(FALSE),
    m_stoppedDeferredParse(FALSE)
{
    AssertMsg(size == sizeof(Parser), "verify conditionals affecting the size of Parser agree");
    Assert(scriptContext != nullptr);

    // init PID members
    InitPids();
}

Parser::~Parser(void)
{
    this->ReleaseTemporaryGuestArena();

#if ENABLE_BACKGROUND_PARSING
    if (this->m_hasParallelJob)
    {
        // Let the background threads know that they can decommit their arena pages.
        BackgroundParser *bgp = m_scriptContext->GetBackgroundParser();
        Assert(bgp);
        if (bgp->Processor()->ProcessesInBackground())
        {
            JsUtil::BackgroundJobProcessor *processor = static_cast<JsUtil::BackgroundJobProcessor*>(bgp->Processor());

            bool result = processor->IterateBackgroundThreads([&](JsUtil::ParallelThreadData *threadData)->bool {
                threadData->canDecommit = true;
                return false;
            });
            Assert(result);
        }
    }
#endif
}

void Parser::OutOfMemory()
{
    throw ParseExceptionObject(ERRnoMemory);
}

void Parser::Error(HRESULT hr)
{
    throw ParseExceptionObject(hr);
}

void Parser::Error(HRESULT hr, ParseNodePtr pnode)
{
    if (pnode && pnode->ichLim)
    {
        Error(hr, pnode->ichMin, pnode->ichLim);
    }
    else
    {
        Error(hr);
    }
}

void Parser::Error(HRESULT hr, charcount_t ichMin, charcount_t ichLim)
{
    this->GetScanner()->SetErrorPosition(ichMin, ichLim);
    Error(hr);
}

void Parser::IdentifierExpectedError(const Token& token)
{
    Assert(token.tk != tkID);

    HRESULT hr;
    if (token.IsReservedWord())
    {
        if (token.IsKeyword())
        {
            hr = ERRKeywordNotId;
        }
        else
        {
            Assert(token.IsFutureReservedWord(true));
            if (token.IsFutureReservedWord(false))
            {
                // Future reserved word in strict and non-strict modes
                hr = ERRFutureReservedWordNotId;
            }
            else
            {
                // Future reserved word only in strict mode. The token would have been converted to tkID by the scanner if not
                // in strict mode.
                Assert(IsStrictMode());
                hr = ERRFutureReservedWordInStrictModeNotId;
            }
        }
    }
    else
    {
        hr = ERRnoIdent;
    }

    Error(hr);
}

HRESULT Parser::ValidateSyntax(LPCUTF8 pszSrc, size_t encodedCharCount, bool isGenerator, bool isAsync, CompileScriptException *pse, void (Parser::*validateFunction)())
{
    Assert(pszSrc);

    PROBE_STACK_NO_DISPOSE(m_scriptContext, Js::Constants::MinStackDefault);

    HRESULT hr;
    SmartFPUControl smartFpuControl;

    BOOL fDeferSave = m_deferringAST;
    try
    {
        hr = NOERROR;

        m_length = encodedCharCount;
        m_originalLength = encodedCharCount;

        // make sure deferred parsing is turned off
        ULONG grfscr = fscrNil;

        // Give the scanner the source and get the first token
        this->GetScanner()->SetText(pszSrc, 0, encodedCharCount, 0, false, grfscr);
        this->GetScanner()->SetYieldIsKeywordRegion(isGenerator);
        this->GetScanner()->SetAwaitIsKeywordRegion(isAsync);
        this->GetScanner()->Scan();

        uint nestedCount = 0;
        m_pnestedCount = &nestedCount;

        ParseNodePtr pnodeScope = nullptr;
        m_ppnodeScope = &pnodeScope;
        m_ppnodeExprScope = nullptr;

        uint nextFunctionId = 0;
        m_nextFunctionId = &nextFunctionId;

        m_inDeferredNestedFunc = false;
        m_deferringAST = true;

        m_nextBlockId = 0;

        ParseNodeFnc *pnodeFnc = CreateAllowDeferNodeForOpT<knopFncDecl>();
        pnodeFnc->SetIsGenerator(isGenerator);
        pnodeFnc->SetIsAsync(isAsync);

        m_ppnodeVar = &pnodeFnc->pnodeVars;
        m_currentNodeFunc = pnodeFnc;
        m_currentNodeDeferredFunc = NULL;
        m_sourceContextInfo = nullptr;
        AssertMsg(m_pstmtCur == NULL, "Statement stack should be empty when we start parse function body");

        ParseNodeBlock * block = StartParseBlock<false>(PnodeBlockType::Function, ScopeType_FunctionBody);
        (this->*validateFunction)();
        FinishParseBlock(block);

        pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
        pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
        pnodeFnc->pnodeVars = nullptr;

        // there should be nothing after successful parsing for a given construct
        if (m_token.tk != tkEOF)
            Error(ERRsyntax);

        m_deferringAST = fDeferSave;
    }
    catch (ParseExceptionObject& e)
    {
        m_deferringAST = fDeferSave;
        hr = e.GetError();
    }

    if (nullptr != pse && FAILED(hr))
    {
        hr = pse->ProcessError(this->GetScanner(), hr, /* pnodeBase */ NULL);
    }
    return hr;
}

HRESULT Parser::ParseSourceInternal(
    __out ParseNodeProg ** parseTree, LPCUTF8 pszSrc, size_t offsetInBytes, size_t encodedCharCount, charcount_t offsetInChars,
    bool isUtf8, ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber, SourceContextInfo * sourceContextInfo)
{
    Assert(parseTree);
    Assert(pszSrc);

    if (this->IsBackgroundParser())
    {
        PROBE_STACK_NO_DISPOSE(m_scriptContext, Js::Constants::MinStackDefault);
    }
    else
    {
        PROBE_STACK(m_scriptContext, Js::Constants::MinStackDefault);
    }

#ifdef PROFILE_EXEC
    m_scriptContext->ProfileBegin(Js::ParsePhase);
#endif
    JS_ETW_INTERNAL(EventWriteJSCRIPT_PARSE_START(m_scriptContext, 0));

    *parseTree = NULL;
    m_sourceLim = 0;

    m_grfscr = grfscr;
    m_sourceContextInfo = sourceContextInfo;

    ParseNodeProg * pnodeBase = NULL;
    HRESULT hr;
    SmartFPUControl smartFpuControl;

    try
    {
        if ((grfscr & fscrIsModuleCode) != 0)
        {
            // Module source flag should not be enabled unless module is enabled
            Assert(m_scriptContext->GetConfig()->IsES6ModuleEnabled());

            // Module code is always strict mode code.
            this->m_fUseStrictMode = TRUE;
        }

        // parse the source
        pnodeBase = Parse(pszSrc, offsetInBytes, encodedCharCount, offsetInChars, isUtf8, grfscr, lineNumber, nextFunctionId, pse);

        Assert(pnodeBase);

        // Record the actual number of words parsed.
        m_sourceLim = pnodeBase->ichLim - offsetInChars;

        // TODO: The assert can be false positive in some scenarios and chuckj to fix it later
        // Assert(utf8::ByteIndexIntoCharacterIndex(pszSrc + offsetInBytes, encodedCharCount, isUtf8 ? utf8::doDefault : utf8::doAllowThreeByteSurrogates) == m_sourceLim);

#if DBG_DUMP
        if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::ParsePhase))
        {
            PrintPnodeWIndent(pnodeBase, 4);
            fflush(stdout);
        }
#endif

        *parseTree = pnodeBase;

        hr = NOERROR;
    }
    catch (ParseExceptionObject& e)
    {
        hr = e.GetError();
    }
    catch (Js::AsmJsParseException&)
    {
        hr = JSERR_AsmJsCompileError;
    }

    if (FAILED(hr))
    {
        hr = pse->ProcessError(this->GetScanner(), hr, pnodeBase);
    }

#if ENABLE_BACKGROUND_PARSING
    if (this->m_hasParallelJob)
    {
        ///// Wait here for remaining jobs to finish. Then look for errors, do final const bindings.
        // pleath TODO: If there are remaining jobs, let the main thread help finish them.
        BackgroundParser *bgp = m_scriptContext->GetBackgroundParser();
        Assert(bgp);

        CompileScriptException se;
        this->WaitForBackgroundJobs(bgp, &se);

        BackgroundParseItem *failedItem = bgp->GetFailedBackgroundParseItem();
        if (failedItem)
        {
            CompileScriptException *bgPse = failedItem->GetPSE();
            Assert(bgPse);
            *pse = *bgPse;
            hr = failedItem->GetHR();
            bgp->SetFailedBackgroundParseItem(nullptr);
        }

        if (this->fastScannedRegExpNodes != nullptr)
        {
            this->FinishBackgroundRegExpNodes();
        }

        for (BackgroundParseItem *item = this->backgroundParseItems; item; item = item->GetNext())
        {
            Parser *parser = item->GetParser();
            parser->FinishBackgroundPidRefs(item, this != parser);
        }
    }
#endif

    // done with the scanner
    this->GetScanner()->Clear();

#ifdef PROFILE_EXEC
    m_scriptContext->ProfileEnd(Js::ParsePhase);
#endif
    JS_ETW_INTERNAL(EventWriteJSCRIPT_PARSE_STOP(m_scriptContext, 0));

    return hr;
}

#if ENABLE_BACKGROUND_PARSING
void Parser::WaitForBackgroundJobs(BackgroundParser *bgp, CompileScriptException *pse)
{
    // The scan of the script is done, but there may be unfinished background jobs in the queue.
    // Enlist the main thread to help with those.
    BackgroundParseItem *item;
    if (!*bgp->GetPendingBackgroundItemsPtr())
    {
        // We're done.
        return;
    }

    // Save parser state, since we'll need to restore it in order to bind references correctly later.
    this->m_isInBackground = true;
    this->SetCurrBackgroundParseItem(nullptr);
    uint blockIdSave = this->m_nextBlockId;
    uint functionIdSave = *this->m_nextFunctionId;
    StmtNest *pstmtSave = this->m_pstmtCur;

    if (!bgp->Processor()->ProcessesInBackground())
    {
        // No background thread. Just walk the jobs with no locking and process them.
        for (item = bgp->GetNextUnprocessedItem(); item; item = bgp->GetNextUnprocessedItem())
        {
            bgp->Processor()->RemoveJob(item);
            bool succeeded = bgp->Process(item, this, pse);
            bgp->JobProcessed(item, succeeded);
        }
        Assert(!*bgp->GetPendingBackgroundItemsPtr());
    }
    else
    {
        // Background threads. We need to have the critical section in order to:
        // - Check for unprocessed jobs;
        // - Remove jobs from the processor queue;
        // - Do JobsProcessed work (such as removing jobs from the BackgroundParser's unprocessed list).
        CriticalSection *pcs = static_cast<JsUtil::BackgroundJobProcessor*>(bgp->Processor())->GetCriticalSection();
        pcs->Enter();
        for (;;)
        {
            // Grab a job (in lock)
            item = bgp->GetNextUnprocessedItem();
            if (item == nullptr)
            {
                break;
            }
            bgp->Processor()->RemoveJob(item);
            pcs->Leave();

            // Process job (if there is one) (outside lock)
            bool succeeded = bgp->Process(item, this, pse);

            pcs->Enter();
            bgp->JobProcessed(item, succeeded);
        }
        pcs->Leave();

        // Wait for the background threads to finish jobs they're already processing (if any).
        // TODO: Replace with a proper semaphore.
        while (*bgp->GetPendingBackgroundItemsPtr());
    }

    Assert(!*bgp->GetPendingBackgroundItemsPtr());

    // Restore parser state.
    this->m_pstmtCur = pstmtSave;
    this->m_isInBackground = false;
    this->m_nextBlockId = blockIdSave;
    *this->m_nextFunctionId = functionIdSave;
}

void Parser::FinishBackgroundPidRefs(BackgroundParseItem *item, bool isOtherParser)
{
    for (BlockInfoStack *blockInfo = item->GetParseContext()->currentBlockInfo; blockInfo; blockInfo = blockInfo->pBlockInfoOuter)
    {
        if (isOtherParser)
        {
            this->BindPidRefs<true>(blockInfo, item->GetMaxBlockId());
        }
        else
        {
            this->BindPidRefs<false>(blockInfo, item->GetMaxBlockId());
        }
    }
}

void Parser::FinishBackgroundRegExpNodes()
{
    // We have a list of RegExp nodes that we saw on the UI thread in functions we're parallel parsing,
    // and for each background job we have a list of RegExp nodes for which we couldn't allocate patterns.
    // We need to copy the pattern pointers from the UI thread nodes to the corresponding nodes on the
    // background nodes.
    // There may be UI thread nodes for which there are no background thread equivalents, because the UI thread
    // has to assume that the background thread won't defer anything.

    // Note that because these lists (and the list of background jobs) are SList's built by prepending, they are
    // all in reverse lexical order.

    Assert(!this->IsBackgroundParser());
    Assert(this->fastScannedRegExpNodes);
    Assert(this->backgroundParseItems != nullptr);

    BackgroundParseItem *currBackgroundItem;

#if DBG
    for (currBackgroundItem = this->backgroundParseItems;
        currBackgroundItem;
        currBackgroundItem = currBackgroundItem->GetNext())
    {
        if (currBackgroundItem->RegExpNodeList())
        {
            FOREACH_DLIST_ENTRY(ParseNodePtr, ArenaAllocator, pnode, currBackgroundItem->RegExpNodeList())
            {
                Assert(pnode->AsParseNodeRegExp()->regexPattern == nullptr);
            }
            NEXT_DLIST_ENTRY;
        }
    }
#endif

    // Hook up the patterns allocated on the main thread to the nodes created on the background thread.
    // Walk the list of foreground nodes, advancing through the work items and looking up each item.
    // Note that the background thread may have chosen to defer a given RegEx literal, so not every foreground
    // node will have a matching background node. Doesn't matter for correctness.
    // (It's inefficient, of course, to have to restart the inner loop from the beginning of the work item's
    // list, but it should be unusual to have many RegExes in a single work item's chunk of code. Figure out how
    // to start the inner loop from a known internal node within the list if that turns out to be important.)
    currBackgroundItem = this->backgroundParseItems;
    FOREACH_DLIST_ENTRY(ParseNodePtr, ArenaAllocator, pnodeFgnd, this->fastScannedRegExpNodes)
    {
        Assert(pnodeFgnd->nop == knopRegExp);
        Assert(pnodeFgnd->AsParseNodeRegExp()->regexPattern != nullptr);
        bool quit = false;

        while (!quit)
        {
            // Find the next work item with a RegEx in it.
            while (currBackgroundItem && currBackgroundItem->RegExpNodeList() == nullptr)
            {
                currBackgroundItem = currBackgroundItem->GetNext();
            }
            if (!currBackgroundItem)
            {
                break;
            }

            // Walk the RegExps in the work item.
            FOREACH_DLIST_ENTRY(ParseNodePtr, ArenaAllocator, pnodeBgnd, currBackgroundItem->RegExpNodeList())
            {
                Assert(pnodeBgnd->nop == knopRegExp);

                if (pnodeFgnd->ichMin <= pnodeBgnd->ichMin)
                {
                    // Either we found a match, or the next background node is past the foreground node.
                    // In any case, we can stop searching.
                    if (pnodeFgnd->ichMin == pnodeBgnd->ichMin)
                    {
                        Assert(pnodeFgnd->ichLim == pnodeBgnd->ichLim);
                        pnodeBgnd->AsParseNodeRegExp()->regexPattern = pnodeFgnd->AsParseNodeRegExp()->regexPattern;
                    }
                    quit = true;
                    break;
                }
            }
            NEXT_DLIST_ENTRY;

            if (!quit)
            {
                // Need to advance to the next work item.
                currBackgroundItem = currBackgroundItem->GetNext();
            }
        }
    }
    NEXT_DLIST_ENTRY;

#if DBG
    for (currBackgroundItem = this->backgroundParseItems;
        currBackgroundItem;
        currBackgroundItem = currBackgroundItem->GetNext())
    {
        if (currBackgroundItem->RegExpNodeList())
        {
            FOREACH_DLIST_ENTRY(ParseNodePtr, ArenaAllocator, pnode, currBackgroundItem->RegExpNodeList())
            {
                Assert(pnode->AsParseNodeRegExp()->regexPattern != nullptr);
            }
            NEXT_DLIST_ENTRY;
        }
    }
#endif
}
#endif

LabelId* Parser::CreateLabelId(IdentPtr pid)
{
    LabelId* pLabelId;

    pLabelId = (LabelId*)m_nodeAllocator.Alloc(sizeof(LabelId));
    if (NULL == pLabelId)
        Error(ERRnoMemory);
    pLabelId->pid = pid;
    pLabelId->next = NULL;

    return pLabelId;
}

/*****************************************************************************
The following set of routines allocate parse tree nodes of various kinds.
They catch an exception on out of memory.
*****************************************************************************/
void
Parser::AddAstSize(int size)
{
    Assert(!this->m_deferringAST);
    Assert(m_pCurrentAstSize != NULL);
    *m_pCurrentAstSize += size;
}

void
Parser::AddAstSizeAllowDefer(int size)
{
    if (!this->m_deferringAST)
    {
        AddAstSize(size);
    }
}

// StaticCreate
ParseNodeVar * Parser::StaticCreateTempNode(ParseNode* initExpr, ArenaAllocator * alloc)
{
    ParseNodeVar * pnode = Anew(alloc, ParseNodeVar, knopTemp, 0, 0, nullptr);
    pnode->pnodeInit = initExpr;
    return pnode;
}

ParseNodeUni * Parser::StaticCreateTempRef(ParseNode* tempNode, ArenaAllocator * alloc)
{
    return Anew(alloc, ParseNodeUni, knopTempRef, 0, 0, tempNode);
}

// Create Node with limit
template <OpCode nop>
typename OpCodeTrait<nop>::ParseNodeType * Parser::CreateNodeForOpT(charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);
    typename OpCodeTrait<nop>::ParseNodeType * pnode = StaticCreateNodeT<nop>(&m_nodeAllocator, ichMin, ichLim);
    AddAstSize(sizeof(typename OpCodeTrait<nop>::ParseNodeType));
    return pnode;
}

template <OpCode nop>
typename OpCodeTrait<nop>::ParseNodeType * Parser::CreateAllowDeferNodeForOpT(charcount_t ichMin, charcount_t ichLim)
{
    CompileAssert(OpCodeTrait<nop>::AllowDefer);
    typename OpCodeTrait<nop>::ParseNodeType * pnode = StaticCreateNodeT<nop>(&m_nodeAllocator, ichMin, ichLim);
    AddAstSizeAllowDefer(sizeof(typename OpCodeTrait<nop>::ParseNodeType));
    return pnode;
}


#if DBG
static const int g_mpnopcbNode[] =
{
#define PTNODE(nop,sn,pc,nk,ok,json) sizeof(ParseNode##nk),
#include "ptlist.h"
};

void VerifyNodeSize(OpCode nop, int size)
{
    Assert(nop >= 0 && nop < knopLim);
    __analysis_assume(nop < knopLim);
    Assert(g_mpnopcbNode[nop] == size);
}
#endif

// Create ParseNodeUni
ParseNodeUni * Parser::CreateUniNode(OpCode nop, ParseNodePtr pnode1)
{
    charcount_t ichMin;
    charcount_t ichLim;

    if (nullptr == pnode1)
    {
        // no ops
        ichMin = this->GetScanner()->IchMinTok();
        ichLim = this->GetScanner()->IchLimTok();
    }
    else
    {
        // 1 op
        ichMin = pnode1->ichMin;
        ichLim = pnode1->ichLim;
        this->CheckArguments(pnode1);
    }
    return CreateUniNode(nop, pnode1, ichMin, ichLim);
}

ParseNodeUni * Parser::CreateUniNode(OpCode nop, ParseNodePtr pnode1, charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);
    DebugOnly(VerifyNodeSize(nop, sizeof(ParseNodeUni)));
    ParseNodeUni * pnode = Anew(&m_nodeAllocator, ParseNodeUni, nop, ichMin, ichLim, pnode1);
    AddAstSize(sizeof(ParseNodeUni));
    return pnode;
}

// Create ParseNodeBin
ParseNodeBin * Parser::StaticCreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, ArenaAllocator* alloc, charcount_t ichMin, charcount_t ichLim)
{
    DebugOnly(VerifyNodeSize(nop, sizeof(ParseNodeBin)));
    return Anew(alloc, ParseNodeBin, nop, ichMin, ichLim, pnode1, pnode2);
}

ParseNodeBin * Parser::CreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2)
{
    Assert(!this->m_deferringAST);
    charcount_t ichMin;
    charcount_t ichLim;

    if (nullptr == pnode1)
    {
        // no ops
        Assert(nullptr == pnode2);
        ichMin = this->GetScanner()->IchMinTok();
        ichLim = this->GetScanner()->IchLimTok();
    }
    else
    {
        if (nullptr == pnode2)
        {
            // 1 op
            ichMin = pnode1->ichMin;
            ichLim = pnode1->ichLim;
        }
        else
        {
            // 2 ops
            ichMin = pnode1->ichMin;
            ichLim = pnode2->ichLim;
            if (nop != knopDot && nop != knopIndex)
            {
                this->CheckArguments(pnode2);
            }
        }
        if (nop != knopDot && nop != knopIndex)
        {
            this->CheckArguments(pnode1);
        }
    }

    return CreateBinNode(nop, pnode1, pnode2, ichMin, ichLim);
}


ParseNodeBin * Parser::CreateBinNode(OpCode nop, ParseNodePtr pnode1,
    ParseNodePtr pnode2, charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);
    ParseNodeBin * pnode = StaticCreateBinNode(nop, pnode1, pnode2, &m_nodeAllocator, ichMin, ichLim);
    AddAstSize(sizeof(ParseNodeBin));
    return pnode;
}

// Create ParseNodeTri
ParseNodeTri * Parser::CreateTriNode(OpCode nop, ParseNodePtr pnode1,
    ParseNodePtr pnode2, ParseNodePtr pnode3)
{
    charcount_t ichMin;
    charcount_t ichLim;

    if (nullptr == pnode1)
    {
        // no ops
        Assert(nullptr == pnode2);
        Assert(nullptr == pnode3);
        ichMin = this->GetScanner()->IchMinTok();
        ichLim = this->GetScanner()->IchLimTok();
    }
    else if (nullptr == pnode2)
    {
        // 1 op
        Assert(nullptr == pnode3);
        ichMin = pnode1->ichMin;
        ichLim = pnode1->ichLim;
    }
    else if (nullptr == pnode3)
    {
        // 2 op
        ichMin = pnode1->ichMin;
        ichLim = pnode2->ichLim;
    }
    else
    {
        // 3 ops
        ichMin = pnode1->ichMin;
        ichLim = pnode3->ichLim;
    }

    return CreateTriNode(nop, pnode1, pnode2, pnode3, ichMin, ichLim);
}

ParseNodeTri * Parser::CreateTriNode(OpCode nop, ParseNodePtr pnode1,
    ParseNodePtr pnode2, ParseNodePtr pnode3,
    charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);
    DebugOnly(VerifyNodeSize(nop, sizeof(ParseNodeTri)));
    ParseNodeTri * pnode = Anew(&m_nodeAllocator, ParseNodeTri, nop, ichMin, ichLim);
    AddAstSize(sizeof(ParseNodeTri));

    pnode->pnode1 = pnode1;
    pnode->pnode2 = pnode2;
    pnode->pnode3 = pnode3;

    return pnode;
}

// Create ParseNodeBlock
ParseNodeBlock *
Parser::StaticCreateBlockNode(ArenaAllocator* alloc, charcount_t ichMin, charcount_t ichLim, int blockId, PnodeBlockType blockType)
{
    return Anew(alloc, ParseNodeBlock, ichMin, ichLim, blockId, blockType);
}

ParseNodeBlock * Parser::CreateBlockNode(PnodeBlockType blockType)
{
    return CreateBlockNode(this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok(), blockType);
}

ParseNodeBlock * Parser::CreateBlockNode(charcount_t ichMin, charcount_t ichLim, PnodeBlockType blockType)
{
    Assert(OpCodeTrait<knopBlock>::AllowDefer);
    ParseNodeBlock * pnode = StaticCreateBlockNode(&m_nodeAllocator, ichMin, ichLim, this->m_nextBlockId++, blockType);
    AddAstSizeAllowDefer(sizeof(ParseNodeBlock));
    return pnode;
}

// Create ParseNodeVar
ParseNodeVar * Parser::CreateDeclNode(OpCode nop, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl)
{
    Assert(nop == knopVarDecl || nop == knopLetDecl || nop == knopConstDecl);
    ParseNodeVar * pnode = Anew(&m_nodeAllocator, ParseNodeVar, nop, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok(), pid);
    if (symbolType != STUnknown)
    {
        pnode->sym = AddDeclForPid(pnode, pid, symbolType, errorOnRedecl);
    }

    return pnode;
}

ParseNodeInt * Parser::CreateIntNode(int32 lw)
{
    Assert(!this->m_deferringAST);
    ParseNodeInt * pnode = Anew(&m_nodeAllocator, ParseNodeInt, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok(), lw);
    AddAstSize(sizeof(ParseNodeInt));
    return pnode;
}

ParseNodeStr * Parser::CreateStrNode(IdentPtr pid)
{
    Assert(!this->m_deferringAST);
    ParseNodeStr * pnode = Anew(&m_nodeAllocator, ParseNodeStr, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok(), pid);
    pnode->grfpn |= PNodeFlags::fpnCanFlattenConcatExpr;
    AddAstSize(sizeof(ParseNodeStr));
    return pnode;
}

ParseNodeName * Parser::CreateNameNode(IdentPtr pid)
{
    ParseNodeName * pnode = Anew(&m_nodeAllocator, ParseNodeName, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok(), pid);
    AddAstSizeAllowDefer(sizeof(ParseNodeName));
    return pnode;
}

ParseNodeName * Parser::CreateNameNode(IdentPtr pid, PidRefStack * ref, charcount_t ichMin, charcount_t ichLim)
{
    ParseNodeName * pnode = Anew(&m_nodeAllocator, ParseNodeName, ichMin, ichLim, pid);
    pnode->SetSymRef(ref);
    AddAstSize(sizeof(ParseNodeName));
    return pnode;
}

ParseNodeSpecialName * Parser::CreateSpecialNameNode(IdentPtr pid, PidRefStack * ref, charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);
    ParseNodeSpecialName * pnode = Anew(&m_nodeAllocator, ParseNodeSpecialName, ichMin, ichLim, pid);
    pnode->SetSymRef(ref);
    if (pid == wellKnownPropertyPids._this)
    {
        pnode->isThis = true;
    }
    else if (pid == wellKnownPropertyPids._super || pid == wellKnownPropertyPids._superConstructor)
    {
        pnode->isSuper = true;
    }

    AddAstSize(sizeof(ParseNodeSpecialName));
    return pnode;
}

ParseNodeSuperReference * Parser::CreateSuperReferenceNode(OpCode nop, ParseNodeSpecialName * pnode1, ParseNodePtr pnode2)
{
    Assert(!this->m_deferringAST);
    Assert(pnode1 && pnode1->isSuper);
    Assert(pnode2 != nullptr);
    Assert(nop == knopDot || nop == knopIndex);

    ParseNodeSuperReference * pnode = Anew(&m_nodeAllocator, ParseNodeSuperReference, nop, pnode1->ichMin, pnode2->ichLim, pnode1, pnode2);
    AddAstSize(sizeof(ParseNodeSuperReference));

    return pnode;
}

ParseNodeProg * Parser::CreateProgNode(bool isModuleSource, ULONG lineNumber)
{
    ParseNodeProg * pnodeProg;

    if (isModuleSource)
    {
        pnodeProg = CreateNodeForOpT<knopModule>();

        // knopModule is not actually handled anywhere since we would need to handle it everywhere we could
        // have knopProg and it would be treated exactly the same except for import/export statements.
        // We are only using it as a way to get the correct size for PnModule.
        // Consider: Should we add a flag to PnProg which is false but set to true in PnModule?
        //           If we do, it can't be a virtual method since the parse nodes are all in a union.
        pnodeProg->nop = knopProg;
    }
    else
    {
        pnodeProg = CreateNodeForOpT<knopProg>();
    }

    pnodeProg->cbMin = this->GetScanner()->IecpMinTok();
    pnodeProg->lineNumber = lineNumber;
    pnodeProg->homeObjLocation = Js::Constants::NoRegister;
    pnodeProg->superRestrictionState = SuperRestrictionState::Disallowed;
    return pnodeProg;
}

ParseNodeCall * Parser::CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2)
{
    charcount_t ichMin;
    charcount_t ichLim;

    if (nullptr == pnode1)
    {
        Assert(nullptr == pnode2);
        ichMin = this->GetScanner()->IchMinTok();
        ichLim = this->GetScanner()->IchLimTok();
    }
    else
    {
        ichMin = pnode1->ichMin;
        ichLim = pnode2 == nullptr ? pnode1->ichLim : pnode2->ichLim;

        if (pnode1->nop == knopDot || pnode1->nop == knopIndex)
        {
            this->CheckArguments(pnode1->AsParseNodeBin()->pnode1);
        }
    }
    return CreateCallNode(nop, pnode1, pnode2, ichMin, ichLim);
}



ParseNodeCall * Parser::CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, charcount_t ichMin, charcount_t ichLim)
{
    Assert(!this->m_deferringAST);

    // Classes, derived from ParseNodeCall, can be created here as well,
    // as long as their size matches kcbPnCall (that is, they don't add
    // any data members of their own).
    DebugOnly(VerifyNodeSize(nop, sizeof(ParseNodeCall)));
    ParseNodeCall* pnode = Anew(&m_nodeAllocator, ParseNodeCall, nop, ichMin, ichLim, pnode1, pnode2);
    AddAstSize(sizeof(ParseNodeCall));

    return pnode;
}

ParseNodeSuperCall * Parser::CreateSuperCallNode(ParseNodeSpecialName * pnode1, ParseNodePtr pnode2)
{
    Assert(!this->m_deferringAST);
    Assert(pnode1 && pnode1->isSuper);

    ParseNodeSuperCall* pnode = Anew(&m_nodeAllocator, ParseNodeSuperCall, knopCall, pnode1->ichMin, pnode2 == nullptr ? pnode1->ichLim : pnode2->ichLim, pnode1, pnode2);
    AddAstSize(sizeof(ParseNodeSuperCall));
    return pnode;
}

ParseNodeParamPattern * Parser::CreateParamPatternNode(ParseNode * pnode1)
{
    ParseNodeParamPattern * paramPatternNode = CreateNodeForOpT<knopParamPattern>(pnode1->ichMin, pnode1->ichLim);
    paramPatternNode->pnode1 = pnode1;
    paramPatternNode->pnodeNext = nullptr;
    paramPatternNode->location = Js::Constants::NoRegister;
    return paramPatternNode;
}

ParseNodeParamPattern * Parser::CreateDummyParamPatternNode(charcount_t ichMin)
{
    ParseNodeParamPattern * paramPatternNode = CreateNodeForOpT<knopParamPattern>(ichMin);
    paramPatternNode->pnode1 = nullptr;
    paramPatternNode->pnodeNext = nullptr;
    paramPatternNode->location = Js::Constants::NoRegister;
    return paramPatternNode;
}

Symbol* Parser::AddDeclForPid(ParseNodeVar * pnodeVar, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl)
{
    Assert(pnodeVar->IsVarLetOrConst());

    PidRefStack *refForUse = nullptr, *refForDecl = nullptr;
    BlockInfoStack *blockInfo;
    bool fBlockScope = false;
    if (pnodeVar->nop != knopVarDecl || symbolType == STFunction)
    {
        Assert(m_pstmtCur);
        if (m_pstmtCur->GetNop() != knopBlock)
        {
            // Let/const declared in a bare statement context.
            Error(ERRDeclOutOfStmt);
        }

        if (m_pstmtCur->pstmtOuter && m_pstmtCur->pstmtOuter->GetNop() == knopSwitch)
        {
            // Let/const declared inside a switch block (requiring conservative use-before-decl check).
            pnodeVar->isSwitchStmtDecl = true;
        }

        fBlockScope = pnodeVar->nop != knopVarDecl ||
            (
                !GetCurrentBlockInfo()->pnodeBlock->scope ||
                GetCurrentBlockInfo()->pnodeBlock->scope->GetScopeType() != ScopeType_GlobalEvalBlock
                );
    }
    if (fBlockScope)
    {
        blockInfo = GetCurrentBlockInfo();
    }
    else
    {
        blockInfo = GetCurrentFunctionBlockInfo();
    }

    refForDecl = this->FindOrAddPidRef(pid, blockInfo->pnodeBlock->blockId, GetCurrentFunctionNode()->functionId);

    if (refForDecl == nullptr)
    {
        Error(ERRnoMemory);
    }

    if (refForDecl->funcId != GetCurrentFunctionNode()->functionId)
    {
        // Fix up the function id, which is incorrect if we're reparsing lambda parameters
        Assert(this->m_reparsingLambdaParams);
        refForDecl->funcId = GetCurrentFunctionNode()->functionId;
    }

    if (blockInfo == GetCurrentBlockInfo())
    {
        refForUse = refForDecl;
    }
    else
    {
        refForUse = this->PushPidRef(pid);
    }
    pnodeVar->symRef = refForUse->GetSymRef();
    Symbol *sym = refForDecl->GetSym();
    if (sym != nullptr)
    {
        // Multiple declarations in the same scope. 3 possibilities: error, existing one wins, new one wins.
        switch (pnodeVar->nop)
        {
        case knopLetDecl:
        case knopConstDecl:
            if (!sym->GetDecl()->AsParseNodeVar()->isBlockScopeFncDeclVar && !sym->IsArguments())
            {
                // If the built-in arguments is shadowed then don't throw
                Assert(errorOnRedecl);
                // Redeclaration error.
                Error(ERRRedeclaration);
            }
            else
            {
                // (New) let/const hides the (old) var
                sym->SetSymbolType(symbolType);
                sym->SetDecl(pnodeVar);
            }
            break;
        case knopVarDecl:
            if (m_currentScope->GetScopeType() == ScopeType_Parameter && !sym->IsArguments())
            {
                // If this is a parameter list, mark the scope to indicate that it has duplicate definition unless it is shadowing the default arguments symbol.
                // If later this turns out to be a non-simple param list (like function f(a, a, c = 1) {}) then it is a SyntaxError to have duplicate formals.
                m_currentScope->SetHasDuplicateFormals();
            }

            if (sym->GetDecl() == nullptr)
            {
                sym->SetDecl(pnodeVar);
                break;
            }
            switch (sym->GetDecl()->nop)
            {
            case knopLetDecl:
            case knopConstDecl:
                // Destructuring made possible to have the formals to be the let bind. But that shouldn't throw the error.
                if (errorOnRedecl && (!IsES6DestructuringEnabled() || sym->GetSymbolType() != STFormal))
                {
                    Error(ERRRedeclaration);
                }
                // If !errorOnRedecl, (old) let/const hides the (new) var, so do nothing.
                break;
            case knopVarDecl:
                // Legal redeclaration. Who wins?
                if (errorOnRedecl || sym->GetDecl()->AsParseNodeVar()->isBlockScopeFncDeclVar || sym->IsArguments())
                {
                    if (symbolType == STFormal ||
                        (symbolType == STFunction && sym->GetSymbolType() != STFormal) ||
                        sym->GetSymbolType() == STVariable)
                    {
                        // New decl wins.
                        sym->SetSymbolType(symbolType);
                        sym->SetDecl(pnodeVar);
                    }
                }
                break;
            }
            break;
        }
    }
    else
    {
        Scope *scope = blockInfo->pnodeBlock->scope;
        if (scope == nullptr)
        {
            Assert(blockInfo->pnodeBlock->blockType == PnodeBlockType::Regular);
            scope = Anew(&m_nodeAllocator, Scope, &m_nodeAllocator, ScopeType_Block);
            if (this->IsCurBlockInLoop())
            {
                scope->SetIsBlockInLoop();
            }
            blockInfo->pnodeBlock->scope = scope;
            PushScope(scope);
        }

        ParseNodeFnc * pnodeFnc = GetCurrentFunctionNode();
        if (scope->GetScopeType() == ScopeType_GlobalEvalBlock)
        {
            Assert(fBlockScope);
            Assert(scope->GetEnclosingScope() == m_currentNodeProg->scope);
            // Check for same-named decl in Global scope.
            CheckRedeclarationErrorForBlockId(pid, 0);
        }
        else if (scope->GetScopeType() == ScopeType_Global && (this->m_grfscr & fscrEvalCode) &&
            !(m_functionBody && m_functionBody->GetScopeInfo()))
        {
            // Check for same-named decl in GlobalEvalBlock scope. Note that this is not necessary
            // if we're compiling a deferred nested function and the global scope was restored from cached info,
            // because in that case we don't need a GlobalEvalScope.
            Assert(!fBlockScope || (this->m_grfscr & fscrConsoleScopeEval) == fscrConsoleScopeEval);
            CheckRedeclarationErrorForBlockId(pid, 1);
        }
        else if (!pnodeFnc->IsBodyAndParamScopeMerged()
            && scope->GetScopeType() == ScopeType_FunctionBody
            && (pnodeVar->nop == knopLetDecl || pnodeVar->nop == knopConstDecl))
        {
            // In case of split scope function when we add a new let or const declaration to the body
            // we have to check whether the param scope already has the same symbol defined.
            CheckRedeclarationErrorForBlockId(pid, pnodeFnc->pnodeScopes->blockId);
        }

        if (!sym)
        {
            const char16 *name = reinterpret_cast<const char16*>(pid->Psz());
            int nameLength = pid->Cch();
            SymbolName const symName(name, nameLength);

            Assert(!scope->FindLocalSymbol(symName));
            sym = Anew(&m_nodeAllocator, Symbol, symName, pnodeVar, symbolType);
            scope->AddNewSymbol(sym);
            sym->SetPid(pid);
        }
        refForDecl->SetSym(sym);
    }
    return sym;
}

void Parser::CheckRedeclarationErrorForBlockId(IdentPtr pid, int blockId)
{
    // If the ref stack entry for the blockId contains a sym then throw redeclaration error
    PidRefStack *pidRefOld = pid->GetPidRefForScopeId(blockId);
    if (pidRefOld && pidRefOld->GetSym() && !pidRefOld->GetSym()->IsArguments())
    {
        Error(ERRRedeclaration);
    }
}

bool Parser::IsCurBlockInLoop() const
{
    for (StmtNest *stmt = this->m_pstmtCur; stmt != nullptr; stmt = stmt->pstmtOuter)
    {
        OpCode nop = stmt->GetNop();
        if (ParseNode::Grfnop(nop) & fnopContinue)
        {
            return true;
        }
        if (nop == knopFncDecl)
        {
            return false;
        }
    }
    return false;
}

void Parser::RestorePidRefForSym(Symbol *sym)
{
    IdentPtr pid = this->GetHashTbl()->PidHashNameLen(sym->GetName().GetBuffer(), sym->GetName().GetLength());
    Assert(pid);
    sym->SetPid(pid);
    PidRefStack *ref = this->PushPidRef(pid);
    ref->SetSym(sym);
}

void Parser::CheckPidIsValid(IdentPtr pid, bool autoArgumentsObject)
{
    if (IsStrictMode())
    {
        // in strict mode, variable named 'eval' cannot be created
        if (pid == wellKnownPropertyPids.eval)
        {
            Error(ERREvalUsage);
        }
        else if (pid == wellKnownPropertyPids.arguments && !autoArgumentsObject)
        {
            Error(ERRArgsUsage);
        }
    }
}

// CreateVarDecl needs m_ppnodeVar to be pointing to the right function.
// Post-parsing rewriting during bytecode gen may have m_ppnodeVar pointing to the last parsed function.
// This function sets up m_ppnodeVar to point to the given pnodeFnc and creates the new var declaration.
// This prevents accidentally adding var declarations to the last parsed function.
ParseNodeVar * Parser::AddVarDeclNode(IdentPtr pid, ParseNodeFnc * pnodeFnc)
{
    AnalysisAssert(pnodeFnc);

    ParseNodePtr *const ppnodeVarSave = m_ppnodeVar;

    m_ppnodeVar = &pnodeFnc->pnodeVars;
    while (*m_ppnodeVar != nullptr)
    {
        m_ppnodeVar = &(*m_ppnodeVar)->AsParseNodeVar()->pnodeNext;
    }

    ParseNodeVar * pnode = CreateVarDeclNode(pid, STUnknown, false, 0, /* checkReDecl = */ false);

    m_ppnodeVar = ppnodeVarSave;

    return pnode;
}

ParseNodeVar * Parser::CreateModuleImportDeclNode(IdentPtr localName)
{
    ParseNodeVar * declNode = CreateBlockScopedDeclNode(localName, knopConstDecl);
    Symbol* sym = declNode->sym;

    sym->SetIsModuleExportStorage(true);
    sym->SetIsModuleImport(true);

    return declNode;
}

ParseNodeVar * Parser::CreateVarDeclNode(IdentPtr pid, SymbolType symbolType, bool autoArgumentsObject, ParseNodePtr pnodeFnc, bool errorOnRedecl)
{
    ParseNodeVar * pnode = CreateDeclNode(knopVarDecl, pid, symbolType, errorOnRedecl);

    // Append the variable to the end of the current variable list.
    Assert(m_ppnodeVar);
    pnode->pnodeNext = *m_ppnodeVar;
    *m_ppnodeVar = pnode;
    if (nullptr != pid)
    {
        // this is not a temp - make sure temps go after this node
        Assert(pid);
        m_ppnodeVar = &pnode->pnodeNext;
        CheckPidIsValid(pid, autoArgumentsObject);
    }

    return pnode;
}

ParseNodeVar * Parser::CreateBlockScopedDeclNode(IdentPtr pid, OpCode nodeType)
{
    Assert(nodeType == knopConstDecl || nodeType == knopLetDecl);

    ParseNodeVar * pnode = CreateDeclNode(nodeType, pid, STVariable, true);

    if (nullptr != pid)
    {
        Assert(pid);
        AddVarDeclToBlock(pnode);
        CheckPidIsValid(pid);
    }

    return pnode;
}

void Parser::AddVarDeclToBlock(ParseNodeVar *pnode)
{
    Assert(pnode->nop == knopConstDecl || pnode->nop == knopLetDecl);

    // Maintain a combined list of let and const declarations to keep
    // track of declaration order.

    Assert(m_currentBlockInfo->m_ppnodeLex);
    *m_currentBlockInfo->m_ppnodeLex = pnode;
    m_currentBlockInfo->m_ppnodeLex = &pnode->pnodeNext;
    pnode->pnodeNext = nullptr;
}

void Parser::SetCurrentStatement(StmtNest *stmt)
{
    m_pstmtCur = stmt;
}

template<bool buildAST>
ParseNodeBlock * Parser::StartParseBlockWithCapacity(PnodeBlockType blockType, ScopeType scopeType, int capacity)
{
    Scope *scope = nullptr;

    // Block scopes are not created lazily in the case where we're repopulating a persisted scope.

    scope = Anew(&m_nodeAllocator, Scope, &m_nodeAllocator, scopeType, capacity);
    PushScope(scope);

    return StartParseBlockHelper<buildAST>(blockType, scope, nullptr);
}

template<bool buildAST>
ParseNodeBlock * Parser::StartParseBlock(PnodeBlockType blockType, ScopeType scopeType, LabelId* pLabelId)
{
    Scope *scope = nullptr;
    // Block scopes are created lazily when we discover block-scoped content.
    if (scopeType != ScopeType_Unknown && scopeType != ScopeType_Block)
    {
        scope = Anew(&m_nodeAllocator, Scope, &m_nodeAllocator, scopeType);
        PushScope(scope);
    }

    return StartParseBlockHelper<buildAST>(blockType, scope, pLabelId);
}

template<bool buildAST>
ParseNodeBlock * Parser::StartParseBlockHelper(PnodeBlockType blockType, Scope *scope, LabelId* pLabelId)
{
    ParseNodeBlock * pnodeBlock = CreateBlockNode(blockType);
    pnodeBlock->scope = scope;
    BlockInfoStack *newBlockInfo = PushBlockInfo(pnodeBlock);

    PushStmt<buildAST>(&newBlockInfo->pstmt, pnodeBlock, knopBlock, pLabelId);

    return pnodeBlock;
}

void Parser::PushScope(Scope *scope)
{
    Assert(scope);
    scope->SetEnclosingScope(m_currentScope);
    m_currentScope = scope;
}

void Parser::PopScope(Scope *scope)
{
    Assert(scope == m_currentScope);
    m_currentScope = scope->GetEnclosingScope();
    scope->SetEnclosingScope(nullptr);
}

void Parser::PushFuncBlockScope(ParseNodeBlock * pnodeBlock, ParseNodePtr **ppnodeScopeSave, ParseNodePtr **ppnodeExprScopeSave)
{
    // Maintain the scope tree.

    pnodeBlock->pnodeScopes = nullptr;
    pnodeBlock->pnodeNext = nullptr;

    // Insert this block into the active list of scopes (m_ppnodeExprScope or m_ppnodeScope).
    // Save the current block's "next" pointer as the new endpoint of that list.
    if (m_ppnodeExprScope)
    {
        *ppnodeScopeSave = m_ppnodeScope;

        Assert(*m_ppnodeExprScope == nullptr);
        *m_ppnodeExprScope = pnodeBlock;
        *ppnodeExprScopeSave = &pnodeBlock->pnodeNext;
    }
    else
    {
        Assert(m_ppnodeScope);
        Assert(*m_ppnodeScope == nullptr);
        *m_ppnodeScope = pnodeBlock;
        *ppnodeScopeSave = &pnodeBlock->pnodeNext;

        *ppnodeExprScopeSave = m_ppnodeExprScope;
    }

    // Advance the global scope list pointer to the new block's child list.
    m_ppnodeScope = &pnodeBlock->pnodeScopes;
    // Set m_ppnodeExprScope to NULL to make that list inactive.
    m_ppnodeExprScope = nullptr;
}

void Parser::PopFuncBlockScope(ParseNodePtr *ppnodeScopeSave, ParseNodePtr *ppnodeExprScopeSave)
{
    Assert(m_ppnodeExprScope == nullptr || *m_ppnodeExprScope == nullptr);
    m_ppnodeExprScope = ppnodeExprScopeSave;

    Assert(m_ppnodeScope);
    Assert(nullptr == *m_ppnodeScope);
    m_ppnodeScope = ppnodeScopeSave;
}

template<bool buildAST>
ParseNodeBlock * Parser::ParseBlock(LabelId* pLabelId)
{
    ParseNodeBlock * pnodeBlock = nullptr;
    ParseNodePtr *ppnodeScopeSave = nullptr;
    ParseNodePtr *ppnodeExprScopeSave = nullptr;

    pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Regular, ScopeType_Block, pLabelId);

    BlockInfoStack* outerBlockInfo = m_currentBlockInfo->pBlockInfoOuter;
    if (outerBlockInfo != nullptr && outerBlockInfo->pnodeBlock != nullptr
        && outerBlockInfo->pnodeBlock->scope != nullptr
        && outerBlockInfo->pnodeBlock->scope->GetScopeType() == ScopeType_CatchParamPattern)
    {
        // If we are parsing the catch block then destructured params can have let declarations. Let's add them to the new block.
        for (ParseNodePtr pnode = m_currentBlockInfo->pBlockInfoOuter->pnodeBlock->pnodeLexVars; pnode; pnode = pnode->AsParseNodeVar()->pnodeNext)
        {
            PidRefStack* ref = PushPidRef(pnode->AsParseNodeVar()->sym->GetPid());
            ref->SetSym(pnode->AsParseNodeVar()->sym);
        }
    }

    ChkCurTok(tkLCurly, ERRnoLcurly);
    ParseNodePtr * ppnodeList = nullptr;
    if (buildAST)
    {
        PushFuncBlockScope(pnodeBlock, &ppnodeScopeSave, &ppnodeExprScopeSave);
        ppnodeList = &pnodeBlock->pnodeStmt;
    }

    ParseStmtList<buildAST>(ppnodeList);

    if (buildAST)
    {
        PopFuncBlockScope(ppnodeScopeSave, ppnodeExprScopeSave);
    }

    FinishParseBlock(pnodeBlock);

    ChkCurTok(tkRCurly, ERRnoRcurly);

    return pnodeBlock;
}

bool Parser::IsSpecialName(IdentPtr pid)
{
    return pid == wellKnownPropertyPids._this ||
        pid == wellKnownPropertyPids._super ||
        pid == wellKnownPropertyPids._superConstructor ||
        pid == wellKnownPropertyPids._newTarget;
}

ParseNodeSpecialName * Parser::ReferenceSpecialName(IdentPtr pid, charcount_t ichMin, charcount_t ichLim, bool createNode)
{
    PidRefStack* ref = this->PushPidRef(pid);

    if (!createNode)
    {
        return nullptr;
    }

    return CreateSpecialNameNode(pid, ref, ichMin, ichLim);
}

ParseNodeVar * Parser::CreateSpecialVarDeclIfNeeded(ParseNodeFnc * pnodeFnc, IdentPtr pid, bool forceCreate)
{
    Assert(pid != nullptr);

    PidRefStack* ref = pid->GetTopRef();

    // If the function has a reference to pid or we set forceCreate, make a special var decl
    if (forceCreate || (ref && ref->GetScopeId() >= m_currentBlockInfo->pnodeBlock->blockId))
    {
        return this->CreateSpecialVarDeclNode(pnodeFnc, pid);
    }

    return nullptr;
}

void Parser::CreateSpecialSymbolDeclarations(ParseNodeFnc * pnodeFnc)
{
    // Lambda function cannot have any special bindings.
    if (pnodeFnc->IsLambda())
    {
        return;
    }

    bool isTopLevelEventHandler = (this->m_grfscr & fscrImplicitThis) && !pnodeFnc->IsNested();

    // Create a 'this' symbol for non-lambda functions with references to 'this', and all class constructors and top level event hanlders.
    ParseNodePtr varDeclNode = CreateSpecialVarDeclIfNeeded(pnodeFnc, wellKnownPropertyPids._this, pnodeFnc->IsClassConstructor() || isTopLevelEventHandler);
    if (varDeclNode)
    {
        varDeclNode->AsParseNodeVar()->sym->SetIsThis(true);

        if (pnodeFnc->IsDerivedClassConstructor())
        {
            varDeclNode->AsParseNodeVar()->sym->SetNeedDeclaration(true);
        }
    }

    // Create a 'new.target' symbol for any ordinary function with a reference and all class constructors.
    varDeclNode = CreateSpecialVarDeclIfNeeded(pnodeFnc, wellKnownPropertyPids._newTarget, pnodeFnc->IsClassConstructor());
    if (varDeclNode)
    {
        varDeclNode->AsParseNodeVar()->sym->SetIsNewTarget(true);
    }

    // Create a 'super' (as a reference) symbol.
    varDeclNode = CreateSpecialVarDeclIfNeeded(pnodeFnc, wellKnownPropertyPids._super);
    if (varDeclNode)
    {
        varDeclNode->AsParseNodeVar()->sym->SetIsSuper(true);
    }

    // Create a 'super' (as the call target for super()) symbol only for derived class constructors.
        varDeclNode = CreateSpecialVarDeclIfNeeded(pnodeFnc, wellKnownPropertyPids._superConstructor);
        if (varDeclNode)
        {
            varDeclNode->AsParseNodeVar()->sym->SetIsSuperConstructor(true);
        }
    }

void Parser::FinishParseBlock(ParseNodeBlock *pnodeBlock, bool needScanRCurly)
{
    Assert(m_currentBlockInfo != nullptr && pnodeBlock == m_currentBlockInfo->pnodeBlock);

    if (needScanRCurly)
    {
        // Only update the ichLim if we were expecting an RCurly. If there is an
        // expression body without a necessary RCurly, the correct ichLim will
        // have been set already.
        pnodeBlock->ichLim = this->GetScanner()->IchLimTok();
    }

    BindPidRefs<false>(GetCurrentBlockInfo(), m_nextBlockId - 1);

    PopStmt(&m_currentBlockInfo->pstmt);

    PopBlockInfo();

    Scope *scope = pnodeBlock->scope;
    if (scope)
    {
        PopScope(scope);
    }
}

void Parser::FinishParseFncExprScope(ParseNodeFnc * pnodeFnc, ParseNodeBlock * pnodeFncExprScope)
{
    int fncExprScopeId = pnodeFncExprScope->blockId;
    ParseNodePtr pnodeName = pnodeFnc->pnodeName;
    if (pnodeName)
    {
        Assert(pnodeName->nop == knopVarDecl);
        BindPidRefsInScope(pnodeName->AsParseNodeVar()->pid, pnodeName->AsParseNodeVar()->sym, fncExprScopeId, m_nextBlockId - 1);
    }
    FinishParseBlock(pnodeFncExprScope);
}

template <const bool backgroundPidRef>
void Parser::BindPidRefs(BlockInfoStack *blockInfo, uint maxBlockId)
{
    // We need to bind all assignments in order to emit assignment to 'const' error
    int blockId = blockInfo->pnodeBlock->blockId;

    Scope *scope = blockInfo->pnodeBlock->scope;
    if (scope)
    {
        auto bindPidRefs = [blockId, maxBlockId, this](Symbol *sym)
        {
            ParseNodePtr pnode = sym->GetDecl();
            IdentPtr pid;
#if PROFILE_DICTIONARY
            int depth = 0;
#endif
            Assert(pnode);
            switch (pnode->nop)
            {
            case knopVarDecl:
            case knopLetDecl:
            case knopConstDecl:
                pid = pnode->AsParseNodeVar()->pid;
                if (backgroundPidRef)
                {
                    pid = this->GetHashTbl()->FindExistingPid(pid->Psz(), pid->Psz() + pid->Cch(), pid->Cch(), pid->Hash(), nullptr, nullptr
#if PROFILE_DICTIONARY
                        , depth
#endif
                    );
                    if (pid == nullptr)
                    {
                        break;
                    }
                }
                this->BindPidRefsInScope(pid, sym, blockId, maxBlockId);
                break;
            case knopName:
                pid = pnode->AsParseNodeName()->pid;
                if (backgroundPidRef)
                {
                    pid = this->GetHashTbl()->FindExistingPid(pid->Psz(), pid->Psz() + pid->Cch(), pid->Cch(), pid->Hash(), nullptr, nullptr
#if PROFILE_DICTIONARY
                        , depth
#endif
                    );
                    if (pid == nullptr)
                    {
                        break;
                    }
                }
                this->BindPidRefsInScope(pid, sym, blockId, maxBlockId);
                break;
            default:
                Assert(0);
                break;
            }
        };

        scope->ForEachSymbol(bindPidRefs);
    }
}

void Parser::BindPidRefsInScope(IdentPtr pid, Symbol *sym, int blockId, uint maxBlockId)
{
    PidRefStack *ref, *nextRef, *lastRef = nullptr;
    Js::LocalFunctionId funcId = GetCurrentFunctionNode()->functionId;
    Assert(sym);

    if (pid->GetIsModuleExport() && IsTopLevelModuleFunc())
    {
        sym->SetIsModuleExportStorage(true);
    }

    bool hasFuncAssignment = sym->GetHasFuncAssignment();
    bool doesEscape = false;

    for (ref = pid->GetTopRef(); ref && ref->GetScopeId() >= blockId; ref = nextRef)
    {
        // Fix up sym* on PID ref.
        Assert(!ref->GetSym() || ref->GetSym() == sym);
        nextRef = ref->prev;
        Assert(ref->GetScopeId() >= 0);
        if ((uint)ref->GetScopeId() > maxBlockId)
        {
            lastRef = ref;
            continue;
        }
        ref->SetSym(sym);
        this->RemovePrevPidRef(pid, lastRef);

        if (ref->IsUsedInLdElem())
        {
            sym->SetIsUsedInLdElem(true);
        }

        if (ref->IsAssignment())
        {
            sym->PromoteAssignmentState();
            if (sym->GetIsFormal())
            {
                GetCurrentFunctionNode()->SetHasAnyWriteToFormals(true);
            }
        }

        if (ref->GetFuncScopeId() != funcId && !sym->GetIsGlobal() && !sym->GetIsModuleExportStorage())
        {
            Assert(ref->GetFuncScopeId() > funcId);
            sym->SetHasNonLocalReference();
            if (ref->IsDynamicBinding())
            {
                sym->SetNeedsScopeObject();
            }
        }

        if (ref->IsFuncAssignment())
        {
            hasFuncAssignment = true;
        }

        if (ref->IsEscape())
        {
            doesEscape = true;
        }

        if (m_currentNodeFunc && doesEscape && hasFuncAssignment)
        {
            if (m_sourceContextInfo ?
                !PHASE_OFF_RAW(Js::DisableStackFuncOnDeferredEscapePhase, m_sourceContextInfo->sourceContextId, m_currentNodeFunc->functionId) :
                !PHASE_OFF1(Js::DisableStackFuncOnDeferredEscapePhase))
            {
                m_currentNodeFunc->SetNestedFuncEscapes();
            }
        }

        if (ref->GetScopeId() == blockId)
        {
            break;
        }
    }
}

void Parser::MarkEscapingRef(ParseNodePtr pnode, IdentToken *pToken)
{
    if (m_currentNodeFunc == nullptr)
    {
        return;
    }
    if (pnode && pnode->nop == knopFncDecl)
    {
        this->SetNestedFuncEscapes();
    }
    else if (pToken->pid)
    {
        PidRefStack *pidRef = pToken->pid->GetTopRef();
        if (pidRef->sym)
        {
            if (pidRef->sym->GetSymbolType() == STFunction)
            {
                this->SetNestedFuncEscapes();
            }
        }
        else
        {
            pidRef->isEscape = true;
        }
    }
}

void Parser::SetNestedFuncEscapes() const
{
    if (m_sourceContextInfo ?
        !PHASE_OFF_RAW(Js::DisableStackFuncOnDeferredEscapePhase, m_sourceContextInfo->sourceContextId, m_currentNodeFunc->functionId) :
        !PHASE_OFF1(Js::DisableStackFuncOnDeferredEscapePhase))
    {
        m_currentNodeFunc->SetNestedFuncEscapes();
    }
}

void Parser::PopStmt(StmtNest *pStmt)
{
    Assert(pStmt == m_pstmtCur);
    SetCurrentStatement(m_pstmtCur->pstmtOuter);
}

BlockInfoStack *Parser::PushBlockInfo(ParseNodeBlock * pnodeBlock)
{
    BlockInfoStack *newBlockInfo = (BlockInfoStack *)m_nodeAllocator.Alloc(sizeof(BlockInfoStack));
    Assert(nullptr != newBlockInfo);

    newBlockInfo->pnodeBlock = pnodeBlock;
    newBlockInfo->pBlockInfoOuter = m_currentBlockInfo;
    newBlockInfo->m_ppnodeLex = &pnodeBlock->pnodeLexVars;

    if (pnodeBlock->blockType != PnodeBlockType::Regular)
    {
        newBlockInfo->pBlockInfoFunction = newBlockInfo;
    }
    else
    {
        Assert(m_currentBlockInfo);
        newBlockInfo->pBlockInfoFunction = m_currentBlockInfo->pBlockInfoFunction;
    }

    m_currentBlockInfo = newBlockInfo;
    return newBlockInfo;
}

void Parser::PopBlockInfo()
{
    Assert(m_currentBlockInfo);
    PopDynamicBlock();
    m_currentBlockInfo = m_currentBlockInfo->pBlockInfoOuter;
}

void Parser::PushDynamicBlock()
{
    Assert(GetCurrentBlock());
    int blockId = GetCurrentBlock()->blockId;
    if (m_currentDynamicBlock && m_currentDynamicBlock->id == blockId)
    {
        return;
    }
    BlockIdsStack *info = (BlockIdsStack *)m_nodeAllocator.Alloc(sizeof(BlockIdsStack));
    if (nullptr == info)
    {
        Error(ERRnoMemory);
    }

    info->id = blockId;
    info->prev = m_currentDynamicBlock;
    m_currentDynamicBlock = info;
}

void Parser::PopDynamicBlock()
{
    int blockId = GetCurrentDynamicBlockId();
    if (GetCurrentBlock()->blockId != blockId || blockId == -1)
    {
        return;
    }
    Assert(m_currentDynamicBlock);

    this->GetHashTbl()->VisitPids([&](IdentPtr pid) {
        for (PidRefStack *ref = pid->GetTopRef(); ref && ref->GetScopeId() >= blockId; ref = ref->prev)
        {
            ref->SetDynamicBinding();
        }
    });

    m_currentDynamicBlock = m_currentDynamicBlock->prev;
}

int Parser::GetCurrentDynamicBlockId() const
{
    return m_currentDynamicBlock ? m_currentDynamicBlock->id : -1;
}

ParseNodeFnc *Parser::GetCurrentFunctionNode()
{
    if (m_currentNodeDeferredFunc != nullptr)
    {
        return m_currentNodeDeferredFunc;
    }
    else if (m_currentNodeFunc != nullptr)
    {
        return m_currentNodeFunc;
    }
    else
    {
        AssertMsg(GetFunctionBlock()->blockType == PnodeBlockType::Global,
            "Most likely we are trying to find a syntax error, related to 'let' or 'const' in deferred parsing mode with disabled support of 'let' and 'const'");
        return m_currentNodeProg;
    }
}

ParseNodeFnc *Parser::GetCurrentNonLambdaFunctionNode()
{
    if (m_currentNodeNonLambdaDeferredFunc != nullptr)
    {
        return m_currentNodeNonLambdaDeferredFunc;
    }
    return m_currentNodeNonLambdaFunc;

}
void Parser::RegisterRegexPattern(UnifiedRegex::RegexPattern *const regexPattern)
{
    Assert(regexPattern);

    // ensure a no-throw add behavior here, to catch out of memory exceptions, using the guest arena allocator
    if (!m_registeredRegexPatterns.PrependNoThrow(m_tempGuestArena->GetAllocator(), regexPattern))
    {
        Parser::Error(ERRnoMemory);
    }
}

void Parser::CaptureState(ParserState *state)
{
    Assert(state != nullptr);

    state->m_funcInArraySave = m_funcInArray;
    state->m_funcInArrayDepthSave = m_funcInArrayDepth;
    state->m_nestedCountSave = *m_pnestedCount;
    state->m_ppnodeScopeSave = m_ppnodeScope;
    state->m_ppnodeExprScopeSave = m_ppnodeExprScope;
    state->m_pCurrentAstSizeSave = m_pCurrentAstSize;
    state->m_nextBlockId = m_nextBlockId;

    Assert(state->m_ppnodeScopeSave == nullptr || *state->m_ppnodeScopeSave == nullptr);
    Assert(state->m_ppnodeExprScopeSave == nullptr || *state->m_ppnodeExprScopeSave == nullptr);

#if DEBUG
    state->m_currentBlockInfo = m_currentBlockInfo;
#endif
}

void Parser::RestoreStateFrom(ParserState *state)
{
    Assert(state != nullptr);
    Assert(state->m_currentBlockInfo == m_currentBlockInfo);

    m_funcInArray = state->m_funcInArraySave;
    m_funcInArrayDepth = state->m_funcInArrayDepthSave;
    *m_pnestedCount = state->m_nestedCountSave;
    m_pCurrentAstSize = state->m_pCurrentAstSizeSave;
    m_nextBlockId = state->m_nextBlockId;

    if (state->m_ppnodeScopeSave != nullptr)
    {
        *state->m_ppnodeScopeSave = nullptr;
    }

    if (state->m_ppnodeExprScopeSave != nullptr)
    {
        *state->m_ppnodeExprScopeSave = nullptr;
    }

    m_ppnodeScope = state->m_ppnodeScopeSave;
    m_ppnodeExprScope = state->m_ppnodeExprScopeSave;
}

void Parser::AddToNodeListEscapedUse(ParseNode ** ppnodeList, ParseNode *** pppnodeLast,
    ParseNode * pnodeAdd)
{
    AddToNodeList(ppnodeList, pppnodeLast, pnodeAdd);
    pnodeAdd->SetIsInList();
}

void Parser::AddToNodeList(ParseNode ** ppnodeList, ParseNode *** pppnodeLast,
    ParseNode * pnodeAdd)
{
    Assert(!this->m_deferringAST);
    if (nullptr == *pppnodeLast)
    {
        // should be an empty list
        Assert(nullptr == *ppnodeList);

        *ppnodeList = pnodeAdd;
        *pppnodeLast = ppnodeList;
    }
    else
    {
        //
        Assert(*ppnodeList);
        Assert(**pppnodeLast);

        ParseNode *pnodeT = CreateBinNode(knopList, **pppnodeLast, pnodeAdd);
        **pppnodeLast = pnodeT;
        *pppnodeLast = &pnodeT->AsParseNodeBin()->pnode2;
    }
}

// Check reference to "arguments" that indicates the object may escape.
void Parser::CheckArguments(ParseNodePtr pnode)
{
    if (m_currentNodeFunc && this->NodeIsIdent(pnode, wellKnownPropertyPids.arguments))
    {
        m_currentNodeFunc->SetHasHeapArguments();
    }
}

// Check use of "arguments" that requires instantiation of the object.
void Parser::CheckArgumentsUse(IdentPtr pid, ParseNodeFnc * pnodeFnc)
{
    if (pid == wellKnownPropertyPids.arguments)
    {
        if (pnodeFnc != nullptr && pnodeFnc != m_currentNodeProg)
        {
            pnodeFnc->SetUsesArguments(TRUE);
        }
        else
        {
            m_UsesArgumentsAtGlobal = true;
        }
    }
}

void Parser::CheckStrictModeEvalArgumentsUsage(IdentPtr pid, ParseNodePtr pnode)
{
    if (pid != nullptr)
    {
        // In strict mode, 'eval' / 'arguments' cannot be assigned to.
        if (pid == wellKnownPropertyPids.eval)
        {
            Error(ERREvalUsage, pnode);
        }

        if (pid == wellKnownPropertyPids.arguments)
        {
            Error(ERRArgsUsage, pnode);
        }
    }
}

void Parser::ReduceDeferredScriptLength(size_t chars)
{
    // If we're in deferred mode, subtract the given char count from the total length,
    // and see if this puts us under the deferral threshold.
    if (((m_grfscr & (fscrCanDeferFncParse | fscrWillDeferFncParse)) == (fscrCanDeferFncParse | fscrWillDeferFncParse)) &&
        (
            PHASE_OFF1(Js::DeferEventHandlersPhase) ||
            (m_grfscr & fscrGlobalCode)
            )
        )
    {
        if (m_length > chars)
        {
            m_length -= chars;
        }
        else
        {
            m_length = 0;
        }
        if (m_length < Parser::GetDeferralThreshold(this->m_sourceContextInfo->IsSourceProfileLoaded()))
        {
            // Stop deferring.
            m_grfscr &= ~fscrWillDeferFncParse;
            m_stoppedDeferredParse = TRUE;
        }
    }
}

void Parser::EnsureStackAvailable()
{
    bool isInterrupt = false;
    if (!m_scriptContext->GetThreadContext()->IsStackAvailable(Js::Constants::MinStackCompile, &isInterrupt))
    {
        Error(isInterrupt ? E_ABORT : VBSERR_OutOfStack);
    }
}

void Parser::ThrowNewTargetSyntaxErrForGlobalScope()
{
    // If we are parsing a previously deferred function, we can skip throwing the SyntaxError for `new.target` at global scope.
    // If we are at global scope, we would have thrown a SyntaxError when we did the Upfront parse pass and we would not have
    // deferred the function in order to come back now and reparse it.
    if (m_parseType == ParseType_Deferred)
    {
        return;
    }

    if (GetCurrentNonLambdaFunctionNode() != nullptr)
    {
        return;
    }

    if ((this->m_grfscr & fscrEval) != 0)
    {
        Js::JavascriptFunction * caller = nullptr;
        if (Js::JavascriptStackWalker::GetCaller(&caller, m_scriptContext))
        {
            Js::FunctionBody * callerBody = caller->GetFunctionBody();
            Assert(callerBody);
            if (!callerBody->GetIsGlobalFunc() && !(callerBody->IsLambda() && callerBody->GetEnclosedByGlobalFunc()))
            {
                return;
            }
        }
    }

    Error(ERRInvalidNewTarget);
}

template<bool buildAST>
IdentPtr Parser::ParseMetaProperty(tokens metaParentKeyword, charcount_t ichMin, _Out_opt_ BOOL* pfCanAssign)
{
    AssertMsg(metaParentKeyword == tkNEW, "Only supported for tkNEW parent keywords");
    AssertMsg(this->m_token.tk == tkDot, "We must be currently sitting on the dot after the parent keyword");

    this->GetScanner()->Scan();

    if (this->m_token.tk == tkID && this->m_token.GetIdentifier(this->GetHashTbl()) == this->GetTargetPid())
    {
        ThrowNewTargetSyntaxErrForGlobalScope();
        if (pfCanAssign)
        {
            *pfCanAssign = FALSE;
        }
        return wellKnownPropertyPids._newTarget;
    }
    else
    {
        Error(ERRsyntax);
    }
}

template<bool buildAST>
void Parser::ParseNamedImportOrExportClause(ModuleImportOrExportEntryList* importOrExportEntryList, bool isExportClause)
{
    Assert(m_token.tk == tkLCurly);
    Assert(importOrExportEntryList != nullptr);

    this->GetScanner()->Scan();

    while (m_token.tk != tkRCurly && m_token.tk != tkEOF)
    {
        tokens firstToken = m_token.tk;

        if (!(m_token.IsIdentifier() || m_token.IsReservedWord()))
        {
            Error(ERRsyntax);
        }

        IdentPtr identifierName = m_token.GetIdentifier(this->GetHashTbl());
        IdentPtr identifierAs = identifierName;

        this->GetScanner()->Scan();

        if (m_token.tk == tkID)
        {
            // We have the pattern "IdentifierName as"
            if (wellKnownPropertyPids.as != m_token.GetIdentifier(this->GetHashTbl()))
            {
                Error(ERRsyntax);
            }

            this->GetScanner()->Scan();

            // If we are parsing an import statement, the token after 'as' must be a BindingIdentifier.
            if (!isExportClause)
            {
                ChkCurTokNoScan(tkID, ERRsyntax);
            }

            if (!(m_token.IsIdentifier() || m_token.IsReservedWord()))
            {
                Error(ERRsyntax);
            }

            identifierAs = m_token.GetIdentifier(this->GetHashTbl());

            // Scan to the next token.
            this->GetScanner()->Scan();
        }
        else if (!isExportClause && firstToken != tkID)
        {
            // If we are parsing an import statement and this ImportSpecifier clause did not have
            // 'as ImportedBinding' at the end of it, identifierName must be a BindingIdentifier.
            Error(ERRsyntax);
        }

        if (m_token.tk == tkComma)
        {
            // Consume a trailing comma
            this->GetScanner()->Scan();
        }

        if (buildAST)
        {
            // The name we will use 'as' this import/export is a binding identifier in import statements.
            if (!isExportClause)
            {
                CreateModuleImportDeclNode(identifierAs);
                AddModuleImportOrExportEntry(importOrExportEntryList, identifierName, identifierAs, nullptr, nullptr);
            }
            else
            {
                identifierName->SetIsModuleExport();
                AddModuleImportOrExportEntry(importOrExportEntryList, nullptr, identifierName, identifierAs, nullptr);
            }
        }
    }

    // Final token in a named import or export clause must be a '}'
    ChkCurTokNoScan(tkRCurly, ERRsyntax);
}

IdentPtrList* Parser::GetRequestedModulesList()
{
    return m_currentNodeProg->AsParseNodeModule()->requestedModules;
}

ModuleImportOrExportEntryList* Parser::GetModuleImportEntryList()
{
    return m_currentNodeProg->AsParseNodeModule()->importEntries;
}

ModuleImportOrExportEntryList* Parser::GetModuleLocalExportEntryList()
{
    return m_currentNodeProg->AsParseNodeModule()->localExportEntries;
}

ModuleImportOrExportEntryList* Parser::GetModuleIndirectExportEntryList()
{
    return m_currentNodeProg->AsParseNodeModule()->indirectExportEntries;
}

ModuleImportOrExportEntryList* Parser::GetModuleStarExportEntryList()
{
    return m_currentNodeProg->AsParseNodeModule()->starExportEntries;
}

IdentPtrList* Parser::EnsureRequestedModulesList()
{
    if (m_currentNodeProg->AsParseNodeModule()->requestedModules == nullptr)
    {
        m_currentNodeProg->AsParseNodeModule()->requestedModules = Anew(&m_nodeAllocator, IdentPtrList, &m_nodeAllocator);
    }
    return m_currentNodeProg->AsParseNodeModule()->requestedModules;
}

ModuleImportOrExportEntryList* Parser::EnsureModuleImportEntryList()
{
    if (m_currentNodeProg->AsParseNodeModule()->importEntries == nullptr)
    {
        m_currentNodeProg->AsParseNodeModule()->importEntries = Anew(&m_nodeAllocator, ModuleImportOrExportEntryList, &m_nodeAllocator);
    }
    return m_currentNodeProg->AsParseNodeModule()->importEntries;
}

ModuleImportOrExportEntryList* Parser::EnsureModuleLocalExportEntryList()
{
    if (m_currentNodeProg->AsParseNodeModule()->localExportEntries == nullptr)
    {
        m_currentNodeProg->AsParseNodeModule()->localExportEntries = Anew(&m_nodeAllocator, ModuleImportOrExportEntryList, &m_nodeAllocator);
    }
    return m_currentNodeProg->AsParseNodeModule()->localExportEntries;
}

ModuleImportOrExportEntryList* Parser::EnsureModuleIndirectExportEntryList()
{
    if (m_currentNodeProg->AsParseNodeModule()->indirectExportEntries == nullptr)
    {
        m_currentNodeProg->AsParseNodeModule()->indirectExportEntries = Anew(&m_nodeAllocator, ModuleImportOrExportEntryList, &m_nodeAllocator);
    }
    return m_currentNodeProg->AsParseNodeModule()->indirectExportEntries;
}

ModuleImportOrExportEntryList* Parser::EnsureModuleStarExportEntryList()
{
    if (m_currentNodeProg->AsParseNodeModule()->starExportEntries == nullptr)
    {
        m_currentNodeProg->AsParseNodeModule()->starExportEntries = Anew(&m_nodeAllocator, ModuleImportOrExportEntryList, &m_nodeAllocator);
    }
    return m_currentNodeProg->AsParseNodeModule()->starExportEntries;
}

void Parser::AddModuleSpecifier(IdentPtr moduleRequest)
{
    IdentPtrList* requestedModulesList = EnsureRequestedModulesList();

    if (!requestedModulesList->Has(moduleRequest))
    {
        requestedModulesList->Prepend(moduleRequest);
    }
}

ModuleImportOrExportEntry* Parser::AddModuleImportOrExportEntry(ModuleImportOrExportEntryList* importOrExportEntryList, ModuleImportOrExportEntry* importOrExportEntry)
{
    if (importOrExportEntry->exportName != nullptr)
    {
        CheckForDuplicateExportEntry(importOrExportEntryList, importOrExportEntry->exportName);
    }

    importOrExportEntryList->Prepend(*importOrExportEntry);

    return importOrExportEntry;
}

ModuleImportOrExportEntry* Parser::AddModuleImportOrExportEntry(ModuleImportOrExportEntryList* importOrExportEntryList, IdentPtr importName, IdentPtr localName, IdentPtr exportName, IdentPtr moduleRequest)
{
    ModuleImportOrExportEntry* importOrExportEntry = Anew(&m_nodeAllocator, ModuleImportOrExportEntry);

    importOrExportEntry->importName = importName;
    importOrExportEntry->localName = localName;
    importOrExportEntry->exportName = exportName;
    importOrExportEntry->moduleRequest = moduleRequest;

    return AddModuleImportOrExportEntry(importOrExportEntryList, importOrExportEntry);
}

void Parser::AddModuleLocalExportEntry(ParseNodePtr varDeclNode)
{
    AssertOrFailFast(varDeclNode->nop == knopVarDecl || varDeclNode->nop == knopLetDecl || varDeclNode->nop == knopConstDecl);

    IdentPtr localName = varDeclNode->AsParseNodeVar()->pid;
    varDeclNode->AsParseNodeVar()->sym->SetIsModuleExportStorage(true);

    AddModuleImportOrExportEntry(EnsureModuleLocalExportEntryList(), nullptr, localName, localName, nullptr);
}

void Parser::CheckForDuplicateExportEntry(ModuleImportOrExportEntryList* exportEntryList, IdentPtr exportName)
{
    ModuleImportOrExportEntry* findResult = exportEntryList->Find([&](ModuleImportOrExportEntry exportEntry)
    {
        if (exportName == exportEntry.exportName)
        {
            return true;
        }
        return false;
    });

    if (findResult != nullptr)
    {
        Error(ERRsyntax);
    }
}

template<bool buildAST>
void Parser::ParseImportClause(ModuleImportOrExportEntryList* importEntryList, bool parsingAfterComma)
{
    bool parsedNamespaceOrNamedImport = false;

    switch (m_token.tk)
    {
    case tkID:
        // This is the default binding identifier.

        // If we already saw a comma in the import clause, this is a syntax error.
        if (parsingAfterComma)
        {
            Error(ERRsyntax);
        }

        if (buildAST)
        {
            IdentPtr localName = m_token.GetIdentifier(this->GetHashTbl());
            IdentPtr importName = wellKnownPropertyPids._default;

            CreateModuleImportDeclNode(localName);
            AddModuleImportOrExportEntry(importEntryList, importName, localName, nullptr, nullptr);
        }

        break;

    case tkLCurly:
        // This begins a list of named imports.
        ParseNamedImportOrExportClause<buildAST>(importEntryList, false);

        parsedNamespaceOrNamedImport = true;
        break;

    case tkStar:
        // This begins a namespace import clause.
        // "* as ImportedBinding"

        // Token following * must be the identifier 'as'
        this->GetScanner()->Scan();
        if (m_token.tk != tkID || wellKnownPropertyPids.as != m_token.GetIdentifier(this->GetHashTbl()))
        {
            Error(ERRsyntax);
        }

        // Token following 'as' must be a binding identifier.
        this->GetScanner()->Scan();
        ChkCurTokNoScan(tkID, ERRsyntax);

        if (buildAST)
        {
            IdentPtr localName = m_token.GetIdentifier(this->GetHashTbl());
            IdentPtr importName = wellKnownPropertyPids._star;

            CreateModuleImportDeclNode(localName);
            AddModuleImportOrExportEntry(importEntryList, importName, localName, nullptr, nullptr);
        }

        parsedNamespaceOrNamedImport = true;
        break;

    default:
        Error(ERRsyntax);
    }

    this->GetScanner()->Scan();

    if (m_token.tk == tkComma)
    {
        // There cannot be more than one comma in a module import clause.
        // There cannot be a namespace import or named imports list on the left of the comma in a module import clause.
        if (parsingAfterComma || parsedNamespaceOrNamedImport)
        {
            Error(ERRsyntax);
        }

        this->GetScanner()->Scan();

        ParseImportClause<buildAST>(importEntryList, true);
    }
}

bool Parser::IsImportOrExportStatementValidHere()
{
    ParseNodeFnc * curFunc = GetCurrentFunctionNode();

    // Import must be located in the top scope of the module body.
    return curFunc->nop == knopFncDecl
        && curFunc->IsModule()
        && this->m_currentBlockInfo->pnodeBlock == curFunc->pnodeBodyScope
        && (this->m_grfscr & fscrEvalCode) != fscrEvalCode
        && this->m_tryCatchOrFinallyDepth == 0
        && !this->m_disallowImportExportStmt;
}

bool Parser::IsTopLevelModuleFunc()
{
    ParseNodeFnc * curFunc = GetCurrentFunctionNode();
    return curFunc->nop == knopFncDecl && curFunc->IsModule();
}

template<bool buildAST> ParseNodePtr Parser::ParseImportCall()
{
    this->GetScanner()->Scan();
    ParseNodePtr specifier = ParseExpr<buildAST>(koplCma, nullptr, /* fAllowIn */FALSE, /* fAllowEllipsis */FALSE);
    if (m_token.tk != tkRParen)
    {
        Error(ERRnoRparen);
    }

    this->GetScanner()->Scan();
    return buildAST ? CreateCallNode(knopCall, CreateNodeForOpT<knopImport>(), specifier) : nullptr;
}

template<bool buildAST>
ParseNodePtr Parser::ParseImport()
{
    Assert(m_scriptContext->GetConfig()->IsES6ModuleEnabled());
    Assert(m_token.tk == tkIMPORT);

    RestorePoint parsedImport;
    this->GetScanner()->Capture(&parsedImport);
    this->GetScanner()->Scan();

    // import()
    if (m_token.tk == tkLParen)
    {
        if (!m_scriptContext->GetConfig()->IsESDynamicImportEnabled())
        {
            Error(ERRsyntax);
        }

        ParseNodePtr pnode = ParseImportCall<buildAST>();
        BOOL fCanAssign;
        IdentToken token;
        return ParsePostfixOperators<buildAST>(pnode, TRUE, FALSE, FALSE, TRUE, &fCanAssign, &token);
    }

    this->GetScanner()->SeekTo(parsedImport);

    if (!IsImportOrExportStatementValidHere())
    {
        Error(ERRInvalidModuleImportOrExport);
    }

    // We just parsed an import token. Next valid token is *, {, string constant, or binding identifier.
    this->GetScanner()->Scan();

    if (m_token.tk == tkStrCon)
    {
        // This import declaration has no import clause.
        // "import ModuleSpecifier;"
        if (buildAST)
        {
            AddModuleSpecifier(m_token.GetStr());
        }

        // Scan past the module identifier.
        this->GetScanner()->Scan();
    }
    else
    {
        ModuleImportOrExportEntryList importEntryList(&m_nodeAllocator);

        // Parse the import clause (default binding can only exist before the comma).
        ParseImportClause<buildAST>(&importEntryList);

        // Token following import clause must be the identifier 'from'
        IdentPtr moduleSpecifier = ParseImportOrExportFromClause<buildAST>(true);

        if (buildAST)
        {
            Assert(moduleSpecifier != nullptr);

            AddModuleSpecifier(moduleSpecifier);

            importEntryList.Map([this, moduleSpecifier](ModuleImportOrExportEntry& importEntry) {
                importEntry.moduleRequest = moduleSpecifier;
                AddModuleImportOrExportEntry(EnsureModuleImportEntryList(), &importEntry);
            });
        }

        importEntryList.Clear();
    }

    // Import statement is actually a nop, we hoist all the imported bindings to the top of the module.
    return nullptr;
}

template<bool buildAST>
IdentPtr Parser::ParseImportOrExportFromClause(bool throwIfNotFound)
{
    IdentPtr moduleSpecifier = nullptr;

    if (m_token.tk == tkID && wellKnownPropertyPids.from == m_token.GetIdentifier(this->GetHashTbl()))
    {
        this->GetScanner()->Scan();

        // Token following the 'from' token must be a string constant - the module specifier.
        ChkCurTokNoScan(tkStrCon, ERRsyntax);

        if (buildAST)
        {
            moduleSpecifier = m_token.GetStr();
        }

        this->GetScanner()->Scan();
    }
    else if (throwIfNotFound)
    {
        Error(ERRsyntax);
    }

    return moduleSpecifier;
}

template<bool buildAST>
ParseNodePtr Parser::ParseDefaultExportClause()
{
    Assert(m_token.tk == tkDEFAULT);

    this->GetScanner()->Scan();
    ParseNodePtr pnode = nullptr;
    ushort flags = fFncNoFlgs;

    switch (m_token.tk)
    {
    case tkCLASS:
    {
        if (!m_scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled())
        {
            goto LDefault;
        }

        // Before we parse the class itself we need to know if the class has an identifier name.
        // If it does, we'll treat this class as an ordinary class declaration which will bind
        // it to that name. Otherwise the class should parse as a nameless class expression and
        // bind only to the export binding.
        BOOL classHasName = false;
        RestorePoint parsedClass;
        this->GetScanner()->Capture(&parsedClass);
        this->GetScanner()->Scan();

        if (m_token.tk == tkID)
        {
            classHasName = true;
        }

        this->GetScanner()->SeekTo(parsedClass);
        ParseNodeClass * pnodeClass;
        pnode = pnodeClass = ParseClassDecl<buildAST>(classHasName, nullptr, nullptr, nullptr);

        if (buildAST)
        {
            AnalysisAssert(pnode != nullptr);
            Assert(pnode->nop == knopClassDecl);

            pnodeClass->SetIsDefaultModuleExport(true);
        }

        break;
    }
    case tkID:
        // If we parsed an async token, it could either modify the next token (if it is a
        // function token) or it could be an identifier (let async = 0; export default async;).
        // To handle both cases, when we parse an async token we need to keep the parser state
        // and rewind if the next token is not function.
        if (wellKnownPropertyPids.async == m_token.GetIdentifier(this->GetHashTbl()))
        {
            RestorePoint parsedAsync;
            this->GetScanner()->Capture(&parsedAsync);
            this->GetScanner()->Scan();
            if (m_token.tk == tkFUNCTION)
            {
                // Token after async is function, consume the async token and continue to parse the
                // function as an async function.
                flags |= fFncAsync;
                goto LFunction;
            }
            // Token after async is not function, no idea what the async token is supposed to mean
            // so rewind and let the default case handle it.
            this->GetScanner()->SeekTo(parsedAsync);
        }
        goto LDefault;
        break;
    case tkFUNCTION:
    {
    LFunction:
        // We just parsed a function token but we need to figure out if the function
        // has an identifier name or not before we call the helper.
        RestorePoint parsedFunction;
        this->GetScanner()->Capture(&parsedFunction);
        this->GetScanner()->Scan();

        if (m_token.tk == tkStar)
        {
            // If we saw 'function*' that indicates we are going to parse a generator,
            // but doesn't tell us if the generator has an identifier or not.
            // Skip the '*' token for now as it doesn't matter yet.
            this->GetScanner()->Scan();
        }

        // We say that if the function has an identifier name, it is a 'normal' declaration
        // and should create a binding to that identifier as well as one for our default export.
        if (m_token.tk == tkID)
        {
            flags |= fFncDeclaration;
        }
        else
        {
            flags |= fFncNoName;
        }

        // Rewind back to the function token and let the helper handle the parsing.
        this->GetScanner()->SeekTo(parsedFunction);
        pnode = ParseFncDeclCheckScope<buildAST>(flags);

        if (buildAST)
        {
            AnalysisAssert(pnode != nullptr);
            Assert(pnode->nop == knopFncDecl);

            pnode->AsParseNodeFnc()->SetIsDefaultModuleExport(true);
        }
        break;
    }
    default:
    LDefault:
    {
        ParseNodePtr pnodeExpression = ParseExpr<buildAST>();

        // Consider: Can we detect this syntax error earlier?
        if (pnodeExpression && pnodeExpression->nop == knopComma)
        {
            Error(ERRsyntax);
        }

        if (buildAST)
        {
            AnalysisAssert(pnodeExpression != nullptr);

            // Mark this node as the default module export. We need to make sure it is put into the correct
            // module export slot when we emit the node.
            ParseNodeExportDefault * pnodeExportDefault;
            pnode = pnodeExportDefault = CreateNodeForOpT<knopExportDefault>();
            pnode->AsParseNodeExportDefault()->pnodeExpr = pnodeExpression;
        }
        break;
    }
    }

    IdentPtr exportName = wellKnownPropertyPids._default;
    IdentPtr localName = wellKnownPropertyPids._starDefaultStar;
    AddModuleImportOrExportEntry(EnsureModuleLocalExportEntryList(), nullptr, localName, exportName, nullptr);

    return pnode;
}

template<bool buildAST>
ParseNodePtr Parser::ParseExportDeclaration(bool *needTerminator)
{
    Assert(m_scriptContext->GetConfig()->IsES6ModuleEnabled());
    Assert(m_token.tk == tkEXPORT);

    if (!IsImportOrExportStatementValidHere())
    {
        Error(ERRInvalidModuleImportOrExport);
    }

    ParseNodePtr pnode = nullptr;
    IdentPtr moduleIdentifier = nullptr;
    tokens declarationType;

    if (needTerminator != nullptr)
    {
        *needTerminator = false;
    }

    // We just parsed an export token. Next valid tokens are *, {, var, let, const, async, function, class, default.
    this->GetScanner()->Scan();

    switch (m_token.tk)
    {
    case tkStar:
        this->GetScanner()->Scan();

        // A star token in an export declaration must be followed by a from clause which begins with a token 'from'.
        moduleIdentifier = ParseImportOrExportFromClause<buildAST>(true);

        if (buildAST)
        {
            Assert(moduleIdentifier != nullptr);

            AddModuleSpecifier(moduleIdentifier);
            IdentPtr importName = wellKnownPropertyPids._star;

            AddModuleImportOrExportEntry(EnsureModuleStarExportEntryList(), importName, nullptr, nullptr, moduleIdentifier);
        }

        if (needTerminator != nullptr)
        {
            *needTerminator = true;
        }

        break;

    case tkLCurly:
    {
        ModuleImportOrExportEntryList exportEntryList(&m_nodeAllocator);

        ParseNamedImportOrExportClause<buildAST>(&exportEntryList, true);

        this->GetScanner()->Scan();

        // Export clause may be followed by a from clause.
        moduleIdentifier = ParseImportOrExportFromClause<buildAST>(false);

        if (buildAST)
        {
            if (moduleIdentifier != nullptr)
            {
                AddModuleSpecifier(moduleIdentifier);
            }

            exportEntryList.Map([this, moduleIdentifier](ModuleImportOrExportEntry& exportEntry) {
                if (moduleIdentifier != nullptr)
                {
                    exportEntry.moduleRequest = moduleIdentifier;

                    // We need to swap localname and importname when this is a re-export.
                    exportEntry.importName = exportEntry.localName;
                    exportEntry.localName = nullptr;

                    AddModuleImportOrExportEntry(EnsureModuleIndirectExportEntryList(), &exportEntry);
                }
                else
                {
                    AddModuleImportOrExportEntry(EnsureModuleLocalExportEntryList(), &exportEntry);
                }
            });

            exportEntryList.Clear();
        }
    }

    if (needTerminator != nullptr)
    {
        *needTerminator = true;
    }

    break;

    case tkID:
    {
        IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());

        if (wellKnownPropertyPids.let == pid)
        {
            declarationType = tkLET;
            goto ParseVarDecl;
        }
        if (wellKnownPropertyPids.async == pid && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            // In module export statements, async token is only valid if it's followed by function.
            // We need to check here because ParseStatement would think 'async = 20' is a var decl.
            RestorePoint parsedAsync;
            this->GetScanner()->Capture(&parsedAsync);
            this->GetScanner()->Scan();
            if (m_token.tk == tkFUNCTION)
            {
                // Token after async is function, rewind to the async token and let ParseStatement handle it.
                this->GetScanner()->SeekTo(parsedAsync);
                goto ParseFunctionDecl;
            }
            // Token after async is not function, it's a syntax error.
        }
        goto ErrorToken;
    }
    case tkVAR:
    case tkLET:
    case tkCONST:
    {
        declarationType = m_token.tk;

    ParseVarDecl:
        this->GetScanner()->Scan();

        pnode = ParseVariableDeclaration<buildAST>(declarationType, this->GetScanner()->IchMinTok());

        if (buildAST)
        {
            ForEachItemInList(pnode, [&](ParseNodePtr item) {
                if (item->nop == knopAsg)
                {
                    Parser::MapBindIdentifier(item, [&](ParseNodePtr subItem)
                    {
                        AddModuleLocalExportEntry(subItem);
                    });
                }
                else
                {
                    AddModuleLocalExportEntry(item);
                }
            });
        }
    }
    break;

    case tkFUNCTION:
    case tkCLASS:
    {
    ParseFunctionDecl:
        pnode = ParseStatement<buildAST>();

        if (buildAST)
        {
            IdentPtr localName;
            if (pnode->nop == knopClassDecl)
            {
                ParseNodeClass * pnodeClass = pnode->AsParseNodeClass();
                pnodeClass->pnodeDeclName->sym->SetIsModuleExportStorage(true);
                localName = pnodeClass->pnodeName->pid;
            }
            else
            {
                Assert(pnode->nop == knopFncDecl);

                ParseNodeFnc * pnodeFnc = pnode->AsParseNodeFnc();
                pnodeFnc->GetFuncSymbol()->SetIsModuleExportStorage(true);
                localName = pnodeFnc->pid;
            }
            Assert(localName != nullptr);

            AddModuleImportOrExportEntry(EnsureModuleLocalExportEntryList(), nullptr, localName, localName, nullptr);
        }
    }
    break;

    case tkDEFAULT:
    {
        pnode = ParseDefaultExportClause<buildAST>();
    }
    break;

    default:
    {
    ErrorToken:
        Error(ERRsyntax);
    }
    }

    return pnode;
}

/***************************************************************************
Parse an expression term.
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseTerm(BOOL fAllowCall,
    LPCOLESTR pNameHint,
    uint32 *pHintLength,
    uint32 *pShortNameOffset,
    _Inout_opt_ IdentToken* pToken /*= nullptr*/,
    bool fUnaryOrParen /*= false*/,
    BOOL fCanAssignToCall /*= TRUE*/,
    _Out_opt_ BOOL* pfCanAssign /*= nullptr*/,
    _Inout_opt_ BOOL* pfLikelyPattern /*= nullptr*/,
    _Out_opt_ bool* pfIsDotOrIndex /*= nullptr*/,
    _Inout_opt_ charcount_t *plastRParen /*= nullptr*/)
{
    ParseNodePtr pnode = nullptr;
    PidRefStack *savedTopAsyncRef = nullptr;
    charcount_t ichMin = 0;
    charcount_t ichLim = 0;
    size_t iecpMin = 0;
    size_t iecpLim = 0;
    size_t iuMin;
    IdentToken term;
    BOOL fInNew = FALSE;
    BOOL fCanAssign = TRUE;
    bool isAsyncExpr = false;
    bool isLambdaExpr = false;
    bool isSpecialName = false;
    IdentPtr pid = nullptr;
    Assert(pToken == nullptr || pToken->tk == tkNone); // Must be empty initially

    PROBE_STACK_NO_DISPOSE(m_scriptContext, Js::Constants::MinStackParseOneTerm);

    switch (m_token.tk)
    {
    case tkID:
    {
        pid = m_token.GetIdentifier(this->GetHashTbl());
        ichMin = this->GetScanner()->IchMinTok();
        iecpMin = this->GetScanner()->IecpMinTok();
        ichLim = this->GetScanner()->IchLimTok();
        iecpLim = this->GetScanner()->IecpLimTok();

        if (pid == wellKnownPropertyPids.async &&
            m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            isAsyncExpr = true;
        }

        // Put this into a block to avoid previousAwaitIsKeyword being not initialized after jump to LIdentifier
        {
            bool previousAwaitIsKeyword = this->GetScanner()->SetAwaitIsKeywordRegion(isAsyncExpr);
            this->GetScanner()->Scan();
            this->GetScanner()->SetAwaitIsKeywordRegion(previousAwaitIsKeyword);
        }

        // We search for an Async expression (a function declaration or an async lambda expression)
        if (isAsyncExpr && !this->GetScanner()->FHadNewLine())
        {
            if (m_token.tk == tkFUNCTION)
            {
                goto LFunction;
            }
            else if (m_token.tk == tkID || m_token.tk == tkAWAIT)
            {
                isLambdaExpr = true;
                goto LFunction;
            }
            else if (m_token.tk == tkLParen)
            {
                // This is potentially an async arrow function. Save the state of the async references
                // in case it needs to be restored. (Note that the case of a single parameter with no ()'s
                // is detected upstream and need not be handled here.)
                savedTopAsyncRef = pid->GetTopRef();
            }
        }

        CheckArgumentsUse(pid, GetCurrentFunctionNode());

        // Assume this pid is not special - overwrite when we parse a special name
        isSpecialName = false;

    LIdentifier:
        PidRefStack * ref = nullptr;

        // Don't push a reference if this is a single lambda parameter, because we'll reparse with
        // a correct function ID.
        if (m_token.tk != tkDArrow)
        {
            ref = this->PushPidRef(pid);
        }

        if (buildAST)
        {
            if (isSpecialName)
            {
                pnode = CreateSpecialNameNode(pid, ref, ichMin, ichLim);
            }
            else
            {
                pnode = CreateNameNode(pid, ref, ichMin, ichLim);
            }
        }
        else
        {
            // Remember the identifier start and end in case it turns out to be a statement label.
            term.tk = tkID;
            term.pid = pid; // Record the identifier for detection of eval
            term.ichMin = static_cast<charcount_t>(iecpMin);
            term.ichLim = static_cast<charcount_t>(iecpLim);
        }
        break;
    }

    case tkSUPER:
        ichMin = this->GetScanner()->IchMinTok();
        iecpMin = this->GetScanner()->IecpMinTok();
        ichLim = this->GetScanner()->IchLimTok();
        iecpLim = this->GetScanner()->IecpLimTok();

        if (!m_scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled())
        {
            goto LUnknown;
        }

        this->GetScanner()->Scan();

        pid = ParseSuper<buildAST>(!!fAllowCall);
        isSpecialName = true;

        // Super reference and super call need to push a pid ref to 'this' even when not building an AST
        ReferenceSpecialName(wellKnownPropertyPids._this, ichMin, ichLim);
        // Super call needs to reference 'new.target'
        if (pid == wellKnownPropertyPids._superConstructor)
        {
            ReferenceSpecialName(wellKnownPropertyPids._newTarget, ichMin, ichLim);
        }

        goto LIdentifier;

    case tkTHIS:
        ichMin = this->GetScanner()->IchMinTok();
        iecpMin = this->GetScanner()->IecpMinTok();
        ichLim = this->GetScanner()->IchLimTok();
        iecpLim = this->GetScanner()->IecpLimTok();

        pid = wellKnownPropertyPids._this;

        this->GetScanner()->Scan();
        isSpecialName = true;

        goto LIdentifier;

    case tkLParen:
    {
        ichMin = this->GetScanner()->IchMinTok();
        iuMin = this->GetScanner()->IecpMinTok();

        // If we are undeferring a function which has deferred stubs, we can check to see if the next deferred stub
        // is a lambda at the current character. If it is, we know this LParen is the beginning of a lambda nested
        // function and we can avoid parsing the next series of tokens as a parenthetical expression and reparsing
        // after finding the => token.
        if (buildAST && m_currDeferredStub != nullptr && GetCurrentFunctionNode() != nullptr && GetCurrentFunctionNode()->nestedCount < m_currDeferredStubCount)
        {
            DeferredFunctionStub* stub = m_currDeferredStub + GetCurrentFunctionNode()->nestedCount;
            if (stub->ichMin == ichMin)
            {
                Assert((stub->fncFlags & kFunctionIsLambda) == kFunctionIsLambda);

                pnode = ParseFncDeclCheckScope<true>(fFncLambda);
                break;
            }
        }

        this->GetScanner()->Scan();
        if (m_token.tk == tkRParen)
        {
            // Empty parens can only be legal as an empty parameter list to a lambda declaration.
            // We're in a lambda if the next token is =>.
            fAllowCall = FALSE;
            this->GetScanner()->Scan();

            // If the token after the right paren is not => or if there was a newline between () and => this is a syntax error
            if (!IsDoingFastScan() && (m_token.tk != tkDArrow || this->GetScanner()->FHadNewLine()))
            {
                Error(ERRsyntax);
            }

            if (buildAST)
            {
                pnode = CreateNodeForOpT<knopEmpty>();
            }
            break;
        }

        // Advance the block ID here in case this parenthetical expression turns out to be a lambda parameter list.
        // That way the pid ref stacks will be created in their correct final form, and we can simply fix
        // up function ID's.
        uint saveNextBlockId = m_nextBlockId;
        uint saveCurrBlockId = GetCurrentBlock()->blockId;
        GetCurrentBlock()->blockId = m_nextBlockId++;

        AutoDeferErrorsRestore deferErrorRestore(this);

        this->m_funcParenExprDepth++;
        pnode = ParseExpr<buildAST>(koplNo, &fCanAssign, TRUE, FALSE, nullptr, nullptr /*nameLength*/, nullptr  /*pShortNameOffset*/, &term, true, nullptr, plastRParen);
        this->m_funcParenExprDepth--;

        if (buildAST && plastRParen)
        {
            *plastRParen = this->GetScanner()->IchLimTok();
        }

        ChkCurTok(tkRParen, ERRnoRparen);

        GetCurrentBlock()->blockId = saveCurrBlockId;
        if (m_token.tk == tkDArrow)
        {
            // We're going to rewind and reinterpret the expression as a parameter list.
            // Put back the original next-block-ID so the existing pid ref stacks will be correct.
            m_nextBlockId = saveNextBlockId;
        }
        else
        {
            // Emit a deferred ... error if one was parsed.
            if (m_deferEllipsisError)
            {
                this->GetScanner()->SeekTo(m_deferEllipsisErrorLoc);
                Error(ERRInvalidSpreadUse);
            }
            else if (m_deferCommaError)
            {
                // Emit a comma error if that was deferred.
                this->GetScanner()->SeekTo(m_deferCommaErrorLoc);
                Error(ERRsyntax);
            }
        }

        break;
    }

    case tkIntCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        if (buildAST)
        {
            pnode = CreateIntNode(m_token.GetLong());
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkFltCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        if (buildAST)
        {
            ParseNodeFloat * pnodeFloat;
            pnode = pnodeFloat = CreateNodeForOpT<knopFlt>();
            pnodeFloat->dbl = m_token.GetDouble();
            pnodeFloat->maybeInt = m_token.GetDoubleMayBeInt();
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkStrCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        if (buildAST)
        {
            pnode = CreateStrNode(m_token.GetStr());
        }
        else
        {
            // Subtract the string literal length from the total char count for the purpose
            // of deciding whether to defer parsing and byte code generation.
            this->ReduceDeferredScriptLength(this->GetScanner()->IchLimTok() - this->GetScanner()->IchMinTok());
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkTRUE:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopTrue>();
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkFALSE:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopFalse>();
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkNULL:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopNull>();
        }
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkDiv:
    case tkAsgDiv:
        pnode = ParseRegExp<buildAST>();
        fCanAssign = FALSE;
        this->GetScanner()->Scan();
        break;

    case tkNEW:
    {
        ichMin = this->GetScanner()->IchMinTok();
        iecpMin = this->GetScanner()->IecpMinTok();
        this->GetScanner()->Scan();

        if (m_token.tk == tkDot && m_scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled())
        {
            pid = ParseMetaProperty<buildAST>(tkNEW, ichMin, &fCanAssign);

            ichLim = this->GetScanner()->IchLimTok();
            iecpLim = this->GetScanner()->IecpLimTok();

            this->GetScanner()->Scan();
            isSpecialName = true;

            goto LIdentifier;
        }
        else
        {
            ParseNodePtr pnodeExpr = ParseTerm<buildAST>(FALSE, pNameHint, pHintLength, pShortNameOffset, nullptr, false, TRUE, nullptr, nullptr, nullptr, plastRParen);
            if (buildAST)
            {
                pnode = CreateCallNode(knopNew, pnodeExpr, nullptr);
                pnode->ichMin = ichMin;
            }
            fInNew = TRUE;
            fCanAssign = FALSE;
        }
        break;
    }

    case tkLBrack:
    {
        ichMin = this->GetScanner()->IchMinTok();
        this->GetScanner()->Scan();
        pnode = ParseArrayLiteral<buildAST>();
        if (buildAST)
        {
            pnode->ichMin = ichMin;
            pnode->ichLim = this->GetScanner()->IchLimTok();
        }

        if (this->m_arrayDepth == 0)
        {
            Assert(this->GetScanner()->IchLimTok() - ichMin > m_funcInArray);
            this->ReduceDeferredScriptLength(this->GetScanner()->IchLimTok() - ichMin - this->m_funcInArray);
            this->m_funcInArray = 0;
            this->m_funcInArrayDepth = 0;
        }
        ChkCurTok(tkRBrack, ERRnoRbrack);
        if (!IsES6DestructuringEnabled())
        {
            fCanAssign = FALSE;
        }
        else if (pfLikelyPattern != nullptr && !IsPostFixOperators())
        {
            *pfLikelyPattern = TRUE;
        }
        break;
    }

    case tkLCurly:
    {
        ichMin = this->GetScanner()->IchMinTok();
        this->GetScanner()->ScanForcingPid();
        ParseNodePtr pnodeMemberList = ParseMemberList<buildAST>(pNameHint, pHintLength);
        if (buildAST)
        {
            pnode = CreateUniNode(knopObject, pnodeMemberList);
            pnode->ichMin = ichMin;
            pnode->ichLim = this->GetScanner()->IchLimTok();
        }
        ChkCurTok(tkRCurly, ERRnoRcurly);
        if (!IsES6DestructuringEnabled())
        {
            fCanAssign = FALSE;
        }
        else if (pfLikelyPattern != nullptr && !IsPostFixOperators())
        {
            *pfLikelyPattern = TRUE;
        }
        break;
    }

    case tkFUNCTION:
    {
    LFunction:
        if (m_grfscr & fscrDeferredFncExpression)
        {
            // The top-level deferred function body was defined by a function expression whose parsing was deferred. We are now
            // parsing it, so unset the flag so that any nested functions are parsed normally. This flag is only applicable the
            // first time we see it.
            //
            // Normally, deferred functions will be parsed in ParseStatement upon encountering the 'function' token. The first
            // token of the source code of the function may not a 'function' token though, so we still need to reset this flag
            // for the first function we parse. This can happen in compat modes, for instance, for a function expression enclosed
            // in parentheses, where the legacy behavior was to include the parentheses in the function's source code.
            m_grfscr &= ~fscrDeferredFncExpression;
        }
        ushort flags = fFncNoFlgs;
        if (isLambdaExpr)
        {
            flags |= fFncLambda;
        }
        if (isAsyncExpr)
        {
            flags |= fFncAsync;
        }
        pnode = ParseFncDeclNoCheckScope<buildAST>(flags, SuperRestrictionState::Disallowed, pNameHint, /* needsPIDOnRCurlyScan */ false, fUnaryOrParen);
        if (isAsyncExpr)
        {
            pnode->AsParseNodeFnc()->cbMin = iecpMin;
            pnode->ichMin = ichMin;
        }
        fCanAssign = FALSE;
        break;
    }

    case tkCLASS:
        if (m_scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled())
        {
            pnode = ParseClassDecl<buildAST>(FALSE, pNameHint, pHintLength, pShortNameOffset);
        }
        else
        {
            goto LUnknown;
        }
        fCanAssign = FALSE;
        break;

    case tkStrTmplBasic:
    case tkStrTmplBegin:
        pnode = ParseStringTemplateDecl<buildAST>(nullptr);
        fCanAssign = FALSE;
        break;

    case tkIMPORT:
        if (m_scriptContext->GetConfig()->IsES6ModuleEnabled() && m_scriptContext->GetConfig()->IsESDynamicImportEnabled())
        {
            this->GetScanner()->Scan();
            ChkCurTokNoScan(tkLParen, ERRnoLparen);
            pnode = ParseImportCall<buildAST>();
        }
        else
        {
            goto LUnknown;
        }
        break;

#if ENABLE_BACKGROUND_PARSING
    case tkCASE:
    {
        if (!m_doingFastScan)
        {
            goto LUnknown;
        }
        ParseNodePtr pnodeUnused;
        pnode = ParseCase<buildAST>(&pnodeUnused);
        break;
    }

    case tkELSE:
        if (!m_doingFastScan)
        {
            goto LUnknown;
        }
        this->GetScanner()->Scan();
        ParseStatement<buildAST>();
        break;
#endif

    default:
    LUnknown:
        Error(ERRsyntax);
        break;
    }

    pnode = ParsePostfixOperators<buildAST>(pnode, fAllowCall, fInNew, isAsyncExpr, fCanAssignToCall, &fCanAssign, &term, pfIsDotOrIndex);

    if (savedTopAsyncRef != nullptr &&
        this->m_token.tk == tkDArrow)
    {
        // This is an async arrow function; we're going to back up and reparse it.
        // Make sure we don't leave behind a bogus reference to the 'async' identifier.
        for (pid = wellKnownPropertyPids.async; pid->GetTopRef() != savedTopAsyncRef;)
        {
            Assert(pid->GetTopRef() != nullptr);
            pid->RemovePrevPidRef(nullptr);
        }
    }

    // Pass back identifier if requested
    if (pToken && term.tk == tkID)
    {
        *pToken = term;
    }

    if (pfCanAssign)
    {
        *pfCanAssign = fCanAssign;
    }

    return pnode;
}

template <bool buildAST>
ParseNodeRegExp * Parser::ParseRegExp()
{
    ParseNodeRegExp * pnode = nullptr;

    if (buildAST || IsDoingFastScan())
    {
        this->GetScanner()->RescanRegExp();

#if ENABLE_BACKGROUND_PARSING
        BOOL saveDeferringAST = this->m_deferringAST;
        if (m_doingFastScan)
        {
            this->m_deferringAST = false;
        }
#endif
        pnode = CreateNodeForOpT<knopRegExp>();
        pnode->regexPattern = m_token.GetRegex();
#if ENABLE_BACKGROUND_PARSING
        if (m_doingFastScan)
        {
            this->m_deferringAST = saveDeferringAST;
            this->AddFastScannedRegExpNode(pnode);
            if (!buildAST)
            {
                pnode = nullptr;
            }
        }
        else if (this->IsBackgroundParser())
        {
            Assert(pnode->regexPattern == nullptr);
            this->AddBackgroundRegExpNode(pnode);
        }
#endif
    }
    else
    {
        this->GetScanner()->RescanRegExpNoAST();
    }
    Assert(m_token.tk == tkRegExp);

    return pnode;
}

BOOL Parser::NodeIsEvalName(ParseNodePtr pnode)
{
    //WOOB 1107758 Special case of indirect eval binds to local scope in standards mode
    return pnode->nop == knopName && (pnode->AsParseNodeName()->pid == wellKnownPropertyPids.eval);
}

BOOL Parser::NodeIsSuperName(ParseNodePtr pnode)
{
    return pnode->nop == knopName && (pnode->AsParseNodeName()->pid == wellKnownPropertyPids._superConstructor);
}

BOOL Parser::NodeEqualsName(ParseNodePtr pnode, LPCOLESTR sz, uint32 cch)
{
    return pnode->nop == knopName &&
        pnode->AsParseNodeName()->pid->Cch() == cch &&
        !wmemcmp(pnode->AsParseNodeName()->pid->Psz(), sz, cch);
}

BOOL Parser::NodeIsIdent(ParseNodePtr pnode, IdentPtr pid)
{
    for (;;)
    {
        switch (pnode->nop)
        {
        case knopName:
            return (pnode->AsParseNodeName()->pid == pid);

        case knopComma:
            pnode = pnode->AsParseNodeBin()->pnode2;
            break;

        default:
            return FALSE;
        }
    }
}

template<bool buildAST>
ParseNodePtr Parser::ParsePostfixOperators(
    ParseNodePtr pnode,
    BOOL fAllowCall,
    BOOL fInNew,
    BOOL isAsyncExpr,
    BOOL fCanAssignToCallResult,
    BOOL *pfCanAssign,
    _Inout_ IdentToken* pToken,
    _Out_opt_ bool* pfIsDotOrIndex /*= nullptr */)
{
    uint16 count = 0;
    bool callOfConstants = false;
    if (pfIsDotOrIndex)
    {
        *pfIsDotOrIndex = false;
    }

    for (;;)
    {
        uint16 spreadArgCount = 0;
        switch (m_token.tk)
        {
        case tkLParen:
        {
            AutoMarkInParsingArgs autoMarkInParsingArgs(this);

            if (fInNew)
            {
                ParseNodePtr pnodeArgs = ParseArgList<buildAST>(&callOfConstants, &spreadArgCount, &count);
                if (buildAST)
                {
                    Assert(pnode->nop == knopNew);
                    Assert(pnode->AsParseNodeCall()->pnodeArgs == nullptr);
                    pnode->AsParseNodeCall()->pnodeArgs = pnodeArgs;
                    pnode->AsParseNodeCall()->callOfConstants = callOfConstants;
                    pnode->AsParseNodeCall()->isApplyCall = false;
                    pnode->AsParseNodeCall()->isEvalCall = false;
                    pnode->AsParseNodeCall()->isSuperCall = false;
                    pnode->AsParseNodeCall()->hasDestructuring = m_hasDestructuringPattern;
                    Assert(!m_hasDestructuringPattern || count > 0);
                    pnode->AsParseNodeCall()->argCount = count;
                    pnode->AsParseNodeCall()->spreadArgCount = spreadArgCount;
                    pnode->ichLim = this->GetScanner()->IchLimTok();
                }
                else
                {
                    pnode = nullptr;
                    pToken->tk = tkNone; // This is no longer an identifier
                }
                fInNew = FALSE;
                ChkCurTok(tkRParen, ERRnoRparen);
            }
            else
            {
                if (!fAllowCall)
                {
                    return pnode;
                }

                uint saveNextBlockId = m_nextBlockId;
                uint saveCurrBlockId = GetCurrentBlock()->blockId;

                if (isAsyncExpr)
                {
                    // Advance the block ID here in case this parenthetical expression turns out to be a lambda parameter list.
                    // That way the pid ref stacks will be created in their correct final form, and we can simply fix
                    // up function ID's.
                    GetCurrentBlock()->blockId = m_nextBlockId++;
                }

                ParseNodePtr pnodeArgs = ParseArgList<buildAST>(&callOfConstants, &spreadArgCount, &count);
                // We used to un-defer a deferred function body here if it was called as part of the expression that declared it.
                // We now detect this case up front in ParseFncDecl, which is cheaper and simpler.
                if (buildAST)
                {
                    bool fCallIsEval = false;

                    // Detect super()
                    if (this->NodeIsSuperName(pnode))
                    {
                        pnode = CreateSuperCallNode(pnode->AsParseNodeSpecialName(), pnodeArgs);
                        Assert(pnode);

                        pnode->AsParseNodeSuperCall()->pnodeThis = ReferenceSpecialName(wellKnownPropertyPids._this, pnode->ichMin, this->GetScanner()->IchLimTok(), true);
                        pnode->AsParseNodeSuperCall()->pnodeNewTarget = ReferenceSpecialName(wellKnownPropertyPids._newTarget, pnode->ichMin, this->GetScanner()->IchLimTok(), true);
                    }
                    else
                    {
                        pnode = CreateCallNode(knopCall, pnode, pnodeArgs);
                        Assert(pnode);
                    }

                    // Detect call to "eval" and record it on the function.
                    // Note: we used to leave it up to the byte code generator to detect eval calls
                    // at global scope, but now it relies on the flag the parser sets, so set it here.

                    if (count > 0 && this->NodeIsEvalName(pnode->AsParseNodeCall()->pnodeTarget))
                    {
                        this->MarkEvalCaller();
                        fCallIsEval = true;

                        // Eval may reference any of the special symbols so we need to push refs to them here.
                        ReferenceSpecialName(wellKnownPropertyPids._this);
                        ReferenceSpecialName(wellKnownPropertyPids._newTarget);
                        ReferenceSpecialName(wellKnownPropertyPids._super);
                        ReferenceSpecialName(wellKnownPropertyPids._superConstructor);
                        ReferenceSpecialName(wellKnownPropertyPids.arguments);
                    }

                    pnode->AsParseNodeCall()->callOfConstants = callOfConstants;
                    pnode->AsParseNodeCall()->spreadArgCount = spreadArgCount;
                    pnode->AsParseNodeCall()->isApplyCall = false;
                    pnode->AsParseNodeCall()->isEvalCall = fCallIsEval;
                    pnode->AsParseNodeCall()->hasDestructuring = m_hasDestructuringPattern;
                    Assert(!m_hasDestructuringPattern || count > 0);
                    pnode->AsParseNodeCall()->argCount = count;
                    pnode->ichLim = this->GetScanner()->IchLimTok();
                }
                else
                {
                    pnode = nullptr;
                    if (pToken->tk == tkID && pToken->pid == wellKnownPropertyPids.eval && count > 0) // Detect eval
                    {
                        this->MarkEvalCaller();

                        ReferenceSpecialName(wellKnownPropertyPids._this);
                        ReferenceSpecialName(wellKnownPropertyPids._newTarget);
                        ReferenceSpecialName(wellKnownPropertyPids._super);
                        ReferenceSpecialName(wellKnownPropertyPids._superConstructor);
                        ReferenceSpecialName(wellKnownPropertyPids.arguments);
                    }
                    pToken->tk = tkNone; // This is no longer an identifier
                }

                ChkCurTok(tkRParen, ERRnoRparen);

                if (isAsyncExpr)
                {
                    GetCurrentBlock()->blockId = saveCurrBlockId;
                    if (m_token.tk == tkDArrow)
                    {
                        // We're going to rewind and reinterpret the expression as a parameter list.
                        // Put back the original next-block-ID so the existing pid ref stacks will be correct.
                        m_nextBlockId = saveNextBlockId;
                    }
                }
            }
            if (pfCanAssign)
            {
                *pfCanAssign = fCanAssignToCallResult && 
                               (m_sourceContextInfo ?
                                !PHASE_ON_RAW(Js::EarlyErrorOnAssignToCallPhase, m_sourceContextInfo->sourceContextId, GetCurrentFunctionNode()->functionId) :
                                !PHASE_ON1(Js::EarlyErrorOnAssignToCallPhase));
            }
            if (pfIsDotOrIndex)
            {
                *pfIsDotOrIndex = false;
            }
            break;
        }
        case tkLBrack:
        {
            this->GetScanner()->Scan();
            IdentToken tok;
            ParseNodePtr pnodeExpr = ParseExpr<buildAST>(0, FALSE, TRUE, FALSE, nullptr, nullptr, nullptr, &tok);
            if (buildAST)
            {
                AnalysisAssert(pnodeExpr);
                if (pnode && pnode->nop == knopName && pnode->AsParseNodeName()->IsSpecialName() && pnode->AsParseNodeSpecialName()->isSuper)
                {
                    pnode = CreateSuperReferenceNode(knopIndex, pnode->AsParseNodeSpecialName(), pnodeExpr);
                    pnode->AsParseNodeSuperReference()->pnodeThis = ReferenceSpecialName(wellKnownPropertyPids._this, pnode->ichMin, pnode->ichLim, true);
                }
                else
                {
                    pnode = CreateBinNode(knopIndex, pnode, pnodeExpr);
                }

                AnalysisAssert(pnode);
                pnode->ichLim = this->GetScanner()->IchLimTok();
            }
            else
            {
                pnode = nullptr;
                pToken->tk = tkNone; // This is no longer an identifier
            }
            ChkCurTok(tkRBrack, ERRnoRbrack);
            if (pfCanAssign)
            {
                *pfCanAssign = TRUE;
            }
            if (pfIsDotOrIndex)
            {
                *pfIsDotOrIndex = true;
            }

            PidRefStack * topPidRef = nullptr;
            if (buildAST)
            {
                if (pnodeExpr && pnodeExpr->nop == knopName)
                {
                    topPidRef = pnodeExpr->AsParseNodeName()->pid->GetTopRef();
                }
            }
            else if (tok.tk == tkID)
            {
                topPidRef = tok.pid->GetTopRef();
            }
            if (topPidRef)
            {
                topPidRef->SetIsUsedInLdElem(true);
            }

            if (!buildAST)
            {
                break;
            }

            bool shouldConvertToDot = false;
            if (pnode->AsParseNodeBin()->pnode2->nop == knopStr)
            {
                // if the string is empty or contains escape character, we will not convert them to dot node
                shouldConvertToDot = pnode->AsParseNodeBin()->pnode2->AsParseNodeStr()->pid->Cch() > 0 && !this->GetScanner()->IsEscapeOnLastTkStrCon();
            }

            if (shouldConvertToDot)
            {
                LPCOLESTR str = pnode->AsParseNodeBin()->pnode2->AsParseNodeStr()->pid->Psz();
                // See if we can convert o["p"] into o.p and o["0"] into o[0] since they're equivalent and the latter forms
                // are faster
                uint32 uintValue;
                if (Js::JavascriptOperators::TryConvertToUInt32(
                    str,
                    pnode->AsParseNodeBin()->pnode2->AsParseNodeStr()->pid->Cch(),
                    &uintValue) &&
                    !Js::TaggedInt::IsOverflow(uintValue)) // the optimization is not very useful if the number can't be represented as a TaggedInt
                {
                    // No need to verify that uintValue != JavascriptArray::InvalidIndex since all nonnegative TaggedInts are valid indexes
                    auto intNode = CreateIntNode(uintValue); // implicit conversion from uint32 to int32
                    pnode->AsParseNodeBin()->pnode2 = intNode;
                }
                // Field optimization (see GlobOpt::KillLiveElems) checks for value being a Number,
                // and since NaN/Infinity is a number it won't kill o.NaN/o.Infinity which would cause a problem
                // if we decide to hoist o.NaN/o.Infinity.
                // We need to keep o["NaN"] and o["+/-Infinity"] as array element access (we don't hoist that but we may hoist field access),
                // so no matter if it's killed by o[x] inside a loop, we make sure that we never hoist these.
                // We need to follow same logic for strings that convert to a floating point number.
                else
                {
                    bool doConvertToProperty = false;    // Convert a["x"] -> a.x.
                    if (!Parser::IsNaNOrInfinityLiteral<true>(str))
                    {
                        const OLECHAR* terminalChar;
                        double dbl = Js::NumberUtilities::StrToDbl(str, &terminalChar, m_scriptContext);
                        bool convertsToFloat = !Js::NumberUtilities::IsNan(dbl);
                        doConvertToProperty = !convertsToFloat;
                    }

                    if (doConvertToProperty)
                    {
                        ParseNodeName * pnodeNewExpr = CreateNameNode(pnodeExpr->AsParseNodeStr()->pid);
                        pnodeNewExpr->ichMin = pnodeExpr->ichMin;
                        pnodeNewExpr->ichLim = pnodeExpr->ichLim;
                        pnode->AsParseNodeBin()->pnode2 = pnodeNewExpr;
                        pnode->nop = knopDot;
                        pnode->grfpn |= PNodeFlags::fpnIndexOperator;
                    }
                }
            }
        }
        break;

        case tkDot:
        {
            ParseNodePtr name = nullptr;
            OpCode opCode = knopDot;

            this->GetScanner()->Scan();
            if (!m_token.IsIdentifier())
            {
                //allow reserved words in ES5 mode
                if (!(m_token.IsReservedWord()))
                {
                    IdentifierExpectedError(m_token);
                }
            }
            // Note: see comment above about field optimization WRT NaN/Infinity/-Infinity.
            // Convert a.Nan, a.Infinity into a["NaN"], a["Infinity"].
            // We don't care about -Infinity case here because x.-Infinity is invalid in JavaScript.
            // Both NaN and Infinity are identifiers.
            else if (buildAST && Parser::IsNaNOrInfinityLiteral<false>(m_token.GetIdentifier(this->GetHashTbl())->Psz()))
            {
                opCode = knopIndex;
            }

            if (buildAST)
            {
                if (opCode == knopDot)
                {
                    name = CreateNameNode(m_token.GetIdentifier(this->GetHashTbl()));
                }
                else
                {
                    Assert(opCode == knopIndex);
                    name = CreateStrNode(m_token.GetIdentifier(this->GetHashTbl()));
                }
                if (pnode && pnode->nop == knopName && pnode->AsParseNodeName()->IsSpecialName() && pnode->AsParseNodeSpecialName()->isSuper)
                {
                    pnode = CreateSuperReferenceNode(opCode, pnode->AsParseNodeSpecialName(), name);
                    pnode->AsParseNodeSuperReference()->pnodeThis = ReferenceSpecialName(wellKnownPropertyPids._this, pnode->ichMin, pnode->ichLim, true);
                }
                else
                {
                    pnode = CreateBinNode(opCode, pnode, name);
                }
            }
            else
            {
                pnode = nullptr;
                pToken->tk = tkNone;
            }

            if (pfCanAssign)
            {
                *pfCanAssign = TRUE;
            }
            if (pfIsDotOrIndex)
            {
                *pfIsDotOrIndex = true;
            }
            this->GetScanner()->Scan();

            break;
        }

        case tkStrTmplBasic:
        case tkStrTmplBegin:
        {
            ParseNode* templateNode = nullptr;
            if (pnode != nullptr)
            {
                AutoMarkInParsingArgs autoMarkInParsingArgs(this);
                templateNode = ParseStringTemplateDecl<buildAST>(pnode);
            }
            else
            {
                templateNode = ParseStringTemplateDecl<buildAST>(pnode);
            }

            if (!buildAST)
            {
                pToken->tk = tkNone; // This is no longer an identifier
            }

            pnode = templateNode;
            if (pfCanAssign)
            {
                *pfCanAssign = FALSE;
            }
            if (pfIsDotOrIndex)
            {
                *pfIsDotOrIndex = false;
            }
            break;
        }
        default:
            return pnode;
        }
    }
}

/***************************************************************************
Look for an existing label with the given name.
***************************************************************************/
bool Parser::LabelExists(IdentPtr pid, LabelId* pLabelIdList)
{
    StmtNest dummy;
    dummy.pLabelId = pLabelIdList;
    dummy.pstmtOuter = m_pstmtCur;

    // Look through each label list for the current stack of statements
    for (StmtNest* pStmt = &dummy; pStmt != nullptr; pStmt = pStmt->pstmtOuter)
    {
        for (LabelId* pLabelId = pStmt->pLabelId; pLabelId != nullptr; pLabelId = pLabelId->next)
        {
            if (pLabelId->pid == pid)
                return true;
        }
    }

    return false;
}

// Currently only ints and floats are treated as constants in function call
// TODO: Check if we need for other constants as well
BOOL Parser::IsConstantInFunctionCall(ParseNodePtr pnode)
{
    if (pnode->nop == knopInt && !Js::TaggedInt::IsOverflow(pnode->AsParseNodeInt()->lw))
    {
        return TRUE;
    }

    if (pnode->nop == knopFlt)
    {
        return TRUE;
    }

    return FALSE;
}

/***************************************************************************
Parse a list of arguments.
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseArgList(bool *pCallOfConstants, uint16 *pSpreadArgCount, uint16 * pCount)
{
    ParseNodePtr pnodeArg;
    ParseNodePtr pnodeList = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;

    // Check for an empty list
    Assert(m_token.tk == tkLParen);

    if (this->GetScanner()->Scan() == tkRParen)
    {
        return nullptr;
    }

    *pCallOfConstants = true;
    *pSpreadArgCount = 0;

    int count = 0;
    while (true)
    {
        if (count >= Js::Constants::MaxAllowedArgs)
        {
            Error(ERRTooManyArgs);
        }
        // Allow spread in argument lists.
        IdentToken token;
        pnodeArg = ParseExpr<buildAST>(koplCma, nullptr, TRUE, /* fAllowEllipsis */TRUE, NULL, nullptr, nullptr, &token);
        ++count;
        this->MarkEscapingRef(pnodeArg, &token);

        if (buildAST)
        {
            this->CheckArguments(pnodeArg);

            if (*pCallOfConstants && !IsConstantInFunctionCall(pnodeArg))
            {
                *pCallOfConstants = false;
            }

            if (pnodeArg->nop == knopEllipsis)
            {
                (*pSpreadArgCount)++;
            }

            AddToNodeListEscapedUse(&pnodeList, &lastNodeRef, pnodeArg);
        }
        if (m_token.tk != tkComma)
        {
            break;
        }
        this->GetScanner()->Scan();

        if (m_token.tk == tkRParen && m_scriptContext->GetConfig()->IsES7TrailingCommaEnabled())
        {
            break;
        }
    }

    if (pSpreadArgCount != nullptr && (*pSpreadArgCount) > 0) {
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, SpreadFeature, m_scriptContext);
    }

    *pCount = static_cast<uint16>(count);
    if (buildAST)
    {
        Assert(lastNodeRef);
        Assert(*lastNodeRef);
        pnodeList->ichLim = (*lastNodeRef)->ichLim;
    }

    return pnodeList;
}

// Currently only ints are treated as constants in ArrayLiterals
BOOL Parser::IsConstantInArrayLiteral(ParseNodePtr pnode)
{
    if (pnode->nop == knopInt && !Js::TaggedInt::IsOverflow(pnode->AsParseNodeInt()->lw))
    {
        return TRUE;
    }
    return FALSE;
}

template<bool buildAST>
ParseNodeArrLit * Parser::ParseArrayLiteral()
{
    ParseNodeArrLit * pnode = nullptr;
    bool arrayOfTaggedInts = false;
    bool arrayOfInts = false;
    bool arrayOfNumbers = false;
    bool hasMissingValues = false;
    uint count = 0;
    uint spreadCount = 0;

    ParseNodePtr pnode1 = ParseArrayList<buildAST>(&arrayOfTaggedInts, &arrayOfInts, &arrayOfNumbers, &hasMissingValues, &count, &spreadCount);

    if (buildAST)
    {
        pnode = CreateNodeForOpT<knopArray>();
        pnode->pnode1 = pnode1;
        pnode->arrayOfTaggedInts = arrayOfTaggedInts;
        pnode->arrayOfInts = arrayOfInts;
        pnode->arrayOfNumbers = arrayOfNumbers;
        pnode->hasMissingValues = hasMissingValues;
        pnode->count = count;
        pnode->spreadCount = spreadCount;

        if (pnode->pnode1)
        {
            this->CheckArguments(pnode->pnode1);
        }
    }

    return pnode;
}

/***************************************************************************
Create an ArrayLiteral node
Parse a list of array elements. [ a, b, , c, ]
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseArrayList(bool *pArrayOfTaggedInts, bool *pArrayOfInts, bool *pArrayOfNumbers, bool *pHasMissingValues, uint *count, uint *spreadCount)
{
    ParseNodePtr pnodeArg = nullptr;
    ParseNodePtr pnodeList = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;

    *count = 0;

    // Check for an empty list
    if (tkRBrack == m_token.tk)
    {
        return nullptr;
    }

    this->m_arrayDepth++;
    bool arrayOfTaggedInts = buildAST;
    bool arrayOfInts = buildAST;
    bool arrayOfNumbers = buildAST;
    bool arrayOfVarInts = false;
    bool hasMissingValues = false;

    for (;;)
    {
        (*count)++;
        if (tkComma == m_token.tk || tkRBrack == m_token.tk)
        {
            hasMissingValues = true;
            arrayOfTaggedInts = false;
            arrayOfInts = false;
            arrayOfNumbers = false;
            if (buildAST)
            {
                pnodeArg = CreateNodeForOpT<knopEmpty>();
            }
        }
        else
        {
            // Allow Spread in array literals.
            pnodeArg = ParseExpr<buildAST>(koplCma, nullptr, TRUE, /* fAllowEllipsis */ TRUE);
            if (buildAST)
            {
                if (pnodeArg->nop == knopEllipsis)
                {
                    (*spreadCount)++;
                }
                this->CheckArguments(pnodeArg);
            }
        }

#if DEBUG
        if (m_grfscr & fscrEnforceJSON && !IsJSONValid(pnodeArg))
        {
            Error(ERRsyntax);
        }
#endif

        if (buildAST)
        {
            if (arrayOfNumbers)
            {
                if (pnodeArg->nop != knopInt)
                {
                    arrayOfTaggedInts = false;
                    if (pnodeArg->nop != knopFlt)
                    {
                        // Not an array of constants.
                        arrayOfInts = false;
                        arrayOfNumbers = false;
                    }
                    else if (arrayOfInts && Js::JavascriptNumber::IsInt32OrUInt32(pnodeArg->AsParseNodeFloat()->dbl) && (!Js::JavascriptNumber::IsInt32(pnodeArg->AsParseNodeFloat()->dbl) || pnodeArg->AsParseNodeFloat()->dbl == -2147483648.0))
                    {
                        // We've seen nothing but ints, and this is a uint32 but not an int32.
                        // Unless we see an actual float at some point, we want an array of vars
                        // so we can work with tagged ints.
                        arrayOfVarInts = true;
                    }
                    else
                    {
                        // Not an int array, but it may still be a float array.
                        arrayOfInts = false;
                    }
                }
                else
                {
                    if (Js::SparseArraySegment<int32>::IsMissingItem((int32*)&pnodeArg->AsParseNodeInt()->lw))
                    {
                        arrayOfInts = false;
                    }
                    if (Js::TaggedInt::IsOverflow(pnodeArg->AsParseNodeInt()->lw))
                    {
                        arrayOfTaggedInts = false;
                    }
                }
            }
            AddToNodeListEscapedUse(&pnodeList, &lastNodeRef, pnodeArg);
        }

        if (tkComma != m_token.tk)
        {
            break;
        }
        this->GetScanner()->Scan();

        if (tkRBrack == m_token.tk)
        {
            break;
        }
    }

    if (spreadCount != nullptr && *spreadCount > 0) {
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, SpreadFeature, m_scriptContext);
    }

    if (buildAST)
    {
        Assert(lastNodeRef);
        Assert(*lastNodeRef);
        pnodeList->ichLim = (*lastNodeRef)->ichLim;

        if (arrayOfVarInts && arrayOfInts)
        {
            arrayOfInts = false;
            arrayOfNumbers = false;
        }
        *pArrayOfTaggedInts = arrayOfTaggedInts;
        *pArrayOfInts = arrayOfInts;
        *pArrayOfNumbers = arrayOfNumbers;
        *pHasMissingValues = hasMissingValues;
    }
    this->m_arrayDepth--;
    return pnodeList;
}

Parser::MemberNameToTypeMap* Parser::CreateMemberNameMap(ArenaAllocator* pAllocator)
{
    Assert(pAllocator);
    return Anew(pAllocator, MemberNameToTypeMap, pAllocator, 5);
}

template<bool buildAST> void Parser::ParseComputedName(ParseNodePtr* ppnodeName, LPCOLESTR* ppNameHint, LPCOLESTR* ppFullNameHint, uint32 *pNameLength, uint32 *pShortNameOffset)
{
    this->GetScanner()->Scan();
    ParseNodePtr pnodeNameExpr = ParseExpr<buildAST>(koplCma, nullptr, TRUE, FALSE, *ppNameHint, pNameLength, pShortNameOffset);
    if (buildAST)
    {
        *ppnodeName = CreateUniNode(knopComputedName, pnodeNameExpr, pnodeNameExpr->ichMin, pnodeNameExpr->ichLim);
    }

    if (ppFullNameHint && buildAST && CONFIG_FLAG(UseFullName))
    {
        *ppFullNameHint = FormatPropertyString(*ppNameHint, pnodeNameExpr, pNameLength, pShortNameOffset);
    }

    ChkCurTokNoScan(tkRBrack, ERRnoRbrack);
}

/***************************************************************************
    Parse a list of object set/get members, e.g.:
    { get foo(){ ... }, set bar(arg) { ... } }
***************************************************************************/
template<bool buildAST>
ParseNodeBin * Parser::ParseMemberGetSet(OpCode nop, LPCOLESTR* ppNameHint)
{
    ParseNodePtr pnodeName = nullptr;
    Assert(nop == knopGetMember || nop == knopSetMember);
    Assert(ppNameHint);
    IdentPtr pid = nullptr;
    bool isComputedName = false;

    *ppNameHint = nullptr;

    switch (m_token.tk)
    {
    default:
        if (!m_token.IsReservedWord())
        {
            Error(ERRnoMemberIdent);
        }
        // fall through
    case tkID:
        pid = m_token.GetIdentifier(this->GetHashTbl());
        *ppNameHint = pid->Psz();
        if (buildAST)
        {
            pnodeName = CreateStrNode(pid);
        }
        break;
    case tkStrCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }
        pid = m_token.GetStr();
        *ppNameHint = pid->Psz();
        if (buildAST)
        {
            pnodeName = CreateStrNode(pid);
        }
        break;

    case tkIntCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        pid = this->GetScanner()->PidFromLong(m_token.GetLong());
        if (buildAST)
        {
            pnodeName = CreateStrNode(pid);
        }
        break;

    case tkFltCon:
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        pid = this->GetScanner()->PidFromDbl(m_token.GetDouble());
        if (buildAST)
        {
            pnodeName = CreateStrNode(pid);
        }
        break;

    case tkLBrack:
        // Computed property name: get|set [expr] () {  }
        if (!m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
        {
            Error(ERRnoMemberIdent);
        }
        LPCOLESTR emptyHint = nullptr;
        uint32 offset = 0;
        ParseComputedName<buildAST>(&pnodeName, &emptyHint, ppNameHint, &offset);

        isComputedName = true;
        break;
    }

    MemberType memberType;
    ushort flags = fFncMethod | fFncNoName;
    if (nop == knopGetMember)
    {
        memberType = MemberTypeGetter;
        flags |= fFncNoArg;
    }
    else
    {
        Assert(nop == knopSetMember);
        memberType = MemberTypeSetter;
        flags |= fFncOneArg;
    }

    ParseNodeFnc * pnodeFnc = ParseFncDeclNoCheckScope<buildAST>(flags, SuperRestrictionState::PropertyAllowed, *ppNameHint,
        /*needsPIDOnRCurlyScan*/ false);

    if (isComputedName)
    {
        pnodeFnc->SetHasComputedName();
    }
    pnodeFnc->SetHasHomeObj();

    if (buildAST)
    {
        pnodeFnc->SetIsAccessor();
        return CreateBinNode(nop, pnodeName, pnodeFnc);
    }
    else
    {
        return nullptr;
    }
}

/***************************************************************************
Parse a list of object members. e.g. { x:foo, 'y me':bar }
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseMemberList(LPCOLESTR pNameHint, uint32* pNameHintLength, tokens declarationType)
{
    ParseNodeBin * pnodeArg = nullptr;
    ParseNodePtr pnodeName = nullptr;
    ParseNodePtr pnodeList = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;
    LPCOLESTR pFullNameHint = nullptr;       // A calculated full name
    uint32 fullNameHintLength = pNameHintLength ? *pNameHintLength : 0;
    uint32 shortNameOffset = 0;
    bool isProtoDeclared = false;

    // we get declaration tkLCurly - when the possible object pattern found under the expression.
    bool isObjectPattern = (declarationType == tkVAR || declarationType == tkLET || declarationType == tkCONST || declarationType == tkLCurly) && IsES6DestructuringEnabled();

    // Check for an empty list
    if (tkRCurly == m_token.tk)
    {
        return nullptr;
    }

    ArenaAllocator tempAllocator(_u("MemberNames"), m_nodeAllocator.GetPageAllocator(), Parser::OutOfMemory);

    bool hasDeferredInitError = false;

    for (;;)
    {
        bool isComputedName = false;
#if DEBUG
        if ((m_grfscr & fscrEnforceJSON) && (tkStrCon != m_token.tk || !(this->GetScanner()->IsDoubleQuoteOnLastTkStrCon())))
        {
            Error(ERRsyntax);
        }
#endif
        bool isAsyncMethod = false;
        charcount_t ichMin = 0;
        size_t iecpMin = 0;
        if (m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            RestorePoint parsedAsync;
            this->GetScanner()->Capture(&parsedAsync);
            ichMin = this->GetScanner()->IchMinTok();
            iecpMin = this->GetScanner()->IecpMinTok();

            this->GetScanner()->ScanForcingPid();
            if (m_token.tk == tkLParen || m_token.tk == tkColon || m_token.tk == tkRCurly || this->GetScanner()->FHadNewLine() || m_token.tk == tkComma)
            {
                this->GetScanner()->SeekTo(parsedAsync);
            }
            else
            {
                isAsyncMethod = true;
            }
        }

        bool isGenerator = m_scriptContext->GetConfig()->IsES6GeneratorsEnabled() &&
            m_token.tk == tkStar;
        ushort fncDeclFlags = fFncNoName | fFncMethod;
        if (isGenerator)
        {
            if (isAsyncMethod)
            {
                Error(ERRsyntax);
            }

            // Include star character in the function extents
            ichMin = this->GetScanner()->IchMinTok();
            iecpMin = this->GetScanner()->IecpMinTok();

            this->GetScanner()->ScanForcingPid();
            fncDeclFlags |= fFncGenerator;
        }

        IdentPtr pidHint = nullptr;              // A name scoped to current expression
        Token tkHint = m_token;
        charcount_t idHintIchMin = static_cast<charcount_t>(this->GetScanner()->IecpMinTok());
        charcount_t idHintIchLim = static_cast<charcount_t>(this->GetScanner()->IecpLimTok());
        bool wrapInBrackets = false;
        switch (m_token.tk)
        {
        default:
            if (!m_token.IsReservedWord())
            {
                Error(ERRnoMemberIdent);
            }
            // allow reserved words
            wrapInBrackets = true;
            // fall-through
        case tkID:
            pidHint = m_token.GetIdentifier(this->GetHashTbl());
            if (buildAST)
            {
                pnodeName = CreateStrNode(pidHint);
            }
            break;

        case tkStrCon:
            if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
            {
                Error(ERRES5NoOctal);
            }
            wrapInBrackets = true;
            pidHint = m_token.GetStr();
            if (buildAST)
            {
                pnodeName = CreateStrNode(pidHint);
            }
            break;

        case tkIntCon:
            // Object initializers with numeric labels allowed in JS6
            if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
            {
                Error(ERRES5NoOctal);
            }

            pidHint = this->GetScanner()->PidFromLong(m_token.GetLong());
            if (buildAST)
            {
                pnodeName = CreateStrNode(pidHint);
            }
            break;

        case tkFltCon:
            if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
            {
                Error(ERRES5NoOctal);
            }

            pidHint = this->GetScanner()->PidFromDbl(m_token.GetDouble());
            if (buildAST)
            {
                pnodeName = CreateStrNode(pidHint);
            }
            wrapInBrackets = true;
            break;

        case tkLBrack:
            // Computed property name: [expr] : value
            if (!m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
            {
                Error(ERRnoMemberIdent);
            }

            ParseComputedName<buildAST>(&pnodeName, &pNameHint, &pFullNameHint, &fullNameHintLength, &shortNameOffset);

            isComputedName = true;
            break;
        }

        if (pFullNameHint == nullptr)
        {
            if (CONFIG_FLAG(UseFullName))
            {
                pFullNameHint = AppendNameHints(pNameHint, pidHint, &fullNameHintLength, &shortNameOffset, false, wrapInBrackets);
            }
            else
            {
                pFullNameHint = pidHint ? pidHint->Psz() : nullptr;
                fullNameHintLength = pidHint ? pidHint->Cch() : 0;
                shortNameOffset = 0;
            }
        }

        RestorePoint atPid;
        this->GetScanner()->Capture(&atPid);

        this->GetScanner()->ScanForcingPid();

        if (isGenerator && m_token.tk != tkLParen)
        {
            Error(ERRnoLparen);
        }

        if (tkColon == m_token.tk)
        {
            // It is a syntax error is the production of the form __proto__ : <> occurs more than once. From B.3.1 in spec.
            // Note that previous scan is important because only after that we can determine we have a variable.
            if (!isComputedName && pidHint == wellKnownPropertyPids.__proto__)
            {
                if (isProtoDeclared)
                {
                    Error(ERRsyntax);
                }
                else
                {
                    isProtoDeclared = true;
                }
            }

            this->GetScanner()->Scan();
            ParseNodePtr pnodeExpr = nullptr;
            if (isObjectPattern)
            {
                if (m_token.tk == tkEllipsis)
                {
                    Error(ERRUnexpectedEllipsis);
                }

                RestorePoint atExpression;
                if (!buildAST && declarationType == tkLCurly && IsPossiblePatternStart())
                {
                    this->GetScanner()->Capture(&atExpression);

                    int saveNextBlockId = m_nextBlockId;

                    // It is possible that we might encounter the shorthand init error. Lets find that out.
                    bool savedDeferredInitError = m_hasDeferredShorthandInitError;
                    m_hasDeferredShorthandInitError = false;

                    IdentToken token;
                    BOOL fLikelyPattern = false;

                    // First identify that the current expression is indeed the object/array literal. Otherwise we will just use the ParsrExpr to parse that.

                    ParseTerm<buildAST>(/* fAllowCall */ m_token.tk != tkSUPER, nullptr /*pNameHint*/, nullptr /*pHintLength*/, nullptr /*pShortNameOffset*/, &token, false /*fUnaryOrParen*/,
                        TRUE, nullptr /*pfCanAssign*/, &fLikelyPattern);

                     m_nextBlockId = saveNextBlockId;

                    this->GetScanner()->SeekTo(atExpression);

                    if (fLikelyPattern)
                    {
                        pnodeExpr = ParseDestructuredVarDecl<buildAST>(declarationType, declarationType != tkLCurly, nullptr/* *hasSeenRest*/, false /*topLevel*/, false /*allowEmptyExpression*/);
                        if (m_token.tk != tkComma && m_token.tk != tkRCurly)
                        {
                            if (m_token.IsOperator())
                            {
                                Error(ERRDestructNoOper);
                            }
                            Error(ERRsyntax);
                        }
                    }
                    else
                    {
                        if (m_hasDeferredShorthandInitError)
                        {
                            Error(ERRnoColon);
                        }

                        pnodeExpr = ParseExpr<buildAST>(koplCma, nullptr/*pfCantAssign*/, TRUE/*fAllowIn*/, FALSE/*fAllowEllipsis*/, pFullNameHint, &fullNameHintLength, &shortNameOffset);
                    }

                    m_hasDeferredShorthandInitError = savedDeferredInitError;
                }
                else
                {
                    pnodeExpr = ParseDestructuredVarDecl<buildAST>(declarationType, declarationType != tkLCurly, nullptr/* *hasSeenRest*/, false /*topLevel*/, false /*allowEmptyExpression*/);
                    if (m_token.tk != tkComma && m_token.tk != tkRCurly)
                    {
                        if (m_token.IsOperator())
                        {
                            Error(ERRDestructNoOper);
                        }
                        Error(ERRsyntax);
                    }
                }
            }
            else
            {
                pnodeExpr = ParseExpr<buildAST>(koplCma, nullptr/*pfCantAssign*/, TRUE/*fAllowIn*/, FALSE/*fAllowEllipsis*/, pFullNameHint, &fullNameHintLength, &shortNameOffset);

                if (pnodeExpr && pnodeExpr->nop == knopFncDecl)
                {
                    ParseNodeFnc* funcNode = pnodeExpr->AsParseNodeFnc();
                    if (isComputedName)
                    {
                        funcNode->SetHasComputedName();
                    }
                    funcNode->SetHasHomeObj();
                }
            }
#if DEBUG
            if ((m_grfscr & fscrEnforceJSON) && !IsJSONValid(pnodeExpr))
            {
                Error(ERRsyntax);
            }
#endif
            if (buildAST)
            {
                pnodeArg = CreateBinNode(isObjectPattern ? knopObjectPatternMember : knopMember, pnodeName, pnodeExpr);
                if (pnodeArg->pnode1->nop == knopStr)
                {
                    pnodeArg->pnode1->AsParseNodeStr()->pid->PromoteAssignmentState();
                }
            }
        }
        else if (m_token.tk == tkLParen && m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
        {
            if (isObjectPattern)
            {
                Error(ERRInvalidAssignmentTarget);
            }
            // Shorthand syntax: foo() {} -> foo: function() {}

            // Rewind to the PID and parse a function expression.
            this->GetScanner()->SeekTo(atPid);

            ParseNodeFnc * pnodeFnc = ParseFncDeclNoCheckScope<buildAST>(fncDeclFlags | (isAsyncMethod ? fFncAsync : fFncNoFlgs), SuperRestrictionState::PropertyAllowed, pFullNameHint,
                /*needsPIDOnRCurlyScan*/ false);

            if (isAsyncMethod || isGenerator)
            {
                pnodeFnc->cbMin = iecpMin;
                pnodeFnc->ichMin = ichMin;
            }

            if (isComputedName)
            {
                pnodeFnc->SetHasComputedName();
            }
            pnodeFnc->SetHasHomeObj();

            if (buildAST)
            {
                pnodeArg = CreateBinNode(knopMember, pnodeName, pnodeFnc);
            }
        }
        else if (nullptr != pidHint) //Its either tkID/tkStrCon/tkFloatCon/tkIntCon
        {
            Assert(pidHint->Psz() != nullptr);

            if ((pidHint == wellKnownPropertyPids.get || pidHint == wellKnownPropertyPids.set) &&
                // get/set are only pseudo keywords when they are identifiers (i.e. not strings)
                tkHint.tk == tkID && NextTokenIsPropertyNameStart())
            {
                if (isObjectPattern)
                {
                    Error(ERRInvalidAssignmentTarget);
                }

                LPCOLESTR pNameGetOrSet = nullptr;
                OpCode op = pidHint == wellKnownPropertyPids.get ? knopGetMember : knopSetMember;

                pnodeArg = ParseMemberGetSet<buildAST>(op, &pNameGetOrSet);

                if (CONFIG_FLAG(UseFullName) && buildAST && pnodeArg->pnode2->nop == knopFncDecl)
                {
                    // displays as "get object.funcname" or "set object.funcname"
                    uint32 getOrSetOffset = 0;
                    LPCOLESTR intermediateHint = AppendNameHints(pNameHint, pNameGetOrSet, &fullNameHintLength, &shortNameOffset);
                    pFullNameHint = AppendNameHints(pidHint, intermediateHint, &fullNameHintLength, &getOrSetOffset, true);
                    shortNameOffset += getOrSetOffset;
                }
            }
            else if ((m_token.tk == tkRCurly || m_token.tk == tkComma || m_token.tk == tkAsg) && m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
            {
                // Shorthand {foo} -> {foo:foo} syntax.
                // {foo = <initializer>} supported only when on object pattern rules are being applied
                if (tkHint.tk != tkID)
                {
                    Assert(tkHint.IsReservedWord()
                        || tkHint.tk == tkIntCon || tkHint.tk == tkFltCon || tkHint.tk == tkStrCon);
                    // All keywords are banned in non-strict mode.
                    // Future reserved words are banned in strict mode.
                    if (IsStrictMode() || !tkHint.IsFutureReservedWord(true))
                    {
                        IdentifierExpectedError(tkHint);
                    }
                }

                CheckArgumentsUse(pidHint, GetCurrentFunctionNode());

                bool couldBeObjectPattern = !isObjectPattern && m_token.tk == tkAsg;
                // Saving the current state as we may change the isObjectPattern down below.
                bool oldState = isObjectPattern;

                if (couldBeObjectPattern)
                {
                    declarationType = tkLCurly;
                    isObjectPattern = true;

                    // This may be an error but we are deferring for favouring destructuring.
                    hasDeferredInitError = true;
                }

                ParseNodePtr pnodeIdent = nullptr;
                if (isObjectPattern)
                {
                    this->GetScanner()->SeekTo(atPid);
                    pnodeIdent = ParseDestructuredVarDecl<buildAST>(declarationType, declarationType != tkLCurly, nullptr/* *hasSeenRest*/, false /*topLevel*/, false /*allowEmptyExpression*/);

                    if (m_token.tk != tkComma && m_token.tk != tkRCurly)
                    {
                        if (m_token.IsOperator())
                        {
                            Error(ERRDestructNoOper);
                        }
                        Error(ERRsyntax);
                    }
                }
                else
                {
                    // Add a reference to the hinted name so we can bind it properly.
                    PidRefStack *ref = PushPidRef(pidHint);

                    if (buildAST)
                    {
                        pnodeIdent = CreateNameNode(pidHint, ref, idHintIchMin, idHintIchLim);
                    }
                }

                if (buildAST)
                {
                    pnodeArg = CreateBinNode(isObjectPattern && !couldBeObjectPattern ? knopObjectPatternMember : knopMemberShort, pnodeName, pnodeIdent);
                }

                isObjectPattern = oldState;
            }
            else
            {
                Error(ERRnoColon);
            }
        }
        else
        {
            Error(ERRnoColon);
        }

        if (buildAST)
        {
            Assert(pnodeArg->pnode2 != nullptr);
            if (pnodeArg->pnode2->nop == knopFncDecl)
            {
                Assert(fullNameHintLength >= shortNameOffset);
                ParseNodeFnc * pnodeFunc = pnodeArg->pnode2->AsParseNodeFnc();
                pnodeFunc->hint = pFullNameHint;
                pnodeFunc->hintLength = fullNameHintLength;
                pnodeFunc->hintOffset = shortNameOffset;
            }
            AddToNodeListEscapedUse(&pnodeList, &lastNodeRef, pnodeArg);
        }
        pidHint = nullptr;
        pFullNameHint = nullptr;
        if (tkComma != m_token.tk)
        {
            break;
        }
        this->GetScanner()->ScanForcingPid();
        if (tkRCurly == m_token.tk)
        {
            break;
        }
    }

    m_hasDeferredShorthandInitError = m_hasDeferredShorthandInitError || hasDeferredInitError;

    if (buildAST)
    {
        Assert(lastNodeRef);
        Assert(*lastNodeRef);
        pnodeList->ichLim = (*lastNodeRef)->ichLim;
    }

    return pnodeList;
}

BOOL Parser::WillDeferParse(Js::LocalFunctionId functionId)
{
    if ((m_grfscr & fscrWillDeferFncParse) != 0)
    {
        if (m_stoppedDeferredParse)
        {
            return false;
        }
        if (!PHASE_ENABLED_RAW(DeferParsePhase, m_sourceContextInfo->sourceContextId, functionId))
        {
            return false;
        }
        if (PHASE_FORCE_RAW(Js::DeferParsePhase, m_sourceContextInfo->sourceContextId, functionId)
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
            || Js::Configuration::Global.flags.IsEnabled(Js::ForceUndoDeferFlag)
#endif
            )
        {
            return true;
        }
#if ENABLE_PROFILE_INFO
#ifndef DISABLE_DYNAMIC_PROFILE_DEFER_PARSE
        if (m_sourceContextInfo->sourceDynamicProfileManager != nullptr)
        {
            Js::ExecutionFlags flags = m_sourceContextInfo->sourceDynamicProfileManager->IsFunctionExecuted(functionId);
            return flags != Js::ExecutionFlags_Executed;
        }
#endif
#endif
        return true;
    }

    return false;
}

//
// Call this in ParseFncDecl only to check (and reset) if ParseFncDecl is re-parsing a deferred
// function body. If a deferred function is called and being re-parsed, it shouldn't be deferred again.
//
BOOL Parser::IsDeferredFnc()
{
    if (m_grfscr & fscrDeferredFnc)
    {
        m_grfscr &= ~fscrDeferredFnc;
        return true;
    }

    return false;
}

template<bool buildAST>
ParseNode * Parser::ParseFncDeclCheckScope(ushort flags, bool fAllowIn)
{
    ParseNodeBlock * pnodeFncBlockScope = nullptr;
    ParseNodePtr *ppnodeScopeSave = nullptr;
    ParseNodePtr *ppnodeExprScopeSave = nullptr;
    bool fDeclaration = flags & fFncDeclaration;
    bool noStmtContext = false;
    if (fDeclaration)
    {
        noStmtContext = m_pstmtCur->GetNop() != knopBlock;

        if (noStmtContext)
        {
            // We have a function declaration like "if (a) function f() {}". We didn't see
            // a block scope on the way in, so we need to pretend we did. Note that this is a syntax error
            // in strict mode.
            if (!this->FncDeclAllowedWithoutContext(flags))
            {
                Error(ERRsyntax);
            }
            pnodeFncBlockScope = StartParseBlock<buildAST>(PnodeBlockType::Regular, ScopeType_Block);
            if (buildAST)
            {
                PushFuncBlockScope(pnodeFncBlockScope, &ppnodeScopeSave, &ppnodeExprScopeSave);
            }
        }
    }

    ParseNodeFnc * pnodeFnc = ParseFncDeclInternal<buildAST>(flags, nullptr, /* needsPIDOnRCurlyScan */ false, /* fUnaryOrParen */ false, noStmtContext, SuperRestrictionState::Disallowed, fAllowIn);

    if (pnodeFncBlockScope)
    {
        Assert(pnodeFncBlockScope->pnodeStmt == nullptr);
        pnodeFncBlockScope->pnodeStmt = pnodeFnc;
        if (buildAST)
        {
            PopFuncBlockScope(ppnodeScopeSave, ppnodeExprScopeSave);
        }
        FinishParseBlock(pnodeFncBlockScope);
        return pnodeFncBlockScope;
    }

    return pnodeFnc;

}

template<bool buildAST>
ParseNodeFnc * Parser::ParseFncDeclNoCheckScope(ushort flags, SuperRestrictionState::State superRestrictionState, LPCOLESTR pNameHint, const bool needsPIDOnRCurlyScan, bool fUnaryOrParen, bool fAllowIn)
{
    Assert((flags & fFncDeclaration) == 0);
    return ParseFncDeclInternal<buildAST>(flags, pNameHint, needsPIDOnRCurlyScan, fUnaryOrParen, /* noStmtContext */ false, superRestrictionState, fAllowIn);
}

template<bool buildAST>
ParseNodeFnc * Parser::ParseFncDeclInternal(ushort flags, LPCOLESTR pNameHint, const bool needsPIDOnRCurlyScan, bool fUnaryOrParen, bool noStmtContext, SuperRestrictionState::State superRestrictionState, bool fAllowIn)
{
    ParseNodeFnc * pnodeFnc = nullptr;
    ParseNodePtr *ppnodeVarSave = nullptr;

    bool fDeclaration = flags & fFncDeclaration;
    bool fModule = (flags & fFncModule) != 0;
    bool fLambda = (flags & fFncLambda) != 0;
    charcount_t ichMin = this->GetScanner()->IchMinTok();
    bool wasInDeferredNestedFunc = false;

    uint tryCatchOrFinallyDepthSave = this->m_tryCatchOrFinallyDepth;
    this->m_tryCatchOrFinallyDepth = 0;

    if (this->m_arrayDepth)
    {
        this->m_funcInArrayDepth++; // Count function depth within array literal
    }

    // Update the count of functions nested in the current parent.
    Assert(m_pnestedCount || !buildAST);
    uint *pnestedCountSave = m_pnestedCount;
    if (buildAST || m_pnestedCount)
    {
        (*m_pnestedCount)++;
    }

    uint scopeCountNoAstSave = m_scopeCountNoAst;
    m_scopeCountNoAst = 0;

    // Create the node.
    pnodeFnc = CreateAllowDeferNodeForOpT<knopFncDecl>();
    pnodeFnc->SetDeclaration(fDeclaration);

    pnodeFnc->nestedFuncEscapes = false;
    pnodeFnc->cbMin = this->GetScanner()->IecpMinTok();
    pnodeFnc->functionId = (*m_nextFunctionId)++;
    pnodeFnc->superRestrictionState = superRestrictionState;


    // Push new parser state with this new function node
    AppendFunctionToScopeList(fDeclaration, pnodeFnc);

    // Start the argument list.
    ppnodeVarSave = m_ppnodeVar;

    if (buildAST)
    {
        pnodeFnc->lineNumber = this->GetScanner()->LineCur();
        pnodeFnc->columnNumber = CalculateFunctionColumnNumber();
        pnodeFnc->SetNested(m_currentNodeFunc != nullptr); // If there is a current function, then we're a nested function.
        pnodeFnc->SetStrictMode(IsStrictMode()); // Inherit current strict mode -- may be overridden by the function itself if it contains a strict mode directive.

        m_pCurrentAstSize = &pnodeFnc->astSize;
    }
    else // if !buildAST
    {
        wasInDeferredNestedFunc = m_inDeferredNestedFunc;
        m_inDeferredNestedFunc = true;
    }

    m_pnestedCount = &pnodeFnc->nestedCount;

    AnalysisAssert(pnodeFnc);
    pnodeFnc->SetIsAsync((flags & fFncAsync) != 0);
    pnodeFnc->SetIsLambda(fLambda);
    pnodeFnc->SetIsMethod((flags & fFncMethod) != 0);
    pnodeFnc->SetIsClassMember((flags & fFncClassMember) != 0);
    pnodeFnc->SetIsModule(fModule);
    pnodeFnc->SetIsClassConstructor((flags & fFncClassConstructor) != 0);
    pnodeFnc->SetIsBaseClassConstructor((flags & fFncBaseClassConstructor) != 0);
    pnodeFnc->SetHomeObjLocation(Js::Constants::NoRegister);

    IdentPtr pFncNamePid = nullptr;
    bool needScanRCurly = true;
    ParseFncDeclHelper<buildAST>(pnodeFnc, pNameHint, flags, fUnaryOrParen, noStmtContext, &needScanRCurly, fModule, &pFncNamePid, fAllowIn);
    AddNestedCapturedNames(pnodeFnc);
  
    AnalysisAssert(pnodeFnc);

    *m_ppnodeVar = nullptr;
    m_ppnodeVar = ppnodeVarSave;

    if (m_currentNodeFunc && (pnodeFnc->CallsEval() || pnodeFnc->ChildCallsEval()))
    {
        GetCurrentFunctionNode()->SetChildCallsEval(true);
    }

    // Lambdas do not have "arguments" and instead capture their parent's
    // binding of "arguments.  To ensure the arguments object of the enclosing
    // non-lambda function is loaded propagate the UsesArguments flag up to
    // the parent function
    if (fLambda && (pnodeFnc->UsesArguments() || pnodeFnc->CallsEval()))
    {
        ParseNodeFnc * pnodeFncParent = GetCurrentFunctionNode();

        if (pnodeFncParent != nullptr)
        {
            pnodeFncParent->SetUsesArguments();
        }
        else
        {
            m_UsesArgumentsAtGlobal = true;
        }
    }

    if (needScanRCurly && !fModule)
    {
        // Consume the next token now that we're back in the enclosing function (whose strictness may be
        // different from the function we just finished).
#if DBG
        bool expectedTokenValid = m_token.tk == tkRCurly;
        AssertMsg(expectedTokenValid, "Invalid token expected for RCurly match");
#endif
        // The next token may need to have a PID created in !buildAST mode, as we may be parsing a method with a string name.
        if (needsPIDOnRCurlyScan)
        {
            this->GetScanner()->ScanForcingPid();
        }
        else
        {
            this->GetScanner()->Scan();
        }
    }

    m_pnestedCount = pnestedCountSave;
    Assert(!buildAST || !wasInDeferredNestedFunc);
    m_inDeferredNestedFunc = wasInDeferredNestedFunc;

    if (this->m_arrayDepth)
    {
        this->m_funcInArrayDepth--;
        if (this->m_funcInArrayDepth == 0)
        {
            // We disable deferred parsing if array literals dominate.
            // But don't do this if the array literal is dominated by function bodies.
            if (flags & (fFncMethod | fFncClassMember) && m_token.tk != tkSColon)
            {
                // Class member methods have optional separators. We need to check whether we are
                // getting the IchLim of the correct token.
                Assert(this->GetScanner()->m_tkPrevious == tkRCurly && needScanRCurly);

                this->m_funcInArray += this->GetScanner()->IchMinTok() - /*tkRCurly*/ 1 - ichMin;
            }
            else
            {
                this->m_funcInArray += this->GetScanner()->IchLimTok() - ichMin;
            }
        }
    }

    m_scopeCountNoAst = scopeCountNoAstSave;

    if (fDeclaration && !IsStrictMode())
    {
        if (pFncNamePid != nullptr &&
            GetCurrentBlock() &&
            GetCurrentBlock()->blockType == PnodeBlockType::Regular)
        {
            // Add a function-scoped VarDecl with the same name as the function for
            // back compat with pre-ES6 code that declares functions in blocks. The
            // idea is that the last executed declaration wins at the function scope
            // level and we accomplish this by having each block scoped function
            // declaration assign to both the block scoped "let" binding, as well
            // as the function scoped "var" binding.
            ParseNodeVar * vardecl = CreateVarDeclNode(pFncNamePid, STVariable, false, nullptr, false);
            vardecl->isBlockScopeFncDeclVar = true;
            if (GetCurrentFunctionNode() && vardecl->sym->GetIsFormal())
            {
                GetCurrentFunctionNode()->SetHasAnyWriteToFormals(true);
            }
        }
    }

    if (buildAST && fDeclaration)
    {
        Symbol* funcSym = pnodeFnc->GetFuncSymbol();
        if (funcSym->GetIsFormal())
        {
            GetCurrentFunctionNode()->SetHasAnyWriteToFormals(true);
        }
    }    this->m_tryCatchOrFinallyDepth = tryCatchOrFinallyDepthSave;

    return pnodeFnc;
}

bool Parser::FncDeclAllowedWithoutContext(ushort flags)
{
    // Statement context required for strict mode, async functions, and generators.
    // Note that generators aren't detected yet when this method is called; they're checked elsewhere.
    return !IsStrictMode() && !(flags & fFncAsync);
}

uint Parser::CalculateFunctionColumnNumber()
{
    uint columnNumber;

    charcount_t ichMinTok = this->GetScanner()->IchMinTok();
    charcount_t ichMinLine = this->GetScanner()->IchMinLine();
    if (ichMinTok >= ichMinLine)
    {
        // In scenarios involving defer parse IchMinLine() can be incorrect for the first line after defer parse
        columnNumber = ichMinTok - ichMinLine;
        if (m_functionBody != nullptr && m_functionBody->GetRelativeLineNumber() == this->GetScanner()->LineCur())
        {
            // Adjust the column if it falls on the first line, where the re-parse is happening.
            columnNumber += m_functionBody->GetRelativeColumnNumber();
        }
    }
    else if (m_currentNodeFunc)
    {
        // For the first line after defer parse, compute the column relative to the column number
        // of the lexically parent function.
        ULONG offsetFromCurrentFunction = ichMinTok - m_currentNodeFunc->ichMin;
        columnNumber = m_currentNodeFunc->columnNumber + offsetFromCurrentFunction;
    }
    else
    {
        // if there is no current function, lets give a default of 0.
        columnNumber = 0;
    }

    return columnNumber;
}

void Parser::AppendFunctionToScopeList(bool fDeclaration, ParseNodeFnc * pnodeFnc)
{
    if (!fDeclaration && m_ppnodeExprScope)
    {
        // We're tracking function expressions separately from declarations in this scope
        // (e.g., inside a catch scope in standards mode).
        Assert(*m_ppnodeExprScope == nullptr);
        *m_ppnodeExprScope = pnodeFnc;
        m_ppnodeExprScope = &pnodeFnc->pnodeNext;
    }
    else
    {
        Assert(*m_ppnodeScope == nullptr);
        *m_ppnodeScope = pnodeFnc;
        m_ppnodeScope = &pnodeFnc->pnodeNext;
    }
}

/***************************************************************************
Parse a function definition.
***************************************************************************/
template<bool buildAST>
void Parser::ParseFncDeclHelper(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint, ushort flags, bool fUnaryOrParen, bool noStmtContext, bool *pNeedScanRCurly, bool skipFormals, IdentPtr* pFncNamePid, bool fAllowIn)
{
    Assert(pnodeFnc);
    ParseNodeFnc * pnodeFncParent = GetCurrentFunctionNode();
    // is the following correct? When buildAST is false, m_currentNodeDeferredFunc can be nullptr on transition to deferred parse from non-deferred
    ParseNodeFnc * pnodeFncSave = buildAST ? m_currentNodeFunc : m_currentNodeDeferredFunc;
    ParseNodeFnc * pnodeFncSaveNonLambda = buildAST ? m_currentNodeNonLambdaFunc : m_currentNodeNonLambdaDeferredFunc;
    int32* pAstSizeSave = m_pCurrentAstSize;

    bool fDeclaration = (flags & fFncDeclaration) != 0;
    bool fLambda = (flags & fFncLambda) != 0;
    bool fAsync = (flags & fFncAsync) != 0;
    bool fModule = (flags & fFncModule) != 0;
    bool fDeferred = false;
    StmtNest *pstmtSave;
    bool fFunctionInBlock = false;

    if (buildAST)
    {
        fFunctionInBlock = GetCurrentBlockInfo() != GetCurrentFunctionBlockInfo() &&
            (GetCurrentBlockInfo()->pnodeBlock->scope == nullptr ||
                GetCurrentBlockInfo()->pnodeBlock->scope->GetScopeType() != ScopeType_GlobalEvalBlock);
    }

    // Save the position of the scanner in case we need to inspect the name hint later
    RestorePoint beginNameHint;
    this->GetScanner()->Capture(&beginNameHint);

    ParseNodeBlock * pnodeFncExprScope = nullptr;
    Scope *fncExprScope = nullptr;
    if (!fDeclaration)
    {
        if (!fLambda)
        {
            pnodeFncExprScope = StartParseBlock<buildAST>(PnodeBlockType::Function, ScopeType_FuncExpr);
            fncExprScope = pnodeFncExprScope->scope;
        }

        // Function expression: push the new function onto the stack now so that the name (if any) will be
        // local to the new function.

        this->UpdateCurrentNodeFunc<buildAST>(pnodeFnc, fLambda);
    }

    if (!fLambda && !fModule)
    {
        this->ParseFncName<buildAST>(pnodeFnc, flags, pFncNamePid);
    }

    if (fDeclaration)
    {
        // Declaration statement: push the new function now, after parsing the name, so the name is local to the
        // enclosing function.

        this->UpdateCurrentNodeFunc<buildAST>(pnodeFnc, fLambda);
    }

    if (noStmtContext && pnodeFnc->IsGenerator())
    {
        // Generator decl not allowed outside stmt context. (We have to wait until we've parsed the '*' to
        // detect generator.)
        Error(ERRsyntax, pnodeFnc);
    }

    // switch scanner to treat 'yield' as keyword in generator functions
    // or as an identifier in non-generator functions
    bool fPreviousYieldIsKeyword = this->GetScanner()->SetYieldIsKeywordRegion(pnodeFnc->IsGenerator());
    bool fPreviousAwaitIsKeyword = this->GetScanner()->SetAwaitIsKeywordRegion(fAsync);

    if (pnodeFnc->IsGenerator())
    {
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Generator, m_scriptContext);
    }

    if (fncExprScope && pnodeFnc->pnodeName == nullptr)
    {
        FinishParseBlock(pnodeFncExprScope);
        m_nextBlockId--;
        Adelete(&m_nodeAllocator, fncExprScope);
        fncExprScope = nullptr;
        pnodeFncExprScope = nullptr;
    }

    pnodeFnc->scope = fncExprScope;

    // Start a new statement stack.
    bool topLevelStmt =
        buildAST &&
        !fFunctionInBlock &&
        (this->m_pstmtCur == nullptr || this->m_pstmtCur->pnodeStmt->nop == knopBlock);

    pstmtSave = m_pstmtCur;
    SetCurrentStatement(nullptr);

    RestorePoint beginFormals;
    this->GetScanner()->Capture(&beginFormals);
    BOOL fWasAlreadyStrictMode = IsStrictMode();
    BOOL oldStrictMode = this->m_fUseStrictMode;

    if (fLambda)
    {
        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Lambda, m_scriptContext);
    }

    uint uCanDeferSave = m_grfscr & fscrCanDeferFncParse;
    uint uDeferSave = m_grfscr & fscrWillDeferFncParse;
    if (flags & fFncClassMember)
    {
        // Disable deferral on class members or other construct with unusual text bounds
        // as these are usually trivial, and re-parsing is problematic.
        // NOTE: It is probably worth supporting these cases for memory and load-time purposes,
        // especially as they become more and more common.
        m_grfscr &= ~(fscrCanDeferFncParse | fscrWillDeferFncParse);
    }

    bool isTopLevelDeferredFunc = false;

#if ENABLE_BACKGROUND_PARSING
    struct AutoFastScanFlag {
        bool savedDoingFastScan;
        AutoFastScanFlag(Parser *parser) : m_parser(parser) { savedDoingFastScan = m_parser->m_doingFastScan; }
        ~AutoFastScanFlag() { m_parser->m_doingFastScan = savedDoingFastScan; }
        Parser *m_parser;
    } flag(this);
#endif

    bool doParallel = false;
#if ENABLE_BACKGROUND_PARSING
    bool parallelJobStarted = false;
#endif
    if (buildAST)
    {
        bool isLikelyIIFE = !fDeclaration && fUnaryOrParen;

        BOOL isDeferredFnc = IsDeferredFnc();
        // These are the conditions that prohibit upfront deferral *and* redeferral.
        isTopLevelDeferredFunc = 
                (m_grfscr & fscrCanDeferFncParse)
                && !m_InAsmMode
                // Don't defer a module function wrapper because we need to do export resolution at parse time
                && !fModule;

        pnodeFnc->SetCanBeDeferred(isTopLevelDeferredFunc && ParseNodeFnc::CanBeRedeferred(pnodeFnc->fncFlags));

        // These are heuristic conditions that prohibit upfront deferral but not redeferral.
        isTopLevelDeferredFunc = isTopLevelDeferredFunc && !isDeferredFnc && WillDeferParse(pnodeFnc->functionId) &&
            (!isLikelyIIFE || !topLevelStmt || PHASE_FORCE_RAW(Js::DeferParsePhase, m_sourceContextInfo->sourceContextId, pnodeFnc->functionId));

#if ENABLE_BACKGROUND_PARSING
        if (!fLambda &&
            !isDeferredFnc &&
            !isLikelyIIFE &&
            !this->IsBackgroundParser() &&
            !this->m_doingFastScan &&
            !(pnodeFncSave && m_currDeferredStub) &&
            !(this->m_parseType == ParseType_Deferred && this->m_functionBody && this->m_functionBody->GetScopeInfo() && !isTopLevelDeferredFunc))
        {
            doParallel = DoParallelParse(pnodeFnc);

            if (doParallel)
            {
                BackgroundParser *bgp = m_scriptContext->GetBackgroundParser();
                Assert(bgp);
                if (bgp->HasFailedBackgroundParseItem())
                {
                    Error(ERRsyntax);
                }
                doParallel = bgp->ParseBackgroundItem(this, pnodeFnc, isTopLevelDeferredFunc);
                if (doParallel)
                {
                    parallelJobStarted = true;
                    this->m_hasParallelJob = true;
                    this->m_doingFastScan = true;
                    doParallel = FastScanFormalsAndBody();
                    if (doParallel)
                    {
                        // Let the foreground thread take care of marking the limit on the function node,
                        // because in some cases this function's caller will want to change that limit,
                        // so we don't want the background thread to try and touch it.
                        pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
                        pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
                    }
                }
            }
        }
#endif
    }

    if (!doParallel)
    {
#if ENABLE_BACKGROUND_PARSING
        // We don't want to, or couldn't, let the main thread scan past this function body, so parse
        // it for real.
        ParseNodeFnc * pnodeRealFnc = pnodeFnc;
        if (parallelJobStarted)
        {
            // We have to deal with a failure to fast-scan the function (due to syntax error? "/"?) when
            // a background thread may already have begun to work on the job. Both threads can't be allowed to
            // operate on the same node.
            pnodeFnc = CreateDummyFuncNode(fDeclaration);
        }
#endif

        AnalysisAssert(pnodeFnc);
        ParseNodeBlock * pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Parameter, ScopeType_Parameter);
        AnalysisAssert(pnodeBlock != nullptr);
        pnodeFnc->pnodeScopes = pnodeBlock;
        m_ppnodeVar = &pnodeFnc->pnodeParams;
        pnodeFnc->pnodeVars = nullptr;
        ParseNodePtr* varNodesList = &pnodeFnc->pnodeVars;
        ParseNodeVar * argNode = nullptr;

        if (!fModule && !fLambda)
        {
            ParseNodePtr *const ppnodeVarSave = m_ppnodeVar;
            m_ppnodeVar = &pnodeFnc->pnodeVars;

            // Create the built-in arguments symbol
            argNode = this->AddArgumentsNodeToVars(pnodeFnc);

            // Save the updated var list
            varNodesList = m_ppnodeVar;
            m_ppnodeVar = ppnodeVarSave;
        }

        ParseNodePtr *ppnodeScopeSave = nullptr;
        ParseNodePtr *ppnodeExprScopeSave = nullptr;

        ppnodeScopeSave = m_ppnodeScope;
        if (pnodeBlock)
        {
            // This synthetic block scope will contain all the nested scopes.
            m_ppnodeScope = &pnodeBlock->pnodeScopes;
            pnodeBlock->pnodeStmt = pnodeFnc;
        }

        // Keep nested function declarations and expressions in the same list at function scope.
        // (Indicate this by nulling out the current function expressions list.)
        ppnodeExprScopeSave = m_ppnodeExprScope;
        m_ppnodeExprScope = nullptr;

        uint parenExprDepthSave = m_funcParenExprDepth;
        m_funcParenExprDepth = 0;

        if (!skipFormals)
        {
            bool fLambdaParamsSave = m_reparsingLambdaParams;
            if (fLambda)
            {
                m_reparsingLambdaParams = true;
            }

            DeferredFunctionStub* savedDeferredStub = m_currDeferredStub;
            m_currDeferredStub = nullptr;
            this->ParseFncFormals<buildAST>(pnodeFnc, pnodeFncParent, flags, isTopLevelDeferredFunc);
            m_currDeferredStub = savedDeferredStub;

            m_reparsingLambdaParams = fLambdaParamsSave;
        }

        // Create function body scope
        ParseNodeBlock * pnodeInnerBlock = StartParseBlock<buildAST>(PnodeBlockType::Function, ScopeType_FunctionBody);
        // Set the parameter block's child to the function body block.
        // The pnodeFnc->pnodeScopes list is constructed in such a way that it includes all the scopes in this list.
        // For example if the param scope has one function and body scope has one function then the list will look like below,
        // param scope block -> function decl from param scope -> body socpe block -> function decl from body scope.
        *m_ppnodeScope = pnodeInnerBlock;
        pnodeFnc->pnodeBodyScope = pnodeInnerBlock;

        // This synthetic block scope will contain all the nested scopes.
        m_ppnodeScope = &pnodeInnerBlock->pnodeScopes;
        pnodeInnerBlock->pnodeStmt = pnodeFnc;

        // DEFER: Begin deferral here (after names are parsed and name nodes created).
        // Create no more AST nodes until we're done.

        // Try to defer this func if all these are true:
        //  0. We are not already in deferred parsing (i.e. buildAST is true)
        //  1. We are not re-parsing a deferred func which is being invoked.
        //  2. Dynamic profile suggests this func can be deferred (and deferred parse is on).
        //  3. This func is top level or defer nested func is on.
        //  4. Optionally, the function is non-nested and not in eval, or the deferral decision was based on cached profile info,
        //     or the function is sufficiently long. (I.e., don't defer little nested functions unless we're
        //     confident they'll never be executed, because un-deferring nested functions is more expensive.)
        //     NOTE: I'm disabling #4 by default, because we've found other ways to reduce the cost of un-deferral,
        //           and we don't want to create function bodies aggressively for little functions.

        // We will also temporarily defer all asm.js functions, except for the asm.js
        // module itself, which we will never defer
        bool strictModeTurnedOn = false;

        if (isTopLevelDeferredFunc &&
            !(this->m_grfscr & fscrEvalCode) &&
            pnodeFnc->IsNested() &&
#ifndef DISABLE_DYNAMIC_PROFILE_DEFER_PARSE
            m_sourceContextInfo->sourceDynamicProfileManager == nullptr &&
#endif
            PHASE_ON_RAW(Js::ScanAheadPhase, m_sourceContextInfo->sourceContextId, pnodeFnc->functionId) &&
            (
                !PHASE_FORCE_RAW(Js::DeferParsePhase, m_sourceContextInfo->sourceContextId, pnodeFnc->functionId) ||
                PHASE_FORCE_RAW(Js::ScanAheadPhase, m_sourceContextInfo->sourceContextId, pnodeFnc->functionId)
                ))
        {
            // Try to scan ahead to the end of the function. If we get there before we've scanned a minimum
            // number of tokens, don't bother deferring, because it's too small.
            if (this->ScanAheadToFunctionEnd(CONFIG_FLAG(MinDeferredFuncTokenCount)))
            {
                isTopLevelDeferredFunc = false;
            }
        }

        Scope* paramScope = pnodeFnc->pnodeScopes ? pnodeFnc->pnodeScopes->scope : nullptr;
        if (paramScope != nullptr)
        {
            if (CONFIG_FLAG(ForceSplitScope))
            {
                pnodeFnc->ResetBodyAndParamScopeMerged();
            }
            else if (pnodeFnc->HasNonSimpleParameterList() && pnodeFnc->IsBodyAndParamScopeMerged())
            {
                paramScope->ForEachSymbolUntil([this, paramScope, pnodeFnc](Symbol* sym) {
                    if (sym->GetPid()->GetTopRef()->GetFuncScopeId() > pnodeFnc->functionId)
                    {
                        // One of the symbol has non local reference. Mark the param scope as we can't merge it with body scope.
                        pnodeFnc->ResetBodyAndParamScopeMerged();
                        return true;
                    }
                    return false;
                });
            }
        }

        // If the param scope is merged with the body scope we want to use the param scope symbols in the body scope.
        // So add a pid ref for the body using the param scope symbol. Note that in this case the same symbol will occur twice
        // in the same pid ref stack.
        if (paramScope != nullptr && pnodeFnc->IsBodyAndParamScopeMerged())
        {
            paramScope->ForEachSymbol([this](Symbol* paramSym)
            {
                PidRefStack* ref = PushPidRef(paramSym->GetPid());
                ref->SetSym(paramSym);
            });
        }

        AssertMsg(m_funcParenExprDepth == 0, "Paren exprs should have been resolved by the time we finish function formals");

        if (fLambda)
        {
#ifdef ASMJS_PLAT
            if (m_InAsmMode && (isTopLevelDeferredFunc && m_deferAsmJs))
            {
                // asm.js doesn't support lambda functions
                Js::AsmJSCompiler::OutputError(m_scriptContext, _u("Lambda functions are not supported."));
                Js::AsmJSCompiler::OutputError(m_scriptContext, _u("Asm.js compilation failed."));
                throw Js::AsmJsParseException();
            }
#endif
        }

        if (m_token.tk == tkRParen)
        {
            this->GetScanner()->Scan();
        }

        if (fLambda)
        {
            BOOL hadNewLine = this->GetScanner()->FHadNewLine();

            // it can be the case we do not have a fat arrow here if there is a valid expression on the left hand side
            // of the fat arrow, but that expression does not parse as a parameter list.  E.g.
            //    a.x => { }
            // Therefore check for it and error if not found.
            ChkCurTok(tkDArrow, ERRnoDArrow);

            // Newline character between arrow parameters and fat arrow is a syntax error but we want to check for
            // this after verifying there was a => token. Otherwise we would throw the wrong error.
            if (hadNewLine)
            {
                Error(ERRsyntax);
            }
        }

        if (isTopLevelDeferredFunc || (m_InAsmMode && m_deferAsmJs))
        {
            fDeferred = true;

            this->ParseTopLevelDeferredFunc(pnodeFnc, pnodeFncSave, pNameHint, fLambda, pNeedScanRCurly, fAllowIn);
        }
        else
        {
            AnalysisAssert(pnodeFnc);

            // Shouldn't be any temps in the arg list.
            Assert(*m_ppnodeVar == nullptr);

            // Start the var list.
            m_ppnodeVar = varNodesList;

            if (!pnodeFnc->IsBodyAndParamScopeMerged())
            {
                OUTPUT_TRACE_DEBUGONLY(Js::ParsePhase, _u("The param and body scope of the function %s cannot be merged\n"), pnodeFnc->pnodeName ? pnodeFnc->pnodeName->pid->Psz() : _u("Anonymous function"));
            }

            // Keep nested function declarations and expressions in the same list at function scope.
            // (Indicate this by nulling out the current function expressions list.)
            m_ppnodeExprScope = nullptr;

            if (buildAST)
            {
                if (m_token.tk != tkLCurly && fLambda)
                {
                    *pNeedScanRCurly = false;
                }
                uint savedStubCount = m_currDeferredStubCount;
                DeferredFunctionStub* savedStub = m_currDeferredStub;
                if (pnodeFnc->IsNested() && pnodeFncSave != nullptr && m_currDeferredStub != nullptr && pnodeFncSave->ichMin != pnodeFnc->ichMin)
                {
                    DeferredFunctionStub* childStub = m_currDeferredStub + (pnodeFncSave->nestedCount - 1);
                    m_currDeferredStubCount = childStub->nestedCount;
                    m_currDeferredStub = childStub->deferredStubs;
                }
                this->FinishFncDecl(pnodeFnc, pNameHint, fLambda, skipFormals, fAllowIn);
                m_currDeferredStub = savedStub;
                m_currDeferredStubCount = savedStubCount;
            }
            else
            {
                this->ParseNestedDeferredFunc(pnodeFnc, fLambda, pNeedScanRCurly, &strictModeTurnedOn, fAllowIn);
            }
        }

        // Restore the paren count for any outer spread/rest error checking.
        m_funcParenExprDepth = parenExprDepthSave;

        if (pnodeInnerBlock)
        {
            FinishParseBlock(pnodeInnerBlock, *pNeedScanRCurly);
        }

        if (!fModule && (m_token.tk == tkLCurly || !fLambda))
        {
            UpdateArgumentsNode(pnodeFnc, argNode);
        }

        CreateSpecialSymbolDeclarations(pnodeFnc);

        // Restore the lists of scopes that contain function expressions.

        Assert(m_ppnodeExprScope == nullptr || *m_ppnodeExprScope == nullptr);
        m_ppnodeExprScope = ppnodeExprScopeSave;

        Assert(m_ppnodeScope);
        Assert(nullptr == *m_ppnodeScope);
        m_ppnodeScope = ppnodeScopeSave;

        if (pnodeBlock)
        {
            FinishParseBlock(pnodeBlock, *pNeedScanRCurly);
        }

        if (IsStrictMode() || strictModeTurnedOn)
        {
            this->m_fUseStrictMode = TRUE; // Now we know this function is in strict mode

            if (!fWasAlreadyStrictMode)
            {
                // If this function turned on strict mode then we didn't check the formal
                // parameters or function name hint for future reserved word usage. So do that now.
                RestorePoint afterFnc;
                this->GetScanner()->Capture(&afterFnc);

                if (pnodeFnc->pnodeName != nullptr)
                {
                    // Rewind to the function name hint and check if the token is a reserved word.
                    this->GetScanner()->SeekTo(beginNameHint);
                    this->GetScanner()->Scan();
                    if (pnodeFnc->IsGenerator())
                    {
                        Assert(m_token.tk == tkStar);
                        Assert(m_scriptContext->GetConfig()->IsES6GeneratorsEnabled());
                        Assert(!(flags & fFncClassMember));
                        this->GetScanner()->Scan();
                    }
                    if (m_token.IsReservedWord())
                    {
                        IdentifierExpectedError(m_token);
                    }
                    CheckStrictModeEvalArgumentsUsage(m_token.GetIdentifier(this->GetHashTbl()));
                }

                // Fast forward to formal parameter list, check for future reserved words,
                // then restore scanner as it was.
                this->GetScanner()->SeekToForcingPid(beginFormals);
                CheckStrictFormalParameters();
                this->GetScanner()->SeekTo(afterFnc);
            }

            if (buildAST)
            {
                if (pnodeFnc->pnodeName != nullptr)
                {
                    Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
                    CheckStrictModeEvalArgumentsUsage(pnodeFnc->pnodeName->pid, pnodeFnc->pnodeName);
                }
            }

            this->m_fUseStrictMode = oldStrictMode;
            CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, StrictModeFunction, m_scriptContext);
        }

        ProcessCapturedNames(pnodeFnc);

        if (fDeferred)
        {
            AnalysisAssert(pnodeFnc);
            pnodeFnc->pnodeVars = nullptr;
        }

#if ENABLE_BACKGROUND_PARSING
        if (parallelJobStarted)
        {
            pnodeFnc = pnodeRealFnc;
            m_currentNodeFunc = pnodeRealFnc;

            // Let the foreground thread take care of marking the limit on the function node,
            // because in some cases this function's caller will want to change that limit,
            // so we don't want the background thread to try and touch it.
            pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
            pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
        }
#endif
    }

    // after parsing asm.js module, we want to reset asm.js state before continuing
    AnalysisAssert(pnodeFnc);
    if (pnodeFnc->GetAsmjsMode())
    {
        m_InAsmMode = false;
    }

    // Restore the statement stack.
    Assert(nullptr == m_pstmtCur);
    SetCurrentStatement(pstmtSave);

    if (pnodeFncExprScope)
    {
        FinishParseFncExprScope(pnodeFnc, pnodeFncExprScope);
    }
    m_grfscr |= uCanDeferSave;
    if (!m_stoppedDeferredParse)
    {
        m_grfscr |= uDeferSave;
    }

    this->GetScanner()->SetYieldIsKeywordRegion(fPreviousYieldIsKeyword);
    this->GetScanner()->SetAwaitIsKeywordRegion(fPreviousAwaitIsKeyword);

    // Restore the current function.
    if (buildAST)
    {
        Assert(pnodeFnc == m_currentNodeFunc);

        m_currentNodeFunc = pnodeFncSave;
        m_pCurrentAstSize = pAstSizeSave;

        if (!fLambda)
        {
            Assert(pnodeFnc == m_currentNodeNonLambdaFunc);
            m_currentNodeNonLambdaFunc = pnodeFncSaveNonLambda;
        }
    }
    else
    {
        Assert(pnodeFnc == m_currentNodeDeferredFunc);
        if (!fLambda)
        {
            Assert(pnodeFnc == m_currentNodeNonLambdaDeferredFunc);
            m_currentNodeNonLambdaDeferredFunc = pnodeFncSaveNonLambda;
        }
        m_currentNodeDeferredFunc = pnodeFncSave;
    }

    if (m_currentNodeFunc && pnodeFnc->HasWithStmt())
    {
        GetCurrentFunctionNode()->SetHasWithStmt(true);
    }
}

template<bool buildAST>
void Parser::UpdateCurrentNodeFunc(ParseNodeFnc * pnodeFnc, bool fLambda)
{
    if (buildAST)
    {
        // Make this the current function and start its sub-function list.
        m_currentNodeFunc = pnodeFnc;

        Assert(m_currentNodeDeferredFunc == nullptr);

        if (!fLambda)
        {
            m_currentNodeNonLambdaFunc = pnodeFnc;
        }
    }
    else // if !buildAST
    {
        AnalysisAssert(pnodeFnc);

        if (!fLambda)
        {
            m_currentNodeNonLambdaDeferredFunc = pnodeFnc;
        }

        m_currentNodeDeferredFunc = pnodeFnc;
    }
}

void Parser::ParseTopLevelDeferredFunc(ParseNodeFnc * pnodeFnc, ParseNodeFnc * pnodeFncParent, LPCOLESTR pNameHint, bool fLambda, bool *pNeedScanRCurly, bool fAllowIn)
{
    // Parse a function body that is a transition point from building AST to doing fast syntax check.

    pnodeFnc->pnodeVars = nullptr;
    pnodeFnc->pnodeBody = nullptr;

    this->m_deferringAST = TRUE;

    // Put the scanner into "no hashing" mode.
    BYTE deferFlags = this->GetScanner()->SetDeferredParse(TRUE);

    if (!fLambda)
    {
        ChkCurTok(tkLCurly, ERRnoLcurly);
    }
    else
    {
        // Lambda may consist of a single expression instead of a block
        if (this->GetScanner()->m_ptoken->tk == tkLCurly)
        {
            this->GetScanner()->Scan();
        }
        else
        {
            *pNeedScanRCurly = false;
        }
    }

    ParseNodePtr *ppnodeVarSave = m_ppnodeVar;

    m_ppnodeVar = &pnodeFnc->pnodeVars;

    // Don't try and skip scanning nested deferred lambdas which have only a single expression in the body.
    // Their more-complicated text extents won't match the deferred-stub and the single expression should be fast to scan, anyway.
    if (fLambda && !*pNeedScanRCurly)
    {
        ParseExpressionLambdaBody<false>(pnodeFnc, fAllowIn);
    }
    else if (pnodeFncParent != nullptr && m_currDeferredStub != nullptr && !pnodeFncParent->HasDefaultArguments())
    {
        // We've already parsed this function body for syntax errors on the initial parse of the script.
        // We have information that allows us to skip it, so do so.

        Assert(pnodeFncParent->nestedCount != 0);
        DeferredFunctionStub *stub = m_currDeferredStub + (pnodeFncParent->nestedCount - 1);

        Assert(pnodeFnc->ichMin == stub->ichMin
            || (stub->fncFlags & kFunctionIsAsync) == kFunctionIsAsync
            || ((stub->fncFlags & kFunctionIsGenerator) == kFunctionIsGenerator && (stub->fncFlags & kFunctionIsMethod) == kFunctionIsMethod));

        if (stub->fncFlags & kFunctionCallsEval)
        {
            this->MarkEvalCaller();
        }

        PHASE_PRINT_TRACE1(
            Js::SkipNestedDeferredPhase,
            _u("Skipping nested deferred function %d. %s: %d...%d\n"),
            pnodeFnc->functionId, GetFunctionName(pnodeFnc, pNameHint), pnodeFnc->ichMin, stub->restorePoint.m_ichMinTok);

        this->GetScanner()->SeekTo(stub->restorePoint, m_nextFunctionId);

        for (uint i = 0; i < stub->capturedNameCount; i++)
        {
            int stringId = stub->capturedNameSerializedIds[i];
            uint32 stringLength = 0;
            LPCWSTR stringVal = Js::ByteCodeSerializer::DeserializeString(stub, stringId, stringLength);

            OUTPUT_TRACE_DEBUGONLY(Js::SkipNestedDeferredPhase, _u("\tPushing a reference to '%s'\n"), stringVal);

            IdentPtr pid = this->GetHashTbl()->PidHashNameLen(stringVal, stringLength);
            PushPidRef(pid);
        }

        pnodeFnc->nestedCount = stub->nestedCount;
        pnodeFnc->deferredStub = stub->deferredStubs;
        pnodeFnc->fncFlags = (FncFlags)(pnodeFnc->fncFlags | stub->fncFlags);
    }
    else
    {
        ParseStmtList<false>(nullptr, nullptr, SM_DeferredParse, true /* isSourceElementList */);
    }

    if (!fLambda || *pNeedScanRCurly)
    {
        pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
        pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
    }

    m_ppnodeVar = ppnodeVarSave;

    // Restore the scanner's default hashing mode.
    // Do this before we consume the next token.
    this->GetScanner()->SetDeferredParseFlags(deferFlags);

    if (*pNeedScanRCurly)
    {
        ChkCurTokNoScan(tkRCurly, ERRnoRcurly);
    }

#if DBG
    pnodeFnc->deferredParseNextFunctionId = *this->m_nextFunctionId;
#endif
    this->m_deferringAST = FALSE;
}

bool Parser::DoParallelParse(ParseNodeFnc * pnodeFnc) const
{
#if ENABLE_BACKGROUND_PARSING
    if (!PHASE_ENABLED_RAW(ParallelParsePhase, m_sourceContextInfo->sourceContextId, pnodeFnc->functionId))
    {
        return false;
    }

    BackgroundParser *bgp = m_scriptContext->GetBackgroundParser();
    return bgp != nullptr;
#else
    return false;
#endif
}

bool Parser::ScanAheadToFunctionEnd(uint count)
{
    bool found = false;
    uint curlyDepth = 0;

    RestorePoint funcStart;
    this->GetScanner()->Capture(&funcStart);

    for (uint i = 0; i < count; i++)
    {
        switch (m_token.tk)
        {
        case tkStrTmplBegin:
        case tkStrTmplMid:
        case tkStrTmplEnd:
        case tkDiv:
        case tkAsgDiv:
        case tkScanError:
        case tkEOF:
            goto LEnd;

        case tkLCurly:
            UInt32Math::Inc(curlyDepth, Parser::OutOfMemory);
            break;

        case tkRCurly:
            if (curlyDepth == 1)
            {
                found = true;
                goto LEnd;
            }
            if (curlyDepth == 0)
            {
                goto LEnd;
            }
            curlyDepth--;
            break;
        }

        this->GetScanner()->ScanAhead();
    }

LEnd:
    this->GetScanner()->SeekTo(funcStart);
    return found;
}

#if ENABLE_BACKGROUND_PARSING
bool Parser::FastScanFormalsAndBody()
{
    // The scanner is currently pointing just past the name of a function.
    // The idea here is to find the end of the function body as quickly as possible,
    // by tokenizing and tracking {}'s if possible.
    // String templates require some extra logic but can be handled.

    // The real wrinkle is "/" and "/=", which may indicate either a RegExp literal or a division, depending
    // on the context.
    // To handle this with minimal work, keep track of the last ";" seen at each {} depth. If we see one of the
    // difficult tokens, rewind to the last ";" at the current {} depth and parse statements until we pass the
    // point where we had to rewind. This will process the "/" as required.

    RestorePoint funcStart;
    this->GetScanner()->Capture(&funcStart);

    const int maxRestorePointDepth = 16;
    struct FastScanRestorePoint
    {
        RestorePoint restorePoint;
        uint parenDepth;
        Js::LocalFunctionId functionId;
        int blockId;

        FastScanRestorePoint() : restorePoint(), parenDepth(0) {};
    };
    FastScanRestorePoint lastSColonAtCurlyDepth[maxRestorePointDepth];

    charcount_t ichStart = this->GetScanner()->IchMinTok();
    uint blockIdSave = m_nextBlockId;
    uint functionIdSave = *m_nextFunctionId;
    uint curlyDepth = 0;
    uint strTmplDepth = 0;
    for (;;)
    {
        switch (m_token.tk)
        {
        case tkStrTmplBegin:
            UInt32Math::Inc(strTmplDepth, Parser::OutOfMemory);
            // Fall through

        case tkStrTmplMid:
        case tkLCurly:
            UInt32Math::Inc(curlyDepth, Parser::OutOfMemory);
            Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
            break;

        case tkStrTmplEnd:
            // We can assert here, because the scanner will only return this token if we've told it we're
            // in a string template.
            Assert(strTmplDepth > 0);
            strTmplDepth--;
            break;

        case tkRCurly:
            if (curlyDepth == 1)
            {
                Assert(strTmplDepth == 0);
                if (PHASE_TRACE1(Js::ParallelParsePhase))
                {
                    Output::Print(_u("Finished fast seek: %d. %s -- %d...%d\n"),
                        m_currentNodeFunc->functionId,
                        GetFunctionName(m_currentNodeFunc, m_currentNodeFunc->hint),
                        ichStart, this->GetScanner()->IchLimTok());
                }
                return true;
            }
            if (curlyDepth < maxRestorePointDepth)
            {
                lastSColonAtCurlyDepth[curlyDepth].restorePoint.m_ichMinTok = (uint)-1;
            }
            curlyDepth--;
            if (strTmplDepth > 0)
            {
                this->GetScanner()->SetScanState(Scanner_t::ScanState::ScanStateStringTemplateMiddleOrEnd);
            }
            break;

        case tkSColon:
            // Track the location of the ";" (if it's outside parens, as we don't, for instance, want
            // to track the ";"'s in a for-loop header. If we find it's important to rewind within a paren
            // expression, we can do something more sophisticated.)
            if (curlyDepth < maxRestorePointDepth && lastSColonAtCurlyDepth[curlyDepth].parenDepth == 0)
            {
                this->GetScanner()->Capture(&lastSColonAtCurlyDepth[curlyDepth].restorePoint);
                lastSColonAtCurlyDepth[curlyDepth].functionId = *this->m_nextFunctionId;
                lastSColonAtCurlyDepth[curlyDepth].blockId = m_nextBlockId;
            }
            break;

        case tkLParen:
            if (curlyDepth < maxRestorePointDepth)
            {
                UInt32Math::Inc(lastSColonAtCurlyDepth[curlyDepth].parenDepth);
            }
            break;

        case tkRParen:
            if (curlyDepth < maxRestorePointDepth)
            {
                Assert(lastSColonAtCurlyDepth[curlyDepth].parenDepth != 0);
                lastSColonAtCurlyDepth[curlyDepth].parenDepth--;
            }
            break;

        case tkID:
        {
            charcount_t tokLength = this->GetScanner()->IchLimTok() - this->GetScanner()->IchMinTok();
            // Detect the function and class keywords so we can track function ID's.
            // (In fast mode, the scanner doesn't distinguish keywords and doesn't point the token
            // to a PID.)
            // Detect try/catch/for to increment block count for them.
            switch (tokLength)
            {
            case 3:
                if (!memcmp(this->GetScanner()->PchMinTok(), "try", 3) || !memcmp(this->GetScanner()->PchMinTok(), "for", 3))
                {
                    Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
                }
                break;
            case 5:
                if (!memcmp(this->GetScanner()->PchMinTok(), "catch", 5))
                {
                    Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
                }
                else if (!memcmp(this->GetScanner()->PchMinTok(), "class", 5))
                {
                    Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
                    Int32Math::Inc(*this->m_nextFunctionId, (int*)this->m_nextFunctionId);
                }
                break;
            case 8:
                if (!memcmp(this->GetScanner()->PchMinTok(), "function", 8))
                {
                    // Account for the possible func expr scope or dummy block for missing {}'s around a declaration
                    Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
                    Int32Math::Inc(*this->m_nextFunctionId, (int*)this->m_nextFunctionId);
                }
                break;
            }
            break;
        }

        case tkDArrow:
            Int32Math::Inc(m_nextBlockId, &m_nextBlockId);
            Int32Math::Inc(*this->m_nextFunctionId, (int*)this->m_nextFunctionId);
            break;

        case tkDiv:
        case tkAsgDiv:
        {
            int opl;
            OpCode nop;
            tokens tkPrev = this->GetScanner()->m_tkPrevious;
            if ((this->GetHashTbl()->TokIsBinop(tkPrev, &opl, &nop) && nop != knopNone) ||
                (this->GetHashTbl()->TokIsUnop(tkPrev, &opl, &nop) &&
                    nop != knopNone &&
                    tkPrev != tkInc &&
                    tkPrev != tkDec) ||
                tkPrev == tkColon ||
                tkPrev == tkLParen ||
                tkPrev == tkLBrack ||
                tkPrev == tkRETURN)
            {
                // Previous token indicates that we're starting an expression here and can't have a
                // binary operator now.
                // Assume this is a RegExp.
                ParseRegExp<false>();
                break;
            }
            uint tempCurlyDepth = curlyDepth < maxRestorePointDepth ? curlyDepth : maxRestorePointDepth - 1;
            for (; tempCurlyDepth != (uint)-1; tempCurlyDepth--)
            {
                // We don't know whether we've got a RegExp or a divide. Rewind to the last safe ";"
                // if we can and parse statements until we pass this point.
                if (lastSColonAtCurlyDepth[tempCurlyDepth].restorePoint.m_ichMinTok != -1)
                {
                    break;
                }
            }
            if (tempCurlyDepth != (uint)-1)
            {
                ParseNodeFnc * pnodeFncSave = m_currentNodeFunc;
                int32 *pastSizeSave = m_pCurrentAstSize;
                uint *pnestedCountSave = m_pnestedCount;
                ParseNodePtr *ppnodeScopeSave = m_ppnodeScope;
                ParseNodePtr *ppnodeExprScopeSave = m_ppnodeExprScope;

                ParseNodeFnc * pnodeFnc = CreateDummyFuncNode(true);

                charcount_t ichStop = this->GetScanner()->IchLimTok();
                curlyDepth = tempCurlyDepth;
                this->GetScanner()->SeekTo(lastSColonAtCurlyDepth[tempCurlyDepth].restorePoint);
                m_nextBlockId = lastSColonAtCurlyDepth[tempCurlyDepth].blockId;
                *this->m_nextFunctionId = lastSColonAtCurlyDepth[tempCurlyDepth].functionId;

                ParseNodeBlock * pnodeBlock = StartParseBlock<true>(PnodeBlockType::Function, ScopeType_FunctionBody);
                pnodeFnc->pnodeScopes = pnodeBlock;
                m_ppnodeScope = &pnodeBlock->pnodeScopes;
                m_ppnodeExprScope = nullptr;
                this->GetScanner()->Scan();
                do
                {
                    ParseStatement<false>();
                } while (this->GetScanner()->IchMinTok() < ichStop);

                FinishParseBlock(pnodeBlock);

                m_currentNodeFunc = pnodeFncSave;
                m_pCurrentAstSize = pastSizeSave;
                m_pnestedCount = pnestedCountSave;
                m_ppnodeScope = ppnodeScopeSave;
                m_ppnodeExprScope = ppnodeExprScopeSave;

                // We've already consumed the first token of the next statement, so just continue
                // without a further scan.
                continue;
            }
        }

        // fall through to rewind to function start
        case tkScanError:
        case tkEOF:
            // Unexpected token.
            if (PHASE_TRACE1(Js::ParallelParsePhase))
            {
                Output::Print(_u("Failed fast seek: %d. %s -- %d...%d\n"),
                    m_currentNodeFunc->functionId,
                    GetFunctionName(m_currentNodeFunc, m_currentNodeFunc->hint),
                    ichStart, this->GetScanner()->IchLimTok());
            }
            m_nextBlockId = blockIdSave;
            *m_nextFunctionId = functionIdSave;
            this->GetScanner()->SeekTo(funcStart);
            return false;
        }

        this->GetScanner()->ScanNoKeywords();
    }
}
#endif

ParseNodeFnc * Parser::CreateDummyFuncNode(bool fDeclaration)
{
    // Create a dummy node and make it look like the current function declaration.
    // Do this in situations where we want to parse statements without impacting
    // the state of the "real" AST.

    ParseNodeFnc * pnodeFnc = CreateAllowDeferNodeForOpT<knopFncDecl>();
    pnodeFnc->SetDeclaration(fDeclaration);

    pnodeFnc->SetNested(m_currentNodeFunc != nullptr); // If there is a current function, then we're a nested function.
    pnodeFnc->SetStrictMode(IsStrictMode()); // Inherit current strict mode -- may be overridden by the function itself if it contains a strict mode directive.   

    m_pCurrentAstSize = &pnodeFnc->astSize;
    m_currentNodeFunc = pnodeFnc;
    m_pnestedCount = &pnodeFnc->nestedCount;

    return pnodeFnc;
}

void Parser::ParseNestedDeferredFunc(ParseNodeFnc * pnodeFnc, bool fLambda, bool *pNeedScanRCurly, bool *pStrictModeTurnedOn, bool fAllowIn)
{
    // Parse a function nested inside another deferred function.

    size_t lengthBeforeBody = this->GetSourceLength();

    if (m_token.tk != tkLCurly && fLambda)
    {
        ParseExpressionLambdaBody<false>(pnodeFnc, fAllowIn);
        *pNeedScanRCurly = false;
    }
    else
    {
        ChkCurTok(tkLCurly, ERRnoLcurly);

        bool* detectStrictModeOn = IsStrictMode() ? nullptr : pStrictModeTurnedOn;
        m_ppnodeVar = &m_currentNodeDeferredFunc->pnodeVars;

        ParseStmtList<false>(nullptr, nullptr, SM_DeferredParse, true /* isSourceElementList */, detectStrictModeOn);

        ChkCurTokNoScan(tkRCurly, ERRnoRcurly);

        pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
        pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
    }

    if (*pStrictModeTurnedOn)
    {
        pnodeFnc->SetStrictMode(true);
    }

    if (!PHASE_OFF1(Js::SkipNestedDeferredPhase))
    {
        // Record the end of the function and the function ID increment that happens inside the function.
        // Byte code gen will use this to build stub information to allow us to skip this function when the
        // enclosing function is fully parsed.
        RestorePoint *restorePoint = Anew(&m_nodeAllocator, RestorePoint);
        this->GetScanner()->Capture(restorePoint,
            *m_nextFunctionId - pnodeFnc->functionId - 1,
            lengthBeforeBody - this->GetSourceLength());
        pnodeFnc->pRestorePoint = restorePoint;
    }
}

template<bool buildAST>
void Parser::ParseFncName(ParseNodeFnc * pnodeFnc, ushort flags, IdentPtr* pFncNamePid)
{
    Assert(pnodeFnc);
    BOOL fDeclaration = flags & fFncDeclaration;
    BOOL fIsAsync = flags & fFncAsync;
    
    this->GetScanner()->Scan();

    // If generators are enabled then we are in a recent enough version
    // that deferred parsing will create a parse node for pnodeFnc and
    // it is safe to assume it is not null.
    if (flags & fFncGenerator)
    {
        Assert(m_scriptContext->GetConfig()->IsES6GeneratorsEnabled());
        pnodeFnc->SetIsGenerator();
    }
    else if (m_scriptContext->GetConfig()->IsES6GeneratorsEnabled() &&
        m_token.tk == tkStar &&
        !(flags & fFncClassMember))
    {
        if (!fDeclaration)
        {
            bool fPreviousYieldIsKeyword = this->GetScanner()->SetYieldIsKeywordRegion(!fDeclaration);
            this->GetScanner()->Scan();
            this->GetScanner()->SetYieldIsKeywordRegion(fPreviousYieldIsKeyword);
        }
        else
        {
            this->GetScanner()->Scan();
        }

        pnodeFnc->SetIsGenerator();
    }

    if (fIsAsync)
    {
        if (pnodeFnc->IsGenerator())
        {
            Error(ERRsyntax);
        }
        pnodeFnc->SetIsAsync();
    }

    pnodeFnc->pnodeName = nullptr;

    if ((m_token.tk != tkID || flags & fFncNoName)
        && (IsStrictMode() || fDeclaration
            || pnodeFnc->IsGenerator() || pnodeFnc->IsAsync()
                || (m_token.tk != tkYIELD && m_token.tk != tkAWAIT))) // Function expressions can have the name yield/await even inside generator/async functions
    {
        if (fDeclaration ||
            m_token.IsReservedWord())  // For example:  var x = (function break(){});
        {
            IdentifierExpectedError(m_token);
        }
        return;
    }

    Assert(m_token.tk == tkID || (m_token.tk == tkYIELD && !fDeclaration) || (m_token.tk == tkAWAIT && !fDeclaration));

    if (IsStrictMode())
    {
        CheckStrictModeEvalArgumentsUsage(m_token.GetIdentifier(this->GetHashTbl()));
    }


    IdentPtr pidBase = m_token.GetIdentifier(this->GetHashTbl());
    pnodeFnc->pnodeName = CreateDeclNode(knopVarDecl, pidBase, STFunction);
    pnodeFnc->pid = pnodeFnc->pnodeName->pid;

    if (pFncNamePid != nullptr)
    {
        *pFncNamePid = pidBase;
    }

    this->GetScanner()->Scan();
}

void Parser::ValidateFormals()
{
    ParseFncFormals<false>(this->GetCurrentFunctionNode(), nullptr, fFncNoFlgs);
    // Eat the tkRParen. The ParseFncDeclHelper caller expects to see it.
    this->GetScanner()->Scan();
}

void Parser::ValidateSourceElementList()
{
    ParseStmtList<false>(nullptr, nullptr, SM_NotUsed, true);
}

void Parser::UpdateOrCheckForDuplicateInFormals(IdentPtr pid, SList<IdentPtr> *formals)
{
    bool isStrictMode = IsStrictMode();
    if (isStrictMode)
    {
        CheckStrictModeEvalArgumentsUsage(pid);
    }

    if (formals->Has(pid))
    {
        if (isStrictMode)
        {
            Error(ERRES5ArgSame);
        }
        else
        {
            Error(ERRFormalSame);
        }
    }
    else
    {
        formals->Prepend(pid);
    }
}

template<bool buildAST>
void Parser::ParseFncFormals(ParseNodeFnc * pnodeFnc, ParseNodeFnc * pnodeParentFnc, ushort flags, bool isTopLevelDeferredFunc)
{
    bool fLambda = (flags & fFncLambda) != 0;
    bool fMethod = (flags & fFncMethod) != 0;
    bool fNoArg = (flags & fFncNoArg) != 0;
    bool fOneArg = (flags & fFncOneArg) != 0;
    bool fAsync = (flags & fFncAsync) != 0;

    bool fPreviousYieldIsKeyword = false;
    bool fPreviousAwaitIsKeyword = false;

    if (fLambda)
    {
        fPreviousYieldIsKeyword = this->GetScanner()->SetYieldIsKeywordRegion(pnodeParentFnc != nullptr && pnodeParentFnc->IsGenerator());
        fPreviousAwaitIsKeyword = this->GetScanner()->SetAwaitIsKeywordRegion(fAsync || (pnodeParentFnc != nullptr && pnodeParentFnc->IsAsync()));
    }

    Assert(!fNoArg || !fOneArg); // fNoArg and fOneArg can never be true at the same time.

    // strictFormals corresponds to the StrictFormalParameters grammar production
    // in the ES spec which just means duplicate names are not allowed
    bool fStrictFormals = IsStrictMode() || fLambda || fMethod;

    // When detecting duplicated formals pids are needed so force PID creation (unless the function should take 0 or 1 arg).
    bool forcePid = fStrictFormals && !fNoArg && !fOneArg;
    AutoTempForcePid autoForcePid(this->GetScanner(), forcePid);

    // Lambda's allow single formal specified by a single binding identifier without parentheses, special case it.
    if (fLambda && m_token.tk == tkID)
    {
        IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());

        CreateVarDeclNode(pid, STFormal, false, nullptr, false);
        CheckPidIsValid(pid);

        this->GetScanner()->Scan();

        if (m_token.tk != tkDArrow)
        {
            Error(ERRsyntax, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok());
        }

        if (fLambda)
        {
            this->GetScanner()->SetYieldIsKeywordRegion(fPreviousYieldIsKeyword);
            this->GetScanner()->SetAwaitIsKeywordRegion(fPreviousAwaitIsKeyword);
        }

        return;
    }
    else if (fLambda && m_token.tk == tkAWAIT)
    {
        // async await => {}
        IdentifierExpectedError(m_token);
    }

    // Otherwise, must have a parameter list within parens.
    ChkCurTok(tkLParen, ERRnoLparen);

    // Now parse the list of arguments, if present
    if (m_token.tk == tkRParen)
    {
        if (fOneArg)
        {
            Error(ERRSetterMustHaveOneParameter);
        }
    }
    else
    {
        if (fNoArg)
        {
            Error(ERRGetterMustHaveNoParameters);
        }
        SList<IdentPtr> formals(&m_nodeAllocator);
        ParseNodeVar * pnodeT = nullptr;
        bool seenRestParameter = false;
        bool isNonSimpleParameterList = false;
        for (Js::ArgSlot argPos = 0; ; ++argPos)
        {
            bool isBindingPattern = false;
            if (m_scriptContext->GetConfig()->IsES6RestEnabled() && m_token.tk == tkEllipsis)
            {
                // Possible rest parameter
                this->GetScanner()->Scan();
                seenRestParameter = true;
            }
            if (m_token.tk != tkID)
            {
                if (IsES6DestructuringEnabled() && IsPossiblePatternStart())
                {
                    // Mark that the function has a non simple parameter list before parsing the pattern since the pattern can have function definitions.
                    this->GetCurrentFunctionNode()->SetHasNonSimpleParameterList();
                    this->GetCurrentFunctionNode()->SetHasDestructuredParams();

                    ParseNodePtr *const ppnodeVarSave = m_ppnodeVar;
                    m_ppnodeVar = &pnodeFnc->pnodeVars;

                    ParseNodePtr * ppNodeLex = m_currentBlockInfo->m_ppnodeLex;
                    Assert(ppNodeLex != nullptr);

                    ParseNodeParamPattern * paramPattern = nullptr;
                    ParseNode * pnodePattern = nullptr;
                    if (isTopLevelDeferredFunc)
                    {
                        pnodePattern = ParseDestructuredLiteral<false>(tkLET, true /*isDecl*/, false /*topLevel*/);
                    }
                    else
                    {
                        pnodePattern = ParseDestructuredLiteral<buildAST>(tkLET, true /*isDecl*/, false /*topLevel*/);
                    }

                    // Instead of passing the STFormal all the way on many methods, it seems it is better to change the symbol type afterward.
                    for (ParseNodePtr lexNode = *ppNodeLex; lexNode != nullptr; lexNode = lexNode->AsParseNodeVar()->pnodeNext)
                    {
                        Assert(lexNode->IsVarLetOrConst());
                        UpdateOrCheckForDuplicateInFormals(lexNode->AsParseNodeVar()->pid, &formals);
                        lexNode->AsParseNodeVar()->sym->SetSymbolType(STFormal);
                        if (lexNode->AsParseNodeVar()->pid == wellKnownPropertyPids.arguments)
                        {
                            GetCurrentFunctionNode()->grfpn |= PNodeFlags::fpnArguments_overriddenInParam;
                        }
                    }

                    m_ppnodeVar = ppnodeVarSave;

                    if (buildAST)
                    {
                        if (isTopLevelDeferredFunc)
                        {
                            Assert(pnodePattern == nullptr);
                            // Create a dummy pattern node as we need the node to be considered for the param count
                            paramPattern = CreateDummyParamPatternNode(this->GetScanner()->IchMinTok());
                        }
                        else
                        {
                            Assert(pnodePattern);
                            paramPattern = CreateParamPatternNode(pnodePattern);
                        }
                        // Linking the current formal parameter (which is pattern parameter) with other formals.
                        *m_ppnodeVar = paramPattern;
                        paramPattern->pnodeNext = nullptr;
                        m_ppnodeVar = &paramPattern->pnodeNext;
                    }

                    isBindingPattern = true;
                    isNonSimpleParameterList = true;
                }
                else
                {
                    IdentifierExpectedError(m_token);
                }
            }

            if (!isBindingPattern)
            {
                IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());
                LPCOLESTR pNameHint = pid->Psz();
                uint32 nameHintLength = pid->Cch();
                uint32 nameHintOffset = 0;

                if (seenRestParameter)
                {
                    this->GetCurrentFunctionNode()->SetHasNonSimpleParameterList();
                    if (flags & fFncOneArg)
                    {
                        // The parameter of a setter cannot be a rest parameter.
                        Error(ERRUnexpectedEllipsis);
                    }
                    pnodeT = CreateDeclNode(knopVarDecl, pid, STFormal, false);
                    pnodeT->sym->SetIsNonSimpleParameter(true);
                    if (buildAST)
                    {
                        // When only validating formals, we won't have a function node.
                        pnodeFnc->pnodeRest = pnodeT;
                        if (!isNonSimpleParameterList)
                        {
                            // This is the first non-simple parameter we've seen. We need to go back
                            // and set the Symbols of all previous parameters.
                            MapFormalsWithoutRest(m_currentNodeFunc, [&](ParseNodePtr pnodeArg) { pnodeArg->AsParseNodeVar()->sym->SetIsNonSimpleParameter(true); });
                        }
                    }

                    isNonSimpleParameterList = true;
                }
                else
                {
                    pnodeT = CreateVarDeclNode(pid, STFormal, false, nullptr, false);
                    if (isNonSimpleParameterList)
                    {
                        pnodeT->sym->SetIsNonSimpleParameter(true);
                    }
                }

                if (buildAST && pid == wellKnownPropertyPids.arguments)
                {
                    // This formal parameter overrides the built-in 'arguments' object
                    m_currentNodeFunc->grfpn |= PNodeFlags::fpnArguments_overriddenInParam;
                }

                if (fStrictFormals)
                {
                    UpdateOrCheckForDuplicateInFormals(pid, &formals);
                }

                this->GetScanner()->Scan();

                if (seenRestParameter && m_token.tk != tkRParen && m_token.tk != tkAsg)
                {
                    Error(ERRRestLastArg);
                }

                if (m_token.tk == tkAsg && m_scriptContext->GetConfig()->IsES6DefaultArgsEnabled())
                {
                    if (seenRestParameter && m_scriptContext->GetConfig()->IsES6RestEnabled())
                    {
                        Error(ERRRestWithDefault);
                    }

                    // In defer parse mode we have to flag the function node to indicate that it has default arguments
                    // so that it will be considered for any syntax error scenario.
                    // Also mark it before parsing the expression as it may contain functions.
                    ParseNodeFnc * currentFncNode = GetCurrentFunctionNode();
                    if (!currentFncNode->HasDefaultArguments())
                    {
                        currentFncNode->SetHasDefaultArguments();
                        currentFncNode->SetHasNonSimpleParameterList();
                        currentFncNode->firstDefaultArg = argPos;
                    }

                    this->GetScanner()->Scan();

                    ParseNodePtr pnodeInit;
                    if (isTopLevelDeferredFunc)
                    {
                        // Defer default expressions if the function will be deferred, since we know they can't be evaluated
                        // until the function is fully compiled, and generating code for a function nested inside a deferred function
                        // creates inconsistencies.
                        pnodeInit = ParseExpr<false>(koplCma, nullptr, TRUE, FALSE, pNameHint, &nameHintLength, &nameHintOffset);
                    }
                    else
                    {
                        pnodeInit = ParseExpr<buildAST>(koplCma, nullptr, TRUE, FALSE, pNameHint, &nameHintLength, &nameHintOffset);
                    }

                    if (buildAST && pnodeInit && pnodeInit->nop == knopFncDecl)
                    {
                        Assert(nameHintLength >= nameHintOffset);
                        ParseNodeFnc * pnodeFncInit = pnodeInit->AsParseNodeFnc();
                        pnodeFncInit->hint = pNameHint;
                        pnodeFncInit->hintLength = nameHintLength;
                        pnodeFncInit->hintOffset = nameHintOffset;
                    }

                    AnalysisAssert(pnodeT);
                    pnodeT->sym->SetIsNonSimpleParameter(true);
                    if (!isNonSimpleParameterList)
                    {
                        if (buildAST)
                        {
                            // This is the first non-simple parameter we've seen. We need to go back
                            // and set the Symbols of all previous parameters.
                            MapFormalsWithoutRest(m_currentNodeFunc, [&](ParseNodePtr pnodeArg) { pnodeArg->AsParseNodeVar()->sym->SetIsNonSimpleParameter(true); });
                        }

                        // There may be previous parameters that need to be checked for duplicates.
                        isNonSimpleParameterList = true;
                    }

                    if (buildAST)
                    {
                        if (!m_currentNodeFunc->HasDefaultArguments())
                        {
                            CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, DefaultArgFunction, m_scriptContext);
                        }
                        pnodeT->pnodeInit = pnodeInit;
                        pnodeT->ichLim = this->GetScanner()->IchLimTok();
                    }
                }
            }

            if (isNonSimpleParameterList && m_currentScope->GetHasDuplicateFormals())
            {
                Error(ERRFormalSame);
            }

            if (flags & fFncOneArg)
            {
                if (m_token.tk != tkRParen)
                {
                    Error(ERRSetterMustHaveOneParameter);
                }
                break; //enforce only one arg
            }

            if (m_token.tk != tkComma)
            {
                break;
            }

            this->GetScanner()->Scan();

            if (m_token.tk == tkRParen && m_scriptContext->GetConfig()->IsES7TrailingCommaEnabled())
            {
                break;
            }
        }

        if (seenRestParameter)
        {
            CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Rest, m_scriptContext);
        }

        if (m_token.tk != tkRParen)
        {
            Error(ERRnoRparen);
        }

        if (this->GetCurrentFunctionNode()->CallsEval() || this->GetCurrentFunctionNode()->ChildCallsEval())
        {
            Assert(pnodeFnc->HasNonSimpleParameterList());
            pnodeFnc->ResetBodyAndParamScopeMerged();
        }
    }
    Assert(m_token.tk == tkRParen);

    if (fLambda)
    {
        this->GetScanner()->SetYieldIsKeywordRegion(fPreviousYieldIsKeyword);
        this->GetScanner()->SetAwaitIsKeywordRegion(fPreviousAwaitIsKeyword);
    }
}

template<bool buildAST>
ParseNodePtr Parser::GenerateModuleFunctionWrapper()
{
    ParseNodePtr pnodeFnc = ParseFncDeclNoCheckScope<buildAST>(fFncModule, SuperRestrictionState::Disallowed, nullptr, /* needsPIDOnRCurlyScan */ false, /* fUnaryOrParen */ true);
    ParseNodePtr callNode = CreateCallNode(knopCall, pnodeFnc, nullptr);

    return callNode;
}

template<bool buildAST>
ParseNodeFnc * Parser::GenerateEmptyConstructor(bool extends)
{
    ParseNodeFnc * pnodeFnc;

    // Create the node.
    pnodeFnc = CreateAllowDeferNodeForOpT<knopFncDecl>();
    pnodeFnc->SetNested(NULL != m_currentNodeFunc);
    pnodeFnc->SetStrictMode();
    pnodeFnc->SetDeclaration(TRUE);
    pnodeFnc->SetIsMethod(TRUE);
    pnodeFnc->SetIsClassMember(TRUE);
    pnodeFnc->SetIsClassConstructor(TRUE);
    pnodeFnc->SetIsBaseClassConstructor(!extends);
    pnodeFnc->SetHasNonThisStmt();
    pnodeFnc->SetIsGeneratedDefault(TRUE);
    pnodeFnc->SetHasComputedName();
    pnodeFnc->SetHasHomeObj();
    pnodeFnc->SetHomeObjLocation(Js::Constants::NoRegister);

    pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
    pnodeFnc->ichMin = this->GetScanner()->IchMinTok();
    pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
    pnodeFnc->cbMin = this->GetScanner()->IecpMinTok();
    pnodeFnc->lineNumber = this->GetScanner()->LineCur();

    pnodeFnc->functionId = (*m_nextFunctionId);

    // In order to (re-)defer the default constructor, we need to, for instance, track
    // deferred class expression the way we track function expression, since we lose the part of the source
    // that tells us which we have.
    Assert(!pnodeFnc->canBeDeferred);

#ifdef DBG
    pnodeFnc->deferredParseNextFunctionId = *(this->m_nextFunctionId);
#endif

    AppendFunctionToScopeList(true, pnodeFnc);

    if (m_nextFunctionId)
    {
        (*m_nextFunctionId)++;
    }

    // Update the count of functions nested in the current parent.
    if (m_pnestedCount)
    {
        (*m_pnestedCount)++;
    }

    if (this->GetScanner()->IchMinTok() >= this->GetScanner()->IchMinLine())
    {
        // In scenarios involving defer parse IchMinLine() can be incorrect for the first line after defer parse
        pnodeFnc->columnNumber = this->GetScanner()->IchMinTok() - this->GetScanner()->IchMinLine();
    }
    else if (m_currentNodeFunc)
    {
        // For the first line after defer parse, compute the column relative to the column number
        // of the lexically parent function.
        ULONG offsetFromCurrentFunction = this->GetScanner()->IchMinTok() - m_currentNodeFunc->ichMin;
        pnodeFnc->columnNumber = m_currentNodeFunc->columnNumber + offsetFromCurrentFunction;
    }
    else
    {
        // if there is no current function, lets give a default of 0.
        pnodeFnc->columnNumber = 0;
    }

    int32 * pAstSizeSave = m_pCurrentAstSize;
    m_pCurrentAstSize = &(pnodeFnc->astSize);

    // Make this the current function.
    ParseNodeFnc * pnodeFncSave = m_currentNodeFunc;
    m_currentNodeFunc = pnodeFnc;

    ParseNodeName * argsId = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;
    ParseNodeBlock * pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Parameter, ScopeType_Parameter);

    if (buildAST && extends)
    {
        // constructor(...args) { super(...args); }
        //             ^^^^^^^
        ParseNodePtr *const ppnodeVarSave = m_ppnodeVar;
        m_ppnodeVar = &pnodeFnc->pnodeVars;

        IdentPtr pidargs = this->GetHashTbl()->PidHashNameLen(_u("args"), sizeof("args") - 1);
        ParseNodeVar * pnodeT = CreateVarDeclNode(pidargs, STFormal);
        pnodeT->sym->SetIsNonSimpleParameter(true);
        pnodeFnc->pnodeRest = pnodeT;
        PidRefStack *ref = this->PushPidRef(pidargs);

        argsId = CreateNameNode(pidargs, ref, pnodeFnc->ichMin, pnodeFnc->ichLim);
        m_ppnodeVar = ppnodeVarSave;
    }

    ParseNodeBlock * pnodeInnerBlock = StartParseBlock<buildAST>(PnodeBlockType::Function, ScopeType_FunctionBody);
    pnodeBlock->pnodeScopes = pnodeInnerBlock;
    pnodeFnc->pnodeBodyScope = pnodeInnerBlock;
    pnodeFnc->pnodeScopes = pnodeBlock;

    if (buildAST)
    {
        if (extends)
        {
            // constructor(...args) { super(...args); }
            //                        ^^^^^^^^^^^^^^^
            Assert(argsId);
            ParseNodeUni * spreadArg = CreateUniNode(knopEllipsis, argsId, pnodeFnc->ichMin, pnodeFnc->ichLim);
            ParseNodeSpecialName * superRef = ReferenceSpecialName(wellKnownPropertyPids._superConstructor, pnodeFnc->ichMin, pnodeFnc->ichLim, true);
            pnodeFnc->SetHasSuperReference(TRUE);
            ParseNodeSuperCall * callNode = CreateSuperCallNode(superRef, spreadArg);

            callNode->pnodeThis = ReferenceSpecialName(wellKnownPropertyPids._this, pnodeFnc->ichMin, pnodeFnc->ichLim, true);
            callNode->pnodeNewTarget = ReferenceSpecialName(wellKnownPropertyPids._newTarget, pnodeFnc->ichMin, pnodeFnc->ichLim, true);
            callNode->spreadArgCount = 1;
            AddToNodeList(&pnodeFnc->pnodeBody, &lastNodeRef, callNode);
        }

        AddToNodeList(&pnodeFnc->pnodeBody, &lastNodeRef, CreateNodeForOpT<knopEndCode>());
    }

    FinishParseBlock(pnodeInnerBlock);

    CreateSpecialSymbolDeclarations(pnodeFnc);

    FinishParseBlock(pnodeBlock);

    m_currentNodeFunc = pnodeFncSave;
    m_pCurrentAstSize = pAstSizeSave;

    return pnodeFnc;
}

template<bool buildAST>
void Parser::ParseExpressionLambdaBody(ParseNodeFnc * pnodeLambda, bool fAllowIn)
{
    ParseNodePtr *lastNodeRef = nullptr;

    // The lambda body is a single expression, the result of which is the return value.
    ParseNodeReturn * pnodeRet = nullptr;

    if (buildAST)
    {
        pnodeRet = CreateNodeForOpT<knopReturn>();
        pnodeRet->grfpn |= PNodeFlags::fpnSyntheticNode;
        pnodeLambda->pnodeScopes->pnodeStmt = pnodeRet;
    }

    IdentToken token;
    charcount_t lastRParen = 0;

    // We need to disable deferred parse mode in the scanner because the lambda body doesn't end with a right paren.
    // The scanner needs to create a pid in the case of a string constant token immediately following the lambda body expression.
    // Otherwise, we'll save null for the string constant pid which will AV during ByteCode generation.
    BYTE fScanDeferredFlagsSave = this->GetScanner()->SetDeferredParse(FALSE);
    ParseNodePtr result = ParseExpr<buildAST>(koplAsg, nullptr, fAllowIn, FALSE, nullptr, nullptr, nullptr, &token, false, nullptr, &lastRParen);
    this->GetScanner()->SetDeferredParseFlags(fScanDeferredFlagsSave);

    this->MarkEscapingRef(result, &token);

    if (buildAST)
    {
        pnodeRet->pnodeExpr = result;

        pnodeRet->ichMin = pnodeRet->pnodeExpr->ichMin;
        pnodeRet->ichLim = pnodeRet->pnodeExpr->ichLim;

        // Pushing a statement node with PushStmt<>() normally does this initialization
        // but do it here manually since we know there is no outer statement node.
        pnodeRet->grfnop = 0;
        pnodeRet->pnodeOuter = nullptr;

        pnodeLambda->ichLim = max(pnodeRet->ichLim, lastRParen);
        pnodeLambda->cbLim = this->GetScanner()->IecpLimTokPrevious();
        pnodeLambda->pnodeScopes->ichLim = pnodeRet->ichLim;

        pnodeLambda->pnodeBody = nullptr;
        AddToNodeList(&pnodeLambda->pnodeBody, &lastNodeRef, pnodeRet);

        // Append an EndCode node.
        ParseNodePtr end = CreateNodeForOpT<knopEndCode>(pnodeRet->ichLim);
        end->ichLim = end->ichMin; // make end code zero width at the immediate end of lambda body
        AddToNodeList(&pnodeLambda->pnodeBody, &lastNodeRef, end);

        // Lambda's do not have arguments binding
        pnodeLambda->SetHasReferenceableBuiltInArguments(false);
    }
    else
    {
        pnodeLambda->ichLim = max(this->GetScanner()->IchLimTokPrevious(), lastRParen);
        pnodeLambda->cbLim = this->GetScanner()->IecpLimTokPrevious();
    }
}

void Parser::CheckStrictFormalParameters()
{
    if (m_token.tk == tkID)
    {
        // single parameter arrow function case
        IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());
        CheckStrictModeEvalArgumentsUsage(pid);
        return;
    }

    Assert(m_token.tk == tkLParen);
    this->GetScanner()->ScanForcingPid();

    if (m_token.tk != tkRParen)
    {
        SList<IdentPtr> formals(&m_nodeAllocator);
        for (;;)
        {
            if (m_token.tk != tkID)
            {
                IdentifierExpectedError(m_token);
            }

            IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());
            CheckStrictModeEvalArgumentsUsage(pid);
            if (formals.Has(pid))
            {
                Error(ERRES5ArgSame, this->GetScanner()->IchMinTok(), this->GetScanner()->IchLimTok());
            }
            else
            {
                formals.Prepend(pid);
            }

            this->GetScanner()->Scan();

            if (m_token.tk == tkAsg && m_scriptContext->GetConfig()->IsES6DefaultArgsEnabled())
            {
                this->GetScanner()->Scan();
                // We can avoid building the AST since we are just checking the default expression.
                ParseNodePtr pnodeInit = ParseExpr<false>(koplCma);
                Assert(pnodeInit == nullptr);
            }

            if (m_token.tk != tkComma)
            {
                break;
            }
            this->GetScanner()->ScanForcingPid();

            if (m_token.tk == tkRParen && m_scriptContext->GetConfig()->IsES7TrailingCommaEnabled())
            {
                break;
            }
        }
    }
    Assert(m_token.tk == tkRParen);
}

void Parser::FinishFncNode(ParseNodeFnc * pnodeFnc, bool fAllowIn)
{
    AnalysisAssert(pnodeFnc);

    // Finish the AST for a function that was deferred earlier, but which we decided
    // to finish after the fact.
    // We assume that the name(s) and arg(s) have already got parse nodes, so
    // we just have to do the function body.

    // Save the current next function Id, and resume from the old one.
    Js::LocalFunctionId * nextFunctionIdSave = m_nextFunctionId;
    Js::LocalFunctionId tempNextFunctionId = pnodeFnc->functionId + 1;
    this->m_nextFunctionId = &tempNextFunctionId;

    ParseNodeFnc * pnodeFncSave = m_currentNodeFunc;
    uint *pnestedCountSave = m_pnestedCount;
    int32* pAstSizeSave = m_pCurrentAstSize;

    m_currentNodeFunc = pnodeFnc;
    m_pCurrentAstSize = &(pnodeFnc->astSize);

    pnodeFnc->nestedCount = 0;
    m_pnestedCount = &pnodeFnc->nestedCount;

    bool fLambda = pnodeFnc->IsLambda();
    bool fMethod = pnodeFnc->IsMethod();

    // Cue up the parser to the start of the function body.
    if (pnodeFnc->pnodeName)
    {
        // Skip the name(s).
        this->GetScanner()->SetCurrentCharacter(pnodeFnc->pnodeName->ichLim, pnodeFnc->lineNumber);
    }
    else
    {
        this->GetScanner()->SetCurrentCharacter(pnodeFnc->ichMin, pnodeFnc->lineNumber);

        if (fMethod)
        {
            // Method. Skip identifier name, computed property name, "async", "get", "set", and '*' or '(' characters.
            for (;;)
            {
                this->GetScanner()->Scan();
                // '[' character indicates a computed property name for this method. We should consume it.
                if (m_token.tk == tkLBrack)
                {
                    // We don't care what the name expr is.
                    this->GetScanner()->Scan();
                    ParseExpr<false>();
                    Assert(m_token.tk == tkRBrack);
                    continue;
                }
                // Quit scanning ahead when we reach a '(' character which opens the arg list.
                if (m_token.tk == tkLParen)
                {
                    break;
                }
            }
        }
        else if (pnodeFnc->IsAccessor())
        {
            // Getter/setter. The node text starts with the name, so eat that.
            this->GetScanner()->ScanNoKeywords();
        }
        else if (!fLambda)
        {
            // Anonymous function. Skip "async", "function", and '(' or '*' characters.
            for (;;)
            {
                this->GetScanner()->Scan();
                if (m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async)
                {
                    Assert(pnodeFnc->IsAsync());
                    continue;
                }
                // Quit scanning ahead when we reach a 'function' keyword which precedes the arg list.
                if (m_token.tk == tkFUNCTION)
                {
                    break;
                }
                Assert(m_token.tk == tkLParen || m_token.tk == tkStar);
            }
        }
    }

    // switch scanner to treat 'yield' as keyword in generator functions
    // or as an identifier in non-generator functions
    bool fPreviousYieldIsKeyword = this->GetScanner()->SetYieldIsKeywordRegion(pnodeFnc && pnodeFnc->IsGenerator());
    bool fPreviousAwaitIsKeyword = this->GetScanner()->SetAwaitIsKeywordRegion(pnodeFnc && pnodeFnc->IsAsync());

    // Skip the arg list.
    if (!fMethod)
    {
        // If this is a method, we've already advanced to the '(' token.
        this->GetScanner()->Scan();
    }
    if (m_token.tk == tkStar)
    {
        Assert(pnodeFnc->IsGenerator());
        this->GetScanner()->ScanNoKeywords();
    }
    if (fLambda && m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async)
    {
        Assert(pnodeFnc->IsAsync());
        this->GetScanner()->ScanNoKeywords();
    }
    Assert(m_token.tk == tkLParen || (fLambda && m_token.tk == tkID));
    this->GetScanner()->ScanNoKeywords();

    if (m_token.tk != tkRParen && m_token.tk != tkDArrow)
    {
        for (;;)
        {
            if (m_token.tk == tkEllipsis)
            {
                this->GetScanner()->ScanNoKeywords();
            }

            if (m_token.tk == tkID)
            {
                this->GetScanner()->ScanNoKeywords();

                if (m_token.tk == tkAsg)
                {
                    // Eat the default expression
                    this->GetScanner()->Scan();
                    ParseExpr<false>(koplCma);
                }
            }
            else if (IsPossiblePatternStart())
            {
                ParseDestructuredLiteralWithScopeSave(tkLET, false/*isDecl*/, false /*topLevel*/);
            }
            else
            {
                AssertMsg(false, "Unexpected identifier prefix while fast-scanning formals");
            }

            if (m_token.tk != tkComma)
            {
                break;
            }
            this->GetScanner()->ScanNoKeywords();

            if (m_token.tk == tkRParen && m_scriptContext->GetConfig()->IsES7TrailingCommaEnabled())
            {
                break;
            }
        }
    }

    if (m_token.tk == tkRParen)
    {
        this->GetScanner()->Scan();
    }
    if (fLambda && m_token.tk == tkDArrow)
    {
        this->GetScanner()->Scan();
    }

    // Finish the function body.
    {
        // Note that in IE8- modes, surrounding parentheses are considered part of function body. e.g. "( function x(){} )".
        // We lose that context here since we start from middle of function body. So save and restore source range info.
        const charcount_t ichLim = pnodeFnc->ichLim;
        const size_t cbLim = pnodeFnc->cbLim;

        this->FinishFncDecl(pnodeFnc, NULL, fLambda, /* skipCurlyBraces */ false, fAllowIn);

#if DBG
        // The pnode extent may not match the original extent.
        // We expect this to happen only when there are trailing ")"'s.
        // Consume them and make sure that's all we've got.
        if (pnodeFnc->ichLim != ichLim)
        {
            Assert(pnodeFnc->ichLim < ichLim);
            this->GetScanner()->SetCurrentCharacter(pnodeFnc->ichLim);
            while (this->GetScanner()->IchLimTok() != ichLim)
            {
                this->GetScanner()->ScanNoKeywords();
                Assert(m_token.tk == tkRParen);
            }
        }
#endif
        pnodeFnc->ichLim = ichLim;
        pnodeFnc->cbLim = cbLim;
    }

    m_currentNodeFunc = pnodeFncSave;
    m_pCurrentAstSize = pAstSizeSave;
    m_pnestedCount = pnestedCountSave;
    Assert(m_pnestedCount);

    Assert(tempNextFunctionId == pnodeFnc->deferredParseNextFunctionId);
    this->m_nextFunctionId = nextFunctionIdSave;

    this->GetScanner()->SetYieldIsKeywordRegion(fPreviousYieldIsKeyword);
    this->GetScanner()->SetAwaitIsKeywordRegion(fPreviousAwaitIsKeyword);
}

void Parser::FinishFncDecl(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint, bool fLambda, bool skipCurlyBraces, bool fAllowIn)
{
    LPCOLESTR name = NULL;
    JS_ETW(int32 startAstSize = *m_pCurrentAstSize);
    if (IS_JS_ETW(EventEnabledJSCRIPT_PARSE_METHOD_START()) || PHASE_TRACE1(Js::DeferParsePhase))
    {
        name = GetFunctionName(pnodeFnc, pNameHint);
        m_functionBody = NULL;  // for nested functions we do not want to get the name of the top deferred function return name;
        JS_ETW(EventWriteJSCRIPT_PARSE_METHOD_START(m_sourceContextInfo->dwHostSourceContext, GetScriptContext(), pnodeFnc->functionId, 0, m_parseType, name));
        OUTPUT_TRACE(Js::DeferParsePhase, _u("Parsing function (%s) : %s (%d)\n"), GetParseType(), name, pnodeFnc->functionId);
    }

    JS_ETW_INTERNAL(EventWriteJSCRIPT_PARSE_FUNC(GetScriptContext(), pnodeFnc->functionId, /*Undefer*/FALSE));


    // Do the work of creating an AST for a function body.
    // This is common to the un-deferred case and the case in which we un-defer late in the game.

    Assert(pnodeFnc->nop == knopFncDecl);

    if (fLambda && m_token.tk != tkLCurly)
    {
        ParseExpressionLambdaBody<true>(pnodeFnc, fAllowIn);
    }
    else
    {
        if (!skipCurlyBraces)
        {
            ChkCurTok(tkLCurly, ERRnoLcurly);
        }

        ParseNodePtr * lastNodeRef = nullptr;
        ParseStmtList<true>(&pnodeFnc->pnodeBody, &lastNodeRef, SM_OnFunctionCode, true /* isSourceElementList */);
        // Append an EndCode node.
        AddToNodeList(&pnodeFnc->pnodeBody, &lastNodeRef, CreateNodeForOpT<knopEndCode>());

        if (!skipCurlyBraces)
        {
            ChkCurTokNoScan(tkRCurly, ERRnoRcurly);
        }

        pnodeFnc->ichLim = this->GetScanner()->IchLimTok();
        pnodeFnc->cbLim = this->GetScanner()->IecpLimTok();
    }

#ifdef ENABLE_JS_ETW
    int32 astSize = *m_pCurrentAstSize - startAstSize;
    EventWriteJSCRIPT_PARSE_METHOD_STOP(m_sourceContextInfo->dwHostSourceContext, GetScriptContext(), pnodeFnc->functionId, astSize, m_parseType, name);
#endif
}

ParseNodeVar * Parser::CreateSpecialVarDeclNode(ParseNodeFnc * pnodeFnc, IdentPtr pid)
{
    ParseNodeVar * pnode = InsertVarAtBeginning(pnodeFnc, pid);

    pnode->grfpn |= fpnSpecialSymbol;
    // special symbol must not be global
    pnode->sym->SetIsGlobal(false);

    return pnode;
}

ParseNodeVar * Parser::InsertVarAtBeginning(ParseNodeFnc * pnodeFnc, IdentPtr pid)
{
    ParseNodeVar * pnode = nullptr;

    if (m_ppnodeVar == &pnodeFnc->pnodeVars)
    {
        pnode = CreateVarDeclNode(pid, STVariable, true, pnodeFnc);
    }
    else
    {
        ParseNodePtr * const ppnodeVarSave = m_ppnodeVar;
        m_ppnodeVar = &pnodeFnc->pnodeVars;
        pnode = CreateVarDeclNode(pid, STVariable, true, pnodeFnc);
        m_ppnodeVar = ppnodeVarSave;
    }

    Assert(pnode);
    return pnode;
}

ParseNodeVar * Parser::AddArgumentsNodeToVars(ParseNodeFnc * pnodeFnc)
{
    Assert(!GetCurrentFunctionNode()->IsLambda());

    ParseNodeVar * argNode = InsertVarAtBeginning(pnodeFnc, wellKnownPropertyPids.arguments);

    argNode->grfpn |= PNodeFlags::fpnArguments; // Flag this as the built-in arguments node

    return argNode;
}

void Parser::UpdateArgumentsNode(ParseNodeFnc * pnodeFnc, ParseNodeVar * argNode)
{
    if ((pnodeFnc->grfpn & PNodeFlags::fpnArguments_overriddenInParam) || pnodeFnc->IsLambda())
    {
        // There is a parameter named arguments. So we don't have to create the built-in arguments.
        pnodeFnc->SetHasReferenceableBuiltInArguments(false);
    }
    else if ((pnodeFnc->grfpn & PNodeFlags::fpnArguments_overriddenByDecl) && pnodeFnc->IsBodyAndParamScopeMerged())
    {
        // In non-split scope case there is a var or function definition named arguments in the body
        pnodeFnc->SetHasReferenceableBuiltInArguments(false);
    }
    else
    {
        pnodeFnc->SetHasReferenceableBuiltInArguments(true);
        Assert(argNode);
    }

    if (argNode != nullptr && !argNode->sym->IsArguments())
    {
        // A duplicate definition has updated the declaration node. Need to reset it back.
        argNode->grfpn |= PNodeFlags::fpnArguments;
        argNode->sym->SetDecl(argNode);
    }
}

LPCOLESTR Parser::GetFunctionName(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint)
{
    LPCOLESTR name = nullptr;
    if (pnodeFnc->pnodeName != nullptr)
    {
        Assert(pnodeFnc->pnodeName->nop == knopVarDecl);
        name = pnodeFnc->pnodeName->pid->Psz();
    }
    if (name == nullptr && pNameHint != nullptr)
    {
        name = pNameHint;
    }
    if (name == nullptr && (pnodeFnc->IsLambda() ||
        (!pnodeFnc->IsDeclaration() && !pnodeFnc->IsMethod())))
    {
        name = Js::Constants::AnonymousFunction;
    }
    if (name == nullptr && m_functionBody != nullptr)
    {
        name = m_functionBody->GetExternalDisplayName();
    }
    else if (name == nullptr)
    {
        name = Js::Constants::AnonymousFunction;
    }
    return name;
}

IdentPtr Parser::ParseClassPropertyName(IdentPtr * pidHint)
{
    if (m_token.tk == tkID || m_token.tk == tkStrCon || m_token.IsReservedWord())
    {
        IdentPtr pid;
        if (m_token.tk == tkStrCon)
        {
            if (this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
            {
                Error(ERRES5NoOctal);
            }

            pid = m_token.GetStr();
        }
        else
        {
            pid = m_token.GetIdentifier(this->GetHashTbl());
        }
        *pidHint = pid;
        return pid;
    }
    else if (m_token.tk == tkIntCon)
    {
        if (this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        return this->GetScanner()->PidFromLong(m_token.GetLong());
    }
    else if (m_token.tk == tkFltCon)
    {
        if (this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        return this->GetScanner()->PidFromDbl(m_token.GetDouble());
    }

    Error(ERRnoMemberIdent);
}

LPCOLESTR Parser::ConstructFinalHintNode(IdentPtr pClassName, IdentPtr pMemberName, IdentPtr pGetSet, bool isStatic, uint32* nameLength, uint32* pShortNameOffset, bool isComputedName, LPCOLESTR pMemberNameHint)
{
    if ((pMemberName == nullptr && !isComputedName) ||
        (pMemberNameHint == nullptr && isComputedName) ||
        !CONFIG_FLAG(UseFullName))
    {
        return nullptr;
    }

    LPCOLESTR pFinalName = isComputedName ? pMemberNameHint : pMemberName->Psz();
    uint32 fullNameHintLength = (uint32)wcslen(pFinalName);
    uint32 shortNameOffset = 0;
    if (!isStatic)
    {
        // Add prototype.
        pFinalName = AppendNameHints(wellKnownPropertyPids.prototype, pFinalName, &fullNameHintLength, &shortNameOffset);
    }

    if (pClassName)
    {
        uint32 classNameOffset = 0;
        pFinalName = AppendNameHints(pClassName, pFinalName, &fullNameHintLength, &classNameOffset);
        shortNameOffset += classNameOffset;
    }

    if (pGetSet)
    {
        // displays as get/set prototype.funcname
        uint32 getSetOffset = 0;
        pFinalName = AppendNameHints(pGetSet, pFinalName, &fullNameHintLength, &getSetOffset, true);
        shortNameOffset += getSetOffset;
    }

    *nameLength = fullNameHintLength;
    *pShortNameOffset = shortNameOffset;

    return pFinalName;
}

template<bool buildAST>
ParseNodeClass * Parser::ParseClassDecl(BOOL isDeclaration, LPCOLESTR pNameHint, uint32 *pHintLength, uint32 *pShortNameOffset)
{
    bool hasConstructor = false;
    bool hasExtends = false;
    IdentPtr name = nullptr;
    ParseNodeVar * pnodeName = nullptr;
    ParseNodeFnc * pnodeConstructor = nullptr;
    ParseNodePtr pnodeExtends = nullptr;
    ParseNodePtr pnodeMembers = nullptr;
    ParseNodePtr *lastMemberNodeRef = nullptr;
    ParseNodePtr pnodeStaticMembers = nullptr;
    ParseNodePtr *lastStaticMemberNodeRef = nullptr;
    uint32 nameHintLength = pHintLength ? *pHintLength : 0;
    uint32 nameHintOffset = pShortNameOffset ? *pShortNameOffset : 0;

    ArenaAllocator tempAllocator(_u("ClassMemberNames"), m_nodeAllocator.GetPageAllocator(), Parser::OutOfMemory);

    size_t cbMinConstructor = 0;
    ParseNodeClass * pnodeClass = nullptr;
    if (buildAST)
    {
        pnodeClass = CreateNodeForOpT<knopClassDecl>();

        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Class, m_scriptContext);

        cbMinConstructor = this->GetScanner()->IecpMinTok();
    }

    this->GetScanner()->Scan();
    if (m_token.tk == tkID)
    {
        name = m_token.GetIdentifier(this->GetHashTbl());
        this->GetScanner()->Scan();
    }
    else if (isDeclaration)
    {
        IdentifierExpectedError(m_token);
    }

    if (isDeclaration && name == wellKnownPropertyPids.arguments && GetCurrentBlockInfo()->pnodeBlock->blockType == Function)
    {
        GetCurrentFunctionNode()->grfpn |= PNodeFlags::fpnArguments_overriddenByDecl;
    }

    BOOL strictSave = m_fUseStrictMode;
    m_fUseStrictMode = TRUE;

    ParseNodeVar * pnodeDeclName = nullptr;
    if (isDeclaration)
    {
        pnodeDeclName = CreateBlockScopedDeclNode(name, knopLetDecl);
    }

    ParseNodePtr *ppnodeScopeSave = nullptr;
    ParseNodePtr *ppnodeExprScopeSave = nullptr;

    ParseNodeBlock * pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Regular, ScopeType_Block);
    if (buildAST)
    {
        PushFuncBlockScope(pnodeBlock, &ppnodeScopeSave, &ppnodeExprScopeSave);
        pnodeClass->pnodeBlock = pnodeBlock;
    }

    if (name)
    {
        pnodeName = CreateBlockScopedDeclNode(name, knopConstDecl);
    }

    if (m_token.tk == tkEXTENDS)
    {
        this->GetScanner()->Scan();
        pnodeExtends = ParseTerm<buildAST>();
        hasExtends = true;
    }

    if (m_token.tk != tkLCurly)
    {
        Error(ERRnoLcurly);
    }

    OUTPUT_TRACE_DEBUGONLY(Js::ES6VerboseFlag, _u("Parsing class (%s) : %s\n"), GetParseType(), name ? name->Psz() : _u("anonymous class"));

    RestorePoint beginClass;
    this->GetScanner()->Capture(&beginClass);

    this->GetScanner()->ScanForcingPid();

    IdentPtr pClassNamePid = pnodeName ? pnodeName->pid : nullptr;

    for (;;)
    {
        if (m_token.tk == tkSColon)
        {
            this->GetScanner()->ScanForcingPid();
            continue;
        }
        if (m_token.tk == tkRCurly)
        {
            break;
        }

        bool isStatic = false;
        if (m_token.tk == tkSTATIC)
        {
            // 'static' can be used as an IdentifierName here, even in strict mode code. We need to see the next token before we know
            // if this is being used as a keyword. This is similar to the way we treat 'let' in some cases.
            // See https://tc39.github.io/ecma262/#sec-keywords for more info.

            RestorePoint beginStatic;
            this->GetScanner()->Capture(&beginStatic);

            this->GetScanner()->ScanForcingPid();

            if (m_token.tk == tkLParen)
            {
                this->GetScanner()->SeekTo(beginStatic);
            }
            else
            {
                isStatic = true;
            }
        }

        ushort fncDeclFlags = fFncNoName | fFncMethod | fFncClassMember;
        charcount_t ichMin = 0;
        size_t iecpMin = 0;
        ParseNodePtr pnodeMemberName = nullptr;
        IdentPtr pidHint = nullptr;
        IdentPtr memberPid = nullptr;
        LPCOLESTR pMemberNameHint = nullptr;
        uint32     memberNameHintLength = 0;
        uint32     memberNameOffset = 0;
        bool isComputedName = false;
        bool isAsyncMethod = false;

        if (m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            RestorePoint parsedAsync;
            this->GetScanner()->Capture(&parsedAsync);
            ichMin = this->GetScanner()->IchMinTok();
            iecpMin = this->GetScanner()->IecpMinTok();

            this->GetScanner()->Scan();
            if (m_token.tk == tkLParen || this->GetScanner()->FHadNewLine())
            {
                this->GetScanner()->SeekTo(parsedAsync);
            }
            else
            {
                isAsyncMethod = true;
            }
        }

        bool isGenerator = m_scriptContext->GetConfig()->IsES6GeneratorsEnabled() &&
            m_token.tk == tkStar;
        if (isGenerator)
        {
            fncDeclFlags |= fFncGenerator;
            this->GetScanner()->ScanForcingPid();
        }


        if (m_token.tk == tkLBrack && m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
        {
            // Computed member name: [expr] () { }
            LPCOLESTR emptyHint = nullptr;
            ParseComputedName<buildAST>(&pnodeMemberName, &emptyHint, &pMemberNameHint, &memberNameHintLength, &memberNameOffset);
            isComputedName = true;
        }
        else // not computed name
        {
            memberPid = this->ParseClassPropertyName(&pidHint);
            if (pidHint)
            {
                pMemberNameHint = pidHint->Psz();
                memberNameHintLength = pidHint->Cch();
            }
        }

        if (buildAST && memberPid)
        {
            pnodeMemberName = CreateStrNode(memberPid);
        }

        if (!isStatic && memberPid == wellKnownPropertyPids.constructor)
        {
            if (hasConstructor || isAsyncMethod)
            {
                Error(ERRsyntax);
            }
            hasConstructor = true;
            LPCOLESTR pConstructorName = nullptr;
            uint32  constructorNameLength = 0;
            uint32  constructorShortNameHintOffset = 0;
            if (pnodeName && pnodeName->pid)
            {
                pConstructorName = pnodeName->pid->Psz();
                constructorNameLength = pnodeName->pid->Cch();
            }
            else
            {
                pConstructorName = pNameHint;
                constructorNameLength = nameHintLength;
                constructorShortNameHintOffset = nameHintOffset;
            }

            {
                SuperRestrictionState::State state = hasExtends ? SuperRestrictionState::CallAndPropertyAllowed : SuperRestrictionState::PropertyAllowed;

                // Add the class constructor flag and base class constructor flag if pnodeExtends is nullptr
                fncDeclFlags |= fFncClassConstructor | (hasExtends ? kFunctionNone : fFncBaseClassConstructor);
                pnodeConstructor = ParseFncDeclNoCheckScope<buildAST>(fncDeclFlags, state, pConstructorName, /* needsPIDOnRCurlyScan */ true);
            }

            if (pnodeConstructor->IsGenerator())
            {
                Error(ERRConstructorCannotBeGenerator);
            }

            Assert(constructorNameLength >= constructorShortNameHintOffset);
            // The constructor function will get the same name as class.
            pnodeConstructor->hint = pConstructorName;
            pnodeConstructor->hintLength = constructorNameLength;
            pnodeConstructor->hintOffset = constructorShortNameHintOffset;
            pnodeConstructor->pid = pnodeName && pnodeName->pid ? pnodeName->pid : wellKnownPropertyPids.constructor;
            pnodeConstructor->SetHasNonThisStmt();
            pnodeConstructor->SetHasComputedName();
            pnodeConstructor->SetHasHomeObj();
        }
        else
        {
            ParseNodePtr pnodeMember = nullptr;

            bool isMemberNamedGetOrSet = false;
            RestorePoint beginMethodName;
            this->GetScanner()->Capture(&beginMethodName);
            if (memberPid == wellKnownPropertyPids.get || memberPid == wellKnownPropertyPids.set)
            {
                this->GetScanner()->ScanForcingPid();
            }
            if (m_token.tk == tkLParen)
            {
                this->GetScanner()->SeekTo(beginMethodName);
                isMemberNamedGetOrSet = true;
            }

            if ((memberPid == wellKnownPropertyPids.get || memberPid == wellKnownPropertyPids.set) && !isMemberNamedGetOrSet)
            {
                bool isGetter = (memberPid == wellKnownPropertyPids.get);

                if (m_token.tk == tkLBrack && m_scriptContext->GetConfig()->IsES6ObjectLiteralsEnabled())
                {
                    // Computed get/set member name: get|set [expr] () { }
                    LPCOLESTR emptyHint = nullptr;
                    ParseComputedName<buildAST>(&pnodeMemberName, &emptyHint, &pMemberNameHint, &memberNameHintLength, &memberNameOffset);
                    isComputedName = true;
                }
                else // not computed name
                {
                    memberPid = this->ParseClassPropertyName(&pidHint);
                }

                if ((isStatic ? (memberPid == wellKnownPropertyPids.prototype) : (memberPid == wellKnownPropertyPids.constructor)) || isAsyncMethod)
                {
                    Error(ERRsyntax);
                }
                if (buildAST && memberPid && !isComputedName)
                {
                    pnodeMemberName = CreateStrNode(memberPid);
                }

                ParseNodeFnc * pnodeFnc = nullptr;
                {
                    pnodeFnc = ParseFncDeclNoCheckScope<buildAST>(fncDeclFlags | (isGetter ? fFncNoArg : fFncOneArg),
                        SuperRestrictionState::PropertyAllowed, pidHint ? pidHint->Psz() : nullptr, /* needsPIDOnRCurlyScan */ true);
                }

                pnodeFnc->SetIsStaticMember(isStatic);
                if (isComputedName)
                {
                    pnodeFnc->SetHasComputedName();
                }
                pnodeFnc->SetHasHomeObj();

                if (buildAST)
                {
                    pnodeFnc->SetIsAccessor();
                    pnodeMember = CreateBinNode(isGetter ? knopGetMember : knopSetMember, pnodeMemberName, pnodeFnc);
                    pMemberNameHint = ConstructFinalHintNode(pClassNamePid, pidHint,
                        isGetter ? wellKnownPropertyPids.get : wellKnownPropertyPids.set, isStatic,
                        &memberNameHintLength, &memberNameOffset, isComputedName, pMemberNameHint);
                }
            }
            else
            {
                if (isStatic && (memberPid == wellKnownPropertyPids.prototype))
                {
                    Error(ERRsyntax);
                }

                ParseNodeFnc * pnodeFnc = nullptr;
                {
                    if (isAsyncMethod)
                    {
                        fncDeclFlags |= fFncAsync;
                    }
                    pnodeFnc = ParseFncDeclNoCheckScope<buildAST>(fncDeclFlags, SuperRestrictionState::PropertyAllowed, pidHint ? pidHint->Psz() : nullptr, /* needsPIDOnRCurlyScan */ true);
                    if (isAsyncMethod)
                    {
                        pnodeFnc->cbMin = iecpMin;
                        pnodeFnc->ichMin = ichMin;
                    }
                }
                pnodeFnc->SetIsStaticMember(isStatic);
                if (isComputedName)
                {
                    pnodeFnc->SetHasComputedName();
                }
                pnodeFnc->SetHasHomeObj();

                if (buildAST)
                {
                    pnodeMember = CreateBinNode(knopMember, pnodeMemberName, pnodeFnc);
                    pMemberNameHint = ConstructFinalHintNode(pClassNamePid, pidHint, nullptr /*pgetset*/, isStatic, &memberNameHintLength, &memberNameOffset, isComputedName, pMemberNameHint);
                }
            }

            if (buildAST)
            {
                Assert(memberNameHintLength >= memberNameOffset);
                pnodeMember->AsParseNodeBin()->pnode2->AsParseNodeFnc()->hint = pMemberNameHint; // Fully qualified name
                pnodeMember->AsParseNodeBin()->pnode2->AsParseNodeFnc()->hintLength = memberNameHintLength;
                pnodeMember->AsParseNodeBin()->pnode2->AsParseNodeFnc()->hintOffset = memberNameOffset;
                pnodeMember->AsParseNodeBin()->pnode2->AsParseNodeFnc()->pid = memberPid; // Short name

                AddToNodeList(isStatic ? &pnodeStaticMembers : &pnodeMembers, isStatic ? &lastStaticMemberNodeRef : &lastMemberNodeRef, pnodeMember);
            }
        }
    }

    size_t cbLimConstructor = 0;
    if (buildAST)
    {
        pnodeClass->ichLim = this->GetScanner()->IchLimTok();
        cbLimConstructor = this->GetScanner()->IecpLimTok();
    }

    if (!hasConstructor)
    {
        OUTPUT_TRACE_DEBUGONLY(Js::ES6VerboseFlag, _u("Generating constructor (%s) : %s\n"), GetParseType(), name ? name->Psz() : _u("anonymous class"));

        RestorePoint endClass;
        this->GetScanner()->Capture(&endClass);
        this->GetScanner()->SeekTo(beginClass);

        pnodeConstructor = GenerateEmptyConstructor<buildAST>(pnodeExtends != nullptr);

        if (buildAST)
        {
            if (pClassNamePid)
            {
                pnodeConstructor->hint = pClassNamePid->Psz();
                pnodeConstructor->hintLength = pClassNamePid->Cch();
                pnodeConstructor->hintOffset = 0;
            }
            else
            {
                Assert(nameHintLength >= nameHintOffset);
                pnodeConstructor->hint = pNameHint;
                pnodeConstructor->hintLength = nameHintLength;
                pnodeConstructor->hintOffset = nameHintOffset;
            }
            pnodeConstructor->pid = pClassNamePid;
        }

        this->GetScanner()->SeekTo(endClass);
    }

    if (buildAST)
    {
        pnodeConstructor->cbMin = cbMinConstructor;
        pnodeConstructor->cbLim = cbLimConstructor;
        pnodeConstructor->ichMin = pnodeClass->ichMin;
        pnodeConstructor->ichLim = pnodeClass->ichLim;

        PopFuncBlockScope(ppnodeScopeSave, ppnodeExprScopeSave);

        pnodeClass->pnodeDeclName = pnodeDeclName;
        pnodeClass->pnodeName = pnodeName;
        pnodeClass->pnodeConstructor = pnodeConstructor;
        pnodeClass->pnodeExtends = pnodeExtends;
        pnodeClass->pnodeMembers = pnodeMembers;
        pnodeClass->pnodeStaticMembers = pnodeStaticMembers;
        pnodeClass->isDefaultModuleExport = false;
    }
    FinishParseBlock(pnodeBlock);

    m_fUseStrictMode = strictSave;

    this->GetScanner()->Scan();

    return pnodeClass;
}

template<bool buildAST>
ParseNodePtr Parser::ParseStringTemplateDecl(ParseNodePtr pnodeTagFnc)
{
    ParseNodePtr pnodeStringLiterals = nullptr;
    ParseNodePtr* lastStringLiteralNodeRef = nullptr;
    ParseNodePtr pnodeRawStringLiterals = nullptr;
    ParseNodePtr* lastRawStringLiteralNodeRef = nullptr;
    ParseNodePtr pnodeSubstitutionExpressions = nullptr;
    ParseNodePtr* lastSubstitutionExpressionNodeRef = nullptr;
    ParseNodePtr pnodeTagFncArgs = nullptr;
    ParseNodePtr* lastTagFncArgNodeRef = nullptr;
    ParseNodeStr * stringLiteral = nullptr;
    ParseNodeStr * stringLiteralRaw = nullptr;
    ParseNodeStrTemplate * pnodeStringTemplate = nullptr;
    ParseNode * pnodeReturn = nullptr;
    bool templateClosed = false;
    const bool isTagged = pnodeTagFnc != nullptr;
    uint16 stringConstantCount = 0;
    charcount_t ichMin = 0;

    Assert(m_token.tk == tkStrTmplBasic || m_token.tk == tkStrTmplBegin);

    if (buildAST)
    {
        pnodeReturn = pnodeStringTemplate = CreateNodeForOpT<knopStrTemplate>();
        pnodeStringTemplate->countStringLiterals = 0;
        pnodeStringTemplate->isTaggedTemplate = isTagged ? TRUE : FALSE;

        // If this is a tagged string template, we need to start building the arg list for the call
        if (isTagged)
        {
            ichMin = pnodeTagFnc->ichMin;
            AddToNodeListEscapedUse(&pnodeTagFncArgs, &lastTagFncArgNodeRef, pnodeStringTemplate);
        }

    }
    CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, StringTemplates, m_scriptContext);

    OUTPUT_TRACE_DEBUGONLY(
        Js::StringTemplateParsePhase,
        _u("Starting to parse a string template (%s)...\n\tis tagged = %s\n"),
        GetParseType(),
        isTagged ? _u("true") : _u("false (Raw and cooked strings will not differ!)"));

    // String template grammar
    // `...`   Simple string template
    // `...${  String template beginning
    // }...${  String template middle
    // }...`   String template end
    while (!templateClosed)
    {
        // First, extract the string constant part - we always have one
        if (IsStrictMode() && this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber())
        {
            Error(ERRES5NoOctal);
        }

        // We are not able to pass more than a ushort worth of arguments to the tag
        // so use that as a logical limit on the number of string constant pieces.
        if (stringConstantCount >= Js::Constants::MaxAllowedArgs)
        {
            Error(ERRTooManyArgs);
        }

        // Keep track of the string literal count (must be the same for raw strings)
        // We use this in code gen so we don't need to count the string literals list
        stringConstantCount++;

        // If we are not creating parse nodes, there is no need to create strings
        if (buildAST)
        {
            stringLiteral = CreateStrNode(m_token.GetStr());

            AddToNodeList(&pnodeStringLiterals, &lastStringLiteralNodeRef, stringLiteral);

            // We only need to collect a raw string when we are going to pass the string template to a tag
            if (isTagged)
            {
                // Make the scanner create a PID for the raw string constant for the preceding scan
                IdentPtr pid = this->GetScanner()->GetSecondaryBufferAsPid();

                stringLiteralRaw = CreateStrNode(pid);

                // Should have gotten a raw string literal above
                AddToNodeList(&pnodeRawStringLiterals, &lastRawStringLiteralNodeRef, stringLiteralRaw);
            }
            else
            {
#if DBG
                // Assign the raw string for debug tracing below
                stringLiteralRaw = stringLiteral;
#endif
            }

            OUTPUT_TRACE_DEBUGONLY(
                Js::StringTemplateParsePhase,
                _u("Parsed string constant: \n\tcooked = \"%s\" \n\traw = \"%s\" \n\tdiffer = %d\n"),
                stringLiteral->pid->Psz(),
                stringLiteralRaw->pid->Psz(),
                stringLiteral->pid->Psz() == stringLiteralRaw->pid->Psz() ? 0 : 1);
        }

        switch (m_token.tk)
        {
        case tkStrTmplEnd:
        case tkStrTmplBasic:
            // We do not need to parse an expression for either the end or basic string template tokens
            templateClosed = true;
            break;
        case tkStrTmplBegin:
        case tkStrTmplMid:
        {
            // In the middle or begin string template token case, we need to parse an expression next
            this->GetScanner()->Scan();

            // Parse the contents of the curly braces as an expression
            ParseNodePtr expression = ParseExpr<buildAST>(0);

            // After parsing expression, scan should leave us with an RCurly token.
            // Use the NoScan version so we do not automatically perform a scan - we need to
            // set the scan state before next scan but we don't want to set that state if
            // the token is not as expected since we'll error in that case.
            ChkCurTokNoScan(tkRCurly, ERRnoRcurly);

            // Notify the scanner that it should scan for a middle or end string template token
            this->GetScanner()->SetScanState(Scanner_t::ScanState::ScanStateStringTemplateMiddleOrEnd);
            this->GetScanner()->Scan();

            if (buildAST)
            {
                // If we are going to call the tag function, add this expression into the list of args
                if (isTagged)
                {
                    AddToNodeListEscapedUse(&pnodeTagFncArgs, &lastTagFncArgNodeRef, expression);
                }
                else
                {
                    // Otherwise add it to the substitution expression list
                    // TODO: Store the arguments and substitution expressions in a single list?
                    AddToNodeList(&pnodeSubstitutionExpressions, &lastSubstitutionExpressionNodeRef, expression);
                }
            }

            if (!(m_token.tk == tkStrTmplMid || m_token.tk == tkStrTmplEnd))
            {
                // Scan with ScanState ScanStateStringTemplateMiddleOrEnd should only return
                // tkStrTmpMid/End unless it is EOF or tkScanError
                Assert(m_token.tk == tkEOF || m_token.tk == tkScanError);
                Error(ERRsyntax);
            }

            OUTPUT_TRACE_DEBUGONLY(Js::StringTemplateParsePhase, _u("Parsed expression\n"));
        }
        break;
        default:
            Assert(false);
            break;
        }
    }

    if (buildAST)
    {
        pnodeStringTemplate->pnodeStringLiterals = pnodeStringLiterals;
        pnodeStringTemplate->pnodeStringRawLiterals = pnodeRawStringLiterals;
        pnodeStringTemplate->pnodeSubstitutionExpressions = pnodeSubstitutionExpressions;
        pnodeStringTemplate->countStringLiterals = stringConstantCount;

        // We should still have the last string literal.
        // Use the char offset of the end of that constant as the end of the string template.
        pnodeStringTemplate->ichLim = stringLiteral->ichLim;

        // If this is a tagged template, we now have the argument list and can construct a call node
        if (isTagged)
        {
            // Return the call node here and let the byte code generator Emit the string template automagically
            ParseNodeCall * pnodeCall;
            pnodeReturn = pnodeCall = CreateCallNode(knopCall, pnodeTagFnc, pnodeTagFncArgs, ichMin, pnodeStringTemplate->ichLim);

            // We need to set the arg count explicitly
            pnodeCall->argCount = stringConstantCount;
            pnodeCall->hasDestructuring = m_hasDestructuringPattern;
        }
    }

    this->GetScanner()->Scan();

    return pnodeReturn;
}

LPCOLESTR Parser::FormatPropertyString(LPCOLESTR propertyString, ParseNodePtr pNode, uint32 *fullNameHintLength, uint32 *pShortNameOffset)
{
    // propertyString could be null, such as 'this.foo' =
    // propertyString could be empty, found in pattern as in (-1)[""][(x = z)]

    OpCode op = pNode->nop;
    LPCOLESTR rightNode = nullptr;
    if (propertyString == nullptr)
    {
        propertyString = _u("");
    }

    if (op != knopInt && op != knopFlt && op != knopName && op != knopStr)
    {
        rightNode = _u("");
    }
    else if (op == knopStr)
    {
        return AppendNameHints(propertyString, pNode->AsParseNodeStr()->pid, fullNameHintLength, pShortNameOffset, false, true/*add brackets*/);
    }
    else if (op == knopFlt)
    {
        rightNode = this->GetScanner()->StringFromDbl(pNode->AsParseNodeFloat()->dbl);
    }
    else
    {
        rightNode = op == knopInt ? this->GetScanner()->StringFromLong(pNode->AsParseNodeInt()->lw)
            : pNode->AsParseNodeName()->pid->Psz();
    }

    return AppendNameHints(propertyString, rightNode, fullNameHintLength, pShortNameOffset, false, true/*add brackets*/);
}

LPCOLESTR Parser::ConstructNameHint(ParseNodeBin * pNode, uint32* fullNameHintLength, uint32 *pShortNameOffset)
{
    Assert(pNode != nullptr);
    Assert(pNode->nop == knopDot || pNode->nop == knopIndex);

    // This method recursively visits nodes in the AST and could cause an SOE crash for long knopDot chains.
    // Although this method could be made non-recursive, Emit (ByteCodeEmitter.cpp) hits a stack probe
    // for shorter chains than which cause SOE here, so add a stack probe to throw SOE rather than crash on SOE.
    // Because of that correspondence, use Js::Constants::MinStackByteCodeVisitor (which is used in Emit)
    // for the stack probe here. See OS#14711878.
    PROBE_STACK_NO_DISPOSE(this->m_scriptContext, Js::Constants::MinStackByteCodeVisitor);

    LPCOLESTR leftNode = nullptr;
    if (pNode->pnode1->nop == knopDot || pNode->pnode1->nop == knopIndex)
    {
        leftNode = ConstructNameHint(pNode->pnode1->AsParseNodeBin(), fullNameHintLength, pShortNameOffset);
    }
    else if (pNode->pnode1->nop == knopName && !pNode->pnode1->AsParseNodeName()->IsSpecialName())
    {
        // We need to skip special names like 'this' because those shouldn't be appended to the
        // name hint in the debugger stack trace.
        // function ctor() {
        //   this.func = function() {
        //     // If we break here, the stack should say we are in 'func' and not 'this.func'
        //   }
        // }

        IdentPtr pid = pNode->pnode1->AsParseNodeName()->pid;
        leftNode = pid->Psz();
        *fullNameHintLength = pid->Cch();
        *pShortNameOffset = 0;
    }

    if (pNode->nop == knopIndex)
    {
        return FormatPropertyString(
            leftNode ? leftNode : Js::Constants::AnonymousFunction, // e.g. f()[0] = function () {}
            pNode->pnode2, fullNameHintLength, pShortNameOffset);
    }

    Assert(pNode->pnode2->nop == knopDot || pNode->pnode2->nop == knopName);

    LPCOLESTR rightNode = nullptr;
    bool wrapWithBrackets = false;
    if (pNode->pnode2->nop == knopDot)
    {
        rightNode = ConstructNameHint(pNode->pnode2->AsParseNodeBin(), fullNameHintLength, pShortNameOffset);
    }
    else
    {
        rightNode = pNode->pnode2->AsParseNodeName()->pid->Psz();
        wrapWithBrackets = PNodeFlags::fpnIndexOperator == (pNode->grfpn & PNodeFlags::fpnIndexOperator);
    }
    Assert(rightNode != nullptr);
    return AppendNameHints(leftNode, rightNode, fullNameHintLength, pShortNameOffset, false, wrapWithBrackets);
}

LPCOLESTR Parser::AppendNameHints(LPCOLESTR leftStr, uint32 leftLen, LPCOLESTR rightStr, uint32 rightLen, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace, bool wrapInBrackets)
{
    Assert(rightStr != nullptr);
    Assert(leftLen != 0 || wrapInBrackets);
    Assert(rightLen != 0 || wrapInBrackets);

    bool ignoreDot = rightStr[0] == _u('[') && !wrapInBrackets;//if we wrap in brackets it can be a string literal which can have brackets at the first char
    uint32 totalLength = leftLen + rightLen + ((ignoreDot) ? 1 : 2); // 1 (for dot or [) + 1 (for null termination)

    if (wrapInBrackets)
    {
        totalLength++; //1 for ']';
    }
    WCHAR * finalName = AllocateStringOfLength(totalLength);

    if (leftStr != nullptr && leftLen != 0)
    {
        wcscpy_s(finalName, leftLen + 1, leftStr);
    }

    if (ignoreAddDotWithSpace)
    {
        finalName[leftLen++] = (OLECHAR)_u(' ');
    }
    // mutually exclusive from ignoreAddDotWithSpace which is used for getters/setters

    else if (wrapInBrackets)
    {
        finalName[leftLen++] = (OLECHAR)_u('[');
        finalName[totalLength - 2] = (OLECHAR)_u(']');
    }
    else if (!ignoreDot)
    {
        finalName[leftLen++] = (OLECHAR)_u('.');
    }
    //ignore case falls through
    js_wmemcpy_s(finalName + leftLen, rightLen, rightStr, rightLen);
    finalName[totalLength - 1] = (OLECHAR)_u('\0');

    if (pNameLength != nullptr)
    {
        *pNameLength = totalLength - 1;
    }
    if (pShortNameOffset != nullptr)
    {
        *pShortNameOffset = leftLen;
    }

    return finalName;
}

WCHAR * Parser::AllocateStringOfLength(ULONG length)
{
    Assert(length > 0);
    ULONG totalBytes;
    if (ULongMult(length, sizeof(OLECHAR), &totalBytes) != S_OK)
    {
        Error(ERRnoMemory);
    }
    WCHAR* finalName = (WCHAR*)this->GetHashTbl()->GetAllocator()->Alloc(totalBytes);
    if (finalName == nullptr)
    {
        Error(ERRnoMemory);
    }
    return finalName;
}

LPCOLESTR Parser::AppendNameHints(IdentPtr left, IdentPtr right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace, bool wrapInBrackets)
{
    if (pShortNameOffset != nullptr)
    {
        *pShortNameOffset = 0;
    }

    if (left == nullptr && !wrapInBrackets)
    {
        if (right)
        {
            *pNameLength = right->Cch();
            return right->Psz();
        }
        return nullptr;
    }

    uint32 leftLen = 0;
    LPCOLESTR leftStr = _u("");

    if (left != nullptr) // if wrapInBrackets is true
    {
        leftStr = left->Psz();
        leftLen = left->Cch();
    }

    if (right == nullptr)
    {
        *pNameLength = leftLen;
        return left->Psz();
    }
    uint32 rightLen = right->Cch();

    return AppendNameHints(leftStr, leftLen, right->Psz(), rightLen, pNameLength, pShortNameOffset, ignoreAddDotWithSpace, wrapInBrackets);
}

LPCOLESTR Parser::AppendNameHints(IdentPtr left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace, bool wrapInBrackets)
{
    uint32 rightLen = (right == nullptr) ? 0 : (uint32)wcslen(right);

    if (pShortNameOffset != nullptr)
    {
        *pShortNameOffset = 0;
    }

    Assert(rightLen <= ULONG_MAX); // name hints should not exceed ULONG_MAX characters

    if (left == nullptr && !wrapInBrackets)
    {
        *pNameLength = rightLen;
        return right;
    }

    LPCOLESTR leftStr = _u("");
    uint32 leftLen = 0;

    if (left != nullptr) // if wrapInBrackets is true
    {
        leftStr = left->Psz();
        leftLen = left->Cch();
    }

    if (rightLen == 0 && !wrapInBrackets)
    {
        *pNameLength = leftLen;
        return left->Psz();
    }

    return AppendNameHints(leftStr, leftLen, right, rightLen, pNameLength, pShortNameOffset, ignoreAddDotWithSpace, wrapInBrackets);
}

LPCOLESTR Parser::AppendNameHints(LPCOLESTR left, IdentPtr right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace, bool wrapInBrackets)
{
    uint32 leftLen = (left == nullptr) ? 0 : (uint32)wcslen(left);

    if (pShortNameOffset != nullptr)
    {
        *pShortNameOffset = 0;
    }

    Assert(leftLen <= ULONG_MAX); // name hints should not exceed ULONG_MAX characters

    if (left == nullptr || (leftLen == 0 && !wrapInBrackets))
    {
        if (right != nullptr)
        {
            *pNameLength = right->Cch();
            return right->Psz();
        }
        return nullptr;
    }

    if (right == nullptr)
    {
        *pNameLength = leftLen;
        return left;
    }
    uint32 rightLen = right->Cch();

    return AppendNameHints(left, leftLen, right->Psz(), rightLen, pNameLength, pShortNameOffset, ignoreAddDotWithSpace, wrapInBrackets);
}


LPCOLESTR Parser::AppendNameHints(LPCOLESTR left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace, bool wrapInBrackets)
{
    uint32 leftLen = (left == nullptr) ? 0 : (uint32)wcslen(left);
    uint32 rightLen = (right == nullptr) ? 0 : (uint32)wcslen(right);
    if (pShortNameOffset != nullptr)
    {
        *pShortNameOffset = 0;
    }
    Assert(rightLen <= ULONG_MAX && leftLen <= ULONG_MAX); // name hints should not exceed ULONG_MAX characters

    if (leftLen == 0 && !wrapInBrackets)
    {
        *pNameLength = right ? rightLen : 0;
        return right;
    }

    if (rightLen == 0 && !wrapInBrackets)
    {
        *pNameLength = leftLen;
        return left;
    }

    return AppendNameHints(left, leftLen, right, rightLen, pNameLength, pShortNameOffset, ignoreAddDotWithSpace, wrapInBrackets);
}

/**
 * Emits a spread error if there is no ambiguity, or marks defers the error for
 * when we can determine if it is a rest error or a spread error.
 *
 * The ambiguity arises when we are parsing a lambda parameter list but we have
 * not seen the => token. At this point, we are either in a parenthesized
 * expression or a parameter list, and cannot issue an error until the matching
 * RParen has been scanned.
 *
 * The actual emission of the error happens in ParseExpr, when we first know if
 * the expression is a lambda parameter list or not.
 *
 */
void Parser::DeferOrEmitPotentialSpreadError(ParseNodePtr pnodeT)
{
    if (m_funcParenExprDepth > 0)
    {
        if (m_token.tk == tkRParen)
        {
            if (!m_deferEllipsisError)
            {
                // Capture only the first error instance. Because a lambda will cause a reparse in a formals context, we can assume
                // that this will be a spread error. Nested paren exprs will have their own error instance.
                this->GetScanner()->Capture(&m_deferEllipsisErrorLoc);
                m_deferEllipsisError = true;
            }
        }
        else
        {
            Error(ERRUnexpectedEllipsis);
        }
    }
    else
    {
        Error(ERRInvalidSpreadUse);
    }
}

bool Parser::IsTerminateToken(bool fAllowIn)
{
    return (m_token.tk == tkRCurly ||
        m_token.tk == tkRBrack ||
        m_token.tk == tkRParen ||
        m_token.tk == tkSColon ||
        m_token.tk == tkColon ||
        m_token.tk == tkComma ||
        m_token.tk == tkLimKwd ||
        (m_token.tk == tkIN && fAllowIn) ||
        this->GetScanner()->FHadNewLine());
}

/***************************************************************************
Parse an optional sub expression returning null if there was no expression.
Checks for no expression by looking for a token that can follow an
Expression grammar production.
***************************************************************************/
template<bool buildAST>
bool Parser::ParseOptionalExpr(ParseNodePtr* pnode, bool fUnaryOrParen, int oplMin, BOOL *pfCanAssign, BOOL fAllowIn, BOOL fAllowEllipsis, _Inout_opt_ IdentToken* pToken)
{
    *pnode = nullptr;
    if (IsTerminateToken(!fAllowIn))
    {
        return false;
    }

    IdentToken token;
    ParseNodePtr pnodeT = ParseExpr<buildAST>(oplMin, pfCanAssign, fAllowIn, fAllowEllipsis, nullptr /*pNameHint*/, nullptr /*pHintLength*/, nullptr /*pShortNameOffset*/, &token, fUnaryOrParen);
    // Detect nested function escapes of the pattern "return function(){...}" or "yield function(){...}".
    // Doing so in the parser allows us to disable stack-nested-functions in common cases where an escape
    // is not detected at byte code gen time because of deferred parsing.
    this->MarkEscapingRef(pnodeT, &token);
    if (pToken)
    {
        *pToken = token;
    }
    *pnode = pnodeT;
    return true;
}

/***************************************************************************
Parse a sub expression.
'fAllowIn' indicates if the 'in' operator should be allowed in the initializing
expression ( it is not allowed in the context of the first expression in a  'for' loop).
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseExpr(int oplMin,
    BOOL *pfCanAssign,
    BOOL fAllowIn,
    BOOL fAllowEllipsis,
    LPCOLESTR pNameHint,
    uint32 *pHintLength,
    uint32 *pShortNameOffset,
    _Inout_opt_ IdentToken* pToken,
    bool fUnaryOrParen,
    _Inout_opt_ bool* pfLikelyPattern,
    _Inout_opt_ charcount_t *plastRParen)
{
    Assert(pToken == nullptr || pToken->tk == tkNone); // Must be empty initially
    int opl;
    OpCode nop;
    charcount_t ichMin;
    ParseNodePtr pnode = nullptr;
    ParseNodePtr pnodeT = nullptr;
    BOOL fCanAssign = TRUE;
    bool assignmentStmt = false;
    bool fIsDotOrIndex = false;
    IdentToken term;
    RestorePoint termStart;
    uint32 hintLength = 0;
    uint32 hintOffset = 0;
    BOOL fLikelyPattern = FALSE;

    ParserState parserState;

    if (pHintLength != nullptr)
    {
        hintLength = *pHintLength;
    }

    if (pShortNameOffset != nullptr)
    {
        hintOffset = *pShortNameOffset;
    }

    EnsureStackAvailable();

    // Storing the state here as we need to restore this state back when we need to reparse the grammar under lambda syntax.
    CaptureState(&parserState);

    this->GetScanner()->Capture(&termStart);

    bool savedDeferredInitError = m_hasDeferredShorthandInitError;
    m_hasDeferredShorthandInitError = false;

    // Is the current token a unary operator?
    if (this->GetHashTbl()->TokIsUnop(m_token.tk, &opl, &nop) && nop != knopNone)
    {
        IdentToken operandToken;
        ichMin = this->GetScanner()->IchMinTok();

        if (nop == knopYield)
        {
            if (!this->GetScanner()->YieldIsKeywordRegion() || oplMin > opl)
            {
                // The case where 'yield' is scanned as a keyword (tkYIELD) but the scanner
                // is not treating yield as a keyword (!this->GetScanner()->YieldIsKeywordRegion()) occurs
                // in strict mode non-generator function contexts.
                //
                // That is, 'yield' is a keyword because of strict mode, but YieldExpression
                // is not a grammar production outside of generator functions.
                //
                // Otherwise it is an error for a yield to appear in the context of a higher level
                // binding operator, be it unary or binary.
                Error(ERRsyntax);
            }
            if (m_currentScope->GetScopeType() == ScopeType_Parameter
                || (m_currentScope->GetScopeType() == ScopeType_Block && m_currentScope->GetEnclosingScope()->GetScopeType() == ScopeType_Parameter)) // Check whether this is a class definition inside param scope
            {
                Error(ERRsyntax);
            }
        }
        else if (nop == knopAwait)
        {
            if (!this->GetScanner()->AwaitIsKeywordRegion() ||
                m_currentScope->GetScopeType() == ScopeType_Parameter ||
                (m_currentScope->GetScopeType() == ScopeType_Block && m_currentScope->GetEnclosingScope()->GetScopeType() == ScopeType_Parameter)) // Check whether this is a class definition inside param scope
            {
                // As with the 'yield' keyword, the case where 'await' is scanned as a keyword (tkAWAIT)
                // but the scanner is not treating await as a keyword (!this->GetScanner()->AwaitIsKeyword())
                // occurs in strict mode non-async function contexts.
                //
                // That is, 'await' is a keyword because of strict mode, but AwaitExpression
                // is not a grammar production outside of async functions.
                //
                // Further, await expressions are disallowed within parameter scopes.
                Error(ERRBadAwait);
            }
        }

        this->GetScanner()->Scan();

        if (m_token.tk == tkEllipsis) {
            // ... cannot have a unary prefix.
            Error(ERRUnexpectedEllipsis);
        }

        if (nop == knopYield && !this->GetScanner()->FHadNewLine() && m_token.tk == tkStar)
        {
            this->GetScanner()->Scan();
            nop = knopYieldStar;
        }

        if (nop == knopYield)
        {
            if (!ParseOptionalExpr<buildAST>(&pnodeT, false, opl, NULL, fAllowIn, fAllowEllipsis))
            {
                nop = knopYieldLeaf;
                if (buildAST)
                {
                    pnode = CreateNodeForOpT<knopYieldLeaf>(ichMin, this->GetScanner()->IchLimTok());
                }
            }
        }
        else if (nop == knopAwait && m_token.tk == tkColon)
        {
            Error(ERRAwaitAsLabelInAsync);
        }
        else
        {
            // Disallow spread after a unary operator.
            pnodeT = ParseExpr<buildAST>(opl, &fCanAssign, TRUE, FALSE, nullptr /*hint*/, nullptr /*hintLength*/, nullptr /*hintOffset*/, &operandToken, true, nullptr, plastRParen);
        }

        if (nop != knopYieldLeaf)
        {
            if ((nop == knopIncPre || nop == knopDecPre) && (m_token.tk != tkDArrow))
            {
                if (!fCanAssign && 
                    (m_sourceContextInfo 
                     ? !PHASE_OFF_RAW(Js::EarlyReferenceErrorsPhase, m_sourceContextInfo->sourceContextId, GetCurrentFunctionNode()->functionId)
                     : !PHASE_OFF1(Js::EarlyReferenceErrorsPhase)))
                {
                    Error(JSERR_CantAssignTo);
                }
                TrackAssignment<buildAST>(pnodeT, &operandToken);
                if (buildAST)
                {
                    if (IsStrictMode() && pnodeT->nop == knopName)
                    {
                        CheckStrictModeEvalArgumentsUsage(pnodeT->AsParseNodeName()->pid);
                    }
                }
                else
                {
                    if (IsStrictMode() && operandToken.tk == tkID)
                    {
                        CheckStrictModeEvalArgumentsUsage(operandToken.pid);
                    }
                }
            }
            else if (nop == knopEllipsis)
            {
                if (!fAllowEllipsis)
                {
                    DeferOrEmitPotentialSpreadError(pnodeT);
                }
            }
            else if (m_token.tk == tkExpo)
            {
                //Unary operator on the left hand-side of ** is unexpected, except ++, -- or ...
                Error(ERRInvalidUseofExponentiationOperator);
            }

            if (buildAST)
            {
                //Do not do the folding for Asm in case of KnopPos as we need this to determine the type
                if (nop == knopPos && (pnodeT->nop == knopInt || pnodeT->nop == knopFlt) && !this->m_InAsmMode)
                {
                    // Fold away a unary '+' on a number.
                    pnode = pnodeT;
                }
                else if (nop == knopNeg &&
                    ((pnodeT->nop == knopInt && pnodeT->AsParseNodeInt()->lw != 0) ||
                    (pnodeT->nop == knopFlt && (pnodeT->AsParseNodeFloat()->dbl != 0 || this->m_InAsmMode))))
                {
                    // Fold a unary '-' on a number into the value of the number itself.
                    pnode = pnodeT;
                    if (pnode->nop == knopInt)
                    {
                        pnode->AsParseNodeInt()->lw = -pnode->AsParseNodeInt()->lw;
                    }
                    else
                    {
                        pnode->AsParseNodeFloat()->dbl = -pnode->AsParseNodeFloat()->dbl;
                    }
                }
                else
                {
                    pnode = CreateUniNode(nop, pnodeT);
                    this->CheckArguments(pnode->AsParseNodeUni()->pnode1);
                }
                pnode->ichMin = ichMin;
            }

            if (nop == knopDelete)
            {
                if (IsStrictMode())
                {
                    if ((buildAST && pnode->AsParseNodeUni()->pnode1->IsUserIdentifier()) ||
                        (!buildAST && operandToken.tk == tkID && !this->IsSpecialName(operandToken.pid)))
                    {
                        Error(ERRInvalidDelete);
                    }
                }

                if (buildAST)
                {
                    ParseNodePtr pnode1 = pnode->AsParseNodeUni()->pnode1;
                    if (m_currentNodeFunc)
                    {
                        if (pnode1->nop == knopDot || pnode1->nop == knopIndex)
                        {
                            // If we delete an arguments property, use the conservative,
                            // heap-allocated arguments object.
                            this->CheckArguments(pnode1->AsParseNodeBin()->pnode1);
                        }
                    }
                }
            }
        }

        fCanAssign = FALSE;
    }
    else
    {
        ichMin = this->GetScanner()->IchMinTok();
        pnode = ParseTerm<buildAST>(TRUE, pNameHint, &hintLength, &hintOffset, &term, fUnaryOrParen, TRUE, &fCanAssign, IsES6DestructuringEnabled() ? &fLikelyPattern : nullptr, &fIsDotOrIndex, plastRParen);
        if (pfLikelyPattern != nullptr)
        {
            *pfLikelyPattern = !!fLikelyPattern;
        }

        if (m_token.tk == tkDArrow
            // If we have introduced shorthand error above in the ParseExpr, we need to reset if next token is the assignment.
            || (m_token.tk == tkAsg && oplMin <= koplAsg))
        {
            m_hasDeferredShorthandInitError = false;
        }

        if (m_token.tk == tkAsg && oplMin <= koplAsg && fLikelyPattern)
        {
            this->GetScanner()->SeekTo(termStart);

            // As we are reparsing from the beginning of the destructured literal we need to reset the Block IDs as well to make sure the Block IDs
            // on the pidref stack match.
            int saveNextBlockId = m_nextBlockId;
            m_nextBlockId = parserState.m_nextBlockId;

            ParseDestructuredLiteralWithScopeSave(tkLCurly, false/*isDecl*/, false /*topLevel*/, DIC_ShouldNotParseInitializer);

            // Restore the Block ID at the end of the reparsing so it matches the one at the end of the first pass. We need to do this 
            // because we don't parse initializers during reparse and there may be additional blocks (e.g. a class declaration)
            // in the initializers that will cause the next Block ID at the end of the reparsing to be different.
            m_nextBlockId = saveNextBlockId;

            if (buildAST)
            {
                this->SetHasDestructuringPattern(true);
                pnode = ConvertToPattern(pnode);
            }
        }

        if (buildAST)
        {
            pNameHint = NULL;
            if (pnode->nop == knopName)
            {
                IdentPtr pid = pnode->AsParseNodeName()->pid;
                pNameHint = pid->Psz();
                hintLength = pid->Cch();
                hintOffset = 0;
            }
            else if (pnode->nop == knopDot || pnode->nop == knopIndex)
            {
                if (CONFIG_FLAG(UseFullName))
                {
                    pNameHint = ConstructNameHint(pnode->AsParseNodeBin(), &hintLength, &hintOffset);
                }
                else
                {
                    ParseNodePtr pnodeName = pnode;
                    while (pnodeName->nop == knopDot)
                    {
                        pnodeName = pnodeName->AsParseNodeBin()->pnode2;
                    }

                    if (pnodeName->nop == knopName)
                    {
                        IdentPtr pid = pnode->AsParseNodeName()->pid;
                        pNameHint = pid->Psz();
                        hintLength = pid->Cch();
                        hintOffset = 0;
                    }
                }
            }
        }

        // Check for postfix unary operators.
        if (!this->GetScanner()->FHadNewLine() &&
            (tkInc == m_token.tk || tkDec == m_token.tk))
        {
            if (!fCanAssign && 
                (m_sourceContextInfo 
                 ? !PHASE_OFF_RAW(Js::EarlyReferenceErrorsPhase, m_sourceContextInfo->sourceContextId, GetCurrentFunctionNode()->functionId)
                 : !PHASE_OFF1(Js::EarlyReferenceErrorsPhase)))
            {
                Error(JSERR_CantAssignTo);
            }
            TrackAssignment<buildAST>(pnode, &term);
            fCanAssign = FALSE;
            if (buildAST)
            {
                if (IsStrictMode() && pnode->nop == knopName)
                {
                    CheckStrictModeEvalArgumentsUsage(pnode->AsParseNodeName()->pid);
                }
                this->CheckArguments(pnode);
                pnode = CreateUniNode(tkInc == m_token.tk ? knopIncPost : knopDecPost, pnode);
                pnode->ichLim = this->GetScanner()->IchLimTok();
            }
            else
            {
                if (IsStrictMode() && term.tk == tkID)
                {
                    CheckStrictModeEvalArgumentsUsage(term.pid);
                }
                // This expression is not an identifier
                term.tk = tkNone;
            }
            this->GetScanner()->Scan();
        }
    }

    // Process a sequence of operators and operands.
    for (;;)
    {
        if (!this->GetHashTbl()->TokIsBinop(m_token.tk, &opl, &nop) || nop == knopNone)
        {
            break;
        }
        if (!fAllowIn && nop == knopIn)
        {
            break;
        }
        Assert(opl != koplNo);

        if (opl == koplAsg)
        {
            if (m_token.tk != tkDArrow)
            {
                // Assignment operator. These are the only right associative
                // binary operators. We also need to special case the left
                // operand - it should only be a LeftHandSideExpression.
                Assert(ParseNode::Grfnop(nop) & fnopAsg || nop == knopFncDecl);
                TrackAssignment<buildAST>(pnode, &term);
                if (buildAST)
                {
                    if (IsStrictMode() && pnode->nop == knopName)
                    {
                        CheckStrictModeEvalArgumentsUsage(pnode->AsParseNodeName()->pid);
                    }

                    // Assignment stmt of the form "this.<id> = <expr>"
                    if (nop == knopAsg
                        && pnode->nop == knopDot
                        && pnode->AsParseNodeBin()->pnode1->nop == knopName
                        && pnode->AsParseNodeBin()->pnode1->AsParseNodeName()->pid == wellKnownPropertyPids._this
                        && pnode->AsParseNodeBin()->pnode2->nop == knopName)
                    {
                        if (pnode->AsParseNodeBin()->pnode2->AsParseNodeName()->pid != wellKnownPropertyPids.__proto__)
                        {
                            assignmentStmt = true;
                        }
                    }
                }
                else
                {
                    if (IsStrictMode() && term.tk == tkID)
                    {
                        CheckStrictModeEvalArgumentsUsage(term.pid);
                    }
                }
            }

            if (opl < oplMin)
            {
                break;
            }
            if (m_token.tk != tkDArrow && !fCanAssign && 
                (m_sourceContextInfo 
                 ? !PHASE_OFF_RAW(Js::EarlyReferenceErrorsPhase, m_sourceContextInfo->sourceContextId, GetCurrentFunctionNode()->functionId)
                 : !PHASE_OFF1(Js::EarlyReferenceErrorsPhase)))
            {
                Error(JSERR_CantAssignTo);
                // No recovery necessary since this is a semantic, not structural, error.
            }
        }
        else if (opl == koplExpo)
        {
            // ** operator is right associative
            if (opl < oplMin)
            {
                break;
            }

        }
        else if (opl <= oplMin)
        {
            break;
        }

        // This expression is not an identifier
        term.tk = tkNone;

        // Precedence is high enough. Consume the operator token.
        this->GetScanner()->Scan();
        fCanAssign = !!fLikelyPattern;

        // Special case the "?:" operator
        if (nop == knopQmark)
        {
            pnodeT = ParseExpr<buildAST>(koplAsg, NULL, fAllowIn);
            ChkCurTok(tkColon, ERRnoColon);
            ParseNodePtr pnodeT2 = ParseExpr<buildAST>(koplAsg, NULL, fAllowIn, 0, nullptr, nullptr, nullptr, nullptr, false, nullptr, plastRParen);
            if (buildAST)
            {
                pnode = CreateTriNode(nop, pnode, pnodeT, pnodeT2);
                this->CheckArguments(pnode->AsParseNodeTri()->pnode2);
                this->CheckArguments(pnode->AsParseNodeTri()->pnode3);
            }
        }
        else if (nop == knopFncDecl)
        {
            ushort flags = fFncLambda;
            size_t iecpMin = 0;
            bool isAsyncMethod = false;

            RestoreStateFrom(&parserState);

            this->GetScanner()->SeekTo(termStart);
            if (m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
            {
                ichMin = this->GetScanner()->IchMinTok();
                iecpMin = this->GetScanner()->IecpMinTok();

                this->GetScanner()->Scan();
                if ((m_token.tk == tkID || m_token.tk == tkLParen) && !this->GetScanner()->FHadNewLine())
                {
                    flags |= fFncAsync;
                    isAsyncMethod = true;
                }
                else
                {
                    this->GetScanner()->SeekTo(termStart);
                }
            }
            pnode = ParseFncDeclNoCheckScope<buildAST>(flags, SuperRestrictionState::Disallowed, nullptr, /* needsPIDOnRCurlyScan = */false, /* fUnaryOrParen = */ false, fAllowIn);
            if (isAsyncMethod)
            {
                pnode->AsParseNodeFnc()->cbMin = iecpMin;
                pnode->ichMin = ichMin;
            }

            // ArrowFunction/AsyncArrowFunction is part of AssignmentExpression, which should terminate the expression unless followed by a comma
            if (m_token.tk != tkComma && m_token.tk != tkIN)
            {
                if (!(IsTerminateToken(false)))
                {
                    Error(ERRnoSemic);
                }
                break;
            }
        }
        else // a binary operator
        {
            if (nop == knopComma && m_token.tk == tkRParen)
            {
                // Trailing comma
                this->GetScanner()->Capture(&m_deferCommaErrorLoc);
                m_deferCommaError = true;
                break;
            }

            ParseNode* pnode1 = pnode;

            // Parse the operand, make a new node, and look for more
            IdentToken token;
            ParseNode* pnode2 = ParseExpr<buildAST>(
                opl, nullptr, fAllowIn, FALSE, pNameHint, &hintLength, &hintOffset, &token, false, nullptr, plastRParen);

            // Detect nested function escapes of the pattern "o.f = function(){...}" or "o[s] = function(){...}".
            // Doing so in the parser allows us to disable stack-nested-functions in common cases where an escape
            // is not detected at byte code gen time because of deferred parsing.
            if (fIsDotOrIndex && nop == knopAsg)
            {
                this->MarkEscapingRef(pnodeT, &token);
            }

            if (buildAST)
            {
                Assert(pnode2 != nullptr);
                if (pnode2->nop == knopFncDecl)
                {
                    Assert(hintLength >= hintOffset);
                    pnode2->AsParseNodeFnc()->hint = pNameHint;
                    pnode2->AsParseNodeFnc()->hintLength = hintLength;
                    pnode2->AsParseNodeFnc()->hintOffset = hintOffset;

                    if (pnode1->nop == knopDot)
                    {
                        pnode2->AsParseNodeFnc()->isNameIdentifierRef = false;
                    }
                    else if (pnode1->nop == knopName)
                    {
                        PidRefStack *pidRef = pnode1->AsParseNodeName()->pid->GetTopRef();
                        pidRef->isFuncAssignment = true;
                    }
                }
                else if (pnode2->nop == knopClassDecl && pnode1->nop == knopDot)
                {
                    Assert(pnode2->AsParseNodeClass()->pnodeConstructor);

                    if (!pnode2->AsParseNodeClass()->pnodeConstructor->pid)
                    {
                        pnode2->AsParseNodeClass()->pnodeConstructor->isNameIdentifierRef = false;
                    }
                }
                else if (pnode1->nop == knopName && nop == knopIn)
                {
                    PidRefStack* pidRef = pnode1->AsParseNodeName()->pid->GetTopRef();
                    pidRef->SetIsUsedInLdElem(true);
                }

                pnode = CreateBinNode(nop, pnode1, pnode2);
            }
            pNameHint = nullptr;
        }
    }

    if (buildAST)
    {
        if (!assignmentStmt)
        {
            // Don't set the flag for following nodes
            switch (pnode->nop)
            {
            case knopName:
            case knopInt:
            case knopFlt:
            case knopStr:
            case knopRegExp:
            case knopNull:
            case knopFalse:
            case knopTrue:
                break;
            default:
                if (m_currentNodeFunc)
                {
                    m_currentNodeFunc->SetHasNonThisStmt();
                }
                else if (m_currentNodeProg)
                {
                    m_currentNodeProg->SetHasNonThisStmt();
                }
            }
        }
    }

    m_hasDeferredShorthandInitError = m_hasDeferredShorthandInitError || savedDeferredInitError;

    if (NULL != pfCanAssign)
    {
        *pfCanAssign = fCanAssign;
    }

    // Pass back identifier if requested
    if (pToken && term.tk == tkID)
    {
        *pToken = term;
    }

    //Track "obj.a" assignment patterns here - Promote the Assignment state for the property's PID.
    // This includes =, += etc.
    if (pnode != NULL)
    {
        uint nodeType = ParseNode::Grfnop(pnode->nop);
        if (nodeType & fnopAsg)
        {
            if (nodeType & fnopBin)
            {
                ParseNodePtr lhs = pnode->AsParseNodeBin()->pnode1;

                Assert(lhs);
                if (lhs->nop == knopDot)
                {
                    ParseNodePtr propertyNode = lhs->AsParseNodeBin()->pnode2;
                    if (propertyNode->nop == knopName)
                    {
                        propertyNode->AsParseNodeName()->pid->PromoteAssignmentState();
                    }
                }
            }
            else if (nodeType & fnopUni)
            {
                // cases like obj.a++, ++obj.a
                ParseNodePtr lhs = pnode->AsParseNodeUni()->pnode1;
                if (lhs->nop == knopDot)
                {
                    ParseNodePtr propertyNode = lhs->AsParseNodeBin()->pnode2;
                    if (propertyNode->nop == knopName)
                    {
                        propertyNode->AsParseNodeName()->pid->PromoteAssignmentState();
                    }
                }
            }
        }
    }
    return pnode;
}

template<bool buildAST>
void Parser::TrackAssignment(ParseNodePtr pnodeT, IdentToken* pToken)
{
    if (buildAST)
    {
        Assert(pnodeT != nullptr);
        if (pnodeT->nop == knopName)
        {
            PidRefStack *ref = pnodeT->AsParseNodeName()->pid->GetTopRef();
            Assert(ref);
            ref->isAsg = true;
        }
    }
    else
    {
        Assert(pToken != nullptr);
        if (pToken->tk == tkID)
        {
            PidRefStack *ref = pToken->pid->GetTopRef();
            Assert(ref);
            ref->isAsg = true;
        }
    }
}

PidRefStack* Parser::PushPidRef(IdentPtr pid)
{
    ParseNodeFnc* currentFnc = GetCurrentFunctionNode();

    if (this->IsCreatingStateCache())
    {
        IdentPtrSet* capturedNames = currentFnc->EnsureCapturedNames(&m_nodeAllocator);
        capturedNames->AddNew(pid);
    }

    if (PHASE_ON1(Js::ParallelParsePhase))
    {
        // NOTE: the phase check is here to protect perf. See OSG 1020424.
        // In some LS AST-rewrite cases we lose a lot of perf searching the PID ref stack rather
        // than just pushing on the top. This hasn't shown up as a perf issue in non-LS benchmarks.
        return pid->FindOrAddPidRef(&m_nodeAllocator, GetCurrentBlock()->blockId, currentFnc->functionId);
    }

    Assert(GetCurrentBlock() != nullptr);
    AssertMsg(pid != nullptr, "PID should be created");
    PidRefStack *ref = pid->GetTopRef(m_nextBlockId - 1);
    int blockId = GetCurrentBlock()->blockId;
    int funcId = currentFnc->functionId;
    if (!ref || (ref->GetScopeId() < blockId))
    {
        ref = Anew(&m_nodeAllocator, PidRefStack);
        if (ref == nullptr)
        {
            Error(ERRnoMemory);
        }
        pid->PushPidRef(blockId, funcId, ref);
    }
    else if (m_reparsingLambdaParams)
    {
        // If we're reparsing params, then we may have pid refs left behind from the first pass. Make sure we're
        // working with the right ref at this point.
        ref = this->FindOrAddPidRef(pid, blockId, funcId);
        // Fix up the function ID if we're reparsing lambda parameters.
        ref->funcId = funcId;
    }

    return ref;
}

PidRefStack* Parser::FindOrAddPidRef(IdentPtr pid, int scopeId, Js::LocalFunctionId funcId)
{
    PidRefStack *ref = pid->FindOrAddPidRef(&m_nodeAllocator, scopeId, funcId);
    if (ref == NULL)
    {
        Error(ERRnoMemory);
    }
    return ref;
}

void Parser::RemovePrevPidRef(IdentPtr pid, PidRefStack *ref)
{
    PidRefStack *prevRef = pid->RemovePrevPidRef(ref);
    Assert(prevRef);
    if (prevRef->GetSym() == nullptr)
    {
        AllocatorDelete(ArenaAllocator, &m_nodeAllocator, prevRef);
    }
}

void Parser::SetPidRefsInScopeDynamic(IdentPtr pid, int blockId)
{
    PidRefStack *ref = pid->GetTopRef();
    while (ref && ref->GetScopeId() >= blockId)
    {
        ref->SetDynamicBinding();
        ref = ref->prev;
    }
}

ParseNodeBlock* Parser::GetFunctionBlock()
{
    Assert(m_currentBlockInfo != nullptr);
    return m_currentBlockInfo->pBlockInfoFunction->pnodeBlock;
}


ParseNodeBlock* Parser::GetCurrentBlock()
{
    return m_currentBlockInfo != nullptr ? m_currentBlockInfo->pnodeBlock : nullptr;
}

BlockInfoStack* Parser::GetCurrentBlockInfo()
{
    return m_currentBlockInfo;
}

BlockInfoStack* Parser::GetCurrentFunctionBlockInfo()
{
    return m_currentBlockInfo->pBlockInfoFunction;
}

/***************************************************************************
Parse a variable declaration.
'fAllowIn' indicates if the 'in' operator should be allowed in the initializing
expression ( it is not allowed in the context of the first expression in a  'for' loop).
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseVariableDeclaration(
    tokens declarationType, charcount_t ichMin,
    BOOL fAllowIn/* = TRUE*/,
    BOOL* pfForInOk/* = nullptr*/,
    BOOL singleDefOnly/* = FALSE*/,
    BOOL allowInit/* = TRUE*/,
    BOOL isTopVarParse/* = TRUE*/,
    BOOL isFor/* = FALSE*/,
    BOOL* nativeForOk /*= nullptr*/)
{
    ParseNodePtr pnodeThis = nullptr;
    ParseNodePtr pnodeInit;
    ParseNodePtr pnodeList = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;
    LPCOLESTR pNameHint = nullptr;
    uint32     nameHintLength = 0;
    uint32     nameHintOffset = 0;
    Assert(declarationType == tkVAR || declarationType == tkCONST || declarationType == tkLET);

    for (;;)
    {
        if (IsES6DestructuringEnabled() && IsPossiblePatternStart())
        {
            pnodeThis = ParseDestructuredLiteral<buildAST>(declarationType, true, !!isTopVarParse, DIC_None, !!fAllowIn, pfForInOk, nativeForOk);
            if (pnodeThis != nullptr)
            {
                pnodeThis->ichMin = ichMin;
                pnodeThis->SetIsPatternDeclaration();
            }
        }
        else
        {
            if (m_token.tk != tkID)
            {
                IdentifierExpectedError(m_token);
            }

            IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());
            Assert(pid);
            pNameHint = pid->Psz();
            nameHintLength = pid->Cch();
            nameHintOffset = 0;

            if (pid == wellKnownPropertyPids.let && (declarationType == tkCONST || declarationType == tkLET))
            {
                Error(ERRLetIDInLexicalDecl, pnodeThis);
            }

            if (declarationType == tkVAR)
            {
                pnodeThis = CreateVarDeclNode(pid, STVariable);
            }
            else if (declarationType == tkCONST)
            {
                pnodeThis = CreateBlockScopedDeclNode(pid, knopConstDecl);
                CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Const, m_scriptContext);
            }
            else
            {
                pnodeThis = CreateBlockScopedDeclNode(pid, knopLetDecl);
                CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Let, m_scriptContext);
            }

            if (pid == wellKnownPropertyPids.arguments)
            {
                // This var declaration may change the way an 'arguments' identifier in the function is resolved
                if (declarationType == tkVAR)
                {
                    GetCurrentFunctionNode()->grfpn |= PNodeFlags::fpnArguments_varDeclaration;
                }
                else
                {
                    if (GetCurrentBlockInfo()->pnodeBlock->blockType == Function)
                    {
                        // Only override arguments if we are at the function block level.
                        GetCurrentFunctionNode()->grfpn |= PNodeFlags::fpnArguments_overriddenByDecl;
                    }
                }
            }

            if (pnodeThis)
            {
                pnodeThis->ichMin = ichMin;
            }

            this->GetScanner()->Scan();

            if (m_token.tk == tkAsg)
            {
                if (!allowInit)
                {
                    Error(ERRUnexpectedDefault);
                }
                if (pfForInOk && (declarationType == tkLET || declarationType == tkCONST || IsStrictMode()))
                {
                    *pfForInOk = FALSE;
                }

                this->GetScanner()->Scan();
                pnodeInit = ParseExpr<buildAST>(koplCma, nullptr, fAllowIn, FALSE, pNameHint, &nameHintLength, &nameHintOffset);
                if (buildAST)
                {
                    AnalysisAssert(pnodeThis);
                    pnodeThis->AsParseNodeVar()->pnodeInit = pnodeInit;
                    pnodeThis->ichLim = pnodeInit->ichLim;

                    if (pnodeInit->nop == knopFncDecl)
                    {
                        Assert(nameHintLength >= nameHintOffset);
                        pnodeInit->AsParseNodeFnc()->hint = pNameHint;
                        pnodeInit->AsParseNodeFnc()->hintLength = nameHintLength;
                        pnodeInit->AsParseNodeFnc()->hintOffset = nameHintOffset;
                        pnodeThis->AsParseNodeVar()->pid->GetTopRef()->isFuncAssignment = true;
                    }
                    else
                    {
                        this->CheckArguments(pnodeInit);
                    }
                    pNameHint = nullptr;
                }

                //Track var a =, let a= , const a =
                // This is for FixedFields Constant Heuristics
                if (pnodeThis && pnodeThis->AsParseNodeVar()->pnodeInit != nullptr)
                {
                    pnodeThis->AsParseNodeVar()->sym->PromoteAssignmentState();
                }
            }
            else if (declarationType == tkCONST /*pnodeThis->nop == knopConstDecl*/
                && !singleDefOnly
                && !(isFor && TokIsForInOrForOf()))
            {
                Error(ERRUninitializedConst);
            }

            if (m_currentNodeFunc && pnodeThis && pnodeThis->AsParseNodeVar()->sym->GetIsFormal())
            {
                m_currentNodeFunc->SetHasAnyWriteToFormals(true);
            }
        }

        if (singleDefOnly)
        {
            return pnodeThis;
        }

        if (buildAST)
        {
            AddToNodeListEscapedUse(&pnodeList, &lastNodeRef, pnodeThis);
        }

        if (m_token.tk != tkComma)
        {
            return pnodeList;
        }

        if (pfForInOk)
        {
            // don't allow "for (var a, b in c)"
            *pfForInOk = FALSE;
        }
        this->GetScanner()->Scan();
        ichMin = this->GetScanner()->IchMinTok();
    }
}

/***************************************************************************
Parse try-catch-finally statement
***************************************************************************/

// The try-catch-finally tree nests the try-catch within a try-finally.
// This matches the new runtime implementation.
template<bool buildAST>
ParseNodeStmt * Parser::ParseTryCatchFinally()
{
    this->m_tryCatchOrFinallyDepth++;

    ParseNodeTry * pnodeT = ParseTry<buildAST>();
    ParseNodeTryCatch * pnodeTC = nullptr;
    StmtNest stmt;
    bool hasCatch = false;

    if (tkCATCH == m_token.tk)
    {
        hasCatch = true;
        if (buildAST)
        {
            pnodeTC = CreateNodeForOpT<knopTryCatch>();
            pnodeT->pnodeOuter = pnodeTC;
            pnodeTC->pnodeTry = pnodeT;
        }
        PushStmt<buildAST>(&stmt, pnodeTC, knopTryCatch, nullptr);

        ParseNodeCatch * pnodeCatch = ParseCatch<buildAST>();
        if (buildAST)
        {
            pnodeTC->pnodeCatch = pnodeCatch;
        }
        PopStmt(&stmt);
    }
    if (tkFINALLY != m_token.tk)
    {
        if (!hasCatch)
        {
            Error(ERRnoCatch);
        }
        Assert(!buildAST || pnodeTC);
        this->m_tryCatchOrFinallyDepth--;
        return pnodeTC;
    }

    ParseNodeTryFinally * pnodeTF = nullptr;
    if (buildAST)
    {
        pnodeTF = CreateNodeForOpT<knopTryFinally>();
    }
    PushStmt<buildAST>(&stmt, pnodeTF, knopTryFinally, nullptr);
    ParseNodeFinally * pnodeFinally = ParseFinally<buildAST>();
    if (buildAST)
    {
        if (!hasCatch)
        {
            pnodeTF->pnodeTry = pnodeT;
            pnodeT->pnodeOuter = pnodeTF;
        }
        else
        {
            pnodeTF->pnodeTry = CreateNodeForOpT<knopTry>();
            pnodeTF->pnodeTry->pnodeOuter = pnodeTF;
            pnodeTF->pnodeTry->pnodeBody = pnodeTC;
            pnodeTC->pnodeOuter = pnodeTF->pnodeTry;
        }
        pnodeTF->pnodeFinally = pnodeFinally;
    }
    PopStmt(&stmt);
    this->m_tryCatchOrFinallyDepth--;
    return pnodeTF;
}

template<bool buildAST>
ParseNodeTry * Parser::ParseTry()
{
    ParseNodeTry * pnode = nullptr;
    StmtNest stmt;
    Assert(tkTRY == m_token.tk);
    if (buildAST)
    {
        pnode = CreateNodeForOpT<knopTry>();
    }
    this->GetScanner()->Scan();
    if (tkLCurly != m_token.tk)
    {
        Error(ERRnoLcurly);
    }

    PushStmt<buildAST>(&stmt, pnode, knopTry, nullptr);
    ParseNodePtr pnodeBody = ParseStatement<buildAST>();
    if (buildAST)
    {
        pnode->pnodeBody = pnodeBody;
        if (pnode->pnodeBody)
            pnode->ichLim = pnode->pnodeBody->ichLim;
    }
    PopStmt(&stmt);
    return pnode;
}

template<bool buildAST>
ParseNodeFinally * Parser::ParseFinally()
{
    ParseNodeFinally * pnode = nullptr;
    StmtNest stmt;
    Assert(tkFINALLY == m_token.tk);
    if (buildAST)
    {
        pnode = CreateNodeForOpT<knopFinally>();
    }
    this->GetScanner()->Scan();
    if (tkLCurly != m_token.tk)
    {
        Error(ERRnoLcurly);
    }

    PushStmt<buildAST>(&stmt, pnode, knopFinally, nullptr);
    ParseNodePtr pnodeBody = ParseStatement<buildAST>();
    if (buildAST)
    {
        pnode->pnodeBody = pnodeBody;
        if (!pnode->pnodeBody)
            // Will only occur due to error correction.
            pnode->pnodeBody = CreateNodeForOpT<knopEmpty>();
        else
            pnode->ichLim = pnode->pnodeBody->ichLim;
    }
    PopStmt(&stmt);

    return pnode;
}

template<bool buildAST>
ParseNodeCatch * Parser::ParseCatch()
{
    ParseNodePtr *ppnodeExprScopeSave = nullptr;
    ParseNodeCatch * pnode = nullptr;
    ParseNodeBlock * pnodeCatchScope = nullptr;
    StmtNest stmt;
    IdentPtr pidCatch = nullptr;

    if (tkCATCH == m_token.tk)
    {
        charcount_t ichMin;
        if (buildAST)
        {
            ichMin = this->GetScanner()->IchMinTok();
        }
        this->GetScanner()->Scan(); //catch
        ChkCurTok(tkLParen, ERRnoLparen); //catch(

        bool isPattern = false;
        if (tkID != m_token.tk)
        {
            isPattern = IsES6DestructuringEnabled() && IsPossiblePatternStart();
            if (!isPattern)
            {
                IdentifierExpectedError(m_token);
            }
        }

        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopCatch>(ichMin);
            pnode->pnodeNext = nullptr;
            PushStmt<buildAST>(&stmt, pnode, knopCatch, nullptr);
        }

        pnodeCatchScope = StartParseBlock<buildAST>(PnodeBlockType::Regular, isPattern ? ScopeType_CatchParamPattern : ScopeType_Catch);

        if (buildAST)
        {
            // Add this catch to the current scope list.

            if (m_ppnodeExprScope)
            {
                Assert(*m_ppnodeExprScope == nullptr);
                *m_ppnodeExprScope = pnode;
                m_ppnodeExprScope = &pnode->pnodeNext;
            }
            else
            {
                Assert(m_ppnodeScope);
                Assert(*m_ppnodeScope == nullptr);
                *m_ppnodeScope = pnode;
                m_ppnodeScope = &pnode->pnodeNext;
            }

            // Keep a list of function expressions (not declarations) at this scope.

            ppnodeExprScopeSave = m_ppnodeExprScope;
            m_ppnodeExprScope = &pnode->pnodeScopes;
            pnode->pnodeScopes = nullptr;
        }

        if (isPattern)
        {
            ParseNodePtr pnodePattern = ParseDestructuredLiteral<buildAST>(tkLET, true /*isDecl*/, true /*topLevel*/, DIC_ForceErrorOnInitializer);
            if (buildAST)
            {
                pnode->SetParam(CreateParamPatternNode(pnodePattern));
                Scope *scope = pnodeCatchScope->scope;
                pnode->scope = scope;
            }
        }
        else
        {
            if (IsStrictMode())
            {
                IdentPtr pid = m_token.GetIdentifier(this->GetHashTbl());
                if (pid == wellKnownPropertyPids.eval)
                {
                    Error(ERREvalUsage);
                }
                else if (pid == wellKnownPropertyPids.arguments)
                {
                    Error(ERRArgsUsage);
                }
            }

            pidCatch = m_token.GetIdentifier(this->GetHashTbl());
            PidRefStack *ref = this->FindOrAddPidRef(pidCatch, GetCurrentBlock()->blockId, GetCurrentFunctionNode()->functionId);

            ParseNodeName * pnodeParam = CreateNameNode(pidCatch);
            pnodeParam->SetSymRef(ref);

            const char16 *name = reinterpret_cast<const char16*>(pidCatch->Psz());
            int nameLength = pidCatch->Cch();
            SymbolName const symName(name, nameLength);
            Symbol *sym = Anew(&m_nodeAllocator, Symbol, symName, pnodeParam, STVariable);
            if (sym == nullptr)
            {
                Error(ERRnoMemory);
            }
            sym->SetPid(pidCatch);
            Assert(ref->GetSym() == nullptr);
            ref->SetSym(sym);

            Scope *scope = pnodeCatchScope->scope;
            scope->AddNewSymbol(sym);

            if (buildAST)
            {
                pnode->SetParam(pnodeParam);
                pnode->scope = scope;
            }

            this->GetScanner()->Scan();
        }

        charcount_t ichLim;
        if (buildAST)
        {
            ichLim = this->GetScanner()->IchLimTok();
        }
        ChkCurTok(tkRParen, ERRnoRparen); //catch(id[:expr])

        if (tkLCurly != m_token.tk)
        {
            Error(ERRnoLcurly);
        }

        ParseNodePtr pnodeBody = ParseStatement<buildAST>();  //catch(id[:expr]) {block}
        if (buildAST)
        {
            pnode->pnodeBody = pnodeBody;
            pnode->ichLim = ichLim;
        }

        if (pnodeCatchScope != nullptr)
        {
            FinishParseBlock(pnodeCatchScope);
        }

        if (pnodeCatchScope->GetCallsEval() || pnodeCatchScope->GetChildCallsEval())
        {
            GetCurrentBlock()->SetChildCallsEval(true);
        }

        if (pnodeCatchScope->GetCallsEval())
        {
            pnodeBody->AsParseNodeBlock()->SetCallsEval(true);
        }
        if (pnodeCatchScope->GetChildCallsEval())
        {
            pnodeBody->AsParseNodeBlock()->SetChildCallsEval(true);
        }

        if (buildAST)
        {
            PopStmt(&stmt);

            // Restore the lists of function expression scopes.

            Assert(m_ppnodeExprScope);
            Assert(*m_ppnodeExprScope == nullptr);
            m_ppnodeExprScope = ppnodeExprScopeSave;
        }
    }
    return pnode;
}

template<bool buildAST>
ParseNodeCase * Parser::ParseCase(ParseNodePtr *ppnodeBody)
{
    ParseNodeCase * pnodeT = nullptr;

    charcount_t ichMinT = this->GetScanner()->IchMinTok();
    this->GetScanner()->Scan();
    ParseNodePtr pnodeExpr = ParseExpr<buildAST>();
    charcount_t ichLim = this->GetScanner()->IchLimTok();

    ChkCurTok(tkColon, ERRnoColon);

    if (buildAST)
    {
        pnodeT = CreateNodeForOpT<knopCase>(ichMinT);
        pnodeT->pnodeExpr = pnodeExpr;
        pnodeT->ichLim = ichLim;
    }
    ParseStmtList<buildAST>(ppnodeBody);

    return pnodeT;
}

/***************************************************************************
Parse a single statement. Digest a trailing semicolon.
***************************************************************************/
template<bool buildAST>
ParseNodePtr Parser::ParseStatement()
{
    ParseNodePtr pnode = nullptr;
    LabelId* pLabelIdList = nullptr;
    charcount_t ichMin = 0;
    size_t iecpMin = 0;
    StmtNest stmt;
    StmtNest *pstmt;
    BOOL fForInOrOfOkay;
    BOOL fCanAssign;
    IdentPtr pid;
    uint fnop;
    bool expressionStmt = false;
    bool isAsyncMethod = false;
    bool labelledStatement = false;
    tokens tok;
#if EXCEPTION_RECOVERY
    ParseNodeTryCatch * pParentTryCatch = nullptr;
    ParseNodeBlock * pTryBlock = nullptr;
    ParseNodeTry * pTry = nullptr;
    ParseNodeBlock * pParentTryCatchBlock = nullptr;

    StmtNest stmtTryCatchBlock;
    StmtNest stmtTryCatch;
    StmtNest stmtTry;
    StmtNest stmtTryBlock;
#endif

    if (buildAST)
    {
#if EXCEPTION_RECOVERY
        if (Js::Configuration::Global.flags.SwallowExceptions)
        {
            // If we're swallowing exceptions, surround this statement with a try/catch block:
            //
            //   Before: x.y = 3;
            //   After:  try { x.y = 3; } catch(__ehobj) { }
            //
            // This is done to force the runtime to recover from exceptions at the most granular
            // possible point.  Recovering from EH dramatically improves coverage of testing via
            // fault injection.


            // create and push the try-catch node
            pParentTryCatchBlock = CreateBlockNode();
            PushStmt<buildAST>(&stmtTryCatchBlock, pParentTryCatchBlock, knopBlock, nullptr);
            pParentTryCatch = CreateNodeForOpT<knopTryCatch>();
            PushStmt<buildAST>(&stmtTryCatch, pParentTryCatch, knopTryCatch, nullptr);

            // create and push a try node
            pTry = CreateNodeForOpT<knopTry>();
            PushStmt<buildAST>(&stmtTry, pTry, knopTry, nullptr);
            pTryBlock = CreateBlockNode();
            PushStmt<buildAST>(&stmtTryBlock, pTryBlock, knopBlock, nullptr);
            // these nodes will be closed after the statement is parsed.
        }
#endif // EXCEPTION_RECOVERY
    }

    EnsureStackAvailable();

LRestart:
    tok = m_token.tk;

    switch (tok)
    {
    case tkEOF:
        if (labelledStatement)
        {
            Error(ERRLabelFollowedByEOF);
        }
        if (buildAST)
        {
            pnode = nullptr;
        }
        break;

    case tkFUNCTION:
    {
    LFunctionStatement:
        if (m_grfscr & fscrDeferredFncExpression)
        {
            // The top-level deferred function body was defined by a function expression whose parsing was deferred. We are now
            // parsing it, so unset the flag so that any nested functions are parsed normally. This flag is only applicable the
            // first time we see it.
            m_grfscr &= ~fscrDeferredFncExpression;
            pnode = ParseFncDeclNoCheckScope<buildAST>(isAsyncMethod ? fFncAsync : fFncNoFlgs);
        }
        else
        {
            pnode = ParseFncDeclCheckScope<buildAST>(fFncDeclaration | (isAsyncMethod ? fFncAsync : fFncNoFlgs));
        }

        Assert(pnode != nullptr);
        ParseNodeFnc* pNodeFnc = (ParseNodeFnc*)pnode;
        if (labelledStatement)
        {
            if (IsStrictMode())
            {
                Error(ERRFunctionAfterLabelInStrict);
            }
            else if (pNodeFnc->IsAsync())
            {
                Error(ERRLabelBeforeAsyncFncDeclaration);
            }
            else if (pNodeFnc->IsGenerator())
            {
                Error(ERRLabelBeforeGeneratorDeclaration);
            }
        }
        
        if (isAsyncMethod)
        {
            pnode->AsParseNodeFnc()->cbMin = iecpMin;
            pnode->ichMin = ichMin;
        }
        break;
    }

    case tkCLASS:
        if (labelledStatement)
        {
            Error(ERRLabelBeforeClassDeclaration);
        }
        else if (m_scriptContext->GetConfig()->IsES6ClassAndExtendsEnabled())
        {
            pnode = ParseClassDecl<buildAST>(TRUE, nullptr, nullptr, nullptr);
        }
        else
        {
            goto LDefaultToken;
        }
        break;

    case tkID:
        if (m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.let)
        {
            if (labelledStatement)
            {
                Error(ERRLabelBeforeLexicalDeclaration);
            }
            // We see "let" at the start of a statement. This could either be a declaration or an identifier
            // reference. The next token determines which.
            RestorePoint parsedLet;
            this->GetScanner()->Capture(&parsedLet);
            ichMin = this->GetScanner()->IchMinTok();

            this->GetScanner()->Scan();
            if (this->NextTokenConfirmsLetDecl())
            {
                pnode = ParseVariableDeclaration<buildAST>(tkLET, ichMin);
                goto LNeedTerminator;
            }
            this->GetScanner()->SeekTo(parsedLet);
        }
        else if (m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
        {
            RestorePoint parsedAsync;
            this->GetScanner()->Capture(&parsedAsync);
            ichMin = this->GetScanner()->IchMinTok();
            iecpMin = this->GetScanner()->IecpMinTok();

            this->GetScanner()->Scan();
            if (m_token.tk == tkFUNCTION && !this->GetScanner()->FHadNewLine())
            {
                isAsyncMethod = true;
                goto LFunctionStatement;
            }
            this->GetScanner()->SeekTo(parsedAsync);
        }
        goto LDefaultToken;

    case tkCONST:
    case tkLET:
        if (labelledStatement)
        {
            Error(ERRLabelBeforeLexicalDeclaration);
        }
        ichMin = this->GetScanner()->IchMinTok();

        this->GetScanner()->Scan();
        pnode = ParseVariableDeclaration<buildAST>(tok, ichMin);
        goto LNeedTerminator;

    case tkVAR:
        ichMin = this->GetScanner()->IchMinTok();

        this->GetScanner()->Scan();
        pnode = ParseVariableDeclaration<buildAST>(tok, ichMin);
        goto LNeedTerminator;

    case tkFOR:
    {
        ParseNodeBlock * pnodeBlock = nullptr;
        ParseNodePtr *ppnodeScopeSave = nullptr;
        ParseNodePtr *ppnodeExprScopeSave = nullptr;

        ichMin = this->GetScanner()->IchMinTok();
        ChkNxtTok(tkLParen, ERRnoLparen);
        pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Regular, ScopeType_Block);
        if (buildAST)
        {
            PushFuncBlockScope(pnodeBlock, &ppnodeScopeSave, &ppnodeExprScopeSave);
        }

        RestorePoint startExprOrIdentifier;
        fForInOrOfOkay = TRUE;
        fCanAssign = TRUE;
        tok = m_token.tk;
        BOOL nativeForOkay = TRUE;

        ParseNodePtr pnodeT;
        switch (tok)
        {
        case tkID:
            if (m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.let)
            {
                // We see "let" in the init part of a for loop. This could either be a declaration or an identifier
                // reference. The next token determines which.
                RestorePoint parsedLet;
                this->GetScanner()->Capture(&parsedLet);
                auto ichMinInner = this->GetScanner()->IchMinTok();

                this->GetScanner()->Scan();
                if (IsPossiblePatternStart())
                {
                    this->GetScanner()->Capture(&startExprOrIdentifier);
                }
                if (this->NextTokenConfirmsLetDecl() && m_token.tk != tkIN)
                {
                    pnodeT = ParseVariableDeclaration<buildAST>(tkLET, ichMinInner
                        , /*fAllowIn = */FALSE
                        , /*pfForInOk = */&fForInOrOfOkay
                        , /*singleDefOnly*/FALSE
                        , /*allowInit*/TRUE
                        , /*isTopVarParse*/TRUE
                        , /*isFor*/TRUE
                        , &nativeForOkay);
                    break;
                }
                this->GetScanner()->SeekTo(parsedLet);
            }
            goto LDefaultTokenFor;
        case tkLET:
        case tkCONST:
        case tkVAR:
        {
            auto ichMinInner = this->GetScanner()->IchMinTok();

            this->GetScanner()->Scan();
            if (IsPossiblePatternStart())
            {
                this->GetScanner()->Capture(&startExprOrIdentifier);
            }
            pnodeT = ParseVariableDeclaration<buildAST>(tok, ichMinInner
                , /*fAllowIn = */FALSE
                , /*pfForInOk = */&fForInOrOfOkay
                , /*singleDefOnly*/FALSE
                , /*allowInit*/TRUE
                , /*isTopVarParse*/TRUE
                , /*isFor*/TRUE
                , &nativeForOkay);
        }
        break;
        case tkSColon:
            pnodeT = nullptr;
            fForInOrOfOkay = FALSE;
            break;
        default:
        {
        LDefaultTokenFor:
            RestorePoint exprStart;
            tokens beforeToken = tok;
            this->GetScanner()->Capture(&exprStart);
            if (IsPossiblePatternStart())
            {
                this->GetScanner()->Capture(&startExprOrIdentifier);
            }
            bool fLikelyPattern = false;
            if (IsES6DestructuringEnabled() && (beforeToken == tkLBrack || beforeToken == tkLCurly))
            {
                pnodeT = ParseExpr<buildAST>(koplNo,
                    &fCanAssign,
                    /*fAllowIn = */FALSE,
                    /*fAllowEllipsis*/FALSE,
                    /*pHint*/nullptr,
                    /*pHintLength*/nullptr,
                    /*pShortNameOffset*/nullptr,
                    /*pToken*/nullptr,
                    /**fUnaryOrParen*/false,
                    &fLikelyPattern);
            }
            else
            {
                pnodeT = ParseExpr<buildAST>(koplNo, &fCanAssign, /*fAllowIn = */FALSE);
            }

            // We would veryfiy the grammar as destructuring grammar only when  for..in/of case. As in the native for loop case the above ParseExpr call
            // has already converted them appropriately.
            if (fLikelyPattern && TokIsForInOrForOf())
            {
                this->GetScanner()->SeekTo(exprStart);
                ParseDestructuredLiteralWithScopeSave(tkNone, false/*isDecl*/, false /*topLevel*/, DIC_None, false /*allowIn*/);

                if (buildAST)
                {
                    pnodeT = ConvertToPattern(pnodeT);
                }
            }
            if (buildAST)
            {
                Assert(pnodeT);
                pnodeT->isUsed = false;
            }
        }
        break;
        }

        if (TokIsForInOrForOf())
        {
            bool isForOf = (m_token.tk != tkIN);
            Assert(!isForOf || (m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.of));

            if ((buildAST && nullptr == pnodeT) || !fForInOrOfOkay)
            {
                if (isForOf)
                {
                    Error(ERRForOfNoInitAllowed);
                }
                else
                {
                    Error(ERRForInNoInitAllowed);
                }
            }
            if (!fCanAssign && 
                (m_sourceContextInfo 
                 ? !PHASE_OFF_RAW(Js::EarlyReferenceErrorsPhase, m_sourceContextInfo->sourceContextId, GetCurrentFunctionNode()->functionId)
                 : !PHASE_OFF1(Js::EarlyReferenceErrorsPhase)))
            {
                Error(ERRInvalidLHSInFor);
            }

            this->GetScanner()->Scan();
            ParseNodePtr pnodeObj = ParseExpr<buildAST>(isForOf ? koplCma : koplNo);
            charcount_t ichLim = this->GetScanner()->IchLimTok();
            ChkCurTok(tkRParen, ERRnoRparen);

            ParseNodeForInOrForOf * pnodeForInOrForOf = nullptr;
            if (buildAST)
            {
                if (isForOf)
                {
                    pnodeForInOrForOf = CreateNodeForOpT<knopForOf>(ichMin);
                }
                else
                {
                    pnodeForInOrForOf = CreateNodeForOpT<knopForIn>(ichMin);
                }
                pnodeForInOrForOf->pnodeBlock = pnodeBlock;
                pnodeForInOrForOf->pnodeLval = pnodeT;
                pnodeForInOrForOf->pnodeObj = pnodeObj;
                pnodeForInOrForOf->ichLim = ichLim;

                TrackAssignment<true>(pnodeT, nullptr);
            }
            PushStmt<buildAST>(&stmt, pnodeForInOrForOf, isForOf ? knopForOf : knopForIn, pLabelIdList);
            ParseNodePtr pnodeBody = ParseStatement<buildAST>();

            if (buildAST)
            {
                pnodeForInOrForOf->pnodeBody = pnodeBody;
                pnode = pnodeForInOrForOf;
            }
            PopStmt(&stmt);
        }
        else
        {
            if (!nativeForOkay)
            {
                Error(ERRDestructInit);
            }

            ChkCurTok(tkSColon, ERRnoSemic);
            ParseNodePtr pnodeCond = nullptr;
            if (m_token.tk != tkSColon)
            {
                pnodeCond = ParseExpr<buildAST>();
                if (m_token.tk != tkSColon)
                {
                    Error(ERRnoSemic);
                }
            }

            tokens tk;
            tk = this->GetScanner()->Scan();

            ParseNodePtr pnodeIncr = nullptr;
            if (tk != tkRParen)
            {
                pnodeIncr = ParseExpr<buildAST>();
                if (pnodeIncr)
                {
                    pnodeIncr->isUsed = false;
                }
            }

            charcount_t ichLim = this->GetScanner()->IchLimTok();

            ChkCurTok(tkRParen, ERRnoRparen);

            ParseNodeFor * pnodeFor = nullptr;
            if (buildAST)
            {
                pnodeFor = CreateNodeForOpT<knopFor>(ichMin);
                pnodeFor->pnodeBlock = pnodeBlock;
                pnodeFor->pnodeInverted = nullptr;
                pnodeFor->pnodeInit = pnodeT;
                pnodeFor->pnodeCond = pnodeCond;
                pnodeFor->pnodeIncr = pnodeIncr;
                pnodeFor->ichLim = ichLim;
            }
            PushStmt<buildAST>(&stmt, pnodeFor, knopFor, pLabelIdList);
            ParseNodePtr pnodeBody = ParseStatement<buildAST>();
            if (buildAST)
            {
                pnodeFor->pnodeBody = pnodeBody;
                pnode = pnodeFor;
            }
            PopStmt(&stmt);
        }

        if (buildAST)
        {
            PopFuncBlockScope(ppnodeScopeSave, ppnodeExprScopeSave);
        }

        FinishParseBlock(pnodeBlock);

        break;
    }

    case tkSWITCH:
    {
        BOOL fSeenDefault = FALSE;
        ParseNodeBlock * pnodeBlock = nullptr;
        ParseNodePtr *ppnodeScopeSave = nullptr;
        ParseNodePtr *ppnodeExprScopeSave = nullptr;

        ichMin = this->GetScanner()->IchMinTok();
        ChkNxtTok(tkLParen, ERRnoLparen);
        ParseNodePtr pnodeVal = ParseExpr<buildAST>();
        charcount_t ichLim = this->GetScanner()->IchLimTok();

        ChkCurTok(tkRParen, ERRnoRparen);
        ChkCurTok(tkLCurly, ERRnoLcurly);

        ParseNodeSwitch * pnodeSwitch = nullptr;
        if (buildAST)
        {
            pnodeSwitch = CreateNodeForOpT<knopSwitch>(ichMin);
        }
        PushStmt<buildAST>(&stmt, pnodeSwitch, knopSwitch, pLabelIdList);
        pnodeBlock = StartParseBlock<buildAST>(PnodeBlockType::Regular, ScopeType_Block);

        ParseNodeCase ** ppnodeCase = nullptr;
        if (buildAST)
        {
            pnodeSwitch->pnodeVal = pnodeVal;
            pnodeSwitch->pnodeBlock = pnodeBlock;
            pnodeSwitch->ichLim = ichLim;
            PushFuncBlockScope(pnodeSwitch->pnodeBlock, &ppnodeScopeSave, &ppnodeExprScopeSave);

            pnodeSwitch->pnodeDefault = nullptr;
            ppnodeCase = &pnodeSwitch->pnodeCases;
            pnode = pnodeSwitch;
        }

        for (;;)
        {
            ParseNodeCase * pnodeCase;
            ParseNodePtr pnodeBody = nullptr;
            switch (m_token.tk)
            {
            default:
                goto LEndSwitch;
            case tkCASE:
            {
                pnodeCase = this->ParseCase<buildAST>(&pnodeBody);
                break;
            }
            case tkDEFAULT:
                if (fSeenDefault)
                {
                    Error(ERRdupDefault);
                    // No recovery necessary since this is a semantic, not structural, error
                }
                fSeenDefault = TRUE;
                charcount_t ichMinT = this->GetScanner()->IchMinTok();
                this->GetScanner()->Scan();
                charcount_t ichMinInner = this->GetScanner()->IchLimTok();
                ChkCurTok(tkColon, ERRnoColon);
                if (buildAST)
                {
                    pnodeCase = CreateNodeForOpT<knopCase>(ichMinT);
                    pnodeSwitch->pnodeDefault = pnodeCase;
                    pnodeCase->ichLim = ichMinInner;
                    pnodeCase->pnodeExpr = nullptr;
                }
                ParseStmtList<buildAST>(&pnodeBody);
                break;
            }
            // Create a block node to contain the statement list for this case.
            // This helps us insert byte code to return the right value from
            // global/eval code.
            ParseNodeBlock * pnodeFakeBlock = CreateBlockNode();
            if (buildAST)
            {
                if (pnodeBody)
                {
                    pnodeFakeBlock->ichMin = pnodeCase->ichMin;
                    pnodeFakeBlock->ichLim = pnodeCase->ichLim;
                    pnodeCase->pnodeBody = pnodeFakeBlock;
                    pnodeCase->pnodeBody->grfpn |= PNodeFlags::fpnSyntheticNode; // block is not a user specifier block
                    pnodeCase->pnodeBody->pnodeStmt = pnodeBody;
                }
                else
                {
                    pnodeCase->pnodeBody = nullptr;
                }
                *ppnodeCase = pnodeCase;
                ppnodeCase = &pnodeCase->pnodeNext;
            }
        }
    LEndSwitch:
        ChkCurTok(tkRCurly, ERRnoRcurly);
        if (buildAST)
        {
            *ppnodeCase = nullptr;
            PopFuncBlockScope(ppnodeScopeSave, ppnodeExprScopeSave);
            FinishParseBlock(pnode->AsParseNodeSwitch()->pnodeBlock);
        }
        else
        {
            FinishParseBlock(pnodeBlock);
        }
        PopStmt(&stmt);

        break;
    }

    case tkWHILE:
    {
        ichMin = this->GetScanner()->IchMinTok();
        ChkNxtTok(tkLParen, ERRnoLparen);
        ParseNodePtr pnodeCond = ParseExpr<buildAST>();
        charcount_t ichLim = this->GetScanner()->IchLimTok();
        ChkCurTok(tkRParen, ERRnoRparen);

        ParseNodeWhile * pnodeWhile = nullptr;
        if (buildAST)
        {
            pnodeWhile = CreateNodeForOpT<knopWhile>(ichMin);
            pnodeWhile->pnodeCond = pnodeCond;
            pnodeWhile->ichLim = ichLim;
        }
        bool stashedDisallowImportExportStmt = m_disallowImportExportStmt;
        m_disallowImportExportStmt = true;
        PushStmt<buildAST>(&stmt, pnodeWhile, knopWhile, pLabelIdList);
        ParseNodePtr pnodeBody = ParseStatement<buildAST>();
        PopStmt(&stmt);

        if (buildAST)
        {
            pnodeWhile->pnodeBody = pnodeBody;
            pnode = pnodeWhile;
        }
        m_disallowImportExportStmt = stashedDisallowImportExportStmt;
        break;
    }

    case tkDO:
    {
        ParseNodeWhile * pnodeWhile = nullptr;
        if (buildAST)
        {
            pnodeWhile = CreateNodeForOpT<knopDoWhile>();
        }
        PushStmt<buildAST>(&stmt, pnodeWhile, knopDoWhile, pLabelIdList);
        this->GetScanner()->Scan();
        bool stashedDisallowImportExportStmt = m_disallowImportExportStmt;
        m_disallowImportExportStmt = true;
        ParseNodePtr pnodeBody = ParseStatement<buildAST>();
        m_disallowImportExportStmt = stashedDisallowImportExportStmt;
        PopStmt(&stmt);
        charcount_t ichMinT = this->GetScanner()->IchMinTok();

        ChkCurTok(tkWHILE, ERRnoWhile);
        ChkCurTok(tkLParen, ERRnoLparen);

        ParseNodePtr pnodeCond = ParseExpr<buildAST>();
        charcount_t ichLim = this->GetScanner()->IchLimTok();
        ChkCurTok(tkRParen, ERRnoRparen);

        if (buildAST)
        {
            pnodeWhile->pnodeBody = pnodeBody;
            pnodeWhile->pnodeCond = pnodeCond;
            pnodeWhile->ichLim = ichLim;
            pnodeWhile->ichMin = ichMinT;
            pnode = pnodeWhile;
        }

        // REVIEW: Allow do...while statements to be embedded in other compound statements like if..else, or do..while?
        //      goto LNeedTerminator;

        // For now just eat the trailing semicolon if present.
        if (m_token.tk == tkSColon)
        {
            if (pnode)
            {
                pnode->grfpn |= PNodeFlags::fpnExplicitSemicolon;
            }
            this->GetScanner()->Scan();
        }
        else if (pnode)
        {
            pnode->grfpn |= PNodeFlags::fpnAutomaticSemicolon;
        }

        break;
    }

    case tkIF:
    {
        ichMin = this->GetScanner()->IchMinTok();
        ChkNxtTok(tkLParen, ERRnoLparen);
        ParseNodePtr pnodeCond = ParseExpr<buildAST>();
        ParseNodeIf * pnodeIf = nullptr;
        if (buildAST)
        {
            pnodeIf = CreateNodeForOpT<knopIf>(ichMin);
            pnodeIf->ichLim = this->GetScanner()->IchLimTok();
            pnodeIf->pnodeCond = pnodeCond;
        }
        ChkCurTok(tkRParen, ERRnoRparen);

        bool stashedDisallowImportExportStmt = m_disallowImportExportStmt;
        m_disallowImportExportStmt = true;
        PushStmt<buildAST>(&stmt, pnodeIf, knopIf, pLabelIdList);
        ParseNodePtr pnodeTrue = ParseStatement<buildAST>();
        ParseNodePtr pnodeFalse = nullptr;
        if (m_token.tk == tkELSE)
        {
            this->GetScanner()->Scan();
            pnodeFalse = ParseStatement<buildAST>();
        }
        if (buildAST)
        {
            pnodeIf->pnodeTrue = pnodeTrue;
            pnodeIf->pnodeFalse = pnodeFalse;
            pnode = pnodeIf;
        }
        PopStmt(&stmt);
        m_disallowImportExportStmt = stashedDisallowImportExportStmt;
        break;
    }

    case tkTRY:
    {
        ParseNodeBlock * pnodeBlock = CreateBlockNode();
        pnodeBlock->grfpn |= PNodeFlags::fpnSyntheticNode; // block is not a user specifier block
        PushStmt<buildAST>(&stmt, pnodeBlock, knopBlock, pLabelIdList);
        ParseNodePtr pnodeStmt = ParseTryCatchFinally<buildAST>();
        if (buildAST)
        {
            pnodeBlock->pnodeStmt = pnodeStmt;
        }
        PopStmt(&stmt);
        pnode = pnodeBlock;
        break;
    }

    case tkWITH:
    {
        if (IsStrictMode())
        {
            Error(ERRES5NoWith);
        }
        if (m_currentNodeFunc)
        {
            GetCurrentFunctionNode()->SetHasWithStmt(); // Used by DeferNested
        }

        ichMin = this->GetScanner()->IchMinTok();
        ChkNxtTok(tkLParen, ERRnoLparen);
        ParseNodePtr pnodeObj = ParseExpr<buildAST>();
        if (!buildAST)
        {
            m_scopeCountNoAst++;
        }
        charcount_t ichLim = this->GetScanner()->IchLimTok();
        ChkCurTok(tkRParen, ERRnoRparen);

        ParseNodeWith * pnodeWith = nullptr;
        if (buildAST)
        {
            pnodeWith = CreateNodeForOpT<knopWith>(ichMin);
        }
        PushStmt<buildAST>(&stmt, pnodeWith, knopWith, pLabelIdList);

        ParseNodePtr *ppnodeExprScopeSave = nullptr;
        if (buildAST)
        {
            pnodeWith->pnodeObj = pnodeObj;
            this->CheckArguments(pnodeWith->pnodeObj);

            if (m_ppnodeExprScope)
            {
                Assert(*m_ppnodeExprScope == nullptr);
                *m_ppnodeExprScope = pnodeWith;
                m_ppnodeExprScope = &pnodeWith->pnodeNext;
            }
            else
            {
                Assert(m_ppnodeScope);
                Assert(*m_ppnodeScope == nullptr);
                *m_ppnodeScope = pnodeWith;
                m_ppnodeScope = &pnodeWith->pnodeNext;
            }
            pnodeWith->pnodeNext = nullptr;
            pnodeWith->scope = nullptr;

            ppnodeExprScopeSave = m_ppnodeExprScope;
            m_ppnodeExprScope = &pnodeWith->pnodeScopes;
            pnodeWith->pnodeScopes = nullptr;

            pnodeWith->ichLim = ichLim;
            pnode = pnodeWith;
        }

        PushBlockInfo(CreateBlockNode());
        PushDynamicBlock();

        ParseNodePtr pnodeBody = ParseStatement<buildAST>();
        if (buildAST)
        {
            pnode->AsParseNodeWith()->pnodeBody = pnodeBody;
            m_ppnodeExprScope = ppnodeExprScopeSave;
        }
        else
        {
            m_scopeCountNoAst--;
        }

        // The dynamic block is not stored in the actual parse tree and so will not
        // be visited by the byte code generator.  Grab the callsEval flag off it and
        // pass on to outer block in case of:
        // with (...) eval(...); // i.e. blockless form of with
        bool callsEval = GetCurrentBlock()->GetCallsEval();
        PopBlockInfo();
        if (callsEval)
        {
            // be careful not to overwrite an existing true with false
            GetCurrentBlock()->SetCallsEval(true);
        }

        PopStmt(&stmt);
        break;
    }

    case tkLCurly:
        pnode = ParseBlock<buildAST>(pLabelIdList);
        break;

    case tkSColon:
        pnode = nullptr;
        this->GetScanner()->Scan();
        break;

    case tkBREAK:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopBreak>();
        }
        fnop = fnopBreak;
        goto LGetJumpStatement;

    case tkCONTINUE:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopContinue>();
        }
        fnop = fnopContinue;

    LGetJumpStatement:
        this->GetScanner()->ScanForcingPid();
        if (tkID == m_token.tk && !this->GetScanner()->FHadNewLine())
        {
            // Labeled break or continue.
            pid = m_token.GetIdentifier(this->GetHashTbl());
            if (buildAST)
            {
                ParseNodeJump * pnodeJump = pnode->AsParseNodeJump();
                pnodeJump->hasExplicitTarget = true;
                pnodeJump->ichLim = this->GetScanner()->IchLimTok();

                this->GetScanner()->Scan();
                PushStmt<buildAST>(&stmt, pnodeJump, pnodeJump->nop, pLabelIdList);
                Assert(pnodeJump->grfnop == 0);
                for (pstmt = m_pstmtCur; nullptr != pstmt; pstmt = pstmt->pstmtOuter)
                {
                    for (LabelId* label = pstmt->pLabelId; label != nullptr; label = label->next)
                    {
                        if (pid == label->pid)
                        {
                            // Found the label. Make sure we can use it. We can
                            // break out of any statement, but we can only
                            // continue loops.
                            if (fnop == fnopContinue &&
                                !(pstmt->pnodeStmt->Grfnop() & fnop))
                            {
                                Error(ERRbadContinue);
                            }
                            else
                            {
                                pstmt->pnodeStmt->grfnop |= fnop;
                                pnodeJump->pnodeTarget = pstmt->pnodeStmt;
                            }
                            PopStmt(&stmt);
                            goto LNeedTerminator;
                        }
                    }
                    pnodeJump->grfnop |=
                        (pstmt->pnodeStmt->Grfnop() & fnopCleanup);
                }
            }
            else
            {
                this->GetScanner()->Scan();
                for (pstmt = m_pstmtCur; pstmt; pstmt = pstmt->pstmtOuter)
                {
                    LabelId* pLabelId;
                    for (pLabelId = pstmt->pLabelId; pLabelId; pLabelId = pLabelId->next)
                    {

                        if (pid == pLabelId->pid)
                        {
                            // Found the label. Make sure we can use it. We can
                            // break out of any statement, but we can only
                            // continue loops.
                            if (fnop == fnopContinue &&
                                !(ParseNode::Grfnop(pstmt->op) & fnop))
                            {
                                Error(ERRbadContinue);
                            }
                            goto LNeedTerminator;
                        }
                    }
                }
            }
            Error(ERRnoLabel);
        }
        else
        {
            // If we're doing a fast scan, we're not tracking labels, so we can't accurately do this analysis.
            // Let the thread that's doing the full parse detect the error, if there is one.
            if (!this->IsDoingFastScan())
            {
                // Unlabeled break or continue.
                ParseNodeJump * pnodeJump = nullptr;
                if (buildAST)
                {
                    pnodeJump = pnode->AsParseNodeJump();
                    pnodeJump->hasExplicitTarget = false;
                    PushStmt<buildAST>(&stmt, pnodeJump, pnodeJump->nop, pLabelIdList);
                    Assert(pnodeJump->grfnop == 0);
                }

                for (pstmt = m_pstmtCur; nullptr != pstmt; pstmt = pstmt->pstmtOuter)
                {
                    if (buildAST)
                    {
                        AnalysisAssert(pstmt->pnodeStmt);
                        if (pstmt->pnodeStmt->Grfnop() & fnop)
                        {
                            pstmt->pnodeStmt->grfnop |= fnop;
                            pnodeJump->pnodeTarget = pstmt->pnodeStmt;
                            PopStmt(&stmt);
                            goto LNeedTerminator;
                        }
                        pnodeJump->grfnop |=
                            (pstmt->pnodeStmt->Grfnop() & fnopCleanup);
                    }
                    else
                    {
                        if (ParseNode::Grfnop(pstmt->GetNop()) & fnop)
                        {
                            if (!pstmt->isDeferred)
                            {
                                AnalysisAssert(pstmt->pnodeStmt);
                                pstmt->pnodeStmt->grfnop |= fnop;
                            }
                            goto LNeedTerminator;
                        }
                    }
                }
                Error(fnop == fnopBreak ? ERRbadBreak : ERRbadContinue);
            }
            goto LNeedTerminator;
        }

    case tkRETURN:
    {
        ParseNodeReturn * pnodeReturn;
        if (buildAST)
        {
            if (nullptr == m_currentNodeFunc || IsTopLevelModuleFunc())
            {
                Error(ERRbadReturn);
            }
            pnodeReturn = CreateNodeForOpT<knopReturn>();
        }
        this->GetScanner()->Scan();
        ParseNodePtr pnodeExpr = nullptr;
        ParseOptionalExpr<buildAST>(&pnodeExpr, true);

        // Class constructors have special semantics regarding return statements.
        // This might require a reference to 'this'
        if (GetCurrentFunctionNode()->IsClassConstructor())
        {
            ReferenceSpecialName(wellKnownPropertyPids._this);
        }

        if (buildAST)
        {
            pnodeReturn->pnodeExpr = pnodeExpr;
            if (pnodeExpr)
            {
                this->CheckArguments(pnodeReturn->pnodeExpr);
                pnodeReturn->ichLim = pnodeReturn->pnodeExpr->ichLim;
            }
            // See if return should call finally
            PushStmt<buildAST>(&stmt, pnodeReturn, knopReturn, pLabelIdList);
            Assert(pnodeReturn->grfnop == 0);
            for (pstmt = m_pstmtCur; nullptr != pstmt; pstmt = pstmt->pstmtOuter)
            {
                if (pstmt->pnodeStmt->Grfnop() & fnopCleanup)
                {
                    pnodeReturn->grfnop |= fnopCleanup;
                    break;
                }
            }
            PopStmt(&stmt);
            pnode = pnodeReturn;
        }
        goto LNeedTerminator;
    }

    case tkTHROW:
    {
        if (buildAST)
        {
            pnode = CreateUniNode(knopThrow, nullptr);
        }
        this->GetScanner()->Scan();
        ParseNodePtr pnode1 = nullptr;
        if (m_token.tk != tkSColon &&
            m_token.tk != tkRCurly &&
            !this->GetScanner()->FHadNewLine())
        {
            pnode1 = ParseExpr<buildAST>();
        }
        else
        {
            Error(ERRdanglingThrow);
        }

        if (buildAST)
        {
            pnode->AsParseNodeUni()->pnode1 = pnode1;
            if (pnode1)
            {
                this->CheckArguments(pnode->AsParseNodeUni()->pnode1);
                pnode->ichLim = pnode->AsParseNodeUni()->pnode1->ichLim;
            }
        }
        goto LNeedTerminator;
    }

    case tkDEBUGGER:
        if (buildAST)
        {
            pnode = CreateNodeForOpT<knopDebugger>();
        }
        this->GetScanner()->Scan();
        goto LNeedTerminator;

    case tkIMPORT:
        pnode = ParseImport<buildAST>();

        goto LNeedTerminator;

    case tkEXPORT:
    {
        if (!(m_grfscr & fscrIsModuleCode))
        {
            goto LDefaultToken;
        }

        bool needTerminator = false;
        pnode = ParseExportDeclaration<buildAST>(&needTerminator);

        if (needTerminator)
        {
            goto LNeedTerminator;
        }
        else
        {
            break;
        }
    }

LDefaultToken:
    default:
    {
        // First check for a label via lookahead. If not found,
        // rewind and reparse as expression statement.
        if (m_token.tk == tkID)
        {
            RestorePoint idStart;
            this->GetScanner()->Capture(&idStart);

            IdentPtr pidInner = m_token.GetIdentifier(this->GetHashTbl());

            this->GetScanner()->Scan();

            if (m_token.tk == tkColon)
            {
                // We have a label.
                if (LabelExists(pidInner, pLabelIdList))
                {
                    Error(ERRbadLabel);
                }
                LabelId* pLabelId = CreateLabelId(pidInner);
                pLabelId->next = pLabelIdList;
                pLabelIdList = pLabelId;
                this->GetScanner()->Scan();
                labelledStatement = true;
                goto LRestart;
            }

            // No label, rewind back to the tkID and parse an expression
            this->GetScanner()->SeekTo(idStart);
        }

        // Must be an expression statement.
        pnode = ParseExpr<buildAST>();

        if (m_hasDeferredShorthandInitError)
        {
            Error(ERRnoColon);
        }

        if (buildAST)
        {
            expressionStmt = true;

            AnalysisAssert(pnode);
            pnode->isUsed = false;
        }
    }

LNeedTerminator:
    // Need a semicolon, new-line, } or end-of-file.
    // We digest a semicolon if it's there.
    switch (m_token.tk)
    {
    case tkSColon:
        this->GetScanner()->Scan();
        if (pnode != nullptr) pnode->grfpn |= PNodeFlags::fpnExplicitSemicolon;
        break;
    case tkEOF:
    case tkRCurly:
        if (pnode != nullptr) pnode->grfpn |= PNodeFlags::fpnAutomaticSemicolon;
        break;
    default:
        if (!this->GetScanner()->FHadNewLine())
        {
            Error(ERRnoSemic);
        }
        else
        {
            if (pnode != nullptr) pnode->grfpn |= PNodeFlags::fpnAutomaticSemicolon;
        }
        break;
    }
    break;
    }

    if (m_hasDeferredShorthandInitError)
    {
        Error(ERRnoColon);
    }

    if (buildAST)
    {
        // All non expression statements excluded from the "this.x" optimization
        // Another check while parsing expressions
        if (!expressionStmt)
        {
            if (m_currentNodeFunc)
            {
                m_currentNodeFunc->SetHasNonThisStmt();
            }
            else if (m_currentNodeProg)
            {
                m_currentNodeProg->SetHasNonThisStmt();
            }
        }

#if EXCEPTION_RECOVERY
        // close the try/catch block
        if (Js::Configuration::Global.flags.SwallowExceptions)
        {
            // pop the try block and fill in the body
            PopStmt(&stmtTryBlock);
            pTryBlock->pnodeStmt = pnode;
            PopStmt(&stmtTry);
            if (pnode != nullptr)
            {
                pTry->ichLim = pnode->ichLim;
            }
            pTry->pnodeBody = pTryBlock;


            // create a catch block with an empty body
            StmtNest stmtCatch;
            ParseNodeCatch * pCatch;
            pCatch = CreateNodeForOpT<knopCatch>();
            PushStmt<buildAST>(&stmtCatch, pCatch, knopCatch, nullptr);
            pCatch->pnodeBody = nullptr;
            if (pnode != nullptr)
            {
                pCatch->ichLim = pnode->ichLim;
            }
            pCatch->grfnop = 0;
            pCatch->pnodeNext = nullptr;

            // create a fake name for the catch var.
            const WCHAR *uniqueNameStr = _u("__ehobj");
            IdentPtr uniqueName = this->GetHashTbl()->PidHashNameLen(uniqueNameStr, static_cast<int32>(wcslen(uniqueNameStr)));

            pCatch->SetParam(CreateNameNode(uniqueName));

            // Add this catch to the current list. We don't bother adjusting the catch and function expression
            // lists here because the catch is just an empty statement.

            if (m_ppnodeExprScope)
            {
                Assert(*m_ppnodeExprScope == nullptr);
                *m_ppnodeExprScope = pCatch;
                m_ppnodeExprScope = &pCatch->pnodeNext;
            }
            else
            {
                Assert(m_ppnodeScope);
                Assert(*m_ppnodeScope == nullptr);
                *m_ppnodeScope = pCatch;
                m_ppnodeScope = &pCatch->pnodeNext;
            }

            pCatch->pnodeScopes = nullptr;

            PopStmt(&stmtCatch);

            // fill in and pop the try-catch
            pParentTryCatch->pnodeTry = pTry;
            pParentTryCatch->pnodeCatch = pCatch;
            PopStmt(&stmtTryCatch);
            PopStmt(&stmtTryCatchBlock);

            // replace the node that's being returned
            pParentTryCatchBlock->pnodeStmt = pParentTryCatch;
            pnode = pParentTryCatchBlock;
        }
#endif // EXCEPTION_RECOVERY

    }

    return pnode;
}

BOOL
Parser::TokIsForInOrForOf()
{
    return m_token.tk == tkIN ||
        (m_token.tk == tkID &&
            m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.of);
}

/***************************************************************************
Parse a sequence of statements.
***************************************************************************/
template<bool buildAST>
void Parser::ParseStmtList(ParseNodePtr *ppnodeList, ParseNodePtr **pppnodeLast, StrictModeEnvironment smEnvironment, const bool isSourceElementList, bool* strictModeOn)
{
    BOOL doneDirectives = !isSourceElementList; // directives may only exist in a SourceElementList, not a StatementList
    BOOL seenDirectiveContainingOctal = false; // Have we seen an octal directive before a use strict directive?

    BOOL old_UseStrictMode = m_fUseStrictMode;

    ParseNodePtr pnodeStmt;
    ParseNodePtr *lastNodeRef = nullptr;

    if (buildAST)
    {
        Assert(ppnodeList);
        *ppnodeList = nullptr;
    }

    if (CONFIG_FLAG(ForceStrictMode))
    {
        m_fUseStrictMode = TRUE;
    }

    for (;;)
    {
        switch (m_token.tk)
        {
        case tkCASE:
        case tkDEFAULT:
        case tkRCurly:
        case tkEOF:
            if (buildAST && nullptr != pppnodeLast)
            {
                *pppnodeLast = lastNodeRef;
            }
            if (!buildAST)
            {
                m_fUseStrictMode = old_UseStrictMode;
            }
            return;
        }

        if (doneDirectives == FALSE)
        {
            bool isOctalInString = false;
            bool isUseStrictDirective = false;
            bool isUseAsmDirective = false;
            if (smEnvironment != SM_NotUsed && CheckForDirective(&isUseStrictDirective, &isUseAsmDirective, &isOctalInString))
            {
                // Ignore "use asm" statement when not building the AST
                isUseAsmDirective &= buildAST;

                if (isUseStrictDirective)
                {
                    // Functions with non-simple parameter list cannot be made strict mode
                    if (GetCurrentFunctionNode()->HasNonSimpleParameterList())
                    {
                        Error(ERRNonSimpleParamListInStrictMode);
                    }

                    if (seenDirectiveContainingOctal)
                    {
                        // Directives seen before a "use strict" cannot contain an octal.
                        Error(ERRES5NoOctal);
                    }
                    if (!buildAST)
                    {
                        // Turning on strict mode in deferred code.
                        m_fUseStrictMode = TRUE;
                        if (!m_inDeferredNestedFunc)
                        {
                            // Top-level deferred function, so there's a parse node
                            Assert(m_currentNodeFunc != nullptr);
                            m_currentNodeFunc->SetStrictMode();
                        }
                        else if (strictModeOn)
                        {
                            // This turns on strict mode in a deferred function, we need to go back
                            // and re-check duplicated formals.
                            *strictModeOn = true;
                        }
                    }
                    else
                    {
                        if (smEnvironment == SM_OnGlobalCode)
                        {
                            // Turning on strict mode at the top level
                            m_fUseStrictMode = TRUE;
                        }
                        else
                        {
                            // i.e. smEnvironment == SM_OnFunctionCode
                            Assert(m_currentNodeFunc != nullptr);
                            m_currentNodeFunc->SetStrictMode();
                        }
                    }
                }
                else if (isUseAsmDirective)
                {
                    if (smEnvironment != SM_OnGlobalCode) //Top level use asm doesn't mean anything.
                    {
                        // i.e. smEnvironment == SM_OnFunctionCode
                        Assert(m_currentNodeFunc != nullptr);
                        m_currentNodeFunc->SetAsmjsMode();
                        m_currentNodeFunc->SetCanBeDeferred(false);
                        m_InAsmMode = true;

                        CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, AsmJSFunction, m_scriptContext);
                    }
                }
                else if (isOctalInString)
                {
                    seenDirectiveContainingOctal = TRUE;
                }
            }
            else
            {
                // The first time we see anything other than a directive we can have no more directives.
                doneDirectives = TRUE;
            }
        }

        if (nullptr != (pnodeStmt = ParseStatement<buildAST>()))
        {
            if (buildAST)
            {
                AddToNodeList(ppnodeList, &lastNodeRef, pnodeStmt);
            }
        }
    }
}

template <class Fn>
void Parser::FinishFunctionsInScope(ParseNodePtr pnodeScopeList, Fn fn)
{
    Scope * scope;
    Scope * origCurrentScope = this->m_currentScope;
    ParseNodePtr pnodeScope;
    ParseNodeBlock * pnodeBlock;
    for (pnodeScope = pnodeScopeList; pnodeScope;)
    {
        switch (pnodeScope->nop)
        {
        case knopBlock:
        {
            ParseNodeBlock * pnodeBlockScope = pnodeScope->AsParseNodeBlock();
            m_nextBlockId = pnodeBlockScope->blockId + 1;
            PushBlockInfo(pnodeBlockScope);
            scope = pnodeBlockScope->scope;
            if (scope && scope != origCurrentScope)
            {
                PushScope(scope);
            }
            FinishFunctionsInScope(pnodeBlockScope->pnodeScopes, fn);
            if (scope && scope != origCurrentScope)
            {
                BindPidRefs<false>(GetCurrentBlockInfo(), m_nextBlockId - 1);
                PopScope(scope);
            }
            PopBlockInfo();
            pnodeScope = pnodeBlockScope->pnodeNext;
            break;
        }
        case knopFncDecl:
            fn(pnodeScope->AsParseNodeFnc());
            pnodeScope = pnodeScope->AsParseNodeFnc()->pnodeNext;
            break;

        case knopCatch:
            scope = pnodeScope->AsParseNodeCatch()->scope;
            if (scope)
            {
                PushScope(scope);
            }
            pnodeBlock = CreateBlockNode(PnodeBlockType::Regular);
            pnodeBlock->scope = scope;
            PushBlockInfo(pnodeBlock);
            FinishFunctionsInScope(pnodeScope->AsParseNodeCatch()->pnodeScopes, fn);
            if (scope)
            {
                BindPidRefs<false>(GetCurrentBlockInfo(), m_nextBlockId - 1);
                PopScope(scope);
            }
            PopBlockInfo();
            pnodeScope = pnodeScope->AsParseNodeCatch()->pnodeNext;
            break;

        case knopWith:
            PushBlockInfo(CreateBlockNode());
            PushDynamicBlock();
            FinishFunctionsInScope(pnodeScope->AsParseNodeWith()->pnodeScopes, fn);
            PopBlockInfo();
            pnodeScope = pnodeScope->AsParseNodeWith()->pnodeNext;
            break;

        default:
            AssertMsg(false, "Unexpected node with scope list");
            return;
        }
    }
}

// Scripts above this size (minus string literals and comments) will have parsing of
// function bodies deferred.
ULONG Parser::GetDeferralThreshold(bool isProfileLoaded)
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (CONFIG_FLAG(ForceDeferParse) ||
        PHASE_FORCE1(Js::DeferParsePhase) ||
        Js::Configuration::Global.flags.IsEnabled(Js::ForceUndoDeferFlag))
    {
        return 0;
    }
    else if (Js::Configuration::Global.flags.IsEnabled(Js::DeferParseFlag))
    {
        return Js::Configuration::Global.flags.DeferParse;
    }
    else
#endif
    {
        if (isProfileLoaded)
        {
            return DEFAULT_CONFIG_ProfileBasedDeferParseThreshold;
        }
        return DEFAULT_CONFIG_DeferParseThreshold;
    }
}

void Parser::FinishDeferredFunction(ParseNodeBlock * pnodeScopeList)
{
    uint saveNextBlockId = m_nextBlockId;
    m_nextBlockId = pnodeScopeList->blockId + 1;

    FinishFunctionsInScope(pnodeScopeList,
        [this](ParseNodeFnc * pnodeFnc)
    {
        Assert(pnodeFnc->nop == knopFncDecl);

        // Non-simple params (such as default) require a good amount of logic to put vars on appropriate scopes. ParseFncDecl handles it
        // properly (both on defer and non-defer case). This is to avoid write duplicated logic here as well. Function with non-simple-param
        // will remain deferred until they are called.
        if (pnodeFnc->pnodeBody == nullptr && !pnodeFnc->HasNonSimpleParameterList())
        {
            // Go back and generate an AST for this function.
            JS_ETW_INTERNAL(EventWriteJSCRIPT_PARSE_FUNC(this->GetScriptContext(), pnodeFnc->functionId, /*Undefer*/TRUE));

            ParseNodeFnc * pnodeFncSave = this->m_currentNodeFunc;
            this->m_currentNodeFunc = pnodeFnc;

            ParseNodeBlock * pnodeFncExprBlock = nullptr;
            ParseNodePtr pnodeName = pnodeFnc->pnodeName;
            if (pnodeName)
            {
                Assert(pnodeName->nop == knopVarDecl);
                ParseNodeVar * pnodeVarName = pnodeName->AsParseNodeVar();
                Assert(pnodeVarName->pnodeNext == nullptr);

                if (!pnodeFnc->IsDeclaration())
                {
                    // Set up the named function expression symbol so references inside the function can be bound.
                    pnodeFncExprBlock = this->StartParseBlock<true>(PnodeBlockType::Function, ScopeType_FuncExpr);
                    PidRefStack *ref = this->PushPidRef(pnodeVarName->pid);
                    pnodeVarName->symRef = ref->GetSymRef();
                    ref->SetSym(pnodeVarName->sym);

                    Scope *fncExprScope = pnodeFncExprBlock->scope;
                    fncExprScope->AddNewSymbol(pnodeVarName->sym);
                    pnodeFnc->scope = fncExprScope;
                }
            }

            ParseNodeBlock * pnodeBlock = this->StartParseBlock<true>(PnodeBlockType::Parameter, ScopeType_Parameter);
            pnodeFnc->pnodeScopes = pnodeBlock;
            m_ppnodeScope = &pnodeBlock->pnodeScopes;
            pnodeBlock->pnodeStmt = pnodeFnc;

            ParseNodePtr * varNodesList = &pnodeFnc->pnodeVars;
            ParseNodeVar * argNode = nullptr;
            if (!pnodeFnc->IsModule() && !pnodeFnc->IsLambda() && !(pnodeFnc->grfpn & PNodeFlags::fpnArguments_overriddenInParam))
            {
                ParseNodePtr *const ppnodeVarSave = m_ppnodeVar;
                m_ppnodeVar = &pnodeFnc->pnodeVars;

                argNode = this->AddArgumentsNodeToVars(pnodeFnc);

                varNodesList = m_ppnodeVar;
                m_ppnodeVar = ppnodeVarSave;
            }

            // Add the args to the scope, since we won't re-parse those.
            Scope *scope = pnodeBlock->scope;
            uint blockId = GetCurrentBlock()->blockId;
            uint funcId = GetCurrentFunctionNode()->functionId;
            auto addArgsToScope = [&](ParseNodePtr pnodeArg) {
                if (pnodeArg->IsVarLetOrConst())
                {
                    ParseNodeVar * pnodeVarArg = pnodeArg->AsParseNodeVar();

                    PidRefStack *ref = this->FindOrAddPidRef(pnodeVarArg->pid, blockId, funcId);
                    pnodeVarArg->symRef = ref->GetSymRef();
                    if (ref->GetSym() != nullptr)
                    {
                        // Duplicate parameter in a configuration that allows them.
                        // The symbol is already in the scope, just point it to the right declaration.
                        Assert(ref->GetSym() == pnodeVarArg->sym);
                        ref->GetSym()->SetDecl(pnodeVarArg);
                    }
                    else
                    {
                        ref->SetSym(pnodeArg->AsParseNodeVar()->sym);
                        scope->AddNewSymbol(pnodeVarArg->sym);
                    }
                }
            };

            MapFormals(pnodeFnc, addArgsToScope);
            MapFormalsFromPattern(pnodeFnc, addArgsToScope);

            ParseNodeBlock * pnodeInnerBlock = this->StartParseBlock<true>(PnodeBlockType::Function, ScopeType_FunctionBody);
            pnodeFnc->pnodeBodyScope = pnodeInnerBlock;

            // Set the parameter block's child to the function body block.
            *m_ppnodeScope = pnodeInnerBlock;

            ParseNodePtr *ppnodeScopeSave = nullptr;
            ParseNodePtr *ppnodeExprScopeSave = nullptr;

            ppnodeScopeSave = m_ppnodeScope;

            // This synthetic block scope will contain all the nested scopes.
            m_ppnodeScope = &pnodeInnerBlock->pnodeScopes;
            pnodeInnerBlock->pnodeStmt = pnodeFnc;

            // Keep nested function declarations and expressions in the same list at function scope.
            // (Indicate this by nulling out the current function expressions list.)
            ppnodeExprScopeSave = m_ppnodeExprScope;
            m_ppnodeExprScope = nullptr;

            // Shouldn't be any temps in the arg list.
            Assert(*m_ppnodeVar == nullptr);

            // Start the var list.
            m_ppnodeVar = varNodesList;

            if (scope != nullptr)
            {
                Assert(pnodeFnc->IsBodyAndParamScopeMerged());
                blockId = GetCurrentBlock()->blockId;
                funcId = GetCurrentFunctionNode()->functionId;
                scope->ForEachSymbol([this, blockId, funcId](Symbol* paramSym)
                {
                    PidRefStack* ref = this->FindOrAddPidRef(paramSym->GetPid(), blockId, funcId);
                    ref->SetSym(paramSym);
                });
            }

            Assert(m_currentNodeNonLambdaFunc == nullptr);
            m_currentNodeNonLambdaFunc = pnodeFnc;

            this->FinishFncNode(pnodeFnc);

            Assert(pnodeFnc == m_currentNodeNonLambdaFunc);
            m_currentNodeNonLambdaFunc = nullptr;

            m_ppnodeExprScope = ppnodeExprScopeSave;

            Assert(m_ppnodeScope);
            Assert(nullptr == *m_ppnodeScope);
            m_ppnodeScope = ppnodeScopeSave;

            this->FinishParseBlock(pnodeInnerBlock);

            if (!pnodeFnc->IsModule() && (m_token.tk == tkLCurly || !pnodeFnc->IsLambda()))
            {
                UpdateArgumentsNode(pnodeFnc, argNode);
            }

            CreateSpecialSymbolDeclarations(pnodeFnc);

            this->FinishParseBlock(pnodeBlock);
            if (pnodeFncExprBlock)
            {
                this->FinishParseBlock(pnodeFncExprBlock);
            }

            this->m_currentNodeFunc = pnodeFncSave;
        }
    });

    m_nextBlockId = saveNextBlockId;
}

void Parser::InitPids()
{
    wellKnownPropertyPids.arguments = this->GetHashTbl()->PidHashNameLen(g_ssym_arguments.sz, g_ssym_arguments.cch);
    wellKnownPropertyPids.async = this->GetHashTbl()->PidHashNameLen(g_ssym_async.sz, g_ssym_async.cch);
    wellKnownPropertyPids.eval = this->GetHashTbl()->PidHashNameLen(g_ssym_eval.sz, g_ssym_eval.cch);
    wellKnownPropertyPids.get = this->GetHashTbl()->PidHashNameLen(g_ssym_get.sz, g_ssym_get.cch);
    wellKnownPropertyPids.set = this->GetHashTbl()->PidHashNameLen(g_ssym_set.sz, g_ssym_set.cch);
    wellKnownPropertyPids.let = this->GetHashTbl()->PidHashNameLen(g_ssym_let.sz, g_ssym_let.cch);
    wellKnownPropertyPids.constructor = this->GetHashTbl()->PidHashNameLen(g_ssym_constructor.sz, g_ssym_constructor.cch);
    wellKnownPropertyPids.prototype = this->GetHashTbl()->PidHashNameLen(g_ssym_prototype.sz, g_ssym_prototype.cch);
    wellKnownPropertyPids.__proto__ = this->GetHashTbl()->PidHashNameLen(_u("__proto__"), sizeof("__proto__") - 1);
    wellKnownPropertyPids.of = this->GetHashTbl()->PidHashNameLen(_u("of"), sizeof("of") - 1);
    wellKnownPropertyPids.target = this->GetHashTbl()->PidHashNameLen(_u("target"), sizeof("target") - 1);
    wellKnownPropertyPids.as = this->GetHashTbl()->PidHashNameLen(_u("as"), sizeof("as") - 1);
    wellKnownPropertyPids.from = this->GetHashTbl()->PidHashNameLen(_u("from"), sizeof("from") - 1);
    wellKnownPropertyPids._default = this->GetHashTbl()->PidHashNameLen(_u("default"), sizeof("default") - 1);
    wellKnownPropertyPids._starDefaultStar = this->GetHashTbl()->PidHashNameLen(_u("*default*"), sizeof("*default*") - 1);
    wellKnownPropertyPids._star = this->GetHashTbl()->PidHashNameLen(_u("*"), sizeof("*") - 1);
    wellKnownPropertyPids._this = this->GetHashTbl()->PidHashNameLen(_u("*this*"), sizeof("*this*") - 1);
    wellKnownPropertyPids._newTarget = this->GetHashTbl()->PidHashNameLen(_u("*new.target*"), sizeof("*new.target*") - 1);
    wellKnownPropertyPids._super = this->GetHashTbl()->PidHashNameLen(_u("*super*"), sizeof("*super*") - 1);
    wellKnownPropertyPids._superConstructor = this->GetHashTbl()->PidHashNameLen(_u("*superconstructor*"), sizeof("*superconstructor*") - 1);
}

void Parser::RestoreScopeInfo(Js::ScopeInfo * scopeInfo)
{
    if (!scopeInfo)
    {
        return;
    }

    PROBE_STACK_NO_DISPOSE(m_scriptContext, Js::Constants::MinStackByteCodeVisitor);

    RestoreScopeInfo(scopeInfo->GetParentScopeInfo()); // Recursively restore outer func scope info

    scopeInfo->SetScopeId(m_nextBlockId);
    ParseNodeBlock * pnodeScope = nullptr;
    ScopeType scopeType = scopeInfo->GetScopeType();
    PnodeBlockType blockType;
    switch (scopeType)
    {
    case ScopeType_With:
        PushDynamicBlock();
        // fall through
    case ScopeType_Block:
    case ScopeType_Catch:
    case ScopeType_CatchParamPattern:
    case ScopeType_GlobalEvalBlock:
        blockType = PnodeBlockType::Regular;
        break;

    case ScopeType_FunctionBody:
    case ScopeType_FuncExpr:
        blockType = PnodeBlockType::Function;
        break;

    case ScopeType_Parameter:
        blockType = PnodeBlockType::Parameter;
        break;


    default:
        Assert(0);
        return;
    }

    pnodeScope = StartParseBlockWithCapacity<true>(blockType, scopeType, scopeInfo->GetSymbolCount());
    Scope *scope = pnodeScope->scope;
    scope->SetScopeInfo(scopeInfo);
    scopeInfo->ExtractScopeInfo(this, /*nullptr, nullptr,*/ scope);
}

void Parser::FinishScopeInfo(Js::ScopeInfo * scopeInfo)
{
    PROBE_STACK_NO_DISPOSE(m_scriptContext, Js::Constants::MinStackByteCodeVisitor);

    for (; scopeInfo != nullptr; scopeInfo = scopeInfo->GetParentScopeInfo())
    {
        int scopeId = scopeInfo->GetScopeId();

        scopeInfo->GetScope()->ForEachSymbol([this, scopeId](Symbol *sym)
        {
            this->BindPidRefsInScope(sym->GetPid(), sym, scopeId);
        });
        PopScope(scopeInfo->GetScope());
        PopStmt(&m_currentBlockInfo->pstmt);
        PopBlockInfo();
    }
}

/***************************************************************************
Parse the code.
***************************************************************************/
ParseNodeProg * Parser::Parse(LPCUTF8 pszSrc, size_t offset, size_t length, charcount_t charOffset, bool isUtf8, ULONG grfscr, ULONG lineNumber, Js::LocalFunctionId * nextFunctionId, CompileScriptException *pse)
{
    ParseNodeProg * pnodeProg;
    ParseNodePtr *lastNodeRef = nullptr;

    m_nextBlockId = 0;

    bool isDeferred = (grfscr & fscrDeferredFnc) != 0;
    bool isModuleSource = (grfscr & fscrIsModuleCode) != 0;
    bool isGlobalCode = (grfscr & fscrGlobalCode) != 0;

    if (this->m_scriptContext->IsScriptContextInDebugMode()
#ifdef ENABLE_PREJIT
        || Js::Configuration::Global.flags.Prejit
#endif
        || ((grfscr & fscrNoDeferParse) != 0)
        )
    {
        // Don't do deferred parsing if debugger is attached or feature is disabled
        // by command-line switch.
        grfscr &= ~fscrWillDeferFncParse;
    }
    else if (!isGlobalCode &&
        (
            PHASE_OFF1(Js::Phase::DeferEventHandlersPhase) ||
            this->m_scriptContext->IsScriptContextInSourceRundownOrDebugMode()
            )
        )
    {
        // Don't defer event handlers in debug/rundown mode, because we need to register the document,
        // so we need to create a full FunctionBody for the script body.
        grfscr &= ~fscrWillDeferFncParse;
    }

    m_grfscr = grfscr;
    m_length = length;
    m_originalLength = length;
    m_nextFunctionId = nextFunctionId;

    if (m_parseType != ParseType_Deferred)
    {
        JS_ETW(EventWriteJSCRIPT_PARSE_METHOD_START(m_sourceContextInfo->dwHostSourceContext, GetScriptContext(), *m_nextFunctionId, 0, m_parseType, Js::Constants::GlobalFunction));
        OUTPUT_TRACE(Js::DeferParsePhase, _u("Parsing function (%s) : %s (%d)\n"), GetParseType(), Js::Constants::GlobalFunction, *m_nextFunctionId);
    }

    // Give the scanner the source and get the first token
    this->GetScanner()->SetText(pszSrc, offset, length, charOffset, isUtf8, grfscr, lineNumber);
    this->GetScanner()->Scan();

    // Make the main 'knopProg' node
    int32 initSize = 0;
    m_pCurrentAstSize = &initSize;
    pnodeProg = CreateProgNode(isModuleSource, lineNumber);

    if (!isDeferred || (isDeferred && isGlobalCode))
    {
        // In the deferred case, if the global function is deferred parse (which is in no-refresh case),
        // we will re-use the same function body, so start with the correct functionId.
        pnodeProg->functionId = (*m_nextFunctionId)++;
    }

    if (isModuleSource)
    {
        Assert(m_scriptContext->GetConfig()->IsES6ModuleEnabled());

        pnodeProg->AsParseNodeModule()->localExportEntries = nullptr;
        pnodeProg->AsParseNodeModule()->indirectExportEntries = nullptr;
        pnodeProg->AsParseNodeModule()->starExportEntries = nullptr;
        pnodeProg->AsParseNodeModule()->importEntries = nullptr;
        pnodeProg->AsParseNodeModule()->requestedModules = nullptr;
    }

    m_pCurrentAstSize = &(pnodeProg->astSize);

    // initialize parsing variables
    m_currentNodeFunc = nullptr;
    m_currentNodeDeferredFunc = nullptr;
    m_currentNodeProg = pnodeProg;
    m_cactIdentToNodeLookup = 1;

    m_pnestedCount = &pnodeProg->nestedCount;
    m_inDeferredNestedFunc = false;
    m_ppnodeVar = &pnodeProg->pnodeVars;
    SetCurrentStatement(nullptr);
    AssertMsg(m_pstmtCur == nullptr, "Statement stack should be empty when we start parse global code");

    // Create block for const's and let's
    ParseNodeBlock * pnodeGlobalBlock = StartParseBlock<true>(PnodeBlockType::Global, ScopeType_Global);
    pnodeProg->scope = pnodeGlobalBlock->scope;
    ParseNodeBlock * pnodeGlobalEvalBlock = nullptr;

    // Don't track function expressions separately from declarations at global scope.
    m_ppnodeExprScope = nullptr;

    // This synthetic block scope will contain all the nested scopes.    
    pnodeProg->pnodeScopes = pnodeGlobalBlock;
    m_ppnodeScope = &pnodeGlobalBlock->pnodeScopes;

    if ((this->m_grfscr & fscrEvalCode) &&
        !(this->m_functionBody && this->m_functionBody->GetScopeInfo()))
    {
        pnodeGlobalEvalBlock = StartParseBlock<true>(PnodeBlockType::Regular, ScopeType_GlobalEvalBlock);
        pnodeProg->pnodeScopes = pnodeGlobalEvalBlock;
        m_ppnodeScope = &pnodeGlobalEvalBlock->pnodeScopes;
    }

    Js::ScopeInfo *scopeInfo = nullptr;
    if (m_parseType == ParseType_Deferred && m_functionBody)
    {
        // this->m_functionBody can be cleared during parsing, but we need access to the scope info later.
        scopeInfo = m_functionBody->GetScopeInfo();
        if (scopeInfo)
        {
            // Create an enclosing function context.
            m_currentNodeFunc = CreateNodeForOpT<knopFncDecl>();
            m_currentNodeFunc->functionId = m_functionBody->GetLocalFunctionId();
            m_currentNodeFunc->nestedCount = m_functionBody->GetNestedCount();
            m_currentNodeFunc->SetStrictMode(!!this->m_fUseStrictMode);

            this->RestoreScopeInfo(scopeInfo);

            m_currentNodeFunc->SetIsGenerator(scopeInfo->IsGeneratorFunctionBody());
            m_currentNodeFunc->SetIsAsync(scopeInfo->IsAsyncFunctionBody());
        }
    }

    // It's possible for the module global to be defer-parsed in debug scenarios.
    if (isModuleSource && (!isDeferred || (isDeferred && isGlobalCode)))
    {
        ParseNodePtr moduleFunction = GenerateModuleFunctionWrapper<true>();
        pnodeProg->pnodeBody = nullptr;
        AddToNodeList(&pnodeProg->pnodeBody, &lastNodeRef, moduleFunction);
    }
    else
    {
        if (isDeferred && !isGlobalCode)
        {
            // Defer parse for a single function should just parse that one function - there are no other statements.
            ushort flags = fFncNoFlgs;
            size_t iecpMin = 0;
            charcount_t ichMin = 0;
            bool isAsync = false;
            bool isGenerator = false;
            bool isMethod = false;

            // The top-level deferred function body was defined by a function expression whose parsing was deferred. We are now
            // parsing it, so unset the flag so that any nested functions are parsed normally. This flag is only applicable the
            // first time we see it.
            //
            // Normally, deferred functions will be parsed in ParseStatement upon encountering the 'function' token. The first
            // token of the source code of the function may not be a 'function' token though, so we still need to reset this flag
            // for the first function we parse. This can happen in compat modes, for instance, for a function expression enclosed
            // in parentheses, where the legacy behavior was to include the parentheses in the function's source code.
            if (m_grfscr & fscrDeferredFncExpression)
            {
                m_grfscr &= ~fscrDeferredFncExpression;
            }
            else
            {
                flags |= fFncDeclaration;
            }

            if (m_grfscr & fscrDeferredFncIsMethod)
            {
                m_grfscr &= ~fscrDeferredFncIsMethod;
                isMethod = true;
                flags |= fFncNoName | fFncMethod;
            }

            // These are the cases which can confirm async function:
            //   async function() {}         -> async function
            //   async () => {}              -> async lambda with parens around the formal parameter
            //   async arg => {}             -> async lambda with single identifier parameter
            //   async name() {}             -> async method
            //   async [computed_name]() {}  -> async method with a computed name
            if (m_token.tk == tkID && m_token.GetIdentifier(this->GetHashTbl()) == wellKnownPropertyPids.async && m_scriptContext->GetConfig()->IsES7AsyncAndAwaitEnabled())
            {
                ichMin = this->GetScanner()->IchMinTok();
                iecpMin = this->GetScanner()->IecpMinTok();

                // Keep state so we can rewind if it turns out that this isn't an async function:
                //   async() {}              -> method named async
                //   async => {}             -> lambda with single parameter named async
                RestorePoint termStart;
                this->GetScanner()->Capture(&termStart);

                this->GetScanner()->Scan();

                if (m_token.tk == tkDArrow || (m_token.tk == tkLParen && isMethod) || this->GetScanner()->FHadNewLine())
                {
                    this->GetScanner()->SeekTo(termStart);
                }
                else
                {
                    flags |= fFncAsync;
                    isAsync = true;
                }
            }

            if (m_token.tk == tkStar && m_scriptContext->GetConfig()->IsES6GeneratorsEnabled())
            {
                ichMin = this->GetScanner()->IchMinTok();
                iecpMin = this->GetScanner()->IecpMinTok();

                flags |= fFncGenerator;
                isGenerator = true;

                this->GetScanner()->Scan();
            }

            // Eat the computed name expression
            if (m_token.tk == tkLBrack && isMethod)
            {
                this->GetScanner()->Scan();
                ParseExpr<false>();
            }

            if (!isMethod && (m_token.tk == tkID || m_token.tk == tkLParen))
            {
                // If first token of the function is tkID or tkLParen, this is a lambda.
                flags |= fFncLambda;
            }

            ParseNode * pnodeFnc = ParseFncDeclCheckScope<true>(flags);
            pnodeProg->pnodeBody = nullptr;
            AddToNodeList(&pnodeProg->pnodeBody, &lastNodeRef, pnodeFnc);

            // Include the async keyword or star character in the function extents
            if (isAsync || isGenerator)
            {
                pnodeFnc->AsParseNodeFnc()->cbMin = iecpMin;
                pnodeFnc->ichMin = ichMin;
            }
        }
        else
        {
            // Process a sequence of statements/declarations
            ParseStmtList<true>(
                &pnodeProg->pnodeBody,
                &lastNodeRef,
                SM_OnGlobalCode,
                !(m_grfscr & fscrDeferredFncExpression) /* isSourceElementList */);
        }
    }

    if (m_parseType == ParseType_Deferred)
    {
        if (scopeInfo)
        {
            this->FinishScopeInfo(scopeInfo);
        }
    }

    pnodeProg->m_UsesArgumentsAtGlobal = m_UsesArgumentsAtGlobal;

    if (IsStrictMode())
    {
        pnodeProg->SetStrictMode();
    }

#if DEBUG
    if (m_grfscr & fscrEnforceJSON && !IsJSONValid(pnodeProg->pnodeBody))
    {
        Error(ERRsyntax);
    }
#endif

    if (tkEOF != m_token.tk)
        Error(ERRsyntax);

    // Append an EndCode node.
    AddToNodeList(&pnodeProg->pnodeBody, &lastNodeRef,
        CreateNodeForOpT<knopEndCode>());
    Assert(lastNodeRef);
    Assert(*lastNodeRef);
    Assert((*lastNodeRef)->nop == knopEndCode);
    (*lastNodeRef)->ichMin = 0;
    (*lastNodeRef)->ichLim = 0;

    // Get the extent of the code.
    pnodeProg->ichLim = this->GetScanner()->IchLimTok();
    pnodeProg->cbLim = this->GetScanner()->IecpLimTok();

    // Terminate the local list
    *m_ppnodeVar = nullptr;

    Assert(nullptr == *m_ppnodeScope);
    Assert(nullptr == pnodeProg->pnodeNext);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (Js::Configuration::Global.flags.IsEnabled(Js::ForceUndoDeferFlag))
    {
        m_stoppedDeferredParse = true;
    }
#endif

    if (m_stoppedDeferredParse)
    {
#if ENABLE_BACKGROUND_PARSING
        if (this->m_hasParallelJob)
        {
            BackgroundParser *bgp = static_cast<BackgroundParser*>(m_scriptContext->GetBackgroundParser());
            Assert(bgp);
            this->WaitForBackgroundJobs(bgp, pse);
        }
#endif

        // Do any remaining bindings of globals referenced in non-deferred functions.
        if (pnodeGlobalEvalBlock)
        {
            FinishParseBlock(pnodeGlobalEvalBlock);
        }

        FinishParseBlock(pnodeGlobalBlock);

        // Clear out references to undeclared identifiers.
        this->GetHashTbl()->VisitPids([&](IdentPtr pid) { pid->SetTopRef(nullptr); });

        // Restore global scope and blockinfo stacks preparatory to reparsing deferred functions.
        PushScope(pnodeGlobalBlock->scope);
        BlockInfoStack *newBlockInfo = PushBlockInfo(pnodeGlobalBlock);
        PushStmt<true>(&newBlockInfo->pstmt, pnodeGlobalBlock, knopBlock, nullptr);

        if (pnodeGlobalEvalBlock)
        {
            PushScope(pnodeGlobalEvalBlock->scope);
            newBlockInfo = PushBlockInfo(pnodeGlobalEvalBlock);
            PushStmt<true>(&newBlockInfo->pstmt, pnodeGlobalEvalBlock, knopBlock, nullptr);
        }

        // Finally, see if there are any function bodies we now want to generate because we
        // decided to stop deferring.
        FinishDeferredFunction(pnodeProg->pnodeScopes);
    }

    if (pnodeGlobalEvalBlock)
    {
        FinishParseBlock(pnodeGlobalEvalBlock);
    }
    // Append block as body of pnodeProg
    FinishParseBlock(pnodeGlobalBlock);

    m_scriptContext->AddSourceSize(m_length);

    if (m_parseType != ParseType_Deferred)
    {
        JS_ETW(EventWriteJSCRIPT_PARSE_METHOD_STOP(m_sourceContextInfo->dwHostSourceContext, GetScriptContext(), pnodeProg->functionId, *m_pCurrentAstSize, false, Js::Constants::GlobalFunction));
    }
    return pnodeProg;
}


bool Parser::CheckForDirective(bool* pIsUseStrict, bool *pIsUseAsm, bool* pIsOctalInString)
{
    // A directive is a string constant followed by a statement terminating token
    if (m_token.tk != tkStrCon)
        return false;

    // Careful, need to check for octal before calling this->GetScanner()->Scan()
    // because Scan() clears the "had octal" flag on the scanner and
    // this->GetScanner()->Restore() does not restore this flag.
    if (pIsOctalInString != nullptr)
    {
        *pIsOctalInString = this->GetScanner()->IsOctOrLeadingZeroOnLastTKNumber();
    }

    Ident* pidDirective = m_token.GetStr();
    RestorePoint start;
    this->GetScanner()->Capture(&start);
    this->GetScanner()->Scan();

    bool isDirective = true;

    switch (m_token.tk)
    {
    case tkSColon:
    case tkEOF:
    case tkLCurly:
    case tkRCurly:
        break;
    default:
        if (!this->GetScanner()->FHadNewLine())
        {
            isDirective = false;
        }
        break;
    }

    if (isDirective)
    {
        if (pIsUseStrict != nullptr)
        {
            *pIsUseStrict = CheckStrictModeStrPid(pidDirective);
        }
        if (pIsUseAsm != nullptr)
        {
            *pIsUseAsm = CheckAsmjsModeStrPid(pidDirective);
        }
    }

    this->GetScanner()->SeekTo(start);
    return isDirective;
}

bool Parser::CheckStrictModeStrPid(IdentPtr pid)
{
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    if (Js::Configuration::Global.flags.NoStrictMode)
        return false;
#endif

    return pid != nullptr &&
        pid->Cch() == 10 &&
        !this->GetScanner()->IsEscapeOnLastTkStrCon() &&
        wcsncmp(pid->Psz(), _u("use strict"), 10) == 0;
}

bool Parser::CheckAsmjsModeStrPid(IdentPtr pid)
{
#ifdef ASMJS_PLAT
    if (!CONFIG_FLAG(AsmJs))
    {
        return false;
    }

    bool isAsmCandidate = (pid != nullptr &&
        AutoSystemInfo::Data.SSE2Available() &&
        pid->Cch() == 7 &&
        !this->GetScanner()->IsEscapeOnLastTkStrCon() &&
        wcsncmp(pid->Psz(), _u("use asm"), 10) == 0);

#ifdef ENABLE_SCRIPT_DEBUGGING
    if (isAsmCandidate && m_scriptContext->IsScriptContextInDebugMode())
    {
        // We would like to report this to debugger - they may choose to disable debugging.
        // TODO : localization of the string?
        m_scriptContext->RaiseMessageToDebugger(DEIT_ASMJS_IN_DEBUGGING, _u("AsmJs initialization error - AsmJs disabled due to script debugger"), m_sourceContextInfo && !m_sourceContextInfo->IsDynamic() ? m_sourceContextInfo->url : nullptr);
        return false;
    }
#endif

    return isAsmCandidate && !(m_grfscr & fscrNoAsmJs);
#else
    return false;
#endif
}

HRESULT Parser::ParseUtf8Source(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
    Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo)
{
    m_functionBody = nullptr;
    m_parseType = ParseType_Upfront;
    return ParseSourceInternal(parseTree, pSrc, 0, length, 0, true, grfsrc, pse, nextFunctionId, 0, sourceContextInfo);
}

HRESULT Parser::ParseCesu8Source(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
    Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo)
{
    m_functionBody = nullptr;
    m_parseType = ParseType_Upfront;
    return ParseSourceInternal(parseTree, pSrc, 0, length, 0, false, grfsrc, pse, nextFunctionId, 0, sourceContextInfo);
}

#if ENABLE_BACKGROUND_PARSING
void Parser::PrepareForBackgroundParse()
{
    this->GetScanner()->PrepareForBackgroundParse(m_scriptContext);
}

void Parser::AddBackgroundParseItem(BackgroundParseItem *const item)
{
    if (currBackgroundParseItem == nullptr)
    {
        backgroundParseItems = item;
    }
    else
    {
        currBackgroundParseItem->SetNext(item);
    }
    currBackgroundParseItem = item;
}

void Parser::AddFastScannedRegExpNode(ParseNodePtr const pnode)
{
    Assert(!IsBackgroundParser());
    Assert(m_doingFastScan);

    if (fastScannedRegExpNodes == nullptr)
    {
        fastScannedRegExpNodes = Anew(&m_nodeAllocator, NodeDList, &m_nodeAllocator);
    }
    fastScannedRegExpNodes->Append(pnode);
}

void Parser::AddBackgroundRegExpNode(ParseNodePtr const pnode)
{
    Assert(IsBackgroundParser());
    Assert(currBackgroundParseItem != nullptr);

    currBackgroundParseItem->AddRegExpNode(pnode, &m_nodeAllocator);
}

HRESULT Parser::ParseFunctionInBackground(ParseNodeFnc * pnodeFnc, ParseContext *parseContext, bool topLevelDeferred, CompileScriptException *pse)
{
    m_functionBody = nullptr;
    m_parseType = ParseType_Upfront;
    HRESULT hr = S_OK;
    SmartFPUControl smartFpuControl;
    uint nextFunctionId = pnodeFnc->functionId + 1;

    this->RestoreContext(parseContext);
    m_nextFunctionId = &nextFunctionId;
    m_deferringAST = topLevelDeferred;
    m_inDeferredNestedFunc = false;
    m_scopeCountNoAst = 0;

    SetCurrentStatement(nullptr);

    pnodeFnc->pnodeVars = nullptr;
    pnodeFnc->pnodeParams = nullptr;
    pnodeFnc->pnodeBody = nullptr;
    pnodeFnc->nestedCount = 0;

    ParseNodeFnc * pnodeParentFnc = GetCurrentFunctionNode();
    m_currentNodeFunc = pnodeFnc;
    m_currentNodeDeferredFunc = nullptr;
    m_ppnodeScope = nullptr;
    m_ppnodeExprScope = nullptr;

    m_pnestedCount = &pnodeFnc->nestedCount;
    m_pCurrentAstSize = &pnodeFnc->astSize;

    ParseNodeBlock * pnodeBlock = StartParseBlock<true>(PnodeBlockType::Function, ScopeType_FunctionBody);
    pnodeFnc->pnodeScopes = pnodeBlock;
    m_ppnodeScope = &pnodeBlock->pnodeScopes;

    uint uDeferSave = m_grfscr & (fscrCanDeferFncParse | fscrWillDeferFncParse);

    try
    {
        this->GetScanner()->Scan();

        m_ppnodeVar = &pnodeFnc->pnodeParams;
        this->ParseFncFormals<true>(pnodeFnc, pnodeParentFnc, fFncNoFlgs);

        if (m_token.tk == tkRParen)
        {
            this->GetScanner()->Scan();
        }

        ChkCurTok(tkLCurly, ERRnoLcurly);

        m_ppnodeVar = &pnodeFnc->pnodeVars;

        // Put the scanner into "no hashing" mode.
        BYTE deferFlags = this->GetScanner()->SetDeferredParse(topLevelDeferred);

        // Process a sequence of statements/declarations
        if (topLevelDeferred)
        {
            ParseStmtList<false>(nullptr, nullptr, SM_DeferredParse, true);
        }
        else
        {
            ParseNodePtr *lastNodeRef = nullptr;
            ParseStmtList<true>(&pnodeFnc->pnodeBody, &lastNodeRef, SM_OnFunctionCode, true);
            AddArgumentsNodeToVars(pnodeFnc);
            // Append an EndCode node.
            AddToNodeList(&pnodeFnc->pnodeBody, &lastNodeRef, CreateNodeForOpT<knopEndCode>());
        }

        // Restore the scanner's default hashing mode.
        this->GetScanner()->SetDeferredParseFlags(deferFlags);

#if DBG
        pnodeFnc->deferredParseNextFunctionId = *this->m_nextFunctionId;
#endif
        this->m_deferringAST = FALSE;

        // Append block as body of pnodeProg
        FinishParseBlock(pnodeBlock);
    }
    catch (ParseExceptionObject& e)
    {
        hr = e.GetError();
    }

    if (FAILED(hr))
    {
        hr = pse->ProcessError(this->GetScanner(), hr, nullptr);
    }

    if (IsStrictMode())
    {
        pnodeFnc->SetStrictMode();
    }

    if (topLevelDeferred)
    {
        pnodeFnc->pnodeVars = nullptr;
    }

    m_grfscr |= uDeferSave;

    Assert(nullptr == *m_ppnodeScope);

    return hr;
}

#endif

HRESULT Parser::ParseSourceWithOffset(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t offset, size_t cbLength, charcount_t cchOffset,
    bool isCesu8, ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber, SourceContextInfo * sourceContextInfo,
    Js::ParseableFunctionInfo* functionInfo)
{
    m_functionBody = functionInfo;
    if (m_functionBody)
    {
        m_currDeferredStub = m_functionBody->GetDeferredStubs();
        m_currDeferredStubCount = m_currDeferredStub != nullptr ? m_functionBody->GetNestedCount() : 0;
        m_InAsmMode = grfscr & fscrNoAsmJs ? false : m_functionBody->GetIsAsmjsMode();
    }
    m_deferAsmJs = !m_InAsmMode;
    m_parseType = ParseType_Deferred;
    return ParseSourceInternal(parseTree, pSrc, offset, cbLength, cchOffset, !isCesu8, grfscr, pse, nextFunctionId, lineNumber, sourceContextInfo);
}

bool Parser::IsStrictMode() const
{
    return (m_fUseStrictMode ||
        (m_currentNodeFunc != nullptr && m_currentNodeFunc->GetStrictMode()));
}

BOOL Parser::ExpectingExternalSource()
{
    return m_fExpectExternalSource;
}

Symbol *ParseNodeFnc::GetFuncSymbol()
{
    if (pnodeName)
    {
        Assert(pnodeName->nop == knopVarDecl);
        return pnodeName->sym;
    }
    return nullptr;
}

void ParseNodeFnc::SetFuncSymbol(Symbol *sym)
{
    Assert(pnodeName);
    Assert(pnodeName->nop == knopVarDecl);
    pnodeName->sym = sym;
}

ParseNodePtr ParseNodeFnc::GetParamScope() const
{
    if (this->pnodeScopes == nullptr)
    {
        return nullptr;
    }
    Assert(this->pnodeScopes->nop == knopBlock &&
        this->pnodeScopes->pnodeNext == nullptr);
    return this->pnodeScopes->pnodeScopes;
}

ParseNodePtr ParseNodeFnc::GetBodyScope() const
{
    if (this->pnodeBodyScope == nullptr)
    {
        return nullptr;
    }
    Assert(this->pnodeBodyScope->nop == knopBlock &&
        this->pnodeBodyScope->pnodeNext == nullptr);
    return this->pnodeBodyScope->pnodeScopes;
}

bool ParseNodeBlock::HasBlockScopedContent() const
{
    // A block has its own content if a let, const, or function is declared there.

    if (this->pnodeLexVars != nullptr || this->blockType == Parameter)
    {
        return true;
    }

    // The enclosing scopes can contain functions and other things, so walk the list
    // looking specifically for functions.

    for (ParseNodePtr pnode = this->pnodeScopes; pnode;)
    {
        switch (pnode->nop) {

        case knopFncDecl:
            return true;

        case knopBlock:
            pnode = pnode->AsParseNodeBlock()->pnodeNext;
            break;

        case knopCatch:
            pnode = pnode->AsParseNodeCatch()->pnodeNext;
            break;

        case knopWith:
            pnode = pnode->AsParseNodeWith()->pnodeNext;
            break;

        default:
            Assert(UNREACHED);
            return true;
        }
    }

    return false;
}

class ByteCodeGenerator;

// Copy AST; this works mostly on expressions for now
ParseNode* Parser::CopyPnode(ParseNode *pnode) {
    if (pnode == NULL)
        return NULL;
    switch (pnode->nop) {
        //PTNODE(knopName       , "name"        ,None    ,Pid  ,fnopLeaf)
    case knopName: {
        ParseNodeName * nameNode = CreateNameNode(pnode->AsParseNodeName()->pid);
        nameNode->ichMin = pnode->ichMin;
        nameNode->ichLim = pnode->ichLim;
        nameNode->sym = pnode->AsParseNodeName()->sym;
        return nameNode;
    }
                   //PTNODE(knopInt        , "int const"    ,None    ,Int  ,fnopLeaf|fnopConst)
    case knopInt:
        return pnode;
        //PTNODE(knopFlt        , "flt const"    ,None    ,Flt  ,fnopLeaf|fnopConst)
    case knopFlt:
        return pnode;
        //PTNODE(knopStr        , "str const"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopStr:
        return pnode;
        //PTNODE(knopRegExp     , "reg expr"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopRegExp:
        return pnode;
        break;
        //PTNODE(knopNull       , "null"        ,Null    ,None ,fnopLeaf)
    case knopNull:
        return pnode;
        //PTNODE(knopFalse      , "false"        ,False   ,None ,fnopLeaf)
    case knopFalse:
    {
        ParseNode* ret = CreateNodeForOpT<knopFalse>(pnode->ichMin, pnode->ichLim);
        ret->location = pnode->location;
        return ret;
    }
    //PTNODE(knopTrue       , "true"        ,True    ,None ,fnopLeaf)
    case knopTrue:
    {
        ParseNode* ret = CreateNodeForOpT<knopTrue>(pnode->ichMin, pnode->ichLim);
        ret->location = pnode->location;
        return ret;
    }
    //PTNODE(knopEmpty      , "empty"        ,Empty   ,None ,fnopLeaf)
    case knopEmpty:
        return CreateNodeForOpT<knopEmpty>(pnode->ichMin, pnode->ichLim);
        // Unary operators.
        //PTNODE(knopNot        , "~"            ,BitNot  ,Uni  ,fnopUni)
        //PTNODE(knopNeg        , "unary -"    ,Neg     ,Uni  ,fnopUni)
        //PTNODE(knopPos        , "unary +"    ,Pos     ,Uni  ,fnopUni)
        //PTNODE(knopLogNot     , "!"            ,LogNot  ,Uni  ,fnopUni)
        //PTNODE(knopEllipsis     , "..."       ,Spread  ,Uni    , fnopUni)
        //PTNODE(knopDecPost    , "-- post"    ,Dec     ,Uni  ,fnopUni|fnopAsg)
        //PTNODE(knopIncPre     , "++ pre"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
        //PTNODE(knopDecPre     , "-- pre"    ,Dec     ,Uni  ,fnopUni|fnopAsg)
        //PTNODE(knopTypeof     , "typeof"    ,None    ,Uni  ,fnopUni)
        //PTNODE(knopVoid       , "void"        ,Void    ,Uni  ,fnopUni)
        //PTNODE(knopDelete     , "delete"    ,None    ,Uni  ,fnopUni)
    case knopNot:
    case knopNeg:
    case knopPos:
    case knopLogNot:
    case knopEllipsis:
    case knopIncPost:
    case knopDecPost:
    case knopIncPre:
    case knopDecPre:
    case knopTypeof:
    case knopVoid:
    case knopDelete:
        return CreateUniNode(pnode->nop, CopyPnode(pnode->AsParseNodeUni()->pnode1), pnode->ichMin, pnode->ichLim);
        //PTNODE(knopArray      , "arr cnst"    ,None    ,Uni  ,fnopUni)
        //PTNODE(knopObject     , "obj cnst"    ,None    ,Uni  ,fnopUni)
    case knopArray:
    case knopObject:
        // TODO: need to copy arr
        Assert(false);
        break;
        // Binary operators
        //PTNODE(knopAdd        , "+"            ,Add     ,Bin  ,fnopBin)
        //PTNODE(knopSub        , "-"            ,Sub     ,Bin  ,fnopBin)
        //PTNODE(knopMul        , "*"            ,Mul     ,Bin  ,fnopBin)
        //PTNODE(knopExpo       , "**"           ,Expo     ,Bin  ,fnopBin)
        //PTNODE(knopDiv        , "/"            ,Div     ,Bin  ,fnopBin)
        //PTNODE(knopMod        , "%"            ,Mod     ,Bin  ,fnopBin)
        //PTNODE(knopOr         , "|"            ,BitOr   ,Bin  ,fnopBin)
        //PTNODE(knopXor        , "^"            ,BitXor  ,Bin  ,fnopBin)
        //PTNODE(knopAnd        , "&"            ,BitAnd  ,Bin  ,fnopBin)
        //PTNODE(knopEq         , "=="        ,EQ      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopNe         , "!="        ,NE      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopLt         , "<"            ,LT      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopLe         , "<="        ,LE      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopGe         , ">="        ,GE      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopGt         , ">"            ,GT      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopEqv        , "==="        ,Eqv     ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopIn         , "in"        ,In      ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopInstOf     , "instanceof",InstOf  ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopNEqv       , "!=="        ,NEqv    ,Bin  ,fnopBin|fnopRel)
        //PTNODE(knopComma      , ","            ,None    ,Bin  ,fnopBin)
        //PTNODE(knopLogOr      , "||"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopLogAnd     , "&&"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopLsh        , "<<"        ,Lsh     ,Bin  ,fnopBin)
        //PTNODE(knopRsh        , ">>"        ,Rsh     ,Bin  ,fnopBin)
        //PTNODE(knopRs2        , ">>>"        ,Rs2     ,Bin  ,fnopBin)
    case knopAdd:
    case knopSub:
    case knopMul:
    case knopExpo:
    case knopDiv:
    case knopMod:
    case knopOr:
    case knopXor:
    case knopAnd:
    case knopEq:
    case knopNe:
    case knopLt:
    case knopLe:
    case knopGe:
    case knopGt:
    case knopEqv:
    case knopIn:
    case knopInstOf:
    case knopNEqv:
    case knopComma:
    case knopLogOr:
    case knopLogAnd:
    case knopLsh:
    case knopRsh:
    case knopRs2:
        //PTNODE(knopAsg        , "="            ,None    ,Bin  ,fnopBin|fnopAsg)
    case knopAsg:
        //PTNODE(knopDot        , "."            ,None    ,Bin  ,fnopBin)
    case knopDot:
        //PTNODE(knopAsgAdd     , "+="        ,Add     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgAdd:
        //PTNODE(knopAsgSub     , "-="        ,Sub     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgSub:
        //PTNODE(knopAsgMul     , "*="        ,Mul     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgMul:
        //PTNODE(knopAsgDiv     , "/="        ,Div     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgExpo:
        //PTNODE(knopAsgExpo    , "**="       ,Expo    ,Bin  ,fnopBin|fnopAsg)
    case knopAsgDiv:
        //PTNODE(knopAsgMod     , "%="        ,Mod     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgMod:
        //PTNODE(knopAsgAnd     , "&="        ,BitAnd  ,Bin  ,fnopBin|fnopAsg)
    case knopAsgAnd:
        //PTNODE(knopAsgXor     , "^="        ,BitXor  ,Bin  ,fnopBin|fnopAsg)
    case knopAsgXor:
        //PTNODE(knopAsgOr      , "|="        ,BitOr   ,Bin  ,fnopBin|fnopAsg)
    case knopAsgOr:
        //PTNODE(knopAsgLsh     , "<<="        ,Lsh     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgLsh:
        //PTNODE(knopAsgRsh     , ">>="        ,Rsh     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgRsh:
        //PTNODE(knopAsgRs2     , ">>>="        ,Rs2     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgRs2:
        //PTNODE(knopMember     , ":"            ,None    ,Bin  ,fnopBin)
    case knopMember:
    case knopMemberShort:
        //PTNODE(knopIndex      , "[]"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopList       , "<list>"    ,None    ,Bin  ,fnopNone)

    case knopIndex:
    case knopList:
        return CreateBinNode(pnode->nop, CopyPnode(pnode->AsParseNodeBin()->pnode1),
            CopyPnode(pnode->AsParseNodeBin()->pnode2), pnode->ichMin, pnode->ichLim);

        //PTNODE(knopCall       , "()"        ,None    ,Bin  ,fnopBin)
        //PTNODE(knopNew        , "new"        ,None    ,Bin  ,fnopBin)
    case knopNew:
    case knopCall:
        return CreateCallNode(pnode->nop, CopyPnode(pnode->AsParseNodeCall()->pnodeTarget),
            CopyPnode(pnode->AsParseNodeCall()->pnodeArgs), pnode->ichMin, pnode->ichLim);
        //PTNODE(knopQmark      , "?"            ,None    ,Tri  ,fnopBin)
    case knopQmark:
        return CreateTriNode(pnode->nop, CopyPnode(pnode->AsParseNodeTri()->pnode1),
            CopyPnode(pnode->AsParseNodeTri()->pnode2), CopyPnode(pnode->AsParseNodeTri()->pnode3),
            pnode->ichMin, pnode->ichLim);
        // General nodes.
        //PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
    case knopVarDecl: {
        ParseNodeVar* copyNode = Anew(&m_nodeAllocator, ParseNodeVar, knopVarDecl, pnode->ichMin, pnode->ichLim, nullptr);
        copyNode->pnodeInit = CopyPnode(pnode->AsParseNodeVar()->pnodeInit);
        copyNode->sym = pnode->AsParseNodeVar()->sym;
        // TODO: mult-decl
        Assert(pnode->AsParseNodeVar()->pnodeNext == NULL);
        copyNode->pnodeNext = NULL;
        return copyNode;
    }
                      //PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
                      //PTNODE(knopProg       , "program"    ,None    ,Fnc  ,fnopNone)
    case knopFncDecl:
    case knopProg:
        Assert(false);
        break;
        //PTNODE(knopEndCode    , "<endcode>"    ,None    ,None ,fnopNone)
    case knopEndCode:
        break;
        //PTNODE(knopDebugger   , "debugger"    ,None    ,None ,fnopNone)
    case knopDebugger:
        break;
        //PTNODE(knopFor        , "for"        ,None    ,For  ,fnopBreak|fnopContinue)
    case knopFor: {
        ParseNode* copyNode = CreateNodeForOpT<knopFor>(pnode->ichMin, pnode->ichLim);
        copyNode->AsParseNodeFor()->pnodeInverted = NULL;
        copyNode->AsParseNodeFor()->pnodeInit = CopyPnode(pnode->AsParseNodeFor()->pnodeInit);
        copyNode->AsParseNodeFor()->pnodeCond = CopyPnode(pnode->AsParseNodeFor()->pnodeCond);
        copyNode->AsParseNodeFor()->pnodeIncr = CopyPnode(pnode->AsParseNodeFor()->pnodeIncr);
        copyNode->AsParseNodeFor()->pnodeBody = CopyPnode(pnode->AsParseNodeFor()->pnodeBody);
        return copyNode;
    }
                  //PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
    case knopIf:
        Assert(false);
        break;
        //PTNODE(knopWhile      , "while"        ,None    ,While,fnopBreak|fnopContinue)
    case knopWhile:
        Assert(false);
        break;
        //PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
    case knopDoWhile:
        Assert(false);
        break;
        //PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
    case knopForIn:
        Assert(false);
        break;
    case knopForOf:
        Assert(false);
        break;
        //PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
    case knopReturn: {
        ParseNode* copyNode = CreateNodeForOpT<knopReturn>(pnode->ichMin, pnode->ichLim);
        copyNode->AsParseNodeReturn()->pnodeExpr = CopyPnode(pnode->AsParseNodeReturn()->pnodeExpr);
        return copyNode;
    }
                     //PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
    case knopBlock: {
        ParseNodeBlock* copyNode = CreateBlockNode(pnode->ichMin, pnode->ichLim, pnode->AsParseNodeBlock()->blockType);
        if (pnode->grfpn & PNodeFlags::fpnSyntheticNode) {
            // fpnSyntheticNode is sometimes set on PnodeBlockType::Regular blocks which
            // CreateBlockNode() will not automatically set for us, so set it here if it's
            // specified on the source node.
            copyNode->grfpn |= PNodeFlags::fpnSyntheticNode;
        }
        copyNode->pnodeStmt = CopyPnode(pnode->AsParseNodeBlock()->pnodeStmt);
        return copyNode;
    }
                    //PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
    case knopWith:
        Assert(false);
        break;
        //PTNODE(knopBreak      , "break"        ,None    ,Jump ,fnopNone)
    case knopBreak:
        Assert(false);
        break;
        //PTNODE(knopContinue   , "continue"    ,None    ,Jump ,fnopNone)
    case knopContinue:
        Assert(false);
        break;
        //PTNODE(knopSwitch     , "switch"    ,None    ,Switch,fnopBreak)
    case knopSwitch:
        Assert(false);
        break;
        //PTNODE(knopCase       , "case"        ,None    ,Case ,fnopNone)
    case knopCase:
        Assert(false);
        break;
        //PTNODE(knopTryFinally,"try-finally",None,TryFinally,fnopCleanup)
    case knopTryFinally:
        Assert(false);
        break;
    case knopFinally:
        Assert(false);
        break;
        //PTNODE(knopCatch      , "catch"     ,None    ,Catch,fnopNone)
    case knopCatch:
        Assert(false);
        break;
        //PTNODE(knopTryCatch      , "try-catch" ,None    ,TryCatch  ,fnopCleanup)
    case knopTryCatch:
        Assert(false);
        break;
        //PTNODE(knopTry        , "try"       ,None    ,Try  ,fnopCleanup)
    case knopTry:
        Assert(false);
        break;
        //PTNODE(knopThrow      , "throw"     ,None    ,Uni  ,fnopNone)
    case knopThrow:
        Assert(false);
        break;
    default:
        Assert(false);
        break;
    }
    return NULL;
}

// Returns true when str is string for Nan, Infinity or -Infinity.
// Does not check for double number value being in NaN/Infinity range.
// static
template<bool CheckForNegativeInfinity>
inline bool Parser::IsNaNOrInfinityLiteral(LPCOLESTR str)
{
    // Note: wcscmp crashes when one of the parameters is NULL.
    return str &&
        (wcscmp(_u("NaN"), str) == 0 ||
            wcscmp(_u("Infinity"), str) == 0 ||
            (CheckForNegativeInfinity && wcscmp(_u("-Infinity"), str) == 0));
}

template <bool buildAST>
IdentPtr Parser::ParseSuper(bool fAllowCall)
{
    ParseNodeFnc * currentNodeFunc = GetCurrentFunctionNode();
    ParseNodeFnc * currentNonLambdaFunc = GetCurrentNonLambdaFunctionNode();
    IdentPtr superPid = nullptr;

    switch (m_token.tk)
    {
    case tkDot:     // super.prop
    case tkLBrack:  // super[foo]
        superPid = wellKnownPropertyPids._super;
        break;
    case tkLParen:  // super(args)
        superPid = wellKnownPropertyPids._superConstructor;
        break;

    default:
        Error(ERRInvalidSuper);
        break;
    }

    currentNodeFunc->SetHasSuperReference(TRUE);
    CHAKRATEL_LANGSTATS_INC_LANGFEATURECOUNT(ES6, Super, m_scriptContext);

    // If we are defer parsing, we can skip verifying that the super reference is valid.
    // If it wasn't the parser would have thrown during upfront parsing and we wouldn't be defer parsing the function.
    if (m_parseType == ParseType_Deferred)
    {
        return superPid;
    }

    if (!fAllowCall && (m_token.tk == tkLParen))
    {
        Error(ERRInvalidSuper); // new super() is not allowed
    }
    else if ((currentNodeFunc->IsConstructor() && currentNodeFunc->superRestrictionState == SuperRestrictionState::CallAndPropertyAllowed)
                || (currentNonLambdaFunc != nullptr && currentNonLambdaFunc->superRestrictionState == SuperRestrictionState::CallAndPropertyAllowed))
    {
        // Any super access is good within a class constructor
    }
    else if ((this->m_grfscr & fscrEval) == fscrEval || (currentNonLambdaFunc != nullptr && currentNonLambdaFunc->superRestrictionState == SuperRestrictionState::PropertyAllowed))
    {
        // Currently for eval cases during compile time we use propertyallowed and throw during runtime for error cases
        if (m_token.tk == tkLParen)
        {
            if ((this->m_grfscr & fscrEval) == fscrNil)
            {
                // Cannot call super within a class member
                Error(ERRInvalidSuper);
            }
            else
            {
                Js::JavascriptFunction * caller = nullptr;
                if (Js::JavascriptStackWalker::GetCaller(&caller, m_scriptContext))
                {
                    Js::FunctionBody * callerBody = caller->GetFunctionBody();
                    Assert(callerBody);
                    if (!callerBody->GetFunctionInfo()->GetAllowDirectSuper())
                    {
                        Error(ERRInvalidSuper);
                    }
                }
            }
        }
    }
    else
    {
        // Anything else is an error
        Error(ERRInvalidSuper);
    }

    return superPid;
}

void Parser::AppendToList(ParseNodePtr *node, ParseNodePtr nodeToAppend)
{
    Assert(nodeToAppend);
    ParseNodePtr* lastPtr = node;
    while ((*lastPtr) && (*lastPtr)->nop == knopList)
    {
        lastPtr = &(*lastPtr)->AsParseNodeBin()->pnode2;
    }
    auto last = (*lastPtr);
    if (last)
    {
        *lastPtr = CreateBinNode(knopList, last, nodeToAppend, last->ichMin, nodeToAppend->ichLim);
    }
    else
    {
        *lastPtr = nodeToAppend;
    }
}

ParseNodePtr Parser::ConvertArrayToArrayPattern(ParseNodePtr pnode)
{
    Assert(pnode->nop == knopArray);
    pnode->nop = knopArrayPattern;

    ForEachItemRefInList(&pnode->AsParseNodeArrLit()->pnode1, [&](ParseNodePtr *itemRef) {
        ParseNodePtr item = *itemRef;
        if (item->nop == knopEllipsis)
        {
            itemRef = &item->AsParseNodeUni()->pnode1;
            item = *itemRef;
            if (!(item->nop == knopName
                || item->nop == knopDot
                || item->nop == knopIndex
                || item->nop == knopArray
                || item->nop == knopObject))
            {
                Error(ERRInvalidAssignmentTarget);
            }
        }
        else if (item->nop == knopAsg)
        {
            itemRef = &item->AsParseNodeBin()->pnode1;
            item = *itemRef;
        }

        if (item->nop == knopArray)
        {
            ConvertArrayToArrayPattern(item);
        }
        else if (item->nop == knopObject)
        {
            *itemRef = ConvertObjectToObjectPattern(item);
        }
        else if (item->nop == knopName)
        {
            TrackAssignment<true>(item, nullptr);
        }
    });

    return pnode;
}

ParseNodeUni * Parser::ConvertObjectToObjectPattern(ParseNodePtr pnodeMemberList)
{
    charcount_t ichMin = this->GetScanner()->IchMinTok();
    charcount_t ichLim = this->GetScanner()->IchLimTok();
    ParseNodePtr pnodeMemberNodeList = nullptr;
    if (pnodeMemberList != nullptr && pnodeMemberList->nop == knopObject)
    {
        ichMin = pnodeMemberList->ichMin;
        ichLim = pnodeMemberList->ichLim;
        pnodeMemberList = pnodeMemberList->AsParseNodeUni()->pnode1;
    }

    ForEachItemInList(pnodeMemberList, [&](ParseNodePtr item) {
        ParseNodePtr memberNode = ConvertMemberToMemberPattern(item);
        AppendToList(&pnodeMemberNodeList, memberNode);
    });

    return CreateUniNode(knopObjectPattern, pnodeMemberNodeList, ichMin, ichLim);
}

ParseNodePtr Parser::GetRightSideNodeFromPattern(ParseNodePtr pnode)
{
    Assert(pnode != nullptr);
    ParseNodePtr rightNode = nullptr;
    OpCode op = pnode->nop;
    if (op == knopObject)
    {
        rightNode = ConvertObjectToObjectPattern(pnode);
    }
    else if (op == knopArray)
    {
        rightNode = ConvertArrayToArrayPattern(pnode);
    }
    else
    {
        rightNode = pnode;
        if (op == knopName)
        {
            TrackAssignment<true>(pnode, nullptr);
        }
        else if (op == knopAsg)
        {
            TrackAssignment<true>(pnode->AsParseNodeBin()->pnode1, nullptr);
        }
    }

    return rightNode;
}

ParseNodePtr Parser::ConvertMemberToMemberPattern(ParseNodePtr pnodeMember)
{
    if (pnodeMember->nop == knopObjectPatternMember)
    {
        return pnodeMember;
    }

    Assert(pnodeMember->nop == knopMember || pnodeMember->nop == knopMemberShort);

    ParseNodePtr rightNode = GetRightSideNodeFromPattern(pnodeMember->AsParseNodeBin()->pnode2);
    ParseNodePtr resultNode = CreateBinNode(knopObjectPatternMember, pnodeMember->AsParseNodeBin()->pnode1, rightNode);
    resultNode->ichMin = pnodeMember->ichMin;
    resultNode->ichLim = pnodeMember->ichLim;
    return resultNode;
}

ParseNodePtr Parser::ConvertToPattern(ParseNodePtr pnode)
{
    if (pnode != nullptr)
    {
        if (pnode->nop == knopArray)
        {
            ConvertArrayToArrayPattern(pnode);
        }
        else if (pnode->nop == knopObject)
        {
            pnode = ConvertObjectToObjectPattern(pnode);
        }
    }
    return pnode;
}

// This essentially be called for verifying the structure of the current tree with satisfying the destructuring grammar.
void Parser::ParseDestructuredLiteralWithScopeSave(tokens declarationType,
    bool isDecl,
    bool topLevel,
    DestructuringInitializerContext initializerContext/* = DIC_None*/,
    bool allowIn /*= true*/)
{
    // We are going to parse the text again to validate the current grammar as Destructuring. Saving some scopes and
    // AST related information before the validation parsing and later they will be restored.

    ParseNodeFnc * pnodeFncSave = m_currentNodeFunc;
    ParseNodeFnc * pnodeDeferredFncSave = m_currentNodeDeferredFunc;
    if (m_currentNodeDeferredFunc == nullptr)
    {
        m_currentNodeDeferredFunc = m_currentNodeFunc;
    }
    int32 *pAstSizeSave = m_pCurrentAstSize;
    uint *pNestedCountSave = m_pnestedCount;
    ParseNodePtr *ppnodeScopeSave = m_ppnodeScope;
    ParseNodePtr *ppnodeExprScopeSave = m_ppnodeExprScope;

    ParseNodePtr newTempScope = nullptr;
    m_ppnodeScope = &newTempScope;

    int32 newTempAstSize = 0;
    m_pCurrentAstSize = &newTempAstSize;

    uint newTempNestedCount = 0;
    m_pnestedCount = &newTempNestedCount;

    m_ppnodeExprScope = nullptr;

    charcount_t funcInArraySave = m_funcInArray;
    uint funcInArrayDepthSave = m_funcInArrayDepth;

    // we need to reset this as we are going to parse the grammar again.
    m_hasDeferredShorthandInitError = false;

    ParseDestructuredLiteral<false>(declarationType, isDecl, topLevel, initializerContext, allowIn);

    m_currentNodeFunc = pnodeFncSave;
    m_currentNodeDeferredFunc = pnodeDeferredFncSave;
    m_pCurrentAstSize = pAstSizeSave;
    m_pnestedCount = pNestedCountSave;
    m_ppnodeScope = ppnodeScopeSave;
    m_ppnodeExprScope = ppnodeExprScopeSave;
    m_funcInArray = funcInArraySave;
    m_funcInArrayDepth = funcInArrayDepthSave;
}

template <bool buildAST>
ParseNodePtr Parser::ParseDestructuredLiteral(tokens declarationType,
    bool isDecl,
    bool topLevel/* = true*/,
    DestructuringInitializerContext initializerContext/* = DIC_None*/,
    bool allowIn/* = true*/,
    BOOL *forInOfOkay/* = nullptr*/,
    BOOL *nativeForOkay/* = nullptr*/)
{
    ParseNodeUni * pnode = nullptr;
    Assert(IsPossiblePatternStart());
    if (m_token.tk == tkLCurly)
    {
        pnode = ParseDestructuredObjectLiteral<buildAST>(declarationType, isDecl, topLevel);
    }
    else
    {
        pnode = ParseDestructuredArrayLiteral<buildAST>(declarationType, isDecl, topLevel);
    }

    return ParseDestructuredInitializer<buildAST>(pnode, isDecl, topLevel, initializerContext, allowIn, forInOfOkay, nativeForOkay);
}

template <bool buildAST>
ParseNodePtr Parser::ParseDestructuredInitializer(ParseNodeUni * lhsNode,
    bool isDecl,
    bool topLevel,
    DestructuringInitializerContext initializerContext,
    bool allowIn,
    BOOL *forInOfOkay,
    BOOL *nativeForOkay)
{
    this->GetScanner()->Scan();
    if (topLevel && nativeForOkay == nullptr)
    {
        if (initializerContext != DIC_ForceErrorOnInitializer && m_token.tk != tkAsg)
        {
            // e.g. var {x};
            Error(ERRDestructInit);
        }
        else if (initializerContext == DIC_ForceErrorOnInitializer && m_token.tk == tkAsg)
        {
            // e.g. catch([x] = [0])
            Error(ERRDestructNotInit);
        }
    }

    if (m_token.tk != tkAsg || initializerContext == DIC_ShouldNotParseInitializer)
    {
        if (topLevel && nativeForOkay != nullptr)
        {
            // Native loop should have destructuring initializer
            *nativeForOkay = FALSE;
        }

        return lhsNode;
    }

    if (forInOfOkay)
    {
        *forInOfOkay = FALSE;
    }

    this->GetScanner()->Scan();


    bool alreadyHasInitError = m_hasDeferredShorthandInitError;

    ParseNodePtr pnodeDefault = ParseExpr<buildAST>(koplCma, nullptr, allowIn);

    if (m_hasDeferredShorthandInitError && !alreadyHasInitError)
    {
        Error(ERRnoColon);
    }

    ParseNodeBin * pnodeDestructAsg = nullptr;
    if (buildAST)
    {
        Assert(lhsNode != nullptr);

        pnodeDestructAsg = CreateBinNode(knopAsg, lhsNode, pnodeDefault, lhsNode->ichMin, pnodeDefault->ichLim);
    }
    return pnodeDestructAsg;
}

template <bool buildAST>
ParseNodeUni * Parser::ParseDestructuredObjectLiteral(tokens declarationType, bool isDecl, bool topLevel/* = true*/)
{
    Assert(m_token.tk == tkLCurly);
    charcount_t ichMin = this->GetScanner()->IchMinTok();
    this->GetScanner()->Scan();

    if (!isDecl)
    {
        declarationType = tkLCurly;
    }
    ParseNodePtr pnodeMemberList = ParseMemberList<buildAST>(nullptr/*pNameHint*/, nullptr/*pHintLength*/, declarationType);
    Assert(m_token.tk == tkRCurly);

    ParseNodeUni * objectPatternNode = nullptr;
    if (buildAST)
    {
        charcount_t ichLim = this->GetScanner()->IchLimTok();
        objectPatternNode = CreateUniNode(knopObjectPattern, pnodeMemberList, ichMin, ichLim);
    }
    return objectPatternNode;
}

template <bool buildAST>
ParseNodePtr Parser::ParseDestructuredVarDecl(tokens declarationType, bool isDecl, bool *hasSeenRest, bool topLevel/* = true*/, bool allowEmptyExpression/* = true*/)
{
    ParseNodePtr pnodeElem = nullptr;
    int parenCount = 0;
    bool seenRest = false;

    // Save the Block ID prior to the increments, so we can restore it back.
    int originalCurrentBlockId = GetCurrentBlock()->blockId;

    // Eat the left parentheses only when its not a declaration. This will make sure we throw syntax errors early.
    if (!isDecl)
    {
        while (m_token.tk == tkLParen)
        {
            this->GetScanner()->Scan();
            ++parenCount;

            // Match the block increment we do upon entering parenthetical expressions
            // so that the block ID's will match on reparsing of parameters.
            GetCurrentBlock()->blockId = m_nextBlockId++;
        }
    }

    if (m_token.tk == tkEllipsis)
    {
        // As per ES 2015 : Rest can have left-hand-side-expression when on assignment expression, but under declaration only binding identifier is allowed
        // But spec is going to change for this one to allow LHS-expression both on expression and declaration - so making that happen early.

        seenRest = true;
        this->GetScanner()->Scan();

        // Eat the left parentheses only when its not a declaration. This will make sure we throw syntax errors early.
        if (!isDecl)
        {
            while (m_token.tk == tkLParen)
            {
                this->GetScanner()->Scan();
                ++parenCount;

                // Match the block increment we do upon entering parenthetical expressions
                // so that the block ID's will match on reparsing of parameters.
                GetCurrentBlock()->blockId = m_nextBlockId++;
            }
        }

        if (m_token.tk != tkID && m_token.tk != tkTHIS && m_token.tk != tkSUPER && m_token.tk != tkLCurly && m_token.tk != tkLBrack)
        {
            if (isDecl)
            {
                Error(ERRnoIdent);
            }
            else
            {
                Error(ERRInvalidAssignmentTarget);
            }
        }
    }

    if (IsPossiblePatternStart())
    {
        // For the possible pattern start we do not allow the parens before
        if (parenCount != 0)
        {
            Error(ERRDestructIDRef);
        }

        // Go recursively
        pnodeElem = ParseDestructuredLiteral<buildAST>(declarationType, isDecl, false /*topLevel*/, seenRest ? DIC_ShouldNotParseInitializer : DIC_None);
        if (!isDecl)
        {
            BOOL fCanAssign;
            IdentToken token;
            // Look for postfix operator
            pnodeElem = ParsePostfixOperators<buildAST>(pnodeElem, TRUE, FALSE, FALSE, TRUE, &fCanAssign, &token);
        }
    }
    else if (m_token.tk == tkSUPER || m_token.tk == tkID || m_token.tk == tkTHIS)
    {
        if (isDecl)
        {
            charcount_t ichMin = this->GetScanner()->IchMinTok();
            pnodeElem = ParseVariableDeclaration<buildAST>(declarationType, ichMin
                ,/* fAllowIn */false, /* pfForInOk */nullptr, /* singleDefOnly */true, /* allowInit */!seenRest, false /*topLevelParse*/);

        }
        else
        {
            BOOL fCanAssign;
            IdentToken token;
            // We aren't declaring anything, so scan the ID reference manually.
            pnodeElem = ParseTerm<buildAST>(/* fAllowCall */ m_token.tk != tkSUPER, nullptr /*pNameHint*/, nullptr /*pHintLength*/, nullptr /*pShortNameOffset*/, &token, false,
                FALSE, &fCanAssign);

            // In this destructuring case we can force error here as we cannot assign.

            if (!fCanAssign)
            {
                Error(ERRInvalidAssignmentTarget);
            }

            if (buildAST)
            {
                if (IsStrictMode() && pnodeElem != nullptr && pnodeElem->nop == knopName)
                {
                    CheckStrictModeEvalArgumentsUsage(pnodeElem->AsParseNodeName()->pid);
                }
            }
            else
            {
                if (IsStrictMode() && token.tk == tkID)
                {
                    CheckStrictModeEvalArgumentsUsage(token.pid);
                }
                token.tk = tkNone;
            }
        }
    }
    else if (!((m_token.tk == tkComma || m_token.tk == tkRBrack || m_token.tk == tkRCurly) && allowEmptyExpression))
    {
        if (m_token.IsOperator())
        {
            Error(ERRDestructNoOper);
        }
        Error(ERRDestructIDRef);
    }

    // Swallow RParens before a default expression, if any.
    // We eat the left parentheses only when its not a declaration. This will make sure we throw syntax errors early. We need to do the same for right parentheses.
    if (!isDecl)
    {
        while (m_token.tk == tkRParen)
        {
            this->GetScanner()->Scan();
            --parenCount;
        }

        // Restore the Block ID of the current block after the parsing of destructured variable declarations and initializers.
        GetCurrentBlock()->blockId = originalCurrentBlockId;
    }

    if (parenCount != 0)
    {
        Error(ERRnoRparen);
    }

    if (hasSeenRest != nullptr)
    {
        *hasSeenRest = seenRest;
    }

    if (m_token.tk == tkAsg)
    {
        // Parse the initializer.
        if (seenRest)
        {
            Error(ERRRestWithDefault);
        }
        this->GetScanner()->Scan();

        bool alreadyHasInitError = m_hasDeferredShorthandInitError;
        ParseNodePtr pnodeInit = ParseExpr<buildAST>(koplCma);

        if (m_hasDeferredShorthandInitError && !alreadyHasInitError)
        {
            Error(ERRnoColon);
        }

        if (buildAST)
        {
            pnodeElem = CreateBinNode(knopAsg, pnodeElem, pnodeInit);
        }
    }

    if (buildAST && seenRest)
    {
        ParseNodePtr pnodeRest = CreateUniNode(knopEllipsis, pnodeElem);
        pnodeElem = pnodeRest;
    }

    if (!(m_token.tk == tkComma || m_token.tk == tkRBrack || m_token.tk == tkRCurly))
    {
        if (m_token.IsOperator())
        {
            Error(ERRDestructNoOper);
        }
        Error(ERRsyntax);
    }

    return pnodeElem;
}

template <bool buildAST>
ParseNodeUni * Parser::ParseDestructuredArrayLiteral(tokens declarationType, bool isDecl, bool topLevel)
{
    Assert(m_token.tk == tkLBrack);
    charcount_t ichMin = this->GetScanner()->IchMinTok();

    this->GetScanner()->Scan();

    ParseNodeArrLit * pnodeDestructArr = nullptr;
    ParseNodePtr pnodeList = nullptr;
    ParseNodePtr *lastNodeRef = nullptr;
    uint count = 0;
    bool hasMissingValues = false;
    bool seenRest = false;

    if (m_token.tk != tkRBrack)
    {
        while (true)
        {
            ParseNodePtr pnodeElem = ParseDestructuredVarDecl<buildAST>(declarationType, isDecl, &seenRest, topLevel);
            if (buildAST)
            {
                if (pnodeElem == nullptr && buildAST)
                {
                    pnodeElem = CreateNodeForOpT<knopEmpty>();
                    hasMissingValues = true;
                }
                AddToNodeListEscapedUse(&pnodeList, &lastNodeRef, pnodeElem);
            }
            count++;

            if (m_token.tk == tkRBrack)
            {
                break;
            }

            if (m_token.tk != tkComma)
            {
                Error(ERRDestructNoOper);
            }

            if (seenRest) // Rest must be in the last position.
            {
                Error(ERRDestructRestLast);
            }

            this->GetScanner()->Scan();

            // break if we have the trailing comma as well, eg. [a,]
            if (m_token.tk == tkRBrack)
            {
                break;
            }
        }
    }

    if (buildAST)
    {
        pnodeDestructArr = CreateNodeForOpT<knopArrayPattern>(ichMin);
        pnodeDestructArr->pnode1 = pnodeList;
        pnodeDestructArr->arrayOfTaggedInts = false;
        pnodeDestructArr->arrayOfInts = false;
        pnodeDestructArr->arrayOfNumbers = false;
        pnodeDestructArr->hasMissingValues = hasMissingValues;
        pnodeDestructArr->count = count;
        pnodeDestructArr->spreadCount = seenRest ? 1 : 0;

        if (pnodeDestructArr->pnode1)
        {
            this->CheckArguments(pnodeDestructArr->pnode1);
        }
    }

    return pnodeDestructArr;
}

void Parser::CaptureContext(ParseContext *parseContext) const
{
    parseContext->pszSrc = this->GetScanner()->PchBase();
    parseContext->length = this->m_originalLength;
    parseContext->characterOffset = this->GetScanner()->IchMinTok();
    parseContext->offset = parseContext->characterOffset + this->GetScanner()->m_cMultiUnits;
    parseContext->grfscr = this->m_grfscr;
    parseContext->lineNumber = this->GetScanner()->LineCur();

    parseContext->pnodeProg = this->m_currentNodeProg;
    parseContext->isUtf8 = this->GetScanner()->IsUtf8();
    parseContext->strictMode = this->IsStrictMode();
    parseContext->sourceContextInfo = this->m_sourceContextInfo;
    parseContext->currentBlockInfo = this->m_currentBlockInfo;
    parseContext->nextBlockId = this->m_nextBlockId;
}

void Parser::RestoreContext(ParseContext *const parseContext)
{
    m_sourceContextInfo = parseContext->sourceContextInfo;
    m_currentBlockInfo = parseContext->currentBlockInfo;
    m_nextBlockId = parseContext->nextBlockId;
    m_grfscr = parseContext->grfscr;
    m_length = parseContext->length;
    this->GetScanner()->SetText(parseContext->pszSrc, parseContext->offset, parseContext->length, parseContext->characterOffset, parseContext->isUtf8, parseContext->grfscr, parseContext->lineNumber);
    m_currentNodeProg = parseContext->pnodeProg;
    m_fUseStrictMode = parseContext->strictMode;
}

class ByteCodeGenerator;
#if DBG_DUMP

#define INDENT_SIZE 2

void PrintPnodeListWIndent(ParseNode *pnode, int indentAmt);
void PrintFormalsWIndent(ParseNode *pnode, int indentAmt);


void Indent(int indentAmt) {
    for (int i = 0; i < indentAmt; i++) {
        Output::Print(_u(" "));
    }
}

void PrintBlockType(PnodeBlockType type)
{
    switch (type)
    {
    case Global:
        Output::Print(_u("(Global)"));
        break;
    case Function:
        Output::Print(_u("(Function)"));
        break;
    case Regular:
        Output::Print(_u("(Regular)"));
        break;
    case Parameter:
        Output::Print(_u("(Parameter)"));
        break;
    default:
        Output::Print(_u("(unknown blocktype)"));
        break;
    }
}

void PrintScopesWIndent(ParseNode *pnode, int indentAmt) {
    ParseNode *scope = nullptr;
    bool firstOnly = false;
    switch (pnode->nop)
    {
    case knopProg:
    case knopFncDecl: scope = pnode->AsParseNodeFnc()->pnodeScopes; break;
    case knopBlock: scope = pnode->AsParseNodeBlock()->pnodeScopes; break;
    case knopCatch: scope = pnode->AsParseNodeCatch()->pnodeScopes; break;
    case knopWith: scope = pnode->AsParseNodeWith()->pnodeScopes; break;
    case knopSwitch: scope = pnode->AsParseNodeSwitch()->pnodeBlock; firstOnly = true; break;
    case knopFor: scope = pnode->AsParseNodeFor()->pnodeBlock; firstOnly = true; break;
    case knopForIn: scope = pnode->AsParseNodeForInOrForOf()->pnodeBlock; firstOnly = true; break;
    case knopForOf: scope = pnode->AsParseNodeForInOrForOf()->pnodeBlock; firstOnly = true; break;
    }
    if (scope) {
        Output::Print(_u("[%4d, %4d): "), scope->ichMin, scope->ichLim);
        Indent(indentAmt);
        Output::Print(_u("Scopes: "));
        ParseNode *next = nullptr;
        ParseNode *syntheticBlock = nullptr;
        while (scope) {
            switch (scope->nop) {
            case knopFncDecl: Output::Print(_u("knopFncDecl")); next = scope->AsParseNodeFnc()->pnodeNext; break;
            case knopBlock: Output::Print(_u("knopBlock")); PrintBlockType(scope->AsParseNodeBlock()->blockType); next = scope->AsParseNodeBlock()->pnodeNext; break;
            case knopCatch: Output::Print(_u("knopCatch")); next = scope->AsParseNodeCatch()->pnodeNext; break;
            case knopWith: Output::Print(_u("knopWith")); next = scope->AsParseNodeWith()->pnodeNext; break;
            default: Output::Print(_u("unknown")); break;
            }
            if (firstOnly) {
                next = nullptr;
                syntheticBlock = scope;
            }
            if (scope->grfpn & fpnSyntheticNode) {
                Output::Print(_u(" synthetic"));
                if (scope->nop == knopBlock)
                    syntheticBlock = scope;
            }
            Output::Print(_u(" (%d-%d)"), scope->ichMin, scope->ichLim);
            if (next) Output::Print(_u(", "));
            scope = next;
        }
        Output::Print(_u("\n"));
        if (syntheticBlock || firstOnly) {
            PrintScopesWIndent(syntheticBlock, indentAmt + INDENT_SIZE);
        }
    }
}

void PrintPnodeWIndent(ParseNode *pnode, int indentAmt) {
    if (pnode == NULL)
        return;

    Output::Print(_u("[%4d, %4d): "), pnode->ichMin, pnode->ichLim);
    switch (pnode->nop) {
        //PTNODE(knopName       , "name"        ,None    ,Pid  ,fnopLeaf)
    case knopName:
        Indent(indentAmt);
        if (pnode->AsParseNodeName()->pid != NULL) {
            Output::Print(_u("id: %s\n"), pnode->AsParseNodeName()->pid->Psz());
        }
        else {
            Output::Print(_u("name node\n"));
        }
        break;
        //PTNODE(knopInt        , "int const"    ,None    ,Int  ,fnopLeaf|fnopConst)
    case knopInt:
        Indent(indentAmt);
        Output::Print(_u("%d\n"), pnode->AsParseNodeInt()->lw);
        break;
        //PTNODE(knopFlt        , "flt const"    ,None    ,Flt  ,fnopLeaf|fnopConst)
    case knopFlt:
        Indent(indentAmt);
        Output::Print(_u("%lf\n"), pnode->AsParseNodeFloat()->dbl);
        break;
        //PTNODE(knopStr        , "str const"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopStr:
        Indent(indentAmt);
        Output::Print(_u("\"%s\"\n"), pnode->AsParseNodeStr()->pid->Psz());
        break;
        //PTNODE(knopRegExp     , "reg expr"    ,None    ,Pid  ,fnopLeaf|fnopConst)
    case knopRegExp:
        Indent(indentAmt);
        Output::Print(_u("/%x/\n"), pnode->AsParseNodeRegExp()->regexPattern);
        break;
        //PTNODE(knopNull       , "null"        ,Null    ,None ,fnopLeaf)
    case knopNull:
        Indent(indentAmt);
        Output::Print(_u("null\n"));
        break;
        //PTNODE(knopFalse      , "false"        ,False   ,None ,fnopLeaf)
    case knopFalse:
        Indent(indentAmt);
        Output::Print(_u("false\n"));
        break;
        //PTNODE(knopTrue       , "true"        ,True    ,None ,fnopLeaf)
    case knopTrue:
        Indent(indentAmt);
        Output::Print(_u("true\n"));
        break;
        //PTNODE(knopEmpty      , "empty"        ,Empty   ,None ,fnopLeaf)
    case knopEmpty:
        Indent(indentAmt);
        Output::Print(_u("empty\n"));
        break;
        // Unary operators.
        //PTNODE(knopNot        , "~"            ,BitNot  ,Uni  ,fnopUni)
    case knopNot:
        Indent(indentAmt);
        Output::Print(_u("~\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopNeg        , "unary -"    ,Neg     ,Uni  ,fnopUni)
    case knopNeg:
        Indent(indentAmt);
        Output::Print(_u("U-\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopPos        , "unary +"    ,Pos     ,Uni  ,fnopUni)
    case knopPos:
        Indent(indentAmt);
        Output::Print(_u("U+\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLogNot     , "!"            ,LogNot  ,Uni  ,fnopUni)
    case knopLogNot:
        Indent(indentAmt);
        Output::Print(_u("!\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopEllipsis     , "..."       ,Spread  ,Uni    , fnopUni)
    case knopEllipsis:
        Indent(indentAmt);
        Output::Print(_u("...<expr>\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopIncPost    , "++ post"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
    case knopIncPost:
        Indent(indentAmt);
        Output::Print(_u("<expr>++\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDecPost    , "-- post"    ,Dec     ,Uni  ,fnopUni|fnopAsg)
    case knopDecPost:
        Indent(indentAmt);
        Output::Print(_u("<expr>--\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopIncPre     , "++ pre"    ,Inc     ,Uni  ,fnopUni|fnopAsg)
    case knopIncPre:
        Indent(indentAmt);
        Output::Print(_u("++<expr>\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDecPre     , "-- pre"    ,Dec     ,Uni  ,fnopUni|fnopAsg)
    case knopDecPre:
        Indent(indentAmt);
        Output::Print(_u("--<expr>\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopTypeof     , "typeof"    ,None    ,Uni  ,fnopUni)
    case knopTypeof:
        Indent(indentAmt);
        Output::Print(_u("typeof\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopVoid       , "void"        ,Void    ,Uni  ,fnopUni)
    case knopVoid:
        Indent(indentAmt);
        Output::Print(_u("void\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDelete     , "delete"    ,None    ,Uni  ,fnopUni)
    case knopDelete:
        Indent(indentAmt);
        Output::Print(_u("delete\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopArray      , "arr cnst"    ,None    ,Uni  ,fnopUni)

    case knopArrayPattern:
        Indent(indentAmt);
        Output::Print(_u("Array Pattern\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;

    case knopObjectPattern:
        Indent(indentAmt);
        Output::Print(_u("Object Pattern\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;

    case knopArray:
        Indent(indentAmt);
        Output::Print(_u("Array Literal\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopObject     , "obj cnst"    ,None    ,Uni  ,fnopUni)
    case knopObject:
        Indent(indentAmt);
        Output::Print(_u("Object Literal\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        // Binary and Ternary Operators
        //PTNODE(knopAdd        , "+"            ,Add     ,Bin  ,fnopBin)
    case knopAdd:
        Indent(indentAmt);
        Output::Print(_u("+\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopSub        , "-"            ,Sub     ,Bin  ,fnopBin)
    case knopSub:
        Indent(indentAmt);
        Output::Print(_u("-\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopMul        , "*"            ,Mul     ,Bin  ,fnopBin)
    case knopMul:
        Indent(indentAmt);
        Output::Print(_u("*\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDiv        , "/"            ,Div     ,Bin  ,fnopBin)
    case knopExpo:
        Indent(indentAmt);
        Output::Print(_u("**\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopExpo        , "**"            ,Expo     ,Bin  ,fnopBin)

    case knopDiv:
        Indent(indentAmt);
        Output::Print(_u("/\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopMod        , "%"            ,Mod     ,Bin  ,fnopBin)
    case knopMod:
        Indent(indentAmt);
        Output::Print(_u("%%\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopOr         , "|"            ,BitOr   ,Bin  ,fnopBin)
    case knopOr:
        Indent(indentAmt);
        Output::Print(_u("|\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopXor        , "^"            ,BitXor  ,Bin  ,fnopBin)
    case knopXor:
        Indent(indentAmt);
        Output::Print(_u("^\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAnd        , "&"            ,BitAnd  ,Bin  ,fnopBin)
    case knopAnd:
        Indent(indentAmt);
        Output::Print(_u("&\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopEq         , "=="        ,EQ      ,Bin  ,fnopBin|fnopRel)
    case knopEq:
        Indent(indentAmt);
        Output::Print(_u("==\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopNe         , "!="        ,NE      ,Bin  ,fnopBin|fnopRel)
    case knopNe:
        Indent(indentAmt);
        Output::Print(_u("!=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLt         , "<"            ,LT      ,Bin  ,fnopBin|fnopRel)
    case knopLt:
        Indent(indentAmt);
        Output::Print(_u("<\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLe         , "<="        ,LE      ,Bin  ,fnopBin|fnopRel)
    case knopLe:
        Indent(indentAmt);
        Output::Print(_u("<=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopGe         , ">="        ,GE      ,Bin  ,fnopBin|fnopRel)
    case knopGe:
        Indent(indentAmt);
        Output::Print(_u(">=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopGt         , ">"            ,GT      ,Bin  ,fnopBin|fnopRel)
    case knopGt:
        Indent(indentAmt);
        Output::Print(_u(">\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopCall       , "()"        ,None    ,Bin  ,fnopBin)
    case knopCall:
        Indent(indentAmt);
        Output::Print(_u("Call\n"));
        PrintPnodeWIndent(pnode->AsParseNodeCall()->pnodeTarget, indentAmt + INDENT_SIZE);
        PrintPnodeListWIndent(pnode->AsParseNodeCall()->pnodeArgs, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDot        , "."            ,None    ,Bin  ,fnopBin)
    case knopDot:
        Indent(indentAmt);
        Output::Print(_u(".\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsg        , "="            ,None    ,Bin  ,fnopBin|fnopAsg)
    case knopAsg:
        Indent(indentAmt);
        Output::Print(_u("=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopInstOf     , "instanceof",InstOf  ,Bin  ,fnopBin|fnopRel)
    case knopInstOf:
        Indent(indentAmt);
        Output::Print(_u("instanceof\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopIn         , "in"        ,In      ,Bin  ,fnopBin|fnopRel)
    case knopIn:
        Indent(indentAmt);
        Output::Print(_u("in\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopEqv        , "==="        ,Eqv     ,Bin  ,fnopBin|fnopRel)
    case knopEqv:
        Indent(indentAmt);
        Output::Print(_u("===\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopNEqv       , "!=="        ,NEqv    ,Bin  ,fnopBin|fnopRel)
    case knopNEqv:
        Indent(indentAmt);
        Output::Print(_u("!==\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopComma      , ","            ,None    ,Bin  ,fnopBin)
    case knopComma:
        Indent(indentAmt);
        Output::Print(_u(",\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLogOr      , "||"        ,None    ,Bin  ,fnopBin)
    case knopLogOr:
        Indent(indentAmt);
        Output::Print(_u("||\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLogAnd     , "&&"        ,None    ,Bin  ,fnopBin)
    case knopLogAnd:
        Indent(indentAmt);
        Output::Print(_u("&&\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopLsh        , "<<"        ,Lsh     ,Bin  ,fnopBin)
    case knopLsh:
        Indent(indentAmt);
        Output::Print(_u("<<\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopRsh        , ">>"        ,Rsh     ,Bin  ,fnopBin)
    case knopRsh:
        Indent(indentAmt);
        Output::Print(_u(">>\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopRs2        , ">>>"        ,Rs2     ,Bin  ,fnopBin)
    case knopRs2:
        Indent(indentAmt);
        Output::Print(_u(">>>\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopNew        , "new"        ,None    ,Bin  ,fnopBin)
    case knopNew:
        Indent(indentAmt);
        Output::Print(_u("new\n"));
        PrintPnodeWIndent(pnode->AsParseNodeCall()->pnodeTarget, indentAmt + INDENT_SIZE);
        PrintPnodeListWIndent(pnode->AsParseNodeCall()->pnodeArgs, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopIndex      , "[]"        ,None    ,Bin  ,fnopBin)
    case knopIndex:
        Indent(indentAmt);
        Output::Print(_u("[]\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeListWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopQmark      , "?"            ,None    ,Tri  ,fnopBin)
    case knopQmark:
        Indent(indentAmt);
        Output::Print(_u("?:\n"));
        PrintPnodeWIndent(pnode->AsParseNodeTri()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeTri()->pnode2, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeTri()->pnode3, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgAdd     , "+="        ,Add     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgAdd:
        Indent(indentAmt);
        Output::Print(_u("+=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgSub     , "-="        ,Sub     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgSub:
        Indent(indentAmt);
        Output::Print(_u("-=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgMul     , "*="        ,Mul     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgMul:
        Indent(indentAmt);
        Output::Print(_u("*=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgDiv     , "/="        ,Div     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgExpo:
        Indent(indentAmt);
        Output::Print(_u("**=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgExpo     , "**="       ,Expo     ,Bin  ,fnopBin|fnopAsg)

    case knopAsgDiv:
        Indent(indentAmt);
        Output::Print(_u("/=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgMod     , "%="        ,Mod     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgMod:
        Indent(indentAmt);
        Output::Print(_u("%=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgAnd     , "&="        ,BitAnd  ,Bin  ,fnopBin|fnopAsg)
    case knopAsgAnd:
        Indent(indentAmt);
        Output::Print(_u("&=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgXor     , "^="        ,BitXor  ,Bin  ,fnopBin|fnopAsg)
    case knopAsgXor:
        Indent(indentAmt);
        Output::Print(_u("^=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgOr      , "|="        ,BitOr   ,Bin  ,fnopBin|fnopAsg)
    case knopAsgOr:
        Indent(indentAmt);
        Output::Print(_u("|=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgLsh     , "<<="        ,Lsh     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgLsh:
        Indent(indentAmt);
        Output::Print(_u("<<=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgRsh     , ">>="        ,Rsh     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgRsh:
        Indent(indentAmt);
        Output::Print(_u(">>=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopAsgRs2     , ">>>="        ,Rs2     ,Bin  ,fnopBin|fnopAsg)
    case knopAsgRs2:
        Indent(indentAmt);
        Output::Print(_u(">>>=\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;

    case knopComputedName:
        Indent(indentAmt);
        Output::Print(_u("ComputedProperty\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;

        //PTNODE(knopMember     , ":"            ,None    ,Bin  ,fnopBin)
    case knopMember:
    case knopMemberShort:
    case knopObjectPatternMember:
        Indent(indentAmt);
        Output::Print(_u(":\n"));
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode2, indentAmt + INDENT_SIZE);
        break;
        // General nodes.
        //PTNODE(knopList       , "<list>"    ,None    ,Bin  ,fnopNone)
    case knopList:
        Indent(indentAmt);
        Output::Print(_u("List\n"));
        PrintPnodeListWIndent(pnode, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopVarDecl    , "varDcl"    ,None    ,Var  ,fnopNone)
    case knopVarDecl:
        Indent(indentAmt);
        Output::Print(_u("var %s\n"), pnode->AsParseNodeVar()->pid->Psz());
        if (pnode->AsParseNodeVar()->pnodeInit != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeVar()->pnodeInit, indentAmt + INDENT_SIZE);
        break;
    case knopConstDecl:
        Indent(indentAmt);
        Output::Print(_u("const %s\n"), pnode->AsParseNodeVar()->pid->Psz());
        if (pnode->AsParseNodeVar()->pnodeInit != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeVar()->pnodeInit, indentAmt + INDENT_SIZE);
        break;
    case knopLetDecl:
        Indent(indentAmt);
        Output::Print(_u("let %s\n"), pnode->AsParseNodeVar()->pid->Psz());
        if (pnode->AsParseNodeVar()->pnodeInit != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeVar()->pnodeInit, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopFncDecl    , "fncDcl"    ,None    ,Fnc  ,fnopLeaf)
    case knopFncDecl:
        Indent(indentAmt);
        if (pnode->AsParseNodeFnc()->pid != NULL)
        {
            Output::Print(_u("fn decl %d nested %d name %s (%d-%d)\n"), pnode->AsParseNodeFnc()->IsDeclaration(), pnode->AsParseNodeFnc()->IsNested(),
                pnode->AsParseNodeFnc()->pid->Psz(), pnode->ichMin, pnode->ichLim);
        }
        else
        {
            Output::Print(_u("fn decl %d nested %d anonymous (%d-%d)\n"), pnode->AsParseNodeFnc()->IsDeclaration(), pnode->AsParseNodeFnc()->IsNested(), pnode->ichMin, pnode->ichLim);
        }
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintFormalsWIndent(pnode->AsParseNodeFnc()->pnodeParams, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFnc()->pnodeRest, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFnc()->pnodeBody, indentAmt + INDENT_SIZE);
        if (pnode->AsParseNodeFnc()->pnodeBody == nullptr)
        {
            Output::Print(_u("[%4d, %4d): "), pnode->ichMin, pnode->ichLim);
            Indent(indentAmt + INDENT_SIZE);
            Output::Print(_u("<parse deferred body>\n"));
        }
        break;
        //PTNODE(knopProg       , "program"    ,None    ,Fnc  ,fnopNone)
    case knopProg:
        Indent(indentAmt);
        Output::Print(_u("program\n"));
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeListWIndent(pnode->AsParseNodeFnc()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopEndCode    , "<endcode>"    ,None    ,None ,fnopNone)
    case knopEndCode:
        Indent(indentAmt);
        Output::Print(_u("<endcode>\n"));
        break;
        //PTNODE(knopDebugger   , "debugger"    ,None    ,None ,fnopNone)
    case knopDebugger:
        Indent(indentAmt);
        Output::Print(_u("<debugger>\n"));
        break;
        //PTNODE(knopFor        , "for"        ,None    ,For  ,fnopBreak|fnopContinue)
    case knopFor:
        Indent(indentAmt);
        Output::Print(_u("for\n"));
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFor()->pnodeInit, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFor()->pnodeCond, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFor()->pnodeIncr, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeFor()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopIf         , "if"        ,None    ,If   ,fnopNone)
    case knopIf:
        Indent(indentAmt);
        Output::Print(_u("if\n"));
        PrintPnodeWIndent(pnode->AsParseNodeIf()->pnodeCond, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeIf()->pnodeTrue, indentAmt + INDENT_SIZE);
        if (pnode->AsParseNodeIf()->pnodeFalse != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeIf()->pnodeFalse, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopWhile      , "while"        ,None    ,While,fnopBreak|fnopContinue)
    case knopWhile:
        Indent(indentAmt);
        Output::Print(_u("while\n"));
        PrintPnodeWIndent(pnode->AsParseNodeWhile()->pnodeCond, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeWhile()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopDoWhile    , "do-while"    ,None    ,While,fnopBreak|fnopContinue)
    case knopDoWhile:
        Indent(indentAmt);
        Output::Print(_u("do\n"));
        PrintPnodeWIndent(pnode->AsParseNodeWhile()->pnodeCond, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeWhile()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopForIn      , "for in"    ,None    ,ForIn,fnopBreak|fnopContinue|fnopCleanup)
    case knopForIn:
        Indent(indentAmt);
        Output::Print(_u("forIn\n"));
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeLval, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeObj, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
    case knopForOf:
        Indent(indentAmt);
        Output::Print(_u("forOf\n"));
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeLval, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeObj, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeForInOrForOf()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopReturn     , "return"    ,None    ,Uni  ,fnopNone)
    case knopReturn:
        Indent(indentAmt);
        Output::Print(_u("return\n"));
        if (pnode->AsParseNodeReturn()->pnodeExpr != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeReturn()->pnodeExpr, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopBlock      , "{}"        ,None    ,Block,fnopNone)
    case knopBlock:
        Indent(indentAmt);
        Output::Print(_u("block "));
        if (pnode->grfpn & fpnSyntheticNode)
            Output::Print(_u("synthetic "));
        PrintBlockType(pnode->AsParseNodeBlock()->blockType);
        Output::Print(_u("(%d-%d)\n"), pnode->ichMin, pnode->ichLim);
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        if (pnode->AsParseNodeBlock()->pnodeStmt != NULL)
            PrintPnodeWIndent(pnode->AsParseNodeBlock()->pnodeStmt, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopWith       , "with"        ,None    ,With ,fnopCleanup)
    case knopWith:
        Indent(indentAmt);
        Output::Print(_u("with (%d-%d)\n"), pnode->ichMin, pnode->ichLim);
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeWith()->pnodeObj, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeWith()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopBreak      , "break"        ,None    ,Jump ,fnopNone)
    case knopBreak:
        Indent(indentAmt);
        Output::Print(_u("break\n"));
        // TODO: some representation of target
        break;
        //PTNODE(knopContinue   , "continue"    ,None    ,Jump ,fnopNone)
    case knopContinue:
        Indent(indentAmt);
        Output::Print(_u("continue\n"));
        // TODO: some representation of target
        break;
        //PTNODE(knopSwitch     , "switch"    ,None    ,Switch,fnopBreak)
    case knopSwitch:
        Indent(indentAmt);
        Output::Print(_u("switch\n"));
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        for (ParseNodeCase *pnodeT = pnode->AsParseNodeSwitch()->pnodeCases; NULL != pnodeT; pnodeT = pnodeT->pnodeNext) {
            PrintPnodeWIndent(pnodeT, indentAmt + 2);
        }
        break;
        //PTNODE(knopCase       , "case"        ,None    ,Case ,fnopNone)
    case knopCase:
        Indent(indentAmt);
        Output::Print(_u("case\n"));
        PrintPnodeWIndent(pnode->AsParseNodeCase()->pnodeExpr, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeCase()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopTryFinally,"try-finally",None,TryFinally,fnopCleanup)
    case knopTryFinally:
        PrintPnodeWIndent(pnode->AsParseNodeTryFinally()->pnodeTry, indentAmt);
        PrintPnodeWIndent(pnode->AsParseNodeTryFinally()->pnodeFinally, indentAmt);
        break;
    case knopFinally:
        Indent(indentAmt);
        Output::Print(_u("finally\n"));
        PrintPnodeWIndent(pnode->AsParseNodeFinally()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopCatch      , "catch"     ,None    ,Catch,fnopNone)
    case knopCatch:
        Indent(indentAmt);
        Output::Print(_u("catch (%d-%d)\n"), pnode->ichMin, pnode->ichLim);
        PrintScopesWIndent(pnode, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeCatch()->GetParam(), indentAmt + INDENT_SIZE);
        //      if (pnode->AsParseNodeCatch()->pnodeGuard!=NULL)
        //          PrintPnodeWIndent(pnode->AsParseNodeCatch()->pnodeGuard,indentAmt+INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeCatch()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopTryCatch      , "try-catch" ,None    ,TryCatch  ,fnopCleanup)
    case knopTryCatch:
        PrintPnodeWIndent(pnode->AsParseNodeTryCatch()->pnodeTry, indentAmt);
        PrintPnodeWIndent(pnode->AsParseNodeTryCatch()->pnodeCatch, indentAmt);
        break;
        //PTNODE(knopTry        , "try"       ,None    ,Try  ,fnopCleanup)
    case knopTry:
        Indent(indentAmt);
        Output::Print(_u("try\n"));
        PrintPnodeWIndent(pnode->AsParseNodeTry()->pnodeBody, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopThrow      , "throw"     ,None    ,Uni  ,fnopNone)
    case knopThrow:
        Indent(indentAmt);
        Output::Print(_u("throw\n"));
        PrintPnodeWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
        //PTNODE(knopClassDecl, "classDecl", None , Class, fnopLeaf)
    case knopClassDecl:
        Indent(indentAmt);
        Output::Print(_u("class %s"), pnode->AsParseNodeClass()->pnodeName->pid->Psz());
        if (pnode->AsParseNodeClass()->pnodeExtends != nullptr)
        {
            Output::Print(_u(" extends "));
            PrintPnodeWIndent(pnode->AsParseNodeClass()->pnodeExtends, 0);
        }
        else {
            Output::Print(_u("\n"));
        }

        PrintPnodeWIndent(pnode->AsParseNodeClass()->pnodeConstructor, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeClass()->pnodeMembers, indentAmt + INDENT_SIZE);
        PrintPnodeWIndent(pnode->AsParseNodeClass()->pnodeStaticMembers, indentAmt + INDENT_SIZE);
        break;
    case knopStrTemplate:
        Indent(indentAmt);
        Output::Print(_u("string template\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeStrTemplate()->pnodeSubstitutionExpressions, indentAmt + INDENT_SIZE);
        break;
    case knopYieldStar:
        Indent(indentAmt);
        Output::Print(_u("yield*\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
    case knopYield:
    case knopYieldLeaf:
        Indent(indentAmt);
        Output::Print(_u("yield\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
    case knopAwait:
        Indent(indentAmt);
        Output::Print(_u("await\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeUni()->pnode1, indentAmt + INDENT_SIZE);
        break;
    case knopExportDefault:
        Indent(indentAmt);
        Output::Print(_u("export default\n"));
        PrintPnodeListWIndent(pnode->AsParseNodeExportDefault()->pnodeExpr, indentAmt + INDENT_SIZE);
        break;
    default:
        Output::Print(_u("unhandled pnode op %d\n"), pnode->nop);
        break;
    }
}

void PrintPnodeListWIndent(ParseNode *pnode, int indentAmt) {
    if (pnode != NULL) {
        while (pnode->nop == knopList) {
            PrintPnodeWIndent(pnode->AsParseNodeBin()->pnode1, indentAmt);
            pnode = pnode->AsParseNodeBin()->pnode2;
        }
        PrintPnodeWIndent(pnode, indentAmt);
    }
}

void PrintFormalsWIndent(ParseNode *pnodeArgs, int indentAmt)
{
    for (ParseNode *pnode = pnodeArgs; pnode != nullptr; pnode = pnode->GetFormalNext())
    {
        PrintPnodeWIndent(pnode->nop == knopParamPattern ? pnode->AsParseNodeParamPattern()->pnode1 : pnode, indentAmt);
    }
}

void PrintPnode(ParseNode *pnode) {
    PrintPnodeWIndent(pnode, 0);
}

void ParseNode::Dump()
{
    switch (nop)
    {
    case knopFncDecl:
    case knopProg:
        LPCOLESTR name = Js::Constants::AnonymousFunction;
        if (this->AsParseNodeFnc()->pnodeName)
        {
            Assert(this->AsParseNodeFnc()->pnodeName->nop == knopVarDecl);
            name = this->AsParseNodeFnc()->pnodeName->pid->Psz();
        }

        Output::Print(_u("%s (%d) [%d, %d]:\n"), name, this->AsParseNodeFnc()->functionId, this->AsParseNodeFnc()->lineNumber, this->AsParseNodeFnc()->columnNumber);
        Output::Print(_u("hasArguments: %s callsEval:%s childCallsEval:%s HasReferenceableBuiltInArguments:%s ArgumentsObjectEscapes:%s HasWith:%s HasOnlyThis:%s \n"),
            IsTrueOrFalse(this->AsParseNodeFnc()->HasHeapArguments()),
            IsTrueOrFalse(this->AsParseNodeFnc()->CallsEval()),
            IsTrueOrFalse(this->AsParseNodeFnc()->ChildCallsEval()),
            IsTrueOrFalse(this->AsParseNodeFnc()->HasReferenceableBuiltInArguments()),
            IsTrueOrFalse(this->AsParseNodeFnc()->GetArgumentsObjectEscapes()),
            IsTrueOrFalse(this->AsParseNodeFnc()->HasWithStmt()),
            IsTrueOrFalse(this->AsParseNodeFnc()->HasOnlyThisStmts()));
        if (this->AsParseNodeFnc()->funcInfo)
        {
            this->AsParseNodeFnc()->funcInfo->Dump();
        }
        break;
    }
}

void DumpCapturedNames(ParseNodeFnc* pnodeFnc, IdentPtrSet* capturedNames, ArenaAllocator* alloc)
{
    auto sortedNames = JsUtil::List<IdentPtr, ArenaAllocator>::New(alloc);

    capturedNames->Map([=](const IdentPtr& pid) -> void {
        sortedNames->Add(pid);
    });

    sortedNames->Sort([](void* context, const void* left, const void* right) -> int {
        const IdentPtr leftIdentPtr = *(const IdentPtr*)(left);
        const IdentPtr rightIdentPtr = *(const IdentPtr*)(right);
        return ::wcscmp(leftIdentPtr->Psz(), rightIdentPtr->Psz());
    }, nullptr);

    sortedNames->Map([=](int index, const IdentPtr pid) -> void {
        OUTPUT_TRACE_DEBUGONLY(Js::CreateParserStatePhase, _u(" Function %u captured name \"%s\"\n"), pnodeFnc->functionId, pid->Psz());
    });
}
#endif

void Parser::AddNestedCapturedNames(ParseNodeFnc* pnodeChildFnc)
{
    if (m_currentNodeFunc && this->IsCreatingStateCache() && pnodeChildFnc->HasAnyCapturedNames())
    {
        IdentPtrSet* parentCapturedNames = GetCurrentFunctionNode()->EnsureCapturedNames(&m_nodeAllocator);
        IdentPtrSet* childCaptureNames = pnodeChildFnc->GetCapturedNames();

        auto iter = childCaptureNames->GetIterator();

        while (iter.IsValid())
        {
            parentCapturedNames->AddNew(iter.CurrentValue());
            iter.MoveNext();
        }
    }
}

void Parser::ProcessCapturedNames(ParseNodeFnc* pnodeFnc)
{
    if (this->IsCreatingStateCache() && pnodeFnc->HasAnyCapturedNames())
    {
        IdentPtrSet* capturedNames = pnodeFnc->GetCapturedNames();
        auto iter = capturedNames->GetIteratorWithRemovalSupport();

        while (iter.IsValid())
        {
            const IdentPtr& pid = iter.CurrentValueReference();
            PidRefStack* ref = pid->GetTopRef();

            // If the pid has no refs left in our function's scope after binding, we didn't capture it.
            if (!ref || ref->GetFuncScopeId() < pnodeFnc->functionId)
            {
                iter.RemoveCurrent();
            }

            iter.MoveNext();
        }

#if DBG_DUMP
        if (Js::Configuration::Global.flags.Trace.IsEnabled(Js::CreateParserStatePhase))
        {
            DumpCapturedNames(pnodeFnc, capturedNames, &this->m_nodeAllocator);
            fflush(stdout);
        }
#endif
    }
}

void Parser::ReleaseTemporaryGuestArena()
{
    // In case of modules the Parser lives longer than the temporary Guest Arena. We may have already released the arena explicitly.
    if (!m_tempGuestArenaReleased)
    {
        // The regex patterns list has references to the temporary Guest Arena. Reset it first.
        m_registeredRegexPatterns.Reset();

        if (this->m_scriptContext != nullptr)
        {
            this->m_scriptContext->ReleaseTemporaryGuestAllocator(m_tempGuestArena);
            m_tempGuestArena.Unroot();
        }

        m_tempGuestArenaReleased = true;
    }
}

bool Parser::IsCreatingStateCache()
{
    return (((this->m_grfscr & fscrCreateParserState) == fscrCreateParserState)
        && this->m_functionBody == nullptr
        && CONFIG_FLAG(ParserStateCache));
}

DeferredFunctionStub * Parser::BuildDeferredStubTree(ParseNodeFnc *pnodeFnc, Recycler *recycler)
{
    Assert(CONFIG_FLAG(ParserStateCache));

    uint nestedCount = pnodeFnc->nestedCount;
    if (nestedCount == 0)
    {
        return nullptr;
    }

    if (pnodeFnc->deferredStub)
    {
        return pnodeFnc->deferredStub;
    }

    DeferredFunctionStub* deferredStubs = RecyclerNewArray(recycler, DeferredFunctionStub, nestedCount);
    uint i = 0;
    ParseNodeBlock* pnodeBlock = pnodeFnc->pnodeBodyScope;
    ParseNodePtr pnodeChild = nullptr;

    if (pnodeFnc->nop == knopProg)
    {
        Assert(pnodeFnc->pnodeBodyScope == nullptr
            && pnodeFnc->pnodeScopes != nullptr
            && pnodeFnc->pnodeScopes->blockType == PnodeBlockType::Global);

        pnodeBlock = pnodeFnc->pnodeScopes;
        pnodeChild = pnodeFnc->pnodeScopes->pnodeScopes;
    }
    else
    {
        Assert(pnodeBlock != nullptr
            && (pnodeBlock->blockType == PnodeBlockType::Function
                || pnodeBlock->blockType == PnodeBlockType::Parameter));

        pnodeChild = pnodeBlock->pnodeScopes;
    }

    while (pnodeChild != nullptr)
    {
        if (pnodeChild->nop != knopFncDecl)
        {
            // We only expect to find a function body block in a parameter scope block.
            Assert(pnodeChild->nop == knopBlock
                && (pnodeBlock->blockType == PnodeBlockType::Parameter
                    || pnodeChild->AsParseNodeBlock()->blockType == PnodeBlockType::Function));
            pnodeChild = pnodeChild->AsParseNodeBlock()->pnodeNext;
            continue;
        }

        ParseNodeFnc* pnodeFncChild = pnodeChild->AsParseNodeFnc();
        AnalysisAssertOrFailFast(i < nestedCount);

        if (pnodeFncChild->pnodeBody != nullptr)
        {
            // Anomalous case of a non-deferred function nested within a deferred one.
            // Work around by discarding the stub tree.
            return nullptr;
        }

        if (pnodeFncChild->IsGeneratedDefault())
        {
            ++i;
            pnodeChild = pnodeFncChild->pnodeNext;
            continue;
        }

        deferredStubs[i].fncFlags = pnodeFncChild->fncFlags;
        deferredStubs[i].nestedCount = pnodeFncChild->nestedCount;
        deferredStubs[i].restorePoint = *pnodeFncChild->pRestorePoint;
        deferredStubs[i].deferredStubs = BuildDeferredStubTree(pnodeFncChild, recycler);
        deferredStubs[i].ichMin = pnodeChild->ichMin;

        // Save the set of captured names onto the deferred stub.
        // Since this set is allocated in the Parser arena, we'll have to convert these
        // into indices in a string table which will survive when the parser goes away.
        deferredStubs[i].capturedNamePointers = pnodeFncChild->GetCapturedNames();

        ++i;
        pnodeChild = pnodeFncChild->pnodeNext;
    }

    pnodeFnc->deferredStub = deferredStubs;
    return deferredStubs;
}
