//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

namespace IR
{

///----------------------------------------------------------------------------
///
/// Opnd::UseWithNewType
///
///     Creates a Use (a copy if already in use or returns the same)
/// and sets it type
///
///----------------------------------------------------------------------------


Opnd*
Opnd::UseWithNewType(IRType type, Func * func)
{
    Opnd * res = this->Use(func);
    res->SetType(type);
    StackSym* sym = res->GetStackSym();
    if (sym)
    {
        if (TySize[sym->GetType()] < TySize[type])
        {
            Assert(!sym->IsAllocated());
            sym->m_type = type;
        }
    }
    return res;
}

bool
Opnd::IsTaggedInt() const
{
    return GetValueType().IsTaggedInt();
}

bool
Opnd::IsTaggedValue() const
{
    CompileAssert(!FLOATVAR || INT32VAR);
#if FLOATVAR
    return GetValueType().IsNumber();
#else
    return IsTaggedInt();
#endif
}

bool
Opnd::IsNotNumber() const
{
    if (this->GetValueType().IsNotNumber())
    {
        return true;
    }
    if (this->IsRegOpnd())
    {
        const IR::RegOpnd* regOpnd = this->AsRegOpnd();

        if (regOpnd->m_sym == nullptr)
        {
            return true;
        }

        if (regOpnd->m_sym->m_isNotInt)
        {
            // m_isNotInt actually means "is not number". It should not be set to true for definitely-float values.
            return true;
        }
    }
    return false;
}

bool
Opnd::IsNotInt() const
{
    return IsNotNumber() || IsFloat();
}

bool
Opnd::IsNotTaggedValue() const
{
    if (!PHASE_OFF1(Js::OptTagChecksPhase) && this->GetValueType().IsNotTaggedValue())
    {
        return true;
    }
    return this->IsNotNumber();
}

bool
Opnd::IsWriteBarrierTriggerableValue()
{
    // Determines whether if an operand is used as a source in a store instruction, whether the store needs a write barrier

    // If it's a tagged value, we don't need a write barrier
    if (this->IsTaggedValue())
    {
        return false;
    }

    // If this operand is known address, then it doesn't need a write barrier, the address is either not a GC address or is pinned
    if (this->IsAddrOpnd() && static_cast<AddrOpndKind>(this->AsAddrOpnd()->GetKind()) == AddrOpndKindDynamicVar)
    {
        return false;
    }

    if (TySize[this->GetType()] != sizeof(void*))
    {
        return false;
    }

#if DBG
    if (CONFIG_FLAG(ForceSoftwareWriteBarrier) && CONFIG_FLAG(VerifyBarrierBit))
    {
        return true; // No further optimization if we are in verification
    }
#endif

    // If its null/boolean/undefined, we don't need a write barrier since the javascript library will keep those guys alive
    return !(this->GetValueType().IsBoolean() || this->GetValueType().IsNull() || this->GetValueType().IsUndefined());
}

/*
* This is a devirtualized functions See the note above Opnd:Copy()
*/

OpndKind Opnd::GetKind() const
{
    return this->m_kind;
}

/*
* This is a devirtualized functions See the note above Opnd:Copy()
*/

Opnd *
Opnd::CloneDef(Func *func)
{
    switch (this->m_kind)
    {
    case OpndKindSym:
        if ((*static_cast<SymOpnd*>(this)).IsPropertySymOpnd())
        {
            return static_cast<PropertySymOpnd*>(this)->CloneDefInternalSub(func);
        }
        return static_cast<SymOpnd*>(this)->CloneDefInternal(func);

    case OpndKindReg:
        if ((*static_cast<RegOpnd*>(this)).IsArrayRegOpnd())
        {
            return static_cast<ArrayRegOpnd*>(this)->CloneDefInternalSub(func);
        }
        return static_cast<RegOpnd*>(this)->CloneDefInternal(func);

    case OpndKindIndir:
        return static_cast<IndirOpnd*>(this)->CloneDefInternal(func);

    default:
        return this->Copy(func);
    };
}

/*
* This is a devirtualized functions See the note above Opnd:Copy()
*/

Opnd *
Opnd::CloneUse(Func *func)
{
    switch (this->m_kind)
    {
    case OpndKindSym:
        if ((*static_cast<SymOpnd*>(this)).IsPropertySymOpnd())
        {
            return static_cast<PropertySymOpnd*>(this)->CloneUseInternalSub(func);
        }
        return static_cast<SymOpnd*>(this)->CloneUseInternal(func);

    case OpndKindReg:
        if ((*static_cast<RegOpnd*>(this)).IsArrayRegOpnd())
        {
            return static_cast<ArrayRegOpnd*>(this)->CloneUseInternalSub(func);
        }
        return static_cast<RegOpnd*>(this)->CloneUseInternal(func);

    case OpndKindIndir:
        return static_cast<IndirOpnd*>(this)->CloneUseInternal(func);

    default:
        return this->Copy(func);
    };
}

/*
* This is a devirtualized functions See the note above Opnd:Copy()
*/

void Opnd::Free(Func *func)
{
    AssertMsg(!IsInUse(), "Attempting to free in use operand.");

    switch (this->m_kind)
    {
    case OpndKindIntConst:
        //NOTE: use to be Sealed do not do sub class checks like in CloneUse
        static_cast<IntConstOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindInt64Const:
        return static_cast<Int64ConstOpnd*>(this)->FreeInternal(func);

    case OpndKindSimd128Const:
        static_cast<Simd128ConstOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindFloatConst:
        static_cast<FloatConstOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindFloat32Const:
        static_cast<Float32ConstOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindHelperCall:
        static_cast<HelperCallOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindSym:
        static_cast<SymOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindReg:
        if ((*static_cast<RegOpnd*>(this)).IsArrayRegOpnd())
        {
            static_cast<ArrayRegOpnd*>(this)->FreeInternalSub(func);
            break;
        }
        static_cast<RegOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindAddr:
        static_cast<AddrOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindIndir:
        static_cast<IndirOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindMemRef:
        static_cast<MemRefOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindLabel:
        static_cast<LabelOpnd*>(this)->FreeInternal(func);
        break;

    case OpndKindRegBV:
        static_cast<RegBVOpnd*>(this)->FreeInternal(func);
        break;

    default:
        Assert(UNREACHED);
        __assume(UNREACHED);

    };
#if DBG
    if (func->m_alloc->HasDelayFreeList())
    {
        this->isDeleted = true;
    }
#endif
}

/*
* This is a devirtualized functions See the note above Opnd:Copy()
*/

bool Opnd::IsEqual(Opnd *opnd)
{
    switch (this->m_kind)
    {
    case OpndKindIntConst:
        return static_cast<IntConstOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindInt64Const:
        return static_cast<Int64ConstOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindFloatConst:
        return static_cast<FloatConstOpnd*>(this)->IsEqualInternal(opnd);
    case OpndKindFloat32Const:
        return static_cast<Float32ConstOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindHelperCall:
        if ((*static_cast<HelperCallOpnd*>(this)).IsDiagHelperCallOpnd())
        {
            return static_cast<DiagHelperCallOpnd*>(this)->IsEqualInternalSub(opnd);
        }
        return static_cast<HelperCallOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindSym:
        //NOTE: use to be Sealed do not do sub class checks like in CloneUse
        return static_cast<SymOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindReg:
        //NOTE: not sealed but ArrayRegOpnd::isEqual function does not exist, default to RegOpnd only
        return static_cast<RegOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindAddr:
        return static_cast<AddrOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindIndir:
        return static_cast<IndirOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindMemRef:
        return static_cast<MemRefOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindLabel:
        return static_cast<LabelOpnd*>(this)->IsEqualInternal(opnd);

    case OpndKindRegBV:
        return static_cast<RegBVOpnd*>(this)->IsEqualInternal(opnd);
    default:
        Assert(UNREACHED);
        __assume(UNREACHED);
    };
}
/*
* This is a devirtualized functions If you inherit from any of the child classes of Opnd
* And would like to override the default method behavior you must add an
* Is<your new Opnd Type>Opnd() call and check for it  like in examples
* HelperCallOpnd, PropertySymOpnd, & RegOpnd
*/

Opnd * Opnd::Copy(Func *func)
{
    switch (this->m_kind)
    {
    case OpndKindIntConst:
        return static_cast<IntConstOpnd*>(this)->CopyInternal(func);

    case OpndKindInt64Const:
        return static_cast<Int64ConstOpnd*>(this)->CopyInternal(func);

    case OpndKindFloatConst:
        return static_cast<FloatConstOpnd*>(this)->CopyInternal(func);

    case OpndKindHelperCall:
        if ((*static_cast<HelperCallOpnd*>(this)).IsDiagHelperCallOpnd())
        {
            return static_cast<DiagHelperCallOpnd*>(this)->CopyInternalSub(func);
        }
        return static_cast<HelperCallOpnd*>(this)->CopyInternal(func);

    case OpndKindSym:
        if ((*static_cast<SymOpnd*>(this)).IsPropertySymOpnd())
        {
            return static_cast<PropertySymOpnd*>(this)->CopyInternalSub(func);
        }
        return static_cast<SymOpnd*>(this)->CopyInternal(func);

    case OpndKindReg:
        if ((*static_cast<RegOpnd*>(this)).IsArrayRegOpnd())
        {
            return static_cast<ArrayRegOpnd*>(this)->CopyInternalSub(func);
        }
        return static_cast<RegOpnd*>(this)->CopyInternal(func);

    case OpndKindAddr:
        return static_cast<AddrOpnd*>(this)->CopyInternal(func);

    case OpndKindIndir:
        return static_cast<IndirOpnd*>(this)->CopyInternal(func);

    case OpndKindMemRef:
        return static_cast<MemRefOpnd*>(this)->CopyInternal(func);

    case OpndKindLabel:
        return static_cast<LabelOpnd*>(this)->CopyInternal(func);

    case OpndKindRegBV:
        return static_cast<RegBVOpnd*>(this)->CopyInternal(func);
    default:
        Assert(UNREACHED);
        __assume(UNREACHED);

    };
}

StackSym *
Opnd::GetStackSym() const
{
    switch (this->GetKind())
    {
    case OpndKindSym:
        return static_cast<SymOpnd const *>(this)->GetStackSymInternal();
    case OpndKindReg:
        return static_cast<RegOpnd const *>(this)->GetStackSymInternal();
    default:
        return nullptr;
    }
}

Sym*
Opnd::GetSym() const
{
    switch (this->GetKind())
    {
        case OpndKindSym:
            return static_cast<SymOpnd const *>(this)->m_sym;
        case OpndKindReg:
            return static_cast<RegOpnd const *>(this)->m_sym;
        default:
            return nullptr;
    }
}

int64
Opnd::GetImmediateValue(Func* func)
{
    switch (this->GetKind())
    {
    case OpndKindIntConst:
        return this->AsIntConstOpnd()->GetValue();

    case OpndKindInt64Const:
        return this->AsInt64ConstOpnd()->GetValue();

    case OpndKindAddr:
        return (intptr_t)this->AsAddrOpnd()->m_address;

    case OpndKindHelperCall:
        return (intptr_t)IR::GetMethodAddress(func->GetThreadContextInfo(), this->AsHelperCallOpnd());

    default:
        AssertMsg(UNREACHED, "Unexpected immediate opnd kind");
        return 0;
    }
}

#if TARGET_32 && !defined(_M_IX86)
int32
Opnd::GetImmediateValueAsInt32(Func * func)
{
    Assert(!IRType_IsInt64(this->GetType()));
    Assert(this->GetKind() != OpndKindInt64Const);
    return (int32)this->GetImmediateValue(func);
}
#endif

BailoutConstantValue Opnd::GetConstValue()
{
    BailoutConstantValue value;
    if (this->IsIntConstOpnd())
    {
        value.InitIntConstValue(this->AsIntConstOpnd()->GetValue(), this->m_type);
    }
    else if (this->IsFloatConstOpnd())
    {
        value.InitFloatConstValue(this->AsFloatConstOpnd()->m_value);
    }
    else
    {
        AssertMsg(this->IsAddrOpnd(), "Unexpected const sym");
        value.InitVarConstValue(this->AsAddrOpnd()->m_address);
    }
    return value;
}

void Opnd::SetValueType(const ValueType valueType)
{
    if(m_isValueTypeFixed)
    {
        return;
    }

    // ArrayRegOpnd has information specific to the array type, so make sure that doesn't change
    Assert(
        !IsRegOpnd() ||
        !AsRegOpnd()->IsArrayRegOpnd() ||
        valueType.IsObject() && valueType.GetObjectType() == m_valueType.GetObjectType());

    m_valueType = valueType;
}

bool Opnd::IsScopeObjOpnd(Func * func)
{
    if (IsRegOpnd())
    {
        return this->GetStackSym() == func->GetScopeObjSym();
    }
    else if(IsSymOpnd() && AsSymOpnd()->m_sym->IsPropertySym())
    {
        return this->AsSymOpnd()->m_sym->AsPropertySym()->m_stackSym == func->GetScopeObjSym();
    }
    return false;
}

ValueType Opnd::FindProfiledValueType()
{
    if (!this->GetValueType().IsUninitialized())
    {
        return this->GetValueType();
    }

    // could be expanded to cover additional opnd kinds as well.
    if (this->IsRegOpnd() && this->AsRegOpnd()->m_sym->IsSingleDef())
    {
        IR::Instr * defInstr = this->AsRegOpnd()->m_sym->GetInstrDef();
        IR::Opnd * src1 = defInstr->GetSrc1();
        while(defInstr->m_opcode == Js::OpCode::Ld_A)
        {
            if (!src1->IsRegOpnd() || !src1->AsRegOpnd()->m_sym->IsSingleDef())
            {
                return ValueType::Uninitialized;
            }
            defInstr = src1->AsRegOpnd()->m_sym->GetInstrDef();
            src1 = defInstr->GetSrc1();
        }

        if (defInstr->GetDst()->GetValueType().IsAnyArray())
        {
            return defInstr->GetDst()->GetValueType().ToLikely();
        }
        else
        {
            return defInstr->GetDst()->GetValueType();
        }
    }
    return ValueType::Uninitialized;
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
void Opnd::DumpValueType()
{
    if(m_valueType.IsUninitialized())
    {
        return;
    }

    if(!CONFIG_FLAG(Verbose))
    {
        // Skip printing the value type when it's obvious since verbose mode is off
        switch(this->GetKind())
        {
        case OpndKindIntConst:
        case OpndKindInt64Const:
        case OpndKindFloatConst:
            return;

        case OpndKindReg:
            {
                StackSym *const sym = this->AsRegOpnd()->m_sym;
                if(sym && (
                    sym->IsInt32() ||
                    sym->IsFloat32() ||
                    sym->IsFloat64() ||
                    sym->IsInt64() ||
                    sym->IsUint64()
                    ))
                {
                    return;
                }
                break;
            }

        case OpndKindAddr:
            if(this->AsAddrOpnd()->m_address && this->AsAddrOpnd()->IsVar())
            {
                IR::AddrOpnd *addrOpnd = this->AsAddrOpnd();
                Js::Var address = addrOpnd->decodedValue ? addrOpnd->decodedValue : addrOpnd->m_address;

                // Tagged int might be encoded here, so check the type
                if (addrOpnd->GetAddrOpndKind() == AddrOpndKindConstantVar
                    || Js::TaggedInt::Is(address) || (
#if !FLOATVAR
                    !JITManager::GetJITManager()->IsOOPJITEnabled() &&
#endif
                    Js::JavascriptNumber::Is_NoTaggedIntCheck(address)))
                {
                    return;
                }
            }
            break;
        }
    }

    DumpValueType(m_valueType);
}

void Opnd::DumpValueType(const ValueType valueType)
{
    if(valueType.IsUninitialized())
    {
        return;
    }

    char valueTypeStr[VALUE_TYPE_MAX_STRING_SIZE];
    valueType.ToString(valueTypeStr);
    Output::Print(_u("[%S]"), valueTypeStr);
}
#endif

IntConstOpnd *Opnd::CreateUint32Opnd(const uint i, Func *const func)
{
    return IntConstOpnd::New(i, TyUint32, func, true);
}

IntConstOpnd *Opnd::CreateProfileIdOpnd(const Js::ProfileId profileId, Func *const func)
{
    CompileAssert(sizeof(profileId) == sizeof(uint16));
    return IntConstOpnd::New(profileId, TyUint16, func, true);
}

IntConstOpnd *Opnd::CreateInlineCacheIndexOpnd(const Js::InlineCacheIndex inlineCacheIndex, Func *const func)
{
    CompileAssert(sizeof(inlineCacheIndex) == sizeof(uint));
    return CreateUint32Opnd(inlineCacheIndex, func);
}

RegOpnd *Opnd::CreateFramePointerOpnd(Func *const func)
{
    return RegOpnd::New(nullptr, LowererMD::GetRegFramePointer(), TyMachPtr, func);
}

///----------------------------------------------------------------------------
///
/// SymOpnd::New
///
///     Creates a new SymOpnd.
///
///----------------------------------------------------------------------------

SymOpnd *
SymOpnd::New(Sym *sym, IRType type, Func *func)
{
    return SymOpnd::New(sym, 0, type, func);
}

SymOpnd *
SymOpnd::New(Sym *sym, uint32 offset, IRType type, Func *func)
{
    SymOpnd * symOpnd;

    AssertMsg(sym, "A SymOpnd needs a valid symbol.");

    symOpnd = JitAnew(func->m_alloc, IR::SymOpnd);

    symOpnd->m_sym = sym;
    symOpnd->m_offset = offset;
    symOpnd->m_type = type;
    symOpnd->SetIsJITOptimizedReg(false);


    symOpnd->m_kind = OpndKindSym;


    return symOpnd;
}

///----------------------------------------------------------------------------
///
/// SymOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

SymOpnd *
SymOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindSym);
    SymOpnd * newOpnd;

