//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Function expression

// Validate if the function expression shows in the locals window.

(function closureFoo(a) {
    if (a) {                            /**bp:locals(1)**/
        closureFoo();
    }
})(false);

(function closureFoo(a) {
    function inner() {
        if (a) {                        /**bp:locals(1)**/
            closureFoo();
        }
    }
    inner();                            /**bp:locals(1)**/
})(false);

function foo() {
    var j = function foo1(arg1) {
        foo1;                           /**bp:locals(1)**/
    }
    j(10);
}

foo();

function outer() {
    (function closureFoo(a) {
        if (a) {                       /**bp:locals(1)**/
            closureFoo;
        }
    })(false);

}
outer();
WScript.Echo("Pass");
