//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "ParseFlags.h"

namespace Js
{
    class ScopeInfo;
};

// Operator precedence levels
enum
{
    koplNo,     // not an operator
    koplCma,    // ,
    koplSpr,    // ...
    koplAsg,    // = += etc
    koplQue,    // ?:
    koplLor,    // ||
    koplLan,    // &&
    koplBor,    // |
    koplXor,    // ^
    koplBan,    // &
    koplEqu,    // == !=
    koplCmp,    // < <= > >=
    koplShf,    // << >> >>>
    koplAdd,    // + -
    koplMul,    // * / %
    koplExpo,   // **
    koplUni,    // unary operators
    koplLim
};
enum ParseType
{
    ParseType_Upfront,
    ParseType_Deferred
};

enum DestructuringInitializerContext
{
    DIC_None,
    DIC_ShouldNotParseInitializer, // e.g. We don't want to parse the initializer even though we found assignment
    DIC_ForceErrorOnInitializer, // e.g. Catch param where we explicitly want to raise an error when the initializer found
};

enum ScopeType: int;
enum SymbolType : byte;

// Representation of a label used when no AST is being built.
struct LabelId
{
    IdentPtr pid;
    struct LabelId* next;
};

typedef ArenaAllocator ParseNodeAllocator;

/***************************************************************************
Parser object.
***************************************************************************/
class CompileScriptException;
class Parser;
class SourceContextInfo;
struct BlockIdsStack;
class Span;
class BackgroundParser;
struct BackgroundParseItem;
struct PnClass;
class HashTbl;

struct PidRefStack;

struct DeferredFunctionStub;

DeferredFunctionStub * BuildDeferredStubTree(ParseNode *pnodeFnc, Recycler *recycler);

struct StmtNest;
struct BlockInfoStack;
struct ParseContext
{
    LPCUTF8 pszSrc;
    size_t offset;
    size_t length;
    charcount_t characterOffset;
    int nextBlockId;
    ULONG grfscr;
    ULONG lineNumber;
    ParseNodePtr pnodeProg;
    SourceContextInfo* sourceContextInfo;
    BlockInfoStack* currentBlockInfo;
    bool strictMode;
    bool fromExternal;
};

template <bool nullTerminated> class UTF8EncodingPolicyBase;
typedef UTF8EncodingPolicyBase<false> NotNullTerminatedUTF8EncodingPolicy;
template <typename T> class Scanner;

namespace Js
{
    class ParseableFunctionInfo;
    class FunctionBody;
};

class Parser
{
    typedef Scanner<NotNullTerminatedUTF8EncodingPolicy> Scanner_t;

private:
    template <OpCode nop> static int GetNodeSize();

    template <OpCode nop> static ParseNodePtr StaticAllocNode(ArenaAllocator * alloc)
    {
        ParseNodePtr pnode = (ParseNodePtr)alloc->Alloc(GetNodeSize<nop>());
        Assert(pnode != nullptr);
        return pnode;
    }


public:
#if DEBUG
    Parser(Js::ScriptContext* scriptContext, BOOL strictMode = FALSE, PageAllocator *alloc = nullptr, bool isBackground = false, size_t size = sizeof(Parser));
#else
    Parser(Js::ScriptContext* scriptContext, BOOL strictMode = FALSE, PageAllocator *alloc = nullptr, bool isBackground = false);
#endif
    ~Parser(void);

    Js::ScriptContext* GetScriptContext() const { return m_scriptContext; }
    void ClearScriptContext() { m_scriptContext = nullptr; }

#if ENABLE_BACKGROUND_PARSING
    bool IsBackgroundParser() const { return m_isInBackground; }
    bool IsDoingFastScan() const { return m_doingFastScan; }
#else
    bool IsBackgroundParser() const { return false; }
    bool IsDoingFastScan() const { return false; }
#endif

    static IdentPtr PidFromNode(ParseNodePtr pnode);

    ParseNode* CopyPnode(ParseNode* pnode);

    ArenaAllocator *GetAllocator() { return &m_nodeAllocator;}

    size_t GetSourceLength() { return m_length; }
    size_t GetOriginalSourceLength() { return m_originalLength; }
    static ULONG GetDeferralThreshold(bool isProfileLoaded);
    BOOL DeferredParse(Js::LocalFunctionId functionId);
    BOOL IsDeferredFnc();
    void ReduceDeferredScriptLength(size_t chars);

    void RestorePidRefForSym(Symbol *sym);

    HRESULT ValidateSyntax(LPCUTF8 pszSrc, size_t encodedCharCount, bool isGenerator, bool isAsync, CompileScriptException *pse, void (Parser::*validateFunction)());

    // Should be called when the UTF-8 source was produced from UTF-16. This is really CESU-8 source in that it encodes surrogate pairs
    // as 2 three byte sequences instead of 4 bytes as required by UTF-8. It also is a lossless conversion of invalid UTF-16 sequences.
    // This is important in Javascript because Javascript engines are required not to report invalid UTF-16 sequences and to consider
    // the UTF-16 characters pre-canonicalization. Converting this UTF-16 with invalid sequences to valid UTF-8 and back would cause
    // all invalid UTF-16 sequences to be replaced by one or more Unicode replacement characters (0xFFFD), losing the original
    // invalid sequences.
    HRESULT ParseCesu8Source(__out ParseNodePtr* parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
        Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo);

    // Should be called when the source is UTF-8 and invalid UTF-8 sequences should be replaced with the unicode replacement character
    // (0xFFFD). Security concerns require externally produced UTF-8 only allow valid UTF-8 otherwise an attacker could use invalid
    // UTF-8 sequences to fool a filter and cause Javascript to be executed that might otherwise have been rejected.
    HRESULT ParseUtf8Source(__out ParseNodePtr* parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
        Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo);

