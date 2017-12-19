//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#ifdef _M_AMD64
#include "amd64.h"
#endif

#ifdef _M_ARM
#include "arm.h"
#endif

#ifdef _M_ARM64
#include "arm64.h"
#endif

#ifndef GET_CURRENT_FRAME_ID
#if defined(_M_IX86)
#if defined(_M_HYBRID_X86_ARM64)
__MACHINEARM64(unsigned __int64 __getReg(int))
#define GET_CURRENT_FRAME_ID(f) \
    (f = (void*)__getReg(29))
#else
#define GET_CURRENT_FRAME_ID(f) \
    __asm { mov f, ebp }
#endif
#elif defined(_M_X64)
#ifdef _WIN32
#define GET_CURRENT_FRAME_ID(f) \
    (f = _ReturnAddress())
#else
#define GET_CURRENT_FRAME_ID(f) \
    (f = __builtin_frame_address(0))
#endif
#elif defined(_M_ARM)
// ARM, like x86, uses the frame pointer rather than code address
#define GET_CURRENT_FRAME_ID(f) \
    (f = arm_GET_CURRENT_FRAME())
#elif defined(_M_ARM64)
#define GET_CURRENT_FRAME_ID(f) \
    (f = arm64_GET_CURRENT_FRAME())
#else
#define GET_CURRENT_FRAME_ID(f) \
    Js::Throw::NotImplemented()
#endif
#endif

