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
        static Var OrdinaryToPrimitive(Var aValue, JavascriptHint hint, ScriptContext * scriptContext);
        static Var MethodCallToPrimitive(Var aValue, JavascriptHint hint, ScriptContext * scriptContext);
        static Var ToPrimitive(Var aValue, JavascriptHint hint, ScriptContext * scriptContext);
        static BOOL CanonicalNumericIndexString(JavascriptString *aValue, double *indexValue, ScriptContext * scriptContext);

        static void ToPropertyKey(Var argument, ScriptContext* scriptContext, const PropertyRecord** propertyRecord);

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
        static uint8 ToUInt8(Var aValue, ScriptContext* scriptContext);
        static uint8 ToUInt8Clamped(Var aValue, ScriptContext* scriptContext);
        static int16 ToInt16(Var aValue, ScriptContext* scriptContext);
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

        static BOOL ToInt32Finite(Var aValue, ScriptContext* scriptContext, int32* result);

        // ToString(ToPrimitive(aValue), for convert to string on concat
        static JavascriptString * ToPrimitiveString(Var aValue, ScriptContext * scriptContext);

        static int64 ToLength(Var aValue, ScriptContext* scriptContext);
        static int64 F32TOI64(float src, ScriptContext * ctx);
        static uint64 F32TOU64(float src, ScriptContext * ctx);
        static int64 F64TOI64(double src, ScriptContext * ctx);
        static uint64 F64TOU64(double src, ScriptContext * ctx);
        static int32 F32TOI32(float src, ScriptContext * ctx);
        static uint32 F32TOU32(float src, ScriptContext * ctx);
        static int32 F64TOI32(double src, ScriptContext * ctx);
        static uint32 F64TOU32(double src, ScriptContext * ctx);

        static float  LongToFloat(__int64 aValue);
        static float  ULongToFloat(unsigned __int64 aValue);
        static double LongToDouble(__int64 aValue);
        static double ULongToDouble(unsigned __int64 aValue);

    private:
        static BOOL ToInt32Finite(double value, int32* result);
        template<bool zero>
        static bool SameValueCommon(Var aValue, Var bValue);
    };
}
