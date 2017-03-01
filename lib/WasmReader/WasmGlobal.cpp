//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

namespace Wasm
{

    Wasm::WasmConstLitNode WasmGlobal::GetConstInit() const
    {
        if (GetReferenceType() != GlobalReferenceTypes::Const)
        {
            throw WasmCompilationException(_u("Global must be initialized from a const to retrieve the const value"));
        }
        return m_init.cnst;
    }

    uint32 WasmGlobal::GetGlobalIndexInit() const
    {
        if (GetReferenceType() != GlobalReferenceTypes::LocalReference)
        {
            throw WasmCompilationException(_u("Global must be initialized from another global to retrieve its index"));
        }
        return m_init.var.num;
    }

} // namespace Wasm
#endif // ENABLE_WASM
