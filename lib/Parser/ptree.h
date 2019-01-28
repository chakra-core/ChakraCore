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
const uint fnopAllowDefer = 0x1000; // allow to be created during defer parse

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
Data structs for ParseNodes.
***************************************************************************/
class ParseNodeUni;
class ParseNodeBin;
class ParseNodeTri;
class ParseNodeInt;
class ParseNodeBigInt;
class ParseNodeFloat;
class ParseNodeRegExp;
class ParseNodeStr;
class ParseNodeName;
class ParseNodeVar;
class ParseNodeCall;
class ParseNodeSuperCall;
class ParseNodeSpecialName;
class ParseNodeExportDefault;
class ParseNodeStrTemplate;
class ParseNodeSuperReference;
class ParseNodeArrLit;
class ParseNodeObjLit;
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
    ParseNode(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeUni * AsParseNodeUni();
    ParseNodeBin * AsParseNodeBin();
    ParseNodeTri * AsParseNodeTri();
    ParseNodeInt * AsParseNodeInt();
    ParseNodeBigInt * AsParseNodeBigInt();
    ParseNodeFloat * AsParseNodeFloat();
    ParseNodeRegExp * AsParseNodeRegExp();
    ParseNodeVar * AsParseNodeVar();
    ParseNodeStr * AsParseNodeStr();
    ParseNodeName * AsParseNodeName();

    ParseNodeSpecialName * AsParseNodeSpecialName();
    ParseNodeExportDefault * AsParseNodeExportDefault();
    ParseNodeStrTemplate * AsParseNodeStrTemplate();
    ParseNodeSuperReference * AsParseNodeSuperReference();
    ParseNodeArrLit * AsParseNodeArrLit();
    ParseNodeObjLit * AsParseNodeObjLit();

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

    bool IsPatternDeclaration() { return isPatternDeclaration; }
    void SetIsPatternDeclaration() { isPatternDeclaration = true; }

    bool IsUserIdentifier();

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

    bool isUsed : 1;                // indicates whether an expression such as x++ is used

private:
    bool isInList : 1;
    // Use by byte code generator
    bool notEscapedUse : 1;         // Currently, only used by child of knopComma
    bool isCallApplyTargetLoad : 1;

    // Use by bytecodegen to identify the current node is a destructuring pattern declaration node.
    bool isPatternDeclaration : 1;

public:
    ushort grfpn;

    charcount_t ichMin;         // start offset into the original source buffer
    charcount_t ichLim;         // end offset into the original source buffer
    Js::RegSlot location;

#ifdef EDIT_AND_CONTINUE
    ParseNodePtr parent;
#endif
};

#define DISABLE_SELF_CAST(T) private: T * As##T()
// unary operators
class ParseNodeUni : public ParseNode
{
public:
    ParseNodeUni(OpCode nop, charcount_t ichMin, charcount_t ichLim, ParseNode * pnode1);

    ParseNodePtr pnode1;

    DISABLE_SELF_CAST(ParseNodeUni);
};

// binary operators
class ParseNodeBin : public ParseNode
{
public:
    ParseNodeBin(OpCode nop, charcount_t ichMin, charcount_t ichLim, ParseNode * pnode1, ParseNode * pnode2);

    ParseNodePtr pnode1;
    ParseNodePtr pnode2;

    DISABLE_SELF_CAST(ParseNodeBin);
};

// ternary operator
class ParseNodeTri : public ParseNode
{
public:
    ParseNodeTri(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnode1;
    ParseNodePtr pnode2;
    ParseNodePtr pnode3;

    DISABLE_SELF_CAST(ParseNodeTri);
};

// integer constant
class ParseNodeInt : public ParseNode
{
public:
    ParseNodeInt(charcount_t ichMin, charcount_t ichMax, int32 lw);

    int32 lw;

    DISABLE_SELF_CAST(ParseNodeInt);
};

// bigint constant
class ParseNodeBigInt : public ParseNode
{
public:
    ParseNodeBigInt(charcount_t ichMin, charcount_t ichLim, IdentPtr pid);

    IdentPtr const pid;
    bool isNegative : 1;

    DISABLE_SELF_CAST(ParseNodeBigInt);

};

// double constant
class ParseNodeFloat : public ParseNode
{
public:
    ParseNodeFloat(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    double dbl;
    bool maybeInt : 1;

    DISABLE_SELF_CAST(ParseNodeFloat);
};

class ParseNodeRegExp : public ParseNode
{
public:
    ParseNodeRegExp(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    UnifiedRegex::RegexPattern* regexPattern;
    uint regexPatternIndex;

    DISABLE_SELF_CAST(ParseNodeRegExp);
};

// identifier or string

class ParseNodeStr : public ParseNode
{
public:
    ParseNodeStr(charcount_t ichMin, charcount_t ichLim, IdentPtr pid);

