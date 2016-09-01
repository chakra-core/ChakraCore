//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
	Basic for in using let
*/

//Win Blue Bug 184592
var a = {x:2, y:1};


for(let y in a){
        
    y; /**bp:locals(1)**/
}

WScript.Echo('PASSED');