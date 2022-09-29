//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2022 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const array = new Uint32Array([3, 1, 2]);

// May not throw; See https://tc39.es/ecma262/#sec-%typedarray%.prototype.sort
array.sort(undefined);

print("pass");
