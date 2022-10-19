//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("../UnitTestFramework/UnitTestFramework.js");

const tests = [
    {
        name: "Uint8Array.prototype.sort basic properties",
        body() {
            assert.areEqual(1, Uint8Array.prototype.sort.length, "Uint8Array.prototype.sort should have length of 1");
            assert.areEqual("sort", Uint8Array.prototype.sort.name, "Uint8Array.prototype.sort should have name 'sort'");
            const desc = Object.getOwnPropertyDescriptor(Uint8Array.prototype.__proto__, "sort");
            assert.isTrue(desc.writable, "Uint8Array.prototype.sort.writable === true");
            assert.isFalse(desc.enumerable, "Uint8Array.prototype.sort.enumerable === false");
            assert.isTrue(desc.configurable, "Uint8Array.prototype.sort.configurable === true");
            assert.areEqual('function', typeof desc.value, "typeof Uint8Array.prototype.sort === 'function'");
            assert.throws(() => { [].sort("not a function"); }, TypeError);
            assert.throws(() => { [].sort(null); }, TypeError);
            assert.doesNotThrow(() => { [].sort(undefined); }, "Uint8Array.prototype.sort with undefined sort parameter does not throw");
        }
    },
    {
        name: "Uint8Array.prototype.sort basic sort cases with arrays of numbers",
        body() {
            const arrayOne = Uint8Array.from([120, 5, 8, 4, 6, 9, 9, 10, 2, 3]);
            assert.areEqual(Uint8Array.from([2, 3, 4, 5, 6, 8, 9, 9, 10, 120]), arrayOne.slice().sort(undefined), "Uint8Array.sort with default comparator should sort based on numerical ordering");
            const result = Uint8Array.from([2, 3, 4, 5, 6, 8, 9, 9, 10, 120]);
            assert.areEqual(result, arrayOne.sort((x, y) => x - y), "Uint8Array.sort with numerical comparator should sort numerically");
            assert.areEqual(result, arrayOne, "Uint8Array.sort should sort original array as well as return value");
            assert.areEqual(result, arrayOne.sort((x, y) => { return (x > y) ? 1 : ((x < y) ? -1 : 0); }), "1/-1/0 numerical comparison should be the same as x - y");
            const arrayTwo = Uint8Array.from([25, 8, 7, 41]);
            assert.areEqual(Uint8Array.from([41, 25, 8, 7]), arrayTwo.sort((a, b) => b - a), "Uint8Array.sort with (a,b)=>b-a should sort descending");
            assert.areEqual(Uint8Array.from([7, 8, 25, 41]), arrayTwo.sort((a, b) => a - b), "Uint8Array.sort with (a,b)=>a-b should sort ascending");
        }
    },
    {
        name: "Uint16Array.prototype.sort stability",
        body() {
            const arrayOne = Uint16Array.from(Array(50).keys());
            const compare = (a, b) =>  {if (a >> 1 == a / 2 ){ return 1} else { return b - a;}}
            assert.areEqual(arrayOne.sort(compare), Uint16Array.from([
                    49,47,45,43,41,39,37,35,33,31,29,27,25,23,21,19,17,15,13,11,
                    9,7,5,3,1,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,
                    30,32,34,36,38,40,42,44,46,48]), "Uint16Array.prototype.sort with short Array should be stable");
            // longer array has to be tested as well because a different sort algorithm is used for arrays longer than 512 elements
            const arrayTwo = Uint16Array.from(Array(600).keys());
            assert.areEqual(arrayTwo.sort(compare), Uint16Array.from([
                599,597,595,593,591,589,587,585,583,581,579,577,575,573,571,569,567,565,563,561,
                559,557,555,553,551,549,547,545,543,541,539,537,535,533,531,529,527,525,523,521,
                519,517,515,513,511,509,507,505,503,501,499,497,495,493,491,489,487,485,483,481,
                479,477,475,473,471,469,467,465,463,461,459,457,455,453,451,449,447,445,443,441,
                439,437,435,433,431,429,427,425,423,421,419,417,415,413,411,409,407,405,403,401,
                399,397,395,393,391,389,387,385,383,381,379,377,375,373,371,369,367,365,363,361,
                359,357,355,353,351,349,347,345,343,341,339,337,335,333,331,329,327,325,323,321,
                319,317,315,313,311,309,307,305,303,301,299,297,295,293,291,289,287,285,283,281,
                279,277,275,273,271,269,267,265,263,261,259,257,255,253,251,249,247,245,243,241,
                239,237,235,233,231,229,227,225,223,221,219,217,215,213,211,209,207,205,203,201,
                199,197,195,193,191,189,187,185,183,181,179,177,175,173,171,169,167,165,163,161,
                159,157,155,153,151,149,147,145,143,141,139,137,135,133,131,129,127,125,123,121,
                119,117,115,113,111,109,107,105,103,101,99,97,95,93,91,89,87,85,83,81,
                79,77,75,73,71,69,67,65,63,61,59,57,55,53,51,49,47,45,43,41,
                39,37,35,33,31,29,27,25,23,21,19,17,15,13,11,9,7,5,3,1,
                0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,
                40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,
                80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,
                120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,
                160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,
                200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,
                240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,274,276,278,
                280,282,284,286,288,290,292,294,296,298,300,302,304,306,308,310,312,314,316,318,
                320,322,324,326,328,330,332,334,336,338,340,342,344,346,348,350,352,354,356,358,
                360,362,364,366,368,370,372,374,376,378,380,382,384,386,388,390,392,394,396,398,
                400,402,404,406,408,410,412,414,416,418,420,422,424,426,428,430,432,434,436,438,
                440,442,444,446,448,450,452,454,456,458,460,462,464,466,468,470,472,474,476,478,
                480,482,484,486,488,490,492,494,496,498,500,502,504,506,508,510,512,514,516,518,
                520,522,524,526,528,530,532,534,536,538,540,542,544,546,548,550,552,554,556,558,
                560,562,564,566,568,570,572,574,576,578,580,582,584,586,588,590,592,594,596,598
            ]), "Uint16Array.prototype.sort with long array should be stable");
        }
    },
    {
        name: "TypedArray.prototype.sort with a compare function with side effects",
        body() {
            let xyz = 5;
            function setXyz() { xyz = 10; return 0; }
            Uint8Array.from([]).sort(setXyz);
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when length is 0");
            Uint8Array.from([1]).sort(setXyz)
            assert.areEqual(5, xyz, "Uint8Array.sort does not call compare function when length is 1");
            Float32Array.from([NaN, NaN, NaN, NaN]).sort(setXyz);
            assert.areEqual(5, xyz, "Float32Array.sort does not call compare function when all elements are NaN");
            Float32Array.from([5, NaN, , NaN]).sort(setXyz);
            assert.areEqual(5, xyz, "Float32Array.sort does not call compare function when only one element is defined");
            Float32Array.from([1, 2, NaN]).sort(setXyz);
            assert.areEqual(10, xyz, "Float32Array.sort calls compare function if there is > 1 defined element");
            const arr = Uint8Array.from([1, 0]);
            assert.areEqual(Uint8Array.from([0, 1]), arr.sort((a, b) => {arr[0] = 5; return a - b;}), "Uint8Array.sort protects array from compare function side effects");
            assert.throws(function (){arr.sort(() => {ArrayBuffer.detach(arr.buffer); return -1;})}, TypeError, "Uint8Array.sort throws if compare function detaches buffer");
        }
    },
    {
        name : "TypedArray.prototype.sort with an invalid parameter",
        body() {
            assert.throws(()=>{Uint8Array.prototype.sort.call(Uint8Array.from([]),5);}, TypeError);
            assert.throws(()=>{Uint8Array.prototype.sort.call(5,5);}, TypeError);
            assert.throws(()=>{Uint8Array.prototype.sort.call([]);}, TypeError);
        }
    }
];

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });
