//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var strvar1 = "";
function foo() {
  switch (strvar1) {
  case 1:
    this();
  case "":
  }
}
for (let i = 0; i < 1000; ++i) {
  foo();
}
console.log("pass");
