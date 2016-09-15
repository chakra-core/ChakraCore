//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(x) {
    x = 10;
    x; /**bp:evaluate('arguments[0]');locals(1);**/
}
foo([]);

WScript.Echo("Pass");