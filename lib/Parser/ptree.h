//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

struct Ident;
typedef Ident *IdentPtr;

class Scope;

/***************************************************************************
Flags for classifying node operators.
***************************************************************************/
const uint fnopNone     = 0x0000;
const uint fnopConst    = 0x0001; // constant
const uint fnopLeaf     = 0x0002; // leaf
const uint fnopUni      = 0x0004; // unary
const uint fnopBin      = 0x0008; // binary
const uint fnopRel      = 0x0010; // relational
const uint fnopAsg      = 0x0020; // assignment
const uint fnopBreak    = 0x0040; // break can be used within this statement
const uint fnopContinue = 0x0080; // continue can be used within this statement
const uint fnopCleanup  = 0x0100; // requires cleanup (eg, with or for-in).
const uint fnopJump     = 0x0200;
const uint fnopNotExprStmt = 0x0400;
const uint fnopBinList  = 0x0800;
const uint fnopExprMask = (fnopLeaf|fnopUni|fnopBin);

/***************************************************************************
Flags for classifying parse nodes.
***************************************************************************/
enum PNodeFlags : ushort
{
    fpnNone                                  = 0x0000,

    // knopFncDecl nodes.
    fpnArguments_overriddenByDecl            = 0x0001, // function has a let/const decl, class or nested function named 'arguments', which overrides the built-in arguments object in the body
    fpnArguments_overriddenInParam           = 0x0002, // function has a parameter named arguments
    fpnArguments_varDeclaration              = 0x0004, // function has a var declaration named 'arguments', which may change the way an 'arguments' identifier is resolved

    // knopVarDecl nodes.
    fpnArguments                             = 0x0008,
    fpnHidden                                = 0x0010,

    // Statement nodes.
    fpnExplicitSemicolon                     = 0x0020, // statement terminated by an explicit semicolon
    fpnAutomaticSemicolon                    = 0x0040, // statement terminated by an automatic semicolon
    fpnMissingSemicolon                      = 0x0080, // statement missing terminating semicolon, and is not applicable for automatic semicolon insertion
    fpnDclList                               = 0x0100, // statement is a declaration list
    fpnSyntheticNode                         = 0x0200, // node is added by the parser or does it represent user code
    fpnIndexOperator                         = 0x0400, // dot operator is an optimization of an index operator
    fpnJumbStatement                         = 0x0800, // break or continue that was removed by error recovery

    // Unary/Binary nodes
    fpnCanFlattenConcatExpr                  = 0x1000, // the result of the binary operation can participate in concat N

    // Potentially overlapping traversal flags
    // These flags are set and cleared during a single node traversal and their values can be used in other node traversals.
    fpnMemberReference                       = 0x2000, // The node is a member reference symbol
    fpnCapturesSyms                          = 0x4000, // The node is a statement (or contains a sub-statement)
                                                       // that captures symbols.

    fpnSpecialSymbol                         = 0x8000,
};

/***************************************************************************
Data structs for ParseNodes. ParseNode includes a union of these.
***************************************************************************/
class ParseNodeUni;
class ParseNodeBin;
class ParseNodeTri;
class ParseNodeInt;
class ParseNodeFloat;
class ParseNodePid;
class ParseNodeVar;
class ParseNodeCall;
class ParseNodeSuperCall;
class ParseNodeSpecialName;
class ParseNodeExportDefault;
class ParseNodeStrTemplate;
class ParseNodeSuperReference;
class ParseNodeArrLit;
class ParseNodeClass;
class ParseNodeParamPattern;

class ParseNodeStmt;
class ParseNodeBlock;
class ParseNodeJump;
class ParseNodeWith;
class ParseNodeIf;
class ParseNodeSwitch;
class ParseNodeCase;
class ParseNodeReturn;
class ParseNodeTryFinally;
class ParseNodeTryCatch;
class ParseNodeTry;
class ParseNodeCatch;
class ParseNodeFinally;
class ParseNodeLoop;
class ParseNodeWhile;
class ParseNodeFor;
class ParseNodeForInOrForOf;

class ParseNodeFnc;
class ParseNodeProg;
class ParseNodeModule;

