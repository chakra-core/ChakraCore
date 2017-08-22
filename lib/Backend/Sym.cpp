//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "Backend.h"

const Js::ArgSlot StackSym::InvalidSlot = (Js::ArgSlot)-1;

///----------------------------------------------------------------------------
///
/// StackSym::New
///
///     Creates a StackSym
///
///----------------------------------------------------------------------------

StackSym *
StackSym::New(SymID id, IRType type, Js::RegSlot byteCodeRegSlot, Func *func)
{
    StackSym * stackSym;

    if (byteCodeRegSlot != Js::Constants::NoRegister)
    {
        stackSym = AnewZ(func->m_alloc, ByteCodeStackSym, byteCodeRegSlot, func);
        stackSym->m_hasByteCodeRegSlot = true;
    }
    else
    {
        stackSym = AnewZ(func->m_alloc, StackSym);
    }

    stackSym->m_id = id;
    stackSym->m_kind = SymKindStack;

    // Assume SingleDef until proven false.

    stackSym->m_isConst = false;
    stackSym->m_isIntConst = false;
    stackSym->m_isTaggableIntConst = false;
    stackSym->m_isSingleDef = true;
    stackSym->m_isEncodedConstant = false;
    stackSym->m_isFltConst = false;
    stackSym->m_isInt64Const = false;
    stackSym->m_isStrConst = false;
    stackSym->m_isStrEmpty = false;
    stackSym->m_allocated = false;
    stackSym->m_isTypeSpec = false;
    stackSym->m_isArgSlotSym = false;
    stackSym->m_isArgSlotRegSym = false;
    stackSym->m_isParamSym = false;
    stackSym->m_isImplicitParamSym = false;
    stackSym->m_isBailOutReferenced = false;
    stackSym->m_isArgCaptured = false;
    stackSym->m_requiresBailOnNotNumber = false;
    stackSym->m_isCatchObjectSym = false;
    stackSym->m_isClosureSym = false;
    stackSym->m_builtInIndex = Js::BuiltinFunction::None;
    stackSym->m_slotNum = StackSym::InvalidSlot;

    stackSym->m_type = type;
    stackSym->m_equivNext = stackSym;
    stackSym->m_objectInfo = nullptr;

    AssertMsg(func->m_symTable->Find(id) == nullptr, "Trying to add new symbol which already exists.");
    func->m_symTable->Add(stackSym);

    return stackSym;
}

ObjectSymInfo *
ObjectSymInfo::New(Func * func)
{
    ObjectSymInfo * objSymInfo = JitAnewZ(func->m_alloc, ObjectSymInfo);
    return objSymInfo;
}

ObjectSymInfo *
ObjectSymInfo::New(StackSym * typeSym, Func * func)
{
    ObjectSymInfo * objSymInfo = ObjectSymInfo::New(func);
    objSymInfo->m_typeSym = typeSym;
    return objSymInfo;
}

ObjectSymInfo *
StackSym::EnsureObjectInfo(Func * func)
{
    if (this->m_objectInfo == nullptr)
    {
        this->m_objectInfo = ObjectSymInfo::New(func);
    }

    return this->m_objectInfo;
}

///----------------------------------------------------------------------------
///
/// StackSym::New
///
///     Creates a StackSym
///
///----------------------------------------------------------------------------

StackSym *
StackSym::New(Func *func)
{
    return StackSym::New(func->m_symTable->NewID(), TyVar, Js::Constants::NoRegister, func);
}

StackSym *
StackSym::New(IRType type, Func *func)
{
    return StackSym::New(func->m_symTable->NewID(), type, Js::Constants::NoRegister, func);
}

StackSym *
StackSym::NewImplicitParamSym(Js::ArgSlot paramSlotNum, Func * func)
{
    return func->m_symTable->GetImplicitParam(paramSlotNum);
}

StackSym *
StackSym::NewParamSlotSym(Js::ArgSlot paramSlotNum, Func * func)
{
    return NewParamSlotSym(paramSlotNum, func, TyVar);
}

StackSym *
StackSym::NewParamSlotSym(Js::ArgSlot paramSlotNum, Func * func, IRType type)
{
    StackSym * stackSym = StackSym::New(type, func);
    stackSym->m_isParamSym = true;
    stackSym->m_slotNum = paramSlotNum;
    return stackSym;
}

// Represents a temporary sym which is a copy of an arg slot sym.
StackSym *
StackSym::NewArgSlotRegSym(Js::ArgSlot argSlotNum, Func * func, IRType type /* = TyVar */)
{
    StackSym * stackSym = StackSym::New(type, func);
    stackSym->m_isArgSlotRegSym = true;
    stackSym->m_slotNum = argSlotNum;

#if defined(_M_X64)
    stackSym->m_argPosition = 0;
#endif

    return stackSym;
}

