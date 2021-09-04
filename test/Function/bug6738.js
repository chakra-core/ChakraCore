//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function jit() {
    let x = Math.round.call({}, 3133.7);
}

for (var i = 0; i < 0x1000; i++) {
    jit();
}

print("pass");
