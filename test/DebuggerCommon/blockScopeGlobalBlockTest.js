//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let variables appear properly when nested inside of a global block.
let a = 1;
{
    let b = 2;
    b;
    a == 2; /**bp:locals(1);stack()**/
}

a;/**bp:locals();**/
WScript.Echo("PASSED");