class ParseNode
{
public:
    void Init(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeUni * AsParseNodeUni();
    ParseNodeBin * AsParseNodeBin();
    ParseNodeTri * AsParseNodeTri();
    ParseNodeInt * AsParseNodeInt();
    ParseNodeFloat * AsParseNodeFloat();
    ParseNodeVar * AsParseNodeVar();
    ParseNodePid * AsParseNodePid();

    ParseNodeSpecialName * AsParseNodeSpecialName();
    ParseNodeExportDefault * AsParseNodeExportDefault();
    ParseNodeStrTemplate * AsParseNodeStrTemplate();
    ParseNodeSuperReference * AsParseNodeSuperReference();
    ParseNodeArrLit * AsParseNodeArrLit();

    ParseNodeCall * AsParseNodeCall();
    ParseNodeSuperCall * AsParseNodeSuperCall();
    ParseNodeClass * AsParseNodeClass();
    ParseNodeParamPattern * AsParseNodeParamPattern();

    ParseNodeStmt * AsParseNodeStmt();
    ParseNodeBlock * AsParseNodeBlock();
    ParseNodeJump * AsParseNodeJump();
    ParseNodeWith * AsParseNodeWith();
    ParseNodeIf * AsParseNodeIf();
    ParseNodeSwitch * AsParseNodeSwitch();
    ParseNodeCase * AsParseNodeCase();
    ParseNodeReturn * AsParseNodeReturn();

    ParseNodeTryFinally * AsParseNodeTryFinally();
    ParseNodeTryCatch * AsParseNodeTryCatch();
    ParseNodeTry * AsParseNodeTry();
    ParseNodeCatch * AsParseNodeCatch();
    ParseNodeFinally * AsParseNodeFinally();

    ParseNodeLoop * AsParseNodeLoop();
    ParseNodeWhile * AsParseNodeWhile();
    ParseNodeFor * AsParseNodeFor();
    ParseNodeForInOrForOf * AsParseNodeForInOrForOf();

    ParseNodeFnc * AsParseNodeFnc();
    ParseNodeProg * AsParseNodeProg();
    ParseNodeModule * AsParseNodeModule();

    static uint Grfnop(int nop)
    {
        Assert(nop < knopLim);
        return nop < knopLim ? mpnopgrfnop[nop] : fnopNone;
    }

    BOOL IsStatement()
    {
        return nop >= knopList || ((Grfnop(nop) & fnopAsg) != 0);
    }

    uint Grfnop(void)
    {
        Assert(nop < knopLim);
        return nop < knopLim ? mpnopgrfnop[nop] : fnopNone;
    }

    IdentPtr name();

    charcount_t LengthInCodepoints() const
    {
        return (this->ichLim - this->ichMin);
    }

    // This node is a function decl node and function has a var declaration named 'arguments',
    bool HasVarArguments() const
    {
        return ((nop == knopFncDecl) && (grfpn & PNodeFlags::fpnArguments_varDeclaration));
    }

    bool CapturesSyms() const
    {
        return (grfpn & PNodeFlags::fpnCapturesSyms) != 0;
    }

    void SetCapturesSyms()
    {
        grfpn |= PNodeFlags::fpnCapturesSyms;
    }

    bool IsInList() const { return this->isInList; }
    void SetIsInList() { this->isInList = true; }

    bool IsNotEscapedUse() const { return this->notEscapedUse; }
    void SetNotEscapedUse() { this->notEscapedUse = true; }

    bool CanFlattenConcatExpr() const { return !!(this->grfpn & PNodeFlags::fpnCanFlattenConcatExpr); }

    bool IsCallApplyTargetLoad() { return isCallApplyTargetLoad; }
    void SetIsCallApplyTargetLoad() { isCallApplyTargetLoad = true; }

    bool IsSpecialName() { return isSpecialName; }
    void SetIsSpecialName() { isSpecialName = true; }

    bool IsUserIdentifier() const
    {
        return this->nop == knopName && !this->isSpecialName;
    }

    bool IsVarLetOrConst() const
    {
        return this->nop == knopVarDecl || this->nop == knopLetDecl || this->nop == knopConstDecl;
    }

    ParseNodePtr GetFormalNext();

    bool IsPattern() const
    {
        return nop == knopObjectPattern || nop == knopArrayPattern;
    }

#if DBG_DUMP
    void Dump();
#endif
public:
    static const uint mpnopgrfnop[knopLim];

    OpCode nop;
    ushort grfpn;
    charcount_t ichMin;         // start offset into the original source buffer
    charcount_t ichLim;         // end offset into the original source buffer
    Js::RegSlot location;
    bool isUsed;                // indicates whether an expression such as x++ is used
    bool emitLabels;
    bool notEscapedUse;         // Use by byte code generator.  Currently, only used by child of knopComma
    bool isInList;
    bool isCallApplyTargetLoad;
    bool isSpecialName;         // indicates a PnPid (knopName) is a special name like 'this' or 'super'
#ifdef EDIT_AND_CONTINUE
    ParseNodePtr parent;
#endif
};

// unary operators
class ParseNodeUni : public ParseNode
{
public:
    ParseNodePtr pnode1;
};

// binary operators
class ParseNodeBin : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnode1;
    ParseNodePtr pnode2;
};

