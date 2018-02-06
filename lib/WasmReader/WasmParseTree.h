//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    const uint16 EXTENDED_OFFSET = 256;
    namespace Simd {
        const size_t VEC_WIDTH = 4;
        typedef uint32 simdvec [VEC_WIDTH]; //TODO: maybe we should pull in SIMDValue?
        const size_t MAX_LANES = 16;
        void EnsureSimdIsEnabled();
        bool IsEnabled();
    }

    namespace WasmTypes
    {
        enum WasmType
        {
            // based on binary format encoding values
            Void = 0,
            I32 = 1,
            I64 = 2,
            F32 = 3,
            F64 = 4,
#ifdef ENABLE_WASM_SIMD
            M128 = 5,
#endif
            Limit,
            Ptr,
            Any
        };

        extern const char16* const strIds[Limit];

        bool IsLocalType(WasmTypes::WasmType type);
        uint32 GetTypeByteSize(WasmType type);
        const char16* GetTypeName(WasmType type);
    }
    typedef WasmTypes::WasmType Local;

    namespace ExternalKinds
    {
        enum ExternalKind
        {
            Function = 0,
            Table = 1,
            Memory = 2,
            Global = 3,
            Limit
        };
    }

    namespace FunctionIndexTypes
    {
        enum Type
        {
            Invalid = -1,
            ImportThunk,
            Function,
            Import
        };
        bool CanBeExported(Type funcType);
    }

    namespace GlobalReferenceTypes
    {
        enum Type
        {
            Invalid, Const, LocalReference, ImportedReference
        };
    }

    struct WasmOpCodeSignatures
    {
#define WASM_SIGNATURE(id, nTypes, ...) static const WasmTypes::WasmType id[nTypes]; DebugOnly(static const int n##id = nTypes;)
#include "WasmBinaryOpCodes.h"
    };

    enum WasmOp : uint16
    {
#define WASM_OPCODE(opname, opcode, ...) wb##opname = opcode,
// Add prefix to the enum to get a compiler error if there is a collision between operators and prefixes
#define WASM_PREFIX(name, value, ...) prefix##name = value,
#include "WasmBinaryOpCodes.h"
    };

    struct WasmConstLitNode
    {
        union
        {
            float f32;
            double f64;
            int32 i32;
            int64 i64;
            Simd::simdvec v128;
        };
    };

    struct WasmShuffleNode
    {
        uint8 indices[Simd::MAX_LANES];
    };

    struct WasmLaneNode
    {
        uint index;
    };

    struct WasmVarNode
    {
        uint32 num;
    };

    struct WasmMemOpNode
    {
        uint32 offset;
        uint8 alignment;
    };

    struct WasmBrNode
    {
        uint32 depth;
    };

    struct WasmBrTableNode
    {
        uint32 numTargets;
        uint32* targetTable;
        uint32 defaultTarget;
    };

    struct WasmCallNode
    {
        uint32 num; // function id
        FunctionIndexTypes::Type funcType;
    };

    struct WasmBlock
    {
        WasmTypes::WasmType sig;
    };

    struct WasmNode
    {
        WasmOp op;
        union
        {
            WasmBlock block;
            WasmBrNode br;
            WasmBrTableNode brTable;
            WasmCallNode call;
            WasmConstLitNode cnst;
            WasmMemOpNode mem;
            WasmVarNode var;
            WasmLaneNode lane;
            WasmShuffleNode shuffle;
        };
    };

    struct WasmExport
    {
        uint32 index;
        uint32 nameLength;
        const char16* name;
        ExternalKinds::ExternalKind kind;
    };

    struct WasmImport
    {
        ExternalKinds::ExternalKind kind;
        uint32 modNameLen;
        const char16* modName;
        uint32 importNameLen;
        const char16* importName;
    };

    struct CustomSection
    {
        const char16* name;
        charcount_t nameLength;
        const byte* payload;
        uint32 payloadSize;
    };
}
