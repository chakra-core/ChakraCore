//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

namespace Js
{

    template<typename T>
    inline T VECTORCALL minCheckNan(T aLeft, T aRight)
    {
        if (NumberUtilities::IsNan(aLeft) || NumberUtilities::IsNan(aRight))
        {
            return (T)NumberConstants::NaN;
        }
        if (aLeft < aRight)
        {
            return aLeft;
        }
        if (aLeft == aRight)
        {
            if (aRight == 0 && JavascriptNumber::IsNegZero(aLeft))
            {
                return aLeft;
            }
        }
        return aRight;
    }

    template<>
    inline double AsmJsMath::Min<double>(double aLeft, double aRight)
    {
        return minCheckNan(aLeft, aRight);
    }

    template<>
    inline float AsmJsMath::Min<float>(float aLeft, float aRight)
    {
        return minCheckNan(aLeft, aRight);
    }

    template<typename T>
    inline T maxCheckNan(T aLeft, T aRight)
    {
        if (NumberUtilities::IsNan(aLeft) || NumberUtilities::IsNan(aRight))
        {
            return (T)NumberConstants::NaN;
        }
        if (aLeft > aRight)
        {
            return aLeft;
        }
        if (aLeft == aRight)
        {
            if (aLeft == 0 && JavascriptNumber::IsNegZero(aRight))
            {
                return aLeft;
            }
        }
        return aRight;
    }

    template<>
    inline double AsmJsMath::Max<double>(double aLeft, double aRight)
    {
        return maxCheckNan(aLeft, aRight);
    }

    template<>
    inline float AsmJsMath::Max<float>(float aLeft, float aRight)
    {
        return maxCheckNan(aLeft, aRight);
    }

    template<>
    inline double AsmJsMath::Abs<double>(double aLeft)
    {
        uint64 x = (*(uint64*)(&aLeft) & 0x7FFFFFFFFFFFFFFF);
        return *(double*)(&x);
    }

    template<>
    inline float AsmJsMath::Abs<float>(float aLeft)
    {
        uint32 x = (*(uint32*)(&aLeft) & 0x7FFFFFFF);
        return *(float*)(&x);
    }

    template<typename T>
    inline T AsmJsMath::Add( T aLeft, T aRight )
    {
        return aLeft + aRight;
    }

    template<typename T>
    inline T AsmJsMath::Sub( T aLeft, T aRight )
    {
        return aLeft - aRight;
    }

    template<typename T> inline int AsmJsMath::CmpLt( T aLeft, T aRight ){return (int)(aLeft <  aRight);}
    template<typename T> inline int AsmJsMath::CmpLe( T aLeft, T aRight ){return (int)(aLeft <= aRight);}
    template<typename T> inline int AsmJsMath::CmpGt( T aLeft, T aRight ){return (int)(aLeft >  aRight);}
    template<typename T> inline int AsmJsMath::CmpGe( T aLeft, T aRight ){return (int)(aLeft >= aRight);}
    template<typename T> inline int AsmJsMath::CmpEq( T aLeft, T aRight ){return (int)(aLeft == aRight);}
    template<typename T> inline int AsmJsMath::CmpNe( T aLeft, T aRight ){return (int)(aLeft != aRight);}

    template<typename T> 
    inline T AsmJsMath::And( T aLeft, T aRight )
    {
        return aLeft & aRight;
    }

    template<typename T> 
    inline T AsmJsMath::Or( T aLeft, T aRight )
    {
        return aLeft | aRight;
    }

    template<typename T> 
    inline T AsmJsMath::Xor( T aLeft, T aRight )
    {
        return aLeft ^ aRight;
    }

    template<typename T> 
    inline T AsmJsMath::Shl( T aLeft, T aRight )
    {
        return aLeft << aRight;
    }

    template<typename T> 
    inline T AsmJsMath::Shr( T aLeft, T aRight )
    {
        return aLeft >> aRight;
    }

    template<typename T> 
    inline T AsmJsMath::ShrU( T aLeft, T aRight )
    {
        return aLeft >> aRight;
    }

    template<typename T>
    inline T AsmJsMath::Neg( T aLeft )
    {
        return -aLeft;
    }

    inline int AsmJsMath::Not( int aLeft )
    {
        return ~aLeft;
    }

    inline int AsmJsMath::LogNot( int aLeft )
    {
        return !aLeft;
    }

    inline int AsmJsMath::ToBool( int aLeft )
    {
        return !!aLeft;
    }

    inline int AsmJsMath::Clz32( int value)
    {
        DWORD index;
        if (_BitScanReverse(&index, value))
        {
            return 31 - index;
        }
        return 32;
    }
}
