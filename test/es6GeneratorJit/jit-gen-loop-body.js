//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let bar = [];
let j = 0;

function printif (text, x, mod) {
    if (x % mod == 0) {
        print (text + x);
    }
}

function* basicGenerator(a, b) {
    print("Beginning test of " + arguments.callee.name);
    yield "yield 2nd param b =" + b;
    var foo = [0.1, 0.2, 0.3, 0.4, 0];
    while (a < 100) { // Loop 0 - should be jitted
        foo[a] = Math.random();
        printif(arguments.callee.name + " Loop 0 a = ", a, 40);
        printif(arguments.callee.name + " Loop 0 arguments[0] = ", arguments[0], 40);
        for (let k in foo) { bar[k] = a - k;} // Loop 1 - shuld be jitted
        ++a;
    }
    while (true){ // Loop 2 - should not be jitted
        for (j = 0; j < 100; ++j) { // Loop 3 - should not be jitted
            if (j % 40 == 1)
                yield j;

           foo[j] = j;
        }
        print("Loop 3 completed");
        for (let k of bar) { // Loop 4 - should be jitted
            printif("Loop 4 k = ", k, 40);
            foo[0] = foo[1] + 1;
        }
        print("Loop 4 completed");
        yield* bar; // Loop 5 - should not be jitted
        return j;
    }
}

const gen = basicGenerator(5, 1);
for (let x = gen.next(); !x.done; x = gen.next() ) {
    printif("yielded value = ", x.value, 40);
}
