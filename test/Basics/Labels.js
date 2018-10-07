//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

strictMode = "'use strict'\n"

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
        
testBlockStatementAfterLabel=
    `e:
    {
    }`
    
testEmptyStatementAfterLabel=
    `f:
        ;`
        
testIfStatementAfterLabel=
    `i = 3
    g:
        if(i > 4)
            i -= 2;
        else
            i += 2;`
        
testSwitchStatementAfterLabel=
    `var f = 'paper'
    switchLabel:
        switch(f){
            case 'Paper':
                break;
            default:
                f = null;
        }`
        
testContinueAfterLabel = 
    `c:
        for(i = 0; i < 10; i++)
        {
            d:
                continue c;
        }`
        
testBreakAfterLabel = 
    `c:
        for(i = 0; i < 10; i++)
        {
            d:
                break c;
        }`
        
testNestedLabels = 
    `a:
        b:
            c:
                d:
                    e:
                        f:
                            g:
                                z:
                                    var y = 6;`
                    
testThrowAfterLabel = 
    `var y = 0
    if(y > 5)
    {
        a:
            throw("shouldn't be here")
    }`
    
testTryStatementAfterLabel = 
    `var y = 0
    g:
        try{
            y /= 0
        }
        catch(e){
            print(e)
        }`
        
testAwaitAsLabelOutsideAsyncFnc = 
    `await:
        for(i = 0; i < 3; i++)
        {
            continue await;
        }`
        
testAwaitAsLabelInsideAsyncFnc = 
    `async function main() {
        await:
        var i = 10;
        i **= 2;
    }`
    
strictModeOnlyInvalidLabels =  {//Maps the reserved word to the error message given when it is used as a label in strict mode
    "implements" : "Syntax error", 
    "interface" : "Syntax error", 
    "let" : "Syntax error", 
    "package" : "Syntax error", 
    "private" : "Syntax error", 
    "protected" : "Syntax error", 
    "public" : "Syntax error", 
    "static" : "Syntax error",
    "yield" : "Syntax error"
    }

invalidLabels= { //Maps the reserved word to the error message given when it is used as a label in either non-strict or strict mode
    "break" : "Can't have 'break' outside of loop",
    "case" : "Syntax error",
    "catch" : "Syntax error",
    "class" : "Expected identifier",
    "const" : "Expected identifier",
    "continue" : "Can't have 'continue' outside of loop",
    "debugger" : "Expected ';'",
    "default" : "Syntax error",
    "delete" : "Syntax error",
    "do" : "Syntax error",
    "else" : "Syntax error",
    "export" : "Syntax error",
    "extends" : "Syntax error",
    "finally" : "Syntax error",
    "for" : "Expected '('",
    "function" : "Expected identifier",
    "if" : "Expected '('",
    "import" : "Module import or export statement unexpected here",
    "in" : "Syntax error",
    "instanceof" : "Syntax error",
    "new" : "Syntax error",
    "return" : "'return' statement outside of function",
    "super" : "Invalid use of the 'super' keyword",
    "switch" : "Expected '('",
    "this" : "Expected ';'",
    "throw" : "Syntax error",
    "try" : "Expected '{'",
    "typeof" : "Syntax error",
    "var" : "Expected identifier",
    "void" : "Syntax error",
    "while" : "Expected '('",
    "enum" : "Syntax error",
    "null" : "Expected ';'",
    "true" : "Expected ';'",
    "false": "Expected ';'"
    }

testIfLabelIsValid = 
    `:
        for(let i = 0; i < 3; i++)
        {
            i = i+1
        }`

testInvalidLabelSyntaxParens = 
    `function f() {
        // Label in parenthesis is bad syntax. Verify consistency in deferred parsing.
        (a):
            var i = 0;
    }
    f();`
    
testInvalidLabelSyntaxIncrement = 
    `function f() {
        // Bad label, verify consistency in deferred parsing
        a++:
            var i = 0;
    }
    f();`
testInvalidLabelSyntaxDecrement = 
    `function f() {
        // Bad label, verify consistency in deferred parsing
        a--:
            var i = 0;
    }
    f();`

testInvalidLabelSyntaxNonIDContinueCharacterInLabel = 
    `function f() {
        // Bad label, verify consistency in deferred parsing
        a.:
            var i = 0;
    }

    f();`
testInvalidLabelSyntaxArrayAccess = 
    `function f() {
        // Bad label, verify consistency in deferred parsing
        a[0]:
            var i = 0;
    }

    f();`

testInvalidLabelSyntaxFunctionCall = 
    `function f() {
    // Bad label, verify consistency in deferred parsing
    a():
        var i = 0;
    }

    f();`
testRuntimeErrorWithDanglingLetAfterLabel = 
    `if (true) {
        L: let // comment
        {}
    }`
testNoSyntaxErrorWithDanglingLetAfterLabel =
    `if (false) {
        L: let // comment
        {}
    }`
testRuntimeErrorWithDanglingLetAfterLabel2 = 
    `if (true) {
    L: let // ASI
    var g = {}
    }`
