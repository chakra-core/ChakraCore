//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------


// Print sensible output for crashing when Error.toString is overwritten
// See https://github.com/chakra-core/ChakraCore/issues/6567 for more detail

ReferenceError.prototype.toString = function ( ) { print("CALLED toString"); return { toString () { return "hi"}};}
a.b();
