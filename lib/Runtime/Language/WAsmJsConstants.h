//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef TEMP_DISABLE_ASMJS

namespace WAsmJs {
    static const int DOUBLE_SLOTS_SPACE = (sizeof(double) / sizeof(Js::Var)); // 2 in x86 and 1 in x64
    static const double FLOAT_SLOTS_SPACE = (sizeof(float) / (double)sizeof(Js::Var)); // 1 in x86 and 0.5 in x64
    static const double INT_SLOTS_SPACE = (sizeof(int) / (double)sizeof(Js::Var)); // 1 in x86 and 0.5 in x64
    static const int SIMD_SLOTS_SPACE = (sizeof(SIMDValue) / sizeof(Js::Var)); // 4 in x86 and 2 in x64
};

#endif
