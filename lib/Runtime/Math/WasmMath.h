//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class WasmMath
{
public:
    static int Ctz(int value);
    template<typename T> static T Copysign(T aLeft, T aRight);
    static int Eqz(int value);
    template<typename T> static T Trunc(T aLeft);
    template<typename T> static T Nearest(T aLeft);
    static int Rol(int aLeft, int aRight);
    static int Ror(int aLeft, int aRight);
};

} //namespace Wasm

