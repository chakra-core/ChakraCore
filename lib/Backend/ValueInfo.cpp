//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

bool ValueInfo::HasIntConstantValue(const bool includeLikelyInt) const
{
    int32 constantValue;
    return TryGetIntConstantValue(&constantValue, includeLikelyInt);
}

bool ValueInfo::TryGetInt64ConstantValue(int64 *const intValueRef, const bool isUnsigned) const
{
    Assert(intValueRef);
    if (TryGetIntConstantValue(intValueRef, isUnsigned))
    {
        return true;
    }
    else
    {
        int32 int32ValueRef;
        if (TryGetIntConstantValue(&int32ValueRef, false))
        {
            if (isUnsigned)
            {
                *intValueRef = (uint)int32ValueRef;
            }
            else
            {
                *intValueRef = int32ValueRef;
            }
            return true;
        }
    }
    return false;
}

bool ValueInfo::TryGetIntConstantValue(int64 *const intValueRef, const bool isUnsigned) const
{
    Assert(intValueRef);
    if (structureKind == ValueStructureKind::Int64Constant)
    {
        *intValueRef = AsInt64Constant()->IntValue();
        return true;
    }
    return false;
}

bool ValueInfo::TryGetIntConstantValue(int32 *const intValueRef, const bool includeLikelyInt) const
{
    Assert(intValueRef);
    if (!(includeLikelyInt ? IsLikelyInt() : IsInt()))
    {
        return false;
    }

    switch (structureKind)
    {
    case ValueStructureKind::IntConstant:
        if (!includeLikelyInt || IsInt())
        {
            *intValueRef = AsIntConstant()->IntValue();
            return true;
        }
        break;

    case ValueStructureKind::IntRange:
        Assert(includeLikelyInt && !IsInt() || !AsIntRange()->IsConstant());
        break;

    case ValueStructureKind::IntBounded:
    {
        const IntConstantBounds bounds(AsIntBounded()->Bounds()->ConstantBounds());
        if (bounds.IsConstant())
        {
            *intValueRef = bounds.LowerBound();
            return true;
        }
        break;
    }
    }
    return false;
}

bool ValueInfo::TryGetIntConstantLowerBound(int32 *const intConstantBoundRef, const bool includeLikelyInt) const
{
    Assert(intConstantBoundRef);

    if (!(includeLikelyInt ? IsLikelyInt() : IsInt()))
    {
        return false;
    }

    switch (structureKind)
    {
    case ValueStructureKind::IntConstant:
        if (!includeLikelyInt || IsInt())
        {
            *intConstantBoundRef = AsIntConstant()->IntValue();
            return true;
        }
        break;

    case ValueStructureKind::IntRange:
        if (!includeLikelyInt || IsInt())
        {
            *intConstantBoundRef = AsIntRange()->LowerBound();
            return true;
        }
        break;

    case ValueStructureKind::IntBounded:
        *intConstantBoundRef = AsIntBounded()->Bounds()->ConstantLowerBound();
        return true;
    }

    *intConstantBoundRef = IsTaggedInt() ? Js::Constants::Int31MinValue : IntConstMin;
    return true;
}

bool ValueInfo::TryGetIntConstantUpperBound(int32 *const intConstantBoundRef, const bool includeLikelyInt) const
{
    Assert(intConstantBoundRef);

    if (!(includeLikelyInt ? IsLikelyInt() : IsInt()))
    {
        return false;
    }

    switch (structureKind)
    {
    case ValueStructureKind::IntConstant:
        if (!includeLikelyInt || IsInt())
        {
            *intConstantBoundRef = AsIntConstant()->IntValue();
            return true;
        }
        break;

    case ValueStructureKind::IntRange:
        if (!includeLikelyInt || IsInt())
        {
            *intConstantBoundRef = AsIntRange()->UpperBound();
            return true;
        }
        break;

    case ValueStructureKind::IntBounded:
        *intConstantBoundRef = AsIntBounded()->Bounds()->ConstantUpperBound();
        return true;
    }

    *intConstantBoundRef = IsTaggedInt() ? Js::Constants::Int31MaxValue : IntConstMax;
    return true;
}

