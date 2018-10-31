//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "ParseFlags.h"

namespace Js
{
    class ScopeInfo;
    class ByteCodeCache;
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

struct BlockInfoStack;

struct StmtNest
{
    union
    {
        struct
        {
            ParseNodeStmt * pnodeStmt; // This statement node.
        };
        struct
        {
            bool isDeferred : 1;
            OpCode op;              // This statement operation.
        };
    };
    LabelId* pLabelId;              // Labels for this statement.
    StmtNest *pstmtOuter;           // Enclosing statement.

    inline OpCode GetNop() const
    {
        AnalysisAssert(isDeferred || pnodeStmt != nullptr);
        return isDeferred ? op : pnodeStmt->nop;
    }
};

struct ParseContext
{
    LPCUTF8 pszSrc;
    size_t offset;
    size_t length;
    charcount_t characterOffset;
    int nextBlockId;
    ULONG grfscr;
    ULONG lineNumber;
    ParseNodeProg * pnodeProg;
    SourceContextInfo* sourceContextInfo;
    BlockInfoStack* currentBlockInfo;
    bool strictMode;
    bool isUtf8;
};

// DeferredFunctionStub is part of the parser state cache we serialize and restore in an
// attempt to avoid doing another upfront parse of the same source.
// Each deferred stub contains information needed to identify the function location in source,
// flags for the function, the set of names captured by this function, and links to deferred
// stubs for further nested functions.
// These stubs are only created for defer-parsed functions and we create one stub for each
// nested function. When we fully parse the defer-parsed function, we will use information
// in these stubs to skip scanning the nested functions again.
//
//  Example code:
//    let a, b;
//    function foo() {
//        function bar() {
//            return a;
//        }
//        function baz() {
//            return b;
//        }
//    }
//
//  Deferred stubs for foo:
//    capturedNames: { a, b }
//    nestedCount: 2
//    deferredStubs :
//    [
//      // 0 = bar:
//      {
//        capturedNames: { a }
//        nestedCount: 0
//        deferredStubs : nullptr
//        ...
//      },
//      // 1 = baz:
//      {
//        capturedNames: { b }
//        nestedCount: 0
//        deferredStubs : nullptr
//        ...
//      }
//    ]
//    ...
struct DeferredFunctionStub
{
    Field(RestorePoint) restorePoint;
    Field(FncFlags) fncFlags;
    Field(uint) nestedCount;
    Field(charcount_t) ichMin;

    // Number of names captured by this function.
    // This is used as length for capturedNameSerializedIds but should
    // also be equal to the length of capturedNamePointers when
    // capturedNamePointers is not nullptr.
    Field(uint) capturedNameCount;

    // After the parser memory is cleaned-up, we no longer have access to
    // the IdentPtrs allocated from the Parser arena. We keep a list of
    // ids into the string table deserialized from the parser state cache.
    // This list is Recycler-allocated.
    Field(int *) capturedNameSerializedIds;

    // The set of names which are captured by this function.
    // A function captures a name when it references a name not defined within
    // the function.
    // A function also captures all names captured by nested functions.
    // The IdentPtrs in this set and the set itself are allocated from Parser
    // arena memory.
    Field(IdentPtrSet *) capturedNamePointers;

    // List of deferred stubs for further nested functions.
    // Length of this list is equal to nestedCount.
    Field(DeferredFunctionStub *) deferredStubs;

    Field(Js::ByteCodeCache *) byteCodeCache;
};

template <bool nullTerminated> class UTF8EncodingPolicyBase;
typedef UTF8EncodingPolicyBase<false> NotNullTerminatedUTF8EncodingPolicy;
template <typename T> class Scanner;

namespace Js
{
    class ParseableFunctionInfo;
    class FunctionBody;
    template <bool isGuestArena>
    class TempArenaAllocatorWrapper;
};

class Parser
{
    typedef Scanner<NotNullTerminatedUTF8EncodingPolicy> Scanner_t;

public:
#if DEBUG
    Parser(Js::ScriptContext* scriptContext, BOOL strictMode = FALSE, PageAllocator *alloc = nullptr, bool isBackground = false, size_t size = sizeof(Parser));
#else
    Parser(Js::ScriptContext* scriptContext, BOOL strictMode = FALSE, PageAllocator *alloc = nullptr, bool isBackground = false);
#endif
    ~Parser(void);

    Js::ScriptContext* GetScriptContext() const { return m_scriptContext; }
    void ReleaseTemporaryGuestArena();
    bool IsCreatingStateCache();

#if ENABLE_BACKGROUND_PARSING
    bool IsBackgroundParser() const { return m_isInBackground; }
    bool IsDoingFastScan() const { return m_doingFastScan; }
#else
    bool IsBackgroundParser() const { return false; }
    bool IsDoingFastScan() const { return false; }
#endif

