//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const now = WScript.monotonicNow;

// Using <= instead of < because output number precision might be lost
if (now() <= now() && now() <= now()) {
    print("pass");
} else {
    print("fail");
}
