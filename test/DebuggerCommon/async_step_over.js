//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo() {
    return 100;
}

async function af1(a) {
    return await foo();
}

async function af2() {
    return await af1(10);
}

var p = af2();/**bp:
    resume('step_into');
    resume('step_into');
    resume('step_over');stack();
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

p = af2();/**bp:resume('step_over');stack();**/
p.then(result => {
        if (result === 100) {
            print("PASS");
        }
    },
    error => {
        print("Failed : " + error);
    } 
);