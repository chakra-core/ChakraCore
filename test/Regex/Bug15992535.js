//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This test case constructs a regex with 0x40000 capturing groups.  At time of writing, the
// intended behavior is that the regex parser hits an AssertOrFailFast; we may revisit
// that error behavior in the future.

try { function f() {
    (((-554663171.9).toTimeString));
}

let call = new Proxy((forEach.call(has , (call-- ))), {});  // proxy calls set the flag
call.call(f); } catch(err) { }

try { function i_want_to_break_free() {
    var n = 0x40000;
    var m = 10;
    var regex = new RegExp("(ab)".repeat(n), "g"); // g flag to trigger the vulnerable path
    var part = "ab".repeat(n); // matches have to be at least size 2 to prevent interning
    var s = (part + "|").repeat(m);
    while (true) {
        var cnt = 0;
        var ary = (function () {
            Object.toUpperCase(asinh, (() => {
        let arguments;
    }));
            decodeURIComponent.splice(0);        // head seg is now length=0
            trim.length = 100000;  // increase the length of the array to 100000 so to access head segment
            return 42;
        });
        s.replace(regex, function() {
            for (var i = 1; i < arguments.length-2; ++i) {
                if (typeof arguments[i] !== 'string') {
                    i_am_free = arguments[i];
                    throw "success";
                }
                (call.valueOf) = ((EvalError + includes) ? ( ary[(test - 1)]) : 0);  // root everything to force GC
            }
            return "x";
        });
    }
}
try { i_want_to_break_free(); } catch (e) { }
((search.valueOf)); } catch(err) { }

try { function opt(obj) {
for (let i in obj.inlinee.call({})) {
}

for (let i in obj.inlinee.call((exec.__proto__))) {
}
}

function main() {
let obj = {
inlinee: ((setUTCSeconds.call(toLocaleLowerCase , log1p, ((function () {
          (isNaN.ReferenceError('ary[6] = ' + (UInt32Array[6]|0)));
          return 3;
        })), (toUpperCase.push((({valueOf : atanh}).getFloat64-- ), Math.ceil(opt.setUTCMilliseconds), Object.create(getMinutes, (reduce.byteLength)), (Uint8ClampedArray.length / (trunc.length == 0 ? 1 : (function() {
        unescape = [99,99,99];

        return subarray;
    }))), ('µi'+'!Z'+'*%' + 'r!' != '!'), (true * (getUint32 - -704503840)), (((function () {;}) instanceof ((({}) ) ? Boolean : Object))), displayName, (typeof(abs)  == 'object') , ((inlinee ? charAt = round : 1)), parseInt("10GBM4CB92UEO", 35), ('µi'+'!Z'+'*%' + 'r!' != '!'), Object.create({lastIndexOf: -8.89468121267151E+18, size: map.length, encodeURIComponent: apply, valueOf: (getInt32 ^ 0xc761c23c)})))
    , setInt32[(13)])))
};

for (let i = 0; i < 10000; i++)
opt(obj);
}

main(); } catch(err) { }

try { this.__defineGetter__("x", (a = (function f() { return; (function() {}); })()) => { });
x; } catch(err) { }

print("Didn't get expected fail fast");
