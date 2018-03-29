// //-------------------------------------------------------------------------------------------------------
// // Copyright (C) Microsoft. All rights reserved.
// // Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// //-------------------------------------------------------------------------------------------------------

function foo(a)
{ 
    try { a[0] } finally {}
    try { foo(0) } catch(e) {}
    try { foo() } catch(e) {}
}

foo(0)

WScript.Echo("PASS");
