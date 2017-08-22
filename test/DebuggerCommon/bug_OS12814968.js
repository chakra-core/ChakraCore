//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// repro flags: -forcedeferparse

var obj = {
  func : function () { }
};

WScript.DumpFunctionPosition(obj.func);
console.log("PASS");