    newOpnd = SymOpnd::New(m_sym, m_offset, m_type, func);
    newOpnd->m_valueType = m_valueType;
    newOpnd->canStoreTemp = this->canStoreTemp;
    newOpnd->SetIsJITOptimizedReg(this->GetIsJITOptimizedReg());

    return newOpnd;
}

SymOpnd *
SymOpnd::CloneDefInternal(Func *func)
{
    Assert(m_kind == OpndKindSym);
    Sym *sym = this->m_sym;

    if (sym->IsStackSym() && sym->AsStackSym()->m_isSingleDef)
    {
        StackSym * oldSym = sym->AsStackSym();
        StackSym * newSym = oldSym->CloneDef(func)->AsStackSym();
        if (func->GetCloner()->clonedInstrGetOrigArgSlotSym && oldSym->IsArgSlotSym())
        {
            Assert(newSym != oldSym);
            this->m_sym = newSym;
            newSym->m_instrDef = oldSym->m_instrDef;
            oldSym->m_instrDef = nullptr;
            sym = oldSym;
        }
        else
        {
            sym = newSym;
        }
    }

    SymOpnd * newOpnd = SymOpnd::New(sym, m_offset, m_type, func);

    return newOpnd;
}

SymOpnd *
SymOpnd::CloneUseInternal(Func *func)
{
    Assert(m_kind == OpndKindSym);
    Sym *sym = this->m_sym;

    if (sym->IsStackSym() && sym->AsStackSym()->m_isSingleDef)
    {
        StackSym * oldSym = sym->AsStackSym();
        StackSym * newSym = oldSym->CloneUse(func)->AsStackSym();
        if (func->GetCloner()->clonedInstrGetOrigArgSlotSym && oldSym->IsArgSlotSym())
        {
            Assert(newSym != oldSym);
            this->m_sym = newSym;
            sym = oldSym;
        }
        else
        {
            sym = newSym;
        }
    }

    SymOpnd * newOpnd = SymOpnd::New(sym, m_offset, m_type, func);

    return newOpnd;
}

StackSym *
SymOpnd::GetStackSymInternal() const
{
    return (this->m_sym && this->m_sym->IsStackSym()) ? this->m_sym->AsStackSym() : nullptr;
}

///----------------------------------------------------------------------------
///
/// SymOpnd::IsEqual
/// The SymOpnd's offset is 0 if it is called before regalloc. For Stack symopnd,
/// compare the type and symbol's offsets only when the symbol's isAllocated is true.
/// For other cases, compare the type, syms and offsets.
/// For example, following two instructions after RegAlloc phase:
///          iarg65535(s534)<0>.i32 = MOV      (NULL).var
///          iarg65535(s533)<0>.i32 = MOV      (NULL).var
/// are actually same instructions after encoding: mov dword ptr[ebp-0x1c], 0x0
/// Here for dst stack symOpnd, m_sym are different: s534 vs. s533, but offsets and
/// types are the same. So this function will report true if isAllocated is true.
/// Note: for property symopnd, still compare type, offset and sym.
///
///----------------------------------------------------------------------------
bool
SymOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindSym);
    Assert(opnd);
    if (!opnd->IsSymOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    SymOpnd  *opndSym = opnd->AsSymOpnd();
    Assert(opndSym);
    StackSym *thisStackSym = this->GetStackSymInternal();
    StackSym *opndStackSym = opndSym->GetStackSymInternal();
    if (thisStackSym && opndStackSym && thisStackSym->IsAllocated() && opndStackSym->IsAllocated())
    {
        return thisStackSym->m_offset == opndStackSym->m_offset;
    }
    else
    {
        return m_sym == opndSym->m_sym && m_offset == opndSym->m_offset;
    }
}

void
SymOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindSym);
    JitAdelete(func->m_alloc, this);
}


RegOpnd *SymOpnd::CreatePropertyOwnerOpnd(Func *const func) const
{
    Assert(m_sym->IsPropertySym());
    Assert(func);

    StackSym *const propertyOwnerSym = m_sym->AsPropertySym()->m_stackSym;
    RegOpnd *const propertyOwnerOpnd = RegOpnd::New(propertyOwnerSym, propertyOwnerSym->GetType(), func);
    propertyOwnerOpnd->SetValueType(GetPropertyOwnerValueType());
    return propertyOwnerOpnd;
}

PropertySymOpnd *
PropertySymOpnd::New(PropertySym *propertySym, uint inlineCacheIndex, IRType type, Func *func)
{
    PropertySymOpnd *newOpnd = IR::PropertySymOpnd::New(propertySym, type, func);
    newOpnd->Init(inlineCacheIndex, func);
    return newOpnd;
}

void
PropertySymOpnd::Init(uint inlineCacheIndex, Func *func)
{
    this->Init(inlineCacheIndex,
        inlineCacheIndex != -1 ? func->GetRuntimeInlineCache(inlineCacheIndex) : 0,
        inlineCacheIndex != -1 ? func->GetRuntimePolymorphicInlineCache(inlineCacheIndex) : nullptr,
        inlineCacheIndex != -1 ? func->GetObjTypeSpecFldInfo(inlineCacheIndex) : nullptr,
        inlineCacheIndex != -1 ? func->GetPolyCacheUtilToInitialize(inlineCacheIndex) : PolymorphicInlineCacheUtilizationMinValue);
}

PropertySymOpnd *
PropertySymOpnd::New(PropertySym *propertySym, IRType type, Func *func)
{
    PropertySymOpnd *newOpnd = JitAnew(func->m_alloc, IR::PropertySymOpnd);
    newOpnd->m_sym = propertySym;
    newOpnd->m_offset = 0;
    newOpnd->m_type = type;
    newOpnd->SetObjTypeSpecFldInfo(nullptr);
    newOpnd->finalType = JITTypeHolder(nullptr);
    newOpnd->monoGuardType = JITTypeHolder(nullptr);
    newOpnd->guardedPropOps = nullptr;
    newOpnd->writeGuards = nullptr;
    newOpnd->objTypeSpecFlags = 0;
    newOpnd->isPropertySymOpnd = true;
    newOpnd->checkedTypeSetIndex = (uint16)-1;

    newOpnd->m_kind = OpndKindSym;

    return newOpnd;
}

void
PropertySymOpnd::Init(uint inlineCacheIndex, intptr_t runtimeInlineCache, JITTimePolymorphicInlineCache * runtimePolymorphicInlineCache, ObjTypeSpecFldInfo* objTypeSpecFldInfo, byte polyCacheUtil)
{
    this->m_inlineCacheIndex = inlineCacheIndex;
    this->m_runtimeInlineCache = runtimeInlineCache;
    this->m_runtimePolymorphicInlineCache = runtimePolymorphicInlineCache;
    this->m_polyCacheUtil = polyCacheUtil;
    this->SetObjTypeSpecFldInfo(objTypeSpecFldInfo);

    this->SetIsJITOptimizedReg(false);
}

PropertySymOpnd *
PropertySymOpnd::CopyCommon(Func *func)
{
    PropertySymOpnd *newOpnd = PropertySymOpnd::New(this->m_sym->AsPropertySym(), this->m_type, func);
    newOpnd->m_valueType = this->m_valueType;
    newOpnd->m_inlineCacheIndex = this->m_inlineCacheIndex;
    newOpnd->m_runtimeInlineCache = this->m_runtimeInlineCache;
    newOpnd->m_runtimePolymorphicInlineCache = this->m_runtimePolymorphicInlineCache;
    newOpnd->canStoreTemp = this->canStoreTemp;
    return newOpnd;
}

PropertySymOpnd *
PropertySymOpnd::CopyWithoutFlowSensitiveInfo(Func *func)
{
    PropertySymOpnd *newOpnd = CopyCommon(func);
    newOpnd->SetObjTypeSpecFldInfo(this->objTypeSpecFldInfo);

    // This field is not flow sensitive.  It is only on if the instruction is CheckFixedMethodFld.  If we ever
    // hoist CheckFixedMethodFld (or otherwise copy it), we must make sure not to change the opcode.
    newOpnd->usesFixedValue = this->usesFixedValue;

    // Note that the following fields are flow sensitive. If we're cloning this operand in order to attach it to
    // an instruction elsewhere in the flow (e.g. field hoisting or copy propagation), these fields cannot be copied.
    // If the caller knows some of them can be safely copied, the caller must do so manually.
    Assert(newOpnd->typeCheckSeqFlags == 0);
    Assert(newOpnd->finalType == nullptr);
    Assert(newOpnd->guardedPropOps == nullptr);
    Assert(newOpnd->writeGuards == nullptr);

    newOpnd->SetIsJITOptimizedReg(this->GetIsJITOptimizedReg());

    return newOpnd;
}

PropertySymOpnd *
PropertySymOpnd::CopyInternalSub(Func *func)
{
    Assert(m_kind == OpndKindSym && this->IsPropertySymOpnd());
    PropertySymOpnd *newOpnd = CopyCommon(func);

    newOpnd->objTypeSpecFldInfo = this->objTypeSpecFldInfo;
    newOpnd->usesAuxSlot = usesAuxSlot;
    newOpnd->slotIndex = slotIndex;
    newOpnd->checkedTypeSetIndex = checkedTypeSetIndex;

    newOpnd->objTypeSpecFlags = this->objTypeSpecFlags;
    newOpnd->finalType = this->finalType;
    newOpnd->guardedPropOps = this->guardedPropOps != nullptr ? this->guardedPropOps->CopyNew() : nullptr;
    newOpnd->writeGuards = this->writeGuards != nullptr ? this->writeGuards->CopyNew() : nullptr;

    newOpnd->SetIsJITOptimizedReg(this->GetIsJITOptimizedReg());

    return newOpnd;
}

bool
PropertySymOpnd::IsObjectHeaderInlined() const
{
    JITTypeHolder type(nullptr);
    if (this->IsMono())
    {
        type = this->GetType();
    }
    else if (this->HasEquivalentTypeSet())
    {
        type = this->GetFirstEquivalentType();
    }

    if (type != nullptr && Js::DynamicType::Is(type->GetTypeId()))
    {
        return type->GetTypeHandler()->IsObjectHeaderInlinedTypeHandler();
    }

    return false;
}

