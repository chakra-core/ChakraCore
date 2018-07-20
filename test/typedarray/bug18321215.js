//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// OS18321215: Typed array functions need to release the guest arena if errors happen.
// This is added as a seperate file and doesn't use the unittestframework as the test depends
// on certain GC behavior that doesn't trigger otherwise.
try
{
    // Type error in filter function
    WScript.LoadModule(`;`);
    (async () => {
        testArray1 = new Float32Array(1);
        testArray1.filter(function () {
            // type error here
            ArrayBuffer();
        });
    })().then();
    /x/, /x/, /x/, /x/;
}
catch(e){}

try
{
    // Type error in filter function
    WScript.LoadModule(``);
    for (var foo = 0; /x/g && 0; ) {
    }
    /x/g;
    /x/g;
    try {
        testArray2 = new Float64Array(1);
        testArray2.filter(function () {
            // type error here
            ArrayBuffer();
        });
    } catch (e) {
    }
    function bar(baz = /x/g) {
    }
}
catch(e){}

console.log("PASS");
