//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables appear properly as part of the global object.
function blockScopeTryCatchTestFunc() {
    try {
        throw "Exception!";
    }
    catch (ex) {
        let a = 1;
        const bConst = 2;
        a; /**bp:locals(1)**/
        {
            let c = 3;
            const dConst = 4;
            c; /**bp:locals(1)**/
        }
    }
    return 0;
}
blockScopeTryCatchTestFunc();
WScript.Echo("PASSED");