StackSym *
StackSym::NewArgSlotSym(Js::ArgSlot argSlotNum, Func * func, IRType type /* = TyVar */)
{
    StackSym * stackSym = StackSym::New(type, func);
    stackSym->m_isArgSlotSym = true;
    stackSym->m_slotNum = argSlotNum;

#if defined(_M_X64)
    stackSym->m_argPosition = 0;
#endif

    return stackSym;
}

bool
StackSym::IsTempReg(Func *const func) const
{
    return !HasByteCodeRegSlot() || GetByteCodeRegSlot() >= func->GetJITFunctionBody()->GetFirstTmpReg();
}

#if DBG
void
StackSym::VerifyConstFlags() const
{
    if (m_isConst)
    {
        Assert(this->m_isSingleDef);
        Assert(this->m_instrDef);
        if (m_isIntConst)
        {
            Assert(!m_isFltConst);
        }
        else
        {
            Assert(!m_isTaggableIntConst);
        }
    }
    else
    {
        Assert(!m_isIntConst);
        Assert(!m_isTaggableIntConst);
        Assert(!m_isFltConst);
    }
}
#endif

bool
StackSym::IsConst() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isConst;
}

bool
StackSym::IsIntConst() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isIntConst;
}

bool
StackSym::IsInt64Const() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isInt64Const;
}

bool
StackSym::IsTaggableIntConst() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isTaggableIntConst;
}

bool
StackSym::IsFloatConst() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isFltConst;
}

bool
StackSym::IsSimd128Const() const
{
#if DBG
    VerifyConstFlags();
#endif
    return m_isSimd128Const;
}


void
StackSym::SetIsConst()
{
    Assert(this->m_isSingleDef);
    Assert(this->m_instrDef);
    IR::Opnd * src = this->m_instrDef->GetSrc1();

    Assert(src->IsImmediateOpnd() || src->IsFloatConstOpnd() || src->IsSimd128ConstOpnd());

    if (src->IsIntConstOpnd())
    {
        Assert(this->m_instrDef->m_opcode == Js::OpCode::Ld_I4 ||  this->m_instrDef->m_opcode == Js::OpCode::LdC_A_I4 || LowererMD::IsAssign(this->m_instrDef));
        this->SetIsIntConst(src->AsIntConstOpnd()->GetValue());
    }
    else if (src->IsInt64ConstOpnd())
    {
        Assert(this->m_instrDef->m_opcode == Js::OpCode::Ld_I4 || LowererMD::IsAssign(this->m_instrDef));
        this->SetIsInt64Const();
    }
    else if (src->IsFloatConstOpnd())
    {
        Assert(this->m_instrDef->m_opcode == Js::OpCode::LdC_A_R8);
        this->SetIsFloatConst();
    }
    else if (src->IsSimd128ConstOpnd()){
        Assert(this->m_instrDef->m_opcode == Js::OpCode::Simd128_LdC);
        this->SetIsSimd128Const();
    }
    else
    {
        Assert(this->m_instrDef->m_opcode == Js::OpCode::Ld_A || LowererMD::IsAssign(this->m_instrDef));
        Assert(src->IsAddrOpnd());
        IR::AddrOpnd * addrOpnd = src->AsAddrOpnd();
        this->m_isConst = true;
        if (addrOpnd->IsVar())
        {
            Js::Var var = addrOpnd->m_address;
            if (Js::TaggedInt::Is(var))
            {
                this->m_isIntConst = true;
                this->m_isTaggableIntConst = true;
            }
            else if (var)
            {
#if !FLOATVAR
                if (JITManager::GetJITManager()->IsOOPJITEnabled())
                {
                    if (addrOpnd->m_localAddress && Js::JavascriptNumber::Is(addrOpnd->m_localAddress) && Js::JavascriptNumber::IsInt32_NoChecks(addrOpnd->m_localAddress))
                    {
                        this->m_isIntConst = true;
                    }
                }
                else
#endif
                {
                    if (Js::JavascriptNumber::Is(var) && Js::JavascriptNumber::IsInt32_NoChecks(var))
                    {
                        this->m_isIntConst = true;
                    }
                }
            }
        }
    }
}

void
StackSym::SetIsIntConst(IntConstType value)
{
    Assert(this->m_isSingleDef);
    Assert(this->m_instrDef);
    this->m_isConst = true;
    this->m_isIntConst = true;
    this->m_isInt64Const = false;
    this->m_isTaggableIntConst = !Js::TaggedInt::IsOverflow(value);
    this->m_isFltConst = false;
}

