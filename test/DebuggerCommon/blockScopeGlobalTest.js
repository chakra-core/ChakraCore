//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Tests that let/const variables appear properly as part of the global object.
// (SimpleDictionaryTypeHandler)
let gA = 1;
const gConstB = 2;

function blockScopeGlobalTestFunc() {
    return 0;/**bp:locals(1);**/
}
blockScopeGlobalTestFunc();

// Also test that global properties and global let/const variables appear properly when shadowing occurs.
// (DictionaryTypeHandler)
this.gC = 3;
this.gD = 4;
let gC = 5;
const gD = 6;
blockScopeGlobalTestFunc();

WScript.Echo("PASSED");
