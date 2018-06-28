//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/**exception(uncaught):stack();**/

async function f1() {
    await null;
    throw new Error('error in f1');
}
f1();

async function f2() {

    async function f2a() {
        throw "err";
    }

    async function f2b() {
        try {
            var p = f2a();
        } catch (e) {
            console.log("caught " + e);
        }
    }
    
    async function f2c() {
        var p = f2a();
    }

    f2b();
    f2c();
}
f2();