    IdentPtr const pid;

    DISABLE_SELF_CAST(ParseNodeStr);

};

class Symbol;
struct PidRefStack;
class ParseNodeName : public ParseNode
{
public:
    ParseNodeName(charcount_t ichMin, charcount_t ichLim, IdentPtr pid);

    IdentPtr const pid;
private:
    Symbol **symRef;
public:
    Symbol *sym;

    void SetSymRef(PidRefStack *ref);
    void ClearSymRef() { symRef = nullptr; }
    Symbol **GetSymRef() const { return symRef; }
    Js::PropertyId PropertyIdFromNameNode() const;

    bool IsSpecialName() { return isSpecialName; }

    DISABLE_SELF_CAST(ParseNodeName);

protected:
    void SetIsSpecialName() { isSpecialName = true; }

private:
    bool isSpecialName;         // indicates a PnPid (knopName) is a special name like 'this' or 'super'
};

// variable declaration
class ParseNodeVar : public ParseNode
{
public:
    ParseNodeVar(OpCode nop, charcount_t ichMin, charcount_t ichLim, IdentPtr name);

    ParseNodePtr pnodeNext;
    IdentPtr const pid;
    Symbol *sym;
    Symbol **symRef;
    ParseNodePtr pnodeInit;
    BOOLEAN isSwitchStmtDecl;
    BOOLEAN isBlockScopeFncDeclVar;

    DISABLE_SELF_CAST(ParseNodeVar);
};

// Array literal
class ParseNodeArrLit : public ParseNodeUni
{
public:
    ParseNodeArrLit(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    uint count;
    uint spreadCount;
    BYTE arrayOfTaggedInts:1;     // indicates that array initializer nodes are all tagged ints
    BYTE arrayOfInts:1;           // indicates that array initializer nodes are all ints
    BYTE arrayOfNumbers:1;        // indicates that array initializer nodes are all numbers
    BYTE hasMissingValues:1;

    DISABLE_SELF_CAST(ParseNodeArrLit);
};

class ParseNodeObjLit : public ParseNodeUni
{
public:
    ParseNodeObjLit(OpCode nop, charcount_t ichMin, charcount_t ichLim, uint staticCnt=0, uint computedCnt=0, bool rest=false);

    uint staticCount;
    uint computedCount;
    bool hasRest;

