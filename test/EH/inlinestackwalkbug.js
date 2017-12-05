//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function shapeyConstructor() {
  y = iczqcn;
}

function test1() {
  for (var w in [1,2]) {
    try {
      new shapeyConstructor(w);
    } catch (e) {
    }
  }
}

function throwFunc() {
   // dummy try-catch so that this function does not get inlined
   try {
   }
   catch (ex) {
   }
   throw "ex" ;
}

function caller() {
   throwFunc(w);
}

function test2() {
  for (var w in [1,2]) {
    try {
      new caller();
    } catch (e) {
    }
  }
}

test1();
test2();
WScript.Echo("PASS");