// ternary operator
class ParseNodeTri : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnode1;
    ParseNodePtr pnode2;
    ParseNodePtr pnode3;
};

// integer constant
class ParseNodeInt : public ParseNode
{
public:
    int32 lw;
};

// double constant
class ParseNodeFloat : public ParseNode
{
public:
    double dbl;
    bool maybeInt : 1;
};

// identifier or string
class Symbol;
struct PidRefStack;
class ParseNodePid : public ParseNode
{
public:
    IdentPtr pid;
    Symbol **symRef;
    Symbol *sym;
    UnifiedRegex::RegexPattern* regexPattern;
    uint regexPatternIndex;

    void SetSymRef(PidRefStack *ref);
    Symbol **GetSymRef() const { return symRef; }
    Js::PropertyId PropertyIdFromNameNode() const;
};

// variable declaration
class ParseNodeVar : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    IdentPtr pid;
    Symbol *sym;
    Symbol **symRef;
    ParseNodePtr pnodeInit;
    BOOLEAN isSwitchStmtDecl;
    BOOLEAN isBlockScopeFncDeclVar;

    void InitDeclNode(IdentPtr name, ParseNodePtr initExpr)
    {
        this->pid = name;
        this->pnodeInit = initExpr;
        this->pnodeNext = nullptr;
        this->sym = nullptr;
        this->symRef = nullptr;
        this->isSwitchStmtDecl = false;
        this->isBlockScopeFncDeclVar = false;
    }
};

// Array literal
class ParseNodeArrLit : public ParseNodeUni
{
public:
    uint count;
    uint spreadCount;
    BYTE arrayOfTaggedInts:1;     // indicates that array initializer nodes are all tagged ints
    BYTE arrayOfInts:1;           // indicates that array initializer nodes are all ints
    BYTE arrayOfNumbers:1;        // indicates that array initializer nodes are all numbers
    BYTE hasMissingValues:1;
};

class FuncInfo;

enum PnodeBlockType : unsigned
{
    Global,
    Function,
    Regular,
    Parameter
};

enum FncFlags : uint
{
    kFunctionNone                               = 0,
    kFunctionNested                             = 1 << 0, // True if function is nested in another.
    kFunctionDeclaration                        = 1 << 1, // is this a declaration or an expression?
    kFunctionCallsEval                          = 1 << 2, // function uses eval
    kFunctionUsesArguments                      = 1 << 3, // function uses arguments
    kFunctionHasHeapArguments                   = 1 << 4, // function's "arguments" escape the scope
    kFunctionHasReferenceableBuiltInArguments   = 1 << 5, // the built-in 'arguments' object is referenceable in the function
    kFunctionIsAccessor                         = 1 << 6, // function is a property getter or setter
    kFunctionHasNonThisStmt                     = 1 << 7,
    kFunctionStrictMode                         = 1 << 8,
    kFunctionHasDestructuredParams              = 1 << 9,
    kFunctionIsModule                           = 1 << 10, // function is a module body
    // Free = 1 << 11,
    kFunctionHasWithStmt                        = 1 << 12, // function (or child) uses with
    kFunctionIsLambda                           = 1 << 13,
    kFunctionChildCallsEval                     = 1 << 14,
    kFunctionHasNonSimpleParameterList          = 1 << 15,
    kFunctionHasSuperReference                  = 1 << 16,
    kFunctionIsMethod                           = 1 << 17,
    kFunctionIsClassConstructor                 = 1 << 18, // function is a class constructor
    kFunctionIsBaseClassConstructor             = 1 << 19, // function is a base class constructor
    kFunctionIsClassMember                      = 1 << 20, // function is a class member
    // Free = 1 << 21,
    kFunctionIsGeneratedDefault                 = 1 << 22, // Is the function generated by us as a default (e.g. default class constructor)
    kFunctionHasDefaultArguments                = 1 << 23, // Function has one or more ES6 default arguments
    kFunctionIsStaticMember                     = 1 << 24,
    kFunctionIsGenerator                        = 1 << 25, // Function is an ES6 generator function
    kFunctionAsmjsMode                          = 1 << 26,
    // Free = 1 << 27,
    kFunctionIsAsync                            = 1 << 28, // function is async
    kFunctionHasDirectSuper                     = 1 << 29, // super()
    kFunctionIsDefaultModuleExport              = 1 << 30, // function is the default export of a module
    kFunctionHasAnyWriteToFormals               = (uint)1 << 31  // To Track if there are any writes to formals.
};

