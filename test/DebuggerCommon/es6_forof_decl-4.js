//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = 1, y = 2, a = [x, y];

for(let y of a){
    y; /**bp:locals(1)**/
}

WScript.Echo('PASSED');