bool ValueInfo::TryGetIntConstantBounds(IntConstantBounds *const intConstantBoundsRef, const bool includeLikelyInt) const
{
    Assert(intConstantBoundsRef);

    if (!(includeLikelyInt ? IsLikelyInt() : IsInt()))
    {
        return false;
    }

    switch (structureKind)
    {
    case ValueStructureKind::IntConstant:
        if (!includeLikelyInt || IsInt())
        {
            const int32 intValue = AsIntConstant()->IntValue();
            *intConstantBoundsRef = IntConstantBounds(intValue, intValue);
            return true;
        }
        break;

    case ValueStructureKind::IntRange:
        if (!includeLikelyInt || IsInt())
        {
            *intConstantBoundsRef = *AsIntRange();
            return true;
        }
        break;

    case ValueStructureKind::IntBounded:
        *intConstantBoundsRef = AsIntBounded()->Bounds()->ConstantBounds();
        return true;
    }

    *intConstantBoundsRef =
        IsTaggedInt()
        ? IntConstantBounds(Js::Constants::Int31MinValue, Js::Constants::Int31MaxValue)
        : IntConstantBounds(INT32_MIN, INT32_MAX);
    return true;
}

bool ValueInfo::WasNegativeZeroPreventedByBailout() const
{
    if (!IsInt())
    {
        return false;
    }

    switch (structureKind)
    {
    case ValueStructureKind::IntRange:
        return AsIntRange()->WasNegativeZeroPreventedByBailout();

    case ValueStructureKind::IntBounded:
        return AsIntBounded()->WasNegativeZeroPreventedByBailout();
    }
    return false;
}

bool ValueInfo::IsEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    const bool result =
        IsEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2) ||
        IsEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1);
    Assert(!result || !IsNotEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2));
    Assert(!result || !IsNotEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1));
    return result;
}

bool ValueInfo::IsNotEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    const bool result =
        IsNotEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2) ||
        IsNotEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1);
    Assert(!result || !IsEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2));
    Assert(!result || !IsEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1));
    return result;
}

bool ValueInfo::IsEqualTo_NoConverse(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return
        IsGreaterThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2) &&
        IsLessThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2);
}

bool ValueInfo::IsNotEqualTo_NoConverse(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return
        IsGreaterThan(src1Value, min1, max1, src2Value, min2, max2) ||
        IsLessThan(src1Value, min1, max1, src2Value, min2, max2);
}

bool ValueInfo::IsGreaterThanOrEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return IsGreaterThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2, 0);
}

bool ValueInfo::IsGreaterThan(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return IsGreaterThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2, 1);
}

bool ValueInfo::IsLessThanOrEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return IsLessThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2, 0);
}

bool ValueInfo::IsLessThan(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2)
{
    return IsLessThanOrEqualTo(src1Value, min1, max1, src2Value, min2, max2, -1);
}

bool ValueInfo::IsGreaterThanOrEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2,
    const int src2Offset)
{
    return
        IsGreaterThanOrEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2, src2Offset) ||
        src2Offset == IntConstMin ||
        IsLessThanOrEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1, -src2Offset);
}

bool ValueInfo::IsLessThanOrEqualTo(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2,
    const int src2Offset)
{
    return
        IsLessThanOrEqualTo_NoConverse(src1Value, min1, max1, src2Value, min2, max2, src2Offset) ||
        (
            src2Offset != IntConstMin &&
            IsGreaterThanOrEqualTo_NoConverse(src2Value, min2, max2, src1Value, min1, max1, -src2Offset)
            );
}

bool ValueInfo::IsGreaterThanOrEqualTo_NoConverse(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2,
    const int src2Offset)
{
    Assert(src1Value || min1 == max1);
    Assert(!src1Value || src1Value->GetValueInfo()->IsLikelyInt());
    Assert(src2Value || min2 == max2);
    Assert(!src2Value || src2Value->GetValueInfo()->IsLikelyInt());

    if (src1Value)
    {
        if (src2Value && src1Value->GetValueNumber() == src2Value->GetValueNumber())
        {
            return src2Offset <= 0;
        }

        ValueInfo const * const src1ValueInfo = src1Value->GetValueInfo();
        if (src1ValueInfo->structureKind == ValueStructureKind::IntBounded)
        {
            const IntBounds *const bounds = src1ValueInfo->AsIntBounded()->Bounds();
            return
                src2Value
                ? bounds->IsGreaterThanOrEqualTo(src2Value, src2Offset)
                : bounds->IsGreaterThanOrEqualTo(min2, src2Offset);
        }
    }
    return IntBounds::IsGreaterThanOrEqualTo(min1, max2, src2Offset);
}

