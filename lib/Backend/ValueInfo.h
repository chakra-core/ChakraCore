//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

enum class ValueStructureKind
{
    Generic,
    IntConstant,
    Int64Constant,
    IntRange,
    IntBounded,
    FloatConstant,
    VarConstant,
    JsType,
    Array
};

class IntConstantValueInfo;
class Int64ConstantValueInfo;
class IntRangeValueInfo;
class IntBoundedValueInfo;
class FloatConstantValueInfo;
class VarConstantValueInfo;
class JsTypeValueInfo;
class EquivalentTypeSetValueInfo;
class ArrayValueInfo;

class ValueInfo : protected ValueType
{
private:
    const ValueStructureKind structureKind;
    Sym *                   symStore;

protected:
    ValueInfo(const ValueType type, const ValueStructureKind structureKind)
        : ValueType(type), structureKind(structureKind)
    {
        // We can only prove that the representation is a tagged int on a ToVar. Currently, we cannot have more than one value
        // info per value number in a block, so a value info specifying tagged int representation cannot be created for a
        // specific sym. Instead, a value info can be shared by multiple syms, and hence cannot specify tagged int
        // representation. Currently, the tagged int representation info can only be carried on the dst opnd of ToVar, and can't
        // even be propagated forward.
        Assert(!type.IsTaggedInt());

        SetSymStore(nullptr);
    }

private:
    ValueInfo(const ValueInfo &other, const bool)
        : ValueType(other), structureKind(ValueStructureKind::Generic) // uses generic structure kind, as opposed to copying the structure kind
    {
        SetSymStore(other.GetSymStore());
    }

public:
    static ValueInfo *          New(JitArenaAllocator *const alloc, const ValueType type)
    {
        return JitAnew(alloc, ValueInfo, type, ValueStructureKind::Generic);
    }
    static ValueInfo *      MergeLikelyIntValueInfo(JitArenaAllocator* alloc, Value *toDataVal, Value *fromDataVal, const ValueType newValueType);
    static ValueInfo *      NewIntRangeValueInfo(JitArenaAllocator* alloc, int32 min, int32 max, bool wasNegativeZeroPreventedByBailout);

