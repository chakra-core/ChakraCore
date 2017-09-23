//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

void
Security::EncodeLargeConstants()
{
#pragma prefast(suppress:6236 6285, "logical-or of constants is by design")
    if (PHASE_OFF(Js::EncodeConstantsPhase, this->func) || CONFIG_ISENABLED(Js::DebugFlag) || !MD_ENCODE_LG_CONSTS)
    {
        return;
    }

    uint prevInstrConstSize = 0;
    FOREACH_INSTR_IN_FUNC_EDITING(instr, instrNext, this->func)
    {
        if (!instr->IsRealInstr())
        {
            if (instr->IsLabelInstr())
            {
                IR::LabelInstr * label = instr->AsLabelInstr();

                if (label->labelRefs.Count() > 1 || (label->labelRefs.Count() == 1 && label->labelRefs.Head() != label->m_prev))
                {
                    if (this->cookieOpnd != nullptr)
                    {
                        this->cookieOpnd->Free(this->func);
                    }
                    this->baseOpnd = nullptr;
                    this->cookieOpnd = nullptr;
                    this->basePlusCookieOpnd = nullptr;
                }
            }
            continue;
        }

        IR::Opnd *src1 = instr->GetSrc1();
        IR::Opnd *src2 = instr->GetSrc2();
        IR::Opnd *dst = instr->GetDst();

        if (dst && this->baseOpnd && dst->IsEqual(this->baseOpnd))
        {
            if (this->cookieOpnd != nullptr)
            {
                this->cookieOpnd->Free(this->func);
            }
            this->baseOpnd = nullptr;
            this->cookieOpnd = nullptr;
            this->basePlusCookieOpnd = nullptr;
        }

        uint currInstrConstSize = 0;
        uint dstSize = dst ? CalculateConstSize(dst) : 0;
        uint src1Size = 0;
        uint src2Size = 0;
        if (src1)
        {
            src1Size = CalculateConstSize(src1);
            if (src2)
            {
                src2Size = CalculateConstSize(src2);
            }
        }

        prevInstrConstSize = currInstrConstSize;
        currInstrConstSize = dstSize + src1Size + src2Size;

        // we don't need to blind constants if user controlled byte size < 3
        if (currInstrConstSize + prevInstrConstSize <= 2 && !PHASE_FORCE1(Js::EncodeConstantsPhase))
        {
            continue;
        }

        bool isSrc1EqualDst = false;
        if (dstSize >= 2)
        {
            // don't count instrs where dst == src1 against size
            if (src1 && dstSize == src1Size && src1->IsEqual(dst))
            {
                currInstrConstSize -= dstSize;
                isSrc1EqualDst = true;

                if (currInstrConstSize + prevInstrConstSize <= 2 && !PHASE_FORCE1(Js::EncodeConstantsPhase))
                {
                    continue;
                }
            }

            this->EncodeOpnd(instr, dst);
            if (isSrc1EqualDst)
            {
                instr->ReplaceSrc1(dst);
            }
            currInstrConstSize -= dstSize;
            if (currInstrConstSize + prevInstrConstSize <= 2 && !PHASE_FORCE1(Js::EncodeConstantsPhase))
            {
                continue;
            }
        }
        if (src1Size >= 2 && !isSrc1EqualDst)
        {
            this->EncodeOpnd(instr, src1);
            currInstrConstSize -= src1Size;
            if (currInstrConstSize + prevInstrConstSize <= 2 && !PHASE_FORCE1(Js::EncodeConstantsPhase))
            {
                continue;
            }
        }
        if (src2Size >= 2)
        {
            this->EncodeOpnd(instr, src2);
            currInstrConstSize -= src2Size;
        }
    } NEXT_INSTR_IN_FUNC_EDITING;

}

int
Security::GetNextNOPInsertPoint()
{
    uint frequency = (1 << CONFIG_FLAG(NopFrequency)) - 1;
    return (Math::Rand() & frequency) + 1;
}

