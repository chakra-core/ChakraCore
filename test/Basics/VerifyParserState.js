//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Following lines generate a baseline with flags:
// -UseParserStateCache -ParserStateCache -ForceDeferParse -Trace:CreateParserState

console.log('nested function capturing var decl');
WScript.LoadScript(`
function f_1() {
    var a1 = 0;
    function f_2() {
        var b1 = 0;
        return a1 + b1;
    }
}
`);
console.log();

console.log('nested function capturing a let decl');
WScript.LoadScript(`
function f_1() {
    let a2 = 0;
    function f_2() {
        let b2 = 0;
        return a2 + b2;
    }
}
`);
console.log();

console.log('nested function capturing a const decl');
WScript.LoadScript(`
function f_1() {
    const a3 = 0;
    function f_2() {
        const b3 = 0;
        return a3 + b3;
    }
}
`);
console.log();

console.log('nested function capturing non-decl names');
WScript.LoadScript(`
function f_1() {
    function f_2() {
        return a4 + b4;
    }
}
`);
console.log();

console.log('nested function referencing a local shadowing formal');
WScript.LoadScript(`
function f_1(a5) {
    function f_2(a5) {
        return a5;
    }
}
`);
console.log();

console.log('nested function capturing a named formal');
WScript.LoadScript(`
function f_1(a6) {
    function f_2(b6) {
        return a6 + b6;
    }
}
`);
console.log();

console.log('nested functions capturing global declarations');
WScript.LoadScript(`
var v7;
let l7;
const c7 = 7;
function f_1(a7) {
    function f_2(b7) {
        function f_3() {
            return a7 + b7 + v7 + l7 + c7 + d7 + a7 + b7 + v7 + l7 + c7 + d7;
        }
        return a7 + b7 + v7 + l7 + c7 + d7;
    }
    return a7 + b7 + v7 + l7 + c7;
}
`);
console.log();

console.log('nested function capturing a class decl');
WScript.LoadScript(`
class class_1 { };
function f_2() {
    function f_3() {
        return class_1;
    }
    return class_1;
}
`);
console.log();

console.log('object literal methods capturing names');
WScript.LoadScript(`
var v9;
let l9;
const c9 = 9;
let o_9 = {
    f_1(a9) { return a9 + b9 + c9 + l9 + v9 + o_9; },
    *f_2(a9) { return a9 + b9 + c9 + l9 + v9 + o_9; },
    async f_3(a9) { return a9 + b9 + c9 + l9 + v9 + o_9; },
    ['f_4'](a9) { return a9 + b9 + c9 + l9 + v9 + o_9; },
    *['f_5'](a9) { return a9 + b9 + c9 + l9 + v9 + o_9; },
    get f_6() { return a9 + b9 + c9 + l9 + v9 + o_9; },
    set f_7(a9) { a9 + b9 + c9 + l9 + v9 + o_9; }
}
`);
console.log();

console.log('class members capturing names');
WScript.LoadScript(`
var v10;
let l10;
const c10 = 10;
class class_10 {
    f_1(a10) { return a10 + b10 + c10 + l10 + v10 + class_10; }
    *f_2(a10) { return a10 + b10 + c10 + l10 + v10 + class_10; }
    async f_3(a10) { return a10 + b10 + c10 + l10 + v10 + class_10; }
    ['f_4'](a10) { return a10 + b10 + c10 + l10 + v10 + class_10; }
    *['f_5'](a10) { return a10 + b10 + c10 + l10 + v10 + class_10; }
    get f_6() { return a10 + b10 + c10 + l10 + v10 + class_10; }
    set f_7(a10) { a10 + b10 + c10 + l10 + v10 + class_10; }
}
`);
console.log();

console.log('lambda capturing special names');
WScript.LoadScript(`
function f_1() {
    () => arguments; // function 2
    () => this; // function 3
    () => new.target; // function 4
    
    class c extends null {
        constructor() { // function 5
            () => super(); // function 6
        }
        method() { // function 7
            () => super.foo(); // function 8
        }
    }
}
`);
console.log();

console.log('nested function with shadowing block-scoped name');
WScript.LoadScript(`
var v12;
let l12;
function f_1(a12) {
    {
        let l12;
        return l12;
    }
    return v12;
}
`);
console.log();

console.log('nested functions with a few nested captures');
WScript.LoadScript(`
let a13;
function f_1() {
    let b13;
    function f_2() {
        let c13;
        {
            let d13;
            function f_3() {
                return d13 + c13 + b13 + a13;
            }
        }
    }
    function f_4() {
        function f_5() {
            return b13 + f13;
        }
        return e13;
    }
}
`);
console.log();

console.log('names referenced in param scope can be captured');
WScript.LoadScript(`
let a14, c14;
function f_1(b14 = a14, d14 = () => c14) {
    return b14 + d14;
}
`);
console.log();

console.log('eval referencing and declaring names');
WScript.LoadScript(`
let a15, b15;
function f_1() {
    eval('var c15 = a15;');
    return c15 + b15;
}
`);
console.log();

console.log('named function expression referencing itself by name');
WScript.LoadScript(`
(function f_1() {
    return f_1;
})()
`);
console.log();

console.log('nested function containing a with statement which has a property shadowing a local');
WScript.LoadScript(`
var a17 = 'a', b17 = 'b';
var o17 = { c17: 'c', d17: 'd' }
function f_1() {
    let d17 = 'not d';
    with(o17) {
        return a17 + d17;
    }
}
`);
console.log();

console.log('nested function containing a with statement which captures a name');
WScript.LoadScript(`
var a18 = 'a', b18 = 'b';
var o18 = { c18: 'c', d18: 'd' }
function f_1() {
    with(o18) {
        return a18 + d18;
    }
}
`);
console.log();
