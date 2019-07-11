//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(a = (()=>+x)()) {
    function bar() { eval(''); }
    var x;
}

try {
    foo();
    console.log('fail');
} catch {
    console.log("pass");
}

function foo2(a = () => x) { var x = 1; return a(); }

try {
    foo2()
    console.log('fail');
} catch {
    console.log('pass');
}

var foo3 = function foo3a(a = (function foo3b() { +foo3a; })()) {
    a;
};

try {
    foo3()
    console.log('pass');
} catch {
    console.log('fail');
}

var foo4 = function foo4a(a = (function() { +x; })()) {
    function bar() { eval(''); }
    var x;
};

try {
    foo4()
    console.log('fail');
} catch {
    console.log('pass');
}

var foo5 = function foo5a(a = (function(){(function(b = 123) { +x; })()})()) {
    function bar() { eval(''); }
    var x;
};

try {
    foo5();
    console.log('fail');
} catch {
    console.log('pass');
}

function foo6(a, b = (function() {a; +x;})()) {
    function bar() { eval(''); }
    var x;
};

try {
    foo6();
    console.log('fail');
} catch {
    console.log('pass');
}

var foo8 = function foo8a(b, a = (function foo8b() { console.log(x); })()) {
    a;
    var x = 'fail';
};

try {
    foo8()
    console.log('fail');
} catch {
    console.log('pass');
}

var foo9 = function foo9a(b = 'pass', a = (function foo9b() { console.log(b); })()) {
};

try {
    foo9()
    console.log('pass');
} catch {
    console.log('fail');
}