bool ValueInfo::IsLessThanOrEqualTo_NoConverse(
    const Value *const src1Value,
    const int32 min1,
    const int32 max1,
    const Value *const src2Value,
    const int32 min2,
    const int32 max2,
    const int src2Offset)
{
    Assert(src1Value || min1 == max1);
    Assert(!src1Value || src1Value->GetValueInfo()->IsLikelyInt());
    Assert(src2Value || min2 == max2);
    Assert(!src2Value || src2Value->GetValueInfo()->IsLikelyInt());

    if (src1Value)
    {
        if (src2Value && src1Value->GetValueNumber() == src2Value->GetValueNumber())
        {
            return src2Offset >= 0;
        }

        ValueInfo const * const src1ValueInfo = src1Value->GetValueInfo();
        if (src1ValueInfo->structureKind == ValueStructureKind::IntBounded)
        {
            const IntBounds *const bounds = src1ValueInfo->AsIntBounded()->Bounds();
            return
                src2Value
                ? bounds->IsLessThanOrEqualTo(src2Value, src2Offset)
                : bounds->IsLessThanOrEqualTo(min2, src2Offset);
        }
    }
    return IntBounds::IsLessThanOrEqualTo(max1, min2, src2Offset);
}

bool
ValueInfo::IsGeneric() const
{
    return structureKind == ValueStructureKind::Generic;
}

bool
ValueInfo::IsIntConstant() const
{
    return IsInt() && structureKind == ValueStructureKind::IntConstant;
}

bool
ValueInfo::IsInt64Constant() const
{
    return IsInt() && structureKind == ValueStructureKind::Int64Constant;
}

const IntConstantValueInfo *
ValueInfo::AsIntConstant() const
{
    Assert(IsIntConstant());
    return static_cast<const IntConstantValueInfo *>(this);
}
const Int64ConstantValueInfo *
ValueInfo::AsInt64Constant() const
{
    Assert(IsInt64Constant());
    return static_cast<const Int64ConstantValueInfo *>(this);
}

bool
ValueInfo::IsIntRange() const
{
    return IsInt() && structureKind == ValueStructureKind::IntRange;
}

const IntRangeValueInfo *
ValueInfo::AsIntRange() const
{
    Assert(IsIntRange());
    return static_cast<const IntRangeValueInfo *>(this);
}

bool
ValueInfo::IsIntBounded() const
{
    const bool isIntBounded = IsLikelyInt() && structureKind == ValueStructureKind::IntBounded;

    // Bounds for definitely int values should have relative bounds, otherwise those values should use one of the other value
    // infos
    Assert(!isIntBounded || static_cast<const IntBoundedValueInfo *>(this)->Bounds()->RequiresIntBoundedValueInfo(Type()));

    return isIntBounded;
}

const IntBoundedValueInfo *
ValueInfo::AsIntBounded() const
{
    Assert(IsIntBounded());
    return static_cast<const IntBoundedValueInfo *>(this);
}

bool
ValueInfo::IsFloatConstant() const
{
    return IsFloat() && structureKind == ValueStructureKind::FloatConstant;
}

FloatConstantValueInfo *
ValueInfo::AsFloatConstant()
{
    Assert(IsFloatConstant());
    return static_cast<FloatConstantValueInfo *>(this);
}

const FloatConstantValueInfo *
ValueInfo::AsFloatConstant() const
{
    Assert(IsFloatConstant());
    return static_cast<const FloatConstantValueInfo *>(this);
}

bool
ValueInfo::IsVarConstant() const
{
    return structureKind == ValueStructureKind::VarConstant;
}

VarConstantValueInfo *
ValueInfo::AsVarConstant()
{
    Assert(IsVarConstant());
    return static_cast<VarConstantValueInfo *>(this);
}