testNoSyntaxErrorWithDanglingLetAfterLabel2 = 
    `if (false) {
    L: let // ASI
    var g = {}
    }`
testRuntimeErrorWithDanglingLetAfterLabel3=
    `if (true) {
    L: let // ASI
    let a = 3
    }`
testNoSyntaxErrorWithDanglingLetAfterLabel3=
    `if (false) {
    L: let // ASI
    let a = 3
    }`
testRuntimeErrorWithDanglingLetAfterLabel4=
    `if (true) {
    L: let // ASI
    a = 3
    }`
testNoSyntaxErrorWithDanglingLetAfterLabel4=
    `if (false) {
    L: let // ASI
    a = 3
    }`
testNoRuntimeErrorWithDanglingVarAfterLabel=
    `label: var
    x = 0;`
testSyntaxErrorWithDanglingConstAfterLabel=
    `label: const
    x = 0;`
testRuntimeErrorWithDanglingYieldAfterLabel=
    `if (true) {
        L: yield // ASI
        console.log("b");
        1;
    }` 
testNoSyntaxErrorWithDanglingYieldAfterLabel=
    `if (false) {
        L: yield // ASI
        console.log("b");
        1;
    }` 
testGeneratorWithDanglingYieldAfterLabel=
    `function* gen(num)
    {
        L: yield // ASI
        ++num
    }
    
    const iterator = gen(0)
    if(iterator.next().value === 1)
        throw("Yield should not return a number");
    ` 
testNoSyntaxErrorWithDanglingAwaitAfterLabel=
    `function fn()
    {
        for(i = 0; i < 30; i++);
    }
    if(false)
    {
        L: await
        fn()
    }`
testRuntimeErrorWithDanglingAwaitAfterLabel=
    `function fn()
    {
        for(i = 0; i < 30; i++);
    }
    if(true)
    {
        L: await
        fn()
    }`
testNoSyntaxErrorWithDanglingStaticAfterLabel=
    `function g(){
        L: static
        x = 0
    }`
testRuntimeErrorWithDanglingStaticAfterLabel=
    `L: static
    x = 0`

testSyntaxErrorWithContinueAfterLabel=
    `function fn() {
        L:
            continue L;
    }`

testNoLabelNotFoundWithBreakAfterLabel=
    `function fn() {
        L:
            break L;
    }`

    
