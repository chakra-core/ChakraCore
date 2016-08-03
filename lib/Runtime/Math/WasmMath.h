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
    template<typename T> static int Eqz(T value);
    template<typename T> static T Div( T aLeft, T aRight );
    template<typename T> static T Rem( T aLeft, T aRight );
    template<typename T> static T Shl( T aLeft, T aRight );
    template<typename T> static T Shr( T aLeft, T aRight );
    template<typename T> static T ShrU( T aLeft, T aRight );
    template<typename T> static T Copysign(T aLeft, T aRight);
    template<typename T> static T Trunc(T aLeft);
    template<typename T> static T Nearest(T aLeft);
    template<typename T> static T PopCnt(T value);
    template<typename T> static T Ctz(T value);
    template<typename T> static T Clz(T value);
    template<typename T> static T Rol(T aLeft, T aRight);
    template<typename T> static T Ror(T aLeft, T aRight);
};

} //namespace Wasm

