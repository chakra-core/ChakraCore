//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <intsafe.h>

// Parser Includes
#include "WasmReader.h"

#ifdef ENABLE_WASM
// AsmJsFunctionMemory::RequiredVarConstants
#include "../Language/AsmJsModule.h"
#include "../Language/WAsmJsConstants.h"
#endif

// Runtime includes
#include "../Runtime/runtime.h"