    const ValueType &       Type() const { return *this; }
    ValueType &             Type() { return *this; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ValueType imports. Only importing functions that are appropriate to be called on Value.

public:
    using ValueType::IsUninitialized;
    using ValueType::IsDefinite;

    using ValueType::IsTaggedInt;
    using ValueType::IsIntAndLikelyTagged;
    using ValueType::IsLikelyTaggedInt;

    using ValueType::HasBeenUntaggedInt;
    using ValueType::IsIntAndLikelyUntagged;
    using ValueType::IsLikelyUntaggedInt;

    using ValueType::HasBeenInt;
    using ValueType::IsInt;
    using ValueType::IsLikelyInt;

    using ValueType::IsNotInt;
    using ValueType::IsNotNumber;

    using ValueType::HasBeenFloat;
    using ValueType::IsFloat;
    using ValueType::IsLikelyFloat;

    using ValueType::HasBeenNumber;
    using ValueType::IsNumber;
    using ValueType::IsLikelyNumber;

    using ValueType::HasBeenUnknownNumber;
    using ValueType::IsUnknownNumber;
    using ValueType::IsLikelyUnknownNumber;

    using ValueType::HasBeenUndefined;
    using ValueType::IsUndefined;
    using ValueType::IsLikelyUndefined;

    using ValueType::HasBeenNull;
    using ValueType::IsNull;
    using ValueType::IsLikelyNull;

    using ValueType::HasBeenBoolean;
    using ValueType::IsBoolean;
    using ValueType::IsLikelyBoolean;

    using ValueType::HasBeenString;
    using ValueType::IsString;
    using ValueType::HasHadStringTag;
    using ValueType::IsLikelyString;
    using ValueType::IsNotString;

    using ValueType::HasBeenPrimitive;
    using ValueType::IsPrimitive;
    using ValueType::IsLikelyPrimitive;

    using ValueType::HasBeenObject;
    using ValueType::IsObject;
    using ValueType::IsLikelyObject;
    using ValueType::IsNotObject;
    using ValueType::CanMergeToObject;
    using ValueType::CanMergeToSpecificObjectType;

    using ValueType::IsRegExp;
    using ValueType::IsLikelyRegExp;

    using ValueType::IsArray;
    using ValueType::IsLikelyArray;
    using ValueType::IsNotArray;

    using ValueType::IsArrayOrObjectWithArray;
    using ValueType::IsLikelyArrayOrObjectWithArray;
    using ValueType::IsNotArrayOrObjectWithArray;

    using ValueType::IsNativeArray;
    using ValueType::IsLikelyNativeArray;
    using ValueType::IsNotNativeArray;

    using ValueType::IsNativeIntArray;
    using ValueType::IsLikelyNativeIntArray;

    using ValueType::IsNativeFloatArray;
    using ValueType::IsLikelyNativeFloatArray;

    using ValueType::IsTypedArray;
    using ValueType::IsLikelyTypedArray;

    using ValueType::IsOptimizedTypedArray;
    using ValueType::IsLikelyOptimizedTypedArray;
    using ValueType::IsLikelyOptimizedVirtualTypedArray;

    using ValueType::IsAnyArrayWithNativeFloatValues;
    using ValueType::IsLikelyAnyArrayWithNativeFloatValues;

    using ValueType::IsAnyArray;
    using ValueType::IsLikelyAnyArray;

    using ValueType::IsAnyOptimizedArray;
    using ValueType::IsLikelyAnyOptimizedArray;

    // The following apply to object types only
    using ValueType::GetObjectType;

    // The following apply to javascript array types only
    using ValueType::HasNoMissingValues;
    using ValueType::HasIntElements;
    using ValueType::HasFloatElements;
    using ValueType::HasVarElements;

    using ValueType::IsSimd128;
    using ValueType::IsSimd128Float32x4;
    using ValueType::IsSimd128Int32x4;
    using ValueType::IsSimd128Float64x2;
    using ValueType::IsLikelySimd128;
    using ValueType::IsLikelySimd128Float32x4;
    using ValueType::IsLikelySimd128Int32x4;
    using ValueType::IsLikelySimd128Float64x2;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

public:
    bool                            IsGeneric() const;

private:
    bool                            IsIntConstant() const;
    bool                            IsInt64Constant() const;
    const IntConstantValueInfo *    AsIntConstant() const;
    const Int64ConstantValueInfo *  AsInt64Constant() const;
    bool                            IsIntRange() const;
    const IntRangeValueInfo *       AsIntRange() const;

public:
    bool                            IsIntBounded() const;
    const IntBoundedValueInfo *     AsIntBounded() const;
    bool                            IsFloatConstant() const;
    FloatConstantValueInfo *        AsFloatConstant();
    const FloatConstantValueInfo *  AsFloatConstant() const;
    bool                            IsVarConstant() const;
    VarConstantValueInfo *          AsVarConstant();
    bool                            IsJsType() const;
    JsTypeValueInfo *               AsJsType();
    const JsTypeValueInfo *         AsJsType() const;
#if FALSE
    bool                            IsObjectType() const;
    JsTypeValueInfo *               AsObjectType();
    bool                            IsEquivalentTypeSet() const;
    EquivalentTypeSetValueInfo *    AsEquivalentTypeSet();
#endif
    bool                            IsArrayValueInfo() const;
    const ArrayValueInfo *          AsArrayValueInfo() const;
    ArrayValueInfo *                AsArrayValueInfo();

public:
    bool HasIntConstantValue(const bool includeLikelyInt = false) const;
    bool TryGetIntConstantValue(int32 *const intValueRef, const bool includeLikelyInt = false) const;
    bool TryGetIntConstantValue(int64 *const intValueRef, const bool isUnsigned) const;
    bool TryGetInt64ConstantValue(int64 *const intValueRef, const bool isUnsigned) const;
    bool TryGetIntConstantLowerBound(int32 *const intConstantBoundRef, const bool includeLikelyInt = false) const;
    bool TryGetIntConstantUpperBound(int32 *const intConstantBoundRef, const bool includeLikelyInt = false) const;
    bool TryGetIntConstantBounds(IntConstantBounds *const intConstantBoundsRef, const bool includeLikelyInt = false) const;
    bool WasNegativeZeroPreventedByBailout() const;

public:
    static bool IsEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
    static bool IsNotEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
private:
    static bool IsEqualTo_NoConverse(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
    static bool IsNotEqualTo_NoConverse(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);

public:
    static bool IsGreaterThanOrEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
    static bool IsGreaterThan(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
    static bool IsLessThanOrEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);
    static bool IsLessThan(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2);

public:
    static bool IsGreaterThanOrEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2, const int src2Offset);
    static bool IsLessThanOrEqualTo(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2, const int src2Offset);

private:
    static bool IsGreaterThanOrEqualTo_NoConverse(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2, const int src2Offset);
    static bool IsLessThanOrEqualTo_NoConverse(const Value *const src1Value, const int32 min1, const int32 max1, const Value *const src2Value, const int32 min2, const int32 max2, const int src2Offset);

public:
    ValueInfo *SpecializeToInt32(JitArenaAllocator *const allocator, const bool isForLoopBackEdgeCompensation = false);
    ValueInfo *SpecializeToFloat64(JitArenaAllocator *const allocator);

    // SIMD_JS
    ValueInfo *SpecializeToSimd128(IRType type, JitArenaAllocator *const allocator);
    ValueInfo *SpecializeToSimd128F4(JitArenaAllocator *const allocator);
    ValueInfo *SpecializeToSimd128I4(JitArenaAllocator *const allocator);



public:
    Sym *                   GetSymStore() const { return this->symStore; }
    void                    SetSymStore(Sym * sym)
    {
        // Sym store should always be a var sym
        Assert(sym == nullptr || sym->IsPropertySym() || !sym->AsStackSym()->IsTypeSpec()); // property syms always have a var stack sym
        this->symStore = sym;
    }

    bool GetIsShared() const;
    void SetIsShared();

    ValueInfo *             Copy(JitArenaAllocator * allocator);

    ValueInfo *             CopyWithGenericStructureKind(JitArenaAllocator * allocator) const
    {
        return JitAnew(allocator, ValueInfo, *this, false);
    }

    bool                    GetIntValMinMax(int *pMin, int *pMax, bool doAggressiveIntTypeSpec);

#if DBG_DUMP
    void                    Dump();
#endif
#if DBG
    // Add a vtable in debug builds so that the actual can been inspected easily in the debugger without having to manually cast
    virtual void            AddVtable() { Assert(false); }
#endif
};

class Value
{
private:
    const ValueNumber valueNumber;
    ValueInfo *valueInfo;

protected:
    Value(const ValueNumber valueNumber, ValueInfo *valueInfo)
        : valueNumber(valueNumber), valueInfo(valueInfo)
    {
    };

public:
    static Value *New(JitArenaAllocator *const allocator, const ValueNumber valueNumber, ValueInfo *valueInfo)
    {
        return JitAnew(allocator, Value, valueNumber, valueInfo);
    }

