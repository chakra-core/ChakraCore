//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

let desiredLength = 10 * 1000 * 1000;
//desiredLength -= 1;
const json = `"${'a'.repeat(desiredLength - 3)}",`; // "aaa...aaa",
print(json.length);
JSON.parse(json);
print('finished');