void StackSym::SetIsInt64Const()
{
    Assert(this->m_isSingleDef);
    Assert(this->m_instrDef);
    this->m_isConst = true;
    this->m_isInt64Const = true;
    this->m_isIntConst = false;
    this->m_isTaggableIntConst = false;
    this->m_isFltConst = false;
}

void
StackSym::SetIsFloatConst()
{
    Assert(this->m_isSingleDef);
    Assert(this->m_instrDef);
    this->m_isConst = true;
    this->m_isIntConst = false;
    this->m_isTaggableIntConst = false;
    this->m_isFltConst = true;
}

void
StackSym::SetIsSimd128Const()
{
    Assert(this->m_isSingleDef);
    Assert(this->m_instrDef);
    this->m_isConst = true;
    this->m_isIntConst = false;
    this->m_isTaggableIntConst = false;
    this->m_isFltConst = false;
    this->m_isSimd128Const = true;
}

Js::RegSlot
StackSym::GetByteCodeRegSlot() const
{
    Assert(HasByteCodeRegSlot());
    return ((ByteCodeStackSym *)this)->byteCodeRegSlot;
}



Func *
StackSym::GetByteCodeFunc() const
{
    Assert(HasByteCodeRegSlot());
    return ((ByteCodeStackSym *)this)->byteCodeFunc;
}

void
StackSym::IncrementArgSlotNum()
{
    Assert(IsArgSlotSym());
    m_slotNum++;
}

void
StackSym::DecrementArgSlotNum()
{
    Assert(IsArgSlotSym());
    m_slotNum--;
}

void
StackSym::FixupStackOffset(Func * currentFunc)
{
    int offsetFixup = 0;
    Func * parentFunc = currentFunc->GetParentFunc();
    while (parentFunc)
    {
        if (parentFunc->callSiteToArgumentsOffsetFixupMap)
        {
            parentFunc->callSiteToArgumentsOffsetFixupMap->TryGetValue(currentFunc->callSiteIdInParentFunc, &offsetFixup);
            this->m_offset += offsetFixup * MachPtr;
        }
        parentFunc = parentFunc->GetParentFunc();
        currentFunc = currentFunc->GetParentFunc();
        offsetFixup = 0;
    }
}

///----------------------------------------------------------------------------
///
/// StackSym::FindOrCreate
///
///     Look for a StackSym with the given ID.  If not found, create it.
///
///----------------------------------------------------------------------------

StackSym *
StackSym::FindOrCreate(SymID id, Js::RegSlot byteCodeRegSlot, Func *func, IRType type)
{
    StackSym *  stackSym;

    stackSym = func->m_symTable->FindStackSym(id);

    if (stackSym)
    {
        Assert(!stackSym->HasByteCodeRegSlot() ||
            (stackSym->GetByteCodeRegSlot() == byteCodeRegSlot && stackSym->GetByteCodeFunc() == func));
        Assert(stackSym->GetType() == type);
        return stackSym;
    }

    return StackSym::New(id, type, byteCodeRegSlot, func);
}

StackSym *
StackSym::CloneDef(Func *func)
{
    Cloner * cloner = func->GetCloner();
    StackSym *  newSym = nullptr;

    AssertMsg(cloner, "Use Func::BeginClone to initialize cloner");

    if (!this->m_isSingleDef)
    {
        return this;
    }

    switch (this->m_instrDef->m_opcode)
    {
        // Note: we were cloning single-def load constant instr's, but couldn't guarantee
        // that we were cloning all the uses.
    case Js::OpCode::ArgOut_A:
    case Js::OpCode::ArgOut_A_Dynamic:
    case Js::OpCode::ArgOut_A_InlineBuiltIn:
    case Js::OpCode::ArgOut_A_SpreadArg:
    case Js::OpCode::StartCall:
    case Js::OpCode::InlineeMetaArg:
        // Go ahead and clone: we need a single-def sym.
        // Arg/StartCall trees must be single-def.
        // Uses of int-const syms may need to find their defining instr's.
        break;
    default:
        return this;
    }

    if (cloner->symMap == nullptr)
    {
        cloner->symMap = HashTable<StackSym*>::New(cloner->alloc, 7);
    }
    else
    {
        StackSym **entry = cloner->symMap->Get(m_id);
        if (entry)
        {
            newSym = *entry;
        }
    }
    if (newSym == nullptr)
    {
        // NOTE: We don't care about the bytecode register information for cloned symbol
        // As those are the float sym that we will convert back to Var before jumping back
        // to the slow path's bailout.  The bailout can just track the original symbol.
        newSym = StackSym::New(func);
        newSym->m_isConst = m_isConst;
        newSym->m_isIntConst = m_isIntConst;
        newSym->m_isTaggableIntConst = m_isTaggableIntConst;
        newSym->m_isArgSlotSym = m_isArgSlotSym;
        newSym->m_isArgSlotRegSym = m_isArgSlotRegSym;
        newSym->m_isParamSym = m_isParamSym;
        newSym->m_isImplicitParamSym = m_isImplicitParamSym;
        newSym->m_isArgCaptured = m_isArgCaptured;
        newSym->m_isBailOutReferenced = m_isBailOutReferenced;
        newSym->m_slotNum = m_slotNum;

#if defined(_M_X64)
        newSym->m_argPosition = m_argPosition;
#endif

        newSym->m_offset = m_offset;
        newSym->m_allocated = m_allocated;
        newSym->m_isInlinedArgSlot = m_isInlinedArgSlot;
        newSym->m_isCatchObjectSym = m_isCatchObjectSym;
        newSym->m_isClosureSym = m_isClosureSym;
        newSym->m_type = m_type;

        newSym->CopySymAttrs(this);

        // NOTE: It is not clear what the right thing to do is here...
        newSym->m_equivNext = newSym;
        cloner->symMap->FindOrInsert(newSym, m_id);
    }

    return newSym;
}

