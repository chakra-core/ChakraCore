//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var count = 0;
function foo() {
  count++;
  if (count == 3) {
    WScript.LoadScript("", "samethread"); // ScriptContext should be created in sourcerundown mode instead of debugging mode
  }
}
foo();
WScript.Attach(foo);
WScript.Detach(foo);
WScript.Attach(foo);
WScript.Echo("pass");
