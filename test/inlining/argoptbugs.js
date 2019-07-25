//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

{
    let protoObj0 = {};
    let arrObj0 = {};
    let func0 = function () {
    protoObj0.prop2;
    };
    let func1 = function (argMath1 = arguments[argMath0]) {
    func0();
    };

    let func5 = function() {
    func1(arrObj0);
    }
    let func3 = function() {
    func5();
    };
    let func4 = function() {
    return func3();
    };
    func4();
    func4();
}
{
    function f0() {
        return f0.caller;
      };
      function f1() {
        return arguments[f0()];
      };
      function f2() {
        f1();
      }
      function f3() {
        f2();
      };
      function f4() {
        f3();
      };
      f4()
      f4()      
}
{
    function func1() {
        Math.tan('') >= 0;
      };
      function func3() {
        func1();
      };
      function v36() {
        func3();
      }
      function v41() {
        v36();
      }
      v41();
      v41();
}
{
    let a = [];
    function func2() {
    arguments[a[1]];
    };
    function func3() {
    func2(1);
    };
    function v36() {
    func3();
    }
    function v41() {
    v36();
    }
    v41();
    v41();
}
{
    var func1 = function () {
        Math.tan('') >= 0;
      };
      var func2 = function () {
        arguments[ui8[1]];
      };
      var func3 = function () {
        func1(func2(1));
      };
      var ui8 = new Uint8Array(256);
      function v36() {
        func3();
      }
      function v41() {
        var v42 = v36();
      }
      v41();
      v41();
      v41();
      v41();
}
{
    var loopInvariant = 6;
    var obj0 = {};
    var func0 = function () {
        var __loopvar1 = loopInvariant;
        function func5() {
        }
        func0.caller;
    };
    var func1 = function () {
        arguments;
        func0(/\d|\W*|(?![b7])|.*|\W/m);
        func0(/\d|\W*|(?![b7])|.*|\W/m);
        func0(/\d|\W*|(?![b7])|.*|\W/m);
        func0(/\d|\W*|(?![b7])|.*|\W/m);
    };
    var func3 = function () {
        func1();
    };
    obj0.method0 = func3;
    obj0.method1 = obj0.method0;
    obj0.method1();
}
{
    var obj1 = {};
    var arrObj0 = {};
    var func0 = function () {
    function func5() {
    }
    func5();
    };
    var func1 = function () {
    return func0();
    };
    var func2 = function () {
    return (func1() == '') >= 0 ? func1() : 0;
    };
    obj1.method1 = func2;
    arrObj0.method1 = obj1.method1;
    var uniqobj2 = arrObj0.method1();
    var uniqobj3 = [arrObj0];
    var uniqobj4 = uniqobj3[0];
    uniqobj4.method1();
}
{
    var obj0 = {};
    var obj1 = {};
    var func2 = function (argMath7 = ui8) {
      arguments;
    };
    var func3 = function () {
      func2(obj1);
    };
    obj0.method0 = func3;
    obj0.method1 = obj0.method0;
    var ui16 = new Uint16Array(256);
    function v36() {
      obj0.method1();
    }
    function v41() {
      var v42 = v36();
    }
    v41();
    v41();
    v41();
    v41();
}
{
    var f2 = function () {
    return arguments;
    };
    function v7() {
    f2();
    }
    var func2 = function () {
    i32[807574099];
    v7();
    };
    var func3 = function () {
    func2();
    };
    var i32 = new Int32Array();
    func3()
    func3()
    func3()
    func3()
}
{
    var shouldBailout = false;
    var obj0 = {};
    var obj1 = {};
    var func2 = function () {
    if (shouldBailout) {
        return '';
    }
    for (var _i in arguments[obj1]) {
    }
    };
    var func4 = function () {
    func2();
    return 4294967295 ? obj0 : obj1;
    };
    var ary = Array();
    ary[0] = 3;
    func4()
    func4()
    func4()
    func4()
    func4()
}

print("Pass")
