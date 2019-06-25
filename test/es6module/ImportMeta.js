//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Shape of import.meta object",
        body: function() {
            WScript.RegisterModuleSource('t1.js', `
                assert.areEqual('object', typeof import.meta, 'typeof import.meta === "object"');

                import.meta.prop = 'prop';
                assert.areEqual('prop', import.meta.prop, 'import.meta is mutable');
            `);
            WScript.LoadScriptFile('t1.js', 'module');
        }
    },
    {
        name: "Each module has a unique import.meta object",
        body: function() {
            WScript.RegisterModuleSource('t2.js', `
                export function getImportMeta() {
                    return import.meta;
                }
            `);
            WScript.RegisterModuleSource('t3.js', `
                export let getImportMetaArrow = () => import.meta;
            `);
            WScript.RegisterModuleSource('t4.js', `
                export let importmeta = import.meta;
            `);
            WScript.RegisterModuleSource('t5.js', `
                function foo() {
                    return import.meta;
                }
                assert.areEqual(import.meta, foo(), "Each module has a unique import.meta object");

                import { getImportMeta } from 't2.js';
                let t2_import_meta = getImportMeta();
                import { getImportMetaArrow } from 't3.js';
                let t3_import_meta = getImportMetaArrow();
                import { importmeta as t4_import_meta } from 't4.js';

                assert.areEqual('object', typeof t2_import_meta, 'typeof t2_import_meta === "object"');
                assert.isTrue(t2_import_meta !== import.meta, "t2_import_meta !== import.meta");
                assert.areEqual('object', typeof t3_import_meta, 'typeof t3_import_meta === "object"');
                assert.isTrue(t3_import_meta !== import.meta, "t3_import_meta !== import.meta");
                assert.areEqual('object', typeof t4_import_meta, 'typeof t4_import_meta === "object"');
                assert.isTrue(t4_import_meta !== import.meta, "t4_import_meta !== import.meta");
            `);
            WScript.LoadScriptFile('t5.js', 'module');
        }
    },
    {
        name: "The only metaproperty on import is meta",
        body: function() {
            WScript.RegisterModuleSource('t6.js', `
                function foo() {
                    return import.notmeta;
                }
            `);
            assert.throws(()=>WScript.LoadScriptFile('t6.js', 'module'));
        }
    },
    {
        name: "Non-module goal symbol throws early syntax error",
        body: function() {
            assert.throws(()=>WScript.LoadScript('import.meta', 'samethread'));
            assert.throws(()=>WScript.LoadScript('function foo() { return import.meta; }', 'samethread'));
        }
    },
    {
        name: "import.meta is not an L value",
        body: function() {
            WScript.RegisterModuleSource('t7.js', `
                import.meta = {};
            `);
            WScript.RegisterModuleSource('t8.js', `
                import.meta++;
            `);
            assert.throws(()=>WScript.LoadScriptFile('t7.js', 'module'));
            assert.throws(()=>WScript.LoadScriptFile('t8.js', 'module'));
        }
    },
    {
        name: "import.meta is not valid inside eval",
        body: function() {
            WScript.RegisterModuleSource('t9.js', `
                assert.throws(()=>eval('import.meta'));
            `);
            WScript.LoadScriptFile('t9.js', 'module');
        }
    },
    {
        name: "import.meta is not valid when located in parsed Function body or param list",
        body: function() {
            WScript.RegisterModuleSource('t10.js', `
                assert.throws(()=>Function('import.meta'));
            `);
            WScript.LoadScriptFile('t10.js', 'module');
			WScript.RegisterModuleSource('t11.js', `
                assert.throws(()=>Function('a = import.meta', ''));
            `);
            WScript.LoadScriptFile('t11.js', 'module');
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
