//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This test depicts that how javascript array changes to ES5 array while fetching values.

var test10_a = [1];

Object.defineProperty(
    test10_a,
    "p",
    {
        configurable: true,
        enumerable: true,
        get: function() {
            Object.defineProperty(
                test10_a,
                "1",
                {
                    configurable: true,
                    enumerable: true,
                    get: function() { return 5; },
                    set: function() { }
                });
            return 2;
        }
    });

test10_a;
test10_a;               /**bp:evaluate('test10_a',1);evaluate('test10_a',1)**/

WScript.Echo("Pass");
	