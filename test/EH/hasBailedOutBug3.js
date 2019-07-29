//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.Echo = function (n) {
  formatOutput(n.toString());
};
function formatOutput(n) {
  return n.replace(/[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?/g, function () {
  });
}
function test0() {
  var GiantPrintArray = [];
  var protoObj0 = {};
  try {
    function func36() {
      try {
      }
       finally {
        WScript.Echo('' + b);
        GiantPrintArray.push(protoObj0);
      }
    }
    func36();
    __loopSecondaryVar3_0;
  } catch (ex) {
    b = ex;
  }
}
test0();
test0();
test0();
print("Passed\n");
