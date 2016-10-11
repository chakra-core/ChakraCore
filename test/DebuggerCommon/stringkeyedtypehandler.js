//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    var o = { a: 1 };
    // converts type handler to string keyed type handler
    // (requires -DeletedPropertyReuseThreshold:1)
    delete o.a;

    o["b"] = 10;
    o["c"] = 20;

    // Ensure that dictionary lookup of properties works for string keyed dictionary
    // by adding 'constructor' property which JScriptDiag looks up on all objects
    o["constructor"] = undefined;

    o;
    /**bp:locals(2)**/
};
var o = { };
foo.call(o);

WScript.Echo("pass");
