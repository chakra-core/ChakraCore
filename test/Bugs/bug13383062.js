//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function test0() {
    function leaf() {
    }
    class BaseClass {
    }
    var obj0 = {};
    var obj1 = {};
    var arrObj0 = {};
    var litObj1 = {};
    var func0 = function () {
    };
    var func1 = function () {
        var func5 = function* () {
            strvar1 = '!';
        };
        for (var _strvar3 of func5()) {
        }
    };
    var func2 = function () {
    };
    var func3 = function () {
    };
    var func4 = function () {
    };
    obj0.method0 = func2;
    obj0.method1 = func3;
    obj1.method1 = func0;
    var ary = Array();
    var i8 = new Int8Array();
    var i16 = new Int16Array();
    var i32 = new Int32Array();
    var f32 = new Float32Array();
    var IntArr0 = [];
    var VarArr0 = [];
    var b = -2;
    var d = 217;
    var f = -354058415.9;
    var g = 192563783;
    var h = -6607978441461540000;
    var strvar4 = '!';
    var strvar5 = '-';
    var strvar6 = '!$EUI';
    var strvar7 = '#qÀÈ\xA9';
    var protoObj0 = Object(obj0);
    var protoObj1 = Object(obj1);
    class class1 extends BaseClass {
    }
    class class4 extends BaseClass {
    }
    class class7 {
        static set func56(argMath66) {
            while ((argMath66 == h && 217 != obj0.prop5) * (((argMath66 <<= argMath66)))) {
                print("loop 1");
            }
        }
    }
    var __loopvar1000 = 2;
    for (;;) {
        __loopvar1000 -= 2;
        if (__loopvar1000 <= 2 - 6) {
            break;
        }
        print('loop 2');
        protoObj1.method1((class7.func56 = {
            valueOf: function () {
                WScript.Echo("class7.func56.valueOf");
            }
        }, class7.func56 = arrObj0));
    }
}
test0();
// command: D:\ReducerArena\chakrafs\fs\Builds\ChakraFull\unreleased\rs3\1708\00015.24208_170818.1733_michhol_4adec5c86c80a442c3cd8c1fcb25e51243115e8c\bin\x86_debug\20170818175152\jshost.exe -PrintSystemException  -es6generators -es7asyncawait  -maxinterpretcount:1 -maxsimplejitruncount:1 -MinMemOpCount:0 -werexceptionsupport  -force:atom -force:fixdataprops -force:rejit -off:cachedScope -off:stackargopt -off:BoundCheckHoist -off:trackintusage -off:ArrayCheckHoist -force:ScriptFunctionWithInlineCache -off:DelayCapture -oopjit- -ForceArrayBTree -off:fefixedmethods -off:aggressiveinttypespec -off:LoopCountBasedBoundCheckHoist -off:lossyinttypespec -off:checkthis -off:ParallelParse -ValidateInlineStack -off:ArrayLengthHoist step1267.js
// exitcode: 0
// stdout:
// false
// false
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// stderr:
// 
// 
// 
// command: D:\ReducerArena\chakrafs\fs\Builds\ChakraFull\unreleased\rs3\1708\00015.24208_170818.1733_michhol_4adec5c86c80a442c3cd8c1fcb25e51243115e8c\bin\x86_debug\20170818175152\jshost.exe -nonative -werexceptionsupport  -PrintSystemException  -es6generators -es7asyncawait step1267.js
// exitcode: 0
// stdout:
// false
// false
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// stderr:
// 
// 
// 
