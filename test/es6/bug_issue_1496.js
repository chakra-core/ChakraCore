//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// GitHub Issue1496: ES6 constructor returns class instead of object upon constructorCache hit
//
// -mic:1 -maxsimplejitruncount:2

var Test = {};

class A {
    constructor(foo) { this.foo = foo; }
    toB() {
        return new Test.B(this);
    }
}

class B {
    constructor(bar) { this.bar = bar; }
}

Test.B = B;

for (var i=0; i<10; i++)
{
    var a = new A(i);
    var b = a.toB();

    try
    {
        WScript.Echo(b.bar.foo);        
    }
    catch (e)
    {
        WScript.Echo(e);
        WScript.Echo(b);
        break;
    }
}
