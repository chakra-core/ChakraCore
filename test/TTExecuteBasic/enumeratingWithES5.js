//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var propName = "d";
var propValue = "dvalue";

function CreateSimpleTypeHandlerObject()
{
  var obj = Object.create(null);
  obj[propName] = propValue;
  return obj;
}

function CreateSimpleDictionaryTypeHandlerObject()
{
  var obj = {};
  obj[propName] = propValue;
  return obj;
}

function CreateDictionaryTypeHandlerObject()
{
  var obj = {};
  Object.defineProperty(obj, propName,
    {
      get : function() {},
      configurable : true,
      enumerable : true
    });

  delete obj[propName];
  obj[propName] = propValue;
  return obj;
}

function TestNonWritable(o)
{
  var beforeTestValue = null;
  var value = 1;
  value = TestEnumerations(o, beforeTestValue, value);

  SetWritable(o, propName, false);
  value = TestEnumerations(o, beforeTestValue, value);

  SetWritable(o, propName, true);
  value = TestEnumerations(o, beforeTestValue, value);

  telemetryLog("Changing writability during enumeration...", true);
  beforeTestValue = function(o, i, value)
  {
    SetWritable(o, propName, false);
    return value;
  };
  value = TestEnumerations(o, beforeTestValue, value);

  beforeTestValue = function(o, i, value)
  {
    SetWritable(o, propName, true);
    return value;
  };
  value = TestEnumerations(o, beforeTestValue, value);

  telemetryLog("Freezing object", true);
  Object.freeze(o);
  beforeTestValue = null;
  value = TestEnumerations(o, beforeTestValue, value);
}

function TestAccessors()
{
  var o = { a:"aValue" };
  DefineAccessor(o, 'b',
    function() { return "GETTER FOR b"; },
    function(v) { telemetryLog("SETTER FOR b", true); }
  );
  o.c = "cValue";  // to be deleted
  o.d = "dValue";

  // Throw in a non-enumerable property
  Object.defineProperty(o, 'e',
    {
      value : "eValue",
      configurable : true,
      writable : true,
      enumerable : false
    });
  DefineAccessor(o, 'f',
    function() { return "GETTER FOR f"; },
    function(v) { telemetryLog("SETTER FOR f", true); }
  );
  o.g = "gValue";

  delete o.c;

  var value = 1;
  var beforeTestValue = null;
  value = TestEnumerations(o, beforeTestValue, value);

  DefineAccessor(o, propName);  
  value = TestEnumerations(o, beforeTestValue, value);

  DefineDataProperty(o, propName, value++);
  value = TestEnumerations(o, beforeTestValue, value);

  telemetryLog("Defining accessor property during enumeration...", true);
  beforeTestValue = function(o, i, value)
  {
    if (i === propName) DefineAccessor(o, propName);
    return value;
  };

  value = TestEnumerations(o, beforeTestValue, value);

  telemetryLog("Defining data property during enumeration...", true);
  beforeTestValue = function(o, i, value)
  {
    if (i === propName) DefineDataProperty(o, propName, value);
    return value + 1;
  };
  value = TestEnumerations(o, beforeTestValue, value);
}

function SetWritable(o, p, v)
{
  telemetryLog("Setting writability of " + p + " to " + v, true);
  Object.defineProperty(o, p, { writable : v });
}

function DefineAccessor(o, p, getter, setter)
{
  if (!getter) getter = function() { return "GETTER"; };
  if (!setter) setter = function(v) { telemetryLog("SETTER", true); }
  telemetryLog("Defining accessors for " + p, true);
  Object.defineProperty(o, p,
    {
      get : getter,
      set : setter,
      configurable : true,
      enumerable : true
    });
}

function DefineDataProperty(o, p, v)
{
  telemetryLog("Defining data property " + p + " with value " + v, true);
  Object.defineProperty(o, p,
    {
      value : v,
      writable : true,
      configurable : true,
      enumerable : true
    });  
}

function TestEnumerations(o, beforeTestValue, value)
{
  telemetryLog("Testing for-in enumeration", true);
  for (var i in o)
  {
    if (beforeTestValue) value = beforeTestValue(o, i, value);
    TestValue(o, i, value++);
  }

  telemetryLog("Testing getOwnPropertyNames enumeration", true);
  var names = Object.getOwnPropertyNames(o);
  for (var i = 0; i < names.length; i++)
  {
    if (beforeTestValue) value = beforeTestValue(o, i, value);
    TestValue(o, names[i], value++);
  }

  return value;
}

function TestValue(o, i, value)
{
  telemetryLog(i + ": " + o[i], true);
  telemetryLog("Setting value to " + value, true);
  o[i] = value;
  telemetryLog(i + ": " + o[i], true);
}

var th1 = CreateSimpleTypeHandlerObject();
var th2 = CreateSimpleDictionaryTypeHandlerObject();
var th3 = CreateDictionaryTypeHandlerObject();

WScript.SetTimeout(testFunction, 50);

/////////////////

function testFunction()
{
    telemetryLog("Test 1: Non-writable, simple type handler", true);
    TestNonWritable(th1);

    telemetryLog("Test 2: Non-writable, simple dictionary type handler", true);
    TestNonWritable(th2);

    telemetryLog("Test 3: Non-writable, dictionary type handler", true);
    TestNonWritable(th3);

    telemetryLog("Test 4: Accessors", true);
    TestAccessors();

    emitTTDLog(ttdLogURI);
}