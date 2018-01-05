//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function opt(arr, start, end) {
    for (let i = start; i < end; i++) {
        if (i === 10) {
            i += 0;
        }
        arr[i] = 2.3023e-320;
    }
}

let arr = new Array(100);

function main() {
    arr.fill(1.1);

    for (let i = 0; i < 1000; i++)
        opt(arr, 0, 3);

    opt(arr, 0, 100000);
}

main();

WScript.Echo(arr[0] === 2.3023e-320 ? 'pass' : 'fail');