function testModuleScript(source, message, shouldFail = false) {
    let testfunc = () => testRunner.LoadModule(source, 'samethread', shouldFail);

    if (shouldFail) {
        let caught = false;

        // We can't use assert.throws here because the SyntaxError used to construct the thrown error
        // is from a different context so it won't be strictly equal to our SyntaxError.
        try {
            testfunc();
        } catch(e) {
            caught = true;

            // Compare toString output of SyntaxError and other context SyntaxError constructor.
            assert.areEqual(e.constructor.toString(), SyntaxError.toString(), message);
        }

        assert.isTrue(caught, `Expected error not thrown: ${message}`);
    } else {
        assert.doesNotThrow(testfunc, message);
    }
}

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
            assert.throws(() => eval("let x=5; labelA: "), SyntaxError, "Labels must not be followed by EOF", "Unexpected end of script after a label.");
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
            assert.doesNotThrow(() => eval(testBlockStatementAfterLabel), "block statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testEmptyStatementAfterLabel), "empty statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testIfStatementAfterLabel), "if statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testSwitchStatementAfterLabel), "switch statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testContinueAfterLabel), "continue statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testBreakAfterLabel), "break statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testNestedLabels), "labelled statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testThrowAfterLabel), "throw statements should be allowed after a label");
            assert.doesNotThrow(() => eval(testTryStatementAfterLabel), "throw statements should be allowed after a label");
        }
    },
    {
        name : "Label tests with strict mode applied",
        body : function () 
        { 
            assert.throws(() => eval(strictMode + testFunctionAfterLabel), SyntaxError, "Function declarations should not be allowed after a label in strict mode.", "Function declarations not allowed after a label in strict mode."); 
        }
    },
    {
        name : "Label tests with keywords as labels",
        body : function () 
        {             
            testModuleScript("await: for(let i = 0; i < 3; i++){break await;}", "'await' expression not allowed in this context", shouldFail=true) 
            assert.doesNotThrow(() => eval(strictMode + "await: for(let i = 0; i < 3; i++){break await;}"), "Await as label should only be an error when in a module")
            assert.doesNotThrow(() => eval(strictMode + testAwaitAsLabelOutsideAsyncFnc), "'await' should be allowed as label outside of async functions, even in strict mode.")
            assert.throws(() => eval(strictMode + testAwaitAsLabelInsideAsyncFnc), SyntaxError, "Expected 'await' label in async function to be a syntax error.", "Use of 'await' as label in async function is not allowed.");

            assert.throws(() => eval("with" + testIfLabelIsValid), SyntaxError, "Expected syntax error for using invalid label identifier 'with'", "Expected '('"); //The only invalid keyword with a different error message for strict/non-strict modes
            assert.throws(() => eval(strictMode + "with" + testIfLabelIsValid), SyntaxError, "Expected syntax error for using invalid label identifier 'with'", "'with' statements are not allowed in strict mode");

            for(let label in invalidLabels)
            {
                assert.throws(() => eval(label + testIfLabelIsValid), SyntaxError, "Expected syntax error for using invalid label identifier '" + label + "'");
                assert.throws(() => eval(strictMode + label + testIfLabelIsValid), SyntaxError, "Expected syntax error for using invalid label identifier '" + label + "'");
            }
            
            for(let invalidLabelInStrict in strictModeOnlyInvalidLabels)
            {
                assert.throws(() => eval(strictMode + invalidLabelInStrict + testIfLabelIsValid), SyntaxError, "Expected syntax error in strict mode for future reserved keyword '" + invalidLabelInStrict + "'")
                assert.doesNotThrow(() => eval(invalidLabelInStrict + testIfLabelIsValid), "Expected no syntax error for future reserved keyword '" + invalidLabelInStrict + " in non-strict mode")
            }
        }
    },
    {
        name : "Label tests where label is invalid syntax",
        body : function ()
        {
            assert.throws(() => eval(testInvalidLabelSyntaxParens), SyntaxError, "Expected syntax error from using malformed label", "Expected ';'") 
            assert.throws(() => eval(testInvalidLabelSyntaxIncrement), SyntaxError, "Expected syntax error from using malformed label", "Expected ';'") 
            assert.throws(() => eval(testInvalidLabelSyntaxDecrement), SyntaxError, "Expected syntax error from using malformed label", "Expected ';'") 
            assert.throws(() => eval(testInvalidLabelSyntaxNonIDContinueCharacterInLabel), SyntaxError, "Expected syntax error from using malformed label", "Expected identifier") 
            assert.throws(() => eval(testInvalidLabelSyntaxArrayAccess), SyntaxError, "Expected syntax error from using malformed label", "Expected ';'") 
            assert.throws(() => eval(testInvalidLabelSyntaxFunctionCall), SyntaxError, "Expected syntax error from using malformed label", "Expected ';'") 
        }
    },
    {
        name : "Label tests, edge cases",
        body : function ()
        {
            assert.throws(() => eval(testRuntimeErrorWithDanglingLetAfterLabel), ReferenceError, "Expected runtime error from using let as identifier", "'let' is not defined") 
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingLetAfterLabel), "Expected no syntax error from using let as identifier after label") 
            assert.throws(() => eval(testRuntimeErrorWithDanglingLetAfterLabel2), ReferenceError, "Expected runtime error from using let as identifier", "'let' is not defined") 
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingLetAfterLabel2), "Expected no syntax error from using let as identifier after label") 
            assert.throws(() => eval(testRuntimeErrorWithDanglingLetAfterLabel3), ReferenceError, "Expected runtime error from using let as identifier", "'let' is not defined") 
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingLetAfterLabel3), "Expected no syntax error from using let as identifier after label") 
            assert.throws(() => eval(testRuntimeErrorWithDanglingLetAfterLabel4), ReferenceError, "Expected runtime error from using let as identifier", "'let' is not defined") 
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingLetAfterLabel4), "Expected no syntax error from using let as identifier after label") 
            assert.doesNotThrow(() => eval(testNoRuntimeErrorWithDanglingVarAfterLabel), "Expected no syntax error from using var after label") 
            assert.throws(() => eval(testSyntaxErrorWithDanglingConstAfterLabel), SyntaxError, "Expected syntax error from using const after label", "Labels not allowed before lexical declaration") 
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingYieldAfterLabel), "Expected no syntax error from using yield after label") 
            assert.throws(() => eval(testRuntimeErrorWithDanglingYieldAfterLabel), ReferenceError, "Expected runtime error for undefined reference to yield", "'yield' is not defined") 
            assert.doesNotThrow(() => eval(testGeneratorWithDanglingYieldAfterLabel), "Expected no error from using yield after label. Also expect the yield to not be bound to the expression.")
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingAwaitAfterLabel), "Expected no error from using await after label. Also expect the yield to not be bound to the expression.")
            assert.throws(() => eval(testRuntimeErrorWithDanglingAwaitAfterLabel), ReferenceError, "Expected reference error from stranded await being used after label", "'await' is not defined")
            assert.throws(() => eval(testRuntimeErrorWithDanglingStaticAfterLabel), ReferenceError, "Expected reference error from stranded static being used after label", "'static' is not defined")
            assert.doesNotThrow(() => eval(testNoSyntaxErrorWithDanglingStaticAfterLabel), "Expected no issue parsing since static is viewed as an identifier")
            assert.throws(() => eval(testSyntaxErrorWithContinueAfterLabel), SyntaxError, "Expected syntax error from having continue outside of loop", "Can't have 'continue' outside of loop")
            assert.doesNotThrow(() => eval(testNoLabelNotFoundWithBreakAfterLabel), "Expected no issue from finding label")
        }
    }
];

testRunner.runTests(tests, {
    verbose : WScript.Arguments[0] != "summary"
});
