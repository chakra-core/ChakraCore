//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
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
            Limit,
            Any
        };
        bool IsLocalType(WasmTypes::WasmType type);
        uint32 GetTypeByteSize(WasmType type);
        const char16* GetTypeName(WasmType type);
    }

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

    enum WasmOp : byte
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) wb##opname = opcode,
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
        };
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