    bool GetIsInParsingArgList() const { return m_isInParsingArgList; }
    void SetIsInParsingArgList(bool set) { m_isInParsingArgList = set; }

    bool GetHasDestructuringPattern() const { return m_hasDestructuringPattern; }
    void SetHasDestructuringPattern(bool set) { m_hasDestructuringPattern = set; }    

    ParseNode* CopyPnode(ParseNode* pnode);

    ArenaAllocator *GetAllocator() { return &m_nodeAllocator;}

    size_t GetSourceLength() { return m_length; }
    size_t GetOriginalSourceLength() { return m_originalLength; }
    static ULONG GetDeferralThreshold(bool isProfileLoaded);
    BOOL WillDeferParse(Js::LocalFunctionId functionId);
    BOOL IsDeferredFnc();
    void ReduceDeferredScriptLength(size_t chars);
    static DeferredFunctionStub * BuildDeferredStubTree(ParseNodeFnc *pnodeFnc, Recycler *recycler);

    void RestorePidRefForSym(Symbol *sym);

    HRESULT ValidateSyntax(LPCUTF8 pszSrc, size_t encodedCharCount, bool isGenerator, bool isAsync, CompileScriptException *pse, void (Parser::*validateFunction)());

    // Should be called when the UTF-8 source was produced from UTF-16. This is really CESU-8 source in that it encodes surrogate pairs
    // as 2 three byte sequences instead of 4 bytes as required by UTF-8. It also is a lossless conversion of invalid UTF-16 sequences.
    // This is important in Javascript because Javascript engines are required not to report invalid UTF-16 sequences and to consider
    // the UTF-16 characters pre-canonicalization. Converting this UTF-16 with invalid sequences to valid UTF-8 and back would cause
    // all invalid UTF-16 sequences to be replaced by one or more Unicode replacement characters (0xFFFD), losing the original
    // invalid sequences.
    HRESULT ParseCesu8Source(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
        Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo);

    // Should be called when the source is UTF-8 and invalid UTF-8 sequences should be replaced with the unicode replacement character
    // (0xFFFD). Security concerns require externally produced UTF-8 only allow valid UTF-8 otherwise an attacker could use invalid
    // UTF-8 sequences to fool a filter and cause Javascript to be executed that might otherwise have been rejected.
    HRESULT ParseUtf8Source(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t length, ULONG grfsrc, CompileScriptException *pse,
        Js::LocalFunctionId * nextFunctionId, SourceContextInfo * sourceContextInfo);

    // Used by deferred parsing to parse a deferred function.
    HRESULT ParseSourceWithOffset(__out ParseNodeProg ** parseTree, LPCUTF8 pSrc, size_t offset, size_t cbLength, charcount_t cchOffset,
        bool isCesu8, ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber,
        SourceContextInfo * sourceContextInfo, Js::ParseableFunctionInfo* functionInfo);

protected:
    static uint BuildDeferredStubTreeHelper(ParseNodeBlock* pnodeBlock, DeferredFunctionStub* deferredStubs, uint currentStubIndex, uint deferredStubCount, Recycler *recycler);
    void ShiftCurrDeferredStubToChildFunction(ParseNodeFnc* pnodeFnc, ParseNodeFnc* pnodeFncParent);

    HRESULT ParseSourceInternal(
        __out ParseNodeProg ** parseTree, LPCUTF8 pszSrc, size_t offsetInBytes,
        size_t lengthInCodePoints, charcount_t offsetInChars, bool isUtf8,
        ULONG grfscr, CompileScriptException *pse, Js::LocalFunctionId * nextFunctionId, ULONG lineNumber, SourceContextInfo * sourceContextInfo);

