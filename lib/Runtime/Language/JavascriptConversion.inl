//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(_M_IX86) || defined(_M_X64)
#ifdef _WIN32
#include <emmintrin.h>
#endif
#endif

namespace Js {

    inline BOOL JavascriptConversion::ToBoolean(Var aValue,ScriptContext* scriptContext)
    {
        JIT_HELPER_NOT_REENTRANT_HEADER(Conv_ToBoolean, reentrancylock, scriptContext->GetThreadContext());
        if (TaggedInt::Is(aValue))
        {
            return aValue != reinterpret_cast<Var>(AtomTag_IntPtr);
        }
#if FLOATVAR
        else if (JavascriptNumber::Is_NoTaggedIntCheck(aValue))
        {
            double value = JavascriptNumber::GetValue(aValue);
            return (!JavascriptNumber::IsNan(value)) && (!JavascriptNumber::IsZero(value));
        }
#endif
        else
        {
            return ToBoolean_Full(aValue, scriptContext);
        }
    }

    inline bool JavascriptConversion::ToBool(Var aValue,ScriptContext* scriptContext)
    {
        return !!JavascriptConversion::ToBoolean(aValue, scriptContext);
    }

    /// ToNumber() returns an integer value, as described in (ES3.0: S9.3).

    inline double JavascriptConversion::ToNumber(Var aValue, ScriptContext* scriptContext)
    {
        // Optimize for TaggedInt and double before falling back to ToNumber_Full
        if( TaggedInt::Is(aValue) )
        {
            return TaggedInt::ToDouble(aValue);
        }

        if( JavascriptNumber::Is_NoTaggedIntCheck(aValue) )
        {
            return JavascriptNumber::GetValue(aValue);
        }

        return ToNumber_Full(aValue, scriptContext);
    }

    inline double JavascriptConversion::ToInteger(Var aValue, ScriptContext* scriptContext)
    {
        return
            TaggedInt::Is(aValue) ?
            TaggedInt::ToDouble(aValue) :
            ToInteger_Full(aValue, scriptContext);
    }


    inline int32 JavascriptConversion::ToInt32(Var aValue, ScriptContext* scriptContext)
    {
        return
            TaggedInt::Is(aValue) ?
            TaggedInt::ToInt32(aValue) :
            ToInt32_Full(aValue, scriptContext);
    }

    inline uint32 JavascriptConversion::ToUInt32(Var aValue, ScriptContext* scriptContext)
    {
        JIT_HELPER_REENTRANT_HEADER(Conv_ToUInt32);
        return
            TaggedInt::Is(aValue) ?
            TaggedInt::ToUInt32(aValue) :
            ToUInt32_Full(aValue, scriptContext);
    }

    inline uint16 JavascriptConversion::ToUInt16(Var aValue, ScriptContext* scriptContext)
    {
        return
            TaggedInt::Is(aValue) ?
            (uint16) TaggedInt::ToUInt32(aValue) :
            ToUInt16_Full(aValue, scriptContext);
    }

   inline int8 JavascriptConversion::ToInt8(Var aValue, ScriptContext* scriptContext)
   {
       return TaggedInt::Is(aValue) ?
           (int8) TaggedInt::ToInt32(aValue) :
           (int8) ToInt32(aValue, scriptContext);
   }

   inline uint8 JavascriptConversion::ToUInt8(Var aValue, ScriptContext* scriptContext)
   {
       return TaggedInt::Is(aValue) ?
           (uint8) TaggedInt::ToInt32(aValue) :
           (uint8) ToUInt32(aValue, scriptContext);
   }

   inline uint8 JavascriptConversion::ToUInt8Clamped(Var aValue, ScriptContext* scriptContext)
   {
       double dval;

       if (TaggedInt::Is(aValue))
       {
           int32 val = Js::TaggedInt::ToInt32(aValue);

           // Values larger than 0xff should be clamped to 0xff
           if (val > UINT8_MAX)
           {
               return UINT8_MAX;
           }
           // Negative values should be clamped to 0
           if (val < 0)
           {
               return 0;
           }

           return (uint8) val;
       }
       else if (JavascriptOperators::GetTypeId(aValue) == TypeIds_Number)
       {
           dval = JavascriptNumber::GetValue(aValue);
       }
       else
       {
           dval = JavascriptConversion::ToNumber_Full(aValue, scriptContext);
       }

       // This will also cover positive infinity
       // Note: This is strictly greater-than check because 254.5 rounds to 254
       if (dval > 254.5)
       {
           return UINT8_MAX;
       }

       // This will also cover negative infinity, and anything less than INT_MIN
       if (dval < 0)
       {
           return 0;
       }

       // We now have a double value which is between 0 and 255 and just need to convert it
       // to an integer following IEEE 754 rounding rules which round ties to the nearest
       // even integer.
#if defined(_M_IX86) || defined(_M_X64)
       if (AutoSystemInfo::Data.SSE2Available())
       {
           // On x86 we have a convenient CVTSD2SI intrinsic function to handle this.
           __m128d t = _mm_load_sd(&dval);
           return (uint8)_mm_cvtsd_si32(t);
       }
       else
#endif
       {
           // On ARM, there is not a convenient intrinsic (for VCVTRS32F64).
           // Once DevDiv TFS item 656383 is complete, we should replace the below with the intrinsic.
           // 1. Calculate the fractional part of the double value
           // 2. Round up or down as usual if the fractional part is <> 0.5
           // 3. If the fractional part == 0.5, round to nearest even integer:
           //    Divide by 2, add 0.5, cast to integer, multiply by 2 again.
           uint8 u8 = (uint8)dval;
           double frac = dval - u8;

           if (frac > 0.5)
           {
               return (uint8)(dval + 0.5);
           }
           else if (frac < 0.5)
           {
               return u8;
           }
           else
           {
               return ((uint8)(dval / 2.0 + 0.5)) * 2;
           }
       }
   }

