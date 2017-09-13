//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once


class Lifetime;

namespace IR
{
class Instr;
class Opnd;
}

class StackSym;
class PropertySym;

typedef JsUtil::BaseDictionary<StackSym*, StackSym*, JitArenaAllocator, PrimeSizePolicy> StackSymMap;
typedef JsUtil::BaseDictionary<StackSym*, IR::Instr*, JitArenaAllocator, PrimeSizePolicy> SymInstrMap;

enum SymKind : BYTE
{
    SymKindInvalid,
    SymKindStack,
    SymKindProperty
};

typedef uint32 SymID;


///---------------------------------------------------------------------------
///
/// class Sym
///     StackSym
///     PropertySym
///
///---------------------------------------------------------------------------

class Sym
{
public:

    bool        IsStackSym() const;
    StackSym *  AsStackSym();
    StackSym const *  AsStackSym() const;
    bool        IsPropertySym() const;
    PropertySym *  AsPropertySym();
    PropertySym const *  AsPropertySym() const;

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    void Dump(IRDumpFlags flags, const ValueType valueType = ValueType::Uninitialized) const;
    void Dump(const ValueType valueType = ValueType::Uninitialized) const;
    void DumpSimple() const;
#endif
#if DBG_DUMP
    // Having a VTable helps the VS debugger determine which derived class we have
    void virtual DummyVirtualFunc() {};
#endif

public:
    Sym *       m_next;
    SymID       m_id;
    SymKind     m_kind;
};

class ObjectSymInfo
{
public:
    static ObjectSymInfo * New(Func * func);
    static ObjectSymInfo * New(StackSym * typeSym, Func * func);

    ObjectSymInfo(): m_typeSym(nullptr), m_propertySymList(nullptr) {};

public:
    StackSym *      m_typeSym;
    PropertySym *   m_propertySymList;
};

///---------------------------------------------------------------------------
///
/// class StackSym
///
///     Used for stack symbols.  These are the register candidates.
///
///---------------------------------------------------------------------------

class StackSym: public Sym
{
private:
    static StackSym * New(SymID id, IRType type, Js::RegSlot byteCodeRegSlot, Func *func);
public:
    static StackSym * NewArgSlotSym(Js::ArgSlot argSlotNum, Func * func, IRType = TyVar);
    static StackSym * NewArgSlotRegSym(Js::ArgSlot argSlotNum, Func * func, IRType = TyVar);
    static StackSym * NewParamSlotSym(Js::ArgSlot argSlotNum, Func * func);
    static StackSym * NewParamSlotSym(Js::ArgSlot argSlotNum, Func * func, IRType type);
    static StackSym *NewImplicitParamSym(Js::ArgSlot paramSlotNum, Func * func);
    static StackSym * New(IRType type, Func *func);
    static StackSym * New(Func *func);
    static StackSym * FindOrCreate(SymID id, Js::RegSlot byteCodeRegSlot, Func *func, IRType type = TyVar);

    IRType          GetType() const { return this->m_type; }
    bool            IsArgSlotSym() const;
    bool            IsParamSlotSym() const;
    bool            IsImplicitParamSym() const { return this->m_isImplicitParamSym; }
    bool            IsAllocated() const;
    int32           GetIntConstValue() const;
    Js::Var         GetFloatConstValueAsVar_PostGlobOpt() const;
    void *          GetConstAddress(bool useLocal = false) const;
    StackSym *      CloneDef(Func *func);
    StackSym *      CloneUse(Func *func);
    void            CopySymAttrs(StackSym *symSrc);

    bool            IsSingleDef() const { return this->m_isSingleDef && this->m_instrDef; }
    bool            IsConst() const;
    bool            IsIntConst() const;
    bool            IsInt64Const() const;
    bool            IsTaggableIntConst() const;
    bool            IsFloatConst() const;
// SIMD_JS
    bool            IsSimd128Const() const;

    void            SetIsConst();
    void            SetIsIntConst(IntConstType value);
    void            SetIsInt64Const();
    void            SetIsFloatConst();
// SIMD_JS
    void            SetIsSimd128Const();

    intptr_t        GetLiteralConstValue_PostGlobOpt() const;
    IR::Opnd *      GetConstOpnd() const;
    BailoutConstantValue GetConstValueForBailout() const;

    // SIMD_JS
    StackSym *      GetSimd128EquivSym(IRType type, Func *func);
    StackSym *      GetSimd128F4EquivSym(Func *func);
    StackSym *      GetSimd128I4EquivSym(Func *func);
    StackSym *      GetSimd128I16EquivSym(Func *func);
    StackSym *      GetSimd128D2EquivSym(Func *func);
    bool            IsSimd128() const { return IRType_IsSimd128(this->GetType()); }

