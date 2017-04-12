//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var GiantPrintArray = [];
var obj0 = {};
var func2 = function () {
  obj0.method0 = func2.caller;
};
var ary = Array();
ary[0] = -285904746.9;
var protoObj0 = Object(obj0);
(function (argMath7) {
  for (var _strvar1 in ary) {
    arguments[1];
    function v0() {
      var v2 = Array();
      var v4 = {};
      func2();
      v4.z = -_strvar1++;
      v2[0] = v4;
      GiantPrintArray.push(v2[0].z);
    }
    v0();
  }
}());
protoObj0.method0();
WScript.Echo(GiantPrintArray);