struct RestorePoint;
struct DeferredFunctionStub;

// function declaration
class ParseNodeFnc : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnodeName;
    IdentPtr pid;
    LPCOLESTR hint;
    uint32 hintLength;
    uint32 hintOffset;
    bool  isNameIdentifierRef;
    bool  nestedFuncEscapes;
    ParseNodePtr pnodeScopes;
    ParseNodePtr pnodeBodyScope;
    ParseNodePtr pnodeParams;
    ParseNodePtr pnodeVars;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeRest;

    FuncInfo *funcInfo; // function information gathered during byte code generation
    Scope *scope;

    uint nestedCount; // Nested function count (valid until children have been processed)
    uint nestedIndex; // Index within the parent function

    uint16 firstDefaultArg; // Position of the first default argument, if any

    FncFlags fncFlags;
    int32 astSize;
    size_t cbMin; // Min an Lim UTF8 offsets.
    size_t cbLim;
    ULONG lineNumber;   // Line number relative to the current source buffer of the function declaration.
    ULONG columnNumber; // Column number of the declaration.
    Js::LocalFunctionId functionId;
#if DBG
    Js::LocalFunctionId deferredParseNextFunctionId;
#endif
    RestorePoint *pRestorePoint;
    DeferredFunctionStub *deferredStub;
    bool canBeDeferred;
    bool isBodyAndParamScopeMerged; // Indicates whether the param scope and the body scope of the function can be merged together or not.
                                    // We cannot merge both scopes together if there is any closure capture or eval is present in the param scope.

    static const int32 MaxStackClosureAST = 800000;

    static bool CanBeRedeferred(FncFlags flags) { return !(flags & (kFunctionIsGenerator | kFunctionIsAsync)); }

private:
    void SetFlags(uint flags, bool set)
    {
        if (set)
        {
            fncFlags = (FncFlags)(fncFlags | flags);
        }
        else
        {
            fncFlags = (FncFlags)(fncFlags & ~flags);
        }
    }

    bool HasFlags(uint flags) const
    {
        return (fncFlags & flags) == flags;
    }

    bool HasAnyFlags(uint flags) const
    {
        return (fncFlags & flags) != 0;
    }

    bool HasNoFlags(uint flags) const
    {
        return (fncFlags & flags) == 0;
    }

