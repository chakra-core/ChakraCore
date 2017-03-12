//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Sanity check that typed array constructor and object show up correctly in the debugger
Int8Array; /**bp:evaluate("Int8Array", 3)**/
var ta = new Int8Array([1]);
ta; /**bp:evaluate("ta", 3)**/

// Verify that all the TypedArray APIs that have callbacks show up on the call stack
function callback() {
    return; /**bp:stack()**/
}

ta2 = new Int8Array([1,2]);

ta.every(callback);
ta.filter(callback);
ta.find(callback);
ta.findIndex(callback);
ta.forEach(callback);
ta.map(callback);
ta2.reduce(callback);
ta2.reduceRight(callback);
ta.some(callback);
ta2.sort(callback);

WScript.Echo("PASS");
