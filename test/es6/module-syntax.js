//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Module syntax tests -- verifies syntax of import and export statements

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

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
        name: "All valid (non-default) export statements",
        body: function () {
            assert.doesNotThrow(function () { WScript.LoadModuleFile('ValidExportStatements.js', 'samethread'); }, "Valid export statements");
        }
    },
    {
        name: "Syntax error return in module top level",
        body: function () {
            testModuleScript('return;', 'Syntax error if in module\'s top-level', true);
            testModuleScript('if (false) return;', 'Syntax error if in module\'s top-level', true);
            testModuleScript('if (false) {} else return;', 'Syntax error if in module\'s top-level', true);
            testModuleScript('while (false) return;', 'Syntax error if in module\'s top-level', true);
            testModuleScript('do return while(false);', 'Syntax error if in module\'s top-level', true);
        }
    },
    {
        name: "Valid default export statements",
        body: function () {
            testModuleScript('export default function () { };', 'Unnamed function expression default export');
            testModuleScript('export default function _fn2 () { }', 'Named function expression default export');
            testModuleScript('export default function* () { };', 'Unnamed generator function expression default export');
            testModuleScript('export default function* _gn2 () { }', 'Named generator function expression default export');
            testModuleScript('export default class { };', 'Unnamed class expression default export');
            testModuleScript('export default class _cl2 { }', 'Named class default expression export');
            testModuleScript('export default 1;', 'Primitive type default export');
            testModuleScript('var a; export default a = 10;', 'Variable in assignment expression default export');
            testModuleScript('export default () => 3', 'Simple default lambda expression export statement');
            testModuleScript('function _default() { }; export default _default', 'Named function statement default export');
            testModuleScript('function* g() { }; export default g', 'Named generator function statement default export');
            testModuleScript('class c { }; export default c', 'Named class statement default export');
            testModuleScript("var _ = { method: function() { return 'method_result'; }, method2: function() { return 'method2_result'; } }; export default _", 'Export object with methods - framework model');
        }
    },
    {
        name: "Valid export statements without semicolon",
        body: function () {
            testModuleScript('export var v0 if (true) { }', 'var declaration export');
            testModuleScript('export let l0 if (true) { }', 'let declaration export');
            testModuleScript('export const const0 = 0 if (true) { }', 'const declaration export');
            testModuleScript('export function f() { } if (true) { }', 'function declaration export');
            testModuleScript('export function *g() { } if (true) { }', 'function generator declaration export');
            testModuleScript('export var c0 = class { } if (true) { }', 'var with unnamed class expression export');
            testModuleScript('export class c1 { } if (true) { }', 'Named class expression export');
            testModuleScript('export default function () { } if (true) { }', 'Unnamed function expression default export');
            testModuleScript('export default function _fn2 () { } if (true) { }', 'Named function expression default export');
            testModuleScript('export default function* () { } if (true) { }', 'Unnamed generator function expression default export');
            testModuleScript('export default function* _gn2 () { } if (true) { }', 'Named generator function expression default export');
            testModuleScript('export default class { } if (true) { }', 'Unnamed class expression default export');
            testModuleScript('export default class _cl2 { } if (true) { }', 'Named class default expression export');
            testModuleScript('export default 1 if (true) { }', 'Primitive type default export');
            testModuleScript('var a; export default a = 10 if (true) { }', 'Variable in assignment expression default export');
            testModuleScript('export default () => 3 if (true) { }', 'Simple default lambda expression export statement');
            testModuleScript('function _default() { }; export default _default if (true) { }', 'Named function statement default export');
            testModuleScript('function* g() { }; export default g if (true) { }', 'Named generator function statement default export');
            testModuleScript('class c { }; export default c if (true) { }', 'Named class statement default export');
            testModuleScript("var _ = { method: function() { return 'method_result'; }, method2: function() { return 'method2_result'; } }; export default _ if (true) { }", 'Export object with methods - framework model');
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
            testModuleScript('function foo() { }; export { , foo };', 'Syntax error if named export list contains an empty element', true);
            testModuleScript('function foo() { }; () => { export { foo }; }', 'Syntax error if export statement is in arrow function', true);
            testModuleScript('function foo() { }; try { export { foo }; } catch(e) { }', 'Syntax error if export statement is in try catch statement', true);
            testModuleScript('function foo() { }; { export { foo }; }', 'Syntax error if export statement is in any block', true);
            testModuleScript('export default 1, 2, 3;', "Export default takes an assignment expression which doesn't allow comma expressions", true);
            testModuleScript('export 12;', 'Syntax error if export is followed by non-identifier', true);
            testModuleScript("export 'string_constant';", 'Syntax error if export is followed by string constant', true);
            testModuleScript('export ', 'Syntax error if export is followed by EOF', true);
            testModuleScript('function foo() { }; export { foo as 100 };', 'Syntax error in named export clause if trying to export as numeric constant', true);
            testModuleScript('if (false) export default null;', 'Syntax error export in if', true);
            testModuleScript('if (false) {} else export default null;', 'Syntax error export after else', true);
            testModuleScript('for(var i=0; i<1; i++) export default null;', 'Syntax error export in for', true);
            testModuleScript('while(false) export default null;', 'Syntax error export in while', true);
            testModuleScript(`do export default null
                                while (false);`, 'Syntax error export in while', true);
            testModuleScript('function () { export default null; }', 'Syntax error export in function', true);
        }
    },
    {
        name: "Syntax error import statements",
        body: function () {
            testModuleScript('function foo() { import foo from "ValidExportStatements.js"; }', 'Syntax error if import statement is in nested function', true);
            testModuleScript('import foo, bar from "ValidExportStatements.js";', 'Syntax error if import statement has multiple default bindings', true);
            testModuleScript('import { foo, foo } from "ValidExportStatements.js";', 'Redeclaration error if multiple imports have the same local name', true);
            testModuleScript('import { foo, bar as foo } from "ValidExportStatements.js";', 'Redeclaration error if multiple imports have the same local name', true);
            testModuleScript('const foo = 12; import { foo } from "ValidExportStatements.js";', 'Syntax error if module body has a const declaration bound to the same name as a module import', true);
            testModuleScript('function foo() { }; import { foo } from "ValidExportStatements.js";', 'Syntax error if module body has a function declaration bound to the same name as a module import', true);
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
            testModuleScript('import foo from "ValidExportStatements.js"; import foo from "ValidExportStatements.js";', 'Default import cannot be bound to the same symbol', true);
            testModuleScript('import { foo } from "ValidExportStatements.js"; import { foo } from "ValidExportStatements.js";', 'Multiple named imports cannot be bound to the same symbol', true);
            testModuleScript('import * as foo from "ValidExportStatements.js"; import * as foo from "ValidExportStatements.js";', 'Multiple namespace imports cannot be bound to the same symbol', true);
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
            testModuleScript('if (false) import { default } from "module";', 'Syntax error export in if', true);
            testModuleScript('if (false) {} import { default } from "module";', 'Syntax error export after else', true);
            testModuleScript('for(var i=0; i<1; i++) import { default } from "module";', 'Syntax error export in for', true);
            testModuleScript('while(false) import { default } from "module";', 'Syntax error export in while', true);
            testModuleScript(`do import { default } from "module"
                                while (false);`, 'Syntax error export in while', true);
            testModuleScript('function () { import { default } from "module"; }', 'Syntax error export in function', true);
        }
    },
    {
        name: "Runtime error import statements",
        body: function () {
            testModuleScript('import foo from "ValidExportStatements.js"; assert.throws(()=>{ foo =12; }, TypeError, "assignment to const");', 'Imported default bindings are constant bindings', false);
            testModuleScript('import { foo } from "ValidExportStatements.js"; assert.throws(()=>{ foo = 12; }, TypeError, "assignment to const");', 'Imported named bindings are constant bindings', false);

            testModuleScript('import * as foo from "ValidExportStatements.js"; assert.throws(()=>{ foo = 12; }, TypeError, "assignment to const");', 'Namespace import bindings are constant bindings', false);

            testModuleScript('import { foo as foo22 } from "ValidExportStatements.js"; assert.throws(()=>{ foo22 = 12; }, TypeError, "assignment to const");', 'Renamed import bindings are constant bindings', false);
        }
    },
    {
        name: "All valid re-export statements",
        body: function () {
            assert.doesNotThrow(function () { WScript.LoadModuleFile('ValidReExportStatements.js', 'samethread'); }, "Valid re-export statements");
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
