//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Module syntax tests -- verifies syntax of import and export statements

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testModuleScript(source, message, shouldFail) {
    let testfunc = () => WScript.LoadModule(source, 'samethread');

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
        name: "All valid (non-default) export statements",
        body: function () {
            assert.doesNotThrow(function () { WScript.LoadModuleFile('.\\module\\ValidExportStatements.js', 'samethread'); }, "Valid export statements");
        }
    },
    {
        name: "Valid default export statements",
        body: function () {
            testModuleScript('export default function () { };', 'Unnamed function expression default export');
            testModuleScript('export default function fn2 () { }', 'Named function expression default export');
            testModuleScript('export default function* () { };', 'Unnamed generator function expression default export');
            testModuleScript('export default function* gn2 () { }', 'Named generator function expression default export');
            testModuleScript('export default class { };', 'Unnamed class expression default export');
            testModuleScript('export default class cl2 { }', 'Named class default expression export');
            testModuleScript('export default 1;', 'Primitive type default export');
            testModuleScript('var a; export default a = 10;', 'Variable in assignment expression default export');
            testModuleScript('export default () => 3', 'Simple default lambda expression export statement');
            testModuleScript('function foo() { }; export default foo', 'Named function statement default export');
            testModuleScript('function* g() { }; export default g', 'Named generator function statement default export');
            testModuleScript('class c { }; export default c', 'Named class statement default export');
            testModuleScript("var _ = { method: function() { return 'method_result'; }, method2: function() { return 'method2_result'; } }; export default _", 'Export object with methods - framework model');
        }
    },
    {
        name: "Syntax error export statements",
        body: function () {
            testModuleScript('export const const1;', 'Syntax error if const decl is missing initializer', true);
            testModuleScript('function foo() { }; export foo;', "Syntax error if we're trying to export an identifier without default or curly braces", true);
            testModuleScript('export function () { }', 'Syntax error if function declaration is missing binding identifier', true);
            testModuleScript('export function* () { }', 'Syntax error if generator declaration is missing binding identifier', true);
            testModuleScript('export class { }', 'Syntax error if class declaration is missing binding identifier', true);
            testModuleScript('function foo() { }; export [ foo ];', 'Syntax error if we use brackets instead of curly braces in export statement', true);
            testModuleScript('function foo() { export default function() { } }', 'Syntax error if export statement is in a nested function', true);
//            testModuleScript("eval('export default function() { }');", 'Syntax error if export statement is in eval', true);
            testModuleScript('function foo() { }; export { , foo };', 'Syntax error if named export list contains an empty element', true);
            testModuleScript('function foo() { }; () => { export { foo }; }', 'Syntax error if export statement is in arrow function', true);
            testModuleScript('function foo() { }; try { export { foo }; } catch(e) { }', 'Syntax error if export statement is in try catch statement', true);
            testModuleScript('function foo() { }; { export { foo }; }', 'Syntax error if export statement is in any block', true);
            testModuleScript('export default 1, 2, 3;', "Export default takes an assignment expression which doesn't allow comma expressions", true);
            testModuleScript('export 12;', 'Syntax error if export is followed by non-identifier', true);
            testModuleScript("export 'string_constant';", 'Syntax error if export is followed by string constant', true);
            testModuleScript('export ', 'Syntax error if export is followed by EOF', true);
            testModuleScript('function foo() { }; export { foo as 100 };', 'Syntax error in named export clause if trying to export as numeric constant', true);
        }
    },
    {
        name: "Syntax error import statements",
        body: function () {
//            testModuleScript(`eval('import foo from "ValidExportStatements.js";');`, 'Syntax error if import statement is in eval', true);
            testModuleScript('function foo() { import foo from "ValidExportStatements.js"; }', 'Syntax error if import statement is in nested function', true);
            testModuleScript('import foo, bar from "ValidExportStatements.js";', 'Syntax error if import statement has multiple default bindings', true);
            testModuleScript('import foo;', 'Syntax error if import statement is missing from clause', true);
            testModuleScript('import * as foo, from "ValidExportStatements.js";', 'Syntax error if import statement has comma after namespace import', true);
            testModuleScript('import * as foo, bar from "ValidExportStatements.js";', 'Syntax error if import statement has default binding after namespace import', true);
            testModuleScript('import * as foo, { bar } from "ValidExportStatements.js";', 'Syntax error if import statement has named import list after namespace import', true);
            testModuleScript('import { foo }, from "ValidExportStatements.js";', 'Syntax error if import statement has comma after named import list', true);
            testModuleScript('import { foo }, bar from "ValidExportStatements.js";', 'Syntax error if import statement has default binding after named import list', true);
            testModuleScript('import { foo }, * as ns1 from "ValidExportStatements.js";', 'Syntax error if import statement has namespace import after named import list', true);
            testModuleScript('import { foo }', 'Syntax error if import statement is missing from clause', true);
            testModuleScript('import [ foo ] from "ValidExportStatements.js";', 'Syntax error if named import clause uses brackets', true);
            testModuleScript('import * foo from "ValidExportStatements.js";', 'Syntax error if namespace import is missing "as" keyword', true);
            testModuleScript('import * as "foo" from "ValidExportStatements.js";', 'Syntax error if namespace imported binding name is not identifier', true);
            testModuleScript('import { , foo } from "ValidExportStatements.js";', 'Syntax error if named import list contains an empty element', true);
            testModuleScript('import foo from "ValidExportStatements.js"; foo = 12;', 'Imported default bindings are constant bindings', true);
            testModuleScript('import foo from "ValidExportStatements.js"; import foo from "ValidExportStatements.js";', 'Default import cannot be bound to the same symbol', true);
            testModuleScript('import { foo } from "ValidExportStatements.js"; foo = 12;', 'Imported named bindings are constant bindings', true);
            testModuleScript('import { foo } from "ValidExportStatements.js"; import { foo } from "ValidExportStatements.js";', 'Multiple named imports cannot be bound to the same symbol', true);
            testModuleScript('import * as foo from "ValidExportStatements.js"; foo = 12;', 'Namespace import bindings are constant bindings', true);
            testModuleScript('import * as foo from "ValidExportStatements.js"; import * as foo from "ValidExportStatements.js";', 'Multiple namespace imports cannot be bound to the same symbol', true);
            testModuleScript('import { foo as foo22 } from "ValidExportStatements.js"; foo22 = 12;', 'Renamed import bindings are constant bindings', true);
            testModuleScript('import { foo as bar, bar } from "ValidExportStatements.js";', 'Named import clause may not contain multiple binding identifiers with the same name', true);
            testModuleScript('import foo from "ValidExportStatements.js"; import * as foo from "ValidExportStatements.js";', 'Imported bindings cannot be overwritten by later imports', true);
            testModuleScript('() => { import arrow from ""; }', 'Syntax error if import statement is in arrow function', true);
            testModuleScript('try { import _try from ""; } catch(e) { }', 'Syntax error if import statement is in try catch statement', true);
            testModuleScript('{ import in_block from ""; }', 'Syntax error if import statement is in any block', true);
            testModuleScript('import {', 'Named import clause which has EOF after left curly', true);
            testModuleScript('import { foo', 'Named import clause which has EOF after identifier', true);
            testModuleScript('import { foo as ', 'Named import clause which has EOF after identifier as', true);
            testModuleScript('import { foo as bar ', 'Named import clause which has EOF after identifier as identifier', true);
            testModuleScript('import { foo as bar, ', 'Named import clause which has EOF after identifier as identifier comma', true);
            testModuleScript('import { switch } from "module";', 'Named import clause which has non-identifier token as the first token', true);
            testModuleScript('import { foo bar } from "module";', 'Named import clause missing "as" token', true);
            testModuleScript('import { foo as switch } from "module";', 'Named import clause with non-identifier token after "as"', true);
            testModuleScript('import { foo, , } from "module";', 'Named import clause with too many trailing commas', true);
        }
    },
    {
        name: "All valid re-export statements",
        body: function () {
            assert.doesNotThrow(function () { WScript.LoadModuleFile('.\\module\\ValidReExportStatements.js', 'samethread'); }, "Valid re-export statements");
        }
    },
    {
        name: "HTML comments do not parse in module code",
        body: function () {
            testModuleScript("<!--\n",     "HTML open comment does not parse in module code",  true);
            testModuleScript("\n-->",      "HTML close comment does not parse in module code", true);
            testModuleScript("<!-- -->",   "HTML comment does not parse in module code",       true);
            testModuleScript("/* */ -->",  "HTML comment after delimited comment does not parse in module code", true);
            testModuleScript("/* */\n-->", "HTML comment after delimited comment does not parse in module code", true);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