    DISABLE_SELF_CAST(ParseNodeObjLit);
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
    kFunctionHasComputedName                    = 1 << 11,
    kFunctionHasWithStmt                        = 1 << 12, // function (or child) uses with
    kFunctionIsLambda                           = 1 << 13,
    kFunctionChildCallsEval                     = 1 << 14,
    kFunctionHasNonSimpleParameterList          = 1 << 15,
    kFunctionHasSuperReference                  = 1 << 16,
    kFunctionIsMethod                           = 1 << 17,
    kFunctionIsClassConstructor                 = 1 << 18, // function is a class constructor
    kFunctionIsBaseClassConstructor             = 1 << 19, // function is a base class constructor
    kFunctionIsClassMember                      = 1 << 20, // function is a class member
    kFunctionHasHomeObj                         = 1 << 21, // function has home object. Class or member functions need to have home obj.
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

namespace SuperRestrictionState {
    enum State {
        Disallowed = 0,
        CallAndPropertyAllowed = 1,
        PropertyAllowed = 2
    };
}

// function declaration
class ParseNodeFnc : public ParseNode
{
public:
    ParseNodeFnc(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeNext;
    ParseNodeVar * pnodeName;
    IdentPtr pid;
    LPCOLESTR hint;
    uint32 hintLength;
    uint32 hintOffset;
    ParseNodeBlock * pnodeScopes;
    ParseNodeBlock * pnodeBodyScope;
    ParseNodePtr pnodeParams;
    ParseNodePtr pnodeVars;
    ParseNodePtr pnodeBody;
    ParseNodeVar * pnodeRest;

    FuncInfo *funcInfo; // function information gathered during byte code generation
    Scope *scope;

    uint nestedCount; // Nested function count (valid until children have been processed)
    uint nestedIndex; // Index within the parent function (Used by ByteCodeGenerator)

    FncFlags fncFlags;
    int32 astSize;
    size_t cbMin; // Min an Lim UTF8 offsets.
    size_t cbStringMin;
    size_t cbLim;
    ULONG lineNumber;   // Line number relative to the current source buffer of the function declaration.
    ULONG columnNumber; // Column number of the declaration.
    Js::LocalFunctionId functionId;
#if DBG
    Js::LocalFunctionId deferredParseNextFunctionId;
#endif
    RestorePoint *pRestorePoint;
    DeferredFunctionStub *deferredStub;
    IdentPtrSet *capturedNames;
    uint16 firstDefaultArg; // Position of the first default argument, if any
    bool isNameIdentifierRef;
    bool nestedFuncEscapes;
    bool canBeDeferred;
    bool isBodyAndParamScopeMerged; // Indicates whether the param scope and the body scope of the function can be merged together or not.
                                    // We cannot merge both scopes together if there is any closure capture or eval is present in the param scope.
    Js::RegSlot homeObjLocation;    // Stores the RegSlot from where the home object needs to be copied

    static const int32 MaxStackClosureAST = 800000;

    SuperRestrictionState::State superRestrictionState;

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
    void SetHasComputedName(bool set = true) { SetFlags(kFunctionHasComputedName, set); }
    void SetHasHomeObj(bool set = true) { SetFlags(kFunctionHasHomeObj, set); }
    void SetUsesArguments(bool set = true) { SetFlags(kFunctionUsesArguments, set); }
    void SetIsDefaultModuleExport(bool set = true) { SetFlags(kFunctionIsDefaultModuleExport, set); }
    void SetNestedFuncEscapes(bool set = true) { nestedFuncEscapes = set; }
    void SetCanBeDeferred(bool set = true) { canBeDeferred = set; }
    void ResetBodyAndParamScopeMerged() { isBodyAndParamScopeMerged = false; }
    void SetHomeObjLocation(Js::RegSlot location) { this->homeObjLocation = location; }

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
    bool HasComputedName() const { return HasFlags(kFunctionHasComputedName); }
    bool HasHomeObj() const { return HasFlags(kFunctionHasHomeObj); }
    bool UsesArguments() const { return HasFlags(kFunctionUsesArguments); }
    bool IsDefaultModuleExport() const { return HasFlags(kFunctionIsDefaultModuleExport); }
    bool NestedFuncEscapes() const { return nestedFuncEscapes; }
    bool CanBeDeferred() const { return canBeDeferred; }
    bool IsBodyAndParamScopeMerged() { return isBodyAndParamScopeMerged; }
    Js::RegSlot GetHomeObjLocation() const { return homeObjLocation; }
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
        fn(this->pnodeScopes->pnodeScopes);
        if (this->pnodeBodyScope != nullptr)
        {
            fn(this->pnodeBodyScope->pnodeScopes);
        }
    }

    IdentPtrSet* EnsureCapturedNames(ArenaAllocator* alloc);
    IdentPtrSet* GetCapturedNames();
    bool HasAnyCapturedNames();

    DISABLE_SELF_CAST(ParseNodeFnc);
};

// class declaration
class ParseNodeClass : public ParseNode
{
public:
    ParseNodeClass(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeVar * pnodeName;
    ParseNodeVar * pnodeDeclName;
    ParseNodeBlock * pnodeBlock;
    ParseNodeFnc * pnodeConstructor;
    ParseNodePtr pnodeMembers;
    ParseNodePtr pnodeStaticMembers;
    ParseNodePtr pnodeExtends;

    bool isDefaultModuleExport;

    void SetIsDefaultModuleExport(bool set) { isDefaultModuleExport = set; }
    bool IsDefaultModuleExport() const { return isDefaultModuleExport; }

    DISABLE_SELF_CAST(ParseNodeClass);
};

// export default expr
class ParseNodeExportDefault : public ParseNode
{
public:
    ParseNodeExportDefault(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeExpr;

    DISABLE_SELF_CAST(ParseNodeExportDefault);
};

// string template declaration
class ParseNodeStrTemplate : public ParseNode
{
public:
    ParseNodeStrTemplate(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeStringLiterals;
    ParseNodePtr pnodeStringRawLiterals;
    ParseNodePtr pnodeSubstitutionExpressions;
    uint16 countStringLiterals;
    BYTE isTaggedTemplate:1;

    DISABLE_SELF_CAST(ParseNodeStrTemplate);
};

// global program
class ParseNodeProg : public ParseNodeFnc
{
public:
    ParseNodeProg(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeLastValStmt;  // Used by ByteCodeGenerator
    bool m_UsesArgumentsAtGlobal;

    DISABLE_SELF_CAST(ParseNodeProg);
};

// global module
class ParseNodeModule : public ParseNodeProg
{
public:
    ParseNodeModule(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ModuleImportOrExportEntryList* localExportEntries;
    ModuleImportOrExportEntryList* indirectExportEntries;
    ModuleImportOrExportEntryList* starExportEntries;
    ModuleImportOrExportEntryList* importEntries;
    IdentPtrList* requestedModules;

    DISABLE_SELF_CAST(ParseNodeModule);
};

// function call
class ParseNodeCall : public ParseNode
{
public:
    ParseNodeCall(OpCode nop, charcount_t ichMin, charcount_t ichLim, ParseNodePtr pnodeTarget, ParseNodePtr pnodeArgs);

