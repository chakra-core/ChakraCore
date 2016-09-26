//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class Int64Math: public IntMathCommon<int64>
{
public:
    static bool Mul(int64 left, int64 right, int64 *pResult);
    static bool Shl(int64 left, int64 right, int64 *pResult);
    static bool Shr(int64 left, int64 right, int64 *pResult);
    static bool ShrU(int64 left, int64 right, int64 *pResult);
};
