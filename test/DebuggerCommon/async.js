//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    return this.x; /**bp:locals(1);stack()**/
}

async function af1(a) {
    await a;
    return await foo.call({ x : 100 }); /**bp:locals();stack()**/
}

async function af2() {
    return await af1(10);
}

var p = af2();/**bp:
    resume('step_into');stack();
    resume('step_into');stack();
    resume('step_into');stack();
**/
p.then(result => {
        if (result === 100) {
            print("PASS");
        }
    },
    error => {
        print("Failed : " + error);
    } 
);