StackSym *
StackSym::CloneUse(Func *func)
{
    Cloner * cloner = func->GetCloner();
    StackSym **  newSym;

    AssertMsg(cloner, "Use Func::BeginClone to initialize cloner");

    if (cloner->symMap == nullptr)
    {
        return this;
    }

    newSym = cloner->symMap->Get(m_id);

    if (newSym == nullptr)
    {
        return this;
    }

    return *newSym;
}

void
StackSym::CopySymAttrs(StackSym *symSrc)
{
    m_isNotInt = symSrc->m_isNotInt;
    m_isSafeThis = symSrc->m_isSafeThis;
    m_builtInIndex = symSrc->m_builtInIndex;
}

// StackSym::GetIntConstValue
int32
StackSym::GetIntConstValue() const
{
    Assert(this->IsIntConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();

    if (src1->IsIntConstOpnd())
    {
        Assert(defInstr->m_opcode == Js::OpCode::Ld_I4 || defInstr->m_opcode == Js::OpCode::LdC_A_I4 || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn || LowererMD::IsAssign(defInstr));
        return src1->AsIntConstOpnd()->AsInt32();
    }

    if (src1->IsAddrOpnd())
    {
        Assert(defInstr->m_opcode == Js::OpCode::Ld_A || LowererMD::IsAssign(defInstr) || defInstr->m_opcode == Js::OpCode::ArgOut_A || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
        IR::AddrOpnd *addr = src1->AsAddrOpnd();
        Assert(addr->IsVar());
        Js::Var var = src1->AsAddrOpnd()->m_address;
        if (Js::TaggedInt::Is(var))
        {
            return Js::TaggedInt::ToInt32(var);
        }
        int32 value;
        const bool isInt32 = Js::JavascriptNumber::TryGetInt32Value(Js::JavascriptNumber::GetValue(var), &value);
        Assert(isInt32);
        return value;
    }
    Assert(src1->IsRegOpnd());
    return src1->AsRegOpnd()->m_sym->GetIntConstValue();
}

Js::Var StackSym::GetFloatConstValueAsVar_PostGlobOpt() const
{
    Assert(this->IsConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();

    StackSym * stackSym = nullptr;

    if (src1->IsRegOpnd())
    {
        stackSym = src1->AsRegOpnd()->m_sym;
        Assert(!stackSym->m_isEncodedConstant);
        if (stackSym->m_isSingleDef)
        {
            //In ARM constant load is always legalized. Try to get the constant from src def
            defInstr = stackSym->m_instrDef;
            src1 = defInstr->GetSrc1();
        }
    }

    Assert(this->IsFloatConst() || (stackSym && stackSym->IsFloatConst()));

    IR::AddrOpnd *addrOpnd;
    if (src1->IsAddrOpnd())
    {
        Assert(defInstr->m_opcode == Js::OpCode::Ld_A);
        addrOpnd = src1->AsAddrOpnd();
    }
    else
    {
        Assert(src1->IsFloatConstOpnd());
        Assert(defInstr->m_opcode == Js::OpCode::LdC_A_R8);

        addrOpnd = src1->AsFloatConstOpnd()->GetAddrOpnd(defInstr->m_func);

        // This is just to prevent creating multiple numbers when the sym is used multiple times. We can only do this
        // post-GlobOpt, as otherwise it violates some invariants assumed in GlobOpt.
        defInstr->ReplaceSrc1(addrOpnd);
        defInstr->m_opcode = Js::OpCode::Ld_A;
    }

    const Js::Var address = addrOpnd->m_address;
    Assert(Js::JavascriptNumber::Is(addrOpnd->m_localAddress? addrOpnd->m_localAddress: addrOpnd->m_address));
    return address;
}

void *StackSym::GetConstAddress(bool useLocal /*= false*/) const
{
    Assert(this->IsConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();

    StackSym * stackSym = nullptr;

    if (src1->IsRegOpnd())
    {
        stackSym = src1->AsRegOpnd()->m_sym;
        Assert(!stackSym->m_isEncodedConstant);
        if (stackSym->m_isSingleDef)
        {
            //In ARM constant load is always legalized. Try to get the constant from src def
            defInstr = stackSym->m_instrDef;
            src1 = defInstr->GetSrc1();
        }
    }

    Assert(src1->IsAddrOpnd());
    Assert(defInstr->m_opcode == Js::OpCode::Ld_A || defInstr->m_opcode == Js::OpCode::LdStr || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn || LowererMD::IsAssign(defInstr));
    return useLocal ? src1->AsAddrOpnd()->m_localAddress : src1->AsAddrOpnd()->m_address;
}

intptr_t StackSym::GetLiteralConstValue_PostGlobOpt() const
{
    Assert(this->IsConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();

    StackSym * stackSym = nullptr;

    if (src1->IsRegOpnd())
    {
        stackSym = src1->AsRegOpnd()->m_sym;
        if (stackSym->m_isEncodedConstant)
        {
            Assert(!stackSym->m_isSingleDef);
            Assert(LowererMD::IsAssign(defInstr) || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
            return stackSym->constantValue;
        }

        if (stackSym->m_isSingleDef)
        {
            //In ARM constant load is always legalized. Try to get the constant from src def
            defInstr = stackSym->m_instrDef;
            src1 = defInstr->GetSrc1();
        }
    }

    if (src1->IsAddrOpnd())
    {
        Assert(defInstr->m_opcode == Js::OpCode::Ld_A || defInstr->m_opcode == Js::OpCode::LdStr || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn || LowererMD::IsAssign(defInstr));
        return reinterpret_cast<intptr_t>(src1->AsAddrOpnd()->m_address);
    }
    if (src1->IsIntConstOpnd())
    {
        Assert(this->IsIntConst() || (stackSym && stackSym->IsIntConst()));
        if (defInstr->m_opcode == Js::OpCode::LdC_A_I4)
        {
            IR::AddrOpnd *const addrOpnd = IR::AddrOpnd::NewFromNumber(src1->AsIntConstOpnd()->GetValue(), defInstr->m_func);
            const Js::Var address = addrOpnd->m_address;

            // This is just to prevent creating multiple numbers when the sym is used multiple times. We can only do this
            // post-GlobOpt, as otherwise it violates some invariants assumed in GlobOpt.
            defInstr->ReplaceSrc1(addrOpnd);
            defInstr->m_opcode = Js::OpCode::Ld_A;
            return reinterpret_cast<intptr_t>(address);
        }
        Assert(defInstr->m_opcode == Js::OpCode::Ld_I4 || LowererMD::IsAssign(defInstr) || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
        return src1->AsIntConstOpnd()->GetValue();
    }
    if (src1->IsFloatConstOpnd())
    {
        Assert(this->IsFloatConst() || (stackSym && stackSym->IsFloatConst()));
        Assert(defInstr->m_opcode == Js::OpCode::LdC_A_R8);

        IR::AddrOpnd *const addrOpnd = src1->AsFloatConstOpnd()->GetAddrOpnd(defInstr->m_func);
        const Js::Var address = addrOpnd->m_address;

        // This is just to prevent creating multiple numbers when the sym is used multiple times. We can only do this
        // post-GlobOpt, as otherwise it violates some invariants assumed in GlobOpt.
        defInstr->ReplaceSrc1(addrOpnd);
        defInstr->m_opcode = Js::OpCode::Ld_A;
        return reinterpret_cast<intptr_t>(address);
    }

    AssertMsg(UNREACHED, "Unknown const value");
    return 0;
}

IR::Opnd *
StackSym::GetConstOpnd() const
{
    Assert(IsConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();
    if (!src1)
    {
#if defined(_M_IX86) || defined(_M_X64)
        Assert(defInstr->m_opcode == Js::OpCode::MOVSD_ZERO);
#else
        Assert(UNREACHED);
#endif
    }
    else if (src1->IsIntConstOpnd())
    {
        Assert(this->IsIntConst());
        if (defInstr->m_opcode == Js::OpCode::LdC_A_I4)
        {
            src1 = IR::AddrOpnd::NewFromNumber(src1->AsIntConstOpnd()->GetValue(), defInstr->m_func);
            defInstr->ReplaceSrc1(src1);
            defInstr->m_opcode = Js::OpCode::Ld_A;
        }
        else
        {
            Assert(defInstr->m_opcode == Js::OpCode::Ld_I4 || LowererMD::IsAssign(defInstr) || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
        }
    }
    else if (src1->IsInt64ConstOpnd())
    {
        Assert(this->m_isInt64Const);
        Assert(defInstr->m_opcode == Js::OpCode::Ld_I4 || LowererMD::IsAssign(defInstr));
    }
    else if (src1->IsFloatConstOpnd())
    {
        Assert(this->IsFloatConst());
        Assert(defInstr->m_opcode == Js::OpCode::LdC_A_R8);

        src1 = src1->AsFloatConstOpnd()->GetAddrOpnd(defInstr->m_func);
        defInstr->ReplaceSrc1(src1);
        defInstr->m_opcode = Js::OpCode::Ld_A;

    }
    else if (src1->IsFloat32ConstOpnd())
    {
        Assert(this->IsFloatConst());
        Assert(defInstr->m_opcode == Js::OpCode::LdC_F8_R8 || LowererMD::IsAssign(defInstr));
    }
    else if (src1->IsAddrOpnd())
    {
        Assert(defInstr->m_opcode == Js::OpCode::Ld_A || LowererMD::IsAssign(defInstr) || defInstr->m_opcode == Js::OpCode::ArgOut_A_InlineBuiltIn);
    }
    else if (src1->IsMemRefOpnd())
    {
        Assert(this->IsFloatConst() || this->IsSimd128Const());
    }
    else
    {
        AssertMsg(UNREACHED, "Invalid const value");
    }
    return src1;
}

BailoutConstantValue StackSym::GetConstValueForBailout() const
{
    Assert(this->IsConst());
    IR::Instr *defInstr = this->m_instrDef;
    IR::Opnd *src1 = defInstr->GetSrc1();

    return src1->GetConstValue();
}


// SIMD_JS
StackSym *
StackSym::GetSimd128EquivSym(IRType type, Func *func)
{
    switch (type)
    {
    case TySimd128F4:
        return this->GetSimd128F4EquivSym(func);
        break;
    case TySimd128I4:
        return this->GetSimd128I4EquivSym(func);
        break;
    case TySimd128I16:
        return this->GetSimd128I16EquivSym(func);
        break;
    case TySimd128D2:
        return this->GetSimd128D2EquivSym(func);
        break;
    default:
        Assert(UNREACHED);
        return nullptr;
    }
}

StackSym *
StackSym::GetSimd128F4EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TySimd128F4, func);
}

StackSym *
StackSym::GetSimd128I4EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TySimd128I4, func);
}

StackSym *
StackSym::GetSimd128I16EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TySimd128I16, func);
}

StackSym *
StackSym::GetSimd128D2EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TySimd128D2, func);
}

StackSym *
StackSym::GetFloat64EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TyFloat64, func);
}

