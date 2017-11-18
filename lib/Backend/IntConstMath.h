//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class IntConstMath
{
public:
    static bool Add(IntConstType left, IntConstType right, IRType type, IntConstType * result);
    static bool Sub(IntConstType left, IntConstType right, IRType type, IntConstType * result);
    static bool Mul(IntConstType left, IntConstType right, IRType type, IntConstType * result);
    static bool Div(IntConstType left, IntConstType right, IRType type, IntConstType * result);
    static bool Mod(IntConstType left, IntConstType right, IRType type, IntConstType * result);

    static bool Dec(IntConstType val, IRType type, IntConstType * result);
    static bool Inc(IntConstType val, IRType type, IntConstType * result);
    static bool Neg(IntConstType val, IRType type, IntConstType * result);

private:
    static bool IsValid(IntConstType val, IRType type);
};
