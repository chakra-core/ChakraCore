//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables appear properly as part of a with block.
function blockScopeWithTestFunc() {
    let o = { p1: 1, p2: "2" };
    {
        let a = 3;
        with (o)
        {
            let b = 4;
            {
                let c = p1.toString();
                const dConst = p2;
                a; /**bp:locals(1)**/
            }
            a; /**bp:locals(1)**/
        }
        a; /**bp:locals(1)**/
    }
    return 0; /**bp:locals(1)**/
}
blockScopeWithTestFunc();
WScript.Echo("PASSED");
