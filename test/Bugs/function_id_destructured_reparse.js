//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    ({
        p: o = ({
            bar() {
                (function () {})
            }
        },
        (this))
    } = 0)
}
test0()

console.log('pass');
