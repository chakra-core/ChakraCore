//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f() {
    function inner() { }
    inner.__proto__ = {b:'b'};  // Put enumerable property into prototype chain
    new inner();                // Populate ctor cache
    for (var s in inner) {      // Cache 'prototype' in TypePropertyCache while enumerating
        inner[s];
    }
    inner.prototype = {sox:'red'}; // Set new prototype, using inline cache on 2nd invocation
    return new inner();            // On 2nd invocation, try to construct using stale ctor cache
}

f();
var Boston = f();
if (Boston.sox === 'red')
    WScript.Echo('pass');

