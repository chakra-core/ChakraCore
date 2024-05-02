//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Simpler mini-test harness to avoid any complicating factors when testing these jit bugs
var results = 0;
var test = 0;
const verbose = WScript.Arguments[0] != "summary";

function check(actual, expected) {
    if (actual != expected)
        throw new Error("Generator produced " + actual + " instead of " + expected);
    if (verbose)
        print('Result ' + ++results +  ' Generator correctly produced ' + actual);
}

function title (name) {
    if (verbose) {
        print("Beginning Test " + ++test + ": " + name);
        results = 0;
    }
}


// Test 1 - const that is unused/replaced with undefined
title("const that is unused/replaced with undefined");
function* gf1() {
    const temp2 = null;
    while (true) {
        yield temp2;
    }
}

const gen = gf1();

check(gen.next().value, null);
check(gen.next().value, null);
check(gen.next().value, null);

// Test 2 - load for-in enumerator in nested for-in loop
title("load for-in enumerator in nested for-in loop");
const arr = [0, 1, 2];
function* gf2() {
    for (let a in arr) {
        for (let b in arr) {
            yield a + b;
        }
    }
}

const gen2 = gf2();

check(gen2.next().value, "00");
check(gen2.next().value, "01");
check(gen2.next().value, "02");
check(gen2.next().value, "10");
check(gen2.next().value, "11");
check(gen2.next().value, "12");
check(gen2.next().value, "20");
check(gen2.next().value, "21");
check(gen2.next().value, "22");
check(gen2.next().value, undefined);

// Test 3 - Bail on no profile losing loop control variable
title("Bail on no profile losing loop control variable");
function* gf3() {
    for (let i = 0; i < 3; ++i) {
        yield i;
    }
}

const gen3 = gf3();

check(gen3.next().value, 0);
check(gen3.next().value, 1);
check(gen3.next().value, 2);

// Test 4 - yield* iterator fails to be restored after Bail on No Profile
title("Bail on no profile losing yield* iterator");
function* gf4() {
    yield 0;
    yield* [1,2,3];
}

const gen4 = gf4();

check(gen4.next().value, 0);
check(gen4.next().value, 1);
check(gen4.next().value, 2);
check(gen4.next().value, 3);

// Test 5 - scope slots fail to load inside for-in loop
title("Load Scope Slots in presence of for-in");
function* gf5(v1) {
    for(v0 in v1) {
        yield undefined;
        let v2 = {}
        function v3() { v2;}
    }
}

const gen5 = gf5([0, 1]);

check(gen5.next().value, undefined);
check(gen5.next().value, undefined);
check(gen5.next().value, undefined);
check(gen5.next().value, undefined);

// Test 6 - scope slots used in loop control have invalid values
title("Load Scope Slots used in loop control");
function* gf6 () {
    for (let v1 = 0; v1 < 1000; ++v1) {
        function foo() {v1;}
        yield v1;
    }
}

const gen6 = gf6();

check(gen6.next().value, 0);
check(gen6.next().value, 1);
check(gen6.next().value, 2);
check(gen6.next().value, 3);

// Test 7 - storing scoped slot from loop control in array 
title("Load Scope Slots used in loop control and captured indirectly");
function* gf7(v1) {
    for (const v2 in v1) {
        yield v2;
        const v4 = [v2];
        function foo() { v4; }
    }
}

const gen7 = gf7([0, 1, 2]);
check(gen7.next().value, 0);
check(gen7.next().value, 1);
check(gen7.next().value, 2);
check(gen7.next().value, undefined);

// Test 8 - copy prop'd sym is counted as two values - hits bookkeeping FailFast 
title("Copy prop sym double counted in unrestorable symbols hits FailFast");
function* gf8() {
    var v8 = 1.1;
    yield* [];
    yield {v8};
}

check(gf8().next().value.v8, 1.1);
check(gf8().next().value.v8, 1.1);
check(gf8().next().value.v8, 1.1);


print("pass");
