//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#define GENERATE_BYTE_CODE_BUFFER_LIBRARY   0x00000001
#define GENERATE_BYTE_CODE_FOR_NATIVE       0x00000002
#define GENERATE_BYTE_CODE_PARSER_STATE     0x00000004
// When the return buffer needs to be allocated while generating bytecode, use one
// of these flags to indicate how to allocate the memory. The absence of both flags
// indicates that no allocation is needed.
#define GENERATE_BYTE_CODE_COTASKMEMALLOC   0x00000008
#define GENERATE_BYTE_CODE_ALLOC_ANEW       0x00000010
