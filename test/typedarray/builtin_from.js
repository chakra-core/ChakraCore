//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var a = "yy?x¼$w    5?åoê?»'i?ºE-N¼ë6_»\\ d";
if (Uint8Array.from(a, c=>c.charCodeAt(0)) + "" == "121,121,63,120,253,36,119,32,32,32,32,53,63,253,111,253,63,253,39,105,63,253,69,45,78,253,253,54,95,253,92,32,100")
{
    print("pass");
}
