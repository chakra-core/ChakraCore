//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function attach(f) {
  (function (r) {
    WScript.Attach(r);
  })(f);
}

async function mainTest(notAttachCall) {
    if (notAttachCall) {
        for (let i = 0; i < 1; ++i) {
            await attach(mainTest);
        }
    } else {
        var i = 10;/**bp:locals()**/
    }
}
mainTest(true);
WScript.Echo("PASSED");