//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var value = 10;
function f1(arguments)
{
    if (arguments === value) {
        print("PASSED");
    } else {
        print("FAILED : " + arguments);
    }
}
f1(value);