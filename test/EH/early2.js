//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function earlyReturnTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      if (num > 5) return;
    }finally {
        WScript.Echo("return outer finally");
    }
  }
}

function earlyBreakTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      if (num > 5) break;
    }finally {
        WScript.Echo("break outer finally");
    }
  }
}

function earlyContinueTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      if (i < 3) continue;
    }finally {
      WScript.Echo("continue outer finally " + i);
    }
  }
}

function earlyReturnNestedTFTC(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (num > 5) return;
      }
      catch(e) {
        WScript.Echo("inner catch");
      }
    }finally {
        WScript.Echo("outer finally");
    }
  }
}

function earlyReturnNestedTFTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (num > 5) return;
      }
      finally {
        WScript.Echo("inner finally");
      }
    }finally {
        WScript.Echo("outer finally");
    }
  }
}

function earlyBreakNestedTFTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (num > 5) break;
      }
      finally {
        WScript.Echo("inner finally");
      }
    }finally {
        WScript.Echo("outer finally");
    }
  }
}

function earlyContinueNestedTFTF(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (i > 3) continue;
      }
      finally {
        WScript.Echo("inner finally");
      }
    }finally {
        WScript.Echo("continue outer finally " + i);
    }
  }
}

function earlyBreakNestedTFTC(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (num > 5) break;
      }
      catch(e) {
        WScript.Echo("inner catch");
      }
    }finally {
        WScript.Echo("break outer finally");
    }
  }
}

function earlyContinueNestedTFTC(num) {
  for (var i = 0; i < num; i++) {
    try {
      try {
        if (num > 5) continue;
      }
      catch(e) {
        WScript.Echo("inner catch");
      }
    }finally {
        WScript.Echo("continue outer finally " + i);
    }
  }
}

function test0() {
  earlyReturnTF(7);
  earlyBreakTF(7);
  earlyContinueTF(7);
  earlyReturnNestedTFTC(7);
  earlyReturnNestedTFTF(7);
  earlyBreakNestedTFTF(7);
  earlyContinueNestedTFTF(7);
  earlyReturnNestedTFTC(7);
  earlyBreakNestedTFTC(7);
  earlyContinueNestedTFTC(7);
}

test0();
test0();
test0();
