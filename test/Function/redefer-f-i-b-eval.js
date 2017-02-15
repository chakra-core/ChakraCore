//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

eval(
'function outer() {' +
'    var f = "f";' +
'    if (true) {' +
'        let o = { x : function() { if (f !== "f") { WScript.Echo("fail"); throw 1; } } };' +
'        function i() {}' +
'        o.x();' +
'    }' +
'}');

for (var i = 0; i < 100; i++)
    arr = [10000];
outer();
WScript.Echo('pass');
