//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let pass = 'pass';
let fail = 'fail';

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

function func6(a = v => { console.log(pass); }, b = v => { return a; }) {
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

function func13(a = (b = () => pass, c = () => fail) => b()) {
    return a();
}
console.log(func13());

function func14(a = (b = () => { return fail; }, c = () => { return pass; }) => { return c(); }) {
    return a();
}
console.log(func14());

function func15(a = class A { meth() { return pass } static meth2() { return fail; } }, b = v => fail, c = (v) => { return fail }, d = fail) {
    return new a().meth();
}
console.log(func15());
function func16(a = class A { meth() { return fail } static meth2() { return pass; } }, b = v => fail, c = (v) => { return fail }, d = fail) {
    return a.meth2();
}
console.log(func16());
function func17(a = class A { meth() { return fail } static meth2() { return fail; } }, b = v => pass, c = (v) => { return fail }, d = fail) {
    return b();
}
console.log(func17());
function func18(a = class A { meth() { return fail } static meth2() { return fail; } }, b = v => fail, c = (v) => { return pass }, d = fail) {
    return c();
}
console.log(func18());

function func19() {
  return (function(a = { b() {} }){ return pass; })();
}
console.log(func19());

function func20() {
  return (function(a = { b() {} }, c = function() { return pass; }){ return c(); })();
}
console.log(func20());