    ValueNumber GetValueNumber() const { return this->valueNumber; }
    ValueInfo * GetValueInfo() { return this->valueInfo; }
    ValueInfo const * GetValueInfo() const { return this->valueInfo; }
    ValueInfo * ShareValueInfo() const { this->valueInfo->SetIsShared(); return this->valueInfo; }

    void        SetValueInfo(ValueInfo * newValueInfo) { Assert(newValueInfo); this->valueInfo = newValueInfo; }

    Value *     Copy(JitArenaAllocator * allocator, ValueNumber newValueNumber) const { return Value::New(allocator, newValueNumber, this->ShareValueInfo()); }

#if DBG_DUMP
    _NOINLINE void Dump() const { Output::Print(_u("0x%X  ValueNumber: %3d,  -> "), this, this->valueNumber);  this->valueInfo->Dump(); }
#endif
};

template<> inline
ValueNumber JsUtil::ValueToKey<ValueNumber, Value *>::ToKey(Value *const &value)
{
    Assert(value);
    return value->GetValueNumber();
}


template <typename T, typename U, ValueStructureKind kind>
class _IntConstantValueInfo : public ValueInfo
{
private:
    const T intValue;
public:
    static U *New(JitArenaAllocator *const allocator, const T intValue)
    {
        return JitAnew(allocator, U, intValue);
    }