bool
ValueInfo::IsJsType() const
{
    Assert(!(structureKind == ValueStructureKind::JsType && !IsUninitialized()));
    return structureKind == ValueStructureKind::JsType;
}

JsTypeValueInfo *
ValueInfo::AsJsType()
{
    Assert(IsJsType());
    return static_cast<JsTypeValueInfo *>(this);
}

const JsTypeValueInfo *
ValueInfo::AsJsType() const
{
    Assert(IsJsType());
    return static_cast<const JsTypeValueInfo *>(this);
}

bool
ValueInfo::IsArrayValueInfo() const
{
    return IsAnyOptimizedArray() && structureKind == ValueStructureKind::Array;
}

const ArrayValueInfo *
ValueInfo::AsArrayValueInfo() const
{
    Assert(IsArrayValueInfo());
    return static_cast<const ArrayValueInfo *>(this);
}

ArrayValueInfo *
ValueInfo::AsArrayValueInfo()
{
    Assert(IsArrayValueInfo());
    return static_cast<ArrayValueInfo *>(this);
}

ValueInfo *
ValueInfo::SpecializeToInt32(JitArenaAllocator *const allocator, const bool isForLoopBackEdgeCompensation)
{
    // Int specialization in some uncommon loop cases involving dependencies, needs to allow specializing values of arbitrary
    // types, even values that are definitely not int, to compensate for aggressive assumptions made by a loop prepass. In all
    // other cases, only values that are likely int may be int-specialized.
    Assert(IsUninitialized() || IsLikelyInt() || isForLoopBackEdgeCompensation);

    if (IsInt())
    {
        return this;
    }

    if (!IsIntBounded())
    {
        ValueInfo *const newValueInfo = CopyWithGenericStructureKind(allocator);
        newValueInfo->Type() = ValueType::GetInt(true);
        return newValueInfo;
    }

    const IntBoundedValueInfo *const boundedValueInfo = AsIntBounded();
    const IntBounds *const bounds = boundedValueInfo->Bounds();
    const IntConstantBounds constantBounds = bounds->ConstantBounds();
    if (bounds->RequiresIntBoundedValueInfo())
    {
        IntBoundedValueInfo *const newValueInfo = boundedValueInfo->Copy(allocator);
        newValueInfo->Type() = constantBounds.GetValueType();
        return newValueInfo;
    }

    ValueInfo *const newValueInfo =
        constantBounds.IsConstant()
        ? static_cast<ValueInfo *>(IntConstantValueInfo::New(allocator, constantBounds.LowerBound()))
        : IntRangeValueInfo::New(allocator, constantBounds.LowerBound(), constantBounds.UpperBound(), false);
    newValueInfo->SetSymStore(GetSymStore());
    return newValueInfo;
}

ValueInfo *
ValueInfo::SpecializeToFloat64(JitArenaAllocator *const allocator)
{
    if (IsNumber())
    {
        return this;
    }

    ValueInfo *const newValueInfo = CopyWithGenericStructureKind(allocator);

    // If the value type was likely int, after float-specializing, it's preferable to use Int_Number rather than Float, as the
    // former is also likely int and allows int specialization later.
    newValueInfo->Type() = IsLikelyInt() ? Type().ToDefiniteAnyNumber() : Type().ToDefiniteAnyFloat();

    return newValueInfo;
}

// SIMD_JS
ValueInfo *
ValueInfo::SpecializeToSimd128(IRType type, JitArenaAllocator *const allocator)
{
    switch (type)
    {
    case TySimd128F4:
        return SpecializeToSimd128F4(allocator);
    case TySimd128I4:
        return SpecializeToSimd128I4(allocator);
    default:
        Assert(UNREACHED);
        return nullptr;
    }

}

ValueInfo *
ValueInfo::SpecializeToSimd128F4(JitArenaAllocator *const allocator)
{
    if (IsSimd128Float32x4())
    {
        return this;
    }

    ValueInfo *const newValueInfo = CopyWithGenericStructureKind(allocator);

    newValueInfo->Type() = ValueType::GetSimd128(ObjectType::Simd128Float32x4);

    return newValueInfo;
}

