//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let set = false;
let a = JSON.parse('["first", null]', function (key, value) {                                    
  if(!set) {
    this[1] = arguments;
    set = true;
  }
  return value;
});
let passed = JSON.stringify(a) === `["first",{"0":"0","1":"first"}]`;

print(passed ? "Pass" : "Fail")