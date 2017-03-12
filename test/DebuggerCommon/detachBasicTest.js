//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that detaching works after attach.
function detachBasicTest() {
    var a = 0;
    a; /**bp:locals(1)**/
}

WScript.Attach(detachBasicTest);
WScript.Detach(detachBasicTest);
WScript.Echo("PASSED");