ValueInfo *
ValueInfo::SpecializeToSimd128I4(JitArenaAllocator *const allocator)
{
    if (IsSimd128Int32x4())
    {
        return this;
    }

    ValueInfo *const newValueInfo = CopyWithGenericStructureKind(allocator);

    newValueInfo->Type() = ValueType::GetSimd128(ObjectType::Simd128Int32x4);

    return newValueInfo;
}

bool
ValueInfo::GetIsShared() const
{
    return IsJsType() ? AsJsType()->GetIsShared() : false;
}

void
ValueInfo::SetIsShared()
{
    if (IsJsType()) AsJsType()->SetIsShared();
}

ValueInfo *
ValueInfo::Copy(JitArenaAllocator * allocator)
{
    if (IsIntConstant())
    {
        return AsIntConstant()->Copy(allocator);
    }
    if (IsIntRange())
    {
        return AsIntRange()->Copy(allocator);
    }
    if (IsIntBounded())
    {
        return AsIntBounded()->Copy(allocator);
    }
    if (IsFloatConstant())
    {
        return AsFloatConstant()->Copy(allocator);
    }
    if (IsJsType())
    {
        return AsJsType()->Copy(allocator);
    }
    if (IsArrayValueInfo())
    {
        return AsArrayValueInfo()->Copy(allocator);
    }
    return CopyWithGenericStructureKind(allocator);
}

bool
ValueInfo::GetIntValMinMax(int *pMin, int *pMax, bool doAggressiveIntTypeSpec)
{
    IntConstantBounds intConstantBounds;
    if (TryGetIntConstantBounds(&intConstantBounds, doAggressiveIntTypeSpec))
    {
        *pMin = intConstantBounds.LowerBound();
        *pMax = intConstantBounds.UpperBound();
        return true;
    }

    Assert(!IsInt());
    Assert(!doAggressiveIntTypeSpec || !IsLikelyInt());
    return false;
}

ValueInfo *
ValueInfo::MergeLikelyIntValueInfo(JitArenaAllocator* alloc, Value *toDataVal, Value *fromDataVal, ValueType const newValueType)
{
    Assert(newValueType.IsLikelyInt());

    ValueInfo *const toDataValueInfo = toDataVal->GetValueInfo();
    ValueInfo *const fromDataValueInfo = fromDataVal->GetValueInfo();
    Assert(toDataValueInfo != fromDataValueInfo);

    bool wasNegativeZeroPreventedByBailout;
    if(newValueType.IsInt())
    {
        int32 toDataIntConstantValue, fromDataIntConstantValue;
        if (toDataValueInfo->TryGetIntConstantValue(&toDataIntConstantValue) &&
            fromDataValueInfo->TryGetIntConstantValue(&fromDataIntConstantValue) &&
            toDataIntConstantValue == fromDataIntConstantValue)
        {
            // A new value number must be created to register the fact that the value has changed. Otherwise, if the value
            // changed inside a loop, the sym may look invariant on the loop back-edge (and hence not turned into a number
            // value), and its constant value from the first iteration may be incorrectly propagated after the loop.
            return IntConstantValueInfo::New(alloc, toDataIntConstantValue);
        }

        wasNegativeZeroPreventedByBailout =
            toDataValueInfo->WasNegativeZeroPreventedByBailout() ||
            fromDataValueInfo->WasNegativeZeroPreventedByBailout();
    }
    else
    {
        wasNegativeZeroPreventedByBailout = false;
    }

    const IntBounds *const toDataValBounds =
        toDataValueInfo->IsIntBounded() ? toDataValueInfo->AsIntBounded()->Bounds() : nullptr;
    const IntBounds *const fromDataValBounds =
        fromDataValueInfo->IsIntBounded() ? fromDataValueInfo->AsIntBounded()->Bounds() : nullptr;
    if(toDataValBounds || fromDataValBounds)
    {
        const IntBounds *mergedBounds;
        if(toDataValBounds && fromDataValBounds)
        {
            mergedBounds = IntBounds::Merge(toDataVal, toDataValBounds, fromDataVal, fromDataValBounds);
        }
        else
        {
            IntConstantBounds constantBounds;
            if(toDataValBounds)
            {
                mergedBounds =
                    fromDataValueInfo->TryGetIntConstantBounds(&constantBounds, true)
                        ? IntBounds::Merge(toDataVal, toDataValBounds, fromDataVal, constantBounds)
                        : nullptr;
            }
            else
            {
                Assert(fromDataValBounds);
                mergedBounds =
                    toDataValueInfo->TryGetIntConstantBounds(&constantBounds, true)
                        ? IntBounds::Merge(fromDataVal, fromDataValBounds, toDataVal, constantBounds)
                        : nullptr;
            }
        }

        if(mergedBounds)
        {
            if(mergedBounds->RequiresIntBoundedValueInfo(newValueType))
            {
                return IntBoundedValueInfo::New(newValueType, mergedBounds, wasNegativeZeroPreventedByBailout, alloc);
            }
            mergedBounds->Delete();
        }
    }

    if(newValueType.IsInt())
    {
        int32 min1, max1, min2, max2;
        toDataValueInfo->GetIntValMinMax(&min1, &max1, false);
        fromDataValueInfo->GetIntValMinMax(&min2, &max2, false);
        return ValueInfo::NewIntRangeValueInfo(alloc, min(min1, min2), max(max1, max2), wasNegativeZeroPreventedByBailout);
    }

    return ValueInfo::New(alloc, newValueType);
}

