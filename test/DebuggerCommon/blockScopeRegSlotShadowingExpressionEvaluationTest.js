//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let shadowing works for reg slot variables in the debugger evaluation.
// Bug 209657.

function Run() {
    function verify() { }

    function level1Func() {
        let a = 'level1';      
        a += 'level1';      
        try {
            throw 'level2';
        } catch (e) {
            let a = 'level2';          
            a += 'level2';                 
        }

        /**bp:evaluate('a')**/ 
    }

    level1Func();
}

Run();
WScript.Echo("PASSED");