//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function fn() {
    var o = {};
    var p = {};
    o.__proto__ = p;
    o.pizza;                                  // Fill missing property cache
    p.__proto__ = { pizza: 'pizza' };         // Add property to o's prototype chain: should invalidate cache
    return o.pizza;                           // Look for property again
}
if (fn() === 'pizza')
    WScript.Echo('pass');
