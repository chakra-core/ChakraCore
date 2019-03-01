//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let desiredLength = 10 * 1000 * 1000;
let threw = false;
const json = `"${'a'.repeat(desiredLength - 3)}",`;
try {
    JSON.parse(json);
} catch(e) {
    threw = true;
}
print(threw ? "Pass" : "Fail");
