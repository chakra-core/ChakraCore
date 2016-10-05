//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Test for fully qualified names

var a = 10;
var k = function() { 
    a;
    a++;/**bp:stack()**/
}
k();


k.subF1 = function() { 
    a;
    a++;/**bp:stack()**/
}
k.subF1();

k.subF1.subsubF1 = function() { 
    a;
    a++;/**bp:stack()**/
}

 var m = k.subF1.subsubF1;
 m();

var k2 = k.subF2 = function () {         
    a;
    a++;/**bp:stack()**/
}
 
 k2();

var k3 = 1;
k.subF3 = k3 = function () {         
    a;
    a++;/**bp:stack()**/
}

k3();

var obj1 = {}
obj1[0] = function () {
    a;
    a++;/**bp:stack()**/
}
obj1[0]();
obj1["test"] = function () {
    a;
    a++;/**bp:stack()**/
}
obj1["test"]();

function returnObj() { return obj1; }
returnObj()[2] = function () {
    a;
    a++;/**bp:stack()**/
}
obj1[2]();

obj1[0][0] = function () {
    a;
    a++;/**bp:stack()**/
}
obj1[0][0]();

WScript.Echo("Pass");
