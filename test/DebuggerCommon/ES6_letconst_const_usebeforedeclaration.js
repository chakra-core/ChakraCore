//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Const Use before declaration
*/
const a = 1; /**bp:evaluate('a')**/
WScript.Echo('PASSED');