    bool            IsSimd128F4()  const { return this->GetType() == TySimd128F4;  }
    bool            IsSimd128I4()  const { return this->GetType() == TySimd128I4;  }
    bool            IsSimd128I8()  const { return this->GetType() == TySimd128I8;  }
    bool            IsSimd128I16() const { return this->GetType() == TySimd128I16; }
    bool            IsSimd128U4()  const { return this->GetType() == TySimd128U4;  }
    bool            IsSimd128U8()  const { return this->GetType() == TySimd128U8;  }
    bool            IsSimd128U16() const { return this->GetType() == TySimd128U16; }
    bool            IsSimd128B4()  const { return this->GetType() == TySimd128B4;  }
    bool            IsSimd128B8()  const { return this->GetType() == TySimd128B8;  }
    bool            IsSimd128B16() const { return this->GetType() == TySimd128B16; }
    bool            IsSimd128D2()  const { return this->GetType() == TySimd128D2;  }

    StackSym *      GetFloat64EquivSym(Func *func);
    bool            IsFloat64() const { return this->GetType() == TyFloat64; }
    bool            IsFloat32() const { return this->GetType() == TyFloat32; }
    StackSym *      GetInt32EquivSym(Func *func);
    bool            IsInt32() const { return this->GetType() == TyInt32; }
    bool            IsUInt32() const { return this->GetType() == TyUint32; }
    bool            IsInt64() const { return this->GetType() == TyInt64; }
    bool            IsUint64() const { return this->GetType() == TyUint64; }

    StackSym *      GetVarEquivSym(Func *func);
    StackSym *      GetVarEquivSym_NoCreate();
    StackSym const * GetVarEquivSym_NoCreate() const;
    bool            IsVar() const { return this->GetType() == TyVar; }
    bool            IsTypeSpec() const { return this->m_isTypeSpec; }
    static StackSym *GetVarEquivStackSym_NoCreate(Sym * sym);
    static StackSym const *GetVarEquivStackSym_NoCreate(Sym const * const sym);

    bool            HasByteCodeRegSlot() const { return m_hasByteCodeRegSlot; }
    Func *          GetByteCodeFunc() const;
    Js::RegSlot     GetByteCodeRegSlot() const;
    bool            IsTempReg(Func *const func) const;
    bool            IsFromByteCodeConstantTable() const { return m_isFromByteCodeConstantTable; }
    void            SetIsFromByteCodeConstantTable() { this->m_isFromByteCodeConstantTable = true; }
    Js::ArgSlot     GetArgSlotNum() const { Assert(HasArgSlotNum()); return m_slotNum; }
    bool            HasArgSlotNum() const { return !!(m_isArgSlotSym | m_isArgSlotRegSym); }
    void            IncrementArgSlotNum();
    void            DecrementArgSlotNum();
    Js::ArgSlot     GetParamSlotNum() const { Assert(IsParamSlotSym()); return m_slotNum; }

    IR::Instr *     GetInstrDef() const { return this->IsSingleDef() ? this->m_instrDef : nullptr; }

    bool            HasObjectInfo() const { Assert(!IsTypeSpec()); return this->m_objectInfo != nullptr; };
    ObjectSymInfo * GetObjectInfo() const { Assert(this->m_objectInfo); return this->m_objectInfo; }
    void            SetObjectInfo(ObjectSymInfo * value) { this->m_objectInfo = value; }
    ObjectSymInfo * EnsureObjectInfo(Func * func);

