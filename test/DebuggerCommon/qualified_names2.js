//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Validation of fully qualified names (object literals and es6 classes)
var a = 10;
var f2 = function () {}
 
f2.prototype = { 
        subF1 : function () { 
            a;
            a++;/**bp:stack()**/
        },
        subInt : 10,
        subF2 : function () { 
            a;
            a++;/**bp:stack()**/
        }
        
 }

var obj1 = new f2();
obj1.subF1();
obj1.subF2();


f2.prototype = { subF3 : { subSubF3 : function () { 
        a;
        a++;/**bp:stack()**/
 } } }
 
obj1 = new f2();
obj1.subF3.subSubF3();

var Foo = function () {
    this.subF1 = function () {         
        a;
        a++;/**bp:stack()**/
    }
    this.val = "value"
    this.subF2 = function () {         
        a;
        a++;/**bp:stack()**/
    }
}

obj1 = new Foo();
obj1.subF1();
obj1.subF2();

class OneClass {

    constructor(a) { 
        a;
        a++;/**bp:stack()**/ 
    }
    static method1() {     
        a;
        a++;/**bp:stack()**/ 
    }
    
    method() { 
        a;
        a++;/**bp:stack()**/ 
    }
    
    get method2() {
        var str = "getter";
        a++;/**bp:evaluate('str');stack()**/ 
        return a;
    }
    
    set method2(abc) { 
        var str = "setter";
        a++;/**bp:evaluate('str');stack()**/ 
    }
}

var obj = new OneClass();
obj.method();
OneClass.method1();
var k = obj.method2;
obj.method2 = 31;
 
WScript.Echo("Pass");