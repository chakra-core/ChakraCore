//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function module0(stdlib) {
    "use asm"
    var b8 = stdlib.SIMD.Bool16x8;
    var b8and = b8.and;
    var b8check = b8.check;
    function foo() {
        var abc = b8(1,1,1,1,1,1,1,1);
        return b8check(b8and(abc, abc));
    }
    return { foo:foo }
}
var c = module0(this);

function module(stdlib) {
    "use asm"
    var b8 = stdlib.SIMD.Bool16x8;
    var b8and = b8.and;
    function foo() {
        var abc = b8(1,1,1,1,1,1,1,1);
        return b8and(abc, abc);
    }
    return { foo:foo }
}
var c = module(this);
