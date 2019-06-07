//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a = (()=>+x)()) {
    function bar() { eval(''); }
    var x;
}

try {
    foo();
    console.log('fail');
} catch {
    console.log("pass");
}

function foo2(a = () => x) { var x = 1; return a(); }

try {
    foo2()
    console.log('fail');
} catch {
    console.log('pass');
}
