//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// ES6 Module syntax tests -- verifies syntax of import and export statements

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

function testModuleScript(source, message, shouldFail = false) {
    let testfunc = () => testRunner.LoadModule(source, undefined, shouldFail);

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
        name: "Issue3249: Namespace object's property attributes",
        body: function () {
			testModuleScript(`
            function verifyPropertyDesc(obj, prop, desc, propName) {
                var actualDesc = Object.getOwnPropertyDescriptor(obj, prop);
                if (typeof propName === "undefined") { propName = prop; }
                assert.areEqual(desc.configurable, actualDesc.configurable, propName+"'s attribute: configurable");
                assert.areEqual(desc.enumerable, actualDesc.enumerable, propName+"'s attribute: enumerable");
                assert.areEqual(desc.writable, actualDesc.writable, propName+"'s attribute: writable");
            }

			import * as foo from "ValidExportStatements.js";
            assert.areEqual("Module", foo[Symbol.toStringTag], "@@toStringTag is the String value'Module'");
            verifyPropertyDesc(foo, Symbol.toStringTag, {configurable:false, enumerable: false, writable: false}, "Symbol.toStringTag");
            verifyPropertyDesc(foo, "default", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var7", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var6", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var4", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var3", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "var1", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "foo4", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "bar2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "foobar", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "foo3", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "baz2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "foo2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "baz", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "bar", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "foo", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "const6", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "const5", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "const4", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "const3", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "const2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let7", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let6", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let5", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let4", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "let1", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "cl2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "cl1", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "gn2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "gn1", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "fn2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo, "fn1", {configurable:false, enumerable: true, writable: true});

            import * as foo1 from "ValidReExportStatements.js";
            verifyPropertyDesc(foo1, "foo", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo1, "bar", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo1, "baz", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo1, "foo2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo1, "bar2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo1, "foo3", {configurable:false, enumerable: true, writable: true});

            import * as foo2 from "ModuleComplexReexports.js";
            verifyPropertyDesc(foo2, "ModuleComplexReexports_foo", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "bar2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "localfoo2", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "bar", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "localfoo", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "baz", {configurable:false, enumerable: true, writable: true});
            verifyPropertyDesc(foo2, "foo", {configurable:false, enumerable: true, writable: true});
		    `, '', false);
        }
    },
    {
        name: "Basic import namespace",
        body: function () {
			testModuleScript(`
			import * as foo from "ValidExportStatements.js";
			assert.areEqual("default", foo.default, "default");
			assert.areEqual(undefined, foo.var7, "var7");
			assert.areEqual(undefined, foo.var6, "var6");
			assert.areEqual(undefined, foo.var5, "var5");
			assert.areEqual(undefined, foo.var4, "var4");
			assert.areEqual(5, foo.var3, "var3");
			assert.areEqual(undefined, foo.var2, "var2");
			assert.areEqual("string", foo.var1, "var1");
			assert.areEqual("function foo() { }", foo.foo4.toString(), "foo4");
			assert.areEqual("class bar { }", foo.bar2.toString(), "bar2");
			assert.areEqual("function foobar() { }", foo.foobar.toString(), "foobar");
			assert.areEqual("function foo() { }", foo.foo3.toString(), "foo3");
			assert.areEqual("function* baz() { }", foo.baz2.toString(), "baz2");
			assert.areEqual("function foo() { }", foo.foo2.toString(), "foo2");
			assert.areEqual("function* baz() { }", foo.baz.toString(), "baz");
			assert.areEqual("class bar { }", foo.bar.toString(), "bar");
			assert.areEqual("function foo() { }", foo.foo.toString(), "foo");
			assert.areEqual([], foo.const6, "const6");
			assert.areEqual({}, foo.const5, "const5");
			assert.areEqual(4, foo.const4, "const4");
			assert.areEqual(3, foo.const3, "const3");
			assert.areEqual("str", foo.const2, "const2");
			assert.areEqual([], foo.let7, "let7");
			assert.areEqual({}, foo.let6, "let6");
			assert.areEqual(undefined, foo.let5, "let5");
			assert.areEqual(undefined, foo.let4, "let4");
			assert.areEqual(undefined, foo.let3, "let3");
			assert.areEqual(2, foo.let2, "let2");
			assert.areEqual(undefined, foo.let1, "let1");
			assert.areEqual("class cl2 { }", foo.cl2.toString(), "cl2");
			assert.areEqual("class cl1 { }", foo.cl1.toString(), "cl1");
			assert.areEqual("function* gn2() { }", foo.gn2.toString(), "gn2");
			assert.areEqual("function* gn1() { }", foo.gn1.toString(), "gn1");
			assert.areEqual("function fn2() { }", foo.fn2.toString(), "fn2");
			assert.areEqual("function fn1() { }", foo.fn1.toString(), "fn1");
		    `, '', false);
        }
    },
    {
        name: "import namespace with verification",
        body: function () {
			testModuleScript(`
			import * as foo from "moduleExport1.js";
			assert.areEqual("default", foo.default, "default");
			assert.areEqual(undefined, foo.var7, "var7");
			assert.areEqual(undefined, foo.var6, "var6");
			assert.areEqual(undefined, foo.var5, "var5");
			assert.areEqual(undefined, foo.var4, "var4");
			assert.areEqual(5, foo.var3, "var3");
			assert.areEqual(undefined, foo.var2, "var2");
			assert.areEqual("string", foo.var1, "var1");
			assert.areEqual("function foo() { }", foo.foo4.toString(), "foo4");
			assert.areEqual("class bar { }", foo.bar2.toString(), "bar2");
			assert.areEqual("function foobar() { }", foo.foobar.toString(), "foobar");
			assert.areEqual("function foo() { }", foo.foo3.toString(), "foo3");
			assert.areEqual("function* baz() { }", foo.baz2.toString(), "baz2");
			assert.areEqual("function foo() { }", foo.foo2.toString(), "foo2");
			assert.areEqual("function* baz() { }", foo.baz.toString(), "baz");
			assert.areEqual("class bar { }", foo.bar.toString(), "bar");
			assert.areEqual("function foo() { }", foo.foo.toString(), "foo");
			assert.areEqual([], foo.const6, "const6");
			assert.areEqual({}, foo.const5, "const5");
			assert.areEqual(4, foo.const4, "const4");
			assert.areEqual(3, foo.const3, "const3");
			assert.areEqual("str", foo.const2, "const2");
			assert.areEqual([], foo.let7, "let7");
			assert.areEqual({}, foo.let6, "let6");
			assert.areEqual(undefined, foo.let5, "let5");
			assert.areEqual(undefined, foo.let4, "let4");
			assert.areEqual(undefined, foo.let3, "let3");
			assert.areEqual(2, foo.let2, "let2");
			assert.areEqual(undefined, foo.let1, "let1");
			assert.areEqual("class cl2 { }", foo.cl2.toString(), "cl2");
			assert.areEqual("class cl1 { }", foo.cl1.toString(), "cl1");
			assert.areEqual("function* gn2() { }", foo.gn2.toString(), "gn2");
			assert.areEqual("function* gn1() { }", foo.gn1.toString(), "gn1");
			assert.areEqual("function fn2() { }", foo.fn2.toString(), "fn2");
			assert.areEqual("function fn1() { }", foo.fn1.toString(), "fn1");
			foo.verifyNamespace(foo);
			foo.changeContext();
			foo.verifyNamespace(foo);
			`, '', false);
        }
    },
    {
        name: "reexport only",
        body: function () {
            testModuleScript(`
            import * as foo from "ValidReExportStatements.js";
			assert.areEqual("function foo() { }", foo.foo.toString(), "foo");
			assert.areEqual("class bar { }", foo.bar.toString(), "bar");
			assert.areEqual("function* baz() { }", foo.baz.toString(), "baz");
			assert.areEqual("function foo() { }", foo.foo2.toString(), "foo2");
			assert.areEqual("class bar { }", foo.bar2.toString(), "bar2");
			assert.areEqual("function foo() { }", foo.foo3.toString(), "foo3");
			`, '', false);
        }
    },
    {
        name: "complex reexport",
        body: function () {
            testModuleScript(`import * as fooComplex from "ModuleComplexReexports.js";
			assert.areEqual("function bar() { return 'bar'; }", fooComplex.ModuleComplexReexports_foo.toString(), "ModuleComplexReexports_foo");
			assert.areEqual(undefined, fooComplex.switch, "switch");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.bar2.toString(), "bar2");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.localfoo2.toString(), "localfoo2");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.bar.toString(), "bar");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.localfoo.toString(), "localfoo");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.baz.toString(), "baz");
			assert.areEqual("function foo() { return 'foo'; }", fooComplex.foo.toString(), "foo");
		    `, '', false);
        }
    },
    {
        name: "namespace as prototype",
        body: function () {
            testModuleScript(`import * as foo from "ValidExportStatements.js";
			var childObj = Object.create(foo);
			assert.areEqual("default", childObj.default, "default");
			assert.areEqual(undefined, childObj.var7, "var7");
			assert.areEqual(undefined, childObj.var6, "var6");
			assert.areEqual(undefined, childObj.var5, "var5");
			assert.areEqual(undefined, childObj.var4, "var4");
			assert.areEqual(5, childObj.var3, "var3");
			assert.areEqual(undefined, childObj.var2, "var2");
			assert.areEqual("string", childObj.var1, "var1");
			assert.areEqual("function foo() { }", childObj.foo4.toString(), "foo4");
			assert.areEqual("class bar { }", childObj.bar2.toString(), "bar2");
			assert.areEqual("function foobar() { }", childObj.foobar.toString(), "foobar");
			assert.areEqual("function foo() { }", childObj.foo3.toString(), "foo3");
			assert.areEqual("function* baz() { }", childObj.baz2.toString(), "baz2");
			assert.areEqual("function foo() { }", childObj.foo2.toString(), "foo2");
			assert.areEqual("function* baz() { }", childObj.baz.toString(), "baz");
			assert.areEqual("class bar { }", childObj.bar.toString(), "bar");
			assert.areEqual("function foo() { }", childObj.foo.toString(), "foo");
			assert.areEqual([], childObj.const6, "const6");
			assert.areEqual({}, childObj.const5, "const5");
			assert.areEqual(4, childObj.const4, "const4");
			assert.areEqual(3, childObj.const3, "const3");
			assert.areEqual("str", childObj.const2, "const2");
			assert.areEqual([], childObj.let7, "let7");
			assert.areEqual({}, childObj.let6, "let6");
			assert.areEqual(undefined, childObj.let5, "let5");
			assert.areEqual(undefined, childObj.let4, "let4");
			assert.areEqual(undefined, childObj.let3, "let3");
			assert.areEqual(2, childObj.let2, "let2");
			assert.areEqual(undefined, childObj.let1, "let1");
			assert.areEqual("class cl2 { }", childObj.cl2.toString(), "cl2");
			assert.areEqual("class cl1 { }", childObj.cl1.toString(), "cl1");
			assert.areEqual("function* gn2() { }", childObj.gn2.toString(), "gn2");
			assert.areEqual("function* gn1() { }", childObj.gn1.toString(), "gn1");
			assert.areEqual("function fn2() { }", childObj.fn2.toString(), "fn2");
			assert.areEqual("function fn1() { }", childObj.fn1.toString(), "fn1");
			`, '', false);
       }
    },
    {
        name: "namespace internal operations",
        body: function () {
            let functionBody =
                `import * as foo from "ValidExportStatements.js";
                assert.areEqual(null, Object.getPrototypeOf(foo), 'namespace prototype is null');
                assert.areEqual(false, Object.isExtensible(foo), 'namespace is not extensible');
                assert.areEqual(false, Reflect.set(foo, "non_existing", 20), '[[set]] returns false ');
                assert.areEqual(undefined, foo.non_existing, 'namespace object is immutable');
                assert.areEqual(false, Reflect.set(foo, "4", 20), 'cannot set item in namespace obect');
                assert.areEqual(undefined, foo[4], 'cannot export item in namespace obect');
                assert.areEqual(false, Reflect.deleteProperty(foo, "var1"), 'cannot delete export in namespace obect');
                assert.areEqual(true, Reflect.deleteProperty(foo, "nonevar"), 'cannot delete export in namespace obect');
                assert.areEqual(undefined, foo[6], 'cannot get item in namespace obect');
                assert.areEqual(false, Reflect.set(foo, Symbol.species, 20), 'no species property');
                assert.areEqual(undefined, foo[Symbol.species], 'namespace is not contructor');
                assert.areEqual("Module", foo[Symbol.toStringTag], 'namespace toStringTag');
                assert.areEqual("[object Module]", Object.prototype.toString.call(foo), 'Object.prototype.toString uses the module namespace @@toStringTag value');
                var symbols = Object.getOwnPropertySymbols(foo);
                assert.areEqual(1, symbols.length, "one symbol");
                assert.areEqual(symbols[0], Symbol.toStringTag, "first symbol is toStringTag");
                assert.isFalse(Object.prototype.hasOwnProperty.call(foo, Symbol.iterator), "Module namespace object should not have Symbol.iterator property");
                assert.throws( function() {Object.setPrototypeOf(foo, Object)}, TypeError, 'Cannot create property for a non-extensible object');
                assert.areEqual(true, Reflect.preventExtensions(foo), '[[PreventExtensions]] for namespace object returns true');`;
            testModuleScript(functionBody, "Test importing as different binding identifiers", false);
       }
    },
    {
        name: "Issue3246: namespace property names are sorted",
        body: function () {
            let functionBody =
                `
                import * as ns from 'ValidExportStatements2.js';
                var p = new Proxy(ns, {});
                var names = ["sym0","default","$","$$","_","\u03bb","aa","A","a","zz","z","\u03bc","Z","za","__","az","\u03c0"].sort();

                var verifyNamespaceOwnProperty = function(obj, objKind) {
                    assert.areEqual(names, Object.getOwnPropertyNames(obj), objKind+" getOwnPropertyNames()");

                    var propDesc = Object.getOwnPropertyDescriptors(obj);
                    assert.areEqual('{"value":"Module","writable":false,"enumerable":false,"configurable":false}', JSON.stringify(propDesc[Symbol.toStringTag]),
                        objKind+" getOwnPropertyDescriptors() @@toStringTag");
                    assert.areEqual(names, Object.keys(propDesc), "ModuleNamespace", objKind+" getOwnPropertyDescriptors()");
                };

                verifyNamespaceOwnProperty(ns, "ModuleNamespace");
                verifyNamespaceOwnProperty(p, "Proxy");

                var propEn = [];
                for (var k in ns) { propEn.push(k); }
                assert.areEqual(names, propEn, "ModuleNamespace enumerator");
                `;
            testModuleScript(functionBody, "Test importing as different binding identifiers", false);
       }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
