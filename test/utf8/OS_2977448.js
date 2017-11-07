//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

try {
    // Ensure that character classifier does not incorrectly classify \u2e2f as a letter.
    eval("ⸯ");
} catch (e) {
    if (e instanceof SyntaxError)
    {
        WScript.Echo("PASS");
    }
}
