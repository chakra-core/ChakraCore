//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
var CreateBaseline = false; // Set True to generate Baseline data. Copy the numbers from console to initialize the results variable below.
var debug = false; // Set True to print debug messages.
var debugTestNum = -1; // Set test num to run a specific test. -1 otherwise

var test_values = [-5, 5, 124, 248, 654, 987, -1026, 98768, -88754, 1<<32, -(1<<32),  (1<<32)-1, 1<<31, -(1<<31), 1<<25, -1<<25, -2147483648, 2147483647, 65536, 46341];

var results = [-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,-2147483648,-2147483648,33554432,-33554432,-2147483648,2147483647,65536,46341,5,-5,-124,-248,-654,-987,1026,-98768,88754,-1,1,0,-2147483648,-2147483648,-33554432,33554432,-2147483648,-2147483647,-65536,-46341,
-1,1,41,82,218,329,-342,32922,-29584,0,0,0,-715827882,-715827882,11184810,-11184810,-715827882,715827882,21845,15447,-1,1,24,49,130,197,-205,19753,-17750,0,0,0,-429496729,-429496729,6710886,-6710886,-429496729,429496729,13107,9268,0,0,17,35,93,141,-146,14109,
-12679,0,0,0,-306783378,-306783378,4793490,-4793490,-306783378,306783378,9362,6620,1,-1,-41,-82,-218,-329,342,-32922,29584,0,0,0,715827882,715827882,-11184810,11184810,715827882,-715827882,-21845,-15447,1,-1,-24,-49,-130,-197,205,-19753,17750,0,0,0,
429496729,429496729,-6710886,6710886,429496729,-429496729,-13107,-9268,0,0,-17,-35,-93,-141,146,-14109,12679,0,0,0,306783378,306783378,-4793490,4793490,306783378,-306783378,-9362,-6620,0,0,15,31,81,123,-128,12346,-11094,0,0,0,-268435456,-268435456,4194304,-4194304,
-268435456,268435455,8192,5792,0,0,13,27,72,109,-114,10974,-9861,0,0,0,-238609294,-238609294,3728270,-3728270,-238609294,238609294,7281,5149,0,0,9,19,50,75,-78,7597,-6827,0,0,0,-165191049,-165191049,2581110,-2581110,-165191049,165191049,5041,3564,0,0,3,7,19,
29,-31,2992,-2689,0,0,0,-65075262,-65075262,1016800,-1016800,-65075262,65075262,1985,1404,0,0,2,4,10,16,-17,1646,-1479,0,0,0,-35791394,-35791394,559240,-559240,-35791394,35791394,1092,772,0,0,1,2,6,9,-10,987,-887,0,0,0,-21474836,-21474836,335544,-335544,
-21474836,21474836,655,463,0,0,1,2,5,8,-8,823,-739,0,0,0,-17895697,-17895697,279620,-279620,-17895697,17895697,546,386,0,0,0,0,0,0,-1,98,-88,0,0,0,-2145338,-2145338,33520,-33520,-2145338,2145338,65,46,0,0,0,0,0,0,0,9,-8,0,0,0,-214769,-214769,3355,-3355,-214769,
214769,6,4,0,0,0,0,0,0,0,0,0,0,0,0,-21475,-21475,335,-335,-21475,21475,0,0,-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,-2147483648,-2147483648,33554432,-33554432,-2147483648,2147483647,65536,46341,5,-5,-124,-248,-654,-987,1026,-98768,88754,-1,1,0,
-2147483648,-2147483648,-33554432,33554432,-2147483648,-2147483647,-65536,-46341,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,0,0,-1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,1,0,0,0,-2,2,1,2,0,0,0,2,-2,1,-1,0,-2,-2,2,-2,-2,1,1,0,-2,2,1,2,0,0,0,2,
-2,1,-1,0,-2,-2,2,-2,-2,1,1,0,0,0,4,3,4,2,-1,3,-4,1,-1,0,-3,-3,2,-2,-3,2,1,1,-5,5,5,3,3,0,-4,5,-1,1,-1,0,-2,-2,2,-2,-2,1,2,1,-5,5,5,3,3,0,-4,5,-1,1,-1,0,-2,-2,2,-2,-2,1,2,1,-5,5,7,1,4,12,-12,7,-3,1,-1,0,-11,-11,2,-2,-11,10,3,9,-5,5,19,3,24,7,-11,33,-29,1,-1,
0,-23,-23,2,-2,-23,22,16,1,-5,5,4,8,54,27,-66,8,-74,1,-1,0,-8,-8,32,-32,-8,7,16,21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,0,0,33554432,-33554432,0,2147483647,65536,46341,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
-5,5,124,248,654,987,-1026,98768,-88754,1,-1,0,0,0,33554432,-33554432,0,2147483647,65536,46341]

