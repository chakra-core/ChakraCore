//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function check(val, equals, msg) {
    if(val !== equals) {
        throw new Error(msg);
    }
}

function test () {
    var arr = [1, 2, 3, 4];
    Object.defineProperty(arr, 1, {configurable: false});
    for(var i = 0; i < 80; i++) {
        if(!arr.length) {
            arr[0] = -1;
        }
        else {
            arr.length = 0;
            check(arr.length, 2, "cannot delete a non-configurable property");
        }
    }
}

for(var j = 0; j < 80; j++) {
    test();
}

print("PASS");
