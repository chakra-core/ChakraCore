//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let pass = 'pass';

function func4(a = 123) {
    function v8() {
        function v9() {
            return v9;
        }
        return v9();
    }
    return v8();
}
func4();


var func5 = (a = 123) => (function v6() {
    function v7() {
        return v7;
    }
    return v7();
})()
func5();

function func6(a = v => { console.log('pass'); }, b = v => { return a; }) {
    function c() {
        return b();
    }
    return c();
}
func6()();

function func7(a, b = function() { return pass; }, c) {
    function func8(d, e = function() { return b; }, f) {
        return e;
    }
    return func8();
}
console.log(func7()()());

var func9 = (a, b = () => pass, c) => {
    var func10 = (d, e = () => b, f) => {
        return e;
    }
    return func10();
}
console.log(func9()()());

var func11 = (a, b = () => { return pass; }, c) => {
    var func12 = (d, e = () => { return b; }, f) => {
        return e;
    }
    return func12();
}
console.log(func11()()());
