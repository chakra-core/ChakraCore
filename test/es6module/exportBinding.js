//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

export class A1 {
    attemptBindingChange() { A1 = 1; }
    bindingUnmodified() { return A1 !== 1; }
}
var A1Inst = new A1();

assert.throws(function () { A1Inst.attemptBindingChange(); }, TypeError, 'Exported class name captured inside class is const', "Assignment to const");
assert.doesNotThrow(function () { A1 = 1; }, 'Exported class name decl binding is mutable outside class');
assert.throws(function () { A1Inst.attemptBindingChange(); }, TypeError, 'Mutation of class decl did not change constness', "Assignment to const");
assert.areEqual(true, A1Inst.bindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { A1Inst.attemptBindingChange(); }, TypeError, 'Exported class name captured inside class is still const', "Assignment to const");

class A2 {
    attemptBindingChange() { A2 = 1; }
    bindingUnmodified() { return A2 !== 1; }
}
var A2Inst = new A2();
export { A2 };

assert.throws(function () { A2Inst.attemptBindingChange(); }, TypeError, 'Delay exported class name captured in class is const', "Assignment to const");
assert.doesNotThrow(function () { A2 = 1; }, 'Delay exported class name decl binding is mutable outside class');
assert.throws(function () { A2Inst.attemptBindingChange(); }, TypeError, 'Mutation of class decl did not change constness', "Assignment to const");
assert.areEqual(true, A2Inst.bindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { A2Inst.attemptBindingChange(); }, TypeError, 'Delay exported class name captured in class is still const', "Assignment to const");

export let B1 = class B1 {
    attemptBindingChange() { B1 = 1; }
    bindingUnmodified() { return B1 !== 1; }
}
var B1Inst = new B1();

assert.throws(function () { B1Inst.attemptBindingChange(); }, TypeError, 'Exported class expr name captured inside class is const', "Assignment to const");
assert.doesNotThrow(function () { B1 = 1; }, 'Exported class expr name decl binding is mutable outside class');
assert.throws(function () { B1Inst.attemptBindingChange(); }, TypeError, 'Mutation of class expr decl did not change constness', "Assignment to const");
assert.areEqual(true, B1Inst.bindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { B1Inst.attemptBindingChange(); }, TypeError, 'Exported class expr name captured inside class is still const', "Assignment to const");

let B2 = class B2 {
    attemptBindingChange() { B2 = 1; }
    bindingUnmodified() { return B2 !== 1; }
}
var B2Inst = new B2();
export { B2 };

assert.throws(function () { B2Inst.attemptBindingChange(); }, TypeError, 'Delay exported class expr name captured in class is const', "Assignment to const");
assert.doesNotThrow(function () { B2 = 1; }, 'Delay exported class expr name decl binding is mutable outside class');
assert.throws(function () { B2Inst.attemptBindingChange(); }, TypeError, 'Mutation of class expr decl did not change constness', "Assignment to const");
assert.areEqual(true, B2Inst.bindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { B2Inst.attemptBindingChange(); }, TypeError, 'Delay exported class expr name captured in class is still const', "Assignment to const");

export let C1 = class NotC1 {
    attemptOuterBindingChange() { C1 = 1; }
    attemptInnerBindingChange() { NotC1 = 1; }
    outerbindingUnmodified() { return C1 !== 1; }
    innerbindingUnmodified() { return NotC1 !== 1; }
}
var C1Inst = new C1();

assert.throws(function () { C1Inst.attemptInnerBindingChange(); }, TypeError, 'Exported class expr name captured inside class is const', "Assignment to const");
assert.doesNotThrow(function () { C1Inst.attemptOuterBindingChange(); }, 'Exported class expr name decl binding is mutable outside class');
assert.throws(function () { C1Inst.attemptInnerBindingChange(); }, TypeError, 'Mutation of class expr decl did not change constness', "Assignment to const");
assert.areEqual(false, C1Inst.outerbindingUnmodified(), 'Captured class decl observes mutation');
assert.areEqual(true, C1Inst.innerbindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { C1Inst.attemptInnerBindingChange(); }, TypeError, 'Exported class expr name captured inside class is still const', "Assignment to const");

let C2 = class NotC2 {
    attemptOuterBindingChange() { C2 = 1; }
    attemptInnerBindingChange() { NotC2 = 1; }
    outerbindingUnmodified() { return C2 !== 1; }
    innerbindingUnmodified() { return NotC2 !== 1; }
}
var C2Inst = new C2();
export { C2 };

assert.throws(function () { C2Inst.attemptInnerBindingChange(); }, TypeError, 'Exported class expr name captured inside class is const', "Assignment to const");
assert.doesNotThrow(function () { C2Inst.attemptOuterBindingChange(); }, 'Exported class expr name decl binding is mutable outside class');
assert.throws(function () { C2Inst.attemptInnerBindingChange(); }, TypeError, 'Mutation of class expr decl did not change constness', "Assignment to const");
assert.areEqual(false, C2Inst.outerbindingUnmodified(), 'Captured class decl observes mutation');
assert.areEqual(true, C2Inst.innerbindingUnmodified(), 'Captured class name does not observe mutation');
assert.throws(function () { C2Inst.attemptInnerBindingChange(); }, TypeError, 'Exported class expr name captured inside class is still const', "Assignment to const");

console.log("PASS");