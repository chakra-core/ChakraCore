//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function write(v) { WScript.Echo(v + ""); }

function foo() {}


var all = [ undefined, null, '', '0',
            true, false, new Boolean(true), new Boolean(false),
            NaN, +0, -0, 0, 1, 10, 10.0, 10.1, -1, -10, -10.0, -10.1,
            Number.MAX_VALUE, Number.MIN_VALUE, Number.NaN, Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY,
            new Number(NaN), new Number(+0), new Number(-0), new Number(0), new Number(1),
            new Number(10), new Number(10.0), new Number(10.1), new Number(-1), 
            new Number(-10), new Number(-10.0), new Number(-10.1),
            new Number(Number.MAX_VALUE), new Number(Number.MIN_VALUE), new Number(Number.NaN), 
            new Number(Number.POSITIVE_INFINITY), new Number(Number.NEGATIVE_INFINITY),
            "", "hello", "hel" + "lo",
            new String(""), new String("hello"), new String("he" + "llo"),
            new Object(), [1,2,3], new Object(), [1,2,3] , foo
          ];
    
for (var i=0; i<all.length; ++i) {
    for (var j=0; j<all.length; ++j) {
        write("all[" + i + "](" + all[i] + ") === all[" + j + "](" + all[j] + ") = " + (all[i] === all[j]));
    }
}

function test(o1, o2)
{
  if(typeof o1 !== typeof o2){
    return false;
  }
  if(o1 === o2){
    return true;
  }
  
  if(typeof o1 == "object"){
    var props = {};
    for(var prop in o1) {
      if (!test(o1[prop], o2[prop])) return false;
      props[prop] = true;
    }
    for(var prop in o2) {
      if (!props[prop]) 
        return false;
    }
    return true;
  }
  else{
    return false
  }
}

print(test({prop1: {}, prop2: "abc", prop3: true}, {prop1: {}, prop2: "abc", prop3: true}));
print(test("abc", "abc"));
print(test({int: 1, string: "xyz"}, {int: 1, string: "xyz"}));
print(test({int: 1, string: "xyz", bool: false}, {int: 1, string: "xyz", float: 1.2}));
print(test({}, {}));
print(test(1,2));
print(test(true, false));
print(test(new Object(), true));