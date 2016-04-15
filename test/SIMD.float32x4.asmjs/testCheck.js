//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function module0(stdlib) {
    "use asm"
    var b = stdlib.SIMD.Float32x4;
    var badd = b.add;
    var bcheck = b.check;
    function foo() {
        var abc = b(1.0,1.0,1.0,1.0);
        return bcheck(badd(abc, abc));
    }
    return { foo:foo }
}
var c = module0(this);

function module(stdlib) {
    "use asm"
    var b = stdlib.SIMD.Float32x4;
    var badd = b.add;
    function foo() {
        var abc = b(1.0,1.0,1.0,1.0);
        return badd(abc, abc);
    }
    return { foo:foo }
}
var c = module(this);
