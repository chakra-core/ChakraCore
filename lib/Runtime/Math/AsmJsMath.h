//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class AsmJsMath
    {
    public:
        template<typename T> static T Add( T aLeft, T aRight );
        template<typename T> static T Sub( T aLeft, T aRight );
        template<typename T> static T Mul( T aLeft, T aRight );
        template<typename T> static T Div( T aLeft, T aRight );
        template<typename T> static T Rem( T aLeft, T aRight );
        template<typename T> static T Min( T aLeft, T aRight );
        template<typename T> static T Max( T aLeft, T aRight );

        template<typename T = int> static T And( T aLeft, T aRight );
        template<typename T = int> static T Or( T aLeft, T aRight );
        template<typename T = int> static T Xor( T aLeft, T aRight );
        template<typename T = int> static T Shl( T aLeft, T aRight );
        template<typename T = int> static T Shr( T aLeft, T aRight );
        template<typename T = int> static T ShrU( T aLeft, T aRight );
        template<typename T> static T Neg( T aLeft);
        static int Not( int aLeft);
        static int LogNot( int aLeft);
        static int ToBool( int aLeft );
        static int Clz32( int value);

        template<typename T> static int CmpLt( T aLeft, T aRight );
        template<typename T> static int CmpLe( T aLeft, T aRight );
        template<typename T> static int CmpGt( T aLeft, T aRight );
        template<typename T> static int CmpGe( T aLeft, T aRight );
        template<typename T> static int CmpEq( T aLeft, T aRight );
        template<typename T> static int CmpNe( T aLeft, T aRight );
    };



}
