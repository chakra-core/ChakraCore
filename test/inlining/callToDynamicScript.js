//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Minimal switches: -bgjit- -loopinterpretcount:1

// Either a.b.call() should not be inlined, or a.b should be constructed in such a way that
// attempting to recursively inline its contents does not assert.
function test0(){
    const a = { b: new Function("[].slice();") };
    for (let i = 0; i < 2; ++i)
        a.b.call();
};

test0();
WScript.Echo("Passed");