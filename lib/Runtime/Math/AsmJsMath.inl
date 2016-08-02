//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Js
{

    template<typename T>
    inline T minCheckNan(T aLeft, T aRight)
    {
        if (NumberUtilities::IsNan(aLeft) || NumberUtilities::IsNan(aRight))
        {
            return (T)NumberConstants::NaN;
        }
        return aLeft < aRight ? aLeft : aRight;
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
        return aLeft > aRight ? aLeft : aRight;
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

    template<typename T>
    inline T AsmJsMath::Add( T aLeft, T aRight )
    {
        return aLeft + aRight;
    }

    template<>
    inline int32 AsmJsMath::Div<int32>( int32 aLeft, int32 aRight )
    {
        return aRight == 0 ? 0 : ( aLeft == (1<<31) && aRight == -1) ? aLeft : aLeft / aRight;
    }

    template<>
    inline uint32 AsmJsMath::Div<uint32>(uint32 aLeft, uint32 aRight)
    {
        return aRight == 0 ? 0 : aLeft / aRight;
    }

    template<typename T>
    inline T AsmJsMath::Div( T aLeft, T aRight )
    {
        return aLeft / aRight;
    }

    template<typename T>
    inline T AsmJsMath::Mul( T aLeft, T aRight )
    {
        return aLeft * aRight;
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
    inline T AsmJsMath::Rem( T aLeft, T aRight )
    {
        return (aRight == 0) ? 0 : aLeft % aRight;
    }

    template<>
    inline int AsmJsMath::Rem<int>( int aLeft, int aRight )
    {
        return ((aRight == 0) || (aLeft == (1<<31) && aRight == -1)) ? 0 : aLeft % aRight;
    }

    template<>
    inline double AsmJsMath::Rem<double>( double aLeft, double aRight )
    {
        return NumberUtilities::Modulus( aLeft, aRight );
    }

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