ValueInfo * ValueInfo::NewIntRangeValueInfo(JitArenaAllocator * alloc, int32 min, int32 max, bool wasNegativeZeroPreventedByBailout)
{
    if (min == max)
    {
        // Since int constant values are const-propped, negative zero tracking does not track them, and so it's okay to ignore
        // 'wasNegativeZeroPreventedByBailout'
        return IntConstantValueInfo::New(alloc, max);
    }

    return IntRangeValueInfo::New(alloc, min, max, wasNegativeZeroPreventedByBailout);
}

#if DBG_DUMP
void ValueInfo::Dump()
{
    if (!IsJsType()) // The value type is uninitialized for a type value
    {
        char typeStr[VALUE_TYPE_MAX_STRING_SIZE];
        Type().ToString(typeStr);
        Output::Print(_u("%S"), typeStr);
    }

    IntConstantBounds intConstantBounds;
    if (TryGetIntConstantBounds(&intConstantBounds))
    {
        if (intConstantBounds.IsConstant())
        {
            Output::Print(_u(" constant:%d"), intConstantBounds.LowerBound());
            return;
        }
        Output::Print(_u(" range:%d - %d"), intConstantBounds.LowerBound(), intConstantBounds.UpperBound());
    }
    else if (IsFloatConstant())
    {
        Output::Print(_u(" constant:%g"), AsFloatConstant()->FloatValue());
    }
    else if (IsJsType())
    {
        const JITTypeHolder type(AsJsType()->GetJsType());
        type != nullptr ? Output::Print(_u("type: 0x%p, "), type->GetAddr()) : Output::Print(_u("type: null, "));
        Output::Print(_u("type Set: "));
        Js::EquivalentTypeSet* typeSet = AsJsType()->GetJsTypeSet();
        if (typeSet != nullptr)
        {
            uint16 typeCount = typeSet->GetCount();
            for (uint16 ti = 0; ti < typeCount - 1; ti++)
            {
                Output::Print(_u("0x%p, "), typeSet->GetType(ti));
            }
            Output::Print(_u("0x%p"), typeSet->GetType(typeCount - 1));
        }
        else
        {
            Output::Print(_u("null"));
        }
    }
    else if (IsArrayValueInfo())
    {
        const ArrayValueInfo *const arrayValueInfo = AsArrayValueInfo();
        if (arrayValueInfo->HeadSegmentSym())
        {
            Output::Print(_u(" seg: "));
            arrayValueInfo->HeadSegmentSym()->Dump();
        }
        if (arrayValueInfo->HeadSegmentLengthSym())
        {
            Output::Print(_u(" segLen: "));
            arrayValueInfo->HeadSegmentLengthSym()->Dump();
        }
        if (arrayValueInfo->LengthSym())
        {
            Output::Print(_u(" len: "));
            arrayValueInfo->LengthSym()->Dump();
        }
    }

    if (this->GetSymStore())
    {
        Output::Print(_u("\t\tsym:"));
        this->GetSymStore()->Dump();
    }
}
#endif