StackSym *
StackSym::GetInt32EquivSym(Func *func)
{
    return this->GetTypeEquivSym(TyInt32, func);
}

StackSym *
StackSym::GetVarEquivSym(Func *func)
{
    return this->GetTypeEquivSym(TyVar, func);
}

StackSym *
StackSym::GetVarEquivSym_NoCreate()
{
    return this->GetTypeEquivSym_NoCreate(TyVar);
}

StackSym const *
StackSym::GetVarEquivSym_NoCreate() const
{
    return this->GetTypeEquivSym_NoCreate(TyVar);
}

StackSym *
StackSym::GetTypeEquivSym(IRType type, Func *func)
{
    StackSym *sym = this->GetTypeEquivSym_NoCreate(type);

    if (sym != nullptr)
    {
        return sym;
    }

    // Don't allocate if func wasn't passed in.
    if (func == nullptr)
    {
        return nullptr;
    }

    if (this->HasByteCodeRegSlot())
    {
        sym = StackSym::New(func->m_symTable->NewID(), type,
            this->GetByteCodeRegSlot(), this->GetByteCodeFunc());
    }
    else
    {
        sym = StackSym::New(type, func);
    }
    sym->m_equivNext = this->m_equivNext;
    this->m_equivNext = sym;
    if (type != TyVar)
    {
        sym->m_isTypeSpec = true;
    }

    return sym;
}

