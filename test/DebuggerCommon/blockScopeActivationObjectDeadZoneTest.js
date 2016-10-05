//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables display properly when leaving a dead zone in the activation object case.
function blockScopeActivationObjectDeadZoneTest() {
    var a = 0; /**bp:locals()**/
    let b = 1; /**bp:locals()**/
    const c = 2; /**bp:locals()**/
    eval(""); /**bp:locals()**/
}

blockScopeActivationObjectDeadZoneTest();
WScript.Echo("PASSED");