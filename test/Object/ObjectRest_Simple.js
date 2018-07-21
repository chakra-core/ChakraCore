//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Object Rest unit tests

if (this.WScript && this.WScript.LoadScriptFile) { // Check for running in ch
    this.WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");
}

var tests = [
    {
        name: "let assignment with simple Object",
        body: function() {
            let {a, b, ...rest} = {a: 1, b: 2, c: 3, d: 4};
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual({c: 3, d: 4}, rest);
        }
    },
    {
        name: "var assignment with simple Object",
        body: function() {
            var {a, b, ...rest} = {a: 1, b: 2, c: 3, d: 4};
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual({c: 3, d: 4}, rest);
        }
    },
    {
        name: "Rest in assignment expression",
        body: function() {
            ({a, b, ...rest} = {a: 1, b: 2, c: 3, d: 4});
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual({c: 3, d: 4}, rest);
        }
    },
    {
        name: "Rest with simple function parameter Object",
        body: function() {
            function foo({a: _a, b: _b, ..._rest}) {
                assert.areEqual(1, _a);
                assert.areEqual(2, _b);
                assert.areEqual({c: 3, d: 4}, _rest);
            }
            foo({a: 1, b: 2, c: 3, d: 4});
        }
    },
    {
        name: "Rest with simple catch parameter Object",
        body: function() {
            try {
                throw {a: 1, b: 2, c: 3, d: 4};
            } catch({a: _a, b: _b, ..._rest}) {
                assert.areEqual(1, _a);
                assert.areEqual(2, _b);
                assert.areEqual({c: 3, d: 4}, _rest);
            }
        }
    },
    {
        name: "Rest with simple for variable declaration",
        body: function() {
            bar = [{a: 1, b: 2, c: 3, d: 4}];
            for({a, b, ...rest} of bar) {
                assert.areEqual(1, a);
                assert.areEqual(2, b);
                assert.areEqual({c: 3, d: 4}, rest);
            }
        }
    },
    {
        name: "Rest nested in destructuring",
        body: function() {
            let {a, b, double: {c, ...rest}} = {a: 1, b: 2, double: {c: 3, d: 4}};
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual(3, c);
            assert.areEqual({d: 4}, rest);
        }
    },
    {
        name: "Rest with nested function parameter Object",
        body: function() {
            function foo({a: _a, b: _b, double: {c: _c, ..._rest}}) {
                assert.areEqual(1, _a);
                assert.areEqual(2, _b);
                assert.areEqual(3, _c);
                assert.areEqual({d: 4}, _rest);
            }
            foo({a: 1, b: 2, double: {c: 3, d: 4}});
        }
    },
    {
        name: "Rest with computed properties",
        body: function() {
            let {a, ["b"]:b, ...rest} = {a: 1, b: 2, c: 3, d: 4};
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual({c: 3, d: 4}, rest);
        }
    },
    {
        name: "Rest with computed properties in function parameter binding",
        body: function() {
            function foo({a: _a, ["b"]: _b, ..._rest}) {
                assert.areEqual(1, _a);
                assert.areEqual(2, _b);
                assert.areEqual({c: 3, d: 4}, _rest);
            }
            foo({a: 1, b: 2, c: 3, d: 4});
        }
    },
    {
        name: "Rest inside re-entrant function",
        body: function() {
            function foo(r) {
                if (r) {
                    var {a, [foo(false)]:b, ...rest} = {a: 1, b: 2, c: 3, d: 4};
                    assert.areEqual(1, a);
                    assert.areEqual(2, b);
                    assert.areEqual({c: 3, d: 4}, rest);
                } else {
                    var {one, ...rest} = {one:1, two:2, three:3};
                    assert.areEqual(1, one);
                    assert.areEqual({two: 2, three: 3}, rest);
                }
                return "b";
            }
            foo(true);
        }
    },
    {
        name: "Rest nested in Computed Value",
        body: function() {
            let {[eval("let {..._rest} = {a: 1, b: 2, c: 3, d: 4};\"a\"")]:nest, ...rest} = {a: 10, b: 20, c: 30, d: 40};
            assert.areEqual(10, nest);
            assert.areEqual({b: 20, c: 30, d: 40}, rest);
        }
    },
    {
        name: "Rest with no values left to destructure",
        body: function() {
            let {a, b, ...rest} = {a: 1, b: 2};
            assert.areEqual(1, a);
            assert.areEqual(2, b);
            assert.areEqual({}, rest);
        }
    },
    {
        name: "Getters in rhs object should be evaluated",
        body: function() {
            let getterExecuted = false;
            let obj = {a: 1, get b() {getterExecuted = true; return 2;}};
            let {...rest} = obj;
            assert.areEqual(1, rest.a);
            assert.isTrue(getterExecuted);
            assert.areEqual(2, rest.b);
        }
    },
    {
        name: "Rest modifying source object",
        body: function() {
            let val = 1;
            let source = {get a() {val++; return 1;}, get b() {return val;}};
            let {b, ...rest} = source;
            assert.areEqual(1, b);
            assert.areEqual(1, rest.a);
        }
    },
    {
        name: "Source object changed by destructuring before Rest",
        body: function() {
            let val = 1;
            let source = {get a() {val++; return 1;}, get b() {return val;}};
            let {a, ...rest} = source;
            assert.areEqual(1, a);
            assert.areEqual(2, rest.b);
        }
    },
    {
        name: "Copy only own properties",
        body: function() {
            let parent = {i: 1, j: 2};
            let child = Object.create(parent);
            child.i = 3;
            let {...rest} = child;

            assert.areEqual(3, child.i);
            assert.areEqual(2, child.j);
            assert.areEqual(3, rest.i);
            assert.isFalse(rest.hasOwnProperty("j"));
        }
    },
    {
        name: "Rest includes symbols in properties",
        body: function() {
            let sym = Symbol("foo");
            let a = {};
            a[sym] = 1;
            let {...rest} = a;
            assert.areEqual(1, rest[sym], "property with Symbol property name identifier should be copied over");
            assert.areEqual(1, Object.getOwnPropertySymbols(rest).length);
        }
    },
    {
        name: "Object Rest interacting with Arrays",
        body: function() {
            let arr = [1, 2, 3];
            let {[2]:foo, ...rest} = arr;
            assert.areEqual(2, Object.keys(rest).length);
            assert.areEqual(1, rest[0]);
            assert.areEqual(2, rest[1]);
            assert.areEqual(3, foo);
        }
    },
    // TODO: Fix bug regarding nested destrucuring in array rest. 
    // Disabling this test for now
    // {
    //     name: "Object Rest interacting with Array Rest",
    //     body: function() {
    //         function foo(a, ...{...rest}) {
    //             assert.areEqual(1, a);
    //             assert.areEqual(2, rest[0]);
    //             assert.areEqual(3, rest[1]);
    //             assert.areEqual(2, Object.keys(rest).length);
    //         }            
    //         foo(1, 2, 3);
    //     }
    // },
    {
        name: "Object Rest interacting with Numbers",
        body: function() {
            let {...rest} = 1;
            assert.areEqual(0, Object.keys(rest).length);
        }
    },
    {
        name: "Object Rest interacting with Functions",
        body: function() {
            let {...rest} = function i() {return 1;}
            assert.areEqual(0, Object.keys(rest).length);
        }
    },
    {
        name: "Object Rest interacting with Strings",
        body: function() {
            let {...rest} = "edge";
            assert.areEqual(4, Object.keys(rest).length);
            assert.areEqual("e", rest[0]);
            assert.areEqual("d", rest[1]);
            assert.areEqual("g", rest[2]);
            assert.areEqual("e", rest[3]);
        }
    },
    {
        name: "Test Proxy Object",
        body: function() {
            let proxy = new Proxy({i: 1, j: 2}, {});
            let {...rest} = proxy;
            assert.areEqual(2, Object.keys(rest).length);
            assert.areEqual(1, rest.i);
            assert.areEqual(2, rest.j);
        }
    },
    {
        name: "Test Proxy Object with custom getter",
        body: function() {
            let handler = {get: function(obj, prop) {return obj[prop];}};
            let proxy = new Proxy({i: 1, j: 2}, handler);
            let {...rest} = proxy;
            assert.areEqual(2, Object.keys(rest).length);
            assert.areEqual(1, rest.i);
            assert.areEqual(2, rest.j);
        }
    },
    {
        name: "Test Proxy Object with custom getter and setter",
        body: function() {
            let setterCalled = false;
            let handler = {
                get: function(obj, prop) {
                    return obj[prop];
                },
                set: function(obj, prop, value) {
                    setterCalled = true;
                }
            };
            let proxy = new Proxy({i: 1, j: 2}, handler);
            let {...rest} = proxy;
            assert.areEqual(2, Object.keys(rest).length);
            assert.areEqual(1, rest.i);
            assert.areEqual(2, rest.j);
            assert.isFalse(setterCalled);
        }
    },
    {
        name: "Test Syntax Errors",
        body: function() {
            assert.throws(function () { eval("let {...rest1, ...rest2} = {a:1, b:2};"); }, SyntaxError, "Destructuring assignment can only have 1 Rest", "Destructuring rest variables must be in the last position of the expression");
            assert.throws(function () { eval("let {...{a, b}} = {a:1, b:2};"); }, SyntaxError, "Destructuring inside Rest is invalid syntax", "Expected identifier");
            assert.throws(function () { eval("let {...{a, ...rest}} = {a:1, b:2};"); }, SyntaxError, "Nested Rest is invalid syntax", "Expected identifier");
            assert.throws(function () { eval("let {...rest, a} = {a:1, b:2};"); }, SyntaxError, "Rest before other variables is invalid syntax", "Destructuring rest variables must be in the last position of the expression");
            assert.throws(function () { eval("...(rest)"); }, SyntaxError, "Rest must be inside destructuring", "Invalid use of the ... operator. Spread can only be used in call arguments or an array literal.");
            assert.throws(function () { eval("let {...(rest)} = {a:1, b:2};"); }, SyntaxError, "Destructuring expressions can only have identifier references");
            assert.throws(function () { eval("let {...++rest} = {a: 1, b: 2};"); }, SyntaxError, "Prefix operators before rest is invalid syntax", "Unexpected operator in destructuring expression");
            assert.throws(function () { eval("let {...rest++} = {a: 1, b: 2};"); }, SyntaxError, "Postfix operators after rest is invalid syntax", "Unexpected operator in destructuring expression");
            assert.throws(function () { eval("let {...rest+1} = {a: 1, b: 2};"); }, SyntaxError, "Infix operators after rest is invalid syntax", "Unexpected operator in destructuring expression");
            assert.throws(function () { eval("let {... ...rest} = {};"); }, SyntaxError, "Incomplete rest expression", "Unexpected operator in destructuring expression");
            assert.throws(function () { eval("let {...} = {};"); }, SyntaxError, "Incomplete rest expression", "Destructuring expressions can only have identifier references");
            assert.throws(function () { eval("function foo({...rest={}}){};"); }, SyntaxError, "Rest cannot be default initialized", "Unexpected default initializer");
        }
    },
    {
        name: "Test Type Errors",
        body: function() {
            assert.throws(function () { eval("let {...rest} = undefined;"); }, TypeError, "Cannot destructure undefined", "Cannot convert null or undefined to object");
            assert.throws(function () { eval("let {...rest} = null;"); }, TypeError, "Cannot destructure null", "Cannot convert null or undefined to object");
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
