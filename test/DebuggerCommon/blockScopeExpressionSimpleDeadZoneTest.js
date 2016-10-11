//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


// Tests that simple expression evaluation works
// for variables in a dead zone.

function test() {
    /**bp:evaluate('a')**/
    let a = 0;
    /**bp:evaluate('a')**/
    a;
    /**bp:evaluate('c')**/
    const c = 1;
    /**bp:evaluate('c')**/
};

test();
WScript.Echo("Pass");