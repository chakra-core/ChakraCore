//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Inspecting frames - Array ES5
*/


try {
    e++;
} catch (e) {
    with ({ o: 2 }) {
        var arr = [];
        arr.push(1);

        arr.forEach(function (key, val, map) {
            key;/**bp:stack();locals()**/
        });
    }
}
WScript.Echo('PASSED');