public:
    void ClearFlags()
    {
        fncFlags = kFunctionNone;
        canBeDeferred = false;
        isBodyAndParamScopeMerged = true;
    }

    void SetAsmjsMode(bool set = true) { SetFlags(kFunctionAsmjsMode, set); }
    void SetCallsEval(bool set = true) { SetFlags(kFunctionCallsEval, set); }
    void SetChildCallsEval(bool set = true) { SetFlags(kFunctionChildCallsEval, set); }
    void SetDeclaration(bool set = true) { SetFlags(kFunctionDeclaration, set); }
    void SetHasDefaultArguments(bool set = true) { SetFlags(kFunctionHasDefaultArguments, set); }
    void SetHasDestructuredParams(bool set = true) { SetFlags(kFunctionHasDestructuredParams, set); }
    void SetHasHeapArguments(bool set = true) { SetFlags(kFunctionHasHeapArguments, set); }
    void SetHasAnyWriteToFormals(bool set = true) { SetFlags((uint)kFunctionHasAnyWriteToFormals, set); }
    void SetHasNonSimpleParameterList(bool set = true) { SetFlags(kFunctionHasNonSimpleParameterList, set); }
    void SetHasNonThisStmt(bool set = true) { SetFlags(kFunctionHasNonThisStmt, set); }
    void SetHasReferenceableBuiltInArguments(bool set = true) { SetFlags(kFunctionHasReferenceableBuiltInArguments, set); }
    void SetHasSuperReference(bool set = true) { SetFlags(kFunctionHasSuperReference, set); }
    void SetHasDirectSuper(bool set = true) { SetFlags(kFunctionHasDirectSuper, set); }
    void SetHasWithStmt(bool set = true) { SetFlags(kFunctionHasWithStmt, set); }
    void SetIsAccessor(bool set = true) { SetFlags(kFunctionIsAccessor, set); }
    void SetIsAsync(bool set = true) { SetFlags(kFunctionIsAsync, set); }
    void SetIsClassConstructor(bool set = true) { SetFlags(kFunctionIsClassConstructor, set); }
    void SetIsBaseClassConstructor(bool set = true) { SetFlags(kFunctionIsBaseClassConstructor, set); }
    void SetIsClassMember(bool set = true) { SetFlags(kFunctionIsClassMember, set); }
    void SetIsGeneratedDefault(bool set = true) { SetFlags(kFunctionIsGeneratedDefault, set); }
    void SetIsGenerator(bool set = true) { SetFlags(kFunctionIsGenerator, set); }
    void SetIsLambda(bool set = true) { SetFlags(kFunctionIsLambda, set); }
    void SetIsMethod(bool set = true) { SetFlags(kFunctionIsMethod, set); }
    void SetIsStaticMember(bool set = true) { SetFlags(kFunctionIsStaticMember, set); }
    void SetNested(bool set = true) { SetFlags(kFunctionNested, set); }
    void SetStrictMode(bool set = true) { SetFlags(kFunctionStrictMode, set); }
    void SetIsModule(bool set = true) { SetFlags(kFunctionIsModule, set); }
    void SetUsesArguments(bool set = true) { SetFlags(kFunctionUsesArguments, set); }
    void SetIsDefaultModuleExport(bool set = true) { SetFlags(kFunctionIsDefaultModuleExport, set); }
    void SetNestedFuncEscapes(bool set = true) { nestedFuncEscapes = set; }
    void SetCanBeDeferred(bool set = true) { canBeDeferred = set; }
    void ResetBodyAndParamScopeMerged() { isBodyAndParamScopeMerged = false; }

    bool CallsEval() const { return HasFlags(kFunctionCallsEval); }
    bool ChildCallsEval() const { return HasFlags(kFunctionChildCallsEval); }
    bool GetArgumentsObjectEscapes() const { return HasFlags(kFunctionHasHeapArguments); }
    bool GetAsmjsMode() const { return HasFlags(kFunctionAsmjsMode); }
    bool GetStrictMode() const { return HasFlags(kFunctionStrictMode); }
    bool HasDefaultArguments() const { return HasFlags(kFunctionHasDefaultArguments); }
    bool HasDestructuredParams() const { return HasFlags(kFunctionHasDestructuredParams); }
    bool HasHeapArguments() const { return true; /* HasFlags(kFunctionHasHeapArguments); Disabling stack arguments. Always return HeapArguments as True */ }
    bool HasAnyWriteToFormals() const { return HasFlags((uint)kFunctionHasAnyWriteToFormals); }
    bool HasOnlyThisStmts() const { return !HasFlags(kFunctionHasNonThisStmt); }
    bool HasReferenceableBuiltInArguments() const { return HasFlags(kFunctionHasReferenceableBuiltInArguments); }
    bool HasSuperReference() const { return HasFlags(kFunctionHasSuperReference); }
    bool HasDirectSuper() const { return HasFlags(kFunctionHasDirectSuper); }
    bool HasNonSimpleParameterList() { return HasFlags(kFunctionHasNonSimpleParameterList); }
    bool HasWithStmt() const { return HasFlags(kFunctionHasWithStmt); }
    bool IsAccessor() const { return HasFlags(kFunctionIsAccessor); }
    bool IsAsync() const { return HasFlags(kFunctionIsAsync); }
    bool IsConstructor() const { return HasNoFlags(kFunctionIsAsync|kFunctionIsLambda|kFunctionIsAccessor);  }
    bool IsClassConstructor() const { return HasFlags(kFunctionIsClassConstructor); }
    bool IsBaseClassConstructor() const { return HasFlags(kFunctionIsBaseClassConstructor); }
    bool IsDerivedClassConstructor() const { return IsClassConstructor() && !IsBaseClassConstructor(); }
    bool IsClassMember() const { return HasFlags(kFunctionIsClassMember); }
    bool IsDeclaration() const { return HasFlags(kFunctionDeclaration); }
    bool IsGeneratedDefault() const { return HasFlags(kFunctionIsGeneratedDefault); }
    bool IsGenerator() const { return HasFlags(kFunctionIsGenerator); }
    bool IsCoroutine() const { return HasAnyFlags(kFunctionIsGenerator | kFunctionIsAsync); }
    bool IsLambda() const { return HasFlags(kFunctionIsLambda); }
    bool IsMethod() const { return HasFlags(kFunctionIsMethod); }
    bool IsNested() const { return HasFlags(kFunctionNested); }
    bool IsStaticMember() const { return HasFlags(kFunctionIsStaticMember); }
    bool IsModule() const { return HasFlags(kFunctionIsModule); }
    bool UsesArguments() const { return HasFlags(kFunctionUsesArguments); }
    bool IsDefaultModuleExport() const { return HasFlags(kFunctionIsDefaultModuleExport); }
    bool NestedFuncEscapes() const { return nestedFuncEscapes; }
    bool CanBeDeferred() const { return canBeDeferred; }
    bool IsBodyAndParamScopeMerged() { return isBodyAndParamScopeMerged; }
    // Only allow the normal functions to be asm.js
    bool IsAsmJsAllowed() const { return (fncFlags & ~(
        kFunctionAsmjsMode |
        kFunctionNested |
        kFunctionDeclaration |
        kFunctionStrictMode |
        kFunctionHasReferenceableBuiltInArguments |
        kFunctionHasNonThisStmt |
        // todo:: we shouldn't accept kFunctionHasAnyWriteToFormals on the asm module, but it looks like a bug is setting that flag incorrectly
        kFunctionHasAnyWriteToFormals
    )) == 0; }

    size_t LengthInBytes()
    {
        return cbLim - cbMin;
    }

    Symbol *GetFuncSymbol();
    void SetFuncSymbol(Symbol *sym);

    ParseNodePtr GetParamScope() const;
    ParseNodePtr GetBodyScope() const;
    ParseNodePtr GetTopLevelScope() const
    {
        // Top level scope will be the same for knopProg and knopFncDecl.
        return GetParamScope();
    }

    template<typename Fn>
    void MapContainerScopes(Fn fn)
    {
        fn(this->pnodeScopes->AsParseNodeBlock()->pnodeScopes);
        if (this->pnodeBodyScope != nullptr)
        {
            fn(this->pnodeBodyScope->AsParseNodeBlock()->pnodeScopes);
        }
    }
};

