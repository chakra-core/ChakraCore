//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables appear properly as part of a switch statement.
function blockScopeSwitchTestFunc() {
    var x = 1;
    for (var i = 0; i < 2; ++i)
    {
        switch (i)
        {
            case 0:
                let b = 1;
                i; /**bp:locals()**/
                break;
            case 1:
                const c = 2;
                i;/**bp:locals()**/
                break;
        }
    }
}

blockScopeSwitchTestFunc();
WScript.Echo("PASSED");
