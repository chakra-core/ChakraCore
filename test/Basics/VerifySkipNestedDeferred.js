//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Following lines generate a baseline with flags:
// -UseParserStateCache -ParserStateCache -ForceDeferParse -Trace:SkipNestedDeferred

function assertEqual(left, right, msg) {
    if (left != right) {
        console.log(`Fail: ${msg} ('${left}' != '${right}')`);
        return false;
    }
    console.log(`Pass: ${msg}`);
    return true;
}
var promiseToString ='[object Promise]';

var x = 'something'
function bar() {
    function foo() {
        function baz() {
            function onemorefoo() {
                return x;
            }
            return onemorefoo();
        }
        return baz();
    }
    return foo();
}
assertEqual(x, bar(), 'A few nested functions');

function foo() {
    var n = (function() {
        var x = 'x';
        var l = () => {
            var x = 'y';
            return function() {
                return x;
            }
        }
        return l;
    })()
    return n()();
}
assertEqual('y', foo(), 'Nested unnamed function expression');

function f1() {
    async function a1() {
        async function a2() {
            return x;
        }
        return a2();
    }
    return a1();
}
assertEqual(promiseToString, f1(), 'Function with nested async functions');

function f2() {
    function* g1() {
        function* g2() {
            yield x;
        }
        yield g2().next().value;
    }
    return g1().next().value;
}
assertEqual(x, f2(), 'Function with nested generator functions');

function f3() {
    var l1 = (s) => x;
    return l1()
}
assertEqual(x, f3(), 'Nested concise-body lambda');


function f4() {
    var l1 = (s) => { return x }
    return l1()
}
assertEqual(x, f4(), 'Simple nested lambda');

function f5() {
    var l1 = s => { return x }
    return l1()
}
assertEqual(x, f5(), 'Nested concise-argument list lambda');

function f52() {
    var l5 = s => { return s => { return x; } };
    return l5();
}
assertEqual(x, f52()(), 'Couple of nested lambda');

function f6() {
    var o = {
        method(s) { return x; },
        get method2() { return x; },
        ['method3'](arg) { return x; },
        get ['method4']() { return x; },
        async method5(s) { return x; },
        *method6(s) { yield x; },
        *['method7'](s) { yield x; },
        async ['method8'](s) { return x; },
        f8: function() { return x; },
        f9: () => { return x; }
    }
    
    return assertEqual(x, o.method(), 'Simple method') &&
           assertEqual(x, o.method2, 'Simple getter') &&
           assertEqual(x, o.method3(), 'Computed-property named method') &&
           assertEqual(x, o.method4, 'Computed-property named getter') &&
           assertEqual(promiseToString, o.method5(), 'Async method') &&
           assertEqual(x, o.method6().next().value, 'Generator method') &&
           assertEqual(x, o.method7().next().value, 'Generator method with computed-property name') &&
           assertEqual(promiseToString, o.method8(), 'Async method with computed-property name') &&
           assertEqual(x, o.f8(), 'Function stored in object literal property') &&
           assertEqual(x, o.f9(), 'Lambda stored in object literal property');
}
assertEqual(true, f6(), 'Several object literal methods');

function f7() {
    eval('function f5(s) { return x; }');
    return f5();
}
assertEqual(x, f7(), 'Simple function defined in eval');
    
function f8() {
    eval('function f7(r) { function f9() { return x; }; return f9(); }');
    return f7();
}
assertEqual(x, f8(), 'Nested eval functions');
