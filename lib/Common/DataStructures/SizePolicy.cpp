//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonDataStructuresPch.h"

static const uint primes[] = {
    3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107, 131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919,
    1103, 1327, 1597, 1931, 2333, 2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591,
    17519, 21023, 25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363, 156437,
    187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403, 968897, 1162687, 1395263,
    1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369, 10453007, 13169977, 20906033
};

// If we know the modulo number at compile time, the compiler can optimize instructions.
// These macros let the compiler do these optimizations for modulus optimizations of known primes
static_assert(_countof(primes) == UNKNOWN_MOD_INDEX, "Adding / removing prime numbers ?");

#define MOD_FNC_(x) static uint mod##x(const uint k, const uint) { return k % x##llu; }
#define MOD_FNC__(a,b,c,d,e) MOD_FNC_(a) MOD_FNC_(b) MOD_FNC_(c) MOD_FNC_(d) MOD_FNC_(e)
#define MOD_FNC(a,b,c,d,e,f,g,h,j,k) MOD_FNC__(a,b,c,d,e) MOD_FNC__(f,g,h,j,k)

MOD_FNC(3, 7, 11, 17, 23, 29, 37, 47, 59, 71)
MOD_FNC(89, 107, 131, 163, 197, 239, 293, 353, 431, 521)
MOD_FNC(631, 761, 919, 1103, 1327, 1597, 1931, 2333, 2801, 3371)
MOD_FNC(4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17519, 21023)
MOD_FNC(25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363)
MOD_FNC(156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403)
MOD_FNC(968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559)
MOD_FNC__(5999471, 7199369, 10453007, 13169977, 20906033)

static uint modLAST(const uint k, const uint p) { return k % p; }

#undef MOD_FNC_
#undef MOD_FNC__
#undef MOD_FNC

#define MOD_FNC_(x) &mod##x
#define MOD_FNC__(a,b,c,d,e) MOD_FNC_(a),MOD_FNC_(b),MOD_FNC_(c),MOD_FNC_(d),MOD_FNC_(e)
#define MOD_FNC(a,b,c,d,e,f,g,h,j,k) MOD_FNC__(a,b,c,d,e),MOD_FNC__(f,g,h,j,k),

// TODO: (obastemur) MOVE THIS UNDER .h file!!
// We could have this on header file and inline the proxy function for good.
// However, not every target we support comes with constexpr... and we need this done during compile time.
static OPT_CONSTEXPR uint (* const mod_func[])(const uint, const uint) =
{
    MOD_FNC(3, 7, 11, 17, 23, 29, 37, 47, 59, 71)
    MOD_FNC(89, 107, 131, 163, 197, 239, 293, 353, 431, 521)
    MOD_FNC(631, 761, 919, 1103, 1327, 1597, 1931, 2333, 2801, 3371)
    MOD_FNC(4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17519, 21023)
    MOD_FNC(25229, 30293, 36353, 43627, 52361, 62851, 75431, 90523, 108631, 130363)
    MOD_FNC(156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403)
    MOD_FNC(968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 4999559)
    MOD_FNC__(5999471, 7199369, 10453007, 13169977, 20906033)
    , &modLAST
};

#undef MOD_FNC_
#undef MOD_FNC__
#undef MOD_FNC

hash_t PrimePolicy::ModPrime(hash_t key, uint prime, int modFunctionIndex)
{
    return mod_func[modFunctionIndex](key, prime);
}

bool
PrimePolicy::IsPrime(uint candidate)
{
    if ((candidate & 1) != 0)
    {
        int limit = (uint)sqrt((FLOAT)candidate);
        for (int divisor = 3; divisor <= limit; divisor += 2)
        {
            if ((candidate % divisor) == 0)
                return false;
        }
        return true;
    }
    return (candidate == 2);
}

uint
PrimePolicy::GetPrime(uint min, int *modFunctionIndex)
{
    if (min <= 0)
    {
        *modFunctionIndex = 3;
        AssertMsg(primes[3] == 17, "Update modFunctionIndex for 17");
        return 17;
    }

    for (int i = 0; i < sizeof(primes)/sizeof(uint); i++)
    {
        uint prime = primes[i];
        if (prime >= min)
        {
            *modFunctionIndex = i;
            return prime;
        }
    }

    *modFunctionIndex = UNKNOWN_MOD_INDEX;
    //outside of our predefined table.
    //compute the hard way.
    for (uint i = (min | 1); i < 0x7FFFFFFF; i += 2)
    {
        if (IsPrime(i))
        {
            return i;
        }
    }
    return min;
}