    U *Copy(JitArenaAllocator *const allocator) const
    {
        return JitAnew(allocator, U, *((U*)this));
    }

    _IntConstantValueInfo(const T intValue)
        : ValueInfo(GetInt(IsTaggable(intValue)), kind),
        intValue(intValue)
    {}

    static bool IsTaggable(const T i)
    {
        return U::IsTaggable(i);
    }

    T IntValue() const
    {
        return intValue;
    }
};

class IntConstantValueInfo : public _IntConstantValueInfo<int, IntConstantValueInfo, ValueStructureKind::IntConstant>
{
public:
    static IntConstantValueInfo *New(JitArenaAllocator *const allocator, const int intValue)
    {
        return _IntConstantValueInfo::New(allocator, intValue);
    }
private:
    IntConstantValueInfo(int value) : _IntConstantValueInfo(value) {};
    static bool IsTaggable(const int i)
    {
#if INT32VAR
        // All 32-bit ints are taggable on 64-bit architectures
        return true;
#else
        return i >= Js::Constants::Int31MinValue && i <= Js::Constants::Int31MaxValue;
#endif
    }

    friend _IntConstantValueInfo;
};

class Int64ConstantValueInfo : public _IntConstantValueInfo<int64, Int64ConstantValueInfo, ValueStructureKind::Int64Constant>
{
public:
    static Int64ConstantValueInfo *New(JitArenaAllocator *const allocator, const int64 intValue)
    {
        return _IntConstantValueInfo::New(allocator, intValue);
    }
private:
    static bool IsTaggable(const int64 i) { return false; }
    Int64ConstantValueInfo(int64 value) : _IntConstantValueInfo(value) {};

    friend _IntConstantValueInfo;
};

class IntRangeValueInfo : public ValueInfo, public IntConstantBounds
{
private:
    // Definitely-int values are inherently not negative zero. This member variable, if true, indicates that this value was
    // produced by an int-specialized instruction that prevented a negative zero result using a negative zero bailout
    // (BailOutOnNegativeZero). Negative zero tracking in the dead-store phase tracks this information to see if some of these
    // negative zero bailout checks can be removed.
    bool wasNegativeZeroPreventedByBailout;

protected:
    IntRangeValueInfo(
        const IntConstantBounds &constantBounds,
        const bool wasNegativeZeroPreventedByBailout,
        const ValueStructureKind structureKind = ValueStructureKind::IntRange)
        : ValueInfo(constantBounds.GetValueType(), structureKind),
        IntConstantBounds(constantBounds),
        wasNegativeZeroPreventedByBailout(wasNegativeZeroPreventedByBailout)
    {
        Assert(!wasNegativeZeroPreventedByBailout || constantBounds.LowerBound() <= 0 && constantBounds.UpperBound() >= 0);
    }

public:
    static IntRangeValueInfo *New(
        JitArenaAllocator *const allocator,
        const int32 lowerBound,
        const int32 upperBound,
        const bool wasNegativeZeroPreventedByBailout)
    {
        return
            JitAnew(
                allocator,
                IntRangeValueInfo,
                IntConstantBounds(lowerBound, upperBound),
                wasNegativeZeroPreventedByBailout);
    }

    IntRangeValueInfo *Copy(JitArenaAllocator *const allocator) const
    {
        return JitAnew(allocator, IntRangeValueInfo, *this);
    }

public:
    bool WasNegativeZeroPreventedByBailout() const
    {
        return wasNegativeZeroPreventedByBailout;
    }
};

class FloatConstantValueInfo : public ValueInfo
{
private:
    const FloatConstType floatValue;

public:
    FloatConstantValueInfo(const FloatConstType floatValue)
        : ValueInfo(Float, ValueStructureKind::FloatConstant), floatValue(floatValue)
    {
    }

