//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var passed = 1;
passed &= typeof Reflect.enumerate === 'undefined';

var proxy = new Proxy({}, {
  enumerate: function() {
    passed = 0;
  }
});
for(var key in proxy);

var proxy = new Proxy({x:1}, {
  ownKeys: function() {
    return ['a','b'];
  }
});

var keys=""
for(var key in proxy){ keys += key;}
passed &= keys==="ab";

if (passed) {
WScript.Echo("PASS");
}