    ParseNodeProg * Parse(LPCUTF8 pszSrc, size_t offset, size_t length, charcount_t charOffset, bool isUtf8, ULONG grfscr, ULONG lineNumber,
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
    bool                m_tempGuestArenaReleased;
    int                 m_nextBlockId;

    AutoRecyclerRootPtr<Js::TempArenaAllocatorWrapper<true>> m_tempGuestArena;
    // RegexPattern objects created for literal regexes are recycler-allocated and need to be kept alive until the function body
    // is created during byte code generation. The RegexPattern pointer is stored in a temporary guest
    // arena for that purpose. This list is then unregistered from the guest arena at the end of parsing/scanning.
    SList<UnifiedRegex::RegexPattern *, ArenaAllocator> m_registeredRegexPatterns;

protected:
    Js::ScriptContext* m_scriptContext;
    HashTbl * GetHashTbl() { return this->GetScanner()->GetHashTbl(); }

    LPCWSTR GetTokenString(tokens token);
    __declspec(noreturn) void Error(HRESULT hr, LPCWSTR stringOne = _u(""), LPCWSTR stringTwo = _u(""));
private:
    __declspec(noreturn) void Error(HRESULT hr, ParseNodePtr pnode);
    __declspec(noreturn) void Error(HRESULT hr, charcount_t ichMin, charcount_t ichLim);
    __declspec(noreturn) static void OutOfMemory();

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
    Scanner_t   m_scan;

    Scanner_t const * GetScanner() const { return &m_scan; }
    Scanner_t * GetScanner() { return &m_scan; }

    void AddAstSize(int size);
    void AddAstSizeAllowDefer(int size);

    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateNodeForOpT() { return CreateNodeForOpT<nop>(this->GetScanner()->IchMinTok()); }
    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateNodeForOpT(charcount_t ichMin) { return CreateNodeForOpT<nop>(ichMin, this->GetScanner()->IchLimTok()); }
    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateNodeForOpT(charcount_t ichMin, charcount_t ichLim);
    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateAllowDeferNodeForOpT() { return CreateAllowDeferNodeForOpT<nop>(this->GetScanner()->IchMinTok()); }
    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateAllowDeferNodeForOpT(charcount_t ichMin) { return CreateAllowDeferNodeForOpT<nop>(ichMin, this->GetScanner()->IchLimTok()); }
    template <OpCode nop> typename OpCodeTrait<nop>::ParseNodeType * CreateAllowDeferNodeForOpT(charcount_t ichMin, charcount_t ichLim);
public:

    // create nodes using arena allocator; used by AST transformation
    template <OpCode nop>
    static typename OpCodeTrait<nop>::ParseNodeType * StaticCreateNodeT(ArenaAllocator* alloc, charcount_t ichMin = 0, charcount_t ichLim = 0)
    {
        return Anew(alloc, typename OpCodeTrait<nop>::ParseNodeType, nop, ichMin, ichLim);        
    }
    
    static ParseNodeBin * StaticCreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, ArenaAllocator* alloc, charcount_t ichMin = 0, charcount_t ichLim = 0);
    static ParseNodeBlock * StaticCreateBlockNode(ArenaAllocator* alloc, charcount_t ichMin = 0, charcount_t ichLim = 0, int blockId = -1, PnodeBlockType blockType = PnodeBlockType::Regular);
    static ParseNodeVar * StaticCreateTempNode(ParseNode* initExpr, ArenaAllocator* alloc);
    static ParseNodeUni * StaticCreateTempRef(ParseNode* tempNode, ArenaAllocator* alloc);

private:    
    ParseNodeUni * CreateUniNode(OpCode nop, ParseNodePtr pnodeOp);
    ParseNodeUni * CreateUniNode(OpCode nop, ParseNodePtr pnode1, charcount_t ichMin, charcount_t ichLim);
    ParseNodeBin * CreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2);
    ParseNodeBin * CreateBinNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, charcount_t ichMin, charcount_t ichLim);
    ParseNodeTri * CreateTriNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, ParseNodePtr pnode3);
    ParseNodeTri * CreateTriNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, ParseNodePtr pnode3, charcount_t ichMin, charcount_t ichLim);
    ParseNodeBlock * CreateBlockNode(PnodeBlockType blockType = PnodeBlockType::Regular);
    ParseNodeBlock * CreateBlockNode(charcount_t ichMin, charcount_t ichLim, PnodeBlockType blockType = PnodeBlockType::Regular);
    ParseNodeVar * CreateDeclNode(OpCode nop, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl = true);

    ParseNodeInt * CreateIntNode(int32 lw);
    ParseNodeStr * CreateStrNode(IdentPtr pid);
    ParseNodeBigInt * CreateBigIntNode(IdentPtr pid);
    ParseNodeName * CreateNameNode(IdentPtr pid);
    ParseNodeName * CreateNameNode(IdentPtr pid, PidRefStack * ref, charcount_t ichMin, charcount_t ichLim);
    ParseNodeSpecialName * CreateSpecialNameNode(IdentPtr pid, PidRefStack * ref, charcount_t ichMin, charcount_t ichLim);
    ParseNodeSuperReference * CreateSuperReferenceNode(OpCode nop, ParseNodeSpecialName * pnode1, ParseNodePtr pnode2);
    ParseNodeProg * CreateProgNode(bool isModuleSource, ULONG lineNumber);

    ParseNodeCall * CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2);
    ParseNodeCall * CreateCallNode(OpCode nop, ParseNodePtr pnode1, ParseNodePtr pnode2, charcount_t ichMin, charcount_t ichLim);
    ParseNodeSuperCall * CreateSuperCallNode(ParseNodeSpecialName * pnode1, ParseNodePtr pnode2);
       
    ParseNodeFnc * CreateDummyFuncNode(bool fDeclaration);

    ParseNodeParamPattern * CreateParamPatternNode(ParseNodePtr pnode1);
    ParseNodeParamPattern * CreateDummyParamPatternNode(charcount_t ichMin);

    ParseNodeObjLit * CreateObjectPatternNode(ParseNodePtr pnodeMemberList, charcount_t ichMin, charcount_t ichLim, bool convertToPattern=false);

    Symbol*      AddDeclForPid(ParseNodeVar * pnode, IdentPtr pid, SymbolType symbolType, bool errorOnRedecl);
    void         CheckRedeclarationErrorForBlockId(IdentPtr pid, int blockId);

