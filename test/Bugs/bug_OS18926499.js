//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let throwingFunctions = [{
    msg: "Split-scope parent, param-scope child capturing symbol from parent body scope",
    body: function foo(a = (()=>+x)()) {
            function bar() { eval(''); }
            var x;
          }
},
{
    msg: "Split-scope parent, param-scope child capturing symbol from parent body scope",
    body: function foo(a = (()=>+x)()) {
            eval('');
            var x;
          }
},
{
    msg: "Merged-scope parent, param-scope child capturing symbol from parent body scope",
    body: function foo(a = () => +x) {
            var x = 1;
            return a();
          }
},
{
    msg: "Merged-scope parent, param-scope child capturing symbol from parent body scope",
    body: function foo(a = (()=>+x)()) {
            var x;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing symbol from parent body scope",
    body: foo3 = function foo3a(a = (function foo3b() { return +x; })()) {
            var x = 123;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing symbol from parent body scope",
    body: foo3 = function foo3a(a = (function foo3b() { return +x; })()) {
            eval('');
            var x = 123;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing symbol from parent body scope",
    body: foo3 = function foo3a(a = (function foo3b() { return +x; })()) {
            function bar() { eval(''); }
            var x = 123;
          }
},
{
    msg: "Param-scope func expr child with nested func expr capturing symbol from parent body scope",
    body: foo5 = function foo5a(a = (function(){(function(b = 123) { +x; })()})()) {
                    function bar() { eval(''); }
                    var x;
                 }
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { +x; })()){})()){})()){ var x;}
}];

let nonThrowingFunctions = [{
    msg: "Func expr parent, param-scope func expr child capturing parent func expr name",
    body: foo3 = function foo3a(a = (function foo3b() { +foo3a; })()) {
            return +a;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing own func expr name",
    body: foo3 = function foo3a(a = (function foo3b() { +foo3b; })()) {
            return +a;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing expression name hint",
    body: foo3 = function foo3a(a = (function foo3b() { +foo3; })()) {
            return +a;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing parent argument name",
    body: foo3 = function foo3a(b = 123, a = (function foo3b() { return +b; })()) {
            if (123 !== a) throw 123;
          }
},
{
    msg: "Func expr parent, param-scope func expr child capturing symbol from parent body scope",
    body: foo3 = function foo3a(b = 123, a = (function foo3b() { return +b; })()) {
            if (123 !== a) throw 123;
          }
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { foo3d; })()){})()){})()){}
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { foo3c; })()){})()){})()){}
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { foo3b; })()){})()){})()){}
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { foo3a; })()){})()){})()){}
},
{
    msg: "Multiple nested func expr, inner param-scope function capturing outer func expr name",
    body: foo3 = function foo3a(a = (function foo3b(b = (function foo3c(c = (function foo3d() { foo3; })()){})()){})()){}
}];

for (let fn of throwingFunctions) {
    try {
        fn.body();
        console.log(`fail: ${fn.msg}`);
    } catch (e) {
        console.log("pass");
    }
}

for (let fn of nonThrowingFunctions) {
    try {
        fn.body();
        console.log("pass");
    } catch (e) {
        console.log(`fail: ${fn.msg}`);
    }
}
