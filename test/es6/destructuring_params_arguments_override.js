//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f() {
    ({a = () => {
        let arguments;
    }} = 1);

    arguments.x;
}

f();

function g() {
    ({a = ([arguments]) => {}} = 1);

    arguments.x;
}

g();

WScript.Echo('pass');