StackSym *
StackSym::GetTypeEquivSym_NoCreate(IRType type)
{
    return const_cast<StackSym*>(static_cast<const StackSym*>(this)->GetTypeEquivSym_NoCreate(type));
}

StackSym const *
StackSym::GetTypeEquivSym_NoCreate(IRType type) const
{
    Assert(this->m_type != type);

    StackSym *sym = this->m_equivNext;
    int i = 1;
    while (sym != this)
    {
        Assert(i <= 5); // circular of at most 6 syms : var, f64, i32, simd128I4, simd128F4, simd128D2
        if (sym->m_type == type)
        {
            return sym;
        }
        sym = sym->m_equivNext;
        i++;
    }
    return nullptr;
}

StackSym *StackSym::GetVarEquivStackSym_NoCreate(Sym *const sym)
{
    return const_cast<StackSym*>(GetVarEquivStackSym_NoCreate((Sym const * const)sym));
}

StackSym const *StackSym::GetVarEquivStackSym_NoCreate(Sym const * const sym)
{
    Assert(sym);

    if(!sym->IsStackSym())
    {
        return nullptr;
    }

    StackSym const *stackSym = sym->AsStackSym();
    if(stackSym->IsTypeSpec())
    {
        stackSym = stackSym->GetVarEquivSym_NoCreate();
    }
    return stackSym;
}