    ParseNodePtr pnodeTarget;
    ParseNodePtr pnodeArgs;
    uint16 argCount;
    uint16 spreadArgCount;
    BYTE callOfConstants : 1;
    BYTE isApplyCall : 1;
    BYTE isEvalCall : 1;
    BYTE isSuperCall : 1;
    BYTE hasDestructuring : 1;

    DISABLE_SELF_CAST(ParseNodeCall);
};

// base for statement nodes
class ParseNodeStmt : public ParseNode
{
public:
    ParseNodeStmt(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeStmt * pnodeOuter;

    // Set by parsing code, used by code gen.
    uint grfnop;

    // Needed for byte code gen.
    Js::ByteCodeLabel breakLabel;
    Js::ByteCodeLabel continueLabel;

    bool emitLabels;

    DISABLE_SELF_CAST(ParseNodeStmt);
};

// block { }
class ParseNodeBlock : public ParseNodeStmt
{
public:
    ParseNodeBlock(charcount_t ichMin, charcount_t ichLim, int blockId, PnodeBlockType blockType);

    ParseNodePtr pnodeStmt;
    ParseNodePtr pnodeLastValStmt;
    ParseNodePtr pnodeLexVars;
    ParseNodePtr pnodeScopes;
    ParseNodePtr pnodeNext;
    Scope        *scope;

    ParseNodeBlock * enclosingBlock;
    int blockId;
    PnodeBlockType blockType:2;
    BYTE         callsEval:1;
    BYTE         childCallsEval:1;

    void SetCallsEval(bool does) { callsEval = does; }
    bool GetCallsEval() const { return callsEval; }

    void SetChildCallsEval(bool does) { childCallsEval = does; }
    bool GetChildCallsEval() const { return childCallsEval; }

    void SetEnclosingBlock(ParseNodeBlock * pnode) { enclosingBlock = pnode; }
    ParseNodeBlock * GetEnclosingBlock() const { return enclosingBlock; }

    bool HasBlockScopedContent() const;

