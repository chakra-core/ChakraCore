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

// check ownKeys
var proxy = new Proxy({x:1}, {
  ownKeys: function() {
    return ['a','b'];
  }
});
var keys=""
for(var key in proxy){ keys += key;}
passed &= keys==="";

// check property descriptor
var proxy = new Proxy({b:1,a:2}, {
  ownKeys: function() {
    return ['a','b'];
  }
});
var keys=""
for(var key in proxy){ keys += key;}
passed &= keys==="ab";

// check property descriptor trap
var already_non_enmerable = false;
var proxy = new Proxy({}, {
  ownKeys: function() {
    return ['a','b','a']; // make first a non-enumerable, and second a enumerable, second a won't show up in for-in
  },
  getOwnPropertyDescriptor: function(target, key){
    var enumerable = true;
    if(key === "a" && !already_non_enmerable)
    {
      enumerable=false;
      already_non_enmerable = true;
    }
    return {
      configurable: true,
      enumerable: enumerable, 
      value: 42, 
      writable: true 
    };
  }
});
var keys=""
for(var key in proxy){ keys += key;}
passed &= keys==="b";

if (passed) {
  WScript.Echo("PASS");
}
