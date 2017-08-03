//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "Serialize functions with unicode sequences",
        body: function () {
            assert.areEqual('function foo() { /* 𢭃 */ }', '' + function foo() { /* 𢭃 */ }, 'Serialized function declaration produces correct string in presense of multi-byte unicode characters');
            assert.areEqual('function 𢭃() { /* 𢭃 */ }', '' + function 𢭃() { /* 𢭃 */ }, 'Serialized function with a unicode identifier');
            assert.areEqual('function 𢭃(ā,食) { /* 𢭃 */ }', '' + function 𢭃(ā,食) { /* 𢭃 */ }, 'Serialized function with a unicode identifier and unicode argument list');
            
            assert.areEqual('async function foo() { /* 𢭃 */ }', '' + async function foo() { /* 𢭃 */ }, 'Serialized async function declaration produces correct string in presense of multi-byte unicode characters');
            assert.areEqual('async function 𢭃() { /* 𢭃 */ }', '' + async function 𢭃() { /* 𢭃 */ }, 'Serialized async function with a unicode identifier');
            assert.areEqual('async function 𢭃(ā,食) { /* 𢭃 */ }', '' + async function 𢭃(ā,食) { /* 𢭃 */ }, 'Serialized async function with a unicode identifier and unicode argument list');
            
            assert.areEqual('function* foo() { /* 𢭃 */ }', '' + function* foo() { /* 𢭃 */ }, 'Serialized generator function declaration produces correct string in presense of multi-byte unicode characters');
            assert.areEqual('function* 𢭃() { /* 𢭃 */ }', '' + function* 𢭃() { /* 𢭃 */ }, 'Serialized generator function with a unicode identifier');
            assert.areEqual('function* 𢭃(ā,食) { /* 𢭃 */ }', '' + function* 𢭃(ā,食) { /* 𢭃 */ }, 'Serialized generator function with a unicode identifier and unicode argument list');
            
            assert.areEqual('() => { /* 𢭃 */ }', '' + (() => { /* 𢭃 */ }), 'Serialized arrow function declaration produces correct string in presense of multi-byte unicode characters');
            assert.areEqual('(ā,食) => { /* 𢭃 */ }', '' + ((ā,食) => { /* 𢭃 */ }), 'Serialized arrow function declaration with a unicode argument list');
            
            assert.areEqual('async () => { /* 𢭃 */ }', '' + (async () => { /* 𢭃 */ }), 'Serialized async arrow function declaration produces correct string in presense of multi-byte unicode characters');
            assert.areEqual('async (ā,食) => { /* 𢭃 */ }', '' + (async (ā,食) => { /* 𢭃 */ }), 'Serialized async arrow function declaration with a unicode argument list');
        }
    },
    {
        name: "Serialize classes with unicode sequences",
        body: function () {
            assert.areEqual('class 𢭃 { /* 𢭃 */ }', '' + class 𢭃 { /* 𢭃 */ }, 'Serialized class declaration produces correct string in presense of multi-byte unicode characters');
            
            class ā { 𢭃(物) { /* 𢭃 */ } static 飲(物) { /* 𢭃 */ } async 知(物) { /* 𢭃 */ } static async 愛(物) { /* 𢭃 */ } *泳(物) { /* 𢭃 */ } static *赤(物) { /* 𢭃 */ } get 青() { /* 𢭃 */ } set 緑(物) { /* 𢭃 */ } }
            
            assert.areEqual(
            'class ā { 𢭃(物) { /* 𢭃 */ } static 飲(物) { /* 𢭃 */ } async 知(物) { /* 𢭃 */ } static async 愛(物) { /* 𢭃 */ } *泳(物) { /* 𢭃 */ } static *赤(物) { /* 𢭃 */ } get 青() { /* 𢭃 */ } set 緑(物) { /* 𢭃 */ } }',
            '' + ā,
            'Serialized class with different types of members');
            
            class 食 extends ā { 母(物) { /* 𢭃 */ } static 父(物) { /* 𢭃 */ } async 妹(物) { /* 𢭃 */ } static async 姉(物) { /* 𢭃 */ } *兄(物) { /* 𢭃 */ } static *耳(物) { /* 𢭃 */ } get 明() { /* 𢭃 */ } set 日(物) { /* 𢭃 */ } }
            
            assert.areEqual(
            `class 食 extends ā { 母(物) { /* 𢭃 */ } static 父(物) { /* 𢭃 */ } async 妹(物) { /* 𢭃 */ } static async 姉(物) { /* 𢭃 */ } *兄(物) { /* 𢭃 */ } static *耳(物) { /* 𢭃 */ } get 明() { /* 𢭃 */ } set 日(物) { /* 𢭃 */ } }`,
            '' + 食,
            'Serialized class with an extends clause');
        }
    },
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
