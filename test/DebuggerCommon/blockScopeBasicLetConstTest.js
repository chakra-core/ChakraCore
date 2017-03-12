//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables display properly in locals.
function blockScopeBasicLetConstTestFunc() {
    let integer = 1;
    let string = "2";
    let object = { p1: 3, p2: 4 };
    const constInteger = 5;
    const constString = "6";
    const constObject = { cp1: { cp2: 7, cp3: 8 }, cp4: 9 };
    return 0;// /**bp:locals(2)**/
}
blockScopeBasicLetConstTestFunc.apply({});
WScript.Echo("PASSED");