    // Used by deferred parsing to parse a deferred function.
    HRESULT ParseSourceWithOffset(__out ParseNodePtr* parseTree, LPCUTF8 pSrc, size_t offset, size_t cbLength, charcount_t cchOffset,
        bool isCesu8, ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber,
        SourceContextInfo * sourceContextInfo, Js::ParseableFunctionInfo* functionInfo);

protected:
    HRESULT ParseSourceInternal(
        __out ParseNodePtr* parseTree, LPCUTF8 pszSrc, size_t offsetInBytes,
        size_t lengthInCodePoints, charcount_t offsetInChars, bool fromExternal,
        ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber, SourceContextInfo * sourceContextInfo);

    ParseNodePtr Parse(LPCUTF8 pszSrc, size_t offset, size_t length, charcount_t charOffset, ULONG grfscr, ULONG lineNumber,
        Js::LocalFunctionId * nextFunctionId, CompileScriptException *pse);

private:
    /***********************************************************************
    Core members.
    ***********************************************************************/
    ParseNodeAllocator m_nodeAllocator;
    int32        m_cactIdentToNodeLookup;
    uint32       m_grfscr;
    size_t      m_length;             // source length in characters excluding comments and literals
    size_t      m_originalLength;             // source length in characters excluding comments and literals
    Js::LocalFunctionId * m_nextFunctionId;
    SourceContextInfo*    m_sourceContextInfo;

#if ENABLE_BACKGROUND_PARSING
    bool                m_hasParallelJob;
    bool                m_isInBackground;
    bool                m_doingFastScan;
#endif
    int                 m_nextBlockId;

    // RegexPattern objects created for literal regexes are recycler-allocated and need to be kept alive until the function body
    // is created during byte code generation. The RegexPattern pointer is stored in the script context's guest
    // arena for that purpose. This list is then unregistered from the guest arena at the end of parsing/scanning.
    SList<UnifiedRegex::RegexPattern *, ArenaAllocator> m_registeredRegexPatterns;

protected:
    Js::ScriptContext* m_scriptContext;
    HashTbl *   m_phtbl;

    static const uint HASH_TABLE_SIZE = 256;

    __declspec(noreturn) void Error(HRESULT hr);
private:
    __declspec(noreturn) void Error(HRESULT hr, ParseNodePtr pnode);
    __declspec(noreturn) void Error(HRESULT hr, charcount_t ichMin, charcount_t ichLim);
    __declspec(noreturn) static void OutOfMemory();

    void GenerateCode(ParseNodePtr pnode, void *pvUser, int32 cbUser,
        LPCOLESTR pszSrc, int32 cchSrc, LPCOLESTR pszTitle);

    void EnsureStackAvailable();

    void IdentifierExpectedError(const Token& token);

    bool CheckForDirective(bool* pIsUseStrict, bool* pIsUseAsm, bool* pIsOctalInString);
    bool CheckStrictModeStrPid(IdentPtr pid);
    bool CheckAsmjsModeStrPid(IdentPtr pid);

    bool IsCurBlockInLoop() const;

    void InitPids();

    /***********************************************************************
    Members needed just for parsing.
    ***********************************************************************/
protected:
    Token       m_token;
    Scanner_t*  m_pscan;

public:

    // create nodes using arena allocator; used by AST transformation
    template <OpCode nop>
    static ParseNodePtr StaticCreateNodeT(ArenaAllocator* alloc, charcount_t ichMin = 0, charcount_t ichLim = 0)
    {
        ParseNodePtr pnode = StaticAllocNode<nop>(alloc);
        InitNode(nop,pnode);
        // default - may be changed
        pnode->ichMin = ichMin;
        pnode->ichLim = ichLim;

        return pnode;
    }

