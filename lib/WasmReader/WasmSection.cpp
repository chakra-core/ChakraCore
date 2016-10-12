//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM
namespace Wasm
{
    SectionInfo SectionInfo::All[bSectLimit] = {
#define WASM_SECTION(name, id, flag, precedent) {flag, bSect ## precedent, static_cast<const wchar_t*>(_u(#name)), static_cast<const char*>(id)},
#include "WasmSections.h"
    };
}
#endif
