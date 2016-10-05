//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "Backend.h"

#if !ENABLE_OOP_NATIVE_CODEGEN
JITManager JITManager::s_jitManager; // dummy object when OOP JIT disabled
#endif
