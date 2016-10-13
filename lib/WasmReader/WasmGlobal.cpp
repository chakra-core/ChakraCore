//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

WasmTypes::WasmType
WasmGlobal::GetType() const
{
    return m_type;
}

bool
WasmGlobal::GetMutability() const
{
    return m_mutability;
}
} // namespace Wasm
#endif // ENABLE_WASM
