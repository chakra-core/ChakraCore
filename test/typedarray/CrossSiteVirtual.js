//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function foo( type )
{
    var g = WScript.LoadScript("a = new " + type + "(16777216);", "samethread");
    g.a[0] = 0;
    g.a[0];
}

foo("Int8Array");
foo("Uint8Array");
foo("Uint8ClampedArray");
foo("Int16Array");
foo("Uint16Array");
foo("Int32Array");
foo("Uint32Array");
foo("Float32Array");
foo("Float64Array");

WScript.Echo("PASSED");