bool
PropertySymOpnd::ChangesObjectLayout() const
{
    JITTypeHolder cachedType = this->IsMono() ? this->GetType() : this->GetFirstEquivalentType();

    JITTypeHolder finalType = this->GetFinalType();

    if (finalType != nullptr && Js::DynamicType::Is(finalType->GetTypeId()))
    {
        // This is the case where final type opt may cause pro-active type transition to take place.

    Assert(cachedType != nullptr && Js::DynamicType::Is(cachedType->GetTypeId()));

    return cachedType->GetTypeHandler()->GetInlineSlotCapacity() != finalType->GetTypeHandler()->GetInlineSlotCapacity() ||
        cachedType->GetTypeHandler()->GetOffsetOfInlineSlots() != finalType->GetTypeHandler()->GetOffsetOfInlineSlots();
    }

    if (!this->HasInitialType())
    {
        return false;
    }

    JITTypeHolder initialType = this->GetInitialType();
    if (initialType != nullptr && Js::DynamicType::Is(initialType->GetTypeId()))
    {
        // This is the case where the type transition actually occurs. (This is the only case that's detectable
        // during the loop pre-pass, since final types are not in place yet.)

        Assert(cachedType != nullptr && Js::DynamicType::Is(cachedType->GetTypeId()));

        const JITTypeHandler * cachedTypeHandler = cachedType->GetTypeHandler();
        const JITTypeHandler * initialTypeHandler = initialType->GetTypeHandler();

        return cachedTypeHandler->GetInlineSlotCapacity() != initialTypeHandler->GetInlineSlotCapacity() ||
            cachedTypeHandler->GetOffsetOfInlineSlots() != initialTypeHandler->GetOffsetOfInlineSlots();
    }

    return false;
}

void
PropertySymOpnd::UpdateSlotForFinalType()
{
    JITTypeHolder finalType = this->GetFinalType();

    Assert(this->IsMono() || this->checkedTypeSetIndex != (uint16)-1);
    JITTypeHolder cachedType =
        this->IsMono() ? this->GetType() : this->GetEquivalentTypeSet()->GetType(checkedTypeSetIndex);

    Assert(finalType != nullptr && Js::DynamicType::Is(finalType->GetTypeId()));
    Assert(cachedType != nullptr && Js::DynamicType::Is(cachedType->GetTypeId()));

    if (finalType == cachedType)
    {
        return;
    }

    // TODO: OOP JIT: should assert about runtime type handler addr 
    Assert(cachedType->GetTypeHandler() != finalType->GetTypeHandler());

    if (cachedType->GetTypeHandler()->GetInlineSlotCapacity() == finalType->GetTypeHandler()->GetInlineSlotCapacity() &&
        cachedType->GetTypeHandler()->GetOffsetOfInlineSlots() == finalType->GetTypeHandler()->GetOffsetOfInlineSlots())
    {
        // Nothing can change, since the variables aren't changing.
        return;
    }

    // Get the slot index and figure out the property index
    uint16 index = this->GetSlotIndex();
    if (this->UsesAuxSlot())
    {
        index += cachedType->GetTypeHandler()->GetInlineSlotCapacity();
    }
    else
    {
        index -= cachedType->GetTypeHandler()->GetOffsetOfInlineSlots() / sizeof(Js::Var);
    }

    // Figure out the slot index and aux-ness from the property index
    if (index >= finalType->GetTypeHandler()->GetInlineSlotCapacity())
    {
        this->SetUsesAuxSlot(true);
        index -= finalType->GetTypeHandler()->GetInlineSlotCapacity();
    }
    else
    {
        this->SetUsesAuxSlot(false);
        index += finalType->GetTypeHandler()->GetOffsetOfInlineSlots() / sizeof(Js::Var);
    }
    this->SetSlotIndex(index);
}

bool PropertySymOpnd::HasFinalType() const
{
    return this->finalType != nullptr;
}

PropertySymOpnd *
PropertySymOpnd::CloneDefInternalSub(Func *func)
{
    return this->CopyInternalSub(func);
}

PropertySymOpnd *
PropertySymOpnd::CloneUseInternalSub(Func *func)
{
    return this->CopyInternalSub(func);
}

RegOpnd::RegOpnd(StackSym *sym, RegNum reg, IRType type)
{
    Initialize(sym, reg, type);
}

RegOpnd::RegOpnd(const RegOpnd &other, StackSym *const sym)
{
    Initialize(sym, other.m_reg, other.m_type);

    m_valueType = other.m_valueType;
    SetIsJITOptimizedReg(other.GetIsJITOptimizedReg());
    m_dontDeadStore = other.m_dontDeadStore;
    m_wasNegativeZeroPreventedByBailout = other.m_wasNegativeZeroPreventedByBailout;

#if DBG
    m_symValueFrozen = other.m_symValueFrozen;
#endif
}

void RegOpnd::Initialize(StackSym *sym, RegNum reg, IRType type)
{
    AssertMsg(sym || reg != RegNOREG, "A RegOpnd needs a valid symbol or register.");
    Assert(!sym || sym->GetType() != TyMisc);

    m_kind = OpndKindReg;

    m_sym = sym;
    SetReg(reg);
    m_type = type;

    m_isTempLastUse = false;
    m_isCallArg = false;
    SetIsJITOptimizedReg(false);
    m_dontDeadStore = false;
    m_fgPeepTmp = false;
    m_wasNegativeZeroPreventedByBailout = false;
    m_isArrayRegOpnd = false;

#if DBG
    m_symValueFrozen = false;
#endif
}

///----------------------------------------------------------------------------
///
/// RegOpnd::New
///
///     Creates a new RegOpnd.
///
///----------------------------------------------------------------------------

RegOpnd *
    RegOpnd::New(IRType type, Func *func)
{
    return RegOpnd::New(StackSym::New(type, func), RegNOREG, type, func);
}

RegOpnd *
RegOpnd::New(StackSym *sym, IRType type, Func *func)
{
    return RegOpnd::New(sym, RegNOREG, type, func);
}

///----------------------------------------------------------------------------
///
/// RegOpnd::New
///
///     Creates a new RegOpnd.
///
///----------------------------------------------------------------------------

RegOpnd *
RegOpnd::New(StackSym *sym, RegNum reg, IRType type, Func *func)
{
    return JitAnew(func->m_alloc, IR::RegOpnd, sym, reg, type);
}

///----------------------------------------------------------------------------
///
/// RegOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------
RegOpnd *
RegOpnd::CopyInternal(StackSym * sym, Func *func)
{
    Assert(m_kind == OpndKindReg);
    return JitAnew(func->m_alloc, IR::RegOpnd, *this, sym);
}

RegOpnd *
RegOpnd::CopyInternal(Func *func)
{
    return CopyInternal(m_sym, func);
}

RegOpnd *
RegOpnd::CloneDefInternal(Func *func)
{
    StackSym * sym = m_sym ? m_sym->CloneDef(func) :  nullptr;
    return CopyInternal(sym, func);
}

RegOpnd *
RegOpnd::CloneUseInternal(Func *func)
{
    StackSym * sym = m_sym ? m_sym->CloneUse(func) : nullptr;
    return CopyInternal(sym, func);
}

StackSym *
RegOpnd::GetStackSymInternal() const
{
    return this->m_sym;
}

StackSym *
RegOpnd::TryGetStackSym(Opnd *const opnd)
{
    return opnd && opnd->IsRegOpnd() ? opnd->AsRegOpnd()->m_sym : nullptr;
}

///----------------------------------------------------------------------------
///
/// RegOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
RegOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindReg);
    return IsSameRegUntyped(opnd) && (this->GetType() == opnd->GetType());
}

void
RegOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindReg);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// RegOpnd::IsSameReg
///
/// Same as IsEqual except the type only need to be equal size
///
///----------------------------------------------------------------------------

bool
RegOpnd::IsSameReg(Opnd *opnd)
{
    return IsSameRegUntyped(opnd) && (TySize[this->GetType()] == TySize[opnd->GetType()]);
}

///----------------------------------------------------------------------------
///
/// RegOpnd::IsSameRegUntyped
///
/// Same as IsEqual but without any types comparison
///
///----------------------------------------------------------------------------

bool
RegOpnd::IsSameRegUntyped(Opnd *opnd)
{
    if (!opnd->IsRegOpnd())
    {
        return false;
    }
    RegOpnd *regOpnd = opnd->AsRegOpnd();

    if (m_reg != RegNOREG)
    {
        return m_reg == regOpnd->m_reg;
    }

    return m_sym == regOpnd->m_sym && regOpnd->m_reg == RegNOREG;
}

///----------------------------------------------------------------------------
///
/// ArrayRegOpnd
///
///----------------------------------------------------------------------------

ArrayRegOpnd::ArrayRegOpnd(
    StackSym *const arraySym,
    const ValueType valueType,
    StackSym *const headSegmentSym,
    StackSym *const headSegmentLengthSym,
    StackSym *const lengthSym,
    const bool eliminatedLowerBoundCheck,
    const bool eliminatedUpperBoundCheck)
    : RegOpnd(arraySym, RegNOREG, TyVar),
    headSegmentSym(headSegmentSym),
    headSegmentLengthSym(headSegmentLengthSym),
    lengthSym(lengthSym),
    eliminatedLowerBoundCheck(eliminatedLowerBoundCheck),
    eliminatedUpperBoundCheck(eliminatedUpperBoundCheck)
{
    Assert(valueType.IsAnyOptimizedArray());
    m_valueType = valueType;
    m_isArrayRegOpnd = true;
}

ArrayRegOpnd::ArrayRegOpnd(
    const RegOpnd &other,
    StackSym *const arraySym,
    const ValueType valueType,
    StackSym *const headSegmentSym,
    StackSym *const headSegmentLengthSym,
    StackSym *const lengthSym,
    const bool eliminatedLowerBoundCheck,
    const bool eliminatedUpperBoundCheck)
    : RegOpnd(other, arraySym),
    headSegmentSym(headSegmentSym),
    headSegmentLengthSym(headSegmentLengthSym),
    lengthSym(lengthSym),
    eliminatedLowerBoundCheck(eliminatedLowerBoundCheck),
    eliminatedUpperBoundCheck(eliminatedUpperBoundCheck)
{
    Assert(valueType.IsAnyOptimizedArray());
    m_valueType = valueType;
    m_isArrayRegOpnd = true;
}

ArrayRegOpnd *ArrayRegOpnd::New(
    StackSym *const arraySym,
    const ValueType valueType,
    StackSym *const headSegmentSym,
    StackSym *const headSegmentLengthSym,
    StackSym *const lengthSym,
    const bool eliminatedLowerBoundCheck,
    const bool eliminatedUpperBoundCheck,
    Func *const func)
{
    Assert(func);

    return
        JitAnew(
            func->m_alloc,
            ArrayRegOpnd,
            arraySym,
            valueType,
            headSegmentSym,
            headSegmentLengthSym,
            lengthSym,
            eliminatedLowerBoundCheck,
            eliminatedUpperBoundCheck);
}

ArrayRegOpnd *ArrayRegOpnd::New(
    const RegOpnd *const other,
    const ValueType valueType,
    StackSym *const headSegmentSym,
    StackSym *const headSegmentLengthSym,
    StackSym *const lengthSym,
    const bool eliminatedLowerBoundCheck,
    const bool eliminatedUpperBoundCheck,
    Func *const func)
{
    Assert(func);

    return
        JitAnew(
            func->m_alloc,
            ArrayRegOpnd,
            *other,
            other->m_sym,
            valueType,
            headSegmentSym,
            headSegmentLengthSym,
            lengthSym,
            eliminatedLowerBoundCheck,
            eliminatedUpperBoundCheck);
}

RegOpnd *ArrayRegOpnd::CopyAsRegOpnd(Func *func)
{
    RegOpnd *const regOpndCopy = RegOpnd::CopyInternal(func);
    Assert(!regOpndCopy->IsArrayRegOpnd());
    return regOpndCopy;
}

ArrayRegOpnd *ArrayRegOpnd::CopyInternalSub(Func *func)
{
    Assert(m_kind == OpndKindReg && this->IsArrayRegOpnd());
    return Clone(m_sym, headSegmentSym, headSegmentLengthSym, lengthSym, func);
}

ArrayRegOpnd *ArrayRegOpnd::CloneDefInternalSub(Func *func)
{
    Assert(m_kind == OpndKindReg && this->IsArrayRegOpnd());
    return
        Clone(
            m_sym ? m_sym->CloneDef(func) : nullptr,
            headSegmentSym ? headSegmentSym->CloneUse(func) : nullptr,
            headSegmentLengthSym ? headSegmentLengthSym->CloneUse(func) : nullptr,
            lengthSym ? lengthSym->CloneUse(func) : nullptr,
            func);
}

ArrayRegOpnd *ArrayRegOpnd::CloneUseInternalSub(Func *func)
{
    Assert(m_kind == OpndKindReg && this->IsArrayRegOpnd());
    return
        Clone(
            m_sym ? m_sym->CloneUse(func) : nullptr,
            headSegmentSym ? headSegmentSym->CloneUse(func) : nullptr,
            headSegmentLengthSym ? headSegmentLengthSym->CloneUse(func) : nullptr,
            lengthSym ? lengthSym->CloneUse(func) : nullptr,
            func);
}

ArrayRegOpnd *ArrayRegOpnd::Clone(
    StackSym *const arraySym,
    StackSym *const headSegmentSym,
    StackSym *const headSegmentLengthSym,
    StackSym *const lengthSym,
    Func *const func) const
{
    Assert(func);

    // Careful how clones are used. Only GlobOpt knows when it's valid to use the information in this class, so ideally cloning
    // should be done only at lowering time.
    return
        JitAnew(
            func->m_alloc,
            ArrayRegOpnd,
            *this,
            arraySym,
            m_valueType,
            headSegmentSym,
            headSegmentLengthSym,
            lengthSym,
            eliminatedLowerBoundCheck,
            eliminatedUpperBoundCheck);
}

