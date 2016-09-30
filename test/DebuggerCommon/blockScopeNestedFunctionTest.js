//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables show properly in locals at
// inside nested functions.
function blockScopeNestedFunctionTestFunc() {
    var x = -1; /**bp:locals()**/
    let a = 0;
    const aConst = 1;
    a; /**bp:locals()**/
    function innerFunc() {
        /**bp:locals(1)**/
        const bConst = 2;
        let b = 3;
        b; /**bp:locals(1)**/
        function innerInnerFunc() {
            /**bp:locals(1)**/
            let c = 4;
            const cConst = 5;
            a;
            aConst;
            b;
            bConst;
            c; /**bp:locals(1)**/
        }
        innerInnerFunc();
        return 0; /**bp:locals(1)**/
    }
    innerFunc(); 
    return 0; /**bp:locals()**/
}
blockScopeNestedFunctionTestFunc();
WScript.Echo("PASSED");