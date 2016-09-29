//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function func(d0) {
    i1 = ((d0) == (+((((((0x517ddbc)-(0xc7276b6f)+(-0x8000000))>>>(((0x0)))) >= (0x4866034e))))));
    return i1 + d0;
}

var d1 = (1.015625);
func(d1);
func(d1);

func(d1);
func(d1);
func(d1);
func(d1);

runningJITtedCode = true;
func(d1);
func(d1);

WScript.Echo("PASSED");
