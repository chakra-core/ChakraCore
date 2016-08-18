//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


var a = Wasm.instantiateModule(readbuffer('address.wasm'), {test: {print: function(a){print(a); return 2;}}});
a.exports.good(0);
