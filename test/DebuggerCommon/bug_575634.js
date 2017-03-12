//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var foo = function() {
  var x; /**bp:stack()**/
}

var o = {
  bar: function() {
    this; /**bp:stack()**/
    return 0;
  }
};

foo();
o.bar();

WScript.Echo("PASS");