   inline int16 JavascriptConversion::ToInt16(Var aValue, ScriptContext* scriptContext)
   {
       return TaggedInt::Is(aValue) ?
           (int16) TaggedInt::ToInt32(aValue) :
           (int16) ToUInt32(aValue, scriptContext);
   }

   inline float JavascriptConversion::ToFloat(Var aValue, ScriptContext* scriptContext)
   {
       return (float)ToNumber(aValue, scriptContext);
   }

   inline bool JavascriptConversion::SameValue(Var aValue, Var bValue)
   {
       return SameValueCommon<false>(aValue, bValue);
   }

   inline bool JavascriptConversion::SameValueZero(Var aValue, Var bValue)
   {
       return SameValueCommon<true>(aValue, bValue);
   }

   template <typename T, bool allowNegOne>
   inline Var JavascriptConversion::TryCanonicalizeIntHelper(T val)
   {
       if (TaggedInt::IsOverflow(val))
       {
           return nullptr;
       }

       if (!allowNegOne && val == -1)
       {
           return nullptr;
       }

       return TaggedInt::ToVarUnchecked((int)val);
   }

   template <bool allowNegOne, bool allowLossyConversion>
   inline Var JavascriptConversion::TryCanonicalizeAsTaggedInt(Var value, TypeId typeId)
   {
       switch (typeId)
       {
       case TypeIds_Integer:
           return (allowNegOne || value != TaggedInt::ToVarUnchecked(-1))
               ? value
               : nullptr;

       case TypeIds_Number:
       {
           double doubleVal = JavascriptNumber::GetValue(value);
           int32 intVal = 0;

           if (!JavascriptNumber::TryGetInt32Value<allowLossyConversion>(doubleVal, &intVal))
           {
               return nullptr;
           }
           return TryCanonicalizeIntHelper<int32, allowNegOne>(intVal);
       }
       case TypeIds_Int64Number:
       {
           if (!allowLossyConversion)
           {
               return nullptr;
           }
           int64 int64Val = UnsafeVarTo<JavascriptInt64Number>(value)->GetValue();

           return TryCanonicalizeIntHelper<int64, allowNegOne>(int64Val);

       }
       case TypeIds_UInt64Number:
       {
           if (!allowLossyConversion)
           {
               return nullptr;
           }
           uint64 uint64Val = UnsafeVarTo<JavascriptUInt64Number>(value)->GetValue();

           return TryCanonicalizeIntHelper<uint64, allowNegOne>(uint64Val);
       }
       default:
           return nullptr;
       }
   }

   template <bool allowNegOne, bool allowLossyConversion>
   inline Var JavascriptConversion::TryCanonicalizeAsTaggedInt(Var value)
   {
       TypeId typeId = JavascriptOperators::GetTypeId(value);
       return TryCanonicalizeAsTaggedInt<allowNegOne, allowLossyConversion>(value, typeId);
   }

   // Lossy conversion means values are StrictEqual equivalent,
   // but we cannot reconstruct the original value after canonicalization
   // (e.g. -0 or an Int64Number object)
   template <bool allowLossyConversion>
   inline Var JavascriptConversion::TryCanonicalizeAsSimpleVar(Var value)
   {
       TypeId typeId = JavascriptOperators::GetTypeId(value);
       switch (typeId)
       {
       case TypeIds_Integer:
       case TypeIds_Number:
       case TypeIds_Int64Number:
       case TypeIds_UInt64Number:
       {
           Var taggedInt = TryCanonicalizeAsTaggedInt<true, allowLossyConversion>(value, typeId);
           if (taggedInt)
           {
               return taggedInt;
           }
#if FLOATVAR
           if (typeId == TypeIds_Number)
           {
               // NaN could have sign bit set, but that isn't observable so canonicalize to positive NaN
               double numberValue = JavascriptNumber::GetValue(value);
               return JavascriptNumber::IsNan(numberValue)
                   ? JavascriptNumber::ToVar(JavascriptNumber::NaN)
                   : value;
           }
#else
           return nullptr;
#endif
       }
       case TypeIds_String:
           return nullptr;

       default:
           return value;
       }
   }

} // namespace Js