// class declaration
class ParseNodeClass : public ParseNode
{
public:
    ParseNodePtr pnodeName;
    ParseNodePtr pnodeDeclName;
    ParseNodePtr pnodeBlock;
    ParseNodePtr pnodeConstructor;
    ParseNodePtr pnodeMembers;
    ParseNodePtr pnodeStaticMembers;
    ParseNodePtr pnodeExtends;

    bool isDefaultModuleExport;

    void SetIsDefaultModuleExport(bool set) { isDefaultModuleExport = set; }
    bool IsDefaultModuleExport() const { return isDefaultModuleExport; }
};

// export default expr
class ParseNodeExportDefault : public ParseNode
{
public:
    ParseNodePtr pnodeExpr;
};

// string template declaration
class ParseNodeStrTemplate : public ParseNode
{
public:
    ParseNodePtr pnodeStringLiterals;
    ParseNodePtr pnodeStringRawLiterals;
    ParseNodePtr pnodeSubstitutionExpressions;
    uint16 countStringLiterals;
    BYTE isTaggedTemplate:1;
};

// global program
class ParseNodeProg : public ParseNodeFnc
{
public:
    ParseNodePtr pnodeLastValStmt;
    bool m_UsesArgumentsAtGlobal;
};

// global module
class ParseNodeModule : public ParseNodeProg
{
public:
    ModuleImportOrExportEntryList* localExportEntries;
    ModuleImportOrExportEntryList* indirectExportEntries;
    ModuleImportOrExportEntryList* starExportEntries;
    ModuleImportOrExportEntryList* importEntries;
    IdentPtrList* requestedModules;
};

