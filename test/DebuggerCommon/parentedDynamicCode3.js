//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var count = 0;
var myEval = eval;
function foo() {
    myEval("var x" + count + " = 0;"); // Eval the existing source
    myEval("var x" + count++ + " = 0;"); // Create new source
    var y; /**bp:evaluate('eval("x0==0;");');dumpSourceList();**/ //Simulates console eval
}
foo();
WScript.Attach(foo);
WScript.Detach(foo);
WScript.Attach(foo);
WScript.Echo("pass");
