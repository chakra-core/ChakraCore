//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
   Strict mode, eval has its own scope
*/


function Run(){
    "use strict";
    let a = 1;
    eval('let a = 1; a++;');   //new scope
    WScript.Echo(a); /**bp:locals(1);evaluate('a')**/
}


WScript.Attach(Run);