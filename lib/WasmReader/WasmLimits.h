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
        static const uint32 MaxBrTableElems = 1000000;

        static const uint32 MaxMemoryInitialPages = 16384;
        static const uint32 MaxMemoryMaximumPages = 65536;
        static const uint32 MaxModuleSize = 1024 * 1024 * 1024;
        static const uint32 MaxFunctionSize = 128 * 1024;
    public:
        // Use accessors to easily switch to config flags if needed
        static uint32 GetMaxTypes() { return MaxTypes; }
        static uint32 GetMaxFunctions() { return MaxFunctions; }
        static uint32 GetMaxImports() { return MaxImports; }
        static uint32 GetMaxExports() { return MaxExports; }
        static uint32 GetMaxGlobals() { return MaxGlobals; }
        static uint32 GetMaxDataSegments() { return MaxDataSegments; }
        static uint32 GetMaxElementSegments() { return MaxElementSegments; }
        static uint32 GetMaxTableSize() { return CONFIG_FLAG(WasmMaxTableSize); }
        static uint32 GetMaxStringSize() { return MaxStringSize; }
        static uint32 GetMaxFunctionLocals() { return MaxFunctionLocals; }
        static uint32 GetMaxFunctionParams() { return MaxFunctionParams; }
        static uint64 GetMaxBrTableElems() { return MaxBrTableElems; }
        static uint32 GetMaxMemoryInitialPages() { return MaxMemoryInitialPages; }
        static uint32 GetMaxMemoryMaximumPages() { return MaxMemoryMaximumPages; }
        static uint32 GetMaxModuleSize() { return MaxModuleSize; }
        static uint32 GetMaxFunctionSize() { return MaxFunctionSize; }
    };
}
#endif
