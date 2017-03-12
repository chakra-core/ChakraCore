//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//
// Test simple expression evaluate
//

// Some data on prototypes to test ToObject
Number.prototype.nump = "numv";
Number.prototype[1] = "Number.p[1]";
Number.prototype[2] = 2;
Boolean.prototype[1] = "Boolean.p[1]";
Boolean.prototype[2] = 2;
String.prototype[1] = "String.p[1]";
String.prototype[2] = 2;
Object.prototype[99] = "Object.p[99]";

//
// Not supported
//
(function foo() {
    var a = {};
    /**bp:
        evaluate('"=========== not supported"');
        evaluate('1 + 2'); evaluate('1 - 2'); evaluate('-2'); evaluate('1*2'); evaluate('1/2');
        evaluate('"\""'); evaluate('a.x = 1'); evaluate('(1)'); evaluate('1;'); evaluate('{');
    **/
})();

//
// empty expression
//
var not_used = 0; // -targeted tests seem to have problems bp at random places. Wrap with statements if not working.
/**bp:
    evaluate(''); evaluate('  ');
    evaluate('Object.prototype.hasOwnProperty'); // Global lookup from global function
**/

//
// this
//
(function foo() {
    var data = [null, undefined, 12, false, "abc", new String("def"), [3, 4, 5], new Int8Array([0, 1, 2, 3, 0, -1, -2, -3]), { thisp: "thisv" }];
    /**bp:
        evaluate('"=========== this"');
    **/

    data.forEach(function (v) {
        v;
        not_used = 0;
        /**bp:
            evaluate('v'); evaluate('v.length'); evaluate('v[1]'); evaluate('v[8]'); evaluate('v.hasOwnProperty');
        **/
        not_used = 0;

        (function () {
            var x;
            /**bp:
                evaluate('this'); evaluate('  this  ', 1); evaluate('this . thisp'); evaluate('this[1]');  evaluate('this[8]');
            **/
        }).apply(v);
    });
})();

//
// arguments
//
(function foo() {
    /**bp:
        evaluate('"=========== arguments"');
    **/

    // Fake arguments
    (function (x) {
        x;
        /**bp:
            evaluate(' arguments ', 1);
            evaluate('arguments.length'); evaluate('arguments[0]'); evaluate('arguments[1]'); evaluate('arguments[2]');
            evaluate('x'); evaluate(' x . nump '); evaluate('x[1]'); evaluate('x.nop'); evaluate('x.nop[1]'); evaluate('x[3]'); evaluate('x[3].nop');
        **/
    }).apply({}, [12, 13]);

    // Real arguments
    (function (x) {
        delete arguments[1];
        x;
        /**bp:
            evaluate(' arguments ', 1);
            evaluate('arguments.length'); evaluate('arguments[0]'); evaluate('arguments[1]'); evaluate('arguments[2]');
            evaluate('x'); evaluate(' x . nump '); evaluate('x[1]'); evaluate('x.nop'); evaluate('x.nop[1]'); evaluate('x[3]'); evaluate('x[3].nop');
        **/
    }).apply({}, [12, 13]);
})();

//
// locals
//
(function foo(x) {
    var a = {
        a1: "a1",
        a2: {
            a21: "a21",
            "1": 0
        },
        a3: [0, 1, 2, , 4],
        "1": 1,
        "2": 2,
        "true": true,
        "undefined": "undef",
        __proto__: {ap: "ap"}
    };
    Object.defineProperty(a, "ah", { value: "non-enum", enumerable: false });
    Object.defineProperty(a.__proto__, "aph", { value: "non-enum", enumerable: false });
    Object.defineProperty(a, "getn", { get: function() { return "getv"; }, enumerable: false });
    this.true = "this_true";

    a;

    /**bp:
        evaluate('"=========== local var"');
        evaluate('a'); evaluate('a.a1'); evaluate('a.a2.a21');
        evaluate('a.b'); evaluate('a.b.c');

        evaluate('a[0]');
        evaluate('a[1]'); evaluate('a[1].b');
        evaluate('a[1][0]'); evaluate('a[1][1]');
        evaluate('a["a1"]'); evaluate('a[a.a1]'); evaluate('a[a.a1[ 2] ]'); evaluate('a[ "a1"[2]] '); evaluate('a["1"]');
        evaluate('a[a.a2[1]]');
        evaluate('a[1[2]]');

        evaluate('a.a3'); evaluate('a.a3.length'); evaluate('a.a3[0]'); evaluate('a.a3[3]'); evaluate('a.a3[a.a3[1[2]]]');

        evaluate('a.true'); evaluate('a.undefined');
        evaluate('a[true]'); evaluate('a[a.true]'); evaluate('a[true[2]]');

        evaluate('123'); evaluate('123[1[2]]');
        evaluate('true');           // const, NOT this.true
        evaluate('this.true');      // compare with above
        evaluate('true[0]');        // missing
        evaluate('true[1]');        // exists

        evaluate('a.ah'); evaluate('a.ap'); evaluate('a.aph');
        evaluate('a.getn'); evaluate('a.getn.length'); evaluate('a[a.getn]');
    **/
}).apply({thisp: "thisv"}, [12, 13]);

//
// scoped lookup
//
(function foo(x) {
    var a = [0, , 2, 3];
    /**bp:evaluate('"=========== scoped lookup"');**/

    for (var i = 0; i < a.length; i++) {
        (function () {
            a[i];
            /**bp:evaluate('a[i]')**/
        })();
    }

    /**bp:
        evaluate('Object.prototype.hasOwnProperty');  // Global lookup from local function
    **/
}).apply({ thisp: "thisv" }, [12, 13]);

//
// BuiltIns
//
(function foo() {
    /**bp:evaluate('"=========== builtins"');**/

    var a = [0, , 2, 3];
    /**bp:evaluate('a'); evaluate('a.length'); evaluate('a[3]'); evaluate('a[99]'); **/
    var a = "abcd";
    /**bp:evaluate('a'); evaluate('a.length'); evaluate('a[3]'); evaluate('a[99]'); **/
    var a = new String("efgh"); a[11] = 11;
    /**bp:evaluate('a'); evaluate('a.length'); evaluate('a[3]'); evaluate('a[11]'); evaluate('a[99]'); **/
    var a = new Uint8Array([1, 2, 3, 4]);
    /**bp:evaluate('a'); evaluate('a.length'); evaluate('a[3]'); evaluate('a[99]');
        evaluate('a.byteLength'); evaluate('a.byteOffset'); evaluate('a.BYTES_PER_ELEMENT'); evaluate('a.buffer');**/

}).apply({});


WScript.Echo("pass");
