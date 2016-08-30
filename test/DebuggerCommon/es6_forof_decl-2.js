//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var x = 1;

for(let x of [0,1] /**bp:locals()**/ ){
    const z = 1;
    z;/**bp:locals(1)**/
}
WScript.Echo('PASSED');