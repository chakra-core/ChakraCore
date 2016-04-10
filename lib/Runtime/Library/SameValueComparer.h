//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename Key, bool zero>
    struct SameValueComparerCommon
    {
        static bool Equals(Key, Key) { static_assert(false, "Can only use SameValueComparer with Var as the key type"); }
        static hash_t GetHashCode(Key) { static_assert(false, "Can only use SameValueComparer with Var as the key type"); }
    };

    template <typename Key> using SameValueComparer = SameValueComparerCommon<Key, false>;
    template <typename Key> using SameValueZeroComparer = SameValueComparerCommon<Key, true>;

    template <bool zero>
    struct SameValueComparerCommon<Var, zero>
    {
        static bool Equals(Var x, Var y)
        {
            if (zero)
            {
                return JavascriptConversion::SameValueZero(x, y);
            }
            else
            {
                return JavascriptConversion::SameValue(x, y);
            }
        }

        static hash_t HashDouble(double d)
        {
            if (JavascriptNumber::IsNan(d))
            {
                return 0;
            }

            if (zero)
            {
                // SameValueZero treats -0 and +0 the same, so normalize to get same hash code
                if (JavascriptNumber::IsNegZero(d))
                {
                    d = 0.0;
                }
            }

            __int64 v = *(__int64*)&d;
            return (uint)v ^ (uint)(v >> 32);
        }

        static hash_t GetHashCode(Var i)
        {
            switch (JavascriptOperators::GetTypeId(i))
            {
            case TypeIds_Integer:
                // int32 can be fully represented in a double, so hash it as a double
                // to ensure that tagged ints hash to the same value as JavascriptNumbers.
                return HashDouble((double)TaggedInt::ToInt32(i));

            case TypeIds_Int64Number:
            case TypeIds_UInt64Number:
                {
                    __int64 v = JavascriptInt64Number::FromVar(i)->GetValue();
                    double d = (double) v;
                    if (v != (__int64) d)
                    {
                        // this int64 is too large to represent in a double
                        // and thus will never be equal to a double so hash it
                        // as an int64
                        return (uint)v ^ (uint)(v >> 32);
                    }

                    // otherwise hash it as a double
                    return HashDouble(d);
                }

            case TypeIds_Number:
                {
                    double d = JavascriptNumber::GetValue(i);
                    return HashDouble(d);
                }

            case TypeIds_String:
                {
                    JavascriptString* v = JavascriptString::FromVar(i);
                    return JsUtil::CharacterBuffer<WCHAR>::StaticGetHashCode(v->GetString(), v->GetLength());
                }

            default:
                return RecyclerPointerComparer<Var>::GetHashCode(i);
            }
        }
    };
}
