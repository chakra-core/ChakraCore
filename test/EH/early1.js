//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function oos() {
  oos();
}

function earlyReturn(num) {
for (var i = 0; i < num; i++) {
  try {
    if (num > 5) {
      oos();
      return;
    }
  }
  catch(e) {
    WScript.Echo("catch " + i);
  }
}
WScript.Echo("done");
}

function earlyBreak(num) {
for (var i = 0; i < num; i++) {
  try {
    if (i > 5) {
     break;
    }
  }
  finally {
    WScript.Echo("finally " + i);
  }
}
WScript.Echo("done");
}

function earlyContinue(num) {
for (var i = 0; i < num; i++) {
  try {
    if (i > 5) {
     continue;
    }
  }
  finally {
    WScript.Echo("finally " + i);
  }
}
WScript.Echo("done");
}

function earlyReturnFromFinally(num)
{

for (var i = 0; i < num; i++) {
  try {
  }
  finally {
    WScript.Echo("finally " + i);
    if (i > 5) {
      return;
    }
  }
}
WScript.Echo("done");
}

function earlyReturnFromCatch(num)
{
for (var i = 0; i < num; i++) {
  try {
    oos();
  }
  catch(e) {
    WScript.Echo("catch " + i);
    if (num > 5) {
      return;
    }
  }
}
WScript.Echo("done");
}

function earlyReturnFromNestedFinally(num)
{
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) return;
    }
    finally{
        WScript.Echo("inner finally " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
WScript.Echo("done");
}

function earlyReturnFromNestedTFTC(num)
{
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) return;
    }
    catch (e){
        WScript.Echo("inner catch " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
WScript.Echo("done");
}

function earlyBreakFromNestedTFTC(num)
{
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) break;
    }
    catch (e){
        WScript.Echo("inner catch " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
WScript.Echo("done");
}

function earlyContinueFromNestedTFTC(num)
{
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) continue;
    }
    catch (e){
        WScript.Echo("inner catch " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
WScript.Echo("done");
}

function earlyBreakLabelFromNestedTFTC(num)
{
outer:for (var x = 0; x < num; x++) {
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) break outer;
    }
    catch (e){
        WScript.Echo("inner catch " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
}
WScript.Echo("done");
}

function earlyContinueLabelFromNestedTFTC(num)
{
outer:for (var x = 0; x < num; x++) {
for (var i = 0; i < num; i++) {
  try {
    try {
      if (num > 5) continue outer;
    }
    catch (e){
        WScript.Echo("inner catch " + i);
    }
  }
  finally {
    WScript.Echo("outer finally " + i);
  }
}
}
WScript.Echo("done");
}

function earlyReturnFromCatchInTryFinally(num)
{
for (var i = 0; i < num; i++) {
try {
  try {
    throw "Err";
  }
  catch(e) {
    WScript.Echo("catch " + i);
    if (num > 5) {
      return;
    }
  }
}
finally {
WScript.Echo("finally " + i);
}
}
WScript.Echo("done");
}

function earlyReturnFromCatchInTryCatchTryFinally(num)
{
for (var i = 0; i < num; i++) {
try {
  try {
    throw "Err";
  }
  catch(e) {
    WScript.Echo("catch " + i);
    if (num > 5) {
      return;
    }
  }
}
finally {
WScript.Echo("finally " + i);
}
}
WScript.Echo("done");
}

function earlyReturnFromFinallyInTryFinally(num)
{
for (var i = 0; i < num; i++) {
try {
  try {
    WScript.Echo("try");
  }
  finally {
    WScript.Echo("inner finally " + i);
    return;
  }
}
finally {
WScript.Echo("outer finally " + i);
}
}
WScript.Echo("done");
}

function earlyReturnFromCatchInInfiniteLoop(num)
{
while (true) {
try {
  try {
    throw "Err";
  }
  catch(e) {
    WScript.Echo("infinite loop catch");
    if (num > 5) {
      return;
    }
  }
}
finally {
WScript.Echo("infinite loop finally");
}
}
WScript.Echo("done");
}

function test0() {
  earlyReturn(7);
  WScript.Echo("Done earlyReturn");
  earlyBreak(7);
  WScript.Echo("Done earlyBreak");
  earlyContinue(7);
  WScript.Echo("Done earlyContinue");
  earlyReturnFromFinally(7);
  WScript.Echo("Done earlyReturnFromFinally");
  earlyReturnFromCatch(7);
  WScript.Echo("Done earlyReturnFromCatch");
  earlyReturnFromNestedFinally(7);
  WScript.Echo("Done earlyReturnFromNestedFinally");
  earlyReturnFromNestedTFTC(7);
  WScript.Echo("Done earlyReturnFromNestedTFTC");
  earlyBreakFromNestedTFTC(7);
  WScript.Echo("Done earlyBreakFromNestedTFTC");
  earlyContinueFromNestedTFTC(7);
  WScript.Echo("Done earlyContinueFromNestedTFTC");
  earlyBreakLabelFromNestedTFTC(7);
  WScript.Echo("Done earlyReturnFromNestedTFTC");
  earlyContinueLabelFromNestedTFTC(7);
  WScript.Echo("Done earlyReturnFromNestedTFTC");
  earlyReturnFromCatchInTryFinally(7);
  WScript.Echo("earlyReturnFromCatchInTryFinally");
  earlyReturnFromCatchInTryCatchTryFinally(7);
  WScript.Echo("earlyReturnFromCatchInTryCatchTryFinally");
  earlyReturnFromFinallyInTryFinally(7);
  WScript.Echo("Done earlyReturnFromFinallyInTryFinally");
  earlyReturn(7);
  earlyReturnFromCatchInInfiniteLoop(7);
  WScript.Echo("earlyReturnFromCatchInInfiniteLoop");
}

test0();
test0();
test0();
