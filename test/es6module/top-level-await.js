//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const promises = [];

let resolveOne, resolveTwo;
let counter = 0;
const firstPromise = new Promise(r => { resolveOne = r});
const secondPromise = new Promise(r => { resolveTwo = function(a)
    {
        if (counter > 0) throw new Error("should only be called once");
        ++ counter;
        r(a);
    }});
export default resolveOne;
export {resolveTwo};

// async tests without a baseline by using an array of promises
function AddPromise(test, promise, result, shouldFail = false)
{
    const resultPromise = shouldFail ?
    promise.then(
        ()=>{throw new Error(`${test} failed - promise should have been rejected`);},
        (x)=>{if (result !== x) {throw new Error(`${test} failed - ${JSON.stringify(x)} should equal ${JSON.stringify(result)}`);}}
        ) :
    promise.then(
        (x)=>{if (result !== x) {throw new Error(`${test} failed - ${JSON.stringify(x)} should equal ${JSON.stringify(result)}`);}},
        (x)=>{throw new Error(`${test} failed - ${x}`);}
    );
    promises.push(resultPromise);
}

const tests = [
    {
        name : "Load a single async module",
        body(test) {
            WScript.RegisterModuleSource("Single1", `
                export default 5;
                await 0;
            `);
            AddPromise(test, import("Single1").then(x => x.default), 5, false);
        }
    },
    {
        name : "Syntax in an async module",
        body(test) {
            WScript.RegisterModuleSource("Syntax1", '(await 0) = 1;');
            WScript.RegisterModuleSource("Syntax2", 'await = 1;');
            WScript.RegisterModuleSource("Syntax3", 'await import "Syntax1"');
            WScript.RegisterModuleSource("Syntax4", 'await + 5;');
            WScript.RegisterModuleSource("Syntax5", 'import await "bar";');
            WScript.RegisterModuleSource("Syntax6", 'await 0; function bar () { await 0; }');
            WScript.RegisterModuleSource("Syntax7", 'await 0; yield 5;');
            WScript.RegisterModuleSource("Syntax8", 'function foo() { await 0; }');
            WScript.RegisterModuleSource("Syntax9", 'function* foo() { await 0; }');

            AddPromise(test, import("Syntax1").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax2").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax3").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax4").then(x => x.default), undefined, false);
            AddPromise(test, import("Syntax5").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax6").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax7").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax8").catch(x => (x instanceof SyntaxError)), true, false);
            AddPromise(test, import("Syntax9").catch(x => (x instanceof SyntaxError)), true, false);
        }
    },
    {
        name : "Basic Async load order",
        body(test) {
            WScript.RegisterModuleSource("LoadOrder1", 'export default [];');
            WScript.RegisterModuleSource("LoadOrder2", `
                import arr from 'LoadOrder1';
                export {arr as default};
                arr.push("LO2a1");
                await 0;
                arr.push("LO2a2");
                await 0;
                await 0;
                arr.push("LO2a3");
                await 0;
                await 0;
                await 0;
                arr.push("LO2a4");
                await 0;
                arr.push("LO2a5");
                await 0;
                await 0;
                await 0;
                arr.push("LO2a6");
            `);
            WScript.RegisterModuleSource("LoadOrder3", `
                import arr from 'LoadOrder1';
                export {arr as default};
                arr.push("LO3a1");
                await 0;
                arr.push("LO3a2");
            `);
            WScript.RegisterModuleSource("LoadOrder4", `
                import arr from 'LoadOrder2';
                import 'LoadOrder3'
                export {arr as default};
                arr.push("LO4");
            `);
            WScript.RegisterModuleSource("LoadOrder5", `
                import arr from 'LoadOrder3';
                export {arr as default};
                arr.push("LO5a1");
                await 0;
                arr.push("LO5a2");
            `);
            WScript.RegisterModuleSource("LoadOrder6", `
                import arr from 'LoadOrder5';
                export {arr as default};
                arr.push("LO6");
            `);
            WScript.RegisterModuleSource("LoadOrder7", `
                import arr from 'LoadOrder6';
                import 'LoadOrder4';
                export {arr as default};
                arr.push("LO7");
            `);    
            import('LoadOrder4');
            AddPromise(test, import('LoadOrder1').then(() => import('LoadOrder7').then(x => x.default.toString())),
                "LO2a1,LO3a1,LO2a2,LO3a2,LO2a3,LO5a1,LO2a4,LO5a2,LO2a5,LO6,LO2a6,LO4,LO7", false);
        }
    },
    {
        name : "Cyclic load order",
        body(test) {
            WScript.RegisterModuleSource("CyclicOrder1", 'export default [];');
            WScript.RegisterModuleSource("CyclicOrder2", `
                import arr from 'CyclicOrder1';
                import {testFunc} from 'CyclicOrder4';
                testFunc();
                arr.push("CO2a1");
                await 0;
                arr.push("CO2a2");
            `);
            WScript.RegisterModuleSource("CyclicOrder3", `
                import arr from 'CyclicOrder1';
                import 'CyclicOrder2';
                arr.push("CO3a1");
                await 0;
                arr.push("CO3a2");
            `);
            WScript.RegisterModuleSource("CyclicOrder4", `
                import arr from 'CyclicOrder1';
                export {arr as default};
                export function testFunc () { arr.push("testFunc");}
                import 'CyclicOrder3';
                arr.push("CO4a1");
                await 0;
                arr.push("CO4a2");
            `);
            AddPromise(test, import('CyclicOrder4').then(x => x.default.toString()), "testFunc,CO2a1,CO2a2,CO3a1,CO3a2,CO4a1,CO4a2", false);
        }
    },
    {
        name : "Double Cyclic load order",
        body(test) {
            WScript.RegisterModuleSource('CyclicDouble1', 'export default [];');
            WScript.RegisterModuleSource('CyclicDouble2', `
                import arr from 'CyclicDouble1';
                import 'CyclicDouble4';
                arr.push("CD2a1");
                await 0;
                arr.push("CD2a2");
            `);
            WScript.RegisterModuleSource('CyclicDouble3', `
                import arr from 'CyclicDouble1';
                import 'CyclicDouble2'
                arr.push("CD3a1");
                await 0;
                arr.push("CD3a2");
            `);
            WScript.RegisterModuleSource('CyclicDouble4', `
                import arr from 'CyclicDouble1';
                import 'CyclicDouble3'
                arr.push("CD4a1");
                await 0;
                arr.push("CD4a2");
            `);
            WScript.RegisterModuleSource('CyclicDouble5', `
                import arr from 'CyclicDouble1';
                arr.push("CD5");
            `);
            WScript.RegisterModuleSource('CyclicDouble6', `
                import arr from 'CyclicDouble1';
                import 'CyclicDouble5'
                import 'CyclicDouble3'
                export {arr as default};
                arr.push("CD6a1");
                await 0;
                arr.push("CD6a2");
            `);
            import ('CyclicDouble4');
            AddPromise(test, import ('CyclicDouble1').then(() => import('CyclicDouble6').then(x => x.default.toString())), "CD2a1,CD2a2,CD3a1,CD3a2,CD5,CD4a1,CD6a1,CD4a2,CD6a2", false);
        }
    },
    {
        name : "Rejections",
        body (test) {
            WScript.RegisterModuleSource("Rejection1", 'await 0; throw new Error("rejection")');
            WScript.RegisterModuleSource("Rejection2", 'import "Rejection1"');

            AddPromise(test, import('Rejection1').catch(x => (x instanceof Error && x.message === "rejection")), true, false);
            AddPromise(test, import('Rejection2').catch(x => (x instanceof Error && x.message === "rejection")), true, false);

            WScript.RegisterModuleSource("CyclicRejection1", 'export default [];');
            WScript.RegisterModuleSource("CyclicRejection2", `
                import arr from 'CyclicRejection1';
                import 'CyclicRejection3';
                arr.push("CR1a1");
                await 0;
                arr.push("CR1a2");
            `);
            WScript.RegisterModuleSource("CyclicRejection3", `
                import arr from 'CyclicRejection1';
                import 'CyclicRejection2';
                arr.push("CR2a1");
                await Promise.reject('REJECTED');
                arr.push("CR2a2");
            `);

            AddPromise(test, import('CyclicRejection3'), "REJECTED", true);
            AddPromise(test, import('CyclicRejection3').catch(() => import('CyclicRejection1').then(x => x.default.toString())), "CR1a1,CR1a2,CR2a1", false);
        }
    },
    {
        name : "Test static loading",
        body (test) {
            WScript.RegisterModuleSource("Static", `
                import resolve from 'top-level-await.js';
                await 0;
                resolve("static pass");
            `);
            WScript.LoadScriptFile("Static", "module");
            AddPromise(test, firstPromise, "static pass", false);
        }
    },
    {
        name : "Root and dynamic",
        body (test) {
            WScript.RegisterModuleSource("Dynamic Root", `
                export let bar = 0;
                import {resolveTwo} from 'top-level-await.js';
                await 0;
                resolveTwo("other pass");
            `);
            WScript.LoadScriptFile("Dynamic Root", "module");
            AddPromise(test, import("Dynamic Root").then(x => x.bar), 0, false);
            AddPromise(test, secondPromise, "other pass", false);
        }
    },
    {
        name : "For-await-of in module",
        body (test) {
            WScript.RegisterModuleSource("for-await-of", `
                let out = 0;
                for await (const bar of [2,3,4]) {
                    out += bar;
                }
                export default out;
            `);
            AddPromise(test, import('for-await-of').then (x => x.default), 9, false);
        }
    }
];


for(let i = 0; i < tests.length; ++i)
{
    const test = tests[i];
    test.body(`Test ${i + 1}: ${test.name}`);
}

Promise.all(promises).then(x => print("pass"), x => print (x));
