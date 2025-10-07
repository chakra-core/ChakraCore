//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This file contains stubs needed to make BigIntTest successfully compile and link as well
// as a means to emulate behavior of objects that interact with BigInt class

#include "..\..\lib\Common\Warnings.h"
#include "..\..\lib\Common\Core\Api.cpp"
#include "..\..\lib\Common\Common\NumberUtilities.cpp"

namespace Js
{
    void Throw::FatalInternalError(long) 
    {
        Assert(false);
    }

    bool Throw::ReportAssert(_In_ char const *, unsigned int, _In_ char const *, _In_ char const *)
    {
        return false;
    }

    void Throw::LogAssert(void) {}
}

template <typename EncodedChar>
double Js::NumberUtilities::StrToDbl(const EncodedChar *, const EncodedChar **, LikelyNumberType& , bool, bool)
{
    Assert(false);
    return 0.0;// don't care
}

#if defined(_M_IX86) || defined(_M_X64)
BOOL
AutoSystemInfo::SSE3Available() const
{
    Assert(false);
    return TRUE;
}

AutoSystemInfo AutoSystemInfo::Data;

void AutoSystemInfo::Initialize(void){}
#endif

#include "..\..\lib\Common\DataStructures\BigUInt.h"
#include "..\..\lib\Common\DataStructures\BigUInt.cpp"
