//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var count = 0;
class A {
    constructor() { count++; }
    increment() { count++; }
}
class B extends A {
    constructor() {
        super();
        ((B) => { super.increment() })();
        (A=> { super.increment() })();
        let C = async (B) => { B };
        let D = async A => { A };
    }
}
let b = new B();
class async extends A {
    constructor() {
        super();
        let Q = async A => { A };
    }
}
let a = new async();
if (count !== 4) {
    WScript.Echo('fail');
}

(vjczgj = (function(y) { try{}catch(e){} }), fkvcij = (y)) => {};

(omabpn = (function(x) {return { getOwnPropertyNames: function(){ switch({}) { case 0:  "" ; }},  }; }), pkgrln = (TypeError(x))) => {};

var error = false;
try {
    eval('( oqixuw = function  window () {}, (window)) => undefined;');
}
catch(e) {
    // Above all should throw syntax error on parenthesized name in formals list
    error = true;    
}
if (!error) WScript.Echo('fail');

WScript.Echo('pass');
