//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo(x) {
  var testValue = 100;	
  if (x > 100) {
    testValue = 0;  
  }
  if (testValue != 0) {
    print(testValue);
  }
}

foo(100);
foo(200);
foo(100);
print("Done foo\n");

function bar(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 100;  	    
  }
  if (testValue != 0) {
    print(testValue);
  }
}

bar(200);
bar(0);
bar(200);
print("Done bar\n");

function baz(x) {
  var testVal = {};
  if (x > 100) {
    testVal.fld = 100;
  }
  else {
    testVal.fld = 0;
  }
  if (testVal.fld != 0) {
    print(testVal.fld);
  }
}

baz(200);
baz(0);
baz(200);
print("Done baz\n");

function bargt(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 100;
  }
  if (testValue > 50) {
    print(testValue);
  }
}

bargt(200);
bargt(0);
bargt(200);
print("Done bargt\n");

function barlt(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 100;
  }
  if (testValue < 50) {
    print(testValue);
  }
}

barlt(200);
barlt(0);
barlt(200);
print("Done barlt\n");

function barle(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 100;
  }
  if (testValue <= 50) {
    print(testValue);
  }
}

barle(200);
barle(0);
barle(200);
print("Done barle\n");

function barge(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 100;
  }
  if (testValue >= 50) {
    print(testValue);
  }
}

barge(200);
barge(0);
barge(200);
print("Done barge\n");

function bareq(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 400;
  }
  if (testValue == 400) {
    print(testValue);
  }
}

bareq(200);
bareq(0);
bareq(200);
print("Done bareq\n");

function barnt(x) {
  var testValue;	
  if (x > 100) {
    testValue = 0;  
  }
  else {
    testValue = 400;
  }
  if (!testValue) {
    print(testValue);
  }
}

barnt(200);
barnt(0);
barnt(200);
print("Done barnt\n");

function barntbool(x) {
  var testValue;	
  if (x > 100) {
    testValue = false;
  }
  else {
    testValue = true;
  }
  if (!testValue) {
    print(testValue);
  }
}

barntbool(200);
barntbool(0);
barntbool(200);
print("Done barntbool\n");

function barfalse(x) {
  var testValue;	
  if (x > 100) {
    testValue = false;
  }
  else {
    testValue = true;
  }
  if (testValue) {
    print(testValue);
  }
}

barfalse(200);
barfalse(0);
barfalse(200);
print("Done barfalse\n");

function next(arr, len) {
  if (len > 5) {
    return 0;
  }
  return arr[len];
}

function iterator(x) {
  var arr = [0,1,2,3,4,5,6,7,8,9,10];
  var res = next(arr, x);
  if (res != 0) {
    print(res);
  }
}

iterator(5);
iterator(100);
iterator(5);
print("Done iterator\n");

function next2(arr, len) {
  if (len > 5) {
    return {done:true, value:undefined};
  }
  return {done:false, value:arr[len]};
}

function iterator2(x) {
  var arr = [0,1,2,3,4,5,6,7,8,9,10];
  var res = next2(arr, x);
  if (!res.done) {
    print(res.value);
  }
}

iterator2(5);
iterator2(100);
iterator2(5);
print("Done iterator2\n");
