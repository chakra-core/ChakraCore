//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
   eval has its own scope
*/

let a = 1;
eval('let a = 1; a++;');   //new scope
WScript.Echo('PASSED'); /**bp:locals(1)**/