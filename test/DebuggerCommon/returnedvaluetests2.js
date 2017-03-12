//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validating the object returned and step-in and step-out

var obj1 = {B : function () { return {x:"x"}; } }
function A() {
    return obj1;
}

function test1() {
    var o = A();    /**bp:locals();resume('step_over');locals();resume('step_over');locals(1);resume('step_over');locals(1);**/
    o = A().B();
    return o;
}
test1();

function f1() { return 70; }

function f2() {
    var m = 31;
    m++;
    var coll = new Intl.Collator();
    m += f1();
    return m;
}

function test2() {
    var k = f2();   /**bp:locals();resume('step_into');locals();resume('step_out');locals();removeExpr();**/ 
    k++;
}

test2();

WScript.Echo("Pass");