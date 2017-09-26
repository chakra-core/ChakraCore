//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const areSame = Date.prototype.toGMTString === Date.prototype.toUTCString;
if (areSame) {
    console.log("PASS");
} else {
    console.log("FAIL");
}
