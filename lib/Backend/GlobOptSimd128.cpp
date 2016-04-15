//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// SIMD_JS
// GlobOpt bits related to SIMD.js

#include "Backend.h"

/*
Handles all Simd128 type-spec of an instr, if possible.
*/
bool
GlobOpt::TypeSpecializeSimd128(
IR::Instr *instr,
Value **pSrc1Val,
Value **pSrc2Val,
Value **pDstVal
)
{
    if (this->GetIsAsmJSFunc() || SIMD128_TYPE_SPEC_FLAG == false)
    {
        // no type-spec for ASMJS code or flag is off.
        return false;
    }

    bool lossy = true;

    switch (instr->m_opcode)
    {
    case Js::OpCode::ArgOut_A_InlineBuiltIn:
        if (instr->GetSrc1()->IsRegOpnd())
        {
            StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
            if (IsSimd128TypeSpecialized(sym, this->currentBlock))
            {
                ValueType valueType = (*pSrc1Val)->GetValueInfo()->Type();
                Assert(valueType.IsSimd128());
                ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, *pSrc1Val, nullptr, GetIRTypeFromValueType(valueType), GetBailOutKindFromValueType(valueType));

                return true;
            }
        }
        return false;

    case Js::OpCode::Ld_A:
        if (instr->GetSrc1()->IsRegOpnd())
        {
            StackSym *sym = instr->GetSrc1()->AsRegOpnd()->m_sym;
            IRType type = TyIllegal;

#define SIMD_TYPE(_TAG_) \
            if (IsSimd128TypeSpecialized<TySimd128##_TAG_##>(sym, this->currentBlock))\
            {\
                type = TySimd128##_TAG_##;\
            } else
            SIMD_EXPAND_W_TAG(SIMD_TYPE)
            {
                return false;
            }
#undef SIMD_TYPE

            ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, *pSrc1Val, nullptr, type, IR::BailOutSimd128F4Only /*not used for Ld_A*/);
            TypeSpecializeSimd128Dst(type, instr, *pSrc1Val, *pSrc1Val, pDstVal);
            return true;
        }

        return false;

    case Js::OpCode::ExtendArg_A:

        if (Simd128DoTypeSpec(instr, *pSrc1Val, *pSrc2Val, *pDstVal))
        {
            Assert(instr->m_opcode == Js::OpCode::ExtendArg_A);
            Assert(instr->GetDst()->GetType() == TyVar);
            ValueType valueType = instr->GetDst()->GetValueType();

            // Type-spec src1 only based on dst type. Dst type is set by the inliner based on func signature.
            ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, *pSrc1Val, nullptr, GetIRTypeFromValueType(valueType), GetBailOutKindFromValueType(valueType), lossy);
            ToVarRegOpnd(instr->GetDst()->AsRegOpnd(), this->currentBlock);
            return true;
        }
        return false;
    }

    if (!Js::IsSimd128Opcode(instr->m_opcode))
    {
        return false;
    }

    // Simd instr
    if (Simd128DoTypeSpec(instr, *pSrc1Val, *pSrc2Val, *pDstVal))
    {
        ThreadContext::SimdFuncSignature simdFuncSignature;
        
        instr->m_func->GetScriptContext()->GetThreadContext()->GetSimdFuncSignatureFromOpcode(instr->m_opcode, simdFuncSignature);
        // type-spec logic

        // special handling for load/store
        // OptArraySrc will type-spec the array and the index. We type-spec the value here.
        if (Js::IsSimd128Load(instr->m_opcode))
        {
            TypeSpecializeSimd128Dst(GetIRTypeFromValueType(simdFuncSignature.returnType), instr, nullptr, *pSrc1Val, pDstVal);
            Simd128SetIndirOpndType(instr->GetSrc1()->AsIndirOpnd(), instr->m_opcode);
            return true;
        }
        if (Js::IsSimd128Store(instr->m_opcode))
        {
            ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, *pSrc1Val, nullptr, GetIRTypeFromValueType(simdFuncSignature.args[2]), GetBailOutKindFromValueType(simdFuncSignature.args[2]), lossy);
            Simd128SetIndirOpndType(instr->GetDst()->AsIndirOpnd(), instr->m_opcode);
            return true;
        }

        // For op with ExtendArg. All sources are already type-specialized, just type-specialize dst
        if (simdFuncSignature.argCount <= 2)
        {
            Assert(instr->GetSrc1());

            ToTypeSpecUse(instr, instr->GetSrc1(), this->currentBlock, *pSrc1Val, nullptr, GetIRTypeFromValueType(simdFuncSignature.args[0]), GetBailOutKindFromValueType(simdFuncSignature.args[0]), lossy);

            if (instr->GetSrc2())
            {
                ToTypeSpecUse(instr, instr->GetSrc2(), this->currentBlock, *pSrc2Val, nullptr, GetIRTypeFromValueType(simdFuncSignature.args[1]), GetBailOutKindFromValueType(simdFuncSignature.args[1]), lossy);
            }
        }

        if (instr->GetDst())
        {
            IRType dstType = GetIRTypeFromValueType(simdFuncSignature.returnType);
            if (IRType_IsSimd128(dstType))
            {
                TypeSpecializeSimd128Dst(dstType, instr, nullptr, *pSrc1Val, pDstVal);
            }
            else if (IRType_IsFloat(dstType))
            {
                // ExtractLane
                Assert(instr->m_opcode == Js::OpCode::Simd128_ExtractLane_F4);
                TypeSpecializeFloatDst(instr, nullptr, *pSrc1Val, *pSrc2Val, pDstVal); // Float64, will be cast to Float32 in Lowerer. 
            }
            else if (IRType_IsNativeInt(dstType))
            {
                // ExtractLane
                Assert(
                    instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I4 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_U4 ||
                    instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I8 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_U8 ||
                    instr->m_opcode == Js::OpCode::Simd128_ExtractLane_I16 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_U16
                    );
                // ToDo: Refine Int range info based on which SIMD int type we are working on
                TypeSpecializeIntDst(instr, instr->m_opcode, nullptr, *pSrc1Val, *pSrc2Val, IR::BailOutKind::BailOutInvalid /*unused*/, ValueType::Int, INT32_MIN, INT32_MAX, pDstVal);
            }
            else
            {
                Assert(dstType == TyVar && simdFuncSignature.returnType == ValueType::Boolean);
                // ExtractLane
                Assert(instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B4 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B8 || instr->m_opcode == Js::OpCode::Simd128_ExtractLane_B16);
                // no dst type-spec for bool return type, lowerer will make sure to box to Bool
                ToVarRegOpnd(instr->GetDst()->AsRegOpnd(), this->currentBlock);
            }
        }
        return true;
    }
    else
    {
        // We didn't type-spec
        if (!IsLoopPrePass())
        {
            // Emit bailout if not loop prepass.
            // The inliner inserts bytecodeUses of original args after the instruction. Bailout is safe.
            // We should end up with
            // InlineBuiltInStart
            // NOP
            // BailOutOnNoSimdTypeSpec
            // InlineBuiltInEnd
            IR::Instr * bailoutInstr = IR::BailOutInstr::New(Js::OpCode::BailOnNoSimdTypeSpec, IR::BailOutNoSimdTypeSpec, instr, instr->m_func);
            IR::Instr * builtInEnd = nullptr;

            bailoutInstr->SetByteCodeOffset(instr);
            instr->InsertAfter(bailoutInstr);

            instr->m_opcode = Js::OpCode::Nop;
            if (instr->GetSrc1())
            {
                instr->FreeSrc1();
                if (instr->GetSrc2())
                {
                    instr->FreeSrc2();
                }
            }
            if (instr->GetDst())
            {
                instr->FreeDst();
            }
            builtInEnd = instr;
            while (builtInEnd->m_opcode != Js::OpCode::InlineBuiltInEnd)
            {
                builtInEnd = builtInEnd->m_next;
            }
            builtInEnd->Unlink();

            if (this->byteCodeUses)
            {
                // All inlined SIMD ops have jitOptimizedReg srcs
                Assert(this->byteCodeUses->IsEmpty());
                JitAdelete(this->alloc, this->byteCodeUses);
                this->byteCodeUses = nullptr;
            }
            RemoveCodeAfterNoFallthroughInstr(bailoutInstr);
            bailoutInstr->InsertAfter(builtInEnd);
            return true;
        }
    }
    return false;
}