    DISABLE_SELF_CAST(ParseNodeBlock);
};

// break and continue
class ParseNodeJump : public ParseNodeStmt
{
public:
    ParseNodeJump(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeStmt * pnodeTarget;
    bool hasExplicitTarget;

    DISABLE_SELF_CAST(ParseNodeJump);
};

// base for loop nodes
class ParseNodeLoop : public ParseNodeStmt
{
public:
    ParseNodeLoop(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    // Needed for byte code gen
    uint loopId;


    DISABLE_SELF_CAST(ParseNodeLoop);
};

// while and do-while loops
class ParseNodeWhile : public ParseNodeLoop
{
public:
    ParseNodeWhile(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeBody;

    DISABLE_SELF_CAST(ParseNodeWhile);
};

// with
class ParseNodeWith : public ParseNodeStmt
{
public:
    ParseNodeWith(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeObj;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeScopes;
    ParseNodePtr pnodeNext;
    Scope        *scope;

    DISABLE_SELF_CAST(ParseNodeWith);
};

// Destructure pattern for function/catch parameter
class ParseNodeParamPattern : public ParseNodeUni
{
public:
    ParseNodeParamPattern(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeNext;
    Js::RegSlot location;
    DISABLE_SELF_CAST(ParseNodeParamPattern);
};

// if
class ParseNodeIf : public ParseNodeStmt
{
public:
    ParseNodeIf(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeTrue;
    ParseNodePtr pnodeFalse;

    DISABLE_SELF_CAST(ParseNodeIf);
};

// for-in loop
class ParseNodeForInOrForOf : public ParseNodeLoop
{
public:
    ParseNodeForInOrForOf(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeObj;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeLval;
    ParseNodeBlock * pnodeBlock;
    Js::RegSlot itemLocation;

    DISABLE_SELF_CAST(ParseNodeForInOrForOf);
};

// for loop
class ParseNodeFor : public ParseNodeLoop
{
public:
    ParseNodeFor(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeCond;
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeInit;
    ParseNodePtr pnodeIncr;
    ParseNodeBlock * pnodeBlock;
    ParseNodeFor * pnodeInverted;

    DISABLE_SELF_CAST(ParseNodeFor);
};

// switch
class ParseNodeSwitch : public ParseNodeStmt
{
public:
    ParseNodeSwitch(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeVal;
    ParseNodeCase * pnodeCases;
    ParseNodeCase * pnodeDefault;
    ParseNodeBlock * pnodeBlock;

    DISABLE_SELF_CAST(ParseNodeSwitch);
};

// switch case
class ParseNodeCase : public ParseNodeStmt
{
public:
    ParseNodeCase(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeCase * pnodeNext;
    ParseNodePtr pnodeExpr; // nullptr for default
    ParseNodeBlock * pnodeBody;
    Js::ByteCodeLabel labelCase;


    DISABLE_SELF_CAST(ParseNodeCase);
};

// return [expr]
class ParseNodeReturn : public ParseNodeStmt
{
public:
    ParseNodeReturn(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeExpr;

    DISABLE_SELF_CAST(ParseNodeReturn);
};

// try-catch-finally
class ParseNodeTryFinally : public ParseNodeStmt
{
public:
    ParseNodeTryFinally(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeTry * pnodeTry;
    ParseNodeFinally * pnodeFinally;

    DISABLE_SELF_CAST(ParseNodeTryFinally);
};

// try-catch
class ParseNodeTryCatch : public ParseNodeStmt
{
public:
    ParseNodeTryCatch(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodeTry * pnodeTry;
    ParseNodeCatch * pnodeCatch;

    DISABLE_SELF_CAST(ParseNodeTryCatch);
};

// try-catch
class ParseNodeTry : public ParseNodeStmt
{
public:
    ParseNodeTry(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeBody;

    DISABLE_SELF_CAST(ParseNodeTry);
};

// { catch(e : expr) {body} }
class ParseNodeCatch : public ParseNodeStmt
{
public:
    ParseNodeCatch(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    bool HasPatternParam() { return pnodeParam != nullptr && pnodeParam->nop == knopParamPattern; }
    bool HasParam() { return pnodeParam != nullptr;  }

    ParseNodePtr GetParam() { return pnodeParam; }
    void SetParam(ParseNodeName * pnode) { pnodeParam = pnode;  }
    void SetParam(ParseNodeParamPattern * pnode) { pnodeParam = pnode; }    

    ParseNodePtr pnodeNext;

private:
    ParseNode *  pnodeParam;   // Name or ParamPattern
public:
    ParseNodePtr pnodeBody;
    ParseNodePtr pnodeScopes;
    Scope        *scope;

    DISABLE_SELF_CAST(ParseNodeCatch);
};

// finally
class ParseNodeFinally : public ParseNodeStmt
{
public:
    ParseNodeFinally(OpCode nop, charcount_t ichMin, charcount_t ichLim);

    ParseNodePtr pnodeBody;

    DISABLE_SELF_CAST(ParseNodeFinally);
};

// special name like 'this'
class ParseNodeSpecialName : public ParseNodeName
{
public:
    ParseNodeSpecialName(charcount_t ichMin, charcount_t ichLim, IdentPtr pid);

    bool isThis : 1;
    bool isSuper : 1;

    DISABLE_SELF_CAST(ParseNodeSpecialName);
};

// binary operator with super reference
class ParseNodeSuperReference : public ParseNodeBin
{
public:
    ParseNodeSuperReference(OpCode nop, charcount_t ichMin, charcount_t ichLim, ParseNode * pnode1, ParseNode * pnode2);

    ParseNodeSpecialName * pnodeThis;

    DISABLE_SELF_CAST(ParseNodeSuperReference);
};

// call node with super target
class ParseNodeSuperCall : public ParseNodeCall
{
public:
    ParseNodeSuperCall(OpCode nop, charcount_t ichMin, charcount_t ichLim, ParseNode * pnodeTarget, ParseNode * pnodeArgs);

    ParseNodeSpecialName * pnodeThis;
    ParseNodeSpecialName * pnodeNewTarget;

    DISABLE_SELF_CAST(ParseNodeSuperCall);
};

typedef ParseNode ParseNodeNone;
template <OpCode nop> class OpCodeTrait;
#define PTNODE(nop,sn,pc,nk,ok,json) \
    template<> \
    class OpCodeTrait<nop>  \
    { \
    public:  \
        typedef ParseNode##nk ParseNodeType; \
        static const bool AllowDefer = ((ok) & fnopAllowDefer) != 0; \
    };
#include "ptlist.h"
