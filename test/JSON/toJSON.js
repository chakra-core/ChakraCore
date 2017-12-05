//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


var TEST = function(a, b) {
  if (a != b) {
    throw new Error(a + " != " + b);
  }
}

var fnc = function(n) { this.number = n };
fnc.prototype.toJSON = function() {
  return this.number.toString();
}

// test getter - no-enum
{
  var obj = {};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: false });

  obj.a = 1;
  obj.b = 2;
  obj.d = 4;
  TEST('{"a":1,"b":2,"d":4}', JSON.stringify(obj));
}

// test getter - no-enum - delete
{
  var obj = {};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: false, configurable: true });

  obj.a = 1;
  obj.b = 2;
  delete obj['c']
  TEST('{"a":1,"b":2}', JSON.stringify(obj));;
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: false });
  obj.d = 4;
  TEST('{"a":1,"b":2,"d":4}', JSON.stringify(obj));
}

// test getter - enum - delete 'em all
{
  var obj = {};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: true, configurable: true });

  obj.a = 1;
  obj.b = 2;
  delete obj['c']
  TEST('{"a":1,"b":2}', JSON.stringify(obj));;
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: true });
  obj.d = 4;
  TEST('{"c":3,"a":1,"b":2,"d":4}', JSON.stringify(obj));
  obj[9]=19;
  TEST('{"9":19,"c":3,"a":1,"b":2,"d":4}', JSON.stringify(obj));
  delete obj[9]
  TEST('{"c":3,"a":1,"b":2,"d":4}', JSON.stringify(obj));
  delete obj['a']
  TEST('{"c":3,"b":2,"d":4}', JSON.stringify(obj));
}

// test getter
{
  var obj = {a:1, b:2};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: true });

  obj.d = 4;
  TEST('{"a":1,"b":2,"c":3,"d":4}', JSON.stringify(obj));;
}

// test getter - setter
{
  var obj = {a:1, b:2};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: true }); // should print
  Object.defineProperty(obj, "d", { set: () => 4, enumerable: true }); // skips undefined

  obj.e = 5;
  TEST('{"a":1,"b":2,"c":3,"e":5}', JSON.stringify(obj));
}

// test getter with revive
{
  function rev(name, value) {
    if (value == 3) {
      return undefined;
    }
    else if (value == true) {
      return 99;
    }
    return value;
  }
  var obj = {a:1, b:2};
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: true });

  obj.d = 4;
  TEST('{"a":99,"b":2,"d":4}', JSON.stringify( JSON.parse(JSON.stringify(obj), rev) ));
}

// test getter with revive && objectArray
{
  function rev(name, value) {
    if (value == 3) {
      return undefined;
    }
    else if (value == true) {
      return 99;
    }
    return value;
  }
  var obj = {a:1, b:2, q:3};
  obj[9] = 9;
  obj[44] = 44;
  Object.defineProperty(obj, "c", { get: () => 7, enumerable: true });

  obj.d = 4;

  obj = JSON.parse(JSON.stringify(obj), rev);
  TEST('{"9":9,"44":44,"a":99,"b":2,"c":7,"d":4}', JSON.stringify( obj ));
}

// test getter with revive && objectArray && no-enum
{
  function rev(name, value) {
    if (value == 9) {
      return undefined;
    }
    else if (value == true) {
      return 99;
    }
    return value;
  }
  var obj = {a:1, b:2};
  obj[7] = 8;
  obj[9] = 9;
  obj[44] = 44;
  Object.defineProperty(obj, "c", { get: () => 3, enumerable: false });

  obj.d = 4;

  obj = JSON.parse(JSON.stringify(obj), rev);
  TEST('{"7":8,"44":44,"a":99,"b":2,"d":4}', JSON.stringify( obj ));
}

// test - hasObjectArray but no elements in it
{
  var obj = {"a":1};
  obj[0] = 2;
  TEST(JSON.stringify(obj), '{"0":2,"a":1}');
  delete obj[0];
  TEST(JSON.stringify(obj), '{"a":1}');
}

// test internal properties
{
  let obj = {foo: 1};
  let wm = new WeakMap();
  // Add internal WeakMapKeyMap property to obj
  wm.set(obj,obj);
  TEST('{"foo":1}', JSON.stringify(obj));
  Object.defineProperty(obj, "getter", {get: function () { return 2}, enumerable: true, configurable: true});
  TEST('{"foo":1,"getter":2}', JSON.stringify(obj));

  const err = new Error("message");
  TEST('{}', JSON.stringify(err));
}

// test - function prototype new instance
TEST("\"1\"", JSON.stringify(new fnc(1)))

// test - pre-post alter Date toJSON definition
var dateString = JSON.stringify(new Date(0));
TEST("1970", dateString.substr(dateString.indexOf("1970"), 4))

Date.prototype.toJSON = 1;
TEST("{}", JSON.stringify(new Date(0)))

delete Date.prototype.toJSON
TEST("{}", JSON.stringify(new Date(0)))

// test - use from Object prototype
Object.prototype.toJSON = function() { return 2; }
delete fnc.prototype.toJSON;
TEST(2, JSON.stringify(new fnc(1)))

// test - symbol
Object.prototype[Symbol("toJSON")] = function() { return 3; }
TEST(2, JSON.stringify(new fnc(1)))

console.log("PASS")
