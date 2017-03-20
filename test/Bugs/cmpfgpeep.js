//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function v11() {
  h = 4294967296 >>> 0 == 0 ? 1 : 4294967296;
}
v11();
v11();
v11();
if (h == 1) {
  WScript.Echo("PASSED");
}
else {
  WScript.Echo("FAILED");
}
