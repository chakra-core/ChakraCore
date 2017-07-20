//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

async function foo( ) {
    var p1 = new Promise(
        timeout1 => {
            setTimeout( () => {
                timeout1( 1 );
            }, 500 );
        }
    );

    var p2 = new Promise(
        timeout2 => {
            setTimeout( () => {
                timeout2( 2 );
            }, 500 );
        }
    );

    return await p1 + await p2;
}

promise = foo();
promise.__proto__.then = (0x41414141 - 8) >> 0;

try {
    promise.then( function( value ){} );
    WScript.Echo("FAILED");
} catch (e) {
    if (e instanceof TypeError) {
        WScript.Echo("PASSED");
    } else {
        WScript.Echo("FAILED");
    }
}