bool
GlobOpt::Simd128DoTypeSpec(IR::Instr *instr, const Value *src1Val, const Value *src2Val, const Value *dstVal)
{
    bool doTypeSpec = true;

    // TODO: Some operations require additional opnd constraints (e.g. shuffle/swizzle).
    if (Js::IsSimd128Opcode(instr->m_opcode))
    {
        ThreadContext::SimdFuncSignature simdFuncSignature;
        instr->m_func->GetScriptContext()->GetThreadContext()->GetSimdFuncSignatureFromOpcode(instr->m_opcode, simdFuncSignature);
        if (!simdFuncSignature.valid)
        {
            // not implemented yet.
            return false;
        }
        // special handling for Load/Store
        if (Js::IsSimd128Load(instr->m_opcode) || Js::IsSimd128Store(instr->m_opcode))
        {
            return Simd128DoTypeSpecLoadStore(instr, src1Val, src2Val, dstVal, &simdFuncSignature);
        }

        const uint argCount = simdFuncSignature.argCount;
        switch (argCount)
        {
        case 2:
            Assert(src2Val);
            doTypeSpec = doTypeSpec && Simd128CanTypeSpecOpnd(src2Val->GetValueInfo()->Type(), simdFuncSignature.args[1]) && Simd128ValidateIfLaneIndex(instr, instr->GetSrc2(), src2Val, 1);
            // fall-through
        case 1:
            Assert(src1Val);
            doTypeSpec = doTypeSpec && Simd128CanTypeSpecOpnd(src1Val->GetValueInfo()->Type(), simdFuncSignature.args[0]) && Simd128ValidateIfLaneIndex(instr, instr->GetSrc1(), src1Val, 0);
            break;
        default:
        {
            // extended args
            Assert(argCount > 2);
            // Check if all args have been type specialized.

            int arg = argCount - 1;
            IR::Instr * eaInstr = GetExtendedArg(instr);

            while (arg>=0)
            {
                Assert(eaInstr);
                Assert(eaInstr->m_opcode == Js::OpCode::ExtendArg_A);

                ValueType expectedType = simdFuncSignature.args[arg];
                IR::Opnd * opnd = eaInstr->GetSrc1();
                StackSym * sym = opnd->GetStackSym();

                // In Forward Prepass: Check liveness through liveness bits, not IR type, since in prepass no actual type-spec happens.
                // In Forward Pass: Check IRType since Sym can be null, because of const prop.
#define SIMD_DO_TYPESPEC(_NAME_,_TAG_)\
                if (expectedType.IsSimd128##_NAME_##())\
                {\
                if (sym && !IsSimd128TypeSpecialized<TySimd128##_TAG_##>(sym, &currentBlock->globOptData) ||\
                        !sym && opnd->GetType() != TySimd128##_TAG_##)\
                    {\
                        return false;\
                    }\
                } else
                SIMD_EXPAND_W_NAME(SIMD_DO_TYPESPEC)
#undef SIMD_DO_TYPESPEC
                if (expectedType.IsFloat())
                {
                    if (sym && !IsFloat64TypeSpecialized(sym, &currentBlock->globOptData) ||
                        !sym&& opnd->GetType() != TyFloat64)
                    {
                        return false;
                    }
                }
                else if (expectedType.IsInt())
                {
                    if ((sym && !IsInt32TypeSpecialized(sym, &currentBlock->globOptData) && !IsLossyInt32TypeSpecialized(sym, &currentBlock->globOptData)) ||
                        !sym && opnd->GetType() != TyInt32)
                    {
                        return false;
                    }
                    Value *srcVal = sym ? FindValue(sym) : nullptr;
                    // Extra check if arg is a lane index
                    if (!Simd128ValidateIfLaneIndex(instr, opnd, srcVal, arg, true /*extendedOpnd*/))
                    {
                        return false;
                    }
                }
                else
                {
                    Assert(UNREACHED);
                }

                eaInstr = GetExtendedArg(eaInstr);
                arg--;
            }
            // all args are type-spec'd
            doTypeSpec = true;
        }
        }
    }
    else
    {
        Assert(instr->m_opcode == Js::OpCode::ExtendArg_A);
        // For ExtendArg, the expected type is encoded in the dst(link) operand.
        doTypeSpec = doTypeSpec && Simd128CanTypeSpecOpnd(src1Val->GetValueInfo()->Type(), instr->GetDst()->GetValueType());
    }

    return doTypeSpec;
}

bool
GlobOpt::Simd128DoTypeSpecLoadStore(IR::Instr *instr, const Value *src1Val, const Value *src2Val, const Value *dstVal, const ThreadContext::SimdFuncSignature *simdFuncSignature)
{
    IR::Opnd *baseOpnd = nullptr, *indexOpnd = nullptr, *valueOpnd = nullptr;
    IR::Opnd *src, *dst;

    bool doTypeSpec = true;

    // value = Ld [arr + index]
    // [arr + index] = St value
    src = instr->GetSrc1();
    dst = instr->GetDst();
    Assert(dst && src && !instr->GetSrc2());

    if (Js::IsSimd128Load(instr->m_opcode))
    {
        Assert(src->IsIndirOpnd());
        baseOpnd = instr->GetSrc1()->AsIndirOpnd()->GetBaseOpnd();
        indexOpnd = instr->GetSrc1()->AsIndirOpnd()->GetIndexOpnd();
        valueOpnd = instr->GetDst();
    }
    else
    {
        Assert(Js::IsSimd128Store(instr->m_opcode));
        Assert(dst->IsIndirOpnd());
        baseOpnd = instr->GetDst()->AsIndirOpnd()->GetBaseOpnd();
        indexOpnd = instr->GetDst()->AsIndirOpnd()->GetIndexOpnd();
        valueOpnd = instr->GetSrc1();

        // St(arr, index, value). Make sure value can be Simd128 type-spec'd
        doTypeSpec = doTypeSpec && Simd128CanTypeSpecOpnd(FindValue(valueOpnd->AsRegOpnd()->m_sym)->GetValueInfo()->Type(), simdFuncSignature->args[2]);
    }

    if (!baseOpnd->IsRegOpnd() || !baseOpnd->AsRegOpnd()->IsArrayRegOpnd())
    {
        // won't type-spec unless ArrayCheck/Length hoisting happened. Lowerer expects ArrayRegOpnd
        // TODO: Enable with hoisting off ?
        return false;
    }
    // array and index operands should have been type-specialized in OptArraySrc: ValueTypes should be definite at this point. If not, don't type-spec.
    // We can be in a loop prepass, where opnd ValueInfo is not set yet. Get the ValueInfo from the Value Table instead.
    ValueType baseOpndType = FindValue(baseOpnd->AsRegOpnd()->m_sym)->GetValueInfo()->Type();
    
    if (IsLoopPrePass())
    {
        doTypeSpec = doTypeSpec && (baseOpndType.IsObject() && baseOpndType.IsTypedArray());
        // indexOpnd might be missing if loading from [0]
        if (indexOpnd != nullptr)
        {
            ValueType indexOpndType = FindValue(indexOpnd->AsRegOpnd()->m_sym)->GetValueInfo()->Type();
            doTypeSpec = doTypeSpec && indexOpndType.IsLikelyInt();
        }
    }
    else
    {
        doTypeSpec = doTypeSpec && (baseOpndType.IsObject() && baseOpndType.IsTypedArray());
        if (indexOpnd != nullptr)
        {
            ValueType indexOpndType = FindValue(indexOpnd->AsRegOpnd()->m_sym)->GetValueInfo()->Type();
            doTypeSpec = doTypeSpec && indexOpndType.IsInt();
        }
    }

    return doTypeSpec;
}


// We can type spec an opnd if:
// (1) Both profiled/propagated and expected types are not Simd128. e.g. expected type is f64/f32/i32 where there is a conversion logic from the incoming type.
// (2) Opnd type is (Likely) SIMD128 and matches expected type.
// (3) Opnd type is Object. e.g. possibly result of merging different SIMD types. We specialize because we don't know which pass is dynamically taken.
// For (2) and (3), note that we cannot just always type-spec of SIMD128 values. That's because we don't allow conversion from one Simd128 type to another.
// So if we have a var type-spec'ed as F4, and we need I4 type-spec, we cannot convert directly. Also, ToVar followed by FromVar is meaningless in this case since the FromVar is guaranteed to bailout.

bool GlobOpt::Simd128CanTypeSpecOpnd(const ValueType opndType, ValueType expectedType)
{
    // loads and store are not handled here. expectedType should either be a Simd128 type or a primitive
    // IsPrimitive includes IsSimd128()
    if (
        !expectedType.IsSimd128() && expectedType.IsPrimitive() &&
        !opndType.IsSimd128()
        )
    {
        // We will always do lossy int type-spec. We need lossy int type-spec enabled, if value is not primitive. Check GlobOpt::ToTypeSpecUse()
        return opndType.IsPrimitive() || DoLossyIntTypeSpec();
    }

    if (expectedType.IsSimd128())
    {
        // Expecting Simd type
        if (opndType.HasBeenNull() || opndType.HasBeenUndefined())
        {
            return false;
        }

        if (
            (opndType.IsLikelyObject() && opndType.ToDefiniteObject() == expectedType) ||
            (opndType.IsLikelyObject() && opndType.GetObjectType() == ObjectType::Object)
            )
        {
            return true;
        }
    }
    return false;
}

/*
Given an instr, opnd and the opnd position. Return true if opnd is a lane index and valid, or not a lane index all-together..
*/
bool GlobOpt::Simd128ValidateIfLaneIndex(const IR::Instr * instr, IR::Opnd * opnd, const Value *srcVal, uint argPos, bool extendedOpnd /* = false */)
{
    Assert(instr);
    Assert(opnd);

    uint laneIndex = (uint) -1;
    uint argPosLo, argPosHi;
    uint laneIndexLo, laneIndexHi;

    // operation takes a lane index ?
    switch (instr->m_opcode)
    {
    case Js::OpCode::Simd128_Swizzle_F4:
    case Js::OpCode::Simd128_Swizzle_I4:
    case Js::OpCode::Simd128_Swizzle_U4:
        argPosLo = 1; argPosHi = 4;
        laneIndexLo = 0; laneIndexHi = 3;
        break;
    case Js::OpCode::Simd128_Swizzle_I8:
    case Js::OpCode::Simd128_Swizzle_U8:
        argPosLo = 1; argPosHi = 8;
        laneIndexLo = 0; laneIndexHi = 7;
        break;
    case Js::OpCode::Simd128_Swizzle_I16:
    case Js::OpCode::Simd128_Swizzle_U16:
        argPosLo = 1; argPosHi = 16;
        laneIndexLo = 0; laneIndexHi = 15;
        break;
    case Js::OpCode::Simd128_Shuffle_F4:
    case Js::OpCode::Simd128_Shuffle_I4:
    case Js::OpCode::Simd128_Shuffle_U4:
        argPosLo = 2; argPosHi = 5;
        laneIndexLo = 0; laneIndexHi = 7;
        break;
    case Js::OpCode::Simd128_Shuffle_I8:
    case Js::OpCode::Simd128_Shuffle_U8:
        argPosLo = 2; argPosHi = 9;
        laneIndexLo = 0; laneIndexHi = 15;
        break;
    case Js::OpCode::Simd128_Shuffle_I16:
    case Js::OpCode::Simd128_Shuffle_U16:
        argPosLo = 2; argPosHi = 17;
        laneIndexLo = 0; laneIndexHi = 15;
        break;
    case Js::OpCode::Simd128_ReplaceLane_F4:
    case Js::OpCode::Simd128_ReplaceLane_I4:
    case Js::OpCode::Simd128_ReplaceLane_U4:
    case Js::OpCode::Simd128_ReplaceLane_B4:
    case Js::OpCode::Simd128_ExtractLane_F4:
    case Js::OpCode::Simd128_ExtractLane_I4:
    case Js::OpCode::Simd128_ExtractLane_U4:
    case Js::OpCode::Simd128_ExtractLane_B4:
        argPosLo = argPosHi = 1;
        laneIndexLo = 0;  laneIndexHi = 3;
        break;
    case Js::OpCode::Simd128_ReplaceLane_I8:
    case Js::OpCode::Simd128_ReplaceLane_U8:
    case Js::OpCode::Simd128_ReplaceLane_B8:
    case Js::OpCode::Simd128_ExtractLane_I8:
    case Js::OpCode::Simd128_ExtractLane_U8:
    case Js::OpCode::Simd128_ExtractLane_B8:
        argPosLo = argPosHi = 1;
        laneIndexLo = 0; laneIndexHi = 7;
        break;
    case Js::OpCode::Simd128_ReplaceLane_I16:
    case Js::OpCode::Simd128_ReplaceLane_U16:
    case Js::OpCode::Simd128_ReplaceLane_B16:
    case Js::OpCode::Simd128_ExtractLane_I16:
    case Js::OpCode::Simd128_ExtractLane_U16:
    case Js::OpCode::Simd128_ExtractLane_B16:
        argPosLo = argPosHi = 1;
        laneIndexLo = 0; laneIndexHi = 15;
        break;
    default:
        return true; // not a lane index
    }

    // arg in lane index pos of operation ?
    if (argPos < argPosLo || argPos > argPosHi)
    {
        return true; // not a lane index
    }

    // It is a lane index

    // Arg is Int constant (literal or const prop'd) ?
    
    if (IsLoopPrePass())
    {
        // LoopPrepass: Check valueInfo since no actual const prop happened.
        if (!(srcVal && srcVal->GetValueInfo()->TryGetIntConstantValue((int32*)&laneIndex)))
        {
            return false;
        }
    }
    else
    {
        // Actual pass: we must have a literal in some form at this point
        if (extendedOpnd)
        {
            // If we are looking backwards at ExtendedArgs, then the opnd should have been type-specizlied already. Check if it is IntConstOpnd
            if (!opnd->IsIntConstOpnd())
            {
                return false;
            }
            laneIndex = (uint)opnd->AsIntConstOpnd()->GetValue();
        }
        else
        {
            // If the opnd is inlined in the instruction, then it is not type-spec'ed yet. Check if it is AddrOpnd (Tagged Int value).
            if (!opnd->IsAddrOpnd())
            {
                return false;
            }
            laneIndex = Js::TaggedInt::ToInt32((Js::Var)opnd->AsAddrOpnd()->GetImmediateValue());
        }
    }

    // In range ?
    if (laneIndex < laneIndexLo|| laneIndex > laneIndexHi)
    {
        return false;
    }

    return true;
}

IR::Instr * GlobOpt::GetExtendedArg(IR::Instr *instr)
{
    IR::Opnd *src1, *src2;

    src1 = instr->GetSrc1();
    src2 = instr->GetSrc2();

    if (instr->m_opcode == Js::OpCode::ExtendArg_A)
    {
        if (src2)
        {
            // mid chain
            Assert(src2->GetStackSym()->IsSingleDef());
            return src2->GetStackSym()->GetInstrDef();
        }
        else
        {
            // end of chain
            return nullptr;
        }
    }
    else
    {
        // start of chain
        Assert(Js::IsSimd128Opcode(instr->m_opcode));
        Assert(src1);
        Assert(src1->GetStackSym()->IsSingleDef());
        return src1->GetStackSym()->GetInstrDef();
    }
}

IRType GlobOpt::GetIRTypeFromValueType(const ValueType &valueType)
{
    if (valueType.IsFloat())
    {
        return TyFloat64;
    }
    else if (valueType.IsInt())
    {
        return TyInt32;
    }
    else if (valueType.IsBoolean())
    {
        return TyVar; // no type-spec for bools
    }
#define SIMD_GET_TYPE(_NAME_,_TAG_)\
    else if (valueType.IsSimd128##_NAME_##())\
    {\
        return TySimd128##_TAG_##;\
    }
    SIMD_EXPAND_W_NAME(SIMD_GET_TYPE)
#undef SIMD_GET_TYPE
    else
    {
        Assert(UNREACHED);
        return TyVar;
    }
}

ValueType GlobOpt::GetValueTypeFromIRType(const IRType &type)
{
    switch (type)
    {
    case TyInt32:
        return ValueType::GetInt(false);
    case TyFloat64:
        return ValueType::Float;

#define SIMD_GET_TYPE(_NAME_,_TAG_) \
    case TySimd128##_TAG_##: \
        return ValueType::GetSimd128(ObjectType::Simd128##_NAME_##);
    SIMD_EXPAND_W_NAME(SIMD_GET_TYPE)
#undef SIMD_GET_TYPE
    default:
        Assert(UNREACHED);

    }
    return ValueType::UninitializedObject;

}

IR::BailOutKind GlobOpt::GetBailOutKindFromValueType(const ValueType &valueType)
{
    if (valueType.IsFloat())
    {
        // if required valueType is Float, then allow coercion from any primitive except String.
        return IR::BailOutPrimitiveButString;
    }
    else if (valueType.IsInt())
    {
        return IR::BailOutIntOnly;
    }
#define SIMD_GET_BAILOUT(_NAME_,_TAG_)\
    else if (valueType.IsSimd128##_NAME_##())\
    {\
        return IR::BailOutSimd128##_TAG_##Only;\
    }
    SIMD_EXPAND_W_NAME(SIMD_GET_BAILOUT)
#undef SIMD_GET_BAILOUT
    else
    {
        Assert(UNREACHED);
        return IR::BailOutInvalid;
    }
}

void
GlobOpt::UpdateBoundCheckHoistInfoForSimd(ArrayUpperBoundCheckHoistInfo &upperHoistInfo, ValueType arrValueType, const IR::Instr *instr)
{
    if (!upperHoistInfo.HasAnyInfo())
    {
        return;
    }

    int newOffset = GetBoundCheckOffsetForSimd(arrValueType, instr, upperHoistInfo.Offset());
    upperHoistInfo.UpdateOffset(newOffset);
}

int
GlobOpt::GetBoundCheckOffsetForSimd(ValueType arrValueType, const IR::Instr *instr, const int oldOffset /* = -1 */)
{
    if (!(Js::IsSimd128LoadStore(instr->m_opcode)))
    {
        return oldOffset;
    }

    if (!arrValueType.IsTypedArray())
    {
        // no need to adjust for other array types, we will not type-spec (see Simd128DoTypeSpecLoadStore)
        return oldOffset;
    }

    Assert(instr->dataWidth == 4 || instr->dataWidth == 8 || instr->dataWidth == 12 || instr->dataWidth == 16);

    int numOfElems = Lowerer::SimdGetElementCountFromBytes(arrValueType, instr->dataWidth);

    // we want to make bound checks more conservative. We compute how many extra elements we need to add to the bound check
    // e.g. if original bound check is value <= Length + offset, and dataWidth is 16 bytes on Float32 array, then we need room for 4 elements. The bound check guarantees room for 1 element.
    // Hence, we need to ensure 3 more: value <= Length + offset - 3
    // We round up since dataWidth may span a partial lane (e.g. dataWidth = 12, bpe = 8 bytes)

    int offsetBias = -(numOfElems - 1);
    // we should always make an existing bound-check more conservative.
    Assert(offsetBias <= 0);
    return oldOffset + offsetBias;
}

void
GlobOpt::Simd128SetIndirOpndType(IR::IndirOpnd *indirOpnd, Js::OpCode opcode)
{
    switch (opcode)
    {
#define SIMD_TYPE(_NAME_,_TAG_)\
    case Js::OpCode::Simd128_LdArr_##_TAG_##:\
    case Js::OpCode::Simd128_StArr_##_TAG_##:\
        indirOpnd->SetType(TySimd128##_TAG_##);\
        indirOpnd->SetValueType(ValueType::GetSimd128(ObjectType::Simd128##_NAME_##));\
        break;
        SIMD_EXPAND_W_NAME_NUM_ONLY(SIMD_TYPE)
#undef SIMD_TYPE
    
    default:
        Assert(UNREACHED);
    }
}