// function call
class ParseNodeCall : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnodeTarget;
    ParseNodePtr pnodeArgs;
    uint16 argCount;
    uint16 spreadArgCount;
    BYTE callOfConstants : 1;
    BYTE isApplyCall : 1;
    BYTE isEvalCall : 1;
    BYTE isSuperCall : 1;
    BYTE hasDestructuring : 1;

    void Init(OpCode nop, ParseNodePtr pnodeTarget, ParseNodePtr pnodeArgs, charcount_t ichMin, charcount_t ichLim);
};

// base for statement nodes
class ParseNodeStmt : public ParseNode
{
public:
    ParseNodePtr pnodeOuter;

    // Set by parsing code, used by code gen.
    uint grfnop;

    // Needed for byte code gen.
    Js::ByteCodeLabel breakLabel;
    Js::ByteCodeLabel continueLabel;
};

// block { }
class ParseNodeBlock : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeStmt;
    ParseNodePtr pnodeLastValStmt;
    ParseNodePtr pnodeLexVars;
    ParseNodePtr pnodeScopes;
    ParseNodePtr pnodeNext;
    Scope        *scope;

    ParseNodePtr enclosingBlock;
    int blockId;
    PnodeBlockType blockType:2;
    BYTE         callsEval:1;
    BYTE         childCallsEval:1;

    void Init(int blockId, PnodeBlockType blockType, charcount_t ichMin, charcount_t ichLim);

    void SetCallsEval(bool does) { callsEval = does; }
    bool GetCallsEval() const { return callsEval; }

    void SetChildCallsEval(bool does) { childCallsEval = does; }
    bool GetChildCallsEval() const { return childCallsEval; }

    void SetEnclosingBlock(ParseNodePtr pnode) { enclosingBlock = pnode; }
    ParseNodePtr GetEnclosingBlock() const { return enclosingBlock; }

    bool HasBlockScopedContent() const;
};

// break and continue
class ParseNodeJump : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeTarget;
    bool hasExplicitTarget;
};

// base for loop nodes
class ParseNodeLoop : public ParseNodeStmt
{
public:
    // Needed for byte code gen
    uint loopId;
};

// while and do-while loops
class ParseNodeWhile : public ParseNodeLoop
{
public:
    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeBody;
};

// with
class ParseNodeWith : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeObj;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeScopes;
    ParseNodePtr pnodeNext;
    Scope        *scope;
};

// Destructure pattern for function/catch parameter
class ParseNodeParamPattern : public ParseNode
{
public:
    ParseNodePtr pnodeNext;
    Js::RegSlot location;
    ParseNodePtr pnode1;
};

// if
class ParseNodeIf : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeTrue;
    ParseNodePtr pnodeFalse;
};

// for-in loop
class ParseNodeForInOrForOf : public ParseNodeLoop
{
public:
    ParseNodePtr pnodeObj;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeLval;
    ParseNodePtr pnodeBlock;
    Js::RegSlot itemLocation;
};

// for loop
class ParseNodeFor : public ParseNodeLoop
{
public:
    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeInit;
    ParseNodePtr pnodeIncr;
    ParseNodePtr pnodeBlock;
    ParseNodePtr pnodeInverted;
};

// switch
class ParseNodeSwitch : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeVal;
    ParseNodePtr pnodeCases;
    ParseNodePtr pnodeDefault;
    ParseNodePtr pnodeBlock;
};

