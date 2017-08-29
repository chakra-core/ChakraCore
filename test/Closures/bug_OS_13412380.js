//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = 'outside';
var result;

(function() {
  var eval = WScript.LoadScript("", "samethread").eval;

  eval('var x = "inside";');

  result = x;
}());
print("passed");