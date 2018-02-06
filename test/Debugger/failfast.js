//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function empty() { }
WScript.Attach(empty)
WScript.Detach(empty)

for (var i = 0; i < 10; i++)
{
    function emptyn() { }
    WScript.Attach(emptyn)
    WScript.Detach(emptyn)
}

print("PASS")