    static FloatConstantValueInfo *New(
        JitArenaAllocator *const allocator,
        const FloatConstType floatValue)
    {
        return JitAnew(allocator, FloatConstantValueInfo, floatValue);
    }

    FloatConstantValueInfo *Copy(JitArenaAllocator *const allocator) const
    {
        return JitAnew(allocator, FloatConstantValueInfo, *this);
    }

public:
    FloatConstType FloatValue() const
    {
        return floatValue;
    }
};

class VarConstantValueInfo : public ValueInfo
{
private:
    Js::Var const varValue;
    Js::Var const localVarValue;
    bool isFunction;

public:
    VarConstantValueInfo(Js::Var varValue, ValueType valueType, bool isFunction = false, Js::Var localVarValue = nullptr)
        : ValueInfo(valueType, ValueStructureKind::VarConstant),
        varValue(varValue), localVarValue(localVarValue), isFunction(isFunction)
    {
    }

    static VarConstantValueInfo *New(JitArenaAllocator *const allocator, Js::Var varValue, ValueType valueType, bool isFunction = false, Js::Var localVarValue = nullptr)
    {
        return JitAnew(allocator, VarConstantValueInfo, varValue, valueType, isFunction, localVarValue);
    }

    VarConstantValueInfo *Copy(JitArenaAllocator *const allocator) const
    {
        return JitAnew(allocator, VarConstantValueInfo, *this);
    }

public:
    Js::Var VarValue(bool useLocal = false) const
    {
        if(useLocal && this->localVarValue)
        {
            return this->localVarValue;
        }
        else
        {
            return this->varValue;
        }
    }

    bool IsFunction() const
    {
        return this->isFunction;
    }
};

struct ObjectTypePropertyEntry
{
    ObjTypeSpecFldInfo* fldInfo;
    uint blockNumber;
};

typedef JsUtil::BaseDictionary<Js::PropertyId, ObjectTypePropertyEntry, JitArenaAllocator> ObjectTypePropertyMap;

class JsTypeValueInfo : public ValueInfo
{
private:
    JITTypeHolder jsType;
    Js::EquivalentTypeSet * jsTypeSet;
    bool isShared;

public:
    JsTypeValueInfo(JITTypeHolder type)
        : ValueInfo(Uninitialized, ValueStructureKind::JsType),
        jsType(type), jsTypeSet(nullptr), isShared(false)
    {
    }

    JsTypeValueInfo(Js::EquivalentTypeSet * typeSet)
        : ValueInfo(Uninitialized, ValueStructureKind::JsType),
        jsType(nullptr), jsTypeSet(typeSet), isShared(false)
    {
    }

    JsTypeValueInfo(const JsTypeValueInfo& other)
        : ValueInfo(Uninitialized, ValueStructureKind::JsType),
        jsType(other.jsType), jsTypeSet(other.jsTypeSet), isShared(false)
    {
    }

    static JsTypeValueInfo * New(JitArenaAllocator *const allocator, JITTypeHolder typeSet)
    {
        return JitAnew(allocator, JsTypeValueInfo, typeSet);
    }

    static JsTypeValueInfo * New(JitArenaAllocator *const allocator, Js::EquivalentTypeSet * typeSet)
    {
        return JitAnew(allocator, JsTypeValueInfo, typeSet);
    }

    JsTypeValueInfo(const JITTypeHolder type, Js::EquivalentTypeSet * typeSet)
        : ValueInfo(Uninitialized, ValueStructureKind::JsType),
        jsType(type), jsTypeSet(typeSet), isShared(false)
    {
    }

    static JsTypeValueInfo * New(JitArenaAllocator *const allocator, const JITTypeHolder type, Js::EquivalentTypeSet * typeSet)
    {
        return JitAnew(allocator, JsTypeValueInfo, type, typeSet);
    }

public:
    JsTypeValueInfo * Copy(JitArenaAllocator *const allocator) const
    {
        return JitAnew(allocator, JsTypeValueInfo, *this);
    }

