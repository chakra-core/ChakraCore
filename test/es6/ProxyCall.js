//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function f() {
    arguments;
    WScript.Echo('pass');
}

let call = new Proxy(Function.prototype.call, {}); // proxy calls set the flag
call.call(f);
