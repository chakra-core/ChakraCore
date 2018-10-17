//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

typedef int BOOL;
namespace Js {
    class JavascriptConversion  /* All static */
    {
    public:
        template <JavascriptHint hint>
        static Var OrdinaryToPrimitive(_In_ RecyclableObject* value, _In_ ScriptContext * scriptContext);
        template <JavascriptHint hint>
        static Var MethodCallToPrimitive(_In_ RecyclableObject* value, _In_ ScriptContext * scriptContext);
        template <JavascriptHint hint>
        static Var ToPrimitive(_In_ Var aValue, _In_ ScriptContext * scriptContext);
        static BOOL CanonicalNumericIndexString(JavascriptString *aValue, double *indexValue, ScriptContext * scriptContext);

        static void ToPropertyKey(
            Var argument,
            _In_ ScriptContext* scriptContext,
            _Out_ const PropertyRecord** propertyRecord,
            _Out_opt_ PropertyString** propString);

        static PropertyQueryFlags BooleanToPropertyQueryFlags(BOOL val) { return val ? PropertyQueryFlags::Property_Found : PropertyQueryFlags::Property_NotFound; }
        static BOOL PropertyQueryFlagsToBoolean(PropertyQueryFlags val) { return val == PropertyQueryFlags::Property_Found; }

        static JavascriptString* ToString(Var aValue, ScriptContext* scriptContext);
        static JavascriptString* ToLocaleString(Var aValue, ScriptContext* scriptContext);

        static BOOL ToObject(Var aValue, ScriptContext* scriptContext, RecyclableObject** object);

        static BOOL ToBoolean(Var aValue, ScriptContext* scriptContext);
        static BOOL ToBoolean_Full(Var aValue, ScriptContext* scriptContext);

        static bool ToBool(Var aValue, ScriptContext* scriptContext);

        static double ToNumber(Var aValue, ScriptContext* scriptContext);
        static void ToFloat_Helper(Var aValue, float *pResult, ScriptContext* scriptContext);
        static void ToNumber_Helper(Var aValue, double *pResult, ScriptContext* scriptContext);
        static BOOL ToNumber_FromPrimitive(Var aValue, double *pResult, BOOL allowUndefined, ScriptContext* scriptContext);
        static double ToNumber_Full(Var aValue, ScriptContext* scriptContext);

        static double ToInteger(Var aValue, ScriptContext* scriptContext);
        static double ToInteger(double value);
        static double ToInteger_Full(Var aValue, ScriptContext* scriptContext);

        static int32 ToInt32(Var aValue, ScriptContext* scriptContext);
        static __int64 ToInt64(Var aValue, ScriptContext* scriptContext);
        static int32 ToInt32(double value);
        static int32 ToInt32_Full(Var aValue, ScriptContext* scriptContext);

        static int8 ToInt8(Var aValue, ScriptContext* scriptContext);
        static int8 ToInt8(double aValue);
        static uint8 ToUInt8(Var aValue, ScriptContext* scriptContext);
        static uint8 ToUInt8(double aValue);
        static uint8 ToUInt8Clamped(Var aValue, ScriptContext* scriptContext);
        static int16 ToInt16(Var aValue, ScriptContext* scriptContext);
        static int16 ToInt16(double aValue);
        static float ToFloat(Var aValue, ScriptContext* scriptContext);

        static uint32 ToUInt32(Var aValue, ScriptContext* scriptContext);
        static unsigned __int64 ToUInt64(Var aValue, ScriptContext* scriptContext);
        static uint32 ToUInt32(double value);
        static uint32 ToUInt32_Full(Var aValue, ScriptContext* scriptContext);

        static uint16 ToUInt16(Var aValue, ScriptContext* scriptContext);
        static uint16 ToUInt16(double value);
        static uint16 ToUInt16_Full(Var aValue, ScriptContext* scriptContext);

        static JavascriptString *CoerseString(Var aValue, ScriptContext* scriptContext, const char16* apiNameForErrorMsg);
        static BOOL CheckObjectCoercible(Var aValue, ScriptContext* scriptContext);
        static bool SameValue(Var aValue, Var bValue);
        static bool SameValueZero(Var aValue, Var bValue);
        static bool IsCallable(Var aValue);
        static bool IsCallable(_In_ RecyclableObject* aValue);

        static BOOL ToInt32Finite(Var aValue, ScriptContext* scriptContext, int32* result);

        // ToString(ToPrimitive(aValue), for convert to string on concat
        static JavascriptString * ToPrimitiveString(Var aValue, ScriptContext * scriptContext);

        static int64 ToLength(Var aValue, ScriptContext* scriptContext);

        static JavascriptBigInt * ToBigInt(Var aValue, ScriptContext * scriptContext);

        static float  LongToFloat(__int64 aValue);
        static float  ULongToFloat(unsigned __int64 aValue);
        static double LongToDouble(__int64 aValue);
        static double ULongToDouble(unsigned __int64 aValue);

        template <bool allowNegOne, bool allowLossyConversion>
        static Var TryCanonicalizeAsTaggedInt(Var value);
        template <bool allowNegOne, bool allowLossyConversion>
        static Var TryCanonicalizeAsTaggedInt(Var value, TypeId typeId);
        template <bool allowLossyConversion>
        static Var TryCanonicalizeAsSimpleVar(Var value);

    private:
        template <typename T, bool allowNegOne>
        static Var TryCanonicalizeIntHelper(T val);

        static BOOL ToInt32Finite(double value, int32* result);
        template<bool zero>
        static bool SameValueCommon(Var aValue, Var bValue);
    };
}