///----------------------------------------------------------------------------
///
/// PropertySym::New
///
///     Creates a PropertySym
///
///----------------------------------------------------------------------------

PropertySym *
PropertySym::New(SymID stackSymID, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func)
{
    StackSym *  stackSym;

    stackSym = func->m_symTable->FindStackSym(stackSymID);
    AssertMsg(stackSym != nullptr, "Adding propertySym to non-existing stackSym...  Can this happen??");

    return PropertySym::New(stackSym, propertyId, propertyIdIndex, inlineCacheIndex, fieldKind, func);
}

PropertySym *
PropertySym::New(StackSym *stackSym, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func)
{
    PropertySym *  propertySym;

    propertySym = JitAnewZ(func->m_alloc, PropertySym);

    propertySym->m_func = func;
    propertySym->m_id = func->m_symTable->NewID();
    propertySym->m_kind = SymKindProperty;

    propertySym->m_propertyId = propertyId;

    propertyIdIndex = (uint)-1;
    inlineCacheIndex = (uint)-1;

    propertySym->m_propertyIdIndex = propertyIdIndex;
    propertySym->m_inlineCacheIndex = inlineCacheIndex;
    Assert(propertyIdIndex == (uint)-1 || inlineCacheIndex == (uint)-1);
    propertySym->m_loadInlineCacheIndex = (uint)-1;
    propertySym->m_loadInlineCacheFunc = nullptr;
    propertySym->m_fieldKind = fieldKind;

    propertySym->m_stackSym = stackSym;
    propertySym->m_propertyEquivSet = nullptr;

    // Add to list

    func->m_symTable->Add(propertySym);

    // Keep track of all the property we use from this sym so we can invalidate
    // the value in glob opt
    ObjectSymInfo * objectSymInfo = stackSym->EnsureObjectInfo(func);
    propertySym->m_nextInStackSymList = objectSymInfo->m_propertySymList;
    objectSymInfo->m_propertySymList = propertySym;

    return propertySym;
}

///----------------------------------------------------------------------------
///
/// PropertySym::FindOrCreate
///
///     Look for a PropertySym with the given ID/propertyId.
///
///----------------------------------------------------------------------------

PropertySym *
PropertySym::Find(SymID stackSymID, int32 propertyId, Func *func)
{
    return func->m_symTable->FindPropertySym(stackSymID, propertyId);
}

///----------------------------------------------------------------------------
///
/// PropertySym::FindOrCreate
///
///     Look for a PropertySym with the given ID/propertyId.  If not found,
///     create it.
///
///----------------------------------------------------------------------------

PropertySym *
PropertySym::FindOrCreate(SymID stackSymID, int32 propertyId, uint32 propertyIdIndex, uint inlineCacheIndex, PropertyKind fieldKind, Func *func)
{
    PropertySym *  propertySym;

    propertySym = Find(stackSymID, propertyId, func);

    if (propertySym)
    {
        return propertySym;
    }

    return PropertySym::New(stackSymID, propertyId, propertyIdIndex, inlineCacheIndex, fieldKind, func);
}
#if DBG_DUMP || defined(ENABLE_IR_VIEWER)

///----------------------------------------------------------------------------
///
/// Sym::Dump
///
///----------------------------------------------------------------------------

