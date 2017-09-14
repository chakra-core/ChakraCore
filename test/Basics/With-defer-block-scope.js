//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let h = function f(a0 = (function () {
    a0;
    a1;
    a2;
    a3;
    a4;
    a5;
    a6;
    a7 = 0x99999; // oob write

    with ({});
})(), a1, a2, a3, a4, a5, a6, a7) {
    function g() {
        f;
    }
};

for (let i = 0; i < 0x10000; i++) {
h();
}

WScript.Echo('pass');