void
Security::InsertRandomFunctionPad(IR::Instr * instrBeforeInstr)
{
    if (PHASE_OFF(Js::InsertNOPsPhase, instrBeforeInstr->m_func->GetTopFunc())
        || CONFIG_ISENABLED(Js::DebugFlag) || CONFIG_ISENABLED(Js::BenchmarkFlag))
    {
        return;
    }
    DWORD randomPad = Math::Rand() & ((0 - INSTR_ALIGNMENT) & 0xF);
#ifndef _M_ARM
    if (randomPad == 1)
    {
        InsertSmallNOP(instrBeforeInstr, 1);
        return;
    }
    if (randomPad & 1)
    {
        InsertSmallNOP(instrBeforeInstr, 3);
        randomPad -= 3;
    }
#endif
    Assert((randomPad & 1) == 0);
    while (randomPad >= 4)
    {
        InsertSmallNOP(instrBeforeInstr, 4);
        randomPad -= 4;
    }
    Assert(randomPad == 2 || randomPad == 0);
    if (randomPad == 2)
    {
        InsertSmallNOP(instrBeforeInstr, 2);
    }
}


void
Security::InsertNOPs()
{
    if (PHASE_OFF(Js::InsertNOPsPhase, this->func) || CONFIG_ISENABLED(Js::DebugFlag) || CONFIG_ISENABLED(Js::BenchmarkFlag))
    {
        return;
    }

    int count = 0;
    IR::Instr *instr = this->func->m_headInstr;

    while (true)
    {
        count = this->GetNextNOPInsertPoint();
        while (instr && count--)
        {
            instr = instr->GetNextRealInstr();
        }
        if (instr == nullptr)
        {
            break;
        }
        this->InsertNOPBefore(instr);
    };
}

void
Security::InsertNOPBefore(IR::Instr *instr)
{
    InsertSmallNOP(instr, (Math::Rand() & 0x3) + 1);
}

void
Security::InsertSmallNOP(IR::Instr * instr, DWORD nopSize)
{
#if defined(_M_IX86) || defined(_M_X64)
#ifdef _M_IX86
    if (AutoSystemInfo::Data.SSE2Available())
    {   // on x86 system that has SSE2, encode fast NOPs as x64 does
#endif
        Assert(nopSize >= 1 || nopSize <= 4);
        IR::Instr *nop = IR::Instr::New(Js::OpCode::NOP, instr->m_func);

        // Let the encoder know what the size of the NOP needs to be.
        if (nopSize > 1)
        {
            // 2, 3 or 4 byte NOP.
            IR::IntConstOpnd *nopSizeOpnd = IR::IntConstOpnd::New(nopSize, TyInt8, instr->m_func);
            nop->SetSrc1(nopSizeOpnd);
        }

        instr->InsertBefore(nop);
#ifdef _M_IX86
    }
    else
    {
        IR::Instr *nopInstr = nullptr;
        IR::RegOpnd *regOpnd;
        IR::IndirOpnd *indirOpnd;
        switch (nopSize)
        {
        case 1:
            // nop
            nopInstr = IR::Instr::New(Js::OpCode::NOP, instr->m_func);
            break;
        case 2:
            // mov edi, edi         ; 2 bytes
            regOpnd = IR::RegOpnd::New(nullptr, RegEDI, TyInt32, instr->m_func);
            nopInstr = IR::Instr::New(Js::OpCode::MOV, regOpnd, regOpnd, instr->m_func);
            break;
        case 3:
            // lea ecx, [ecx+00]    ; 3 bytes
            regOpnd = IR::RegOpnd::New(nullptr, RegECX, TyInt32, instr->m_func);
            indirOpnd = IR::IndirOpnd::New(regOpnd, (int32)0, TyInt32, instr->m_func);
            nopInstr = IR::Instr::New(Js::OpCode::LEA, regOpnd, indirOpnd, instr->m_func);
            break;
        case 4:
            // lea esp, [esp+00]    ; 4 bytes
            regOpnd = IR::RegOpnd::New(nullptr, RegESP, TyInt32, instr->m_func);
            indirOpnd = IR::IndirOpnd::New(regOpnd, (int32)0, TyInt32, instr->m_func);
            nopInstr = IR::Instr::New(Js::OpCode::LEA, regOpnd, indirOpnd, instr->m_func);
            break;
        default:
            Assert(false);
            break;
        }
        instr->InsertBefore(nopInstr);
    }
#endif
#elif defined(_M_ARM)
    // Can't insert 3 bytes, must choose between 2 and 4.

    IR::Instr *nopInstr = nullptr;

    switch (nopSize)
    {
    case 1:
    case 2:
        nopInstr = IR::Instr::New(Js::OpCode::NOP, instr->m_func);
        break;
    case 3:
    case 4:
        nopInstr = IR::Instr::New(Js::OpCode::NOP_W, instr->m_func);
        break;
    default:
        Assert(false);
        break;
    }

    instr->InsertBefore(nopInstr);
#else
    AssertMsg(false, "Unimplemented");
#endif
}

