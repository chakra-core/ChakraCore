//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Simpler mini-test harness to avoid any complicating factors when testing these jit bugs
var results = 0;
var test = 0;
const verbose = WScript.Arguments[0] != "summary";

function check(actual, expected) {
    if (actual != expected)
        throw new Error("Generator produced " + actual + " instead of " + expected);
    if (verbose)
        print('Result ' + ++results +  ' Generator correctly produced ' + actual);
}

function title (name) {
    if (verbose) {
        print("Beginning Test " + ++test + ": " + name);
        results = 0;
    }
}


// Test 1 - Construction after self-reference in loop control
title("Construction after self-reference in loop control");

function testOne()
{
	function* foo(a1,a2) {
	
		for (let i = a1; i < foo; i = i + a2)
        {
		    yield 0;
		}

		function bar() {}
		var b = new bar();
	}

    foo().next()
    return true;
}


for (var i = 0; i < 30 ;++i){
    testOne();
}

print('pass')
