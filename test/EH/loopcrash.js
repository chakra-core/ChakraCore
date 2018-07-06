//------------------------------------------------------------------------------------------------------- 
// Copyright (C) Microsoft. All rights reserved. 
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information. 
//------------------------------------------------------------------------------------------------------- 
function test0() { 
  var i32 = new Int32Array(1); 
  { 
    class class0 { 
    } 
    class class8 { 
    } 
    class class17 { 
      static func91(argMath135) { 
        if (new class0() * h) { 
        }  
      } 
      static func94() { 
        return class8.func78; 
      } 
    } 
    for (var _strvar2 in i32) { // jit loop body
      continue; // RemoveUnreachableCode after this
      try { 
      } catch (ex) { 
        class8; // LdSlot;SlotArrayCheck at top of jit loop body
      } 
    } 
  } 
} 
test0(); 
test0(); 
print("Passed"); 
