//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Js
{
class WebAssemblyModule;
}

namespace Wasm
{
class WasmFunctionInfo;
struct WasmReaderInfo
{
    Field(WasmFunctionInfo*) m_funcInfo;
    Field(Js::WebAssemblyModule*) m_module;
};
}