public:
#if ENABLE_BACKGROUND_PARSING
    void PrepareForBackgroundParse();
    void AddFastScannedRegExpNode(ParseNodePtr const pnode);
    void AddBackgroundRegExpNode(ParseNodePtr const pnode);
    void AddBackgroundParseItem(BackgroundParseItem *const item);
    void FinishBackgroundRegExpNodes();
    void FinishBackgroundPidRefs(BackgroundParseItem *const item, bool isOtherParser);
    void WaitForBackgroundJobs(BackgroundParser *bgp, CompileScriptException *pse);
    HRESULT ParseFunctionInBackground(ParseNodeFnc * pnodeFnc, ParseContext *parseContext, bool topLevelDeferred, CompileScriptException *pse);
#endif

    void CheckPidIsValid(IdentPtr pid, bool autoArgumentsObject = false);
    void AddVarDeclToBlock(ParseNodeVar *pnode);
    // Add a var declaration. Only use while parsing. Assumes m_ppnodeVar is pointing to the right place already
    ParseNodeVar * CreateVarDeclNode(IdentPtr pid, SymbolType symbolType, bool autoArgumentsObject = false, ParseNodePtr pnodeFnc = NULL, bool checkReDecl = true);
    // Add a var declaration, during parse tree rewriting. Will setup m_ppnodeVar for the given pnodeFnc
    ParseNodeVar * AddVarDeclNode(IdentPtr pid, ParseNodeFnc * pnodeFnc);
    // Add a 'const' or 'let' declaration.
    ParseNodeVar * CreateBlockScopedDeclNode(IdentPtr pid, OpCode nodeType);

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
    ParseNodeFnc * m_currentNodeNonLambdaFunc; // current function or NULL
    ParseNodeFnc * m_currentNodeNonLambdaDeferredFunc; // current function or NULL
    ParseNodeFnc * m_currentNodeFunc; // current function or NULL
    ParseNodeFnc * m_currentNodeDeferredFunc; // current function or NULL
    ParseNodeProg * m_currentNodeProg; // current program
    DeferredFunctionStub *m_currDeferredStub;
    uint m_currDeferredStubCount;
    int32 * m_pCurrentAstSize;
    ParseNodePtr * m_ppnodeScope;  // function list tail
    ParseNodePtr * m_ppnodeExprScope; // function expression list tail
    ParseNodePtr * m_ppnodeVar;  // variable list tail
    bool m_inDeferredNestedFunc; // true if parsing a function in deferred mode, nested within the current node  
    bool m_reparsingLambdaParams;
    bool m_disallowImportExportStmt;
    bool m_isInParsingArgList;
    bool m_hasDestructuringPattern;
    // This bool is used for deferring the shorthand initializer error ( {x = 1}) - as it is allowed in the destructuring grammar.
    bool m_hasDeferredShorthandInitError;
    bool m_deferEllipsisError;
    bool m_deferCommaError;
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
        IdentPtr _this; // Special 'this' identifier
        IdentPtr _newTarget; // Special new.target identifier
        IdentPtr _super; // Special super identifier
        IdentPtr _superConstructor; // Special super constructor identifier
    };

    WellKnownPropertyPids wellKnownPropertyPids;

    charcount_t m_sourceLim; // The actual number of characters parsed.

    Js::ParseableFunctionInfo* m_functionBody; // For a deferred parsed function, the function body is non-null
    ParseType m_parseType;

    uint m_arrayDepth;
    uint m_funcInArrayDepth; // Count func depth within array literal
    charcount_t m_funcInArray;
    uint m_scopeCountNoAst;

    // Used for issuing spread and rest errors when there is ambiguity with lambda parameter lists and parenthesized expressions
    uint m_funcParenExprDepth;
    RestorePoint m_deferEllipsisErrorLoc;
    RestorePoint m_deferCommaErrorLoc;

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

    void AppendFunctionToScopeList(bool fDeclaration, ParseNodeFnc * pnodeFnc);

    // block scoped content helpers
    void SetCurrentStatement(StmtNest *stmt);
    ParseNodeBlock* GetCurrentBlock();
    ParseNodeBlock* GetFunctionBlock();
    BlockInfoStack* GetCurrentBlockInfo();
    BlockInfoStack* GetCurrentFunctionBlockInfo();
    ParseNodeFnc *GetCurrentFunctionNode();
    ParseNodeFnc *GetCurrentNonLambdaFunctionNode();

    bool NextTokenConfirmsLetDecl() const { return m_token.tk == tkID || m_token.tk == tkLBrack || m_token.tk == tkLCurly || m_token.IsReservedWord(); }
    bool NextTokenIsPropertyNameStart() const { return m_token.tk == tkID || m_token.tk == tkStrCon || m_token.tk == tkIntCon || m_token.tk == tkFltCon || m_token.tk == tkLBrack || m_token.IsReservedWord(); }

    template<bool buildAST>
    void PushStmt(StmtNest *pStmt, ParseNodeStmt * pnode, OpCode op, LabelId* pLabelIdList)
    {
        if (buildAST)
        {
            pnode->grfnop = 0;
            pnode->pnodeOuter = (NULL == m_pstmtCur) ? NULL : m_pstmtCur->pnodeStmt;

            pStmt->pnodeStmt = pnode;
        }
        else
        {
            // Assign to pnodeStmt rather than op so that we initialize the whole field.
            pStmt->pnodeStmt = 0;
            pStmt->isDeferred = true;
            pStmt->op = op;
        }
        pStmt->pLabelId = pLabelIdList;
        pStmt->pstmtOuter = m_pstmtCur;
        SetCurrentStatement(pStmt);
    }

    void PopStmt(StmtNest *pStmt);

    BlockInfoStack *PushBlockInfo(ParseNodeBlock * pnodeBlock);
    void PopBlockInfo();
    void PushDynamicBlock();
    void PopDynamicBlock();

    void MarkEvalCaller()
    {
        if (this->GetCurrentFunctionNode())
        {
            ParseNodeFnc *pnodeFunc = GetCurrentFunctionNode();
            pnodeFunc->SetCallsEval(true);
        }
        ParseNodeBlock *pnodeBlock = GetCurrentBlock();
        if (pnodeBlock != NULL)
        {
            pnodeBlock->SetCallsEval(true);
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

    class AutoDeferErrorsRestore
    {
    public:
        AutoDeferErrorsRestore(Parser *p)
            : m_parser(p)
        {
            m_deferEllipsisErrorSave = m_parser->m_deferEllipsisError;
            m_deferCommaError = m_parser->m_deferCommaError;
            m_ellipsisErrorLocSave = m_parser->m_deferEllipsisErrorLoc;
            m_commaErrorLocSave = m_parser->m_deferCommaErrorLoc;
        }

        ~AutoDeferErrorsRestore()
        {
            m_parser->m_deferEllipsisError = m_deferEllipsisErrorSave;
            m_parser->m_deferCommaError = m_deferCommaError;
            m_parser->m_deferEllipsisErrorLoc = m_ellipsisErrorLocSave;
            m_parser->m_deferCommaErrorLoc = m_commaErrorLocSave;
        }
    private:
        Parser *m_parser;
        RestorePoint m_ellipsisErrorLocSave;
        RestorePoint m_commaErrorLocSave;
        bool m_deferEllipsisErrorSave;
        bool m_deferCommaError;
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
    void CheckForDuplicateExportEntry(IdentPtr exportName);
    void CheckForDuplicateExportEntry(ModuleImportOrExportEntryList* exportEntryList, IdentPtr exportName);

    ParseNodeVar * CreateModuleImportDeclNode(IdentPtr localName);

public:
    WellKnownPropertyPids* names(){ return &wellKnownPropertyPids; }

    IdentPtr CreatePid(__in_ecount(len) LPCOLESTR name, charcount_t len)
    {
        return this->GetHashTbl()->PidHashNameLen(name, len);
    }

    bool KnownIdent(__in_ecount(len) LPCOLESTR name, charcount_t len)
    {
        return this->GetHashTbl()->Contains(name, len);
    }

    template <typename THandler>
    static void ForEachItemRefInList(ParseNodePtr *list, THandler handler)
    {
        ParseNodePtr *current = list;
        while (current != nullptr && (*current) != nullptr)
        {
            if ((*current)->nop == knopList)
            {
                handler(&(*current)->AsParseNodeBin()->pnode1);

                // Advance to the next node
                current = &(*current)->AsParseNodeBin()->pnode2;
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
            bindIdentNode = bindIdentNode->AsParseNodeBin()->pnode1;
        }
        else if (bindIdentNode->nop == knopEllipsis)
        {
            bindIdentNode = bindIdentNode->AsParseNodeUni()->pnode1;
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
            patternNode = patternNode->AsParseNodeBin()->pnode1;
        }

        Assert(patternNode->IsPattern());
        if (patternNode->nop == knopArrayPattern)
        {
            ForEachItemInList(patternNode->AsParseNodeArrLit()->pnode1, [&](ParseNodePtr item) {
                MapBindIdentifierFromElement(item, handler);
            });
        }
        else
        {
            ForEachItemInList(patternNode->AsParseNodeUni()->pnode1, [&](ParseNodePtr item) {
                Assert(item->nop == knopObjectPatternMember || item->nop == knopEllipsis);
                if (item->nop == knopObjectPatternMember)
                {
                    MapBindIdentifierFromElement(item->AsParseNodeBin()->pnode2, handler);
                }
                else
                {
                    MapBindIdentifierFromElement(item->AsParseNodeUni()->pnode1, handler);
                }
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
    void CheckArgumentsUse(IdentPtr pid, ParseNodeFnc * pnodeFnc);

    void CheckStrictModeEvalArgumentsUsage(IdentPtr pid, ParseNodePtr pnode = NULL);

    // environments on which the strict mode is set, if found
    enum StrictModeEnvironment
    {
        SM_NotUsed,         // StrictMode environment is don't care
        SM_OnGlobalCode,    // The current environment is a global code
        SM_OnFunctionCode,  // The current environment is a function code
        SM_DeferredParse    // StrictMode used in deferred parse cases
    };

    template<bool buildAST> ParseNodeArrLit * ParseArrayLiteral();

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
#if ENABLE_BACKGROUND_PARSING
    bool FastScanFormalsAndBody();
#endif
    bool ScanAheadToFunctionEnd(uint count);

    bool DoParallelParse(ParseNodeFnc * pnodeFnc) const;

    // TODO: We should really call this StartScope and separate out the notion of scopes and blocks;
    // blocks refer to actual curly braced syntax, whereas scopes contain symbols.  All blocks have
    // a scope, but some statements like for loops or the with statement introduce a block-less scope.
    template<bool buildAST> ParseNodeBlock * StartParseBlock(PnodeBlockType blockType, ScopeType scopeType, LabelId* pLabelId = nullptr);
    template<bool buildAST> ParseNodeBlock * StartParseBlockWithCapacity(PnodeBlockType blockType, ScopeType scopeType, int capacity);
    template<bool buildAST> ParseNodeBlock * StartParseBlockHelper(PnodeBlockType blockType, Scope *scope, LabelId* pLabelId);
    void PushFuncBlockScope(ParseNodeBlock * pnodeBlock, ParseNodePtr **ppnodeScopeSave, ParseNodePtr **ppnodeExprScopeSave);
    void PopFuncBlockScope(ParseNodePtr *ppnodeScopeSave, ParseNodePtr *ppnodeExprScopeSave);
    template<bool buildAST> ParseNodeBlock * ParseBlock(LabelId* pLabelId);
    void FinishParseBlock(ParseNodeBlock * pnodeBlock, bool needScanRCurly = true);
    void FinishParseFncExprScope(ParseNodeFnc * pnodeFnc, ParseNodeBlock * pnodeFncExprScope);

    bool IsSpecialName(IdentPtr pid);
    void CreateSpecialSymbolDeclarations(ParseNodeFnc * pnodeFnc);
    ParseNodeSpecialName * ReferenceSpecialName(IdentPtr pid, charcount_t ichMin = 0, charcount_t ichLim = 0, bool createNode = false);
    ParseNodeVar * CreateSpecialVarDeclIfNeeded(ParseNodeFnc * pnodeFnc, IdentPtr pid, bool forceCreate = false);

    void ProcessCapturedNames(ParseNodeFnc* pnodeFnc);
    void AddNestedCapturedNames(ParseNodeFnc* pnodeChildFnc);

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
    template<bool buildAST> IdentPtr ParseSuper(bool fAllowCall);

    bool IsTerminateToken(bool fAllowIn);

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
    template<bool buildAST> ParseNodeBin * ParseMemberGetSet(OpCode nop, LPCOLESTR* ppNameHint,size_t iecpMin, charcount_t ichMin);
    template<bool buildAST> ParseNode * ParseFncDeclCheckScope(ushort flags, bool fAllowIn = true);
    template<bool buildAST> ParseNodeFnc * ParseFncDeclNoCheckScope(ushort flags, SuperRestrictionState::State superRestrictionState = SuperRestrictionState::Disallowed, LPCOLESTR pNameHint = nullptr, const bool needsPIDOnRCurlyScan = false, bool fUnaryOrParen = false, bool fAllowIn = true);
    template<bool buildAST> ParseNodeFnc * ParseFncDeclInternal(ushort flags, LPCOLESTR pNameHint, const bool needsPIDOnRCurlyScan, bool fUnaryOrParen, bool noStmtContext, SuperRestrictionState::State superRestrictionState = SuperRestrictionState::Disallowed, bool fAllowIn = true);
    template<bool buildAST> void ParseFncName(ParseNodeFnc * pnodeFnc, ushort flags, IdentPtr* pFncNamePid = nullptr);
    template<bool buildAST> void ParseFncFormals(ParseNodeFnc * pnodeFnc, ParseNodeFnc * pnodeParentFnc, ushort flags, bool isTopLevelDeferredFunc = false);
    template<bool buildAST> void ParseFncDeclHelper(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint, ushort flags, bool fUnaryOrParen, bool noStmtContext, bool *pNeedScanRCurly, bool skipFormals = false, IdentPtr* pFncNamePid = nullptr, bool fAllowIn = true);
    template<bool buildAST> void ParseExpressionLambdaBody(ParseNodeFnc * pnodeFnc, bool fAllowIn = true);
    template<bool buildAST> void UpdateCurrentNodeFunc(ParseNodeFnc * pnodeFnc, bool fLambda);
    bool FncDeclAllowedWithoutContext(ushort flags);
    void FinishFncDecl(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint, bool fLambda, bool skipCurlyBraces = false, bool fAllowIn = true);
    void ParseTopLevelDeferredFunc(ParseNodeFnc * pnodeFnc, ParseNodeFnc * pnodeFncParent, LPCOLESTR pNameHint, bool fLambda, bool *pNeedScanRCurly = nullptr, bool fAllowIn = true);
    void ParseNestedDeferredFunc(ParseNodeFnc * pnodeFnc, bool fLambda, bool *pNeedScanRCurly, bool *pStrictModeTurnedOn, bool fAllowIn = true);
    void CheckStrictFormalParameters();
    ParseNodeVar * AddArgumentsNodeToVars(ParseNodeFnc * pnodeFnc);
    ParseNodeVar * InsertVarAtBeginning(ParseNodeFnc * pnodeFnc, IdentPtr pid);
    ParseNodeVar * CreateSpecialVarDeclNode(ParseNodeFnc * pnodeFnc, IdentPtr pid);
    void UpdateArgumentsNode(ParseNodeFnc * pnodeFnc, ParseNodeVar * argNode);
    void UpdateOrCheckForDuplicateInFormals(IdentPtr pid, SList<IdentPtr> *formals);

    LPCOLESTR GetFunctionName(ParseNodeFnc * pnodeFnc, LPCOLESTR pNameHint);
    uint CalculateFunctionColumnNumber();

    template<bool buildAST> ParseNodeFnc * GenerateEmptyConstructor(bool extends = false);

    template<bool buildAST> ParseNodePtr GenerateModuleFunctionWrapper();

    IdentPtr ParseClassPropertyName(IdentPtr * hint);
    template<bool buildAST> ParseNodeClass * ParseClassDecl(BOOL isDeclaration, LPCOLESTR pNameHint, uint32 *pHintLength, uint32 *pShortNameOffset);

    template<bool buildAST> ParseNodePtr ParseStringTemplateDecl(ParseNodePtr pnodeTagFnc);

    // This is used in the es6 class pattern.
    LPCOLESTR ConstructFinalHintNode(IdentPtr pClassName, IdentPtr pMemberName, IdentPtr pGetSet, bool isStatic, uint32* nameLength, uint32* pShortNameOffset, bool isComputedName = false, LPCOLESTR pMemberNameHint = nullptr);

    // Construct the name from the parse node.
    LPCOLESTR FormatPropertyString(LPCOLESTR propertyString, ParseNodePtr pNode, uint32 *fullNameHintLength, uint32 *pShortNameOffset);
    LPCOLESTR ConstructNameHint(ParseNodeBin * pNode, uint32* fullNameHintLength, uint32 *pShortNameOffset);
    LPCOLESTR AppendNameHints(IdentPtr  left, IdentPtr  right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(IdentPtr  left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR left, IdentPtr  right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR left, LPCOLESTR right, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    LPCOLESTR AppendNameHints(LPCOLESTR leftStr, uint32 leftLen, LPCOLESTR rightStr, uint32 rightLen, uint32 *pNameLength, uint32 *pShortNameOffset, bool ignoreAddDotWithSpace = false, bool wrapInBrackets = false);
    WCHAR * AllocateStringOfLength(ULONG length);

    void FinishFncNode(ParseNodeFnc * pnodeFnc, bool fAllowIn = true);

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
        BOOL fCanAssignToCall = TRUE,
        _Out_opt_ BOOL* pfCanAssign = nullptr,
        _Inout_opt_ BOOL* pfLikelyPattern = nullptr,
        _Out_opt_ bool* pfIsDotOrIndex = nullptr,
        _Inout_opt_ charcount_t *plastRParen = nullptr);

    template<bool buildAST> ParseNodePtr ParsePostfixOperators(
        ParseNodePtr pnode,
        BOOL fAllowCall, 
        BOOL fInNew, 
        BOOL isAsyncExpr,
        BOOL fCanAssignToCallResult,
        BOOL *pfCanAssign, 
        _Inout_ IdentToken* pToken, 
        _Out_opt_ bool* pfIsDotOrIndex = nullptr);

    void ThrowNewTargetSyntaxErrForGlobalScope();

    template<bool buildAST> IdentPtr ParseMetaProperty(
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
    BOOL NodeIsSuperName(ParseNodePtr pnode);
    BOOL IsJSONValid(ParseNodePtr pnodeExpr)
    {
        OpCode jnop = (knopNeg == pnodeExpr->nop) ? pnodeExpr->AsParseNodeUni()->pnode1->nop : pnodeExpr->nop;
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

    ParseNodePtr ConvertMemberToMemberPattern(ParseNodePtr pnodeMember);
    ParseNodeUni * ConvertObjectToObjectPattern(ParseNodePtr pnodeMemberList);
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
    template<bool buildAST> ParseNodeStmt * ParseTryCatchFinally();
    template<bool buildAST> ParseNodeTry * ParseTry();
    template<bool buildAST> ParseNodeCatch * ParseCatch();
    template<bool buildAST> ParseNodeFinally * ParseFinally();

    template<bool buildAST> ParseNodeCase * ParseCase(ParseNodePtr *ppnodeBody);
    template<bool buildAST> ParseNodeRegExp * ParseRegExp();

    template <bool buildAST>
    ParseNodeUni * ParseDestructuredArrayLiteral(tokens declarationType, bool isDecl, bool topLevel = true);

    template <bool buildAST>
    ParseNodeUni * ParseDestructuredObjectLiteral(tokens declarationType, bool isDecl, bool topLevel = true);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredLiteral(tokens declarationType,
        bool isDecl,
        bool topLevel = true,
        DestructuringInitializerContext initializerContext = DIC_None,
        bool allowIn = true,
        BOOL *forInOfOkay = nullptr,
        BOOL *nativeForOkay = nullptr);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredVarDecl(tokens declarationType, bool isDecl, bool *hasSeenRest, bool topLevel = true, bool allowEmptyExpression = true, bool isObjectPattern = false);

    template <bool buildAST>
    ParseNodePtr ParseDestructuredInitializer(ParseNodeUni * lhsNode,
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

private:
    void DeferOrEmitPotentialSpreadError(ParseNodePtr pnodeT);
    template<bool buildAST> void TrackAssignment(ParseNodePtr pnodeT, IdentToken* pToken);
    PidRefStack* PushPidRef(IdentPtr pid);
    PidRefStack* FindOrAddPidRef(IdentPtr pid, int blockId, Js::LocalFunctionId funcId);
    void RemovePrevPidRef(IdentPtr pid, PidRefStack *lastRef);
    void SetPidRefsInScopeDynamic(IdentPtr pid, int blockId);

    void RestoreScopeInfo(Js::ScopeInfo * scopeInfo);
    void FinishScopeInfo(Js::ScopeInfo * scopeInfo);

    bool LabelExists(IdentPtr pid, LabelId* pLabelIdList);
    LabelId* CreateLabelId(IdentPtr pid);

    void AddToNodeList(ParseNode ** ppnodeList, ParseNode *** pppnodeLast, ParseNode * pnodeAdd);
    void AddToNodeListEscapedUse(ParseNode ** ppnodeList, ParseNode *** pppnodeLast, ParseNode * pnodeAdd);

    void ChkCurTokNoScan(int tk, int wErr, LPCWSTR stringOne = _u(""), LPCWSTR stringTwo = _u(""))
    {
        if (m_token.tk != tk)
        {
            Error(wErr, stringOne, stringTwo);
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
            this->GetScanner()->Scan();
        }
    }
    void ChkNxtTok(int tk, int wErr)
    {
        this->GetScanner()->Scan();
        ChkCurTok(tk, wErr);
    }

    template <class Fn>
    void FinishFunctionsInScope(ParseNodePtr pnodeScopeList, Fn fn);
    void FinishDeferredFunction(ParseNodeBlock * pnodeScopeList);

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
        fFncClassConstructor        = 1 << 10,
        fFncBaseClassConstructor    = 1 << 11,
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

    class AutoMarkInParsingArgs
    {
    public:
        AutoMarkInParsingArgs(Parser * parser)
            : m_parser(parser)
        {
            m_prevState = m_parser->GetIsInParsingArgList();
            m_prevDestructuringState = m_parser->GetHasDestructuringPattern();
            m_parser->SetHasDestructuringPattern(false);
            m_parser->SetIsInParsingArgList(true);
        }
        ~AutoMarkInParsingArgs()
        {
            m_parser->SetIsInParsingArgList(m_prevState);
            if (!m_prevState)
            {
                m_parser->SetHasDestructuringPattern(false);
            }
            else
            {
                // Reset back to previous state only when the current call node does not have usage of destructuring expression.
                if (!m_parser->GetHasDestructuringPattern())
                {
                    m_parser->SetHasDestructuringPattern(m_prevDestructuringState);
                }
            }
        }

    private:
        Parser *m_parser;
        bool m_prevState;
        bool m_prevDestructuringState;
    };

public:
    charcount_t GetSourceIchLim() { return m_sourceLim; }
    static BOOL NodeEqualsName(ParseNodePtr pnode, LPCOLESTR sz, uint32 cch);
};
