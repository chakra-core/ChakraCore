//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Module functionality tests -- verifies functionality of import and export statements

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testScript(source, message, shouldFail = false, explicitAsync = false) {
    message += " (script)";
    let testfunc = () => testRunner.LoadScript(source, undefined, shouldFail, explicitAsync);

    if (shouldFail) {
        let caught = false;

        assert.throws(testfunc, SyntaxErrr, message);
        assert.isTrue(caught, `Expected error not thrown: ${message}`);
    } else {
        assert.doesNotThrow(testfunc, message);
    }
}

function testModuleScript(source, message, shouldFail = false, explicitAsync = false) {
    message += " (module)";
    let testfunc = () => testRunner.LoadModule(source, 'samethread', shouldFail, explicitAsync);

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

function testDynamicImport(importFunc, thenFunc, catchFunc, _asyncEnter, _asyncExit) {
    var promise = importFunc();
    assert.isTrue(promise instanceof Promise);
    promise.then((v)=>{
        _asyncEnter();
        thenFunc(v);
        _asyncExit();
    }).catch((err)=>{
        _asyncEnter();
        catchFunc(err);
        _asyncExit();
    });
}

var tests = [
    {
        name: "Runtime evaluation of module specifier",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>{
                        var getName = ()=>{ return 'ModuleSimpleExport'; };
                        return import( getName() + '.js');
                    },
                    (v)=>{
                        assert.areEqual('ModuleSimpleExport', v.ModuleSimpleExport_foo(), 'Failed to import ModuleSimpleExport_foo from ModuleSimpleExport.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit
                )`;
            testScript(functionBody, "Test importing a simple exported function", false, true);
            testModuleScript(functionBody, "Test importing a simple exported function", false, true);
        }
    },
    {
        name: "Validate a simple module export",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>{ return import('ModuleSimpleExport.js'); },
                    (v)=>{ assert.areEqual('ModuleSimpleExport', v.ModuleSimpleExport_foo(), 'Failed to import ModuleSimpleExport_foo from ModuleSimpleExport.js'); },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit
                )`;
            testScript(functionBody, "Test importing a simple exported function", false, true);
            testModuleScript(functionBody, "Test importing a simple exported function", false, true);
        }
    },
    {
        name: "Validate importing from multiple modules",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexExports.js'),
                    (v)=>{
                        assert.areEqual('foo', v.foo2(), 'Failed to import foo2 from ModuleComplexExports.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit
                )`;
            testScript(functionBody, "Test importing from multiple modules", false, true);
            testModuleScript(functionBody, "Test importing from multiple modules", false, true);
        }
    },
    {
        name: "Validate a variety of more complex exports",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexExports.js'),
                    (v)=>{
                        assert.areEqual('foo', v.foo(), 'Failed to import foo from ModuleComplexExports.js');
                        assert.areEqual('foo', v.foo2(), 'Failed to import foo2 from ModuleComplexExports.js');
                        assert.areEqual('bar', v.bar(), 'Failed to import bar from ModuleComplexExports.js');
                        assert.areEqual('bar', v.bar2(), 'Failed to import bar2 from ModuleComplexExports.js');
                        assert.areEqual('let2', v.let2, 'Failed to import let2 from ModuleComplexExports.js');
                        assert.areEqual('let3', v.let3, 'Failed to import let3 from ModuleComplexExports.js');
                        assert.areEqual('let2', v.let4, 'Failed to import let4 from ModuleComplexExports.js');
                        assert.areEqual('let3', v.let5, 'Failed to import let5 from ModuleComplexExports.js');
                        assert.areEqual('const2', v.const2, 'Failed to import const2 from ModuleComplexExports.js');
                        assert.areEqual('const3', v.const3, 'Failed to import const3 from ModuleComplexExports.js');
                        assert.areEqual('const2', v.const4, 'Failed to import const4 from ModuleComplexExports.js');
                        assert.areEqual('const3', v.const5, 'Failed to import const5 from ModuleComplexExports.js');
                        assert.areEqual('var2', v.var2, 'Failed to import var2 from ModuleComplexExports.js');
                        assert.areEqual('var3', v.var3, 'Failed to import var3 from ModuleComplexExports.js');
                        assert.areEqual('var2', v.var4, 'Failed to import var4 from ModuleComplexExports.js');
                        assert.areEqual('var3', v.var5, 'Failed to import var5 from ModuleComplexExports.js');
                        assert.areEqual('class2', v.class2.static_member(), 'Failed to import class2 from ModuleComplexExports.js');
                        assert.areEqual('class2', new v.class2().member(), 'Failed to create intance of class2 from ModuleComplexExports.js');
                        assert.areEqual('class2', v.class3.static_member(), 'Failed to import class3 from ModuleComplexExports.js');
                        assert.areEqual('class2', new v.class3().member(), 'Failed to create intance of class3 from ModuleComplexExports.js');
                        assert.areEqual('class4', v.class4.static_member(), 'Failed to import class4 from ModuleComplexExports.js');
                        assert.areEqual('class4', new v.class4().member(), 'Failed to create intance of class4 from ModuleComplexExports.js');
                        assert.areEqual('class4', v.class5.static_member(), 'Failed to import class4 from ModuleComplexExports.js');
                        assert.areEqual('class4', new v.class5().member(), 'Failed to create intance of class4 from ModuleComplexExports.js');
                        assert.areEqual('default', v.default(), 'Failed to import default from ModuleComplexExports.js');
                        assert.areEqual('ModuleComplexExports', v.function, 'Failed to import v.function from ModuleComplexExports.js');
                        assert.areEqual('ModuleComplexExports', v.export, 'Failed to import v.export from ModuleComplexExports.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Test importing a variety of exports", false, true);
            testModuleScript(functionBody, "Test importing a variety of exports", false, true);
        }
    },
    {
        name: "Exporting module changes exported value",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexExports.js'),
                    (v)=>{
                        v.reset();
                        assert.areEqual('before', v.target(), 'Failed to import target from ModuleComplexExports.js');
                        assert.areEqual('ok', v.changeTarget(), 'Failed to import changeTarget from ModuleComplexExports.js');
                        assert.areEqual('after', v.target(), 'changeTarget failed to change export value');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Changing exported value", false, true);
            testModuleScript(functionBody, "Changing exported value", false, true);
        }
    },
    {
        name: "Simple re-export forwards import to correct slot",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleSimpleReexport.js'),
                    (v)=>{
                        assert.areEqual('ModuleSimpleExport', v.ModuleSimpleExport_foo(), 'Failed to import ModuleSimpleExport_foo from ModuleSimpleReexport.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Simple re-export from one module to another", false, true);
            testModuleScript(functionBody, "Simple re-export from one module to another", false, true);
        }
    },
    {
        name: "Renamed re-export and dynamic import",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexReexports.js'),
                    (v)=>{
                        assert.areEqual('bar', v.ModuleComplexReexports_foo(), 'Failed to import ModuleComplexReexports_foo from ModuleComplexReexports.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Rename already renamed re-export", false, true);
            testModuleScript(functionBody, "Rename already renamed re-export", false, true);
        }
    },
    {
        name: "Explicit export/import to default binding",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleDefaultExport1.js'),
                    (v)=>{
                        assert.areEqual('ModuleDefaultExport1', v.default(), 'Failed to import default from ModuleDefaultExport1.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Explicitly export and import a local name to the default binding", false, true);
            testModuleScript(functionBody, "Explicitly export and import a local name to the default binding", false, true);
        }
    },
    {
        name: "Explicit import of default binding",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleDefaultExport2.js'),
                    (v)=>{
                        assert.areEqual('ModuleDefaultExport2', v.default(), 'Failed to import default from ModuleDefaultExport2.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Explicitly import the default export binding", false, true);
            testModuleScript(functionBody, "Explicitly import the default export binding", false, true);
        }
    },
    {
        name: "Exporting module changes value of default export",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleDefaultExport3.js'),
                    (v)=>{
                        assert.areEqual(2, v.default, 'Failed to import default from ModuleDefaultExport3.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Exported value incorrectly bound", false, true);
            testModuleScript(functionBody, "Exported value incorrectly bound", false, true);

            functionBody =
                `testDynamicImport(
                    ()=>import('ModuleDefaultExport4.js'),
                    (v)=>{
                        assert.areEqual(1, v.default, 'Failed to import default from ModuleDefaultExport4.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Exported value incorrectly bound", false, true);
            testModuleScript(functionBody, "Exported value incorrectly bound", false, true);
        }
    },
    {
        name: "Import bindings used in a nested function",
        body: function () {
            let functionBody =
                `function test(func) {
                    assert.areEqual('ModuleDefaultExport2', func(), 'Failed to import default from ModuleDefaultExport2.js');
                }
                testDynamicImport(
                    ()=>import('ModuleDefaultExport2.js'),
                    (v)=>test(v.default),
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Failed to find imported name correctly in nested function", false, true);
            testModuleScript(functionBody, "Failed to find imported name correctly in nested function", false, true);
        }
    },
    {
        name: "Exported name may be any keyword",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexExports.js'),
                    (v)=>{
                        assert.areEqual('ModuleComplexExports', v.export, 'Failed to import export from ModuleDefaultExport2.js');
                        assert.areEqual('ModuleComplexExports', v.function, 'Failed to import function from ModuleDefaultExport2.js');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Exported name may be a keyword (import binding must be binding identifier)", false, true);
            testModuleScript(functionBody, "Exported name may be a keyword (import binding must be binding identifier)", false, true);
        }
    },
    {
        name: "Odd case of 'export { as as as }; import { as as as };'",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexExports.js'),
                    (v)=>{
                        assert.areEqual('as', v.as(), 'String "as" is not reserved word');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;
            testScript(functionBody, "Test 'import { as as as}'", false, true);
            testModuleScript(functionBody, "Test 'import { as as as}'", false, true);
        }
    },
    {
        name: "Typeof a module export",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleDefaultExport2.js'),
                    (v)=>{
                        assert.areEqual('function', typeof v.default, 'typeof default export from ModuleDefaultExport2.js is function');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;

            testScript(functionBody, "Typeof a module export", false, true);
            testModuleScript(functionBody, "Typeof a module export", false, true);
        }
    },
    {
        name: "Circular module dependency",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleCircularFoo.js'),
                    (v)=>{
                        v.reset();
                        assert.areEqual(2, v.circular_foo(), 'This function calls between both modules in the circular dependency incrementing a counter in each');
                        assert.areEqual(4, v.rexportbar(), 'Second call originates in the other module but still increments the counter twice');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;

            testScript(functionBody, "Circular module dependency", false, true);
            testModuleScript(functionBody, "Circular module dependency", false, true);

        }
    },
    {
        name: "Implicitly re-exporting an import binding (import { foo } from ''; export { foo };)",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>import('ModuleComplexReexports.js'),
                    (v)=>{
                        assert.areEqual('foo', v.foo(), 'Simple implicit re-export');
                        assert.areEqual('foo', v.baz(), 'Renamed export imported and renamed during implicit re-export');
                        assert.areEqual('foo', v.localfoo(), 'Export renamed as import and implicitly re-exported');
                        assert.areEqual('foo', v.bar(), 'Renamed export renamed as import and renamed again during implicit re-exported');
                        assert.areEqual('foo', v.localfoo2(), 'Renamed export renamed as import and implicitly re-exported');
                        assert.areEqual('foo', v.bar2(), 'Renamed export renamed as import and renamed again during implicit re-export');
                    },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit);
                `;

            testScript(functionBody, "Implicitly re-exporting an import binding (import { foo } from ''; export { foo };)", false, true);
            testModuleScript(functionBody, "Implicitly re-exporting an import binding (import { foo } from ''; export { foo };)", false, true);
        }
    },
    {
        name: "Validate a simple module export inside eval()",
        body: function () {
            let functionBody =
                `testDynamicImport(
                    ()=>{ return eval("import('ModuleSimpleExport.js')"); },
                    (v)=>{ assert.areEqual('ModuleSimpleExport', v.ModuleSimpleExport_foo(), 'Failed to import ModuleSimpleExport_foo from ModuleSimpleExport.js'); },
                    (err)=>{ assert.fail(err.message); }, _asyncEnter, _asyncExit
                )`;
            testScript(functionBody, "Test importing a simple exported function", false, true);
            testModuleScript(functionBody, "Test importing a simple exported function", false, true);
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
