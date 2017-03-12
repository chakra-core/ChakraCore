//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var count = 0;
function foo() {
    eval("var x" + count++ + " = " + count + ";");
    if (count >= 3) {
        // New dynamic code added after detach
        eval('var y = 1;');
        var f1 = new Function("");
        f1();
    }
    var y; /**bp:dumpSourceList();**/
}
foo();
WScript.Attach(foo);
WScript.Detach(foo);
WScript.Attach(foo);
WScript.Echo("pass");
