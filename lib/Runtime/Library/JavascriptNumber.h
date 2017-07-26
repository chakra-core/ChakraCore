//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class JavascriptNumber :
#if !FLOATVAR
        public RecyclableObject,
#endif
        public NumberConstants
    {
    private:
        double m_value;
#if !FLOATVAR
        DEFINE_VTABLE_CTOR(JavascriptNumber, RecyclableObject);
#endif
    public:
        JavascriptNumber(double value, StaticType*
#if DBG
            , bool oopJIT = false
#endif
        );
        static uint32 GetValueOffset()
        {
            return offsetof(JavascriptNumber, m_value);
        }

        static bool Is(Var aValue);
        static bool Is_NoTaggedIntCheck(Var aValue);

        static Var ToVarWithCheck(double value, ScriptContext* scriptContext);
        static Var ToVarNoCheck(double value, ScriptContext* scriptContext);
        static Var ToVarInPlace(double value, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var ToVarMaybeInPlace(double value, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var ToVarIntCheck(double value, ScriptContext* scriptContext);
        static Var ToVar(int32 nValue, ScriptContext* scriptContext);
#if defined(__clang__) && defined(_M_IX86)
        static Var ToVar(intptr_t nValue, ScriptContext* scriptContext);
#endif
        static Var ToVarInPlace(int32 nValue, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var ToVarInPlace(int64 value, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var ToVarInPlace(uint32 nValue, ScriptContext* scriptContext, JavascriptNumber *result);
        static Var ToVar(uint32 nValue, ScriptContext* scriptContext);
        static Var ToVar(int64 nValue, ScriptContext* scriptContext);
        static Var ToVar(uint64 nValue, ScriptContext* scriptContext);
        static double GetValue(Var aValue);
        static int32 DirectPowIntInt(bool*, int32, int32);
        static double DirectPowDoubleInt(double, int32);
        static double DirectPow(double, double);

        static bool TryToVarFast(int32 nValue, Var* result);
        static bool TryToVarFastWithCheck(double value, Var* result);

        inline static BOOL IsNan(double value) { return NumberUtilities::IsNan(value); }
        static bool IsZero(double value);
        static BOOL IsNegZero(double value);
        static bool IsPosInf(double value);
        static bool IsNegInf(double value);

        template<bool acceptNegZero = false>
        static bool TryGetInt32Value(const double value, int32 *const int32Value)
        {
            Assert(int32Value);

            const int32 i = static_cast<int32>(value);
            if (static_cast<double>(i) != value || (!acceptNegZero && IsNegZero(value)))
            {
                return false;
            }

            *int32Value = i;
            return true;
        }

        static bool TryGetInt32OrUInt32Value(const double value, int32 *const int32Value, bool *const isInt32);
        static bool IsInt32(const double value);
        static bool IsInt32OrUInt32(const double value);
        static bool IsInt32_NoChecks(const Var number);
        static bool IsInt32OrUInt32_NoChecks(const Var number);
        static int32 GetNonzeroInt32Value_NoTaggedIntCheck(const Var object);

        static JavascriptString* ToString(double value, ScriptContext* scriptContext);

        class EntryInfo
        {
        public:
            static FunctionInfo NewInstance;
            // Number constructor built-ins:
            static FunctionInfo IsNaN;
            static FunctionInfo IsFinite;
            static FunctionInfo IsInteger;
            static FunctionInfo IsSafeInteger;
            // Number prototype built-ins:
            static FunctionInfo ToExponential;
            static FunctionInfo ToFixed;
            static FunctionInfo ToPrecision;
            static FunctionInfo ToLocaleString;
            static FunctionInfo ToString;
            static FunctionInfo ValueOf;
        };

        static Var NewInstance(RecyclableObject* function, CallInfo callInfo, ...);
        // Number constructor built-ins:
        static Var EntryIsNaN(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsFinite(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsInteger(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryIsSafeInteger(RecyclableObject* function, CallInfo callInfo, ...);
        // Number prototype built-ins:
        static Var EntryToExponential(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToFixed(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToPrecision(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryToString(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryValueOf(RecyclableObject* function, CallInfo callInfo, ...);

        static Var New(double value, ScriptContext* scriptContext);
        static Var NewWithCheck(double value, ScriptContext* scriptContext);
        static Var NewInlined(double value, ScriptContext* scriptContext);
        static JavascriptNumber* InPlaceNew(double value, ScriptContext* scriptContext, JavascriptNumber* result);

#if ENABLE_NATIVE_CODEGEN
#if FLOATVAR
        static Var NewCodeGenInstance(double value, ScriptContext* scriptContext);
#else
        static Var NewCodeGenInstance(CodeGenNumberAllocator *alloc, double value, ScriptContext* scriptContext);
#endif
#endif

        inline static bool IsSpecial(double value, uint64 nSpecial) { return NumberUtilities::IsSpecial(value, nSpecial); }
        inline static uint64 ToSpecial(double value) { return NumberUtilities::ToSpecial(value); }

        static JavascriptString* ToStringNan(ScriptContext* scriptContext);
        static JavascriptString* ToStringRadix10(double dValue, ScriptContext* scriptContext);

        // when radix is 10, ToStringRadix10 should be used instead
        static JavascriptString* ToStringRadixHelper(double value, int radix, ScriptContext* scriptContext);
        static JavascriptString* ToLocaleString(double dValue, ScriptContext* scriptContext);
        static JavascriptString* ToLocaleStringIntl(ArgumentReader& args, CallInfo callInfo, ScriptContext* scriptContext);
        static JavascriptString* ToLocaleStringIntl(Var* values, CallInfo callInfo, ScriptContext* scriptContext);
        static Var CloneToScriptContext(Var aValue, ScriptContext* requestContext);

#if !FLOATVAR
        static JavascriptNumber* NewUninitialized(Recycler * recycler);
        static Var BoxStackInstance(Var instance, ScriptContext* scriptContext);
        static Var BoxStackNumber(Var instance, ScriptContext* scriptContext);

        // This is needed to ensure JavascriptNumber has a VTABLE and JavascriptNumber::Is (which checks for the VTABLE value) works correctly.
        // This also prevents the vtable from being folded with other classes unexpectedly.
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableJavascriptNumber;
        }
#endif


#if FLOATVAR
        static Var ToVar(double value);
#else
        static JavascriptNumber* FromVar(Var aValue);
#endif

    private:
        void SetValue(double value)
        {
            m_value = value;
        }

        double GetValue() const
        {
            return m_value;
        }

        void SetSpecial(uint64 value)
        {
            uint64* pnOverwrite = reinterpret_cast<uint64 *>(&this->m_value);
            *pnOverwrite = value;
        }

        static JavascriptString* ToStringNanOrInfinite(double val, ScriptContext* scriptContext);
        static JavascriptString* ToStringNanOrInfiniteOrZero(double val, ScriptContext* scriptContext);
        static JavascriptString* ToLocaleStringNanOrInfinite(double val, ScriptContext* scriptContext);

        static Var FormatDoubleToString( double value, Js::NumberUtilities::FormatType formatType, int fractionDigits, ScriptContext* scriptContext );

        static BOOL GetThisValue(Var avalue, double* pDouble);
    };

    //
    // Implementations shared with diagnostics
    //
#if FLOATVAR
    // Since RecyclableObject and Int32 in their Var representation have top 14 bits 0
    // and tagged float now has top 14 XOR'd with 1s - the value with 14 bits non zero is
    // a float.
    // A float can have all 14 bits 0 iff it was a NaN in the first place. Since
    // we can only produce NaNs with top 13 bits set (see k_Nan) - this cannot happen.
    inline bool JavascriptNumber::Is_NoTaggedIntCheck(Var aValue)
    {
        return ((uint64)aValue >> 50) != 0;
    }

    inline double JavascriptNumber::GetValue(Var aValue)
     {
         AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNumber'");
         double result;
        (*(uint64*)(&result)) = (((uint64)aValue) ^ FloatTag_Value);
         return result;
     }
#endif

    inline bool JavascriptNumber::IsZero(double value)
    {
        // succeeds for -0.0 as well
        return value == 0.0;
    }

    inline bool JavascriptNumber::IsPosInf(double value)
    {
        return IsSpecial(value, k_PosInf);
    }

    inline bool JavascriptNumber::IsNegInf(double value)
    {
        return IsSpecial(value, k_NegInf);
    }

    inline BOOL JavascriptNumber::IsNegZero(double value)
    {
        return IsSpecial(value, k_NegZero);
    }
}

#if defined(_M_IX86)
#ifdef DBG
#ifdef HEAP_ENUMERATION_VALIDATION
// Heap enum has an extra field m_heapEnumValidationCookie, but it fills in the alignment
// So the size is the same
CompileAssert(sizeof(Js::JavascriptNumber) == 24);
#else
CompileAssert(sizeof(Js::JavascriptNumber) == 16);
#endif
#else
CompileAssert(sizeof(Js::JavascriptNumber) == 16);
#endif
#endif