    JITTypeHolder GetJsType() const
    {
        return this->jsType;
    }

    void SetJsType(const JITTypeHolder value)
    {
        Assert(!this->isShared);
        this->jsType = value;
    }

    Js::EquivalentTypeSet * GetJsTypeSet() const
    {
        return this->jsTypeSet;
    }

    void SetJsTypeSet(Js::EquivalentTypeSet * value)
    {
        Assert(!this->isShared);
        this->jsTypeSet = value;
    }

    bool GetIsShared() const { return this->isShared; }
    void SetIsShared() { this->isShared = true; }
};

class ArrayValueInfo : public ValueInfo
{
private:
    StackSym *const headSegmentSym;
    StackSym *const headSegmentLengthSym;
    StackSym *const lengthSym;

private:
    ArrayValueInfo(
        const ValueType valueType,
        StackSym *const headSegmentSym,
        StackSym *const headSegmentLengthSym,
        StackSym *const lengthSym,
        Sym *const symStore = nullptr)
        : ValueInfo(valueType, ValueStructureKind::Array),
        headSegmentSym(headSegmentSym),
        headSegmentLengthSym(headSegmentLengthSym),
        lengthSym(lengthSym)
    {
        Assert(valueType.IsAnyOptimizedArray());
        Assert(!(valueType.IsLikelyTypedArray() && !valueType.IsOptimizedTypedArray()));

        // For typed arrays, the head segment length is the same as the array length. For objects with internal arrays, the
        // length behaves like a regular object's property rather than like an array length.
        Assert(!lengthSym || valueType.IsLikelyArray());
        Assert(!lengthSym || lengthSym != headSegmentLengthSym);

        if(symStore)
        {
            SetSymStore(symStore);
        }
    }

public:
    static ArrayValueInfo *New(
        JitArenaAllocator *const allocator,
        const ValueType valueType,
        StackSym *const headSegmentSym,
        StackSym *const headSegmentLengthSym,
        StackSym *const lengthSym,
        Sym *const symStore = nullptr)
    {
        Assert(allocator);

        return JitAnew(allocator, ArrayValueInfo, valueType, headSegmentSym, headSegmentLengthSym, lengthSym, symStore);
    }

    ValueInfo *Copy(
        JitArenaAllocator *const allocator,
        const bool copyHeadSegment = true,
        const bool copyHeadSegmentLength = true,
        const bool copyLength = true) const
    {
        Assert(allocator);

        return
            (copyHeadSegment && headSegmentSym) || (copyHeadSegmentLength && headSegmentLengthSym) || (copyLength && lengthSym)
                ? New(
                    allocator,
                    Type(),
                    copyHeadSegment ? headSegmentSym : nullptr,
                    copyHeadSegmentLength ? headSegmentLengthSym : nullptr,
                    copyLength ? lengthSym : nullptr,
                    GetSymStore())
                : CopyWithGenericStructureKind(allocator);
    }

public:
    StackSym *HeadSegmentSym() const
    {
        return headSegmentSym;
    }

    StackSym *HeadSegmentLengthSym() const
    {
        return headSegmentLengthSym;
    }

    StackSym *LengthSym() const
    {
        return lengthSym;
    }

    IR::ArrayRegOpnd *CreateOpnd(
        IR::RegOpnd *const previousArrayOpnd,
        const bool needsHeadSegment,
        const bool needsHeadSegmentLength,
        const bool needsLength,
        const bool eliminatedLowerBoundCheck,
        const bool eliminatedUpperBoundCheck,
        Func *const func) const
    {
        Assert(previousArrayOpnd);
        Assert(func);

        return
            IR::ArrayRegOpnd::New(
                previousArrayOpnd,
                Type(),
                needsHeadSegment ? headSegmentSym : nullptr,
                needsHeadSegmentLength ? headSegmentLengthSym : nullptr,
                needsLength ? lengthSym : nullptr,
                eliminatedLowerBoundCheck,
                eliminatedUpperBoundCheck,
                func);
    }
};
