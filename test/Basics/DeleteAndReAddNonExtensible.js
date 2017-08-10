//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

var kNumProperties = 32;

var o = {};
for (var i = 0; i < kNumProperties; ++i)
o['a' + i] = i;

Object.preventExtensions(o); // IsNotExtensibleSupported && !this->VerifyIsExtensible

for (var i = 0; i < kNumProperties; ++i)
delete o['a' + i];

for (var i = 0; i < 0x21; ++i)
o['a0'] = 1; // calling TryUndeleteProperty again again

WScript.Echo('pass');
