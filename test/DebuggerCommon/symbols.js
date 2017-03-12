//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test() {
    var o = Object.create(null);

    o[Symbol.iterator] = '[@@iterator]';
    o['string'] = '["string"]';
    o[Symbol('my symbol')] = Symbol('my value');
    o[Symbol('my symbol object')] = Object(Symbol('symobject'));
    o[Symbol('short symbol name')] = Symbol('s');
    o[Symbol('symbol with no name')] = Symbol();

    /**bp:evaluate('o',5)**/
}

test();

WScript.Echo("PASSED");
