//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test(foo) {
    function nest() {
        {
            function foo() {
                console.log('pass');
            }
        }
        foo();
    }
    nest();
}
test(()=>console.log('fail'));
