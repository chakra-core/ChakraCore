//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// This test case constructs a regex with 0x40000 capturing groups.  At time of writing, the
// intended behavior is that the regex parser hits an AssertOrFailFast; we may revisit
// that error behavior in the future.

try { function i_want_to_break_free() {
    var n = 0x8000;
    var m = 2;
    var regex = new RegExp("(ab)".repeat(n), "g");
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

print("Didn't get expected fail fast");
