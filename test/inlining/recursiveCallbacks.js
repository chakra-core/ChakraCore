//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function r(t) {
    if (!(this instanceof r)) {
        return new r(t);
    }
}

function foo() {}

r(foo);
r(foo);
r(foo);

WScript.Echo("Passed");