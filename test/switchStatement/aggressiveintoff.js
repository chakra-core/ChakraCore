//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function opt() {
    for (let i = 0; i < 100; i++) {
        let j = i - 2;
        switch (i) {
            case 2:
            case 4:
            case 6:
            case 8:
            case 10:
            case 12:
            case 14:
            case 16:
            case 18:
            case 20:
            case 22:
            case 24:
            case 26:
            case 28:
            case 30:
            case 32:
            case 34:
            case 36:
            case 38:
                break;
        }

        if (i == 90) {
            i = 'x';
        }
    }
}

function main() {
    for (let i = 0; i < 100; i++) {
        opt();
    }
}

main();

WScript.Echo('pass');
