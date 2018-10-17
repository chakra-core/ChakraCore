//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Wasm {
    class Limits {
        // The limits are agreed upon with other engines for consistency.
        static const uint32 MaxTypes = 1000000;
        static const uint32 MaxFunctions = 1000000;
        static const uint32 MaxImports = 100000;
        static const uint32 MaxExports = 100000;
        static const uint32 MaxGlobals = 1000000;
        static const uint32 MaxDataSegments = 100000;
        static const uint32 MaxElementSegments = 10000000;
        static const uint32 MaxTableSize = DEFAULT_CONFIG_WasmMaxTableSize;

        static const uint32 MaxStringSize = 100000;
        static const uint32 MaxFunctionLocals = 50000;
        static const uint32 MaxFunctionParams = 1000;
        static const uint32 MaxFunctionReturns = 10000; // todo::We need to figure out what is the right limit here
        static const uint32 MaxBrTableElems = 1000000;

        static const uint32 MaxMemoryInitialPages = Js::ArrayBuffer::MaxArrayBufferLength / Js::WebAssembly::PageSize;
        static const uint32 MaxMemoryMaximumPages = 65536;
        static const uint32 MaxModuleSize = 1024 * 1024 * 1024;
        static const uint32 MaxFunctionSize = 7654321;
    public:
        // Use accessors to easily switch to config flags if needed
        static uint32 GetMaxTypes() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxTypes; }
        static uint32 GetMaxFunctions() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxFunctions; }
        static uint32 GetMaxImports() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxImports; }
        static uint32 GetMaxExports() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxExports; }
        static uint32 GetMaxGlobals() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxGlobals; }
        static uint32 GetMaxDataSegments() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxDataSegments; }
        static uint32 GetMaxElementSegments() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxElementSegments; }
        static uint32 GetMaxTableSize() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : CONFIG_FLAG(WasmMaxTableSize); }
        static uint32 GetMaxStringSize() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxStringSize; }
        static uint32 GetMaxFunctionLocals() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxFunctionLocals; }
        static uint32 GetMaxFunctionParams() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxFunctionParams; }
        static uint32 GetMaxFunctionReturns() {
            if (CONFIG_FLAG(WasmMultiValue))
            {
                return CONFIG_FLAG(WasmIgnoreLimits) ? (UINT32_MAX - 1) : MaxFunctionReturns;
            }
            return 1;
        }
        static uint64 GetMaxBrTableElems() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxBrTableElems; }
        static uint32 GetMaxMemoryInitialPages() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxMemoryInitialPages; }
        static uint32 GetMaxMemoryMaximumPages() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxMemoryMaximumPages; }
        static uint32 GetMaxModuleSize() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxModuleSize; }
        static uint32 GetMaxFunctionSize() { return CONFIG_FLAG(WasmIgnoreLimits) ? UINT32_MAX : MaxFunctionSize; }
    };
}
#endif
