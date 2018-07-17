//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");


testFunctionAfterLabel =
    `a: 
        function x()
        {
        }`
testVarAfterLabel = 
    `b:
        var a;`
testForLoopAfterLabel = 
    `c:
        for(i = 0; i < 10; i++)
        {
            d:
                for(j = 0; j < 10; j++)
                {
                    if((i%j)%3 === 0)
                        break d;
                }
        }`
testWhileLoopAfterLabel = 
    `i = 1, j = 1, product = 1
    c:
        while(i <= 20)
        {
            d:
                while(j <= 5)
                {
                    j = j+2
                    product *= i*j;
                    break c;
                }
            i = i+1
        }
        assert.areEqual(3, product)
        `
testNonLexicalDeclarationAfterLabel=
    `d:
        g = "Hola, Mundo."`
var tests = [
    {
        name : "Labels followed by declarations which should all be syntax errors.",
        body : function () 
        {
            assert.throws(() => eval("a: let x;"), SyntaxError, 
            "A let declaration must not be labelled", "Labels not allowed before lexical declaration");
            assert.throws(() => eval("b: const y = 0;"), SyntaxError, 
            "A const declaration must not be labelled", "Labels not allowed before lexical declaration");
            assert.throws(() => eval("c: class z {}"), SyntaxError, 
            "An class declaration must not be labelled", "Labels not allowed before class declaration");
            assert.throws(() => eval("d: function* w() {}"), SyntaxError, 
            "A generator must not be labelled", "Labels not allowed before generator declaration");
            assert.throws(() => eval("e: async function u() { }"), SyntaxError, 
            "An async function must not be labelled", "Labels not allowed before async function declaration");
            /*assert.throws(() => eval("e: async function* u() { }"), SyntaxError, //TODO: Uncomment when async generators supported.
            "An async generator must not be labelled", "Labels not allowed before async function declaration");*/
        }
    },
    {
        name : "Labels followed by non-declarations are valid syntax (non-generator non-async function declarations and non-lexical variable declarations are allowed)",
        body : function () 
        {
            assert.doesNotThrow(() => eval(testFunctionAfterLabel), "Function declarations should be allowed after a label."); 
            assert.doesNotThrow(() => eval(testVarAfterLabel), "var declarations should be allowed after a label.");
            assert.doesNotThrow(() => eval(testForLoopAfterLabel), "for loop statements should be allowed after a label.");
            assert.doesNotThrow(() => eval(testNonLexicalDeclarationAfterLabel), "non-lexical declarations should be allowed after a label.");
            assert.doesNotThrow(() => eval(testWhileLoopAfterLabel), "while loops should be allowed after a label");
        }
    }
];

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