    static ParseNodePtr StaticCreateBinNode(OpCode nop, ParseNodePtr pnode1,ParseNodePtr pnode2,ArenaAllocator* alloc);
    static ParseNodePtr StaticCreateBlockNode(ArenaAllocator* alloc, charcount_t ichMin = 0, charcount_t ichLim = 0, int blockId = -1, PnodeBlockType blockType = PnodeBlockType::Regular);
    ParseNodePtr CreateNode(OpCode nop, charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateDummyFuncNode(bool fDeclaration);


    ParseNodePtr CreateTriNode(OpCode nop, ParseNodePtr pnode1,
                               ParseNodePtr pnode2, ParseNodePtr pnode3,
                               charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateTempNode(ParseNode* initExpr);
    ParseNodePtr CreateTempRef(ParseNode* tempNode);

    ParseNodePtr CreateNode(OpCode nop) { return CreateNode(nop, m_pscan? m_pscan->IchMinTok() : 0); }
    ParseNodePtr CreateDeclNode(OpCode nop, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl = true, bool *isRedecl = nullptr);
    Symbol*      AddDeclForPid(ParseNodePtr pnode, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl, bool *isRedecl = nullptr);
    void         CheckRedeclarationErrorForBlockId(IdentPtr pid, int blockId);
    ParseNodePtr CreateNameNode(IdentPtr pid)
    {
        ParseNodePtr pnode = CreateNode(knopName);
        pnode->sxPid.pid = pid;
        pnode->sxPid.sym=NULL;
        pnode->sxPid.symRef=NULL;
        return pnode;
    }
    ParseNodePtr CreateBlockNode(PnodeBlockType blockType = PnodeBlockType::Regular)
    {
        ParseNodePtr pnode = CreateNode(knopBlock);
        InitBlockNode(pnode, m_nextBlockId++, blockType);
        return pnode;
    }
    // Creating parse nodes.

    ParseNodePtr CreateNode(OpCode nop, charcount_t ichMin);
    ParseNodePtr CreateTriNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, ParseNodePtr pnode3);
    ParseNodePtr CreateIntNode(int32 lw);
    ParseNodePtr CreateStrNode(IdentPtr pid);

    ParseNodePtr CreateUniNode(OpCode nop, ParseNodePtr pnodeOp);
    ParseNodePtr CreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2);
    ParseNodePtr CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2);

    // Create parse node with token limis
    template <OpCode nop>
    ParseNodePtr CreateNodeT(charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateUniNode(OpCode nop, ParseNodePtr pnode1, charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateBlockNode(charcount_t ichMin,charcount_t ichLim, PnodeBlockType blockType = PnodeBlockType::Regular);
    ParseNodePtr CreateNameNode(IdentPtr pid,charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2,
        charcount_t ichMin,charcount_t ichLim);
    ParseNodePtr CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2,
        charcount_t ichMin,charcount_t ichLim);

    void PrepareScanner(bool fromExternal);
#if ENABLE_BACKGROUND_PARSING
    void PrepareForBackgroundParse();
    void AddFastScannedRegExpNode(ParseNodePtr const pnode);
    void AddBackgroundRegExpNode(ParseNodePtr const pnode);
    void AddBackgroundParseItem(BackgroundParseItem *const item);
    void FinishBackgroundRegExpNodes();
    void FinishBackgroundPidRefs(BackgroundParseItem *const item, bool isOtherParser);
    void WaitForBackgroundJobs(BackgroundParser *bgp, CompileScriptException *pse);
    HRESULT ParseFunctionInBackground(ParseNodePtr pnodeFunc, ParseContext *parseContext, bool topLevelDeferred, CompileScriptException *pse);
#endif

    void CheckPidIsValid(IdentPtr pid, bool autoArgumentsObject = false);
    void AddVarDeclToBlock(ParseNode *pnode);
    // Add a var declaration. Only use while parsing. Assumes m_ppnodeVar is pointing to the right place already
    ParseNodePtr CreateVarDeclNode(IdentPtr pid, SymbolType symbolType, bool autoArgumentsObject = false, ParseNodePtr pnodeFnc = NULL, bool checkReDecl = true, bool *isRedecl = nullptr);
    // Add a var declaration, during parse tree rewriting. Will setup m_ppnodeVar for the given pnodeFnc
    ParseNodePtr AddVarDeclNode(IdentPtr pid, ParseNodePtr pnodeFnc);
    // Add a 'const' or 'let' declaration.
    ParseNodePtr CreateBlockScopedDeclNode(IdentPtr pid, OpCode nodeType);

    void RegisterRegexPattern(UnifiedRegex::RegexPattern *const regexPattern);

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    LPCWSTR GetParseType() const
    {
        switch(m_parseType)
        {
            case ParseType_Upfront:
                return _u("Upfront");
            case ParseType_Deferred:
                return _u("Deferred");
        }
        Assert(false);
        return NULL;
    }


#endif

    void CaptureContext(ParseContext *parseContext) const;
    void RestoreContext(ParseContext *const parseContext);
    int GetLastBlockId() const { Assert(m_nextBlockId > 0); return m_nextBlockId - 1; }

private:
    template <OpCode nop> ParseNodePtr CreateNodeWithScanner();
    template <OpCode nop> ParseNodePtr CreateNodeWithScanner(charcount_t ichMin);
    ParseNodePtr CreateStrNodeWithScanner(IdentPtr pid);
    ParseNodePtr CreateIntNodeWithScanner(int32 lw);
    ParseNodePtr CreateProgNodeWithScanner(bool isModuleSource);

    static void InitNode(OpCode nop,ParseNodePtr pnode);
    static void InitBlockNode(ParseNodePtr pnode, int blockId, PnodeBlockType blockType);

private:
    ParseNodePtr m_currentNodeNonLambdaFunc; // current function or NULL
    ParseNodePtr m_currentNodeNonLambdaDeferredFunc; // current function or NULL
    ParseNodePtr m_currentNodeFunc; // current function or NULL
    ParseNodePtr m_currentNodeDeferredFunc; // current function or NULL
    ParseNodePtr m_currentNodeProg; // current program
    DeferredFunctionStub *m_currDeferredStub;
    DeferredFunctionStub *m_prevSiblingDeferredStub;
    int32 * m_pCurrentAstSize;
    ParseNodePtr * m_ppnodeScope;  // function list tail
    ParseNodePtr * m_ppnodeExprScope; // function expression list tail
    ParseNodePtr * m_ppnodeVar;  // variable list tail
    bool m_inDeferredNestedFunc; // true if parsing a function in deferred mode, nested within the current node  
    bool m_reparsingLambdaParams;
    bool m_disallowImportExportStmt;

    // This bool is used for deferring the shorthand initializer error ( {x = 1}) - as it is allowed in the destructuring grammar.
    bool m_hasDeferredShorthandInitError;
    uint * m_pnestedCount; // count of functions nested at one level below the current node

    struct WellKnownPropertyPids
    {
        IdentPtr arguments;
        IdentPtr async;
        IdentPtr eval;
        IdentPtr set;
        IdentPtr get;
        IdentPtr let;
        IdentPtr constructor;
        IdentPtr prototype;
        IdentPtr __proto__;
        IdentPtr of;
        IdentPtr target;
        IdentPtr from;
        IdentPtr as;
        IdentPtr _default;
        IdentPtr _star; // Special '*' identifier for modules
        IdentPtr _starDefaultStar; // Special '*default*' identifier for modules
    };

    WellKnownPropertyPids wellKnownPropertyPids;

    charcount_t m_sourceLim; // The actual number of characters parsed.

    Js::ParseableFunctionInfo* m_functionBody; // For a deferred parsed function, the function body is non-null
    ParseType m_parseType;

    uint m_arrayDepth;
    uint m_funcInArrayDepth; // Count func depth within array literal
    charcount_t m_funcInArray;
    uint m_scopeCountNoAst;


    /*
     * Parsing states for super restriction
     */
    static const uint ParsingSuperRestrictionState_SuperDisallowed = 0;
    static const uint ParsingSuperRestrictionState_SuperCallAndPropertyAllowed = 1;
    static const uint ParsingSuperRestrictionState_SuperPropertyAllowed = 2;
    uint m_parsingSuperRestrictionState;
    friend class AutoParsingSuperRestrictionStateRestorer;

    // Used for issuing spread and rest errors when there is ambiguity with parameter list and parenthesized expressions
    uint m_parenDepth;
    bool m_deferEllipsisError;
    RestorePoint m_EllipsisErrLoc;

    uint m_tryCatchOrFinallyDepth;  // Used to determine if parsing is currently in a try/catch/finally block in order to throw error on yield expressions inside them

    StmtNest *m_pstmtCur; // current statement or NULL
    BlockInfoStack *m_currentBlockInfo;
    Scope *m_currentScope;

    BackgroundParseItem *currBackgroundParseItem;
    BackgroundParseItem *backgroundParseItems;
    typedef DList<ParseNodePtr, ArenaAllocator> NodeDList;
    NodeDList* fastScannedRegExpNodes;

    BlockIdsStack *m_currentDynamicBlock;
    int GetCurrentDynamicBlockId() const;

    void AppendFunctionToScopeList(bool fDeclaration, ParseNodePtr pnodeFnc);

    // block scoped content helpers
    void SetCurrentStatement(StmtNest *stmt);
    ParseNode* GetCurrentBlock();
    ParseNode* GetFunctionBlock();
    BlockInfoStack* GetCurrentBlockInfo();
    BlockInfoStack* GetCurrentFunctionBlockInfo();
    ParseNode *GetCurrentFunctionNode();
    ParseNode *GetCurrentNonLambdaFunctionNode();

    bool IsNodeAllowedInCurrentDeferralState(OpCode op) 
    {
        if (!this->m_deferringAST)
        {
            return true;
        }
        switch(op)
        {
            case knopBlock:
            case knopVarDecl:
            case knopConstDecl:
            case knopLetDecl:
            case knopFncDecl:
            case knopName:
                return true;

            default:
                return false;
        }
    }

    bool NextTokenConfirmsLetDecl() const { return m_token.tk == tkID || m_token.tk == tkLBrack || m_token.tk == tkLCurly || m_token.IsReservedWord(); }
    bool NextTokenIsPropertyNameStart() const { return m_token.tk == tkID || m_token.tk == tkStrCon || m_token.tk == tkIntCon || m_token.tk == tkFltCon || m_token.tk == tkLBrack || m_token.IsReservedWord(); }

    template<bool buildAST>
    void PushStmt(StmtNest *pStmt, ParseNodePtr pnode, OpCode op, ParseNodePtr pnodeLab, LabelId* pLabelIdList)
    {
        AssertMem(pStmt);

        if (buildAST)
        {
            AssertNodeMem(pnode);
            AssertNodeMemN(pnodeLab);

            pnode->sxStmt.grfnop = 0;
            pnode->sxStmt.pnodeOuter = (NULL == m_pstmtCur) ? NULL : m_pstmtCur->pnodeStmt;

            pStmt->pnodeStmt = pnode;
            pStmt->pnodeLab = pnodeLab;
        }
        else
        {
            // Assign to pnodeStmt rather than op so that we initialize the whole field.
            pStmt->pnodeStmt = 0;
            pStmt->isDeferred = true;
            pStmt->op = op;
            pStmt->pLabelId = pLabelIdList;
        }
        pStmt->pstmtOuter = m_pstmtCur;
        SetCurrentStatement(pStmt);
    }

    void PopStmt(StmtNest *pStmt);

    BlockInfoStack *PushBlockInfo(ParseNodePtr pnodeBlock);
    void PopBlockInfo();
    void PushDynamicBlock();
    void PopDynamicBlock();

    ParseNodePtr PnodeLabel(IdentPtr pid, ParseNodePtr pnodeLabels);

    void MarkEvalCaller()
    {
        if (this->GetCurrentFunctionNode())
        {
            ParseNodePtr pnodeFunc = GetCurrentFunctionNode();
            pnodeFunc->sxFnc.SetCallsEval(true);
        }
        ParseNode *pnodeBlock = GetCurrentBlock();
        if (pnodeBlock != NULL)
        {
            pnodeBlock->sxBlock.SetCallsEval(true);
            PushDynamicBlock();
        }
    }

    struct ParserState
    {
        ParseNodePtr *m_ppnodeScopeSave;
        ParseNodePtr *m_ppnodeExprScopeSave;
        charcount_t m_funcInArraySave;
        int32 *m_pCurrentAstSizeSave;
        uint m_funcInArrayDepthSave;
        uint m_nestedCountSave;
        int m_nextBlockId;
#if DEBUG
        // For very basic validation purpose - to check that we are not going restore to some other block.
        BlockInfoStack *m_currentBlockInfo;
#endif
    };

    // This function is going to capture some of the important current state of the parser to an object. Once we learn
    // that we need to reparse the grammar again we could use RestoreStateFrom to restore that state to the parser.
    void CaptureState(ParserState *state);
    void RestoreStateFrom(ParserState *state);
    // Future recommendation : Consider consolidating Parser::CaptureState and Scanner::Capture together if we do CaptureState more often.

public:
    IdentPtrList* GetRequestedModulesList();
    ModuleImportOrExportEntryList* GetModuleImportEntryList();
    ModuleImportOrExportEntryList* GetModuleLocalExportEntryList();
    ModuleImportOrExportEntryList* GetModuleIndirectExportEntryList();
    ModuleImportOrExportEntryList* GetModuleStarExportEntryList();

protected:
    IdentPtrList* EnsureRequestedModulesList();
    ModuleImportOrExportEntryList* EnsureModuleImportEntryList();
    ModuleImportOrExportEntryList* EnsureModuleLocalExportEntryList();
    ModuleImportOrExportEntryList* EnsureModuleIndirectExportEntryList();
    ModuleImportOrExportEntryList* EnsureModuleStarExportEntryList();

    void AddModuleSpecifier(IdentPtr moduleRequest);
    ModuleImportOrExportEntry* AddModuleImportOrExportEntry(ModuleImportOrExportEntryList* importOrExportEntryList, IdentPtr importName, IdentPtr localName, IdentPtr exportName, IdentPtr moduleRequest);
    ModuleImportOrExportEntry* AddModuleImportOrExportEntry(ModuleImportOrExportEntryList* importOrExportEntryList, ModuleImportOrExportEntry* importOrExportEntry);
    void AddModuleLocalExportEntry(ParseNodePtr varDeclNode);
    void CheckForDuplicateExportEntry(ModuleImportOrExportEntryList* exportEntryList, IdentPtr exportName);

    ParseNodePtr CreateModuleImportDeclNode(IdentPtr localName);
    void MarkIdentifierReferenceIsModuleExport(IdentPtr localName);

public:
    WellKnownPropertyPids* names(){ return &wellKnownPropertyPids; }

    IdentPtr CreatePid(__in_ecount(len) LPCOLESTR name, charcount_t len)
    {
        return m_phtbl->PidHashNameLen(name, len);
    }

    bool KnownIdent(__in_ecount(len) LPCOLESTR name, charcount_t len)
    {
        return m_phtbl->Contains(name, len);
    }

    template <typename THandler>
    static void ForEachItemRefInList(ParseNodePtr *list, THandler handler)
    {
        ParseNodePtr *current = list;
        while (current != nullptr && (*current) != nullptr)
        {
            if ((*current)->nop == knopList)
            {
                handler(&(*current)->sxBin.pnode1);

                // Advance to the next node
                current = &(*current)->sxBin.pnode2;
            }
            else
            {
                // The last node
                handler(current);
                current = nullptr;
            }
        }
    }

    template <typename THandler>
    static void ForEachItemInList(ParseNodePtr list, THandler handler)
    {
        ForEachItemRefInList(&list, [&](ParseNodePtr * item) {
            Assert(item != nullptr);
            handler(*item);
        });
    }

    template <class THandler>
    static void MapBindIdentifierFromElement(ParseNodePtr elementNode, THandler handler)
    {
        ParseNodePtr bindIdentNode = elementNode;
        if (bindIdentNode->nop == knopAsg)
        {
            bindIdentNode = bindIdentNode->sxBin.pnode1;
        }
        else if (bindIdentNode->nop == knopEllipsis)
        {
            bindIdentNode = bindIdentNode->sxUni.pnode1;
        }

        if (bindIdentNode->IsPattern())
        {
            MapBindIdentifier(bindIdentNode, handler);
        }
        else if (bindIdentNode->IsVarLetOrConst())
        {
            handler(bindIdentNode);
        }
        else
        {
            AssertMsg(bindIdentNode->nop == knopEmpty, "Invalid bind identifier");
        }
    }

    template <class THandler>
    static void MapBindIdentifier(ParseNodePtr patternNode, THandler handler)
    {
        if (patternNode->nop == knopAsg)
        {
            patternNode = patternNode->sxBin.pnode1;
        }

        Assert(patternNode->IsPattern());
        if (patternNode->nop == knopArrayPattern)
        {
            ForEachItemInList(patternNode->sxArrLit.pnode1, [&](ParseNodePtr item) {
                MapBindIdentifierFromElement(item, handler);
            });
        }
        else
        {
            ForEachItemInList(patternNode->sxUni.pnode1, [&](ParseNodePtr item) {
                Assert(item->nop == knopObjectPatternMember);
                MapBindIdentifierFromElement(item->sxBin.pnode2, handler);
            });
        }
    }

private:
    struct IdentToken
    {
        tokens tk;
        IdentPtr pid;
        charcount_t ichMin;
        charcount_t ichLim;

        IdentToken()
            : tk(tkNone), pid(NULL)
        {
        }
    };

    void CheckArguments(ParseNodePtr pnode);
    void CheckArgumentsUse(IdentPtr pid, ParseNodePtr pnodeFnc);

    void CheckStrictModeEvalArgumentsUsage(IdentPtr pid, ParseNodePtr pnode = NULL);

    // environments on which the strict mode is set, if found
    enum StrictModeEnvironment
    {
        SM_NotUsed,         // StrictMode environment is don't care
        SM_OnGlobalCode,    // The current environment is a global code
        SM_OnFunctionCode,  // The current environment is a function code
        SM_DeferredParse    // StrictMode used in deferred parse cases
    };

    template<bool buildAST> ParseNodePtr ParseArrayLiteral();

    template<bool buildAST> ParseNodePtr ParseStatement();
    template<bool buildAST> ParseNodePtr ParseVariableDeclaration(
        tokens declarationType,
        charcount_t ichMin,
        BOOL fAllowIn = TRUE,
        BOOL* pfForInOk = nullptr,
        BOOL singleDefOnly = FALSE,
        BOOL allowInit = TRUE,
        BOOL isTopVarParse = TRUE,
        BOOL isFor = FALSE,
        BOOL* nativeForOk = nullptr);
    BOOL TokIsForInOrForOf();

    template<bool buildAST>
    void ParseStmtList(
        ParseNodePtr *ppnodeList,
        ParseNodePtr **pppnodeLast = NULL,
        StrictModeEnvironment smEnvironment = SM_NotUsed,
        const bool isSourceElementList = false,
        bool* strictModeOn = NULL);
    bool FastScanFormalsAndBody();
    bool ScanAheadToFunctionEnd(uint count);

    bool DoParallelParse(ParseNodePtr pnodeFnc) const;

    // TODO: We should really call this StartScope and separate out the notion of scopes and blocks;
    // blocks refer to actual curly braced syntax, whereas scopes contain symbols.  All blocks have
    // a scope, but some statements like for loops or the with statement introduce a block-less scope.
    template<bool buildAST> ParseNodePtr StartParseBlock(PnodeBlockType blockType, ScopeType scopeType, ParseNodePtr pnodeLabel = NULL, LabelId* pLabelId = NULL);
    template<bool buildAST> ParseNodePtr StartParseBlockWithCapacity(PnodeBlockType blockType, ScopeType scopeType, int capacity);
    template<bool buildAST> ParseNodePtr StartParseBlockHelper(PnodeBlockType blockType, Scope *scope, ParseNodePtr pnodeLabel, LabelId* pLabelId);
    void PushFuncBlockScope(ParseNodePtr pnodeBlock, ParseNodePtr **ppnodeScopeSave, ParseNodePtr **ppnodeExprScopeSave);
    void PopFuncBlockScope(ParseNodePtr *ppnodeScopeSave, ParseNodePtr *ppnodeExprScopeSave);
    template<bool buildAST> ParseNodePtr ParseBlock(ParseNodePtr pnodeLabel, LabelId* pLabelId);
    void FinishParseBlock(ParseNode *pnodeBlock, bool needScanRCurly = true);
    void FinishParseFncExprScope(ParseNodePtr pnodeFnc, ParseNodePtr pnodeFncExprScope);

    template<const bool backgroundPidRefs>
    void BindPidRefs(BlockInfoStack *blockInfo, uint maxBlockId = (uint)-1);
    void BindPidRefsInScope(IdentPtr pid, Symbol *sym, int blockId, uint maxBlockId = (uint)-1);
    void MarkEscapingRef(ParseNodePtr pnode, IdentToken *pToken);
    void SetNestedFuncEscapes() const;
    void SetSymHasNonLocalReference(Symbol *sym);
    void PushScope(Scope *scope);
    void PopScope(Scope *scope);

    template<bool buildAST> ParseNodePtr ParseArgList(bool *pCallOfConstants, uint16 *pSpreadArgCount, uint16 * pCount);
    template<bool buildAST> ParseNodePtr ParseArrayList(bool *pArrayOfTaggedInts, bool *pArrayOfInts, bool *pArrayOfNumbers, bool *pHasMissingValues, uint *count, uint *spreadCount);
    template<bool buildAST> ParseNodePtr ParseMemberList(LPCOLESTR pNameHint, uint32 *pHintLength, tokens declarationType = tkNone);
    template<bool buildAST> ParseNodePtr ParseSuper(ParseNodePtr pnode, bool fAllowCall);

    // Used to determine the type of JavaScript object member.
    // The values can be combined using bitwise OR.
    //       specifically, it is valid to have getter and setter at the same time.
    enum MemberType
    {
        MemberTypeDataProperty = 1 << 0, // { foo: 1 },
        MemberTypeGetter       = 1 << 1, // { get foo() }
        MemberTypeSetter       = 1 << 2, // { set foo(arg) {} }
        MemberTypeMethod       = 1 << 3, // { foo() {} }
        MemberTypeIdentifier   = 1 << 4  // { foo } (shorthand for { foo: foo })
    };

    // Used to map JavaScript object member name to member type.
    typedef JsUtil::BaseDictionary<WCHAR*, MemberType, ArenaAllocator, PrimeSizePolicy> MemberNameToTypeMap;

    static MemberNameToTypeMap* CreateMemberNameMap(ArenaAllocator* pAllocator);

    template<bool buildAST> void ParseComputedName(ParseNodePtr* ppnodeName, LPCOLESTR* ppNameHint, LPCOLESTR* ppFullNameHint = nullptr, uint32 *pNameLength = nullptr, uint32 *pShortNameOffset = nullptr);
    template<bool buildAST> ParseNodePtr ParseMemberGetSet(OpCode nop, LPCOLESTR* ppNameHint);
    template<bool buildAST> ParseNodePtr ParseFncDecl(ushort flags, LPCOLESTR pNameHint = NULL, const bool needsPIDOnRCurlyScan = false, bool resetParsingSuperRestrictionState = true, bool fUnaryOrParen = false);
    template<bool buildAST> bool ParseFncNames(ParseNodePtr pnodeFnc, ParseNodePtr pnodeFncParent, ushort flags, ParseNodePtr **pLastNodeRef);
    template<bool buildAST> void ParseFncFormals(ParseNodePtr pnodeFnc, ParseNodePtr pnodeParentFnc, ushort flags, bool isTopLevelDeferredFunc = false);
    template<bool buildAST> bool ParseFncDeclHelper(ParseNodePtr pnodeFnc, LPCOLESTR pNameHint, ushort flags, bool *pHasName, bool fUnaryOrParen, bool noStmtContext, bool *pNeedScanRCurly, bool skipFormals = false);
    template<bool buildAST> void ParseExpressionLambdaBody(ParseNodePtr pnodeFnc);
    template<bool buildAST> void UpdateCurrentNodeFunc(ParseNodePtr pnodeFnc, bool fLambda);
    bool FncDeclAllowedWithoutContext(ushort flags);
    void FinishFncDecl(ParseNodePtr pnodeFnc, LPCOLESTR pNameHint, ParseNodePtr *lastNodeRef, bool skipCurlyBraces = false);
    void ParseTopLevelDeferredFunc(ParseNodePtr pnodeFnc, ParseNodePtr pnodeFncParent, LPCOLESTR pNameHint);
    void ParseNestedDeferredFunc(ParseNodePtr pnodeFnc, bool fLambda, bool *pNeedScanRCurly, bool *pStrictModeTurnedOn);
    void CheckStrictFormalParameters();
    ParseNodePtr AddArgumentsNodeToVars(ParseNodePtr pnodeFnc);
    void UpdateArgumentsNode(ParseNodePtr pnodeFnc, ParseNodePtr argNode);
    void UpdateOrCheckForDuplicateInFormals(IdentPtr pid, SList<IdentPtr> *formals);

    LPCOLESTR GetFunctionName(ParseNodePtr pnodeFnc, LPCOLESTR pNameHint);
    uint CalculateFunctionColumnNumber();

    template<bool buildAST> ParseNodePtr GenerateEmptyConstructor(bool extends = false);

    template<bool buildAST> ParseNodePtr GenerateModuleFunctionWrapper();

    IdentPtr ParseClassPropertyName(IdentPtr * hint);
    template<bool buildAST> ParseNodePtr ParseClassDecl(BOOL isDeclaration, LPCOLESTR pNameHint, uint32 *pHintLength, uint32 *pShortNameOffset);

    template<bool buildAST> ParseNodePtr ParseStringTemplateDecl(ParseNodePtr pnodeTagFnc);

    // This is used in the es6 class pattern.
    LPCOLESTR ConstructFinalHintNode(IdentPtr pClassName, IdentPtr pMemberName, IdentPtr pGetSet, bool isStatic, uint32* nameLength, uint32* pShortNameOffset, bool isComputedName = false, LPCOLESTR pMemberNameHint = nullptr);

    // Construct the name from the parse node.
    LPCOLESTR FormatPropertyString(LPCOLESTR propertyString, ParseNodePtr pNode, uint32 *fullNameHintLength, uint32 *pShortNameOffset);
    LPCOLESTR ConstructNameHint(ParseNodePtr pNode, uint32* fullNameHintLength, uint32 *pShortNameOffset);
    LPCOLESTR AppendNameHints(IdentPtr  left, IdentPtr  right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(IdentPtr  left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR left, IdentPtr  right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR leftStr, uint32 leftLen, LPCOLESTR rightStr, uint32 rightLen, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    WCHAR * AllocateStringOfLength(ULONG length);

    void FinishFncNode(ParseNodePtr pnodeFnc);

    template<bool buildAST> bool ParseOptionalExpr(
        ParseNodePtr* pnode,
        bool fUnaryOrParen = false,
        int oplMin = koplNo,
        BOOL *pfCanAssign = NULL,
        BOOL fAllowIn = TRUE,
        BOOL fAllowEllipsis = FALSE,
        _Inout_opt_ IdentToken* pToken = NULL);

    template<bool buildAST> ParseNodePtr ParseExpr(
        int oplMin = koplNo,
        BOOL *pfCanAssign = NULL,
        BOOL fAllowIn = TRUE,
        BOOL fAllowEllipsis = FALSE,
        LPCOLESTR pHint = NULL,
        uint32 *pHintLength = nullptr,
        uint32 *pShortNameOffset = nullptr,
        _Inout_opt_ IdentToken* pToken = NULL,
        bool fUnaryOrParen = false,
        _Inout_opt_ bool* pfLikelyPattern = nullptr,
        _Inout_opt_ charcount_t *plastRParen = nullptr);

    template<bool buildAST> ParseNodePtr ParseTerm(
        BOOL fAllowCall = TRUE,
        LPCOLESTR pNameHint = nullptr,
        uint32 *pHintLength = nullptr,
        uint32 *pShortNameOffset = nullptr,
        _Inout_opt_ IdentToken* pToken = nullptr,
        bool fUnaryOrParen = false,
        _Out_opt_ BOOL* pfCanAssign = nullptr,
        _Inout_opt_ BOOL* pfLikelyPattern = nullptr,
        _Out_opt_ bool* pfIsDotOrIndex = nullptr,
        _Inout_opt_ charcount_t *plastRParen = nullptr);

    template<bool buildAST> ParseNodePtr ParsePostfixOperators(
        ParseNodePtr pnode,
        BOOL fAllowCall, 
        BOOL fInNew, 
        BOOL isAsyncExpr,
        BOOL *pfCanAssign, 
        _Inout_ IdentToken* pToken, 
        _Out_opt_ bool* pfIsDotOrIndex = nullptr);

    void ThrowNewTargetSyntaxErrForGlobalScope();

    template<bool buildAST> ParseNodePtr ParseMetaProperty(
        tokens metaParentKeyword,
        charcount_t ichMin,
        _Out_opt_ BOOL* pfCanAssign = nullptr);

    bool IsImportOrExportStatementValidHere();
    bool IsTopLevelModuleFunc();

    template<bool buildAST> ParseNodePtr ParseImport();
    template<bool buildAST> void ParseImportClause(ModuleImportOrExportEntryList* importEntryList, bool parsingAfterComma = false);
    template<bool buildAST> ParseNodePtr ParseImportCall();

    template<bool buildAST> ParseNodePtr ParseExportDeclaration(bool *needTerminator = nullptr);
    template<bool buildAST> ParseNodePtr ParseDefaultExportClause();

    template<bool buildAST> void ParseNamedImportOrExportClause(ModuleImportOrExportEntryList* importOrExportEntryList, bool isExportClause);
    template<bool buildAST> IdentPtr ParseImportOrExportFromClause(bool throwIfNotFound);

    BOOL NodeIsIdent(ParseNodePtr pnode, IdentPtr pid);
    BOOL NodeIsEvalName(ParseNodePtr pnode);
    BOOL IsJSONValid(ParseNodePtr pnodeExpr)
    {
        OpCode jnop = (knopNeg == pnodeExpr->nop) ? pnodeExpr->sxUni.pnode1->nop : pnodeExpr->nop;
        if (knopNeg == pnodeExpr->nop)
        {
            return (knopInt == jnop ||  knopFlt == jnop);
        }
        else
        {
            return (knopInt == jnop ||  knopFlt == jnop ||
                knopStr == jnop ||  knopNull == jnop ||
                knopTrue == jnop || knopFalse == jnop ||
                knopObject == jnop || knopArray == jnop);
        }
    }

    BOOL IsConstantInFunctionCall(ParseNodePtr pnode);
    BOOL IsConstantInArrayLiteral(ParseNodePtr pnode);

    ParseNodePtr CreateParamPatternNode(ParseNodePtr pnode1);

    ParseNodePtr ConvertMemberToMemberPattern(ParseNodePtr pnodeMember);
    ParseNodePtr ConvertObjectToObjectPattern(ParseNodePtr pnodeMemberList);
    ParseNodePtr GetRightSideNodeFromPattern(ParseNodePtr pnode);
    ParseNodePtr ConvertArrayToArrayPattern(ParseNodePtr pnode);
    ParseNodePtr ConvertToPattern(ParseNodePtr pnode);

    void AppendToList(ParseNodePtr * node, ParseNodePtr nodeToAppend);

    bool IsES6DestructuringEnabled() const;
    bool IsPossiblePatternStart() const { return m_token.tk == tkLCurly || m_token.tk == tkLBrack; }
    bool IsPostFixOperators() const
    {
        return m_token.tk == tkLParen ||
            m_token.tk == tkLBrack ||
            m_token.tk == tkDot ||
            m_token.tk == tkStrTmplBasic ||
            m_token.tk == tkStrTmplBegin;
    }
    template<bool buildAST> ParseNodePtr ParseTryCatchFinally();
    template<bool buildAST> ParseNodePtr ParseTry();
    template<bool buildAST> ParseNodePtr ParseCatch();
    template<bool buildAST> ParseNodePtr ParseFinally();

    template<bool buildAST> ParseNodePtr ParseCase(ParseNodePtr *ppnodeBody);
    template<bool buildAST> ParseNodePtr ParseRegExp();

    template <bool buildAST>
    ParseNodePtr ParseDestructuredArrayLiteral(tokens declarationType, bool isDecl, bool topLevel = true);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredObjectLiteral(tokens declarationType, bool isDecl, bool topLevel = true);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredLiteral(tokens declarationType,
        bool isDecl,
        bool topLevel = true,
        DestructuringInitializerContext initializerContext = DIC_None,
        bool allowIn = true,
        BOOL *forInOfOkay = nullptr,
        BOOL *nativeForOkay = nullptr);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredVarDecl(tokens declarationType, bool isDecl, bool *hasSeenRest, bool topLevel = true, bool allowEmptyExpression = true);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredInitializer(ParseNodePtr lhsNode,
        bool isDecl,
        bool topLevel,
        DestructuringInitializerContext initializerContext,
        bool allowIn,
        BOOL *forInOfOkay,
        BOOL *nativeForOkay);

    template<bool CheckForNegativeInfinity> static bool IsNaNOrInfinityLiteral(LPCOLESTR str);

    void ParseDestructuredLiteralWithScopeSave(tokens declarationType,
        bool isDecl,
        bool topLevel,
        DestructuringInitializerContext initializerContext = DIC_None,
        bool allowIn = true);

public:
    void ValidateSourceElementList();
    void ValidateFormals();

    bool IsStrictMode() const;
    BOOL ExpectingExternalSource();

    IdentPtr GetArgumentsPid() const { return wellKnownPropertyPids.arguments; }
    IdentPtr GetEvalPid() const { return wellKnownPropertyPids.eval; }
    IdentPtr GetTargetPid() const { return wellKnownPropertyPids.target; }
    BackgroundParseItem *GetCurrBackgroundParseItem() const { return currBackgroundParseItem; }
    void SetCurrBackgroundParseItem(BackgroundParseItem *item) { currBackgroundParseItem = item; }

    void Release()
    {
        RELEASEPTR(m_pscan);
        RELEASEPTR(m_phtbl);
    }

private:
    void DeferOrEmitPotentialSpreadError(ParseNodePtr pnodeT);
    template<bool buildAST> void TrackAssignment(ParseNodePtr pnodeT, IdentToken* pToken);
    PidRefStack* PushPidRef(IdentPtr pid);
    PidRefStack* FindOrAddPidRef(IdentPtr pid, int blockId, Js::LocalFunctionId funcId);
    void RemovePrevPidRef(IdentPtr pid, PidRefStack *lastRef);
    void SetPidRefsInScopeDynamic(IdentPtr pid, int blockId);

    void RestoreScopeInfo(Js::ScopeInfo * scopeInfo);
    void FinishScopeInfo(Js::ScopeInfo * scopeInfo);

    BOOL PnodeLabelNoAST(IdentToken* pToken, LabelId* pLabelIdList);
    LabelId* CreateLabelId(IdentToken* pToken);

    void AddToNodeList(ParseNode ** ppnodeList, ParseNode *** pppnodeLast, ParseNode * pnodeAdd);
    void AddToNodeListEscapedUse(ParseNode ** ppnodeList, ParseNode *** pppnodeLast, ParseNode * pnodeAdd);

    void ChkCurTokNoScan(int tk, int wErr)
    {
        if (m_token.tk != tk)
        {
            Error(wErr);
        }
    }

    void ChkCurTok(int tk, int wErr)
    {
        if (m_token.tk != tk)
        {
            Error(wErr);
        }
        else
        {
            m_pscan->Scan();
        }
    }
    void ChkNxtTok(int tk, int wErr)
    {
        m_pscan->Scan();
        ChkCurTok(tk, wErr);
    }

    template <class Fn>
    void FinishFunctionsInScope(ParseNodePtr pnodeScopeList, Fn fn);
    void FinishDeferredFunction(ParseNodePtr pnodeScopeList);

    /***********************************************************************
    Misc
    ***********************************************************************/
    bool        m_UsesArgumentsAtGlobal; // "arguments" used at global code.

    BOOL m_fUseStrictMode; // ES5 Use Strict mode. In AST mode this is a global flag; in NoAST mode it is pushed and popped.
    bool m_InAsmMode; // Currently parsing Asm.Js module
    bool m_deferAsmJs;
    BOOL m_fExpectExternalSource;
    BOOL m_deferringAST;
    BOOL m_stoppedDeferredParse;

    enum FncDeclFlag : ushort
    {
        fFncNoFlgs      = 0,
        fFncDeclaration = 1 << 0,
        fFncNoArg       = 1 << 1,
        fFncOneArg      = 1 << 2, //Force exactly one argument.
        fFncNoName      = 1 << 3,
        fFncLambda      = 1 << 4,
        fFncMethod      = 1 << 5,
        fFncClassMember = 1 << 6,
        fFncGenerator   = 1 << 7,
        fFncAsync       = 1 << 8,
        fFncModule      = 1 << 9,
    };

    //
    // If we need the scanner to force PID creation temporarily, use this auto object
    // to turn scanner deferred parsing off temporarily and restore at destructor.
    //
    class AutoTempForcePid
    {
    private:
        Scanner_t* m_scanner;
        bool m_forcePid;
        BYTE m_oldScannerDeferredParseFlags;

    public:
        AutoTempForcePid(Scanner_t* scanner, bool forcePid)
            : m_scanner(scanner), m_forcePid(forcePid)
        {
            if (forcePid)
            {
                m_oldScannerDeferredParseFlags = scanner->SetDeferredParse(FALSE);
            }
        }

        ~AutoTempForcePid()
        {
            if (m_forcePid)
            {
                m_scanner->SetDeferredParseFlags(m_oldScannerDeferredParseFlags);
            }
        }
    };

public:
    charcount_t GetSourceIchLim() { return m_sourceLim; }
    static BOOL NodeEqualsName(ParseNodePtr pnode, LPCOLESTR sz, uint32 cch);

};

#define PTNODE(nop,sn,pc,nk,ok,json) \
    template<> inline int Parser::GetNodeSize<nop>() { return kcbPn##nk; }
#include "ptlist.h"