// switch case
class ParseNodeCase : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnodeExpr; // nullptr for default
    ParseNodePtr pnodeBody;
    Js::ByteCodeLabel labelCase;
};

// return [expr]
class ParseNodeReturn : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeExpr;
};

// try-catch-finally     
class ParseNodeTryFinally : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeTry;
    ParseNodePtr pnodeFinally;
};

// try-catch
class ParseNodeTryCatch : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeTry;
    ParseNodePtr pnodeCatch;
};

// try-catch
class ParseNodeTry : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeBody;
};

// { catch(e : expr) {body} }
class ParseNodeCatch : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeNext;
    ParseNodePtr pnodeParam;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeScopes;
    Scope        *scope;
};

// finally
class ParseNodeFinally : public ParseNodeStmt
{
public:
    ParseNodePtr pnodeBody;
};

// special name like 'this'
class ParseNodeSpecialName : public ParseNodePid
{
public:
    bool isThis : 1;
    bool isSuper : 1;
};

// binary operator with super reference
class ParseNodeSuperReference : public ParseNodeBin
{
public:
    ParseNodePtr pnodeThis;
};

// call node with super target
class ParseNodeSuperCall : public ParseNodeCall
{
public:
    ParseNodePtr pnodeThis;
    ParseNodePtr pnodeNewTarget;

    void Init(OpCode nop, ParseNodePtr pnodeTarget, ParseNodePtr pnodeArgs, charcount_t ichMin, charcount_t ichLim);
};

const int kcbPnNone           = sizeof(ParseNode);
const int kcbPnArrLit         = sizeof(ParseNodeArrLit);
const int kcbPnBin            = sizeof(ParseNodeBin);
const int kcbPnBlock          = sizeof(ParseNodeBlock);
const int kcbPnCall           = sizeof(ParseNodeCall);
const int kcbPnCase           = sizeof(ParseNodeCase);
const int kcbPnCatch          = sizeof(ParseNodeCatch);
const int kcbPnClass          = sizeof(ParseNodeClass);
const int kcbPnExportDefault  = sizeof(ParseNodeExportDefault);
const int kcbPnFinally        = sizeof(ParseNodeFinally);
const int kcbPnFlt            = sizeof(ParseNodeFloat);
const int kcbPnFnc            = sizeof(ParseNodeFnc);
const int kcbPnFor            = sizeof(ParseNodeFor);
const int kcbPnForIn          = sizeof(ParseNodeForInOrForOf);
const int kcbPnForOf          = sizeof(ParseNodeForInOrForOf);
const int kcbPnIf             = sizeof(ParseNodeIf);
const int kcbPnInt            = sizeof(ParseNodeInt);
const int kcbPnJump           = sizeof(ParseNodeJump);
const int kcbPnModule         = sizeof(ParseNodeModule);
const int kcbPnParamPattern   = sizeof(ParseNodeParamPattern);
const int kcbPnPid            = sizeof(ParseNodePid);
const int kcbPnProg           = sizeof(ParseNodeProg);
const int kcbPnReturn         = sizeof(ParseNodeReturn);
const int kcbPnSpecialName    = sizeof(ParseNodeSpecialName);
const int kcbPnStrTemplate    = sizeof(ParseNodeStrTemplate);
const int kcbPnSuperCall      = sizeof(ParseNodeSuperCall);
const int kcbPnSuperReference = sizeof(ParseNodeSuperReference);
const int kcbPnSwitch         = sizeof(ParseNodeSwitch);
const int kcbPnTri            = sizeof(ParseNodeTri);
const int kcbPnTry            = sizeof(ParseNodeTry);
const int kcbPnTryCatch       = sizeof(ParseNodeTryCatch);
const int kcbPnTryFinally     = sizeof(ParseNodeTryFinally);
const int kcbPnUni            = sizeof(ParseNodeUni);
const int kcbPnVar            = sizeof(ParseNodeVar);
const int kcbPnWhile          = sizeof(ParseNodeWhile);
const int kcbPnWith           = sizeof(ParseNodeWith);

#define AssertNodeMem(pnode) AssertPvCb(pnode, kcbPnNone)
#define AssertNodeMemN(pnode) AssertPvCbN(pnode, kcbPnNone)