void
Sym::Dump(IRDumpFlags flags, const ValueType valueType) const
{
    bool const AsmDumpMode = flags & IRDumpFlags_AsmDumpMode;
    bool const SimpleForm = !!(flags & IRDumpFlags_SimpleForm);

    if (AsmDumpMode)
    {
        if (this->IsStackSym() && this->AsStackSym()->IsArgSlotSym())
        {
            Output::Print(_u("arg "));
        }
        else if (this->IsStackSym() && this->AsStackSym()->IsParamSlotSym())
        {
            Output::Print(_u("param "));
        }
    }
    else if (this->IsStackSym())
    {
        StackSym const *stackSym = this->AsStackSym();

        if (stackSym->IsArgSlotSym())
        {
            if (stackSym->m_isInlinedArgSlot)
            {
                Output::Print(_u("iarg%d"), stackSym->GetArgSlotNum());
            }
            else
            {
                Output::Print(_u("arg%d"), stackSym->GetArgSlotNum());
            }
            Output::Print(_u("(s%d)"), m_id);
        }
        else if (stackSym->IsParamSlotSym())
        {
            if (stackSym->IsImplicitParamSym())
            {
                switch (stackSym->GetParamSlotNum())
                {
                case 1:
                    Output::Print(_u("callInfo"));
                    break;
                case 2:
                    Output::Print(_u("funcInfo"));
                    break;
                case 3:
                    Output::Print(_u("genObj"));
                    break;
                case 4:
                    Output::Print(_u("genFrame"));
                    break;
                default:
                    Output::Print(_u("implPrm%d"), stackSym->GetParamSlotNum());
                }
            }
            else
            {
                Output::Print(_u("prm%d"), stackSym->GetParamSlotNum());
            }
        }
        else
        {
            Output::Print(_u("s%d"), m_id);

            if (Js::Configuration::Global.flags.Debug && stackSym->HasByteCodeRegSlot())
            {
                if (!JITManager::GetJITManager()->IsOOPJITEnabled())
                {
                    Js::FunctionBody* functionBody = (Js::FunctionBody*)stackSym->GetByteCodeFunc()->GetJITFunctionBody()->GetAddr();
                    if (functionBody->GetPropertyIdOnRegSlotsContainer())
                    {
                        if (functionBody->IsNonTempLocalVar(stackSym->GetByteCodeRegSlot()))
                        {
                            uint index = stackSym->GetByteCodeRegSlot() - stackSym->GetByteCodeFunc()->GetJITFunctionBody()->GetConstCount();
                            Js::PropertyId propertyId = functionBody->GetPropertyIdOnRegSlotsContainer()->propertyIdsForRegSlots[index];
                            Output::Print(_u("(%s)"), stackSym->GetByteCodeFunc()->GetInProcThreadContext()->GetPropertyRecord(propertyId)->GetBuffer());
                        }
                    }
                }
            }
            if (stackSym->IsVar())
            {
                if (stackSym->HasObjectTypeSym() && !SimpleForm)
                {
                    Output::Print(_u("<s%d>"), stackSym->GetObjectTypeSym()->m_id);
                }
            }
            else
            {
                StackSym const *varSym = stackSym->GetVarEquivSym_NoCreate();
                if (varSym)
                {
                    Output::Print(_u("(s%d)"), varSym->m_id);
                }
            }
            if (!SimpleForm)
            {
                if (stackSym->m_builtInIndex != Js::BuiltinFunction::None)
                {
                    Output::Print(_u("[ffunc]"));
                }
            }
            IR::Opnd::DumpValueType(valueType);
        }
    }
    else if (this->IsPropertySym())
    {
        PropertySym const *propertySym = this->AsPropertySym();

        if (!SimpleForm)
        {
            Output::Print(_u("s%d("), m_id);
        }

        switch (propertySym->m_fieldKind)
        {
        case PropertyKindData:
            propertySym->m_stackSym->Dump(flags, valueType);
            if (JITManager::GetJITManager()->IsOOPJITEnabled())
            {
                Output::Print(_u("->#%d"), propertySym->m_propertyId);
            }
            else
            {
                Js::PropertyRecord const* fieldName = propertySym->m_func->GetInProcThreadContext()->GetPropertyRecord(propertySym->m_propertyId);
                Output::Print(_u("->%s"), fieldName->GetBuffer());
            }
            break;
        case PropertyKindSlots:
        case PropertyKindSlotArray:
            propertySym->m_stackSym->Dump(flags, valueType);
            Output::Print(_u("[%d]"), propertySym->m_propertyId);
            break;
        case PropertyKindLocalSlots:
            propertySym->m_stackSym->Dump(flags, valueType);
            Output::Print(_u("l[%d]"), propertySym->m_propertyId);
            break;
        default:
            AssertMsg(0, "Unknown field kind");
            break;
        }

        if (!SimpleForm)
        {
            Output::Print(_u(")"));
        }
    }
}


void
Sym::Dump(const ValueType valueType) const
{
    this->Dump(IRDumpFlags_None, valueType);
}

void
Sym::DumpSimple() const
{
    this->Dump(IRDumpFlags_SimpleForm);
}
#endif  // DBG_DUMP