function asmModule() {
    "use asm";

    function div1(x)  { x = x | 0; return ((x | 0) /  1) | 0; }
    function divn1(x) { x = x | 0; return ((x | 0) / -1) | 0; }

    function div3(x)  { x = x | 0; return ((x | 0) /  3) | 0; } // Key Scenario 1
    function div5(x)  { x = x | 0; return ((x | 0) /  5) | 0; } // Key Scenario 2
    function div7(x)  { x = x | 0; return ((x | 0) /  7) | 0; } // Key Scenario 3
    function divn3(x) { x = x | 0; return ((x | 0) / -3) | 0; }
    function divn5(x) { x = x | 0; return ((x | 0) / -5) | 0; }
    function divn7(x) { x = x | 0; return ((x | 0) / -7) | 0; }
    function div8(x)  { x = x | 0; return ((x | 0) /  8) | 0; }
    function div9(x)  { x = x | 0; return ((x | 0) /  9) | 0; }

    function div13(x) { x = x | 0; return ((x | 0) / 13) | 0; }
    function div33(x) { x = x | 0; return ((x | 0) / 33) | 0; }
    function div60(x) { x = x | 0; return ((x | 0) / 60) | 0; }

    function div100(x) { x = x | 0; return ((x | 0) / 100) | 0; }
    function div120(x) { x = x | 0; return ((x | 0) / 120) | 0; }

    function div1001(x) { x = x | 0; return ((x | 0) / 1001) | 0; }
    function div9999(x) { x = x | 0; return ((x | 0) / 9999) | 0; }
    function div99999(x){ x = x | 0; return ((x | 0) / 99999) | 0; }
    

    function divMax0(x) { x = x | 0; return ((x | 0) / (1<<32)) | 0; }
    function divMax1(x) { x = x | 0; return ((x | 0) / ((1<<32)|0-1|0)) | 0; }
    function divMax2(x) { x = x | 0; return ((x | 0) / 2147483647) | 0; }
    function divMin0(x) { x = x | 0; return ((x | 0) / -2147483648)| 0; }
    function divMin1(x) { x = x | 0; return ((x | 0) / (1<<31)|0 * -1|0)| 0; }

    function rem3(x) { x = x|0; return ((x|0) % 3)|0; }
    function remn3(x) { x = x|0; return ((x|0) % -3)|0; }
    function rem5(x) { x = x|0; return ((x|0) % 5)|0; }
    function remn5(x) { x = x|0; return ((x|0) % -5)|0; }
    function rem7(x) { x = x|0; return ((x|0) % 7)|0; }
    function remn7(x) { x = x|0; return ((x|0) % -7)|0; }

    function rem15(x) { x = x|0; return ((x|0) % 13)|0; }
    function rem35(x) { x = x|0; return ((x|0) % 35)|0; }
    function rem120(x) { x = x|0; return ((x|0) % 120)|0; }
    function remMax0(x) { x = x|0; return ((x|0) % (1<<32)|0)|0; }
    function remMax1(x) { x = x|0; return ((x|0) % (1<<31)|0)|0; }
    function remMin0(x) { x = x|0; return ((x|0) % ((1<<32)|0 * -1|0))|0; }
    function remMin1(x) { x = x|0; return ((x|0) % ((1<<31)|0 * -1|0))|0; }

    return {
        div1    : div1,
        divn1   : divn1,
        div3    : div3,
        div5    : div5,
        div7    : div7,
        divn3   : divn3,
        divn5   : divn5,
        divn7   : divn7,
        div8    : div8,
        div9    : div9,
        div13   : div13,
        div33   : div33,
        div60   : div60,
        div100  : div100,
        div120  : div120,
        div1001 : div1001,
        div9999 : div9999,
        div99999: div99999,
        divMax0 : divMax0,
        divMax1 : divMax1,
        divMax2 : divMax2,
        divMin0 : divMin0,
        divMin1 : divMin1,
        rem3    : rem3,
        remn3   : remn3,
        rem5    : rem5,
        remn5   : remn5,
        rem7    : rem7,
        remn7   : remn7,
        rem15   : rem15,
        rem35   : rem35,
        rem120  : rem120,
        remMax0 : remMax0,
        remMax1 : remMax1,
        remMin0 : remMin0,
        remMin1 : remMin1
    };
}
var am = asmModule();     // produces AOT-compiled version
var fns = [am.div1, am.divn1, am.div3, am.div5, am.div7, am.divn3, am.divn5, am.divn7, am.div8, am.div9, am.div13, am.div33, am.div60,
     am.div100, am.div120, am.div1001, am.div9999, am.div99999, am.divMax0, am.divMax1, am.divMax2, am.divMin0, am.divMin1,
     am.rem3, am.remn3, am.rem5, am.rem7, am.remn7, am.rem15, am.rem35, am.rem120, am.remMax0, am.remMax1, am.remMin0, am.remMin1];

/*****Generate Baseline*********/
function GenerateBaseline() {
    var tmp = [];
    var i = 0;
    fns.forEach(function (fn) {
        test_values.forEach(function (value) {
            if(debug)
            {
                print("Test# "+ i++ + " " + fn.name + "(" + value + ") \tResult: " + fn(value));
            }
            tmp.push(fn(value));
        }, this);
    }, this);
    print("[" + tmp + "]");
}

/******End Baseline gen */

/*Math test for int div strength reduction*/
var test_result = "PASSED";
var total=0,fail=0;
function testSignedDivStrengthReduction() {
    var i = 0;
    total = 0;
    fail  = 0;
    fns.forEach(function (fn) {
        test_values.forEach(function (value) {
            if(debug && debugTestNum == -1)
            {
                print("Test# "+ i + " " + fn.name + "(" + value + ") :\t\tExpected " + results[i] + "\t Found " + fn(value));
            }
            else if(debug && i == debugTestNum)
            {
                print("Test# "+ i + " " + fn.name + "(" + value + ") :\tExpected " + results[i] + "\t Found " + fn(value));
            }
            if (results[i] != fn(value)) {
                print();
                print("TestFail at Test# "+ i + " " + fn.name + "(" + value + ") :\tExpected " + results[i] + "\tFound " + fn(value));
                test_result = "Fail";
                ++fail;
            }
            ++i;
            ++total;
        }, this);
    }, this);
}

if( CreateBaseline )
{
    GenerateBaseline();
}
else
{

    // var a = new Date().getTime();
    // for (var i = 0; i < 1; ++i)
        testSignedDivStrengthReduction();
    // var b = new Date().getTime() - a;
    // print("ElapsedTime = " + b);

    if (fail != 0)
    {
        print(fail + "/" + total + " tests Failed.");
    }
    // print(total + "/" + total + " tests Passed.")
print(test_result);
}
