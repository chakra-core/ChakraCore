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

var keys=""
var proxy = new Proxy({x:1,y:2}, {});
for(var key in proxy){ keys += key;}
passed &= keys==="xy";

// check ownKeys
var keys=""
var proxy = new Proxy({"5":1}, {
  ownKeys: function() {
    return ['a', {y:2}, 5, 'b', Symbol.iterator];
  }
});
try{
  for(var key in proxy);
  passed = false;
}
catch(e){}

// check property descriptor
var keys=""
var proxy = new Proxy({b:1,a:2}, {
  ownKeys: function() {
    return ['a', {y:2}, 5, 'b', Symbol.iterator];
  }
});
try{
  for(var key in proxy);
  passed = false;
}
catch(e){}

var keys=""
var proxy = new Proxy({b:1,a:2}, {
  ownKeys: function() {
    return new Proxy(['a', 'b'],{});
  }
});
for(var key in proxy){ keys += key;}
passed &= keys==="ab";

var keys=""
var proxy = new Proxy({c:1,d:2}, {
  ownKeys: function() {
    return new Proxy(['a', 'b'],{
      get(target, propKey, receiver){
        return Reflect.get(['c', 'd'], propKey, receiver);
      }
    });
  }
});
for(var key in proxy){ keys += key;}
passed &= keys==="cd";

var keys=""
var proxy = new Proxy({b:1,a:2}, {
  ownKeys: function() {
    return {x:1,y:2, '0':'a'};
  }
});
for(var key in proxy){ keys += key;}
passed &= keys==="";

var keys=""
var proxy = new Proxy({b:1,a:2}, {
  ownKeys: function() {
    return {x:1,y:2, '0':'a', length:2};
  }
});
for(var key in proxy){ keys += key;}
passed &= keys==="a";

// check property descriptor trap
var keys=""
var already_non_enmerable = false;
var getPrototypeOfCalled = 0;
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
  },
  getPrototypeOf: function(){
    getPrototypeOfCalled++;
    return null;
  }
});
for(var key in proxy){ keys += key;}
passed &= keys==="b";
passed &= getPrototypeOfCalled===1;

// cross site marshal
var IntArr0 = Array();
IntArr0[3] = -6869070206472090000;
IntArr0 = new Proxy(IntArr0, {});
var sc1 = WScript.LoadScript('', 'samethread');
sc1.IntArr0 = IntArr0;
var sc1_cctx = sc1.Debug.parseFunction('x=""; for (var _strvar0 in IntArr0) {x+=IntArr0[_strvar0];}');
sc1_cctx();
passed &= (sc1.x=="-6869070206472090000");


if (passed) {
  WScript.Echo("PASS");
}