void ArrayRegOpnd::FreeInternalSub(Func *func)
{
    Assert(m_kind == OpndKindReg && this->IsArrayRegOpnd());
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::New
///
///     Creates a new IntConstOpnd.
///
///----------------------------------------------------------------------------

IntConstOpnd *
IntConstOpnd::New(IntConstType value, IRType type, Func *func, bool dontEncode)
{
    IntConstOpnd * intConstOpnd;

    Assert(TySize[type] <= sizeof(IntConstType));

    intConstOpnd = JitAnew(func->m_alloc, IR::IntConstOpnd);

    intConstOpnd->m_type = type;
    intConstOpnd->m_kind = OpndKindIntConst;
    intConstOpnd->m_dontEncode = dontEncode;
    intConstOpnd->SetValue(value);

    return intConstOpnd;
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::CreateIntConstOpndFromType
///
///     Create an IntConstOpnd or Int64ConstOpnd depending on the IRType.
///
///----------------------------------------------------------------------------

IR::Opnd* IntConstOpnd::NewFromType(int64 value, IRType type, Func* func)
{
    if (IRType_IsInt64(type))
    {
        return Int64ConstOpnd::New(value, type, func);
    }
    Assert(value < (int64)UINT_MAX);
    return IntConstOpnd::New((IntConstType)value, type, func);
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

IntConstOpnd *
IntConstOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindIntConst);
    IntConstOpnd * newOpnd;
    newOpnd = IntConstOpnd::New(m_value, m_type, func, m_dontEncode);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
IntConstOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindIntConst);
    if (!opnd->IsIntConstOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    return m_value == opnd->AsIntConstOpnd()->m_value;
}

void
IntConstOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindIntConst);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::SetValue
///
///     Modifies the value of the IntConstOpnd
///
///----------------------------------------------------------------------------


void
IntConstOpnd::SetValue(IntConstType value)
{
    if (sizeof(IntConstType) > sizeof(int32))
    {
        Assert(m_type != TyInt32  || (value >= INT32_MIN && value <= INT32_MAX));
        Assert(m_type != TyUint32 || (value >= 0 && value <= UINT32_MAX));
    }

    // TODO: These should be uncommented, unfortunately, Lowerer::UseWithNewType
    // can change m_type (by calling SetType) in such a way that it violates these constraints.
    // If CopyInternal is later called on the IntConstOpnd, these will fail.
    // Assert(m_type != TyInt16  || (value >= INT16_MIN && value <= INT16_MAX));
    // Assert(m_type != TyUint16 || (value >= 0 && value <= UINT16_MAX));
    // Assert(m_type != TyInt8   || (value >= INT8_MIN && value <= INT8_MAX));
    // Assert(m_type != TyUint8  || (value >= 0 && value <= UINT8_MAX));

    m_value = value;
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::AsInt32
///
///     Retrieves the value of the int const opnd as a signed 32-bit integer.
///
///----------------------------------------------------------------------------

int32
IntConstOpnd::AsInt32()
{
    // TODO: Currently, there are cases where we construct IntConstOpnd with TyInt32
    // and retrieve value out as uint32 (or vice versa). Because of these, we allow
    // AsInt32/AsUint32 to cast between int32/uint32 in a lossy manner for now.
    // In the future, we should tighten up usage of IntConstOpnd to avoid these casts

    if (sizeof(IntConstType) == sizeof(int32))
    {
        return (int32)m_value;
    }

    if (m_type == TyUint32)
    {
        Assert(m_value >= 0 && m_value <= UINT32_MAX);
        return (int32)(uint32)m_value;
    }

    Assert(Math::FitsInDWord(m_value));
    return (int32)m_value;
}

///----------------------------------------------------------------------------
///
/// IntConstOpnd::AsUint32
///
///     Retrieves the value of the int const opnd as an unsigned 32-bit integer.
///
///----------------------------------------------------------------------------

uint32
IntConstOpnd::AsUint32()
{
    // TODO: See comment in AsInt32() regarding casts from int32 to uint32

    if (sizeof(uint32) == sizeof(IntConstType))
    {
        return (uint32)m_value;
    }

    Assert(sizeof(uint32) < sizeof(IntConstType));
    Assert(m_value >= 0 && m_value <= UINT32_MAX);
    return (uint32)m_value;
}

///----------------------------------------------------------------------------
///
/// Int64ConstOpnd Methods
///
///----------------------------------------------------------------------------
IR::Int64ConstOpnd* Int64ConstOpnd::New(int64 value, IRType type, Func *func)
{
    AssertMsg(func->GetJITFunctionBody()->IsWasmFunction(), "Only WebAssembly functions should have int64 const operands. Use IntConstOpnd for size_t type");
    Int64ConstOpnd * intConstOpnd;

    Assert(TySize[type] == sizeof(int64));

    intConstOpnd = JitAnew(func->m_alloc, IR::Int64ConstOpnd);

    intConstOpnd->m_type = type;
    intConstOpnd->m_kind = OpndKindInt64Const;
    intConstOpnd->m_value = value;

    return intConstOpnd;
}

IR::Int64ConstOpnd* Int64ConstOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindInt64Const);
    Int64ConstOpnd * newOpnd;
    newOpnd = Int64ConstOpnd::New(m_value, m_type, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}

bool Int64ConstOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindInt64Const);
    if (!opnd->IsInt64ConstOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    return m_value == opnd->AsInt64ConstOpnd()->m_value;
}

void Int64ConstOpnd::FreeInternal(Func * func)
{
    Assert(m_kind == OpndKindInt64Const);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// RegBVOpnd::New
///
///     Creates a new IntConstOpnd.
///
///----------------------------------------------------------------------------

RegBVOpnd *
RegBVOpnd::New(BVUnit32 value, IRType type, Func *func)
{
    RegBVOpnd * regBVOpnd;

    regBVOpnd = JitAnew(func->m_alloc, IR::RegBVOpnd);

    regBVOpnd->m_value.Copy(value);
    regBVOpnd->m_type = type;

    regBVOpnd->m_kind = OpndKindRegBV;

    return regBVOpnd;
}

///----------------------------------------------------------------------------
///
/// RegBVOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

RegBVOpnd *
RegBVOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindRegBV);
    RegBVOpnd * newOpnd;

    newOpnd = RegBVOpnd::New(m_value, m_type, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// RegBVOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
RegBVOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindRegBV);
    if (!opnd->IsRegBVOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    return m_value.Equal(opnd->AsRegBVOpnd()->m_value);
}

void
RegBVOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindRegBV);
    JitAdelete(func->m_alloc, this);
}


///----------------------------------------------------------------------------
///
/// FloatConstOpnd::New
///
///     Creates a new FloatConstOpnd.
///
///----------------------------------------------------------------------------

FloatConstOpnd *
FloatConstOpnd::New(FloatConstType value, IRType type, Func *func)
{
    FloatConstOpnd * floatConstOpnd;

    floatConstOpnd = JitAnew(func->m_alloc, IR::FloatConstOpnd);

    floatConstOpnd->m_value = value;
    floatConstOpnd->m_type = type;
#if !FLOATVAR
    floatConstOpnd->m_number = nullptr;
#endif

    floatConstOpnd->m_kind = OpndKindFloatConst;


    return floatConstOpnd;
}

FloatConstOpnd *
FloatConstOpnd::New(Js::Var floatVar, IRType type, Func *func, Js::Var varLocal /*= nullptr*/)
{
    Assert((varLocal && Js::JavascriptNumber::Is(varLocal)) || Js::JavascriptNumber::Is(floatVar));

    FloatConstType value = Js::JavascriptNumber::GetValue(varLocal ? varLocal : floatVar);
    FloatConstOpnd * floatConstOpnd = FloatConstOpnd::New(value, type, func);

#if !FLOATVAR
    floatConstOpnd->m_number = floatVar;
    floatConstOpnd->m_numberCopy = (Js::JavascriptNumber*)varLocal;
#endif

    return floatConstOpnd;
}

AddrOpnd *
FloatConstOpnd::GetAddrOpnd(Func *func, bool dontEncode)
{
#if !FLOATVAR
    if (this->m_number)
    {
        return AddrOpnd::New(this->m_number, (Js::TaggedNumber::Is(this->m_number) ? AddrOpndKindConstantVar : AddrOpndKindDynamicVar), func, dontEncode, this->m_numberCopy);
    }
#endif

    IR::AddrOpnd *opnd = AddrOpnd::NewFromNumber(this->m_value, func, dontEncode);

#if !FLOATVAR
    this->m_number = opnd->m_address;
#endif

    return opnd;
}

///----------------------------------------------------------------------------
///
/// FloatConstOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

FloatConstOpnd *
FloatConstOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindFloatConst);
    FloatConstOpnd * newOpnd;

    newOpnd = FloatConstOpnd::New(m_value, m_type, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}


///----------------------------------------------------------------------------
///
/// FloatConstOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
FloatConstOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindFloatConst);
    if (!opnd->IsFloatConstOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    return m_value == opnd->AsFloatConstOpnd()->m_value;
}

void
FloatConstOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindFloatConst);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// Float32ConstOpnd::New
///
///     Creates a new Float32ConstOpnd.
///
///----------------------------------------------------------------------------

Float32ConstOpnd *
Float32ConstOpnd::New(float value, IRType type, Func *func)
{
    Assert(type == IRType::TyFloat32); //TODO: should we even allow specifying a type here? It should always be TyFloat32
    Float32ConstOpnd * Float32ConstOpnd;

    Float32ConstOpnd = JitAnew(func->m_alloc, IR::Float32ConstOpnd);

    Float32ConstOpnd->m_value = value;
    Float32ConstOpnd->m_type = type;
    Float32ConstOpnd->m_kind = OpndKindFloat32Const;

    return Float32ConstOpnd;
}

///----------------------------------------------------------------------------
///
/// Float32ConstOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

Float32ConstOpnd *
Float32ConstOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindFloat32Const);
    Float32ConstOpnd * newOpnd;

    newOpnd = Float32ConstOpnd::New(m_value, m_type, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// Float32ConstOpnd::IsEqual
///
///----------------------------------------------------------------------------
bool
Float32ConstOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindFloat32Const);
    if (!opnd->IsFloat32ConstOpnd() || this->GetType() != opnd->GetType() /* TODO: could this be turned into an assert*/)
    {
        return false;
    }

    return m_value == opnd->AsFloat32ConstOpnd()->m_value;
}

void
Float32ConstOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindFloat32Const);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// Simd128ConstOpnd::New
///
///     Creates a new FloatConstOpnd.
///
///----------------------------------------------------------------------------

Simd128ConstOpnd *
Simd128ConstOpnd::New(AsmJsSIMDValue value, IRType type, Func *func)
{
    Simd128ConstOpnd * simd128ConstOpnd;

    simd128ConstOpnd = JitAnew(func->m_alloc, IR::Simd128ConstOpnd);

    simd128ConstOpnd->m_value = value;
    simd128ConstOpnd->m_type = type;

    simd128ConstOpnd->m_kind = OpndKindSimd128Const;


    return simd128ConstOpnd;
}



///----------------------------------------------------------------------------
///
/// Simd128ConstOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

Simd128ConstOpnd *
Simd128ConstOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindSimd128Const);
    Simd128ConstOpnd * newOpnd;

    newOpnd = Simd128ConstOpnd::New(m_value, m_type, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}


///----------------------------------------------------------------------------
///
/// Simd128ConstOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
Simd128ConstOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindSimd128Const);
    if (!opnd->IsSimd128ConstOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }

    return m_value == opnd->AsSimd128ConstOpnd()->m_value;
}

void
Simd128ConstOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindSimd128Const);
    JitAdelete(func->m_alloc, this);
}


///----------------------------------------------------------------------------
///
/// HelperCallOpnd::New
///
///     Creates a new HelperCallOpnd.
///
///----------------------------------------------------------------------------

HelperCallOpnd *
HelperCallOpnd::New(JnHelperMethod fnHelper, Func *func)
{
    HelperCallOpnd *helperCallOpnd = JitAnew(func->m_alloc, IR::HelperCallOpnd);
    helperCallOpnd->Init(fnHelper);

    return helperCallOpnd;
}

void
HelperCallOpnd::Init(JnHelperMethod fnHelper)
{
    Assert(fnHelper    != IR::HelperInvalid);
    this->m_fnHelper    = fnHelper;
    this->m_type        = TyMachPtr;

    this->m_kind = OpndKindHelperCall;
}


///----------------------------------------------------------------------------
///
/// HelperCallOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

HelperCallOpnd *
HelperCallOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindHelperCall);
    HelperCallOpnd *const newOpnd = HelperCallOpnd::New(m_fnHelper, func);
    newOpnd->m_valueType = m_valueType;
    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// HelperCallOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
HelperCallOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindHelperCall);
    if (!opnd->IsHelperCallOpnd())
    {
        return false;
    }

    return m_fnHelper == opnd->AsHelperCallOpnd()->m_fnHelper;
}

void
HelperCallOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindHelperCall);
    JitAdelete(func->m_alloc, this);
}

DiagHelperCallOpnd *
DiagHelperCallOpnd::New(JnHelperMethod fnHelper, Func *func, int argCount)
{
    DiagHelperCallOpnd *helperCallOpnd = JitAnew(func->m_alloc, IR::DiagHelperCallOpnd);
    helperCallOpnd->Init(fnHelper);
    helperCallOpnd->m_argCount = argCount;
    helperCallOpnd->isDiagHelperCallOpnd = true;
    return helperCallOpnd;
}

DiagHelperCallOpnd *
DiagHelperCallOpnd::CopyInternalSub(Func *func)
{
    Assert(m_kind == OpndKindHelperCall && this->IsDiagHelperCallOpnd());
    DiagHelperCallOpnd *const newOpnd = DiagHelperCallOpnd::New(m_fnHelper, func, m_argCount);
    newOpnd->m_valueType = m_valueType;
    return newOpnd;
}

bool
DiagHelperCallOpnd::IsEqualInternalSub(Opnd *opnd)
{
    Assert(m_kind == OpndKindHelperCall && this->IsDiagHelperCallOpnd());
    if (!opnd->IsHelperCallOpnd() || !opnd->AsHelperCallOpnd()->IsDiagHelperCallOpnd())
    {
        return false;
    }

    return
        m_fnHelper == opnd->AsHelperCallOpnd()->m_fnHelper &&
        m_argCount == static_cast<DiagHelperCallOpnd*>(opnd)->m_argCount;
}

