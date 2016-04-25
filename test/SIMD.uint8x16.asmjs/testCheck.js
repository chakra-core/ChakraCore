//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function module0(stdlib) {
    "use asm"
    var b = stdlib.SIMD.Uint8x16;
    var band = b.and;
    var bcheck = b.check;
    function foo() {
        var abc = b(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
        return bcheck(band(abc, abc));
    }
    return { foo:foo }
}
var c = module0(this);

function module(stdlib) {
    "use asm"
    var b = stdlib.SIMD.Uint8x16;
    var band = b.and;
    function foo() {
        var abc = b(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1);
        return band(abc, abc);
    }
    return { foo:foo }
}
var c = module(this);
