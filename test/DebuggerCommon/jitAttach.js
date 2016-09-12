//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var passed = false;

function test0() {
    function makeArrayLength(x) {
        x > 4294967295 || isNaN(x) 
    }
    makeArrayLength();
    makeArrayLength();
    if(passed) 
    {
        WScript.Echo("PASSED");
    }
};
test0();
test0();
passed = true;
WScript.Attach(test0);