bool
Security::DontEncode(IR::Opnd *opnd)
{
    switch (opnd->GetKind())
    {
    case IR::OpndKindIntConst:
    {
        IR::IntConstOpnd *intConstOpnd = opnd->AsIntConstOpnd();
        return intConstOpnd->m_dontEncode;
    }

    case IR::OpndKindAddr:
    {
        IR::AddrOpnd *addrOpnd = opnd->AsAddrOpnd();
        return (addrOpnd->m_dontEncode ||
            !addrOpnd->IsVar() ||
            addrOpnd->m_address == nullptr ||
            !Js::TaggedNumber::Is(addrOpnd->m_address));
    }
    case IR::OpndKindIndir:
    {
        IR::IndirOpnd *indirOpnd = opnd->AsIndirOpnd();
        return indirOpnd->m_dontEncode || indirOpnd->GetOffset() == 0;
    }
    default:
        return true;
    }
}

uint
Security::CalculateConstSize(IR::Opnd *opnd)
{
    if (DontEncode(opnd))
    {
        return 0;
    }
    switch (opnd->GetKind())
    {
    case IR::OpndKindIntConst:
    {
        IR::IntConstOpnd *intConstOpnd = opnd->AsIntConstOpnd();
        return GetByteCount(intConstOpnd->GetValue());
    }
    case IR::OpndKindAddr:
    {
        IR::AddrOpnd *addrOpnd = opnd->AsAddrOpnd();
        return Js::TaggedInt::Is(addrOpnd->m_address) ? GetByteCount(Js::TaggedInt::ToInt32(addrOpnd->m_address)) : GetByteCount((intptr_t)addrOpnd->m_address);
    }
    case IR::OpndKindIndir:
    {
        IR::IndirOpnd * indirOpnd = opnd->AsIndirOpnd();
        return GetByteCount(indirOpnd->GetOffset());
    }
    default:
        Assume(UNREACHED);
    }
    return 0;
}
bool
Security::EncodeOpnd(IR::Instr * instr, IR::Opnd *opnd)
{
    IR::RegOpnd *newOpnd;
    bool isSrc2 = false;

    const auto unlinkSrc = [&]() {
        if (opnd != instr->GetSrc1())
        {
            Assert(opnd == instr->GetSrc2());
            isSrc2 = true;
            instr->UnlinkSrc2();
        }
        else
        {
            instr->UnlinkSrc1();
        }
    };

    switch (opnd->GetKind())
    {
    case IR::OpndKindIntConst:
    {
        IR::IntConstOpnd *intConstOpnd = opnd->AsIntConstOpnd();

        unlinkSrc();

        intConstOpnd->SetEncodedValue(EncodeValue(instr, intConstOpnd, intConstOpnd->GetValue(), &newOpnd));
    }
    break;

    case IR::OpndKindAddr:
    {
        IR::AddrOpnd *addrOpnd = opnd->AsAddrOpnd();

        unlinkSrc();

        addrOpnd->SetEncodedValue((Js::Var)this->EncodeValue(instr, addrOpnd, (IntConstType)addrOpnd->m_address, &newOpnd), addrOpnd->GetAddrOpndKind());
    }
    break;

    case IR::OpndKindIndir:
    {
        IR::IndirOpnd *indirOpnd = opnd->AsIndirOpnd();

        // Using 32 bit cookie causes major perf loss on the subsequent indirs, so only support this path for 16 bit offset
        // It's relatively rare for base to be null or to have index + offset, so fall back to the more generic xor method for these
        if (indirOpnd->GetBaseOpnd() && indirOpnd->GetIndexOpnd() == nullptr && Math::FitsInWord(indirOpnd->GetOffset()))
        {
            if (!this->baseOpnd || !this->baseOpnd->IsEqual(indirOpnd->GetBaseOpnd()))
            {
                if (this->cookieOpnd != nullptr)
                {
                    this->cookieOpnd->Free(this->func);
                }
                this->cookieOpnd = BuildCookieOpnd(TyInt16, instr->m_func);
                this->basePlusCookieOpnd = IR::RegOpnd::New(TyMachReg, instr->m_func);
                this->baseOpnd = indirOpnd->GetBaseOpnd();
                IR::IndirOpnd * indir = IR::IndirOpnd::New(this->baseOpnd, this->cookieOpnd->AsInt32(), TyMachReg, instr->m_func);
                Lowerer::InsertLea(this->basePlusCookieOpnd, indir, instr);
            }
            int32 diff = indirOpnd->GetOffset() - this->cookieOpnd->AsInt32();
            indirOpnd->SetOffset((int32)diff);
            indirOpnd->SetBaseOpnd(this->basePlusCookieOpnd);
            return true;
        }

        IR::IntConstOpnd *indexOpnd = IR::IntConstOpnd::New(indirOpnd->GetOffset(), TyMachReg, instr->m_func);

        indexOpnd->SetValue(EncodeValue(instr, indexOpnd, indexOpnd->GetValue(), &newOpnd));

        indirOpnd->SetOffset(0);
        if (indirOpnd->GetIndexOpnd() != nullptr)
        {
            // update the base rather than the index, because index might have scale
            if (indirOpnd->GetBaseOpnd() != nullptr)
            {
                IR::RegOpnd * newBaseOpnd = IR::RegOpnd::New(TyMachReg, instr->m_func);
                Lowerer::InsertAdd(false, newBaseOpnd, newOpnd, indirOpnd->GetBaseOpnd(), instr);
                indirOpnd->SetBaseOpnd(newBaseOpnd);
            }
            else
            {
                indirOpnd->SetBaseOpnd(newOpnd);
            }
        }
        else
        {
            indirOpnd->SetIndexOpnd(newOpnd);
        }
    }
    return true;

    default:
        return false;
    }

    IR::Opnd *dst = instr->GetDst();

    if (dst)
    {
#if TARGET_64
        // Ensure the left and right operand has the same type (that might not be true for constants on x64)
        newOpnd = (IR::RegOpnd *)newOpnd->UseWithNewType(dst->GetType(), instr->m_func);
#endif
        if (dst->IsRegOpnd())
        {
            IR::RegOpnd *dstRegOpnd = dst->AsRegOpnd();
            StackSym *dstSym = dstRegOpnd->m_sym;

            if (dstSym)
            {
                dstSym->m_isConst = false;
                dstSym->m_isIntConst = false;
                dstSym->m_isInt64Const = false;
                dstSym->m_isTaggableIntConst = false;
                dstSym->m_isFltConst = false;
            }
        }
    }

    LowererMD::ImmedSrcToReg(instr, newOpnd, isSrc2 ? 2 : 1);
    return true;
}