///----------------------------------------------------------------------------
///
/// AddrOpnd::New
///
///     Creates a new AddrOpnd.
///
///----------------------------------------------------------------------------
AddrOpnd *
AddrOpnd::New(intptr_t address, AddrOpndKind addrOpndKind, Func *func, bool dontEncode /* = false */, Js::Var varLocal /* = nullptr*/)
{
    AddrOpnd * addrOpnd;

    addrOpnd = JitAnew(func->m_alloc, IR::AddrOpnd);

    // TODO (michhol): OOP JIT, use intptr_t instead of Js::Var by default so people don't try to dereference
    addrOpnd->m_address = (Js::Var)address;
    addrOpnd->m_localAddress = func->IsOOPJIT() ? varLocal : (Js::Var)address;
    addrOpnd->addrOpndKind = addrOpndKind;
    addrOpnd->m_type = addrOpnd->IsVar() ? TyVar : TyMachPtr;
    addrOpnd->m_dontEncode = dontEncode;
    addrOpnd->m_isFunction = false;

    if (address && addrOpnd->IsVar())
    {
        if (Js::TaggedInt::Is(address))
        {
            addrOpnd->m_valueType = ValueType::GetTaggedInt();
            addrOpnd->SetValueTypeFixed();
        }
        else if (
#if !FLOATVAR
            !func->IsOOPJIT() && CONFIG_FLAG(OOPJITMissingOpts) &&
#endif
            Js::JavascriptNumber::Is_NoTaggedIntCheck(addrOpnd->m_address))
        {
            addrOpnd->m_valueType =
                Js::JavascriptNumber::IsInt32_NoChecks(addrOpnd->m_address)
                ? ValueType::GetInt(false)
                : ValueType::Float;
            addrOpnd->SetValueTypeFixed();
        }
    }

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    addrOpnd->decodedValue = 0;
    addrOpnd->wasVar = addrOpnd->IsVar();
#endif

    addrOpnd->m_kind = OpndKindAddr;

    return addrOpnd;
}

AddrOpnd *
AddrOpnd::New(Js::Var address, AddrOpndKind addrOpndKind, Func *func, bool dontEncode /* = false */, Js::Var varLocal /* = nullptr*/)
{
    AddrOpnd * addrOpnd;

    addrOpnd = JitAnew(func->m_alloc, IR::AddrOpnd);

    addrOpnd->m_address = address;
    addrOpnd->m_localAddress = func->IsOOPJIT() ? varLocal : address;
    addrOpnd->addrOpndKind = addrOpndKind;
    addrOpnd->m_type = addrOpnd->IsVar()? TyVar : TyMachPtr;
    addrOpnd->m_dontEncode = dontEncode;
    addrOpnd->m_isFunction = false;
    addrOpnd->m_metadata = nullptr;

    if(address && addrOpnd->IsVar())
    {
        if(Js::TaggedInt::Is(address))
        {
            addrOpnd->m_valueType = ValueType::GetTaggedInt();
            addrOpnd->SetValueTypeFixed();
        }
        else
        {
            Js::Var var = varLocal ? varLocal : address;
            if (
#if !FLOATVAR
                varLocal || (!func->IsOOPJIT() && CONFIG_FLAG(OOPJITMissingOpts)) &&
#endif
                Js::JavascriptNumber::Is_NoTaggedIntCheck(var))
            {
                addrOpnd->m_valueType =
                    Js::JavascriptNumber::IsInt32_NoChecks(var)
                    ? ValueType::GetInt(false)
                    : ValueType::Float;
                addrOpnd->SetValueTypeFixed();
            }
        }
    }

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    addrOpnd->decodedValue = 0;
    addrOpnd->wasVar = addrOpnd->IsVar();
#endif

    addrOpnd->m_kind = OpndKindAddr;

    return addrOpnd;
}

AddrOpnd *
AddrOpnd::NewFromNumber(int32 value, Func *func, bool dontEncode /* = false */)
{
    if (!Js::TaggedInt::IsOverflow(value))
    {
        return New(Js::TaggedInt::ToVarUnchecked(value), AddrOpndKindConstantVar, func, dontEncode);
    }
    else
    {
        return NewFromNumberVar(value, func, dontEncode);
    }
}

AddrOpnd *
AddrOpnd::NewFromNumber(int64 value, Func *func, bool dontEncode /* = false */)
{
    if (!Js::TaggedInt::IsOverflow(value))
    {
        return New(Js::TaggedInt::ToVarUnchecked((int)value), AddrOpndKindConstantVar, func, dontEncode);
    }
    else
    {
        return NewFromNumberVar((double)value, func, dontEncode);
    }
}

AddrOpnd *
AddrOpnd::NewFromNumber(double value, Func *func, bool dontEncode /* = false */)
{
    //
    // Check if a well-known value:
    // - This significantly cuts down on the below floating-point to integer conversions.
    //

    if (Js::JavascriptNumber::IsNegZero(value))
    {
        return New(func->GetScriptContextInfo()->GetNegativeZeroAddr(), AddrOpndKindDynamicVar, func, dontEncode);
    }
    if (value == +0.0)
    {
        return New(Js::TaggedInt::ToVarUnchecked(0), AddrOpndKindConstantVar, func, dontEncode);
    }
    if (value == 1.0)
    {
        return New(Js::TaggedInt::ToVarUnchecked(1), AddrOpndKindConstantVar, func, dontEncode);
    }

    //
    // Check if number can be reduced back into a TaggedInt:
    // - This avoids extra GC.
    //

    int nValue      = (int) value;
    double dblCheck = (double) nValue;
    if ((dblCheck == value) && (!Js::TaggedInt::IsOverflow(nValue)))
    {
        return New(Js::TaggedInt::ToVarUnchecked(nValue), AddrOpndKindConstantVar, func, dontEncode);
    }

    return NewFromNumberVar(value, func, dontEncode);
}

AddrOpnd *
AddrOpnd::NewFromNumberVar(double value, Func *func, bool dontEncode /* = false */)
{
    Js::Var var = func->AllocateNumber((double)value);
    AddrOpnd* addrOpnd = New((intptr_t)var, AddrOpndKindDynamicVar, func, dontEncode);
    addrOpnd->m_valueType =
        Js::JavascriptNumber::IsInt32(value)
        ? ValueType::GetInt(false)
        : ValueType::Float;
    addrOpnd->SetValueTypeFixed();

    return addrOpnd;
}


AddrOpnd *
AddrOpnd::NewNull(Func *func)
{
    return AddrOpnd::New((Js::Var)0, AddrOpndKindConstantAddress, func, true);
}

///----------------------------------------------------------------------------
///
/// AddrOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

AddrOpnd *
AddrOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindAddr);
    AddrOpnd * newOpnd;

    newOpnd = AddrOpnd::New(nullptr, addrOpndKind, func, m_dontEncode);
    // Constructor evaluates address for type, but this is invalid if the address has been encoded, so we wait to set it
    newOpnd->m_address = m_address;
    newOpnd->m_valueType = m_valueType;
    newOpnd->m_isFunction = m_isFunction;
    newOpnd->m_metadata = m_metadata;
    newOpnd->SetType(m_type);
    if (IsValueTypeFixed())
    {
        newOpnd->SetValueTypeFixed();
    }

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    newOpnd->decodedValue = this->decodedValue;
    newOpnd->wasVar = this->wasVar;
#endif

    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// AddrOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
AddrOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindAddr);
    if (!opnd->IsAddrOpnd())
    {
        return false;
    }

    return m_address == opnd->AsAddrOpnd()->m_address;
}

void
AddrOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindAddr);
    JitAdelete(func->m_alloc, this);
}

void
AddrOpnd::SetEncodedValue(Js::Var address, AddrOpndKind addrOpndKind)
{
#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
    this->decodedValue = this->m_address;
#endif
    this->SetAddress(address, addrOpndKind);
}

void
AddrOpnd::SetAddress(Js::Var address, AddrOpndKind addrOpndKind)
{
    this->m_address = address;
    this->addrOpndKind = addrOpndKind;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::New
///
///     Creates a new IndirOpnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::New(RegOpnd *baseOpnd, RegOpnd *indexOpnd, IRType type, Func *func)
{
    IndirOpnd * indirOpnd;

    AssertMsg(baseOpnd, "An IndirOpnd needs a valid baseOpnd.");
    indirOpnd = JitAnew(func->m_alloc, IndirOpnd);

    indirOpnd->m_func = func;
    indirOpnd->SetBaseOpnd(baseOpnd);
    indirOpnd->SetIndexOpnd(indexOpnd);
    indirOpnd->m_type = type;
    indirOpnd->SetIsJITOptimizedReg(false);


    indirOpnd->m_kind = OpndKindIndir;

    return indirOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::New
///
///     Creates a new IndirOpnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::New(RegOpnd *baseOpnd, RegOpnd *indexOpnd, byte scale, IRType type, Func *func)
{
    IndirOpnd * indirOpnd = IndirOpnd::New(baseOpnd, indexOpnd, type, func);

    indirOpnd->m_scale = scale;

    return indirOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::New
///
///     Creates a new IndirOpnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::New(RegOpnd *indexOpnd, int32 offset, byte scale, IRType type, Func *func)
{
    IndirOpnd * indirOpnd;

    indirOpnd = JitAnew(func->m_alloc, IndirOpnd);

    indirOpnd->m_func = func;
    indirOpnd->SetBaseOpnd(nullptr);
    indirOpnd->SetOffset(offset, true);
    indirOpnd->SetIndexOpnd(indexOpnd);
    indirOpnd->m_type = type;
    indirOpnd->SetIsJITOptimizedReg(false);

    indirOpnd->m_kind = OpndKindIndir;

    indirOpnd->m_scale = scale;

    return indirOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::New
///
///     Creates a new IndirOpnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::New(RegOpnd *baseOpnd, int32 offset, IRType type, Func *func, bool dontEncode /* = false */)
{
    IndirOpnd * indirOpnd;

    indirOpnd = JitAnew(func->m_alloc, IndirOpnd);

    indirOpnd->m_func = func;
    indirOpnd->SetBaseOpnd(baseOpnd);

    indirOpnd->SetOffset(offset, dontEncode);
    indirOpnd->m_type = type;
    indirOpnd->SetIsJITOptimizedReg(false);


    indirOpnd->m_kind = OpndKindIndir;

    return indirOpnd;
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
///----------------------------------------------------------------------------
///
/// IndirOpnd::New
///
///     Creates a new IndirOpnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::New(RegOpnd *baseOpnd, int32 offset, IRType type, const char16 *desc, Func *func, bool dontEncode /* = false */)
{
    IndirOpnd * indirOpnd = IndirOpnd::New(baseOpnd, offset, type, func);
    indirOpnd->m_desc = desc;
    indirOpnd->m_dontEncode = dontEncode;
    return indirOpnd;
}
#endif

IndirOpnd::~IndirOpnd()
{
    if (m_baseOpnd != nullptr)
    {
        m_baseOpnd->UnUse();
        m_baseOpnd->Free(m_func);
    }
    if (m_indexOpnd != nullptr)
    {
        m_indexOpnd->UnUse();
        m_indexOpnd->Free(m_func);
    }
}
///----------------------------------------------------------------------------
///
/// IndirOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

IndirOpnd *
IndirOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindIndir);
    IndirOpnd * newOpnd;

    newOpnd = IndirOpnd::New(m_baseOpnd, m_indexOpnd, m_scale, m_type, func);

    newOpnd->m_valueType = m_valueType;
    newOpnd->canStoreTemp = this->canStoreTemp;
    newOpnd->SetOffset(m_offset, m_dontEncode);
    newOpnd->SetIsJITOptimizedReg(this->GetIsJITOptimizedReg());

#if DBG_DUMP
    newOpnd->m_addrKind = m_addrKind;
    newOpnd->m_originalAddress = m_originalAddress;
#endif
    return newOpnd;
}

IndirOpnd *
IndirOpnd::CloneDefInternal(Func *func)
{
    Assert(m_kind == OpndKindIndir);
    IndirOpnd * newOpnd;

    // The components of an IndirOpnd are always uses, even if the IndirOpnd itself is a def.
    RegOpnd * newBaseOpnd = m_baseOpnd ? m_baseOpnd->CloneUse(func)->AsRegOpnd() : nullptr;
    RegOpnd * newIndexOpnd = m_indexOpnd ? m_indexOpnd->CloneUse(func)->AsRegOpnd() : nullptr;
    newOpnd = IndirOpnd::New(newBaseOpnd, newIndexOpnd, m_scale, m_type, func);

    newOpnd->SetOffset(m_offset, m_dontEncode);

#if DBG_DUMP
    newOpnd->m_addrKind = m_addrKind;
    newOpnd->m_originalAddress = m_originalAddress;
#endif

    return newOpnd;
}

IndirOpnd *
IndirOpnd::CloneUseInternal(Func *func)
{
    Assert(m_kind == OpndKindIndir);
    IndirOpnd * newOpnd;

    RegOpnd * newBaseOpnd = m_baseOpnd ? m_baseOpnd->CloneUse(func)->AsRegOpnd() : nullptr;
    RegOpnd * newIndexOpnd = m_indexOpnd ? m_indexOpnd->CloneUse(func)->AsRegOpnd() : nullptr;
    newOpnd = IndirOpnd::New(newBaseOpnd, newIndexOpnd, m_scale, m_type, func);

    newOpnd->SetOffset(m_offset, m_dontEncode);

#if DBG_DUMP
    newOpnd->m_addrKind = m_addrKind;
    newOpnd->m_originalAddress = m_originalAddress;
#endif
    return newOpnd;
}

bool
IndirOpnd::TryGetIntConstIndexValue(bool trySym, IntConstType *pValue, bool * pIsNotInt)
{
    *pIsNotInt = false;
    IR::RegOpnd * indexOpnd = this->GetIndexOpnd();

    if (!indexOpnd)
    {
        *pValue = (IntConstType)this->GetOffset();
        return true;
    }

    if (!trySym)
    {
        return false;
    }

    StackSym * indexSym = indexOpnd->m_sym;
    *pIsNotInt = indexOpnd->IsNotInt();

    // Const flags for type-specialized syms are not accurate during the forward pass, so the forward pass cannot use that info
    // while the lowerer can. Additionally, due to value transfers being conservative in a loop prepass, the const flags can
    // show that a sym has a constant value even though the value during the forward pass did not. Skip checking const flags for
    // type-specialized index syms and instead, expect that once the above issues are fixed, that the forward pass would fold a
    // constant index into the indir's offset.
    if (!*pIsNotInt && !indexSym->IsTypeSpec() && indexSym->IsIntConst())
    {
        *pValue = indexSym->GetIntConstValue();
        return true;
    }

    return false;
}


///----------------------------------------------------------------------------
///
/// IndirOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
IndirOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindIndir);
    Assert(opnd);

    if (!opnd->IsIndirOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }
    IndirOpnd *indirOpnd = opnd->AsIndirOpnd();

    return m_offset == indirOpnd->m_offset
        && ((m_baseOpnd == nullptr && indirOpnd->m_baseOpnd == nullptr) || (m_baseOpnd && indirOpnd->m_baseOpnd && m_baseOpnd->IsEqual(indirOpnd->m_baseOpnd)))
        && ((m_indexOpnd == nullptr && indirOpnd->m_indexOpnd == nullptr) || (m_indexOpnd && indirOpnd->m_indexOpnd && m_indexOpnd->IsEqual(indirOpnd->m_indexOpnd)));
}

void
IndirOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindIndir);
    JitAdelete(func->m_alloc, this);
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::SetBaseOpnd
///
///----------------------------------------------------------------------------

