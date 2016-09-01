//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables properly enter and leave locals display
// when a block is entered/exited.
function blockScopeBasicScopingTestFunc() {
    var a = 0;

    // Out of locals.
    a;/**bp:locals()**/
    {
        let integer = 1;
        let string = "2";
        let object = { p1: 3, p2: 4 };
        const constInteger = 5;
        const constString = "6";
        const constObject = { cp1: { cp2: 7, cp3: 8 }, cp4: 9 };

        // In locals.
        integer; /**bp:locals()**/
    }
    
    // Out of locals again.
    a;/**bp:locals()**/
    return 0;
}
blockScopeBasicScopingTestFunc();
WScript.Echo("PASSED");