IR::IntConstOpnd *
Security::BuildCookieOpnd(IRType type, Func * func)
{
    IntConstType cookie = 0;
    switch (type)
    {
    case TyInt8:
        cookie = (int8)Math::Rand();
        break;
    case TyUint8:
        cookie = (uint8)Math::Rand();
        break;
    case TyInt16:
        cookie = (int16)Math::Rand();
        break;
    case TyUint16:
        cookie = (uint16)Math::Rand();
        break;
#if TARGET_32
    case TyVar:
#endif
    case TyInt32:
        cookie = (int32)Math::Rand();
        break;
    case TyUint32:
        cookie = (uint32)Math::Rand();
        break;
#if TARGET_64
    case TyVar:
    case TyInt64:
    case TyUint64:
        cookie = Math::Rand();
        break;
#endif
    default:
        Assume(UNREACHED);
    }
    IR::IntConstOpnd * cookieOpnd = IR::IntConstOpnd::New(cookie, type, func);

#if DBG_DUMP
    cookieOpnd->SetName(_u("cookie"));
#endif
    return cookieOpnd;
}

IntConstType
Security::EncodeValue(IR::Instr * instr, IR::Opnd *opnd, IntConstType constValue, _Out_ IR::RegOpnd **pNewOpnd)
{
    if (opnd->GetType() == TyInt32 || opnd->GetType() == TyInt16 || opnd->GetType() == TyInt8
#if TARGET_32
        || opnd->GetType() == TyVar
#endif
        )
    {
        IR::RegOpnd *regOpnd = IR::RegOpnd::New(StackSym::New(opnd->GetType(), instr->m_func), opnd->GetType(), instr->m_func);
        IR::Instr * instrNew = LowererMD::CreateAssign(regOpnd, opnd, instr);
        IR::IntConstOpnd * cookieOpnd = BuildCookieOpnd(opnd->GetType(), instr->m_func);
        instrNew = IR::Instr::New(LowererMD::MDXorOpcode, regOpnd, regOpnd, cookieOpnd, instr->m_func);
        instr->InsertBefore(instrNew);
        LowererMD::Legalize(instrNew);

        StackSym * stackSym = regOpnd->m_sym;
        Assert(!stackSym->m_isSingleDef);
        Assert(stackSym->m_instrDef == nullptr);
        stackSym->m_isEncodedConstant = true;
        stackSym->constantValue = (int32)constValue;

        *pNewOpnd = regOpnd;

        int32 value = (int32)constValue;
        value = value ^ cookieOpnd->AsInt32();
        return value;
    }
    else if (opnd->GetType() == TyUint32 || opnd->GetType() == TyUint16 || opnd->GetType() == TyUint8)
    {
        IR::RegOpnd *regOpnd = IR::RegOpnd::New(StackSym::New(opnd->GetType(), instr->m_func), opnd->GetType(), instr->m_func);
        IR::Instr * instrNew = LowererMD::CreateAssign(regOpnd, opnd, instr);

        IR::IntConstOpnd * cookieOpnd = BuildCookieOpnd(opnd->GetType(), instr->m_func);

        instrNew = IR::Instr::New(LowererMD::MDXorOpcode, regOpnd, regOpnd, cookieOpnd, instr->m_func);
        instr->InsertBefore(instrNew);
        LowererMD::Legalize(instrNew);

        StackSym * stackSym = regOpnd->m_sym;
        Assert(!stackSym->m_isSingleDef);
        Assert(stackSym->m_instrDef == nullptr);
        stackSym->m_isEncodedConstant = true;
        stackSym->constantValue = (uint32)constValue;

        *pNewOpnd = regOpnd;

        uint32 value = (uint32)constValue;
        value = value ^ cookieOpnd->AsUint32();
        return (IntConstType)value;
    }
    else
    {
#if TARGET_64
        return this->EncodeAddress(instr, opnd, constValue, pNewOpnd);
#else
        Assert(false);
        // (Prefast warning on failure to assign *pNewOpnd.)
        *pNewOpnd = nullptr;
        return 0;
#endif
    }
}

#if TARGET_64
size_t
Security::EncodeAddress(IR::Instr * instr, IR::Opnd *opnd, size_t value, _Out_ IR::RegOpnd **pNewOpnd)
{
    IR::Instr   *instrNew = nullptr;
    IR::RegOpnd *regOpnd = IR::RegOpnd::New(TyMachReg, instr->m_func);

    instrNew = LowererMD::CreateAssign(regOpnd, opnd, instr);

    IR::IntConstOpnd *cookieOpnd = BuildCookieOpnd(TyMachReg, instr->m_func);
    instrNew = IR::Instr::New(LowererMD::MDXorOpcode, regOpnd, regOpnd, cookieOpnd, instr->m_func);
    instr->InsertBefore(instrNew);
    LowererMD::Legalize(instrNew);

    StackSym * stackSym = regOpnd->m_sym;
    Assert(!stackSym->m_isSingleDef);
    Assert(stackSym->m_instrDef == nullptr);
    stackSym->m_isEncodedConstant = true;
    stackSym->constantValue = value;

    *pNewOpnd = regOpnd;
    return value ^ cookieOpnd->GetValue();
}
#endif