void
IndirOpnd::SetBaseOpnd(RegOpnd *baseOpnd)
{
    if (m_baseOpnd)
    {
        m_baseOpnd->UnUse();
    }
    if (baseOpnd)
    {
        baseOpnd = baseOpnd->Use(m_func)->AsRegOpnd();
    }

    m_baseOpnd = baseOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::UnlinkBaseOpnd
///
///----------------------------------------------------------------------------

RegOpnd *
IndirOpnd::UnlinkBaseOpnd()
{
    RegOpnd * baseOpnd = this->m_baseOpnd;

    // This will also call UnUse()...
    this->SetBaseOpnd(nullptr);

    return baseOpnd;
}

void
IndirOpnd::ReplaceBaseOpnd(RegOpnd *newBase)
{
    RegOpnd * baseOpnd = this->m_baseOpnd;
    this->UnlinkBaseOpnd();
    baseOpnd->Free(this->m_func);

    this->SetBaseOpnd(newBase);
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::SetIndexOpnd
///
///----------------------------------------------------------------------------

void
IndirOpnd::SetIndexOpnd(RegOpnd *indexOpnd)
{
    if (m_indexOpnd)
    {
        m_indexOpnd->UnUse();
    }
    if (indexOpnd)
    {
        indexOpnd = indexOpnd->Use(m_func)->AsRegOpnd();
    }

    m_indexOpnd = indexOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::UnlinkIndexOpnd
///
///----------------------------------------------------------------------------

RegOpnd *
IndirOpnd::UnlinkIndexOpnd()
{
    RegOpnd * indexOpnd = this->m_indexOpnd;

    // This will also call UnUse()...
    this->SetIndexOpnd(nullptr);

    return indexOpnd;
}

void
IndirOpnd::ReplaceIndexOpnd(RegOpnd *newIndex)
{
    RegOpnd * indexOpnd = this->m_indexOpnd;
    this->UnlinkIndexOpnd();
    indexOpnd->Free(this->m_func);

    this->SetIndexOpnd(newIndex);
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
const char16 *
IndirOpnd::GetDescription()
{
    return this->m_desc;
}

bool
IndirOpnd::HasAddrKind() const
{
#if DBG_DUMP
    return m_addrKind != (IR::AddrOpndKind) - 1;
#else
    return false;
#endif
}

IR::AddrOpndKind
IndirOpnd::GetAddrKind() const
{
    Assert(HasAddrKind());
#if DBG_DUMP
    return m_addrKind;
#else
    return IR::AddrOpndKindDynamicMisc;
#endif
}

void *
IndirOpnd::GetOriginalAddress() const
{
    Assert(HasAddrKind());
#if DBG_DUMP
    Assert(m_originalAddress != nullptr);
    return m_originalAddress;
#else
    return nullptr;
#endif
}
#endif

#if DBG_DUMP
void
IndirOpnd::SetAddrKind(IR::AddrOpndKind kind, void * originalAddress)
{
    Assert(originalAddress != nullptr);
    this->m_addrKind = kind;
    this->m_originalAddress = originalAddress;
}
#endif
///----------------------------------------------------------------------------
///
/// MemRefOpnd::New
///
///     Creates a new MemRefOpnd.
///
///----------------------------------------------------------------------------

MemRefOpnd *
MemRefOpnd::New(intptr_t pMemLoc, IRType type, Func *func, AddrOpndKind addrOpndKind)
{
    MemRefOpnd * memRefOpnd = JitAnew(func->m_alloc, IR::MemRefOpnd);
    memRefOpnd->m_memLoc = pMemLoc;
    memRefOpnd->m_type = type;

    memRefOpnd->m_kind = OpndKindMemRef;
#if DBG_DUMP
    memRefOpnd->m_addrKind = addrOpndKind;
#endif

    return memRefOpnd;
}

// TODO: michhol OOP JIT, remove this signature
MemRefOpnd *
MemRefOpnd::New(void * pMemLoc, IRType type, Func *func, AddrOpndKind addrOpndKind)
{
    MemRefOpnd * memRefOpnd = JitAnew(func->m_alloc, IR::MemRefOpnd);
    memRefOpnd->m_memLoc = (intptr_t)pMemLoc;
    memRefOpnd->m_type = type;

    memRefOpnd->m_kind = OpndKindMemRef;
#if DBG_DUMP
    memRefOpnd->m_addrKind = addrOpndKind;
#endif

    return memRefOpnd;
}

IR::AddrOpndKind
MemRefOpnd::GetAddrKind() const
{
#if DBG_DUMP
    return this->m_addrKind;
#else
    return AddrOpndKindDynamicMisc;
#endif
}


///----------------------------------------------------------------------------
///
/// MemRefOpnd::Copy
///
///     Returns a copy of this opnd.
///
///----------------------------------------------------------------------------

MemRefOpnd *
MemRefOpnd::CopyInternal(Func *func)
{
    Assert(m_kind == OpndKindMemRef);
    MemRefOpnd * newOpnd;

    newOpnd = MemRefOpnd::New(m_memLoc, m_type, func);

    newOpnd->m_valueType = m_valueType;
    newOpnd->m_memLoc = m_memLoc;
#if DBG_DUMP
    newOpnd->m_addrKind = m_addrKind;
#endif
    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// MemRefOpnd::IsEqual
///
///----------------------------------------------------------------------------

bool
MemRefOpnd::IsEqualInternal(Opnd *opnd)
{
    Assert(m_kind == OpndKindMemRef);
    if (!opnd->IsMemRefOpnd() || this->GetType() != opnd->GetType())
    {
        return false;
    }
    MemRefOpnd *memRefOpnd = opnd->AsMemRefOpnd();

    return m_memLoc == memRefOpnd->m_memLoc;
}

void
MemRefOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindMemRef);
    JitAdelete(func->m_alloc, this);
}

LabelOpnd *
LabelOpnd::New(LabelInstr * labelInstr, Func * func)
{
    LabelOpnd * labelOpnd = JitAnew(func->m_alloc, IR::LabelOpnd);

    labelOpnd->m_label = labelInstr;
    labelOpnd->m_type = TyMachPtr;

    labelInstr->m_hasNonBranchRef = true;


    labelOpnd->m_kind = OpndKindLabel;

    return labelOpnd;
}

LabelOpnd *
LabelOpnd::CopyInternal(Func * func)
{
    Assert(m_kind == OpndKindLabel);
    LabelOpnd * newOpnd;

    newOpnd = LabelOpnd::New(m_label, func);
    newOpnd->m_valueType = m_valueType;

    return newOpnd;
}

bool
LabelOpnd::IsEqualInternal(Opnd * opnd)
{
    Assert(m_kind == OpndKindLabel);
    if (!opnd->IsLabelOpnd())
    {
        return false;
    }
    LabelOpnd * newOpnd = opnd->AsLabelOpnd();

    return m_label == newOpnd->GetLabel();
}

void
LabelOpnd::FreeInternal(Func *func)
{
    Assert(m_kind == OpndKindLabel);
    JitAdelete(func->m_alloc, this);
}

IR::RegOpnd *
Opnd::FindRegUse(IR::RegOpnd *regOpnd)
{
    StackSym *regSym = regOpnd->m_sym;

    if (this->IsRegOpnd())
    {
        if (this->AsRegOpnd()->m_sym == regSym)
        {
            return this->AsRegOpnd();
        }
    }
    else if (this->IsIndirOpnd())
    {
        IndirOpnd *indirOpnd = this->AsIndirOpnd();
        if (indirOpnd->GetBaseOpnd() && indirOpnd->GetBaseOpnd()->m_sym == regSym)
        {
            return indirOpnd->GetBaseOpnd();
        }
        if (indirOpnd->GetIndexOpnd() && indirOpnd->GetIndexOpnd()->m_sym == regSym)
        {
            return indirOpnd->GetIndexOpnd();
        }
    }

    return nullptr;
}

bool
Opnd::IsArgumentsObject()
{
    // returns "false" if the sym is not single def (happens when the parent function has formals); the opnd can still be the arguments object.
    // Since we need this information in the inliner where we don't track arguments object sym, going with single def is the best option.
    StackSym * sym = this->GetStackSym();

    return sym && sym->IsSingleDef() && sym->GetInstrDef()->HasAnyLoadHeapArgsOpCode();
}

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)

void
Opnd::DumpAddress(void *address, bool printToConsole, bool skipMaskedAddress)
{
    if (!printToConsole)
    {
        return;
    }

    if (!Js::Configuration::Global.flags.DumpIRAddresses)
    {
        if (skipMaskedAddress)
        {
            return;
        }
        Output::Print(_u("0xXXXXXXXX"));
    }
    else
    {
#ifdef _M_X64
        Output::Print(_u("0x%012I64X"), address);
#else
        Output::Print(_u("0x%08X"), address);
#endif
    }
}

void
Opnd::DumpFunctionInfo(_Outptr_result_buffer_(*count) char16 ** buffer, size_t * count, Js::FunctionInfo * info, bool printToConsole, _In_opt_z_ char16 const * type)
{
    char16 debugStringBuffer[MAX_FUNCTION_BODY_DEBUG_STRING_SIZE];
    if (info->HasBody())
    {
        if (type == nullptr)
        {
            type = _u("FunctionBody");
        }
        Js::FunctionProxy * proxy = info->GetFunctionProxy();
        WriteToBuffer(buffer, count, _u(" (%s [%s%s])"), type, proxy->GetDisplayName(), proxy->GetDebugNumberSet(debugStringBuffer));
    }
    else
    {
        if (type == nullptr)
        {
            type = _u("FunctionInfo");
        }
        WriteToBuffer(buffer, count, _u(" (%s)"), type);
    }
}

template<>
void EncodableOpnd<int32>::DumpEncodable() const
{
    if (name != nullptr)
    {
        Output::Print(_u("<%s> (value: 0x%X)"), name, m_value);
    }
    else if (decodedValue != 0)
    {
        Output::Print(_u("%d (0x%X) [encoded: 0x%X]"), decodedValue, decodedValue, m_value);
    }
    else
    {
        Output::Print(_u("%d (0x%X)"), m_value, m_value);
    }
}

template<>
void EncodableOpnd<int64>::DumpEncodable() const
{
    if (name != nullptr)
    {
        Output::Print(_u("<%s> (value: 0x%llX)"), name, m_value);
    }
    else if (decodedValue != 0)
    {
        Output::Print(_u("%lld (0x%llX) [encoded: 0x%llX]"), decodedValue, decodedValue, m_value);
    }
    else
    {
        Output::Print(_u("%lld (0x%llX)"), m_value, m_value);
    }
}


///----------------------------------------------------------------------------
///
/// Opnd::Dump
///
///     Dump this opnd.
///
///----------------------------------------------------------------------------

void
Opnd::Dump(IRDumpFlags flags, Func *func)
{
    bool const AsmDumpMode = flags & IRDumpFlags_AsmDumpMode;
    bool const SimpleForm = !!(flags & IRDumpFlags_SimpleForm);
    FloatConstType floatValue;
    SymOpnd * symOpnd;
    RegOpnd * regOpnd;
    JnHelperMethod helperMethod;
    bool dumpValueType = !SimpleForm;

    switch(GetKind())
    {
    case OpndKindSym:
        symOpnd = this->AsSymOpnd();
        if(symOpnd->m_sym->IsPropertySym() && !SimpleForm)
        {
            symOpnd->m_sym->Dump(flags, symOpnd->GetPropertyOwnerValueType());
        }
        else
        {
            symOpnd->m_sym->Dump(flags);
        }

        if (symOpnd->m_sym->IsStackSym())
        {
            StackSym * stackSym = symOpnd->m_sym->AsStackSym();
            bool hasOffset = stackSym->IsArgSlotSym()?
                ((stackSym->m_offset != -1) || !stackSym->m_isInlinedArgSlot) :
                (stackSym->m_offset != 0);
            if (hasOffset)
            {
                int offset = stackSym->m_offset;
                if(symOpnd->m_offset != 0)
                {
                    Assert(static_cast<int>(offset + symOpnd->m_offset) >= offset);
                    offset += symOpnd->m_offset;
                }
                Output::Print(_u("<%d>"), offset);
            }
        }
        else if (symOpnd->IsPropertySymOpnd() && !SimpleForm)
        {
            PropertySymOpnd *propertySymOpnd = symOpnd->AsPropertySymOpnd();
            Output::Print(_u("<"));
            if (propertySymOpnd->HasObjTypeSpecFldInfo())
            {
                Output::Print(_u("%u,%s%s%s%s,"), propertySymOpnd->GetObjTypeSpecFldId(), propertySymOpnd->IsPoly() ? _u("p") : _u("m"),
                    propertySymOpnd->IsLoadedFromProto() ? _u("~") : _u(""), propertySymOpnd->UsesFixedValue() ? _u("=") : _u(""),
                    propertySymOpnd->IsBeingAdded() ? _u("+") : _u(""));
            }
            else
            {
                Output::Print(_u("?,,"));
            }
            Output::Print(_u("%s%s,"), propertySymOpnd->MayNeedTypeCheckProtection() ?
                propertySymOpnd->IsMono() ? _u("+") : _u("=") :
                propertySymOpnd->IsRootObjectNonConfigurableFieldLoad() ? _u("~") : _u("-"),
                propertySymOpnd->IsTypeCheckSeqCandidate() ? _u("+") : _u("-"));
            if (propertySymOpnd->HasObjectTypeSym())
            {
                Output::Print(_u("s%d"), propertySymOpnd->GetObjectTypeSym()->m_id);
                if (propertySymOpnd->IsTypeChecked())
                {
                    Output::Print(_u("+%s"), propertySymOpnd->IsMono() ? _u("m") : _u("p"));
                }
                else if (propertySymOpnd->IsTypeAvailable())
                {
                    Output::Print(_u("*"));
                }
                if (propertySymOpnd->IsTypeDead())
                {
                    Output::Print(_u("!"));
                }
            }
            else
            {
                Output::Print(_u("s?"));
            }
            if (propertySymOpnd->m_sym->AsPropertySym()->m_writeGuardSym != nullptr)
            {
                Output::Print(_u(",s%d"), propertySymOpnd->m_sym->AsPropertySym()->m_writeGuardSym->m_id);
                if (propertySymOpnd->IsWriteGuardChecked())
                {
                    Output::Print(_u("+"));
                }
            }
            else
            {
                Output::Print(_u(",s?"));
            }
            if (propertySymOpnd->HasFinalType())
            {
                Output::Print(_u(",final:"));
                this->DumpAddress((void*)propertySymOpnd->GetFinalType()->GetAddr(), /* printToConsole */ true, /* skipMaskedAddress */ false);
            }
            if (propertySymOpnd->GetGuardedPropOps() != nullptr)
            {
                Output::Print(_u(",{"));
                if (func != nullptr)
                {
                    int i = 0;
                    auto guardedPropOps = propertySymOpnd->GetGuardedPropOps();
                    FOREACH_BITSET_IN_SPARSEBV(propertyOpId, guardedPropOps)
                    {
                        if (i++ > 0)
                        {
                            Output::Print(_u(","));
                        }
                        const ObjTypeSpecFldInfo* propertyOpInfo = func->GetTopFunc()->GetGlobalObjTypeSpecFldInfo(propertyOpId);
                        if (!JITManager::GetJITManager()->IsOOPJITEnabled())
                        {
                            Output::Print(_u("%s"), func->GetInProcThreadContext()->GetPropertyRecord(propertyOpInfo->GetPropertyId())->GetBuffer(), propertyOpId);
                        }
                        Output::Print(_u("(%u)"), propertyOpId);
                        
                        if (propertyOpInfo->IsLoadedFromProto())
                        {
                            Output::Print(_u("~"));
                        }
                        if (propertyOpInfo->HasFixedValue())
                        {
                            Output::Print(_u("="));
                        }
                        if (propertyOpInfo->IsBeingAdded())
                        {
                            Output::Print(_u("+"));
                        }
                    }
                    NEXT_BITSET_IN_SPARSEBV;
                }
                else
                {
                    Output::Print(_u("(no func)"));
                }
                Output::Print(_u("}"));
            }
            if (propertySymOpnd->GetWriteGuards() != nullptr)
            {
                Output::Print(_u(",{"));
                int i = 0;
                auto writeGuards = propertySymOpnd->GetWriteGuards();
                FOREACH_BITSET_IN_SPARSEBV(writeGuardSymId, writeGuards)
                {
                    if (i++ > 0)
                    {
                        Output::Print(_u(","));
                    }
                    Output::Print(_u("s%d"), writeGuardSymId);
                }
                NEXT_BITSET_IN_SPARSEBV;
                Output::Print(_u("}"));
            }
            if (propertySymOpnd->canStoreTemp)
            {
                Output::Print(_u(",t"));
            }
            Output::Print(_u(">"));
        }

        break;

    case OpndKindReg:
        regOpnd = this->AsRegOpnd();
        if (regOpnd->m_sym)
        {
            regOpnd->m_sym->Dump(flags);
        }
        if(AsmDumpMode)
        {
            //
            // Print no brackets
            //
            Output::Print(_u("%S"), RegNames[regOpnd->GetReg()]);
        }
        else
        {
            if (regOpnd->GetReg() != RegNOREG)
            {
                Output::Print(_u("(%S)"), RegNames[regOpnd->GetReg()]);
            }
            if (regOpnd->m_isTempLastUse)
            {
                Output::Print(_u("[isTempLastUse]"));
            }
            StackSym *sym = regOpnd->GetStackSym();
            if (sym && func)
            {
                if (sym == func->GetScriptContextSym())
                {
                    Output::Print(_u("[ScriptContext]"));
                }
                else if (sym == func->GetFuncObjSym())
                {
                    Output::Print(_u("[FuncObj]"));
                }
                else if (sym == func->GetFunctionBodySym())
                {
                    Output::Print(_u("[FunctionBody]"));
                }
            }
            if(regOpnd->IsArrayRegOpnd())
            {
                if(dumpValueType)
                {
                    // Dump the array value type before the associated syms
                    DumpValueType();
                    dumpValueType = false;
                }

                const ArrayRegOpnd *const arrayRegOpnd = regOpnd->AsArrayRegOpnd();
                if(arrayRegOpnd->HeadSegmentSym())
                {
                    Output::Print(_u("[seg: "));
                    arrayRegOpnd->HeadSegmentSym()->Dump();
                    Output::Print(_u("]"));
                }
                if(arrayRegOpnd->HeadSegmentLengthSym())
                {
                    Output::Print(_u("[segLen: "));
                    arrayRegOpnd->HeadSegmentLengthSym()->Dump();
                    Output::Print(_u("]"));
                }
                if(arrayRegOpnd->LengthSym() && arrayRegOpnd->LengthSym() != arrayRegOpnd->HeadSegmentLengthSym())
                {
                    Output::Print(_u("[len: "));
                    arrayRegOpnd->LengthSym()->Dump();
                    Output::Print(_u("]"));
                }
                if(arrayRegOpnd->EliminatedLowerBoundCheck() || arrayRegOpnd->EliminatedUpperBoundCheck())
                {
                    Output::Print(_u("["));
                    if(arrayRegOpnd->EliminatedLowerBoundCheck())
                    {
                        Output::Print(_u(">"));
                    }
                    if(arrayRegOpnd->EliminatedUpperBoundCheck())
                    {
                        Output::Print(_u("<"));
                    }
                    Output::Print(_u("]"));
                }
            }
        }
        break;

    case OpndKindInt64Const:
    {
        Int64ConstOpnd * intConstOpnd = this->AsInt64ConstOpnd();
        intConstOpnd->DumpEncodable();
        break;
    }
    case OpndKindIntConst:
    {
        IntConstOpnd * intConstOpnd = this->AsIntConstOpnd();
        intConstOpnd->DumpEncodable();
        break;
    }

    case OpndKindRegBV:
    {
        RegBVOpnd * regBVOpnd = this->AsRegBVOpnd();
        regBVOpnd->m_value.Dump();
        break;
    }

    case OpndKindHelperCall:
        helperMethod = this->AsHelperCallOpnd()->m_fnHelper;
        Output::Print(_u("%s"), IR::GetMethodName(helperMethod));
        break;

    case OpndKindFloatConst:
        floatValue = this->AsFloatConstOpnd()->m_value;
        Output::Print(_u("%G"), floatValue);
        break;

    case OpndKindFloat32Const:
        Output::Print(_u("%G"), this->AsFloat32ConstOpnd()->m_value);
        break;

    case OpndKindAddr:
        DumpOpndKindAddr(AsmDumpMode, func);
        break;

    case OpndKindIndir:
    {
        IndirOpnd * indirOpnd = this->AsIndirOpnd();
        RegOpnd * baseOpnd = indirOpnd->GetBaseOpnd();
        RegOpnd * indexOpnd = indirOpnd->GetIndexOpnd();
        const int32 offset = indirOpnd->GetOffset();

        Output::Print(_u("["));
        if (baseOpnd != nullptr)
        {
            baseOpnd->Dump(flags, func);
        }
        else
        {
            Output::Print(_u("<null>"));
        }

        if (indexOpnd != nullptr)
        {
            Output::Print(_u("+"));
            indexOpnd->Dump(flags, func);
            if (indirOpnd->GetScale() > 0)
            {
                Output::Print(_u("*%d"), 1 << indirOpnd->GetScale());
            }
        }
        if (offset != 0)
        {
            if (!Js::Configuration::Global.flags.DumpIRAddresses && indirOpnd->HasAddrKind())
            {
                Output::Print(_u("+XX"));
            }
            else
            {
                const auto sign = offset >= 0 ? _u("+") : _u("");
                if (AsmDumpMode)
                {
                    Output::Print(_u("%sXXXX%04d"), sign, offset & 0xffff);
                }
                else
                {
                    Output::Print(_u("%s%d"), sign, offset);
                }
            }
        }
        if (indirOpnd->GetDescription())
        {
            Output::Print(_u(" <%s>"), indirOpnd->GetDescription());
        }
        if (indirOpnd->HasAddrKind())
        {
            INT_PTR address = (INT_PTR)indirOpnd->GetOriginalAddress();
            Output::Print(_u(" <"));
            const size_t BUFFER_LEN = 128;
            char16 buffer[BUFFER_LEN];
            GetAddrDescription(buffer, BUFFER_LEN, (void *)address, indirOpnd->GetAddrKind(), AsmDumpMode, /*printToConsole */ true, func, /* skipMaskedAddress */true);
            Output::Print(_u("%s"), buffer);
            Output::Print(_u(">"));
        }

        Output::Print(_u("]"));
        break;
    }
    case OpndKindMemRef:
    {
        DumpOpndKindMemRef(AsmDumpMode, func);
        break;
    }
    case OpndKindLabel:
    {
        LabelOpnd * labelOpnd = this->AsLabelOpnd();
        LabelInstr * labelInstr = labelOpnd->GetLabel();
        if (labelInstr == nullptr)
        {
            Output::Print(_u("??"));
        }
        else
        {
            Output::Print(_u("&$L%d"), labelInstr->m_id);
        }
        break;
    }
    }

    if(!AsmDumpMode && dumpValueType)
    {
        DumpValueType();
    }
    if (!SimpleForm || this->GetType() != TyVar)
    {
        Output::Print(_u("."));
        IRType_Dump(this->GetType());
    }
    if (this->m_isDead && !SimpleForm)
    {
        Output::Print(_u("!"));
    }
}

///----------------------------------------------------------------------------
///
/// Opnd::DumpOpndKindAddr
///
///     Dump this opnd as an address.
///
///----------------------------------------------------------------------------

void
Opnd::DumpOpndKindAddr(bool AsmDumpMode, Func *func)
{
    const size_t BUFFER_LEN = 128;
    char16 buffer[BUFFER_LEN];
    GetAddrDescription(buffer, BUFFER_LEN, AsmDumpMode, true, func);

    Output::Print(_u("%s"), buffer);
}

void
Opnd::DumpOpndKindMemRef(bool AsmDumpMode, Func *func)
{
    MemRefOpnd *memRefOpnd = this->AsMemRefOpnd();
    Output::Print(_u("["));
    const size_t BUFFER_LEN = 128;
    char16 buffer[BUFFER_LEN];
    // TODO: michhol, make this intptr_t
    GetAddrDescription(buffer, BUFFER_LEN, (void*)memRefOpnd->GetMemLoc(), memRefOpnd->GetAddrKind(), AsmDumpMode, true, func);
    Output::Print(_u("%s"), buffer);
    Output::Print(_u("]"));
}

/**
    WriteToBuffer

    Write <fmt> with applicable replacements into <buffer>.

    Subtract the number of characters written from <count>, and increment the address
    <buffer> so that subsequent calls to this function will continue writing at the point
    in the buffer where this function left off and will respect the maximum length specified
    by count.

    @param buffer
        A pointer to a buffer which will hold the result.
    @param count
        The maximum number of characters that should be returned in <buffer>.
    @param fmt
        A format string.
    @param ...
        Additional parameters to be passed to the formatter.
*/
void
Opnd::WriteToBuffer(_Outptr_result_buffer_(*count) char16 **buffer, size_t *count, const char16 *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);

    int len = _vsnwprintf_s(*buffer, *count, _TRUNCATE, fmt, argptr);
    *count -= len;
    *buffer += len;
    va_end(argptr);
}

void
Opnd::GetAddrDescription(__out_ecount(count) char16 *const description, const size_t count,
    void * address, IR::AddrOpndKind addressKind, bool AsmDumpMode, bool printToConsole, Func *func, bool skipMaskedAddress)
{
    char16 *buffer = description;
    size_t n = count;

    if (address)
    {
        switch (addressKind)
        {
        case IR::AddrOpndKindConstantAddress:
        {
#ifdef _M_X64_OR_ARM64
            char16 const * format = _u("0x%012I64X");
#else
            char16 const * format = _u("0x%08X");
#endif
            WriteToBuffer(&buffer, &n, format, address);
        }
        break;
        case IR::AddrOpndKindDynamicVar:
            if (Js::TaggedInt::Is(address))
            {
#ifdef _M_X64_OR_ARM64
                char16 const * format = _u("0x%012I64X (value: %d)");
#else
                char16 const * format = _u("0x%08X  (value: %d)");
#endif
                WriteToBuffer(&buffer, &n, format, address, Js::TaggedInt::ToInt32(address));
            }
#if FLOATVAR
            else if (Js::JavascriptNumber::Is_NoTaggedIntCheck(address))
#else
            else if (!func->IsOOPJIT() && Js::JavascriptNumber::Is_NoTaggedIntCheck(address))
#endif
            {
                WriteToBuffer(&buffer, &n, _u(" (value: %f)"), Js::JavascriptNumber::GetValue(address));
            }
            else
            {
                DumpAddress(address, printToConsole, skipMaskedAddress);
                // TODO: michhol OOP JIT, fix dumping these
                if (func->IsOOPJIT())
                {
                    WriteToBuffer(&buffer, &n, _u(" (unknown)"));
                }
                else
                {
                    switch (Js::RecyclableObject::FromVar(address)->GetTypeId())
                    {
                    case Js::TypeIds_Boolean:
                        WriteToBuffer(&buffer, &n, Js::JavascriptBoolean::FromVar(address)->GetValue() ? _u(" (true)") : _u(" (false)"));
                        break;
                    case Js::TypeIds_String:
                        WriteToBuffer(&buffer, &n, _u(" (\"%s\")"), Js::JavascriptString::FromVar(address)->GetSz());
                        break;
                    case Js::TypeIds_Number:
                        WriteToBuffer(&buffer, &n, _u(" (value: %f)"), Js::JavascriptNumber::GetValue(address));
                        break;
                    case Js::TypeIds_Undefined:
                        WriteToBuffer(&buffer, &n, _u(" (undefined)"));
                        break;
                    case Js::TypeIds_Null:
                        WriteToBuffer(&buffer, &n, _u(" (null)"));
                        break;
                    case Js::TypeIds_GlobalObject:
                        WriteToBuffer(&buffer, &n, _u(" (GlobalObject)"));
                        break;
                    case Js::TypeIds_UndeclBlockVar:
                        WriteToBuffer(&buffer, &n, _u(" (UndeclBlockVar)"));
                        break;
                    case Js::TypeIds_Function:
                        DumpFunctionInfo(&buffer, &n, ((Js::JavascriptFunction *)address)->GetFunctionInfo(), printToConsole, _u("FunctionObject"));
                        break;
                    default:
                        WriteToBuffer(&buffer, &n, _u(" (DynamicObject)"));
                        break;
                    }
                }
            }
            break;
        case IR::AddrOpndKindConstantVar:
        {
#ifdef _M_X64_OR_ARM64
            char16 const * format = _u("0x%012I64X%s");
#else
            char16 const * format = _u("0x%08X%s");
#endif
            char16 const * addressName = _u("");

            if (address == Js::JavascriptArray::MissingItem)
            {
                addressName = _u(" (MissingItem)");
            }
#if FLOATVAR
            else if (address == (Js::Var)Js::FloatTag_Value)
            {
                addressName = _u(" (FloatTag)");
            }
#endif
            WriteToBuffer(&buffer, &n, format, address, addressName);
            break;
        }
        case IR::AddrOpndKindDynamicScriptContext:
            Assert(func == nullptr || (intptr_t)address == func->GetScriptContextInfo()->GetAddr());
            // The script context pointer is unstable allocated from the CRT
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (ScriptContext)"));
            break;
        case IR::AddrOpndKindDynamicCharStringCache:
            Assert(func == nullptr || (intptr_t)address == func->GetScriptContextInfo()->GetCharStringCacheAddr());
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (CharStringCache)"));
            break;

        case IR::AddrOpndKindDynamicBailOutRecord:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (BailOutRecord)"));
            break;

        case IR::AddrOpndKindDynamicInlineCache:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (InlineCache)"));
            break;

        case IR::AddrOpndKindDynamicIsInstInlineCacheFunctionRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&IsInstInlineCache.function)"));
            break;

        case IR::AddrOpndKindDynamicIsInstInlineCacheTypeRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&IsInstInlineCache.type)"));
            break;

        case IR::AddrOpndKindDynamicIsInstInlineCacheResultRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&IsInstInlineCache.result)"));
            break;

        case AddrOpndKindDynamicGuardValueRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&GuardValue)"));
            break;

        case AddrOpndKindDynamicAuxSlotArrayRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&AuxSlotArray)"));
            break;

        case AddrOpndKindDynamicPropertySlotRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&PropertySlot)"));
            break;

        case AddrOpndKindDynamicBailOutKindRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&BailOutKind)"));
            break;

        case AddrOpndKindDynamicArrayCallSiteInfo:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (ArrayCallSiteInfo)"));
            break;

        case AddrOpndKindDynamicTypeCheckGuard:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (TypeCheckGuard)"));
            break;

        case AddrOpndKindDynamicRecyclerAllocatorEndAddressRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&RecyclerAllocatorEndAddress)"));
            break;

        case AddrOpndKindDynamicAuxBufferRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (AuxBufferRef)"));
            break;

        case AddrOpndKindDynamicRecyclerAllocatorFreeListRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&RecyclerAllocatorFreeList)"));
            break;

        case IR::AddrOpndKindDynamicFunctionInfo:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            if (func->IsOOPJIT())
            {
                // TODO: OOP JIT, dump more info
                WriteToBuffer(&buffer, &n, _u(" (FunctionInfo)"));
            }
            else
            {
                DumpFunctionInfo(&buffer, &n, (Js::FunctionInfo *)address, printToConsole);
            }
            break;

        case IR::AddrOpndKindDynamicFunctionBody:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            if (func->IsOOPJIT())
            {
                // TODO: OOP JIT, dump more info
                WriteToBuffer(&buffer, &n, _u(" (FunctionBody)"));
            }
            else
            {
                DumpFunctionInfo(&buffer, &n, ((Js::FunctionBody *)address)->GetFunctionInfo(), printToConsole);
            }
            break;

        case IR::AddrOpndKindDynamicFunctionBodyWeakRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);

            if (func->IsOOPJIT())
            {
                // TODO: OOP JIT, dump more info
                WriteToBuffer(&buffer, &n, _u(" (FunctionBodyWeakRef)"));
            }
            else
            {
                DumpFunctionInfo(&buffer, &n, ((RecyclerWeakReference<Js::FunctionBody> *)address)->FastGet()->GetFunctionInfo(), printToConsole, _u("FunctionBodyWeakRef"));
            }
            break;

        case IR::AddrOpndKindDynamicFunctionEnvironmentRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            DumpFunctionInfo(&buffer, &n, ((Js::ScriptFunction *)((intptr_t)address - Js::ScriptFunction::GetOffsetOfEnvironment()))->GetFunctionInfo(),
                printToConsole, _u("ScriptFunctionEnvironmentRef"));
            break;
        case IR::AddrOpndKindDynamicVtable:
            if ((INT_PTR)address == Js::ScriptContextOptimizationOverrideInfo::InvalidVtable)
            {
                WriteToBuffer(&buffer, &n, _u("%d (Invalid Vtable)"), Js::ScriptContextOptimizationOverrideInfo::InvalidVtable);
            }
            else
            {
                DumpAddress(address, printToConsole, skipMaskedAddress);
                WriteToBuffer(&buffer, &n, _u(" (%S Vtable)"), func->GetVtableName((INT_PTR)address));
            }
            break;

        case IR::AddrOpndKindDynamicTypeHandler:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (TypeHandler)"));
            break;

        case IR::AddrOpndKindDynamicObjectTypeRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            {
                Js::RecyclableObject * dynamicObject = (Js::RecyclableObject *)((intptr_t)address - Js::RecyclableObject::GetOffsetOfType());
                if (!func->IsOOPJIT() && Js::JavascriptFunction::Is(dynamicObject))
                {
                    DumpFunctionInfo(&buffer, &n, Js::JavascriptFunction::FromVar((void *)((intptr_t)address - Js::RecyclableObject::GetOffsetOfType()))->GetFunctionInfo(),
                        printToConsole, _u("FunctionObjectTypeRef"));
                }
                else
                {
                    // TODO: OOP JIT, dump more info
                    WriteToBuffer(&buffer, &n, _u(" (ObjectTypeRef)"));
                }
            }
            break;

        case IR::AddrOpndKindDynamicType:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            // TODO: OOP JIT, dump more info
            if(!func->IsOOPJIT())
            {
                Js::TypeId typeId = ((Js::Type*)address)->GetTypeId();
                switch (typeId)
                {
                case Js::TypeIds_Number:
                    WriteToBuffer(&buffer, &n, _u(" (Type: StaticNumber)"));
                    break;
                case Js::TypeIds_String:
                    WriteToBuffer(&buffer, &n, _u(" (Type: StaticString)"));
                    break;
                case Js::TypeIds_Object:
                    WriteToBuffer(&buffer, &n, _u(" (Type: Object)"));
                    break;
                case Js::TypeIds_RegEx:
                    WriteToBuffer(&buffer, &n, _u(" (Type: Regex)"));
                    break;
                case Js::TypeIds_Array:
                    WriteToBuffer(&buffer, &n, _u(" (Type: Array)"));
                    break;
                case Js::TypeIds_NativeIntArray:
                    WriteToBuffer(&buffer, &n, _u(" (Type: NativeIntArray)"));
                    break;
                case Js::TypeIds_NativeFloatArray:
                    WriteToBuffer(&buffer, &n, _u(" (Type: NativeFltArray)"));
                    break;
                default:
                    WriteToBuffer(&buffer, &n, _u(" (Type: Id %d)"), typeId);
                    break;
                }
            }
            break;

        case AddrOpndKindDynamicFrameDisplay:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            if (!func->IsOOPJIT())
            {
                Js::FrameDisplay * frameDisplay = (Js::FrameDisplay *)address;
                WriteToBuffer(&buffer, &n, (frameDisplay->GetStrictMode() ? _u(" (StrictFrameDisplay len %d)") : _u(" (FrameDisplay len %d)")),
                    frameDisplay->GetLength());
            }
            else
            {
                WriteToBuffer(&buffer, &n, _u(" (FrameDisplay)"));
            }
            break;
        case AddrOpndKindSz:
            WriteToBuffer(&buffer, &n, wcslen((char16 const *)address) > 30 ? _u("\"%.30s...\"") : _u("\"%.30s\""), address);
            break;
        case AddrOpndKindDynamicFloatRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&(float)%f)"), *(float *)address);
            break;
        case AddrOpndKindDynamicDoubleRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&(double)%f)"), *(double *)address);
            break;
        case AddrOpndKindForInCache:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (ForInCache)"));
            break;
        case AddrOpndKindForInCacheType:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&ForInCache->type)"));
            break;
        case AddrOpndKindForInCacheData:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&ForInCache->data)"));
            break;
        case AddrOpndKindDynamicNativeCodeDataRef:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&NativeCodeData)"));
            break;
        case AddrOpndKindWriteBarrierCardTable:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            WriteToBuffer(&buffer, &n, _u(" (&WriteBarrierCardTable)"));
            break;
        default:
            DumpAddress(address, printToConsole, skipMaskedAddress);
            if ((intptr_t)address == func->GetThreadContextInfo()->GetNullFrameDisplayAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (NullFrameDisplay)"));
            }
            else if ((intptr_t)address == func->GetThreadContextInfo()->GetStrictNullFrameDisplayAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (StrictNullFrameDisplay)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetNumberAllocatorAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (NumberAllocator)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetRecyclerAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (Recycler)"));
            }
            else if (func->GetWorkItem()->Type() == JsFunctionType && (intptr_t)address == func->GetWorkItem()->GetCallsCountAddress())
            {
                WriteToBuffer(&buffer, &n, _u(" (&CallCount)"));
            }
            else if ((intptr_t)address == func->GetThreadContextInfo()->GetImplicitCallFlagsAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&ImplicitCallFlags)"));
            }
            else if ((intptr_t)address == func->GetThreadContextInfo()->GetDisableImplicitFlagsAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&DisableImplicitCallFlags)"));
            }
            else if ((intptr_t)address == func->GetThreadContextInfo()->GetThreadStackLimitAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&StackLimit)"));
            }
            else if (func->CanAllocInPreReservedHeapPageSegment() &&
#if ENABLE_OOP_NATIVE_CODEGEN
                (func->IsOOPJIT()
                    ? func->GetOOPThreadContext()->GetPreReservedSectionAllocator()->IsPreReservedEndAddress(address)
                    : func->GetInProcThreadContext()->GetPreReservedVirtualAllocator()->IsPreReservedEndAddress(address)
                )
#else
                func->GetInProcThreadContext()->GetPreReservedVirtualAllocator()->IsPreReservedEndAddress(address)
#endif
                )
            {
                WriteToBuffer(&buffer, &n, _u(" (PreReservedCodeSegmentEnd)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetSideEffectsAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&OptimizationOverrides_SideEffects)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetArraySetElementFastPathVtableAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&OptimizationOverrides_ArraySetElementFastPathVtable)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetIntArraySetElementFastPathVtableAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&OptimizationOverrides_IntArraySetElementFastPathVtable)"));
            }
            else if ((intptr_t)address == func->GetScriptContextInfo()->GetFloatArraySetElementFastPathVtableAddr())
            {
                WriteToBuffer(&buffer, &n, _u(" (&OptimizationOverrides_FloatArraySetElementFastPathVtable)"));
            }
            else
            {
                WriteToBuffer(&buffer, &n, _u(" (Unknown)"));
            }
        }
    }
    else
    {
        WriteToBuffer(&buffer, &n, _u("(NULL)"));
    }
}

