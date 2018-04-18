//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    function bar() {
        // Deferred function   
    }

    return bar;
}

// Create two deferred functions
var func1 = foo();
var func2 = foo();

// Set the prototype of `func2` to `func1`
Object.setPrototypeOf(func2, func1);

// Set a property of `func2` first causing it to be undeferred
func2.x = 1;

// Set a property of `func1` second causing it to share the type of `func2`
func1.x = 2;

// Try to access an undefined property, this will loop infinitely
if (undefined === func1.y)
{
    WScript.Echo('pass');
}

