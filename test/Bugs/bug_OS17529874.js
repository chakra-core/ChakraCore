//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function w_n() {
    function foo() {
        console.log("pass");
    }

    function bar() {
        with ({}) {
            foo();
        }
    }
    bar();
};
w_n()
console.log('pass');