/**
    GetAddrDescription

    Determine the type of the address and place at most <count> wide chars of the
    description into <description>.

    Force null termination of <description>.

    @param description
        A buffer which will hold the description.
    @param count
        The maximum number of characters that should be returned in <description>.
    @param AsmDumpMode
    @param func
*/
void
Opnd::GetAddrDescription(__out_ecount(count) char16 *const description, const size_t count, bool AsmDumpMode,
                         bool printToConsole, Func *func)
{
    char16 *buffer = description;
    size_t n = count;

    IR::AddrOpnd * addrOpnd = this->AsAddrOpnd();
    Js::Var address;

    bool isEncoded = false;
    if (addrOpnd->decodedValue != 0)
    {
        address = addrOpnd->decodedValue;
        isEncoded = true;
    }
    else
    {
        address = addrOpnd->m_address;
    }

    GetAddrDescription(description, count, address, addrOpnd->GetAddrOpndKind(), AsmDumpMode, printToConsole, func);

    if (isEncoded)
    {
        if (AsmDumpMode)
        {
            WriteToBuffer(&buffer, &n, _u(" [encoded]"));
        }
        else
        {
            WriteToBuffer(&buffer, &n, _u(" [encoded: 0x%08X"), addrOpnd->m_address);
        }
    }

    description[count-1] = 0;  // force null termination
}

void
Opnd::Dump()
{
    this->Dump(IRDumpFlags_None, nullptr);
}

#endif

} // namespace IR
