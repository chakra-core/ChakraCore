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

var p = 1;/**bp:
    resume('step_over');
    resume('step_into');
    resume('step_into');
    resume('step_out');stack();
    resume('step_out');stack();**/
// If we put the break point in the below line after stepping out from af2 we will execute the same break
// point again which will add more break points. To avoid this adding the break point to a dummy statement.
p = af2();

p.then(result => {
        if (result === 100) {
            print("PASS");
        }
    },
    error => {
        print("Failed : " + error);
    } 
);