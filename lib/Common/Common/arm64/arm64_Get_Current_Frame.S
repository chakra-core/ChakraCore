//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

NESTED_ENTRY arm64_GET_CURRENT_FRAME, _TEXT, NoHandler

    mov     x0,x29
    br      lr

NESTED_END arm64_GET_CURRENT_FRAME