    bool            HasObjectTypeSym() const { return HasObjectInfo() && GetObjectInfo()->m_typeSym; }
    StackSym *      GetObjectTypeSym() const { Assert(HasObjectTypeSym()); return GetObjectInfo()->m_typeSym; }
    int             GetSymSize(){ return TySize[m_type]; }
    void            FixupStackOffset(Func * currentFunc);

private:
    StackSym *      GetTypeEquivSym(IRType type, Func *func);
    StackSym *      GetTypeEquivSym_NoCreate(IRType type);
    StackSym const * GetTypeEquivSym_NoCreate(IRType type) const;
#if DBG
    void            VerifyConstFlags() const;
#endif
private:
    Js::ArgSlot     m_slotNum;
public:
    uint8           m_isSingleDef:1;            // the symbol only has a single definition in the IR
    uint8           m_isNotInt:1;
    uint8           m_isSafeThis : 1;
    uint8           m_isConst : 1;              // single def and it is a constant
    uint8           m_isIntConst : 1;           // a constant and it's value is an Int32
    uint8           m_isTaggableIntConst : 1;   // a constant and it's value is taggable (Int31 in 32-bit, Int32 in x64)
    uint8           m_isEncodedConstant : 1;    // the constant has
    uint8           m_isInt64Const: 1;
    uint8           m_isFltConst: 1;
    uint8           m_isSimd128Const : 1;
    uint8           m_isStrConst:1;
    uint8           m_isStrEmpty:1;
    uint8           m_allocated:1;
    uint8           m_hasByteCodeRegSlot:1;
    uint8           m_isInlinedArgSlot:1;
    uint8           m_isOrphanedArg :1;
    uint8           m_isTypeSpec:1;
    uint8           m_requiresBailOnNotNumber:1;
    uint8           m_isFromByteCodeConstantTable:1;
    uint8           m_mayNotBeTempLastUse:1;
    uint8           m_isArgSlotSym: 1;        // When set this implies an argument stack slot with no lifetime for register allocation
    uint8           m_isArgSlotRegSym : 1;
    uint8           m_isParamSym : 1;
    uint8           m_isImplicitParamSym : 1;
    uint8           m_isBailOutReferenced: 1;        // argument sym referenced by bailout
    uint8           m_isArgCaptured: 1;       // True if there is a ByteCodeArgOutCapture for this symbol
    uint8           m_nonEscapingArgObjAlias : 1;
    uint8           m_isCatchObjectSym : 1;   // a catch object sym (used while jitting loop bodies)
    uint            m_isClosureSym : 1;
    IRType          m_type;
    Js::BuiltinFunction m_builtInIndex;


    int32           m_offset;
#ifdef  _M_AMD64
    // Only for AsmJs on x64. Argument position for ArgSyms.
    int32           m_argPosition;
#endif
    union
    {
        IR::Instr *     m_instrDef;             // m_isSingleDef
        size_t          constantValue;          // !m_isSingleDef && m_isEncodedConstant
    };
    StackSym *      m_tempNumberSym;
    StackSym *      m_equivNext;
    ObjectSymInfo * m_objectInfo;

    union {
        struct GlobOpt
        {
            int numCompoundedAddSubUses;
        } globOpt;

        struct LinearScan
        {
            Lifetime *      lifetime;
        } linearScan;

        struct Peeps
        {
            RegNum          reg;
        } peeps;
    }scratch;

    static const Js::ArgSlot InvalidSlot;
};

class ByteCodeStackSym : public StackSym
{
private:
    ByteCodeStackSym(Js::RegSlot byteCodeRegSlot, Func * byteCodeFunc)
        : byteCodeRegSlot(byteCodeRegSlot), byteCodeFunc(byteCodeFunc) {}

    Js::RegSlot     byteCodeRegSlot;
    Func *          byteCodeFunc;

    friend class StackSym;
};

///---------------------------------------------------------------------------
///
/// class PropertySym
///
///     Used for field symbols. Property syms for a given objects are linked
///     together.
///
///---------------------------------------------------------------------------

enum PropertyKind : BYTE
{
    PropertyKindData,
    PropertyKindSlots,
    PropertyKindLocalSlots,
    PropertyKindSlotArray,
    PropertyKindWriteGuard
};

class PropertySym: public Sym
{
    friend class Sym;
public:
    static PropertySym * New(SymID stackSymID, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func);
    static PropertySym * New(StackSym *stackSym, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func);
    static PropertySym * Find(SymID stackSymID, int32 propertyId, Func *func);
    static PropertySym * FindOrCreate(SymID stackSymID, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func);

    Func *GetFunc() { return m_func; }
    bool HasPropertyIdIndex() { return m_propertyIdIndex != -1; }
    int32 GetPropertyIdIndex() { return m_propertyIdIndex; }
    bool HasInlineCacheIndex() { return m_inlineCacheIndex != -1; }
    int32 GetInlineCacheIndex() { return m_inlineCacheIndex; }
    bool HasObjectTypeSym() const { return this->m_stackSym->HasObjectTypeSym(); }
    bool HasWriteGuardSym() const { return this->m_writeGuardSym != nullptr; }
    StackSym * GetObjectTypeSym() const { return this->m_stackSym->GetObjectTypeSym(); }

public:
    PropertyKind    m_fieldKind;
    int32           m_propertyId;
    StackSym *      m_stackSym;
    BVSparse<JitArenaAllocator> *m_propertyEquivSet;  // Bit vector of all propertySyms with same propertyId
    PropertySym *   m_nextInStackSymList;
    PropertySym *   m_writeGuardSym;
    Func *          m_func;
    Func *          m_loadInlineCacheFunc;
    uint            m_loadInlineCacheIndex;
private:
    uint32          m_propertyIdIndex;
    uint            m_inlineCacheIndex;
};

