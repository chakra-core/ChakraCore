//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that function declaration bindings show correctly for the
// global function case where shadowing occurs from above
// (with no var binding created).
// Bug 305562.

let f = 'let f';
f; /**bp:evaluate('f')**/
if (true)
{
    function f() { }
    f; /**bp:evaluate('f')**/
}
f; /**bp:evaluate('f')**/

WScript.Echo("PASSED");